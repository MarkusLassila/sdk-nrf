/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <modem/at_parser.h>

#include "at_token.h"
#include "at_match.h"

enum at_num_type {
	AT_NUM_TYPE_INT16,
	AT_NUM_TYPE_UINT16,
	AT_NUM_TYPE_INT32,
	AT_NUM_TYPE_UINT32,
	AT_NUM_TYPE_INT64,
	AT_NUM_TYPE_UINT64,
};

static bool is_crlf(const char *str)
{
	return strlen(str) == 2 && str[0] == '\r' && str[1] == '\n';
}

static bool is_subparam(const struct at_token *token)
{
	switch (token->type) {
	case AT_TOKEN_TYPE_INT:
	case AT_TOKEN_TYPE_QUOTED_STRING:
	case AT_TOKEN_TYPE_ARRAY:
	case AT_TOKEN_TYPE_EMPTY:
		return true;
	default:
		return false;
	}
}

static bool is_resp(const char *str)
{
	static const char * const resp[] = {
		"\r\nOK\r\n",
		"\r\nERROR\r\n",
		"\r\n+CME ERROR",
		"\r\n+CMS ERROR"
	};

	for (size_t i = 0; i < ARRAY_SIZE(resp); i++) {
		if (strncmp(str, resp[i], strlen(resp[i])) == 0) return true;
	}

	return false;
}

static bool is_index_ahead(struct at_parser *parser, size_t index)
{
	return index + 1 > parser->count;
}

static bool lookahead_crlf_or_more(const char *str)
{
	return strlen(str) >= 2 && str[0] == '\r' && str[1] == '\n';
}

static bool lookahead_crlf_and_more(const char *str)
{
	return strlen(str) > 2 && str[0] == '\r' && str[1] == '\n';
}

static void at_token_set_empty(struct at_token *token, const char *start)
{
	token->start = start;
	token->len = 0;
	token->type = AT_TOKEN_TYPE_EMPTY;
	token->variant = AT_TOKEN_VARIANT_NO_COMMA;
}

static int at_parser_check(struct at_parser *parser)
{
	if (!parser) {
		return -EINVAL;
	}

	if (!parser->is_init) {
		return -EPERM;
	}

	if (!parser->at || !parser->cursor) {
		return -EFAULT;
	}

	return 0;
}

static int at_parser_tok(struct at_parser *parser, struct at_token *token)
{
	const char *remainder = NULL;
	size_t cursor_len = 0;
	size_t remainder_len = 0;

	cursor_len = strlen(parser->cursor);

	/* The lexer cannot match empty strings, so intercept the special case where the empty
	 * subparameter is the one after the previously parsed token.
	 * This case is detected in the previous call to this function.
	 */
	if (parser->is_next_empty) {
		at_token_set_empty(token, parser->cursor);

		/* Set the remainder to the current cursor because an empty token has length
		 * zero.
		 */
		remainder = parser->cursor;

		/* Reset the flag. */
		parser->is_next_empty = false;

		goto matched;
	}

	/* Nothing left to tokenize. */
	if (cursor_len == 0 || is_crlf(parser->cursor)) {
		return -EIO;
	}

	/* Don't tokenize response. */
	if (is_resp(parser->cursor)) {
		return -EIO;
	}

	/* Check if there is a new line to parse. */
	if (lookahead_crlf_and_more(parser->cursor)) {
		return -EAGAIN;
	}

	if (parser->count == 0) {
		*token = at_match_cmd(parser->cursor, &remainder);
	} else {
		*token = at_match_subparam(parser->cursor, &remainder);
	}

	if (token->type == AT_TOKEN_TYPE_INVALID) {
		/* If the token is neither a valid command nor a valid subparameter, check if it's a
		 * non-quoted string.
		 */
		*token = at_match_str(parser->cursor, &remainder);
		if (token->type == AT_TOKEN_TYPE_INVALID) {
			return -EBADMSG;
		}
	}

matched:
	remainder_len = strlen(remainder);

	if (is_subparam(token)) {
		/* if the token is a subparameter but does not have a trailing comma then it is the
		 * last subparameter of the line.
		 * If there is more to parse, then the token must be followed by CRLF, otherwise the
		 * string is malformed.
		 */
		if (token->variant != AT_TOKEN_VARIANT_COMMA) {
			if (remainder_len > 0 && !lookahead_crlf_or_more(remainder)) {
				return -EBADMSG;
			}
		} else {
			/* The next token is of type empty if and only if the current token has a
			 * trailing comma and the remaining string either is empty or starts with
			 * CRLF.
			 */
			if (remainder_len == 0 || lookahead_crlf_or_more(remainder)) {
				parser->is_next_empty = true;
			}
		}
	}

	parser->count++;
	parser->cursor = remainder;

	return 0;
}

