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
#include "cdp_txrx_cmn_struct.h"
#include "cdp_txrx_hist_struct.h"
#include "cdp_txrx_extd_struct.h"

#define MSDU_QUEUE_LATENCY_WIN_MIN_SAMPLES 20
#define WLAN_TX_DELAY_UNITS_US 10
#define WLAN_TX_DELAY_MASK 0x1FFFFFFF

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
#endif /* DP_SAWF_H*/
