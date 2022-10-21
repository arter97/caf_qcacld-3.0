/*
 * Copyright (c) 2012-2015, 2020-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

/**
 * DOC: wlan_cm_api.c
 *
 * This file maintains definitaions public apis.
 */

#include <wlan_cm_api.h>
#include "connection_mgr/core/src/wlan_cm_main_api.h"
#include "connection_mgr/core/src/wlan_cm_roam.h"
#include <wlan_vdev_mgr_utils_api.h>

QDF_STATUS wlan_cm_start_connect(struct wlan_objmgr_vdev *vdev,
				 struct wlan_cm_connect_req *req)
{
	return cm_connect_start_req(vdev, req);
}

QDF_STATUS wlan_cm_disconnect(struct wlan_objmgr_vdev *vdev,
			      enum wlan_cm_source source,
			      enum wlan_reason_code reason_code,
			      struct qdf_mac_addr *bssid)
{
	struct wlan_cm_disconnect_req req = {0};

	req.vdev_id = wlan_vdev_get_id(vdev);
	req.source = source;
	req.reason_code = reason_code;
	if (bssid)
		qdf_copy_macaddr(&req.bssid, bssid);

	return cm_disconnect_start_req(vdev, &req);
}

QDF_STATUS wlan_cm_disconnect_sync(struct wlan_objmgr_vdev *vdev,
				   enum wlan_cm_source source,
				   enum wlan_reason_code reason_code)
{
	struct wlan_cm_disconnect_req req = {0};

	req.vdev_id = wlan_vdev_get_id(vdev);
	req.source = source;
	req.reason_code = reason_code;

	return cm_disconnect_start_req_sync(vdev, &req);
}

QDF_STATUS wlan_cm_bss_select_ind_rsp(struct wlan_objmgr_vdev *vdev,
				      QDF_STATUS status)
{
	return cm_bss_select_ind_rsp(vdev, status);
}

QDF_STATUS wlan_cm_bss_peer_create_rsp(struct wlan_objmgr_vdev *vdev,
				       QDF_STATUS status,
				       struct qdf_mac_addr *peer_mac)
{
	uint32_t prefix;
	struct cnx_mgr *cm_ctx = cm_get_cm_ctx(vdev);

	if (!cm_ctx)
		return QDF_STATUS_E_INVAL;

	prefix = CM_ID_GET_PREFIX(cm_ctx->active_cm_id);
	if (prefix == ROAM_REQ_PREFIX)
		return cm_roam_bss_peer_create_rsp(vdev, status, peer_mac);
	else
		return cm_bss_peer_create_rsp(vdev, status, peer_mac);
}

QDF_STATUS wlan_cm_connect_rsp(struct wlan_objmgr_vdev *vdev,
			       struct wlan_cm_connect_resp *resp)
{
	return cm_connect_rsp(vdev, resp);
}

QDF_STATUS wlan_cm_bss_peer_delete_ind(struct wlan_objmgr_vdev *vdev,
				       struct qdf_mac_addr *peer_mac)
{
	return cm_bss_peer_delete_req(vdev, peer_mac);
}

QDF_STATUS wlan_cm_bss_peer_delete_rsp(struct wlan_objmgr_vdev *vdev,
				       uint32_t status)
{
	return cm_vdev_down_req(vdev, status);
}

QDF_STATUS wlan_cm_disconnect_rsp(struct wlan_objmgr_vdev *vdev,
				  struct wlan_cm_discon_rsp *resp)
{
	uint32_t prefix;
	struct cnx_mgr *cm_ctx = cm_get_cm_ctx(vdev);

	if (!cm_ctx)
		return QDF_STATUS_E_INVAL;

	prefix = CM_ID_GET_PREFIX(cm_ctx->active_cm_id);
	if (prefix == ROAM_REQ_PREFIX)
		return cm_roam_disconnect_rsp(vdev, resp);
	else
		return cm_disconnect_rsp(vdev, resp);
}

#ifdef WLAN_FEATURE_HOST_ROAM
QDF_STATUS wlan_cm_reassoc_rsp(struct wlan_objmgr_vdev *vdev,
			       struct wlan_cm_connect_resp *resp)
{
	return cm_reassoc_rsp(vdev, resp);
}
#endif

void wlan_cm_set_max_connect_attempts(struct wlan_objmgr_vdev *vdev,
				      uint8_t max_connect_attempts)
{
	cm_set_max_connect_attempts(vdev, max_connect_attempts);
}

