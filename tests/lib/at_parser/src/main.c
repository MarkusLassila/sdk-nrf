/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <modem/at_parser.h>

static const char * const singleline[] = {
	"+CEREG: 2,\"76C1\",\"0102DA04\", 7\r\n+CME ERROR: 10\r\n",
	"+CEREG: 2,\"76C1\",\"0102DA04\", 7\r\nOK\r\n",
	"+CEREG: 2,\"76C1\",\"0102DA04\", 7\r\n"
};

static const char * const multiline[] = {
	"+CGEQOSRDP: 0,0,,\r\n"
	"+CGEQOSRDP: 1,2,,\r\n"
	"+CGEQOSRDP: 2,4,,,1,65280000\r\n",
	"+CGEQOSRDP: 0,0,,\r\n"
	"+CGEQOSRDP: 1,2,,\r\n"
	"+CGEQOSRDP: 2,4,,,1,65280000\r\nOK\r\n"
	"+CGEQOSRDP: 0,0,,\r\n"
	"+CGEQOSRDP: 1,2,,\r\n"
	"+CGEQOSRDP: 2,4,,,1,65280000\r\nERROR\r\n"
};

static const char * const pduline[] = {
	"+CMT: \"12345678\", 24\r\n"
	"06917429000171040A91747966543100009160402143708006C8329BFD0601\r\n+CME ERROR: 123\r\n",
	"+CMT: \"12345678\", 24\r\n"
	"06917429000171040A91747966543100009160402143708006C8329BFD0601\r\nOK\r\n",
	"+CMT: \"12345678\", 24\r\n"
	"06917429000171040A91747966543100009160402143708006C8329BFD0601\r\n"
};

static const char * const singleparamline[] = {
	"mfw_nrf9160_0.7.0-23.prealpha\r\n+CMS ERROR: 123\r\n",
	"mfw_nrf9160_0.7.0-23.prealpha\r\nOK\r\n",
	"mfw_nrf9160_0.7.0-23.prealpha\r\n"
};

static const char * const emptyparamline[] = {
	"+CPSMS: 1,,,\"10101111\",\"01101100\"\r\n",
	"+CPSMS: 1,,,\"10101111\",\"01101100\"\r\nOK\r\n",
	"+CPSMS: 1,,,\"10101111\",\"01101100\"\r\n+CME ERROR: 123\r\n"
};

static const char * const certificate =
	"%CMNG: 12345678, 0, \"978C...02C4\","
	"\"-----BEGIN CERTIFICATE-----"
	"MIIBc464..."
	"...bW9aAa4"
	"-----END CERTIFICATE-----\"\r\nERROR\r\n";

static void test_params_before(void *fixture)
{
	ARG_UNUSED(fixture);
}

static void test_params_after(void *fixture)
{
	ARG_UNUSED(fixture);
}

ZTEST(at_parser, test_at_parser_quoted_string)
{
	int ret;
	struct at_parser parser = {0};
	char buffer[32];
	uint32_t buffer_len;
	int32_t num;

	const char *str1 = "+TEST:1,\"Hello World!\"\r\n";
	const char *str2 = "%TEST: 1, \"Hello World!\"\r\n";
	const char *str3 = "#TEST:1,\"Hello World!\"\r\n"
			   "+TEST: 2, \"FOOBAR\"\r\n";

	/* String without spaces between parameters (str1)*/
	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("+TEST"), buffer_len);
	zassert_mem_equal("+TEST", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 1);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 2, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("Hello World!"), buffer_len);
	zassert_mem_equal("Hello World!", buffer, buffer_len);

	/* String with spaces between parameters (str2)*/
	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("%TEST"), buffer_len);
	zassert_mem_equal("%TEST", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 1);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 2, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("Hello World!"), buffer_len);
	zassert_mem_equal("Hello World!", buffer, buffer_len);

	/* String with multiple notifications (str3) */
	ret = at_parser_init(&parser, str3);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("#TEST"), buffer_len);
	zassert_mem_equal("#TEST", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 1);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 2, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("Hello World!"), buffer_len);
	zassert_mem_equal("Hello World!", buffer, buffer_len);

	/* Second line. */
	ret = at_parser_next(&parser);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("+TEST"), buffer_len);
	zassert_mem_equal("+TEST", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 2);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 2, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("FOOBAR"), buffer_len);
	zassert_mem_equal("FOOBAR", buffer, buffer_len);
}

ZTEST(at_parser, test_at_parser_empty)
{
	int ret;
	struct at_parser parser = {0};
	char buffer[32];
	uint32_t buffer_len;
	int32_t num;

	const char *str1 = "+TEST: 1,\r\n";
	const char *str2 = "+TEST: ,1\r\n";
	const char *str3 = "+TEST: 1,,\"Hello World!\"";
	const char *str4 = "+TEST: ,,,1\r\n";

	/* Empty parameter at the end of the parameter list */
	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("+TEST"), buffer_len);
	zassert_mem_equal("+TEST", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 1);

	/* Empty parameter at the beginning of the parameter list */
	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("+TEST"), buffer_len);
	zassert_mem_equal("+TEST", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 2, &num);
	zassert_ok(ret);
	zassert_equal(num, 1);

	/* Empty parameter between two other parameter types */
	ret = at_parser_init(&parser, str3);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("+TEST"), buffer_len);
	zassert_mem_equal("+TEST", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 1);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 3, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("Hello World!"), buffer_len);
	zassert_mem_equal("Hello World!", buffer, buffer_len);

	/* 3 empty parameters at the beginning of the parameter list */
	ret = at_parser_init(&parser, str4);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("+TEST"), buffer_len);
	zassert_mem_equal("+TEST", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 4, &num);
	zassert_ok(ret);
	zassert_equal(num, 1);
}

