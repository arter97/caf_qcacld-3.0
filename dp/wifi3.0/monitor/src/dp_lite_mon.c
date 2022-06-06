/*
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

#ifdef QCA_SUPPORT_LITE_MONITOR

#include "dp_lite_mon.h"
#include "qdf_trace.h"
#include "qdf_nbuf.h"
#include "dp_peer.h"
#include "dp_types.h"
#include "dp_internal.h"
#include <dp_htt.h>
#include "dp_mon_filter.h"
#include "dp_mon_filter_2.0.h"
#include "qdf_mem.h"
#include "cdp_txrx_cmn_struct.h"
#include "cdp_txrx_cmn_reg.h"
#include "qdf_lock.h"
#include "dp_rx.h"
#include "dp_mon.h"
#include <dp_mon_2.0.h>
#include <dp_be.h>
#include <dp_rx_mon_2.0.h>

/**
 * dp_lite_mon_free_peers - free peers
 * @config: lite mon tx/rx config
 *
 * Return: void
 */
static void
dp_lite_mon_free_peers(struct dp_lite_mon_config *config)
{
	struct dp_lite_mon_peer *peer = NULL;
	struct dp_lite_mon_peer *temp_peer = NULL;

	TAILQ_FOREACH_SAFE(peer, &config->peer_list,
			   peer_list_elem, temp_peer) {
		/* delete peer from the list */
		TAILQ_REMOVE(&config->peer_list, peer, peer_list_elem);
		config->peer_count[peer->type]--;
		qdf_mem_free(peer);
	}
}

/**
 * dp_lite_mon_reset_config - reset lite mon config
 * @config: lite mon tx/rx config
 *
 * Return: void
 */
static void
dp_lite_mon_reset_config(struct dp_lite_mon_config *config)
{
	/* free peers */
	dp_lite_mon_free_peers(config);

	/* reset config */
	config->enable = 0;
	config->level = 0;
	qdf_mem_zero(config->mgmt_filter, sizeof (config->mgmt_filter));
	qdf_mem_zero(config->ctrl_filter, sizeof (config->ctrl_filter));
	qdf_mem_zero(config->data_filter, sizeof (config->data_filter));
	config->fp_enabled = false;
	config->md_enabled = false;
	config->mo_enabled = false;
	qdf_mem_zero(config->len, sizeof (config->len));
	config->metadata = 0;
	config->debug = 0;
	config->lite_mon_vdev = NULL;
}

/**
 * dp_lite_mon_update_config  - Update curr config with new config
 * @pdev: dp pdev hdl
 * @curr_config: curr tx/rx lite mon config
 * @new_config: new lite mon config
 *
 * Return: void
 */
static void
dp_lite_mon_update_config(struct dp_pdev *pdev,
			  struct dp_lite_mon_config *curr_config,
			  struct cdp_lite_mon_filter_config *new_config)
{
	struct dp_vdev *vdev = NULL;

	if (!new_config->disable) {
		/* Update new config */
		curr_config->level = new_config->level;

		qdf_mem_copy(curr_config->mgmt_filter,
			     new_config->mgmt_filter,
			     sizeof(curr_config->mgmt_filter));
		qdf_mem_copy(curr_config->ctrl_filter,
			     new_config->ctrl_filter,
			     sizeof(curr_config->ctrl_filter));
		qdf_mem_copy(curr_config->data_filter,
			     new_config->data_filter,
			     sizeof(curr_config->data_filter));

		/* enable appropriate modes */
		if (new_config->mgmt_filter[DP_MON_FRM_FILTER_MODE_FP] ||
		    new_config->ctrl_filter[DP_MON_FRM_FILTER_MODE_FP] ||
		    new_config->data_filter[DP_MON_FRM_FILTER_MODE_FP])
			curr_config->fp_enabled = true;

		if (new_config->mgmt_filter[DP_MON_FRM_FILTER_MODE_MD] ||
		    new_config->ctrl_filter[DP_MON_FRM_FILTER_MODE_MD] ||
		    new_config->data_filter[DP_MON_FRM_FILTER_MODE_MD])
			curr_config->md_enabled = true;

		if (new_config->mgmt_filter[DP_MON_FRM_FILTER_MODE_MO] ||
		    new_config->ctrl_filter[DP_MON_FRM_FILTER_MODE_MO] ||
		    new_config->data_filter[DP_MON_FRM_FILTER_MODE_MO])
			curr_config->mo_enabled = true;

		/* set appropriate lengths */
		if (new_config->mgmt_filter[DP_MON_FRM_FILTER_MODE_FP] ||
		    new_config->mgmt_filter[DP_MON_FRM_FILTER_MODE_MD] ||
		    new_config->mgmt_filter[DP_MON_FRM_FILTER_MODE_MO])
			curr_config->len[WLAN_FC0_TYPE_MGMT] =
				new_config->len[WLAN_FC0_TYPE_MGMT];

		if (new_config->ctrl_filter[DP_MON_FRM_FILTER_MODE_FP] ||
		    new_config->ctrl_filter[DP_MON_FRM_FILTER_MODE_MD] ||
		    new_config->ctrl_filter[DP_MON_FRM_FILTER_MODE_MO])
			curr_config->len[WLAN_FC0_TYPE_CTRL] =
				new_config->len[WLAN_FC0_TYPE_CTRL];

		if (new_config->data_filter[DP_MON_FRM_FILTER_MODE_FP] ||
		    new_config->data_filter[DP_MON_FRM_FILTER_MODE_MD] ||
		    new_config->data_filter[DP_MON_FRM_FILTER_MODE_MO])
			curr_config->len[WLAN_FC0_TYPE_DATA] =
				new_config->len[WLAN_FC0_TYPE_DATA];

		curr_config->metadata = new_config->metadata;
		curr_config->debug = new_config->debug;

		if (new_config->vdev_id != CDP_INVALID_VDEV_ID) {
			vdev = dp_vdev_get_ref_by_id(pdev->soc,
						     new_config->vdev_id,
						     DP_MOD_ID_CDP);
			if (vdev) {
				curr_config->lite_mon_vdev = vdev;
				dp_vdev_unref_delete(pdev->soc, vdev,
						     DP_MOD_ID_CDP);
			}
		}

		curr_config->enable = true;
	} else {
		/* if new config disable is set then it means
		 * we need to reset curr config
		 */
		dp_lite_mon_reset_config(curr_config);
	}
}

/**
 * dp_lite_mon_copy_config  - get current config
 * @curr_config: curr tx/rx lite mon config
 * @config: str to hold curr config
 *
 * Return: void
 */
static void
dp_lite_mon_copy_config(struct dp_lite_mon_config *curr_config,
			struct cdp_lite_mon_filter_config *config)
{
	config->level = curr_config->level;
	qdf_mem_copy(config->mgmt_filter,
		     curr_config->mgmt_filter,
		     sizeof(config->mgmt_filter));
	qdf_mem_copy(config->ctrl_filter,
		     curr_config->ctrl_filter,
		     sizeof(config->ctrl_filter));
	qdf_mem_copy(config->data_filter,
		     curr_config->data_filter,
		     sizeof(config->data_filter));
	qdf_mem_copy(config->len,
		     curr_config->len,
		     sizeof(config->len));
	config->metadata = curr_config->metadata;
	config->debug = curr_config->debug;
	if (curr_config->lite_mon_vdev)
		config->vdev_id = curr_config->lite_mon_vdev->vdev_id;
	else
		config->vdev_id = CDP_INVALID_VDEV_ID;
}

/**
 * dp_lite_mon_disable_rx - disables rx lite mon
 * @pdev: dp pdev
 *
 * Return: void
 */
void
dp_lite_mon_disable_rx(struct dp_pdev *pdev)
{
	struct dp_mon_pdev_be *be_mon_pdev;
	struct dp_lite_mon_rx_config *lite_mon_rx_config;
	struct dp_mon_pdev *mon_pdev;
	QDF_STATUS status;

	if (!pdev)
		return;

	mon_pdev = (struct dp_mon_pdev *)pdev->monitor_pdev;
	if (!mon_pdev)
		return;

	be_mon_pdev = dp_get_be_mon_pdev_from_dp_mon_pdev(mon_pdev);
	lite_mon_rx_config = be_mon_pdev->lite_mon_rx_config;

	qdf_spin_lock_bh(&lite_mon_rx_config->lite_mon_rx_lock);
	/* reset filters */
	dp_mon_filter_reset_rx_lite_mon(be_mon_pdev);
	status = dp_mon_filter_update(pdev);
	if (status != QDF_STATUS_SUCCESS)
		dp_mon_err("Failed to reset rx lite mon filters");

	/* reset rx lite mon config */
	dp_lite_mon_reset_config(&lite_mon_rx_config->rx_config);
	qdf_spin_unlock_bh(&lite_mon_rx_config->lite_mon_rx_lock);
}