void wlan_cm_set_max_connect_timeout(struct wlan_objmgr_vdev *vdev,
				     uint32_t max_connect_timeout)
{
	cm_set_max_connect_timeout(vdev, max_connect_timeout);
}

bool wlan_cm_is_vdev_connecting(struct wlan_objmgr_vdev *vdev)
{
	return cm_is_vdev_connecting(vdev);
}

bool wlan_cm_is_vdev_connected(struct wlan_objmgr_vdev *vdev)
{
	return cm_is_vdev_connected(vdev);
}

bool wlan_cm_is_vdev_active(struct wlan_objmgr_vdev *vdev)
{
	return cm_is_vdev_active(vdev);
}

bool wlan_cm_is_vdev_disconnecting(struct wlan_objmgr_vdev *vdev)
{
	return cm_is_vdev_disconnecting(vdev);
}

bool wlan_cm_is_vdev_disconnected(struct wlan_objmgr_vdev *vdev)
{
	return cm_is_vdev_disconnected(vdev);
}

bool wlan_cm_is_vdev_roaming(struct wlan_objmgr_vdev *vdev)
{
	return cm_is_vdev_roaming(vdev);
}

#ifdef WLAN_FEATURE_ROAM_OFFLOAD
bool wlan_cm_is_vdev_roam_started(struct wlan_objmgr_vdev *vdev)
{
	return cm_is_vdev_roam_started(vdev);
}

bool wlan_cm_is_vdev_roam_sync_inprogress(struct wlan_objmgr_vdev *vdev)
{
	return cm_is_vdev_roam_sync_inprogress(vdev);
}
#endif

#ifdef WLAN_FEATURE_HOST_ROAM
bool wlan_cm_is_vdev_roam_preauth_state(struct wlan_objmgr_vdev *vdev)
{
	return cm_is_vdev_roam_preauth_state(vdev);
}

bool wlan_cm_is_vdev_roam_reassoc_state(struct wlan_objmgr_vdev *vdev)
{
	return cm_is_vdev_roam_reassoc_state(vdev);
}
#endif

enum wlan_cm_active_request_type
wlan_cm_get_active_req_type(struct wlan_objmgr_vdev *vdev)
{
	return cm_get_active_req_type(vdev);
}

bool wlan_cm_get_active_connect_req(struct wlan_objmgr_vdev *vdev,
				    struct wlan_cm_vdev_connect_req *req)
{
	return cm_get_active_connect_req(vdev, req);
}

cm_ext_t *wlan_cm_get_ext_hdl(struct wlan_objmgr_vdev *vdev)
{
	return cm_get_ext_hdl(vdev);
}

#ifdef WLAN_FEATURE_HOST_ROAM
bool wlan_cm_get_active_reassoc_req(struct wlan_objmgr_vdev *vdev,
				    struct wlan_cm_vdev_reassoc_req *req)
{
	return cm_get_active_reassoc_req(vdev, req);
}
#endif

bool wlan_cm_get_active_disconnect_req(struct wlan_objmgr_vdev *vdev,
				       struct wlan_cm_vdev_discon_req *req)
{
	return cm_get_active_disconnect_req(vdev, req);
}

