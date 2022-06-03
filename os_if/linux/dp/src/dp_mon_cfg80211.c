/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.

 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.

 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <ieee80211_cfg80211.h>
#include <dp_mon.h>
#include <dp_mon_ol.h>
#include <cdp_txrx_ctrl.h>
#include <cdp_txrx_mon.h>
#include <cdp_txrx_mon_struct.h>
#include <dp_mon_utils.h>
#ifdef QCA_SUPPORT_LITE_MONITOR
#include <dp_lite_mon_pub.h>
#endif

void *extract_command(struct ieee80211com *ic, struct wireless_dev *wdev,
		      int *cmd_type);
int cfg80211_reply_command(struct wiphy *wiphy, int length,
			   void *data, u_int32_t flag);
void *monitor_osif_get_vdev_by_name(char *name);

#if defined(WLAN_RX_PKT_CAPTURE_ENH) || defined(WLAN_TX_PKT_CAPTURE_ENH)
/*
 * wlan_cfg80211_set_peer_pkt_capture_params:
 * Enable/disable enhanced Rx or Tx pkt capture mode for a given peer
 */
int wlan_cfg80211_set_peer_pkt_capture_params(struct wiphy *wiphy,
					      struct wireless_dev *wdev,
					      struct wlan_cfg8011_genric_params *params)
{
	struct cfg80211_context *cfg_ctx = NULL;
	struct ieee80211com *ic = NULL;
	struct net_device *dev = NULL;
	struct ol_ath_softc_net80211 *scn = NULL;
	struct ieee80211_pkt_capture_enh peer_info;
	int return_val = -EOPNOTSUPP;
	void *cmd;
	int cmd_type;

	cfg_ctx = (struct cfg80211_context *)wiphy_priv(wiphy);

	if (!cfg_ctx) {
		dp_mon_err("Invalid context");
		return -EINVAL;
	}

	ic = cfg_ctx->ic;
	if (!ic) {
		dp_mon_err("Invalid interface");
		return -EINVAL;
	}

	cmd = extract_command(ic, wdev, &cmd_type);
	if (!cmd_type) {
		dp_mon_err("Command on invalid interface");
		return -EINVAL;
	}

	if (!params->data || (!params->data_len)) {
		dp_mon_err("%s: Invalid data length %d ", __func__,
			params->data_len);
		return -EINVAL;
	}

	if (params->data_len != QDF_MAC_ADDR_SIZE) {
		dp_mon_err("Invalid MAC address!");
		return -EINVAL;
	}

	qdf_mem_copy(peer_info.peer_mac, params->data, QDF_MAC_ADDR_SIZE);
	peer_info.rx_pkt_cap_enable = !!params->value;
	peer_info.tx_pkt_cap_enable = params->length;

	dp_mon_debug("Peer Pkt Capture: rx_enable = %u, "
		  "tx_enable = %u, mac = %02x:%02x:%02x:%02x:%02x:%02x\n",
		  peer_info.rx_pkt_cap_enable, peer_info.tx_pkt_cap_enable,
		  peer_info.peer_mac[0], peer_info.peer_mac[1],
		  peer_info.peer_mac[2], peer_info.peer_mac[3],
		  peer_info.peer_mac[4], peer_info.peer_mac[5]);

	if (ic->ic_wdev.netdev == wdev->netdev) {
		dev = wdev->netdev;
		scn =  ath_netdev_priv(dev);

		if (ic->ic_cfg80211_radio_handler.ic_set_peer_pkt_capture_params)
			return_val =
				ic->ic_cfg80211_radio_handler.ic_set_peer_pkt_capture_params((void *)scn,
											     &peer_info);
	} else {
			return_val = -EOPNOTSUPP;
	}
	return return_val;
}
#else
int wlan_cfg80211_set_peer_pkt_capture_params(struct wiphy *wiphy,
					      struct wireless_dev *wdev,
					      struct wlan_cfg8011_genric_params *params)
{
	return -EINVAL;
}
#endif /* WLAN_RX_PKT_CAPTURE_ENH || WLAN_TX_PKT_CAPTURE_ENH */

#ifdef QCA_UNDECODED_METADATA_SUPPORT
/* Allowed max phyrx error code are from 0 to 63.
 * Each bit in ic_undecoded_metadata_phyrx_error_mask
 * corresponding to specific error code.
 */
