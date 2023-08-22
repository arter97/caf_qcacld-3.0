/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

/**
 * DOC: contains ll_lt_sap declarations specific to the bearer
 * switch functionalities
 */

#ifndef _WLAN_LL_LT_SAP_BEARER_SWITCH_H_
#define _WLAN_LL_LT_SAP_BEARER_SWITCH_H_

#include "wlan_ll_sap_public_structs.h"
#include <qdf_atomic.h>
#include "wlan_cmn.h"
#include "wlan_ll_sap_main.h"

/**
 * enum bearer_switch_status: Bearer switch request status
 * @XPAN_BLE_SWITCH_INIT: Init status
 * @XPAN_BLE_SWITCH_SUCCESS: Bearer switch success
 * @XPAN_BLE_SWITCH_REJECTED: Bearer switch is rejected
 * @XPAN_BLE_SWITCH_TIMEOUT: Bearer switch request timed out
 */
enum bearer_switch_status {
	XPAN_BLE_SWITCH_INIT,
	XPAN_BLE_SWITCH_SUCCESS,
	XPAN_BLE_SWITCH_REJECTED,
	XPAN_BLE_SWITCH_TIMEOUT,
};

/**
 * struct bearer_switch_request - Data structure to store the bearer switch
 * request
 * @requester_cb: Callback which needs to be invoked to indicate the status of
 * the request to the requester, this callback will be passed by the requester
 * when requester will send the request.
 * @arg: Argument passed by requester, this will be returned back in the
 * callback
 * @request_id: Unique value to identify the request
 */

struct bearer_switch_request {
	requester_callback requester_cb;
	void *arg;
	uint32_t request_id;
};

/**
 * struct bearer_switch_info - Data structure to store the bearer switch
 * requests and related information
 * @request_id: Last allocated request id
 * @ref_count: Reference count corresponding to each vdev and requester
 * @last_status: last status of the bearer switch request
 * @requests: Array of bearer_switch_requests to cache the request information
 */
struct bearer_switch_info {
	qdf_atomic_t request_id;
	uint8_t ref_count[WLAN_UMAC_PSOC_MAX_VDEVS][XPAN_BLE_SWITCH_REQUESTER_MAX];
	enum bearer_switch_status last_status;
	struct bearer_switch_request requests[MAX_BEARER_SWITCH_REQUESTERS];
};

/**
 * ll_lt_sap_bearer_switch_get_id() - Get the request id for bearer switch
 * request
 * @vdev: Pointer to vdev
 * Return: Bearer switch request id
 */
uint32_t ll_lt_sap_bearer_switch_get_id(struct wlan_objmgr_vdev *vdev);
#endif /* _WLAN_LL_LT_SAP_BEARER_SWITCH_H_ */
