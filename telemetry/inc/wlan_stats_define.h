/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
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

#ifndef _WLAN_STATS_DEFINE_H_
#define _WLAN_STATS_DEFINE_H_

/* Flags for Feature specific stats */
#define STATS_FEAT_FLG_RX              0x00000001
#define STATS_FEAT_FLG_TX              0x00000002
#define STATS_FEAT_FLG_AST             0x00000004
#define STATS_FEAT_FLG_CFR             0x00000008
#define STATS_FEAT_FLG_FWD             0x00000010
#define STATS_FEAT_FLG_HTT             0x00000020
#define STATS_FEAT_FLG_RAW             0x00000040
#define STATS_FEAT_FLG_RDK             0x00000080
#define STATS_FEAT_FLG_TSO             0x00000100
#define STATS_FEAT_FLG_TWT             0x00000200
#define STATS_FEAT_FLG_VOW             0x00000400
#define STATS_FEAT_FLG_WDI             0x00000800
#define STATS_FEAT_FLG_WMI             0x00001000
#define STATS_FEAT_FLG_IGMP            0x00002000
#define STATS_FEAT_FLG_LINK            0x00004000
#define STATS_FEAT_FLG_MESH            0x00008000
#define STATS_FEAT_FLG_RATE            0x00010000
#define STATS_FEAT_FLG_DELAY           0x00020000
#define STATS_FEAT_FLG_ME              0x00040000
#define STATS_FEAT_FLG_NAWDS           0x00080000
#define STATS_FEAT_FLG_TXCAP           0x00100000
#define STATS_FEAT_FLG_MONITOR         0x00200000
/* Add new feature flag above and update STATS_FEAT_FLG_ALL */
#define STATS_FEAT_FLG_ALL             \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX | STATS_FEAT_FLG_AST | \
	 STATS_FEAT_FLG_CFR | STATS_FEAT_FLG_FWD | STATS_FEAT_FLG_HTT | \
	 STATS_FEAT_FLG_RAW | STATS_FEAT_FLG_RDK | STATS_FEAT_FLG_TSO | \
	 STATS_FEAT_FLG_TWT | STATS_FEAT_FLG_VOW | STATS_FEAT_FLG_WDI | \
	 STATS_FEAT_FLG_WMI | STATS_FEAT_FLG_IGMP | STATS_FEAT_FLG_LINK | \
	 STATS_FEAT_FLG_MESH | STATS_FEAT_FLG_RATE | STATS_FEAT_FLG_DELAY | \
	 STATS_FEAT_FLG_ME | STATS_FEAT_FLG_NAWDS | STATS_FEAT_FLG_TXCAP | \
	 STATS_FEAT_FLG_MONITOR)

#define STATS_BASIC_AP_CTRL_MASK       0
#define STATS_BASIC_AP_DATA_MASK       (STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX)
#define STATS_BASIC_RADIO_CTRL_MASK                    \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_LINK)
#define STATS_BASIC_RADIO_DATA_MASK    (STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX)
#define STATS_BASIC_VAP_CTRL_MASK      (STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX)
#define STATS_BASIC_VAP_DATA_MASK      (STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX)
#define STATS_BASIC_STA_CTRL_MASK                      \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_LINK | STATS_FEAT_FLG_RATE)
#define STATS_BASIC_STA_DATA_MASK                      \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_LINK | STATS_FEAT_FLG_RATE)

#if WLAN_ADVANCE_TELEMETRY
#define STATS_ADVANCE_AP_CTRL_MASK     STATS_BASIC_AP_CTRL_MASK
#define STATS_ADVANCE_AP_DATA_MASK     STATS_BASIC_AP_DATA_MASK
#define STATS_ADVANCE_RADIO_CTRL_MASK  STATS_BASIC_RADIO_CTRL_MASK
#define STATS_ADVANCE_RADIO_DATA_MASK                  \
	(STATS_BASIC_RADIO_DATA_MASK |                 \
	 STATS_FEAT_FLG_ME | STATS_FEAT_FLG_RAW |      \
	 STATS_FEAT_FLG_TSO | STATS_FEAT_FLG_IGMP |    \
	 STATS_FEAT_FLG_MESH | STATS_FEAT_FLG_NAWDS)
