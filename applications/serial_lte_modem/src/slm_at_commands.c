/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <modem/at_cmd_custom.h>
#include <zephyr/init.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <dfu/dfu_target.h>
#include <modem/at_cmd_parser.h>
#include <modem/lte_lc.h>
#include <modem/modem_jwt.h>
#include <modem/nrf_modem_lib.h>
#include "nrf_modem.h"
#include "ncs_version.h"

#include "slm_util.h"
#include "slm_settings.h"
#include "slm_at_host.h"
#include "slm_at_tcp_proxy.h"
#include "slm_at_udp_proxy.h"
#include "slm_at_socket.h"
#include "slm_at_icmp.h"
#include "slm_at_sms.h"
#include "slm_at_fota.h"
#if defined(CONFIG_SLM_NATIVE_TLS)
#include "slm_at_cmng.h"
#endif
#if defined(CONFIG_SLM_NRF_CLOUD)
#include "slm_at_nrfcloud.h"
#endif
#if defined(CONFIG_SLM_GNSS)
#include "slm_at_gnss.h"
#endif
#if defined(CONFIG_SLM_FTPC)
#include "slm_at_ftp.h"
#endif
#if defined(CONFIG_SLM_MQTTC)
#include "slm_at_mqtt.h"
#endif
#if defined(CONFIG_SLM_HTTPC)
#include "slm_at_httpc.h"
#endif
#if defined(CONFIG_SLM_TWI)
#include "slm_at_twi.h"
#endif
#if defined(CONFIG_SLM_GPIO)
#include "slm_at_gpio.h"
#endif
#if defined(CONFIG_SLM_CARRIER)
#include "slm_at_carrier.h"
#endif
#if defined(CONFIG_SLM_PPP)
#include "slm_ppp.h"
#endif
#if defined(CONFIG_SLM_CMUX)
#include "slm_cmux.h"
#endif

LOG_MODULE_REGISTER(slm_at, CONFIG_SLM_LOG_LEVEL);

/** @brief Shutdown modes. */
enum sleep_modes {
	SLEEP_MODE_INVALID,
	SLEEP_MODE_DEEP,
	SLEEP_MODE_IDLE
};

/** @brief AT command handler type. */
typedef int (*slm_at_handler_t) (enum at_cmd_type);

#if POWER_PIN_IS_ENABLED
static struct {
	struct k_work_delayable work;
	uint32_t mode;
} sleep;
#endif

bool verify_datamode_control(uint16_t time_limit, uint16_t *time_limit_min);

bool slm_is_modem_functional_mode(enum lte_lc_func_mode mode)
{
	int cfun;
	int rc = slm_util_at_scanf("AT+CFUN?", "+CFUN: %d", &cfun);

	return (rc == 1 && cfun == mode);
}

int slm_power_off_modem(void)
{
	/* "[...] there may be a delay until modem is disconnected from the network."
	 * https://infocenter.nordicsemi.com/topic/ps_nrf9160/chapters/pmu/doc/operationmodes/system_off_mode.html
	 * This will return once the modem responds, which means it has actually stopped.
	 * This has been observed to take between 1 and 2 seconds when it is not already stopped.
	 */
	return slm_util_at_printf("AT+CFUN=0");
}

AT_CMD_CUSTOM(xslmver, "AT#XSLMVER", handle_at_slmver);
static int handle_at_slmver(char *buf, size_t len, char *at_cmd)
{
	int ret = -EINVAL;
	enum at_cmd_type cmd_type = at_parser_cmd_type_get(at_cmd);

	set_default_at_response(buf, len);

	if (cmd_type == AT_CMD_TYPE_SET_COMMAND) {
		char *libmodem = nrf_modem_build_version();

		if (strlen(CONFIG_SLM_CUSTOMER_VERSION) > 0) {
			rsp_send("\r\n#XSLMVER: %s,\"%s\",\"%s\"\r\n",
				 STRINGIFY(NCS_VERSION_STRING), libmodem,
				 CONFIG_SLM_CUSTOMER_VERSION);
		} else {
			rsp_send("\r\n#XSLMVER: %s,\"%s\"\r\n",
				 STRINGIFY(NCS_VERSION_STRING), libmodem);
		}
		ret = 0;
	}

	return ret;
}