ZTEST(at_parser, test_at_parser_testcases)
{
	int ret;
	struct at_parser parser = {0};
	char buffer[128] = {0};
	int len = sizeof(buffer);
	int32_t num = 0;
	size_t valid_count = 0;

	/* Try to parse the singleline string */
	for (size_t i = 0; i < ARRAY_SIZE(singleline); i++) {
		ret = at_parser_init(&parser, singleline[i]);
		zassert_ok(ret);

		ret = at_parser_cmd_count_get(&parser, &valid_count);
		zassert_ok(ret);
		zassert_equal(valid_count, 5);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 0, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("+CEREG", buffer, len);

		ret = at_parser_num_get(&parser, 1, &num);
		zassert_ok(ret);
		zassert_equal(num, 2);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 2, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("76C1", buffer, len);
	
		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 3, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("0102DA04", buffer, len);

		ret = at_parser_num_get(&parser, 4, &num);
		zassert_ok(ret);
		zassert_equal(num, 7);
	}

	/* Try to parse the multiline string */
	for (size_t i = 0; i < ARRAY_SIZE(multiline); i++) {
		ret = at_parser_init(&parser, multiline[i]);
		zassert_ok(ret);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 0, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("+CGEQOSRDP", buffer, len);

		ret = at_parser_num_get(&parser, 1, &num);
		zassert_ok(ret);
		zassert_equal(num, 0);

		ret = at_parser_num_get(&parser, 2, &num);
		zassert_ok(ret);
		zassert_equal(num, 0);

		/* Second line. */
		ret = at_parser_next(&parser);
		zassert_ok(ret);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 0, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("+CGEQOSRDP", buffer, len);

		ret = at_parser_num_get(&parser, 1, &num);
		zassert_ok(ret);
		zassert_equal(num, 1);

		ret = at_parser_num_get(&parser, 2, &num);
		zassert_ok(ret);
		zassert_equal(num, 2);

		/* Third line. */
		ret = at_parser_next(&parser);
		zassert_ok(ret);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 0, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("+CGEQOSRDP", buffer, len);

		ret = at_parser_num_get(&parser, 1, &num);
		zassert_ok(ret);
		zassert_equal(num, 2);

		ret = at_parser_num_get(&parser, 2, &num);
		zassert_ok(ret);
		zassert_equal(num, 4);

		ret = at_parser_num_get(&parser, 5, &num);
		zassert_ok(ret);
		zassert_equal(num, 1);

		ret = at_parser_num_get(&parser, 6, &num);
		zassert_ok(ret);
		zassert_equal(num, 65280000);
	}

	/* Try to parse the pduline string */
	for (size_t i = 0; i < ARRAY_SIZE(pduline); i++) {
		ret = at_parser_init(&parser, pduline[i]);
		zassert_ok(ret);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 0, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("+CMT", buffer, len);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 1, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("12345678", buffer, len);

		ret = at_parser_num_get(&parser, 2, &num);
		zassert_ok(ret);
		zassert_equal(num, 24);

		/* Second line. */
		ret = at_parser_next(&parser);
		zassert_ok(ret);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 0, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("06917429000171040A91747966543100009160402143708006C8329BFD0601",
				  buffer, len);
	}

	/* Try to parse the singleparamline string */
	for (size_t i = 0; i < ARRAY_SIZE(singleparamline); i++) {
		ret = at_parser_init(&parser, singleparamline[i]);
		zassert_ok(ret);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 0, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("mfw_nrf9160_0.7.0-23.prealpha", buffer, len);
	}

	/* Try to parse the emptyparamline string */
	for (size_t i = 0; i < ARRAY_SIZE(emptyparamline); i++) {
		ret = at_parser_init(&parser, emptyparamline[i]);
		zassert_ok(ret);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 0, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("+CPSMS", buffer, len);

		ret = at_parser_num_get(&parser, 1, &num);
		zassert_ok(ret);
		zassert_equal(num, 1);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 4, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("10101111", buffer, len);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 5, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("01101100", buffer, len);
	}

	/* Try to parse the certificate string */
	ret = at_parser_init(&parser, certificate);
	zassert_ok(ret);

	len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &len);
	zassert_ok(ret);
	zassert_mem_equal("%CMNG", buffer, len);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 12345678);

	ret = at_parser_num_get(&parser, 2, &num);
	zassert_ok(ret);
	zassert_equal(num, 0);

	len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 3, buffer, &len);
	zassert_ok(ret);
	zassert_mem_equal("978C...02C4", buffer, len);

	len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 4, buffer, &len);
	zassert_ok(ret);
	zassert_mem_equal("-----BEGIN CERTIFICATE-----"
			  "MIIBc464..."
			  "...bW9aAa4"
			  "-----END CERTIFICATE-----", buffer, len, "%s", buffer);
}

ZTEST(at_parser, test_at_cmd_type_get_cmd_set)
{
	int ret;
	struct at_parser parser = {0};
	enum at_parser_cmd_type type;
	char buffer[64];
	uint32_t buffer_len;
	int16_t tmpshrt;

	/* CGMI */
	static const char at_cmd_cgmi[] = "AT+CGMI";

	ret = at_parser_init(&parser, at_cmd_cgmi);
	zassert_ok(ret);

	ret = at_parser_cmd_type_get(&parser, &type);
	zassert_ok(ret);
	zassert_equal(type, AT_PARSER_CMD_TYPE_SET);
	
	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("AT+CGMI", buffer, buffer_len);

	/* CCLK */
	static const char at_cmd_cclk[] = "AT+CCLK=\"18/12/06,22:10:00+08\"";

	ret = at_parser_init(&parser, at_cmd_cclk);
	zassert_ok(ret);

	ret = at_parser_cmd_type_get(&parser, &type);
	zassert_ok(ret);
	zassert_equal(type, AT_PARSER_CMD_TYPE_SET);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("AT+CCLK", buffer, buffer_len);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 1, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("18/12/06,22:10:00+08", buffer, buffer_len);

	/* XSYSTEMMODE */
	static const char at_cmd_xsystemmode[] = "AT%XSYSTEMMODE=1,2,3,4";

	ret = at_parser_init(&parser, at_cmd_xsystemmode);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("AT%XSYSTEMMODE", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 1, &tmpshrt);
	zassert_ok(ret);
	zassert_equal(tmpshrt, 1);

	ret = at_parser_num_get(&parser, 2, &tmpshrt);
	zassert_ok(ret);
	zassert_equal(tmpshrt, 2);

	ret = at_parser_num_get(&parser, 3, &tmpshrt);
	zassert_ok(ret);
	zassert_equal(tmpshrt, 3);

	ret = at_parser_num_get(&parser, 4, &tmpshrt);
	zassert_ok(ret);
	zassert_equal(tmpshrt, 4);

	ret = at_parser_cmd_type_get(&parser, &type);
	zassert_ok(ret);
	zassert_equal(type, AT_PARSER_CMD_TYPE_SET);

	/* AT */
	static const char lone_at_cmd[] = "AT";

	ret = at_parser_init(&parser, lone_at_cmd);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("AT", buffer, buffer_len);

	ret = at_parser_cmd_type_get(&parser, &type);
	zassert_ok(ret);
	zassert_equal(type, AT_PARSER_CMD_TYPE_SET);

	/* CLAC */
	static const char at_cmd_clac[] = "AT+CLAC\r\n";

	ret = at_parser_init(&parser, at_cmd_clac);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("AT+CLAC", buffer, buffer_len);

	ret = at_parser_cmd_type_get(&parser, &type);
	zassert_ok(ret);
	zassert_equal(type, AT_PARSER_CMD_TYPE_SET);

	/* CLAC RSP */
	static const char at_clac_rsp[] = "AT+CLAC\r\nAT+COPS\r\nAT%COPS\r\n";

	ret = at_parser_init(&parser, at_clac_rsp);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("AT+CLAC", buffer, buffer_len);

	ret = at_parser_next(&parser);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("AT+COPS", buffer, buffer_len);

	ret = at_parser_next(&parser);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("AT%COPS", buffer, buffer_len);
}

