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
#include <wlan_osif_priv.h>
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

static inline bool is_flow_tuple_ipv4(struct flow_info *flow_tuple)
{
	if (qdf_likely((flow_tuple->flags | DP_FLOW_TUPLE_FLAGS_IPV4)))
		return true;

	return false;
}

static inline bool is_flow_tuple_ipv6(struct flow_info *flow_tuple)
{
	if (qdf_likely((flow_tuple->flags | DP_FLOW_TUPLE_FLAGS_IPV6)))
		return true;

	return false;
}

static inline void os_if_dp_print_tuple(struct sk_buff *flow_sample_event,
					struct flow_info *flow_tuple)
{
	if (!flow_sample_event)
		return;

	osif_err("STC: Flow tuple:");
	if (is_flow_tuple_ipv4(flow_tuple)) {
		osif_err("STC: ipv4 src addr 0x%x",
			 flow_tuple->src_ip.ipv4_addr);
		osif_err("STC: ipv4 dst addr 0x%x",
			 flow_tuple->dst_ip.ipv4_addr);
	} else if (is_flow_tuple_ipv6(flow_tuple)) {
		osif_err("STC: ipv6 src addr 0x%x 0x%x 0x%x 0x%x",
			 flow_tuple->src_ip.ipv6_addr[0],
			 flow_tuple->src_ip.ipv6_addr[1],
			 flow_tuple->src_ip.ipv6_addr[2],
			 flow_tuple->src_ip.ipv6_addr[3]);
		osif_err("STC: ipv6 dst addr 0x%x 0x%x 0x%x 0x%x",
			 flow_tuple->dst_ip.ipv6_addr[0],
			 flow_tuple->dst_ip.ipv6_addr[1],
			 flow_tuple->dst_ip.ipv6_addr[2],
			 flow_tuple->dst_ip.ipv6_addr[3]);
	}

	osif_err("STC: src port %d", flow_tuple->src_port);
	osif_err("STC: dst port %d", flow_tuple->dst_port);
	osif_err("STC: proto %d", flow_tuple->proto);
}

static inline void
os_if_dp_print_txrx_stats(struct sk_buff *flow_sample_event,
			  struct wlan_dp_stc_txrx_stats *txrx_stats)
{
	if (!flow_sample_event)
		return;

	osif_err("STC: TXRX Stats:");
	osif_err("STC: bytes %lld", txrx_stats->bytes);
	osif_err("STC: pkts %d", txrx_stats->pkts);
	osif_err("STC: pkt_size_min %d", txrx_stats->pkt_size_min);
	osif_err("STC: pkt_size_max %d", txrx_stats->pkt_size_max);
	osif_err("STC: pkt_iat_min %lld", txrx_stats->pkt_iat_min);
	osif_err("STC: pkt_iat_max %lld", txrx_stats->pkt_iat_max);
	osif_err("STC: pkt_iat_sum %lld", txrx_stats->pkt_iat_sum);
}

static inline void
os_if_dp_print_burst_stats(struct sk_buff *flow_sample_event,
			   struct wlan_dp_stc_burst_stats *burst_stats)
{
	if (!flow_sample_event)
		return;

	osif_err("STC: Burst Stats:");
	osif_err("STC: burst_duration_min %d", burst_stats->burst_duration_min);
	osif_err("STC: burst_duration_max %d", burst_stats->burst_duration_max);
	osif_err("STC: burst_duration_sum %d", burst_stats->burst_duration_sum);
	osif_err("STC: burst_size_min %d", burst_stats->burst_size_min);
	osif_err("STC: burst_size_max %d", burst_stats->burst_size_max);
	osif_err("STC: burst_size_sum%d", burst_stats->burst_size_sum);
	osif_err("STC: burst_count %d", burst_stats->burst_count);
}

#define nla_nest_end_checked(skb, start) do {		\
	if ((skb) && (start))				\
		nla_nest_end(skb, start);		\
} while (0)

static inline int
os_if_dp_fill_flow_tuple(struct sk_buff *flow_sample_event,
			 struct flow_info *flow_tuple)
{
	struct nlattr *nla_attr;
	int len = 0;
	uint32_t attr_id;