#define STATS_ADVANCE_VAP_CTRL_MASK    STATS_BASIC_VAP_CTRL_MASK
#define STATS_ADVANCE_VAP_DATA_MASK                    \
	(STATS_BASIC_VAP_DATA_MASK |                   \
	 STATS_FEAT_FLG_ME | STATS_FEAT_FLG_RAW |      \
	 STATS_FEAT_FLG_TSO | STATS_FEAT_FLG_IGMP |    \
	 STATS_FEAT_FLG_MESH | STATS_FEAT_FLG_NAWDS)
#define STATS_ADVANCE_STA_CTRL_MASK                    \
	(STATS_BASIC_STA_CTRL_MASK | STATS_FEAT_FLG_TWT)
#define STATS_ADVANCE_STA_DATA_MASK                    \
	(STATS_BASIC_STA_DATA_MASK |                   \
	 STATS_FEAT_FLG_FWD | STATS_FEAT_FLG_TWT |     \
	 STATS_FEAT_FLG_RAW | STATS_FEAT_FLG_RDK |     \
	 STATS_FEAT_FLG_NAWDS | STATS_FEAT_FLG_DELAY)
#endif /* WLAN_ADVANCE_TELEMETRY */

#if WLAN_DEBUG_TELEMETRY
#define STATS_DEBUG_AP_CTRL_MASK                       \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_LINK | STATS_FEAT_FLG_RATE)
#define STATS_DEBUG_AP_DATA_MASK                       \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_LINK | STATS_FEAT_FLG_RATE)
#define STATS_DEBUG_RADIO_CTRL_MASK                    \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_LINK)
#define STATS_DEBUG_RADIO_DATA_MASK    (STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX)
#define STATS_DEBUG_VAP_CTRL_MASK      (STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX)
#define STATS_DEBUG_VAP_DATA_MASK      (STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX)
#define STATS_DEBUG_STA_CTRL_MASK                      \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_LINK | STATS_FEAT_FLG_RATE)
#define STATS_DEBUG_STA_DATA_MASK                      \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_LINK | STATS_FEAT_FLG_RATE)
#endif /* WLAN_DEBUG_TELEMETRY */

/**
 * enum stats_level_e: Defines detailing levels
 * @STATS_LVL_BASIC:    Very minimal stats data
 * @STATS_LVL_ADVANCE:  Mostly feature specific stats data
 * @STATS_LVL_DEBUG:    Stats data for debug purpose
 * @STATS_LVL_MAX:      Max supported Stats levels
 */
enum stats_level_e {
	STATS_LVL_BASIC,
	STATS_LVL_ADVANCE,
	STATS_LVL_DEBUG,
	STATS_LVL_MAX = STATS_LVL_DEBUG,
};

/**
 * enum stats_object_e: Defines the Stats specific to object
 * @STATS_OBJ_STA:   Stats for station/peer associated to AP
 * @STATS_OBJ_VAP:   Stats for VAP
 * @STATS_OBJ_RADIO: Stats for particular Radio
 * @STATS_OBJ_AP:    Stats for SoC
 * @STATS_OBJ_MAX:   Max supported objects
 */
enum stats_object_e {
	STATS_OBJ_STA,
	STATS_OBJ_VAP,
	STATS_OBJ_RADIO,
	STATS_OBJ_AP,
	STATS_OBJ_MAX = STATS_OBJ_AP,
};

/**
 * enum stats_type_e: Defines the Stats for specific category
 * @STATS_TYPE_DATA: Stats for Data frames
 * @STATS_TYPE_CTRL: Stats for Control/Management frames
 * @STATS_TYPE_MAX:  Max supported types
 */
enum stats_type_e {
	STATS_TYPE_DATA,
	STATS_TYPE_CTRL,
	STATS_TYPE_MAX = STATS_TYPE_CTRL,
};

/**
 * struct pkt_info: Structure to hold packet info
 * @num:  Number packets
 * @bytes:  NUmber of bytes
 */
struct pkt_info {
	u_int64_t num;
	u_int64_t bytes;
};

/* Basic Peer Data */
struct basic_peer_data_tx {
	struct pkt_info tx_success;
	struct pkt_info comp_pkt;
	u_int64_t dropped_count;
	u_int64_t tx_failed;
};

struct basic_peer_data_rx {
	struct pkt_info to_stack;
	struct pkt_info total_rcvd;
	u_int64_t rx_error_count;
};

