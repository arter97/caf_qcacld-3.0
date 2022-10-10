/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "dp_types.h"
#include <dp_internal.h>
#include <dp_htt.h>
#include "dp_rh.h"
#include "dp_rh_tx.h"
#include "dp_rh_htt.h"
#include "dp_tx_desc.h"
#include "dp_rh_rx.h"
#include "dp_peer.h"
#include <wlan_utility.h>

static void dp_soc_cfg_attach_rh(struct dp_soc *soc)
{
}

qdf_size_t dp_get_context_size_rh(enum dp_context_type context_type)
{
	switch (context_type) {
	case DP_CONTEXT_TYPE_SOC:
		return sizeof(struct dp_soc_rh);
	case DP_CONTEXT_TYPE_PDEV:
		return sizeof(struct dp_pdev_rh);
	case DP_CONTEXT_TYPE_VDEV:
		return sizeof(struct dp_vdev_rh);
	case DP_CONTEXT_TYPE_PEER:
		return sizeof(struct dp_peer_rh);
	default:
		return 0;
	}
}

qdf_size_t dp_mon_get_context_size_rh(enum dp_context_type context_type)
{
	switch (context_type) {
	case DP_CONTEXT_TYPE_MON_PDEV:
		return sizeof(struct dp_mon_pdev_rh);
	case DP_CONTEXT_TYPE_MON_SOC:
		return sizeof(struct dp_mon_soc_rh);
	default:
		return 0;
	}
}

