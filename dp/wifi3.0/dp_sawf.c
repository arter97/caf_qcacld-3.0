/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <dp_types.h>
#include <dp_peer.h>
#include <dp_internal.h>
#include <dp_htt.h>
#include <dp_sawf_htt.h>
#include <dp_sawf.h>
#include <dp_hist.h>
#include <hal_tx.h>

QDF_STATUS
dp_peer_sawf_ctx_alloc(struct dp_soc *soc,
		       struct dp_peer *peer)
{
	struct dp_peer_sawf *sawf_ctx;

	sawf_ctx = qdf_mem_malloc(sizeof(struct dp_peer_sawf));
	if (!sawf_ctx) {
		qdf_err("Failed to allocate peer SAWF ctx");
		return QDF_STATUS_E_FAILURE;
	}

	peer->sawf = sawf_ctx;
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
dp_peer_sawf_ctx_free(struct dp_soc *soc,
		      struct dp_peer *peer)
{
	if (!peer->sawf) {
		qdf_err("Failed to free peer SAWF ctx");
		return QDF_STATUS_E_FAILURE;
	}

	if (peer->sawf)
		qdf_mem_free(peer->sawf);

	return QDF_STATUS_SUCCESS;
}

struct dp_peer_sawf *dp_peer_sawf_ctx_get(struct dp_peer *peer)
{
	struct dp_peer_sawf *sawf_ctx;

	sawf_ctx = peer->sawf;
	if (!sawf_ctx) {
		qdf_err("Failed to allocate peer SAWF ctx");
		return NULL;
	}

