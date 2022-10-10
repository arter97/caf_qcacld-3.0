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
#include "dp_rh.h"
#include <ce_api.h>
#include <ce_internal.h>

extern uint8_t sec_type_map[MAX_CDP_SEC_TYPE];

#if defined(FEATURE_TSO)
/**
 * dp_tx_adjust_tso_download_len_rh() - Adjust download length for TSO packet
 * @nbuf: socket buffer
 * @msdu_info: handle to struct dp_tx_msdu_info_s
 * @download_len: Packet download length that needs adjustment
 *
 * Return: uint32_t (Adjusted packet download length)
 */
static uint32_t
dp_tx_adjust_tso_download_len_rh(qdf_nbuf_t nbuf,
				 struct dp_tx_msdu_info_s *msdu_info,
				 uint32_t download_len)
{
	uint32_t frag0_len;
	uint32_t delta;
	uint32_t eit_hdr_len;

	frag0_len = qdf_nbuf_get_frag_len(nbuf, 0);
	download_len -= frag0_len;

	eit_hdr_len = msdu_info->u.tso_info.curr_seg->seg.tso_frags[0].length;

	/* If EIT header length is less than the MSDU download length, then
	 * adjust the download length to just hold EIT header.
	 */
	if (eit_hdr_len < download_len) {
		delta = download_len - eit_hdr_len;
		download_len -= delta;
	}

	return download_len;
}
#else
static uint32_t
dp_tx_adjust_tso_download_len_rh(qdf_nbuf_t nbuf,
				 struct dp_tx_msdu_info_s *msdu_info,
				 uint32_t download_len)
{
	return download_len;
}
#endif /* FEATURE_TSO */

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

static inline uint32_t
dp_tx_adjust_download_len_rh(qdf_nbuf_t nbuf, uint32_t download_len)
{
	uint32_t frag0_len; /* TCL_DATA_CMD */
	uint32_t frag1_len; /* 64 byte payload */

	frag0_len = qdf_nbuf_get_frag_len(nbuf, 0);
	frag1_len = download_len - frag0_len;

	if (qdf_unlikely(qdf_nbuf_len(nbuf) < frag1_len))
		frag1_len = qdf_nbuf_len(nbuf);

	return frag0_len + frag1_len;
}

static inline void dp_tx_fill_nbuf_data_attr_rh(qdf_nbuf_t nbuf)
{
	uint32_t pkt_offset;
	uint32_t tx_classify;
	uint32_t data_attr;

	/* Enable tx_classify bit in CE SRC DESC for all data packets */
	tx_classify = 1;
	pkt_offset = qdf_nbuf_get_frag_len(nbuf, 0);

	data_attr = tx_classify << CE_DESC_TX_CLASSIFY_BIT_S;
	data_attr |= pkt_offset  << CE_DESC_PKT_OFFSET_BIT_S;

	qdf_nbuf_data_attr_set(nbuf, data_attr);
}

QDF_STATUS
dp_tx_hw_enqueue_rh(struct dp_soc *soc, struct dp_vdev *vdev,
		    struct dp_tx_desc_s *tx_desc, uint16_t fw_metadata,
		    struct cdp_tx_exception_metadata *tx_exc_metadata,
		    struct dp_tx_msdu_info_s *msdu_info)
{
	struct dp_pdev_rh *rh_pdev = dp_get_rh_pdev_from_dp_pdev(vdev->pdev);
	struct dp_tx_ep_info_rh *tx_ep_info = &rh_pdev->tx_ep_info;
	uint32_t download_len = tx_ep_info->download_len;
	qdf_nbuf_t nbuf = tx_desc->nbuf;
	uint8_t tid = msdu_info->tid;
	uint32_t *hal_tx_desc_cached;
	int ret;

	/*
	 * Setting it initialization statically here to avoid
	 * a memset call jump with qdf_mem_set call
	 */
	uint8_t cached_desc[HAL_TX_DESC_LEN_BYTES] = { 0 };

	enum cdp_sec_type sec_type = ((tx_exc_metadata &&
			tx_exc_metadata->sec_type != CDP_INVALID_SEC_TYPE) ?
			tx_exc_metadata->sec_type : vdev->sec_type);