#if POWER_PIN_IS_ENABLED

static void go_sleep_wk(struct k_work *)
{
	if (sleep.mode == SLEEP_MODE_IDLE) {
		if (slm_at_host_power_off() == 0) {
			slm_enter_idle();
		} else {
			LOG_ERR("failed to power off UART");
		}
	} else if (sleep.mode == SLEEP_MODE_DEEP) {
		slm_enter_sleep();
	}
}

AT_CMD_CUSTOM(xsleep, "AT#XSLEEP", handle_at_sleep);
static int handle_at_sleep(char *buf, size_t len, char *at_cmd)
{
	int ret = -EINVAL;
	const struct at_param_list *list = slm_get_at_param_list(at_cmd);
	enum at_cmd_type cmd_type = at_parser_cmd_type_get(at_cmd);

	set_default_at_response(buf, len);

	if (cmd_type == AT_CMD_TYPE_SET_COMMAND) {
		ret = at_params_unsigned_int_get(list, 1, &sleep.mode);
		if (ret) {
			return -EINVAL;
		}
		if (sleep.mode == SLEEP_MODE_DEEP || sleep.mode == SLEEP_MODE_IDLE) {
			k_work_reschedule(&sleep.work, SLM_UART_RESPONSE_DELAY);
		} else {
			ret = -EINVAL;
		}
	} else if (cmd_type == AT_CMD_TYPE_TEST_COMMAND) {
		rsp_send("\r\n#XSLEEP: (%d,%d)\r\n", SLEEP_MODE_DEEP, SLEEP_MODE_IDLE);
		ret = 0;
	}

	return ret;
}

#endif /* POWER_PIN_IS_ENABLED */

static void final_call(void (*func)(void))
{
	/* Delegate the final call to a worker so that the "OK" response is properly sent. */
	static struct k_work_delayable worker;

	k_work_init_delayable(&worker, (k_work_handler_t)func);
	k_work_schedule(&worker, SLM_UART_RESPONSE_DELAY);
}

static void slm_shutdown(void)
{
	slm_at_host_uninit();
	slm_power_off_modem();
	LOG_PANIC();
	slm_enter_shutdown();
}

AT_CMD_CUSTOM(xshutdown, "AT#XSHUTDOWN", handle_at_shutdown);
static int handle_at_shutdown(char *buf, size_t len, char *at_cmd)
{
	if (at_parser_cmd_type_get(at_cmd) != AT_CMD_TYPE_SET_COMMAND) {
		return -EINVAL;
	}
	set_default_at_response(buf, len);

	final_call(slm_shutdown);
	return 0;
}

FUNC_NORETURN void slm_reset(void)
{
	slm_at_host_uninit();
	slm_power_off_modem();
	LOG_PANIC();
	sys_reboot(SYS_REBOOT_COLD);
}

AT_CMD_CUSTOM(xreset, "AT#XRESET", handle_at_reset);
static int handle_at_reset(char *buf, size_t len, char *at_cmd)
{
	if (at_parser_cmd_type_get(at_cmd) != AT_CMD_TYPE_SET_COMMAND) {
		return -EINVAL;
	}
	set_default_at_response(buf, len);

	final_call(slm_reset);
	return 0;
}

