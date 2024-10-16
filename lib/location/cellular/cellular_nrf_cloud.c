/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#if defined(CONFIG_NRF_CLOUD_MQTT)
#include <net/nrf_cloud_location.h>
#else
#include <net/nrf_cloud_rest.h>
#endif

#include "cellular_service.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

/* Verify that MQTT or REST is enabled */
BUILD_ASSERT(
	IS_ENABLED(CONFIG_NRF_CLOUD_MQTT) || IS_ENABLED(CONFIG_NRF_CLOUD_REST),
	"CONFIG_NRF_CLOUD_MQTT or CONFIG_NRF_CLOUD_REST transport must be enabled");


#if defined(CONFIG_NRF_CLOUD_MQTT)
/* Verify that LOCATION is enabled if MQTT is enabled */
BUILD_ASSERT(
	IS_ENABLED(CONFIG_NRF_CLOUD_LOCATION),
	"If CONFIG_NRF_CLOUD_MQTT is enabled also CONFIG_NRF_CLOUD_LOCATION must be enabled");

static struct location_data nrf_cloud_location;
static K_SEM_DEFINE(location_ready, 0, 1);
#endif

#if defined(CONFIG_NRF_CLOUD_MQTT)
static void location_service_location_ready_cb(const struct nrf_cloud_location_result *const result)
{
	if ((result != NULL) && (result->err == NRF_CLOUD_ERROR_NONE)) {
		nrf_cloud_location.latitude = result->lat;
		nrf_cloud_location.longitude = result->lon;
		nrf_cloud_location.accuracy = (double)result->unc;

		k_sem_give(&location_ready);
	} else {
		if (result) {
			LOG_ERR("Unable to determine location from cellular data, error: %d",
				result->err);
		}
		/* Reset the semaphore to unblock cellular_nrf_cloud_pos_get()
		 * and make it return an error.
		 */
		k_sem_reset(&location_ready);
	}
}

int cellular_nrf_cloud_pos_get(
	const struct location_cellular_serv_pos_req *params,
	char * const rcv_buf,
	const size_t rcv_buf_len,
	struct location_data *const location)
{
	ARG_UNUSED(rcv_buf);
	ARG_UNUSED(rcv_buf_len);

	int err;

	k_sem_reset(&location_ready);

	LOG_DBG("Sending cellular positioning request (MQTT)");
	err = nrf_cloud_location_request(
		params->cell_data, NULL, true, location_service_location_ready_cb);
	if (err == -EACCES) {
		LOG_ERR("Cloud connection is not established");
		return err;
	} else if (err) {
		LOG_ERR("Failed to request cellular positioning data, error: %d", err);
		return err;
	}

	LOG_INF("Cellular positioning request sent");

	if (k_sem_take(&location_ready, K_MSEC(params->timeout)) == -EAGAIN) {
		LOG_ERR("Cellular positioning data request timed out or "
			"cloud did not return a location");
		return -ETIMEDOUT;
	}

	*location = nrf_cloud_location;

	return err;
}
#else /* defined(CONFIG_NRF_CLOUD_MQTT) */
int cellular_nrf_cloud_pos_get(
	const struct location_cellular_serv_pos_req *params,
	char * const rcv_buf,
	const size_t rcv_buf_len,
	struct location_data *const location)
{
	int err;
	struct nrf_cloud_location_result result;
	struct nrf_cloud_rest_context rest_ctx = {
		.connect_socket = -1,
		.keep_alive = false,
		.timeout_ms = params->timeout,
		.rx_buf = rcv_buf,
		.rx_buf_len = rcv_buf_len,
		.fragment_size = 0
	};
	const struct nrf_cloud_rest_location_request loc_req = {
		.cell_info = (struct lte_lc_cells_info *)params->cell_data,
		.wifi_info = NULL
	};

	LOG_DBG("Sending cellular positioning request (REST)");
	err = nrf_cloud_rest_location_get(&rest_ctx, &loc_req, &result);
	if (!err) {
		location->accuracy = (double)result.unc;
		location->latitude = result.lat;
		location->longitude = result.lon;
	}

	return err;
}
#endif /* defined(CONFIG_NRF_CLOUD_MQTT) */