/**
 * dp_lite_mon_set_rx_config - Update rx lite mon config
 * @be_pdev: be dp pdev
 * @config: new lite mon config
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS
dp_lite_mon_set_rx_config(struct dp_pdev_be *be_pdev,
			  struct cdp_lite_mon_filter_config *config)
{
	struct dp_mon_pdev_be *be_mon_pdev =
			(struct dp_mon_pdev_be *)be_pdev->pdev.monitor_pdev;
	struct dp_lite_mon_rx_config *lite_mon_rx_config;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	if (!be_mon_pdev)
		return QDF_STATUS_E_FAILURE;

	lite_mon_rx_config = be_mon_pdev->lite_mon_rx_config;
	if (lite_mon_rx_config->rx_config.enable == !config->disable) {
		dp_mon_err("Rx lite mon already enabled/disabled");
		return QDF_STATUS_E_INVAL;
	}

	if (config->disable) {
		qdf_spin_lock_bh(&lite_mon_rx_config->lite_mon_rx_lock);
		/* reset lite mon filters */
		dp_mon_filter_reset_rx_lite_mon(be_mon_pdev);
		status = dp_mon_filter_update(&be_pdev->pdev);
		if (status != QDF_STATUS_SUCCESS)
			dp_mon_err("Failed to reset rx lite mon filters");

		/* reset rx lite mon config */
		dp_lite_mon_update_config(&be_pdev->pdev,
					  &lite_mon_rx_config->rx_config,
					  config);
		qdf_spin_unlock_bh(&lite_mon_rx_config->lite_mon_rx_lock);

		/* restore full mon filter if enabled */
		qdf_spin_lock_bh(&be_mon_pdev->mon_pdev.mon_lock);
		if (be_mon_pdev->mon_pdev.monitor_configured) {
			dp_mon_filter_setup_mon_mode(&be_pdev->pdev);
			if (dp_mon_filter_update(&be_pdev->pdev) !=
							QDF_STATUS_SUCCESS)
				dp_mon_info("failed to setup full mon filters");
		}
		qdf_spin_unlock_bh(&be_mon_pdev->mon_pdev.mon_lock);
	} else {
		/* validate configuration */
		/* if level is MPDU/PPDU and data pkt is full len, then
		 * this is a contradicting request as full data pkt can span
		 * multiple mpdus. Whereas Mgmt/Ctrl pkt full len with level
		 * MPDU/PPDU is supported as they span max one mpdu.
		 */
		if ((config->level != CDP_LITE_MON_LEVEL_MSDU) &&
		    (config->len[WLAN_FC0_TYPE_DATA] == CDP_LITE_MON_LEN_FULL)) {
			dp_mon_err("Filter combination not supported "
				   "level mpdu/ppdu and data full pkt");
			return QDF_STATUS_E_INVAL;
		}

		/* reset full mon filters if enabled */
		qdf_spin_lock_bh(&be_mon_pdev->mon_pdev.mon_lock);
		if (be_mon_pdev->mon_pdev.monitor_configured) {
			dp_mon_filter_reset_mon_mode(&be_pdev->pdev);
			if (dp_mon_filter_update(&be_pdev->pdev) !=
							QDF_STATUS_SUCCESS)
				dp_mon_info("failed to reset full mon filters");
		}
		qdf_spin_unlock_bh(&be_mon_pdev->mon_pdev.mon_lock);

		qdf_spin_lock_bh(&lite_mon_rx_config->lite_mon_rx_lock);
		/* store rx lite mon config */
		dp_lite_mon_update_config(&be_pdev->pdev,
					  &lite_mon_rx_config->rx_config,
					  config);

		/* setup rx lite mon filters */
		dp_mon_filter_setup_rx_lite_mon(be_mon_pdev);
		status = dp_mon_filter_update(&be_pdev->pdev);
		if (status != QDF_STATUS_SUCCESS) {
			dp_mon_err("Failed to setup rx lite mon filters");
			dp_mon_filter_reset_rx_lite_mon(be_mon_pdev);
			config->disable = 1;
			dp_lite_mon_update_config(&be_pdev->pdev,
						  &lite_mon_rx_config->rx_config,
						  config);
		}
		qdf_spin_unlock_bh(&lite_mon_rx_config->lite_mon_rx_lock);
	}

	return status;
}

/**
 * dp_lite_mon_set_tx_config - Update tx lite mon config
 * @be_pdev: be dp pdev
 * @config: new lite mon config
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS
dp_lite_mon_set_tx_config(struct dp_pdev_be *be_pdev,
			  struct cdp_lite_mon_filter_config *config)
{
	struct dp_mon_pdev_be *be_mon_pdev =
			(struct dp_mon_pdev_be *)be_pdev->pdev.monitor_pdev;
	struct dp_lite_mon_tx_config *lite_mon_tx_config;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	if (!be_mon_pdev)
		return QDF_STATUS_E_FAILURE;

	lite_mon_tx_config = be_mon_pdev->lite_mon_tx_config;
	if (lite_mon_tx_config->tx_config.enable == !config->disable) {
		dp_mon_err("Tx lite mon already enabled/disabled");
		return QDF_STATUS_E_INVAL;
	}

	if (config->disable) {
		/* lite mon pending:
		 * -reset and update tx filter
		 * -reset tx lite mon config
		 */
		qdf_spin_lock_bh(&lite_mon_tx_config->lite_mon_tx_lock);
		dp_lite_mon_update_config(&be_pdev->pdev,
					  &lite_mon_tx_config->tx_config,
					  config);
		qdf_spin_unlock_bh(&lite_mon_tx_config->lite_mon_tx_lock);
	} else {
		/* store tx lite mon config */
		qdf_spin_lock_bh(&lite_mon_tx_config->lite_mon_tx_lock);
		dp_lite_mon_update_config(&be_pdev->pdev,
					  &lite_mon_tx_config->tx_config,
					  config);
		qdf_spin_unlock_bh(&lite_mon_tx_config->lite_mon_tx_lock);
		/* lite mon pending: setup and update rx filter */
	}

	return status;
}

