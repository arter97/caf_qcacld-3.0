/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

/**
 * DOC: Implement various notification handlers which are accessed
 * internally in mgmt_rx_srng component only.
 */

#include <wlan_mgmt_rx_srng_main.h>

QDF_STATUS wlan_mgmt_rx_srng_pdev_create_notification(
				struct wlan_objmgr_pdev *pdev, void *arg)
{
	struct mgmt_rx_srng_pdev_priv *pdev_priv;
	QDF_STATUS status;

	pdev_priv = qdf_mem_malloc(sizeof(*pdev_priv));
	if (!pdev_priv)
		return QDF_STATUS_E_NOMEM;

	status = wlan_objmgr_pdev_component_obj_attach(
					pdev, WLAN_UMAC_COMP_MGMT_RX_SRNG,
					pdev_priv, QDF_STATUS_SUCCESS);
	if (QDF_IS_STATUS_ERROR(status)) {
		mgmt_rx_srng_err("Failed to attach mgmt rx srng priv to pdev");
		return status;
	}
	pdev_priv->pdev = pdev;
	return status;
}

QDF_STATUS wlan_mgmt_rx_srng_pdev_destroy_notification(
				struct wlan_objmgr_pdev *pdev, void *arg)
{
	struct mgmt_rx_srng_pdev_priv *pdev_priv;
	struct mgmt_srng_cfg *mgmt_ring_cfg;
	QDF_STATUS status;

	pdev_priv = wlan_objmgr_pdev_get_comp_private_obj(
					pdev, WLAN_UMAC_COMP_MGMT_RX_SRNG);
	if (!pdev_priv) {
		mgmt_rx_srng_err("pdev priv is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	status = wlan_objmgr_pdev_component_obj_detach(
					pdev, WLAN_UMAC_COMP_MGMT_RX_SRNG,
					pdev_priv);
	if (QDF_IS_STATUS_ERROR(status)) {
		mgmt_rx_srng_err("Failed to detach mgmt rx srng pdev_priv");
		return status;
	}
	qdf_mem_free(pdev_priv);

	return status;
}

QDF_STATUS wlan_mgmt_rx_srng_psoc_create_notification(
				struct wlan_objmgr_psoc *psoc, void *arg)
{
	struct mgmt_rx_srng_psoc_priv *psoc_priv;
	QDF_STATUS status;

	psoc_priv = qdf_mem_malloc(sizeof(*psoc_priv));
	if (!psoc_priv)
		return QDF_STATUS_E_NOMEM;

	status = wlan_objmgr_psoc_component_obj_attach(
				psoc, WLAN_UMAC_COMP_MGMT_RX_SRNG,
				psoc_priv, QDF_STATUS_SUCCESS);
	if (QDF_IS_STATUS_ERROR(status)) {
		mgmt_rx_srng_err("Failed to attach psoc component obj");
		goto free_psoc_priv;
	}
	psoc_priv->psoc = psoc;

	return status;

free_psoc_priv:
	qdf_mem_free(psoc_priv);
	return status;
}

QDF_STATUS wlan_mgmt_rx_srng_psoc_destroy_notification(
				struct wlan_objmgr_psoc *psoc, void *arg)
{
	struct mgmt_rx_srng_psoc_priv *psoc_priv;
	QDF_STATUS status;

	psoc_priv = wlan_objmgr_psoc_get_comp_private_obj(
					psoc, WLAN_UMAC_COMP_MGMT_RX_SRNG);
	if (!psoc_priv) {
		mgmt_rx_srng_err("psoc priv is NULL");
		return QDF_STATUS_E_FAILURE;
	}
	status = wlan_objmgr_psoc_component_obj_detach(
					psoc, WLAN_UMAC_COMP_MGMT_RX_SRNG,
					psoc_priv);
	if (QDF_IS_STATUS_ERROR(status)) {
		mgmt_rx_srng_err("Failed to detach psoc component obj");
		return status;
	}

	qdf_mem_free(psoc_priv);
	return status;
}
