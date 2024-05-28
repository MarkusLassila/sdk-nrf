/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Generated by re2c 3.0
 *
 * re2c at_match.re -o at_match.c -W --no-debug-info --tags
 */

#include "at_token.h"

struct at_token at_match_cmd(const char *at, const char **remainder)
{
	const char *cursor = at;
	const char *marker = NULL;

	/*!local:re2c
		re2c:define:YYCTYPE = "unsigned char";
		re2c:define:YYCURSOR = cursor;
		re2c:define:YYMARKER = marker;
		re2c:yyfill:enable = 0;

		AT_CASE_INS = [Aa][Tt];
		CMD = [#%+][A-Za-z0-9]+;

		cmd_test             = AT_CASE_INS CMD "=?";
		cmd_read             = AT_CASE_INS CMD "?";
		cmd_set              = AT_CASE_INS CMD "=";
		cmd_set_no_subparams = AT_CASE_INS CMD?;
		notif                = CMD ":";

		*                    { return (struct at_token){ .type = AT_TOKEN_TYPE_INVALID }; }

		cmd_test
		{
			if (remainder) *remainder = cursor;
			return (struct at_token){
				.start = at, .len = cursor - at - 2,
				.type = AT_TOKEN_TYPE_CMD_TEST
			};
		}

		cmd_read
		{
			if (remainder) *remainder = cursor;
			return (struct at_token){
				.start = at, .len = cursor - at - 1,
				.type = AT_TOKEN_TYPE_CMD_READ
			};
		}

		cmd_set
		{
			if (remainder) *remainder = cursor;
			return (struct at_token){
				.start = at, .len = cursor - at - 1,
				.type = AT_TOKEN_TYPE_CMD_SET
			};
		}

		cmd_set_no_subparams
		{
			if (remainder) *remainder = cursor;
			return (struct at_token){
				.start = at, .len = cursor - at,
				.type = AT_TOKEN_TYPE_CMD_SET
			};
		}

		notif
		{
			if (remainder) *remainder = cursor;
			return (struct at_token){
				.start = at, .len = cursor - at - 1,
				.type = AT_TOKEN_TYPE_NOTIF
			};
		}
	*/
}

struct at_token at_match_subparam(const char *at, const char **remainder)
{
	const char *cursor = at;
	const char *marker = NULL;
	const char *t1;
	const char *t2;

	/*!stags:re2c format = 'const char *@@;\n'; */

	/*!local:re2c
		re2c:define:YYCTYPE = "unsigned char";
		re2c:define:YYCURSOR = cursor;
		re2c:define:YYMARKER = marker;
		re2c:yyfill:enable = 0;

		SPACE = " ";
		INT = "0"|[+\-]?[1-9][0-9]*;
		COMMA = ",";
		QUOTE = "\"";
		CRLF = "\r\n";
		QUOTED_STR = QUOTE @t1 [^"\x00]* @t2 QUOTE;
		OPEN_PAR = "(";
		CLOSE_PAR = ")";
		ARRAY = OPEN_PAR [^()\x00]+ CLOSE_PAR;

		int         = SPACE? @t1 INT @t2 COMMA?;
		quoted_str  = SPACE? QUOTED_STR COMMA?;
		array       = SPACE? @t1 ARRAY @t2 COMMA?;
		empty       = SPACE? @t1 @t2 COMMA;

		*           { return (struct at_token){ .type = AT_TOKEN_TYPE_INVALID }; }

		int
		{
			char last = *(cursor - 1);

			if (remainder) *remainder = cursor;
			return (struct at_token){
				.start = t1, .len = t2 - t1,
				.type = AT_TOKEN_TYPE_INT,
				.variant = last == ',' ? AT_TOKEN_VARIANT_COMMA :
							 AT_TOKEN_VARIANT_NO_COMMA
			};
		}

		quoted_str
		{
			char last = *(cursor - 1);

			if (remainder) *remainder = cursor;
			return (struct at_token){
				.start = t1, .len = t2 - t1,
				.type = AT_TOKEN_TYPE_QUOTED_STRING,
				.variant = last == ',' ? AT_TOKEN_VARIANT_COMMA :
							 AT_TOKEN_VARIANT_NO_COMMA
			};
		}

		array
		{
			char last = *(cursor - 1);

			if (remainder) *remainder = cursor;
			return (struct at_token){
				.start = t1, .len = t2 - t1,
				.type = AT_TOKEN_TYPE_ARRAY,
				.variant = last == ',' ? AT_TOKEN_VARIANT_COMMA :
							 AT_TOKEN_VARIANT_NO_COMMA
			};
		}

		empty
		{
			if (remainder) *remainder = cursor;
			return (struct at_token){
				.start = t1, .len = t2 - t1,
				.type = AT_TOKEN_TYPE_EMPTY, .variant = AT_TOKEN_VARIANT_COMMA
			};
		}
	*/
}

struct at_token at_match_str(const char *at, const char **remainder)
{
	const char *cursor = at;
	const char *marker = NULL;
	const char *t1;
	const char *t2;

	/*!stags:re2c format = 'const char *@@;\n'; */

	/*!local:re2c
		re2c:define:YYCTYPE = "unsigned char";
		re2c:define:YYCURSOR = cursor;
		re2c:define:YYMARKER = marker;
		re2c:yyfill:enable = 0;

		SPACE = " ";
		CRLF = "\r\n";
		STR = [A-Za-z0-9][A-Za-z_\-.0-9 ]*;

		str         = SPACE? @t1 STR @t2 CRLF?;

		*           { return (struct at_token){ .type = AT_TOKEN_TYPE_INVALID }; }

		str
		{
			if (remainder) *remainder = t2;
			return (struct at_token){
				.start = t1, .len = t2 - t1,
				.type = AT_TOKEN_TYPE_STRING,
			};
		}
	*/
}