	os_if_dp_print_tuple(flow_sample_event, flow_tuple);
	if (flow_sample_event) {
		attr_id = QCA_WLAN_VENDOR_ATTR_FLOW_STATS_FLOW_TUPLE;
		nla_attr = nla_nest_start(flow_sample_event, attr_id);
		if (!nla_attr) {
			osif_err("STC: Flow tuple nest start failed");
			goto fail;
		}
	} else {
		len += nla_total_size(0);
	}

	if (is_flow_tuple_ipv4(flow_tuple)) {
		if (flow_sample_event &&
		    nla_put_u32(flow_sample_event,
				QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_IPV4_SRC_ADDR,
				flow_tuple->src_ip.ipv4_addr)) {
			osif_err("ipv4 src_addr nla put failed");
			goto fail;
		} else {
			len += nla_total_size(sizeof(u32));
		}

		if (flow_sample_event &&
		    nla_put_u32(flow_sample_event,
				QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_IPV4_DST_ADDR,
				flow_tuple->dst_ip.ipv4_addr)) {
			osif_err("ipv4 dst_addr nla put failed");
			goto fail;
		} else {
			len += nla_total_size(sizeof(u32));
		}
	} else if (is_flow_tuple_ipv6(flow_tuple)) {
		struct in6_addr ipv6_addr;

		qdf_mem_copy(ipv6_addr.s6_addr32, flow_tuple->src_ip.ipv6_addr,
			     sizeof(struct in6_addr));
		attr_id = QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_IPV4_SRC_ADDR;
		if (flow_sample_event &&
		    nla_put_in6_addr(flow_sample_event, attr_id, &ipv6_addr)) {
			osif_err("ipv6 src_addr nla put failed");
			goto fail;
		} else {
			len += nla_total_size(sizeof(u32) * 4);
		}

		qdf_mem_copy(ipv6_addr.s6_addr32, flow_tuple->dst_ip.ipv6_addr,
			     sizeof(struct in6_addr));
		attr_id = QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_IPV6_DST_ADDR;
		if (flow_sample_event &&
		    nla_put_in6_addr(flow_sample_event, attr_id, &ipv6_addr)) {
			osif_err("ipv6 dst_addr nla put failed");
			goto fail;
		} else {
			len += nla_total_size(sizeof(u32) * 4);
		}
	}

	if (flow_sample_event &&
	    nla_put_u16(flow_sample_event,
			QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_SRC_PORT,
			flow_tuple->src_port)) {
		osif_err("src_port nla put failed");
		goto fail;
	} else {
		len += nla_total_size(sizeof(u16));
	}

	if (flow_sample_event &&
	    nla_put_u16(flow_sample_event,
			QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_DST_PORT,
			flow_tuple->dst_port)) {
		osif_err("dst_port nla put failed");
		goto fail;
	} else {
		len += nla_total_size(sizeof(u16));
	}

	if (flow_sample_event &&
	    nla_put_u8(flow_sample_event,
		       QCA_WLAN_VENDOR_ATTR_FLOW_TUPLE_PROTOCOL,
		       flow_tuple->proto)) {
		osif_err("ip proto nla put failed");
		goto fail;
	} else {
		len += nla_total_size(sizeof(u8));
	}

	nla_nest_end_checked(flow_sample_event, nla_attr);
	return len;

fail:
	return -EINVAL;
}

typedef struct wlan_dp_stc_txrx_samples (*txrx_samples_t)[DP_TXRX_SAMPLES_WINDOW_MAX];

static inline int
os_if_dp_fill_txrx_stats(struct sk_buff *flow_sample_event,
			 struct wlan_dp_stc_txrx_stats *txrx_stats)
{
	int len = 0;
	uint32_t attr_id;

	os_if_dp_print_txrx_stats(flow_sample_event, txrx_stats);
	attr_id = QCA_WLAN_VENDOR_ATTR_TXRX_STATS_NUM_BYTES;
	if (flow_sample_event &&
	    wlan_cfg80211_nla_put_u64(flow_sample_event, attr_id,
				      txrx_stats->bytes)) {
		osif_err("STC: bytes put failed");
		goto fail;
	} else {
		len += nla_total_size(sizeof(u64));
	}

