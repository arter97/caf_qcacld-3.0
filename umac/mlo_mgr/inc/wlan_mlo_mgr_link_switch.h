/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * DOC: contains MLO manager public file containing link switch functionality
 */
#ifndef _WLAN_MLO_MGR_LINK_SWITCH_H_
#define _WLAN_MLO_MGR_LINK_SWITCH_H_

#include <wlan_mlo_mgr_public_structs.h>

struct wlan_channel;

#define WLAN_MLO_LSWITCH_MAX_HISTORY 5
#ifndef WLAN_MAX_ML_BSS_LINKS
#define WLAN_MAX_ML_BSS_LINKS 1
#endif

/**
 * enum wlan_mlo_link_switch_cnf_reason: Link Switch reject reason
 *
 * @MLO_LINK_SWITCH_CNF_REASON_BSS_PARAMS_CHANGED: Link's BSS params changed
 * @MLO_LINK_SWITCH_CNF_REASON_CONCURRECNY_CONFLICT: Rejected because of
 * Concurrency
 * @MLO_LINK_SWITCH_CNF_REASON_HOST_INTERNAL_ERROR: Host internal error
 * @MLO_LINK_SWITCH_CNF_REASON_MAX: Maximum reason for link switch rejection
 */
enum wlan_mlo_link_switch_cnf_reason {
	MLO_LINK_SWITCH_CNF_REASON_BSS_PARAMS_CHANGED = 1,
	MLO_LINK_SWITCH_CNF_REASON_CONCURRECNY_CONFLICT = 2,
	MLO_LINK_SWITCH_CNF_REASON_HOST_INTERNAL_ERROR = 3,
	MLO_LINK_SWITCH_CNF_REASON_MAX,
};

/**
 * enum wlan_mlo_link_switch_cnf_status: Link Switch Confirmation status
 *
 * @MLO_LINK_SWITCH_CNF_STATUS_ACCEPT: Link switch accepted
 * @MLO_LINK_SWITCH_CNF_STATUS_REJECT: Rejected because link switch cnf reason
 * @MLO_LINK_SWITCH_CNF_STATUS_MAX: Maximum reason for link status
 */
enum wlan_mlo_link_switch_cnf_status {
	MLO_LINK_SWITCH_CNF_STATUS_ACCEPT = 0,
	MLO_LINK_SWITCH_CNF_STATUS_REJECT = 1,
	MLO_LINK_SWITCH_CNF_STATUS_MAX,
};

/**
 * struct wlan_mlo_link_switch_cnf: structure to hold link switch conf info
 *
 * @vdev_id: VDEV ID of link switch link
 * @status: Link Switch Confirmation status
 * @reason: Link Switch Reject reason
 */
struct wlan_mlo_link_switch_cnf {
	uint32_t vdev_id;
	enum wlan_mlo_link_switch_cnf_status status;
	enum wlan_mlo_link_switch_cnf_reason reason;
};

/**
 * enum wlan_mlo_link_switch_reason- Reason for link switch
 *
 * @MLO_LINK_SWITCH_REASON_RSSI_CHANGE: Link switch reason is because of RSSI
 * @MLO_LINK_SWITCH_REASON_LOW_QUALITY: Link switch reason is because of low
 * quality
 * @MLO_LINK_SWITCH_REASON_C2_CHANGE: Link switch reason is because of C2 Metric
 * @MLO_LINK_SWITCH_REASON_HOST_FORCE: Link switch reason is because of host
 * force active/inactive
 * @MLO_LINK_SWITCH_REASON_T2LM: Link switch reason is because of T2LM
 * @MLO_LINK_SWITCH_REASON_MAX: Link switch reason max
 */
enum wlan_mlo_link_switch_reason {
	MLO_LINK_SWITCH_REASON_RSSI_CHANGE = 1,
	MLO_LINK_SWITCH_REASON_LOW_QUALITY = 2,
	MLO_LINK_SWITCH_REASON_C2_CHANGE   = 3,
	MLO_LINK_SWITCH_REASON_HOST_FORCE  = 4,
	MLO_LINK_SWITCH_REASON_T2LM        = 5,
	MLO_LINK_SWITCH_REASON_MAX,
};

/**
 * struct wlan_mlo_link_switch_req - Data Structure because of link switch
 * request
 * @is_in_progress: is link switch in progress
 * @restore_vdev_flag: VDEV Flag to be restored post link switch.
 * @vdev_id: VDEV Id of the link which is under link switch
 * @curr_ieee_link_id: Current link id of the ML link
 * @new_ieee_link_id: Link id of the link to which going to link switched
 * @peer_mld_addr: Peer MLD address
 * @new_primary_freq: primary frequency of link switch link
 * @new_phymode: Phy mode of link switch link
 * @reason: Link switch reason
 * @link_switch_ts: Link switch timestamp
 */
struct wlan_mlo_link_switch_req {
	bool is_in_progress;
	bool restore_vdev_flag;
	uint8_t vdev_id;
	uint8_t curr_ieee_link_id;
	uint8_t new_ieee_link_id;
	struct qdf_mac_addr peer_mld_addr;
	uint32_t new_primary_freq;
	uint32_t new_phymode;
	enum wlan_mlo_link_switch_reason reason;
	qdf_time_t link_switch_ts;
};