/**
 * dp_lite_mon_set_config - Update tx/rx lite mon config
 * @soc_hdl: dp soc hdl
 * @config: new lite mon config
 * @pdev_id: pdev id
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_lite_mon_set_config(struct cdp_soc_t *soc_hdl,
		       struct cdp_lite_mon_filter_config *config,
		       uint8_t pdev_id)
{
	struct dp_pdev *pdev =
		dp_get_pdev_from_soc_pdev_id_wifi3(cdp_soc_t_to_dp_soc(soc_hdl),
						   pdev_id);
	struct dp_pdev_be *be_pdev = dp_get_be_pdev_from_dp_pdev(pdev);
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	if (!be_pdev || !config)
		return QDF_STATUS_E_FAILURE;

	if (config->direction == CDP_LITE_MON_DIRECTION_RX) {
		status = dp_lite_mon_set_rx_config(be_pdev,
						   config);
	} else if (config->direction == CDP_LITE_MON_DIRECTION_TX) {
		status = dp_lite_mon_set_tx_config(be_pdev,
						   config);
	} else {
		dp_mon_err("Invalid direction");
		status = QDF_STATUS_E_INVAL;
	}

	return status;
}

/**
 * dp_lite_mon_get_config - get tx/rx lite mon config
 * @soc_hdl: dp soc hdl
 * @config: str to hold config
 * @pdev_id: pdev id
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_lite_mon_get_config(struct cdp_soc_t *soc_hdl,
		       struct cdp_lite_mon_filter_config *config,
		       uint8_t pdev_id)
{
	struct dp_pdev *pdev =
		dp_get_pdev_from_soc_pdev_id_wifi3(cdp_soc_t_to_dp_soc(soc_hdl),
						   pdev_id);
	struct dp_pdev_be *be_pdev = dp_get_be_pdev_from_dp_pdev(pdev);
	struct dp_mon_pdev_be *be_mon_pdev = NULL;
	struct dp_lite_mon_rx_config *lite_mon_rx_config;
	struct dp_lite_mon_tx_config *lite_mon_tx_config;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	if (!be_pdev || !config)
		return QDF_STATUS_E_FAILURE;

	be_mon_pdev = (struct dp_mon_pdev_be *)be_pdev->pdev.monitor_pdev;
	if (!be_mon_pdev)
		return QDF_STATUS_E_FAILURE;

	if (config->direction == CDP_LITE_MON_DIRECTION_RX) {
		lite_mon_rx_config = be_mon_pdev->lite_mon_rx_config;
		qdf_spin_lock_bh(&lite_mon_rx_config->lite_mon_rx_lock);
		dp_lite_mon_copy_config(&lite_mon_rx_config->rx_config,
					config);
		qdf_spin_unlock_bh(&lite_mon_rx_config->lite_mon_rx_lock);
	} else if (config->direction == CDP_LITE_MON_DIRECTION_TX) {
		lite_mon_tx_config = be_mon_pdev->lite_mon_tx_config;
		qdf_spin_lock_bh(&lite_mon_tx_config->lite_mon_tx_lock);
		dp_lite_mon_copy_config(&lite_mon_tx_config->tx_config,
					config);
		qdf_spin_unlock_bh(&lite_mon_tx_config->lite_mon_tx_lock);
	} else {
		dp_mon_err("Invalid direction");
		status = QDF_STATUS_E_INVAL;
	}

	return status;
}

/**
 * dp_lite_mon_update_peers - Update peer list
 * @config: tx/rx lite mon config
 * @peer_config: new peer config
 * @new_peer: new peer entry
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS
dp_lite_mon_update_peers(struct dp_lite_mon_config *config,
			 struct cdp_lite_mon_peer_config *peer_config,
			 struct dp_lite_mon_peer *new_peer)
{
	struct dp_lite_mon_peer *peer;
	bool peer_deleted = false;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	uint8_t total_peers = 0;

	if (peer_config->action == CDP_LITE_MON_PEER_ADD) {
		total_peers =
			config->peer_count[CDP_LITE_MON_PEER_TYPE_ASSOCIATED] +
			config->peer_count[CDP_LITE_MON_PEER_TYPE_NON_ASSOCIATED];
		if (total_peers >= CDP_LITE_MON_PEER_MAX) {
			dp_mon_err("cannot add peer, max peer limit reached");
			return QDF_STATUS_E_NOMEM;
		}

		TAILQ_FOREACH(peer, &config->peer_list,
			      peer_list_elem) {
			if (!qdf_mem_cmp(&peer->peer_mac.raw[0],
					 peer_config->mac, QDF_MAC_ADDR_SIZE)) {
				dp_mon_err("%pM Peer already exist",
					   peer_config->mac);
				return QDF_STATUS_E_ALREADY;
			}
		}

		qdf_mem_copy(&new_peer->peer_mac.raw[0],
			     peer_config->mac, QDF_MAC_ADDR_SIZE);
		new_peer->type = peer_config->type;

		/* add peer to lite mon peer list */
		TAILQ_INSERT_TAIL(&config->peer_list,
				  new_peer, peer_list_elem);
		config->peer_count[peer_config->type]++;
	} else if (peer_config->action == CDP_LITE_MON_PEER_REMOVE) {
		TAILQ_FOREACH(peer, &config->peer_list,
			      peer_list_elem) {
			if (!qdf_mem_cmp(&peer->peer_mac.raw[0],
					 peer_config->mac, QDF_MAC_ADDR_SIZE)) {
				/* delete peer from lite mon peer list */
				TAILQ_REMOVE(&config->peer_list,
					     peer, peer_list_elem);
				config->peer_count[peer->type]--;
				qdf_mem_free(peer);
				peer_deleted = true;
				break;
			}
		}
		if (!peer_deleted)
			status = QDF_STATUS_E_FAILURE;
	} else {
		dp_mon_err("invalid peer action");
		status = QDF_STATUS_E_FAILURE;
	}

	return status;
}

/**
 * dp_lite_mon_set_tx_peer_config - Update lite mon tx peer list
 * @soc: dp soc
 * @be_pdev: be dp pdev
 * @peer_config: new peer config
 *
 * Return: QDF_STATUS
 */
static inline QDF_STATUS
dp_lite_mon_set_tx_peer_config(struct dp_soc *soc,
			       struct dp_pdev_be *be_pdev,
			       struct cdp_lite_mon_peer_config *peer_config)
{
	struct dp_lite_mon_tx_config *lite_mon_tx_config;
	struct dp_mon_pdev_be *be_mon_pdev =
			(struct dp_mon_pdev_be *)be_pdev->pdev.monitor_pdev;
	struct dp_lite_mon_peer *peer = NULL;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	if (!be_mon_pdev)
		return QDF_STATUS_E_FAILURE;

	lite_mon_tx_config = be_mon_pdev->lite_mon_tx_config;
	if (!lite_mon_tx_config) {
		dp_mon_err("lite mon tx config is null");
		return QDF_STATUS_E_FAILURE;
	}

	if (peer_config->type != CDP_LITE_MON_PEER_TYPE_ASSOCIATED) {
		dp_mon_err("Invalid tx peer type");
		return QDF_STATUS_E_INVAL;
	}

	if (peer_config->action == CDP_LITE_MON_PEER_ADD) {
		peer = qdf_mem_malloc(sizeof(struct dp_lite_mon_peer));
		if (!peer)
			return QDF_STATUS_E_NOMEM;
	}

	qdf_spin_lock_bh(&lite_mon_tx_config->lite_mon_tx_lock);
	status = dp_lite_mon_update_peers(&lite_mon_tx_config->tx_config,
					  peer_config, peer);
	qdf_spin_unlock_bh(&lite_mon_tx_config->lite_mon_tx_lock);

	if (status != QDF_STATUS_SUCCESS) {
		if (peer)
			qdf_mem_free(peer);
	}

	return status;
}

/**
 * dp_lite_mon_set_rx_peer_config - Update lite mon rx peer list
 * @soc: soc
 * @be_pdev: be dp pdev
 * @peer_config: new peer config
 *
 * Return: QDF_STATUS
 */
static inline QDF_STATUS
dp_lite_mon_set_rx_peer_config(struct dp_soc *soc,
			       struct dp_pdev_be *be_pdev,
			       struct cdp_lite_mon_peer_config *peer_config)
{
	struct dp_lite_mon_rx_config *lite_mon_rx_config;
	struct dp_mon_pdev_be *be_mon_pdev =
			(struct dp_mon_pdev_be *)be_pdev->pdev.monitor_pdev;
	struct dp_lite_mon_peer *peer = NULL;
	enum cdp_nac_param_cmd cmd;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	if (!be_mon_pdev)
		return QDF_STATUS_E_FAILURE;

	lite_mon_rx_config = be_mon_pdev->lite_mon_rx_config;
	if (!lite_mon_rx_config) {
		dp_mon_err("lite mon rx config is null");
		return QDF_STATUS_E_FAILURE;
	}

	if ((peer_config->type != CDP_LITE_MON_PEER_TYPE_ASSOCIATED) &&
	    (peer_config->type != CDP_LITE_MON_PEER_TYPE_NON_ASSOCIATED)) {
		dp_mon_err("Invalid rx peer type");
		return QDF_STATUS_E_INVAL;
	}

	if (peer_config->action == CDP_LITE_MON_PEER_ADD) {
		peer = qdf_mem_malloc(sizeof(struct dp_lite_mon_peer));
		if (!peer)
			return QDF_STATUS_E_NOMEM;
	}

	qdf_spin_lock_bh(&lite_mon_rx_config->lite_mon_rx_lock);
	status = dp_lite_mon_update_peers(&lite_mon_rx_config->rx_config,
					  peer_config, peer);
	qdf_spin_unlock_bh(&lite_mon_rx_config->lite_mon_rx_lock);

	if (status == QDF_STATUS_SUCCESS) {
		/* configure peer to fw */
		if (peer_config->action == CDP_LITE_MON_PEER_ADD)
			cmd = CDP_NAC_PARAM_ADD;
		else
			cmd = CDP_NAC_PARAM_DEL;

		if (soc->cdp_soc.ol_ops->config_lite_mon_peer)
			soc->cdp_soc.ol_ops->config_lite_mon_peer(soc->ctrl_psoc,
								  be_pdev->pdev.pdev_id,
								  peer_config->vdev_id,
								  cmd, peer_config->mac);
	} else {
		/* peer update failed */
		if (peer)
			qdf_mem_free(peer);
	}

	return status;
}

/**
 * dp_lite_mon_set_peer_config - set lite mon tx/rx peer
 * @soc_hdl: dp soc hdl
 * @peer_config: new peer config
 * @pdev_id: pdev id
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_lite_mon_set_peer_config(struct cdp_soc_t *soc_hdl,
			    struct cdp_lite_mon_peer_config *peer_config,
			    uint8_t pdev_id)
{
	struct dp_soc *soc = cdp_soc_t_to_dp_soc(soc_hdl);
	struct dp_pdev *pdev =
		dp_get_pdev_from_soc_pdev_id_wifi3(soc,
						   pdev_id);
	struct dp_pdev_be *be_pdev = dp_get_be_pdev_from_dp_pdev(pdev);
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	if (!be_pdev || !peer_config)
		return QDF_STATUS_E_FAILURE;

	if (peer_config->direction == CDP_LITE_MON_DIRECTION_RX) {
		status = dp_lite_mon_set_rx_peer_config(soc, be_pdev,
							peer_config);
	} else if (peer_config->direction == CDP_LITE_MON_DIRECTION_TX) {
		status = dp_lite_mon_set_tx_peer_config(soc, be_pdev,
							peer_config);
	} else {
		dp_mon_err("Invalid direction");
		status = QDF_STATUS_E_INVAL;
	}

	return status;
}

/**
 * dp_lite_mon_get_peers - get peer list
 * @config: lite mon tx/rx config
 * @peer_info: str to hold peer list
 *
 * Return: void
 */