	QDF_STATUS status = QDF_STATUS_E_RESOURCES;

	if (!dp_tx_is_desc_id_valid(soc, tx_desc->id)) {
		dp_err_rl("Invalid tx desc id:%d", tx_desc->id);
		return QDF_STATUS_E_RESOURCES;
	}

	hal_tx_desc_cached = (void *)cached_desc;

	hal_tx_desc_set_buf_addr(soc->hal_soc, hal_tx_desc_cached,
				 tx_desc->dma_addr, 0, tx_desc->id,
				 (tx_desc->flags & DP_TX_DESC_FLAG_FRAG));
	hal_tx_desc_set_lmac_id(soc->hal_soc, hal_tx_desc_cached,
				vdev->lmac_id);
	hal_tx_desc_set_search_type(soc->hal_soc, hal_tx_desc_cached,
				    vdev->search_type);
	hal_tx_desc_set_search_index(soc->hal_soc, hal_tx_desc_cached,
				     vdev->bss_ast_idx);

	hal_tx_desc_set_encrypt_type(hal_tx_desc_cached,
				     sec_type_map[sec_type]);
	hal_tx_desc_set_cache_set_num(soc->hal_soc, hal_tx_desc_cached,
				      (vdev->bss_ast_hash & 0xF));

	hal_tx_desc_set_fw_metadata(hal_tx_desc_cached, fw_metadata);
	hal_tx_desc_set_buf_length(hal_tx_desc_cached, tx_desc->length);
	hal_tx_desc_set_buf_offset(hal_tx_desc_cached, tx_desc->pkt_offset);
	hal_tx_desc_set_encap_type(hal_tx_desc_cached, tx_desc->tx_encap_type);
	hal_tx_desc_set_addr_search_flags(hal_tx_desc_cached,
					  vdev->hal_desc_addr_search_flags);

	if (tx_desc->flags & DP_TX_DESC_FLAG_TO_FW)
		hal_tx_desc_set_to_fw(hal_tx_desc_cached, 1);

	/* verify checksum offload configuration*/
	if ((qdf_nbuf_get_tx_cksum(nbuf) == QDF_NBUF_TX_CKSUM_TCP_UDP) ||
	    qdf_nbuf_is_tso(nbuf))  {
		hal_tx_desc_set_l3_checksum_en(hal_tx_desc_cached, 1);
		hal_tx_desc_set_l4_checksum_en(hal_tx_desc_cached, 1);
	}

	if (tid != HTT_TX_EXT_TID_INVALID)
		hal_tx_desc_set_hlos_tid(hal_tx_desc_cached, tid);

	if (tx_desc->flags & DP_TX_DESC_FLAG_MESH)
		hal_tx_desc_set_mesh_en(soc->hal_soc, hal_tx_desc_cached, 1);

	if (!dp_tx_desc_set_ktimestamp(vdev, tx_desc))
		dp_tx_desc_set_timestamp(tx_desc);

	dp_verbose_debug("length:%d , type = %d, dma_addr %llx, offset %d desc id %u",
			 tx_desc->length,
			 (tx_desc->flags & DP_TX_DESC_FLAG_FRAG),
			 (uint64_t)tx_desc->dma_addr, tx_desc->pkt_offset,
			 tx_desc->id);

	hal_tx_desc_sync(hal_tx_desc_cached, tx_desc->tcl_cmd_vaddr);

	qdf_nbuf_frag_push_head(nbuf, DP_RH_TX_TCL_DESC_SIZE,
				(char *)tx_desc->tcl_cmd_vaddr,
				tx_desc->tcl_cmd_paddr);

	download_len = dp_tx_adjust_download_len_rh(nbuf, download_len);

	if (qdf_nbuf_is_tso(nbuf)) {
		QDF_NBUF_CB_PADDR(nbuf) =
			msdu_info->u.tso_info.curr_seg->seg.tso_frags[0].paddr;
		download_len = dp_tx_adjust_tso_download_len_rh(nbuf, msdu_info,
								download_len);
	}

