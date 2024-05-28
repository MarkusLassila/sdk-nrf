/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef AT_TOKEN_H__
#define AT_TOKEN_H__

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

enum at_token_type {
	AT_TOKEN_TYPE_INVALID,
	AT_TOKEN_TYPE_CMD_TEST,
	AT_TOKEN_TYPE_CMD_READ,
	AT_TOKEN_TYPE_CMD_SET,
	AT_TOKEN_TYPE_NOTIF,
	AT_TOKEN_TYPE_INT,
	AT_TOKEN_TYPE_QUOTED_STRING,
	AT_TOKEN_TYPE_ARRAY,
	AT_TOKEN_TYPE_EMPTY,
	AT_TOKEN_TYPE_STRING,
	AT_TOKEN_TYPE_RESP
};

enum at_token_variant {
	AT_TOKEN_VARIANT_NO_COMMA,
	AT_TOKEN_VARIANT_COMMA
};

struct at_token {
	const char *start;
	size_t len;
	enum at_token_type type;
	enum at_token_variant variant;
};

#ifdef __cplusplus
}
#endif

#endif /* AT_TOKEN_H__ */
