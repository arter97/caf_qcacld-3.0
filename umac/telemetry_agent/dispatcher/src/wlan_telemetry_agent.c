/*
 * Copyright (c) 2022, Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <qdf_status.h>
#include <qdf_types.h>
#include <osdep_adf.h>
#include <wlan_objmgr_cmn.h>
#include <wlan_objmgr_psoc_obj.h>
#include <wlan_objmgr_pdev_obj.h>
#include <wlan_objmgr_peer_obj.h>
#include <wlan_objmgr_global_obj.h>
#include "wlan_telemetry_agent.h"

struct telemetry_agent_ops *g_agent_ops;

void wlan_telemetry_agent_application_init_notify(void)
{
	qdf_info(">");
	if (g_agent_ops)
		g_agent_ops->agent_notify_app_init();
}

static QDF_STATUS
telemetry_agent_psoc_create_handler(struct wlan_objmgr_psoc *psoc,
				    void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct agent_psoc_obj psoc_obj = {0};

	qdf_info("> soc_id: %d", wlan_psoc_get_id(psoc));

	psoc_obj.psoc_id = wlan_psoc_get_id(psoc);
	psoc_obj.psoc_back_pointer = psoc;

	if (g_agent_ops)
		g_agent_ops->agent_psoc_create_handler(psoc, &psoc_obj);
	return status;
}

static QDF_STATUS
telemetry_agent_pdev_create_handler(struct wlan_objmgr_pdev *pdev,
				    void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_objmgr_psoc *psoc;
	struct agent_pdev_obj pdev_obj = {0};

	psoc = wlan_pdev_get_psoc(pdev);
	qdf_info("> soc_id: %d pdevid: %d",
		 wlan_psoc_get_id(psoc),
		 wlan_objmgr_pdev_get_pdev_id(pdev));

	pdev_obj.pdev_back_pointer = pdev;
	pdev_obj.psoc_back_pointer = psoc;
	pdev_obj.psoc_id = wlan_psoc_get_id(psoc);
	pdev_obj.pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);

	if (g_agent_ops)
		g_agent_ops->agent_pdev_create_handler(pdev, &pdev_obj);

	return status;
}

static QDF_STATUS
telemetry_agent_peer_create_handler(struct wlan_objmgr_peer *peer,
				    void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct agent_peer_obj peer_obj = {0};

	struct wlan_objmgr_psoc *psoc =  wlan_peer_get_psoc(peer);
	struct wlan_objmgr_vdev *vdev = wlan_peer_get_vdev(peer);
	struct wlan_objmgr_pdev *pdev = wlan_vdev_get_pdev(vdev);

	peer_obj.peer_back_pointer = peer;
	peer_obj.pdev_back_pointer = pdev;
	peer_obj.psoc_back_pointer = psoc;

	peer_obj.psoc_id = wlan_psoc_get_id(psoc);
	peer_obj.pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);
	WLAN_ADDR_COPY(&peer_obj.peer_mac_addr, peer->macaddr);

	qdf_info("> soc_id: %d pdevid: %d peer_mac: %s",
		 wlan_psoc_get_id(psoc),
		 wlan_objmgr_pdev_get_pdev_id(pdev),
		 ether_sprintf(&peer_obj.peer_mac_addr[0]));

	if (g_agent_ops)
		g_agent_ops->agent_peer_create_handler(peer, &peer_obj);

	return status;
}


static QDF_STATUS
telemetry_agent_psoc_delete_handler(struct wlan_objmgr_psoc *psoc,
				    void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct agent_psoc_obj psoc_obj = {0};

	qdf_info("> socid: %d", wlan_psoc_get_id(psoc));
	psoc_obj.psoc_id = wlan_psoc_get_id(psoc);
	psoc_obj.psoc_back_pointer = psoc;

	if (g_agent_ops)
		g_agent_ops->agent_psoc_destroy_handler(psoc, &psoc_obj);

	return status;
}

static QDF_STATUS
telemetry_agent_pdev_delete_handler(struct wlan_objmgr_pdev *pdev,
				    void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_objmgr_psoc *psoc;
	struct agent_pdev_obj pdev_obj = {0};

	psoc = wlan_pdev_get_psoc(pdev);
	qdf_info("> soc_id: %d pdevid: %d",
		 wlan_psoc_get_id(psoc),
		 wlan_objmgr_pdev_get_pdev_id(pdev));

	pdev_obj.pdev_back_pointer = pdev;
	pdev_obj.psoc_back_pointer = psoc;
	pdev_obj.psoc_id = wlan_psoc_get_id(psoc);
	pdev_obj.pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);

	if (g_agent_ops)
		g_agent_ops->agent_pdev_destroy_handler(pdev, &pdev_obj);

	return status;
}

static QDF_STATUS
telemetry_agent_peer_delete_handler(struct wlan_objmgr_peer *peer,
				    void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct agent_peer_obj peer_obj = {0};

	struct wlan_objmgr_psoc *psoc =  wlan_peer_get_psoc(peer);
	struct wlan_objmgr_vdev *vdev = wlan_peer_get_vdev(peer);
	struct wlan_objmgr_pdev *pdev = wlan_vdev_get_pdev(vdev);

	peer_obj.peer_back_pointer = peer;
	peer_obj.pdev_back_pointer = pdev;
	peer_obj.psoc_back_pointer = psoc;

	peer_obj.psoc_id = wlan_psoc_get_id(psoc);
	peer_obj.pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);
	WLAN_ADDR_COPY(&peer_obj.peer_mac_addr, peer->macaddr);

	qdf_info("> soc_id: %d pdevid: %d peer_mac: %s",
		 wlan_psoc_get_id(psoc),
		 wlan_objmgr_pdev_get_pdev_id(pdev),
		 ether_sprintf(&peer_obj.peer_mac_addr[0]));

	if (g_agent_ops)
		g_agent_ops->agent_peer_destroy_handler(peer, &peer_obj);

	return status;
}

int register_telemetry_agent_ops(struct telemetry_agent_ops *agent_ops)
{
	g_agent_ops = agent_ops;
	qdf_info("Registered Telemetry Agent ops: %p", g_agent_ops);
	return QDF_STATUS_SUCCESS;
}

int
unregister_telemetry_agent_ops(struct telemetry_agent_ops *agent_ops)
{
	g_agent_ops = NULL;
	qdf_info("UnRegistered Telemetry Agent ops: %p", g_agent_ops);
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_telemetry_agent_init(void)
{
	int status = QDF_STATUS_SUCCESS;

	if (wlan_objmgr_register_psoc_create_handler(WLAN_COMP_TELEMETRY_AGENT,
				telemetry_agent_psoc_create_handler,
				NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto psoc_create_failed;
	}

	if (wlan_objmgr_register_pdev_create_handler(WLAN_COMP_TELEMETRY_AGENT,
				telemetry_agent_pdev_create_handler,
				NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto pdev_create_failed;
	}

	if (wlan_objmgr_register_peer_create_handler(WLAN_COMP_TELEMETRY_AGENT,
				telemetry_agent_peer_create_handler,
				NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto peer_create_failed;
	}

	if (wlan_objmgr_register_psoc_destroy_handler(WLAN_COMP_TELEMETRY_AGENT,
				telemetry_agent_psoc_delete_handler,
				NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto psoc_destroy_failed;
	}

	if (wlan_objmgr_register_pdev_destroy_handler(WLAN_COMP_TELEMETRY_AGENT,
				telemetry_agent_pdev_delete_handler,
				NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto pdev_destroy_failed;
	}

	if (wlan_objmgr_register_peer_destroy_handler(WLAN_COMP_TELEMETRY_AGENT,
				telemetry_agent_peer_delete_handler,
				NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto peer_destroy_failed;
	}

	return status;

peer_destroy_failed:
	if (wlan_objmgr_unregister_pdev_destroy_handler(WLAN_COMP_TELEMETRY_AGENT,
				telemetry_agent_pdev_delete_handler,
				NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
	}
pdev_destroy_failed:
	if (wlan_objmgr_unregister_psoc_destroy_handler(WLAN_COMP_TELEMETRY_AGENT,
				telemetry_agent_psoc_delete_handler,
				NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
	}
psoc_destroy_failed:
	if (wlan_objmgr_unregister_peer_create_handler(WLAN_COMP_TELEMETRY_AGENT,
				telemetry_agent_peer_create_handler,
				NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
	}

peer_create_failed:
	if (wlan_objmgr_unregister_pdev_create_handler(WLAN_COMP_TELEMETRY_AGENT,
				telemetry_agent_pdev_create_handler,
				NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
	}

pdev_create_failed:
	if (wlan_objmgr_unregister_psoc_create_handler(WLAN_COMP_TELEMETRY_AGENT,
				telemetry_agent_psoc_create_handler,
				NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
	}

psoc_create_failed:

	return status;
}

QDF_STATUS wlan_telemetry_agent_deinit(void)
{
	int status = QDF_STATUS_SUCCESS;

	if (wlan_objmgr_unregister_psoc_create_handler(WLAN_COMP_TELEMETRY_AGENT,
				telemetry_agent_psoc_create_handler,
				NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
	}

	if (wlan_objmgr_unregister_pdev_create_handler(WLAN_COMP_TELEMETRY_AGENT,
				telemetry_agent_pdev_create_handler,
				NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
	}

	if (wlan_objmgr_unregister_peer_create_handler(WLAN_COMP_TELEMETRY_AGENT,
				telemetry_agent_peer_create_handler,
				NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
	}

	if (wlan_objmgr_unregister_psoc_destroy_handler(WLAN_COMP_TELEMETRY_AGENT,
				telemetry_agent_psoc_delete_handler,
				NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
	}

	if (wlan_objmgr_unregister_pdev_destroy_handler(WLAN_COMP_TELEMETRY_AGENT,
				telemetry_agent_pdev_delete_handler,
				NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
	}

	if (wlan_objmgr_unregister_peer_destroy_handler(WLAN_COMP_TELEMETRY_AGENT,
				telemetry_agent_peer_delete_handler,
				NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
	}

	return status;
}
