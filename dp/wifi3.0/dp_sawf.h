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

#ifndef _DP_SAWF_H_
#define _DP_SAWF_H_

#include <qdf_lock.h>
#include <dp_types.h>
#include <dp_peer.h>
#include "dp_internal.h"
#include "dp_types.h"
#include "cdp_txrx_cmn_struct.h"
#include "cdp_txrx_hist_struct.h"
#include "cdp_txrx_extd_struct.h"

#define dp_sawf_alert(params...) \
	QDF_TRACE_FATAL(QDF_MODULE_ID_DP_SAWF, params)
#define dp_sawf_err(params...) \
	QDF_TRACE_ERROR(QDF_MODULE_ID_DP_SAWF, params)
#define dp_sawf_warn(params...) \
	QDF_TRACE_WARN(QDF_MODULE_ID_DP_SAWF, params)
#define dp_sawf_info(params...) \
	QDF_TRACE_INFO(QDF_MODULE_ID_DP_SAWF, params)
#define dp_sawf_debug(params...) \
	QDF_TRACE_DEBUG(QDF_MODULE_ID_DP_SAWF, params)

#define dp_sawf_nofl_alert(params...) \
	QDF_TRACE_FATAL_NO_FL(QDF_MODULE_ID_DP_SAWF, params)
#define dp_sawf_nofl_err(params...) \
	QDF_TRACE_ERROR_NO_FL(QDF_MODULE_ID_DP_SAWF, params)
#define dp_sawf_nofl_warn(params...) \
	QDF_TRACE_WARN_NO_FL(QDF_MODULE_ID_DP_SAWF, params)
#define dp_sawf_nofl_info(params...) \
	QDF_TRACE_INFO_NO_FL(QDF_MODULE_ID_DP_SAWF, params)
#define dp_sawf_nofl_debug(params...) \
	QDF_TRACE_DEBUG_NO_FL(QDF_MODULE_ID_DP_SAWF, params)

#define dp_sawf_alert_rl(params...) \
	QDF_TRACE_FATAL_RL(QDF_MODULE_ID_DP_SAWF, params)
#define dp_sawf_err_rl(params...) \
	QDF_TRACE_ERROR_RL(QDF_MODULE_ID_DP_SAWF, params)
#define dp_sawf_warn_rl(params...) \
	QDF_TRACE_WARN_RL(QDF_MODULE_ID_DP_SAWF, params)
#define dp_sawf_info_rl(params...) \
	QDF_TRACE_INFO_RL(QDF_MODULE_ID_DP_SAWF, params)
#define dp_sawf_debug_rl(params...) \
	QDF_TRACE_DEBUG_RL(QDF_MODULE_ID_DP_SAWF, params)

#define dp_sawf_debug_hex(ptr, size) \
	qdf_trace_hex_dump(QDF_MODULE_ID_DP_SAWF, \
			QDF_TRACE_LEVEL_DEBUG, ptr, size)

#define MSDU_QUEUE_LATENCY_WIN_MIN_SAMPLES 20
#define WLAN_TX_DELAY_UNITS_US 10
#define WLAN_TX_DELAY_MASK 0x1FFFFFFF
#define DP_SAWF_DEFINED_Q_PTID_MAX 2
#define DP_SAWF_DEFAULT_Q_PTID_MAX 2
#define DP_SAWF_TID_MAX 8
#define DP_SAWF_Q_MAX (DP_SAWF_DEFINED_Q_PTID_MAX * DP_SAWF_TID_MAX)
#define DP_SAWF_DEFAULT_Q_MAX (DP_SAWF_DEFAULT_Q_PTID_MAX * DP_SAWF_TID_MAX)
#define dp_sawf(peer, msduq_num, field) ((peer)->sawf->msduq[msduq_num].field)
#define DP_SAWF_DEFAULT_Q_INVALID 0xff
#define DP_SAWF_PEER_Q_INVALID 0xffff
#define DP_SAWF_INVALID_AST_IDX 0xffff

/**
 * sawf_stats - sawf stats
 * @delay: delay stats per-TID per-queue
 * @tx_stats: Tx stats per-TID per-queue
 * @lock: Protection for sawf-stats
 */
struct sawf_stats {
	struct sawf_delay_stats delay[DP_SAWF_MAX_TIDS][DP_SAWF_MAX_QUEUES];
	struct sawf_tx_stats tx_stats[DP_SAWF_MAX_TIDS][DP_SAWF_MAX_QUEUES];
	struct qdf_spinlock lock;
};

struct dp_peer_sawf_stats {
	struct sawf_stats stats;
};

struct sawf_def_queue_report {
	uint8_t svc_class_id;
};

/**
 * sawf_mov_avg_params - SAWF telemetry moving average params
 * @packet: num of packets per window
 * @window: num of windows
 *
 */