/**
 * struct mlo_link_switch_stats - hold information regarding link switch stats
 * @total_num_link_switch: Total number of link switch
 * @req_reason: Reason of link switch received from FW
 * @cnf_reason: Confirm reason sent to FW
 * @req_ts: Link switch timestamp
 * @lswitch_status: structure to hold link switch status
 */
struct mlo_link_switch_stats {
	uint32_t total_num_link_switch;
	struct {
		enum wlan_mlo_link_switch_reason req_reason;
		enum wlan_mlo_link_switch_cnf_reason cnf_reason;
		qdf_time_t req_ts;
	} lswitch_status[WLAN_MLO_LSWITCH_MAX_HISTORY];
};

/**
 * struct mlo_link_switch_context - Link switch data structure.
 * @links_info: Hold information regarding all the links of ml connection
 * @last_req: Last link switch request received from FW
 * @lswitch_stats: History of the link switch stats
 *                 Includes both fail and success stats.
 */
struct mlo_link_switch_context {
	struct mlo_link_info links_info[WLAN_MAX_ML_BSS_LINKS];
	struct wlan_mlo_link_switch_req last_req;
	struct mlo_link_switch_stats lswitch_stats[MLO_LINK_SWITCH_CNF_STATUS_MAX];
};

/**
 * mlo_mgr_update_link_info_mac_addr() - MLO mgr update link info mac address
 * @vdev: Object manager vdev
 * @mlo_mac_update: ML link mac addresses update.
 *
 * Update link mac addresses for the ML links
 * Return: none
 */
void mlo_mgr_update_link_info_mac_addr(struct wlan_objmgr_vdev *vdev,
			       struct wlan_mlo_link_mac_update *mlo_mac_update);

/**
 * mlo_mgr_update_link_info_reset() - Reset link info of ml dev context
 * @ml_dev: MLO device context
 *
 * Reset link info of ml links
 * Return: QDF_STATUS
 */
void mlo_mgr_update_link_info_reset(struct wlan_mlo_dev_context *ml_dev);

/**
 * mlo_mgr_update_ap_link_info() - Update AP links information
 * @vdev: Object Manager vdev
 * @link_id: Link id of the AP MLD link
 * @ap_link_addr: AP link addresses
 * @channel: wlan channel information of the link
 *
 * Update AP link information for each link of AP MLD
 * Return: void
 */
void mlo_mgr_update_ap_link_info(struct wlan_objmgr_vdev *vdev, uint8_t link_id,
				 uint8_t *ap_link_addr,
				 struct wlan_channel channel);
/**
 * mlo_mgr_update_ap_channel_info() - Update AP channel information
 * @vdev: Object Manager vdev
 * @link_id: Link id of the AP MLD link
 * @ap_link_addr: AP link addresses
 * @channel: wlan channel information of the link
 *
 * Update AP channel information for each link of AP MLD
 * Return: void
 */
void mlo_mgr_update_ap_channel_info(struct wlan_objmgr_vdev *vdev,
				    uint8_t link_id,
				    uint8_t *ap_link_addr,
				    struct wlan_channel channel);

/**
 * mlo_mgr_get_ap_link_by_link_id() - Get mlo link info from link id
 * @vdev: Object Manager vdev
 * @link_id: Link id of the AP MLD link
 *
 * Search for the @link_id in the array in link_ctx in mlo_dev_ctx.
 * Returns the pointer of mlo_link_info element matching the @link_id,
 * or else NULL.
 *
 * Return: Pointer of link info
 */
struct mlo_link_info*
mlo_mgr_get_ap_link_by_link_id(struct wlan_objmgr_vdev *vdev, int link_id);

/**
 * mlo_mgr_link_switch_request_params() - Link switch request params from FW.
 * @psoc: PSOC object manager
 * @evt_params: Link switch params received from FW.
 *
 * The @params contain link switch request parameters received from FW as
 * an indication to host to trigger link switch sequence on the specified
 * VDEV. If the @params are not valid link switch will be terminated.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS mlo_mgr_link_switch_request_params(struct wlan_objmgr_psoc *psoc,
					      void *evt_params);
/**
 * mlo_mgr_link_switch_complete() - Link switch complete notification to FW
 * @vdev: VDV object manager
 *
 * Notify the status of link switch to FW once the link switch sequence is
 * completed.
 *
 * Return: QDF_STATUS;
 */
QDF_STATUS mlo_mgr_link_switch_complete(struct wlan_objmgr_vdev *vdev);

#ifdef WLAN_FEATURE_11BE_MLO_ADV_FEATURE
/**
 * mlo_mgr_link_switch_init() - API to initialize link switch
 * @ml_dev: MLO dev context
 *
 * Initializes the MLO link context in @ml_dev and allocates various
 * buffers needed.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS mlo_mgr_link_switch_init(struct wlan_mlo_dev_context *ml_dev);

/**
 * mlo_mgr_link_switch_deinit() - API to de-initialize link switch
 * @ml_dev: MLO dev context
 *
 * De-initialize the MLO link context in @ml_dev on and frees memory
 * allocated as part of initialization.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS mlo_mgr_link_switch_deinit(struct wlan_mlo_dev_context *ml_dev);
#else
static inline QDF_STATUS
mlo_mgr_link_switch_deinit(struct wlan_mlo_dev_context *ml_dev)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS
mlo_mgr_link_switch_init(struct wlan_mlo_dev_context *ml_dev)
{
	return QDF_STATUS_SUCCESS;
}
#endif
#endif