ZTEST(at_parser, test_at_cmd_type_get_cmd_read)
{
	int ret;
	char buffer[64];
	uint32_t buffer_len;
	struct at_parser parser = {0};
	enum at_parser_cmd_type type;

	static const char at_cmd_cfun_read[] = "AT+CFUN?";

	ret = at_parser_init(&parser, at_cmd_cfun_read);
	zassert_ok(ret);

	ret = at_parser_cmd_type_get(&parser, &type);
	zassert_ok(ret);
	zassert_equal(type, AT_PARSER_CMD_TYPE_READ);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("AT+CFUN", buffer, buffer_len);
}

ZTEST(at_parser, test_at_cmd_type_get_cmd_test)
{
	int ret;
	char buffer[64];
	uint32_t buffer_len;
	struct at_parser parser = {0};
	enum at_parser_cmd_type type;

	static const char at_cmd_cfun_test[] = "AT+CFUN=?";

	ret = at_parser_init(&parser, at_cmd_cfun_test);
	zassert_ok(ret);

	ret = at_parser_cmd_type_get(&parser, &type);
	zassert_ok(ret);
	zassert_equal(type, AT_PARSER_CMD_TYPE_TEST);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("AT+CFUN", buffer, buffer_len);
}

ZTEST(at_parser, test_at_parser_init_einval)
{
	int ret;
	struct at_parser parser = { 0 };

	ret = at_parser_init(NULL, NULL);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_init(&parser, NULL);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_init(NULL, "AT+CFUN?");
	zassert_equal(ret, -EINVAL);

	/* String too short. */
	ret = at_parser_init(&parser, "");
	zassert_equal(ret, -EINVAL);

	/* String too large. */
	static char str[UINT16_MAX + 8] = {0};

	for (int i = 0; i < UINT16_MAX + 1; i++) {
		str[i] = 'A';
	}

	zassert_true(strlen(str) > UINT16_MAX);

	ret = at_parser_init(&parser, str);
	zassert_equal(ret, -EINVAL);
}

ZTEST(at_parser, test_at_parser_init)
{
	int ret;
	struct at_parser parser = { 0 };

	const char *str1 = "+NOTIF: 1,2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);
}

// ZTEST(at_parser, test_at_parser_tok_einval)
// {
// 	int ret;
// 	struct at_parser parser = { 0 };
// 	struct at_token tok = { 0 };

// 	ret = at_parser_tok(NULL, NULL);
// 	zassert_equal(ret, -EINVAL);

// 	ret = at_parser_tok(&parser, NULL);
// 	zassert_equal(ret, -EINVAL);

// 	ret = at_parser_tok(NULL, &tok);
// 	zassert_equal(ret, -EINVAL);

// 	const char *str1 = "+TEST: 1,2\r\nOK\r\n";

// 	/* Initialize and then modify the parser's `ptr` cursor. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	parser.cursor = NULL;

// 	ret = at_parser_tok(&parser, &tok);
// 	zassert_equal(ret, -EINVAL);
// }

// ZTEST(at_parser, test_at_parser_tok_eperm)
// {
// 	int ret;
// 	struct at_parser parser = { 0 };
// 	struct at_token tok = { 0 };

// 	/* The parser has not been initialized. */
// 	ret = at_parser_tok(&parser, &tok);
// 	zassert_equal(ret, -EPERM);
// }

// ZTEST(at_parser, test_at_parser_tok_eio)
// {
// 	int ret;
// 	struct at_parser parser = { 0 };
// 	struct at_token tok = { 0 };

// 	const char *str1 = "+NOTIF: 1,2,3\r\nOK\r\n";

// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_tok(&parser, &tok);
// 	zassert_ok(ret);
// 	zassert_equal(tok.type, AT_TOKEN_TYPE_NOTIF);

// 	ret = at_parser_tok(&parser, &tok);
// 	zassert_ok(ret);
// 	zassert_equal(tok.type, AT_TOKEN_TYPE_INT);

// 	ret = at_parser_tok(&parser, &tok);
// 	zassert_ok(ret);
// 	zassert_equal(tok.type, AT_TOKEN_TYPE_INT);

// 	ret = at_parser_tok(&parser, &tok);
// 	zassert_ok(ret);
// 	zassert_equal(tok.type, AT_TOKEN_TYPE_INT);

// 	/* There is nothing left to tokenize (the response is not parsed). */
// 	ret = at_parser_tok(&parser, &tok);
// 	zassert_equal(ret, -EIO);
// }

// ZTEST(at_parser, test_at_parser_tok_invalid_token_ebadmsg)
// {
// 	int ret;
// 	struct at_parser parser = { 0 };
// 	struct at_token tok = { 0 };

// 	/* Invalid token. `#31` is not a valid token. */
// 	const char *str1 = "+NOTIF: 1,#31,1";

// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_tok(&parser, &tok);
// 	zassert_ok(ret);
// 	zassert_equal(tok.type, AT_TOKEN_TYPE_NOTIF);

// 	ret = at_parser_tok(&parser, &tok);
// 	zassert_ok(ret);
// 	zassert_equal(tok.type, AT_TOKEN_TYPE_INT);

// 	ret = at_parser_tok(&parser, &tok);
// 	zassert_equal(ret, -EBADMSG);

// 	/* Unchanged. */
// 	zassert_equal(tok.type, AT_TOKEN_TYPE_INT);

// 	/* Invalid token. ` !+NOTIF: ` is not a valid token. */
// 	const char *str2 = " !+NOTIF: 1,2,3";

// 	ret = at_parser_init(&parser, str2);
// 	zassert_ok(ret);

// 	ret = at_parser_tok(&parser, &tok);
// 	zassert_equal(ret, -EBADMSG);

// 	/* Invalid token. `+!NOTIF: ` is not a valid token. */
// 	const char *str3 = "+!NOTIF: 1,2,3";

// 	ret = at_parser_init(&parser, str3);
// 	zassert_ok(ret);

// 	ret = at_parser_tok(&parser, &tok);
// 	zassert_equal(ret, -EBADMSG);

// 	/* Invalid token. `%!NOTIF: ` is not a valid token. */
// 	const char *str4 = "%!NOTIF: 1,2,3";