#define PHYRX_ERROR_MASK_CONTINUE 0x3FFFFF
int
wlan_cfg80211_set_phyrx_error_mask(struct wiphy *wiphy,
				   struct wireless_dev *wdev,
				   struct wlan_cfg8011_genric_params *params)
{
	struct cfg80211_context *cfg_ctx = NULL;
	struct ieee80211com *ic = NULL;
	struct net_device *dev = NULL;
	struct ol_ath_softc_net80211 *scn = NULL;
	int ret = -EOPNOTSUPP;
	void *cmd;
	int cmd_type;
	uint32_t mask = 0, mask_cont = 0;
	uint32_t *data = (uint32_t *)params->data;

	if (!data) {
		dp_mon_err("Invalid Arguments");
		return -EINVAL;
	}

	cfg_ctx = (struct cfg80211_context *)wiphy_priv(wiphy);
	if (!cfg_ctx) {
		dp_mon_err("Invalid context");
		return -EINVAL;
	}

	ic = cfg_ctx->ic;
	if (!ic) {
		dp_mon_err("Invalid interface");
		return -EINVAL;
	}

	cmd = extract_command(ic, wdev, &cmd_type);
	if (!cmd_type) {
		dp_mon_err("Command on invalid interface");
		return -EINVAL;
	}

	mask = (uint32_t)params->value;
	mask_cont = data[0];

	if (ic->ic_wdev.netdev == wdev->netdev) {
		dev = wdev->netdev;
		scn =  ath_netdev_priv(dev);

		ret = ol_ath_set_phyrx_error_mask((void *)scn, mask,
						  mask_cont);
		if (!ret) {
			ic->ic_phyrx_error_mask = mask;
			ic->ic_phyrx_error_mask_cont = mask_cont;
		} else {
			dp_mon_err("Failed to config error mask");
		}
	} else {
		ret = -EOPNOTSUPP;
	}

	dp_mon_info("Configured mask(31-0): 0x%x mask(53-32): 0x%x\n",
		    ic->ic_phyrx_error_mask, ic->ic_phyrx_error_mask_cont);

	return ret;
}

int
wlan_cfg80211_get_phyrx_error_mask(struct wiphy *wiphy,
				   struct wireless_dev *wdev,
				   struct wlan_cfg8011_genric_params *params)
{
#define PHYRX_ERROR_MASK_STRING 46
#define PHYRX_ERROR_MASK_ARGS 2
	struct cfg80211_context *cfg_ctx = NULL;
	struct ieee80211com *ic = NULL;
	struct net_device *dev = NULL;
	struct ol_ath_softc_net80211 *scn = NULL;
	int cmd_type, ret = 0;
	void *cmd;
	int val[PHYRX_ERROR_MASK_ARGS] = {0};
	char string[PHYRX_ERROR_MASK_STRING];
	uint32_t mask = 0, mask_cont = 0;

	cfg_ctx = (struct cfg80211_context *)wiphy_priv(wiphy);
	ic = cfg_ctx->ic;

	cmd = extract_command(ic, wdev, &cmd_type);

	if (!cmd_type) {
		qdf_err(" Command on Invalid interface");
		return -EINVAL;
	}

	if (ic->ic_wdev.netdev == wdev->netdev) {
		dev = wdev->netdev;
		scn =  ath_netdev_priv(dev);

		ret = ol_ath_get_phyrx_error_mask((void *)scn, &mask,
				&mask_cont);
		if (ret)
			dp_mon_err("Failed to get error mask");
	} else {
		ret = -EOPNOTSUPP;
	}

	val[0] = mask;
	val[1] = mask_cont;

	snprintf(string, PHYRX_ERROR_MASK_STRING,
		 "mask(31-0):0x%08X mask(53-32):0x%08X", val[0], val[1]);

	cfg80211_reply_command(wiphy, sizeof(string), string, 0);

	return ret;
}
#else
int
wlan_cfg80211_set_phyrx_error_mask(struct wiphy *wiphy,
				   struct wireless_dev *wdev,
				   struct wlan_cfg8011_genric_params *params)
{
	return -EINVAL;
}

int
wlan_cfg80211_get_phyrx_error_mask(struct wiphy *wiphy,
				   struct wireless_dev *wdev,
				   struct wlan_cfg8011_genric_params *params)
{
	return -EINVAL;
}
#endif /* QCA_UNDECODED_METADATA_SUPPORT */

#ifdef QCA_SUPPORT_LITE_MONITOR
/**
 * wlan_set_lite_monitor_config - set lite monitor config
 * @vscn: scn hdl
 * @mon_config: new lite monitor config
 * @status: status
 *
 * Return: 0 if success else appropriate error code
 */
