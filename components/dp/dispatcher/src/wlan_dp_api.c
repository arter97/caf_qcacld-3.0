/*
 * Copyright (c) 2023, Qualcomm Innovation Center, Inc. All rights reserved.
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
 * DOC: wlan_dp_api.c
 *
 */

#include "wlan_dp_main.h"
#include "wlan_dp_api.h"
#include <wlan_dp_fisa_rx.h>

void wlan_dp_update_peer_map_unmap_version(uint8_t *version)
{
	__wlan_dp_update_peer_map_unmap_version(version);
}

QDF_STATUS wlan_dp_runtime_suspend(ol_txrx_soc_handle soc, uint8_t pdev_id)
{
	return __wlan_dp_runtime_suspend(soc, pdev_id);
}

QDF_STATUS wlan_dp_runtime_resume(ol_txrx_soc_handle soc, uint8_t pdev_id)
{
	return __wlan_dp_runtime_resume(soc, pdev_id);
}

void wlan_dp_print_fisa_rx_stats(enum cdp_fisa_stats_id stats_id)
{
	dp_print_fisa_rx_stats(stats_id);
}

void wlan_dp_set_fst_in_cmem(bool fst_in_cmem)
{
	dp_set_fst_in_cmem(fst_in_cmem);
}

void wlan_dp_set_fisa_dynamic_aggr_size_support(bool dynamic_aggr_size_support)
{
	dp_set_fisa_dynamic_aggr_size_support(dynamic_aggr_size_support);
}

#ifdef WLAN_FEATURE_LOCAL_PKT_CAPTURE
bool wlan_dp_is_local_pkt_capture_enabled(struct wlan_objmgr_psoc *psoc)
{
	void *soc = cds_get_context(QDF_MODULE_ID_SOC);

	return cdp_cfg_get(soc, cfg_dp_local_pkt_capture);
}
#endif

#ifdef IPA_WDS_EASYMESH_FEATURE
int wlan_dp_send_ipa_wds_peer_map_event(struct cdp_ctrl_objmgr_psoc *cpsoc,
					uint16_t peer_id, uint16_t hw_peer_id,
					uint8_t vdev_id, uint8_t *wds_mac_addr,
					enum cdp_txrx_ast_entry_type peer_type,
					uint32_t tx_ast_hashidx)
{
	struct wlan_objmgr_psoc *psoc = (struct wlan_objmgr_psoc *)cpsoc;
	struct wlan_dp_psoc_context *dp_ctx;

	if (peer_type != CDP_TXRX_AST_TYPE_WDS)
		return 0;

	dp_ctx = dp_psoc_get_priv(psoc);
	if (qdf_unlikely(!dp_ctx)) {
		dp_err_rl("Invalid psoc private object");
		return 0;
	}

	/* With CDP_TXRX_AST_TYPE_WDS, it can be inferred that WDS AST
	 * learning is enabled on the vdev.
	 */
	if (dp_ctx->dp_ops.wlan_dp_ipa_wds_peer_cb)
		return dp_ctx->dp_ops.wlan_dp_ipa_wds_peer_cb(vdev_id,
							      peer_id,
							      wds_mac_addr,
							      true);

	return 0;
}

int wlan_dp_send_ipa_wds_peer_unmap_event(struct cdp_ctrl_objmgr_psoc *cpsoc,
					  uint16_t peer_id, uint8_t vdev_id,
					  uint8_t *wds_mac_addr)
{
	struct wlan_objmgr_psoc *psoc = (struct wlan_objmgr_psoc *)cpsoc;
	struct wlan_dp_psoc_context *dp_ctx;

	dp_ctx = dp_psoc_get_priv(psoc);
	if (qdf_unlikely(!dp_ctx)) {
		dp_err_rl("Invalid psoc private object");
		return 0;
	}

	if (dp_ctx->dp_ops.wlan_dp_ipa_wds_peer_cb)
		return dp_ctx->dp_ops.wlan_dp_ipa_wds_peer_cb(vdev_id,
							      peer_id,
							      wds_mac_addr,
							      false);

	return 0;
}

void wlan_dp_send_ipa_wds_peer_disconnect(struct cdp_ctrl_objmgr_psoc *cpsoc,
					  uint8_t *wds_mac_addr,
					  uint8_t vdev_id)
{
	struct wlan_objmgr_psoc *psoc = (struct wlan_objmgr_psoc *)cpsoc;
	struct wlan_dp_psoc_context *dp_ctx;

	dp_ctx = dp_psoc_get_priv(psoc);
	if (qdf_unlikely(!dp_ctx)) {
		dp_err_rl("Invalid psoc private object");
		return;
	}

	/* IPA component does not care about peer_id in the disconnect case.
	 * Hence put a dummy 0 here for peer_id.
	 */
	if (dp_ctx->dp_ops.wlan_dp_ipa_wds_peer_cb)
		dp_ctx->dp_ops.wlan_dp_ipa_wds_peer_cb(vdev_id, 0, wds_mac_addr,
						       false);
}
#endif /* IPA_WDS_EASYMESH_FEATURE */
