/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
 * Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef _STATS_LIB_H_
#define _STATS_LIB_H_

/* Network Interface name length */
#define IFNAME_LEN                   IFNAMSIZ
#define MAX_RADIO_NUM                3
#define MAX_VAP_NUM                  (17 * MAX_RADIO_NUM)
/* This path is for network interfaces */
#define PATH_SYSNET_DEV              "/sys/class/net/"
#define STATS_IF_MCS_VALID           1
#define STATS_IF_MCS_INVALID         0
#define STATS_IF_MAX_MCS_STRING_LEN  34

enum stats_if_packet_type {
	STATS_IF_DOT11_A = 0,
	STATS_IF_DOT11_B = 1,
	STATS_IF_DOT11_N = 2,
	STATS_IF_DOT11_AC = 3,
	STATS_IF_DOT11_AX = 4,
#ifdef WLAN_FEATURE_11BE
	STATS_IF_DOT11_BE = 5,
#endif
	STATS_IF_DOT11_MAX,
};

struct stats_if_rate_debug {
	char mcs_type[STATS_IF_MAX_MCS_STRING_LEN];
	uint8_t valid;
};

#ifdef WLAN_FEATURE_11BE
static const struct stats_if_rate_debug rate_string[STATS_IF_DOT11_MAX]
						   [STATS_IF_MAX_MCS] = {
	{
		{"OFDM 48 Mbps", STATS_IF_MCS_VALID},
		{"OFDM 24 Mbps", STATS_IF_MCS_VALID},
		{"OFDM 12 Mbps", STATS_IF_MCS_VALID},
		{"OFDM 6 Mbps ", STATS_IF_MCS_VALID},
		{"OFDM 54 Mbps", STATS_IF_MCS_VALID},
		{"OFDM 36 Mbps", STATS_IF_MCS_VALID},
		{"OFDM 18 Mbps", STATS_IF_MCS_VALID},
		{"OFDM 9 Mbps ", STATS_IF_MCS_VALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
	},
	{
		{"CCK 11 Mbps Long  ", STATS_IF_MCS_VALID},
		{"CCK 5.5 Mbps Long ", STATS_IF_MCS_VALID},
		{"CCK 2 Mbps Long   ", STATS_IF_MCS_VALID},
		{"CCK 1 Mbps Long   ", STATS_IF_MCS_VALID},
		{"CCK 11 Mbps Short ", STATS_IF_MCS_VALID},
		{"CCK 5.5 Mbps Short", STATS_IF_MCS_VALID},
		{"CCK 2 Mbps Short  ", STATS_IF_MCS_VALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
	},
	{
		{"HT MCS 0 (BPSK 1/2)  ", STATS_IF_MCS_VALID},
		{"HT MCS 1 (QPSK 1/2)  ", STATS_IF_MCS_VALID},
		{"HT MCS 2 (QPSK 3/4)  ", STATS_IF_MCS_VALID},
		{"HT MCS 3 (16-QAM 1/2)", STATS_IF_MCS_VALID},
		{"HT MCS 4 (16-QAM 3/4)", STATS_IF_MCS_VALID},
		{"HT MCS 5 (64-QAM 2/3)", STATS_IF_MCS_VALID},
		{"HT MCS 6 (64-QAM 3/4)", STATS_IF_MCS_VALID},
		{"HT MCS 7 (64-QAM 5/6)", STATS_IF_MCS_VALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
	},
	{
		{"VHT MCS 0 (BPSK 1/2)     ", STATS_IF_MCS_VALID},
		{"VHT MCS 1 (QPSK 1/2)     ", STATS_IF_MCS_VALID},
		{"VHT MCS 2 (QPSK 3/4)     ", STATS_IF_MCS_VALID},
		{"VHT MCS 3 (16-QAM 1/2)   ", STATS_IF_MCS_VALID},
		{"VHT MCS 4 (16-QAM 3/4)   ", STATS_IF_MCS_VALID},
		{"VHT MCS 5 (64-QAM 2/3)   ", STATS_IF_MCS_VALID},
		{"VHT MCS 6 (64-QAM 3/4)   ", STATS_IF_MCS_VALID},
		{"VHT MCS 7 (64-QAM 5/6)   ", STATS_IF_MCS_VALID},
		{"VHT MCS 8 (256-QAM 3/4)  ", STATS_IF_MCS_VALID},
		{"VHT MCS 9 (256-QAM 5/6)  ", STATS_IF_MCS_VALID},
		{"VHT MCS 10 (1024-QAM 3/4)", STATS_IF_MCS_VALID},
		{"VHT MCS 11 (1024-QAM 5/6)", STATS_IF_MCS_VALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
	},
	{
		{"HE MCS 0 (BPSK 1/2)     ", STATS_IF_MCS_VALID},
		{"HE MCS 1 (QPSK 1/2)     ", STATS_IF_MCS_VALID},
		{"HE MCS 2 (QPSK 3/4)     ", STATS_IF_MCS_VALID},
		{"HE MCS 3 (16-QAM 1/2)   ", STATS_IF_MCS_VALID},
		{"HE MCS 4 (16-QAM 3/4)   ", STATS_IF_MCS_VALID},
		{"HE MCS 5 (64-QAM 2/3)   ", STATS_IF_MCS_VALID},
		{"HE MCS 6 (64-QAM 3/4)   ", STATS_IF_MCS_VALID},
		{"HE MCS 7 (64-QAM 5/6)   ", STATS_IF_MCS_VALID},
		{"HE MCS 8 (256-QAM 3/4)  ", STATS_IF_MCS_VALID},
		{"HE MCS 9 (256-QAM 5/6)  ", STATS_IF_MCS_VALID},
		{"HE MCS 10 (1024-QAM 3/4)", STATS_IF_MCS_VALID},
		{"HE MCS 11 (1024-QAM 5/6)", STATS_IF_MCS_VALID},
		{"HE MCS 12 (4096-QAM 3/4)", STATS_IF_MCS_VALID},
		{"HE MCS 13 (4096-QAM 5/6)", STATS_IF_MCS_VALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
	},
	{
		{"EHT MCS 0 (BPSK 1/2)     ", STATS_IF_MCS_VALID},
		{"EHT MCS 1 (QPSK 1/2)     ", STATS_IF_MCS_VALID},
		{"EHT MCS 2 (QPSK 3/4)     ", STATS_IF_MCS_VALID},
		{"EHT MCS 3 (16-QAM 1/2)   ", STATS_IF_MCS_VALID},
		{"EHT MCS 4 (16-QAM 3/4)   ", STATS_IF_MCS_VALID},
		{"EHT MCS 5 (64-QAM 2/3)   ", STATS_IF_MCS_VALID},
		{"EHT MCS 6 (64-QAM 3/4)   ", STATS_IF_MCS_VALID},
		{"EHT MCS 7 (64-QAM 5/6)   ", STATS_IF_MCS_VALID},
		{"EHT MCS 8 (256-QAM 3/4)  ", STATS_IF_MCS_VALID},
		{"EHT MCS 9 (256-QAM 5/6)  ", STATS_IF_MCS_VALID},
		{"EHT MCS 10 (1024-QAM 3/4)", STATS_IF_MCS_VALID},
		{"EHT MCS 11 (1024-QAM 5/6)", STATS_IF_MCS_VALID},
		{"EHT MCS 12 (4096-QAM 3/4)", STATS_IF_MCS_VALID},
		{"EHT MCS 13 (4096-QAM 5/6)", STATS_IF_MCS_VALID},
		{"EHT MCS 14 (BPSK-DCM 1/2)", STATS_IF_MCS_VALID},
		{"EHT MCS 15 (BPSK-DCM 1/2)", STATS_IF_MCS_VALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
	}
};
#else
static const struct stats_if_rate_debug rate_string[STATS_IF_DOT11_MAX]
						   [STATS_IF_MAX_MCS] = {
	{
		{"OFDM 48 Mbps", STATS_IF_MCS_VALID},
		{"OFDM 24 Mbps", STATS_IF_MCS_VALID},
		{"OFDM 12 Mbps", STATS_IF_MCS_VALID},
		{"OFDM 6 Mbps ", STATS_IF_MCS_VALID},
		{"OFDM 54 Mbps", STATS_IF_MCS_VALID},
		{"OFDM 36 Mbps", STATS_IF_MCS_VALID},
		{"OFDM 18 Mbps", STATS_IF_MCS_VALID},
		{"OFDM 9 Mbps ", STATS_IF_MCS_VALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
	},
	{
		{"CCK 11 Mbps Long  ", STATS_IF_MCS_VALID},
		{"CCK 5.5 Mbps Long ", STATS_IF_MCS_VALID},
		{"CCK 2 Mbps Long   ", STATS_IF_MCS_VALID},
		{"CCK 1 Mbps Long   ", STATS_IF_MCS_VALID},
		{"CCK 11 Mbps Short ", STATS_IF_MCS_VALID},
		{"CCK 5.5 Mbps Short", STATS_IF_MCS_VALID},
		{"CCK 2 Mbps Short  ", STATS_IF_MCS_VALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
	},
	{
		{"HT MCS 0 (BPSK 1/2)  ", STATS_IF_MCS_VALID},
		{"HT MCS 1 (QPSK 1/2)  ", STATS_IF_MCS_VALID},
		{"HT MCS 2 (QPSK 3/4)  ", STATS_IF_MCS_VALID},
		{"HT MCS 3 (16-QAM 1/2)", STATS_IF_MCS_VALID},
		{"HT MCS 4 (16-QAM 3/4)", STATS_IF_MCS_VALID},
		{"HT MCS 5 (64-QAM 2/3)", STATS_IF_MCS_VALID},
		{"HT MCS 6 (64-QAM 3/4)", STATS_IF_MCS_VALID},
		{"HT MCS 7 (64-QAM 5/6)", STATS_IF_MCS_VALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
	},
	{
		{"VHT MCS 0 (BPSK 1/2)     ", STATS_IF_MCS_VALID},
		{"VHT MCS 1 (QPSK 1/2)     ", STATS_IF_MCS_VALID},
		{"VHT MCS 2 (QPSK 3/4)     ", STATS_IF_MCS_VALID},
		{"VHT MCS 3 (16-QAM 1/2)   ", STATS_IF_MCS_VALID},
		{"VHT MCS 4 (16-QAM 3/4)   ", STATS_IF_MCS_VALID},
		{"VHT MCS 5 (64-QAM 2/3)   ", STATS_IF_MCS_VALID},
		{"VHT MCS 6 (64-QAM 3/4)   ", STATS_IF_MCS_VALID},
		{"VHT MCS 7 (64-QAM 5/6)   ", STATS_IF_MCS_VALID},
		{"VHT MCS 8 (256-QAM 3/4)  ", STATS_IF_MCS_VALID},
		{"VHT MCS 9 (256-QAM 5/6)  ", STATS_IF_MCS_VALID},
		{"VHT MCS 10 (1024-QAM 3/4)", STATS_IF_MCS_VALID},
		{"VHT MCS 11 (1024-QAM 5/6)", STATS_IF_MCS_VALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
	},
	{
		{"HE MCS 0 (BPSK 1/2)     ", STATS_IF_MCS_VALID},
		{"HE MCS 1 (QPSK 1/2)     ", STATS_IF_MCS_VALID},
		{"HE MCS 2 (QPSK 3/4)     ", STATS_IF_MCS_VALID},
		{"HE MCS 3 (16-QAM 1/2)   ", STATS_IF_MCS_VALID},
		{"HE MCS 4 (16-QAM 3/4)   ", STATS_IF_MCS_VALID},
		{"HE MCS 5 (64-QAM 2/3)   ", STATS_IF_MCS_VALID},
		{"HE MCS 6 (64-QAM 3/4)   ", STATS_IF_MCS_VALID},
		{"HE MCS 7 (64-QAM 5/6)   ", STATS_IF_MCS_VALID},
		{"HE MCS 8 (256-QAM 3/4)  ", STATS_IF_MCS_VALID},
		{"HE MCS 9 (256-QAM 5/6)  ", STATS_IF_MCS_VALID},
		{"HE MCS 10 (1024-QAM 3/4)", STATS_IF_MCS_VALID},
		{"HE MCS 11 (1024-QAM 5/6)", STATS_IF_MCS_VALID},
		{"HE MCS 12 (4096-QAM 3/4)", STATS_IF_MCS_VALID},
		{"HE MCS 13 (4096-QAM 5/6)", STATS_IF_MCS_VALID},
		{"INVALID ", STATS_IF_MCS_INVALID},
	}
};
#endif

/**
 * struct interface_list: Structure to hold interfaces for Driver Communication
 * @r_count:  Number of Radio Interfaces
 * @v_count:  Number of Vap Interfaces
 * @r_names:  All radio interface names
 * @v_names:  All vap interface names
 */
struct interface_list {
	u_int8_t r_count;
	u_int8_t v_count;
	char *r_names[MAX_RADIO_NUM];
	char *v_names[MAX_VAP_NUM];
};

/* Basic peer data stats holder */
struct basic_peer_data {
	struct basic_peer_data_tx *tx;
	struct basic_peer_data_rx *rx;
	struct basic_peer_data_link *link;
	struct basic_peer_data_rate *rate;
};

/* Basic peer control stats holder */
struct basic_peer_ctrl {
	struct basic_peer_ctrl_tx *tx;
	struct basic_peer_ctrl_rx *rx;
	struct basic_peer_ctrl_link *link;
	struct basic_peer_ctrl_rate *rate;
};

/* Basic vdev data stats holder */
struct basic_vdev_data {
	struct basic_vdev_data_tx *tx;
	struct basic_vdev_data_rx *rx;
};

/* Basic vdev control stats holder */
struct basic_vdev_ctrl {
	struct basic_vdev_ctrl_tx *tx;
	struct basic_vdev_ctrl_rx *rx;
};

/* Basic pdev data stats holder */
struct basic_pdev_data {
	struct basic_pdev_data_tx *tx;
	struct basic_pdev_data_rx *rx;
};

/* Basic pdev control stats holder */
struct basic_pdev_ctrl {
	struct basic_pdev_ctrl_tx *tx;
	struct basic_pdev_ctrl_rx *rx;
	struct basic_pdev_ctrl_link *link;
};

/* Basic psoc data stats holder */
struct basic_psoc_data {
	struct basic_psoc_data_tx *tx;
	struct basic_psoc_data_rx *rx;
};

#if WLAN_ADVANCE_TELEMETRY
/* Advance peer data stats holder */
struct advance_peer_data {
	struct advance_peer_data_tx *tx;
	struct advance_peer_data_rx *rx;
	struct advance_peer_data_fwd *fwd;
	struct advance_peer_data_raw *raw;
	struct advance_peer_data_twt *twt;
	struct advance_peer_data_link *link;
	struct advance_peer_data_rate *rate;
	struct advance_peer_data_nawds *nawds;
};

/* Advance peer control stats holder */
struct advance_peer_ctrl {
	struct advance_peer_ctrl_tx *tx;
	struct advance_peer_ctrl_rx *rx;
	struct advance_peer_ctrl_twt *twt;
	struct advance_peer_ctrl_link *link;
	struct advance_peer_ctrl_rate *rate;
};

/* Advance vdev data stats holder */
struct advance_vdev_data {
	struct advance_vdev_data_me *me;
	struct advance_vdev_data_tx *tx;
	struct advance_vdev_data_rx *rx;
	struct advance_vdev_data_raw *raw;
	struct advance_vdev_data_tso *tso;
	struct advance_vdev_data_igmp *igmp;
	struct advance_vdev_data_mesh *mesh;
	struct advance_vdev_data_nawds *nawds;
};

/* Advance vdev control stats holder */
struct advance_vdev_ctrl {
	struct advance_vdev_ctrl_tx *tx;
	struct advance_vdev_ctrl_rx *rx;
};

/* Advance pdev data stats holder */
struct advance_pdev_data {
	struct advance_pdev_data_me *me;
	struct advance_pdev_data_tx *tx;
	struct advance_pdev_data_rx *rx;
	struct advance_pdev_data_raw *raw;
	struct advance_pdev_data_tso *tso;
	struct advance_pdev_data_igmp *igmp;
	struct advance_pdev_data_mesh *mesh;
	struct advance_pdev_data_nawds *nawds;
};

/* Advance pdev control stats holder */
struct advance_pdev_ctrl {
	struct advance_pdev_ctrl_tx *tx;
	struct advance_pdev_ctrl_rx *rx;
	struct advance_pdev_ctrl_link *link;
};

/* Advance psoc data stats holder */
struct advance_psoc_data {
	struct advance_psoc_data_tx *tx;
	struct advance_psoc_data_rx *rx;
};
#endif /* WLAN_ADVANCE_TELEMETRY */

/**
 * struct stats_obj: Declares structure to hold Stats
 * @lvl: Stats level Basic/Advance/Debug
 * @obj_type: Stats object STA/VAP/RADIO/AP
 * @type: Stats type data or control
 * @pif_name: Parent interface name
 * @u_id.mac_addr: MAC address for STA object
 * @u_id.if_name: Interface name for VAP/RADIO/AP objects
 * @stats: Stats based on above meta information
 * @next: Next stats_obj
 */
struct stats_obj {
	enum stats_level_e lvl;
	enum stats_object_e obj_type;
	enum stats_type_e type;
	char pif_name[IFNAME_LEN];
	union {
		u_int8_t mac_addr[ETH_ALEN];
		char if_name[IFNAME_LEN];
	} u_id;
	void *stats;
	struct stats_obj *next;
};

/**
 * struct reply_buffer: Defines structure to hold the driver reply
 * @obj_head:  Head pointer of stats_obj list
 * @obj_last:  Last pointer of stats_obj list
 */
struct reply_buffer {
	struct stats_obj *obj_head;
	struct stats_obj *obj_last;
};

/**
 * struct stats_command: Defines interface level command structure
 * @lvl:       Stats level
 * @obj:       Stats object
 * @type:      Stats traffic type
 * @recursive: Stats recursiveness
 * @feat_flag: Stats requested for combination of Features
 * @sta_mac:   Station MAC address if Stats requested for STA object
 * @if_name:   Interface name on which Stats is requested
 * @reply:     Pointer to reply buffer provided by user
 */
struct stats_command {
	enum stats_level_e lvl;
	enum stats_object_e obj;
	enum stats_type_e type;
	bool recursive;
	char if_name[IFNAME_LEN];
	u_int64_t feat_flag;
	struct ether_addr sta_mac;
	struct reply_buffer *reply;
};

/**
 * libstats_get_feature_flag(): Function to parse Feature flags and return value
 * @feat_flags: String holding feature flag names separted by dilimeter '|'
 *
 * Return: Combination of requested feature flag value or 0
 */
u_int64_t libstats_get_feature_flag(char *feat_flags);

/**
 * libstats_request_handle(): Function to send stats request to driver
 * @cmd: Filled command structure by Application
 *
 * Return: 0 on Success, -1 on Failure
 */
int32_t libstats_request_handle(struct stats_command *cmd);

/**
 * libstats_free_reply_buffer(): Function to free all reply objects
 * @cmd: Pointer to command structure
 *
 * Return: None
 */
void libstats_free_reply_buffer(struct stats_command *cmd);
#endif /* _STATS_LIB_H_ */