static int at_parser_seek(struct at_parser *parser, size_t index, struct at_token *token)
{
	int err;

	if (!is_index_ahead(parser, index)) {
		/* Rewind parser. */
		parser->cursor = parser->at;
		parser->count = 0;
	}

	do {
		err = at_parser_tok(parser, token);
	} while (is_index_ahead(parser, index) && !err);

	return err;
}

int at_parser_init(struct at_parser *parser, const char *at)
{
	if (!parser || !at) {
		return -EINVAL;
	}

	size_t len = strlen(at);

	/* Check bounds.
	 * Minimum length of 2 characters ("AT").
	 * Maximum legnth of 65535 characters.
	 */
	if (len < 2 || len > UINT16_MAX) {
		return -EINVAL;
	}

	memset(parser, 0, sizeof(struct at_parser));

	parser->at = at;
	parser->cursor = at;
	parser->is_init = true;

	return 0;
}

int at_parser_next(struct at_parser *parser)
{
	int err;
	struct at_token token = {0};

	err = at_parser_check(parser);
	if (err) {
		return err;
	}

	do {
		err = at_parser_tok(parser, &token);
	} while (!err);

	if (err != -EAGAIN) {
		return err;
	}

	/* Skip CRLF. */
	parser->cursor += 2;
	/* Reset count. */
	parser->count = 0;
	/* Set pointer of current AT command string to the current cursor, which points to the
	 * beginning of a new AT command line.
	 */
	parser->at = parser->cursor;

	return 0;
}

int at_parser_cmd_get(struct at_parser *parser, char **at)
{
	int err;

	if (!at) {
		return -EINVAL;
	}

	err = at_parser_check(parser);
	if (err) {
		return err;
	}

	*at = (char *)parser->at;

	return 0;
}