// 	ret = at_parser_init(&parser, str4);
// 	zassert_ok(ret);

// 	ret = at_parser_tok(&parser, &tok);
// 	zassert_equal(ret, -EBADMSG);

// 	/* Invalid token. Cannot have null terminator inside quoted string. */
// 	const char *str5 = "+NOTIF: \"\0ABCD\"";

// 	ret = at_parser_init(&parser, str5);
// 	zassert_ok(ret);

// 	ret = at_parser_tok(&parser, &tok);
// 	zassert_ok(ret);
// 	zassert_equal(tok.type, AT_TOKEN_TYPE_NOTIF);

// 	ret = at_parser_tok(&parser, &tok);
// 	zassert_equal(ret, -EBADMSG);

// 	/* Invalid token. Cannot have null terminator inside quoted string. */
// 	const char *str6 = "+NOTIF: \"A\0BCD\"";

// 	ret = at_parser_init(&parser, str6);
// 	zassert_ok(ret);

// 	ret = at_parser_tok(&parser, &tok);
// 	zassert_ok(ret);
// 	zassert_equal(tok.type, AT_TOKEN_TYPE_NOTIF);

// 	ret = at_parser_tok(&parser, &tok);
// 	zassert_equal(ret, -EBADMSG);

// 	/* Invalid token. A non-quoted string cannot start with `-`. */
// 	const char *str7 = "-ABCD";

// 	ret = at_parser_init(&parser, str7);
// 	zassert_ok(ret);

// 	ret = at_parser_tok(&parser, &tok);
// 	zassert_equal(ret, -EBADMSG);
// }

// ZTEST(at_parser, test_at_parser_tok_last_subparameter_ebadmsg)
// {
// 	int ret;
// 	struct at_parser parser = {0};
// 	struct at_token token = {0};

// 	/* 1) "TEST" looks like the last subparameter of the line since it is missing a trailing
// 	 * comma, however it is followed by an integer rather than a CRLF. Thus, the string is
// 	 * malformed.
// 	 */
// 	const char *str1 = "+NOTIF: 1,2,\"TEST\"9,(1,7,2,0),,3,\"TEST2, \",\r\nOK\r\n";

// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_tok(&parser, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_NOTIF);

// 	ret = at_parser_tok(&parser, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_parser_tok(&parser, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_parser_tok(&parser, &token);
// 	zassert_equal(ret, -EBADMSG);

// 	/* Unchanged. */
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	/* 2) "TEST" looks like the last subparameter of the line since it is missing a trailing
// 	 * comma, however it is followed by only a CR rather than a CRLF. Thus, the string is
// 	 * malformed.
// 	 */
// 	const char *str2 = "+NOTIF: 1,2,\"TEST\"\r9,(1,7,2,0),,3,\"TEST2, \",\r\nOK\r\n";

// 	ret = at_parser_init(&parser, str2);
// 	zassert_ok(ret);

// 	ret = at_parser_tok(&parser, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_NOTIF);

// 	ret = at_parser_tok(&parser, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_parser_tok(&parser, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_parser_tok(&parser, &token);
// 	zassert_equal(ret, -EBADMSG);

// 	/* Unchanged. */
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	/* 3) "TEST" looks like the last subparameter of the line since it is missing a trailing
// 	 * comma, however it is followed by only a CR which is also the last character of the
// 	 * string. Thus, the string is malformed.
// 	 */
// 	const char *str3 = "+NOTIF: 1,2,\"TEST\"\r";

// 	ret = at_parser_init(&parser, str3);
// 	zassert_ok(ret);

// 	ret = at_parser_tok(&parser, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_NOTIF);

// 	ret = at_parser_tok(&parser, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_parser_tok(&parser, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_parser_tok(&parser, &token);
// 	zassert_equal(ret, -EBADMSG);

// 	/* Unchanged. */
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	/* 4) The last subparamter of the line is empty, however it is followed only by a CR which
// 	 * is also the last character of the string. Thus, the string is malformed.
// 	 */
// 	const char *str4 = "+NOTIF: 1,2,\r";

// 	ret = at_parser_init(&parser, str4);
// 	zassert_ok(ret);

// 	ret = at_parser_tok(&parser, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_NOTIF);

// 	ret = at_parser_tok(&parser, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_parser_tok(&parser, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_parser_tok(&parser, &token);
// 	zassert_equal(ret, -EBADMSG);

// 	/* Unchanged. */
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);
// }

// ZTEST(at_parser, test_at_parser_tok_long_string)
// {
// 	int ret;
// 	struct at_parser parser = {0};
// 	struct at_token token = {0};
// 	int32_t val;
// 	char buffer[32] = {0};
// 	size_t buffer_len = sizeof(buffer);

// 	const char *str = "+NOTIF: ,178,+21,\"TEST\",-9,,\"TEST2, \",\r\n"
// 			  "#ABBA: 1,2\r\n"
// 			  "%TEST: 1\r\n"
// 			  "OK\r\n";
// 	const char *str_spaces = "+NOTIF: , 178, +21, \"TEST\", -9, ,\"TEST2, \",\r\n"
// 				 "#ABBA:1, 2\r\n"
// 				 "%TEST: 1\r\n"
// 				 "OK\r\n";
// 	const char *strings[2] = {str, str_spaces};

// 	for (int i = 0; i < ARRAY_SIZE(strings); i++) {
// 		ret = at_parser_init(&parser, strings[i]);
// 		zassert_ok(ret);

// 		ret = at_parser_tok(&parser, &token);
// 		zassert_ok(ret);
// 		zassert_equal(token.type, AT_TOKEN_TYPE_NOTIF);

// 		buffer_len = sizeof(buffer);
// 		ret = at_token_string_get(&token, buffer, &buffer_len);
// 		zassert_ok(ret);
// 		zassert_equal(strlen("+NOTIF"), buffer_len);
// 		zassert_mem_equal("+NOTIF", buffer, buffer_len);

// 		ret = at_parser_tok(&parser, &token);
// 		zassert_ok(ret);
// 		zassert_equal(token.type, AT_TOKEN_TYPE_EMPTY);

// 		ret = at_parser_tok(&parser, &token);
// 		zassert_ok(ret);
// 		zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 		ret = at_token_int32_get(&token, &val);
// 		zassert_ok(ret);
// 		zassert_equal(val, 178);

// 		ret = at_parser_tok(&parser, &token);
// 		zassert_ok(ret);
// 		zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 		ret = at_token_int32_get(&token, &val);
// 		zassert_ok(ret);
// 		zassert_equal(val, 21);

