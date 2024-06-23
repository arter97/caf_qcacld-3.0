/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

#include <wlan_objmgr_pdev_obj.h>
#include <wlan_dp_main.h>
#include <wlan_dp_priv.h>
#include <wlan_dp_resource_mgr.h>
#include "wlan_objmgr_vdev_obj_i.h"
#include "wlan_mlo_mgr_peer.h"
#include "wlan_mlme_main.h"
#include <nan_ucfg_api.h>
#include <wma.h>
#include <cdp_txrx_cmn.h>
#include <cdp_txrx_ctrl.h>

static uint64_t
wlan_dp_resource_mgr_phymode_to_tput(enum wlan_phymode phymode)
{
	switch (phymode) {
	case WLAN_PHYMODE_11A:
		return WLAN_PHYMODE_11A_TPUT_MBPS;
	case WLAN_PHYMODE_11B:
		return WLAN_PHYMODE_11B_TPUT_MBPS;
	case WLAN_PHYMODE_11G:
		return WLAN_PHYMODE_11G_TPUT_MBPS;
	case WLAN_PHYMODE_11G_ONLY:
		return WLAN_PHYMODE_11G_ONLY_TPUT_MBPS;
	case WLAN_PHYMODE_11NA_HT20:
		return WLAN_PHYMODE_11NA_HT20_TPUT_MBPS;
	case WLAN_PHYMODE_11NG_HT20:
		return WLAN_PHYMODE_11NG_HT20_TPUT_MBPS;
	case WLAN_PHYMODE_11NA_HT40:
		return WLAN_PHYMODE_11NA_HT40_TPUT_MBPS;
	case WLAN_PHYMODE_11NG_HT40PLUS:
		return WLAN_PHYMODE_11NG_HT40PLUS_TPUT_MBPS;
	case WLAN_PHYMODE_11NG_HT40MINUS:
		return WLAN_PHYMODE_11NG_HT40MINUS_TPUT_MBPS;
	case WLAN_PHYMODE_11NG_HT40:
		return WLAN_PHYMODE_11NG_HT40_TPUT_MBPS;
	case WLAN_PHYMODE_11AC_VHT20:
		return WLAN_PHYMODE_11AC_VHT20_TPUT_MBPS;
	case WLAN_PHYMODE_11AC_VHT20_2G:
		return WLAN_PHYMODE_11AC_VHT20_2G_TPUT_MBPS;
	case WLAN_PHYMODE_11AC_VHT40:
		return WLAN_PHYMODE_11AC_VHT40_TPUT_MBPS;
	case WLAN_PHYMODE_11AC_VHT40PLUS_2G:
		return WLAN_PHYMODE_11AC_VHT40PLUS_2G_TPUT_MBPS;
	case WLAN_PHYMODE_11AC_VHT40MINUS_2G:
		return WLAN_PHYMODE_11AC_VHT40MINUS_2G_TPUT_MBPS;
	case WLAN_PHYMODE_11AC_VHT40_2G:
		return WLAN_PHYMODE_11AC_VHT40_2G_TPUT_MBPS;
	case WLAN_PHYMODE_11AC_VHT80:
		return WLAN_PHYMODE_11AC_VHT80_TPUT_MBPS;
	case WLAN_PHYMODE_11AC_VHT80_2G:
		return WLAN_PHYMODE_11AC_VHT80_2G_TPUT_MBPS;
	case WLAN_PHYMODE_11AC_VHT160:
		return WLAN_PHYMODE_11AC_VHT160_TPUT_MBPS;
	case WLAN_PHYMODE_11AC_VHT80_80:
		return WLAN_PHYMODE_11AC_VHT80_80_TPUT_MBPS;
	case WLAN_PHYMODE_11AXA_HE20:
		return WLAN_PHYMODE_11AXA_HE20_TPUT_MBPS;
	case WLAN_PHYMODE_11AXG_HE20:
		return WLAN_PHYMODE_11AXG_HE20_TPUT_MBPS;
	case WLAN_PHYMODE_11AXA_HE40:
		return WLAN_PHYMODE_11AXA_HE40_TPUT_MBPS;
	case WLAN_PHYMODE_11AXG_HE40PLUS:
		return WLAN_PHYMODE_11AXG_HE40PLUS_TPUT_MBPS;
	case WLAN_PHYMODE_11AXG_HE40MINUS:
		return WLAN_PHYMODE_11AXG_HE40MINUS_TPUT_MBPS;
	case WLAN_PHYMODE_11AXG_HE40:
		return WLAN_PHYMODE_11AXG_HE40_TPUT_MBPS;
	case WLAN_PHYMODE_11AXA_HE80:
		return WLAN_PHYMODE_11AXA_HE80_TPUT_MBPS;
	case WLAN_PHYMODE_11AXG_HE80:
		return WLAN_PHYMODE_11AXG_HE80_TPUT_MBPS;
	case WLAN_PHYMODE_11AXA_HE160:
		return WLAN_PHYMODE_11AXA_HE160_TPUT_MBPS;
	case WLAN_PHYMODE_11AXA_HE80_80:
		return WLAN_PHYMODE_11AXA_HE80_80_TPUT_MBPS;
#ifdef WLAN_FEATURE_11BE
	case WLAN_PHYMODE_11BEA_EHT20:
		return WLAN_PHYMODE_11BEA_EHT20_TPUT_MBPS;
	case WLAN_PHYMODE_11BEG_EHT20:
		return WLAN_PHYMODE_11BEG_EHT20_TPUT_MBPS;
	case WLAN_PHYMODE_11BEA_EHT40:
		return WLAN_PHYMODE_11BEA_EHT40_TPUT_MBPS;
	case WLAN_PHYMODE_11BEG_EHT40PLUS:
		return WLAN_PHYMODE_11BEG_EHT40PLUS_TPUT_MBPS;
	case WLAN_PHYMODE_11BEG_EHT40MINUS:
		return WLAN_PHYMODE_11BEG_EHT40MINUS_TPUT_MBPS;
	case WLAN_PHYMODE_11BEG_EHT40:
		return WLAN_PHYMODE_11BEG_EHT40_TPUT_MBPS;
	case WLAN_PHYMODE_11BEA_EHT80:
		return WLAN_PHYMODE_11BEA_EHT80_TPUT_MBPS;
	case WLAN_PHYMODE_11BEG_EHT80:
		return WLAN_PHYMODE_11BEG_EHT80_TPUT_MBPS;
	case WLAN_PHYMODE_11BEA_EHT160:
		return WLAN_PHYMODE_11BEA_EHT160_TPUT_MBPS;
	case WLAN_PHYMODE_11BEA_EHT320:
		return WLAN_PHYMODE_11BEA_EHT320_TPUT_MBPS;
#endif
	default:
		return WLAN_PHYMODE_DEFAULT_TPUT_MBPS;
	}
}