const char *wlan_cm_reason_code_to_str(enum wlan_reason_code reason)
{
	if (reason > REASON_PROP_START)
		return "";

	switch (reason) {
	CASE_RETURN_STRING(REASON_UNSPEC_FAILURE);
	CASE_RETURN_STRING(REASON_PREV_AUTH_NOT_VALID);
	CASE_RETURN_STRING(REASON_DEAUTH_NETWORK_LEAVING);
	CASE_RETURN_STRING(REASON_DISASSOC_DUE_TO_INACTIVITY);
	CASE_RETURN_STRING(REASON_DISASSOC_AP_BUSY);
	CASE_RETURN_STRING(REASON_CLASS2_FRAME_FROM_NON_AUTH_STA);
	CASE_RETURN_STRING(REASON_CLASS3_FRAME_FROM_NON_ASSOC_STA);
	CASE_RETURN_STRING(REASON_DISASSOC_NETWORK_LEAVING);
	CASE_RETURN_STRING(REASON_STA_NOT_AUTHENTICATED);
	CASE_RETURN_STRING(REASON_BAD_PWR_CAPABILITY);
	CASE_RETURN_STRING(REASON_BAD_SUPPORTED_CHANNELS);
	CASE_RETURN_STRING(REASON_DISASSOC_BSS_TRANSITION);
	CASE_RETURN_STRING(REASON_INVALID_IE);
	CASE_RETURN_STRING(REASON_MIC_FAILURE);
	CASE_RETURN_STRING(REASON_4WAY_HANDSHAKE_TIMEOUT);
	CASE_RETURN_STRING(REASON_GROUP_KEY_UPDATE_TIMEOUT);
	CASE_RETURN_STRING(REASON_IN_4WAY_DIFFERS);
	CASE_RETURN_STRING(REASON_INVALID_GROUP_CIPHER);
	CASE_RETURN_STRING(REASON_INVALID_PAIRWISE_CIPHER);
	CASE_RETURN_STRING(REASON_INVALID_AKMP);
	CASE_RETURN_STRING(REASON_UNSUPPORTED_RSNE_VER);
	CASE_RETURN_STRING(REASON_INVALID_RSNE_CAPABILITIES);
	CASE_RETURN_STRING(REASON_1X_AUTH_FAILURE);
	CASE_RETURN_STRING(REASON_CIPHER_SUITE_REJECTED);
	CASE_RETURN_STRING(REASON_TDLS_PEER_UNREACHABLE);
	CASE_RETURN_STRING(REASON_TDLS_UNSPEC);
	CASE_RETURN_STRING(REASON_DISASSOC_SSP_REQUESTED);
	CASE_RETURN_STRING(REASON_NO_SSP_ROAMING_AGREEMENT);
	CASE_RETURN_STRING(REASON_BAD_CIPHER_OR_AKM);
	CASE_RETURN_STRING(REASON_LOCATION_NOT_AUTHORIZED);
	CASE_RETURN_STRING(REASON_SERVICE_CHANGE_PRECLUDES_TS);
	CASE_RETURN_STRING(REASON_QOS_UNSPECIFIED);
	CASE_RETURN_STRING(REASON_NO_BANDWIDTH);
	CASE_RETURN_STRING(REASON_XS_UNACKED_FRAMES);
	CASE_RETURN_STRING(REASON_EXCEEDED_TXOP);
	CASE_RETURN_STRING(REASON_STA_LEAVING);
	CASE_RETURN_STRING(REASON_END_TS_BA_DLS);
	CASE_RETURN_STRING(REASON_UNKNOWN_TS_BA);
	CASE_RETURN_STRING(REASON_TIMEDOUT);
	CASE_RETURN_STRING(REASON_PEERKEY_MISMATCH);
	CASE_RETURN_STRING(REASON_AUTHORIZED_ACCESS_LIMIT_REACHED);
	CASE_RETURN_STRING(REASON_EXTERNAL_SERVICE_REQUIREMENTS);
	CASE_RETURN_STRING(REASON_INVALID_FT_ACTION_FRAME_COUNT);
	CASE_RETURN_STRING(REASON_INVALID_PMKID);
	CASE_RETURN_STRING(REASON_INVALID_MDE);
	CASE_RETURN_STRING(REASON_INVALID_FTE);
	CASE_RETURN_STRING(REASON_MESH_PEERING_CANCELLED);
	CASE_RETURN_STRING(REASON_MESH_MAX_PEERS);
	CASE_RETURN_STRING(REASON_MESH_CONFIG_POLICY_VIOLATION);
	CASE_RETURN_STRING(REASON_MESH_CLOSE_RCVD);
	CASE_RETURN_STRING(REASON_MESH_MAX_RETRIES);
	CASE_RETURN_STRING(REASON_MESH_CONFIRM_TIMEOUT);
	CASE_RETURN_STRING(REASON_MESH_INVALID_GTK);
	CASE_RETURN_STRING(REASON_MESH_INCONSISTENT_PARAMS);
	CASE_RETURN_STRING(REASON_MESH_INVALID_SECURITY_CAP);
	CASE_RETURN_STRING(REASON_MESH_PATH_ERROR_NO_PROXY_INFO);
	CASE_RETURN_STRING(REASON_MESH_PATH_ERROR_NO_FORWARDING_INFO);
	CASE_RETURN_STRING(REASON_MESH_PATH_ERROR_DEST_UNREACHABLE);
	CASE_RETURN_STRING(REASON_MAC_ADDRESS_ALREADY_EXISTS_IN_MBSS);
	CASE_RETURN_STRING(REASON_MESH_CHANNEL_SWITCH_REGULATORY_REQ);
	CASE_RETURN_STRING(REASON_MESH_CHANNEL_SWITCH_UNSPECIFIED);
	CASE_RETURN_STRING(REASON_POOR_RSSI_CONDITIONS);
	default:
		return "Unknown";
	}
}