AT_CMD_CUSTOM(xmodemreset, "AT#XMODEMRESET", handle_at_modemreset);
static int handle_at_modemreset(char *buf, size_t len, char *at_cmd)
{
	if (at_parser_cmd_type_get(at_cmd) != AT_CMD_TYPE_SET_COMMAND) {
		return -EINVAL;
	}
	set_default_at_response(buf, len);

	/* The modem must be put in minimal function mode before being shut down. */
	slm_power_off_modem();

	unsigned int step = 1;
	int ret;

	ret = nrf_modem_lib_shutdown();
	if (ret != 0) {
		goto out;
	}
	++step;

#if defined(CONFIG_SLM_FULL_FOTA)
	if (slm_modem_full_fota) {
		slm_finish_modem_full_fota();
	}
#endif

	ret = nrf_modem_lib_init();

	if (slm_fota_type & DFU_TARGET_IMAGE_TYPE_ANY_MODEM) {
		slm_fota_post_process();
	}

out:
	if (ret) {
		/* Error; print the step that failed and its error code. */
		rsp_send("\r\n#XMODEMRESET: %u,%d\r\n", step, ret);
		return 0;
	}

	rsp_send("\r\n#XMODEMRESET: 0\r\n");
	return 0;
}

AT_CMD_CUSTOM(xuuid, "AT#XUUID", handle_at_uuid);
static int handle_at_uuid(char *buf, size_t len, char *at_cmd)
{
	int ret;

	if (at_parser_cmd_type_get(at_cmd) != AT_CMD_TYPE_SET_COMMAND) {
		return -EINVAL;
	}
	set_default_at_response(buf, len);

	struct nrf_device_uuid dev = {0};

	ret = modem_jwt_get_uuids(&dev, NULL);
	if (ret) {
		LOG_ERR("Get device UUID error: %d", ret);
	} else {
		rsp_send("\r\n#XUUID: %s\r\n", dev.str);
	}

	return ret;
}

AT_CMD_CUSTOM(xdatactrl, "AT#XDATACTRL", handle_at_datactrl);
static int handle_at_datactrl(char *buf, size_t len, char *at_cmd)
{
	int ret = 0;
	uint16_t time_limit, time_limit_min;
	const struct at_param_list *list = slm_get_at_param_list(at_cmd);
	enum at_cmd_type cmd_type = at_parser_cmd_type_get(at_cmd);

	set_default_at_response(buf, len);

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		ret = at_params_unsigned_short_get(list, 1, &time_limit);
		if (ret) {
			return ret;
		}
		if (time_limit > 0 && verify_datamode_control(time_limit, NULL)) {
			slm_datamode_time_limit = time_limit;
		} else {
			return -EINVAL;
		}
		break;

	case AT_CMD_TYPE_READ_COMMAND:
		(void)verify_datamode_control(slm_datamode_time_limit, &time_limit_min);
		rsp_send("\r\n#XDATACTRL: %d,%d\r\n", slm_datamode_time_limit, time_limit_min);
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XDATACTRL=<time_limit>\r\n");
		break;

	default:
		break;
	}

	return ret;
}