static uint32_t
wlan_dp_resource_mgr_get_mac_id(struct wlan_objmgr_vdev *vdev)
{
	struct mlme_legacy_priv *mlme_priv;
	uint32_t mac_id = 0;

	mlme_priv = wlan_vdev_mlme_get_ext_hdl(vdev);
	if (!mlme_priv) {
		dp_err("Unable to get mlme priv");
		return mac_id;
	}

	if (mlme_priv->mac_id < MAX_MAC_RESOURCES)
		mac_id = mlme_priv->mac_id;

	return mac_id;
}

static void
wlan_dp_resource_mgr_find_max_mac_phymodes(
			struct wlan_dp_resource_mgr_ctx *rsrc_ctx,
			struct wlan_dp_resource_vote_node **max_mac_phymodes)
{
	struct wlan_dp_resource_vote_node *vote_node_cur;
	qdf_list_node_t *cur_node = NULL;
	QDF_STATUS status;
	int i;

	for (i = 0; i < rsrc_ctx->mac_count; i++) {
		status = qdf_list_peek_front(&rsrc_ctx->mac_list[i], &cur_node);
		if (QDF_IS_STATUS_SUCCESS(status)) {
			vote_node_cur = qdf_container_of(cur_node,
				struct wlan_dp_resource_vote_node, node);
			max_mac_phymodes[i] = vote_node_cur;
		}
	}
}