#ifdef WLAN_POLICY_MGR_ENABLE
void wlan_cm_hw_mode_change_resp(struct wlan_objmgr_pdev *pdev, uint8_t vdev_id,
				 wlan_cm_id cm_id, QDF_STATUS status)
{
	uint32_t prefix;

	prefix = CM_ID_GET_PREFIX(cm_id);
	if (prefix == ROAM_REQ_PREFIX)
		cm_reassoc_hw_mode_change_resp(pdev, vdev_id, cm_id, status);
	else
		cm_hw_mode_change_resp(pdev, vdev_id, cm_id, status);
}
#endif /* ifdef POLICY_MGR_ENABLE */

#ifdef SM_ENG_HIST_ENABLE
void wlan_cm_sm_history_print(struct wlan_objmgr_vdev *vdev)
{
	return cm_sm_history_print(vdev);
}

void wlan_cm_req_history_print(struct wlan_objmgr_vdev *vdev)
{
	struct cnx_mgr *cm_ctx = cm_get_cm_ctx(vdev);

	if (!cm_ctx)
		return;

	cm_req_history_print(cm_ctx);
}
#endif /* SM_ENG_HIST_ENABLE */

#ifndef CONN_MGR_ADV_FEATURE
void wlan_cm_set_candidate_advance_filter_cb(
		struct wlan_objmgr_vdev *vdev,
		void (*filter_fun)(struct wlan_objmgr_vdev *vdev,
				   struct scan_filter *filter))
{
	cm_set_candidate_advance_filter_cb(vdev, filter_fun);
}

void wlan_cm_set_candidate_custom_sort_cb(
		struct wlan_objmgr_vdev *vdev,
		void (*sort_fun)(struct wlan_objmgr_vdev *vdev,
				 qdf_list_t *list))
{
	cm_set_candidate_custom_sort_cb(vdev, sort_fun);
}

#endif

struct reduced_neighbor_report *wlan_cm_get_rnr(struct wlan_objmgr_vdev *vdev,
						wlan_cm_id cm_id)
{
	enum QDF_OPMODE op_mode = wlan_vdev_mlme_get_opmode(vdev);
	struct cm_req *cm_req;
	struct cnx_mgr *cm_ctx;

	if (op_mode != QDF_STA_MODE && op_mode != QDF_P2P_CLIENT_MODE) {
		mlme_err("vdev %d Invalid mode %d",
			 wlan_vdev_get_id(vdev), op_mode);
		return NULL;
	}

	cm_ctx = cm_get_cm_ctx(vdev);
	if (!cm_ctx)
		return NULL;
	cm_req = cm_get_req_by_cm_id(cm_ctx, cm_id);
	if (!cm_req)
		return NULL;

	if (cm_req->connect_req.cur_candidate &&
	    cm_req->connect_req.cur_candidate->entry)
		return &cm_req->connect_req.cur_candidate->entry->rnr;

	return NULL;
}

QDF_STATUS
wlan_cm_disc_cont_after_rso_stop(struct wlan_objmgr_vdev *vdev,
				 struct wlan_cm_vdev_discon_req *req)
{
	return cm_handle_rso_stop_rsp(vdev, req);
}

#ifdef WLAN_FEATURE_11BE
QDF_STATUS wlan_cm_sta_update_bw_puncture(struct wlan_objmgr_vdev *vdev,
					  uint8_t *peer_mac,
					  uint16_t ori_punc,
					  enum phy_ch_width ori_bw,
					  uint8_t ccfs0, uint8_t ccfs1,
					  enum phy_ch_width new_bw)
{
	struct wlan_channel *des_chan;
	uint16_t curr_punc = 0;
	uint16_t new_punc = 0;
	enum phy_ch_width curr_bw;
	uint16_t primary_puncture_bitmap = 0;
	struct wlan_objmgr_pdev *pdev;
	struct reg_channel_list chan_list;
	qdf_freq_t sec_ch_2g_freq = 0;
	qdf_freq_t center_freq_320 = 0;
	qdf_freq_t center_freq_40 = 0;
	uint8_t band_mask;
	uint32_t bw_puncture = 0;

