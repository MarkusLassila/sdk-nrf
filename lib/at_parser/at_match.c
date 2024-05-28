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

	{
		unsigned char yych;
		unsigned int yyaccept = 0;
		yych = *cursor;
		switch (yych) {
		case '#':
		case '%':
		case '+':
			goto yy3;
		case 'A':
		case 'a':
			goto yy4;
		default:
			goto yy1;
		}
yy1:
		++cursor;
yy2: {
	return (struct at_token){.type = AT_TOKEN_TYPE_INVALID};
}
yy3:
		yyaccept = 0;
		yych = *(marker = ++cursor);
		switch (yych) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G':
		case 'H':
		case 'I':
		case 'J':
		case 'K':
		case 'L':
		case 'M':
		case 'N':
		case 'O':
		case 'P':
		case 'Q':
		case 'R':
		case 'S':
		case 'T':
		case 'U':
		case 'V':
		case 'W':
		case 'X':
		case 'Y':
		case 'Z':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':
		case 'i':
		case 'j':
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'o':
		case 'p':
		case 'q':
		case 'r':
		case 's':
		case 't':
		case 'u':
		case 'v':
		case 'w':
		case 'x':
		case 'y':
		case 'z':
			goto yy5;
		default:
			goto yy2;
		}
yy4:
		yych = *++cursor;
		switch (yych) {
		case 'T':
		case 't':
			goto yy7;
		default:
			goto yy2;
		}
yy5:
		yych = *++cursor;
		switch (yych) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G':
		case 'H':
		case 'I':
		case 'J':
		case 'K':
		case 'L':
		case 'M':
		case 'N':
		case 'O':
		case 'P':
		case 'Q':
		case 'R':
		case 'S':
		case 'T':
		case 'U':
		case 'V':
		case 'W':
		case 'X':
		case 'Y':
		case 'Z':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':
		case 'i':
		case 'j':
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'o':
		case 'p':
		case 'q':
		case 'r':
		case 's':
		case 't':
		case 'u':
		case 'v':
		case 'w':
		case 'x':
		case 'y':
		case 'z':
			goto yy5;
		case ':':
			goto yy9;
		default:
			goto yy6;
		}
yy6:
		cursor = marker;
		if (yyaccept == 0) {
			goto yy2;
		} else {
			goto yy8;
		}
yy7:
		yyaccept = 1;
		yych = *(marker = ++cursor);
		switch (yych) {
		case '#':
		case '%':
		case '+':
			goto yy10;
		default:
			goto yy8;
		}
yy8: {
	if (remainder) {
		*remainder = cursor;
	}
	return (struct at_token){.start = at, .len = cursor - at, .type = AT_TOKEN_TYPE_CMD_SET};
}
yy9:
		++cursor;
		{
			if (remainder) {
				*remainder = cursor;
			}
			return (struct at_token){
				.start = at, .len = cursor - at - 1, .type = AT_TOKEN_TYPE_NOTIF};
		}
yy10:
		yych = *++cursor;
		switch (yych) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G':
		case 'H':
		case 'I':
		case 'J':
		case 'K':
		case 'L':
		case 'M':
		case 'N':
		case 'O':
		case 'P':
		case 'Q':
		case 'R':
		case 'S':
		case 'T':
		case 'U':
		case 'V':
		case 'W':
		case 'X':
		case 'Y':
		case 'Z':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':
		case 'i':
		case 'j':
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'o':
		case 'p':
		case 'q':
		case 'r':
		case 's':
		case 't':
		case 'u':
		case 'v':
		case 'w':
		case 'x':
		case 'y':
		case 'z':
			goto yy11;
		default:
			goto yy6;
		}
yy11:
		yych = *++cursor;
		switch (yych) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G':
		case 'H':
		case 'I':
		case 'J':
		case 'K':
		case 'L':
		case 'M':
		case 'N':
		case 'O':
		case 'P':
		case 'Q':
		case 'R':
		case 'S':
		case 'T':
		case 'U':
		case 'V':
		case 'W':
		case 'X':
		case 'Y':
		case 'Z':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':
		case 'i':
		case 'j':
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'o':
		case 'p':
		case 'q':
		case 'r':
		case 's':
		case 't':
		case 'u':
		case 'v':
		case 'w':
		case 'x':
		case 'y':
		case 'z':
			goto yy11;
		case '=':
			goto yy12;
		case '?':
			goto yy14;
		default:
			goto yy8;
		}
yy12:
		yych = *++cursor;
		switch (yych) {
		case '?':
			goto yy15;
		default:
			goto yy13;
		}