static void
wlan_dp_resource_mgr_sort_phymodes(struct wlan_dp_resource_vote_node **phymodes,
				   uint32_t mac_count)
{
	struct wlan_dp_resource_vote_node *vote_node;
	int i = 0, j;

	for (i = 0; i < mac_count; i++) {
		for (j = i + 1; j < mac_count; j++) {
			if (phymodes[i] && phymodes[j]) {
				/*Both vote nodes present normal sort*/
				if (phymodes[i]->tput > phymodes[j]->tput) {
					vote_node =  phymodes[i];
					phymodes[i] = phymodes[j];
					phymodes[j] = vote_node;
				}
			} else if (!phymodes[i]) {
				/*No vote node on this entry sort*/
				phymodes[i] = phymodes[j];
				phymodes[j] = NULL;
			}
		}
	}
}

static void
wlan_dp_resource_mgr_select_resource_level(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx)
{
	struct wlan_dp_resource_vote_node *vote_node;
	uint64_t total_tput = 0;
	enum wlan_dp_resource_level prev_level;
	int i;

	dp_rsrc_mgr_debug("Select resource level called");
	for (i = 0; i < rsrc_ctx->mac_count; i++) {
		vote_node = rsrc_ctx->max_phymode_nodes[i];
		if (vote_node)
			total_tput += vote_node->tput;
	}

	for (i = 0; i < RESOURCE_LVL_MAX; i++)
		if (total_tput <= rsrc_ctx->cur_rsrc_map[i].max_tput)
			break;

	/*Level overflow reset to max supported*/
	if (i >= RESOURCE_LVL_MAX)
		i = (RESOURCE_LVL_MAX - 1);

	prev_level = rsrc_ctx->cur_resource_level;
	rsrc_ctx->cur_resource_level = i;
	rsrc_ctx->cur_max_tput = total_tput;
	if (prev_level < rsrc_ctx->cur_resource_level)
		wlan_dp_resource_mgr_post_upscale_resource_req(rsrc_ctx);
	else if (prev_level > rsrc_ctx->cur_resource_level)
		wlan_dp_resource_mgr_post_downscale_resource_req(rsrc_ctx);
	else
		dp_info("Resource level change not required");

	dp_rsrc_mgr_debug("Resource level:%u selected for tput:%u req_tput:%u",
			  rsrc_ctx->cur_resource_level, rsrc_ctx->cur_max_tput,
			  total_tput);
}

static void
wlan_dp_resource_mgr_select_max_phymodes(
			struct wlan_dp_resource_mgr_ctx *rsrc_ctx)
{
	struct wlan_dp_resource_vote_node *max_mac_phymodes[MAX_MAC_RESOURCES] = {NULL};
	struct wlan_dp_resource_vote_node *max_mac_n_phymodes[MAX_MAC_RESOURCES] = {NULL};
	struct wlan_dp_resource_vote_node *max_nan_phymodes[MAX_MAC_RESOURCES] = {NULL};
	uint32_t mac_count = rsrc_ctx->mac_count;
	int i, j, k;

	dp_rsrc_mgr_debug("Select max phymode called");

	wlan_dp_resource_mgr_find_nan_max_phymodes(rsrc_ctx, max_nan_phymodes);
	wlan_dp_resource_mgr_find_max_mac_phymodes(rsrc_ctx, max_mac_phymodes);
	wlan_dp_resource_mgr_find_max_mac_n_phymodes(rsrc_ctx,
						     max_mac_n_phymodes);

	qdf_mem_set(&rsrc_ctx->max_phymode_nodes, sizeof(rsrc_ctx->max_phymode_nodes), 0);
	/*MAX MAC based phymodes including NAN peers*/
	for (i = 0; i < mac_count; i++)
		max_mac_phymodes[i] =
			wlan_dp_resource_mgr_max_of_two_vote_nodes(
							max_mac_phymodes[i],
							max_nan_phymodes[i]);
	wlan_dp_resource_mgr_sort_phymodes(max_mac_phymodes, mac_count);
	if (qdf_list_empty(&rsrc_ctx->mac_n_list)) {
		qdf_mem_copy(&rsrc_ctx->max_phymode_nodes, &max_mac_phymodes,
			     (sizeof(rsrc_ctx->max_phymode_nodes[0]) * mac_count));
		dp_rsrc_mgr_debug("ML STA list empty");
		goto select_resource_level;
	}