// 		ret = at_parser_tok(&parser, &token);
// 		zassert_ok(ret);
// 		zassert_equal(token.type, AT_TOKEN_TYPE_QUOTED_STRING);

// 		buffer_len = sizeof(buffer);
// 		ret = at_token_string_get(&token, buffer, &buffer_len);
// 		zassert_ok(ret);
// 		zassert_equal(strlen("TEST"), buffer_len);
// 		zassert_mem_equal("TEST", buffer, buffer_len);

// 		ret = at_parser_tok(&parser, &token);
// 		zassert_ok(ret);
// 		zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 		ret = at_token_int32_get(&token, &val);
// 		zassert_ok(ret);
// 		zassert_equal(val, -9);

// 		ret = at_parser_tok(&parser, &token);
// 		zassert_ok(ret);
// 		zassert_equal(token.type, AT_TOKEN_TYPE_EMPTY);

// 		ret = at_parser_tok(&parser, &token);
// 		zassert_ok(ret);
// 		zassert_equal(token.type, AT_TOKEN_TYPE_QUOTED_STRING);

// 		buffer_len = sizeof(buffer);
// 		ret = at_token_string_get(&token, buffer, &buffer_len);
// 		zassert_ok(ret);
// 		zassert_equal(strlen("TEST2, "), buffer_len);
// 		zassert_mem_equal("TEST2, ", buffer, buffer_len);

// 		ret = at_parser_tok(&parser, &token);
// 		zassert_equal(ret, -EAGAIN);
// 		zassert_equal(token.type, AT_TOKEN_TYPE_EMPTY);

// 		ret = at_parser_tok(&parser, &token);
// 		zassert_ok(ret);
// 		zassert_equal(token.type, AT_TOKEN_TYPE_NOTIF);

// 		buffer_len = sizeof(buffer);
// 		ret = at_token_string_get(&token, buffer, &buffer_len);
// 		zassert_ok(ret);
// 		zassert_equal(strlen("#ABBA"), buffer_len);
// 		zassert_mem_equal("#ABBA", buffer, buffer_len);

// 		ret = at_parser_tok(&parser, &token);
// 		zassert_ok(ret);
// 		zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 		ret = at_token_int32_get(&token, &val);
// 		zassert_ok(ret);
// 		zassert_equal(val, 1);

// 		ret = at_parser_tok(&parser, &token);
// 		zassert_equal(ret, -EAGAIN);
// 		zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 		ret = at_token_int32_get(&token, &val);
// 		zassert_ok(ret);
// 		zassert_equal(val, 2);

// 		ret = at_parser_tok(&parser, &token);
// 		zassert_ok(ret);
// 		zassert_equal(token.type, AT_TOKEN_TYPE_NOTIF);

// 		buffer_len = sizeof(buffer);
// 		ret = at_token_string_get(&token, buffer, &buffer_len);
// 		zassert_ok(ret);
// 		zassert_equal(strlen("%TEST"), buffer_len);
// 		zassert_mem_equal("%TEST", buffer, buffer_len);

// 		ret = at_parser_tok(&parser, &token);
// 		zassert_ok(ret);
// 		zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 		ret = at_token_int32_get(&token, &val);
// 		zassert_ok(ret);
// 		zassert_equal(val, 1);

// 		ret = at_parser_tok(&parser, &token);
// 		zassert_equal(ret, -EIO);
// 	}
// }

ZTEST(at_parser, test_at_parser_num_get_einval)
{
	int ret;
	int32_t val;
	struct at_parser parser = {0};

	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

	/* 1) Invalid input pointers. */
	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_int32_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, 1);

	ret = at_parser_int32_get(NULL, 1, NULL);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_int32_get(&parser, 1, NULL);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_int32_get(NULL, 1, &val);
	zassert_equal(ret, -EINVAL);

	// /* 2) The token is pointing to a null string. */
	// ret = at_parser_init(&parser, str1);
	// zassert_ok(ret);

	// ret = at_parser_uint16_get(&token, 1, &val);
	// zassert_ok(ret);
	// zassert_equal(val, 1);

	// /* Override. */
	// token.start = NULL;

	// ret = at_token_uint16_get(&token, &val);
	// zassert_equal(ret, -EINVAL);

	// /* 3) The token is pointing to an empty string. */
	// ret = at_parser_init(&parser, str1);
	// zassert_ok(ret);

	// ret = at_parser_uint16_get(&token, 1, &val);
	// zassert_ok(ret);
	// zassert_equal(val, 1);

	// token.start = "";

	// ret = at_token_uint16_get(&token, &val);
	// zassert_equal(ret, -EINVAL);

	// /* 4) The length of the token is zero. */
	// ret = at_parser_init(&parser, str1);
	// zassert_ok(ret);

	// ret = at_parser_uint16_get(&token, 1, &val);
	// zassert_ok(ret);
	// zassert_equal(val, 1);

	// token.len = 0;

	// ret = at_token_uint16_get(&token, &val);
	// zassert_equal(ret, -EINVAL);

	// /* 5) The length of the token is larger than the length of the string it points to. */
	// ret = at_parser_init(&parser, str1);
	// zassert_ok(ret);

	// ret = at_parser_uint16_get(&token, 1, &val);
	// zassert_ok(ret);
	// zassert_equal(val, 1);

	// token.len = strlen(token.start) + 1;

	// ret = at_parser_uint16_get(&token, &val);
	// zassert_equal(ret, -EINVAL);

	// /* 6) The token has an invalid type. */
	// ret = at_parser_init(&parser, str1);
	// zassert_ok(ret);

	// ret = at_parser_seek(&parser, 1, &token);
	// zassert_ok(ret);
	// zassert_equal(token.type, AT_TOKEN_TYPE_INT);

	// ret = at_token_uint16_get(&token, &val);
	// zassert_ok(ret);
	// zassert_equal(val, 1);

	// /* Override type with anything other than `AT_TOKEN_TYPE_INT`. */
	// for (int i = AT_TOKEN_TYPE_INVALID; i < AT_TOKEN_TYPE_RESP; i++) {
	// 	if (i != AT_TOKEN_TYPE_INT) {
	// 		token.type = i;
	// 		ret = at_token_uint16_get(&token, &val);
	// 		zassert_equal(ret, -EINVAL);
	// 	}
	// }
}

// ZTEST(at_parser, test_at_token_uint16_get_erange)
// {
// 	int ret;
// 	uint16_t val;
// 	struct at_parser parser = {0};
// 	struct at_token token = {0};

// 	char str1[64] = {0};

// 	snprintf(str1, sizeof(str1), "+NOTIF: %d,-2,3\r\nOK\r\n", UINT16_MAX + 1);

// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);
	
