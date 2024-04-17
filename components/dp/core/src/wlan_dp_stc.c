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

static void wlan_dp_stc_flow_monitor_work_handler(void *arg)
{
	/*
	 * 1) Monitor TX/RX flows
	 * 2) Add long flow to Sampling flow table
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

	burst_sample = &sampling_entry->flow_samples.burst_sample;
	if (!(sampling_entry->flags & WLAN_DP_SAMPLING_FLAGS_TX_FLOW_VALID))
		goto save_rx_flow_samples;

	flow = &dp_stc->tx_flow_table.entries[sampling_entry->tx_flow_id];
	qdf_mem_copy(&burst_sample->txrx_samples.tx, &flow->txrx_stats,
		     sizeof(burst_sample->txrx_samples.tx));
	qdf_mem_copy(&burst_sample->tx, &flow->burst_stats,
		     sizeof(burst_sample->tx));

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
		/* The current burst has not yet completed */
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

QDF_STATUS wlan_dp_stc_attach(struct wlan_dp_psoc_context *dp_ctx)
{
	struct wlan_dp_stc *dp_stc;
	QDF_STATUS status;

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

	dp_info("STC: detach");
	qdf_timer_sync_cancel(&dp_stc->flow_sampling_timer);
	qdf_periodic_work_destroy(&dp_stc->flow_monitor_work);
	qdf_mem_free(dp_ctx->dp_stc);

	return QDF_STATUS_SUCCESS;
}
#endif