	/*ML-STA connections present include them in resource selection*/
	i = j = k = 0;
	while (i < mac_count && j < mac_count) {
		/*If both nodes are present normal merge sort scenario*/
		if (max_mac_phymodes[i] && max_mac_n_phymodes[j]) {
			if (max_mac_phymodes[i]->tput > max_mac_n_phymodes[j]->tput) {
				rsrc_ctx->max_phymode_nodes[k] =
					max_mac_phymodes[i];
				i++;
			} else {
				rsrc_ctx->max_phymode_nodes[k] =
					max_mac_n_phymodes[j];
				j++;
			}
			k++;
			continue;
		} else if (max_mac_phymodes[i]) {
			/*Fetch entries from non null array*/
			qdf_mem_copy(&rsrc_ctx->max_phymode_nodes[k],
				     &max_mac_phymodes[i],
				sizeof(rsrc_ctx->max_phymode_nodes[k]) * (mac_count - k));
		} else if (max_mac_n_phymodes[j]) {
			qdf_mem_copy(&rsrc_ctx->max_phymode_nodes[k],
				     &max_mac_n_phymodes[j],
				sizeof(rsrc_ctx->max_phymode_nodes[k]) * (mac_count - k));
		}
		break;
	}
	dp_rsrc_mgr_debug("ML STA phymodes considered i:%u j:%u k:%u", i, j, k);

select_resource_level:
	wlan_dp_resource_mgr_select_resource_level(rsrc_ctx);
}

static void
wlan_dp_resource_mgr_list_insert_vote_node(qdf_list_t *mac_list,
				struct wlan_dp_resource_vote_node *vote_node,
				enum QDF_OPMODE opmode)
{
	struct wlan_dp_resource_vote_node *vote_node_cur;
	qdf_list_node_t *cur_node = NULL, *next_node = NULL;
	QDF_STATUS status;

	status = qdf_list_peek_front(mac_list, &cur_node);
	while (QDF_IS_STATUS_SUCCESS(status)) {
		vote_node_cur = qdf_container_of(cur_node,
				struct wlan_dp_resource_vote_node, node);

		if (opmode == QDF_NDI_MODE) {
			/*NAN case sort based on MAC0 tput*/
			if (vote_node->tput0 > vote_node_cur->tput0) {
				qdf_list_insert_before(mac_list,
						       &vote_node->node,
						       &vote_node_cur->node);
				break;
			}
		} else {
			if (vote_node->tput > vote_node_cur->tput) {
				qdf_list_insert_before(mac_list,
						       &vote_node->node,
						       &vote_node_cur->node);
				break;
			}
		}

		status = qdf_list_peek_next(mac_list, cur_node, &next_node);
		cur_node = next_node;
		next_node = NULL;
	}

	if (!cur_node)
		qdf_list_insert_back(mac_list, &vote_node->node);
	dp_rsrc_mgr_debug("vote node inserted to list for opmode:%u", opmode);
}

static struct wlan_dp_resource_vote_node *
wlan_dp_resource_mgr_add_vote_node(struct wlan_dp_resource_mgr_ctx *rsrc_ctx,
				   struct wlan_objmgr_peer *peer,
				   enum wlan_phymode phymode,
				   enum QDF_OPMODE opmode)
{
	struct wlan_dp_resource_vote_node *vote_node;
	qdf_list_t *mac_list;
	uint32_t mac_id;

	vote_node = qdf_mem_malloc(sizeof(*vote_node));
	if (!vote_node) {
		dp_err("Failed to allocate memory for resource vote node");
		return NULL;
	}

