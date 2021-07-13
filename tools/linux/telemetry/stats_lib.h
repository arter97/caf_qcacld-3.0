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

#ifndef _STATS_LIB_H_
#define _STATS_LIB_H_

/* Network Interface name length */
#define IFNAME_LEN                IFNAMSIZ
#define MAX_RADIO_NUM             3
#define MAX_VAP_NUM               (17 * MAX_RADIO_NUM)
/* This path is for network interfaces */
#define PATH_SYSNET_DEV           "/sys/class/net/"

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

/**
 * struct stats_list_manager: Structure to hold object list node pointers
 * @next:  Points to next node in same list
 * @parent:  Points to parent node from parent list
 * @child_root:  Points to child head node of child list
 */
struct stats_list_manager {
	void *next;
	void *parent;
	void *child_root;
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
 * struct stats_sta: Declares structure to hold Sta Stats
 * @mgr:  List manager for Sta list
 * @u_basic:  Union of Basic Data and Control stats for Sta
 * @u_advance:  Union of Advance Data and Control stats for Sta
 * @mac_addr:  Sta MAC Address reported from Driver
 */
struct stats_sta {
	struct stats_list_manager mgr;
	union {
		struct basic_peer_data data;
		struct basic_peer_ctrl ctrl;
	} u_basic;
#if WLAN_ADVANCE_TELEMETRY
	union {
		struct advance_peer_data data;
		struct advance_peer_ctrl ctrl;
	} u_advance;
#endif /* WLAN_ADVANCE_TELEMETRY */
	u_int8_t mac_addr[ETH_ALEN];
};

/**
 * struct stats_vap: Declares structure to hold Vap Stats
 * @mgr:  List manager for Vap list
 * @u_basic:  Union of Basic Data and Control stats for Vap
 * @u_advance:  Union of Advance Data and Control stats for Vap
 * @recursive:  Recursive flag
 * @id:  ID of Vap maintained internally to link corresponding child objects
 * @vap_name: Vap name reported from Driver
 */
struct stats_vap {
	struct stats_list_manager mgr;
	union {
		struct basic_vdev_data data;
		struct basic_vdev_ctrl ctrl;
	} u_basic;
#if WLAN_ADVANCE_TELEMETRY
	union {
		struct advance_vdev_data data;
		struct advance_vdev_ctrl ctrl;
	} u_advance;
#endif /* WLAN_ADVANCE_TELEMETRY */
	bool recursive;
	u_int8_t id;
	char vap_name[IFNAME_LEN];
};

/**
 * struct stats_radio: Declares structure to hold Radio Stats
 * @mgr:  List manager for Radio list
 * @u_basic:  Union of Basic Data and Control stats for Radio
 * @u_advance:  Union of Advance Data and Control stats for Radio
 * @recursive:  Recursive flag
 * @id:  ID of Radio maintained internally to link corresponding child objects
 * @radio_name: Radio Name based on the id reported from Driver
 */
struct stats_radio {
	struct stats_list_manager mgr;
	union {
		struct basic_pdev_data data;
		struct basic_pdev_ctrl ctrl;
	} u_basic;
#if WLAN_ADVANCE_TELEMETRY
	union {
		struct advance_pdev_data data;
		struct advance_pdev_ctrl ctrl;
	} u_advance;
#endif /* WLAN_ADVANCE_TELEMETRY */
	bool recursive;
	u_int8_t id;
	char radio_name[IFNAME_LEN];
};

/**
 * struct stats_ap: Declares structure to hold AP stats
 * @mgr:  List manager for AP list
 * @u:  Union of Basic and Advance stats for AP
 * @b_data:  Basic stats holder
 * @a_data:  Advance Stats holder
 * @recursive:  Recursive flag
 * @id:  ID of SoC maintained internally to link corresponding child objects
 * @ap_name:  SoC name based on the id reported from Driver
 */
struct stats_ap {
	struct stats_list_manager mgr;
	union {
		struct basic_psoc_data b_data;
#if WLAN_ADVANCE_TELEMETRY
		struct advance_psoc_data a_data;
#endif /* WLAN_ADVANCE_TELEMETRY */
	} u;
	bool recursive;
	u_int8_t id;
	char ap_name[IFNAME_LEN];
};

/**
 * struct reply_buffer: Defines structure to hold the driver reply
 * @root_obj:  The User requested Stats object
 * @vap_count:  Number of VAP's stats are parsed from driver response
 * @radio_count:  Number of Radio's stats are parsed from driver response
 * @ap_count:  Number of SoC's stats parsed from driver response
 * @obj_list:  Internal structure to hold below members
 * @sta_list:  Pointer to the head of STA list
 * @vap_list:  Pointer to the head of VAP list
 * @radio_list:  Pointer to the head of Radio list
 * @ap_list:  Pointer to the head of AP list
 */
struct reply_buffer {
	enum stats_object_e root_obj;
	u_int8_t vap_count;
	u_int8_t radio_count;
	u_int8_t ap_count;
	struct {
		struct stats_sta *sta_list;
		struct stats_vap *vap_list;
		struct stats_radio *radio_list;
		struct stats_ap *ap_list;
	} obj_list;
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
