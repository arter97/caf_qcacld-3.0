/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
  * DOC: wlan_dp_stc.c
  *
  *
  */

#include "wlan_dp_stc.h"
#include "wlan_dp_fisa_rx.h"
#include "target_if.h"

#ifdef WLAN_DP_FEATURE_STC

/* API to be called from RX Flow table entry add */
/* API to be called from TX Flow table entry add */

QDF_STATUS
wlan_dp_stc_track_flow_features(struct wlan_dp_stc *dp_stc, qdf_nbuf_t nbuf,
				struct wlan_dp_stc_flow_table_entry *flow_entry,
				uint8_t vdev_id, uint16_t peer_id,
				uint32_t metadata)
{
	/* TODO - Fix the below API for getting pkt length */
	uint32_t pkt_len = qdf_nbuf_len(nbuf);
	uint64_t pkt_iat = 0;
	uint64_t curr_pkt_ts = dp_stc_get_timestamp();
	uint16_t s, w;

	if (flow_entry->metadata != metadata) {
		/* Clear the flow entry */
		qdf_mem_zero(flow_entry, sizeof(*flow_entry));
		flow_entry->metadata = metadata;
	}

	/* TxRx Stats - Start */
	if (flow_entry->prev_pkt_arrival_ts) {
		pkt_iat = curr_pkt_ts - flow_entry->prev_pkt_arrival_ts;
		DP_STC_UPDATE_MIN_MAX_SUM_STATS(flow_entry->txrx_stats.pkt_iat,
						pkt_iat);
	}
	flow_entry->prev_pkt_arrival_ts = curr_pkt_ts;

	flow_entry->txrx_stats.bytes += pkt_len;
	flow_entry->txrx_stats.pkts++;
	DP_STC_UPDATE_MIN_MAX_STATS(flow_entry->txrx_stats.pkt_size,
				    pkt_len);

	s = flow_entry->idx.s.sample_idx;
	w = flow_entry->idx.s.win_idx;
	if (s < DP_STC_TXRX_SAMPLES_MAX && w < DP_TXRX_SAMPLES_WINDOW_MAX) {
		struct wlan_dp_stc_txrx_min_max_stats *txrx_min_max_stats;

		/* Copy win-1 min-max to win-2 */
		if (w != 0 && !flow_entry->win_transition_complete) {
			struct wlan_dp_stc_txrx_min_max_stats *win_1;
			struct wlan_dp_stc_txrx_min_max_stats *win_2;

			win_1 = &flow_entry->txrx_min_max_stats[s][0];
			win_2 = &flow_entry->txrx_min_max_stats[s][w];

			win_2->pkt_size_min = win_1->pkt_size_min;
			win_2->pkt_size_max = win_1->pkt_size_max;
			win_2->pkt_iat_min = win_1->pkt_iat_min;
			win_2->pkt_iat_max = win_1->pkt_iat_max;
			flow_entry->win_transition_complete = 1;
		} else {
			flow_entry->win_transition_complete = 0;
		}

		txrx_min_max_stats = &flow_entry->txrx_min_max_stats[s][w];
		DP_STC_UPDATE_WIN_MIN_MAX_STATS(txrx_min_max_stats->pkt_size,
						pkt_len);
		DP_STC_UPDATE_WIN_MIN_MAX_STATS(txrx_min_max_stats->pkt_iat,
						pkt_iat);
	}
	/* TxRx Stats - End */

	/* Burst stats - START */
	switch (flow_entry->burst_state) {
	case BURST_DETECTION_INIT:
		/*
		 * We should move to BURST_DETECTION_START, since this piece of
		 * code will be executed only if the sampling is going on.
		 */
		flow_entry->burst_start_time = curr_pkt_ts;
		flow_entry->burst_state = BURST_DETECTION_START;
		flow_entry->burst_start_detect_bytes = pkt_len;
		break;
	case BURST_DETECTION_START:
	{
		uint32_t time_delta = curr_pkt_ts -
						flow_entry->burst_start_time;
		/*
		 * If time_delta < threshold then for the burst start detection
		 * current pkt bytes also should be considered.
		 */
		uint32_t bytes = flow_entry->burst_start_detect_bytes;

		if (time_delta < BURST_START_TIME_THRESHOLD_NS &&
		    (bytes + pkt_len) < BURST_START_BYTES_THRESHOLD) {
			/* BURST not started, but continue detection */
			flow_entry->burst_start_detect_bytes += pkt_len;
		} else if (time_delta > BURST_START_TIME_THRESHOLD_NS &&
			   bytes < BURST_START_BYTES_THRESHOLD) {
			/* Burst start failed, move to new burst detection */
			flow_entry->burst_state = BURST_DETECTION_INIT;
		} else if (time_delta < BURST_START_TIME_THRESHOLD_NS &&
			   (bytes + pkt_len) > BURST_START_BYTES_THRESHOLD) {
			/* Valid burst start */
			flow_entry->burst_start_detect_bytes += pkt_len;
			flow_entry->burst_state = BURST_DETECTION_BURST_START;
			flow_entry->burst_stats.burst_count++;
			flow_entry->cur_burst_bytes +=
					flow_entry->burst_start_detect_bytes;
		} else {
			/*
			 * (time_delta > BURST_START_TIME_THRESHOLD_NS &&
			 *     bytes > BURST_START_BYTES_THRESHOLD)
			 */
			/* Burst start failed, move to new burst detection */
			flow_entry->burst_start_time = curr_pkt_ts;
			flow_entry->burst_state = BURST_DETECTION_START;
			flow_entry->burst_start_detect_bytes = pkt_len;
			flow_entry->cur_burst_bytes = 0;
		}
		break;
	}
	case BURST_DETECTION_BURST_START:
	{
		if (pkt_iat > BURST_END_TIME_THRESHOLD_NS) {
			struct wlan_dp_stc_burst_stats *burst_stats;
			uint32_t burst_dur, burst_size;

			flow_entry->burst_state = BURST_DETECTION_INIT;
			burst_stats = &flow_entry->burst_stats;

			/*
			 * Burst ended at the last packet, so calculate
			 * burst duration using the last pkt timestamp
			 */
			burst_dur = flow_entry->prev_pkt_arrival_ts -
						flow_entry->burst_start_time;
			burst_size = flow_entry->cur_burst_bytes;

			DP_STC_UPDATE_MIN_MAX_SUM_STATS(burst_stats->burst_duration,
							burst_dur);
			DP_STC_UPDATE_MIN_MAX_SUM_STATS(burst_stats->burst_size,
							burst_size);
			flow_entry->burst_start_time = curr_pkt_ts;
			flow_entry->burst_state = BURST_DETECTION_START;
			flow_entry->burst_start_detect_bytes = pkt_len;
			flow_entry->cur_burst_bytes = 0;
			break;
		}

		flow_entry->cur_burst_bytes += pkt_len;
		break;
	}
	default:
		break;
	}
	/* Burst stats - END */

	return QDF_STATUS_SUCCESS;
}

