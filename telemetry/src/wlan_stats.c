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
#include <qdf_event.h>
#include <ieee80211_var.h>
#include <wlan_stats.h>

static void fill_basic_peer_data_tx(struct basic_peer_data_tx *data,
				    struct cdp_tx_stats *tx)
{
	data->tx_success.num = tx->tx_success.num;
	data->tx_success.bytes = tx->tx_success.bytes;
	data->comp_pkt.num = tx->comp_pkt.num;
	data->comp_pkt.bytes = tx->comp_pkt.bytes;
	data->tx_failed = tx->tx_failed;
	data->dropped_count = tx->dropped.fw_rem.num + tx->dropped.fw_rem_notx +
			      tx->dropped.fw_rem_tx + tx->dropped.age_out +
			      tx->dropped.fw_reason1 + tx->dropped.fw_reason2 +
			      tx->dropped.fw_reason3;
}

static void fill_basic_peer_ctrl_tx(struct basic_peer_ctrl_tx *ctrl,
				    struct peer_ic_cp_stats *peer_cp_stats)
{
	ctrl->cs_tx_mgmt = peer_cp_stats->cs_tx_mgmt;
	ctrl->cs_is_tx_not_ok = peer_cp_stats->cs_is_tx_not_ok;
}

static void fill_basic_peer_data_rx(struct basic_peer_data_rx *data,
				    struct cdp_rx_stats *rx)
{
	data->to_stack.num = rx->to_stack.num;
	data->to_stack.bytes = rx->to_stack.bytes;
	data->total_rcvd.num = rx->unicast.num + rx->multicast.num;
	data->total_rcvd.bytes = rx->unicast.bytes + rx->multicast.bytes;
	data->rx_error_count = rx->err.mic_err + rx->err.decrypt_err +
			       rx->err.fcserr + rx->err.pn_err +
			       rx->err.oor_err;
}

static void fill_basic_peer_ctrl_rx(struct basic_peer_ctrl_rx *ctrl,
				    struct peer_ic_cp_stats *peer_cp_stats)
{
	ctrl->cs_rx_mgmt = peer_cp_stats->cs_rx_mgmt;
	ctrl->cs_rx_decryptcrc = peer_cp_stats->cs_rx_decryptcrc;
	ctrl->cs_rx_security_failure = peer_cp_stats->cs_rx_wepfail +
				       peer_cp_stats->cs_rx_ccmpmic +
				       peer_cp_stats->cs_rx_wpimic;
}

static void fill_basic_peer_data_link(struct basic_peer_data_link *data,
				      struct cdp_rx_stats *rx)
{
	data->avg_snr = rx->avg_snr;
	data->snr = rx->snr;
	data->last_snr = rx->last_snr;
}

static void fill_basic_peer_ctrl_link(struct basic_peer_ctrl_link *ctrl,
				      struct peer_ic_cp_stats *cp_stats)
{
	ctrl->cs_rx_mgmt_snr = cp_stats->cs_rx_mgmt_snr;
}

static void fill_basic_peer_data_rate(struct basic_peer_data_rate *data,
				      struct cdp_peer_stats *peer_stats)
{
	struct cdp_tx_stats *tx = &peer_stats->tx;
	struct cdp_rx_stats *rx = &peer_stats->rx;

	data->rx_rate = rx->rx_rate;
	data->last_rx_rate = rx->last_rx_rate;
	data->tx_rate = tx->tx_rate;
	data->last_tx_rate = tx->last_tx_rate;
}

static void fill_basic_peer_ctrl_rate(struct basic_peer_ctrl_rate *ctrl,
				      struct peer_ic_cp_stats *cp_stats)
{
	ctrl->cs_rx_mgmt_rate = cp_stats->cs_rx_mgmt_rate;
}

static void fill_basic_vdev_data_tx(struct basic_vdev_data_tx *data,
				    struct cdp_vdev_stats *vdev_stats)
{
	struct cdp_tx_ingress_stats *tx_i = &vdev_stats->tx_i;
	struct cdp_tx_stats *tx = &vdev_stats->tx;

	data->ingress.num = tx_i->rcvd.num;
	data->ingress.bytes = tx_i->rcvd.bytes;
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
}

static void fill_basic_vdev_ctrl_tx(struct basic_vdev_ctrl_tx *ctrl,
				    struct vdev_ic_cp_stats *cp_stats)
{
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
}

static void fill_basic_vdev_data_rx(struct basic_vdev_data_rx *data,
				    struct cdp_rx_stats *rx)
{
	data->to_stack.num = rx->to_stack.num;
	data->to_stack.bytes = rx->to_stack.bytes;
	data->total_rcvd.num = rx->unicast.num + rx->multicast.num;
	data->total_rcvd.bytes = rx->unicast.bytes + rx->multicast.bytes;
	data->rx_error_count = rx->err.mic_err + rx->err.decrypt_err +
			       rx->err.fcserr + rx->err.pn_err +
			       rx->err.oor_err;
}

static void fill_basic_vdev_ctrl_rx(struct basic_vdev_ctrl_rx *ctrl,
				    struct vdev_ic_cp_stats *cp_stats)
{
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
}

static void fill_basic_pdev_data_tx(struct basic_pdev_data_tx *data,
				    struct cdp_pdev_stats *pdev_stats)
{
	struct cdp_tx_ingress_stats *tx_i = &pdev_stats->tx_i;
	struct cdp_tx_stats *tx = &pdev_stats->tx;

	data->ingress.num = tx_i->rcvd.num;
	data->ingress.bytes = tx_i->rcvd.bytes;
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
}

static void fill_basic_pdev_ctrl_tx(struct basic_pdev_ctrl_tx *ctrl,
				    struct pdev_ic_cp_stats *pdev_cp_stats)
{
	ctrl->cs_tx_mgmt = pdev_cp_stats->stats.cs_tx_mgmt;
	ctrl->cs_tx_frame_count = pdev_cp_stats->stats.cs_tx_frame_count;
}

static void fill_basic_pdev_data_rx(struct basic_pdev_data_rx *data,
				    struct cdp_pdev_stats *pdev_stats)
{
	struct cdp_rx_stats *rx = &pdev_stats->rx;

	data->to_stack.num = rx->to_stack.num;
	data->to_stack.bytes = rx->to_stack.bytes;
	data->total_rcvd.num = rx->unicast.num + rx->multicast.num;
	data->total_rcvd.bytes = rx->unicast.bytes + rx->multicast.bytes;
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
}

static void fill_basic_pdev_ctrl_rx(struct basic_pdev_ctrl_rx *ctrl,
				    struct pdev_ic_cp_stats *pdev_cp_stats)
{
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
}

static void fill_basic_pdev_ctrl_link(struct basic_pdev_ctrl_link *ctrl,
				      struct pdev_ic_cp_stats *cp_stats)
{
	ctrl->cs_chan_tx_pwr = cp_stats->stats.cs_chan_tx_pwr;
	ctrl->cs_rx_rssi_comb = cp_stats->stats.cs_rx_rssi_comb;
	ctrl->cs_chan_nf = cp_stats->stats.cs_chan_nf;
	ctrl->cs_chan_nf_sec80 = cp_stats->stats.cs_chan_nf_sec80;
	ctrl->dcs_total_util = cp_stats->stats.chan_stats.dcs_total_util;
}

static void fill_basic_psoc_data_tx(struct basic_psoc_data_tx *data,
				    struct cdp_soc_stats *soc_stats)
{
	data->egress.num = soc_stats->tx.egress.num;
	data->egress.bytes = soc_stats->tx.egress.bytes;
}

static void fill_basic_psoc_data_rx(struct basic_psoc_data_rx *data,
				    struct cdp_soc_stats *soc_stats)
{
	data->ingress.num = soc_stats->rx.ingress.num;
	data->ingress.bytes = soc_stats->rx.ingress.bytes;
}

static QDF_STATUS get_basic_peer_data_tx(struct unified_stats *stats,
					 struct cdp_peer_stats *peer_stats)
{
	struct basic_peer_data_tx *data = NULL;

