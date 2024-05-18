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

#define DP_STC_UPDATE_MIN_MAX_STATS(__field, __val)			\
	do {								\
		if (__field##_min == 0 || __field##_min > __val)	\
			__field##_min = __val;				\
		if (__field##_max < __val)				\
			__field##_max = __val;				\
	} while (0)

#define DP_STC_UPDATE_MIN_MAX_SUM_STATS(__field, __val)			\
	do {								\
		__field##_sum += __val;					\
		if (__field##_min == 0 || __field##_min > __val)	\
			__field##_min = __val;				\
		if (__field##_max < __val)				\
			__field##_max = __val;				\
	} while (0)

#define DP_STC_UPDATE_WIN_MIN_MAX_STATS(__field, __val)			\
	do {								\
		if (__field##_min == 0 || __field##_min > __val)	\
			__field##_min = __val;				\
		if (__field##_max < __val)				\
			__field##_max = __val;				\
	} while (0)

/* TODO - This macro needs to be same as the max peers in CMN DP */
#define DP_STC_MAX_PEERS 64

#define BURST_START_TIME_THRESHOLD_NS 10000000
#define BURST_START_BYTES_THRESHOLD 3000
#define BURST_END_TIME_THRESHOLD_NS 300000000

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

/**
 * enum wlan_dp_stc_burst_state - Burst detection state
 * @BURST_DETECTION_INIT: Burst detection not started
 * @BURST_DETECTION_START: Burst detection has started
 * @BURST_DETECTION_BURST_START: Burst start is detected and burst has started
 */
enum wlan_dp_stc_burst_state {
	BURST_DETECTION_INIT,
	BURST_DETECTION_START,
	BURST_DETECTION_BURST_START,
};

#define DP_STC_SAMPLE_FLOWS_MAX 32
#define DP_STC_SAMPLE_BIDI_FLOW_MAX 16
#define DP_STC_SAMPLE_RX_FLOW_MAX 8
#define DP_STC_SAMPLE_TX_FLOW_MAX 8

#define DP_STC_LONG_WINDOW_MS 30000
#define DP_STC_TIMER_THRESH_MS 600

/**
 * enum wlan_stc_sampling_state - Sampling state
 * @WLAN_DP_SAMPLING_STATE_INIT: init state
 * @WLAN_DP_SAMPLING_STATE_FLOW_ADDED: flow added for sampling
 * @WLAN_DP_SAMPLING_STATE_SAMPLING_START: sampling started
 * @WLAN_DP_SAMPLING_STATE_SAMPLING_BURST_STATS: sampling burst stats
 * @WLAN_DP_SAMPLING_STATE_SAMPLING_DONE: sampling completed
 * @WLAN_DP_SAMPLING_STATE_SAMPLES_SENT: samples sent
 * @WLAN_DP_SAMPLING_STATE_CLASSIFIED: flow classified
 */
enum wlan_stc_sampling_state {
	WLAN_DP_SAMPLING_STATE_INIT,
	WLAN_DP_SAMPLING_STATE_FLOW_ADDED,
	WLAN_DP_SAMPLING_STATE_SAMPLING_START,
	WLAN_DP_SAMPLING_STATE_SAMPLING_BURST_STATS,
	WLAN_DP_SAMPLING_STATE_SAMPLING_DONE,
	WLAN_DP_SAMPLING_STATE_SAMPLES_SENT,
	WLAN_DP_SAMPLING_STATE_CLASSIFIED,
};

/* Bit-fields used for "flags" in struct wlan_dp_stc_sampling_candidate */
#define WLAN_DP_SAMPLING_CANDIDATE_VALID BIT(0)
#define WLAN_DP_SAMPLING_CANDIDATE_TX_FLOW_VALID BIT(1)
#define WLAN_DP_SAMPLING_CANDIDATE_RX_FLOW_VALID BIT(2)

enum wlan_dp_flow_dir {
	WLAN_DP_FLOW_DIR_INVALID,
	WLAN_DP_FLOW_DIR_TX,
	WLAN_DP_FLOW_DIR_RX,
	WLAN_DP_FLOW_DIR_BIDI,
};

/**
 * struct wlan_dp_stc_sampling_candidate - Sampling candidate
 * @peer_id: Peer ID
 * @flags: flags
 * @tx_flow_id: TX flow ID
 * @tx_flow_metadata: TX flow metadata
 * @rx_flow_id: RX flow ID
 * @rx_flow_metadata: RX flow metadata
 * @flow_tuple: flow tuple
 * @dir: flow direction
 */
struct wlan_dp_stc_sampling_candidate {
	uint16_t peer_id;
	uint32_t flags;
	uint32_t tx_flow_id;
	uint32_t tx_flow_metadata;
	uint32_t rx_flow_id;
	uint32_t rx_flow_metadata;
	struct flow_info flow_tuple;
	enum wlan_dp_flow_dir dir;
};

/* bit-fields used for "flags" in struct wlan_dp_stc_sampling_table_entry */
#define WLAN_DP_SAMPLING_FLAGS_TX_FLOW_VALID BIT(0)
#define WLAN_DP_SAMPLING_FLAGS_RX_FLOW_VALID BIT(1)
#define WLAN_DP_SAMPLING_FLAGS_TXRX_SAMPLES_READY BIT(2)
#define WLAN_DP_SAMPLING_FLAGS_BURST_SAMPLES_READY BIT(3)

#define WLAN_DP_SAMPLING_FLAGS1_TXRX_SAMPLES_SENT BIT(0)
#define WLAN_DP_SAMPLING_FLAGS1_BURST_SAMPLES_SENT BIT(1)

/*
 * struct wlan_dp_stc_sampling_table_entry - Sampling table entry
 * @state: State of sampling for this flow
 * @flags: flags set by timer
 * @flags1: flags set by periodic work
 * @next_sample_idx: next sample index to fill min/max stats in per-packet path
 * @next_win_idx: next window index to fill min/max stats in per-packet path
 * @max_num_sample_attempts: max number of sampling_timer runs to collect
 *			     txrx and burst state
 * @tx_flow_id: tx flow ID
 * @tx_flow_metadata: tx flow metadata
 * @rx_flow_id: rx flow ID
 * @rx_flow_metadata: rx flow metadata
 * @tx_stats_ref: tx window stats reference
 * @rx_stats_ref: rx window stats reference
 * @flow_samples: flow samples
 */
struct wlan_dp_stc_sampling_table_entry {
	enum wlan_stc_sampling_state state;
	uint32_t flags;
	uint32_t flags1;
	uint8_t next_sample_idx;
	uint8_t next_win_idx;
	uint8_t max_num_sample_attempts;
	uint32_t tx_flow_id;
	uint32_t tx_flow_metadata;
	uint32_t rx_flow_id;
	uint32_t rx_flow_metadata;
	uint64_t tuple_hash;
	struct flow_info flow_tuple;
	struct wlan_dp_stc_txrx_stats tx_stats_ref;
	struct wlan_dp_stc_txrx_stats rx_stats_ref;
	struct wlan_dp_stc_flow_samples flow_samples;
};

/*
 * struct wlan_dp_stc_sampling_table - Sampling table
 * @num_valid_entries: Number of valid flows added to sampling flow table
 * @num_bidi_flows: number of Bi-Di flows added to sampling flow table
 * @num_tx_only_flows: number of TX only flows added to sampling flow table
 * @num_rx_only_flows: number of RX only flows added to sampling flow table
 * @entries: records added to sampling table
 */
struct wlan_dp_stc_sampling_table {
	qdf_atomic_t num_valid_entries;
	qdf_atomic_t num_bidi_flows;
	qdf_atomic_t num_tx_only_flows;
	qdf_atomic_t num_rx_only_flows;
	struct wlan_dp_stc_sampling_table_entry entries[DP_STC_SAMPLE_FLOWS_MAX];
};

/**
 * struct wlan_dp_stc_flow_table_entry - Flow table maintained in per pkt path
 * @prev_pkt_arrival_ts: previous packet arrival time
 * @win_transition_complete: Window transition completion flag.
 * @metadata: flow metadata
 * @burst_state: burst state
 * @burst_start_time: burst start time
 * @burst_start_detect_bytes: current snapshot of total bytes during burst
 *			      start detection phase
 * @cur_burst_bytes: total bytes accumulated in current burst
 * @idx: union for storing sample & window index
 * @idx.sample_win_idx: sample and window index
 * @idx.s.win_idx: window index
 * @idx.s.sample_idx: sample index
 * @txrx_stats: txrx stats
 * @txrx_min_max_stats: placeholder for stats which needs per window
 *			min/max values
 * @burst_stats: burst stats
 */
struct wlan_dp_stc_flow_table_entry {
	uint64_t prev_pkt_arrival_ts;
	uint8_t win_transition_complete;
	uint32_t metadata;
	enum wlan_dp_stc_burst_state burst_state;
	uint64_t burst_start_time;
	uint32_t burst_start_detect_bytes;
	uint32_t cur_burst_bytes;
	/* Can be atomic.! Decide based on the accuracy during test */
	union {
		uint32_t sample_win_idx;
		struct {
			uint32_t win_idx:16,
				 sample_idx:16;
		} s;
	} idx;
	struct wlan_dp_stc_txrx_stats txrx_stats;
	struct wlan_dp_stc_txrx_min_max_stats txrx_min_max_stats[DP_STC_TXRX_SAMPLES_MAX][DP_TXRX_SAMPLES_WINDOW_MAX];
	struct wlan_dp_stc_burst_stats burst_stats;
};

#define DP_STC_FLOW_TABLE_ENTRIES_MAX 256
/**
 * struct wlan_dp_stc_rx_flow_table - RX flow table
 * @entries: RX flow table records
 */
struct wlan_dp_stc_rx_flow_table {
	struct wlan_dp_stc_flow_table_entry entries[DP_STC_FLOW_TABLE_ENTRIES_MAX];
};

/**
 * struct wlan_dp_stc_tx_flow_table - TX flow table
 * @entries: TX flow table records
 */
struct wlan_dp_stc_tx_flow_table {
	struct wlan_dp_stc_flow_table_entry entries[DP_STC_FLOW_TABLE_ENTRIES_MAX];
};

/**
 * enum wlan_dp_stc_timer_state - Sampling timer state
 * @WLAN_DP_STC_TIMER_INIT: Init state
 * @WLAN_DP_STC_TIMER_STOPPED: timer stopped state
 * @WLAN_DP_STC_TIMER_STARTED: timer started state
 * @WLAN_DP_STC_TIMER_RUNNING: timer running state
 */
enum wlan_dp_stc_timer_state {
	WLAN_DP_STC_TIMER_INIT,
	WLAN_DP_STC_TIMER_STOPPED,
	WLAN_DP_STC_TIMER_STARTED,
	WLAN_DP_STC_TIMER_RUNNING,
};

/**
 * struct wlan_dp_stc - Smart traffic classifier context
 * @dp_ctx: DP component global context
 * @send_flow_stats: Flag to indicate whether flow stats are to be reported
 * @send_classified_flow_stats: Flag to indicate whether the classified
 *				flow stats are to be reported
 * @flow_monitor_work: periodic work to process all the misc work for STC
 * @flow_monitor_interval: periodic flow monitor work interval
 * @periodic_work_state: States of the periodic flow monitor work
 * @flow_sampling_timer: timer to sample all the short-listed flows
 * @sample_timer_state: sampling timer state
 * @peer_ping_info: Ping tracking per peer
 * @sampling_flow_table: Sampling flow table
 * @rx_flow_table: RX flow table
 * @tx_flow_table: TX flow table
 */
struct wlan_dp_stc {
	struct wlan_dp_psoc_context *dp_ctx;
	uint8_t send_flow_stats;
	uint8_t send_classified_flow_stats;
	struct qdf_periodic_work flow_monitor_work;
	uint32_t flow_monitor_interval;
	enum wlan_dp_stc_periodic_work_state periodic_work_state;
	qdf_timer_t flow_sampling_timer;
	enum wlan_dp_stc_timer_state sample_timer_state;
	struct wlan_dp_stc_peer_ping_info peer_ping_info[DP_STC_MAX_PEERS];
	struct wlan_dp_stc_sampling_table sampling_flow_table;
	struct wlan_dp_stc_rx_flow_table rx_flow_table;
	struct wlan_dp_stc_tx_flow_table tx_flow_table;
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

static inline uint8_t wlan_dp_ip_proto_to_stc_proto(uint16_t ip_proto)
{
	switch (ip_proto) {
	case IPPROTO_TCP:
		return QCA_WLAN_VENDOR_FLOW_POLICY_PROTO_TCP;
	case IPPROTO_UDP:
		return QCA_WLAN_VENDOR_FLOW_POLICY_PROTO_UDP;
	default:
		break;
	}

	return -EINVAL;
}

/**
 * wlan_dp_stc_populate_flow_tuple() - Populate the STC flow tuple using the
 *				       flow tuple in active flow table
 * @flow_tuple: STC flow tuple
 * @flow_tuple_info: flow tuple info
 *
 * Return: None
 */
static inline void
wlan_dp_stc_populate_flow_tuple(struct flow_info *flow_tuple,
				struct cdp_rx_flow_tuple_info *flow_tuple_info)
{
	uint8_t is_ipv4_flow = 1;	//TODO - Get from fisa flow table entry
	uint8_t is_ipv6_flow = 0;
	uint8_t proto = flow_tuple_info->l4_protocol;

	if (is_ipv4_flow) {
		flow_tuple->src_ip.ipv4_addr = flow_tuple_info->src_ip_31_0;
		flow_tuple->dst_ip.ipv4_addr = flow_tuple_info->dest_ip_31_0;
	} else if (is_ipv6_flow) {
		flow_tuple->src_ip.ipv6_addr[0] = flow_tuple_info->src_ip_31_0;
		flow_tuple->src_ip.ipv6_addr[1] = flow_tuple_info->src_ip_63_32;
		flow_tuple->src_ip.ipv6_addr[2] = flow_tuple_info->src_ip_95_64;
		flow_tuple->src_ip.ipv6_addr[3] =
						flow_tuple_info->src_ip_127_96;

		flow_tuple->dst_ip.ipv6_addr[0] = flow_tuple_info->dest_ip_31_0;
		flow_tuple->dst_ip.ipv6_addr[1] =
						flow_tuple_info->dest_ip_63_32;
		flow_tuple->dst_ip.ipv6_addr[2] =
						flow_tuple_info->dest_ip_95_64;
		flow_tuple->dst_ip.ipv6_addr[3] =
						flow_tuple_info->dest_ip_127_96;
	}
	flow_tuple->src_port = flow_tuple_info->src_port;
	flow_tuple->dst_port = flow_tuple_info->dest_port;
	flow_tuple->proto = wlan_dp_ip_proto_to_stc_proto(proto);
	flow_tuple->flags = 0;
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
 * wlan_dp_indicate_rx_flow_add() - Indication to STC when rx flow is added
 * @dp_ctx: Global DP psoc context
 *
 * Return: None
 */
static inline void
wlan_dp_indicate_rx_flow_add(struct wlan_dp_psoc_context *dp_ctx)
{
	struct wlan_dp_stc *dp_stc = dp_ctx->dp_stc;

	/* RCU or atomic variable?? */
	if (dp_stc->periodic_work_state < WLAN_DP_STC_WORK_STARTED) {
		qdf_periodic_work_start(&dp_stc->flow_monitor_work,
					dp_stc->flow_monitor_interval);
		dp_stc->periodic_work_state = WLAN_DP_STC_WORK_STARTED;
	}
}

/**
 * wlan_dp_stc_track_flow_features() - Track flow features
 * @dp_stc: STC context
 * @nbuf: packet handle
 * @flow_entry: flow entry (to which the current pkt belongs)
 * @vdev_id: ID of vdev to which the packet belongs
 * @peer_id: ID of peer to which the packet belongs
 * @metadata: RX flow metadata
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
wlan_dp_stc_track_flow_features(struct wlan_dp_stc *dp_stc, qdf_nbuf_t nbuf,
				struct wlan_dp_stc_flow_table_entry *flow_entry,
				uint8_t vdev_id, uint16_t peer_id,
				uint32_t metadata);

static inline QDF_STATUS
wlan_dp_stc_check_n_track_rx_flow_features(struct wlan_dp_psoc_context *dp_ctx,
					   qdf_nbuf_t nbuf)
{
	struct wlan_dp_stc_flow_table_entry *flow_entry;
	struct wlan_dp_stc *dp_stc;
	uint8_t vdev_id;
	uint16_t peer_id;
	uint16_t flow_id;
	uint32_t metadata;

	if (qdf_likely(!QDF_NBUF_CB_RX_TRACK_FLOW(nbuf)))
		return QDF_STATUS_SUCCESS;

	dp_stc = dp_ctx->dp_stc;
	vdev_id = QDF_NBUF_CB_RX_VDEV_ID(nbuf);
	peer_id = QDF_NBUF_CB_RX_PEER_ID(nbuf);
	flow_id = QDF_NBUF_CB_EXT_RX_FLOW_ID(nbuf);
	metadata = QDF_NBUF_CB_RX_FLOW_METADATA(nbuf);

	flow_entry = &dp_stc->rx_flow_table.entries[flow_id];

	return wlan_dp_stc_track_flow_features(dp_stc, nbuf, flow_entry,
					       vdev_id, peer_id, metadata);
}

/**
 * wlan_dp_stc_check_n_track_tx_flow_features() - Update the stats if flow is
 *						  tracking enabled
 * @dp_ctx: DP global psoc context
 * @nbuf: Packet nbuf
 * @flow_track_enabled: Flow tracking enabled
 * @flow_id: Flow id of flow
 * @vdev_id: Vdev of the flow
 * @peer_id: Peer_id of the flow
 * @metadata: Metadata of the flow
 *
 * Return: QDF_STATUS
 */
static inline QDF_STATUS
wlan_dp_stc_check_n_track_tx_flow_features(struct wlan_dp_psoc_context *dp_ctx,
					   qdf_nbuf_t nbuf,
					   uint32_t flow_track_enabled,
					   uint16_t flow_id, uint8_t vdev_id,
					   uint16_t peer_id, uint32_t metadata)
{
	struct wlan_dp_stc *dp_stc = dp_ctx->dp_stc;
	struct wlan_dp_stc_flow_table_entry *flow_entry;

	if (qdf_likely(!flow_track_enabled))
		return QDF_STATUS_SUCCESS;

	flow_entry = &dp_stc->tx_flow_table.entries[flow_id];

	return wlan_dp_stc_track_flow_features(dp_stc, nbuf, flow_entry,
					       vdev_id, peer_id, metadata);
}

QDF_STATUS
wlan_dp_stc_handle_flow_stats_policy(enum qca_async_stats_type type,
				     enum qca_async_stats_action);
/**
 * wlan_dp_stc_handle_flow_classify_result() - Handle flow classify result
 * @flow_classify_result:
 *
 * Return: none
 */
void
wlan_dp_stc_handle_flow_classify_result(struct wlan_dp_stc_flow_classify_result *flow_classify_result);
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
wlan_dp_stc_populate_flow_tuple(struct flow_info *flow_tuple,
				struct cdp_rx_flow_tuple_info *flow_tuple_info)
{
}

static inline void
wlan_dp_indicate_rx_flow_add(struct wlan_dp_psoc_context *dp_ctx)
{
}

static inline void
wlan_dp_stc_mark_ping_ts(struct wlan_dp_psoc_context *dp_ctx,
			 struct qdf_mac_addr *peer_mac_addr, uint16_t peer_id)
{
}

static inline QDF_STATUS
wlan_dp_stc_check_n_track_rx_flow_features(struct wlan_dp_psoc_context *dp_ctx,
					   qdf_nbuf_t nbuf)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS
wlan_dp_stc_check_n_track_tx_flow_features(struct wlan_dp_psoc_context *dp_ctx,
					   qdf_nbuf_t nbuf,
					   uint32_t flow_track_enabled,
					   uint16_t flow_id, uint8_t vdev_id,
					   uint16_t peer_id, uint32_t metadata)
{
	return QDF_STATUS_SUCCESS;
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
