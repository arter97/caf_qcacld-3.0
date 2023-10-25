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
 * DOC: contains policy manager ll_sap definitions specific to the ll_sap module
 */

#include "wlan_objmgr_psoc_obj.h"

#ifdef WLAN_FEATURE_LL_LT_SAP
/**
 * wlan_policy_mgr_get_ll_lt_sap_vdev_id() - Get ll_lt_sap vdev id
 * @psoc: PSOC object
 *
 * API to find ll_lt_sap vdev id
 *
 * Return: vdev id
 */
uint8_t wlan_policy_mgr_get_ll_lt_sap_vdev_id(struct wlan_objmgr_psoc *psoc);

/**
 * __policy_mgr_is_ll_lt_sap_restart_required() - Check in ll_lt_sap restart is
 * required
 * @psoc: PSOC object
 * @func: Function pointer of the caller function.
 *
 * This API checks if ll_lt_sap restart is required or not
 *
 * Return: true/false
 */
bool __policy_mgr_is_ll_lt_sap_restart_required(struct wlan_objmgr_psoc *psoc,
						const char *func);

#define policy_mgr_is_ll_lt_sap_restart_required(psoc) \
	__policy_mgr_is_ll_lt_sap_restart_required(psoc, __func__)
#else

static inline bool
policy_mgr_is_ll_lt_sap_restart_required(struct wlan_objmgr_psoc *psoc)
{
	return false;
}

static inline
uint8_t wlan_policy_mgr_get_ll_lt_sap_vdev_id(struct wlan_objmgr_psoc *psoc)
{
	return WLAN_INVALID_VDEV_ID;
}
#endif