static void
dp_lite_mon_get_peers(struct dp_lite_mon_config *config,
		      struct cdp_lite_mon_peer_info *peer_info)
{
	struct dp_lite_mon_peer *peer;

	peer_info->count = 0;
	TAILQ_FOREACH(peer, &config->peer_list, peer_list_elem) {
		if (peer->type != peer_info->type)
			continue;
		if (peer_info->count < CDP_LITE_MON_PEER_MAX) {
			qdf_mem_copy(&peer_info->mac[peer_info->count],
				     &peer->peer_mac.raw[0],
				     QDF_MAC_ADDR_SIZE);
			peer_info->count++;
		}
	}
}

/**
 * dp_lite_mon_get_tx_peer_config - get lite mon tx peer list
 * @be_mon_pdev: be dp mon pdev
 * @peer_info: str to hold peer list
 *
 * Return: void
 */
static inline void
dp_lite_mon_get_tx_peer_config(struct dp_mon_pdev_be *be_mon_pdev,
			       struct cdp_lite_mon_peer_info *peer_info)
{
	struct dp_lite_mon_tx_config *lite_mon_tx_config;

	lite_mon_tx_config = be_mon_pdev->lite_mon_tx_config;
	if (!lite_mon_tx_config) {
		dp_mon_err("lite mon tx config is null");
		return;
	}

	qdf_spin_lock_bh(&lite_mon_tx_config->lite_mon_tx_lock);
	dp_lite_mon_get_peers(&lite_mon_tx_config->tx_config,
			      peer_info);
	qdf_spin_unlock_bh(&lite_mon_tx_config->lite_mon_tx_lock);
}

/**
 * dp_lite_mon_get_rx_peer_config - get lite mon rx peer list
 * @be_mon_pdev: be dp mon pdev
 * @peer_info: str to hold peer list
 *
 * Return: void
 */
static inline void
dp_lite_mon_get_rx_peer_config(struct dp_mon_pdev_be *be_mon_pdev,
			       struct cdp_lite_mon_peer_info *peer_info)
{
	struct dp_lite_mon_rx_config *lite_mon_rx_config;

	lite_mon_rx_config = be_mon_pdev->lite_mon_rx_config;
	if (!lite_mon_rx_config) {
		dp_mon_err("lite mon rx config is null");
		return;
	}

	qdf_spin_lock_bh(&lite_mon_rx_config->lite_mon_rx_lock);
	dp_lite_mon_get_peers(&lite_mon_rx_config->rx_config,
			      peer_info);
	qdf_spin_unlock_bh(&lite_mon_rx_config->lite_mon_rx_lock);
}

/**
 * dp_lite_mon_get_peer_config - get lite mon tx/rx peer list
 * @soc_hdl: dp soc hdl
 * @peer_info: str to hold peer list
 * @pdev_id: pdev id
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_lite_mon_get_peer_config(struct cdp_soc_t *soc_hdl,
			    struct cdp_lite_mon_peer_info *peer_info,
			    uint8_t pdev_id)
{
	struct dp_pdev *pdev =
		dp_get_pdev_from_soc_pdev_id_wifi3(cdp_soc_t_to_dp_soc(soc_hdl),
						   pdev_id);
	struct dp_pdev_be *be_pdev = dp_get_be_pdev_from_dp_pdev(pdev);
	struct dp_mon_pdev_be *be_mon_pdev = NULL;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	if (!be_pdev || !peer_info)
		return QDF_STATUS_E_FAILURE;

	be_mon_pdev = (struct dp_mon_pdev_be *)be_pdev->pdev.monitor_pdev;
	if (!be_mon_pdev)
		return QDF_STATUS_E_FAILURE;

	if (peer_info->direction == CDP_LITE_MON_DIRECTION_RX) {
		dp_lite_mon_get_rx_peer_config(be_mon_pdev,
					       peer_info);
	} else if (peer_info->direction == CDP_LITE_MON_DIRECTION_TX) {
		dp_lite_mon_get_tx_peer_config(be_mon_pdev,
					       peer_info);
	} else {
		dp_mon_err("Invalid direction");
		status = QDF_STATUS_E_INVAL;
	}

	return status;
}

/**
 * dp_lite_mon_is_level_msdu - check if level is msdu
 * @mon_pdev: monitor pdev handle
 *
 * Return: 0 if level is not msdu else return 1
 */
int dp_lite_mon_is_level_msdu(struct dp_mon_pdev *mon_pdev)
{
	struct dp_mon_pdev_be *be_mon_pdev;
	struct dp_lite_mon_config *rx_config;

	if (!mon_pdev)
		return 0;

	be_mon_pdev = dp_get_be_mon_pdev_from_dp_mon_pdev(mon_pdev);
	rx_config = &be_mon_pdev->lite_mon_rx_config->rx_config;
	if (be_mon_pdev->lite_mon_rx_config->rx_config.level ==
						CDP_LITE_MON_LEVEL_MSDU)
		return 1;

	return 0;
}

/**
 * dp_lite_mon_is_rx_enabled - get lite mon rx enable status
 * @mon_pdev: monitor pdev handle
 *
 * Return: 0 if disabled, 1 if enabled
 */
int dp_lite_mon_is_rx_enabled(struct dp_mon_pdev *mon_pdev)
{
	struct dp_mon_pdev_be *be_mon_pdev;

	if (!mon_pdev)
		return 0;

	be_mon_pdev = dp_get_be_mon_pdev_from_dp_mon_pdev(mon_pdev);

	return be_mon_pdev->lite_mon_rx_config->rx_config.enable;
}

/**
 * dp_lite_mon_is_tx_enabled - get lite mon tx enable status
 * @mon_pdev: monitor pdev handle
 *
 * Return: 0 if disabled, 1 if enabled
 */
int dp_lite_mon_is_tx_enabled(struct dp_mon_pdev *mon_pdev)
{
	struct dp_mon_pdev_be *be_mon_pdev;

	if (!mon_pdev)
		return 0;

	be_mon_pdev = dp_get_be_mon_pdev_from_dp_mon_pdev(mon_pdev);

	return be_mon_pdev->lite_mon_tx_config->tx_config.enable;
}

/**
 * dp_lite_mon_is_enabled - get lite mon enable status
 * @soc_hdl: dp soc hdl
 * @pdev_id: pdev id
 * @direction: tx/rx
 *
 * Return: 0 if disabled, 1 if enabled
 */
int
dp_lite_mon_is_enabled(struct cdp_soc_t *soc_hdl,
		       uint8_t pdev_id, uint8_t direction)
{
	struct dp_pdev *pdev =
		dp_get_pdev_from_soc_pdev_id_wifi3(cdp_soc_t_to_dp_soc(soc_hdl),
						   pdev_id);

	if (!pdev)
		return 0;

	if (direction == CDP_LITE_MON_DIRECTION_RX)
		return dp_lite_mon_is_rx_enabled(pdev->monitor_pdev);
	else if (direction == CDP_LITE_MON_DIRECTION_TX)
		return dp_lite_mon_is_tx_enabled(pdev->monitor_pdev);

	return 0;
}

/**
 * dp_lite_mon_config_nac_peer - config nac peer and filter
 * @soc_hdl: dp soc hdl
 * @vdev_id: vdev id
 * @cmd: peer cmd
 * @macaddr: peer mac
 *
 * Return: 1 if success, 0 if failure
 */