	vote_node->phymode = phymode;
	vote_node->tput = wlan_dp_resource_mgr_phymode_to_tput(phymode);
	mac_id = wlan_dp_resource_mgr_get_mac_id(wlan_peer_get_vdev(peer));
	if (mac_id > MAX_MAC_RESOURCES) {
		dp_err("MAC id is not valid");
		QDF_BUG(0);
	}
	vote_node->mac_id = mac_id;
	mac_list = &rsrc_ctx->mac_list[mac_id];
	wlan_dp_resource_mgr_list_insert_vote_node(mac_list, vote_node, opmode);
	dp_rsrc_mgr_debug("New vote node added to list mac_id:%u len:%u phymode:%u",
			  vote_node->mac_id, qdf_list_size(mac_list),
			  vote_node->phymode);

	return vote_node;
}

static void
wlan_dp_resource_mgr_phymode_update(struct wlan_objmgr_peer *peer, void *arg)
{
	struct wlan_dp_resource_mgr_ctx *rsrc_ctx =
		(struct wlan_dp_resource_mgr_ctx *)arg;
	struct wlan_dp_peer_priv_context *priv_ctx;
	struct wlan_dp_resource_vote_node *vote_node;
	struct wlan_objmgr_peer *vote_peer = peer;
	enum wlan_phymode vote_phymode = wlan_peer_get_phymode(peer);
	struct  wlan_objmgr_vdev *vdev;
	struct wlan_channel *desc_chan;
	enum QDF_OPMODE opmode;
	uint64_t prev_tput;

	vdev = wlan_peer_get_vdev(peer);
	opmode = wlan_vdev_mlme_get_opmode(vdev);

	dp_rsrc_mgr_debug("update phymode called for opmode:%u", opmode);

	/*NAN peers handled by NDP events*/
	if (opmode == QDF_NDI_MODE)
		return;

	if ((opmode == QDF_STA_MODE) && wlan_peer_is_mlo(peer))
		return wlan_dp_resource_mgr_handle_ml_sta_phymode_update(
							rsrc_ctx, peer);

	if ((opmode == QDF_SAP_MODE) || (opmode == QDF_P2P_GO_MODE)) {
		/*
		 * For SAP resource vote node is attached to self peer,
		 * And also fetch phymode from vdev desc_chan.
		 */
		vote_peer = wlan_vdev_get_selfpeer(peer->peer_objmgr.vdev);
		if (!vote_peer) {
			dp_err("SAP self peer not present");
			return;
		}
		desc_chan = wlan_vdev_mlme_get_des_chan(vdev);
		vote_phymode = desc_chan->ch_phymode;
	}

	priv_ctx = dp_get_peer_priv_obj(vote_peer);
	if (priv_ctx->vote_node) {
		vote_node = priv_ctx->vote_node;
		if (vote_node->phymode == vote_phymode) {
			dp_info("same phymode update for opmode:%u", opmode);
			return;
		}

		prev_tput = vote_node->tput;
		vote_node->phymode = vote_phymode;
		vote_node->tput =
			wlan_dp_resource_mgr_phymode_to_tput(vote_phymode);

		/*
		 * Removing from the sorted list, since phymode changed.
		 * Adding to list in based on new phymode throughput value.
		 */
		qdf_list_remove_node(&rsrc_ctx->mac_list[vote_node->mac_id],
				     &vote_node->node);
		wlan_dp_resource_mgr_list_insert_vote_node(
			&rsrc_ctx->mac_list[vote_node->mac_id],
			vote_node, opmode);

		dp_rsrc_mgr_debug("phymode update for existing vote node phymode:%u", vote_node->phymode);
		/*
		 * Check if current resource vote node change increase
		 * overall system tput requirement.
		 */
		if (wlan_dp_resource_mgr_detect_tput_level_increase(rsrc_ctx,
								vote_node,
								prev_tput)) {
			wlan_dp_resource_mgr_select_resource_level(rsrc_ctx);
			return;
		}
		goto select_max_phymodes;
	}

	priv_ctx->vote_node =
		wlan_dp_resource_mgr_add_vote_node(rsrc_ctx,
						   vote_peer, vote_phymode,
						   opmode);
select_max_phymodes:
	wlan_dp_resource_mgr_select_max_phymodes(rsrc_ctx);
}

