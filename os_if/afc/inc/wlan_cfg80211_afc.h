/*
 * Copyright (c) 2022, Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.

 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * DOC: wlan_cfg80211_afc.h
 *
 * This Header file provide declaration for cfg80211 command handler API
 */

#ifndef __WLAN_CFG80211_AFC_H__
#define __WLAN_CFG80211_AFC_H__

#include <wlan_objmgr_cmn.h>
#include <qdf_types.h>
#include <net/cfg80211.h>
#include <qca_vendor.h>

#ifdef CONFIG_AFC_SUPPORT
extern const struct nla_policy
wlan_cfg80211_afc_response_policy[QCA_WLAN_VENDOR_ATTR_AFC_RESP_MAX + 1];

#define QCA_NL80211_AFC_RESP_BUF_MAX_SIZE 4000

/**
 * wlan_cfg80211_afc_response() - Interface to handle AFC response received
 * from user space
 * @wiphy: Pointer to wiphy object
 * @wdev: Pointer to wireless device
 * @data: Pointer to data from user space
 * @data_len: Data length
 *
 * Return: 0 if success, otherwise error code
 */
int wlan_cfg80211_afc_response(struct wiphy *wiphy,
			       struct wireless_dev *wdev,
			       const void *data,
			       int data_len);

#define FEATURE_AFC_VENDOR_COMMANDS \
{ \
	.info.vendor_id = QCA_NL80211_VENDOR_ID, \
	.info.subcmd = QCA_NL80211_VENDOR_SUBCMD_AFC_RESPONSE, \
	.flags = 0, \
	.doit = wlan_cfg80211_afc_response, \
	vendor_command_policy(wlan_cfg80211_afc_response_policy, \
			      QCA_WLAN_VENDOR_ATTR_AFC_RESP_MAX) \
},

#define FEATURE_AFC_VENDOR_EVENTS \
[QCA_NL80211_VENDOR_SUBCMD_AFC_EVENT_INDEX] = {            \
	.vendor_id = QCA_NL80211_VENDOR_ID,                \
	.subcmd = QCA_NL80211_VENDOR_SUBCMD_AFC_EVENT,     \
},

/**
 * wlan_cfg80211_trigger_afc_event() - Interface to send AFC event to user space
 * @pdev: Pointer to pdev object
 * @event_type: AFC event type
 * @afc_data: AFC data send to user space
 *
 * Return: 0 if success, otherwise error code
 */
int
wlan_cfg80211_trigger_afc_event(struct wlan_objmgr_pdev *pdev,
				enum qca_wlan_vendor_afc_event_type event_type,
				void *afc_data);
#else
#define FEATURE_AFC_VENDOR_COMMANDS
#define FEATURE_AFC_VENDOR_EVENTS
#endif
#endif