	dp_tx_fill_nbuf_data_attr_rh(nbuf);

	ret = ce_send_fast(tx_ep_info->ce_tx_hdl, nbuf,
			   tx_ep_info->tx_endpoint, download_len);
	if (!ret) {
		dp_verbose_debug("CE tx ring full");
		/* TODO: Should this be a separate ce_ring_full stat? */
		DP_STATS_INC(soc, tx.tcl_ring_full[0], 1);
		DP_STATS_INC(vdev, tx_i.dropped.enqueue_fail, 1);
		goto enqueue_fail;
	}

	tx_desc->flags |= DP_TX_DESC_FLAG_QUEUED_TX;
	dp_vdev_peer_stats_update_protocol_cnt_tx(vdev, nbuf);
	DP_STATS_INC_PKT(vdev, tx_i.processed, 1, tx_desc->length);
	status = QDF_STATUS_SUCCESS;

enqueue_fail:
	dp_pkt_add_timestamp(vdev, QDF_PKT_TX_DRIVER_EXIT,
			     qdf_get_log_timestamp(), tx_desc->nbuf);

	return status;
}

/**
 * dp_tx_tcl_desc_pool_alloc_rh() - Allocate the tcl descriptor pool
 *				    based on pool_id
 * @soc: Handle to DP SoC structure
 * @num_elem: Number of descriptor elements per pool
 * @pool_id: Pool to allocate
 *
 * Return: QDF_STATUS_SUCCESS
 *	   QDF_STATUS_E_NOMEM
 */
static QDF_STATUS
dp_tx_tcl_desc_pool_alloc_rh(struct dp_soc *soc, uint32_t num_elem,
			     uint8_t pool_id)
{
	struct dp_soc_rh *rh_soc = dp_get_rh_soc_from_dp_soc(soc);
	struct dp_tx_tcl_desc_pool_s *tcl_desc_pool;
	uint16_t elem_size = DP_RH_TX_TCL_DESC_SIZE;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	qdf_dma_context_t memctx = 0;

	if (pool_id > MAX_TXDESC_POOLS - 1)
		return QDF_STATUS_E_INVAL;

	/* Allocate tcl descriptors in coherent memory */
	tcl_desc_pool = &rh_soc->tcl_desc_pool[pool_id];
	memctx = qdf_get_dma_mem_context(tcl_desc_pool, memctx);
	dp_desc_multi_pages_mem_alloc(soc, DP_TX_TCL_DESC_TYPE,
				      &tcl_desc_pool->desc_pages,
				      elem_size, num_elem, memctx, false);

	if (!tcl_desc_pool->desc_pages.num_pages) {
		dp_err("failed to allocate tcl desc Pages");
		status = QDF_STATUS_E_NOMEM;
		goto err_alloc_fail;
	}

	return status;

err_alloc_fail:
	dp_desc_multi_pages_mem_free(soc, DP_TX_TCL_DESC_TYPE,
				     &tcl_desc_pool->desc_pages,
				     memctx, false);
	return status;
}

/**
 * dp_tx_tcl_desc_pool_free_rh() -  Free the tcl descriptor pool
 * @soc: Handle to DP SoC structure
 * @pool_id: pool to free
 *
 */
static void dp_tx_tcl_desc_pool_free_rh(struct dp_soc *soc, uint8_t pool_id)
{
	struct dp_soc_rh *rh_soc = dp_get_rh_soc_from_dp_soc(soc);
	struct dp_tx_tcl_desc_pool_s *tcl_desc_pool;
	qdf_dma_context_t memctx = 0;

	if (pool_id > MAX_TXDESC_POOLS - 1)
		return;

	tcl_desc_pool = &rh_soc->tcl_desc_pool[pool_id];
	memctx = qdf_get_dma_mem_context(tcl_desc_pool, memctx);

	dp_desc_multi_pages_mem_free(soc, DP_TX_TCL_DESC_TYPE,
				     &tcl_desc_pool->desc_pages,
				     memctx, false);
}