// 	ret = at_token_uint16_get(&token, &val);
// 	zassert_equal(ret, -ERANGE);
// }

// ZTEST(at_parser, test_at_token_uint16_get)
// {
// 	int ret;
// 	uint16_t val;
// 	struct at_parser parser = {0};
// 	struct at_token token = {0};

// 	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_uint16_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	char str2[64] = {0};

// 	snprintf(str2, sizeof(str2), "+NOTIF: %d,-2,3\r\nOK\r\n", UINT16_MAX);

// 	ret = at_parser_init(&parser, str2);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);
	
// 	ret = at_token_uint16_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, UINT16_MAX, "%d", val);
// }

// ZTEST(at_parser, test_at_token_int16_get_einval)
// {
// 	int ret;
// 	int16_t val;
// 	struct at_parser parser = {0};
// 	struct at_token token = {0};

// 	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

// 	/* 1) Invalid input pointers. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int16_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	ret = at_token_int16_get(NULL, NULL);
// 	zassert_equal(ret, -EINVAL);

// 	ret = at_token_int16_get(&token, NULL);
// 	zassert_equal(ret, -EINVAL);

// 	ret = at_token_int16_get(NULL, &val);
// 	zassert_equal(ret, -EINVAL);

// 	/* 2) The token is pointing to a null string. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int16_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	token.start = NULL;

// 	ret = at_token_int16_get(&token, &val);
// 	zassert_equal(ret, -EINVAL);

// 	/* 3) The token is pointing to an empty string. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int16_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	token.start = "";

// 	ret = at_token_int16_get(&token, &val);
// 	zassert_equal(ret, -EINVAL);

// 	/* 4) The length of the token is zero. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int16_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	token.len = 0;

// 	ret = at_token_int16_get(&token, &val);
// 	zassert_equal(ret, -EINVAL);

// 	/* 5) The length of the token is larger than the length of the string it points to. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int16_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	token.len = strlen(token.start) + 1;

// 	ret = at_token_int16_get(&token, &val);
// 	zassert_equal(ret, -EINVAL);

// 	/* 6) The token has an invalid type. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int16_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	/* Override type with anything other than `AT_TOKEN_TYPE_INT`. */
// 	for (int i = AT_TOKEN_TYPE_INVALID; i < AT_TOKEN_TYPE_RESP; i++) {
// 		if (i != AT_TOKEN_TYPE_INT) {
// 			token.type = i;
// 			ret = at_token_int16_get(&token, &val);
// 			zassert_equal(ret, -EINVAL);
// 		}
// 	}
// }

// ZTEST(at_parser, test_at_token_int16_get_erange)
// {
// 	int ret;
// 	int16_t val;
// 	struct at_parser parser = {0};
// 	struct at_token token = {0};

// 	/* 1) Larger than allowed by `int16_t`. */
// 	char str1[64] = {0};

// 	snprintf(str1, sizeof(str1), "+NOTIF: %d,-2,3\r\nOK\r\n", INT16_MAX + 1);

// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);
	
// 	ret = at_token_int16_get(&token, &val);
// 	zassert_equal(ret, -ERANGE);

// 	/* 2) Smaller than allowed by `int16_t`. */
// 	char str2[64] = {0};

// 	snprintf(str2, sizeof(str2), "+NOTIF: %d,-2,3\r\nOK\r\n", INT16_MIN - 1);

// 	ret = at_parser_init(&parser, str2);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);
	
// 	ret = at_token_int16_get(&token, &val);
// 	zassert_equal(ret, -ERANGE);
// }

// ZTEST(at_parser, test_at_token_int16_get)
// {
// 	int ret;
// 	int16_t val;
// 	struct at_parser parser = {0};
// 	struct at_token token = {0};

// 	/* 1) */
// 	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int16_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	/* 2) */
// 	char str2[64] = {0};

// 	snprintf(str2, sizeof(str2), "+NOTIF: %d,-2,3\r\nOK\r\n", INT16_MAX);

// 	ret = at_parser_init(&parser, str2);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);
	
// 	ret = at_token_int16_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, INT16_MAX);

// 	/* 3) */
// 	char str3[64] = {0};

// 	snprintf(str3, sizeof(str3), "+NOTIF: %d,-2,3\r\nOK\r\n", INT16_MIN);

// 	ret = at_parser_init(&parser, str3);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);
	
// 	ret = at_token_int16_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, INT16_MIN);
// }

// ZTEST(at_parser, test_at_token_uint32_get_einval)
// {
// 	int ret;
// 	uint32_t val;
// 	struct at_parser parser = {0};
// 	struct at_token token = {0};

// 	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

// 	/* 1) Invalid input pointers. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_uint32_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	ret = at_token_uint32_get(NULL, NULL);
// 	zassert_equal(ret, -EINVAL);

// 	ret = at_token_uint32_get(&token, NULL);
// 	zassert_equal(ret, -EINVAL);

// 	ret = at_token_uint32_get(NULL, &val);
// 	zassert_equal(ret, -EINVAL);

// 	/* 2) The token is pointing to a null string. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_uint32_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	token.start = NULL;

// 	ret = at_token_uint32_get(&token, &val);
// 	zassert_equal(ret, -EINVAL);

// 	/* 3) The token is pointing to an empty string. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_uint32_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	token.start = "";

// 	ret = at_token_uint32_get(&token, &val);
// 	zassert_equal(ret, -EINVAL);

// 	/* 4) The length of the token is zero. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_uint32_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	token.len = 0;

// 	ret = at_token_uint32_get(&token, &val);
// 	zassert_equal(ret, -EINVAL);

// 	/* 5) The length of the token is larger than the length of the string it points to. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_uint32_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	token.len = strlen(token.start) + 1;

// 	ret = at_token_uint32_get(&token, &val);
// 	zassert_equal(ret, -EINVAL);

// 	/* 6) The token has an invalid type. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_uint32_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	/* Override type with anything other than `AT_TOKEN_TYPE_INT`. */
// 	for (int i = AT_TOKEN_TYPE_INVALID; i < AT_TOKEN_TYPE_RESP; i++) {
// 		if (i != AT_TOKEN_TYPE_INT) {
// 			token.type = i;
// 			ret = at_token_uint32_get(&token, &val);
// 			zassert_equal(ret, -EINVAL);
// 		}
// 	}
// }

// ZTEST(at_parser, test_at_token_uint32_get_erange)
// {
// 	int ret;
// 	uint32_t val;
// 	struct at_parser parser = {0};
// 	struct at_token token = {0};

// 	char str1[64] = {0};

// 	snprintf(str1, sizeof(str1), "+NOTIF: %lld,-2,3\r\nOK\r\n", (long long int)UINT32_MAX + 1);

// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);
	
