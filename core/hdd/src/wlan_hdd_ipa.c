/*
 * Copyright (c) 2013-2021 The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * DOC: wlan_hdd_ipa.c
 *
 * WLAN HDD and ipa interface implementation
 */

/* Include Files */
#include <wlan_hdd_includes.h>
#include <wlan_hdd_ipa.h>
#include "wlan_policy_mgr_api.h"
#include "wlan_ipa_ucfg_api.h"
#include <wlan_hdd_softap_tx_rx.h>
#include <linux/inetdevice.h>
#include <qdf_trace.h>

#if defined(QCA_CONFIG_SMP) && defined(PF_WAKE_UP_IDLE)
/**
 * hdd_ipa_get_wake_up_idle() - Get PF_WAKE_UP_IDLE flag in the task structure
 *
 * Get PF_WAKE_UP_IDLE flag in the task structure
 *
 * Return: 1 if PF_WAKE_UP_IDLE flag is set, 0 otherwise
 */
static uint32_t hdd_ipa_get_wake_up_idle(void)
{
	return sched_get_wake_up_idle(current);
}

/**
 * hdd_ipa_set_wake_up_idle() - Set PF_WAKE_UP_IDLE flag in the task structure
 *
 * Set PF_WAKE_UP_IDLE flag in the task structure
 * This task and any task woken by this will be waken to idle CPU
 *
 * Return: None
 */
static void hdd_ipa_set_wake_up_idle(bool wake_up_idle)
{
	sched_set_wake_up_idle(current, wake_up_idle);
}
#else
static uint32_t hdd_ipa_get_wake_up_idle(void)
{
	return 0;
}

static void hdd_ipa_set_wake_up_idle(bool wake_up_idle)
{
}
#endif

/**
 * hdd_ipa_send_to_nw_stack() - Check if IPA supports NAPI
 * polling during RX
 * @skb : data buffer sent to network stack
 *
 * If IPA LAN RX supports NAPI polling mechanism use
 * netif_receive_skb instead of netif_rx_ni to forward the skb
 * to network stack.
 *
 * Return: Return value from netif_rx_ni/netif_receive_skb
 */
static int hdd_ipa_send_to_nw_stack(qdf_nbuf_t skb)
{
	int result;

	if (qdf_ipa_get_lan_rx_napi())
		result = netif_receive_skb(skb);
	else
		result = netif_rx_ni(skb);
	return result;
}

#ifdef QCA_CONFIG_SMP

/**
 * hdd_ipa_aggregated_rx_ind() - Submit aggregated packets to the stack
 * @skb: skb to be submitted to the stack
 *
 * For CONFIG_SMP systems, simply call netif_rx_ni.
 * For non CONFIG_SMP systems call netif_rx till
 * IPA_WLAN_RX_SOFTIRQ_THRESH. When threshold is reached call netif_rx_ni.
 * In this manner, UDP/TCP packets are sent in an aggregated way to the stack.
 * For IP/ICMP packets, simply call netif_rx_ni.
 *
 * Check if IPA supports NAPI polling then use netif_receive_skb
 * instead of netif_rx_ni.
 *
 * Return: return value from the netif_rx_ni/netif_rx api.
 */
static int hdd_ipa_aggregated_rx_ind(qdf_nbuf_t skb)
{
	int ret;

	ret =  hdd_ipa_send_to_nw_stack(skb);
	return ret;
}
#else
static int hdd_ipa_aggregated_rx_ind(qdf_nbuf_t skb)
{
	struct iphdr *ip_h;
	static atomic_t softirq_mitigation_cntr =
		ATOMIC_INIT(IPA_WLAN_RX_SOFTIRQ_THRESH);
	int result;

	ip_h = (struct iphdr *)(skb->data);
	if ((skb->protocol == htons(ETH_P_IP)) &&
		(ip_h->protocol == IPPROTO_ICMP)) {
		result = hdd_ipa_send_to_nw_stack(skb);
	} else {
		/* Call netif_rx_ni for every IPA_WLAN_RX_SOFTIRQ_THRESH packets
		 * to avoid excessive softirq's.
		 */
		if (atomic_dec_and_test(&softirq_mitigation_cntr)) {
			result = hdd_ipa_send_to_nw_stack(skb);
			atomic_set(&softirq_mitigation_cntr,
					IPA_WLAN_RX_SOFTIRQ_THRESH);
		} else {
			result = netif_rx(skb);
		}
	}

	return result;
}
#endif