	if (!vdev || !peer_mac) {
		mlme_err("invalid input parameters");
		return QDF_STATUS_E_INVAL;
	}
	pdev = wlan_vdev_get_pdev(vdev);
	des_chan = wlan_vdev_mlme_get_des_chan(vdev);
	if (!des_chan || !pdev) {
		mlme_err("invalid des chan");
		return QDF_STATUS_E_INVAL;
	}
	if (ori_bw == CH_WIDTH_320MHZ) {
		if (WLAN_REG_IS_6GHZ_CHAN_FREQ(des_chan->ch_freq))
			band_mask = BIT(REG_BAND_6G);
		else
			band_mask = BIT(REG_BAND_5G);
		center_freq_320 = wlan_reg_chan_band_to_freq(pdev, ccfs1,
							     band_mask);
	} else if (ori_bw == CH_WIDTH_40MHZ) {
		if (WLAN_REG_IS_24GHZ_CH_FREQ(des_chan->ch_freq)) {
			band_mask = BIT(REG_BAND_2G);
			center_freq_40 = wlan_reg_chan_band_to_freq(pdev,
								    ccfs0,
								    band_mask);
			if (center_freq_40 ==  des_chan->ch_freq + BW_10_MHZ)
				sec_ch_2g_freq = des_chan->ch_freq + BW_20_MHZ;
			if (center_freq_40 ==  des_chan->ch_freq - BW_10_MHZ)
				sec_ch_2g_freq = des_chan->ch_freq - BW_20_MHZ;
		}
	}
	qdf_mem_zero(&chan_list, sizeof(chan_list));
	curr_punc = des_chan->puncture_bitmap;
	curr_bw = des_chan->ch_width;
	wlan_reg_extract_puncture_by_bw(ori_bw, ori_punc,
					des_chan->ch_freq,
					center_freq_320,
					CH_WIDTH_20MHZ,
					&primary_puncture_bitmap);
	if (primary_puncture_bitmap) {
		mlme_err("sta vdev %d freq %d RX bw %d puncture 0x%x primary chan is punctured",
			 wlan_vdev_get_id(vdev), des_chan->ch_freq,
			 ori_bw, ori_punc);
		return QDF_STATUS_E_FAULT;
	}
	if (new_bw == ori_bw)
		new_punc = ori_punc;
	else
		wlan_reg_extract_puncture_by_bw(ori_bw, ori_punc,
						des_chan->ch_freq,
						center_freq_320,
						new_bw,
						&new_punc);
	if (curr_bw == new_bw) {
		if (curr_punc != new_punc)
			des_chan->puncture_bitmap = new_punc;
		else
			return QDF_STATUS_SUCCESS;
	} else {
		if (new_bw != CH_WIDTH_320MHZ)
			center_freq_320 = 0;
		wlan_reg_fill_channel_list(pdev, des_chan->ch_freq,
					   sec_ch_2g_freq, new_bw,
					   center_freq_320, &chan_list,
					   true);
		des_chan->ch_freq_seg1 =
				chan_list.chan_param[0].center_freq_seg0;
		des_chan->ch_freq_seg2 =
				chan_list.chan_param[0].center_freq_seg1;
		des_chan->ch_cfreq1 = chan_list.chan_param[0].mhz_freq_seg0;
		des_chan->ch_cfreq2 = chan_list.chan_param[1].mhz_freq_seg1;
		des_chan->puncture_bitmap = new_punc;
		des_chan->ch_width = new_bw;
	}
	mlme_debug("sta vdev %d freq %d bw %d puncture 0x%x ch_cfreq1 %d ch_cfreq2 %d",
		   wlan_vdev_get_id(vdev), des_chan->ch_freq,
		   des_chan->ch_width, des_chan->puncture_bitmap,
		   des_chan->ch_cfreq1, des_chan->ch_cfreq2);
	QDF_SET_BITS(bw_puncture, 0, 8, des_chan->ch_width);
	QDF_SET_BITS(bw_puncture, 8, 16, des_chan->puncture_bitmap);
	return wlan_util_vdev_peer_set_param_send(vdev, peer_mac,
						  WLAN_MLME_PEER_BW_PUNCTURE,
						  bw_puncture);
}
#endif /* WLAN_FEATURE_11BE */