static inline int
wlan_dp_stc_get_valid_sampling_flow_count(struct wlan_dp_stc *dp_stc)
{
	return qdf_atomic_read(&dp_stc->sampling_flow_table.num_valid_entries);
}

static inline void
wlan_dp_stc_get_avail_flow_quota(struct wlan_dp_stc *dp_stc, uint8_t *bidi,
				 uint8_t *tx, uint8_t *rx)
{
	struct wlan_dp_stc_sampling_table *sampling_table;

	sampling_table = &dp_stc->sampling_flow_table;

	*bidi = DP_STC_SAMPLE_BIDI_FLOW_MAX -
			qdf_atomic_read(&sampling_table->num_bidi_flows);
	*tx = DP_STC_SAMPLE_TX_FLOW_MAX -
			qdf_atomic_read(&sampling_table->num_tx_only_flows);
	*rx = DP_STC_SAMPLE_RX_FLOW_MAX -
			qdf_atomic_read(&sampling_table->num_rx_only_flows);
}

static inline void
wlan_dp_stc_fill_rx_flow_candidate(struct wlan_dp_stc *dp_stc,
				   struct wlan_dp_stc_sampling_candidate *candidate,
				   uint16_t rx_flow_id,
				   uint32_t rx_flow_metadata)
{
	candidate->rx_flow_id = rx_flow_id;
	candidate->rx_flow_metadata = rx_flow_metadata;
	candidate->flags |= WLAN_DP_SAMPLING_CANDIDATE_VALID;
	candidate->flags |= WLAN_DP_SAMPLING_CANDIDATE_RX_FLOW_VALID;
}

static inline void
wlan_dp_stc_fill_bidi_flow_candidate(struct wlan_dp_stc *dp_stc,
				     struct wlan_dp_stc_sampling_candidate *candidate,
				     uint16_t tx_flow_id,
				     uint32_t tx_flow_metadata,
				     uint16_t rx_flow_id,
				     uint32_t rx_flow_metadata)
{
	candidate->tx_flow_id = tx_flow_id;
	candidate->tx_flow_metadata = tx_flow_metadata;
	candidate->rx_flow_id = rx_flow_id;
	candidate->rx_flow_metadata = rx_flow_metadata;
	candidate->flags |= WLAN_DP_SAMPLING_CANDIDATE_VALID;
	candidate->flags |= WLAN_DP_SAMPLING_CANDIDATE_TX_FLOW_VALID;
	candidate->flags |= WLAN_DP_SAMPLING_CANDIDATE_RX_FLOW_VALID;
}

static inline void
wlan_dp_move_candidate_to_sample_table(struct wlan_dp_stc *dp_stc,
				       struct wlan_dp_stc_sampling_candidate *candidate,
				       struct wlan_dp_stc_sampling_table_entry *sampling_flow)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_stc->dp_ctx;
	struct wlan_dp_stc_sampling_table *sampling_table =
					&dp_stc->sampling_flow_table;

	sampling_flow->dir = candidate->dir;
	switch (candidate->dir) {
	case WLAN_DP_FLOW_DIR_TX:
		qdf_atomic_inc(&sampling_table->num_tx_only_flows);
		break;
	case WLAN_DP_FLOW_DIR_RX:
		qdf_atomic_inc(&sampling_table->num_rx_only_flows);
		break;
	case WLAN_DP_FLOW_DIR_BIDI:
		qdf_atomic_inc(&sampling_table->num_bidi_flows);
		break;
	default:
		break;
	}

	sampling_flow->peer_id = candidate->peer_id;
	if (candidate->flags & WLAN_DP_SAMPLING_CANDIDATE_TX_FLOW_VALID) {
		sampling_flow->flags |= WLAN_DP_SAMPLING_FLAGS_TX_FLOW_VALID;
		sampling_flow->tx_flow_id = candidate->tx_flow_id;
		sampling_flow->tx_flow_metadata = candidate->tx_flow_metadata;
	}

	if (candidate->flags & WLAN_DP_SAMPLING_CANDIDATE_RX_FLOW_VALID) {
		struct dp_rx_fst *fisa_hdl = dp_ctx->rx_fst;
		struct dp_fisa_rx_sw_ft *sw_ft_entry;

		sw_ft_entry = &(((struct dp_fisa_rx_sw_ft *)fisa_hdl->base)[candidate->rx_flow_id]);
		sampling_flow->flags |= WLAN_DP_SAMPLING_FLAGS_RX_FLOW_VALID;
		sampling_flow->rx_flow_id = candidate->rx_flow_id;
		sampling_flow->rx_flow_metadata = candidate->rx_flow_metadata;
		wlan_dp_stc_populate_flow_tuple(&sampling_flow->flow_samples.flow_tuple,
						&sw_ft_entry->rx_flow_tuple_info);
		sampling_flow->tuple_hash = sw_ft_entry->flow_tuple_hash;
	}

	sampling_flow->state = WLAN_DP_SAMPLING_STATE_FLOW_ADDED;
}

static inline struct wlan_dp_stc_sampling_table_entry *
wlan_dp_get_free_sampling_table_entry(struct wlan_dp_stc *dp_stc)
{
	int i;
	struct wlan_dp_stc_sampling_table *table = &dp_stc->sampling_flow_table;

	for (i = 0; i < DP_STC_SAMPLE_FLOWS_MAX; i++) {
		if (table->entries[i].state == WLAN_DP_SAMPLING_STATE_INIT)
			return &table->entries[i];
	}

	return NULL;
}

