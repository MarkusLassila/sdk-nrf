/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file downloader_transport.h
 *
 * @ingroup downloader
 * @{
 * @brief Downloader transport definition.
 */

#ifndef DOWNLOADER_TRANSPORT_H
#define DOWNLOADER_TRANSPORT_H

#include <net/downloader.h>

int dl_transport_evt_data(struct downloader *dl, void *data, size_t len);

/**
 * Downloader transport API
 */
struct dl_transport {
	/**
	 * Parse protocol
	 *
	 * @param dl Downloader instance.
	 * @param uri URI
	 *
	 * @retval true if protocol is supported by the transport
	 * @retval false if protocol is not supported by the transport
	 */
	bool (*proto_supported)(struct downloader *dl, const char *uri);
	/**
	 * Initialize DLC transport
	 *
	 * @param dl Downloader instance.
	 * @param host_cfg Host configuration.
	 * @param URI URI
	 *
	 * @returns 0 on success, negative error on failure.
	 */
	int (*init)(struct downloader *dl, struct downloader_host_cfg *host_cgf, const char *uri);
	/**
	 * Deinitialize DLC transport
	 *
	 * @param dl Downloader instance.
	 *
	 * @returns 0 on success, negative error on failure.
	 */
	int (*deinit)(struct downloader *dl);
	/**
	 * Connect DLC transport.
	 *
	 * Connection result is given by callback to @c dl_transport_event_connected.
	 *
	 * @param dl Downloader instance.
	 *
	 * @returns 0 on success, negative error on failure.
	 */
	int (*connect)(struct downloader *dl);
	/**
	 * Close DLC transport
	 *
	 * @param dl Downloader instance.
	 *
	 * @returns 0 on success, negative error on failure.
	 */
	int (*close)(struct downloader *dl);
	/**
	 * Download data with DLC transport
	 *
	 * @param dl Downloader instance.
	 *
	 * @returns 0 on success, negative error on failure.
	 * Return -ECONNRESET if the downloader can reconnect to resume the download.
	 */
	int (*download)(struct downloader *dl);
};

/** Downloader transport entry */
struct dl_transport_entry {
	/** Transport */
	struct dl_transport *transport;
};

/**
 * @brief Define a DLC transport.
 *
 * @param entry The entry name.
 * @param _transport The transport.
 */
#define DLC_TRANSPORT(entry, _transport)                                                           \
	static STRUCT_SECTION_ITERABLE(dl_transport_entry, entry) = {                              \
		.transport = _transport,                                                           \
	}

#endif /* DOWNLOADER_TRANSPORT_H */

/**@} */
