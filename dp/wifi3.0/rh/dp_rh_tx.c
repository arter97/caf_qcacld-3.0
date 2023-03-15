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
#include "cdp_txrx_cmn_struct.h"
#include "dp_types.h"
#include "dp_tx.h"
#include "dp_rh_tx.h"
#include "dp_tx_desc.h"
#include <dp_internal.h>
#include <dp_htt.h>
#include <hal_rh_api.h>
#include <hal_rh_tx.h>
#include "dp_peer.h"
#ifdef FEATURE_WDS
#include "dp_txrx_wds.h"
#endif
#include "dp_rh.h"

extern uint8_t sec_type_map[MAX_CDP_SEC_TYPE];

void dp_tx_comp_get_params_from_hal_desc_rh(struct dp_soc *soc,
					    void *tx_comp_hal_desc,
					    struct dp_tx_desc_s **r_tx_desc)
{
}

void dp_tx_process_htt_completion_rh(struct dp_soc *soc,
				     struct dp_tx_desc_s *tx_desc,
				     uint8_t *status,
				     uint8_t ring_id)
{
}

QDF_STATUS
dp_tx_hw_enqueue_rh(struct dp_soc *soc, struct dp_vdev *vdev,
		    struct dp_tx_desc_s *tx_desc, uint16_t fw_metadata,
		    struct cdp_tx_exception_metadata *tx_exc_metadata,
		    struct dp_tx_msdu_info_s *msdu_info)
{
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS dp_tx_desc_pool_init_rh(struct dp_soc *soc,
				   uint32_t num_elem,
				   uint8_t pool_id)
{
	uint32_t id, count, page_id, offset, pool_id_32;
	struct dp_tx_desc_s *tx_desc;
	struct dp_tx_desc_pool_s *tx_desc_pool;
	uint16_t num_desc_per_page;

	tx_desc_pool = &soc->tx_desc[pool_id];
	tx_desc = tx_desc_pool->freelist;
	count = 0;
	pool_id_32 = (uint32_t)pool_id;
	num_desc_per_page = tx_desc_pool->desc_pages.num_element_per_page;
	while (tx_desc) {
		page_id = count / num_desc_per_page;
		offset = count % num_desc_per_page;
		id = ((pool_id_32 << DP_TX_DESC_ID_POOL_OS) |
			(page_id << DP_TX_DESC_ID_PAGE_OS) | offset);

		tx_desc->id = id;
		tx_desc->pool_id = pool_id;
		dp_tx_desc_set_magic(tx_desc, DP_TX_MAGIC_PATTERN_FREE);
		tx_desc = tx_desc->next;
		count++;
	}

	return QDF_STATUS_SUCCESS;
}

void dp_tx_desc_pool_deinit_rh(struct dp_soc *soc,
			       struct dp_tx_desc_pool_s *tx_desc_pool,
			       uint8_t pool_id)
{
}

QDF_STATUS dp_tx_compute_tx_delay_rh(struct dp_soc *soc,
				     struct dp_vdev *vdev,
				     struct hal_tx_completion_status *ts,
				     uint32_t *delay_us)
{
	return QDF_STATUS_SUCCESS;
}