static inline bool
wlan_dp_txrx_samples_ready(struct wlan_dp_stc_sampling_table_entry *flow)
{
	if (flow->flags & WLAN_DP_SAMPLING_FLAGS_TXRX_SAMPLES_READY)
		return true;

	return false;
}

static inline bool
wlan_dp_txrx_samples_reported(struct wlan_dp_stc_sampling_table_entry *flow)
{
	if (flow->flags1 & WLAN_DP_SAMPLING_FLAGS1_TXRX_SAMPLES_SENT)
		return true;

	return false;
}

static inline bool
wlan_dp_burst_samples_ready(struct wlan_dp_stc_sampling_table_entry *flow)
{
	if (flow->flags & WLAN_DP_SAMPLING_FLAGS_BURST_SAMPLES_READY)
		return true;

	return false;
}

static inline bool
wlan_dp_burst_samples_reported(struct wlan_dp_stc_sampling_table_entry *flow)
{
	if (flow->flags1 & WLAN_DP_SAMPLING_FLAGS1_BURST_SAMPLES_SENT)
		return true;

	return false;
}

static inline QDF_STATUS
wlan_dp_stc_find_ul_flow(struct wlan_dp_stc *dp_stc, uint16_t rx_flow_id,
			 uint64_t rx_flow_tuple_hash, uint16_t *tx_flow_id,
			 uint64_t *tx_flow_metadata)
{
	return QDF_STATUS_E_INVAL;
}

static inline QDF_STATUS
wlan_dp_send_txrx_sample(struct wlan_dp_stc *dp_stc,
			 struct wlan_dp_stc_sampling_table_entry *flow)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_stc->dp_ctx;
	struct wlan_objmgr_psoc *psoc = dp_ctx->psoc;

	if (dp_ctx->dp_ops.send_flow_stats_event)
		return dp_ctx->dp_ops.send_flow_stats_event(psoc,
						&flow->flow_samples,
						WLAN_DP_TXRX_SAMPLES_READY);

	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS
wlan_dp_send_burst_sample(struct wlan_dp_stc *dp_stc,
			  struct wlan_dp_stc_sampling_table_entry *flow)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_stc->dp_ctx;
	struct wlan_objmgr_psoc *psoc = dp_ctx->psoc;

	if (dp_ctx->dp_ops.send_flow_stats_event)
		return dp_ctx->dp_ops.send_flow_stats_event(psoc,
						&flow->flow_samples,
						WLAN_DP_BURST_SAMPLES_READY);

	return QDF_STATUS_SUCCESS;
}

#define FLOW_TRACK_ELIGIBLE_THRESH_US 10000000

static void wlan_dp_stc_flow_monitor_work_handler(void *arg)
{
	struct wlan_dp_stc *dp_stc = (struct wlan_dp_stc *)arg;
	struct wlan_dp_psoc_context *dp_ctx = dp_stc->dp_ctx;
	struct dp_rx_fst *fst = dp_ctx->rx_fst;
	struct wlan_dp_stc_sampling_candidate candidates[DP_STC_SAMPLE_FLOWS_MAX];
	uint32_t candidate_idx = 0;
	uint8_t bidi, tx, rx;
	uint16_t rx_flow_id, tx_flow_id, flow_id;
	struct dp_rx_fst *fisa_hdl = dp_ctx->rx_fst;
	uint64_t rx_flow_tuple_hash;
	struct dp_fisa_rx_sw_ft *sw_ft_entry;
	struct wlan_dp_stc_sampling_table_entry *sampling_flow;
	uint64_t cur_ts = dp_stc_get_timestamp();
	uint64_t tx_flow_metadata;
	QDF_STATUS status;
	int i;
	bool start_timer = false, candidate_selected = false;

	/*
	 * 1) Monitor TX/RX flows
	 * 2) Add long flow to Sampling flow table
	 */

	if (wlan_dp_stc_get_valid_sampling_flow_count(dp_stc) ==
						DP_STC_SAMPLE_FLOWS_MAX)
		goto other_checks;

	wlan_dp_stc_get_avail_flow_quota(dp_stc, &bidi, &tx, &rx);

	for (rx_flow_id = 0; rx_flow_id < fst->max_entries; rx_flow_id++) {
		if (!rx && !bidi)
			break;
		/* loop through entire FISA table */
		sw_ft_entry = &(((struct dp_fisa_rx_sw_ft *)fisa_hdl->base)[rx_flow_id]);
		if (!sw_ft_entry->is_populated ||
		    sw_ft_entry->selected_to_sample ||
		    sw_ft_entry->classified)
			continue;

		if (cur_ts - sw_ft_entry->flow_init_ts <
						FLOW_TRACK_ELIGIBLE_THRESH_US)
			continue;

		/* Temp place holder function */
		rx_flow_tuple_hash =
				wlan_dp_fisa_get_flow_tuple_hash(sw_ft_entry);
		/*
		 * This function should search and mark the flow id in
		 * tx_flow for cross referencing
		 */
		status = wlan_dp_stc_find_ul_flow(dp_stc,
						  rx_flow_id,
						  rx_flow_tuple_hash,
						  &tx_flow_id,
						  &tx_flow_metadata);
		if (QDF_IS_STATUS_ERROR(status)) {
			if (rx) {
				/* This is a RX only flow. */
				wlan_dp_stc_fill_rx_flow_candidate(dp_stc,
						&candidates[candidate_idx],
						rx_flow_id,
						sw_ft_entry->metadata);
				candidates[candidate_idx].peer_id =
							sw_ft_entry->peer_id;
				candidates[candidate_idx].dir = WLAN_DP_FLOW_DIR_RX;
				candidate_idx++;
				rx--;
				candidate_selected = true;
			}
			continue;
		}

		if (bidi) {
			wlan_dp_stc_fill_bidi_flow_candidate(dp_stc,
					&candidates[candidate_idx],
					tx_flow_id, tx_flow_metadata,
					rx_flow_id, sw_ft_entry->metadata);
			candidates[candidate_idx].dir = WLAN_DP_FLOW_DIR_BIDI;
			candidate_idx++;
			bidi--;
			candidate_selected = true;
			continue;
		}
	}

	if (!candidate_selected)
		goto other_checks;

	for (i = 0; i < DP_STC_SAMPLE_FLOWS_MAX; i++) {
		if (!(candidates[i].flags & WLAN_DP_SAMPLING_CANDIDATE_VALID))
			continue;

		/* Optimize this function to return array of pointers */
		sampling_flow = wlan_dp_get_free_sampling_table_entry(dp_stc);
		if (!sampling_flow) {
			/*
			 * Quota is present but entry is not free.
			 * This cannot happen
			 */
			qdf_assert_always(0);
			break;
		}

		/*
		 * Set the indication in tx & rx flows,
		 * for us to not shortlist them again.
		 */
		if (sampling_flow->flags |
					WLAN_DP_SAMPLING_FLAGS_RX_FLOW_VALID) {
			sw_ft_entry = &(((struct dp_fisa_rx_sw_ft *)fisa_hdl->base)[candidates[i].rx_flow_id]);
			sw_ft_entry->selected_to_sample = 1;
		}

		wlan_dp_move_candidate_to_sample_table(dp_stc, &candidates[i],
						       sampling_flow);
		start_timer = true;
	}

	if (start_timer &&
	    dp_stc->sample_timer_state < WLAN_DP_STC_TIMER_STARTED) {
		qdf_timer_mod(&dp_stc->flow_sampling_timer,
			      DP_STC_TIMER_THRESH_MS);
	}

other_checks:
	/* 3) Check if flow samples are ready to be sent to userspace */
	for (flow_id = 0; flow_id < DP_STC_SAMPLE_FLOWS_MAX; flow_id++) {
		sampling_flow = &dp_stc->sampling_flow_table.entries[flow_id];

		if (wlan_dp_txrx_samples_ready(sampling_flow) &&
		    !wlan_dp_txrx_samples_reported(sampling_flow)) {
			/* TXRX sample is ready to be sent */
			sampling_flow->flags1 |=
				WLAN_DP_SAMPLING_FLAGS1_TXRX_SAMPLES_SENT;
			wlan_dp_send_txrx_sample(dp_stc, sampling_flow);
			/*
			 * Set flag to indicate that the txrx samples
			 * have been reported
			 */
		}

		if (wlan_dp_burst_samples_ready(sampling_flow) &&
		    !wlan_dp_burst_samples_reported(sampling_flow)) {
			/* Burst samples are ready to be sent */
			sampling_flow->flags1 |=
				WLAN_DP_SAMPLING_FLAGS1_BURST_SAMPLES_SENT;
			wlan_dp_send_burst_sample(dp_stc, sampling_flow);
			/*
			 * Set flag to indicate that the burst sample
			 * has been reported
			 */
		}
	}

	/*
	 * 3) Flow inactivity detection
	 * 4) Ping inactivity detection
	 */
}