	return sawf_ctx;
}

QDF_STATUS
dp_sawf_def_queues_get_map_report(struct cdp_soc_t *soc_hdl,
				  uint8_t *mac_addr)
{
	struct dp_soc *dp_soc;
	struct dp_peer *peer;
	uint8_t tid;
	struct dp_peer_sawf *sawf_ctx;

	dp_soc = cdp_soc_t_to_dp_soc(soc_hdl);

	if (!dp_soc) {
		qdf_err("Invalid soc");
		return QDF_STATUS_E_INVAL;
	}

	peer = dp_peer_find_hash_find(dp_soc, mac_addr, 0,
				      DP_VDEV_ALL, DP_MOD_ID_CDP);
	if (!peer) {
		qdf_err("Invalid peer");
		return QDF_STATUS_E_FAILURE;
	}

	sawf_ctx = dp_peer_sawf_ctx_get(peer);
	if (!sawf_ctx) {
		qdf_err("Invalid SAWF ctx");
		dp_peer_unref_delete(peer, DP_MOD_ID_CDP);
		return QDF_STATUS_E_FAILURE;
	}

	qdf_info("Peer ", QDF_MAC_ADDR_FMT, QDF_MAC_ADDR_REF(mac_addr));
	qdf_nofl_info("TID\tService Class ID");
	for (tid = 0; tid < DP_SAWF_MAX_TIDS; ++tid) {
		if (sawf_ctx->tid_reports[tid].svc_class_id ==
				HTT_SAWF_SVC_CLASS_INVALID_ID) {
			continue;
		}
		qdf_nofl_info("%u\t%u", tid,
			      sawf_ctx->tid_reports[tid].svc_class_id);
	}

	dp_peer_unref_delete(peer, DP_MOD_ID_CDP);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
dp_sawf_def_queues_map_req(struct cdp_soc_t *soc_hdl,
			   uint8_t *mac_addr, uint8_t svc_class_id)
{
	struct dp_soc *dp_soc;
	struct dp_peer *peer;
	uint16_t peer_id;
	QDF_STATUS status;

	dp_soc = cdp_soc_t_to_dp_soc(soc_hdl);

	if (!dp_soc) {
		qdf_err("Invalid soc");
		return QDF_STATUS_E_INVAL;
	}

	peer = dp_peer_find_hash_find(dp_soc, mac_addr, 0,
				      DP_VDEV_ALL, DP_MOD_ID_CDP);
	if (!peer) {
		qdf_err("Invalid peer");
		return QDF_STATUS_E_FAILURE;
	}

	peer_id = peer->peer_id;

	dp_peer_unref_delete(peer, DP_MOD_ID_CDP);

	qdf_info("peer " QDF_MAC_ADDR_FMT "svc id %u peer id %u",
		 QDF_MAC_ADDR_REF(mac_addr), svc_class_id, peer_id);

	status = dp_htt_h2t_sawf_def_queues_map_req(dp_soc->htt_handle,
						    svc_class_id, peer_id);

	if (status != QDF_STATUS_SUCCESS)
		return status;

	/*
	 * Request map repot conf from FW for all TIDs
	 */
	return dp_htt_h2t_sawf_def_queues_map_report_req(dp_soc->htt_handle,
							 peer_id, 0xff);
}

QDF_STATUS
dp_sawf_def_queues_unmap_req(struct cdp_soc_t *soc_hdl,
			     uint8_t *mac_addr, uint8_t svc_id)
{
	struct dp_soc *dp_soc;
	struct dp_peer *peer;
	uint16_t peer_id;
	QDF_STATUS status;
	uint8_t wildcard_mac[QDF_MAC_ADDR_SIZE] = {0xff, 0xff, 0xff,
		0xff, 0xff, 0xff};

	dp_soc = cdp_soc_t_to_dp_soc(soc_hdl);

	if (!dp_soc) {
		qdf_err("Invalid soc");
		return QDF_STATUS_E_INVAL;
	}

	if (!qdf_mem_cmp(mac_addr, wildcard_mac, QDF_MAC_ADDR_SIZE)) {
		/* wildcard unmap */
		peer_id = HTT_H2T_SAWF_DEF_QUEUES_UNMAP_PEER_ID_WILDCARD;
	} else {
		peer = dp_peer_find_hash_find(dp_soc, mac_addr, 0,
					      DP_VDEV_ALL, DP_MOD_ID_CDP);
		if (!peer) {
			qdf_err("Invalid peer");
			return QDF_STATUS_E_FAILURE;
		}
		peer_id = peer->peer_id;
		dp_peer_unref_delete(peer, DP_MOD_ID_CDP);
	}

	qdf_info("peer " QDF_MAC_ADDR_FMT "svc id %u peer id %u",
		 QDF_MAC_ADDR_REF(mac_addr), svc_id, peer_id);

	status =  dp_htt_h2t_sawf_def_queues_unmap_req(dp_soc->htt_handle,
						       svc_id, peer_id);

	if (status != QDF_STATUS_SUCCESS)
		return status;

	/*
	 * Request map repot conf from FW for all TIDs
	 */
	return dp_htt_h2t_sawf_def_queues_map_report_req(dp_soc->htt_handle,
							 peer_id, 0xff);
}

QDF_STATUS
dp_peer_sawf_stats_ctx_alloc(struct dp_soc *soc,
			     struct dp_txrx_peer *txrx_peer)
{
	struct dp_peer_sawf_stats *ctx;
	struct sawf_stats *stats;
	uint8_t tid, q_idx;

	ctx = qdf_mem_malloc(sizeof(struct dp_peer_sawf_stats));
	if (!ctx) {
		qdf_err("Failed to allocate peer SAWF stats");
		return QDF_STATUS_E_FAILURE;
	}

	txrx_peer->sawf_stats = ctx;
	stats = &ctx->stats;

	/* Initialize delay stats hist */
	for (tid = 0; tid < DP_SAWF_MAX_TIDS; tid++) {
		for (q_idx = 0; q_idx < DP_SAWF_MAX_QUEUES; q_idx++) {
			dp_hist_init(&stats->delay[tid][q_idx].delay_hist,
				     CDP_HIST_TYPE_HW_COMP_DELAY);
		}
	}

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
dp_peer_sawf_stats_ctx_free(struct dp_soc *soc,
			    struct dp_txrx_peer *txrx_peer)
{
	if (!txrx_peer->sawf_stats) {
		qdf_err("Failed to free peer SAWF stats");
		return QDF_STATUS_E_FAILURE;
	}

	qdf_mem_free(txrx_peer->sawf_stats);
	txrx_peer->sawf_stats = NULL;

	return QDF_STATUS_SUCCESS;
}

struct dp_peer_sawf_stats *
dp_peer_sawf_stats_ctx_get(struct dp_txrx_peer *txrx_peer)
{
	struct dp_peer_sawf_stats *sawf_stats;

	sawf_stats = txrx_peer->sawf_stats;
	if (!sawf_stats) {
		qdf_err("Failed to get SAWF stats ctx");
		return NULL;
	}

	return sawf_stats;
}

static int
dp_sawf_msduq_delay_window_switch(struct sawf_delay_stats *tx_delay)
{
	uint8_t cur_win;

	cur_win = tx_delay->cur_win;

	if (tx_delay->win_avgs[cur_win].count >
			MSDU_QUEUE_LATENCY_WIN_MIN_SAMPLES) {
		if (++cur_win == DP_SAWF_NUM_AVG_WINDOWS)
			cur_win = 0;
		tx_delay->cur_win = cur_win;
		tx_delay->win_avgs[cur_win].sum = 0;
		tx_delay->win_avgs[cur_win].count = 0;
	}

	return cur_win;
}

static QDF_STATUS
dp_sawf_compute_tx_delay_us(struct dp_soc *soc,
			    struct dp_vdev *vdev,
			    struct hal_tx_completion_status *ts,
			    uint32_t *delay_us)
{
	int32_t buffer_ts;
	int32_t delta_tsf;
	int32_t delay;

	/* Tx_rate_stats_info_valid is 0 and tsf is invalid then */
	if (!ts->valid) {
		qdf_info("Invalid Tx rate stats info");
		return QDF_STATUS_E_FAILURE;
	}

	if (!vdev) {
		qdf_err("vdev is null");
		return QDF_STATUS_E_INVAL;
	}

	delta_tsf = vdev->delta_tsf;

	/* buffer_timestamp is in units of 1024 us and is [31:13] of
	 * WBM_RELEASE_RING_4. After left shift 10 bits, it's
	 * valid up to 29 bits.
	 */
	buffer_ts = ts->buffer_timestamp << WLAN_TX_DELAY_UNITS_US;

	delay = ts->tsf - buffer_ts - delta_tsf;
	/* mask 29 BITS */
	delay &= WLAN_TX_DELAY_MASK;

	if (delay > 0x1000000) {
		qdf_info("----------------------\n"
			 "Tx completion status:\n"
			 "----------------------\n"
			 "release_src = %d\n"
			 "ppdu_id = 0x%x\n"
			 "release_reason = %d\n"
			 "tsf = %u (0x%x)\n"
			 "buffer_timestamp = %u (0x%x)\n"
			 "delta_tsf = %d (0x%x)\n",
			 ts->release_src, ts->ppdu_id, ts->status,
			 ts->tsf, ts->tsf, ts->buffer_timestamp,
			 ts->buffer_timestamp, delta_tsf, delta_tsf);
		return QDF_STATUS_E_FAILURE;
	}

	if (delay_us)
		*delay_us = delay;

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
dp_sawf_update_tx_delay(struct dp_soc *soc,
			struct dp_vdev *vdev,
			struct hal_tx_completion_status *ts,
			struct sawf_stats *stats,
			uint8_t tid,
			uint8_t msduq_idx)
{
	struct sawf_delay_stats *tx_delay;
	uint32_t hw_delay;
	uint8_t cur_win;
	QDF_STATUS status;

	status = dp_sawf_compute_tx_delay_us(soc, vdev, ts, &hw_delay);
	if (status != QDF_STATUS_SUCCESS) {
		qdf_err("Fail to update tx delay");
		return status;
	}

	tx_delay = &stats->delay[tid][msduq_idx];

	/* Update hist */
	dp_hist_update_stats(&tx_delay->delay_hist, hw_delay);

	/* Update total average */
	tx_delay->avg.sum += hw_delay;
	tx_delay->avg.count++;

	/* Update window average. Switch window if needed */
	cur_win = dp_sawf_msduq_delay_window_switch(tx_delay);
	tx_delay->win_avgs[cur_win].sum += hw_delay;
	tx_delay->win_avgs[cur_win].count++;

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
dp_sawf_get_msduq_map_info(struct dp_soc *soc, uint8_t peer_id,
			   uint8_t host_q_idx,
			   uint8_t *remaped_tid, uint8_t *target_q_idx)
{
	struct dp_peer *peer;
	uint8_t tid, q_idx;

	peer = dp_peer_get_ref_by_id(soc, peer_id, DP_MOD_ID_CDP);
	if (!peer) {
		qdf_err("Invalid peer (peer id %u)", peer_id);
		return QDF_STATUS_E_FAILURE;
	}

	/*
	 * Find tid and msdu queue idx from host msdu queue number
	 * host idx to be taken from the tx descriptor
	 */
	tid = 0;
	q_idx = 0;

	if (remaped_tid)
		*remaped_tid = tid;

	if (target_q_idx)
		*target_q_idx = q_idx;

	dp_peer_unref_delete(peer, DP_MOD_ID_CDP);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
dp_sawf_tx_compl_update_peer_stats(struct dp_soc *soc,
				   struct dp_vdev *vdev,
				   struct dp_txrx_peer *txrx_peer,
				   struct dp_tx_desc_s *tx_desc,
				   struct hal_tx_completion_status *ts,
				   uint8_t host_tid)
{
	struct dp_peer_sawf_stats *sawf_ctx;
	struct sawf_tx_stats *tx_stats;
	uint8_t tid, msduq_idx, host_msduq_idx;
	uint16_t peer_id;
	qdf_size_t length;
	QDF_STATUS status;

	if (!ts || !ts->valid)
		return QDF_STATUS_E_INVAL;

	sawf_ctx = dp_peer_sawf_stats_ctx_get(txrx_peer);

	if (!sawf_ctx) {
		qdf_err("Invalid SAWF stats ctx");
		return QDF_STATUS_E_FAILURE;
	}

	peer_id = ts->peer_id;

	/*
	 * Find remaped tid and target msdu queue idx to fill the stats
	 */

	host_msduq_idx = 0;
	status = dp_sawf_get_msduq_map_info(soc, peer_id,
					    host_msduq_idx,
					    &tid, &msduq_idx);
	if (status != QDF_STATUS_SUCCESS) {
		qdf_err("unable to find tid and msduq idx");
		return status;
	}

	qdf_assert(tid < DP_SAWF_MAX_TIDS);
	qdf_assert(msduq_idx < DP_SAWF_MAX_QUEUES);

	dp_sawf_update_tx_delay(soc, vdev, ts,
				&sawf_ctx->stats, tid, msduq_idx);

	length = qdf_nbuf_len(tx_desc->nbuf);

	DP_STATS_INCC_PKT(sawf_ctx, tx_stats[tid][msduq_idx].tx_success, 1,
			  length, (ts->status == HAL_TX_TQM_RR_FRAME_ACKED));

	DP_STATS_INCC_PKT(sawf_ctx, tx_stats[tid][msduq_idx].dropped.fw_rem, 1,
			  length, (ts->status == HAL_TX_TQM_RR_REM_CMD_REM));

	DP_STATS_INCC(sawf_ctx, tx_stats[tid][msduq_idx].dropped.fw_rem_notx, 1,
		      (ts->status == HAL_TX_TQM_RR_REM_CMD_NOTX));

	DP_STATS_INCC(sawf_ctx, tx_stats[tid][msduq_idx].dropped.fw_rem_tx, 1,
		      (ts->status == HAL_TX_TQM_RR_REM_CMD_TX));

	DP_STATS_INCC(sawf_ctx, tx_stats[tid][msduq_idx].dropped.age_out, 1,
		      (ts->status == HAL_TX_TQM_RR_REM_CMD_AGED));

	DP_STATS_INCC(sawf_ctx, tx_stats[tid][msduq_idx].dropped.fw_reason1, 1,
		      (ts->status == HAL_TX_TQM_RR_FW_REASON1));

	DP_STATS_INCC(sawf_ctx, tx_stats[tid][msduq_idx].dropped.fw_reason2, 1,
		      (ts->status == HAL_TX_TQM_RR_FW_REASON2));

	DP_STATS_INCC(sawf_ctx, tx_stats[tid][msduq_idx].dropped.fw_reason3, 1,
		      (ts->status == HAL_TX_TQM_RR_FW_REASON3));

	tx_stats = &sawf_ctx->stats.tx_stats[tid][msduq_idx];

	tx_stats->tx_failed = tx_stats->dropped.fw_rem.num +
				tx_stats->dropped.fw_rem_notx +
				tx_stats->dropped.fw_rem_tx +
				tx_stats->dropped.age_out +
				tx_stats->dropped.fw_reason1 +
				tx_stats->dropped.fw_reason2 +
				tx_stats->dropped.fw_reason3;

	if (tx_stats->queue_depth > 0)
		tx_stats->queue_depth--;

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
dp_sawf_tx_enqueue_peer_stats(struct dp_soc *soc,
			      struct dp_tx_desc_s *tx_desc,
			      uint8_t host_tid)
{
	struct dp_peer_sawf_stats *sawf_ctx;
	struct sawf_tx_stats *tx_stats;
	struct dp_peer *peer;
	struct dp_txrx_peer *txrx_peer;
	uint8_t tid, msduq_idx, host_msduq_idx;
	uint16_t peer_id;
	QDF_STATUS status;

	peer_id = tx_desc->peer_id;

	/*
	 * Find remaped tid and target msdu queue idx to fill the stats
	 */
	host_msduq_idx = 0;
	status = dp_sawf_get_msduq_map_info(soc, peer_id,
					    host_msduq_idx,
					    &tid, &msduq_idx);
	if (status != QDF_STATUS_SUCCESS) {
		qdf_err("unable to find tid and msduq idx");
		return status;
	}

	peer = dp_peer_get_ref_by_id(soc, peer_id, DP_MOD_ID_TX);
	if (!peer) {
		qdf_err("Invalid peer_id %u", peer_id);
		return QDF_STATUS_E_FAILURE;
	}

	txrx_peer = dp_get_txrx_peer(peer);
	if (!txrx_peer) {
		qdf_err("Invalid peer_id %u", peer_id);
		goto fail;
	}

	sawf_ctx = dp_peer_sawf_stats_ctx_get(txrx_peer);
	if (!sawf_ctx) {
		qdf_err("Invalid SAWF stats ctx");
		goto fail;
	}

	tx_stats = &sawf_ctx->stats.tx_stats[tid][msduq_idx];
	tx_stats->queue_depth++;

	dp_peer_unref_delete(peer, DP_MOD_ID_TX);

	return QDF_STATUS_SUCCESS;
fail:
	dp_peer_unref_delete(peer, DP_MOD_ID_TX);
	return QDF_STATUS_E_FAILURE;
}