int
dp_lite_mon_config_nac_peer(struct cdp_soc_t *soc_hdl,
			    uint8_t vdev_id,
			    uint32_t cmd, uint8_t *macaddr)
{
	struct dp_soc *soc = cdp_soc_t_to_dp_soc(soc_hdl);
	struct dp_vdev *vdev =
		dp_vdev_get_ref_by_id(soc, vdev_id,
				      DP_MOD_ID_CDP);
	struct dp_pdev_be *be_pdev;
	struct dp_mon_pdev_be *be_mon_pdev;
	struct dp_lite_mon_config *rx_config;
	struct cdp_lite_mon_peer_config peer_config;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	if (!vdev || !vdev->pdev || !macaddr) {
		dp_mon_err("Invalid vdev or macaddr");
		goto fail;
	}

	be_pdev = dp_get_be_pdev_from_dp_pdev(vdev->pdev);
	be_mon_pdev = (struct dp_mon_pdev_be *)be_pdev->pdev.monitor_pdev;
	if (!be_mon_pdev) {
		dp_mon_err("Invalid mon pdev");
		goto fail;
	}

	/* populate peer config fields */
	qdf_mem_zero(&peer_config,
		     sizeof(struct cdp_lite_mon_peer_config));
	peer_config.direction = CDP_LITE_MON_DIRECTION_RX;
	peer_config.type = CDP_LITE_MON_PEER_TYPE_NON_ASSOCIATED;
	peer_config.vdev_id = vdev_id;
	qdf_mem_copy(peer_config.mac, macaddr, QDF_MAC_ADDR_SIZE);
	if (cmd == DP_NAC_PARAM_ADD)
		peer_config.action = CDP_LITE_MON_PEER_ADD;
	else if (cmd == DP_NAC_PARAM_DEL)
		peer_config.action = CDP_LITE_MON_PEER_REMOVE;

	rx_config = &be_mon_pdev->lite_mon_rx_config->rx_config;
	if (peer_config.action == CDP_LITE_MON_PEER_ADD) {
		/* first neighbor */
		if (!rx_config->peer_count[CDP_LITE_MON_PEER_TYPE_NON_ASSOCIATED]) {
			struct cdp_lite_mon_filter_config filter_config;

			/* setup filter settings */
			qdf_mem_zero(&filter_config,
				     sizeof(struct cdp_lite_mon_filter_config));
			filter_config.disable = 0;
			filter_config.direction = CDP_LITE_MON_DIRECTION_RX;
			filter_config.level = CDP_LITE_MON_LEVEL_PPDU;
			filter_config.data_filter[DP_MON_FRM_FILTER_MODE_MD] =
							CDP_LITE_MON_FILTER_ALL;
			filter_config.len[WLAN_FC0_TYPE_DATA] =
							CDP_LITE_MON_LEN_128B;
			filter_config.metadata = DP_LITE_MON_RTAP_HDR_BITMASK;
			filter_config.vdev_id = vdev_id;
			status = dp_lite_mon_set_rx_config(be_pdev,
							   &filter_config);
			if (status != QDF_STATUS_SUCCESS)
				goto fail;
		}

		/* add peer */
		status = dp_lite_mon_set_rx_peer_config(soc, be_pdev,
							&peer_config);
		if (status != QDF_STATUS_SUCCESS)
			goto fail;
	} else if (peer_config.action == CDP_LITE_MON_PEER_REMOVE) {
		/* remove peer */
		status = dp_lite_mon_set_rx_peer_config(soc, be_pdev,
							&peer_config);
		if (status != QDF_STATUS_SUCCESS)
			goto fail;

		/* last neighbor */
		if (!rx_config->peer_count[CDP_LITE_MON_PEER_TYPE_NON_ASSOCIATED]) {
			struct cdp_lite_mon_filter_config filter_config;

			/* reset filter settings */
			qdf_mem_zero(&filter_config,
				     sizeof(struct cdp_lite_mon_filter_config));
			filter_config.disable = 1;
			filter_config.direction = CDP_LITE_MON_DIRECTION_RX;
			status = dp_lite_mon_set_rx_config(be_pdev,
							   &filter_config);
			if (status != QDF_STATUS_SUCCESS)
				goto fail;
		}
	}
	dp_vdev_unref_delete(soc, vdev, DP_MOD_ID_CDP);

	return 1;

fail:
	if (vdev)
		dp_vdev_unref_delete(soc, vdev, DP_MOD_ID_CDP);

	return 0;
}

/**
 * dp_lite_mon_get_nac_peer_rssi - get nac peer rssi
 * @soc_hdl: dp soc hdl
 * @vdev_id: vdev id
 * @macaddr: peer mac
 * @rssi: peer rssi
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_lite_mon_get_nac_peer_rssi(struct cdp_soc_t *soc_hdl,
			      uint8_t vdev_id, char *macaddr,
			      uint8_t *rssi)
{
	struct dp_soc *soc = cdp_soc_t_to_dp_soc(soc_hdl);
	struct dp_vdev *vdev =
		dp_vdev_get_ref_by_id(soc, vdev_id,
				      DP_MOD_ID_CDP);
	struct dp_pdev_be *be_pdev;
	struct dp_mon_pdev_be *be_mon_pdev;
	struct dp_lite_mon_config *rx_config;
	struct dp_lite_mon_rx_config *lite_mon_rx_config;
	struct dp_lite_mon_peer *peer;
	QDF_STATUS status = QDF_STATUS_E_FAILURE;

	if (!vdev || !vdev->pdev || !macaddr) {
		dp_mon_err("Invalid vdev or macaddr");
		return status;
	}

	be_pdev = dp_get_be_pdev_from_dp_pdev(vdev->pdev);
	be_mon_pdev = (struct dp_mon_pdev_be *)be_pdev->pdev.monitor_pdev;
	if (!be_mon_pdev) {
		dp_mon_err("Invalid mon pdev");
		return status;
	}

	lite_mon_rx_config = be_mon_pdev->lite_mon_rx_config;
	rx_config = &lite_mon_rx_config->rx_config;
	qdf_spin_lock_bh(&lite_mon_rx_config->lite_mon_rx_lock);
	TAILQ_FOREACH(peer, &rx_config->peer_list, peer_list_elem) {
		if (peer->type !=
			CDP_LITE_MON_PEER_TYPE_NON_ASSOCIATED)
			continue;

		if (qdf_mem_cmp(&peer->peer_mac.raw[0],
				macaddr,
				QDF_MAC_ADDR_SIZE) == 0) {
			*rssi = peer->rssi;
			status = QDF_STATUS_SUCCESS;
			break;
		}
	}
	qdf_spin_unlock_bh(&lite_mon_rx_config->lite_mon_rx_lock);
	dp_vdev_unref_delete(soc, vdev, DP_MOD_ID_CDP);

	return status;
}

/**
 * dp_lite_mon_config_nac_rssi_peer - config nac rssi peer
 * @soc_hdl: dp soc hdl
 * @cmd: peer cmd
 * @vdev_id: vdev id
 * @bssid: peer bssid
 * @macaddr: peer mac
 * @chan_num: channel num
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_lite_mon_config_nac_rssi_peer(struct cdp_soc_t *soc_hdl,
				 uint8_t vdev_id,
				 enum cdp_nac_param_cmd cmd,
				 char *bssid, char *macaddr,
				 uint8_t chan_num)
{
	int ret;

	ret = dp_lite_mon_config_nac_peer(soc_hdl, vdev_id,
					  cmd, macaddr);
	if (!ret) {
		dp_mon_err("failed to add nac rssi peers");
		return QDF_STATUS_E_FAILURE;
	}

	return QDF_STATUS_SUCCESS;
}

/**
 * dp_lite_mon_rx_filter_check - check if mpdu rcvd match filter setting
 * @ppdu_info: ppdu info context
 * @config: current lite mon config
 * @user: user id
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS
dp_lite_mon_rx_filter_check(struct hal_rx_ppdu_info *ppdu_info,
			    struct dp_lite_mon_config *config,
			    uint8_t user)
{
	uint8_t filter_category;

	filter_category = ppdu_info->rx_user_status[user].filter_category;
	switch (filter_category) {
	case DP_MPDU_FILTER_CATEGORY_FP:
		if (config->fp_enabled &&
		    (!config->peer_count[CDP_LITE_MON_PEER_TYPE_ASSOCIATED] ||
		     !config->fpmo_enabled))
			return QDF_STATUS_SUCCESS;
		break;
	case DP_MPDU_FILTER_CATEGORY_MD:
		if (config->md_enabled)
			return QDF_STATUS_SUCCESS;
		break;
	case DP_MPDU_FILTER_CATEGORY_MO:
		if (config->mo_enabled &&
		    (!config->peer_count[CDP_LITE_MON_PEER_TYPE_NON_ASSOCIATED] ||
		     !config->md_enabled))
			return QDF_STATUS_SUCCESS;
		break;
	case DP_MPDU_FILTER_CATEGORY_FP_MO:
		if (config->fpmo_enabled)
			return QDF_STATUS_SUCCESS;
		break;
	}

	return QDF_STATUS_E_FAILURE;
}

#ifdef META_HDR_NOT_YET_SUPPORTED
#ifdef WLAN_SUPPORT_RX_PROTOCOL_TYPE_TAG
/**
 * dp_lite_mon_update_protocol_tag - update ptag to headroom
 * @be_pdev: be pdev context
 * @frag_idx: frag index
 * @mpdu_nbuf: mpdu nbuf
 *
 * Return: void
 */
