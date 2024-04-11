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
  * DOC: wlan_dp_stc.h
  *
  *
  */
#ifndef __WLAN_DP_STC_H__
#define __WLAN_DP_STC_H__

#include "wlan_dp_main.h"

/* TODO - This macro needs to be same as the max peers in CMN DP */
#define DP_STC_MAX_PEERS 64

/**
 * struct wlan_dp_stc_peer_ping_info - Active ping information table with
 *				       per peer records
 * @mac_addr: mac address of the peer
 * @last_ping_ts: timestamp when the last ping packet was traced
 */
struct wlan_dp_stc_peer_ping_info {
	struct qdf_mac_addr mac_addr;
	uint64_t last_ping_ts;
};

/**
 * enum wlan_dp_stc_periodic_work_state - Periodic work states
 * @WLAN_DP_STC_WORK_INIT: INIT state
 * @WLAN_DP_STC_WORK_STOPPED: Stopped state
 * @WLAN_DP_STC_WORK_STARTED: Started state
 * @WLAN_DP_STC_WORK_RUNNING: Running state
 */
enum wlan_dp_stc_periodic_work_state {
	WLAN_DP_STC_WORK_INIT,
	WLAN_DP_STC_WORK_STOPPED,
	WLAN_DP_STC_WORK_STARTED,
	WLAN_DP_STC_WORK_RUNNING,
};

#define DP_STC_SAMPLE_FLOWS_MAX 32
#define DP_STC_TXRX_SAMPLES_MAX 5
#define DP_TXRX_SAMPLES_WINDOW_MAX 2

/*
 * struct wlan_dp_stc_txrx_stats - TxRx stats
 * @bytes: total number of bytes in a window
 * @pkts: total number of pkts in a window
 * @pkt_size_min: minimum packet size in a window
 * @pkt_size_max: maximum packet size in a window
 * @pkt_iat_min: minimum packet inter-arrival time in a window
 * @pkt_iat_max: maximum packet inter-arrival time in a window
 * @pkt_iat_sum: SUM of all the packet inter-arrival time in a window
 */
struct wlan_dp_stc_txrx_stats {
	uint64_t bytes;
	uint32_t pkts;
	uint32_t pkt_size_min;
	uint32_t pkt_size_max;
	uint32_t pkt_iat_min;
	uint32_t pkt_iat_max;
	uint32_t pkt_iat_sum;
};

/*
 * struct wlan_dp_stc_txrx_samples - TxRx samples
 * @tx: uplink/Tx samples
 * @rx: downlink/Rx samples
 */
struct wlan_dp_stc_txrx_samples {
	struct wlan_dp_stc_txrx_stats tx;
	struct wlan_dp_stc_txrx_stats rx;
};

/*
 * struct wlan_dp_stc_burst_stats - Burst stats
 * @burst_duration_min: minimum burst duration in a window
 * @burst_duration_max: maximum burst duration in a window
 * @burst_duration_sum: SUM of all the burst duration in a window
 * @burst_size_min: minimum burst size in a window
 * @burst_size_max: maximum burst size in a window
 * @burst_size_sum: SUM of all the burst size in a window
 * @burst_count: Total number of bursts
 */
struct wlan_dp_stc_burst_stats {
	uint32_t burst_duration_min;
	uint32_t burst_duration_max;
	uint32_t burst_duration_sum;
	uint32_t burst_size_min;
	uint32_t burst_size_max;
	uint64_t burst_size_sum;
	uint32_t burst_count;
};

/*
 * struct wlan_dp_stc_burst_samples - Burst samples
 * @tx: Uplink/Tx burst samples
 * @rx: downlink/Rx burst samples
 */
struct wlan_dp_stc_burst_samples {
	struct wlan_dp_stc_burst_stats tx;
	struct wlan_dp_stc_burst_stats rx;
};

/*
 * struct wlan_dp_stc_sampling_table_entry - Sampling table entry
 * @state: State of sampling for this flow
 * @flags: flags
 * @flow_tuple: flow tuple of this flow
 * @txrx_samples: TxRx samples
 * @burst_sample: Burst samples
 */
