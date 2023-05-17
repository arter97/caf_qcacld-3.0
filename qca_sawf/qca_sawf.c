/*
 * Copyright (c) 2022-2023, Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifdef CONFIG_SAWF
#include <dp_sawf.h>
#include <wlan_sawf.h>
#include <cdp_txrx_sawf.h>
#ifdef WLAN_SUPPORT_SCS
#include <qca_scs_if.h>
#endif
uint16_t qca_sawf_get_msduq(struct net_device *netdev, uint8_t *peer_mac,
			    uint32_t service_id)
{
	if (!wlan_service_id_valid(service_id) ||
	    !wlan_service_id_configured(service_id)) {
	   return DP_SAWF_PEER_Q_INVALID;
	}

	return dp_sawf_get_msduq(netdev, peer_mac, service_id);
}

/* qca_sawf_get_vdev() - Fetch vdev from netdev
 *
 * @netdev : Netdevice
 *
 * Return: Pointer to struct wlan_objmgr_vdev
 */
static struct wlan_objmgr_vdev *qca_sawf_get_vdev(struct net_device *netdev)
{
	struct wlan_objmgr_vdev *vdev = NULL;
	osif_dev *osdev = NULL;

	osdev = ath_netdev_priv(netdev);

#ifdef QCA_SUPPORT_WDS_EXTENDED
	if (osdev->dev_type == OSIF_NETDEV_TYPE_WDS_EXT) {
		osif_peer_dev *osifp = NULL;
		osif_dev *parent_osdev = NULL;

		osifp = ath_netdev_priv(netdev);
		if (!osifp->parent_netdev)
			return NULL;

		parent_osdev = ath_netdev_priv(osifp->parent_netdev);
		osdev = parent_osdev;
	}
#endif
	vdev = osdev->ctrl_vdev;
	return vdev;
}

/* qca_sawf_get_default_msduq() - Return default msdu queue_id
 *
 * @netdev : Netdevice
 * @peer_mac : Destination peer mac address
 * @service_id : Service class id
 * @rule_id : Rule id
 *
 * Return: 16 bits msdu queue_id
 */
#ifdef WLAN_SUPPORT_SCS

#define SVC_ID_TO_QUEUE_ID(svc_id) (svc_id - SAWF_SCS_SVC_CLASS_MIN)

static uint16_t qca_sawf_get_default_msduq(struct net_device *netdev,
					   uint8_t *peer_mac,
					   uint32_t service_id,
					   uint32_t rule_id)
{
	struct wlan_objmgr_vdev *vdev = NULL;
	struct wlan_objmgr_psoc *psoc = NULL;
	uint16_t queue_id = DP_SAWF_PEER_Q_INVALID;

	vdev = qca_sawf_get_vdev(netdev);
	if (vdev) {
		psoc = wlan_vdev_get_psoc(vdev);

		/**
		 * When device is operating in WDS_EXT mode, then mac address
		 * is also populated in the rule which makes the rule peer
		 * specific and hence there is not need of rule check against
		 * the peer, else rule check has to be against the peer when
		 * operating in non WDS_EXT mode.
		 */
		if (psoc &&
		    wlan_psoc_nif_feat_cap_get(psoc, WLAN_SOC_F_WDS_EXTENDED)) {
			queue_id = SVC_ID_TO_QUEUE_ID(service_id);
		} else {
			if (qca_scs_peer_lookup_n_rule_match(rule_id,
							     peer_mac))
				queue_id = SVC_ID_TO_QUEUE_ID(service_id);
		}
	}

	return queue_id;
}
#else
static uint16_t qca_sawf_get_default_msduq(struct net_device *netdev,
					   uint8_t *peer_mac,
					   uint32_t service_id,
					   uint32_t rule_id)
{
	return DP_SAWF_PEER_Q_INVALID;
}
#endif

uint16_t qca_sawf_get_msduq_v2(struct net_device *netdev, uint8_t *peer_mac,
			       uint32_t service_id, uint32_t dscp,
			       uint32_t rule_id, uint8_t sawf_rule_type)
{
	if (!netdev->ieee80211_ptr)
		return DP_SAWF_PEER_Q_INVALID;

	/* Return default queue_id in case of valid SAWF_SCS */
	if (wlan_service_id_scs_valid(sawf_rule_type, service_id))
		return qca_sawf_get_default_msduq(netdev, peer_mac,
						  service_id, rule_id);

	return qca_sawf_get_msduq(netdev, peer_mac, service_id);
}

