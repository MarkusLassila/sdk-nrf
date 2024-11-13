/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file downloader_transport_http.h
 *
 * @ingroup downloader
 * @{
 * @brief Downloader transport HTTP definitions.
 */

#ifndef __DOWNLOADER_TRANSPORT_HTTP_H
#define __DOWNLOADER_TRANSPORT_HTTP_H

#include <net/downloader.h>

struct downloader_transport_http_conf {
	/** Socket receive timeout */
	uint32_t sock_recv_timeo;
};

/**
 * @brief Set Downloader HTTP transport settings
 *
 * @param dl downloader instance
 * @param conf HTTP transport configuration
 *
 * @return Zero on success, negative errno on failure.
 */
int downloader_transport_http_set_config(struct downloader *dl,
					 struct downloader_transport_http_conf *conf);

#endif /* __DOWNLOADER_TRANSPORT_HTTP_H */

/**@} */