/**
 * dp_tx_tcl_desc_pool_init_rh() - Initialize tcl descriptor pool
 *				   based on pool_id
 * @soc: Handle to DP SoC structure
 * @num_elem: Number of descriptor elements per pool
 * @pool_id: pool to initialize
 *
 * Return: QDF_STATUS_SUCCESS
 *	   QDF_STATUS_E_FAULT
 */
static QDF_STATUS
dp_tx_tcl_desc_pool_init_rh(struct dp_soc *soc, uint32_t num_elem,
			    uint8_t pool_id)
{
	struct dp_soc_rh *rh_soc = dp_get_rh_soc_from_dp_soc(soc);
	struct dp_tx_tcl_desc_pool_s *tcl_desc_pool;
	struct qdf_mem_dma_page_t *page_info;
	QDF_STATUS status;

	tcl_desc_pool = &rh_soc->tcl_desc_pool[pool_id];
	tcl_desc_pool->elem_size = DP_RH_TX_TCL_DESC_SIZE;
	tcl_desc_pool->elem_count = num_elem;

	/* Link tcl descriptors into a freelist */
	if (qdf_mem_multi_page_link(soc->osdev, &tcl_desc_pool->desc_pages,
				    tcl_desc_pool->elem_size,
				    tcl_desc_pool->elem_count,
				    false)) {
		dp_err("failed to link tcl desc Pages");
		status = QDF_STATUS_E_FAULT;
		goto err_link_fail;
	}

	page_info = tcl_desc_pool->desc_pages.dma_pages;
	tcl_desc_pool->freelist = (uint32_t *)page_info->page_v_addr_start;

	return QDF_STATUS_SUCCESS;

err_link_fail:
	return status;
}

/**
 * dp_tx_tcl_desc_pool_deinit_rh() - De-initialize tcl descriptor pool
 *				     based on pool_id
 * @soc: Handle to DP SoC structure
 * @pool_id: pool to de-initialize
 *
 */
static void dp_tx_tcl_desc_pool_deinit_rh(struct dp_soc *soc, uint8_t pool_id)
{
}

/**
 * dp_tx_alloc_tcl_desc_rh() - Allocate a tcl descriptor from the pool
 * @tcl_desc_pool: Tcl descriptor pool
 * @tx_desc: SW TX descriptor
 * @index: Index into the tcl descriptor pool
 */
static void dp_tx_alloc_tcl_desc_rh(struct dp_tx_tcl_desc_pool_s *tcl_desc_pool,
				    struct dp_tx_desc_s *tx_desc,
				    uint32_t index)
{
	struct qdf_mem_dma_page_t *dma_page;
	uint32_t page_id;
	uint32_t offset;

	tx_desc->tcl_cmd_vaddr = (void *)tcl_desc_pool->freelist;

	if (tcl_desc_pool->freelist)
		tcl_desc_pool->freelist =
			*((uint32_t **)tcl_desc_pool->freelist);

	page_id = index / tcl_desc_pool->desc_pages.num_element_per_page;
	offset = index % tcl_desc_pool->desc_pages.num_element_per_page;
	dma_page = &tcl_desc_pool->desc_pages.dma_pages[page_id];

	tx_desc->tcl_cmd_paddr =
		dma_page->page_p_addr + offset * tcl_desc_pool->elem_size;
}