static inline void
dp_lite_mon_update_protocol_tag(struct dp_pdev_be *be_pdev,
				uint8_t frag_idx,
				qdf_nbuf_t mpdu_nbuf)
{
	uint16_t cce_metadata = 0;
	uint16_t protocol_tag = 0;
	uint8_t *nbuf_head = NULL;

	if (qdf_unlikely(be_pdev->pdev.is_rx_protocol_tagging_enabled)) {
		nbuf_head = qdf_nbuf_head(mpdu_nbuf);
		nbuf_head += (frag_idx * DP_RX_MON_CCE_FSE_METADATA_SIZE);

		cce_metadata = *((uint16_t *)nbuf_head);
		/* check if cce_metadata is in valid range */
		if (qdf_likely(cce_metadata >= RX_PROTOCOL_TAG_START_OFFSET) &&
			  (cce_metadata < (RX_PROTOCOL_TAG_START_OFFSET +
			   RX_PROTOCOL_TAG_MAX))) {
			/*
			 * The CCE metadata received is just the
			 * packet_type + RX_PROTOCOL_TAG_START_OFFSET
			 */
			cce_metadata -= RX_PROTOCOL_TAG_START_OFFSET;

			/*
			 * Update the nbuf headroom with the user-specified
			 * tag/metadata by looking up tag value for
			 * received protocol type */
			protocol_tag = be_pdev->pdev.rx_proto_tag_map[cce_metadata].tag;
		}
	}
	nbuf_head = qdf_nbuf_head(mpdu_nbuf);
	nbuf_head += (frag_idx * DP_RX_MON_PF_TAG_SIZE);
	*((uint16_t *)nbuf_head) = protocol_tag;
}
#else
static inline void
dp_lite_mon_update_protocol_tag(struct dp_pdev_be *be_pdev,
				uint8_t frag_idx,
				qdf_nbuf_t mpdu_nbuf)
{
}
#endif

#ifdef WLAN_SUPPORT_RX_FLOW_TAG
/**
 * dp_lite_mon_update_flow_tag - update ftag to headroom
 * @dp_soc: dp_soc context
 * @frag_idx: frag index
 * @mpdu_nbuf: mpdu nbuf
 *
 * Return: void
 */
static inline void
dp_lite_mon_update_flow_tag(struct dp_soc *soc,
			    uint8_t frag_idx,
			    qdf_nbuf_t mpdu_nbuf)
{
	uint16_t flow_tag = 0;
	uint8_t *nbuf_head = NULL;

	if (qdf_unlikely(wlan_cfg_is_rx_flow_tag_enabled(soc->wlan_cfg_ctx))) {
		nbuf_head = qdf_nbuf_head(mpdu_nbuf);
		nbuf_head += ((frag_idx * DP_RX_MON_CCE_FSE_METADATA_SIZE) +
				DP_RX_MON_CCE_METADATA_SIZE);

		/* limit FSE metadata to 16bit */
		flow_tag = (uint16_t)(*((uint32_t *)nbuf_head) & 0xFFFF);
	}
	nbuf_head = qdf_nbuf_head(mpdu_nbuf);
	nbuf_head += (frag_idx * DP_RX_MON_PF_TAG_SIZE) + sizeof(uint16_t);
	*((uint16_t *)nbuf_head) = flow_tag;
}
#else
static inline void
dp_lite_mon_update_flow_tag(struct dp_soc *soc,
			    uint8_t frag_idx,
			    qdf_nbuf_t mpdu_nbuf)
{
}
#endif
#endif

/**
 * dp_lite_mon_rx_adjust_mpdu_len - adjust mpdu len as per set type len
 * @be_pdev: be pdev context
 * @ppdu_info: ppdu info context
 * @config: curr lite mon config
 * @mpdu_nbuf: mpdu nbuf
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS
dp_lite_mon_rx_adjust_mpdu_len(struct dp_pdev_be *be_pdev,
			       struct hal_rx_ppdu_info *ppdu_info,
			       struct dp_lite_mon_config *config,
			       qdf_nbuf_t mpdu_nbuf)
{
	struct dp_soc *soc = be_pdev->pdev.soc;
	struct ieee80211_frame *wh;
	qdf_nbuf_t curr_nbuf = NULL;
	uint32_t frag_size;
	int trim_size;
	uint16_t type_len;
	uint8_t type;
	uint8_t num_frags = 0;
	uint8_t frag_iter = 0;
	bool is_head_nbuf = true;
	bool is_data_pkt = false;
	bool is_rx_mon_protocol_flow_tag_en;

	if (qdf_unlikely(!soc))
		return QDF_STATUS_E_INVAL;

	wh = (struct ieee80211_frame *)qdf_nbuf_get_frag_addr(mpdu_nbuf, 0);
	type = ((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) >> IEEE80211_FC0_TYPE_SHIFT);
	type_len = (type < CDP_MON_FRM_TYPE_MAX) ? config->len[type] : 0;
	if (qdf_unlikely(!type_len || type_len == CDP_LITE_MON_LEN_FULL)) {
		dp_mon_debug("Invalid type(%d) len, mpdu len adjust failed", type);
		return QDF_STATUS_E_INVAL;
	}

	/* if user has configured different custom pkt len
	 * (for ex: data 256bytes, mgmt/ctrl 128bytes) for
	 * different types we subscribe header tlv with max
	 * custom len(265bytes for above ex). We need to trim
	 * mgmt/ctrl hdrs to their respective lengths(128bytes for above ex).
	 * Below logic handles that */

	if (ppdu_info->rx_status.frame_control_info_valid &&
	    ((ppdu_info->rx_status.frame_control & IEEE80211_FC0_TYPE_MASK) ==
	      IEEE80211_FC0_TYPE_DATA))
		is_data_pkt = true;

	is_rx_mon_protocol_flow_tag_en =
		wlan_cfg_is_rx_mon_protocol_flow_tag_enabled(soc->wlan_cfg_ctx);

	for (curr_nbuf = mpdu_nbuf; curr_nbuf;) {
		num_frags = qdf_nbuf_get_nr_frags(curr_nbuf);
		if (num_frags > 1 &&
		    config->level != CDP_LITE_MON_LEVEL_MSDU) {
			dp_mon_err("level mpdu/ppdu expects only single frag");
			return QDF_STATUS_E_INVAL;
		}

		for (frag_iter = 0; frag_iter < num_frags; ++frag_iter) {
			frag_size = qdf_nbuf_get_frag_size_by_idx(curr_nbuf,
								  frag_iter);
			if (frag_size > type_len) {
				trim_size = frag_size - type_len;
				qdf_nbuf_trim_add_frag_size(curr_nbuf,
							    frag_iter,
							    -(trim_size), 0);
			}

#ifdef META_HDR_NOT_YET_SUPPORTED
			if (config->level == CDP_LITE_MON_LEVEL_MSDU) {
				if (is_data_pkt &&
				    is_rx_mon_protocol_flow_tag_en) {
					/* update protocol tag from cce metadata */
					dp_lite_mon_update_protocol_tag(be_pdev,
									frag_iter,
									mpdu_nbuf);

					/* update flow tag from fse metadata */
					dp_lite_mon_update_flow_tag(soc,
								    frag_iter,
								    mpdu_nbuf);

					/* litemon pending :check to add pf tag
					 * trailer info for debug
					 */
				}
			}
#endif /* META_HDR_NOT_YET_SUPPORTED */
		}
		if (is_head_nbuf) {
			curr_nbuf = qdf_nbuf_get_ext_list(curr_nbuf);
			is_head_nbuf = false;
		} else
			curr_nbuf = qdf_nbuf_queue_next(curr_nbuf);
	}

	return QDF_STATUS_SUCCESS;
}

#ifdef META_HDR_NOT_YET_SUPPORTED
/**
 * dp_lite_mon_add_tlv - add tlv to headroom
 * @id: tlv id
 * @len: tlv len
 * @value: tlv value
 * @mpdu_nbuf: mpdu nbuf
 *
 * Return: number of bytes pushed
 */
static inline
uint32_t dp_lite_mon_add_tlv(uint8_t id, uint16_t len, void *value,
			     qdf_nbuf_t mpdu_nbuf)
{
	uint8_t *dest = NULL;
	uint32_t num_bytes_pushed = 0;

	/* Add tlv id field */
	dest = qdf_nbuf_push_head(mpdu_nbuf, sizeof(uint8_t));
	if (qdf_likely(dest)) {
		*((uint8_t *)dest) = id;
		num_bytes_pushed += sizeof(uint8_t);
	}

	/* Add tlv len field */
	dest = qdf_nbuf_push_head(mpdu_nbuf, sizeof(uint16_t));
	if (qdf_likely(dest)) {
		*((uint16_t *)dest) = len;
		num_bytes_pushed += sizeof(uint16_t);
	}

	/* Add tlv value field */
	dest = qdf_nbuf_push_head(mpdu_nbuf, len);
	if (qdf_likely(dest)) {
		qdf_mem_copy(dest, value, len);
		num_bytes_pushed += len;
	}

	return num_bytes_pushed;
}