static inline QDF_STATUS
wlan_dp_stc_trigger_sampling(struct wlan_dp_stc *dp_stc, uint16_t flow_id,
			     uint8_t track, enum qdf_proto_dir dir)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_stc->dp_ctx;

	switch (dir) {
	case QDF_TX:
		break;
	case QDF_RX:
	{
		struct dp_rx_fst *fisa_hdl = dp_ctx->rx_fst;
		struct dp_fisa_rx_sw_ft *sw_ft_entry;

		sw_ft_entry =
			&(((struct dp_fisa_rx_sw_ft *)fisa_hdl->base)[flow_id]);
		sw_ft_entry->track_flow_stats = track;
		dp_info("STC: RX flow %d track %d", flow_id, track);
		break;
	}
	default:
		break;
	}

	return QDF_STATUS_SUCCESS;
}

static inline void
wlan_dp_stc_save_delta_stats(struct wlan_dp_stc_txrx_stats *dst,
			     struct wlan_dp_stc_txrx_stats *cur,
			     struct wlan_dp_stc_txrx_stats *ref)
{
	dst->bytes = cur->bytes - ref->bytes;
	dst->pkts = cur->pkts - ref->pkts;
	dst->pkt_iat_sum = cur->pkt_iat_sum - ref->pkt_iat_sum;
}

static inline void
wlan_dp_stc_save_min_max_state(struct wlan_dp_stc_txrx_stats *dst,
			       struct wlan_dp_stc_txrx_min_max_stats *src)
{
	dst->pkt_size_min = src->pkt_size_min;
	dst->pkt_size_max = src->pkt_size_max;
	dst->pkt_iat_min = src->pkt_iat_min;
	dst->pkt_iat_max = src->pkt_iat_max;
}

static inline void
wlan_dp_stc_save_burst_samples(struct wlan_dp_stc *dp_stc,
			       struct wlan_dp_stc_sampling_table_entry *sampling_entry)
{
	struct wlan_dp_stc_flow_table_entry *flow;
	struct wlan_dp_stc_burst_samples *burst_sample;
	uint32_t burst_dur, burst_size;

	burst_sample = &sampling_entry->flow_samples.burst_sample;
	if (!(sampling_entry->flags & WLAN_DP_SAMPLING_FLAGS_TX_FLOW_VALID))
		goto save_rx_flow_samples;

	flow = &dp_stc->tx_flow_table.entries[sampling_entry->tx_flow_id];
	qdf_mem_copy(&burst_sample->txrx_samples.tx, &flow->txrx_stats,
		     sizeof(burst_sample->txrx_samples.tx));
	qdf_mem_copy(&burst_sample->tx, &flow->burst_stats,
		     sizeof(burst_sample->tx));
	if (flow->burst_state == BURST_DETECTION_BURST_START) {
		struct wlan_dp_stc_burst_stats *burst_stats;

		burst_stats = &burst_sample->tx;

		/*
		 * Burst ended at the last packet, so calculate
		 * burst duration using the last pkt timestamp
		 */
		burst_dur = flow->prev_pkt_arrival_ts -
					flow->burst_start_time;
		burst_size = flow->cur_burst_bytes;

		DP_STC_UPDATE_MIN_MAX_SUM_STATS(burst_stats->burst_duration,
						burst_dur);
		DP_STC_UPDATE_MIN_MAX_SUM_STATS(burst_stats->burst_size,
						burst_size);
	}

save_rx_flow_samples:
	if (!(sampling_entry->flags & WLAN_DP_SAMPLING_FLAGS_RX_FLOW_VALID))
		return;

