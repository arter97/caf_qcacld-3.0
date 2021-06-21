/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
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

#include <wlan_objmgr_cmn.h>
#include <wlan_objmgr_psoc_obj.h>
#include <wlan_objmgr_pdev_obj.h>
#include <wlan_objmgr_vdev_obj.h>
#include <wlan_objmgr_peer_obj.h>
#include <cdp_txrx_host_stats.h>
#include <wlan_cfg80211_ic_cp_stats.h>
#include <wlan_stats.h>

static QDF_STATUS get_basic_peer_data_tx(struct unified_stats *stats,
					 struct cdp_peer_stats *peer_stats)
{
	struct basic_peer_data_tx *data = NULL;
	struct cdp_tx_stats *tx = NULL;

	if (!stats || !peer_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	tx = &peer_stats->tx;
	data = qdf_mem_malloc(sizeof(struct basic_peer_data_tx));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	data->tx_success.num = tx->tx_success.num;
	data->tx_success.bytes = tx->tx_success.bytes;
	data->comp_pkt.num = tx->comp_pkt.num;
	data->comp_pkt.bytes = tx->comp_pkt.bytes;
	data->tx_failed = tx->tx_failed;
	data->dropped_count = tx->dropped.fw_rem.num + tx->dropped.fw_rem_notx +
			      tx->dropped.fw_rem_tx + tx->dropped.age_out +
			      tx->dropped.fw_reason1 + tx->dropped.fw_reason2 +
			      tx->dropped.fw_reason3;

	stats->tx = data;
	stats->tx_size = sizeof(struct basic_peer_data_tx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_peer_ctrl_tx(struct unified_stats *stats,
					 struct peer_ic_cp_stats *peer_cp_stats)
{
	struct basic_peer_ctrl_tx *ctrl = NULL;

	if (!stats || !peer_cp_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	ctrl = qdf_mem_malloc(sizeof(struct basic_peer_ctrl_tx));
	if (!ctrl) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	ctrl->cs_tx_mgmt = peer_cp_stats->cs_tx_mgmt;
	ctrl->cs_is_tx_not_ok = peer_cp_stats->cs_is_tx_not_ok;

	stats->tx = ctrl;
	stats->tx_size = sizeof(struct basic_peer_ctrl_tx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_peer_data_rx(struct unified_stats *stats,
					 struct cdp_peer_stats *peer_stats)
{
	struct basic_peer_data_rx *data = NULL;
	struct cdp_rx_stats *rx = NULL;

	if (!stats || !peer_stats) {
		qdf_err("Invalid Psoc or Peer!");
		return QDF_STATUS_E_INVAL;
	}
	data = qdf_mem_malloc(sizeof(struct basic_peer_data_rx));
	if (!data) {
		qdf_err("Failed Allocation");
		return QDF_STATUS_E_NOMEM;
	}
	rx = &peer_stats->rx;
	data->to_stack.num = rx->to_stack.num;
	data->to_stack.bytes = rx->to_stack.bytes;
	data->total_pkt_rcvd = rx->unicast.num + rx->multicast.num +
			       rx->bcast.num;
	data->total_bytes_rcvd = rx->unicast.bytes + rx->multicast.bytes +
				 rx->bcast.bytes;
	data->rx_error_count = rx->err.mic_err + rx->err.decrypt_err +
			       rx->err.fcserr + rx->err.pn_err +
			       rx->err.oor_err;

	stats->rx = data;
	stats->rx_size = sizeof(struct basic_peer_data_rx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_peer_ctrl_rx(struct unified_stats *stats,
					 struct peer_ic_cp_stats *peer_cp_stats)
{
	struct basic_peer_ctrl_rx *ctrl = NULL;

	if (!stats || !peer_cp_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	ctrl = qdf_mem_malloc(sizeof(struct basic_peer_ctrl_rx));
	if (!ctrl) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	ctrl->cs_rx_mgmt = peer_cp_stats->cs_rx_mgmt;
	ctrl->cs_rx_decryptcrc = peer_cp_stats->cs_rx_decryptcrc;
	ctrl->cs_rx_security_failure = peer_cp_stats->cs_rx_wepfail +
				       peer_cp_stats->cs_rx_ccmpmic +
				       peer_cp_stats->cs_rx_wpimic;

	stats->rx = ctrl;
	stats->rx_size = sizeof(struct basic_peer_ctrl_rx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_peer_data_link(struct unified_stats *stats,
					   struct cdp_peer_stats *peer_stats)
{
	struct basic_peer_data_link *data = NULL;
	struct cdp_rx_stats *rx = NULL;

	if (!stats || !peer_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	rx = &peer_stats->rx;
	data = qdf_mem_malloc(sizeof(struct basic_peer_data_link));
	if (!data) {
		qdf_err("Failed Allocation");
		return QDF_STATUS_E_NOMEM;
	}
	data->avg_snr = rx->avg_snr;
	data->snr = rx->snr;
	data->last_snr = rx->last_snr;

	stats->link = data;
	stats->link_size = sizeof(struct basic_peer_data_link);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_peer_ctrl_link(struct unified_stats *stats,
					   struct peer_ic_cp_stats *cp_stats)
{
	struct basic_peer_ctrl_link *ctrl = NULL;

	if (!stats || !cp_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	ctrl = qdf_mem_malloc(sizeof(struct basic_peer_ctrl_link));
	if (!ctrl) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	ctrl->cs_rx_mgmt_snr = cp_stats->cs_rx_mgmt_snr;

	stats->link = ctrl;
	stats->link_size = sizeof(struct basic_peer_ctrl_link);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_peer_data_rate(struct unified_stats *stats,
					   struct cdp_peer_stats *peer_stats)
{
	struct basic_peer_data_rate *data = NULL;
	struct cdp_tx_stats *tx = NULL;
	struct cdp_rx_stats *rx = NULL;

	if (!stats || !peer_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	tx = &peer_stats->tx;
	rx = &peer_stats->rx;
	data = qdf_mem_malloc(sizeof(struct basic_peer_data_rate));
	if (!data) {
		qdf_err("Failed Allocation");
		return QDF_STATUS_E_NOMEM;
	}
	data->rx_rate = rx->rx_rate;
	data->last_rx_rate = rx->last_rx_rate;
	data->tx_rate = tx->tx_rate;
	data->last_tx_rate = tx->last_tx_rate;

	stats->rate = data;
	stats->rate_size = sizeof(struct basic_peer_data_rate);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_peer_ctrl_rate(struct unified_stats *stats,
					   struct peer_ic_cp_stats *cp_stats)
{
	struct basic_peer_ctrl_rate *ctrl = NULL;

	if (!stats || !cp_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	ctrl = qdf_mem_malloc(sizeof(struct basic_peer_ctrl_rate));
	if (!ctrl) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	ctrl->cs_rx_mgmt_rate = cp_stats->cs_rx_mgmt_rate;

	stats->rate = ctrl;
	stats->rate_size = sizeof(struct basic_peer_ctrl_rate);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_vdev_data_tx(struct unified_stats *stats,
					 struct cdp_vdev_stats *vdev_stats)
{
	struct basic_vdev_data_tx *data = NULL;
	struct cdp_tx_stats *tx = NULL;
	struct cdp_tx_ingress_stats *tx_i = NULL;

	if (!stats || !vdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	tx_i = &vdev_stats->tx_i;
	tx = &vdev_stats->tx;
	data = qdf_mem_malloc(sizeof(struct basic_vdev_data_tx));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	data->rcvd.num = tx_i->rcvd.num;
	data->rcvd.bytes = tx_i->rcvd.bytes;
	data->processed.num = tx_i->processed.num;
	data->processed.bytes = tx_i->processed.bytes;
	data->dropped.num = tx_i->dropped.dropped_pkt.num;
	data->dropped.bytes = tx_i->dropped.dropped_pkt.bytes;
	data->tx_success.num = tx->tx_success.num;
	data->tx_success.bytes = tx->tx_success.bytes;
	data->comp_pkt.num = tx->comp_pkt.num;
	data->comp_pkt.bytes = tx->comp_pkt.bytes;
	data->tx_failed = tx->tx_failed;
	data->dropped_count = tx->dropped.fw_rem.num + tx->dropped.fw_rem_notx +
			      tx->dropped.fw_rem_tx + tx->dropped.age_out +
			      tx->dropped.fw_reason1 + tx->dropped.fw_reason2 +
			      tx->dropped.fw_reason3;

	stats->tx = data;
	stats->tx_size = sizeof(struct basic_vdev_data_tx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_vdev_ctrl_tx(struct unified_stats *stats,
					 struct vdev_ic_cp_stats *cp_stats)
{
	struct basic_vdev_ctrl_tx *ctrl = NULL;

	if (!stats || !cp_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	ctrl = qdf_mem_malloc(sizeof(struct basic_vdev_ctrl_tx));
	if (!ctrl) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	ctrl->cs_tx_mgmt = cp_stats->ucast_stats.cs_tx_mgmt +
			   cp_stats->mcast_stats.cs_tx_mgmt;
	ctrl->cs_tx_error_counter = cp_stats->stats.cs_tx_nodefkey +
				    cp_stats->stats.cs_tx_noheadroom +
				    cp_stats->stats.cs_tx_nobuf +
				    cp_stats->stats.cs_tx_nonode +
				    cp_stats->stats.cs_tx_cipher_err +
				    cp_stats->stats.cs_tx_not_ok;
	ctrl->cs_tx_discard = cp_stats->ucast_stats.cs_tx_discard +
			      cp_stats->mcast_stats.cs_tx_discard;

	stats->tx = ctrl;
	stats->tx_size = sizeof(struct basic_vdev_ctrl_tx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_vdev_data_rx(struct unified_stats *stats,
					 struct cdp_vdev_stats *vdev_stats)
{
	struct basic_vdev_data_rx *data = NULL;
	struct cdp_rx_stats *rx = NULL;

	if (!stats || !vdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	rx = &vdev_stats->rx;
	data = qdf_mem_malloc(sizeof(struct basic_vdev_data_rx));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	data->to_stack.num = rx->to_stack.num;
	data->to_stack.bytes = rx->to_stack.bytes;
	data->total_pkt_rcvd = rx->unicast.num + rx->multicast.num +
			       rx->bcast.num;
	data->total_bytes_rcvd = rx->unicast.bytes + rx->multicast.bytes +
				 rx->bcast.bytes;
	data->rx_error_count = rx->err.mic_err + rx->err.decrypt_err +
			       rx->err.fcserr + rx->err.pn_err +
			       rx->err.oor_err;

	stats->rx = data;
	stats->rx_size = sizeof(struct basic_vdev_data_rx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_vdev_ctrl_rx(struct unified_stats *stats,
					 struct vdev_ic_cp_stats *cp_stats)
{
	struct basic_vdev_ctrl_rx *ctrl = NULL;

	if (!stats || !cp_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	ctrl = qdf_mem_malloc(sizeof(struct basic_vdev_ctrl_rx));
	if (!ctrl) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	ctrl->cs_rx_mgmt = cp_stats->ucast_stats.cs_rx_mgmt +
			   cp_stats->mcast_stats.cs_rx_mgmt;
	ctrl->cs_rx_error_counter = cp_stats->stats.cs_rx_wrongbss +
				    cp_stats->stats.cs_rx_tooshort +
				    cp_stats->stats.cs_rx_ssid_mismatch;
	ctrl->cs_rx_mgmt_discard = cp_stats->stats.cs_rx_mgmt_discard;
	ctrl->cs_rx_ctl = cp_stats->stats.cs_rx_ctl;
	ctrl->cs_rx_discard = cp_stats->ucast_stats.cs_rx_discard +
			      cp_stats->mcast_stats.cs_rx_discard;
	ctrl->cs_rx_security_failure = cp_stats->ucast_stats.cs_rx_wepfail +
				       cp_stats->mcast_stats.cs_rx_wepfail +
				       cp_stats->ucast_stats.cs_rx_tkipicv +
				       cp_stats->mcast_stats.cs_rx_tkipicv +
				       cp_stats->ucast_stats.cs_rx_ccmpmic +
				       cp_stats->mcast_stats.cs_rx_ccmpmic +
				       cp_stats->ucast_stats.cs_rx_wpimic +
				       cp_stats->mcast_stats.cs_rx_wpimic;

	stats->rx = ctrl;
	stats->rx_size = sizeof(struct basic_vdev_ctrl_rx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_pdev_data_tx(struct unified_stats *stats,
					 struct cdp_pdev_stats *pdev_stats)
{
	struct basic_pdev_data_tx *data = NULL;
	struct cdp_tx_ingress_stats *tx_i = NULL;
	struct cdp_tx_stats *tx = NULL;

	if (!stats || !pdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	tx_i = &pdev_stats->tx_i;
	tx = &pdev_stats->tx;
	data = qdf_mem_malloc(sizeof(struct basic_pdev_data_tx));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	data->rcvd.num = tx_i->rcvd.num;
	data->rcvd.bytes = tx_i->rcvd.bytes;
	data->processed.num = tx_i->processed.num;
	data->processed.bytes = tx_i->processed.bytes;
	data->dropped.num = tx_i->dropped.dropped_pkt.num;
	data->dropped.bytes = tx_i->dropped.dropped_pkt.bytes;
	data->tx_success.num = tx->tx_success.num;
	data->tx_success.bytes = tx->tx_success.bytes;
	data->comp_pkt.num = tx->comp_pkt.num;
	data->comp_pkt.bytes = tx->comp_pkt.bytes;
	data->tx_failed = tx->tx_failed;
	data->dropped_count = tx->dropped.fw_rem.num + tx->dropped.fw_rem_notx +
			      tx->dropped.fw_rem_tx + tx->dropped.age_out +
			      tx->dropped.fw_reason1 + tx->dropped.fw_reason2 +
			      tx->dropped.fw_reason3;

	stats->tx = data;
	stats->tx_size = sizeof(struct basic_pdev_data_tx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_pdev_data_rx(struct unified_stats *stats,
					 struct cdp_pdev_stats *pdev_stats)
{
	struct basic_pdev_data_rx *data = NULL;
	struct cdp_rx_stats *rx = NULL;

	if (!stats || !pdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	rx = &pdev_stats->rx;
	data = qdf_mem_malloc(sizeof(struct basic_pdev_data_rx));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	data->to_stack.num = rx->to_stack.num;
	data->to_stack.bytes = rx->to_stack.bytes;
	data->total_pkt_rcvd = rx->unicast.num + rx->multicast.num +
			       rx->bcast.num;
	data->total_bytes_rcvd = rx->unicast.bytes + rx->multicast.bytes +
				 rx->bcast.bytes;
	data->rx_error_count = rx->err.mic_err + rx->err.decrypt_err +
			       rx->err.fcserr + rx->err.pn_err +
			       rx->err.oor_err;
	data->dropped_count = pdev_stats->dropped.msdu_not_done +
			      pdev_stats->dropped.mec +
			      pdev_stats->dropped.mesh_filter +
			      pdev_stats->dropped.wifi_parse +
			      pdev_stats->dropped.mon_rx_drop +
			      pdev_stats->dropped.mon_radiotap_update_err;
	data->err_count = pdev_stats->err.desc_alloc_fail +
			  pdev_stats->err.ip_csum_err +
			  pdev_stats->err.tcp_udp_csum_err +
			  pdev_stats->err.rxdma_error +
			  pdev_stats->err.reo_error;

	stats->rx = data;
	stats->rx_size = sizeof(struct basic_pdev_data_rx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_pdev_ctrl_tx(struct unified_stats *stats,
					 struct pdev_ic_cp_stats *pdev_cp_stats)
{
	struct basic_pdev_ctrl_tx *ctrl = NULL;

	if (!stats || !pdev_cp_stats) {
		qdf_err("Invalid Input");
		return QDF_STATUS_E_INVAL;
	}
	ctrl = qdf_mem_malloc(sizeof(struct basic_pdev_ctrl_tx));
	if (!ctrl) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	ctrl->cs_tx_mgmt = pdev_cp_stats->stats.cs_tx_mgmt;
	ctrl->cs_tx_frame_count = pdev_cp_stats->stats.cs_tx_frame_count;

	stats->tx = ctrl;
	stats->tx_size = sizeof(struct basic_pdev_ctrl_tx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_pdev_ctrl_rx(struct unified_stats *stats,
					 struct pdev_ic_cp_stats *pdev_cp_stats)
{
	struct basic_pdev_ctrl_rx *ctrl = NULL;

	if (!stats || !pdev_cp_stats) {
		qdf_err("Invalid Input");
		return QDF_STATUS_E_INVAL;
	}
	ctrl = qdf_mem_malloc(sizeof(struct basic_pdev_ctrl_rx));
	if (!ctrl) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	ctrl->cs_rx_mgmt = pdev_cp_stats->stats.cs_rx_mgmt;
	ctrl->cs_rx_num_mgmt = pdev_cp_stats->stats.cs_rx_num_mgmt;
	ctrl->cs_rx_num_ctl = pdev_cp_stats->stats.cs_rx_num_ctl;
	ctrl->cs_rx_frame_count = pdev_cp_stats->stats.cs_rx_frame_count;
	ctrl->cs_rx_error_sum = pdev_cp_stats->stats.cs_fcsbad +
				pdev_cp_stats->stats.cs_be_nobuf +
				pdev_cp_stats->stats.cs_rx_overrun +
				pdev_cp_stats->stats.cs_rx_phy_err +
				pdev_cp_stats->stats.cs_rx_ack_err +
				pdev_cp_stats->stats.cs_rx_rts_err +
				pdev_cp_stats->stats.cs_no_beacons +
				pdev_cp_stats->stats.cs_rx_mgmt_rssi_drop +
				pdev_cp_stats->stats.cs_phy_err_count;

	stats->rx = ctrl;
	stats->rx_size = sizeof(struct basic_pdev_ctrl_rx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_pdev_ctrl_link(struct unified_stats *stats,
					   struct pdev_ic_cp_stats *cp_stats)
{
	struct basic_pdev_ctrl_link *ctrl = NULL;

	if (!stats || !cp_stats) {
		qdf_err("Invalid Input");
		return QDF_STATUS_E_INVAL;
	}
	ctrl = qdf_mem_malloc(sizeof(struct basic_pdev_ctrl_link));
	if (!ctrl) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	ctrl->cs_chan_tx_pwr = cp_stats->stats.cs_chan_tx_pwr;
	ctrl->cs_rx_rssi_comb = cp_stats->stats.cs_rx_rssi_comb;
	ctrl->cs_chan_nf = cp_stats->stats.cs_chan_nf;
	ctrl->cs_chan_nf_sec80 = cp_stats->stats.cs_chan_nf_sec80;
	ctrl->dcs_total_util = cp_stats->stats.chan_stats.dcs_total_util;

	stats->link = ctrl;
	stats->link_size = sizeof(struct basic_pdev_ctrl_link);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_psoc_data_tx(struct unified_stats *stats)
{
	struct basic_psoc_data_tx *data = NULL;

	if (!stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	data = qdf_mem_malloc(sizeof(struct basic_psoc_data_tx));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	data->egress.num = 0;
	data->egress.bytes = 0;

	stats->tx = data;
	stats->tx_size = sizeof(struct basic_psoc_data_tx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_psoc_data_rx(struct unified_stats *stats)
{
	struct basic_psoc_data_rx *data = NULL;

	if (!stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	data = qdf_mem_malloc(sizeof(struct basic_psoc_data_rx));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	data->ingress.num = 0;
	data->ingress.bytes = 0;

	stats->rx = data;
	stats->rx_size = sizeof(struct basic_psoc_data_rx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_peer_data(struct wlan_objmgr_psoc *psoc,
				      struct wlan_objmgr_peer *peer,
				      struct unified_stats *stats,
				      uint32_t feat)
{
	QDF_STATUS ret = QDF_STATUS_SUCCESS;
	struct cdp_peer_stats *peer_stats = NULL;
	uint8_t vdev_id = 0;

	if (!psoc || !peer) {
		qdf_err("Invalid Psoc or Peer!");
		return QDF_STATUS_E_INVAL;
	}
	peer_stats = qdf_mem_malloc(sizeof(struct cdp_peer_stats));
	if (!peer_stats) {
		qdf_err("Failed allocation!");
		return QDF_STATUS_E_NOMEM;
	}
	vdev_id = wlan_vdev_get_id(peer->peer_objmgr.vdev);
	ret = cdp_host_get_peer_stats(wlan_psoc_get_dp_handle(psoc), vdev_id,
				      peer->macaddr, peer_stats);
	if (ret != QDF_STATUS_SUCCESS) {
		qdf_err("Unable to fetch stats!");
		qdf_mem_free(peer_stats);
		return ret;
	}

	if (feat & STATS_FEAT_FLG_TX)
		ret = get_basic_peer_data_tx(stats, peer_stats);
	if (feat & STATS_FEAT_FLG_RX)
		ret = get_basic_peer_data_rx(stats, peer_stats);
	if (feat & STATS_FEAT_FLG_LINK)
		ret = get_basic_peer_data_link(stats, peer_stats);
	if (feat & STATS_FEAT_FLG_RATE)
		ret = get_basic_peer_data_rate(stats, peer_stats);
	qdf_mem_free(peer_stats);

	return ret;
}

static QDF_STATUS get_basic_peer_ctrl(struct wlan_objmgr_peer *peer,
				      struct unified_stats *stats,
				      uint32_t feat)
{
	QDF_STATUS ret = QDF_STATUS_SUCCESS;
	struct peer_ic_cp_stats *peer_cp_stats = NULL;

	if (!peer) {
		qdf_err("Invalid Peer!");
		return QDF_STATUS_E_INVAL;
	}
	peer_cp_stats = qdf_mem_malloc(sizeof(struct peer_ic_cp_stats));
	if (!peer_cp_stats) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	ret = wlan_cfg80211_get_peer_cp_stats(peer, peer_cp_stats);
	if (QDF_STATUS_SUCCESS != ret) {
		qdf_err("Failed to get Peer Control Stats!");
		qdf_mem_free(peer_cp_stats);
		return ret;
	}

	if (feat & STATS_FEAT_FLG_TX)
		ret = get_basic_peer_ctrl_tx(stats, peer_cp_stats);
	if (feat & STATS_FEAT_FLG_RX)
		ret = get_basic_peer_ctrl_rx(stats, peer_cp_stats);
	if (feat & STATS_FEAT_FLG_LINK)
		ret = get_basic_peer_ctrl_link(stats, peer_cp_stats);
	if (feat & STATS_FEAT_FLG_RATE)
		ret = get_basic_peer_ctrl_rate(stats, peer_cp_stats);
	qdf_mem_free(peer_cp_stats);

	return ret;
}

static QDF_STATUS get_basic_vdev_data(struct wlan_objmgr_psoc *psoc,
				      struct wlan_objmgr_vdev *vdev,
				      struct unified_stats *stats,
				      uint32_t feat)
{
	struct cdp_vdev_stats *vdev_stats = NULL;
	QDF_STATUS ret = QDF_STATUS_SUCCESS;

	if (!psoc || !vdev) {
		qdf_err("Invalid psoc or vdev!");
		return QDF_STATUS_E_INVAL;
	}
	vdev_stats = qdf_mem_malloc(sizeof(struct cdp_vdev_stats));
	if (!vdev_stats) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	ret = cdp_host_get_vdev_stats(wlan_psoc_get_dp_handle(psoc),
				      wlan_vdev_get_id(vdev),
				      vdev_stats, true);
	if (ret != QDF_STATUS_SUCCESS) {
		qdf_err("Unable to get Vdev Stats!");
		qdf_mem_free(vdev_stats);
		return ret;
	}
	if (feat & STATS_FEAT_FLG_TX)
		ret = get_basic_vdev_data_tx(stats, vdev_stats);
	if (feat & STATS_FEAT_FLG_RX)
		ret = get_basic_vdev_data_rx(stats, vdev_stats);
	qdf_mem_free(vdev_stats);

	return ret;
}

static QDF_STATUS get_basic_vdev_ctrl(struct wlan_objmgr_vdev *vdev,
				      struct unified_stats *stats,
				      uint32_t feat)
{
	struct vdev_ic_cp_stats *cp_stats = NULL;
	QDF_STATUS ret = QDF_STATUS_SUCCESS;

	if (!vdev) {
		qdf_err("Invalid vdev!");
		return QDF_STATUS_E_INVAL;
	}
	cp_stats = qdf_mem_malloc(sizeof(struct vdev_ic_cp_stats));
	if (!cp_stats) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	ret = wlan_cfg80211_get_vdev_cp_stats(vdev, cp_stats);
	if (QDF_STATUS_SUCCESS != ret) {
		qdf_err("Unable to get Vdev Control stats!");
		qdf_mem_free(cp_stats);
		return ret;
	}
	if (feat & STATS_FEAT_FLG_TX)
		ret = get_basic_vdev_ctrl_tx(stats, cp_stats);
	if (feat & STATS_FEAT_FLG_RX)
		ret = get_basic_vdev_ctrl_rx(stats, cp_stats);
	qdf_mem_free(cp_stats);

	return ret;
}

static QDF_STATUS get_basic_pdev_data(struct wlan_objmgr_psoc *psoc,
				      struct wlan_objmgr_pdev *pdev,
				      struct unified_stats *stats,
				      uint32_t feat)
{
	struct cdp_pdev_stats *pdev_stats = NULL;
	QDF_STATUS ret = QDF_STATUS_SUCCESS;

	if (!psoc || !pdev) {
		qdf_err("Invalid pdev and psoc!");
		return QDF_STATUS_E_INVAL;
	}
	pdev_stats = qdf_mem_malloc(sizeof(struct cdp_pdev_stats));
	if (!pdev_stats) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	ret = cdp_host_get_pdev_stats(wlan_psoc_get_dp_handle(psoc),
				      wlan_objmgr_pdev_get_pdev_id(pdev),
				      pdev_stats);
	if (ret != QDF_STATUS_SUCCESS) {
		qdf_err("Unable to get Pdev stats!");
		qdf_mem_free(pdev_stats);
		return ret;
	}
	if (feat & STATS_FEAT_FLG_TX)
		ret = get_basic_pdev_data_tx(stats, pdev_stats);
	if (feat & STATS_FEAT_FLG_RX)
		ret = get_basic_pdev_data_rx(stats, pdev_stats);
	qdf_mem_free(pdev_stats);

	return ret;
}

static QDF_STATUS get_basic_pdev_ctrl(struct wlan_objmgr_pdev *pdev,
				      struct unified_stats *stats,
				      uint32_t feat)
{
	struct pdev_ic_cp_stats *pdev_cp_stats = NULL;
	QDF_STATUS ret = QDF_STATUS_SUCCESS;

	if (!pdev) {
		qdf_err("Invalid pdev!");
		return QDF_STATUS_E_INVAL;
	}
	pdev_cp_stats = qdf_mem_malloc(sizeof(struct pdev_ic_cp_stats));
	if (!pdev_cp_stats) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	ret = wlan_cfg80211_get_pdev_cp_stats(pdev, pdev_cp_stats);
	if (QDF_STATUS_SUCCESS != ret) {
		qdf_err("Unbale to get pdev control stats!");
		qdf_mem_free(pdev_cp_stats);
		return ret;
	}
	if (feat & STATS_FEAT_FLG_TX)
		ret = get_basic_pdev_ctrl_tx(stats, pdev_cp_stats);
	if (feat & STATS_FEAT_FLG_RX)
		ret = get_basic_pdev_ctrl_rx(stats, pdev_cp_stats);
	if (feat & STATS_FEAT_FLG_LINK)
		ret = get_basic_pdev_ctrl_link(stats, pdev_cp_stats);
	qdf_mem_free(pdev_cp_stats);

	return ret;
}

static QDF_STATUS get_basic_psoc_data(struct wlan_objmgr_psoc *psoc,
				      struct unified_stats *stats,
				      uint32_t feat)
{
	QDF_STATUS ret = QDF_STATUS_SUCCESS;

	if (!psoc) {
		qdf_err("Invalid psoc!");
		return QDF_STATUS_E_INVAL;
	}
	if (feat & STATS_FEAT_FLG_TX)
		ret = get_basic_psoc_data_tx(stats);
	if (feat & STATS_FEAT_FLG_RX)
		ret = get_basic_psoc_data_rx(stats);

	return ret;
}

/* Public APIs */
QDF_STATUS wlan_stats_get_peer_stats(struct wlan_objmgr_psoc *psoc,
				     struct wlan_objmgr_peer *peer,
				     struct stats_config *cfg,
				     struct unified_stats *stats)
{
	QDF_STATUS ret = QDF_STATUS_SUCCESS;

	switch (cfg->lvl) {
	case STATS_LVL_BASIC:
		if (cfg->type == STATS_TYPE_DATA)
			ret = get_basic_peer_data(psoc, peer, stats, cfg->feat);
		else
			ret = get_basic_peer_ctrl(peer, stats, cfg->feat);
		break;
	case STATS_LVL_ADVANCE:
		break;
	case STATS_LVL_DEBUG:
		break;
	}

	return ret;
}

QDF_STATUS wlan_stats_get_vdev_stats(struct wlan_objmgr_psoc *psoc,
				     struct wlan_objmgr_vdev *vdev,
				     struct stats_config *cfg,
				     struct unified_stats *stats)
{
	QDF_STATUS ret = QDF_STATUS_SUCCESS;

	switch (cfg->lvl) {
	case STATS_LVL_BASIC:
		if (cfg->type == STATS_TYPE_DATA)
			ret = get_basic_vdev_data(psoc, vdev, stats, cfg->feat);
		else
			ret = get_basic_vdev_ctrl(vdev, stats, cfg->feat);
		break;
	case STATS_LVL_ADVANCE:
		break;
	case STATS_LVL_DEBUG:
		break;
	}

	return ret;
}

QDF_STATUS wlan_stats_get_pdev_stats(struct wlan_objmgr_psoc *psoc,
				     struct wlan_objmgr_pdev *pdev,
				     struct stats_config *cfg,
				     struct unified_stats *stats)
{
	QDF_STATUS ret = QDF_STATUS_SUCCESS;

	switch (cfg->lvl) {
	case STATS_LVL_BASIC:
		if (cfg->type == STATS_TYPE_DATA)
			ret = get_basic_pdev_data(psoc, pdev, stats, cfg->feat);
		else
			ret = get_basic_pdev_ctrl(pdev, stats, cfg->feat);
		break;
	case STATS_LVL_ADVANCE:
		break;
	case STATS_LVL_DEBUG:
		break;
	}

	return ret;
}

QDF_STATUS wlan_stats_get_psoc_stats(struct wlan_objmgr_psoc *psoc,
				     struct stats_config *cfg,
				     struct unified_stats *stats)
{
	QDF_STATUS ret = QDF_STATUS_SUCCESS;

	switch (cfg->lvl) {
	case STATS_LVL_BASIC:
		if (cfg->type == STATS_TYPE_DATA)
			ret = get_basic_psoc_data(psoc, stats, cfg->feat);
		break;
	case STATS_LVL_ADVANCE:
		break;
	case STATS_LVL_DEBUG:
		break;
	}

	return ret;
}

void wlan_stats_free_unified_stats(struct unified_stats *stats)
{
	if (stats->tx)
		qdf_mem_free(stats->tx);
	if (stats->rx)
		qdf_mem_free(stats->rx);
	if (stats->link)
		qdf_mem_free(stats->link);
	if (stats->rate)
		qdf_mem_free(stats->rate);
}
