/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <net/lwm2m_client_utils.h>
#include <modem/lte_lc.h>
#include <zephyr/settings/settings.h>
#include <modem/modem_info.h>
#include <modem/nrf_modem_lib.h>
#include <lwm2m_util.h>

#include "stubs.h"

extern void lwm2m_firmware_emulate_modem_lib_init(int modem_init_ret_val);
static void tear_down_test(void *fixie);
static void setup_tear_up(void *fixie);
static void *init_firmware(void);

ZTEST_SUITE(lwm2m_client_utils_firmware, NULL, init_firmware, setup_tear_up, tear_down_test, NULL);

#if defined(CONFIG_LWM2M_CLIENT_UTILS_ADV_FIRMWARE_UPDATE_OBJ_SUPPORT)
#define FIRM_UPDATE_OBJECT_ID 33629
#define INSTANCE_COUNT 2
static const char app_state[] = "lwm2m:fir/33629/0/3";
static const char app_result[] = "lwm2m:fir/33629/0/5";
static const char modem_state[] = "lwm2m:fir/33629/1/3";
static const char modem_result[] = "lwm2m:fir/33629/1/5";
static const char modem_type[] = "/33629/1";

static uint8_t app_instance;
static uint8_t modem_instance = 1;
#else
#define FIRM_UPDATE_OBJECT_ID 5
#define INSTANCE_COUNT 1
static const char app_state[] = "lwm2m:fir/5/0/3";
static const char app_result[] = "lwm2m:fir/5/0/5";

static const char modem_state[] = "lwm2m:fir/5/0/3";
static const char modem_result[] = "lwm2m:fir/5/0/5";
static const char *modem_type = "/5/0";

static uint8_t app_instance;
static uint8_t modem_instance;
#endif

struct firmware_object {
	uint8_t state;
	uint8_t result;
	uint8_t image_type;
};

static struct firmware_object test_object[2];
static uint8_t *stream_buf;
static size_t stream_buf_len;
static fota_download_callback_t firmware_fota_download_cb;

static struct settings_handler *handler;
static int fota_download_ret_val;
static int boot_img_num;
static bool target_reset_done;
static bool target_done_result;
static size_t target_offset;
static uint8_t test_state_reg;

static int dfu_target_mcuboot_set_buf_stub(uint8_t *buf, size_t len)
{
	stream_buf = buf;
	stream_buf_len = len;
	return 0;
}

static const char *lwm2m_adv_path(uint16_t inst, uint16_t res)
{
	int ret;
	static char path[LWM2M_MAX_PATH_STR_LEN];

	ret = snprintk(path, sizeof(path), "%" PRIu16 "/%" PRIu16 "/%" PRIu16,
		       FIRM_UPDATE_OBJECT_ID, inst, res);
	if (ret < 0 || ret >= sizeof(path)) {
		memset(path, 0, sizeof(path));
	}
	return path;
}

static struct lwm2m_engine_res *lwm2m_resource_get(uint16_t obj_inst_id, uint16_t resource_id)
{
	const char *pathstr;
	struct lwm2m_obj_path path;

	pathstr = lwm2m_adv_path(obj_inst_id, resource_id);

	if (lwm2m_string_to_path(pathstr, &path, '/')) {
		return NULL;
	}
	return lwm2m_engine_get_res(&path);
}

static int post_write_to_resource(uint16_t instance_id, uint16_t resource_id, uint8_t *data,
				  uint16_t data_len, bool last_block, size_t total_size)
{
	struct lwm2m_engine_res *res;

	res = lwm2m_resource_get(instance_id, resource_id);
	if (!res || !res->post_write_cb) {
		return -1;
	}

	return res->post_write_cb(instance_id, res->res_id, 0, data, data_len, last_block,
				  total_size);
}