	flow = &dp_stc->rx_flow_table.entries[sampling_entry->rx_flow_id];
	qdf_mem_copy(&burst_sample->txrx_samples.rx, &flow->txrx_stats,
		     sizeof(burst_sample->txrx_samples.rx));
	burst_sample->txrx_samples.win_size = DP_STC_LONG_WINDOW_MS;
	qdf_mem_copy(&burst_sample->rx, &flow->burst_stats,
		     sizeof(burst_sample->rx));
	if (flow->burst_state == BURST_DETECTION_BURST_START) {
		struct wlan_dp_stc_burst_stats *burst_stats;

		burst_stats = &burst_sample->rx;

		/*
		 * Burst ended at the last packet, so calculate
		 * burst duration using the last pkt timestamp
		 */
		burst_dur = flow->prev_pkt_arrival_ts -
					flow->burst_start_time;
		burst_size = flow->cur_burst_bytes;

		DP_STC_UPDATE_MIN_MAX_SUM_STATS(burst_stats->burst_duration,
						burst_dur);
		DP_STC_UPDATE_MIN_MAX_SUM_STATS(burst_stats->burst_size,
						burst_size);

	}
}

static inline QDF_STATUS
wlan_dp_stc_remove_sampling_table_entry(struct wlan_dp_stc *dp_stc,
					struct wlan_dp_stc_sampling_table_entry *sampling_entry)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_stc->dp_ctx;
	struct wlan_dp_stc_sampling_table *sampling_table =
					&dp_stc->sampling_flow_table;
	struct dp_rx_fst *fisa_hdl = dp_ctx->rx_fst;
	struct dp_fisa_rx_sw_ft *sw_ft_entry;

	sampling_entry->state = WLAN_DP_SAMPLING_STATE_INIT;

	if (sampling_entry->flags & WLAN_DP_SAMPLING_FLAGS_TX_FLOW_VALID) {
		/* TODO - Add code for clearing TX AFT */
	}

	if (sampling_entry->flags & WLAN_DP_SAMPLING_FLAGS_RX_FLOW_VALID) {
		uint16_t flow_id = sampling_entry->rx_flow_id;

		sw_ft_entry =
			&(((struct dp_fisa_rx_sw_ft *)fisa_hdl->base)[flow_id]);
		if (sampling_entry->rx_flow_metadata == sw_ft_entry->metadata)
			sw_ft_entry->track_flow_stats = 0;
	}

	switch (sampling_entry->dir) {
	case WLAN_DP_FLOW_DIR_TX:
		qdf_atomic_dec(&sampling_table->num_tx_only_flows);
		break;
	case WLAN_DP_FLOW_DIR_RX:
		qdf_atomic_dec(&sampling_table->num_rx_only_flows);
		break;
	case WLAN_DP_FLOW_DIR_BIDI:
		qdf_atomic_dec(&sampling_table->num_bidi_flows);
		break;
	default:
		break;
	}

	return QDF_STATUS_SUCCESS;
}

static inline void
wlan_dp_stc_stop_flow_tracking(struct wlan_dp_stc *dp_stc,
			       struct wlan_dp_stc_sampling_table_entry *sampling_entry)
{
	uint32_t flow_id;

	if (sampling_entry->flags & WLAN_DP_SAMPLING_FLAGS_TX_FLOW_VALID) {
		flow_id = sampling_entry->tx_flow_id;
		wlan_dp_stc_trigger_sampling(dp_stc, flow_id, 0, QDF_TX);
	}

	if (sampling_entry->flags & WLAN_DP_SAMPLING_FLAGS_RX_FLOW_VALID) {
		flow_id = sampling_entry->rx_flow_id;
		wlan_dp_stc_trigger_sampling(dp_stc, flow_id, 0, QDF_RX);
	}
}

static inline bool
wlan_dp_stc_sample_flow(struct wlan_dp_stc *dp_stc,
			struct wlan_dp_stc_sampling_table_entry *sampling_entry)
{
	struct wlan_dp_stc_txrx_stats tx_stats = {0};
	struct wlan_dp_stc_txrx_stats rx_stats = {0};
	struct wlan_dp_stc_txrx_samples *txrx_samples;
	struct wlan_dp_stc_flow_samples *flow_samples =
						&sampling_entry->flow_samples;
	uint8_t sample_idx, win_idx;
	bool sampling_pending = false;
	struct wlan_dp_stc_flow_table_entry *flow;
	uint32_t flow_id;

