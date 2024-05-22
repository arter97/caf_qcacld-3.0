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
#include <dp_types.h>
#include <dp_internal.h>
#include <wlan_dp_priv.h>
#include <wlan_dp_spm.h>
#include <wlan_dp_fim.h>
#include <qdf_nbuf.h>
#include <wlan_hdd_ether.h>

#define SAWFISH_FLOW_ID_MAX 0x3F
#define SAWFISH_INVALID_FLOW_ID 0xFFFF
#define SPM_INVALID_SVC 0xFFFF

/**
 * wlan_dp_parse_skb_flow_info() - Parse flow info from skb
 * @skb: network buffer
 * @flow: pointer to flow tuple info
 * @keys: keys to dissect flow
 *
 * Return: none
 */
static void wlan_dp_parse_skb_flow_info(struct sk_buff *skb,
					struct flow_info *flow,
					qdf_flow_keys_t *keys)
{
	qdf_nbuf_flow_dissect_flow_keys(skb, keys);

	if (qdf_unlikely(qdf_flow_is_first_frag(keys))) {
		if (skb->protocol == htons(ETH_P_IP)) {
			qdf_nbuf_flow_get_ports(skb, keys);
		} else {
			flow->flags |= FLOW_INFO_PRESENT_IP_FRAGMENT;
			return;
		}
	} else if (qdf_flow_is_frag(keys)) {
		flow->flags |= FLOW_INFO_PRESENT_IP_FRAGMENT;
		return;
	}

	flow->src_port = qdf_ntohs(qdf_flow_parse_src_port(keys));
	flow->flags |= FLOW_INFO_PRESENT_SRC_PORT;

	flow->dst_port = qdf_ntohs(qdf_flow_parse_dst_port(keys));
	flow->flags |= FLOW_INFO_PRESENT_DST_PORT;

	flow->proto = qdf_flow_get_proto(keys);
	flow->flags |= FLOW_INFO_PRESENT_PROTO;

	if (skb->protocol == qdf_ntohs(QDF_NBUF_TRAC_IPV4_ETH_TYPE)) {
		flow->src_ip.ipv4_addr =
				qdf_ntohl(qdf_flow_get_ipv4_src_addr(keys));
		flow->flags |= FLOW_INFO_PRESENT_IPV4_SRC_IP;

		flow->dst_ip.ipv4_addr =
				qdf_ntohl(qdf_flow_get_ipv4_dst_addr(keys));
		flow->flags |= FLOW_INFO_PRESENT_IPV4_DST_IP;
	} else if (skb->protocol == qdf_ntohs(QDF_NBUF_TRAC_IPV6_ETH_TYPE)) {
		qdf_flow_get_ipv6_src_addr(keys, &flow->src_ip.ipv6_addr);
		flow->flags |= FLOW_INFO_PRESENT_IPV6_SRC_IP;

		qdf_flow_get_ipv6_dst_addr(keys, &flow->dst_ip.ipv6_addr);
		flow->flags |= FLOW_INFO_PRESENT_IPV4_DST_IP;

		flow->flow_label = qdf_flow_get_flow_label(keys);
	}
}

/**
 * wlan_dp_sawfish_update_metadata() - Parse flow info from skb and update svc
 *				       data
 * @dp_intf: pointer to DP interface
 * @skb: Packet nbuf
 *
 * Return: Success on valid metadata update
 */
int wlan_dp_sawfish_update_metadata(struct wlan_dp_intf *dp_intf,
				    qdf_nbuf_t skb)
{
	struct sock *sk = NULL;
	struct flow_info flow = {0};
	qdf_flow_keys_t keys;
	uint16_t peer_id;
	uint16_t sk_tx_flow_id;
	int status = QDF_STATUS_E_INVAL;
	union generic_ethhdr *eth_hdr;
	uint8_t *pkt;

	if (!dp_intf->spm_intf_ctx)
		return QDF_STATUS_E_NOSUPPORT;

	sk = skb->sk;
	if (qdf_unlikely(!sk)) {
		skb->mark = SPM_INVALID_SVC;
		return QDF_STATUS_E_INVAL;
	}

	sk_tx_flow_id = sk->sk_txtime_unused;

	/* sk_tx_flow_id != 0 means flow_id valid */
	if (qdf_unlikely(!sk_tx_flow_id)) {
		wlan_dp_parse_skb_flow_info(skb, &flow, &keys);
		if (qdf_unlikely(!flow.flags ||
				 flow.flags & FLOW_INFO_PRESENT_IP_FRAGMENT)) {
			skb->mark = SPM_INVALID_SVC;
			return QDF_STATUS_E_INVAL;
		}
		pkt = skb->data;
		eth_hdr = (union generic_ethhdr *)pkt;
		peer_id = cdp_get_peer_id(dp_intf->dp_ctx->cdp_soc,
					  CDP_VDEV_ALL, eth_hdr->eth_II.h_dest);

		status = wlan_dp_spm_get_flow_id_origin(dp_intf, &sk_tx_flow_id,
							&flow, (uint64_t)sk,
							peer_id);
		/* Check status */
		if (qdf_unlikely(sk_tx_flow_id > SAWFISH_FLOW_ID_MAX)) {
			/* skb->mark with svc_metadata, lapb_metadata */
			return QDF_STATUS_E_INVAL;
		}
		sk->sk_txtime_unused = sk_tx_flow_id;
	}

	status = wlan_dp_spm_svc_get_metadata(dp_intf, skb, sk_tx_flow_id,
					      (uint64_t)sk);
	if (qdf_unlikely(status != QDF_STATUS_SUCCESS)) {
		/*
		 * Flow is possibly evicted, so mark for invalid flow.
		 * Subsequent packets of flow will have new flow allocated.
		 */
		sk->sk_txtime_unused = 0;

		return QDF_STATUS_E_INVAL;
	}


	return QDF_STATUS_SUCCESS;
}