static void tear_down_test(void *fixie)
{
	uint8_t result, state;

	for (uint16_t i = 0; i < INSTANCE_COUNT; i++) {
		post_write_to_resource(i, LWM2M_FOTA_PACKAGE_URI_ID, NULL, 0, false, 0);
		lwm2m_engine_get_u8(lwm2m_adv_path(i, LWM2M_FOTA_UPDATE_RESULT_ID), &result);
		lwm2m_engine_get_u8(lwm2m_adv_path(i, LWM2M_FOTA_STATE_ID), &state);
		printf("Firmware instance %d cancel result %d state %d\r\n", i, result, state);
	}
}

static uint8_t get_app_result(void)
{
	uint8_t result = 9;

	lwm2m_engine_get_u8(lwm2m_adv_path(app_instance, LWM2M_FOTA_UPDATE_RESULT_ID), &result);

	return result;
}

static uint8_t get_result(uint16_t obj_inst_id)
{
	uint8_t result = 9;

	lwm2m_engine_get_u8(lwm2m_adv_path(obj_inst_id, LWM2M_FOTA_UPDATE_RESULT_ID), &result);

	return result;
}

static uint8_t get_modem_result(void)
{
	uint8_t result = 9;

	lwm2m_engine_get_u8(lwm2m_adv_path(modem_instance, LWM2M_FOTA_UPDATE_RESULT_ID), &result);

	return result;
}

static uint8_t get_app_state(void)
{
	uint8_t state = 9;

	lwm2m_engine_get_u8(lwm2m_adv_path(app_instance, LWM2M_FOTA_STATE_ID), &state);

	return state;
}

static uint8_t get_modem_state(void)
{
	uint8_t state = 9;

	lwm2m_engine_get_u8(lwm2m_adv_path(modem_instance, LWM2M_FOTA_STATE_ID), &state);

	return state;
}

static int lwm2m_firmware_update(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	struct lwm2m_engine_res *res;

	res = lwm2m_resource_get(obj_inst_id, LWM2M_FOTA_UPDATE_ID);

	if (!res || !res->execute_cb) {
		return -1;
	}

	return res->execute_cb(obj_inst_id, args, args_len);
}

static void test_cancel_and_clear_state(uint16_t instance_id)
{
	uint8_t result, cmp_result;

#if defined(CONFIG_LWM2M_CLIENT_UTILS_ADV_FIRMWARE_UPDATE_OBJ_SUPPORT)
	struct lwm2m_engine_res *res;

	cmp_result = RESULT_ADV_FOTA_CANCELLED;

	res = lwm2m_resource_get(instance_id, LWM2M_FOTA_CANCEL_ID);
	if (res && res->execute_cb) {
		printf("Cancel %d", instance_id);
		res->execute_cb(instance_id, NULL, 0);
	} else {
		printf("no resource for Cancel execute %d", instance_id);
	}

#else
	cmp_result = RESULT_DEFAULT;
	post_write_to_resource(instance_id, LWM2M_FOTA_PACKAGE_URI_ID, NULL, 0, false, 0);
#endif
	result = get_result(instance_id);
	printf("Result of cancel %d", result);
	zassert_equal(result, cmp_result, "wrong result value");
}

static int check_stored_settings(const char *name, const void *value, size_t val_len)
{
	zassert_not_null(name, "NULL name settings");
	zassert_not_null(value, "NULL");

	printf("Store settings to %s\r\n", name);

	if (strcmp(name, app_state) == 0) {
		memcpy(&test_object[app_instance].state, value, val_len);
	} else if (strcmp(name, app_result) == 0) {
		memcpy(&test_object[app_instance].result, value, val_len);
	} else if (strcmp(name, modem_state) == 0) {
		memcpy(&test_object[modem_instance].state, value, val_len);
	} else if (strcmp(name, modem_result) == 0) {
		memcpy(&test_object[modem_instance].result, value, val_len);
	}
	return 0;
}

static int copy_settings_hanler(struct settings_handler *cf)
{
	handler = cf;
	return 0;
}

static ssize_t read_cb(void *cb_arg, void *data, size_t len)
{
	memcpy(data, cb_arg, len);
	return len;
}

static int fota_download_init_stub(fota_download_callback_t client_callback)
{
	firmware_fota_download_cb = client_callback;
	return fota_download_ret_val;
}