void
wlan_dp_resource_mgr_vote_node_free(struct wlan_objmgr_peer *peer)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_get_context();
	struct wlan_dp_resource_mgr_ctx *rsrc_ctx = dp_ctx->rsrc_mgr_ctx;
	struct wlan_dp_peer_priv_context *priv_ctx;
	struct wlan_dp_resource_vote_node *vote_node;
	struct wlan_objmgr_peer *vote_peer = peer;
	struct  wlan_objmgr_vdev *vdev;
	enum QDF_OPMODE opmode;

	vdev = wlan_peer_get_vdev(peer);
	opmode = wlan_vdev_mlme_get_opmode(vdev);

	dp_rsrc_mgr_debug("vote node free called for opmode:%u", opmode);

	if ((opmode == QDF_SAP_MODE) || (opmode == QDF_P2P_GO_MODE)) {
		/* Check for active clients connected to SAP*/
		if (wlan_vdev_get_peer_count(vdev) > 2)
			return;
		vote_peer = wlan_vdev_get_selfpeer(peer->peer_objmgr.vdev);
		if (!vote_peer) {
			dp_err("SAP self peer not present");
			return;
		}
	}

	priv_ctx = dp_get_peer_priv_obj(vote_peer);
	if (priv_ctx->vote_node) {
		vote_node = priv_ctx->vote_node;
		if (opmode == QDF_NDI_MODE)
			qdf_list_remove_node(&rsrc_ctx->nan_list,
					     &vote_node->node);
		else
			qdf_list_remove_node(
				&rsrc_ctx->mac_list[vote_node->mac_id],
				&vote_node->node);

		dp_rsrc_mgr_debug("vote node freed on vdev_id:%u mac_id:%u list_len:%u",
				  wlan_vdev_get_id(vdev), vote_node->mac_id,
				  qdf_list_size(&rsrc_ctx->mac_list[vote_node->mac_id]));
		qdf_mem_free(vote_node);
		priv_ctx->vote_node = NULL;
		wlan_dp_resource_mgr_select_max_phymodes(rsrc_ctx);
	} else if (opmode == QDF_STA_MODE)
		return wlan_dp_resource_mgr_handle_ml_sta_remove_phymode(
								rsrc_ctx, peer);
}

static void
wlan_dp_resource_mgr_list_attach(struct wlan_dp_resource_mgr_ctx *rsrc_ctx)
{
	int i;

	qdf_list_create(&rsrc_ctx->mac_n_list, 0);
	qdf_list_create(&rsrc_ctx->nan_list, 0);
	for (i = 0; i < MAX_MAC_RESOURCES; i++)
		qdf_list_create(&rsrc_ctx->mac_list[i], 0);
}

static void
wlan_dp_resource_mgr_list_detach(struct wlan_dp_resource_mgr_ctx *rsrc_ctx)
{
	int i;

	qdf_list_destroy(&rsrc_ctx->mac_n_list);
	qdf_list_destroy(&rsrc_ctx->nan_list);
	for (i = 0; i < MAX_MAC_RESOURCES; i++)
		qdf_list_destroy(&rsrc_ctx->mac_list[i]);
}

void
wlan_dp_resource_mgr_set_req_resources(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx)
{
	enum wlan_dp_resource_level rsrc_level;
	uint64_t req_rx_buff_descs;
	QDF_STATUS status;

	if (!rsrc_ctx) {
		dp_err("DP resource mgr context not present");
		return;
	}

	rsrc_level = rsrc_ctx->cur_resource_level;
	req_rx_buff_descs = rsrc_ctx->cur_rsrc_map[rsrc_level].num_rx_buffers;

	status = cdp_set_req_buff_descs(rsrc_ctx->dp_ctx->cdp_soc,
					req_rx_buff_descs, 0);
	if (QDF_IS_STATUS_ERROR(status)) {
		wlan_dp_resource_mgr_detach(rsrc_ctx->dp_ctx);
		dp_err("Unable to set req DP rx desc");
	}
	dp_rsrc_mgr_debug("req_rx_buffer_desc:%u set", req_rx_buff_descs);
}