yy13: {
	if (remainder) {
		*remainder = cursor;
	}
	return (struct at_token){
		.start = at, .len = cursor - at - 1, .type = AT_TOKEN_TYPE_CMD_SET};
}
yy14:
		++cursor;
		{
			if (remainder) {
				*remainder = cursor;
			}
			return (struct at_token){.start = at,
						 .len = cursor - at - 1,
						 .type = AT_TOKEN_TYPE_CMD_READ};
		}
yy15:
		++cursor;
		{
			if (remainder) {
				*remainder = cursor;
			}
			return (struct at_token){.start = at,
						 .len = cursor - at - 2,
						 .type = AT_TOKEN_TYPE_CMD_TEST};
		}
	}
}

struct at_token at_match_subparam(const char *at, const char **remainder)
{
	const char *cursor = at;
	const char *marker = NULL;
	const char *t1;
	const char *t2;

	const char *yyt1;
	const char *yyt2;

	{
		unsigned char yych;
		yych = *cursor;
		switch (yych) {
		case ' ':
			goto yy19;
		case '"':
			goto yy20;
		case '(':
			yyt2 = cursor;
			goto yy21;
		case '+':
		case '-':
			yyt2 = cursor;
			goto yy22;
		case ',':
			goto yy23;
		case '0':
			yyt2 = cursor;
			goto yy24;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			yyt2 = cursor;
			goto yy26;
		default:
			goto yy17;
		}
yy17:
		++cursor;
yy18: {
	return (struct at_token){.type = AT_TOKEN_TYPE_INVALID};
}
yy19:
		yych = *(marker = ++cursor);
		switch (yych) {
		case '"':
			goto yy27;
		case '(':
			yyt2 = cursor;
			goto yy29;
		case '+':
		case '-':
			yyt2 = cursor;
			goto yy30;
		case ',':
			goto yy23;
		case '0':
			yyt2 = cursor;
			goto yy24;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			yyt2 = cursor;
			goto yy26;
		default:
			goto yy18;
		}
yy20:
		yych = *(marker = ++cursor);
		switch (yych) {
		case 0x00:
			goto yy18;
		case '"':
			yyt1 = yyt2 = cursor;
			goto yy32;
		default:
			yyt1 = cursor;
			goto yy31;
		}
yy21:
		yych = *(marker = ++cursor);
		switch (yych) {
		case 0x00:
		case '(':
		case ')':
			goto yy18;
		default:
			goto yy34;
		}
yy22:
		yych = *++cursor;
		switch (yych) {
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			goto yy26;
		default:
			goto yy18;
		}
yy23:
		++cursor;
		t1 = cursor - 1;
		t2 = cursor - 1;
		{
			if (remainder) {
				*remainder = cursor;
			}
			return (struct at_token){.start = t1,
						 .len = t2 - t1,
						 .type = AT_TOKEN_TYPE_EMPTY,
						 .variant = AT_TOKEN_VARIANT_COMMA};
		}
yy24:
		yych = *++cursor;
		yyt1 = cursor;
		switch (yych) {
		case ',':
			goto yy36;
		default:
			goto yy25;
		}
yy25:
		t1 = yyt2;
		t2 = yyt1;
		{
			char last = *(cursor - 1);

			if (remainder) {
				*remainder = cursor;
			}
			return (struct at_token){.start = t1,
						 .len = t2 - t1,
						 .type = AT_TOKEN_TYPE_INT,
						 .variant = last == ','
								    ? AT_TOKEN_VARIANT_COMMA
								    : AT_TOKEN_VARIANT_NO_COMMA};
		}
yy26:
		yych = *++cursor;
		switch (yych) {
		case ',':
			yyt1 = cursor;
			goto yy36;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			goto yy26;
		default:
			yyt1 = cursor;
			goto yy25;
		}
yy27:
		yych = *++cursor;
		switch (yych) {
		case 0x00:
			goto yy28;
		case '"':
			yyt1 = yyt2 = cursor;
			goto yy32;
		default:
			yyt1 = cursor;
			goto yy31;
		}
yy28:
		cursor = marker;
		goto yy18;
yy29:
		yych = *++cursor;
		switch (yych) {
		case ')':
			goto yy28;
		default:
			goto yy35;
		}
yy30:
		yych = *++cursor;
		switch (yych) {
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			goto yy26;
		default:
			goto yy28;
		}
yy31:
		yych = *++cursor;
		switch (yych) {
		case 0x00:
			goto yy28;
		case '"':
			yyt2 = cursor;
			goto yy32;
		default:
			goto yy31;
		}
yy32:
		yych = *++cursor;
		switch (yych) {
		case ',':
			goto yy37;
		default:
			goto yy33;
		}
yy33:
		t1 = yyt1;
		t2 = yyt2;
		{
			char last = *(cursor - 1);

			if (remainder) {
				*remainder = cursor;
			}
			return (struct at_token){.start = t1,
						 .len = t2 - t1,
						 .type = AT_TOKEN_TYPE_QUOTED_STRING,
						 .variant = last == ','
								    ? AT_TOKEN_VARIANT_COMMA
								    : AT_TOKEN_VARIANT_NO_COMMA};
		}
yy34:
		yych = *++cursor;
yy35:
		switch (yych) {
		case 0x00:
		case '(':
			goto yy28;
		case ')':
			goto yy38;
		default:
			goto yy34;
		}
yy36:
		++cursor;
		goto yy25;
yy37:
		++cursor;
		goto yy33;
yy38:
		yych = *++cursor;
		yyt1 = cursor;
		switch (yych) {
		case ',':
			goto yy40;
		default:
			goto yy39;
		}
yy39:
		t1 = yyt2;
		t2 = yyt1;
		{
			char last = *(cursor - 1);

			if (remainder) {
				*remainder = cursor;
			}
			return (struct at_token){.start = t1,
						 .len = t2 - t1,
						 .type = AT_TOKEN_TYPE_ARRAY,
						 .variant = last == ','
								    ? AT_TOKEN_VARIANT_COMMA
								    : AT_TOKEN_VARIANT_NO_COMMA};
		}
yy40:
		++cursor;
		goto yy39;
	}
}

