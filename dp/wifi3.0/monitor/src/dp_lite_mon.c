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
#include "qdf_lock.h"
#include "dp_rx.h"
#include "dp_mon.h"
#include <dp_mon_2.0.h>
#include <dp_be.h>

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

		if (new_config->mgmt_filter[CDP_LITE_MON_MODE_FP] ||
		    new_config->ctrl_filter[CDP_LITE_MON_MODE_FP] ||
		    new_config->data_filter[CDP_LITE_MON_MODE_FP])
			curr_config->fp_enabled = true;

		if (new_config->mgmt_filter[CDP_LITE_MON_MODE_MD] ||
		    new_config->ctrl_filter[CDP_LITE_MON_MODE_MD] ||
		    new_config->data_filter[CDP_LITE_MON_MODE_MD])
			curr_config->md_enabled = true;

		if (new_config->mgmt_filter[CDP_LITE_MON_MODE_MO] ||
		    new_config->ctrl_filter[CDP_LITE_MON_MODE_MO] ||
		    new_config->data_filter[CDP_LITE_MON_MODE_MO])
			curr_config->mo_enabled = true;

		qdf_mem_copy(curr_config->len,
			     new_config->len,
			     sizeof(curr_config->len));

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
			filter_config.data_filter[CDP_LITE_MON_MODE_MD] =
							CDP_LITE_MON_FILTER_ALL;
			filter_config.len[CDP_LITE_MON_FRM_TYPE_DATA] =
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