static int target_schedule_update_stub(int img_num)
{
	boot_img_num = img_num;
	return 0;
}

static int target_reset_stub(void)
{
	target_reset_done = true;
	return 0;
}

static int target_done_stub(bool successful)
{
	target_done_result = successful;
	return 0;
}

static int target_offset_get_stub(size_t *offset)
{
	*offset = target_offset;
	return 0;
}

static int test_lwm2m_firmware_get_update_state_cb(uint8_t update_state)
{
	test_state_reg = update_state;
	return 0;
}

static void setup_tear_up(void *fixie)
{
	/* Register resets */
	DO_FOREACH_FAKE(RESET_FAKE);

	/* reset common FFF internal structures */
	FFF_RESET_HISTORY();
	for (int i = 0; i < 2; i++) {
		test_object[i].result = RESULT_DEFAULT;
		test_object[i].state = STATE_IDLE;
	}

	firmware_fota_download_cb = NULL;
	fota_download_ret_val = 0;
	fota_download_start_with_image_type_fake.return_val = 0;
	fota_download_target_fake.return_val = DFU_TARGET_IMAGE_TYPE_MCUBOOT;
	fota_download_init_fake.custom_fake = fota_download_init_stub;
	settings_subsys_init_fake.return_val = 0;
	settings_register_fake.custom_fake = copy_settings_hanler;
	settings_save_one_fake.custom_fake = check_stored_settings;
	target_offset = 0;
	dfu_target_mcuboot_set_buf_fake.custom_fake = dfu_target_mcuboot_set_buf_stub;
	dfu_target_mcuboot_schedule_update_fake.custom_fake = target_schedule_update_stub;
	dfu_target_modem_delta_schedule_update_fake.custom_fake = target_schedule_update_stub;
	dfu_target_mcuboot_reset_fake.custom_fake = target_reset_stub;
	dfu_target_modem_delta_reset_fake.custom_fake = target_reset_stub;
	dfu_target_mcuboot_done_fake.custom_fake = target_done_stub;
	dfu_target_modem_delta_done_fake.custom_fake = target_done_stub;
	dfu_target_mcuboot_offset_get_fake.custom_fake = target_offset_get_stub;
	dfu_target_modem_delta_offset_get_fake.custom_fake = target_offset_get_stub;
}

static void init_firmware_success(void)
{
	int rc;

	settings_register_fake.return_val = 0;
	settings_register_fake.custom_fake = copy_settings_hanler;
	rc = lwm2m_init_firmware();
	zassert_equal(rc, 0, "wrong return value");
	zassert_not_null(stream_buf, "MCUboot is NULL");
	zassert_not_null(handler, "Did not set handler");
	zassert_not_null(handler->h_set, "Did not set handler");
}

static void *init_firmware(void)
{
	int rc;

	/* Validate is inside setup function*/
	setup_tear_up(NULL);
	handler = NULL;

	settings_register_fake.custom_fake = NULL;
	stream_buf = NULL;
	stream_buf_len = 0;
	settings_subsys_init_fake.return_val = -1;
	rc = lwm2m_init_firmware();
	zassert_equal(rc, -1, "wrong return value");
	settings_subsys_init_fake.return_val = 0;
	settings_register_fake.return_val = -1;

	rc = lwm2m_init_firmware();
	zassert_equal(rc, -1, "wrong return value");
	init_firmware_success();
	return NULL;
}

static void modem_firmware_update(uint8_t *state, uint8_t *result, int modem_lib_init_ret_val)
{
	lwm2m_firmware_emulate_modem_lib_init(modem_lib_init_ret_val);
	lwm2m_verify_modem_fw_update();
	*state = test_object[modem_instance].state;
	*result = test_object[modem_instance].result;
}

