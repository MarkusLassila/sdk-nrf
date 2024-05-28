/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef AT_PARSER_H__
#define AT_PARSER_H__

#include <stdbool.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

enum at_parser_cmd_type {
	AT_PARSER_CMD_TYPE_UNKNOWN,
	AT_PARSER_CMD_TYPE_SET,
	AT_PARSER_CMD_TYPE_READ,
	AT_PARSER_CMD_TYPE_TEST
};

struct at_parser {
	const char *at;
	const char *cursor;
	size_t count;
	bool is_next_empty;
	bool is_init;
};

#define at_parser_num_get(parser, index, value) \
	_Generic((value), \
		int16_t*: at_parser_int16_get, \
		uint16_t*: at_parser_uint16_get, \
		int32_t*: at_parser_int32_get, \
		uint32_t*: at_parser_uint32_get, \
		int64_t*: at_parser_int64_get, \
		uint64_t*: at_parser_uint64_get \
	)(parser, index, value)

int at_parser_init(struct at_parser *parser, const char *at);
int at_parser_next(struct at_parser *parser);
int at_parser_cmd_get(struct at_parser *parser, char **at);
int at_parser_cmd_type_get(struct at_parser *parser, enum at_parser_cmd_type *type);
int at_parser_cmd_count_get(struct at_parser *parser, size_t *count);
int at_parser_int16_get(struct at_parser *parser, size_t index, int16_t *value);
int at_parser_uint16_get(struct at_parser *parser, size_t index, uint16_t *value);
int at_parser_int32_get(struct at_parser *parser, size_t index, int32_t *value);
int at_parser_uint32_get(struct at_parser *parser, size_t index, uint32_t *value);
int at_parser_int64_get(struct at_parser *parser, size_t index, int64_t *value);
int at_parser_uint64_get(struct at_parser *parser, size_t index, uint64_t *value);
int at_parser_string_get(struct at_parser *parser, size_t index, char *str, size_t *len);

#ifdef __cplusplus
}
#endif

#endif /* AT_PARSER_H__ */