struct sawf_mov_avg_params {
	uint32_t packet;
	uint32_t window;
};

/**
 * sawf_mov_avg_params - SAWF telemetry SLA params
 * @num_packets: num of packets for SLA detection
 * @time_secs: num of sec for SLA detection
 *
 */
struct sawf_sla_params {
	uint32_t num_packets;
	uint32_t time_secs;
};

/**
 * sawf_telemetry_params - SAWF telemetry  params
 * @mov_avg: moving average params
 * @sla: SLA params
 *
 */
struct sawf_telemetry_params {
	struct sawf_mov_avg_params mov_avg;
	struct sawf_sla_params sla;
};

/**
 * dp_sawf_def_queues_unmap_req - unmap peer to service class ID mapping
 * @soc: soc handle
 * @mac_addr: mac address
 * @svc_id: service class ID
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_sawf_def_queues_unmap_req(struct cdp_soc_t *soc_hdl,
			     uint8_t *mac_addr,
			     uint8_t svc_id);

/**
 * dp_sawf_def_queues_get_map_report - get peer to sevice class ID mappings
 * @soc: soc handle
 * @mac_addr: mac address
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_sawf_def_queues_get_map_report(struct cdp_soc_t *soc_hdl,
				  uint8_t *mac_addr);

/**
 * dp_sawf_def_queues_map_req - map peer to service class ID
 * @soc: soc handle
 * @mac_addr: mac address
 * @svc_clss_id: service class ID
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_sawf_def_queues_map_req(struct cdp_soc_t *soc_hdl,
			   uint8_t *mac_addr, uint8_t svc_class_id);

/**
 * dp_peer_sawf_ctx_alloc - allocate SAWF ctx
 * @soc: soc handle
 * @peer: dp peer
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_peer_sawf_ctx_alloc(struct dp_soc *soc,
		       struct dp_peer *peer);

/**
 * dp_peer_sawf_ctx_free - free SAWF ctx
 * @soc: soc handle
 * @peer: dp peer
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_peer_sawf_ctx_free(struct dp_soc *soc,
		      struct dp_peer *peer);

/**
 * dp_peer_sawf_ctx_get - get SAWF ctx
 * @peer: dp peer
 *
 * Return: SAWF ctx on success; NULL otherwise
 */
struct dp_peer_sawf *dp_peer_sawf_ctx_get(struct dp_peer *peer);

/**
 * dp_peer_sawf_stats_ctx_alloc - allocate SAWF stats ctx
 * @soc: soc handle
 * @txrx_peer: DP txrx peer
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_peer_sawf_stats_ctx_alloc(struct dp_soc *soc,
			     struct dp_txrx_peer *txrx_peer);

/**
 * dp_peer_sawf_stats_ctx_free - free SAWF stats ctx
 * @soc: soc handle
 * @txrx_peer: DP txrx peer
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_peer_sawf_stats_ctx_free(struct dp_soc *soc,
			    struct dp_txrx_peer *txrx_peer);

/**
 * dp_peer_sawf_stats_ctx_free - free SAWF stats ctx
 * @txrx_peer: DP txrx peer
 *
 * Return: SAWF stas ctx on success, NULL otherwise
 */
struct dp_peer_sawf_stats *
dp_peer_sawf_stats_ctx_get(struct dp_txrx_peer *txrx_peer);

/**
 * dp_sawf_tx_compl_update_peer_stats - update SAWF stats in Tx completion
 * @soc: soc handle
 * @vdev: DP vdev context
 * @txrx_peer: DP txrx peer
 * @tx_desc: Tx descriptor
 * @ts: Tx completion status
 * @tid: TID
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_sawf_tx_compl_update_peer_stats(struct dp_soc *soc,
				   struct dp_vdev *vdev,
				   struct dp_txrx_peer *txrx_peer,
				   struct dp_tx_desc_s *tx_desc,
				   struct hal_tx_completion_status *ts,
				   uint8_t tid);

/**
 * dp_sawf_tx_enqueue_fail_peer_stats - update SAWF stats in Tx enqueue failure
 * @soc: soc handle
 * @tx_desc: Tx descriptor
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_sawf_tx_enqueue_fail_peer_stats(struct dp_soc *soc,
				   struct dp_tx_desc_s *tx_desc);

/**
 * dp_sawf_tx_enqueue_peer_stats - update SAWF stats in Tx enqueue
 * @soc: soc handle
 * @tx_desc: Tx descriptor
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_sawf_tx_enqueue_peer_stats(struct dp_soc *soc,
			      struct dp_tx_desc_s *tx_desc);

#define DP_SAWF_STATS_SVC_CLASS_ID_ALL	0

/**
 * dp_sawf_dump_peer_stats - print peer stats
 * @txrx_peer: DP txrx peer
 *
 * Return: Success
 */
