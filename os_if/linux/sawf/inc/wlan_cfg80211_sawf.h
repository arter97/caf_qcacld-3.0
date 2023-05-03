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

#ifndef _WLAN_CFG80211_SAWF_H_
#define _WLAN_CFG80211_SAWF_H_

#include <qdf_types.h>
#include <wlan_cfg80211.h>

/*
 * Enum for SAWF rule operation
 */
enum {
	WLAN_CFG80211_SAWF_RULE_ADD,
	WLAN_CFG80211_SAWF_RULE_DELETE,
};

/*
 * sawf_rule_config_intf
 *
 * @rule_id: Rule id
 * @req_type: Request type
 * @output_tid: TID
 * @classifier_type: Classifier type
 * @service_class_id: Service class ID
 *
 * @dst_mac: Destination MAC address
 * @src_mac: Source MAC address
 */
struct sawf_rule_config_intf {
	uint32_t rule_id;
	uint8_t req_type;
	uint8_t output_tid;
	uint8_t classifier_type;
	uint8_t service_class_id;

	uint8_t dst_mac[QDF_MAC_ADDR_SIZE];
	uint8_t src_mac[QDF_MAC_ADDR_SIZE];
};

/*
 * wlan_cfg80211_sawf_send_rule() - Send rule attributs to user space client
 *
 * @whipy: Wiphy
 * @wdev: wireless dev
 * @rule: Rule attributes
 *
 * Return: void
 */
void wlan_cfg80211_sawf_send_rule(struct wiphy *wiphy,
				  struct wireless_dev *wdev,
				  struct sawf_rule_config_intf *rule);
#endif /* _WLAN_CFG80211_SAWF_H_ */