struct wlan_dp_stc_sampling_table_entry {
	uint32_t state;
	uint32_t flags;
	struct flow_info flow_tuple;
	struct wlan_dp_stc_txrx_samples txrx_samples[DP_STC_TXRX_SAMPLES_MAX][DP_TXRX_SAMPLES_WINDOW_MAX];
	struct wlan_dp_stc_burst_samples burst_sample;
};

/*
 * struct wlan_dp_stc_sampling_table - Sampling table
 * @entries: records added to sampling table
 */
struct wlan_dp_stc_sampling_table {
	struct wlan_dp_stc_sampling_table_entry entries[DP_STC_SAMPLE_FLOWS_MAX];
};

/**
 * struct wlan_dp_stc - Smart traffic classifier context
 * @dp_ctx: DP component global context
 * @flow_monitor_work: periodic work to process all the misc work for STC
 * @flow_monitor_interval: periodic flow monitor work interval
 * @periodic_work_state: States of the periodic flow monitor work
 * @flow_sampling_timer: timer to sample all the short-listed flows
 * @peer_ping_info: Ping tracking per peer
 * @sampling_flow_table: Sampling flow table
 */
struct wlan_dp_stc {
	struct wlan_dp_psoc_context *dp_ctx;
	struct qdf_periodic_work flow_monitor_work;
	uint32_t flow_monitor_interval;
	enum wlan_dp_stc_periodic_work_state periodic_work_state;
	qdf_timer_t flow_sampling_timer;
	struct wlan_dp_stc_peer_ping_info peer_ping_info[DP_STC_MAX_PEERS];
	struct wlan_dp_stc_sampling_table sampling_flow_table;
};

/* Function Declaration - START */

#ifdef WLAN_DP_FEATURE_STC
/**
 * dp_stc_get_timestamp() - Get current timestamp to be used in STC
 *			    critical per packet path and its related calculation
 *
 * Return: current timestamp in ns
 */
static inline uint64_t dp_stc_get_timestamp(void)
{
	return qdf_sched_clock();
}

/**
 * wlan_dp_stc_mark_ping_ts() - Mark the last ping timestamp in STC table
 * @dp_ctx: DP global psoc context
 * @peer_mac_addr: peer mac address
 * @peer_id: peer id
 *
 * Return: void
 */
static inline void
wlan_dp_stc_mark_ping_ts(struct wlan_dp_psoc_context *dp_ctx,
			 struct qdf_mac_addr *peer_mac_addr, uint16_t peer_id)
{
	struct wlan_dp_stc *dp_stc = dp_ctx->dp_stc;
	bool send_fw_indication = false;

	if (!qdf_is_macaddr_equal(&dp_stc->peer_ping_info[peer_id].mac_addr,
				  peer_mac_addr)) {
		qdf_copy_macaddr(&dp_stc->peer_ping_info[peer_id].mac_addr,
				 peer_mac_addr);
		send_fw_indication = true;
	}

	if (dp_stc->peer_ping_info[peer_id].last_ping_ts == 0)
		send_fw_indication = true;

	dp_stc->peer_ping_info[peer_id].last_ping_ts = dp_stc_get_timestamp();

	if (send_fw_indication) {
		/* TODO - Send indication to work, for sending fw_indication */
	}
}

/**
 * wlan_dp_stc_attach() - STC attach
 * @dp_ctx: DP global psoc context
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_dp_stc_attach(struct wlan_dp_psoc_context *dp_ctx);

/**
 * wlan_dp_stc_detach() - STC detach
 * @dp_ctx: DP global psoc context
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_dp_stc_detach(struct wlan_dp_psoc_context *dp_ctx);
#else
static inline void
wlan_dp_stc_mark_ping_ts(struct wlan_dp_psoc_context *dp_ctx,
			 struct qdf_mac_addr *peer_mac_addr, uint16_t peer_id)
{
}

static inline
QDF_STATUS wlan_dp_stc_attach(struct wlan_dp_psoc_context *dp_ctx)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS wlan_dp_stc_detach(struct wlan_dp_psoc_context *dp_ctx)
{
	return QDF_STATUS_SUCCESS;
}
#endif
/* Function Declaration - END */
#endif