int at_parser_cmd_type_get(struct at_parser *parser, enum at_parser_cmd_type *type)
{
	int err;
	struct at_token tok;

	if (!type) {
		return -EINVAL;
	}

	err = at_parser_check(parser);
	if (err) {
		return err;
	}

	tok = at_match_cmd(parser->at, NULL);
	switch (tok.type) {
	case AT_TOKEN_TYPE_CMD_TEST:
		*type = AT_PARSER_CMD_TYPE_TEST;
		break;
	case AT_TOKEN_TYPE_CMD_READ:
		*type = AT_PARSER_CMD_TYPE_READ;
		break;
	case AT_TOKEN_TYPE_CMD_SET:
		*type = AT_PARSER_CMD_TYPE_SET;
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

int at_parser_cmd_count_get(struct at_parser *parser, size_t *count)
{
	int err;
	struct at_token token = {0};

	if (!count) {
		return -EINVAL;
	}

	err = at_parser_check(parser);
	if (err) {
		return err;
	}

	do {
		err = at_parser_tok(parser, &token);
	} while (!err);

	*count = parser->count;

	return err == -EIO ? 0 : err;
}

static int at_parser_num_get_impl(struct at_parser *parser, size_t index, void *value,
				  enum at_num_type type) {
	int err;
	struct at_token token = {0};

	if (!value) {
		return -EINVAL;
	}

	err = at_parser_check(parser);
	if (err) {
		return err;
	}

	err = at_parser_seek(parser, index, &token);
	if (err) {
		return err;
	}

	if (token.type != AT_TOKEN_TYPE_INT) {
		return -EOPNOTSUPP;
	}

	errno = 0;
	char *next = NULL;

	/* Check unsigned long long first, using its own parsing function. */
	if (type == AT_NUM_TYPE_UINT64) {
		uint64_t val = strtoull(token.start, &next, 10);
		if (errno == ERANGE && (val == 0 || val == UINT64_MAX)) {
			return -ERANGE;
		}

		*(uint64_t *)value = val;

		return 0;
	}

	/* Use long long parsing for everything else. */
	int64_t val = strtoll(token.start, &next, 10);
	if (errno == ERANGE && (val == INT64_MIN || val == INT64_MAX)) {
		return -ERANGE;
	}

	switch (type) {
	case AT_NUM_TYPE_INT16:
		if (val < INT16_MIN || val > INT16_MAX) return -ERANGE;
		*(int16_t *)value = (int16_t)val;
		break;
	case AT_NUM_TYPE_UINT16:
		if (val < 0 || val > UINT16_MAX) return -ERANGE;
		*(uint16_t *)value = (uint16_t)val;
		break;
	case AT_NUM_TYPE_INT32:
		if (val < INT32_MIN || val > INT32_MAX) return -ERANGE;
		*(int32_t *)value = (int32_t)val;
		break;
	case AT_NUM_TYPE_UINT32:
		if (val < 0 || val > UINT32_MAX) return -ERANGE;
		*(uint32_t *)value = (uint32_t)val;
		break;
	case AT_NUM_TYPE_INT64:
		*(int64_t *)value = val;
		break;
	default:
	}

	return 0;
}

int at_parser_int16_get(struct at_parser *parser, size_t index, int16_t *value) {
	return at_parser_num_get_impl(parser, index, value, AT_NUM_TYPE_INT16);
}

int at_parser_uint16_get(struct at_parser *parser, size_t index, uint16_t *value) {
	return at_parser_num_get_impl(parser, index, value, AT_NUM_TYPE_UINT16);
}

int at_parser_int32_get(struct at_parser *parser, size_t index, int32_t *value) {
	return at_parser_num_get_impl(parser, index, value, AT_NUM_TYPE_INT32);
}

int at_parser_uint32_get(struct at_parser *parser, size_t index, uint32_t *value) {
	return at_parser_num_get_impl(parser, index, value, AT_NUM_TYPE_UINT32);
}

int at_parser_int64_get(struct at_parser *parser, size_t index, int64_t *value) {
	return at_parser_num_get_impl(parser, index, value, AT_NUM_TYPE_INT64);
}

int at_parser_uint64_get(struct at_parser *parser, size_t index, uint64_t *value) {
	return at_parser_num_get_impl(parser, index, value, AT_NUM_TYPE_UINT64);
}

int at_parser_string_get(struct at_parser *parser, size_t index, char *str, size_t *len)
{
	int err;
	struct at_token token = {0};

	if (!str || !len) {
		return -EINVAL;
	}

	err = at_parser_check(parser);
	if (err) {
		return err;
	}

	err = at_parser_seek(parser, index, &token);
	if (err) {
		return err;
	}

	switch (token.type) {
	/* Acceptable types. */
	case AT_TOKEN_TYPE_CMD_TEST:
	case AT_TOKEN_TYPE_CMD_SET:
	case AT_TOKEN_TYPE_CMD_READ:
	case AT_TOKEN_TYPE_NOTIF:
	case AT_TOKEN_TYPE_QUOTED_STRING:
	case AT_TOKEN_TYPE_STRING:
	case AT_TOKEN_TYPE_ARRAY:
	case AT_TOKEN_TYPE_RESP:
		break;
	default:
		return -EOPNOTSUPP;
	}

	/* Check if there is enough memory for null terminator. */
	if (*len < token.len + 1) {
		return -ENOMEM;
	}

	memcpy(str, token.start, token.len);

	/* Null-terminate the output string. */
	str[token.len] = '\0';

	/* Update the length to reflect the copied string length. */
	*len = token.len;

	return 0;
}
