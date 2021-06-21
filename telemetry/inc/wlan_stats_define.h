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

/* Flags for Faeture specific stats */
#define STATS_FEAT_FLG_RX         0x00000001
#define STATS_FEAT_FLG_TX         0x00000002
#define STATS_FEAT_FLG_AST        0x00000004
#define STATS_FEAT_FLG_CFR        0x00000008
#define STATS_FEAT_FLG_FWD        0x00000010
#define STATS_FEAT_FLG_HTT        0x00000020
#define STATS_FEAT_FLG_RAW        0x00000040
#define STATS_FEAT_FLG_RDK        0x00000080
#define STATS_FEAT_FLG_TSO        0x00000100
#define STATS_FEAT_FLG_TWT        0x00000200
#define STATS_FEAT_FLG_VOW        0x00000400
#define STATS_FEAT_FLG_WDI        0x00000800
#define STATS_FEAT_FLG_WMI        0x00001000
#define STATS_FEAT_FLG_IGMP       0x00002000
#define STATS_FEAT_FLG_LINK       0x00004000
#define STATS_FEAT_FLG_MESH       0x00008000
#define STATS_FEAT_FLG_RATE       0x00010000
#define STATS_FEAT_FLG_DELAY      0x00020000
#define STATS_FEAT_FLG_ME         0x00040000
#define STATS_FEAT_FLG_NAWDS      0x00080000
#define STATS_FEAT_FLG_TXCAP      0x00100000
#define STATS_FEAT_FLG_MONITOR    0x00200000
#define STATS_FEAT_FLG_ALL        0xFFFFFFFF

#define STATS_MAX_GI              (4 + 1)
#define STATS_SS_COUNT            8
#define STATS_MAX_BW              7
#define STATS_MAX_MCS             (14 + 1)
#define STATS_WME_AC_MAX          4
#define STATS_MAX_DATA_TIDS       9
#define STATS_MAX_TXRX_CTX        4

/**
 * enum stats_level_e: Defines detailing levels
 * @STATS_LVL_BASIC:    Very minimal stats data
 * @STATS_LVL_DEBUG:    Stats data for debug purpose
 * @STATS_LVL_ADVANCE:  Mostly feature specific stats data
 * @STATS_LVL_MAX:      Max supported Stats levels
 */
enum stats_level_e {
	STATS_LVL_BASIC,
	STATS_LVL_DEBUG,
	STATS_LVL_ADVANCE,
	STATS_LVL_MAX = STATS_LVL_ADVANCE,
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

struct pkt_info {
	u_int32_t num;
	u_int64_t bytes;
};

/* Basic Peer Data */
struct basic_peer_data_tx {
	struct pkt_info tx_success;
	struct pkt_info comp_pkt;
	u_int32_t tx_failed;
	u_int64_t dropped_count;
};

struct basic_peer_data_rx {
	struct pkt_info to_stack;
	u_int64_t total_pkt_rcvd;
	u_int64_t total_bytes_rcvd;
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
	struct pkt_info rcvd;
	struct pkt_info processed;
	struct pkt_info dropped;
	struct pkt_info tx_success;
	struct pkt_info comp_pkt;
	u_int32_t tx_failed;
	u_int64_t dropped_count;
};

struct basic_vdev_data_rx {
	struct pkt_info to_stack;
	u_int64_t total_pkt_rcvd;
	u_int64_t total_bytes_rcvd;
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
	struct pkt_info rcvd;
	struct pkt_info processed;
	struct pkt_info dropped;
	struct pkt_info tx_success;
	struct pkt_info comp_pkt;
	u_int32_t tx_failed;
	u_int64_t dropped_count;
};

struct basic_pdev_data_rx {
	struct pkt_info to_stack;
	u_int64_t total_pkt_rcvd;
	u_int64_t total_bytes_rcvd;
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

#endif /* _WLAN_STATS_DEFINE_H_ */