static const char * const slm_at_cmd_list[] = {
	/* Generic commands */
	"AT#XSLMVER",
#if POWER_PIN_IS_ENABLED
	"AT#XSLEEP",
#endif
	"AT#XSHUTDOWN",
	"AT#XRESET",
	"AT#XMODEMRESET",
	"AT#XUUID",
	"AT#XCLAC",
	"AT#XDATACTRL",

	/* TCP proxy commands */
	"AT#XTCPSVR",
	"AT#XTCPCLI",
	"AT#XTCPSEND",
	"AT#XTCPHANGUP",

	/* UDP proxy commands */
	"AT#XUDPSVR",
	"AT#XUDPCLI",
	"AT#XUDPSEND",

	/* Socket-type TCPIP commands */
	"AT#XSOCKET",
	"AT#XSSOCKET",
	"AT#XSOCKETSELECT",
	"AT#XSOCKETOPT",
	"AT#XSSOCKETOPT",
	"AT#XBIND",
	"AT#XCONNECT",
	"AT#XLISTEN",
	"AT#XACCEPT",
	"AT#XSEND",
	"AT#XRECV",
	"AT#XSENDTO",
	"AT#XRECVFROM",
	"AT#XPOLL",
	"AT#XGETADDRINFO",

#if defined(CONFIG_SLM_NATIVE_TLS)
	"AT#XCMNG",
#endif
	/* ICMP commands */
	"AT#XPING",

#if defined(CONFIG_SLM_SMS)
	/* SMS commands */
	"AT#XSMS",
#endif

	/* FOTA commands */
	"AT#XFOTA",

#if defined(CONFIG_SLM_NRF_CLOUD)
	"AT#XNRFCLOUD",
#if defined(CONFIG_NRF_CLOUD_LOCATION)
	"AT#XNRFCLOUDPOS",
#endif
#endif

#if defined(CONFIG_SLM_GNSS)
	/* GNSS commands */
	"AT#XGPS",
	"AT#XGPSDEL",
#endif

#if defined(CONFIG_SLM_FTPC)
	/* FTP commands */
	"AT#XFTP",
#endif
#if defined(CONFIG_SLM_TFTPC)
	/* TFTP commands */
	"AT#XTFTP",
#endif

#if defined(CONFIG_SLM_MQTTC)
	"AT#XMQTTCFG",
	"AT#XMQTTCON",
	"AT#XMQTTPUB",
	"AT#XMQTTSUB",
	"AT#XMQTTUNSUB",
#endif

#if defined(CONFIG_SLM_HTTPC)
	"AT#XHTTPCCON",
	"AT#XHTTPCREQ",
#endif

#if defined(CONFIG_SLM_TWI)
	"AT#XTWILS",
	"AT#XTWIW",
	"AT#XTWIR",
	"AT#XTWIWR",
#endif

#if defined(CONFIG_SLM_GPIO)
	"AT#XGPIOCFG",
	"AT#XGPIO",
#endif

#if defined(CONFIG_SLM_CARRIER)
	"AT#XCARRIER",
#endif

#if defined(CONFIG_LWM2M_CARRIER_SETTINGS)
	"AT#XCARRIERCFG",
#endif

#if defined(CONFIG_SLM_PPP)
	"AT#XPPP",
#endif

#if defined(CONFIG_SLM_CMUX)
	"AT#XCMUX",
#endif
};

AT_CMD_CUSTOM(xclac, "AT#XCLAC", handle_at_clac);
static int handle_at_clac(char *buf, size_t len, char *at_cmd)
{
	int ret = -EINVAL;
	enum at_cmd_type cmd_type = at_parser_cmd_type_get(at_cmd);

	set_default_at_response(buf, len);

	if (cmd_type == AT_CMD_TYPE_SET_COMMAND) {
		int total = ARRAY_SIZE(slm_at_cmd_list);

		for (int i = 0; i < total; i++) {
			rsp_send("%s\r\n", slm_at_cmd_list[i]);
		}
		ret = 0;
	}

	return ret;
}