static void prepare_firmware_pull(uint8_t instance_id, enum dfu_target_image_type type)
{
	int rc;
	char test_url[] = "https://test_server.com/test.bin";

	fota_download_target_fake.return_val = type;
	if (fota_download_ret_val == 0) {
		/* Clear only when successfully init */
		firmware_fota_download_cb = NULL;
	}

	rc = post_write_to_resource(instance_id, LWM2M_FOTA_PACKAGE_URI_ID, test_url,
				    sizeof(test_url), false, sizeof(test_url));

	zassert_equal(rc, 0, "wrong return value");
	k_sleep(K_SECONDS(1));
}

static void pull_callback_event_stub(enum fota_download_evt_id event_id,
				     enum fota_download_error_cause cause)
{
	struct fota_download_evt fota_evt;

	fota_evt.id = event_id;

	switch (event_id) {
	case FOTA_DOWNLOAD_EVT_PROGRESS:
		fota_evt.progress = 10;
		break;

	case FOTA_DOWNLOAD_EVT_ERROR:
		fota_evt.cause = cause;
		break;
	default:
		break;
	}

	firmware_fota_download_cb(&fota_evt);
	k_sleep(K_SECONDS(2));
}

static void do_firmware_update(uint8_t instance)
{
	int rc;

	boot_img_num = -1;
	rc = lwm2m_firmware_update(instance, NULL, 0);
	if (rc) {
		printf("Firmware update fail %d", rc);
	}
	k_sleep(K_SECONDS(6));
}

ZTEST(lwm2m_client_utils_firmware, test_verfy_modem_fw_update)
{
	int image_type;
	uint8_t state, result;

	lwm2m_firmware_emulate_modem_lib_init(MODEM_DFU_RESULT_OK);
	image_type = DFU_TARGET_IMAGE_TYPE_MODEM_DELTA;
	handler->h_set(modem_type, sizeof(image_type), read_cb, &image_type);
	modem_firmware_update(&state, &result, MODEM_DFU_RESULT_OK);
	zassert_equal(state, STATE_IDLE, "wrong state");
	zassert_equal(result, RESULT_SUCCESS, "wrong result");

	modem_firmware_update(&state, &result, MODEM_DFU_RESULT_UUID_ERROR);
	zassert_equal(state, STATE_IDLE, "wrong state");
	zassert_equal(result, RESULT_UPDATE_FAILED, "wrong result");

	modem_firmware_update(&state, &result, MODEM_DFU_RESULT_HARDWARE_ERROR);
	zassert_equal(state, STATE_IDLE, "wrong state");
	zassert_equal(result, RESULT_UPDATE_FAILED, "wrong result");

	modem_firmware_update(&state, &result, 0);
	zassert_equal(state, STATE_IDLE, "wrong state");
	zassert_equal(result, RESULT_UPDATE_FAILED, "wrong result");
}

ZTEST(lwm2m_client_utils_firmware, test_init_image_failure)
{
	int rc;
	uint8_t result, state;

	/* Down load app first */
	prepare_firmware_pull(app_instance, DFU_TARGET_IMAGE_TYPE_MCUBOOT);
	state = get_app_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADING, "wrong result value");
	zassert_not_null(firmware_fota_download_cb, "Fota client cb is NULL");

	pull_callback_event_stub(FOTA_DOWNLOAD_EVT_FINISHED, 0);
	state = get_app_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADED, "wrong result value");

	rc = lwm2m_firmware_update(app_instance, NULL, 0);
	printf("Update %d\r\n", rc);
	zassert_equal(rc, 0, "wrong result value");
	k_sleep(K_SECONDS(6));
	state = get_app_state();
	zassert_equal(state, STATE_UPDATING, "wrong result value");
	/* Simulate Boot and verify image */
	boot_is_img_confirmed_fake.return_val = true;
	rc = lwm2m_init_image();
	result = get_app_result();
	state = get_app_state();
	zassert_equal(rc, 0, "wrong return value");
	zassert_equal(result, RESULT_UPDATE_FAILED, "wrong state");
	zassert_equal(state, STATE_IDLE, "wrong result value");
}