struct basic_peer_data_rate {
	u_int32_t rx_rate;
	u_int32_t last_rx_rate;
	u_int32_t tx_rate;
	u_int32_t last_tx_rate;
};

struct basic_peer_data_link {
	u_int32_t avg_snr;
	u_int8_t snr;
	u_int8_t last_snr;
};

/* Basic Peer Ctrl */
struct basic_peer_ctrl_tx {
	u_int32_t cs_tx_mgmt;
	u_int32_t cs_is_tx_not_ok;
};

struct basic_peer_ctrl_rx {
	u_int32_t cs_rx_mgmt;
	u_int32_t cs_rx_decryptcrc;
	u_int32_t cs_rx_security_failure;
};

struct basic_peer_ctrl_rate {
	u_int32_t cs_rx_mgmt_rate;
};

struct basic_peer_ctrl_link {
	int8_t cs_rx_mgmt_snr;
};

/* Basic vdev Data */
struct basic_vdev_data_tx {
	struct pkt_info ingress;
	struct pkt_info processed;
	struct pkt_info dropped;
	struct pkt_info tx_success;
	struct pkt_info comp_pkt;
	u_int64_t dropped_count;
	u_int64_t tx_failed;
};

struct basic_vdev_data_rx {
	struct pkt_info to_stack;
	struct pkt_info total_rcvd;
	u_int64_t rx_error_count;
};

/* Basic vdev Ctrl */
struct basic_vdev_ctrl_tx {
	u_int64_t cs_tx_mgmt;
	u_int64_t cs_tx_error_counter;
	u_int64_t cs_tx_discard;
};

struct basic_vdev_ctrl_rx {
	u_int64_t cs_rx_mgmt;
	u_int64_t cs_rx_error_counter;
	u_int64_t cs_rx_mgmt_discard;
	u_int64_t cs_rx_ctl;
	u_int64_t cs_rx_discard;
	u_int64_t cs_rx_security_failure;
};

/* Basic Pdev Data */
struct basic_pdev_data_tx {
	struct pkt_info ingress;
	struct pkt_info processed;
	struct pkt_info dropped;
	struct pkt_info tx_success;
	struct pkt_info comp_pkt;
	u_int64_t dropped_count;
	u_int64_t tx_failed;
};

struct basic_pdev_data_rx {
	struct pkt_info to_stack;
	struct pkt_info total_rcvd;
	u_int64_t rx_error_count;
	u_int64_t dropped_count;
	u_int64_t err_count;
};

/* Basic pdev Ctrl */
struct basic_pdev_ctrl_tx {
	u_int32_t cs_tx_mgmt;
	u_int32_t cs_tx_frame_count;
};

struct basic_pdev_ctrl_rx {
	u_int32_t cs_rx_mgmt;
	u_int32_t cs_rx_num_mgmt;
	u_int32_t cs_rx_num_ctl;
	u_int32_t cs_rx_frame_count;
	u_int32_t cs_rx_error_sum;
};

struct basic_pdev_ctrl_link {
	u_int32_t cs_chan_tx_pwr;
	u_int32_t cs_rx_rssi_comb;
	int16_t cs_chan_nf;
	int16_t cs_chan_nf_sec80;
	u_int8_t dcs_total_util;
};

/* Basic psoc Data */
struct basic_psoc_data_tx {
	struct pkt_info egress;
};

struct basic_psoc_data_rx {
	struct pkt_info ingress;
};

/* Advance Peer Data */
#if WLAN_ADVANCE_TELEMETRY
struct advance_peer_data_tx {
	struct basic_peer_data_tx b_tx;
	struct pkt_info ucast;
	struct pkt_info mcast;
	struct pkt_info bcast;
	u_int32_t nss[SS_COUNT];
	u_int32_t sgi_count[MAX_GI];
	u_int32_t bw[MAX_BW];
	u_int32_t retries;
	u_int32_t non_amsdu_cnt;
	u_int32_t amsdu_cnt;
	u_int32_t ampdu_cnt;
	u_int32_t non_ampdu_cnt;
};