// 	ret = at_token_uint32_get(&token, &val);
// 	zassert_equal(ret, -ERANGE);
// }

// ZTEST(at_parser, test_at_token_uint32_get)
// {
// 	int ret;
// 	uint32_t val;
// 	struct at_parser parser = {0};
// 	struct at_token token = {0};

// 	/* 1) */
// 	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_uint32_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	/* 2) */
// 	char str2[64] = {0};

// 	snprintf(str2, sizeof(str2), "+NOTIF: %lld,-2,3\r\nOK\r\n", (long long int)UINT32_MAX);

// 	ret = at_parser_init(&parser, str2);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);
	
// 	ret = at_token_uint32_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, UINT32_MAX);
// }

// ZTEST(at_parser, test_at_token_int32_get_einval)
// {
// 	int ret;
// 	int32_t val;
// 	struct at_parser parser = {0};
// 	struct at_token token = {0};

// 	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

// 	/* 1) Invalid input pointers. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int32_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	ret = at_token_int32_get(NULL, NULL);
// 	zassert_equal(ret, -EINVAL);

// 	ret = at_token_int32_get(&token, NULL);
// 	zassert_equal(ret, -EINVAL);

// 	ret = at_token_int32_get(NULL, &val);
// 	zassert_equal(ret, -EINVAL);

// 	/* 2) The token is pointing to a null string. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int32_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	token.start = NULL;

// 	ret = at_token_int32_get(&token, &val);
// 	zassert_equal(ret, -EINVAL);

// 	/* 3) The token is pointing to an empty string. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int32_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	token.start = "";

// 	ret = at_token_int32_get(&token, &val);
// 	zassert_equal(ret, -EINVAL);

// 	/* 4) The length of the token is zero. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int32_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	token.len = 0;

// 	ret = at_token_int32_get(&token, &val);
// 	zassert_equal(ret, -EINVAL);

// 	/* 5) The length of the token is larger than the length of the string it points to. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int32_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	token.len = strlen(token.start) + 1;

// 	ret = at_token_int32_get(&token, &val);
// 	zassert_equal(ret, -EINVAL);

// 	/* 6) The token has an invalid type. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int32_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	/* Override type with anything other than `AT_TOKEN_TYPE_INT`. */
// 	for (int i = AT_TOKEN_TYPE_INVALID; i < AT_TOKEN_TYPE_RESP; i++) {
// 		if (i != AT_TOKEN_TYPE_INT) {
// 			token.type = i;
// 			ret = at_token_int32_get(&token, &val);
// 			zassert_equal(ret, -EINVAL);
// 		}
// 	}
// }

// ZTEST(at_parser, test_at_token_int32_get_erange)
// {
// 	int ret;
// 	int32_t val;
// 	struct at_parser parser = {0};
// 	struct at_token token = {0};

// 	/* 1) */
// 	char str1[64] = {0};

// 	snprintf(str1, sizeof(str1), "+NOTIF: %lld,-2,3\r\nOK\r\n", (long long int)INT32_MAX + 1);

// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);
	
// 	ret = at_token_int32_get(&token, &val);
// 	zassert_equal(ret, -ERANGE);

// 	/* 2) */
// 	char str2[64] = {0};

// 	snprintf(str2, sizeof(str2), "+NOTIF: %lld,-2,3\r\nOK\r\n", (long long int)INT32_MIN - 1);

// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);
	
// 	ret = at_token_int32_get(&token, &val);
// 	zassert_equal(ret, -ERANGE);
// }

// ZTEST(at_parser, test_at_token_int32_get)
// {
// 	int ret;
// 	int32_t val;
// 	struct at_parser parser = {0};
// 	struct at_token token = {0};

// 	/* 1) */
// 	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int32_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	/* 2) */
// 	char str2[64] = {0};

// 	snprintf(str2, sizeof(str2), "+NOTIF: %lld,-2,3\r\nOK\r\n", (long long int)INT32_MAX);

// 	ret = at_parser_init(&parser, str2);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);
	
// 	ret = at_token_int32_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, INT32_MAX);

// 	/* 3) */
// 	char str3[64] = {0};

// 	snprintf(str3, sizeof(str3), "+NOTIF: %lld,-2,3\r\nOK\r\n", (long long int)INT32_MIN);

// 	ret = at_parser_init(&parser, str3);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);
	
// 	ret = at_token_int32_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, INT32_MIN);
// }

// ZTEST(at_parser, test_at_token_int64_get_einval)
// {
// 	int ret;
// 	int64_t val;
// 	struct at_parser parser = {0};
// 	struct at_token token = {0};

// 	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

// 	/* 1) Invalid input pointers. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int64_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	ret = at_token_int64_get(NULL, NULL);
// 	zassert_equal(ret, -EINVAL);

// 	ret = at_token_int64_get(&token, NULL);
// 	zassert_equal(ret, -EINVAL);

// 	ret = at_token_int64_get(NULL, &val);
// 	zassert_equal(ret, -EINVAL);

// 	/* 2) The token is pointing to a null string. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int64_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	token.start = NULL;

// 	ret = at_token_int64_get(&token, &val);
// 	zassert_equal(ret, -EINVAL);

// 	/* 3) The token is pointing to an empty string. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int64_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	token.start = "";

// 	ret = at_token_int64_get(&token, &val);
// 	zassert_equal(ret, -EINVAL);

// 	/* 4) The length of the token is zero. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int64_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	token.len = 0;

// 	ret = at_token_int64_get(&token, &val);
// 	zassert_equal(ret, -EINVAL);

// 	/* 5) The length of the token is larger than the length of the string it points to. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int64_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	token.len = strlen(token.start) + 1;

// 	ret = at_token_int64_get(&token, &val);
// 	zassert_equal(ret, -EINVAL);

// 	/* 6) The token has an invalid type. */
// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int64_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	/* Override type with anything other than `AT_TOKEN_TYPE_INT`. */
// 	for (int i = AT_TOKEN_TYPE_INVALID; i < AT_TOKEN_TYPE_RESP; i++) {
// 		if (i != AT_TOKEN_TYPE_INT) {
// 			token.type = i;
// 			ret = at_token_int64_get(&token, &val);
// 			zassert_equal(ret, -EINVAL);
// 		}
// 	}
// }

// ZTEST(at_parser, test_at_token_int64_get_erange)
// {
// 	int ret;
// 	int64_t val;
// 	struct at_parser parser = {0};
// 	struct at_token token = {0};

// 	/* 1) */
// 	const char *str1 = "+NOTIF: 100000000000000000000,-2,3\r\nOK\r\n";

// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);
	
// 	ret = at_token_int64_get(&token, &val);
// 	zassert_equal(ret, -ERANGE);

