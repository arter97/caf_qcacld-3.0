/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "os_if_dp_stc.h"
#include "wlan_cfg80211.h"
#include "qdf_trace.h"
#include "wlan_dp_ucfg_api.h"

/* Short names for QCA vendor attributes */
#define FLOW_CLASSIFY_RESULT_FLOW_TUPLE		\
		QCA_WLAN_VENDOR_ATTR_FLOW_CLASSIFY_RESULT_FLOW_TUPLE
#define FLOW_CLASSIFY_RESULT_TRAFFIC_TYPE	\
		QCA_WLAN_VENDOR_ATTR_FLOW_CLASSIFY_RESULT_TRAFFIC_TYPE

const struct nla_policy
flow_tuple_policy[QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_MAX + 1] = {
	[QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_IPV4_SRC_ADDR] = {.type = NLA_U32},
	[QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_IPV4_DST_ADDR] = {.type = NLA_U32},
	[QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_IPV6_SRC_ADDR] =
				NLA_POLICY_EXACT_LEN(sizeof(struct in6_addr)),
	[QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_IPV6_DST_ADDR] =
				NLA_POLICY_EXACT_LEN(sizeof(struct in6_addr)),
	[QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_SRC_PORT] = {.type = NLA_U16},
	[QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_DST_PORT] = {.type = NLA_U16},
	[QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_PROTOCOL] = {.type = NLA_U8},
};

const struct nla_policy
flow_classify_result_policy[QCA_WLAN_VENDOR_ATTR_FLOW_CLASSIFY_RESULT_MAX  + 1] = {
	[FLOW_CLASSIFY_RESULT_FLOW_TUPLE] = {.type = NLA_NESTED },
	[FLOW_CLASSIFY_RESULT_TRAFFIC_TYPE] = {.type = NLA_U8},
};

QDF_STATUS os_if_dp_flow_classify_result(struct wiphy *wiphy, const void *data,
					 int data_len)
{
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_FLOW_CLASSIFY_RESULT_MAX + 1];
	struct nlattr *flow_tuple_tb[QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_MAX + 1];
	struct wlan_dp_stc_flow_classify_result flow_classify_result;
	uint32_t ipv4_src_attr = QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_IPV4_SRC_ADDR;
	uint32_t ipv4_dst_attr = QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_IPV4_DST_ADDR;
	uint32_t ipv6_src_attr = QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_IPV6_SRC_ADDR;
	uint32_t ipv6_dst_attr = QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_IPV6_DST_ADDR;
	uint32_t nest_attr_id, attr_id;
	int ret;

	attr_id = QCA_WLAN_VENDOR_ATTR_FLOW_CLASSIFY_RESULT_MAX;
	if (wlan_cfg80211_nla_parse(tb, attr_id, data, data_len,
				    flow_classify_result_policy)) {
		osif_err("STC: Invalid flow classify result attributes");
		return QDF_STATUS_E_INVAL;
	}

	/* Validate and extract the flow tuple */
	nest_attr_id = FLOW_CLASSIFY_RESULT_FLOW_TUPLE;
	if (!tb[nest_attr_id]) {
		osif_err("STC: flow tuple not specified");
		return QDF_STATUS_E_INVAL;
	}

	attr_id = QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_MAX;
	ret = wlan_cfg80211_nla_parse_nested(flow_tuple_tb, attr_id,
					     tb[nest_attr_id],
					     flow_tuple_policy);
	if (ret) {
		osif_err("STC: parsing flow tuple failed");
		return QDF_STATUS_E_INVAL;
	}

	/* flow_tuple: Extract IPv4/IPv6 SRC & DST address */
	if (flow_tuple_tb[ipv4_src_attr] &&
	    flow_tuple_tb[ipv4_dst_attr]) {
		flow_classify_result.flow_tuple.src_ip.ipv4_addr =
				nla_get_u32(flow_tuple_tb[ipv4_src_attr]);
		flow_classify_result.flow_tuple.dst_ip.ipv4_addr =
				nla_get_u32(flow_tuple_tb[ipv4_dst_attr]);
	} else if (flow_tuple_tb[ipv6_src_attr] &&
		   flow_tuple_tb[ipv6_dst_attr]) {
		struct in6_addr ipv6_addr;

		ipv6_addr = nla_get_in6_addr(flow_tuple_tb[ipv6_src_attr]);
		qdf_mem_copy(flow_classify_result.flow_tuple.src_ip.ipv6_addr,
			     ipv6_addr.s6_addr32, sizeof(struct in6_addr));

		ipv6_addr = nla_get_in6_addr(flow_tuple_tb[ipv6_dst_attr]);
		qdf_mem_copy(flow_classify_result.flow_tuple.dst_ip.ipv6_addr,
			     ipv6_addr.s6_addr32, sizeof(struct in6_addr));
	} else {
		osif_err("STC: IP address not present in flow tuple");
		return QDF_STATUS_E_INVAL;
	}

	/* flow_tuple: Extract source port */
	attr_id = QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_SRC_PORT;
	if (!flow_tuple_tb[attr_id]) {
		osif_err("STC: source port missing in flow tuple");
		return QDF_STATUS_E_INVAL;
	}
	flow_classify_result.flow_tuple.src_port =
					nla_get_u16(flow_tuple_tb[attr_id]);

	/* flow_tuple: Extract destination port */
	attr_id = QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_DST_PORT;
	if (!flow_tuple_tb[attr_id]) {
		osif_err("STC: destination port missing in flow tuple");
		return QDF_STATUS_E_INVAL;
	}
	flow_classify_result.flow_tuple.dst_port =
					nla_get_u16(flow_tuple_tb[attr_id]);

	/* flow_tuple: Extract protocol */
	attr_id = QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_PROTOCOL;
	if (!flow_tuple_tb[attr_id]) {
		osif_err("STC: Protocol missing in flow tuple");
		return QDF_STATUS_E_INVAL;
	}
	flow_classify_result.flow_tuple.proto =
					nla_get_u16(flow_tuple_tb[attr_id]);

	/* Validate and extract the traffic type */
	attr_id = FLOW_CLASSIFY_RESULT_TRAFFIC_TYPE;
	if (!tb[attr_id]) {
		osif_err("STC: traffic type not specified");
		return QDF_STATUS_E_INVAL;
	}
	flow_classify_result.traffic_type = nla_get_u8(tb[attr_id]);

	ucfg_dp_flow_classify_result(&flow_classify_result);

	return QDF_STATUS_SUCCESS;
}