void hdd_ipa_send_nbuf_to_network(qdf_nbuf_t nbuf, qdf_netdev_t dev)
{
	struct hdd_adapter *adapter = (struct hdd_adapter *) netdev_priv(dev);
	int result;
	unsigned int cpu_index;
	uint32_t enabled;

	if (hdd_validate_adapter(adapter)) {
		kfree_skb(nbuf);
		return;
	}

	if (cds_is_driver_unloading()) {
		kfree_skb(nbuf);
		return;
	}

	hdd_ipa_update_rx_mcbc_stats(adapter, nbuf);

	if ((adapter->device_mode == QDF_SAP_MODE) &&
	    (qdf_nbuf_is_ipv4_dhcp_pkt(nbuf) == true)) {
		/* Send DHCP Indication to FW */
		hdd_softap_inspect_dhcp_packet(adapter, nbuf, QDF_RX);
	}

	qdf_dp_trace_set_track(nbuf, QDF_RX);

	hdd_event_eapol_log(nbuf, QDF_RX);
	qdf_dp_trace_log_pkt(adapter->vdev_id,
			     nbuf, QDF_RX, QDF_TRACE_DEFAULT_PDEV_ID);
	DPTRACE(qdf_dp_trace(nbuf,
			     QDF_DP_TRACE_RX_HDD_PACKET_PTR_RECORD,
			     QDF_TRACE_DEFAULT_PDEV_ID,
			     qdf_nbuf_data_addr(nbuf),
			     sizeof(qdf_nbuf_data(nbuf)), QDF_RX));
	DPTRACE(qdf_dp_trace_data_pkt(nbuf, QDF_TRACE_DEFAULT_PDEV_ID,
				      QDF_DP_TRACE_RX_PACKET_RECORD, 0,
				      QDF_RX));

	/*
	 * Set PF_WAKE_UP_IDLE flag in the task structure
	 * This task and any task woken by this will be waken to idle CPU
	 */
	enabled = hdd_ipa_get_wake_up_idle();
	if (!enabled)
		hdd_ipa_set_wake_up_idle(true);

	nbuf->dev = adapter->dev;
	nbuf->protocol = eth_type_trans(nbuf, nbuf->dev);
	nbuf->ip_summed = CHECKSUM_NONE;

	cpu_index = wlan_hdd_get_cpu();

	++adapter->hdd_stats.tx_rx_stats.rx_packets[cpu_index];

	/*
	 * Update STA RX exception packet stats.
	 * For SAP as part of IPA HW stats are updated.
	 */

	++adapter->stats.rx_packets;
	adapter->stats.rx_bytes += nbuf->len;

	result = hdd_ipa_aggregated_rx_ind(nbuf);
	if (result == NET_RX_SUCCESS)
		++adapter->hdd_stats.tx_rx_stats.rx_delivered[cpu_index];
	else
		++adapter->hdd_stats.tx_rx_stats.rx_refused[cpu_index];

	/*
	 * Restore PF_WAKE_UP_IDLE flag in the task structure
	 */
	if (!enabled)
		hdd_ipa_set_wake_up_idle(false);
}

void hdd_ipa_set_mcc_mode(bool mcc_mode)
{
	struct hdd_context *hdd_ctx;

	hdd_ctx = cds_get_context(QDF_MODULE_ID_HDD);
	if (!hdd_ctx)
		return;

	ucfg_ipa_set_mcc_mode(hdd_ctx->pdev, mcc_mode);
}