static
int wlan_set_lite_monitor_config(void *vscn,
				 struct lite_mon_config *mon_config)
{
	struct ol_ath_softc_net80211 *scn =
			(struct ol_ath_softc_net80211 *)vscn;
	struct ieee80211com *ic  = &scn->sc_ic;
	ol_txrx_soc_handle soc_txrx_handle;
	struct cdp_lite_mon_filter_config *config;
	struct wlan_objmgr_vdev *vdev = NULL;
	uint8_t pdev_id;
	uint8_t lite_mon_enabled;
	char *ifname;
	int retval = 0;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	config = qdf_mem_malloc(sizeof(struct cdp_lite_mon_filter_config));
	if (!config) {
		dp_mon_err("Unable to allocate config");
		return -ENOMEM;
	}

	config->direction = mon_config->direction;
	config->debug = mon_config->debug;
	config->disable = mon_config->data.filter_config.disable;
	if (!config->disable) {
		/* enable */
		config->level = mon_config->data.filter_config.level;
		ifname = mon_config->data.filter_config.interface_name;
		if (strlen(ifname)) {
			vdev = monitor_osif_get_vdev_by_name(ifname);
			if (vdev) {
				status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_LITE_MON_ID);
				if (QDF_IS_STATUS_ERROR(status)) {
					dp_mon_err("unable to get output vdev ref");
					retval = -EINVAL;
					goto fail;
				}
				if (wlan_vdev_get_pdev(vdev) != ic->ic_pdev_obj) {
					dp_mon_err("output interface is on diff radio");
					wlan_objmgr_vdev_release_ref(vdev, WLAN_LITE_MON_ID);
					retval = -EINVAL;
					goto fail;
				}
			} else {
				dp_mon_err("invalid output interface");
				retval = -EINVAL;
				goto fail;
			}
		}
		qdf_mem_copy(config->mgmt_filter,
			     mon_config->data.filter_config.mgmt_filter,
			     sizeof(config->mgmt_filter));
		qdf_mem_copy(config->data_filter,
			     mon_config->data.filter_config.data_filter,
			     sizeof(config->data_filter));
		qdf_mem_copy(config->ctrl_filter,
			     mon_config->data.filter_config.ctrl_filter,
			     sizeof(config->ctrl_filter));
		qdf_mem_copy(config->len,
			     mon_config->data.filter_config.len,
			     sizeof(config->len));
		config->metadata = mon_config->data.filter_config.metadata;
		config->vdev_id = (vdev) ?
				  wlan_vdev_get_id(vdev) :
				  WLAN_INVALID_VDEV_ID;
	}

	soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
	pdev_id = wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev);
	lite_mon_enabled = cdp_is_lite_mon_enabled(soc_txrx_handle,
						   pdev_id, config->direction);
	if (QDF_STATUS_SUCCESS !=
	    cdp_set_lite_mon_config(soc_txrx_handle,
				    config,
				    pdev_id)) {
		if (vdev)
			wlan_objmgr_vdev_release_ref(vdev, WLAN_LITE_MON_ID);
		retval = -EINVAL;
		goto fail;
	}

	if (config->direction == LITE_MON_DIRECTION_RX) {
		if (!lite_mon_enabled && !config->disable) {
			/* rx enable */
			/* subscribe for lite mon rx wdi event */
			wlan_lite_mon_rx_subscribe(scn);
		} else if (lite_mon_enabled && config->disable) {
			/* rx disable */
			/* unsubscribe for lite mon rx wdi event */
			wlan_lite_mon_rx_unsubscribe(scn);
		}
	} else if (config->direction == LITE_MON_DIRECTION_TX) {
		if (!lite_mon_enabled && !config->disable) {
			/* tx enable */
			/* subscribe for lite mon tx wdi event */
			wlan_lite_mon_tx_subscribe(scn);
		} else if (lite_mon_enabled && config->disable) {
			/* tx disable */
			/* unsubscribe for lite mon tx wdi event */
			wlan_lite_mon_tx_unsubscribe(scn);
		}
	}
	if (vdev)
		wlan_objmgr_vdev_release_ref(vdev, WLAN_LITE_MON_ID);

fail:
	qdf_mem_free(config);

	return retval;
}

/**
 * wlan_get_lite_monitor_config - get lite monitor config
 * @vscn: scn hdl
 * @mon_config: str to copy lite monitor config
 * @status: status
 *
 * Return: 0 if success else appropriate error code
 */