// 	/* 2) */
// 	const char *str2 = "+NOTIF: -100000000000000000000,-2,3\r\nOK\r\n";

// 	ret = at_parser_init(&parser, str2);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);
	
// 	ret = at_token_int64_get(&token, &val);
// 	zassert_equal(ret, -ERANGE);
// }

// ZTEST(at_parser, test_at_token_int64_get)
// {
// 	int ret;
// 	int64_t val;
// 	struct at_parser parser = {0};
// 	struct at_token token = {0};

// 	/* 1) */
// 	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);

// 	ret = at_token_int64_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, 1);

// 	/* 2) */
// 	char str2[64] = {0};

// 	snprintf(str2, sizeof(str2), "+NOTIF: %lld,-2,3\r\nOK\r\n", (long long int)INT64_MAX);

// 	ret = at_parser_init(&parser, str2);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);
	
// 	ret = at_token_int64_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, INT64_MAX);

// 	/* 3) */
// 	char str3[64] = {0};

// 	snprintf(str3, sizeof(str3), "+NOTIF: %lld,-2,3\r\nOK\r\n", (long long int)INT64_MIN);

// 	ret = at_parser_init(&parser, str3);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 1, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_INT);
	
// 	ret = at_token_int64_get(&token, &val);
// 	zassert_ok(ret);
// 	zassert_equal(val, INT64_MIN);
// }

ZTEST(at_parser, test_at_parser_string_get_enomem)
{
	int ret;
	struct at_parser parser = {0};
	char buffer[32] = { 0 };
	size_t len = sizeof(buffer);

	const char *str1 = "+NOTIF: 1,2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	len = 1;

	ret = at_parser_string_get(&parser, 0, buffer, &len);
	zassert_equal(ret, -ENOMEM);
}

// ZTEST(at_parser, test_at_token_string_get_einval)
// {
// 	int ret;
// 	struct at_parser parser = {0};
// 	struct at_token token = {0};
// 	struct at_token mod_token = {0};
// 	char buffer[32] = { 0 };
// 	size_t len = sizeof(buffer);

// 	const char *str1 = "+NOTIF: 1,2,3\r\nOK\r\n";

// 	ret = at_token_string_get(NULL, buffer, &len);
// 	zassert_equal(ret, -EINVAL);

// 	ret = at_token_string_get(&token, NULL, &len);
// 	zassert_equal(ret, -EINVAL);

// 	ret = at_token_string_get(&token, buffer, NULL);
// 	zassert_equal(ret, -EINVAL);

// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_tok(&parser, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_NOTIF);

// 	memcpy(&mod_token, &token, sizeof(struct at_token));
// 	mod_token.type = AT_TOKEN_TYPE_INT;

// 	ret = at_token_string_get(&mod_token, buffer, &len);
// 	zassert_equal(ret, -EINVAL);

// 	memcpy(&mod_token, &token, sizeof(struct at_token));
// 	mod_token.start = NULL;

// 	ret = at_token_string_get(&mod_token, buffer, &len);
// 	zassert_equal(ret, -EINVAL);
// }

// ZTEST(at_parser, test_at_token_string_get)
// {
// 	int ret;
// 	struct at_parser parser = {0};
// 	struct at_token token = {0};
// 	char buffer[32] = { 0 };
// 	size_t len = sizeof(buffer);

// 	const char *str1 = "+CGEV: ME PDN ACT 0";

// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_tok(&parser, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_NOTIF);

// 	ret = at_parser_tok(&parser, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_STRING);

// 	ret = at_token_string_get(&token, buffer, &len);
// 	zassert_ok(ret);
// 	zassert_mem_equal("ME PDN ACT 0", buffer, len);
// }

// ZTEST(at_parser, test_at_cmd_type_get_invalid)
// {
// 	enum at_token_type type;

// 	type = at_cmd_type_get(NULL);
// 	zassert_equal(type, AT_TOKEN_TYPE_INVALID);

// 	type = at_cmd_type_get("");
// 	zassert_equal(type, AT_TOKEN_TYPE_INVALID);

// 	const char *str1 = "ABBA";

// 	type = at_cmd_type_get(str1);
// 	zassert_equal(type, AT_TOKEN_TYPE_INVALID);

// 	/* Not a valid command (note: it's a notification). */
// 	const char *str2 = "+NOTIF: 1,2,3\r\nOK\r\n";

// 	type = at_cmd_type_get(str2);
// 	zassert_equal(type, AT_TOKEN_TYPE_INVALID);
// }

// ZTEST(at_parser, test_at_cmd_type_get_valid)
// {
// 	enum at_token_type type;

// 	const char *str1 = "AT+CMD=?";

// 	type = at_cmd_type_get(str1);
// 	zassert_equal(type, AT_TOKEN_TYPE_CMD_TEST);

// 	const char *str2 = "AT+CMD?";

// 	type = at_cmd_type_get(str2);
// 	zassert_equal(type, AT_TOKEN_TYPE_CMD_READ);

// 	const char *str3 = "AT+CMD=1,2,3";

// 	type = at_cmd_type_get(str3);
// 	zassert_equal(type, AT_TOKEN_TYPE_CMD_SET);
// }

// ZTEST(at_parser, test_at_token_type_empty_cmd_set)
// {
// 	int ret;
// 	struct at_parser parser = {0};
// 	struct at_token token = {0};

// 	const char *str1 = "AT+CMD=1,2,3,,";

// 	ret = at_parser_init(&parser, str1);
// 	zassert_ok(ret);

// 	ret = at_parser_seek(&parser, 4, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_EMPTY);

// 	ret = at_parser_tok(&parser, &token);
// 	zassert_ok(ret);
// 	zassert_equal(token.type, AT_TOKEN_TYPE_EMPTY);
// }

ZTEST(at_parser, test_at_next_line_get)
{
	int ret;
	struct at_parser parser = {0};

	const char *first_line = "+NOTIF: 1,2,3,,\r\n";
	const char *second_line = "+NEXT: 1,2\r\n";
	const char *third_line = "+NEXT: 4,5,6\r\n"
				 "OK\r\n";
	char at1[128] = {0};
	char at2[64] = {0};

	snprintf(at2, sizeof(at2), "%s%s", second_line, third_line);
	snprintf(at1, sizeof(at1), "%s%s", first_line, at2);

	ret = at_parser_init(&parser, at1);
	zassert_ok(ret);

	ret = at_parser_next(&parser);
	zassert_ok(ret);

	ret = at_parser_next(&parser);
	zassert_ok(ret);
}

ZTEST_SUITE(at_parser, NULL, NULL, test_params_before, test_params_after, NULL);