	if (!stats || !peer_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	data = qdf_mem_malloc(sizeof(struct basic_peer_data_tx));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_peer_data_tx(data, &peer_stats->tx);

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
	fill_basic_peer_ctrl_tx(ctrl, peer_cp_stats);

	stats->tx = ctrl;
	stats->tx_size = sizeof(struct basic_peer_ctrl_tx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_peer_data_rx(struct unified_stats *stats,
					 struct cdp_peer_stats *peer_stats)
{
	struct basic_peer_data_rx *data = NULL;

	if (!stats || !peer_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	data = qdf_mem_malloc(sizeof(struct basic_peer_data_rx));
	if (!data) {
		qdf_err("Failed Allocation");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_peer_data_rx(data, &peer_stats->rx);

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
	fill_basic_peer_ctrl_rx(ctrl, peer_cp_stats);

	stats->rx = ctrl;
	stats->rx_size = sizeof(struct basic_peer_ctrl_rx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_peer_data_link(struct unified_stats *stats,
					   struct cdp_peer_stats *peer_stats)
{
	struct basic_peer_data_link *data = NULL;

	if (!stats || !peer_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	data = qdf_mem_malloc(sizeof(struct basic_peer_data_link));
	if (!data) {
		qdf_err("Failed Allocation");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_peer_data_link(data, &peer_stats->rx);

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
	fill_basic_peer_ctrl_link(ctrl, cp_stats);

	stats->link = ctrl;
	stats->link_size = sizeof(struct basic_peer_ctrl_link);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_peer_data_rate(struct unified_stats *stats,
					   struct cdp_peer_stats *peer_stats)
{
	struct basic_peer_data_rate *data = NULL;

	if (!stats || !peer_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	data = qdf_mem_malloc(sizeof(struct basic_peer_data_rate));
	if (!data) {
		qdf_err("Failed Allocation");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_peer_data_rate(data, peer_stats);

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
	fill_basic_peer_ctrl_rate(ctrl, cp_stats);

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
	fill_basic_vdev_data_tx(data, vdev_stats);

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
	fill_basic_vdev_ctrl_tx(ctrl, cp_stats);

	stats->tx = ctrl;
	stats->tx_size = sizeof(struct basic_vdev_ctrl_tx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_vdev_data_rx(struct unified_stats *stats,
					 struct cdp_vdev_stats *vdev_stats)
{
	struct basic_vdev_data_rx *data = NULL;

	if (!stats || !vdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	data = qdf_mem_malloc(sizeof(struct basic_vdev_data_rx));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_vdev_data_rx(data, &vdev_stats->rx);

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
	fill_basic_vdev_ctrl_rx(ctrl, cp_stats);

	stats->rx = ctrl;
	stats->rx_size = sizeof(struct basic_vdev_ctrl_rx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_pdev_data_tx(struct unified_stats *stats,
					 struct cdp_pdev_stats *pdev_stats)
{
	struct basic_pdev_data_tx *data = NULL;

	if (!stats || !pdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	data = qdf_mem_malloc(sizeof(struct basic_pdev_data_tx));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_pdev_data_tx(data, pdev_stats);

	stats->tx = data;
	stats->tx_size = sizeof(struct basic_pdev_data_tx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_pdev_data_rx(struct unified_stats *stats,
					 struct cdp_pdev_stats *pdev_stats)
{
	struct basic_pdev_data_rx *data = NULL;

	if (!stats || !pdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	data = qdf_mem_malloc(sizeof(struct basic_pdev_data_rx));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_pdev_data_rx(data, pdev_stats);

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
	fill_basic_pdev_ctrl_tx(ctrl, pdev_cp_stats);

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
	fill_basic_pdev_ctrl_rx(ctrl, pdev_cp_stats);

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
	fill_basic_pdev_ctrl_link(ctrl, cp_stats);

	stats->link = ctrl;
	stats->link_size = sizeof(struct basic_pdev_ctrl_link);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_psoc_data_tx(struct unified_stats *stats,
					 struct cdp_soc_stats *soc_stats)
{
	struct basic_psoc_data_tx *data = NULL;

	if (!stats || !soc_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	data = qdf_mem_malloc(sizeof(struct basic_psoc_data_tx));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_psoc_data_tx(data, soc_stats);

	stats->tx = data;
	stats->tx_size = sizeof(struct basic_psoc_data_tx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_basic_psoc_data_rx(struct unified_stats *stats,
					 struct cdp_soc_stats *soc_stats)
{
	struct basic_psoc_data_rx *data = NULL;

	if (!stats || !soc_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	data = qdf_mem_malloc(sizeof(struct basic_psoc_data_rx));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_psoc_data_rx(data, soc_stats);

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
		goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_TX) {
		ret = get_basic_peer_data_tx(stats, peer_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RX) {
		ret = get_basic_peer_data_rx(stats, peer_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_LINK) {
		ret = get_basic_peer_data_link(stats, peer_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RATE) {
		ret = get_basic_peer_data_rate(stats, peer_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}

get_failed:
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
		goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_TX) {
		ret = get_basic_peer_ctrl_tx(stats, peer_cp_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RX) {
		ret = get_basic_peer_ctrl_rx(stats, peer_cp_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_LINK) {
		ret = get_basic_peer_ctrl_link(stats, peer_cp_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RATE) {
		ret = get_basic_peer_ctrl_rate(stats, peer_cp_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}

get_failed:
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
		goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_TX) {
		ret = get_basic_vdev_data_tx(stats, vdev_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RX) {
		ret = get_basic_vdev_data_rx(stats, vdev_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}

get_failed:
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
		goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_TX) {
		ret = get_basic_vdev_ctrl_tx(stats, cp_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RX) {
		ret = get_basic_vdev_ctrl_rx(stats, cp_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}

get_failed:
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
		goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_TX) {
		ret = get_basic_pdev_data_tx(stats, pdev_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RX) {
		ret = get_basic_pdev_data_rx(stats, pdev_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}

get_failed:
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
		goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_TX) {
		ret = get_basic_pdev_ctrl_tx(stats, pdev_cp_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RX) {
		ret = get_basic_pdev_ctrl_rx(stats, pdev_cp_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_LINK) {
		ret = get_basic_pdev_ctrl_link(stats, pdev_cp_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}

get_failed:
	qdf_mem_free(pdev_cp_stats);

	return ret;
}

static QDF_STATUS get_basic_psoc_data(struct wlan_objmgr_psoc *psoc,
				      struct unified_stats *stats,
				      uint32_t feat)
{
	struct cdp_soc_stats *psoc_stats = NULL;
	QDF_STATUS ret = QDF_STATUS_SUCCESS;

	if (!psoc) {
		qdf_err("Invalid psoc!");
		return QDF_STATUS_E_INVAL;
	}
	psoc_stats = qdf_mem_malloc(sizeof(struct cdp_soc_stats));
	if (!psoc_stats) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	ret = cdp_host_get_soc_stats(wlan_psoc_get_dp_handle(psoc), psoc_stats);
	if (QDF_STATUS_SUCCESS != ret) {
		qdf_err("Unable to get Psoc stats!");
		goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_TX) {
		ret = get_basic_psoc_data_tx(stats, psoc_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RX) {
		ret = get_basic_psoc_data_rx(stats, psoc_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}

get_failed:
	qdf_mem_free(psoc_stats);

	return ret;
}

#if WLAN_ADVANCE_TELEMETRY
static QDF_STATUS get_advance_peer_data_tx(struct unified_stats *stats,
					   struct cdp_peer_stats *peer_stats)
{
	struct advance_peer_data_tx *data = NULL;
	struct cdp_tx_stats *tx = NULL;
	uint8_t inx = 0;

	if (!stats || !peer_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	tx = &peer_stats->tx;
	data = qdf_mem_malloc(sizeof(struct advance_peer_data_tx));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_peer_data_tx(&data->b_tx, tx);

	data->ucast.num = tx->ucast.num;
	data->ucast.bytes = tx->ucast.bytes;
	data->mcast.num = tx->mcast.num;
	data->mcast.bytes = tx->mcast.bytes;
	data->bcast.num = tx->bcast.num;
	data->bcast.bytes = tx->bcast.bytes;
	for (inx = 0; inx < MAX_GI; inx++)
		data->sgi_count[inx] = tx->sgi_count[inx];
	for (inx = 0; inx < SS_COUNT; inx++)
		data->nss[inx] = tx->nss[inx];
	for (inx = 0; inx < MAX_BW; inx++)
		data->bw[inx] = tx->bw[inx];
	data->retries = tx->retries;
	data->non_amsdu_cnt = tx->non_amsdu_cnt;
	data->amsdu_cnt = tx->amsdu_cnt;
	data->ampdu_cnt = tx->ampdu_cnt;
	data->non_ampdu_cnt = tx->non_ampdu_cnt;

	stats->tx = data;
	stats->tx_size = sizeof(struct advance_peer_data_tx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_peer_data_rx(struct unified_stats *stats,
					   struct cdp_peer_stats *peer_stats)
{
	struct advance_peer_data_rx *data = NULL;
	struct cdp_rx_stats *rx = NULL;
	uint8_t inx = 0;

	if (!stats || !peer_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	data = qdf_mem_malloc(sizeof(struct advance_peer_data_rx));
	if (!data) {
		qdf_err("Failed Allocation");
		return QDF_STATUS_E_NOMEM;
	}
	rx = &peer_stats->rx;
	fill_basic_peer_data_rx(&data->b_rx, rx);

	data->unicast.num = rx->unicast.num;
	data->unicast.bytes = rx->unicast.bytes;
	data->multicast.num = rx->multicast.num;
	data->multicast.bytes = rx->multicast.bytes;
	data->bcast.num = rx->bcast.num;
	data->bcast.bytes = rx->bcast.bytes;
	for (inx = 0; inx < MAX_MCS; inx++)
		data->su_ax_ppdu_cnt[inx] = rx->su_ax_ppdu_cnt.mcs_count[inx];
	for (inx = 0; inx < WME_AC_MAX; inx++)
		data->wme_ac_type[inx] = rx->wme_ac_type[inx];
	for (inx = 0; inx < MAX_GI; inx++)
		data->sgi_count[inx] = rx->sgi_count[inx];
	for (inx = 0; inx < SS_COUNT; inx++)
		data->nss[inx] = rx->nss[inx];
	for (inx = 0; inx < SS_COUNT; inx++)
		data->ppdu_nss[inx] = rx->ppdu_nss[inx];
	for (inx = 0; inx < MAX_BW; inx++)
		data->bw[inx] = rx->bw[inx];
	for (inx = 0; inx < MAX_MCS; inx++)
		data->rx_mpdu_cnt[inx] = rx->rx_mpdu_cnt[inx];
	data->mpdu_cnt_fcs_ok = rx->mpdu_cnt_fcs_ok;
	data->mpdu_cnt_fcs_err = rx->mpdu_cnt_fcs_err;
	data->non_amsdu_cnt = rx->non_amsdu_cnt;
	data->non_ampdu_cnt = rx->non_ampdu_cnt;
	data->ampdu_cnt = rx->ampdu_cnt;
	data->amsdu_cnt = rx->amsdu_cnt;
	data->bar_recv_cnt = rx->bar_recv_cnt;
	data->rx_retries = rx->rx_retries;
	data->multipass_rx_pkt_drop = rx->multipass_rx_pkt_drop;

	stats->rx = data;
	stats->rx_size = sizeof(struct advance_peer_data_rx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_peer_data_link(struct unified_stats *stats,
					     struct cdp_peer_stats *peer_stats)
{
	struct advance_peer_data_link *data = NULL;
	struct cdp_rx_stats *rx = NULL;

	if (!stats || !peer_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	rx = &peer_stats->rx;
	data = qdf_mem_malloc(sizeof(struct advance_peer_data_link));
	if (!data) {
		qdf_err("Failed Allocation");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_peer_data_link(&data->b_link, rx);

	data->rx_snr_measured_time = rx->rx_snr_measured_time;

	stats->link = data;
	stats->link_size = sizeof(struct advance_peer_data_link);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_peer_data_rate(struct unified_stats *stats,
					     struct cdp_peer_stats *peer_stats)
{
	struct advance_peer_data_rate *data = NULL;
	struct cdp_tx_stats *tx = NULL;
	struct cdp_rx_stats *rx = NULL;

	if (!stats || !peer_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	tx = &peer_stats->tx;
	rx = &peer_stats->rx;
	data = qdf_mem_malloc(sizeof(struct advance_peer_data_rate));
	if (!data) {
		qdf_err("Failed Allocation");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_peer_data_rate(&data->b_rate, peer_stats);

	data->rnd_avg_rx_rate = rx->rnd_avg_rx_rate;
	data->avg_rx_rate = rx->avg_rx_rate;
	data->rnd_avg_tx_rate = tx->rnd_avg_tx_rate;
	data->avg_tx_rate = tx->avg_tx_rate;

	stats->rate = data;
	stats->rate_size = sizeof(struct advance_peer_data_rate);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_peer_data_raw(struct unified_stats *stats,
					    struct cdp_peer_stats *peer_stats)
{
	struct advance_peer_data_raw *data = NULL;
	struct cdp_rx_stats *rx = NULL;

	if (!stats || !peer_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	rx = &peer_stats->rx;
	data = qdf_mem_malloc(sizeof(struct advance_peer_data_raw));
	if (!data) {
		qdf_err("Failed Allocation");
		return QDF_STATUS_E_NOMEM;
	}
	data->raw.num = rx->raw.num;
	data->raw.bytes = rx->raw.bytes;

	stats->raw = data;
	stats->raw_size = sizeof(struct advance_peer_data_raw);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_peer_data_fwd(struct unified_stats *stats,
					    struct cdp_peer_stats *peer_stats)
{
	struct advance_peer_data_fwd *data = NULL;
	struct cdp_rx_stats *rx = NULL;

	if (!stats || !peer_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	rx = &peer_stats->rx;
	data = qdf_mem_malloc(sizeof(struct advance_peer_data_fwd));
	if (!data) {
		qdf_err("Failed Allocation");
		return QDF_STATUS_E_NOMEM;
	}
	data->pkts.num = rx->intra_bss.pkts.num;
	data->pkts.bytes = rx->intra_bss.pkts.bytes;
	data->fail.num = rx->intra_bss.fail.num;
	data->fail.bytes = rx->intra_bss.fail.bytes;
	data->mdns_no_fwd = rx->intra_bss.mdns_no_fwd;

	stats->fwd = data;
	stats->fwd_size = sizeof(struct advance_peer_data_fwd);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_peer_data_twt(struct unified_stats *stats,
					    struct cdp_peer_stats *peer_stats)
{
	struct advance_peer_data_twt *data = NULL;
	struct cdp_rx_stats *rx = NULL;
	struct cdp_tx_stats *tx = NULL;

	if (!stats || !peer_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	rx = &peer_stats->rx;
	tx = &peer_stats->tx;
	data = qdf_mem_malloc(sizeof(struct advance_peer_data_twt));
	if (!data) {
		qdf_err("Failed Allocation");
		return QDF_STATUS_E_NOMEM;
	}
	data->to_stack_twt.num = rx->to_stack_twt.num;
	data->to_stack_twt.bytes = rx->to_stack_twt.bytes;
	data->tx_success_twt.num = tx->tx_success_twt.num;
	data->tx_success_twt.bytes = tx->tx_success_twt.bytes;

	stats->twt = data;
	stats->twt_size = sizeof(struct advance_peer_data_twt);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_peer_data_nawds(struct unified_stats *stats,
					      struct cdp_peer_stats *peer_stats)
{
	struct advance_peer_data_nawds *data = NULL;
	struct cdp_rx_stats *rx = NULL;
	struct cdp_tx_stats *tx = NULL;

	if (!stats || !peer_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	rx = &peer_stats->rx;
	tx = &peer_stats->tx;
	data = qdf_mem_malloc(sizeof(struct advance_peer_data_nawds));
	if (!data) {
		qdf_err("Failed Allocation");
		return QDF_STATUS_E_NOMEM;
	}
	data->nawds_mcast.num = tx->nawds_mcast.num;
	data->nawds_mcast.bytes = tx->nawds_mcast.bytes;
	data->nawds_mcast_tx_drop = tx->nawds_mcast_drop;
	data->nawds_mcast_rx_drop = rx->nawds_mcast_drop;

	stats->nawds = data;
	stats->nawds_size = sizeof(struct advance_peer_data_nawds);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_peer_ctrl_tx(struct unified_stats *stats,
					   struct peer_ic_cp_stats *cp_stats)
{
	struct advance_peer_ctrl_tx *ctrl = NULL;

	if (!stats || !cp_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	ctrl = qdf_mem_malloc(sizeof(struct advance_peer_ctrl_tx));
	if (!ctrl) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_peer_ctrl_tx(&ctrl->b_tx, cp_stats);

	ctrl->cs_tx_assoc = cp_stats->cs_tx_assoc;
	ctrl->cs_tx_assoc_fail = cp_stats->cs_tx_assoc_fail;

	stats->tx = ctrl;
	stats->tx_size = sizeof(struct advance_peer_ctrl_tx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_peer_ctrl_rx(struct unified_stats *stats,
					   struct peer_ic_cp_stats *cp_stats)
{
	struct advance_peer_ctrl_rx *ctrl = NULL;

	if (!stats || !cp_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	ctrl = qdf_mem_malloc(sizeof(struct advance_peer_ctrl_rx));
	if (!ctrl) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_peer_ctrl_rx(&ctrl->b_rx, cp_stats);

	stats->rx = ctrl;
	stats->rx_size = sizeof(struct advance_peer_ctrl_rx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_peer_ctrl_twt(struct unified_stats *stats,
					    struct peer_ic_cp_stats *cp_stats)
{
	struct advance_peer_ctrl_twt *ctrl = NULL;

	if (!stats || !cp_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	ctrl = qdf_mem_malloc(sizeof(struct advance_peer_ctrl_twt));
	if (!ctrl) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	ctrl->cs_twt_event_type = cp_stats->cs_twt_event_type;
	ctrl->cs_twt_flow_id = cp_stats->cs_twt_flow_id;
	ctrl->cs_twt_bcast = cp_stats->cs_twt_bcast;
	ctrl->cs_twt_trig = cp_stats->cs_twt_trig;
	ctrl->cs_twt_announ = cp_stats->cs_twt_announ;
	ctrl->cs_twt_dialog_id = cp_stats->cs_twt_dialog_id;
	ctrl->cs_twt_wake_dura_us = cp_stats->cs_twt_wake_dura_us;
	ctrl->cs_twt_wake_intvl_us = cp_stats->cs_twt_wake_intvl_us;
	ctrl->cs_twt_sp_offset_us = cp_stats->cs_twt_sp_offset_us;

	stats->twt = ctrl;
	stats->twt_size = sizeof(struct advance_peer_ctrl_twt);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_peer_ctrl_link(struct unified_stats *stats,
					     struct peer_ic_cp_stats *cp_stats)
{
	struct advance_peer_ctrl_link *ctrl = NULL;

	if (!stats || !cp_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	ctrl = qdf_mem_malloc(sizeof(struct advance_peer_ctrl_link));
	if (!ctrl) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_peer_ctrl_link(&ctrl->b_link, cp_stats);

	stats->link = ctrl;
	stats->link_size = sizeof(struct advance_peer_ctrl_link);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_peer_ctrl_rate(struct unified_stats *stats,
					     struct peer_ic_cp_stats *cp_stats)
{
	struct advance_peer_ctrl_rate *ctrl = NULL;

	if (!stats || !cp_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	ctrl = qdf_mem_malloc(sizeof(struct advance_peer_ctrl_rate));
	if (!ctrl) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_peer_ctrl_rate(&ctrl->b_rate, cp_stats);

	stats->rate = ctrl;
	stats->rate_size = sizeof(struct advance_peer_ctrl_rate);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_peer_data(struct wlan_objmgr_psoc *psoc,
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
		goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_TX) {
		ret = get_advance_peer_data_tx(stats, peer_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RX) {
		ret = get_advance_peer_data_rx(stats, peer_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_LINK) {
		ret = get_advance_peer_data_link(stats, peer_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RATE) {
		ret = get_advance_peer_data_rate(stats, peer_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RAW) {
		ret = get_advance_peer_data_raw(stats, peer_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_FWD) {
		ret = get_advance_peer_data_fwd(stats, peer_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_TWT) {
		ret = get_advance_peer_data_twt(stats, peer_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_NAWDS) {
		ret = get_advance_peer_data_nawds(stats, peer_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}

get_failed:
	qdf_mem_free(peer_stats);

	return ret;
}

static QDF_STATUS get_advance_peer_ctrl(struct wlan_objmgr_peer *peer,
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
		goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RX) {
		ret = get_advance_peer_ctrl_rx(stats, peer_cp_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_TX) {
		ret = get_advance_peer_ctrl_tx(stats, peer_cp_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_TWT) {
		ret = get_advance_peer_ctrl_twt(stats, peer_cp_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_LINK) {
		ret = get_advance_peer_ctrl_link(stats, peer_cp_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RATE) {
		ret = get_advance_peer_ctrl_rate(stats, peer_cp_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}

get_failed:
	qdf_mem_free(peer_cp_stats);

	return ret;
}

static QDF_STATUS get_advance_vdev_data_tx(struct unified_stats *stats,
					   struct cdp_vdev_stats *vdev_stats)
{
	struct advance_vdev_data_tx *data = NULL;
	struct cdp_tx_stats *tx = NULL;
	struct cdp_tx_ingress_stats *tx_i = NULL;
	uint8_t inx = 0;

	if (!stats || !vdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	tx_i = &vdev_stats->tx_i;
	tx = &vdev_stats->tx;
	data = qdf_mem_malloc(sizeof(struct advance_vdev_data_tx));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_vdev_data_tx(&data->b_tx, vdev_stats);

	data->reinject_pkts.num = tx_i->reinject_pkts.num;
	data->reinject_pkts.bytes = tx_i->reinject_pkts.bytes;
	data->inspect_pkts.num = tx_i->inspect_pkts.num;
	data->inspect_pkts.bytes = tx_i->inspect_pkts.bytes;
	data->ucast.num = tx->ucast.num;
	data->ucast.bytes = tx->ucast.bytes;
	data->mcast.num = tx->mcast.num;
	data->mcast.bytes = tx->mcast.bytes;
	data->bcast.num = tx_i->bcast.num;
	data->bcast.bytes = tx_i->bcast.bytes;
	for (inx = 0; inx < MAX_GI; inx++)
		data->sgi_count[inx] = tx->sgi_count[inx];
	for (inx = 0; inx < SS_COUNT; inx++)
		data->nss[inx] = tx->nss[inx];
	for (inx = 0; inx < MAX_BW; inx++)
		data->bw[inx] = tx->bw[inx];
	data->retries = tx->retries;
	data->non_amsdu_cnt = tx->non_amsdu_cnt;
	data->amsdu_cnt = tx->amsdu_cnt;
	data->ampdu_cnt = tx->ampdu_cnt;
	data->non_ampdu_cnt = tx->non_ampdu_cnt;
	data->cce_classified = tx_i->cce_classified;

	stats->tx = data;
	stats->tx_size = sizeof(struct advance_vdev_data_tx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_vdev_data_rx(struct unified_stats *stats,
					   struct cdp_vdev_stats *vdev_stats)
{
	struct advance_vdev_data_rx *data = NULL;
	struct cdp_rx_stats *rx = NULL;
	uint8_t inx = 0;

	if (!stats || !vdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	rx = &vdev_stats->rx;
	data = qdf_mem_malloc(sizeof(struct advance_vdev_data_rx));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_vdev_data_rx(&data->b_rx, rx);

	data->unicast.num = rx->unicast.num;
	data->unicast.bytes = rx->unicast.bytes;
	data->multicast.num = rx->multicast.num;
	data->multicast.bytes = rx->multicast.bytes;
	data->bcast.num = rx->bcast.num;
	data->bcast.bytes = rx->bcast.bytes;
	for (inx = 0; inx < MAX_MCS; inx++)
		data->su_ax_ppdu_cnt[inx] = rx->su_ax_ppdu_cnt.mcs_count[inx];
	for (inx = 0; inx < WME_AC_MAX; inx++)
		data->wme_ac_type[inx] = rx->wme_ac_type[inx];
	for (inx = 0; inx < MAX_GI; inx++)
		data->sgi_count[inx] = rx->sgi_count[inx];
	for (inx = 0; inx < SS_COUNT; inx++)
		data->nss[inx] = rx->nss[inx];
	for (inx = 0; inx < SS_COUNT; inx++)
		data->ppdu_nss[inx] = rx->ppdu_nss[inx];
	for (inx = 0; inx < MAX_BW; inx++)
		data->bw[inx] = rx->bw[inx];
	for (inx = 0; inx < MAX_MCS; inx++)
		data->rx_mpdu_cnt[inx] = rx->rx_mpdu_cnt[inx];
	data->mpdu_cnt_fcs_ok = rx->mpdu_cnt_fcs_ok;
	data->mpdu_cnt_fcs_err = rx->mpdu_cnt_fcs_err;
	data->non_amsdu_cnt = rx->non_amsdu_cnt;
	data->non_ampdu_cnt = rx->non_ampdu_cnt;
	data->ampdu_cnt = rx->ampdu_cnt;
	data->amsdu_cnt = rx->amsdu_cnt;
	data->bar_recv_cnt = rx->bar_recv_cnt;
	data->rx_retries = rx->rx_retries;
	data->multipass_rx_pkt_drop = rx->multipass_rx_pkt_drop;

	stats->rx = data;
	stats->rx_size = sizeof(struct advance_vdev_data_rx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_vdev_data_me(struct unified_stats *stats,
					   struct cdp_vdev_stats *vdev_stats)
{
	struct advance_vdev_data_me *data = NULL;
	struct cdp_tx_ingress_stats *tx_i = NULL;

	if (!stats || !vdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	tx_i = &vdev_stats->tx_i;
	data = qdf_mem_malloc(sizeof(struct advance_vdev_data_me));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	data->mcast_pkt.num = tx_i->mcast_en.mcast_pkt.num;
	data->mcast_pkt.bytes = tx_i->mcast_en.mcast_pkt.bytes;
	data->ucast = tx_i->mcast_en.ucast;

	stats->me = data;
	stats->me_size = sizeof(struct advance_vdev_data_me);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_vdev_data_raw(struct unified_stats *stats,
					    struct cdp_vdev_stats *vdev_stats)
{
	struct advance_vdev_data_raw *data = NULL;
	struct cdp_tx_ingress_stats *tx_i = NULL;
	struct cdp_rx_stats *rx = NULL;

	if (!stats || !vdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	tx_i = &vdev_stats->tx_i;
	rx = &vdev_stats->rx;
	data = qdf_mem_malloc(sizeof(struct advance_vdev_data_raw));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	data->rx_raw.num = rx->raw.num;
	data->rx_raw.bytes = rx->raw.bytes;
	data->tx_raw_pkt.num = tx_i->raw.raw_pkt.num;
	data->tx_raw_pkt.bytes = tx_i->raw.raw_pkt.bytes;
	data->cce_classified_raw = tx_i->cce_classified_raw;

	stats->raw = data;
	stats->raw_size = sizeof(struct advance_vdev_data_raw);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_vdev_data_tso(struct unified_stats *stats,
					    struct cdp_vdev_stats *vdev_stats)
{
	struct advance_vdev_data_tso *data = NULL;
	struct cdp_tso_stats *tso = NULL;
	struct cdp_tx_ingress_stats *tx_i = NULL;

	if (!stats || !vdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	tx_i = &vdev_stats->tx_i;
	tso = &tx_i->tso_stats;
	data = qdf_mem_malloc(sizeof(struct advance_vdev_data_tso));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	data->sg_pkt.num = tx_i->sg.sg_pkt.num;
	data->sg_pkt.bytes = tx_i->sg.sg_pkt.bytes;
	data->non_sg_pkts.num = tx_i->sg.non_sg_pkts.num;
	data->non_sg_pkts.bytes = tx_i->sg.non_sg_pkts.bytes;
	data->num_tso_pkts.num = tso->num_tso_pkts.num;
	data->num_tso_pkts.bytes = tso->num_tso_pkts.bytes;

	stats->tso = data;
	stats->tso_size = sizeof(struct advance_vdev_data_tso);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_vdev_data_igmp(struct unified_stats *stats,
					     struct cdp_vdev_stats *vdev_stats)
{
	struct advance_vdev_data_igmp *data = NULL;
	struct cdp_tx_ingress_stats *tx_i = NULL;

	if (!stats || !vdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	tx_i = &vdev_stats->tx_i;
	data = qdf_mem_malloc(sizeof(struct advance_vdev_data_igmp));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	data->igmp_rcvd = tx_i->igmp_mcast_en.igmp_rcvd;
	data->igmp_ucast_converted = tx_i->igmp_mcast_en.igmp_ucast_converted;

	stats->igmp = data;
	stats->igmp_size = sizeof(struct advance_vdev_data_igmp);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_vdev_data_mesh(struct unified_stats *stats,
					     struct cdp_vdev_stats *vdev_stats)
{
	struct advance_vdev_data_mesh *data = NULL;
	struct cdp_tx_ingress_stats *tx_i = NULL;

	if (!stats || !vdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	tx_i = &vdev_stats->tx_i;
	data = qdf_mem_malloc(sizeof(struct advance_vdev_data_mesh));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	data->exception_fw = tx_i->mesh.exception_fw;
	data->completion_fw = tx_i->mesh.completion_fw;

	stats->mesh = data;
	stats->mesh_size = sizeof(struct advance_vdev_data_mesh);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_vdev_data_nawds(struct unified_stats *stats,
					      struct cdp_vdev_stats *vdev_stats)
{
	struct advance_vdev_data_nawds *data = NULL;
	struct cdp_rx_stats *rx = NULL;
	struct cdp_tx_stats *tx = NULL;

	if (!stats || !vdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	rx = &vdev_stats->rx;
	tx = &vdev_stats->tx;
	data = qdf_mem_malloc(sizeof(struct advance_vdev_data_nawds));
	if (!data) {
		qdf_err("Failed Allocation");
		return QDF_STATUS_E_NOMEM;
	}
	data->tx_nawds_mcast.num = tx->nawds_mcast.num;
	data->tx_nawds_mcast.bytes = tx->nawds_mcast.bytes;
	data->nawds_mcast_tx_drop = tx->nawds_mcast_drop;
	data->nawds_mcast_rx_drop = rx->nawds_mcast_drop;

	stats->nawds = data;
	stats->nawds_size = sizeof(struct advance_vdev_data_nawds);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_vdev_ctrl_tx(struct unified_stats *stats,
					   struct vdev_ic_cp_stats *cp_stats)
{
	struct advance_vdev_ctrl_tx *ctrl = NULL;

	if (!stats || !cp_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	ctrl = qdf_mem_malloc(sizeof(struct advance_vdev_ctrl_tx));
	if (!ctrl) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_vdev_ctrl_tx(&ctrl->b_tx, cp_stats);

	ctrl->cs_tx_offchan_mgmt = cp_stats->stats.cs_tx_offchan_mgmt;
	ctrl->cs_tx_offchan_data = cp_stats->stats.cs_tx_offchan_data;
	ctrl->cs_tx_offchan_fail = cp_stats->stats.cs_tx_offchan_fail;
	ctrl->cs_tx_bcn_success = cp_stats->stats.cs_tx_bcn_success;
	ctrl->cs_tx_bcn_outage = cp_stats->stats.cs_tx_bcn_outage;
	ctrl->cs_fils_frames_sent = cp_stats->stats.cs_fils_frames_sent;
	ctrl->cs_fils_frames_sent_fail =
		cp_stats->stats.cs_fils_frames_sent_fail;
	ctrl->cs_tx_offload_prb_resp_succ_cnt =
		cp_stats->stats.cs_tx_offload_prb_resp_succ_cnt;
	ctrl->cs_tx_offload_prb_resp_fail_cnt =
		cp_stats->stats.cs_tx_offload_prb_resp_fail_cnt;

	stats->tx = ctrl;
	stats->tx_size = sizeof(struct advance_vdev_ctrl_tx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_vdev_ctrl_rx(struct unified_stats *stats,
					   struct vdev_ic_cp_stats *cp_stats)
{
	struct advance_vdev_ctrl_rx *ctrl = NULL;

	if (!stats || !cp_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	ctrl = qdf_mem_malloc(sizeof(struct advance_vdev_ctrl_rx));
	if (!ctrl) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_vdev_ctrl_rx(&ctrl->b_rx, cp_stats);

	ctrl->cs_rx_action = cp_stats->stats.cs_rx_action;
	ctrl->cs_mlme_auth_attempt = cp_stats->stats.cs_mlme_auth_attempt;
	ctrl->cs_mlme_auth_success = cp_stats->stats.cs_mlme_auth_success;
	ctrl->cs_authorize_attempt = cp_stats->stats.cs_authorize_attempt;
	ctrl->cs_authorize_success = cp_stats->stats.cs_authorize_success;
	ctrl->cs_prob_req_drops = cp_stats->stats.cs_prob_req_drops;
	ctrl->cs_oob_probe_req_count = cp_stats->stats.cs_oob_probe_req_count;
	ctrl->cs_wc_probe_req_drops = cp_stats->stats.cs_wc_probe_req_drops;
	ctrl->cs_sta_xceed_rlim = cp_stats->stats.cs_sta_xceed_rlim;
	ctrl->cs_sta_xceed_vlim = cp_stats->stats.cs_sta_xceed_vlim;

	stats->rx = ctrl;
	stats->rx_size = sizeof(struct advance_vdev_ctrl_rx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_vdev_data(struct wlan_objmgr_psoc *psoc,
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
		goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_TX) {
		ret = get_advance_vdev_data_tx(stats, vdev_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RX) {
		ret = get_advance_vdev_data_rx(stats, vdev_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_ME) {
		ret = get_advance_vdev_data_me(stats, vdev_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RAW) {
		ret = get_advance_vdev_data_raw(stats, vdev_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_TSO) {
		ret = get_advance_vdev_data_tso(stats, vdev_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_IGMP) {
		ret = get_advance_vdev_data_igmp(stats, vdev_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_MESH) {
		ret = get_advance_vdev_data_mesh(stats, vdev_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_NAWDS) {
		ret = get_advance_vdev_data_nawds(stats, vdev_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}

get_failed:
	qdf_mem_free(vdev_stats);

	return ret;
}

static QDF_STATUS get_vdev_cp_stats(struct wlan_objmgr_vdev *vdev,
				    struct vdev_ic_cp_stats *cp_stats)
{
	qdf_event_t wait_event;
	struct ieee80211vap *vap;

	vap = wlan_vdev_mlme_get_ext_hdl(vdev);
	if (!vap) {
		qdf_err("vap is NULL!");
		return QDF_STATUS_E_INVAL;
	}

	if (vap->get_vdev_bcn_stats &&
	    (CONVERT_SYSTEM_TIME_TO_MS(OS_GET_TICKS() -
	    vap->vap_bcn_stats_time) > 2000)) {
		qdf_mem_zero(&wait_event, sizeof(wait_event));
		qdf_event_create(&wait_event);
		qdf_event_reset(&wait_event);
		vap->get_vdev_bcn_stats(vap);
		/* give enough delay in ms (50ms) to get beacon stats */
		qdf_wait_single_event(&wait_event, 50);
		if (qdf_atomic_read(&vap->vap_bcn_event) != 1)
			qdf_debug("VAP BCN STATS FAILED");
		qdf_event_destroy(&wait_event);
	}
	if (vap->get_vdev_prb_fils_stats) {
		qdf_mem_zero(&wait_event, sizeof(wait_event));
		qdf_event_create(&wait_event);
		qdf_event_reset(&wait_event);
		qdf_atomic_init(&vap->vap_prb_fils_event);
		vap->get_vdev_prb_fils_stats(vap);
		/* give enough delay in ms (50ms) to get fils stats */
		qdf_wait_single_event(&wait_event, 50);
		if (qdf_atomic_read(&vap->vap_prb_fils_event) != 1)
			qdf_debug("VAP PRB and FILS STATS FAILED");
		qdf_event_destroy(&wait_event);
	}

	return wlan_cfg80211_get_vdev_cp_stats(vdev, cp_stats);
}

static QDF_STATUS get_advance_vdev_ctrl(struct wlan_objmgr_vdev *vdev,
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
	ret = get_vdev_cp_stats(vdev, cp_stats);
	if (QDF_STATUS_SUCCESS != ret) {
		qdf_err("Unable to get Vdev Control stats!");
		goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_TX) {
		ret = get_advance_vdev_ctrl_tx(stats, cp_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RX) {
		ret = get_advance_vdev_ctrl_rx(stats, cp_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}

get_failed:
	qdf_mem_free(cp_stats);

	return ret;
}

static QDF_STATUS get_advance_pdev_data_tx(struct unified_stats *stats,
					   struct cdp_pdev_stats *pdev_stats)
{
	struct advance_pdev_data_tx *data = NULL;
	struct cdp_tx_ingress_stats *tx_i = NULL;
	struct cdp_tx_stats *tx = NULL;
	struct cdp_hist_tx_comp *hist = NULL;
	uint8_t inx = 0;

	if (!stats || !pdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	tx_i = &pdev_stats->tx_i;
	tx = &pdev_stats->tx;
	hist = &pdev_stats->tx_comp_histogram;
	data = qdf_mem_malloc(sizeof(struct advance_pdev_data_tx));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_pdev_data_tx(&data->b_tx, pdev_stats);

	data->reinject_pkts.num = tx_i->reinject_pkts.num;
	data->reinject_pkts.bytes = tx_i->reinject_pkts.bytes;
	data->inspect_pkts.num = tx_i->inspect_pkts.num;
	data->inspect_pkts.bytes = tx_i->inspect_pkts.bytes;
	data->ucast.num = tx->ucast.num;
	data->ucast.bytes = tx->ucast.bytes;
	data->mcast.num = tx->mcast.num;
	data->mcast.bytes = tx->mcast.bytes;
	data->bcast.num = tx_i->bcast.num;
	data->bcast.bytes = tx_i->bcast.bytes;
	for (inx = 0; inx < MAX_GI; inx++)
		data->sgi_count[inx] = tx->sgi_count[inx];
	for (inx = 0; inx < SS_COUNT; inx++)
		data->nss[inx] = tx->nss[inx];
	for (inx = 0; inx < MAX_BW; inx++)
		data->bw[inx] = tx->bw[inx];
	data->retries = tx->retries;
	data->non_amsdu_cnt = tx->non_amsdu_cnt;
	data->amsdu_cnt = tx->amsdu_cnt;
	data->ampdu_cnt = tx->ampdu_cnt;
	data->non_ampdu_cnt = tx->non_ampdu_cnt;
	data->cce_classified = tx_i->cce_classified;
	data->tx_hist.pkts_1 = hist->pkts_1;
	data->tx_hist.pkts_2_20 = hist->pkts_2_20;
	data->tx_hist.pkts_21_40 = hist->pkts_21_40;
	data->tx_hist.pkts_41_60 = hist->pkts_41_60;
	data->tx_hist.pkts_61_80 = hist->pkts_61_80;
	data->tx_hist.pkts_81_100 = hist->pkts_81_100;
	data->tx_hist.pkts_101_200 = hist->pkts_101_200;
	data->tx_hist.pkts_201_plus = hist->pkts_201_plus;

	stats->tx = data;
	stats->tx_size = sizeof(struct advance_pdev_data_tx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_pdev_data_rx(struct unified_stats *stats,
					   struct cdp_pdev_stats *pdev_stats)
{
	struct advance_pdev_data_rx *data = NULL;
	struct cdp_rx_stats *rx = NULL;
	struct cdp_hist_rx_ind *hist = NULL;
	uint8_t inx = 0;

	if (!stats || !pdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	rx = &pdev_stats->rx;
	hist = &pdev_stats->rx_ind_histogram;
	data = qdf_mem_malloc(sizeof(struct advance_pdev_data_rx));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_pdev_data_rx(&data->b_rx, pdev_stats);

	data->unicast.num = rx->unicast.num;
	data->unicast.bytes = rx->unicast.bytes;
	data->multicast.num = rx->multicast.num;
	data->multicast.bytes = rx->multicast.bytes;
	data->bcast.num = rx->bcast.num;
	data->bcast.bytes = rx->bcast.bytes;
	for (inx = 0; inx < MAX_MCS; inx++)
		data->su_ax_ppdu_cnt[inx] = rx->su_ax_ppdu_cnt.mcs_count[inx];
	for (inx = 0; inx < WME_AC_MAX; inx++)
		data->wme_ac_type[inx] = rx->wme_ac_type[inx];
	for (inx = 0; inx < MAX_GI; inx++)
		data->sgi_count[inx] = rx->sgi_count[inx];
	for (inx = 0; inx < SS_COUNT; inx++)
		data->nss[inx] = rx->nss[inx];
	for (inx = 0; inx < SS_COUNT; inx++)
		data->ppdu_nss[inx] = rx->ppdu_nss[inx];
	for (inx = 0; inx < MAX_BW; inx++)
		data->bw[inx] = rx->bw[inx];
	for (inx = 0; inx < MAX_MCS; inx++)
		data->rx_mpdu_cnt[inx] = rx->rx_mpdu_cnt[inx];
	data->mpdu_cnt_fcs_ok = rx->mpdu_cnt_fcs_ok;
	data->mpdu_cnt_fcs_err = rx->mpdu_cnt_fcs_err;
	data->non_amsdu_cnt = rx->non_amsdu_cnt;
	data->non_ampdu_cnt = rx->non_ampdu_cnt;
	data->ampdu_cnt = rx->ampdu_cnt;
	data->amsdu_cnt = rx->amsdu_cnt;
	data->bar_recv_cnt = rx->bar_recv_cnt;
	data->rx_retries = rx->rx_retries;
	data->multipass_rx_pkt_drop = rx->multipass_rx_pkt_drop;
	data->rx_hist.pkts_1 = hist->pkts_1;
	data->rx_hist.pkts_2_20 = hist->pkts_2_20;
	data->rx_hist.pkts_21_40 = hist->pkts_21_40;
	data->rx_hist.pkts_41_60 = hist->pkts_41_60;
	data->rx_hist.pkts_61_80 = hist->pkts_61_80;
	data->rx_hist.pkts_81_100 = hist->pkts_81_100;
	data->rx_hist.pkts_101_200 = hist->pkts_101_200;
	data->rx_hist.pkts_201_plus = hist->pkts_201_plus;

	stats->rx = data;
	stats->rx_size = sizeof(struct advance_pdev_data_rx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_pdev_data_me(struct unified_stats *stats,
					   struct cdp_pdev_stats *pdev_stats)
{
	struct advance_pdev_data_me *data = NULL;
	struct cdp_tx_ingress_stats *tx_i = NULL;

	if (!stats || !pdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	tx_i = &pdev_stats->tx_i;
	data = qdf_mem_malloc(sizeof(struct advance_pdev_data_me));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	data->mcast_pkt.num = tx_i->mcast_en.mcast_pkt.num;
	data->mcast_pkt.bytes = tx_i->mcast_en.mcast_pkt.bytes;
	data->ucast = tx_i->mcast_en.ucast;

	stats->me = data;
	stats->me_size = sizeof(struct advance_pdev_data_me);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_pdev_data_raw(struct unified_stats *stats,
					    struct cdp_pdev_stats *pdev_stats)
{
	struct advance_pdev_data_raw *data = NULL;
	struct cdp_tx_ingress_stats *tx_i = NULL;
	struct cdp_rx_stats *rx = NULL;

	if (!stats || !pdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	tx_i = &pdev_stats->tx_i;
	rx = &pdev_stats->rx;
	data = qdf_mem_malloc(sizeof(struct advance_pdev_data_raw));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	data->rx_raw.num = rx->raw.num;
	data->rx_raw.bytes = rx->raw.bytes;
	data->tx_raw_pkt.num = tx_i->raw.raw_pkt.num;
	data->tx_raw_pkt.bytes = tx_i->raw.raw_pkt.bytes;
	data->cce_classified_raw = tx_i->cce_classified_raw;
	data->rx_raw_pkts = pdev_stats->rx_raw_pkts;

	stats->raw = data;
	stats->raw_size = sizeof(struct advance_pdev_data_raw);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_pdev_data_tso(struct unified_stats *stats,
					    struct cdp_pdev_stats *pdev_stats)
{
	struct advance_pdev_data_tso *data = NULL;
	struct cdp_tso_stats *tso = NULL;
	struct cdp_tx_ingress_stats *tx_i = NULL;

	if (!stats || !pdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	tso = &pdev_stats->tso_stats;
	tx_i = &pdev_stats->tx_i;
	data = qdf_mem_malloc(sizeof(struct advance_pdev_data_tso));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	data->sg_pkt.num = tx_i->sg.sg_pkt.num;
	data->sg_pkt.bytes = tx_i->sg.sg_pkt.bytes;
	data->non_sg_pkts.num = tx_i->sg.non_sg_pkts.num;
	data->non_sg_pkts.bytes = tx_i->sg.non_sg_pkts.bytes;
	data->num_tso_pkts.num = tso->num_tso_pkts.num;
	data->num_tso_pkts.bytes = tso->num_tso_pkts.bytes;
	data->tso_comp = tso->tso_comp;
#if FEATURE_TSO_STATS
	data->segs_1 = tso->seg_histogram.segs_1;
	data->segs_2_5 = tso->seg_histogram.segs_2_5;
	data->segs_6_10 = tso->seg_histogram.segs_6_10;
	data->segs_11_15 = tso->seg_histogram.segs_11_15;
	data->segs_16_20 = tso->seg_histogram.segs_16_20;
	data->segs_20_plus = tso->seg_histogram.segs_20_plus;
#endif /* FEATURE_TSO_STATS */

	stats->tso = data;
	stats->tso_size = sizeof(struct advance_pdev_data_tso);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_pdev_data_igmp(struct unified_stats *stats,
					     struct cdp_pdev_stats *pdev_stats)
{
	struct advance_pdev_data_igmp *data = NULL;
	struct cdp_tx_ingress_stats *tx_i = NULL;

	if (!stats || !pdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	tx_i = &pdev_stats->tx_i;
	data = qdf_mem_malloc(sizeof(struct advance_pdev_data_igmp));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	data->igmp_rcvd = tx_i->igmp_mcast_en.igmp_rcvd;
	data->igmp_ucast_converted = tx_i->igmp_mcast_en.igmp_ucast_converted;

	stats->igmp = data;
	stats->igmp_size = sizeof(struct advance_pdev_data_igmp);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_pdev_data_mesh(struct unified_stats *stats,
					     struct cdp_pdev_stats *pdev_stats)
{
	struct advance_pdev_data_mesh *data = NULL;
	struct cdp_tx_ingress_stats *tx_i = NULL;

	if (!stats || !pdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	tx_i = &pdev_stats->tx_i;
	data = qdf_mem_malloc(sizeof(struct advance_pdev_data_mesh));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	data->exception_fw = tx_i->mesh.exception_fw;
	data->completion_fw = tx_i->mesh.completion_fw;

	stats->mesh = data;
	stats->mesh_size = sizeof(struct advance_pdev_data_mesh);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_pdev_data_nawds(struct unified_stats *stats,
					      struct cdp_pdev_stats *pdev_stats)
{
	struct advance_pdev_data_nawds *data = NULL;
	struct cdp_rx_stats *rx = NULL;
	struct cdp_tx_stats *tx = NULL;

	if (!stats || !pdev_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	rx = &pdev_stats->rx;
	tx = &pdev_stats->tx;
	data = qdf_mem_malloc(sizeof(struct advance_pdev_data_nawds));
	if (!data) {
		qdf_err("Failed Allocation");
		return QDF_STATUS_E_NOMEM;
	}
	data->tx_nawds_mcast.num = tx->nawds_mcast.num;
	data->tx_nawds_mcast.bytes = tx->nawds_mcast.bytes;
	data->nawds_mcast_tx_drop = tx->nawds_mcast_drop;
	data->nawds_mcast_rx_drop = rx->nawds_mcast_drop;

	stats->nawds = data;
	stats->nawds_size = sizeof(struct advance_pdev_data_nawds);

	return QDF_STATUS_SUCCESS;
}

static void aggregate_vdev_stats(struct wlan_objmgr_pdev *pdev,
				 void *object, void *arg)
{
	struct wlan_objmgr_vdev *vdev = object;
	struct advance_pdev_ctrl_tx *ctrl = arg;
	struct vdev_ic_cp_stats *cp_stats;

	if (!vdev || !ctrl)
		return;

	if (wlan_vdev_mlme_get_opmode(vdev) != QDF_SAP_MODE)
		return;

	cp_stats = qdf_mem_malloc(sizeof(struct vdev_ic_cp_stats));
	if (!cp_stats) {
		qdf_debug("Allocation Failed!");
		return;
	}
	get_vdev_cp_stats(vdev, cp_stats);
	ctrl->cs_tx_beacon += cp_stats->stats.cs_tx_bcn_success +
			      cp_stats->stats.cs_tx_bcn_outage;
	qdf_mem_free(cp_stats);
}

static QDF_STATUS get_advance_pdev_ctrl_tx(struct wlan_objmgr_pdev *pdev,
					   struct unified_stats *stats,
					   struct pdev_ic_cp_stats *cp_stats,
					   bool aggregate)
{
	struct advance_pdev_ctrl_tx *ctrl = NULL;

	if (!stats || !cp_stats) {
		qdf_err("Invalid Input");
		return QDF_STATUS_E_INVAL;
	}
	ctrl = qdf_mem_malloc(sizeof(struct advance_pdev_ctrl_tx));
	if (!ctrl) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_pdev_ctrl_tx(&ctrl->b_tx, cp_stats);

	ctrl->cs_tx_beacon = cp_stats->stats.cs_tx_beacon;
	ctrl->cs_tx_retries = cp_stats->stats.cs_tx_retries;

	if (aggregate)
		wlan_objmgr_pdev_iterate_obj_list(pdev, WLAN_VDEV_OP,
						  aggregate_vdev_stats, ctrl,
						  1, WLAN_MLME_SB_ID);

	stats->tx = ctrl;
	stats->tx_size = sizeof(struct advance_pdev_ctrl_tx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_pdev_ctrl_rx(struct unified_stats *stats,
					   struct pdev_ic_cp_stats *cp_stats)
{
	struct advance_pdev_ctrl_rx *ctrl = NULL;

	if (!stats || !cp_stats) {
		qdf_err("Invalid Input");
		return QDF_STATUS_E_INVAL;
	}
	ctrl = qdf_mem_malloc(sizeof(struct advance_pdev_ctrl_rx));
	if (!ctrl) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_pdev_ctrl_rx(&ctrl->b_rx, cp_stats);

	ctrl->cs_rx_mgmt_rssi_drop = cp_stats->stats.cs_rx_mgmt_rssi_drop;

	stats->rx = ctrl;
	stats->rx_size = sizeof(struct advance_pdev_ctrl_rx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_pdev_ctrl_link(struct unified_stats *stats,
					     struct pdev_ic_cp_stats *cp_stats)
{
	struct advance_pdev_ctrl_link *ctrl = NULL;

	if (!stats || !cp_stats) {
		qdf_err("Invalid Input");
		return QDF_STATUS_E_INVAL;
	}
	ctrl = qdf_mem_malloc(sizeof(struct advance_pdev_ctrl_link));
	if (!ctrl) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_pdev_ctrl_link(&ctrl->b_link, cp_stats);

	ctrl->dcs_ap_tx_util = cp_stats->stats.chan_stats.dcs_ap_tx_util;
	ctrl->dcs_ap_rx_util = cp_stats->stats.chan_stats.dcs_ap_rx_util;
	ctrl->dcs_self_bss_util = cp_stats->stats.chan_stats.dcs_self_bss_util;
	ctrl->dcs_obss_util = cp_stats->stats.chan_stats.dcs_obss_util;
	ctrl->dcs_obss_rx_util = cp_stats->stats.chan_stats.dcs_obss_rx_util;
	ctrl->dcs_free_medium = cp_stats->stats.chan_stats.dcs_free_medium;
	ctrl->dcs_non_wifi_util = cp_stats->stats.chan_stats.dcs_non_wifi_util;
	ctrl->dcs_ss_under_util = cp_stats->stats.chan_stats.dcs_ss_under_util;
	ctrl->dcs_sec_20_util = cp_stats->stats.chan_stats.dcs_sec_20_util;
	ctrl->dcs_sec_40_util = cp_stats->stats.chan_stats.dcs_sec_40_util;
	ctrl->dcs_sec_80_util = cp_stats->stats.chan_stats.dcs_sec_80_util;
	ctrl->cs_tx_rssi = cp_stats->stats.cs_tx_rssi;
	ctrl->rx_rssi_chain0_pri20 =
		cp_stats->stats.cs_rx_rssi_chain0.rx_rssi_pri20;
	ctrl->rx_rssi_chain0_sec20 =
		cp_stats->stats.cs_rx_rssi_chain0.rx_rssi_sec20;
	ctrl->rx_rssi_chain0_sec40 =
		cp_stats->stats.cs_rx_rssi_chain0.rx_rssi_sec40;
	ctrl->rx_rssi_chain0_sec80 =
		cp_stats->stats.cs_rx_rssi_chain0.rx_rssi_sec80;
	ctrl->rx_rssi_chain1_pri20 =
		cp_stats->stats.cs_rx_rssi_chain1.rx_rssi_pri20;
	ctrl->rx_rssi_chain1_sec20 =
		cp_stats->stats.cs_rx_rssi_chain1.rx_rssi_sec20;
	ctrl->rx_rssi_chain1_sec40 =
		cp_stats->stats.cs_rx_rssi_chain1.rx_rssi_sec40;
	ctrl->rx_rssi_chain1_sec80 =
		cp_stats->stats.cs_rx_rssi_chain1.rx_rssi_sec80;
	ctrl->rx_rssi_chain2_pri20 =
		cp_stats->stats.cs_rx_rssi_chain2.rx_rssi_pri20;
	ctrl->rx_rssi_chain2_sec20 =
		cp_stats->stats.cs_rx_rssi_chain2.rx_rssi_sec20;
	ctrl->rx_rssi_chain2_sec40 =
		cp_stats->stats.cs_rx_rssi_chain2.rx_rssi_sec40;
	ctrl->rx_rssi_chain2_sec80 =
		cp_stats->stats.cs_rx_rssi_chain2.rx_rssi_sec80;
	ctrl->rx_rssi_chain3_pri20 =
		cp_stats->stats.cs_rx_rssi_chain3.rx_rssi_pri20;
	ctrl->rx_rssi_chain3_sec20 =
		cp_stats->stats.cs_rx_rssi_chain3.rx_rssi_sec20;
	ctrl->rx_rssi_chain3_sec40 =
		cp_stats->stats.cs_rx_rssi_chain3.rx_rssi_sec40;
	ctrl->rx_rssi_chain3_sec80 =
		cp_stats->stats.cs_rx_rssi_chain3.rx_rssi_sec80;

	stats->link = ctrl;
	stats->link_size = sizeof(struct advance_pdev_ctrl_link);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_pdev_data(struct wlan_objmgr_psoc *psoc,
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
		goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_TX) {
		ret = get_advance_pdev_data_tx(stats, pdev_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RX) {
		ret = get_advance_pdev_data_rx(stats, pdev_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_ME) {
		ret = get_advance_pdev_data_me(stats, pdev_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RAW) {
		ret = get_advance_pdev_data_raw(stats, pdev_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_TSO) {
		ret = get_advance_pdev_data_tso(stats, pdev_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_IGMP) {
		ret = get_advance_pdev_data_igmp(stats, pdev_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_MESH) {
		ret = get_advance_pdev_data_mesh(stats, pdev_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_NAWDS) {
		ret = get_advance_pdev_data_nawds(stats, pdev_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}

get_failed:
	qdf_mem_free(pdev_stats);

	return ret;
}

static QDF_STATUS get_advance_pdev_ctrl(struct wlan_objmgr_pdev *pdev,
					struct unified_stats *stats,
					uint32_t feat, bool aggregate)
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
		goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_TX) {
		ret = get_advance_pdev_ctrl_tx(pdev, stats, pdev_cp_stats,
					       aggregate);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RX) {
		ret = get_advance_pdev_ctrl_rx(stats, pdev_cp_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_LINK) {
		ret = get_advance_pdev_ctrl_link(stats, pdev_cp_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}

get_failed:
	qdf_mem_free(pdev_cp_stats);

	return ret;
}

static QDF_STATUS get_advance_psoc_data_tx(struct unified_stats *stats,
					   struct cdp_soc_stats *soc_stats)
{
	struct advance_psoc_data_tx *data = NULL;

	if (!stats || !soc_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	data = qdf_mem_malloc(sizeof(struct advance_psoc_data_tx));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_psoc_data_tx(&data->b_tx, soc_stats);

	stats->tx = data;
	stats->tx_size = sizeof(struct advance_psoc_data_tx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_psoc_data_rx(struct unified_stats *stats,
					   struct cdp_soc_stats *soc_stats)
{
	struct advance_psoc_data_rx *data = NULL;

	if (!stats || !soc_stats) {
		qdf_err("Invalid Input!");
		return QDF_STATUS_E_INVAL;
	}
	data = qdf_mem_malloc(sizeof(struct advance_psoc_data_rx));
	if (!data) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	fill_basic_psoc_data_rx(&data->b_rx, soc_stats);

	data->err_ring_pkts = soc_stats->rx.err_ring_pkts;
	data->rx_frags = soc_stats->rx.rx_frags;
	data->reo_reinject = soc_stats->rx.reo_reinject;
	data->bar_frame = soc_stats->rx.bar_frame;
	data->rejected = soc_stats->rx.err.rx_rejected;
	data->raw_frm_drop = soc_stats->rx.err.rx_raw_frm_drop;

	stats->rx = data;
	stats->rx_size = sizeof(struct advance_psoc_data_rx);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS get_advance_psoc_data(struct wlan_objmgr_psoc *psoc,
					struct unified_stats *stats,
					uint32_t feat)
{
	struct cdp_soc_stats *psoc_stats = NULL;
	QDF_STATUS ret = QDF_STATUS_SUCCESS;

	if (!psoc) {
		qdf_err("Invalid psoc!");
		return QDF_STATUS_E_INVAL;
	}
	psoc_stats = qdf_mem_malloc(sizeof(struct cdp_soc_stats));
	if (!psoc_stats) {
		qdf_err("Allocation Failed!");
		return QDF_STATUS_E_NOMEM;
	}
	ret = cdp_host_get_soc_stats(wlan_psoc_get_dp_handle(psoc), psoc_stats);
	if (QDF_STATUS_SUCCESS != ret) {
		qdf_err("Unable to get Psoc stats!");
		goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_TX) {
		ret = get_advance_psoc_data_tx(stats, psoc_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}
	if (feat & STATS_FEAT_FLG_RX) {
		ret = get_advance_psoc_data_rx(stats, psoc_stats);
		if (ret != QDF_STATUS_SUCCESS)
			goto get_failed;
	}

get_failed:
	qdf_mem_free(psoc_stats);

	return ret;
}
#endif /* WLAN_ADVANCE_TELEMETRY */

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
#if WLAN_ADVANCE_TELEMETRY
	case STATS_LVL_ADVANCE:
		if (cfg->type == STATS_TYPE_DATA)
			ret = get_advance_peer_data(psoc, peer,
						    stats, cfg->feat);
		else
			ret = get_advance_peer_ctrl(peer, stats, cfg->feat);
		break;
#endif /* WLAN_ADVANCE_TELEMETRY */
#if WLAN_DEBUG_TELEMETRY
	case STATS_LVL_DEBUG:
		break;
#endif /* WLAN_DEBUG_TELEMETRY */
	default:
		qdf_err("Unexpected Level %d!\n", cfg->lvl);
		ret = QDF_STATUS_E_INVAL;
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
#if WLAN_ADVANCE_TELEMETRY
	case STATS_LVL_ADVANCE:
		if (cfg->type == STATS_TYPE_DATA)
			ret = get_advance_vdev_data(psoc, vdev,
						    stats, cfg->feat);
		else
			ret = get_advance_vdev_ctrl(vdev, stats, cfg->feat);
		break;
#endif /* WLAN_ADVANCE_TELEMETRY */
#if WLAN_DEBUG_TELEMETRY
	case STATS_LVL_DEBUG:
		break;
#endif /* WLAN_DEBUG_TELEMETRY */
	default:
		qdf_err("Unexpected Level %d!\n", cfg->lvl);
		ret = QDF_STATUS_E_INVAL;
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
#if WLAN_ADVANCE_TELEMETRY
	case STATS_LVL_ADVANCE:
		if (cfg->type == STATS_TYPE_DATA)
			ret = get_advance_pdev_data(psoc, pdev,
						    stats, cfg->feat);
		else
			ret = get_advance_pdev_ctrl(pdev, stats, cfg->feat,
						    !cfg->recursive);
		break;
#endif /* WLAN_ADVANCE_TELEMETRY */
#if WLAN_DEBUG_TELEMETRY
	case STATS_LVL_DEBUG:
		break;
#endif /* WLAN_DEBUG_TELEMETRY */
	default:
		qdf_err("Unexpected Level %d!\n", cfg->lvl);
		ret = QDF_STATUS_E_INVAL;
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
		else if (!cfg->recursive)
			ret = QDF_STATUS_E_INVAL;
		break;
#if WLAN_ADVANCE_TELEMETRY
	case STATS_LVL_ADVANCE:
		if (cfg->type == STATS_TYPE_DATA)
			ret = get_advance_psoc_data(psoc, stats, cfg->feat);
		else if (!cfg->recursive)
			ret = QDF_STATUS_E_INVAL;
		break;
#endif /* WLAN_ADVANCE_TELEMETRY */
#if WLAN_DEBUG_TELEMETRY
	case STATS_LVL_DEBUG:
		break;
#endif /* WLAN_DEBUG_TELEMETRY */
	default:
		qdf_err("Unexpected Level %d!\n", cfg->lvl);
		ret = QDF_STATUS_E_INVAL;
	}

	return ret;
}

bool wlan_stats_is_recursive_valid(struct stats_config *cfg,
				   enum stats_object_e obj)
{
	bool recursive = false;

	if (!cfg->recursive)
		return false;
	if (cfg->feat == STATS_FEAT_FLG_ALL)
		return true;

	switch (obj) {
	case STATS_OBJ_AP:
		if (cfg->type == STATS_TYPE_DATA) {
			if ((cfg->lvl == STATS_LVL_BASIC) &&
			    ((cfg->feat & STATS_BASIC_RADIO_DATA_MASK) ||
			    (cfg->feat & STATS_BASIC_VAP_DATA_MASK) ||
			    (cfg->feat & STATS_BASIC_STA_DATA_MASK)))
				recursive = true;
#if WLAN_ADVANCE_TELEMETRY
			else if ((cfg->lvl == STATS_LVL_ADVANCE) &&
				 ((cfg->feat & STATS_ADVANCE_RADIO_DATA_MASK) ||
				 (cfg->feat & STATS_ADVANCE_VAP_DATA_MASK) ||
				 (cfg->feat & STATS_ADVANCE_STA_DATA_MASK)))
				recursive = true;
#endif /* WLAN_ADVANCE_TELEMETRY */
		} else if (cfg->type == STATS_TYPE_CTRL) {
			if ((cfg->lvl == STATS_LVL_BASIC) &&
			    ((cfg->feat & STATS_BASIC_RADIO_CTRL_MASK) ||
			    (cfg->feat & STATS_BASIC_VAP_CTRL_MASK) ||
			    (cfg->feat & STATS_BASIC_STA_CTRL_MASK)))
				recursive = true;
#if WLAN_ADVANCE_TELEMETRY
			else if ((cfg->lvl == STATS_LVL_ADVANCE) &&
				 ((cfg->feat & STATS_ADVANCE_RADIO_CTRL_MASK) ||
				 (cfg->feat & STATS_ADVANCE_VAP_CTRL_MASK) ||
				 (cfg->feat & STATS_ADVANCE_STA_CTRL_MASK)))
				recursive = true;
#endif /* WLAN_ADVANCE_TELEMETRY */
		}
		break;
	case STATS_OBJ_RADIO:
		if (cfg->type == STATS_TYPE_DATA) {
			if ((cfg->lvl == STATS_LVL_BASIC) &&
			    ((cfg->feat & STATS_BASIC_VAP_DATA_MASK) ||
			    (cfg->feat & STATS_BASIC_STA_DATA_MASK)))
				recursive = true;
#if WLAN_ADVANCE_TELEMETRY
			else if ((cfg->lvl == STATS_LVL_ADVANCE) &&
				 ((cfg->feat & STATS_ADVANCE_VAP_DATA_MASK) ||
				 (cfg->feat & STATS_ADVANCE_STA_DATA_MASK)))
				recursive = true;
#endif /* WLAN_ADVANCE_TELEMETRY */
		} else if (cfg->type == STATS_TYPE_CTRL) {
			if ((cfg->lvl == STATS_LVL_BASIC) &&
			    ((cfg->feat & STATS_BASIC_VAP_CTRL_MASK) ||
			    (cfg->feat & STATS_BASIC_STA_CTRL_MASK)))
				recursive = true;
#if WLAN_ADVANCE_TELEMETRY
			else if ((cfg->lvl == STATS_LVL_ADVANCE) &&
				 ((cfg->feat & STATS_ADVANCE_VAP_CTRL_MASK) ||
				 (cfg->feat & STATS_ADVANCE_STA_CTRL_MASK)))
				recursive = true;
#endif /* WLAN_ADVANCE_TELEMETRY */
		}
		break;
	case STATS_OBJ_VAP:
		if (cfg->type == STATS_TYPE_DATA) {
			if ((cfg->lvl == STATS_LVL_BASIC) &&
			    (cfg->feat & STATS_BASIC_STA_DATA_MASK))
				recursive = true;
#if WLAN_ADVANCE_TELEMETRY
			else if ((cfg->lvl == STATS_LVL_ADVANCE) &&
				 (cfg->feat & STATS_ADVANCE_STA_DATA_MASK))
				recursive = true;
#endif /* WLAN_ADVANCE_TELEMETRY */
		} else if (cfg->type == STATS_TYPE_CTRL) {
			if ((cfg->lvl == STATS_LVL_BASIC) &&
			    (cfg->feat & STATS_BASIC_STA_CTRL_MASK))
				recursive = true;
#if WLAN_ADVANCE_TELEMETRY
			else if ((cfg->lvl == STATS_LVL_ADVANCE) &&
				 (cfg->feat & STATS_ADVANCE_STA_CTRL_MASK))
				recursive = true;
#endif /* WLAN_ADVANCE_TELEMETRY */
		}
		break;
	case STATS_OBJ_STA:
	default:
		recursive = false;
	}

	return recursive;
}

void wlan_stats_free_unified_stats(struct unified_stats *stats)
{
	if (stats->rx)
		qdf_mem_free(stats->rx);
	if (stats->tx)
		qdf_mem_free(stats->tx);
	if (stats->me)
		qdf_mem_free(stats->me);
	if (stats->fwd)
		qdf_mem_free(stats->fwd);
	if (stats->tso)
		qdf_mem_free(stats->tso);
	if (stats->twt)
		qdf_mem_free(stats->twt);
	if (stats->raw)
		qdf_mem_free(stats->raw);
	if (stats->igmp)
		qdf_mem_free(stats->igmp);
	if (stats->link)
		qdf_mem_free(stats->link);
	if (stats->mesh)
		qdf_mem_free(stats->mesh);
	if (stats->rate)
		qdf_mem_free(stats->rate);
	if (stats->nawds)
		qdf_mem_free(stats->nawds);
	stats->tx_size = 0;
	stats->rx_size = 0;
	stats->me_size = 0;
	stats->fwd_size = 0;
	stats->tso_size = 0;
	stats->twt_size = 0;
	stats->raw_size = 0;
	stats->mesh_size = 0;
	stats->igmp_size = 0;
	stats->link_size = 0;
	stats->rate_size = 0;
	stats->nawds_size = 0;
}