ZTEST(lwm2m_client_utils_firmware, test_firmware_pull)
{
	uint8_t result, state;
	char test_broken_url[] = "https://test_server.com";
	char test_broken_url2[] = "https:/test_server.com/test.bin";

	/* Test Pull start fail by EBUSY*/
	fota_download_ret_val = -1;
	prepare_firmware_pull(app_instance, DFU_TARGET_IMAGE_TYPE_MCUBOOT);
	result = get_app_result();
	printf("Result %d\r\n", result);
	zassert_equal(result, RESULT_NO_STORAGE, "wrong result value");
	fota_download_ret_val = 0;

	/* Test Pull start fail by EINVAL*/
	post_write_to_resource(app_instance, LWM2M_FOTA_PACKAGE_URI_ID, test_broken_url,
			       sizeof(test_broken_url), false, sizeof(test_broken_url));
	result = get_app_result();
	printf("Result %d\r\n", result);
	zassert_equal(result, RESULT_INVALID_URI, "wrong result value");

	post_write_to_resource(app_instance, LWM2M_FOTA_PACKAGE_URI_ID, test_broken_url2,
			       sizeof(test_broken_url2), false, sizeof(test_broken_url2));
	result = get_app_result();
	printf("Result %d\r\n", result);
	zassert_equal(result, RESULT_INVALID_URI, "wrong result value");

	fota_download_start_with_image_type_fake.return_val = -1;
	prepare_firmware_pull(app_instance, DFU_TARGET_IMAGE_TYPE_MCUBOOT);
	result = get_app_result();
	printf("Result %d\r\n", result);
	zassert_equal(result, RESULT_CONNECTION_LOST, "wrong result value");
	fota_download_ret_val = 0;

	/* Test PULL start OK */
	fota_download_start_with_image_type_fake.return_val = 0;
	lwm2m_firmware_set_update_state_cb(test_lwm2m_firmware_get_update_state_cb);
	prepare_firmware_pull(app_instance, DFU_TARGET_IMAGE_TYPE_MCUBOOT);
	state = get_app_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADING, "wrong result value");
	zassert_not_null(firmware_fota_download_cb, "Fota client cb is NULL");
	zassert_equal(test_state_reg, STATE_DOWNLOADING, "State Not correct");

	pull_callback_event_stub(FOTA_DOWNLOAD_EVT_PROGRESS, 0);

	/* Test Cancel and clear ongoing id */
	target_reset_done = false;
	test_cancel_and_clear_state(app_instance);
	state = get_app_state();
	zassert_equal(state, STATE_IDLE, "State Not correct");
	zassert_equal(test_state_reg, STATE_IDLE, "State Not correct");
	zassert_equal(target_reset_done, true, "dfu target reset fail");
}

ZTEST(lwm2m_client_utils_firmware, test_firmware_pull_failures_cb)
{
	int rc;
	uint8_t result, state;
	uint8_t test_dummy_data[32];

	prepare_firmware_pull(app_instance, DFU_TARGET_IMAGE_TYPE_MCUBOOT);
	state = get_app_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADING, "wrong result value");
	zassert_not_null(firmware_fota_download_cb, "Fota client cb is NULL");

	/* Test FOTA download cancelled. */
	pull_callback_event_stub(FOTA_DOWNLOAD_EVT_CANCELLED, 0);
	result = get_app_result();
	state = get_app_state();
	zassert_equal(result, RESULT_CONNECTION_LOST, "wrong result value");
	zassert_equal(state, STATE_IDLE, "wrong result value");

	/* Test FOTA download error. */
	prepare_firmware_pull(app_instance, DFU_TARGET_IMAGE_TYPE_MCUBOOT);
	state = get_app_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADING, "wrong result value");
	zassert_not_null(firmware_fota_download_cb, "Fota client cb is NULL");
	pull_callback_event_stub(FOTA_DOWNLOAD_EVT_ERROR, FOTA_DOWNLOAD_ERROR_CAUSE_NO_ERROR);
	result = get_app_result();
	state = get_app_state();
	zassert_equal(result, RESULT_CONNECTION_LOST, "wrong result value");
	zassert_equal(state, STATE_IDLE, "wrong result value");

	prepare_firmware_pull(app_instance, DFU_TARGET_IMAGE_TYPE_MCUBOOT);
	state = get_app_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADING, "wrong result value");
	zassert_not_null(firmware_fota_download_cb, "Fota client cb is NULL");

	/* Test FOTA PUSH return EAGAIN progress report. */
	dfu_target_mcuboot_identify_fake.return_val = true;
	rc = post_write_to_resource(app_instance, LWM2M_FOTA_PACKAGE_ID, test_dummy_data, 32, false,
				    64);
	state = get_app_state();
	printf("RC %d state %d\r\n", rc, state);
	zassert_equal(rc, -EAGAIN, "wrong return value");