struct advance_peer_data_rx {
	struct basic_peer_data_rx b_rx;
	struct pkt_info unicast;
	struct pkt_info multicast;
	struct pkt_info bcast;
	u_int32_t su_ax_ppdu_cnt[MAX_MCS];
	u_int32_t rx_mpdu_cnt[MAX_MCS];
	u_int32_t wme_ac_type[WME_AC_MAX];
	u_int32_t sgi_count[MAX_GI];
	u_int32_t nss[SS_COUNT];
	u_int32_t ppdu_nss[SS_COUNT];
	u_int32_t bw[MAX_BW];
	u_int32_t mpdu_cnt_fcs_ok;
	u_int32_t mpdu_cnt_fcs_err;
	u_int32_t non_amsdu_cnt;
	u_int32_t non_ampdu_cnt;
	u_int32_t ampdu_cnt;
	u_int32_t amsdu_cnt;
	u_int32_t bar_recv_cnt;
	u_int32_t rx_retries;
	u_int32_t multipass_rx_pkt_drop;
};

struct advance_peer_data_raw {
	struct pkt_info raw;
};

struct advance_peer_data_fwd {
	struct pkt_info pkts;
	struct pkt_info fail;
	uint32_t mdns_no_fwd;
};

struct advance_peer_data_twt {
	struct pkt_info to_stack_twt;
	struct pkt_info tx_success_twt;
};

struct advance_peer_data_rate {
	struct basic_peer_data_rate b_rate;
	u_int32_t rnd_avg_rx_rate;
	u_int32_t avg_rx_rate;
	u_int32_t rnd_avg_tx_rate;
	u_int32_t avg_tx_rate;
};

struct advance_peer_data_link {
	struct basic_peer_data_link b_link;
	u_int32_t rx_snr_measured_time;
};

struct advance_peer_data_nawds {
	struct pkt_info nawds_mcast;
	u_int32_t nawds_mcast_tx_drop;
	u_int32_t nawds_mcast_rx_drop;
};

struct advance_peer_data_delay {
	struct cdp_delay_tid_stats delay_stats[CDP_MAX_DATA_TIDS]
					       [CDP_MAX_TXRX_CTX];
	u_int32_t tx_avg_jitter;
	u_int32_t tx_avg_delay;
	u_int64_t tx_avg_err;
	u_int64_t tx_total_success;
	u_int64_t tx_drop;
};

/* Advance Peer Ctrl */
struct advance_peer_ctrl_tx {
	struct basic_peer_ctrl_tx b_tx;
	u_int32_t cs_tx_assoc;
	u_int32_t cs_tx_assoc_fail;
};

struct advance_peer_ctrl_rx {
	struct basic_peer_ctrl_rx b_rx;
};

struct advance_peer_ctrl_twt {
	u_int32_t cs_twt_event_type;
	u_int32_t cs_twt_dialog_id;
	u_int32_t cs_twt_wake_dura_us;
	u_int32_t cs_twt_wake_intvl_us;
	u_int32_t cs_twt_sp_offset_us;
	u_int32_t cs_twt_flow_id:16,
		  cs_twt_bcast:1,
		  cs_twt_trig:1,
		  cs_twt_announ:1;
};

struct advance_peer_ctrl_rate {
	struct basic_peer_ctrl_rate b_rate;
};

struct advance_peer_ctrl_link {
	struct basic_peer_ctrl_link b_link;
};

/* Advance Vdev Data */
struct advance_vdev_data_tx {
	struct basic_vdev_data_tx b_tx;
	struct pkt_info reinject_pkts;
	struct pkt_info inspect_pkts;
	struct pkt_info ucast;
	struct pkt_info mcast;
	struct pkt_info bcast;
	u_int32_t nss[SS_COUNT];
	u_int32_t sgi_count[MAX_GI];
	u_int32_t bw[MAX_BW];
	u_int32_t retries;
	u_int32_t non_amsdu_cnt;
	u_int32_t amsdu_cnt;
	u_int32_t ampdu_cnt;
	u_int32_t non_ampdu_cnt;
	u_int32_t cce_classified;
};

struct advance_vdev_data_rx {
	struct basic_vdev_data_rx b_rx;
	struct pkt_info unicast;
	struct pkt_info multicast;
	struct pkt_info bcast;
	u_int32_t su_ax_ppdu_cnt[MAX_MCS];
	u_int32_t rx_mpdu_cnt[MAX_MCS];
	u_int32_t nss[SS_COUNT];
	u_int32_t ppdu_nss[SS_COUNT];
	u_int32_t bw[MAX_BW];
	u_int32_t sgi_count[MAX_GI];
	u_int32_t wme_ac_type[WME_AC_MAX];
	u_int32_t mpdu_cnt_fcs_ok;
	u_int32_t mpdu_cnt_fcs_err;
	u_int32_t non_amsdu_cnt;
	u_int32_t non_ampdu_cnt;
	u_int32_t ampdu_cnt;
	u_int32_t amsdu_cnt;
	u_int32_t bar_recv_cnt;
	u_int32_t rx_retries;
	u_int32_t multipass_rx_pkt_drop;
};