QDF_STATUS dp_tx_desc_pool_init_rh(struct dp_soc *soc,
				   uint32_t num_elem,
				   uint8_t pool_id)
{
	struct dp_soc_rh *rh_soc = dp_get_rh_soc_from_dp_soc(soc);
	uint32_t id, count, page_id, offset, pool_id_32;
	struct dp_tx_desc_s *tx_desc;
	struct dp_tx_tcl_desc_pool_s *tcl_desc_pool;
	struct dp_tx_desc_pool_s *tx_desc_pool;
	uint16_t num_desc_per_page;
	QDF_STATUS status;

	status = dp_tx_tcl_desc_pool_init_rh(soc, num_elem, pool_id);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("failed to initialise tcl desc pool %d", pool_id);
		goto err_out;
	}

	status = dp_tx_ext_desc_pool_init_by_id(soc, num_elem, pool_id);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("failed to initialise tx ext desc pool %d", pool_id);
		goto err_deinit_tcl_pool;
	}

	status = dp_tx_tso_desc_pool_init_by_id(soc, num_elem, pool_id);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("failed to initialise tso desc pool %d", pool_id);
		goto err_deinit_tx_ext_pool;
	}

	status = dp_tx_tso_num_seg_pool_init_by_id(soc, num_elem, pool_id);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("failed to initialise tso num seg pool %d", pool_id);
		goto err_deinit_tso_pool;
	}

	tx_desc_pool = &soc->tx_desc[pool_id];
	tcl_desc_pool = &rh_soc->tcl_desc_pool[pool_id];
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
		dp_tx_alloc_tcl_desc_rh(tcl_desc_pool, tx_desc, count);
		tx_desc = tx_desc->next;
		count++;
	}

	return QDF_STATUS_SUCCESS;

err_deinit_tso_pool:
	dp_tx_tso_desc_pool_deinit_by_id(soc, pool_id);
err_deinit_tx_ext_pool:
	dp_tx_ext_desc_pool_deinit_by_id(soc, pool_id);
err_deinit_tcl_pool:
	dp_tx_tcl_desc_pool_deinit_rh(soc, pool_id);
err_out:
	/* TODO: is assert needed ? */
	qdf_assert_always(0);
	return status;
}

void dp_tx_desc_pool_deinit_rh(struct dp_soc *soc,
			       struct dp_tx_desc_pool_s *tx_desc_pool,
			       uint8_t pool_id)
{
	dp_tx_tso_num_seg_pool_free_by_id(soc, pool_id);
	dp_tx_tso_desc_pool_deinit_by_id(soc, pool_id);
	dp_tx_ext_desc_pool_deinit_by_id(soc, pool_id);
	dp_tx_tcl_desc_pool_deinit_rh(soc, pool_id);
}

QDF_STATUS dp_tx_compute_tx_delay_rh(struct dp_soc *soc,
				     struct dp_vdev *vdev,
				     struct hal_tx_completion_status *ts,
				     uint32_t *delay_us)
{
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS dp_tx_desc_pool_alloc_rh(struct dp_soc *soc, uint32_t num_elem,
				    uint8_t pool_id)
{
	QDF_STATUS status;

	status = dp_tx_tcl_desc_pool_alloc_rh(soc, num_elem, pool_id);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("failed to allocate tcl desc pool %d\n", pool_id);
		goto err_tcl_desc_pool;
	}

	status = dp_tx_ext_desc_pool_alloc_by_id(soc, num_elem, pool_id);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("failed to allocate tx ext desc pool %d\n", pool_id);
		goto err_free_tcl_pool;
	}

	status = dp_tx_tso_desc_pool_alloc_by_id(soc, num_elem, pool_id);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("failed to allocate tso desc pool %d\n", pool_id);
		goto err_free_tx_ext_pool;
	}

	status = dp_tx_tso_num_seg_pool_alloc_by_id(soc, num_elem, pool_id);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("failed to allocate tso num seg pool %d\n", pool_id);
		goto err_free_tso_pool;
	}

	return status;

err_free_tso_pool:
	dp_tx_tso_desc_pool_free_by_id(soc, pool_id);
err_free_tx_ext_pool:
	dp_tx_ext_desc_pool_free_by_id(soc, pool_id);
err_free_tcl_pool:
	dp_tx_tcl_desc_pool_free_rh(soc, pool_id);
err_tcl_desc_pool:
	/* TODO: is assert needed ? */
	qdf_assert_always(0);
	return status;
}

void dp_tx_desc_pool_free_rh(struct dp_soc *soc, uint8_t pool_id)
{
	dp_tx_tso_num_seg_pool_free_by_id(soc, pool_id);
	dp_tx_tso_desc_pool_free_by_id(soc, pool_id);
	dp_tx_ext_desc_pool_free_by_id(soc, pool_id);
	dp_tx_tcl_desc_pool_free_rh(soc, pool_id);
}
