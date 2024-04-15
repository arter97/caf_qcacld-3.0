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

static void wlan_dp_stc_flow_sampling_timer(void *arg)
{
	/*
	 * 1) Walk over the Sampling flow table
	 * 2) Get stats for the flows in Sampling flow table
	 * 3) Get delta stats against the last cached value.
	 * 4) Store the sample in Sampling flow table, against the flow entry
	 * 5) Mark Sampling complete
	 */
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