QDF_STATUS
dp_sawf_dump_peer_stats(struct dp_txrx_peer *txrx_peer);

/**
 * dp_sawf_get_peer_delay_stats - get delay stats
 * @soc: soc handle
 * @svc_id: service class ID
 * @mac: mac address
 * @data: data to be filled
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_sawf_get_peer_delay_stats(struct cdp_soc_t *soc,
			     uint32_t svc_id, uint8_t *mac, void *data);

/**
 * dp_sawf_get_tx_stats- get MSDU Tx stats
 * @arg: argument
 * @in_bytes: Ingress Bytes
 * @in_cnt: Ingress Count
 * @tx_bytes: Transmit Bytes
 * @tx_cnt: Transmit count
 * @tid: TID
 * @msduq: MSDUQ ID
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_sawf_get_tx_stats(void *arg, uint64_t *in_bytes, uint64_t *in_cnt,
		     uint64_t *tx_bytes, uint64_t *tx_cnt,
		     uint8_t tid, uint8_t msduq);

/**
 * dp_sawf_get_mpdu_sched_stats - get MPDU scheduling stats
 * @arg: argument
 * @svc_int_pass: MPDU with service interval passed
 * @svc_int_fail: MPDU with service interval failed
 * @burst_pass: MPDU with burst size failed
 * @burst_fail: MPDU with burst size passed
 * @tid: TID
 * @msduq: MSDUQ ID
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_sawf_get_mpdu_sched_stats(void *arg, uint64_t *svc_int_pass,
			     uint64_t *svc_int_fail, uint64_t *burst_pass,
			     uint64_t *burst_fail, uint8_t tid, uint8_t msduq);

/**
 * dp_sawf_get_drop_stats- get MSDU drop stats
 * @arg: argument
 * @pass: MSDU trasnmitted successfully
 * @drop: MSDU dropped for any reason
 * @drop_ttl: MSDU dropped for TTL expiry
 * @tid: TID
 * @msduq: MSDUQ ID
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_sawf_get_drop_stats(void *arg, uint64_t *pass, uint64_t *drop,
		       uint64_t *drop_ttl, uint8_t tid, uint8_t msduq);

/**
 * dp_sawf_get_peer_tx_stats - get Tx stats
 * @soc: soc handle
 * @svc_id: service class ID
 * @mac: mac address
 * @data: data to be filled
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_sawf_get_peer_tx_stats(struct cdp_soc_t *soc,
			  uint32_t svc_id, uint8_t *mac, void *data);

struct dp_sawf_msduq {
	uint8_t ref_count;
	uint8_t htt_msduq;
	uint8_t remapped_tid;
	bool is_used;
	bool del_in_progress;
	uint32_t tx_flow_number;
	uint32_t svc_id;
};

struct dp_sawf_msduq_tid_map {
	uint8_t host_queue_id;
};

struct dp_peer_sawf {
	/* qdf_bitmap queue_usage; */
	struct dp_sawf_msduq msduq[DP_SAWF_Q_MAX];
	struct dp_sawf_msduq_tid_map
	       msduq_map[DP_SAWF_TID_MAX][DP_SAWF_DEFINED_Q_PTID_MAX];
	struct sawf_def_queue_report tid_reports[DP_SAWF_TID_MAX];
	void *telemetry_ctx;
};

uint16_t dp_sawf_get_msduq(struct net_device *netdev, uint8_t *peer_mac,
			   uint32_t service_id);
uint32_t dp_sawf_get_search_index(struct dp_soc *soc, qdf_nbuf_t nbuf,
				  uint8_t vdev_id, uint16_t queue_id);
uint32_t dp_sawf_queue_id_get(qdf_nbuf_t nbuf);
void dp_sawf_tcl_cmd(uint16_t *htt_tcl_metadata, qdf_nbuf_t nbuf);
bool dp_sawf_tag_valid_get(qdf_nbuf_t nbuf);

/*
 * dp_sawf_mpdu_stats_req() - Send MPDU basic stats request to target
 * @soc_hdl: SOC handle
 * @enable: 1: Enable 0: Disable
 *
 * @Return: QDF_STATUS_SUCCESS on success
 */
QDF_STATUS
dp_sawf_mpdu_stats_req(struct cdp_soc_t *soc_hdl, uint8_t enable);

/*
 * dp_sawf_mpdu_details_stats_req() - Send MPDU details stats request to target
 * @soc_hdl: SOC handle
 * @enable: 1: Enable 0: Disable
 *
 * @Return: QDF_STATUS_SUCCESS on success
 */
QDF_STATUS
dp_sawf_mpdu_details_stats_req(struct cdp_soc_t *soc_hdl, uint8_t enable);

