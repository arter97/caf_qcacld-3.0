/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

#ifndef _WLAN_DP_RESOURCE_MGR_H_
#define _WLAN_DP_RESOURCE_MGR_H_

#include "wlan_dp_priv.h"

/*Max throughput(Mbps) supported in different resource levels*/
#define RESOURCE_LVL_1_TPUT_MBPS  2400
#define RESOURCE_LVL_2_TPUT_MBPS  5100

/*RX buffers required in different resource levels*/
#define RESOURCE_LVL_1_RX_BUFFERS (6 * 1024)
#define RESOURCE_LVL_2_RX_BUFFERS (16 * 1024)

/**
 * enum wlan_dp_resource_level - DP Resource levels
 * @RESOURCE_LVL_1: Default Resource level
 * @RESOURCE_LVL_2: Higher resource level in peak tput
 * @RESOURCE_LVL_MAX: MAX resource level undefined
 */
enum wlan_dp_resource_level {
	RESOURCE_LVL_1,
	RESOURCE_LVL_2,
	RESOURCE_LVL_MAX
};

/**
 * struct wlan_dp_resource_map - DP Resource map config
 * @resource_level: Resource level
 * @max_tput: MAX throughput current level supports
 * @num_rx_buffers: MAX number of RX buffers required
 */
struct wlan_dp_resource_map {
	enum wlan_dp_resource_level resource_level;
	uint16_t max_tput;
	uint16_t num_rx_buffers;
};

/*DP resource MAP used in resource level selection*/
static struct wlan_dp_resource_map dp_resource_map[] = {
	{RESOURCE_LVL_1, RESOURCE_LVL_1_TPUT_MBPS, RESOURCE_LVL_1_RX_BUFFERS},
	{RESOURCE_LVL_2, RESOURCE_LVL_2_TPUT_MBPS, RESOURCE_LVL_2_RX_BUFFERS},
};

/**
 * struct wlan_dp_resource_vote_node - DP Resource vote node context
 * @priv_ctx: DP peer priv context ref
 * @peer: OBJMGR peer reference
 * @self_mac: Self MAC address
 * @remote_mac: Remote MAC address
 * @phymode: phymode it can operate
 * @opmode: Operating mode of vdev
 * @node: List node entry
 * @vdev_id: Vdev id
 * @mac_id: HW MAC id
 */
struct wlan_dp_resource_vote_node {
	struct wlan_dp_peer_priv_context *priv_ctx;
	struct wlan_objmgr_peer *peer;
	struct qdf_mac_addr self_mac;
	struct qdf_mac_addr remote_mac;
	enum wlan_phymode phymode;
	enum QDF_OPMODE opmode;
	qdf_list_node_t node;
	uint8_t vdev_id;
	uint8_t mac_id;
};

/**
 * struct wlan_dp_resource_mgr_ctx - DP Resource manager context
 * @cur_rsrc_map: Current DP resource map
 */
struct wlan_dp_resource_mgr_ctx {
	struct wlan_dp_resource_map cur_rsrc_map[RESOURCE_LVL_MAX];
};

#ifdef WLAN_DP_DYNAMIC_RESOURCE_MGMT
/**
 * wlan_dp_resource_mgr_attach() - Attach DP resource manager
 * @dp_ctx: pointer to the DP context
 *
 * Return: None
 */
void wlan_dp_resource_mgr_attach(struct wlan_dp_psoc_context *dp_ctx);

/**
 * wlan_dp_resource_mgr_detach: Detach DP resource manager
 * @dp_ctx: DP context pointer
 *
 * Return: None
 */
void wlan_dp_resource_mgr_detach(struct wlan_dp_psoc_context *dp_ctx);
#else

static inline
void wlan_dp_resource_mgr_attach(struct wlan_dp_psoc_context *dp_ctx)
{
}

static inline
void wlan_dp_resource_mgr_detach(struct wlan_dp_psoc_context *dp_ctx)
{
}
#endif /*WLAN_DP_DYNAMIC_RESOURCE_MGMT*/

#endif /*_WLAN_DP_RESOURCE_MGR_H_*/