static QDF_STATUS dp_soc_attach_rh(struct dp_soc *soc,
				   struct cdp_soc_attach_params *params)
{
	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS dp_soc_detach_rh(struct dp_soc *soc)
{
	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS dp_soc_init_rh(struct dp_soc *soc)
{
	/*Register RX offload flush handlers*/
	hif_offld_flush_cb_register(soc->hif_handle, dp_rx_data_flush);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS dp_soc_deinit_rh(struct dp_soc *soc)
{
	/*Degister RX offload flush handlers*/
	hif_offld_flush_cb_deregister(soc->hif_handle);

	return QDF_STATUS_SUCCESS;
}

/**
 * dp_pdev_fill_tx_endpoint_info_rh() - Prefill fixed TX endpoint information
 *					that is used during packet transmit
 * @pdev: Handle to DP pdev struct
 *
 * Return: QDF_STATUS_SUCCESS/QDF_STATUS_E_NOENT
 */
static QDF_STATUS dp_pdev_fill_tx_endpoint_info_rh(struct dp_pdev *pdev)
{
	struct dp_pdev_rh *rh_pdev = dp_get_rh_pdev_from_dp_pdev(pdev);
	struct dp_soc_rh *rh_soc = dp_get_rh_soc_from_dp_soc(pdev->soc);
	struct dp_tx_ep_info_rh *tx_ep_info = &rh_pdev->tx_ep_info;
	struct hif_opaque_softc *hif_handle = pdev->soc->hif_handle;
	int ul_is_polled, dl_is_polled;
	uint8_t ul_pipe, dl_pipe;
	int status;

	status = hif_map_service_to_pipe(hif_handle, HTT_DATA2_MSG_SVC,
					 &ul_pipe, &dl_pipe,
					 &ul_is_polled, &dl_is_polled);
	if (status) {
		hif_err("Failed to map tx pipe: %d", status);
		return QDF_STATUS_E_NOENT;
	}

	tx_ep_info->ce_tx_hdl = hif_get_ce_handle(hif_handle, ul_pipe);

	tx_ep_info->download_len = HAL_TX_DESC_LEN_BYTES +
				   sizeof(struct tlv_32_hdr) +
				   DP_RH_TX_HDR_SIZE_OUTER_HDR_MAX +
				   DP_RH_TX_HDR_SIZE_802_1Q +
				   DP_RH_TX_HDR_SIZE_LLC_SNAP +
				   DP_RH_TX_HDR_SIZE_IP;

	tx_ep_info->tx_endpoint = rh_soc->tx_endpoint;

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS dp_pdev_attach_rh(struct dp_pdev *pdev,
				    struct cdp_pdev_attach_params *params)
{
	return dp_pdev_fill_tx_endpoint_info_rh(pdev);
}

static QDF_STATUS dp_pdev_detach_rh(struct dp_pdev *pdev)
{
	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS dp_vdev_attach_rh(struct dp_soc *soc, struct dp_vdev *vdev)
{
	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS dp_vdev_detach_rh(struct dp_soc *soc, struct dp_vdev *vdev)
{
	return QDF_STATUS_SUCCESS;
}

#ifdef AST_OFFLOAD_ENABLE
static void dp_peer_map_detach_rh(struct dp_soc *soc)
{
	dp_soc_wds_detach(soc);
	dp_peer_ast_table_detach(soc);
	dp_peer_ast_hash_detach(soc);
	dp_peer_mec_hash_detach(soc);
}

static QDF_STATUS dp_peer_map_attach_rh(struct dp_soc *soc)
{
	QDF_STATUS status;

	soc->max_peer_id = soc->max_peers;

	status = dp_peer_ast_table_attach(soc);
	if (!QDF_IS_STATUS_SUCCESS(status))
		return status;

	status = dp_peer_ast_hash_attach(soc);
	if (!QDF_IS_STATUS_SUCCESS(status))
		goto ast_table_detach;

	status = dp_peer_mec_hash_attach(soc);
	if (!QDF_IS_STATUS_SUCCESS(status))
		goto hash_detach;

	dp_soc_wds_attach(soc);

	return QDF_STATUS_SUCCESS;

hash_detach:
	dp_peer_ast_hash_detach(soc);
ast_table_detach:
	dp_peer_ast_table_detach(soc);

	return status;
}
#else
static void dp_peer_map_detach_rh(struct dp_soc *soc)
{
}

static QDF_STATUS dp_peer_map_attach_rh(struct dp_soc *soc)
{
	soc->max_peer_id = soc->max_peers;

	return QDF_STATUS_SUCCESS;
}
#endif

qdf_size_t dp_get_soc_context_size_rh(void)
{
	return sizeof(struct dp_soc_rh);
}

#ifdef NO_RX_PKT_HDR_TLV
/**
 * dp_rxdma_ring_sel_cfg_rh() - Setup RXDMA ring config
 * @soc: Common DP soc handle
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS
dp_rxdma_ring_sel_cfg_rh(struct dp_soc *soc)
{
	int i;
	int mac_id;
	struct htt_rx_ring_tlv_filter htt_tlv_filter = {0};
	struct dp_srng *rx_mac_srng;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	htt_tlv_filter.mpdu_start = 1;
	htt_tlv_filter.msdu_start = 1;
	htt_tlv_filter.mpdu_end = 1;
	htt_tlv_filter.msdu_end = 1;
	htt_tlv_filter.attention = 1;
	htt_tlv_filter.packet = 1;
	htt_tlv_filter.packet_header = 0;

	htt_tlv_filter.ppdu_start = 0;
	htt_tlv_filter.ppdu_end = 0;
	htt_tlv_filter.ppdu_end_user_stats = 0;
	htt_tlv_filter.ppdu_end_user_stats_ext = 0;
	htt_tlv_filter.ppdu_end_status_done = 0;
	htt_tlv_filter.enable_fp = 1;
	htt_tlv_filter.enable_md = 0;
	htt_tlv_filter.enable_md = 0;
	htt_tlv_filter.enable_mo = 0;

	htt_tlv_filter.fp_mgmt_filter = 0;
	htt_tlv_filter.fp_ctrl_filter = FILTER_CTRL_BA_REQ;
	htt_tlv_filter.fp_data_filter = (FILTER_DATA_UCAST |
					 FILTER_DATA_MCAST |
					 FILTER_DATA_DATA);
	htt_tlv_filter.mo_mgmt_filter = 0;
	htt_tlv_filter.mo_ctrl_filter = 0;
	htt_tlv_filter.mo_data_filter = 0;
	htt_tlv_filter.md_data_filter = 0;

	htt_tlv_filter.offset_valid = true;

	htt_tlv_filter.rx_packet_offset = soc->rx_pkt_tlv_size;
	/*Not subscribing rx_pkt_header*/
	htt_tlv_filter.rx_header_offset = 0;
	htt_tlv_filter.rx_mpdu_start_offset =
				hal_rx_mpdu_start_offset_get(soc->hal_soc);
	htt_tlv_filter.rx_mpdu_end_offset =
				hal_rx_mpdu_end_offset_get(soc->hal_soc);
	htt_tlv_filter.rx_msdu_start_offset =
				hal_rx_msdu_start_offset_get(soc->hal_soc);
	htt_tlv_filter.rx_msdu_end_offset =
				hal_rx_msdu_end_offset_get(soc->hal_soc);
	htt_tlv_filter.rx_attn_offset =
				hal_rx_attn_offset_get(soc->hal_soc);

	for (i = 0; i < MAX_PDEV_CNT; i++) {
		struct dp_pdev *pdev = soc->pdev_list[i];

		if (!pdev)
			continue;

		for (mac_id = 0; mac_id < NUM_RXDMA_RINGS_PER_PDEV; mac_id++) {
			int mac_for_pdev =
				dp_get_mac_id_for_pdev(mac_id, pdev->pdev_id);
			/*
			 * Obtain lmac id from pdev to access the LMAC ring
			 * in soc context
			 */
			int lmac_id =
				dp_get_lmac_id_for_pdev_id(soc, mac_id,
							   pdev->pdev_id);

			rx_mac_srng = dp_get_rxdma_ring(pdev, lmac_id);
			htt_h2t_rx_ring_cfg(soc->htt_handle, mac_for_pdev,
					    rx_mac_srng->hal_srng,
					    RXDMA_BUF, RX_DATA_BUFFER_SIZE,
					    &htt_tlv_filter);
		}
	}

	if (QDF_IS_STATUS_SUCCESS(status))
		status = dp_htt_h2t_rx_ring_rfs_cfg(soc->htt_handle);

	return status;
}
#else

static QDF_STATUS
dp_rxdma_ring_sel_cfg_rh(struct dp_soc *soc)
{
	int i;
	int mac_id;
	struct htt_rx_ring_tlv_filter htt_tlv_filter = {0};
	struct dp_srng *rx_mac_srng;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	htt_tlv_filter.mpdu_start = 1;
	htt_tlv_filter.msdu_start = 1;
	htt_tlv_filter.mpdu_end = 1;
	htt_tlv_filter.msdu_end = 1;
	htt_tlv_filter.attention = 1;
	htt_tlv_filter.packet = 1;
	htt_tlv_filter.packet_header = 1;

	htt_tlv_filter.ppdu_start = 0;
	htt_tlv_filter.ppdu_end = 0;
	htt_tlv_filter.ppdu_end_user_stats = 0;
	htt_tlv_filter.ppdu_end_user_stats_ext = 0;
	htt_tlv_filter.ppdu_end_status_done = 0;
	htt_tlv_filter.enable_fp = 1;
	htt_tlv_filter.enable_md = 0;
	htt_tlv_filter.enable_md = 0;
	htt_tlv_filter.enable_mo = 0;

	htt_tlv_filter.fp_mgmt_filter = 0;
	htt_tlv_filter.fp_ctrl_filter = FILTER_CTRL_BA_REQ;
	htt_tlv_filter.fp_data_filter = (FILTER_DATA_UCAST |
					 FILTER_DATA_MCAST |
					 FILTER_DATA_DATA);
	htt_tlv_filter.mo_mgmt_filter = 0;
	htt_tlv_filter.mo_ctrl_filter = 0;
	htt_tlv_filter.mo_data_filter = 0;
	htt_tlv_filter.md_data_filter = 0;

	htt_tlv_filter.offset_valid = true;

	htt_tlv_filter.rx_packet_offset = soc->rx_pkt_tlv_size;
	htt_tlv_filter.rx_header_offset =
				hal_rx_pkt_tlv_offset_get(soc->hal_soc);
	htt_tlv_filter.rx_mpdu_start_offset =
				hal_rx_mpdu_start_offset_get(soc->hal_soc);
	htt_tlv_filter.rx_mpdu_end_offset =
				hal_rx_mpdu_end_offset_get(soc->hal_soc);
	htt_tlv_filter.rx_msdu_start_offset =
				hal_rx_msdu_start_offset_get(soc->hal_soc);
	htt_tlv_filter.rx_msdu_end_offset =
				hal_rx_msdu_end_offset_get(soc->hal_soc);
	htt_tlv_filter.rx_attn_offset =
				hal_rx_attn_offset_get(soc->hal_soc);

	for (i = 0; i < MAX_PDEV_CNT; i++) {
		struct dp_pdev *pdev = soc->pdev_list[i];

		if (!pdev)
			continue;

		for (mac_id = 0; mac_id < NUM_RXDMA_RINGS_PER_PDEV; mac_id++) {
			int mac_for_pdev =
				dp_get_mac_id_for_pdev(mac_id, pdev->pdev_id);
			/*
			 * Obtain lmac id from pdev to access the LMAC ring
			 * in soc context
			 */
			int lmac_id =
				dp_get_lmac_id_for_pdev_id(soc, mac_id,
							   pdev->pdev_id);

			rx_mac_srng = dp_get_rxdma_ring(pdev, lmac_id);
			htt_h2t_rx_ring_cfg(soc->htt_handle, mac_for_pdev,
					    rx_mac_srng->hal_srng,
					    RXDMA_BUF, RX_DATA_BUFFER_SIZE,
					    &htt_tlv_filter);
		}
	}

	if (QDF_IS_STATUS_SUCCESS(status))
		status = dp_htt_h2t_rx_ring_rfs_cfg(soc->htt_handle);

	return status;
}
#endif

static void dp_soc_srng_deinit_rh(struct dp_soc *soc)
{
}

static void dp_soc_srng_free_rh(struct dp_soc *soc)
{
}

static QDF_STATUS dp_soc_srng_alloc_rh(struct dp_soc *soc)
{
	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS dp_soc_srng_init_rh(struct dp_soc *soc)
{
	return QDF_STATUS_SUCCESS;
}

static void dp_tx_implicit_rbm_set_rh(struct dp_soc *soc,
				      uint8_t tx_ring_id,
				      uint8_t bm_id)
{
}

static QDF_STATUS dp_txrx_set_vdev_param_rh(struct dp_soc *soc,
					    struct dp_vdev *vdev,
					    enum cdp_vdev_param_type param,
					    cdp_config_param_type val)
{
	return QDF_STATUS_SUCCESS;
}

static struct dp_peer *dp_find_peer_by_destmac_rh(struct dp_soc *soc,
						  uint8_t *dest_mac,
						  uint8_t vdev_id)
{
	struct dp_peer *peer = NULL;
	struct dp_ast_entry *ast_entry = NULL;
	uint16_t peer_id;

	qdf_spin_lock_bh(&soc->ast_lock);
	ast_entry = dp_peer_ast_hash_find_by_vdevid(soc, dest_mac, vdev_id);

	if (!ast_entry) {
		qdf_spin_unlock_bh(&soc->ast_lock);
		dp_err("NULL ast entry");
		return NULL;
	}

	peer_id = ast_entry->peer_id;
	qdf_spin_unlock_bh(&soc->ast_lock);

	if (peer_id == HTT_INVALID_PEER)
		return NULL;

	peer = dp_peer_get_ref_by_id(soc, peer_id,
				     DP_MOD_ID_SAWF);
	return peer;
}

static void dp_get_rx_hash_key_rh(struct dp_soc *soc,
				  struct cdp_lro_hash_config *lro_hash)
{
	dp_get_rx_hash_key_bytes(lro_hash);
}

void dp_initialize_arch_ops_rh(struct dp_arch_ops *arch_ops)
{
	arch_ops->tx_hw_enqueue = dp_tx_hw_enqueue_rh;
	arch_ops->tx_comp_get_params_from_hal_desc =
		dp_tx_comp_get_params_from_hal_desc_rh;
	arch_ops->dp_tx_process_htt_completion =
			dp_tx_process_htt_completion_rh;
	arch_ops->dp_wbm_get_rx_desc_from_hal_desc =
			dp_wbm_get_rx_desc_from_hal_desc_rh;
	arch_ops->dp_tx_desc_pool_alloc = dp_tx_desc_pool_alloc_rh;
	arch_ops->dp_tx_desc_pool_free = dp_tx_desc_pool_free_rh;
	arch_ops->dp_tx_desc_pool_init = dp_tx_desc_pool_init_rh;
	arch_ops->dp_tx_desc_pool_deinit = dp_tx_desc_pool_deinit_rh;
	arch_ops->dp_rx_desc_pool_init = dp_rx_desc_pool_init_rh;
	arch_ops->dp_rx_desc_pool_deinit = dp_rx_desc_pool_deinit_rh;
	arch_ops->dp_tx_compute_hw_delay = dp_tx_compute_tx_delay_rh;
	arch_ops->txrx_get_context_size = dp_get_context_size_rh;
	arch_ops->txrx_get_mon_context_size = dp_mon_get_context_size_rh;
	arch_ops->txrx_soc_attach = dp_soc_attach_rh;
	arch_ops->txrx_soc_detach = dp_soc_detach_rh;
	arch_ops->txrx_soc_init = dp_soc_init_rh;
	arch_ops->txrx_soc_deinit = dp_soc_deinit_rh;
	arch_ops->txrx_soc_srng_alloc = dp_soc_srng_alloc_rh;
	arch_ops->txrx_soc_srng_init = dp_soc_srng_init_rh;
	arch_ops->txrx_soc_srng_deinit = dp_soc_srng_deinit_rh;
	arch_ops->txrx_soc_srng_free = dp_soc_srng_free_rh;
	arch_ops->txrx_pdev_attach = dp_pdev_attach_rh;
	arch_ops->txrx_pdev_detach = dp_pdev_detach_rh;
	arch_ops->txrx_vdev_attach = dp_vdev_attach_rh;
	arch_ops->txrx_vdev_detach = dp_vdev_detach_rh;
	arch_ops->txrx_peer_map_attach = dp_peer_map_attach_rh;
	arch_ops->txrx_peer_map_detach = dp_peer_map_detach_rh;
	arch_ops->get_rx_hash_key = dp_get_rx_hash_key_rh;
	arch_ops->dp_rx_desc_cookie_2_va =
			dp_rx_desc_cookie_2_va_rh;
	arch_ops->dp_rx_intrabss_mcast_handler =
					dp_rx_intrabss_handle_nawds_rh;
	arch_ops->dp_rx_word_mask_subscribe = dp_rx_word_mask_subscribe_rh;
	arch_ops->dp_rxdma_ring_sel_cfg = dp_rxdma_ring_sel_cfg_rh;
	arch_ops->dp_rx_peer_metadata_peer_id_get =
					dp_rx_peer_metadata_peer_id_get_rh;
	arch_ops->soc_cfg_attach = dp_soc_cfg_attach_rh;
	arch_ops->tx_implicit_rbm_set = dp_tx_implicit_rbm_set_rh;
	arch_ops->txrx_set_vdev_param = dp_txrx_set_vdev_param_rh;
	arch_ops->txrx_print_peer_stats = dp_print_peer_txrx_stats_rh;
	arch_ops->dp_peer_rx_reorder_queue_setup =
					dp_peer_rx_reorder_queue_setup_rh;
	arch_ops->dp_find_peer_by_destmac = dp_find_peer_by_destmac_rh;
	arch_ops->peer_get_reo_hash = dp_peer_get_reo_hash_rh;
	arch_ops->reo_remap_config = dp_reo_remap_config_rh;
}
