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
#include "dp_internal.h"
#include "dp_types.h"
#include "cdp_txrx_cmn_struct.h"
#include "cdp_txrx_hist_struct.h"
#include "cdp_txrx_extd_struct.h"

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
#define DP_SAWF_INVALID_AST_IDX 0xffff

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

struct dp_peer_sawf {
	struct sawf_def_queue_report tid_reports[DP_SAWF_MAX_TIDS];
};

QDF_STATUS
dp_sawf_def_queues_unmap_req(struct cdp_soc_t *soc_hdl,
			     uint8_t *mac_addr,
			     uint8_t svc_id);

QDF_STATUS
dp_sawf_def_queues_get_map_report(struct cdp_soc_t *soc_hdl,
				  uint8_t *mac_addr);

QDF_STATUS
dp_sawf_def_queues_map_req(struct cdp_soc_t *soc_hdl,
			   uint8_t *mac_addr, uint8_t svc_class_id);

QDF_STATUS
dp_peer_sawf_ctx_alloc(struct dp_soc *soc,
		       struct dp_peer *peer);

QDF_STATUS
dp_peer_sawf_ctx_free(struct dp_soc *soc,
		      struct dp_peer *peer);

struct dp_peer_sawf *dp_peer_sawf_ctx_get(struct dp_peer *peer);

QDF_STATUS
dp_peer_sawf_stats_ctx_alloc(struct dp_soc *soc,
			     struct dp_txrx_peer *txrx_peer);

QDF_STATUS
dp_peer_sawf_stats_ctx_free(struct dp_soc *soc,
			    struct dp_txrx_peer *txrx_peer);

struct dp_peer_sawf_stats *
dp_peer_sawf_stats_ctx_get(struct dp_txrx_peer *txrx_peer);

QDF_STATUS
dp_sawf_tx_compl_update_peer_stats(struct dp_soc *soc,
				   struct dp_vdev *vdev,
				   struct dp_txrx_peer *txrx_peer,
				   struct dp_tx_desc_s *tx_desc,
				   struct hal_tx_completion_status *ts,
				   uint8_t tid);

QDF_STATUS
dp_sawf_tx_enqueue_peer_stats(struct dp_soc *soc,
			      struct dp_tx_desc_s *tx_desc,
			      uint8_t tid);

#define DP_SAWF_STATS_SVC_CLASS_ID_ALL	0

QDF_STATUS
dp_sawf_dump_peer_stats(struct dp_txrx_peer *txrx_peer);

QDF_STATUS
dp_sawf_get_peer_delay_stats(struct cdp_soc_t *soc,
			     uint32_t svc_id, uint8_t *mac, void *data);

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
	uint32_t host_queue_id;
};

struct dp_peer_sawf {
	/* qdf_bitmap queue_usage; */
	struct dp_sawf_msduq msduq[DP_SAWF_Q_MAX];
	struct dp_sawf_msduq_tid_map
	       msduq_map[DP_SAWF_TID_MAX][DP_SAWF_DEFAULT_Q_PTID_MAX];
	struct sawf_def_queue_report tid_reports[DP_SAWF_TID_MAX];
};

uint16_t dp_sawf_get_msduq(struct net_device *netdev, uint8_t *peer_mac,
			   uint32_t service_id);
uint32_t dp_sawf_get_search_index(struct dp_soc *soc, qdf_nbuf_t nbuf,
				  uint8_t vdev_id, uint16_t queue_id);

#endif /* DP_SAWF_H*/