	switch (sampling_entry->state) {
	case WLAN_DP_SAMPLING_STATE_INIT:
		/* Skip this entry, since no flow is added here */
		break;
	case WLAN_DP_SAMPLING_STATE_FLOW_ADDED:
	{
		struct wlan_dp_stc_burst_samples *burst_sample;

		burst_sample = &flow_samples->burst_sample;
		/* Send an indication to TX and RX flow to start tracking */
		/*
		 * Flow is just added, so take the stats snapshot and
		 * move it to WLAN_DP_SAMPLING_STATE_SAMPLING_START
		 */
		sampling_entry->next_sample_idx = 0;
		sampling_entry->next_win_idx = 0;

		if (sampling_entry->flags &
					WLAN_DP_SAMPLING_FLAGS_TX_FLOW_VALID) {
			flow_id = sampling_entry->tx_flow_id;
			wlan_dp_stc_trigger_sampling(dp_stc, flow_id, 1, QDF_TX);
			flow = &dp_stc->tx_flow_table.entries[flow_id];
#ifdef METADATA_CHECK_NEEDED_DURING_ADD
			if (sampling_entry->tx_flow_metadata !=
							flow->guid) {
				qdf_assert_always(0);
				goto rx_flow_sample;
			}
#endif
			flow->metadata = sampling_entry->tx_flow_metadata;

			flow->idx.sample_win_idx =
				(sampling_entry->next_sample_idx << 16) |
				(sampling_entry->next_win_idx);
			qdf_mem_copy(&sampling_entry->tx_stats_ref,
				     &flow->txrx_stats,
				     sizeof(sampling_entry->tx_stats_ref));
			/*
			 * Push these ref stats to burst_stats->txrx_stats,
			 * since it can be used as ref for 30-sec window
			 */
			qdf_mem_copy(&burst_sample->txrx_samples.tx,
				     &sampling_entry->tx_stats_ref,
				     sizeof(struct wlan_dp_stc_txrx_stats));
		}

#ifdef METADATA_CHECK_NEEDED_DURING_ADD
sample:
#endif
		if (sampling_entry->flags &
					WLAN_DP_SAMPLING_FLAGS_RX_FLOW_VALID) {
			flow_id = sampling_entry->rx_flow_id;
			wlan_dp_stc_trigger_sampling(dp_stc, flow_id, 1, QDF_RX);
			flow = &dp_stc->rx_flow_table.entries[flow_id];
#ifdef METADATA_CHECK_NEEDED_DURING_ADD
			if (sampling_entry->rx_flow_metadata !=
							flow->metadata) {
				qdf_assert_always(0);
				goto fail;
			}
#endif

			flow->metadata = sampling_entry->rx_flow_metadata;
			flow->idx.sample_win_idx =
				(sampling_entry->next_sample_idx << 16) |
				(sampling_entry->next_win_idx);
			qdf_mem_copy(&sampling_entry->rx_stats_ref,
				     &flow->txrx_stats,
				     sizeof(sampling_entry->rx_stats_ref));

			/*
			 * Push these ref stats to burst_stats->txrx_stats,
			 * since it can be used as ref for 30-sec window
			 */
			qdf_mem_copy(&burst_sample->txrx_samples.rx,
				     &sampling_entry->rx_stats_ref,
				     sizeof(struct wlan_dp_stc_txrx_stats));
		}

		sampling_entry->max_num_sample_attempts =
				DP_STC_LONG_WINDOW_MS / DP_STC_TIMER_THRESH_MS;
		sampling_entry->state = WLAN_DP_SAMPLING_STATE_SAMPLING_START;
		sampling_pending = true;
		break;
	}
	case WLAN_DP_SAMPLING_STATE_SAMPLING_START:
		sample_idx = sampling_entry->next_sample_idx;
		win_idx = sampling_entry->next_win_idx;
		txrx_samples = &flow_samples->txrx_samples[sample_idx][win_idx];
		dp_info("STC: collect sample %d win %d", sample_idx, win_idx);
		if ((win_idx + 1) == DP_TXRX_SAMPLES_WINDOW_MAX) {
			sampling_entry->next_win_idx = 0;
			sampling_entry->next_sample_idx++;
			txrx_samples->win_size = DP_STC_TIMER_THRESH_MS +
						 DP_STC_TIMER_THRESH_MS;
		} else {
			sampling_entry->next_win_idx++;
			txrx_samples->win_size = DP_STC_TIMER_THRESH_MS;
		}

		sampling_entry->max_num_sample_attempts--;
		/*
		 * 1) wlan_dp_stc_get_txrx_stats for Tx and Rx flow
		 * 2) Calculate delta stats
		 * 3) Push to the sampling flow table
		 * 4) Update next_sample_idx & next_win_idx
		 * 5) If next_sample_idx & next_win_idx = (5/0),
		 *	mark sampling done
		 * 6) Incr some global atomic variable ?? to indicate to
		 *	periodic work.
		 * There should be no further access to this index.
		 */

		if (sampling_entry->flags &
					WLAN_DP_SAMPLING_FLAGS_TX_FLOW_VALID) {
			flow_id = sampling_entry->tx_flow_id;
			flow = &dp_stc->tx_flow_table.entries[flow_id];
			if (sampling_entry->tx_flow_metadata !=
							flow->metadata) {
				wlan_dp_stc_remove_sampling_table_entry(dp_stc,
									sampling_entry);
				/* continue to next flow */
				break;
			}

			flow->idx.sample_win_idx =
				((sampling_entry->next_sample_idx) << 16) |
				(sampling_entry->next_win_idx);
			qdf_mem_copy(&tx_stats,
				     &flow->txrx_stats,
				     sizeof(tx_stats));
			wlan_dp_stc_save_delta_stats(&txrx_samples->tx,
						     &tx_stats,
						     &sampling_entry->tx_stats_ref);
			wlan_dp_stc_save_min_max_state(&txrx_samples->tx,
						       &flow->txrx_min_max_stats[sample_idx][win_idx]);
		}

		if (sampling_entry->flags &
					WLAN_DP_SAMPLING_FLAGS_RX_FLOW_VALID) {
			flow_id = sampling_entry->rx_flow_id;
			flow = &dp_stc->rx_flow_table.entries[flow_id];
			if (sampling_entry->rx_flow_metadata !=
							flow->metadata) {
				wlan_dp_stc_remove_sampling_table_entry(dp_stc,
									sampling_entry);
				/* continue to next flow */
				break;
			}

			flow->idx.sample_win_idx =
				((sampling_entry->next_sample_idx) << 16) |
				(sampling_entry->next_win_idx);
			qdf_mem_copy(&rx_stats,
				     &flow->txrx_stats,
				     sizeof(rx_stats));
			wlan_dp_stc_save_delta_stats(&txrx_samples->rx,
						     &rx_stats,
						     &sampling_entry->rx_stats_ref);
			wlan_dp_stc_save_min_max_state(&txrx_samples->rx,
						       &flow->txrx_min_max_stats[sample_idx][win_idx]);
		}

		if (sampling_entry->next_win_idx == 0) {
			qdf_mem_copy(&sampling_entry->tx_stats_ref,
				     &tx_stats,
				     sizeof(sampling_entry->tx_stats_ref));
			qdf_mem_copy(&sampling_entry->rx_stats_ref,
				     &rx_stats,
				     sizeof(sampling_entry->rx_stats_ref));
		}
		if (sampling_entry->next_sample_idx ==
						DP_STC_TXRX_SAMPLES_MAX) {
			sampling_entry->flags |=
				WLAN_DP_SAMPLING_FLAGS_TXRX_SAMPLES_READY;
			sampling_entry->state =
				WLAN_DP_SAMPLING_STATE_SAMPLING_BURST_STATS;
		}
		sampling_pending = true;
		break;
	case WLAN_DP_SAMPLING_STATE_SAMPLING_BURST_STATS:
		sampling_entry->max_num_sample_attempts--;
		if (qdf_unlikely(!sampling_entry->max_num_sample_attempts)) {
			/* Burst samples are ready, push to Global */
			wlan_dp_stc_save_burst_samples(dp_stc,
						       sampling_entry);
			sampling_entry->flags |=
				WLAN_DP_SAMPLING_FLAGS_BURST_SAMPLES_READY;
			sampling_entry->state =
				WLAN_DP_SAMPLING_STATE_SAMPLING_DONE;
			/* Set some indication to periodic work */
			wlan_dp_stc_stop_flow_tracking(dp_stc,
						       sampling_entry);
			break;
		}
		sampling_pending = true;
		break;
	case WLAN_DP_SAMPLING_STATE_SAMPLING_DONE:
		/*
		 * Nothing to be done.
		 * Periodic work will take care of NL packing.
		 * The periodic work will remove this entry as well
		 * from the sampling table.
		 */
		/* Maybe mark done in original flow table,
		 * so that code execution does not reach here
		 */
		break;
	case WLAN_DP_SAMPLING_STATE_SAMPLES_SENT:
		fallthrough;
	case WLAN_DP_SAMPLING_STATE_CLASSIFIED:
		fallthrough;
	default:
		break;
	}