struct psoc_sawf_ul_itr {
	uint8_t *mac_addr;
	uint32_t service_interval;
	uint32_t burst_size;
	uint32_t min_tput;
	uint32_t max_latency;
	uint8_t tid;
	uint8_t add_or_sub;
};

static void qca_sawf_psoc_ul_cb(struct wlan_objmgr_psoc *psoc, void *cbd,
				uint8_t index)
{
	ol_txrx_soc_handle soc_txrx_handle;
	struct psoc_sawf_ul_itr *param = (struct psoc_sawf_ul_itr *)cbd;

	soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
	cdp_sawf_peer_config_ul(soc_txrx_handle,
				param->mac_addr, param->tid,
				param->service_interval, param->burst_size,
				param->min_tput, param->max_latency,
				param->add_or_sub);
}

static inline
void qca_sawf_peer_config_ul(uint8_t *mac_addr, uint8_t tid,
			     uint32_t service_interval, uint32_t burst_size,
			     uint32_t min_tput, uint32_t max_latency,
			     uint8_t add_or_sub)
{
	struct psoc_sawf_ul_itr param = {
		.mac_addr = mac_addr,
		.service_interval = service_interval,
		.burst_size = burst_size,
		.min_tput = min_tput,
		.max_latency = max_latency,
		.tid = tid,
		.add_or_sub = add_or_sub
	};

	wlan_objmgr_iterate_psoc_list(qca_sawf_psoc_ul_cb, &param,
				      WLAN_SAWF_ID);
}

void qca_sawf_config_ul(uint8_t *dst_mac, uint8_t *src_mac,
			uint8_t fw_service_id, uint8_t rv_service_id,
			uint8_t add_or_sub)
{
	uint32_t svc_interval = 0, burst_size = 0;
	uint32_t min_tput = 0, max_latency = 0;
	uint8_t tid = 0;

	qdf_nofl_debug("src " QDF_MAC_ADDR_FMT " dst " QDF_MAC_ADDR_FMT
		       " fwd_service_id %u rvrs_service_id %u",
		       QDF_MAC_ADDR_REF(src_mac), QDF_MAC_ADDR_REF(dst_mac),
		       fw_service_id, rv_service_id);

	if (QDF_IS_STATUS_SUCCESS(wlan_sawf_get_uplink_params(fw_service_id,
							      &tid,
							      &svc_interval,
							      &burst_size,
							      &min_tput,
							      &max_latency))) {
		if (svc_interval && burst_size)
			qca_sawf_peer_config_ul(src_mac, tid,
						svc_interval, burst_size,
						min_tput, max_latency,
						add_or_sub);
	}

	svc_interval = 0;
	burst_size = 0;
	tid = 0;
	if (QDF_IS_STATUS_SUCCESS(wlan_sawf_get_uplink_params(rv_service_id,
							      &tid,
							      &svc_interval,
							      &burst_size,
							      &min_tput,
							      &max_latency))) {
		if (svc_interval && burst_size)
			qca_sawf_peer_config_ul(dst_mac, tid,
						svc_interval, burst_size,
						min_tput, max_latency,
						add_or_sub);
	}
}
#else

#include "qdf_module.h"
#define DP_SAWF_PEER_Q_INVALID 0xffff
uint16_t qca_sawf_get_msduq(struct net_device *netdev, uint8_t *peer_mac,
			    uint32_t service_id)
{
	return DP_SAWF_PEER_Q_INVALID;
}

uint16_t qca_sawf_get_msduq_v2(struct net_device *netdev, uint8_t *peer_mac,
			       uint32_t service_id, uint32_t dscp,
			       uint32_t rule_id, uint8_t sawf_rule_type)
{
	return DP_SAWF_PEER_Q_INVALID;
}

void qca_sawf_config_ul(uint8_t *dst_mac, uint8_t *src_mac,
			uint8_t fw_service_id, uint8_t rv_service_id,
			uint8_t add_or_sub)
{}
#endif

uint16_t qca_sawf_get_msdu_queue(struct net_device *netdev,
				 uint8_t *peer_mac, uint32_t service_id,
				 uint32_t dscp, uint32_t rule_id)
{
	return qca_sawf_get_msduq(netdev, peer_mac, service_id);
}


qdf_export_symbol(qca_sawf_get_msduq);
qdf_export_symbol(qca_sawf_get_msduq_v2);
qdf_export_symbol(qca_sawf_get_msdu_queue);
qdf_export_symbol(qca_sawf_config_ul);
