/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "qdf_types.h"
#include "qdf_util.h"
#include "qdf_types.h"
#include "qdf_lock.h"
#include "qdf_mem.h"
#include "qdf_nbuf.h"
#include "hal_internal.h"
#include "hal_api.h"
#include "target_type.h"
#include "wcss_version.h"
#include "qdf_module.h"
#include "hal_flow.h"
#include "rx_flow_search_entry.h"
#include "hal_rx_flow_info.h"

struct hal_hw_srng_config hw_srng_table_wcn6450[] = {
	/* TODO: max_rings can populated by querying HW capabilities */
	{/* REO_DST */ 0},
	{/* REO_EXCEPTION */ 0},
	{/* REO_REINJECT */ 0},
	{/* REO_CMD */ 0},
	{/* REO_STATUS */ 0},
	{/* TCL_DATA */ 0},
	{/* TCL_CMD */ 0},
	{/* TCL_STATUS */ 0},
	{/* CE_SRC */ 0},
	{/* CE_DST */ 0},
	{/* CE_DST_STATUS */ 0},
	{/* WBM_IDLE_LINK */ 0},
	{/* SW2WBM_RELEASE */ 0},
	{/* WBM2SW_RELEASE */ 0},
	{ /* RXDMA_BUF */
		.start_ring_id = HAL_SRNG_WMAC1_SW2RXDMA0_BUF0,
#ifdef IPA_OFFLOAD
		.max_rings = 3,
#else
		.max_rings = 2,
#endif
		.entry_size = sizeof(struct wbm_buffer_ring) >> 2,
		.lmac_ring = TRUE,
		.ring_dir = HAL_SRNG_SRC_RING,
		/* reg_start is not set because LMAC rings are not accessed
		 * from host
		 */
		.reg_start = {},
		.reg_size = {},
		.max_size = HAL_RXDMA_MAX_RING_SIZE,
	},
	{ /* RXDMA_DST */
		.start_ring_id = HAL_SRNG_WMAC1_RXDMA2SW0,
		.max_rings = 1,
		.entry_size = sizeof(struct reo_entrance_ring) >> 2,
		.lmac_ring =  TRUE,
		.ring_dir = HAL_SRNG_DST_RING,
		/* reg_start is not set because LMAC rings are not accessed
		 * from host
		 */
		.reg_start = {},
		.reg_size = {},
		.max_size = HAL_RXDMA_MAX_RING_SIZE,
	},
	{/* RXDMA_MONITOR_BUF */ 0},
	{ /* RXDMA_MONITOR_STATUS */
		.start_ring_id = HAL_SRNG_WMAC1_SW2RXDMA1_STATBUF,
		.max_rings = 1,
		.entry_size = sizeof(struct wbm_buffer_ring) >> 2,
		.lmac_ring = TRUE,
		.ring_dir = HAL_SRNG_SRC_RING,
		/* reg_start is not set because LMAC rings are not accessed
		 * from host
		 */
		.reg_start = {},
		.reg_size = {},
		.max_size = HAL_RXDMA_MAX_RING_SIZE,
	},
	{/* RXDMA_MONITOR_DST */ 0},
	{/* RXDMA_MONITOR_DESC */ 0},
	{/* DIR_BUF_RX_DMA_SRC */ 0},
#ifdef WLAN_FEATURE_CIF_CFR
	{/* WIFI_POS_SRC */ 0},
#endif
	{ /* REO2PPE */ 0},
	{ /* PPE2TCL */ 0},
	{ /* PPE_RELEASE */ 0},
	{ /* TX_MONITOR_BUF */ 0},
	{ /* TX_MONITOR_DST */ 0},
	{ /* SW2RXDMA_NEW */ 0},
};

/**
 * hal_wcn6450_attach() - Attach 6450 target specific hal_soc ops,
 *				offset and srng table
 * @hal_soc: HAL Soc handle
 *
 * Return: None
 */
void hal_wcn6450_attach(struct hal_soc *hal_soc)
{
	hal_soc->hw_srng_table = hw_srng_table_wcn6450;
}