	return sampling_pending;
}

static void wlan_dp_stc_flow_sampling_timer(void *arg)
{
	struct wlan_dp_stc *dp_stc = (struct wlan_dp_stc *)arg;
	struct wlan_dp_stc_sampling_table_entry *sampling_entry;
	bool sampling_pending = false;
	int i;

	for (i = 0; i < DP_STC_SAMPLE_FLOWS_MAX; i++) {
		sampling_entry = &dp_stc->sampling_flow_table.entries[i];
		sampling_pending |= wlan_dp_stc_sample_flow(dp_stc,
							    sampling_entry);
	}

	if (sampling_pending)
		qdf_timer_mod(&dp_stc->flow_sampling_timer,
			      DP_STC_TIMER_THRESH_MS);

	return;
}

QDF_STATUS
wlan_dp_stc_handle_flow_stats_policy(enum qca_async_stats_type type,
				     enum qca_async_stats_action action)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_get_context();
	struct wlan_dp_stc *dp_stc = dp_ctx->dp_stc;

	switch (type) {
	case QCA_ASYNC_STATS_TYPE_FLOW_STATS:
		if (action == QCA_ASYNC_STATS_ACTION_START)
			dp_stc->send_flow_stats = 1;
		else if (action == QCA_ASYNC_STATS_ACTION_STOP)
			dp_stc->send_flow_stats = 0;
		else
			return QDF_STATUS_E_INVAL;
		break;
	case QCA_ASYNC_STATS_TYPE_CLASSIFIED_FLOW_STATS:
		if (action == QCA_ASYNC_STATS_ACTION_START)
			dp_stc->send_classified_flow_stats = 1;
		else if (action == QCA_ASYNC_STATS_ACTION_STOP)
			dp_stc->send_classified_flow_stats = 0;
		else
			return QDF_STATUS_E_INVAL;
		break;
	default:
		break;
	}

	return QDF_STATUS_SUCCESS;
}

static inline bool wlan_dp_is_flow_exact_match(struct flow_info *tuple1,
					       struct flow_info *tuple2)
{
	if ((tuple1->src_port ^ tuple2->src_port) ||
	    (tuple1->dst_port ^ tuple2->dst_port) ||
	    (tuple1->proto ^ tuple2->proto))
		return false;

	if ((tuple1->flags & DP_FLOW_TUPLE_FLAGS_IPV4) &&
	    ((tuple1->src_ip.ipv4_addr ^ tuple2->src_ip.ipv4_addr) ||
	     (tuple1->dst_ip.ipv4_addr ^ tuple2->dst_ip.ipv4_addr)))
		return false;

	if ((tuple1->flags & DP_FLOW_TUPLE_FLAGS_IPV6) &&
	    ((tuple1->src_ip.ipv6_addr[0] ^ tuple2->src_ip.ipv6_addr[0]) ||
	     (tuple1->src_ip.ipv6_addr[1] ^ tuple2->src_ip.ipv6_addr[1]) ||
	     (tuple1->src_ip.ipv6_addr[2] ^ tuple2->src_ip.ipv6_addr[2]) ||
	     (tuple1->src_ip.ipv6_addr[3] ^ tuple2->src_ip.ipv6_addr[3]) ||
	     (tuple1->dst_ip.ipv6_addr[0] ^ tuple2->dst_ip.ipv6_addr[0]) ||
	     (tuple1->dst_ip.ipv6_addr[1] ^ tuple2->dst_ip.ipv6_addr[1]) ||
	     (tuple1->dst_ip.ipv6_addr[2] ^ tuple2->dst_ip.ipv6_addr[2]) ||
	     (tuple1->dst_ip.ipv6_addr[3] ^ tuple2->dst_ip.ipv6_addr[3])))
		return false;

	return true;
}

void
wlan_dp_stc_handle_flow_classify_result(struct wlan_dp_stc_flow_classify_result *flow_classify_result)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_get_context();
	struct wlan_dp_stc *dp_stc = dp_ctx->dp_stc;
	struct wlan_dp_stc_sampling_table *sampling_table;
	struct flow_info *flow_tuple = &flow_classify_result->flow_tuple;
	struct wlan_dp_stc_sampling_table_entry *flow;
	uint64_t hash;
	int i;

	hash = wlan_dp_get_flow_hash(dp_ctx, flow_tuple);
	sampling_table = &dp_stc->sampling_flow_table;

	for (i = 0; i < DP_STC_SAMPLE_FLOWS_MAX; i++) {
		flow = &sampling_table->entries[i];
		if (flow->state == WLAN_DP_SAMPLING_STATE_INIT)
			continue;

		/* Neither samples have been reported, so skip this flow */
		if (!(wlan_dp_txrx_samples_reported(flow) ||
		      wlan_dp_burst_samples_reported(flow)))
			continue;

		if (flow->tuple_hash != hash)
			continue;

		if (!wlan_dp_is_flow_exact_match(flow_tuple,
						 &flow->flow_samples.flow_tuple))
			continue;

		/*
		 * Flow is an exact match.
		 * The classification result is for this flow only.
		 */
		/*
		 * 1) Indicate to TX and RX flow
		 * 2) Change state to classified,
		 *	timer will remove it from table
		 * The risk in changing state here is that, if the flow is
		 * classified using txrx samples, then timer may change the
		 * state to BURST_SAMPLING. This is write from 2 contexts.
		 */
		if (wlan_dp_burst_samples_reported(flow))
			wlan_dp_stc_remove_sampling_table_entry(dp_stc, flow);
		flow->state = WLAN_DP_SAMPLING_STATE_CLASSIFIED;
		break;
	}

	/*
	 * Currently there is no case of sampling table entry being removed
	 * without the classification result. So assert if we are unable to
	 * find the flow which has been classified.
	 */
	if (i == DP_STC_SAMPLE_FLOWS_MAX) {
		dp_info("STC: Could not find the classified flow in table");
		qdf_assert_always(0);
	}
}

