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

#ifndef QCA_SINGLE_WIFI_3_0
static int __init monitor_mod_init(void)
#else
int monitor_mod_init(void)
#endif
{
	QDF_STATUS status;
	struct wlan_objmgr_psoc *psoc;
	struct dp_soc *soc;
	struct dp_pdev *pdev;
	uint8_t index = 0, pdev_id = 0;

	while (index < WLAN_OBJMGR_MAX_DEVICES) {
		psoc = g_umac_glb_obj->psoc[index];
		if (!psoc) {
			index++;
			continue;
		}
		soc = wlan_psoc_get_dp_handle(psoc);

		while (pdev_id < MAX_PDEV_CNT) {
			pdev = soc->pdev_list[pdev_id];
			if (!pdev) {
				pdev_id++;
				continue;
			}
			status = dp_mon_pdev_attach(pdev);
			if (status == QDF_STATUS_SUCCESS) {
				status = dp_mon_pdev_init(pdev);
				if (status != QDF_STATUS_SUCCESS)
					qdf_err("%s: monitor pdev init failed ",
						__func__);
			} else {
				qdf_err("%s: monitor pdev attach failed",
					__func__);
			}
			pdev_id++;
		}
		status = dp_mon_soc_attach(soc);
		if (status == QDF_STATUS_SUCCESS)
			mon_soc_ol_attach(psoc);
		else
			qdf_err("%s: monitor soc attach failed ", __func__);
		index++;
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
	uint8_t index = 0, pdev_id = 0;

	while (index < WLAN_OBJMGR_MAX_DEVICES) {
		psoc = g_umac_glb_obj->psoc[index];
		if (!psoc) {
			index++;
			continue;
		}
		soc = wlan_psoc_get_dp_handle(psoc);
		while (pdev_id < MAX_PDEV_CNT) {
			pdev = soc->pdev_list[pdev_id];
			if (!pdev) {
				pdev_id++;
				continue;
			}
			dp_mon_pdev_detach(pdev);
			pdev_id++;
		}
		dp_mon_soc_detach(soc);
		index++;
	}
}

#ifndef QCA_SINGLE_WIFI_3_0
module_exit(monitor_mod_exit);
#endif
