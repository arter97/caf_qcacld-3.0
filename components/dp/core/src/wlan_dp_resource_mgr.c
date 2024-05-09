/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

#include <wlan_objmgr_pdev_obj.h>
#include <wlan_dp_main.h>
#include <wlan_dp_priv.h>
#include <wlan_dp_resource_mgr.h>
#include <cdp_txrx_cmn.h>
#include <cdp_txrx_ctrl.h>

void wlan_dp_resource_mgr_attach(struct wlan_dp_psoc_context *dp_ctx)
{
	struct wlan_dp_resource_mgr_ctx *rsrc_ctx;
	struct cdp_soc_t *cdp_soc = dp_ctx->cdp_soc;
	cdp_config_param_type val = {0};
	QDF_STATUS status;
	int i;

	rsrc_ctx = qdf_mem_malloc(sizeof(*rsrc_ctx));
	if (!rsrc_ctx) {
		dp_err("Failed to create DP resource mgr context");
		return;
	}
	qdf_mem_copy(rsrc_ctx->cur_rsrc_map, dp_resource_map,
		     (sizeof(struct wlan_dp_resource_map) * RESOURCE_LVL_MAX));

	status = cdp_txrx_get_psoc_param(cdp_soc,
					 CDP_CFG_RXDMA_REFILL_RING_SIZE, &val);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("Failed to fetch Refill ring config status:%u", status);
		goto attach_err;
	}

	/*
	 * Config value is less than default resource map, then
	 * do not enable resource mgr, since resource mgr cannot
	 * operated on less than default resources.
	 * If config value greater than default resources, than
	 * use that value for max reource level selection.
	 */
	if (val.cdp_rxdma_refill_ring_size <=
	    dp_resource_map[RESOURCE__MBPSLVL_1].num_rx_buffers) {
		dp_info("Unsupported RX buffers config:%u, disabling rsrc mgr",
			val.cdp_rxdma_refill_ring_size);
		goto attach_err;
	} else if (val.cdp_rxdma_refill_ring_size !=
		   dp_resource_map[RESOURCE_LVL_2].num_rx_buffers) {
		dp_info("Adjusting Resource lvl_2 rx buffers map_val:%u cfg_val:%u",
			dp_resource_map[RESOURCE_LVL_2].num_rx_buffers,
			val.cdp_rxdma_refill_ring_size);
		rsrc_ctx->cur_rsrc_map[RESOURCE_LVL_2].num_rx_buffers =
			val.cdp_rxdma_refill_ring_size;
	}

	for (i = 0; i < RESOURCE_LVL_MAX; i++) {
		dp_info("DP resource level:%u_MAX_TPUT:%u NUM_RX_BUF:%u",
			rsrc_ctx->cur_rsrc_map[i].max_tput,
			rsrc_ctx->cur_rsrc_map[i].num_rx_buffers);
	}

	dp_ctx->rsrc_mgr_ctx = rsrc_ctx;
	return;

attach_err:
	qdf_mem_free(rsrc_ctx);
}

void wlan_dp_resource_mgr_detach(struct wlan_dp_psoc_context *dp_ctx)
{
	dp_info("DP resource mgr detaching %s",
		dp_ctx->rsrc_mgr_ctx ? "true" : "false");
	if (dp_ctx->rsrc_mgr_ctx) {
		qdf_mem_free(dp_ctx->rsrc_mgr_ctx);
		dp_ctx->rsrc_mgr_ctx = NULL;
	}
}
