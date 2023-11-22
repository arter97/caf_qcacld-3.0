/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * DOC: contains policy manager ll_sap definitions specific to the ll_sap module
 */

#include "wlan_policy_mgr_ll_sap.h"
#include "wlan_policy_mgr_public_struct.h"
#include "wlan_policy_mgr_api.h"
#include "wlan_policy_mgr_i.h"
#include "wlan_cmn.h"

uint8_t wlan_policy_mgr_get_ll_lt_sap_vdev_id(struct wlan_objmgr_psoc *psoc)
{
	uint8_t ll_lt_sap_cnt;
	uint8_t vdev_id_list[MAX_NUMBER_OF_CONC_CONNECTIONS];

	ll_lt_sap_cnt = policy_mgr_get_mode_specific_conn_info(psoc, NULL,
							vdev_id_list,
							PM_LL_LT_SAP_MODE);

	/* Currently only 1 ll_lt_sap is supported */
	if (!ll_lt_sap_cnt)
		return WLAN_INVALID_VDEV_ID;

	return vdev_id_list[0];
}

bool __policy_mgr_is_ll_lt_sap_restart_required(struct wlan_objmgr_psoc *psoc,
						const char *func)
{
	qdf_freq_t ll_lt_sap_freq = 0;
	uint8_t scc_vdev_id;
	bool is_scc = false;
	uint8_t conn_idx = 0;
	struct policy_mgr_psoc_priv_obj *pm_ctx;

	pm_ctx = policy_mgr_get_context(psoc);
	if (!pm_ctx) {
		policy_mgr_err("Invalid pm ctx");
		return false;
	}

	ll_lt_sap_freq = policy_mgr_get_ll_lt_sap_freq(psoc);

	if (!ll_lt_sap_freq)
		return false;

	/*
	 * Restart ll_lt_sap if any other interface is present in SCC
	 * with LL_LT_SAP.
	 */
	qdf_mutex_acquire(&pm_ctx->qdf_conc_list_lock);
	for (conn_idx = 0; conn_idx < MAX_NUMBER_OF_CONC_CONNECTIONS;
	     conn_idx++) {
		if (pm_conc_connection_list[conn_idx].mode ==
		      PM_LL_LT_SAP_MODE)
			continue;

		if (ll_lt_sap_freq == pm_conc_connection_list[conn_idx].freq) {
			scc_vdev_id = pm_conc_connection_list[conn_idx].vdev_id;
			is_scc = true;
			break;
		}
	}
	qdf_mutex_release(&pm_ctx->qdf_conc_list_lock);

	if (is_scc) {
		uint8_t ll_lt_sap_vdev_id =
				wlan_policy_mgr_get_ll_lt_sap_vdev_id(psoc);

		policymgr_nofl_debug("%s ll_lt_sap vdev %d with freq %d is in scc with vdev %d",
				     func, ll_lt_sap_vdev_id, ll_lt_sap_freq,
				     scc_vdev_id);
		return true;
	}

	return false;
}