static
int wlan_get_lite_monitor_config(void *vscn,
				 struct lite_mon_config *mon_config)
{
	struct cdp_lite_mon_filter_config *config;
	struct ol_ath_softc_net80211 *scn =
			(struct ol_ath_softc_net80211 *)vscn;
	struct ieee80211com *ic  = &scn->sc_ic;
	struct wlan_objmgr_vdev *vdev = NULL;
	struct vdev_osif_priv *osif_priv = NULL;
	int retval = 0;

	config = qdf_mem_malloc(sizeof(struct cdp_lite_mon_filter_config));
	if (!config) {
		dp_mon_err("Unable to allocate config");
		return -ENOMEM;
	}

	config->direction = mon_config->direction;
	if (QDF_STATUS_SUCCESS !=
	    cdp_get_lite_mon_config(wlan_psoc_get_dp_handle(scn->soc->psoc_obj),
				    config,
				    wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev))) {
		retval = -EINVAL;
		goto fail;
	}

	mon_config->debug = config->debug;
	mon_config->data.filter_config.level = config->level;
	qdf_mem_copy(mon_config->data.filter_config.mgmt_filter,
		     config->mgmt_filter,
		     sizeof(mon_config->data.filter_config.mgmt_filter));
	qdf_mem_copy(mon_config->data.filter_config.data_filter,
		     config->data_filter,
		     sizeof(mon_config->data.filter_config.data_filter));
	qdf_mem_copy(mon_config->data.filter_config.ctrl_filter,
		     config->ctrl_filter,
		     sizeof(mon_config->data.filter_config.ctrl_filter));
	qdf_mem_copy(mon_config->data.filter_config.len,
		     config->len,
		     sizeof(mon_config->data.filter_config.len));
	mon_config->data.filter_config.metadata = config->metadata;
	if (config->vdev_id != WLAN_INVALID_VDEV_ID) {
		/* get vap name from vdev id */
		vdev = wlan_objmgr_get_vdev_by_id_from_pdev(ic->ic_pdev_obj,
							    config->vdev_id,
							    WLAN_LITE_MON_ID);
		if (vdev) {
			osif_priv = wlan_vdev_get_ospriv(vdev);
			strlcpy(mon_config->data.filter_config.interface_name,
				osif_priv->wdev->netdev->name,
				sizeof(mon_config->data.filter_config.interface_name));
			wlan_objmgr_vdev_release_ref(vdev, WLAN_LITE_MON_ID);
		}
	}

fail:
	qdf_mem_free(config);

	return retval;
}

/**
 * wlan_validate_lite_mon_peer - validate peer type and mac
 * @ic: ic hdl
 * @mac: peer mac
 * @type: peer type
 *
 * Return: 0 if success else appropriate error code
 */
static int
wlan_validate_lite_mon_peer(struct ieee80211com *ic,
			    uint8_t *mac, uint8_t type)
{
	struct ieee80211_node *ni = NULL;
	struct ieee80211vap *tmpvap = NULL;

	ni = ieee80211_find_node(ic, mac, WLAN_LITE_MON_ID);
	if (ni) {
		if (type == LITE_MON_PEER_NON_ASSOCIATED) {
			dp_mon_err("Error! %pM is self peer", mac);
			ieee80211_free_node(ni, WLAN_LITE_MON_ID);
			return -EINVAL;
		}
		ieee80211_free_node(ni, WLAN_LITE_MON_ID);
	} else {
		if (type == LITE_MON_PEER_ASSOCIATED) {
			dp_mon_err("Error! %pM is not self peer", mac);
			return -EINVAL;
		}
	}

	TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
		if (IEEE80211_ADDR_EQ(mac, tmpvap->iv_myaddr)) {
			dp_mon_err("Error! %pM is same as self vap's addr",
				   mac);
			return -EINVAL;
		}
	}

	return EOK;
}

/**
 * wlan_get_lite_mon_peer_type - get cdp lite mon peer type
 * @config_type: appln sent peer type
 *
 * Return: valid peer type on success else max peer type
 */
static inline
uint8_t wlan_get_lite_mon_peer_type(uint8_t config_type)
{
	if (config_type == LITE_MON_PEER_ASSOCIATED)
		return CDP_LITE_MON_PEER_TYPE_ASSOCIATED;
	else if (config_type == LITE_MON_PEER_NON_ASSOCIATED)
		return CDP_LITE_MON_PEER_TYPE_NON_ASSOCIATED;
	else
		return CDP_LITE_MON_PEER_TYPE_MAX;
}