	attr_id = QCA_WLAN_VENDOR_ATTR_TXRX_STATS_NUM_PKTS;
	if (flow_sample_event &&
	    nla_put_u32(flow_sample_event, attr_id, txrx_stats->pkts)) {
		osif_err("STC: pkts put failed");
		goto fail;
	} else {
		len += nla_total_size(sizeof(u32));
	}

	attr_id = QCA_WLAN_VENDOR_ATTR_TXRX_STATS_PKT_SIZE_MIN;
	if (flow_sample_event &&
	    nla_put_u32(flow_sample_event, attr_id, txrx_stats->pkt_size_min)) {
		osif_err("STC: pkt_size_min put failed");
		goto fail;
	} else {
		len += nla_total_size(sizeof(u32));
	}

	attr_id = QCA_WLAN_VENDOR_ATTR_TXRX_STATS_PKT_SIZE_MAX;
	if (flow_sample_event &&
	    nla_put_u32(flow_sample_event, attr_id, txrx_stats->pkt_size_max)) {
		osif_err("STC: pkt_size_max put failed");
		goto fail;
	} else {
		len += nla_total_size(sizeof(u32));
	}

	attr_id = QCA_WLAN_VENDOR_ATTR_TXRX_STATS_PKT_IAT_MIN;
	if (flow_sample_event &&
	    wlan_cfg80211_nla_put_u64(flow_sample_event, attr_id,
				      txrx_stats->pkt_iat_min)) {
		osif_err("STC: pkt_iat_min put failed");
		goto fail;
	} else {
		len += nla_total_size(sizeof(u64));
	}

	attr_id = QCA_WLAN_VENDOR_ATTR_TXRX_STATS_PKT_IAT_MAX;
	if (flow_sample_event &&
	    wlan_cfg80211_nla_put_u64(flow_sample_event, attr_id,
				      txrx_stats->pkt_iat_max)) {
		osif_err("STC: pkt_iat_max put failed");
		goto fail;
	} else {
		len += nla_total_size(sizeof(u64));
	}

	attr_id = QCA_WLAN_VENDOR_ATTR_TXRX_STATS_PKT_IAT_SUM;
	if (flow_sample_event &&
	    wlan_cfg80211_nla_put_u64(flow_sample_event, attr_id,
				      txrx_stats->pkt_iat_sum)) {
		osif_err("STC: pkt_iat_sum put failed");
		goto fail;
	} else {
		len += nla_total_size(sizeof(u64));
	}

	return len;

fail:
	return -EINVAL;
}

static inline int
os_if_dp_fill_txrx_one_win_samples(struct sk_buff *flow_sample_event,
				   struct wlan_dp_stc_txrx_samples *win_sample)
{
	struct nlattr *stats_attr;
	int ret, len = 0;
	uint32_t attr_id;

	osif_err("STC: win size %d", win_sample->win_size);
	attr_id = QCA_WLAN_VENDOR_ATTR_TXRX_SAMPLES_WINDOWS_WINDOW_SIZE;
	if (flow_sample_event &&
	    nla_put_u32(flow_sample_event, attr_id, win_sample->win_size)) {
		osif_err("STC: window size put failed");
		goto fail;
	} else {
		len += nla_total_size(sizeof(u32));
	}

	attr_id = QCA_WLAN_VENDOR_ATTR_TXRX_SAMPLES_WINDOWS_UL_STATS;
	if (flow_sample_event) {
		stats_attr = nla_nest_start(flow_sample_event, attr_id);
		if (!stats_attr) {
			osif_err("STC: UL txrx stats start put failed");
			goto fail;
		}
	} else {
		len += nla_total_size(0);
	}

	ret = os_if_dp_fill_txrx_stats(flow_sample_event, &win_sample->tx);
	if (ret < 0)
		goto fail;
	len += ret;
	nla_nest_end_checked(flow_sample_event, stats_attr);

	if (flow_sample_event) {
		attr_id = QCA_WLAN_VENDOR_ATTR_TXRX_SAMPLES_WINDOWS_DL_STATS;
		stats_attr = nla_nest_start(flow_sample_event, attr_id);
		if (!stats_attr) {
			osif_err("STC: DL txrx stats start put failed");
			goto fail;
		}
	} else {
		len += nla_total_size(0);
	}