struct advance_vdev_data_me {
	struct pkt_info mcast_pkt;
	u_int32_t ucast;
};

struct advance_vdev_data_raw {
	struct pkt_info rx_raw;
	struct pkt_info tx_raw_pkt;
	u_int32_t cce_classified_raw;
};

struct advance_vdev_data_tso {
	struct pkt_info sg_pkt;
	struct pkt_info non_sg_pkts;
	struct pkt_info num_tso_pkts;
};

struct advance_vdev_data_igmp {
	u_int32_t igmp_rcvd;
	u_int32_t igmp_ucast_converted;
};

struct advance_vdev_data_mesh {
	u_int32_t exception_fw;
	u_int32_t completion_fw;
};

struct advance_vdev_data_nawds {
	struct pkt_info tx_nawds_mcast;
	u_int32_t nawds_mcast_tx_drop;
	u_int32_t nawds_mcast_rx_drop;
};

/* Advance Vdev Ctrl */
struct advance_vdev_ctrl_tx {
	struct basic_vdev_ctrl_tx b_tx;
	u_int64_t cs_tx_offchan_mgmt;
	u_int64_t cs_tx_offchan_data;
	u_int64_t cs_tx_offchan_fail;
	u_int64_t cs_tx_bcn_success;
	u_int64_t cs_tx_bcn_outage;
	u_int64_t cs_fils_frames_sent;
	u_int64_t cs_fils_frames_sent_fail;
	u_int64_t cs_tx_offload_prb_resp_succ_cnt;
	u_int64_t cs_tx_offload_prb_resp_fail_cnt;
};

struct advance_vdev_ctrl_rx {
	struct basic_vdev_ctrl_rx b_rx;
	u_int64_t cs_rx_action;
	u_int64_t cs_mlme_auth_attempt;
	u_int64_t cs_mlme_auth_success;
	u_int64_t cs_authorize_attempt;
	u_int64_t cs_authorize_success;
	u_int64_t cs_prob_req_drops;
	u_int64_t cs_oob_probe_req_count;
	u_int64_t cs_wc_probe_req_drops;
	u_int64_t cs_sta_xceed_rlim;
	u_int64_t cs_sta_xceed_vlim;
};

/* Advance Pdev Data */
struct histogram_stats {
	u_int32_t pkts_1;
	u_int32_t pkts_2_20;
	u_int32_t pkts_21_40;
	u_int32_t pkts_41_60;
	u_int32_t pkts_61_80;
	u_int32_t pkts_81_100;
	u_int32_t pkts_101_200;
	u_int32_t pkts_201_plus;
};

struct advance_pdev_data_tx {
	struct basic_pdev_data_tx b_tx;
	struct pkt_info reinject_pkts;
	struct pkt_info inspect_pkts;
	struct pkt_info ucast;
	struct pkt_info mcast;
	struct pkt_info bcast;
	struct histogram_stats tx_hist;
	u_int32_t sgi_count[MAX_GI];
	u_int32_t nss[SS_COUNT];
	u_int32_t bw[MAX_BW];
	u_int32_t retries;
	u_int32_t non_amsdu_cnt;
	u_int32_t amsdu_cnt;
	u_int32_t ampdu_cnt;
	u_int32_t non_ampdu_cnt;
	u_int32_t cce_classified;
};

struct advance_pdev_data_rx {
	struct basic_pdev_data_rx b_rx;
	struct pkt_info unicast;
	struct pkt_info multicast;
	struct pkt_info bcast;
	struct histogram_stats rx_hist;
	u_int32_t su_ax_ppdu_cnt[MAX_MCS];
	u_int32_t rx_mpdu_cnt[MAX_MCS];
	u_int32_t nss[SS_COUNT];
	u_int32_t ppdu_nss[SS_COUNT];
	u_int32_t bw[MAX_BW];
	u_int32_t sgi_count[MAX_GI];
	u_int32_t wme_ac_type[WME_AC_MAX];
	u_int32_t mpdu_cnt_fcs_ok;
	u_int32_t mpdu_cnt_fcs_err;
	u_int32_t non_amsdu_cnt;
	u_int32_t non_ampdu_cnt;
	u_int32_t ampdu_cnt;
	u_int32_t amsdu_cnt;
	u_int32_t bar_recv_cnt;
	u_int32_t rx_retries;
	u_int32_t multipass_rx_pkt_drop;
};