void wlan_dp_resource_mgr_attach(struct wlan_dp_psoc_context *dp_ctx)
{
	struct wlan_dp_resource_mgr_ctx *rsrc_ctx;
	struct cdp_soc_t *cdp_soc = dp_ctx->cdp_soc;
	cdp_config_param_type val = {0};
	QDF_STATUS status;
	int i;

	rsrc_ctx = qdf_mem_malloc(sizeof(*rsrc_ctx));
	if (!rsrc_ctx) {
		dp_err("Failed to create DP resource mgr context");
		return;
	}
	qdf_mem_copy(rsrc_ctx->cur_rsrc_map, dp_resource_map,
		     (sizeof(struct wlan_dp_resource_map) * RESOURCE_LVL_MAX));

	status = cdp_txrx_get_psoc_param(cdp_soc,
					 CDP_CFG_RXDMA_REFILL_RING_SIZE, &val);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("Failed to fetch Refill ring config status:%u", status);
		goto attach_err;
	}

	/*
	 * Config value is less than default resource map, then
	 * do not enable resource mgr, since resource mgr cannot
	 * operated on less than default resources.
	 * If config value greater than default resources, than
	 * use that value for max reource level selection.
	 */
	if (val.cdp_rxdma_refill_ring_size <=
	    dp_resource_map[RESOURCE_LVL_1].num_rx_buffers) {
		dp_info("Unsupported RX buffers config:%u, disabling rsrc mgr",
			val.cdp_rxdma_refill_ring_size);
		goto attach_err;
	} else if ((val.cdp_rxdma_refill_ring_size - 1) !=
		   dp_resource_map[RESOURCE_LVL_2].num_rx_buffers) {
		dp_info("Adjusting Resource lvl_2 rx buffers map_val:%u cfg_val:%u",
			dp_resource_map[RESOURCE_LVL_2].num_rx_buffers,
			val.cdp_rxdma_refill_ring_size);
		rsrc_ctx->cur_rsrc_map[RESOURCE_LVL_2].num_rx_buffers =
			(val.cdp_rxdma_refill_ring_size - 1);
	}

	for (i = 0; i < RESOURCE_LVL_MAX; i++) {
		dp_info("DP resource level:%u MAX_TPUT:%u NUM_RX_BUF:%u",
			i, rsrc_ctx->cur_rsrc_map[i].max_tput,
			rsrc_ctx->cur_rsrc_map[i].num_rx_buffers);
	}

	status = wlan_objmgr_register_peer_phymode_change_notify_handler(
					WLAN_COMP_DP,
					wlan_dp_resource_mgr_phymode_update,
					rsrc_ctx);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("Failed to register for peer phymode change notif");
		goto attach_err;
	}

	wlan_dp_resource_mgr_list_attach(rsrc_ctx);

	rsrc_ctx->mac_count = MAX_MAC_RESOURCES;

	rsrc_ctx->dp_ctx = dp_ctx;
	dp_ctx->rsrc_mgr_ctx = rsrc_ctx;

	dp_info("DP resource mgr attach done mac_count:%u",
		rsrc_ctx->mac_count);
	return;

attach_err:
	dp_err("DP resource mgr attach failure");
	qdf_mem_free(rsrc_ctx);
}

void wlan_dp_resource_mgr_detach(struct wlan_dp_psoc_context *dp_ctx)
{
	QDF_STATUS status;

	dp_info("DP resource mgr detaching %s",
		dp_ctx->rsrc_mgr_ctx ? "true" : "false");
	if (dp_ctx->rsrc_mgr_ctx) {
		status =
		wlan_objmgr_unregister_peer_phymode_change_notify_handler(
					WLAN_COMP_DP,
					wlan_dp_resource_mgr_phymode_update,
					dp_ctx->rsrc_mgr_ctx);
		if (QDF_IS_STATUS_ERROR(status)) {
			dp_err("Failed to unregister peer phymode notification handler");
			return;
		}

		wlan_dp_resource_mgr_list_detach(dp_ctx->rsrc_mgr_ctx);
		qdf_mem_free(dp_ctx->rsrc_mgr_ctx);
		dp_ctx->rsrc_mgr_ctx = NULL;
	}
}