	ret = os_if_dp_fill_txrx_stats(flow_sample_event, &win_sample->rx);
	if (ret < 0)
		goto fail;
	len += ret;
	nla_nest_end_checked(flow_sample_event, stats_attr);

	return len;

fail:
	return -EINVAL;
}

static inline QDF_STATUS
os_if_dp_fill_txrx_win_samples(struct sk_buff *flow_sample_event,
			       struct wlan_dp_stc_txrx_samples *txrx_win_samples)
{
	struct nlattr *win_attr;
	int ret, len = 0;
	uint32_t i;

	for (i = 0; i < DP_TXRX_SAMPLES_WINDOW_MAX; i++) {
		if (flow_sample_event) {
			win_attr = nla_nest_start(flow_sample_event, i);
			if (!win_attr) {
				osif_err("STC: win array[%] start put failed",
					 i);
				goto fail;
			}
		} else {
			len += nla_total_size(0);
		}

		ret = os_if_dp_fill_txrx_one_win_samples(flow_sample_event,
							 &txrx_win_samples[i]);
		if (ret < 0)
			goto fail;
		len += ret;
		nla_nest_end_checked(flow_sample_event, win_attr);
	}

	return len;
fail:
	return -EINVAL;
}

static inline int
os_if_dp_fill_txrx_samples(struct sk_buff *flow_sample_event,
			   txrx_samples_t txrx_samples)
{
	struct nlattr *nla_attr, *txrx_samples_attr, *win_attr;
	int i, ret, len = 0;
	uint32_t attr_id;

	if (flow_sample_event) {
		attr_id = QCA_WLAN_VENDOR_ATTR_FLOW_STATS_TXRX_SAMPLES;
		nla_attr = nla_nest_start(flow_sample_event, attr_id);
		if (!nla_attr) {
			osif_err("STC: txrx samples start put failed");
			goto fail;
		}
	} else {
		len += nla_total_size(0);
	}

	for (i = 0; i < 5; i++) {
		if (flow_sample_event) {
			txrx_samples_attr = nla_nest_start(flow_sample_event,
							   i);
			if (!txrx_samples_attr) {
				osif_err("STC: txrx samples array start put failed");
				goto fail;
			}
		} else {
			len += nla_total_size(0);
		}

		if (flow_sample_event) {
			attr_id = QCA_WLAN_VENDOR_ATTR_TXRX_SAMPLES_WINDOWS;
			win_attr = nla_nest_start(flow_sample_event, attr_id);
			if (!win_attr) {
				osif_err("STC: txrx samples start put failed");
				goto fail;
			}
		} else {
			len += nla_total_size(0);
		}

		ret = os_if_dp_fill_txrx_win_samples(flow_sample_event,
						     txrx_samples[i]);
		if (ret < 0)
			goto fail;
		len += ret;

		nla_nest_end_checked(flow_sample_event, win_attr);
		nla_nest_end_checked(flow_sample_event, txrx_samples_attr);
	}

	nla_nest_end_checked(flow_sample_event, nla_attr);

	return len;

fail:
	return -EINVAL;
}

static inline int
os_if_dp_fill_burst_stats(struct sk_buff *flow_sample_event,
			  struct wlan_dp_stc_burst_stats *burst_stats)
{
	int len = 0;
	uint32_t attr_id;

	os_if_dp_print_burst_stats(flow_sample_event, burst_stats);
	if (flow_sample_event &&
	    nla_put_u32(flow_sample_event,
			QCA_WLAN_VENDOR_ATTR_BURST_STATS_BURST_DURATION_MIN,
			burst_stats->burst_duration_min)) {
		osif_err("STC: burst_duration_min put failed");
		goto fail;
	} else {
		len += nla_total_size(sizeof(u32));
	}

	if (flow_sample_event &&
	    nla_put_u32(flow_sample_event,
			QCA_WLAN_VENDOR_ATTR_BURST_STATS_BURST_DURATION_MAX,
			burst_stats->burst_duration_max)) {
		osif_err("STC: burst_duration_max put failed");
		goto fail;
	} else {
		len += nla_total_size(sizeof(u32));
	}