struct at_token at_match_str(const char *at, const char **remainder)
{
	const char *cursor = at;
	const char *marker = NULL;
	const char *t1;
	const char *t2;

	const char *yyt1;
	const char *yyt2;

	{
		unsigned char yych;
		yych = *cursor;
		switch (yych) {
		case ' ':
			goto yy44;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G':
		case 'H':
		case 'I':
		case 'J':
		case 'K':
		case 'L':
		case 'M':
		case 'N':
		case 'O':
		case 'P':
		case 'Q':
		case 'R':
		case 'S':
		case 'T':
		case 'U':
		case 'V':
		case 'W':
		case 'X':
		case 'Y':
		case 'Z':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':
		case 'i':
		case 'j':
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'o':
		case 'p':
		case 'q':
		case 'r':
		case 's':
		case 't':
		case 'u':
		case 'v':
		case 'w':
		case 'x':
		case 'y':
		case 'z':
			yyt1 = cursor;
			goto yy45;
		default:
			goto yy42;
		}
yy42:
		++cursor;
yy43: {
	return (struct at_token){.type = AT_TOKEN_TYPE_INVALID};
}
yy44:
		yych = *++cursor;
		switch (yych) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G':
		case 'H':
		case 'I':
		case 'J':
		case 'K':
		case 'L':
		case 'M':
		case 'N':
		case 'O':
		case 'P':
		case 'Q':
		case 'R':
		case 'S':
		case 'T':
		case 'U':
		case 'V':
		case 'W':
		case 'X':
		case 'Y':
		case 'Z':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':
		case 'i':
		case 'j':
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'o':
		case 'p':
		case 'q':
		case 'r':
		case 's':
		case 't':
		case 'u':
		case 'v':
		case 'w':
		case 'x':
		case 'y':
		case 'z':
			yyt1 = cursor;
			goto yy45;
		default:
			goto yy43;
		}
yy45:
		yych = *(marker = ++cursor);
		switch (yych) {
		case '\r':
			yyt2 = cursor;
			goto yy47;
		case ' ':
		case '-':
		case '.':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G':
		case 'H':
		case 'I':
		case 'J':
		case 'K':
		case 'L':
		case 'M':
		case 'N':
		case 'O':
		case 'P':
		case 'Q':
		case 'R':
		case 'S':
		case 'T':
		case 'U':
		case 'V':
		case 'W':
		case 'X':
		case 'Y':
		case 'Z':
		case '_':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':
		case 'i':
		case 'j':
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'o':
		case 'p':
		case 'q':
		case 'r':
		case 's':
		case 't':
		case 'u':
		case 'v':
		case 'w':
		case 'x':
		case 'y':
		case 'z':
			goto yy45;
		default:
			yyt2 = cursor;
			goto yy46;
		}
yy46:
		t1 = yyt1;
		t2 = yyt2;
		{
			if (remainder) {
				*remainder = t2;
			}
			return (struct at_token){
				.start = t1,
				.len = t2 - t1,
				.type = AT_TOKEN_TYPE_STRING,
			};
		}
yy47:
		yych = *++cursor;
		switch (yych) {
		case '\n':
			goto yy49;
		default:
			goto yy48;
		}
yy48:
		cursor = marker;
		yyt2 = cursor;
		goto yy46;
yy49:
		++cursor;
		goto yy46;
	}
}
