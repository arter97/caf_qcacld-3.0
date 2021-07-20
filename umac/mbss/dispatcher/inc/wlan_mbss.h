/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* DOC: wlan_mbss.h
 * This file provides prototypes of the functions
 * exposed to be called from other components
 */

#ifndef _WLAN_MBSS_H_
#define _WLAN_MBSS_H_

#include "wlan_if_mgr_public_struct.h"
#include "qdf_status.h"
#include "wlan_objmgr_vdev_obj.h"

typedef void (*if_mgr_ev_cb_t)(struct wlan_objmgr_vdev *vdev,
			       void *data);

/**
 * enum wlan_mbss_acs_source: MBSS ACS event sources
 * @MBSS_ACS_SRC_CFG: ACS event from cfg80211
 * @MBSS_ACS_SRC_MLME: ACS event from MLME
 * @MBSS_ACS_SRC_DCS: ACS event from DCS
 * @MBSS_ACS_SRC_SPECTRAL: ACS event from spectral
 * @MBSS_ACS_SRC_BG_SCAN: ACS event from background scan
 * @MBSS_ACS_SRC_MAX: ACS event max source
 */
enum wlan_mbss_acs_source {
	MBSS_ACS_SRC_CFG = 0,
	MBSS_ACS_SRC_MLME = 1,
	MBSS_ACS_SRC_DCS = 2,
	MBSS_ACS_SRC_SPECTRAL = 3,
	MBSS_ACS_SRC_BG_SCAN = 4,
	MBSS_ACS_SRC_MAX = 5,
};

/**
 * enum wlan_mbss_ht40_source: MBSS HT40 event sources
 * @MBSS_HT40_SRC_MLME: HT40 event from MLME
 * @MBSS_HT40_SRC_MAX: HT40 event max source
 */
enum wlan_mbss_ht40_source {
	MBSS_HT40_SRC_MLME = 0,
	MBSS_HT40_SRC_MAX = 1,
};

/**
 * struct wlan_mbss_ev_data: MBSS event data
 * @if_mgr_event: interface manager event
 * @ext_cb: cb for handling the event response
 * @source: source of the event
 * @arg: argument to be passed to the cb
 * @status: status for the event notification
 */
struct wlan_mbss_ev_data {
	enum wlan_if_mgr_evt if_mgr_event;
	if_mgr_ev_cb_t ext_cb;
	union {
		enum wlan_mbss_acs_source acs_source;
		enum wlan_mbss_ht40_source ht40_source;
	} source;
	void *arg;
	QDF_STATUS status;
};

/**
 * struct mbss_ext_cb - Legacy action callbacks
 * @mbss_start_acs: Trigger ext ACS scan
 * @mbss_cancel_acs: Trigger ext ACS cancel
 * @mbss_start_ht40: Notify HT40 intolerance scan
 * @mbss_cancel_ht40: Notify HT40 intolerance scan
 */
struct wlan_mbss_ext_cb {
	QDF_STATUS (*mbss_start_acs)(
			struct wlan_objmgr_vdev *vdev, void *event_data);
	QDF_STATUS (*mbss_cancel_acs)(
			struct wlan_objmgr_vdev *vdev, void *event_data);
	QDF_STATUS (*mbss_start_ht40)(
			struct wlan_objmgr_vdev *vdev, void *event_data);
	QDF_STATUS (*mbss_cancel_ht40)(
			struct wlan_objmgr_vdev *vdev, void *event_data);
};

/**
 * struct mbss_ops - MBSS callbacks
 * @ext_ops: MBSS ext ops
 */
struct wlan_mbss_ops {
	struct wlan_mbss_ext_cb ext_ops;
};

/* wlan_mbss_alloc_ops() - alloc MBSS ops
 *
 * Allocate memory for MBSS ops
 *
 * Return: QDF_STATUS
 */

QDF_STATUS wlan_mbss_alloc_ops(void);

/* wlan_mbss_free_ops() - free MBSS ops
 *
 * Free memory for MSS ops
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_mbss_free_ops(void);

/* wlan_mbss_get_ops() - get MBSS ops
 *
 * Get the MBSS ops pointer
 *
 * Return: QDF_STATUS
 */
struct wlan_mbss_ops *wlan_mbss_get_ops(void);

/* wlan_mbss_set_ext_ops() - Set MBSS ext ops
 *
 * Set the MBSS ext ops pointer
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_mbss_set_ext_ops(struct wlan_mbss_ext_cb *ext_ops);

/* wlan_mbss_get_ext_ops() - get MBSS ext ops
 *
 * Get the MBSS ext ops pointer
 *
 * Return: MBSS ext ops
 */
struct wlan_mbss_ext_cb *wlan_mbss_get_ext_ops(void);

/* wlan_mbss_init(): Initialize the MBSS framework
 *
 * return: none
 */
QDF_STATUS wlan_mbss_init(void);

/* wlan_mbss_deinit() - Deinitialize the MBSS framework
 *
 * return: none
 */
QDF_STATUS wlan_mbss_deinit(void);

/* wlan_mbss_if_mgr_send_event() - Deliver if manager event to MBSS framework
 *
 * @vdev: vdev object
 * @data: event data
 *
 * return: QDF_STATUS
 */
QDF_STATUS wlan_mbss_if_mgr_send_event(struct wlan_objmgr_vdev *vdev,
				       void *data);

#endif /* _WLAN_MBSS_H_ */