	attr_id = QCA_WLAN_VENDOR_ATTR_BURST_STATS_BURST_DURATION_SUM;
	if (flow_sample_event &&
	    wlan_cfg80211_nla_put_u64(flow_sample_event, attr_id,
				      burst_stats->burst_duration_sum)) {
		osif_err("STC: burst_duration_sum put failed");
		goto fail;
	} else {
		len += nla_total_size(sizeof(u64));
	}

	attr_id = QCA_WLAN_VENDOR_ATTR_BURST_STATS_BURST_SIZE_MIN;
	if (flow_sample_event &&
	    wlan_cfg80211_nla_put_u64(flow_sample_event, attr_id,
				      burst_stats->burst_size_min)) {
		osif_err("STC: burst_size_min put failed");
		goto fail;
	} else {
		len += nla_total_size(sizeof(u64));
	}

	attr_id = QCA_WLAN_VENDOR_ATTR_BURST_STATS_BURST_SIZE_MAX;
	if (flow_sample_event &&
	    wlan_cfg80211_nla_put_u64(flow_sample_event, attr_id,
				      burst_stats->burst_size_max)) {
		osif_err("STC: burst_size_max put failed");
		goto fail;
	} else {
		len += nla_total_size(sizeof(u64));
	}

	attr_id = QCA_WLAN_VENDOR_ATTR_BURST_STATS_BURST_SIZE_SUM;
	if (flow_sample_event &&
	    wlan_cfg80211_nla_put_u64(flow_sample_event, attr_id,
				      burst_stats->burst_size_sum)) {
		osif_err("STC: burst_size_sum put failed");
		goto fail;
	} else {
		len += nla_total_size(sizeof(u64));
	}

	if (flow_sample_event &&
	    nla_put_u32(flow_sample_event,
			QCA_WLAN_VENDOR_ATTR_BURST_STATS_BURST_COUNT,
			burst_stats->burst_count)) {
		osif_err("STC: burst_count put failed");
		goto fail;
	} else {
		len += nla_total_size(sizeof(u32));
	}

	return len;

fail:
	return -EINVAL;
}

static inline QDF_STATUS
os_if_dp_fill_burst_samples(struct sk_buff *flow_sample_event,
			    struct wlan_dp_stc_flow_samples *flow_samples)
{
	struct wlan_dp_stc_burst_samples *burst_sample;
	struct wlan_dp_stc_txrx_samples *txrx_samples;
	struct nlattr *burst_samples_attr, *nla_attr;
	int len = 0, ret;
	uint32_t attr_id;

	burst_sample = &flow_samples->burst_sample;
	txrx_samples = &burst_sample->txrx_samples;

	if (flow_sample_event) {
		attr_id = QCA_WLAN_VENDOR_ATTR_FLOW_STATS_BURST_SAMPLES;
		burst_samples_attr = nla_nest_start(flow_sample_event, attr_id);
		if (!burst_samples_attr) {
			osif_err("STC: burst_samples start put fail");
			goto fail;
		}
	} else {
		len += nla_total_size(0);
	}

	if (flow_sample_event) {
		attr_id = QCA_WLAN_VENDOR_ATTR_BURST_SAMPLES_TXRX_STATS;
		nla_attr = nla_nest_start(flow_sample_event, attr_id);
		if (!nla_attr) {
			osif_err("STC: burst_samples:txrx start put fail");
			goto fail;
		}
	} else {
		len += nla_total_size(0);
	}

	ret = os_if_dp_fill_txrx_one_win_samples(flow_sample_event,
						 txrx_samples);
	if (ret < 0)
		goto fail;
	len += ret;

	nla_nest_end_checked(flow_sample_event, nla_attr);

	if (flow_sample_event) {
		attr_id = QCA_WLAN_VENDOR_ATTR_BURST_SAMPLES_UL_BURST_STATS;
		nla_attr = nla_nest_start(flow_sample_event, attr_id);
		if (!nla_attr) {
			osif_err("STC: burst_samples:ul start put fail");
			goto fail;
		}
	} else {
		len += nla_total_size(0);
	}

	ret = os_if_dp_fill_burst_stats(flow_sample_event,
					&burst_sample->tx);
	if (ret < 0)
		goto fail;
	len += ret;