#if defined(CONFIG_LWM2M_CLIENT_UTILS_ADV_FIRMWARE_UPDATE_OBJ_SUPPORT)
	zassert_equal(state, STATE_DOWNLOADING, "wrong result value");
#else
	result = get_app_result();
	zassert_equal(result, RESULT_UPDATE_FAILED, "wrong result value");
	zassert_equal(state, STATE_IDLE, "wrong result value");
	prepare_firmware_pull(app_instance, DFU_TARGET_IMAGE_TYPE_MCUBOOT);
	state = get_app_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADING, "wrong result value");
	zassert_not_null(firmware_fota_download_cb, "Fota client cb is NULL");
#endif
	pull_callback_event_stub(FOTA_DOWNLOAD_EVT_ERROR,
				 FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED);
	result = get_app_result();
	zassert_equal(result, RESULT_CONNECTION_LOST, "wrong result value");

	prepare_firmware_pull(app_instance, DFU_TARGET_IMAGE_TYPE_MCUBOOT);
	state = get_app_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADING, "wrong result value");
	zassert_not_null(firmware_fota_download_cb, "Fota client cb is NULL");

	pull_callback_event_stub(FOTA_DOWNLOAD_EVT_ERROR, FOTA_DOWNLOAD_ERROR_CAUSE_INVALID_UPDATE);
	result = get_app_result();
	zassert_equal(result, RESULT_UNSUP_FW, "wrong result value");
}

ZTEST(lwm2m_client_utils_firmware, test_firmware_pull_success_cb)
{
	uint8_t state;

	prepare_firmware_pull(app_instance, DFU_TARGET_IMAGE_TYPE_MCUBOOT);
	state = get_app_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADING, "wrong result value");
	zassert_not_null(firmware_fota_download_cb, "Fota client cb is NULL");

	/* Test FOTA download progress report. */
	pull_callback_event_stub(FOTA_DOWNLOAD_EVT_PROGRESS, 0);
	pull_callback_event_stub(FOTA_DOWNLOAD_EVT_FINISHED, 0);
	state = get_app_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADED, "wrong result value");

	/* Test Cancel and clear ongoing id */
	test_cancel_and_clear_state(app_instance);
}