/**
 * wlan_set_lite_monitor_peer_config - set lite monitor peer config
 * @vscn: scn hdl
 * @mon_config: new lite monitor peer config
 *
 * Return: 0 if success else appropriate error code
 */
static
int wlan_set_lite_monitor_peer_config(void *vscn,
				      struct lite_mon_config *mon_config)
{
	struct ol_ath_softc_net80211 *scn =
			(struct ol_ath_softc_net80211 *)vscn;
	struct ieee80211com *ic  = &scn->sc_ic;
	struct wlan_objmgr_vdev *vdev = NULL;
	ol_txrx_soc_handle soc_txrx_handle;
	struct cdp_lite_mon_peer_config *peer_config;
	uint8_t i, pdev_id;
	uint8_t peer_count;
	char *ifname = NULL;
	int retval = 0;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	peer_config = qdf_mem_malloc(sizeof(struct cdp_lite_mon_peer_config));
	if (!peer_config) {
		dp_mon_err("Unable to allocate peer config");
		return -ENOMEM;
	}

	ifname = mon_config->data.peer_config.interface_name;
	if (!strlen(ifname)) {
		dp_mon_err("invalid vap interface");
		retval = -EINVAL;
		goto fail;
	}

	vdev = monitor_osif_get_vdev_by_name(ifname);
	if (vdev) {
		status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_LITE_MON_ID);
		if (QDF_IS_STATUS_ERROR(status)) {
			dp_mon_err("unable to get vdev ref");
			retval = -EINVAL;
			goto fail;
		}
		if (wlan_vdev_get_pdev(vdev) != ic->ic_pdev_obj) {
			dp_mon_err("vap interface is on diff radio");
			wlan_objmgr_vdev_release_ref(vdev, WLAN_LITE_MON_ID);
			retval = -EINVAL;
			goto fail;
		}
	} else {
		dp_mon_err("failed to get vdev");
		retval = -EINVAL;
		goto fail;
	}

	peer_config->vdev_id = wlan_vdev_get_id(vdev);

	peer_config->direction = mon_config->direction;
	peer_config->action = mon_config->data.peer_config.action;
	peer_config->type =
		wlan_get_lite_mon_peer_type(mon_config->data.peer_config.type);
	if (peer_config->type  == CDP_LITE_MON_PEER_TYPE_MAX) {
		dp_mon_err("Invalid peer type");
		wlan_objmgr_vdev_release_ref(vdev, WLAN_LITE_MON_ID);
		retval = -EINVAL;
		goto fail;
	}
	peer_count = mon_config->data.peer_config.count;
	if (peer_count > CDP_LITE_MON_PEER_MAX) {
		dp_mon_err("Invalid peer count");
		wlan_objmgr_vdev_release_ref(vdev, WLAN_LITE_MON_ID);
		retval = -EINVAL;
		goto fail;
	}

	soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
	pdev_id = wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev);
	for (i = 0; i < peer_count; ++i) {
		if (mon_config->data.peer_config.action == LITE_MON_PEER_ADD) {
			if (EOK != wlan_validate_lite_mon_peer(&scn->sc_ic,
					mon_config->data.peer_config.mac_addr[i],
					mon_config->data.peer_config.type)) {
				retval = -EINVAL;
				continue;
			}
		}

		qdf_mem_copy(peer_config->mac,
			     mon_config->data.peer_config.mac_addr[i],
			     QDF_MAC_ADDR_SIZE);

		if (QDF_STATUS_SUCCESS ==
		    cdp_set_lite_mon_peer_config(soc_txrx_handle,
						 peer_config,
						 pdev_id)) {
		} else {
			retval = -EINVAL;
		}
	}

	wlan_objmgr_vdev_release_ref(vdev, WLAN_LITE_MON_ID);

fail:
	qdf_mem_free(peer_config);

	return retval;
}

/**
 * wlan_get_lite_monitor_peer_config - get lite monitor peer list
 * @vscn: scn hdl
 * @mon_config: str to copy lite monitor peer list
 *
 * Return: 0 if success else appropriate error code
 */
static
int wlan_get_lite_monitor_peer_config(void *vscn,
				      struct lite_mon_config *mon_config)
{
	struct ol_ath_softc_net80211 *scn =
			(struct ol_ath_softc_net80211 *)vscn;
	ol_txrx_soc_handle soc_txrx_handle;
	struct cdp_lite_mon_peer_info *peer_info;
	uint8_t i, pdev_id;
	int retval = 0;