	nla_nest_end_checked(flow_sample_event, nla_attr);

	if (flow_sample_event) {
		attr_id = QCA_WLAN_VENDOR_ATTR_BURST_SAMPLES_DL_BURST_STATS;
		nla_attr = nla_nest_start(flow_sample_event, attr_id);
		if (!nla_attr) {
			osif_err("STC: burst_samples:dl start put fail");
			goto fail;
		}
	} else {
		len += nla_total_size(0);
	}

	ret = os_if_dp_fill_burst_stats(flow_sample_event, &burst_sample->rx);
	if (ret < 0)
		goto fail;
	len += ret;

	nla_nest_end_checked(flow_sample_event, nla_attr);

	nla_nest_end_checked(flow_sample_event, burst_samples_attr);

	return len;

fail:
	return -EINVAL;
}

static inline int
os_if_dp_flow_stats_update_or_get_len(struct sk_buff *flow_sample_event,
				      struct wlan_dp_stc_flow_samples *flow_samples,
				      uint32_t flags)
{
	int len = NLMSG_HDRLEN;
	int ret;

	ret = os_if_dp_fill_flow_tuple(flow_sample_event,
				       &flow_samples->flow_tuple);
	if (ret < 0)
		goto fail;
	len += ret;

	if (flags & WLAN_DP_TXRX_SAMPLES_READY) {
		ret = os_if_dp_fill_txrx_samples(flow_sample_event,
						 flow_samples->txrx_samples);
		if (ret < 0)
			goto fail;
		len += ret;
	}

	if (flags & WLAN_DP_BURST_SAMPLES_READY) {
		ret = os_if_dp_fill_burst_samples(flow_sample_event,
						  flow_samples);
		if (ret < 0)
			goto fail;
		len += ret;
	}

	return len;

fail:
	return -EINVAL;
}

static int
os_if_dp_send_flow_stats_event(struct wlan_objmgr_psoc *psoc,
			       struct wlan_dp_stc_flow_samples *flow_samples,
			       uint32_t flags)
{
	struct sk_buff *flow_sample_event;
	struct wlan_objmgr_pdev *pdev;
	struct pdev_osif_priv *os_priv;
	enum qca_nl80211_vendor_subcmds_index index =
				QCA_NL80211_VENDOR_SUBCMD_FLOW_STATS_INDEX;
	uint32_t event_len;
	int ret;

	pdev = wlan_objmgr_get_pdev_by_id(psoc, 0, WLAN_DP_ID);
	if (!pdev)
		return -EINVAL;

	os_priv = wlan_pdev_get_ospriv(pdev);
	if (!os_priv) {
		osif_err("PDEV OS private structure is NULL");
		return -EINVAL;
	}

	event_len = os_if_dp_flow_stats_update_or_get_len(NULL, flow_samples,
							  flags);
	if (event_len < 0)
		return -EINVAL;

	flow_sample_event = wlan_cfg80211_vendor_event_alloc(os_priv->wiphy,
							     NULL, event_len,
							     index, GFP_KERNEL);
	if (!flow_sample_event) {
		osif_err("STC: vendor event alloc failed for flow sample");
		wlan_objmgr_pdev_release_ref(pdev, WLAN_DP_ID);
		return -EINVAL;
	}

	ret = os_if_dp_flow_stats_update_or_get_len(flow_sample_event,
						    flow_samples, flags);
	if (ret < 0)
		goto flow_sample_nla_fail;

	wlan_cfg80211_vendor_event(flow_sample_event, GFP_KERNEL);
	wlan_objmgr_pdev_release_ref(pdev, WLAN_DP_ID);
	return ret;

flow_sample_nla_fail:
	wlan_objmgr_pdev_release_ref(pdev, WLAN_DP_ID);
	osif_err("STC: flow sample nla_put api failed");
	wlan_cfg80211_vendor_free_skb(flow_sample_event);
	return -EINVAL;
}

void osif_dp_register_stc_callbacks(struct wlan_dp_psoc_callbacks *cb_obj)
{
	cb_obj->send_flow_stats_event = os_if_dp_send_flow_stats_event;
}