/**
 * dp_lite_mon_add_pf_tag_tlv_to_headroom - add pftag tlv to headroom
 * @mpdu_nbuf: mpdu nbuf
 *
 * Return: void
 */
static inline void
dp_lite_mon_add_pf_tag_tlv_to_headroom(qdf_nbuf_t mpdu_nbuf)
{
	qdf_nbuf_t curr_nbuf;
	uint8_t tlv_id;
	uint16_t tlv_len;
	uint32_t pull_bytes = 0;
	bool is_head_nbuf = true;

	if (!qdf_nbuf_get_nr_frags(mpdu_nbuf))
		return;

	tlv_id = DP_LITE_MON_META_HDR_TLV_PFTAG;
	for (curr_nbuf = mpdu_nbuf; curr_nbuf;) {
		/* litemon pending: check for available headroom
		 * also check if we should consider max nr frags for copying */
		tlv_len = qdf_nbuf_get_nr_frags(curr_nbuf) * DP_RX_MON_PF_TAG_SIZE;
		pull_bytes += dp_lite_mon_add_tlv(tlv_id, tlv_len,
						  qdf_nbuf_head(curr_nbuf),
						  curr_nbuf);
		qdf_nbuf_pull_head(curr_nbuf, pull_bytes);

		if (is_head_nbuf) {
			curr_nbuf = qdf_nbuf_get_ext_list(curr_nbuf);
			is_head_nbuf = false;
		} else
			curr_nbuf = qdf_nbuf_queue_next(curr_nbuf);
	}
}

/**
 * dp_lite_mon_add_meta_header_to_headroom - add lite mon meta hdr
 * @be_pdev: be pdev context
 * @ppdu_info: ppdu info context
 * @mpdu_nbuf: mpdu nbuf
 *
 * Return: void
 */
void
dp_lite_mon_add_meta_header_to_headroom(struct dp_pdev_be *be_pdev,
					struct hal_rx_ppdu_info *ppdu_info,
					qdf_nbuf_t mpdu_nbuf)
{
	struct dp_mon_pdev_be *be_mon_pdev;
	struct dp_lite_mon_config *config;
	struct dp_soc *soc = be_pdev->pdev.soc;
	uint32_t pull_bytes = 0;
	uint16_t meta_hdr_marker = DP_LITE_MON_META_HDR_MARKER;
	uint8_t tlv_id;
	uint16_t tlv_len;

	if (qdf_unlikely(!soc))
		return;

	be_mon_pdev = (struct dp_mon_pdev_be *)be_pdev->pdev.monitor_pdev;
	config = &be_mon_pdev->lite_mon_rx_config->rx_config;

	/* litemon pending: put check for available headroom it must be
	   large enough to hold meta header */

	/* Add marker: we first add marker, this will indicate
	 * beginning of meta header information in headroom.
	 */
	qdf_nbuf_push_head(mpdu_nbuf, sizeof(meta_hdr_marker));
	qdf_mem_copy(qdf_nbuf_data(mpdu_nbuf), &meta_hdr_marker,
		     sizeof(meta_hdr_marker));
	pull_bytes += sizeof(meta_hdr_marker);

	/* Add Meta hdr: Meta hdr consists of various information in
	 * TLV format [TLV ID][TLV Len][TLV Value] */

	/* add PPDU ID TLV */
	tlv_id = DP_LITE_MON_META_HDR_TLV_PPDUID;
	tlv_len = sizeof(ppdu_info->com_info.ppdu_id);
	pull_bytes += dp_lite_mon_add_tlv(tlv_id, tlv_len,
					  &ppdu_info->com_info.ppdu_id,
					  mpdu_nbuf);

	/* add PFTAG TLV */
	if (config->level == CDP_LITE_MON_LEVEL_MSDU) {
		/* proceed only if data pkt */
		if (ppdu_info->rx_status.frame_control_info_valid &&
		    ((ppdu_info->rx_status.frame_control & IEEE80211_FC0_TYPE_MASK) ==
		      IEEE80211_FC0_TYPE_DATA)) {
			bool is_rx_mon_protocol_flow_tag_en;

			/* check if rx mon protocol flow tag enabled */
			is_rx_mon_protocol_flow_tag_en =
				wlan_cfg_is_rx_mon_protocol_flow_tag_enabled(
							soc->wlan_cfg_ctx);
			if (is_rx_mon_protocol_flow_tag_en)
				dp_lite_mon_add_pf_tag_tlv_to_headroom(mpdu_nbuf);
		}
	}

	qdf_nbuf_pull_head(mpdu_nbuf, pull_bytes);
}
#endif /* META_HDR_NOT_YET_SUPPORTED */

/**
 * dp_lite_mon_rx_mpdu_process - core lite mon mpdu processing
 * @pdev: pdev context
 * @ppdu_info: ppdu info context
 * @mon_mpdu: mpdu nbuf
 * @mpdu_id: mpdu id
 * @user: user id
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_lite_mon_rx_mpdu_process(struct dp_pdev *pdev,
			    struct hal_rx_ppdu_info *ppdu_info,
			    qdf_nbuf_t mon_mpdu, uint16_t mpdu_id,
			    uint8_t user)
{
	struct dp_pdev_be *be_pdev;
	struct dp_mon_pdev_be *be_mon_pdev;
	struct hal_rx_mon_mpdu_info *mpdu_meta;
	struct dp_lite_mon_rx_config *lite_mon_rx_config;
	struct dp_lite_mon_config *config;
	struct dp_vdev *lite_mon_vdev;
	struct dp_mon_vdev *mon_vdev;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	if (qdf_unlikely(!mon_mpdu || !ppdu_info)) {
		dp_mon_debug("mon mpdu or ppdu info is NULL");
		return QDF_STATUS_E_INVAL;
	}

	if (qdf_unlikely(!pdev)) {
		dp_mon_debug("pdev is NULL");
		return QDF_STATUS_E_INVAL;
	}
	be_pdev = dp_get_be_pdev_from_dp_pdev(pdev);

	be_mon_pdev = (struct dp_mon_pdev_be *)be_pdev->pdev.monitor_pdev;
	if (qdf_unlikely(!be_mon_pdev)) {
		dp_mon_debug("be mon pdev is NULL");
		return QDF_STATUS_E_INVAL;
	}

	lite_mon_rx_config = be_mon_pdev->lite_mon_rx_config;
	if (qdf_unlikely(!lite_mon_rx_config)) {
		dp_mon_debug("lite mon rx config is NULL");
		return QDF_STATUS_E_INVAL;
	}

	config = &lite_mon_rx_config->rx_config;
	qdf_spin_lock_bh(&lite_mon_rx_config->lite_mon_rx_lock);
	if (!config->enable) {
		qdf_spin_unlock_bh(&lite_mon_rx_config->lite_mon_rx_lock);
		dp_mon_debug("lite mon rx is not enabled");
		status = QDF_STATUS_E_INVAL;
		goto done;
	}

	/* update rssi for nac client, we can do this only for
	 * first mpdu as rssi is at ppdu level */
	if (mpdu_id == 0 &&
	    config->peer_count[CDP_LITE_MON_PEER_TYPE_NON_ASSOCIATED]) {
		struct dp_lite_mon_peer *peer = NULL;

		TAILQ_FOREACH(peer, &config->peer_list,
			      peer_list_elem) {
			if (!qdf_mem_cmp(&peer->peer_mac,
					 &ppdu_info->nac_info.mac_addr2,
					 QDF_MAC_ADDR_SIZE)) {
				peer->rssi = ppdu_info->rx_status.rssi_comb;
				break;
			}
		}
	}

	/* if level is PPDU we need only first MPDU, drop others
	 * and return success*/
	if (config->level == CDP_LITE_MON_LEVEL_PPDU && mpdu_id != 0) {
		qdf_spin_unlock_bh(&lite_mon_rx_config->lite_mon_rx_lock);
		dp_mon_debug("level is PPDU drop subsequent mpdu");
		qdf_nbuf_free(mon_mpdu);
		goto done;
	}

	if (QDF_STATUS_SUCCESS !=
		dp_lite_mon_rx_filter_check(ppdu_info, config, user)) {
		/* mpdu did not pass filter check drop and return success */
		qdf_spin_unlock_bh(&lite_mon_rx_config->lite_mon_rx_lock);
		dp_mon_debug("filter check did not pass, drop mpdu");
		qdf_nbuf_free(mon_mpdu);
		goto done;
	}

	mpdu_meta = (struct hal_rx_mon_mpdu_info *)qdf_nbuf_data(mon_mpdu);
	if (mpdu_meta->full_pkt) {
		if (qdf_unlikely(mpdu_meta->truncated)) {
			qdf_spin_unlock_bh(&lite_mon_rx_config->lite_mon_rx_lock);
			qdf_nbuf_free(mon_mpdu);
			goto done;
		}
		/* full pkt, restitch required */
		dp_rx_mon_handle_full_mon(pdev, ppdu_info, mon_mpdu);
	} else {
		/* not a full pkt contains only headers, adjust header
		 * lengths as per user configured value */
		status = dp_lite_mon_rx_adjust_mpdu_len(be_pdev, ppdu_info,
							config, mon_mpdu);
		if (status != QDF_STATUS_SUCCESS) {
			qdf_spin_unlock_bh(&lite_mon_rx_config->lite_mon_rx_lock);
			dp_mon_debug("mpdu len adjustment failed");
			goto done;
		}
	}

	/* Add rtap header if requested */
	if (config->metadata & DP_LITE_MON_RTAP_HDR_BITMASK) {
		if (!qdf_nbuf_update_radiotap(&ppdu_info->rx_status,
					      mon_mpdu,
					      qdf_nbuf_headroom(mon_mpdu)))
			dp_mon_debug("rtap hdr update failed");
	}

	/* Add meta header if requested */
	if (config->metadata & DP_LITE_MON_META_HDR_BITMASK) {
#ifdef META_HDR_NOT_YET_SUPPORTED
		/* update meta hdr */
		dp_lite_mon_add_meta_header_to_headroom(be_pdev, ppdu_info,
							mon_mpdu);
#endif /* META_HDR_NOT_YET_SUPPORTED */
	}

	/* get output vap */
	lite_mon_vdev = config->lite_mon_vdev;
	qdf_spin_unlock_bh(&lite_mon_rx_config->lite_mon_rx_lock);

	/* If output vap is set send frame to stack else send WDI event */
	if (lite_mon_vdev && lite_mon_vdev->osif_vdev &&
	    lite_mon_vdev->monitor_vdev &&
	    lite_mon_vdev->monitor_vdev->osif_rx_mon) {
		mon_vdev = lite_mon_vdev->monitor_vdev;

		lite_mon_vdev->monitor_vdev->osif_rx_mon(lite_mon_vdev->osif_vdev,
							 mon_mpdu,
							 &ppdu_info->rx_status);
	} else {
		/* send lite mon rx wdi event */
		dp_wdi_event_handler(WDI_EVENT_LITE_MON_RX,
				     be_pdev->pdev.soc,
				     mon_mpdu,
				     HTT_INVALID_PEER,
				     WDI_NO_VAL,
				     be_pdev->pdev.pdev_id);
	}