ZTEST(lwm2m_client_utils_firmware, test_firmware_update)
{
	int rc;
	uint8_t state, result;

	prepare_firmware_pull(app_instance, DFU_TARGET_IMAGE_TYPE_MCUBOOT);
	state = get_app_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADING, "wrong result value");
	zassert_not_null(firmware_fota_download_cb, "Fota client cb is NULL");
	pull_callback_event_stub(FOTA_DOWNLOAD_EVT_FINISHED, 0);
	state = get_app_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADED, "wrong result value");

	do_firmware_update(app_instance);
	state = get_app_state();
	zassert_equal(boot_img_num, 0, "wrong img num");
	zassert_equal(state, STATE_UPDATING, "wrong result value");

	boot_is_img_confirmed_fake.return_val = false;
	rc = lwm2m_init_image();
	result = get_app_result();
	state = get_app_state();
	zassert_equal(rc, 0, "wrong return value");
	zassert_equal(result, RESULT_SUCCESS, "wrong state");
	zassert_equal(state, STATE_IDLE, "wrong result value");

	prepare_firmware_pull(modem_instance, DFU_TARGET_IMAGE_TYPE_MODEM_DELTA);
	state = get_modem_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADING, "wrong result value");
	zassert_not_null(firmware_fota_download_cb, "Fota client cb is NULL");

	pull_callback_event_stub(FOTA_DOWNLOAD_EVT_FINISHED, 0);
	state = get_modem_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADED, "wrong result value");

	lwm2m_firmware_emulate_modem_lib_init(MODEM_DFU_RESULT_AUTH_ERROR);
	do_firmware_update(modem_instance);
	result = get_modem_result();
	state = get_app_state();
	printf("Result %d img num %d", result, boot_img_num);
	zassert_equal(boot_img_num, 0, "wrong img num");
	zassert_equal(result, RESULT_UPDATE_FAILED, "wrong img num");
	zassert_equal(state, STATE_IDLE, "wrong result value");

	/* Stub successful update */
	prepare_firmware_pull(modem_instance, DFU_TARGET_IMAGE_TYPE_MODEM_DELTA);
	state = get_modem_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADING, "wrong result value");
	zassert_not_null(firmware_fota_download_cb, "Fota client cb is NULL");

	pull_callback_event_stub(FOTA_DOWNLOAD_EVT_FINISHED, 0);
	state = get_modem_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADED, "wrong result value");

	lwm2m_firmware_emulate_modem_lib_init(MODEM_DFU_RESULT_OK);
	do_firmware_update(modem_instance);
	result = get_modem_result();
	zassert_equal(boot_img_num, 0, "wrong img num");
	zassert_equal(result, RESULT_SUCCESS, "wrong img num");

	prepare_firmware_pull(modem_instance, DFU_TARGET_IMAGE_TYPE_FULL_MODEM);
	state = get_modem_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADING, "wrong result value");
	zassert_not_null(firmware_fota_download_cb, "Fota client cb is NULL");

	pull_callback_event_stub(FOTA_DOWNLOAD_EVT_FINISHED, 0);
	state = get_modem_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADED, "wrong result value");

	rc = lwm2m_firmware_update(modem_instance, NULL, 0);
	state = get_modem_state();
	result = get_modem_result();
	printf("Update %d\r\n", rc);
	zassert_equal(rc, 0, "wrong result value");
	zassert_equal(result, RESULT_UPDATE_FAILED, "wrong result value");
	zassert_equal(state, STATE_IDLE, "wrong result value");
}

ZTEST(lwm2m_client_utils_firmware, test_firmware_push_update)
{
	int rc;
	uint8_t test_dummy_data[32];
	uint8_t state;

	dfu_target_mcuboot_identify_fake.return_val = false;
	rc = post_write_to_resource(app_instance, LWM2M_FOTA_PACKAGE_ID, NULL, 0, false, 0);
	zassert_equal(rc, -EINVAL, "wrong return value");

	rc = post_write_to_resource(app_instance, LWM2M_FOTA_PACKAGE_ID, test_dummy_data, 32, false,
				    64);
	zassert_equal(rc, -ENOMSG, "wrong return value");
	dfu_target_mcuboot_identify_fake.return_val = true;
	rc = post_write_to_resource(app_instance, LWM2M_FOTA_PACKAGE_ID, test_dummy_data, 32, false,
				    64);
	state = get_app_state();
	zassert_equal(rc, 0, "wrong return value");
	zassert_equal(state, STATE_DOWNLOADING, "wrong result value");

	target_offset = 32;
	target_done_result = false;
	rc = post_write_to_resource(app_instance, LWM2M_FOTA_PACKAGE_ID, test_dummy_data, 32, true,
				    64);
	state = get_app_state();
	zassert_equal(rc, 0, "wrong return value");
	zassert_equal(state, STATE_DOWNLOADED, "wrong result value");
	zassert_equal(target_done_result, true, "wrong return value");

	test_cancel_and_clear_state(app_instance);

	rc = post_write_to_resource(app_instance, LWM2M_FOTA_PACKAGE_ID, test_dummy_data, 32, false,
				    0);
	state = get_app_state();
	zassert_equal(state, STATE_DOWNLOADING, "wrong result value");
	zassert_equal(rc, 0, "wrong return value");
	target_offset = 32;
	target_done_result = false;
	rc = post_write_to_resource(app_instance, LWM2M_FOTA_PACKAGE_ID, test_dummy_data, 32, true,
				    0);
	state = get_app_state();
	zassert_equal(rc, 0, "wrong return value");
	zassert_equal(state, STATE_DOWNLOADED, "wrong result value");
	zassert_equal(target_done_result, true, "wrong return value");
}

