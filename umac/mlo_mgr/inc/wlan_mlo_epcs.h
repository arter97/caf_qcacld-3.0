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
 * DOC: contains EPCS APIs
 */

#ifndef _WLAN_MLO_EPCS_H_
#define _WLAN_MLO_EPCS_H_

#include <wlan_cmn_ieee80211.h>
#include <wlan_mlo_mgr_public_structs.h>
#ifdef WMI_AP_SUPPORT
#include <wlan_cmn.h>
#endif

/**
 * enum wlan_epcs_category - epcs category
 *
 * @WLAN_EPCS_CATEGORY_NONE: none
 * @WLAN_EPCS_CATEGORY_REQUEST: EPCS request
 * @WLAN_EPCS_CATEGORY_RESPONSE: EPCS response
 * @WLAN_EPCS_CATEGORY_TEARDOWN: EPCS teardown
 * @WLAN_EPCS_CATEGORY_INVALID: Invalid
 */
enum wlan_epcs_category {
	WLAN_EPCS_CATEGORY_NONE = 0,
	WLAN_EPCS_CATEGORY_REQUEST = 3,
	WLAN_EPCS_CATEGORY_RESPONSE = 4,
	WLAN_EPCS_CATEGORY_TEARDOWN = 5,
	WLAN_EPCS_CATEGORY_INVALID,
};

#define epcs_alert(format, args...) \
		QDF_TRACE_FATAL(QDF_MODULE_ID_EPCS, format, ## args)

#define epcs_err(format, args...) \
		QDF_TRACE_ERROR(QDF_MODULE_ID_EPCS, format, ## args)

#define epcs_warn(format, args...) \
		QDF_TRACE_WARN(QDF_MODULE_ID_EPCS, format, ## args)

#define epcs_info(format, args...) \
		QDF_TRACE_INFO(QDF_MODULE_ID_EPCS, format, ## args)

#define epcs_debug(format, args...) \
		QDF_TRACE_DEBUG(QDF_MODULE_ID_EPCS, format, ## args)

#define epcs_rl_debug(format, args...) \
		QDF_TRACE_DEBUG_RL(QDF_MODULE_ID_EPCS, format, ## args)

#endif /* _WLAN_MLO_EPCS_H_ */
