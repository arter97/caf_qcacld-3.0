/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.

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

#include <linux/module.h>
#include <osdep.h>
#include <wlan_objmgr_global_obj_i.h>
#include <wlan_objmgr_psoc_obj.h>
#include <dp_types.h>
#include <dp_internal.h>
#include <dp_htt.h>
#include <dp_mon.h>

#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif

MODULE_LICENSE("Dual BSD/GPL");

QDF_STATUS mon_soc_ol_attach(struct wlan_objmgr_psoc *psoc);
void mon_soc_ol_detach(struct wlan_objmgr_psoc *psoc);

static inline QDF_STATUS
dp_mon_ring_config(struct dp_soc *soc, struct dp_pdev *pdev,
		   int mac_for_pdev)
{
	int lmac_id;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	lmac_id = dp_get_lmac_id_for_pdev_id(soc, 0, mac_for_pdev);
	status = dp_mon_htt_srng_setup(soc, pdev, lmac_id, mac_for_pdev);

	return status;
}

#ifndef QCA_SINGLE_WIFI_3_0
static int __init monitor_mod_init(void)
#else
int monitor_mod_init(void)
#endif
{
	QDF_STATUS status;
	struct wlan_objmgr_psoc *psoc;
	struct dp_mon_soc *mon_soc;
	struct dp_soc *soc;
	struct dp_pdev *pdev;
	uint8_t index = 0;
	uint8_t pdev_id = 0;
	uint8_t pdev_count = 0;
	bool pdev_attach_success;

	for (index = 0; index < WLAN_OBJMGR_MAX_DEVICES; index++) {
		psoc = g_umac_glb_obj->psoc[index];
		if (!psoc)
			continue;

		soc = wlan_psoc_get_dp_handle(psoc);
		if (!soc) {
			dp_mon_err("dp_soc is NULL, psoc = %pK", psoc);
			continue;
		}

		mon_soc = (struct dp_mon_soc *)qdf_mem_malloc(sizeof(*mon_soc));
		if (!mon_soc) {
			dp_mon_err("%pK: mem allocation failed", soc);
			continue;
		}
		soc->monitor_soc = mon_soc;
		dp_mon_soc_cfg_init(soc);
		pdev_attach_success = false;

		pdev_count = psoc->soc_objmgr.wlan_pdev_count;
		for (pdev_id = 0; pdev_id < pdev_count; pdev_id++) {
			pdev = soc->pdev_list[pdev_id];
			if (!pdev)
				continue;

			status = dp_mon_pdev_attach(pdev);
			if (status != QDF_STATUS_SUCCESS) {
				dp_mon_err("mon pdev attach failed, dp pdev = %pK",
					pdev);
				continue;
			}
			status = dp_mon_pdev_init(pdev);
			if (status != QDF_STATUS_SUCCESS) {
				dp_mon_err("mon pdev init failed, dp pdev = %pK",
					pdev);
				dp_mon_pdev_detach(pdev);
				continue;
			}

			status = dp_mon_ring_config(soc, pdev, pdev_id);
			if (status != QDF_STATUS_SUCCESS) {
				dp_mon_err("mon ring config failed, dp pdev = %pK",
					pdev);
				dp_mon_pdev_deinit(pdev);
				dp_mon_pdev_detach(pdev);
				continue;
			}
			dp_tx_capture_debugfs_init(pdev);
			pdev_attach_success = true;
		}
		if (!pdev_attach_success) {
			dp_mon_err("mon attach failed for all, dp soc = %pK",
				soc);
			soc->monitor_soc = NULL;
			qdf_mem_free(mon_soc);
			continue;
		}
		dp_mon_cdp_ops_register(soc);
		dp_mon_ops_register(mon_soc);
		mon_soc_ol_attach(psoc);
	}
	return 0;
}

#ifndef QCA_SINGLE_WIFI_3_0
module_init(monitor_mod_init);
#endif
/**
 * monitor_mod_exit() - module remove
 *
 * Return: void
 */
#ifndef QCA_SINGLE_WIFI_3_0
static void __exit monitor_mod_exit(void)
#else
void monitor_mod_exit(void)
#endif
{
	struct wlan_objmgr_psoc *psoc;
	struct dp_soc *soc;
	struct dp_pdev *pdev;
	struct dp_mon_soc *mon_soc;
	uint8_t index = 0;
	uint8_t pdev_id = 0;
	uint8_t pdev_count = 0;

	for (index = 0; index < WLAN_OBJMGR_MAX_DEVICES; index++) {
		psoc = g_umac_glb_obj->psoc[index];
		if (!psoc)
			continue;

		soc = wlan_psoc_get_dp_handle(psoc);
		if (!soc) {
			dp_mon_err("dp_soc is NULL");
			continue;
		}

		if (!soc->monitor_soc)
			continue;

		dp_soc_reset_mon_intr_mask(soc);
		/* add delay before proceeding to allow any pending
		 * mon process to complete */
		qdf_mdelay(500);

		mon_soc_ol_detach(psoc);
		dp_mon_cdp_ops_deregister(soc);
		pdev_count = psoc->soc_objmgr.wlan_pdev_count;
		for (pdev_id = 0; pdev_id < pdev_count; pdev_id++) {
			pdev = soc->pdev_list[pdev_id];
			if (!pdev || !pdev->monitor_pdev)
				continue;

			dp_mon_pdev_deinit(pdev);
			dp_mon_pdev_detach(pdev);
		}
		mon_soc = soc->monitor_soc;
		soc->monitor_soc = NULL;
		qdf_mem_free(mon_soc);
	}
}

#ifndef QCA_SINGLE_WIFI_3_0
module_exit(monitor_mod_exit);
#endif