done:
	return status;
}

/**
 * dp_lite_mon_rx_alloc - alloc lite mon rx context
 * @be_mon_pdev: be dp mon pdev
 *
 * Return: QDF_STATUS
 */
static inline
QDF_STATUS dp_lite_mon_rx_alloc(struct dp_mon_pdev_be *be_mon_pdev)
{
	be_mon_pdev->lite_mon_rx_config =
		qdf_mem_malloc(sizeof(struct dp_lite_mon_rx_config));
	if (!be_mon_pdev->lite_mon_rx_config) {
		dp_mon_err("rx lite mon alloc fail");
		return QDF_STATUS_E_NOMEM;
	}

	TAILQ_INIT(&be_mon_pdev->lite_mon_rx_config->rx_config.peer_list);
	qdf_spinlock_create(&be_mon_pdev->lite_mon_rx_config->lite_mon_rx_lock);

	return QDF_STATUS_SUCCESS;
}

/**
 * dp_lite_mon_tx_alloc - alloc lite mon tx context
 * @be_mon_pdev: be dp mon pdev
 *
 * Return: QDF_STATUS
 */
static inline
QDF_STATUS dp_lite_mon_tx_alloc(struct dp_mon_pdev_be *be_mon_pdev)
{
	be_mon_pdev->lite_mon_tx_config =
		qdf_mem_malloc(sizeof(struct dp_lite_mon_tx_config));
	if (!be_mon_pdev->lite_mon_tx_config) {
		dp_mon_err("tx lite mon alloc fail");
		return QDF_STATUS_E_NOMEM;
	}

	TAILQ_INIT(&be_mon_pdev->lite_mon_tx_config->tx_config.peer_list);
	qdf_spinlock_create(&be_mon_pdev->lite_mon_tx_config->lite_mon_tx_lock);

	return QDF_STATUS_SUCCESS;
}

/**
 * dp_lite_mon_alloc - alloc lite mon tx and rx contexts
 * @pdev: dp pdev
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
dp_lite_mon_alloc(struct dp_pdev *pdev)
{
	struct dp_pdev_be *be_pdev = dp_get_be_pdev_from_dp_pdev(pdev);
	struct dp_mon_pdev_be *be_mon_pdev =
			(struct dp_mon_pdev_be *)be_pdev->pdev.monitor_pdev;

	if (QDF_STATUS_SUCCESS !=
	    dp_lite_mon_rx_alloc(be_mon_pdev))
		return QDF_STATUS_E_FAILURE;

	if (QDF_STATUS_SUCCESS !=
	    dp_lite_mon_tx_alloc(be_mon_pdev))
		return QDF_STATUS_E_FAILURE;

	return QDF_STATUS_SUCCESS;
}

/**
 * dp_lite_mon_rx_dealloc - free lite mon rx context
 * @be_pdev: be dp pdev
 *
 * Return: void
 */
static inline
void dp_lite_mon_rx_dealloc(struct dp_pdev_be *be_pdev)
{
	struct dp_mon_pdev_be *be_mon_pdev =
		(struct dp_mon_pdev_be *)be_pdev->pdev.monitor_pdev;

	if (be_mon_pdev->lite_mon_rx_config) {
		/* disable rx lite mon */
		dp_lite_mon_disable_rx(&be_pdev->pdev);
		qdf_spinlock_destroy(&be_mon_pdev->lite_mon_rx_config->lite_mon_rx_lock);
		qdf_mem_free(be_mon_pdev->lite_mon_rx_config);
	}
}

/**
 * dp_lite_mon_tx_dealloc - free lite mon tx context
 * @be_pdev: be dp pdev
 *
 * Return: void
 */
static inline
void dp_lite_mon_tx_dealloc(struct dp_pdev_be *be_pdev)
{
	struct dp_mon_pdev_be *be_mon_pdev =
		(struct dp_mon_pdev_be *)be_pdev->pdev.monitor_pdev;

	if (be_mon_pdev->lite_mon_tx_config) {
		/* delete tx peers from the list */
		dp_lite_mon_free_peers(&be_mon_pdev->lite_mon_tx_config->tx_config);
		qdf_spinlock_destroy(&be_mon_pdev->lite_mon_tx_config->lite_mon_tx_lock);
		qdf_mem_free(be_mon_pdev->lite_mon_tx_config);
	}
}

/**
 * dp_lite_mon_vdev_delete - delete tx/rx lite mon vdev
 * @pdev: dp pdev
 * @vdev: dp vdev being deleted
 *
 * Return: void
 */
void
dp_lite_mon_vdev_delete(struct dp_pdev *pdev, struct dp_vdev *vdev)
{
	struct dp_pdev_be *be_pdev = dp_get_be_pdev_from_dp_pdev(pdev);
	struct dp_mon_pdev_be *be_mon_pdev =
			(struct dp_mon_pdev_be *)be_pdev->pdev.monitor_pdev;
	struct dp_lite_mon_config *config;

	if (be_mon_pdev->lite_mon_tx_config) {
		config = &be_mon_pdev->lite_mon_tx_config->tx_config;
		if (config->lite_mon_vdev &&
		    config->lite_mon_vdev == vdev)
			config->lite_mon_vdev = NULL;
	}

	if (be_mon_pdev->lite_mon_rx_config) {
		config = &be_mon_pdev->lite_mon_rx_config->rx_config;
		if (config->lite_mon_vdev &&
		    config->lite_mon_vdev == vdev)
			config->lite_mon_vdev = NULL;
	}
}

/**
 * dp_lite_mon_dealloc - free lite mon rx and tx contexts
 * @pdev: dp pdev
 *
 * Return: void
 */
void
dp_lite_mon_dealloc(struct dp_pdev *pdev)
{
	struct dp_pdev_be *be_pdev = dp_get_be_pdev_from_dp_pdev(pdev);

	dp_lite_mon_rx_dealloc(be_pdev);
	dp_lite_mon_tx_dealloc(be_pdev);
}
#endif