	peer_info = qdf_mem_malloc(sizeof(struct cdp_lite_mon_peer_info));
	if (!peer_info) {
		dp_mon_err("Unable to allocate peer info");
		return -ENOMEM;
	}

	peer_info->direction = mon_config->direction;
	peer_info->type =
		wlan_get_lite_mon_peer_type(mon_config->data.peer_config.type);
	soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
	pdev_id = wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev);
	if (QDF_STATUS_SUCCESS !=
	    cdp_get_lite_mon_peer_config(soc_txrx_handle,
					 peer_info,
					 pdev_id)) {
		dp_mon_err("Failed to retrieve peer info");
		retval = -EINVAL;
		goto fail;
	}

	mon_config->data.peer_config.count = peer_info->count;
	for (i = 0; i < peer_info->count && i < CDP_LITE_MON_PEER_MAX; ++i) {
		qdf_mem_copy(mon_config->data.peer_config.mac_addr[i],
			     peer_info->mac[i],
			     QDF_MAC_ADDR_SIZE);
	}

fail:
	qdf_mem_free(peer_info);

	return retval;
}

/**
 * wlan_cfg80211_lite_monitor_config - parse and set lite mon config
 * @wiphy: wireless phy ptr
 * @wdev: wireless dev ptr
 * @params: lite mon config
 *
 * Return: 0 on success else appropriate errno
 */
int wlan_cfg80211_lite_monitor_config(struct wiphy *wiphy,
				      struct wireless_dev *wdev,
				      struct wlan_cfg8011_genric_params *params)
{
	struct cfg80211_context *cfg_ctx = NULL;
	struct ieee80211com *ic = NULL;
	struct lite_mon_config *mon_config = NULL;
	struct net_device *dev = NULL;
	struct ol_ath_softc_net80211 *scn = NULL;
	uint32_t expected_len = 0;
	void *cmd;
	int cmd_type;
	int retval = -EOPNOTSUPP;

	cfg_ctx = (struct cfg80211_context *)wiphy_priv(wiphy);
	if (!cfg_ctx) {
		dp_mon_err("Invalid cfg context");
		return -EINVAL;
	}

	ic = cfg_ctx->ic;
	if (!ic) {
		dp_mon_err("Invalid interface");
		return -EINVAL;
	}

	if (!(ic->ic_monitor_version == MONITOR_VERSION_2)) {
		dp_mon_err("cmd not supported");
		return -EOPNOTSUPP;
	}

	cmd = extract_command(ic, wdev, &cmd_type);
	if (!cmd_type) {
		dp_mon_err("Command on invalid interface");
		return -EINVAL;
	}

	if (!params->data || (!params->data_len)) {
		dp_mon_err("Invalid data or data length");
		return -EINVAL;
	}

	expected_len = sizeof(struct lite_mon_config);
	if (params->data_len != expected_len) {
		dp_mon_err("Invalid lite mon config");
		return -EINVAL;
	}

	mon_config = (struct lite_mon_config *)params->data;
	if (ic->ic_wdev.netdev == wdev->netdev) {
		dev = wdev->netdev;
		scn =  ath_netdev_priv(dev);
		switch (mon_config->cmdtype) {
		case LITE_MON_SET_FILTER:
			retval = wlan_set_lite_monitor_config((void *)scn,
							      mon_config);
			break;

		case LITE_MON_SET_PEER:
			retval = wlan_set_lite_monitor_peer_config((void *)scn,
								   mon_config);
			break;

		case LITE_MON_GET_FILTER:
			retval = wlan_get_lite_monitor_config((void *)scn,
							      mon_config);
			if (!retval) {
				retval = cfg80211_reply_command(wiphy,
								sizeof(struct lite_mon_config),
								mon_config, 0);
			}
			break;

		case LITE_MON_GET_PEER:
			retval = wlan_get_lite_monitor_peer_config((void *)scn,
								   mon_config);
			if (!retval) {
				retval = cfg80211_reply_command(wiphy,
								sizeof(struct lite_mon_config),
								mon_config, 0);
			}
			break;

		default:
			dp_mon_err("Invalid lite mon request");
			retval = -EINVAL;
			break;
		}
	} else {
		retval = -EOPNOTSUPP;
	}

	return retval;
}
#endif /* QCA_SUPPORT_LITE_MONITOR */