struct advance_pdev_data_me {
	struct pkt_info mcast_pkt;
	u_int32_t ucast;
};

struct advance_pdev_data_raw {
	struct pkt_info rx_raw;
	struct pkt_info tx_raw_pkt;
	u_int32_t cce_classified_raw;
	u_int32_t rx_raw_pkts;
};

struct advance_pdev_data_tso {
	struct pkt_info sg_pkt;
	struct pkt_info non_sg_pkts;
	struct pkt_info num_tso_pkts;
	u_int64_t segs_1;
	u_int64_t segs_2_5;
	u_int64_t segs_6_10;
	u_int64_t segs_11_15;
	u_int64_t segs_16_20;
	u_int64_t segs_20_plus;
	u_int32_t tso_comp;
};

struct advance_pdev_data_igmp {
	u_int32_t igmp_rcvd;
	u_int32_t igmp_ucast_converted;
};

struct advance_pdev_data_mesh {
	u_int32_t exception_fw;
	u_int32_t completion_fw;
};

struct advance_pdev_data_nawds {
	struct pkt_info tx_nawds_mcast;
	u_int32_t nawds_mcast_tx_drop;
	u_int32_t nawds_mcast_rx_drop;
};

/* Advance Pdev Ctrl */
struct advance_pdev_ctrl_tx {
	struct basic_pdev_ctrl_tx b_tx;
	u_int64_t cs_tx_beacon;
	u_int64_t cs_tx_retries;
};

struct advance_pdev_ctrl_rx {
	struct basic_pdev_ctrl_rx b_rx;
	u_int32_t cs_rx_mgmt_rssi_drop;
};

struct advance_pdev_ctrl_link {
	struct basic_pdev_ctrl_link b_link;
	u_int32_t dcs_ss_under_util;
	u_int32_t dcs_sec_20_util;
	u_int32_t dcs_sec_40_util;
	u_int32_t dcs_sec_80_util;
	u_int32_t cs_tx_rssi;
	u_int8_t dcs_ap_tx_util;
	u_int8_t dcs_ap_rx_util;
	u_int8_t dcs_self_bss_util;
	u_int8_t dcs_obss_util;
	u_int8_t dcs_obss_rx_util;
	u_int8_t dcs_free_medium;
	u_int8_t dcs_non_wifi_util;
	u_int8_t rx_rssi_chain0_pri20;
	u_int8_t rx_rssi_chain0_sec20;
	u_int8_t rx_rssi_chain0_sec40;
	u_int8_t rx_rssi_chain0_sec80;
	u_int8_t rx_rssi_chain1_pri20;
	u_int8_t rx_rssi_chain1_sec20;
	u_int8_t rx_rssi_chain1_sec40;
	u_int8_t rx_rssi_chain1_sec80;
	u_int8_t rx_rssi_chain2_pri20;
	u_int8_t rx_rssi_chain2_sec20;
	u_int8_t rx_rssi_chain2_sec40;
	u_int8_t rx_rssi_chain2_sec80;
	u_int8_t rx_rssi_chain3_pri20;
	u_int8_t rx_rssi_chain3_sec20;
	u_int8_t rx_rssi_chain3_sec40;
	u_int8_t rx_rssi_chain3_sec80;
};

/* Advance Psoc Data */
struct advance_psoc_data_tx {
	struct basic_psoc_data_tx b_tx;
};

struct advance_psoc_data_rx {
	struct basic_psoc_data_rx b_rx;
	u_int32_t err_ring_pkts;
	u_int32_t rx_frags;
	u_int32_t reo_reinject;
	u_int32_t bar_frame;
	u_int32_t rejected;
	u_int32_t raw_frm_drop;
};
#endif /* WLAN_ADVANCE_TELEMETRY */
#endif /* _WLAN_STATS_DEFINE_H_ */