#if defined(CONFIG_LWM2M_CLIENT_UTILS_ADV_FIRMWARE_UPDATE_OBJ_SUPPORT)
ZTEST(lwm2m_client_utils_firmware, test_firmware_multinstace_download)
{
	int rc;
	uint8_t state, result;
	fota_download_callback_t temp_copy;
	char link_modem_instance[] = "0='</33629/1>'";
	char link_modem_instance2[] = "0='</33629/2>'";

	/* Down load app first */
	prepare_firmware_pull(app_instance, DFU_TARGET_IMAGE_TYPE_MCUBOOT);
	state = get_app_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADING, "wrong result value");
	zassert_not_null(firmware_fota_download_cb, "Fota client cb is NULL");

	/* Test to start modem download middle of app download phase */
	temp_copy = firmware_fota_download_cb;
	prepare_firmware_pull(modem_instance, DFU_TARGET_IMAGE_TYPE_MODEM_DELTA);
	result = get_modem_result();
	zassert_equal(result, RESULT_ADV_CONFLICT_STATE, "wrong result value");
	firmware_fota_download_cb = temp_copy;

	pull_callback_event_stub(FOTA_DOWNLOAD_EVT_FINISHED, 0);
	state = get_app_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADED, "wrong result value");

	/* Test linkek update with unknown object link */
	rc = lwm2m_firmware_update(app_instance, link_modem_instance2,
				   sizeof(link_modem_instance2));
	printf("Update %d\r\n", rc);
	zassert_equal(rc, -ECANCELED, "wrong result value");

	/* Download Modem */
	prepare_firmware_pull(modem_instance, DFU_TARGET_IMAGE_TYPE_MODEM_DELTA);
	state = get_modem_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADING, "wrong result value");

	/* Test linkek update with not downloaded linked instance */
	rc = lwm2m_firmware_update(app_instance, link_modem_instance, sizeof(link_modem_instance));
	printf("Update %d\r\n", rc);
	zassert_equal(rc, -ECANCELED, "wrong result value");

	zassert_not_null(firmware_fota_download_cb, "Fota client cb is NULL");
	pull_callback_event_stub(FOTA_DOWNLOAD_EVT_FINISHED, 0);
	state = get_modem_state();
	printf("State %d\r\n", state);
	zassert_equal(state, STATE_DOWNLOADED, "wrong result value");
	/* Test Linked Instance update by update app + modem same time */

	rc = lwm2m_firmware_update(app_instance, link_modem_instance, sizeof(link_modem_instance));
	printf("Update %d\r\n", rc);
	zassert_equal(rc, 0, "wrong result value");
	/* Stub Linked write for update */
	lwm2m_firmware_emulate_modem_lib_init(MODEM_DFU_RESULT_OK);
	k_sleep(K_SECONDS(6));
	state = get_modem_state();
	zassert_equal(state, STATE_IDLE, "wrong result value");
	result = get_modem_result();
	zassert_equal(result, RESULT_SUCCESS, "wrong img num");
	state = get_app_state();
	zassert_equal(state, STATE_UPDATING, "wrong result value");

	boot_is_img_confirmed_fake.return_val = false;
	rc = lwm2m_init_image();
	result = get_app_result();
	state = get_app_state();
	zassert_equal(rc, 0, "wrong return value");
	zassert_equal(result, RESULT_SUCCESS, "wrong state");
	zassert_equal(state, STATE_IDLE, "wrong result value");
}
#endif