QDF_STATUS wlan_dp_stc_peer_event_notify(ol_txrx_soc_handle soc,
					 enum cdp_peer_event event,
					 uint16_t peer_id, uint8_t vdev_id,
					 uint8_t *peer_mac_addr)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_get_context();
	struct wlan_dp_stc *dp_stc = dp_ctx->dp_stc;
	struct wlan_dp_stc_peer_traffic_map *active_traffic_map;

	if (peer_id >= DP_STC_MAX_PEERS)
		return QDF_STATUS_E_INVAL;

	active_traffic_map = &dp_stc->peer_traffic_map[peer_id];

	switch (event) {
	case CDP_PEER_EVENT_MAP:
		if (active_traffic_map->valid) {
			dp_info("STC: Peer map notify for active peer");
			qdf_assert_always(0);
			return QDF_STATUS_E_BUSY;
		}

		active_traffic_map->vdev_id = vdev_id;
		active_traffic_map->peer_id = peer_id;
		qdf_mem_copy(active_traffic_map->mac_addr.bytes,
			     peer_mac_addr, QDF_MAC_ADDR_SIZE);
		active_traffic_map->valid = 1;
		break;
	case CDP_PEER_EVENT_UNMAP:
		if (!active_traffic_map->valid) {
			dp_info("STC: Peer unmap notify for inactive peer");
			qdf_assert_always(0);
			return QDF_STATUS_E_BUSY;
		}

		if (qdf_mem_cmp(active_traffic_map->mac_addr.bytes,
				peer_mac_addr, QDF_MAC_ADDR_SIZE) != 0) {
			dp_err("STC: peer unmap notify: mac addr mismatch");
			return QDF_STATUS_E_INVAL;
		}
		active_traffic_map->valid = 0;
		break;
	default:
		break;
	}

	return QDF_STATUS_SUCCESS;
}

static bool
wlan_dp_stc_is_traffic_conext_supported(struct wlan_objmgr_psoc *psoc)
{
	struct wmi_unified *wmi_handle;

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		dp_err("Invalid WMI handle");
		return false;
	}

	return wmi_service_enabled(wmi_handle,
				   wmi_service_traffic_context_support);
}

static bool wlan_dp_stc_clients_available(struct wlan_dp_psoc_context *dp_ctx)
{
	if (wlan_dp_stc_is_traffic_conext_supported(dp_ctx->psoc))
		return true;

	return false;
}

void wlan_dp_stc_cfg_init(struct wlan_dp_psoc_cfg *config,
			  struct wlan_objmgr_psoc *psoc)
{
	config->stc_enable = cfg_get(psoc, CFG_DP_STC_ENABLE);
}

QDF_STATUS wlan_dp_stc_attach(struct wlan_dp_psoc_context *dp_ctx)
{
	struct wlan_dp_stc *dp_stc;
	QDF_STATUS status;

	if (!wlan_dp_cfg_is_stc_enabled(&dp_ctx->dp_cfg)) {
		dp_info("STC: feature not enabled via cfg");
		return QDF_STATUS_SUCCESS;
	}

	if (!wlan_dp_stc_clients_available(dp_ctx)) {
		dp_info("STC: No clients available, skip attach");
		dp_ctx->dp_stc = NULL;
		return QDF_STATUS_SUCCESS;
	}

	dp_info("STC: attach");
	dp_stc = qdf_mem_malloc(sizeof(*dp_stc));
	if (!dp_stc) {
		dp_err("STC context alloc failed");
		return QDF_STATUS_E_NOMEM;
	}

	dp_ctx->dp_stc = dp_stc;
	dp_stc->dp_ctx = dp_ctx;

	/* Init periodic work */
	status = qdf_periodic_work_create(&dp_stc->flow_monitor_work,
					  wlan_dp_stc_flow_monitor_work_handler,
					  dp_stc);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_info("STC flow monitor work creation failed");
		goto periodic_work_creation_fail;
	}

	dp_stc->flow_monitor_interval = 100;
	dp_stc->periodic_work_state = WLAN_DP_STC_WORK_INIT;

	/* Init timer */
	status = qdf_timer_init(dp_ctx->qdf_dev, &dp_stc->flow_sampling_timer,
				wlan_dp_stc_flow_sampling_timer, dp_stc,
				QDF_TIMER_TYPE_SW);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_info("STC flow sampling timer init failed");
		goto timer_init_fail;
	}

	dp_stc->sample_timer_state = WLAN_DP_STC_TIMER_INIT;
	dp_fisa_rx_add_tcp_flow_to_fst(dp_ctx);

	return QDF_STATUS_SUCCESS;

timer_init_fail:
	qdf_periodic_work_destroy(&dp_stc->flow_monitor_work);

periodic_work_creation_fail:
	qdf_mem_free(dp_stc);

	return status;
}

QDF_STATUS wlan_dp_stc_detach(struct wlan_dp_psoc_context *dp_ctx)
{
	struct wlan_dp_stc *dp_stc = dp_ctx->dp_stc;

	if (!dp_stc) {
		dp_info("STC: module not initialized, skip detach");
		return QDF_STATUS_SUCCESS;
	}

	dp_info("STC: detach");
	qdf_timer_sync_cancel(&dp_stc->flow_sampling_timer);
	qdf_periodic_work_destroy(&dp_stc->flow_monitor_work);
	qdf_mem_free(dp_ctx->dp_stc);
	dp_ctx->dp_stc = NULL;

	return QDF_STATUS_SUCCESS;
}
#endif