int slm_at_init(void)
{
	int err;

#if POWER_PIN_IS_ENABLED
	k_work_init_delayable(&sleep.work, go_sleep_wk);
#endif

	err = slm_at_tcp_proxy_init();
	if (err) {
		LOG_ERR("TCP Server could not be initialized: %d", err);
		return -EFAULT;
	}
	err = slm_at_udp_proxy_init();
	if (err) {
		LOG_ERR("UDP Server could not be initialized: %d", err);
		return -EFAULT;
	}
	err = slm_at_socket_init();
	if (err) {
		LOG_ERR("TCPIP could not be initialized: %d", err);
		return -EFAULT;
	}
	err = slm_at_icmp_init();
	if (err) {
		LOG_ERR("ICMP could not be initialized: %d", err);
		return -EFAULT;
	}
#if defined(CONFIG_SLM_SMS)
	err = slm_at_sms_init();
	if (err) {
		LOG_ERR("SMS could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
	err = slm_at_fota_init();
	if (err) {
		LOG_ERR("FOTA could not be initialized: %d", err);
		return -EFAULT;
	}
#if defined(CONFIG_SLM_NRF_CLOUD)
	err = slm_at_nrfcloud_init();
	if (err) {
		LOG_ERR("nRF Cloud could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
#if defined(CONFIG_SLM_GNSS)
	err = slm_at_gnss_init();
	if (err) {
		LOG_ERR("GNSS could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
#if defined(CONFIG_SLM_FTPC)
	err = slm_at_ftp_init();
	if (err) {
		LOG_ERR("FTP could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
#if defined(CONFIG_SLM_MQTTC)
	err = slm_at_mqtt_init();
	if (err) {
		LOG_ERR("MQTT could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
#if defined(CONFIG_SLM_HTTPC)
	err = slm_at_httpc_init();
	if (err) {
		LOG_ERR("HTTP could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
#if defined(CONFIG_SLM_GPIO)
	err = slm_at_gpio_init();
	if (err) {
		LOG_ERR("GPIO could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
#if defined(CONFIG_SLM_TWI)
	err = slm_at_twi_init();
	if (err) {
		LOG_ERR("TWI could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
#if defined(CONFIG_SLM_CARRIER)
	err = slm_at_carrier_init();
	if (err) {
		LOG_ERR("LwM2M carrier could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
#if defined(CONFIG_SLM_CMUX)
	slm_cmux_init();
#endif
#if defined(CONFIG_SLM_PPP)
	err = slm_ppp_init();
	if (err) {
		LOG_ERR("PPP initialization failed. (%d)", err);
		return err;
	}
#endif
	return err;
}

void slm_at_uninit(void)
{
	int err;

	err = slm_at_tcp_proxy_uninit();
	if (err) {
		LOG_WRN("TCP Server could not be uninitialized: %d", err);
	}
	err = slm_at_udp_proxy_uninit();
	if (err) {
		LOG_WRN("UDP Server could not be uninitialized: %d", err);
	}
	err = slm_at_socket_uninit();
	if (err) {
		LOG_WRN("TCPIP could not be uninitialized: %d", err);
	}
	err = slm_at_icmp_uninit();
	if (err) {
		LOG_WRN("ICMP could not be uninitialized: %d", err);
	}
#if defined(CONFIG_SLM_SMS)
	err = slm_at_sms_uninit();
	if (err) {
		LOG_WRN("SMS could not be uninitialized: %d", err);
	}
#endif
	err = slm_at_fota_uninit();
	if (err) {
		LOG_WRN("FOTA could not be uninitialized: %d", err);
	}
#if defined(CONFIG_SLM_NRF_CLOUD)
	err = slm_at_nrfcloud_uninit();
	if (err) {
		LOG_WRN("nRF Cloud could not be uninitialized: %d", err);
	}
#endif
#if defined(CONFIG_SLM_GNSS)
	err = slm_at_gnss_uninit();
	if (err) {
		LOG_WRN("GNSS could not be uninitialized: %d", err);
	}
#endif
#if defined(CONFIG_SLM_FTPC)
	err = slm_at_ftp_uninit();
	if (err) {
		LOG_WRN("FTP could not be uninitialized: %d", err);
	}
#endif
#if defined(CONFIG_SLM_MQTTC)
	err = slm_at_mqtt_uninit();
	if (err) {
		LOG_WRN("MQTT could not be uninitialized: %d", err);
	}
#endif
#if defined(CONFIG_SLM_HTTPC)
	err = slm_at_httpc_uninit();
	if (err) {
		LOG_WRN("HTTP could not be uninitialized: %d", err);
	}
#endif
#if defined(CONFIG_SLM_TWI)
	err = slm_at_twi_uninit();
	if (err) {
		LOG_ERR("TWI could not be uninit: %d", err);
	}
#endif
#if defined(CONFIG_SLM_GPIO)
	err = slm_at_gpio_uninit();
	if (err) {
		LOG_ERR("GPIO could not be uninit: %d", err);
	}
#endif
#if defined(CONFIG_SLM_NRF52_DFU)
	err = slm_at_dfu_uninit();
	if (err) {
		LOG_ERR("DFU could not be uninitialized: %d", err);
	}
#endif
#if defined(CONFIG_SLM_CARRIER)
	err = slm_at_carrier_uninit();
	if (err) {
		LOG_ERR("LwM2M carrier could not be uninitialized: %d", err);
	}
#endif
}