/**
 * dp_sawf_set_mov_avg_params- Set moving average pararms
 * @num_pkt: No of packets per window to calucalte moving average
 * @num_win: No of windows to calucalte moving average
 *
 * Return: QDF_STATUS
 */
QDF_STATUS dp_sawf_set_mov_avg_params(uint32_t num_pkt,
				      uint32_t num_win);

/**
 * dp_sawf_set_sla_params- Set SLA pararms
 * @num_pkt: No of packets to detect SLA breach
 * @time_secs: Time ins secs to detect breach
 *
 * Return: QDF_STATUS
 */
QDF_STATUS dp_sawf_set_sla_params(uint32_t num_pkt,
				  uint32_t time_secs);

/**
 * dp_sawf_init_telemtry_param - Initialize telemetry params
 *
 * Return: QDF_STATUS
 */

QDF_STATUS dp_sawf_init_telemetry_params(void);

/** dp_sawf_update_svc_intval_mpdu_stats - Update Tx-mpdu_stats for svc_interval
 * @soc: soc handle
 * @peer_id: peer ID
 * @tid: tid used for Tx
 * @q_idx: msdu queue-index used for Tx
 * @success_cnt: no of msdu's successfully transmitted
 * @failure_cnt: no of msdu's failed to be transmitted
 *
 * Return: void
 */
static void dp_sawf_update_svc_intval_mpdu_stats(struct dp_soc *soc,
						 uint16_t peer_id,
						 uint8_t tid, uint8_t q_idx,
						 uint64_t success_cnt,
						 uint64_t failure_cnt)
{
	struct dp_txrx_peer *txrx_peer;
	dp_txrx_ref_handle txrx_ref_handle = NULL;
	struct dp_peer_sawf_stats *sawf_stats_ctx;
	struct sawf_fw_mpdu_stats *stats;

	if (!soc) {
		qdf_err("Invalid soc context");
		return;
	}

	txrx_peer = dp_txrx_peer_get_ref_by_id(soc, peer_id, &txrx_ref_handle,
					       DP_MOD_ID_SAWF);
	if (!txrx_peer) {
		qdf_err("No txrx-peer for id %d", peer_id);
		return;
	}

	sawf_stats_ctx = dp_peer_sawf_stats_ctx_get(txrx_peer);
	if (!sawf_stats_ctx) {
		qdf_err("No sawf stats ctx for peer_id %d", peer_id);
		dp_txrx_peer_unref_delete(txrx_ref_handle, DP_MOD_ID_SAWF);
		return;
	}

	stats = &sawf_stats_ctx->stats.tx_stats[tid][q_idx].svc_intval_stats;
	stats->success_cnt += success_cnt;
	stats->failure_cnt += failure_cnt;
	dp_txrx_peer_unref_delete(txrx_ref_handle, DP_MOD_ID_SAWF);
}

/**
 * dp_sawf_update_burst_size_mpdu_stats - Update Tx-mpdu_stats for burst_size
 * @soc: soc handle
 * @peer_id: peer ID
 * @tid: tid used for Tx
 * @q_idx: msdu queue-index used for Tx
 * @success_cnt: no of msdu's successfully transmitted
 * @failure_cnt: no of msdu's failed to be transmitted
 *
 * Return: void
 */
static void dp_sawf_update_burst_size_mpdu_stats(struct dp_soc *soc,
						 uint16_t peer_id,
						 uint8_t tid, uint8_t q_idx,
						 uint64_t success_cnt,
						 uint64_t failure_cnt)
{
	struct dp_txrx_peer *txrx_peer;
	dp_txrx_ref_handle txrx_ref_handle = NULL;
	struct dp_peer_sawf_stats *sawf_stats_ctx;
	struct sawf_fw_mpdu_stats *stats;

	if (!soc) {
		qdf_err("Invalid soc context");
		return;
	}

	txrx_peer = dp_txrx_peer_get_ref_by_id(soc, peer_id, &txrx_ref_handle,
					       DP_MOD_ID_SAWF);
	if (!txrx_peer) {
		qdf_err("No txrx_peer for id %d", peer_id);
		return;
	}

	sawf_stats_ctx = dp_peer_sawf_stats_ctx_get(txrx_peer);
	if (!sawf_stats_ctx) {
		qdf_err("No sawf stats ctx for peer_id %d", peer_id);
		dp_txrx_peer_unref_delete(txrx_ref_handle, DP_MOD_ID_SAWF);
		return;
	}

	stats = &sawf_stats_ctx->stats.tx_stats[tid][q_idx].burst_size_stats;
	stats->success_cnt += success_cnt;
	stats->failure_cnt += failure_cnt;
	dp_txrx_peer_unref_delete(txrx_ref_handle, DP_MOD_ID_SAWF);
}
#endif /* DP_SAWF_H*/
