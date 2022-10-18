/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include <linux/netdevice.h>
#include <qdf_nbuf.h>
#include <dp_types.h>
#include <dp_internal.h>
#include <hal_be_hw_headers.h>
#include <dp_peer.h>
#include <dp_tx.h>
#include <be/dp_be_tx.h>
#include <hal_tx.h>
#include <hal_be_api.h>
#include <be/dp_be.h>
#include <ppe_vp_public.h>
#include "dp_ppeds.h"
#include <ppeds_wlan.h>
#include <wlan_osif_priv.h>
#include <dp_tx_desc.h>
#include "dp_rx.h"
#include "dp_be_rx.h"
#include "dp_rx_buffer_pool.h"

/**
 * dp_ppeds_deinit_ppe_vp_tbl_be - PPE VP table dealoc
 * @be_soc: BE SoC
 *
 * Deallocate the VP table
 *
 * Return: QDF_STATUS
 */
static void dp_ppeds_deinit_ppe_vp_tbl_be(struct dp_soc_be *be_soc)
{
	if (be_soc->ppe_vp_tbl)
		qdf_mem_free(be_soc->ppe_vp_tbl);
	qdf_mutex_destroy(&be_soc->ppe_vp_tbl_lock);
}

/**
 * dp_ppeds_init_ppe_vp_tbl_be - PPE VP table alloc
 * @be_soc: BE SoC
 *
 * Allocate the VP table
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS dp_ppeds_init_ppe_vp_tbl_be(struct dp_soc_be *be_soc)
{
	int num_ppe_vp_entries, i;

	num_ppe_vp_entries =
		hal_tx_get_num_ppe_vp_tbl_entries(be_soc->soc.hal_soc);

	be_soc->ppe_vp_tbl =
		qdf_mem_malloc(num_ppe_vp_entries *
			       sizeof(*be_soc->ppe_vp_tbl));

	if (!be_soc->ppe_vp_tbl)
		return QDF_STATUS_E_NOMEM;

	qdf_mutex_create(&be_soc->ppe_vp_tbl_lock);

	for (i = 0; i < num_ppe_vp_entries; i++)
		be_soc->ppe_vp_tbl[i].is_configured = false;

	return QDF_STATUS_SUCCESS;
}

/**
 * dp_ppeds_get_vdev_vp_config_be() - Get the PPE vp config
 * @be_vdev: BE vdev
 * @vp_cfg: VP config
 *
 * Get the VDEV config for PPE
 *
 * Return: void
 */
static void dp_ppeds_get_vdev_vp_config_be(struct dp_vdev_be *be_vdev,
					    union hal_tx_ppe_vp_config *vp_cfg)
{
	struct dp_vdev *vdev = &be_vdev->vdev;

	/*
	 * Update the parameters frm profile.
	 */
	vp_cfg->vp_num = be_vdev->ppe_vp_profile.vp_num;

	/* This is valid when index based search is
	 * enabled for STA vap in bank register
	 */
	vp_cfg->search_idx_reg_num =
		be_vdev->ppe_vp_profile.search_idx_reg_num;

	vp_cfg->use_ppe_int_pri = be_vdev->ppe_vp_profile.use_ppe_int_pri;
	vp_cfg->to_fw = be_vdev->ppe_vp_profile.to_fw;
	vp_cfg->drop_prec_enable = be_vdev->ppe_vp_profile.drop_prec_enable;

	vp_cfg->bank_id = be_vdev->bank_id;
	vp_cfg->pmac_id = vdev->lmac_id;
	vp_cfg->vdev_id = vdev->vdev_id;
}

/**
 * dp_ppeds_setup_vp_entry_be: Setup the PPE VP entry
 * be_soc: Be Soc
 * be_vdev: BE vdev
 *
 * Setup the PPE VP entry for HAL
 *
 * Return: void
 */
static void dp_ppeds_setup_vp_entry_be(struct dp_soc_be *be_soc,
					struct dp_vdev_be *be_vdev)
{
	int ppe_vp_num_idx;
	union hal_tx_ppe_vp_config vp_cfg = {0};

	dp_ppeds_get_vdev_vp_config_be(be_vdev, &vp_cfg);

	ppe_vp_num_idx = be_vdev->ppe_vp_profile.ppe_vp_num_idx;

	hal_tx_populate_ppe_vp_entry(be_soc->soc.hal_soc,
				     &vp_cfg,
				     ppe_vp_num_idx);
}

/**
 * dp_ppeds_alloc_vp_tbl_entry_be() - PPE VP entry alloc
 * @be_soc: BE SoC
 * @be_vdev: BE VAP
 *
 * PPE VP table entry alloc
 *
 * Return: Return PPE VP index to be used
 */
static int dp_ppeds_alloc_vp_tbl_entry_be(struct dp_soc_be *be_soc,
					   struct dp_vdev_be *be_vdev)
{
	int num_ppe_vp_max, i;

	num_ppe_vp_max =
		hal_tx_get_num_ppe_vp_tbl_entries(be_soc->soc.hal_soc);

	qdf_mutex_acquire(&be_soc->ppe_vp_tbl_lock);
	if (be_soc->num_ppe_vp_entries == num_ppe_vp_max) {
		qdf_mutex_release(&be_soc->ppe_vp_tbl_lock);
		dp_err("Maximum ppe_vp count reached for vap");
		return -ENOSPC;
	}

	/*
	 * Increase the count in atomic context.
	 */
	be_soc->num_ppe_vp_entries++;

	for (i = 0; i < num_ppe_vp_max; i++) {
		if (!be_soc->ppe_vp_tbl[i].is_configured)
			break;
	}

	/*
	 * As we already found the current count not maxed out, we should
	 * always find an non configured entry.
	 */
	qdf_assert_always(!(i == num_ppe_vp_max));

	be_soc->ppe_vp_tbl[i].is_configured = true;
	qdf_mutex_release(&be_soc->ppe_vp_tbl_lock);
	return i;
}

/**
 * dp_ppeds_dealloc_vp_tbl_entry_be() - PPE VP entry dealloc
 * @be_soc: BE SoC
 * @ppe_vp_num_idx: PPE VP index
 *
 * PPE VP table entry dealloc
 *
 * Return: void
 */
static void dp_ppeds_dealloc_vp_tbl_entry_be(struct dp_soc_be *be_soc,
					      int ppe_vp_num_idx)
{
	int num_ppe_vp_max;
	union hal_tx_ppe_vp_config vp_cfg = {0};

	num_ppe_vp_max =
		hal_tx_get_num_ppe_vp_tbl_entries(be_soc->soc.hal_soc);

	if (ppe_vp_num_idx < 0 || ppe_vp_num_idx >= num_ppe_vp_max) {
		dp_err("INvalid PPE VP free index");
		return;
	}

	/*
	 * Clear the HAL specific register for this PPE VP index.
	 */
	hal_tx_populate_ppe_vp_entry(be_soc->soc.hal_soc,
				     &vp_cfg,
				     ppe_vp_num_idx);

	/*
	 * Release the current index for reuse.
	 */
	qdf_mutex_acquire(&be_soc->ppe_vp_tbl_lock);
	if (!be_soc->ppe_vp_tbl[ppe_vp_num_idx].is_configured) {
		qdf_mutex_release(&be_soc->ppe_vp_tbl_lock);
		dp_err("PPE VP is not configured at idx:%d", ppe_vp_num_idx);
		return;
	}

	be_soc->ppe_vp_tbl[ppe_vp_num_idx].is_configured = false;
	be_soc->num_ppe_vp_entries--;
	qdf_mutex_release(&be_soc->ppe_vp_tbl_lock);
}

/**
 * dp_ppeds_get_batched_tx_desc() - Get Tx descriptors in a buffer array
 * @ppeds_handle: PPE-DS WLAN handle
 * @arr: Tx buffer array to be filled
 * @num_buff_req: Number of buffer to be allocated
 * @buff_size: Size of the buffer to be allocated
 * @headroom: Headroom required in the allocated buffer
 *
 * PPE-DS get buffer and the Tx descriptors in an array
 *
 * Return: Number of buffers allocated
 */
static
uint32_t dp_ppeds_get_batched_tx_desc(ppeds_wlan_handle_t *ppeds_handle,
				      struct ppeds_wlan_txdesc_elem *arr,
				      uint32_t num_buff_req,
				      uint32_t buff_size,
				      uint32_t headroom)
{
	uint32_t i = 0;
	unsigned int cpu;
	struct dp_tx_desc_s *tx_desc;
	struct dp_soc *soc = *((struct dp_soc **)ppeds_wlan_priv(ppeds_handle));
	qdf_dma_addr_t paddr;

	cpu = qdf_get_cpu();
	for (i = 0; i < num_buff_req; i++) {

		/*
		 * Get a free skb.
		 */
		qdf_nbuf_t nbuf = qdf_nbuf_alloc_simple(soc->osdev, buff_size, 0, 0, 0);
		if (qdf_unlikely(!nbuf)) {
			break;
		}

		/*
		 * Reserve headrrom
		 */
		qdf_nbuf_reserve(nbuf, headroom);

		/*
		 * Map(Get Phys address)
		 */
		qdf_nbuf_dma_inv_range_no_dsb((void *)nbuf->data,
				(void *)(nbuf->data + buff_size - headroom));
		paddr = (qdf_dma_addr_t)qdf_mem_virt_to_phys(nbuf->data);

		tx_desc = dp_tx_desc_alloc(soc, cpu);
		if (!tx_desc) {
			qdf_nbuf_free_simple(nbuf);
			dp_err("ran out of txdesc");
			break;
		}

		tx_desc->flags |= DP_TX_DESC_FLAG_PPEDS;
		tx_desc->pdev = NULL;
		tx_desc->nbuf = nbuf;

		arr[i].opaque_lo = tx_desc->id;
		arr[i].opaque_hi = 0;
		arr[i].buff_addr = paddr;
	}

	return i;
}

/**
 * dp_ppeds_release_rx_desc() - Release the Rx descriptors from buffer array
 * @ppeds_handle: PPE-DS WLAN handle
 * @arr: Rx buffer array containing Rx descriptors
 * @count: Number of Rx descriptors to be freed
 *
 * Return: void
 */
static void dp_ppeds_release_rx_desc(ppeds_wlan_handle_t *ppeds_handle,
					struct ppeds_wlan_rxdesc_elem *arr, uint16_t count)
{
	struct dp_rx_desc *rx_desc;
	struct dp_soc *soc = *((struct dp_soc **)ppeds_wlan_priv(ppeds_handle));
	union dp_rx_desc_list_elem_t *head[WLAN_MAX_MLO_CHIPS][MAX_PDEV_CNT] = {0};
	union dp_rx_desc_list_elem_t *tail[WLAN_MAX_MLO_CHIPS][MAX_PDEV_CNT] = {0};
	uint32_t rx_bufs_reaped[WLAN_MAX_MLO_CHIPS][MAX_PDEV_CNT] = {0};
	struct dp_srng *dp_rxdma_srng;
	struct rx_desc_pool *rx_desc_pool;
	struct dp_soc *replenish_soc;
	uint32_t i, mac_id;
	uint8_t chip_id;

	for (i = 0; i < count; i++) {
		rx_desc = (struct dp_rx_desc *)arr[i].cookie;
		if (rx_desc == NULL) {
			dp_err("Rx descriptor is NULL\n");
			continue;
		}
		rx_bufs_reaped[rx_desc->chip_id][rx_desc->pool_id]++;

		dp_rx_nbuf_unmap(soc, rx_desc, REO2PPE_DST_IND);
		rx_desc->unmapped = 1;
		dp_rx_buffer_pool_nbuf_free(soc, rx_desc->nbuf, rx_desc->pool_id);

		dp_rx_add_to_free_desc_list(&head[rx_desc->chip_id][rx_desc->pool_id],
				&tail[rx_desc->chip_id][rx_desc->pool_id], rx_desc);
	}

	for (chip_id = 0; chip_id < WLAN_MAX_MLO_CHIPS; chip_id++) {
		for (mac_id = 0; mac_id < MAX_PDEV_CNT; mac_id++) {

			if (!rx_bufs_reaped[chip_id][mac_id])
				continue;

			replenish_soc = dp_rx_replensih_soc_get(soc, chip_id);

			dp_rxdma_srng = &replenish_soc->rx_refill_buf_ring[mac_id];
			rx_desc_pool = &replenish_soc->rx_desc_buf[mac_id];

			dp_rx_buffers_replenish(replenish_soc, mac_id, dp_rxdma_srng,
					rx_desc_pool, rx_bufs_reaped[chip_id][mac_id],
					&head[chip_id][mac_id], &tail[chip_id][mac_id],
					true);
		}
	}
}

/**
 * dp_ppeds_release_tx_desc_single() - Release the Tx descriptor
 * @ppeds_handle: PPE-DS WLAN handle
 * @cookie: Cookie to get the descriptor to be freed
 *
 * PPE-DS release single Tx descriptor API
 *
 * Return: void
 */
static
void dp_ppeds_release_tx_desc_single(ppeds_wlan_handle_t *ppeds_handle,
				     uint32_t cookie)
{
	return;
}

/**
 * dp_ppeds_set_tcl_prod_idx() - Set produce index of the PPE2TCL ring
 * @ppeds_handle: PPE-DS WLAN handle
 * @tcl_prod_idx: Producer index to be set
 *
 * Set producer index of the PPE2TCL ring
 *
 * Return: void
 */
static void dp_ppeds_set_tcl_prod_idx(ppeds_wlan_handle_t *ppeds_handle,
				      uint16_t tcl_prod_idx)
{
	struct dp_soc_be *soc =
			*((struct dp_soc_be **)ppeds_wlan_priv(ppeds_handle));
	struct dp_soc *dpsoc = (struct dp_soc *)soc;
	struct dp_srng *ppe2tcl_ring = &soc->ppe2tcl_ring;

	hal_srng_src_set_hp(ppe2tcl_ring->hal_srng, tcl_prod_idx);
	dp_tx_ring_access_end_wrapper(dpsoc, ppe2tcl_ring->hal_srng, 0);
}

/**
 * dp_ppeds_get_tcl_cons_idx() - Get consumer index of the PPE2TCL ring
 * @ppeds_handle: PPE-DS WLAN handle
 *
 * Get consumer index of the PPE2TCL ring
 *
 * Return: Consumer index of the PPE2TCL ring
 */
static uint16_t dp_ppeds_get_tcl_cons_idx(ppeds_wlan_handle_t *ppeds_handle)
{
	struct dp_soc_be *soc =
			*((struct dp_soc_be **)ppeds_wlan_priv(ppeds_handle));
	struct dp_srng *ppe2tcl_ring = &soc->ppe2tcl_ring;

	return hal_srng_src_get_tpidx(ppe2tcl_ring->hal_srng);
}

/**
 * dp_ppeds_set_reo_cons_idx() - Set consumer index of the REO2PPE ring
 * @ppeds_handle: PPE-DS WLAN handle
 * @tcl_prod_idx: Producer index to be set
 *
 * Set the consumer index of the PPE2TCL ring
 *
 * Return: void
 */
static void dp_ppeds_set_reo_cons_idx(ppeds_wlan_handle_t *ppeds_handle,
				      uint16_t reo_cons_idx)
{
	struct dp_soc_be *soc =
			*((struct dp_soc_be **)ppeds_wlan_priv(ppeds_handle));
	struct dp_soc *dpsoc = (struct dp_soc *)soc;
	struct dp_srng *reo2ppe_ring = &soc->reo2ppe_ring;

	hal_srng_dst_set_tp(reo2ppe_ring->hal_srng, reo_cons_idx);
	hal_srng_access_end_unlocked(dpsoc->hal_soc, reo2ppe_ring->hal_srng);
}

/**
 * dp_ppeds_get_reo_prod_idx() - Get producer index of the REO2PPE ring
 * @ppeds_handle: PPE-DS WLAN handle
 *
 * Get producer index of the REO2PPE ring
 *
 * Return: Producer index of the REO2PPE ring
 */
static uint16_t dp_ppeds_get_reo_prod_idx(ppeds_wlan_handle_t *ppeds_handle)
{
	struct dp_soc_be *soc =
			*((struct dp_soc_be **)ppeds_wlan_priv(ppeds_handle));
	struct dp_srng *reo2ppe_ring = &soc->reo2ppe_ring;

	return hal_srng_dst_get_hpidx(reo2ppe_ring->hal_srng);
}

/**
 * ppeds_ops
 *	PPE-DS WLAN operations
 */
static struct ppeds_wlan_ops ppeds_ops = {
	.get_tx_desc_many = dp_ppeds_get_batched_tx_desc,
	.release_tx_desc_single = dp_ppeds_release_tx_desc_single,
	.set_tcl_prod_idx  = dp_ppeds_set_tcl_prod_idx,
	.set_reo_cons_idx = dp_ppeds_set_reo_cons_idx,
	.get_tcl_cons_idx = dp_ppeds_get_tcl_cons_idx,
	.get_reo_prod_idx = dp_ppeds_get_reo_prod_idx,
	.release_rx_desc = dp_ppeds_release_rx_desc,
};

/**
 * dp_ppeds_attach_vdev_be() - PPE DS table entry alloc
 * @soc_hdl: CDP SoC Tx/Rx handle
 * @vdev_id: vdev id
 * @vp_arg: PPE VP opaque
 * @ppe_vp_num: Allocated PPE VP number
 *
 * Allocate a DS VP port and attach to BE VAP
 *
 * Return: QDF_STATUS
 */
QDF_STATUS dp_ppeds_attach_vdev_be(struct cdp_soc_t *soc_hdl, uint8_t vdev_id,
				   void *vp_arg, int32_t *ppe_vp_num)
{
	ol_osif_vdev_handle osif;
	struct net_device *dev;
	struct dp_soc *soc = cdp_soc_t_to_dp_soc(soc_hdl);
	struct dp_soc_be *be_soc;
	struct dp_vdev *vdev;
	struct dp_vdev_be *be_vdev;
	struct dp_ppe_vp_profile *vp_profile;
	int8_t ppe_vp_idx;

	be_soc = dp_get_be_soc_from_dp_soc(soc);

	/*
	 * Enable once PPEDS module is ready
	 */
	if (!be_soc->ppeds_handle) {
		dp_err("DS is not enabled on this SOC");
		return QDF_STATUS_E_INVAL;
	}

	vdev = dp_vdev_get_ref_by_id(soc, vdev_id, DP_MOD_ID_CDP);
	if (!vdev) {
		dp_err("%p: Could not find vdev for id:%d", be_soc, vdev_id);
		return QDF_STATUS_E_INVAL;
	}

	osif = vdev->osif_vdev;
	dev = OSIF_TO_NETDEV(osif);
	if (!dev) {
		dp_err("Could not find netdev");
		dp_vdev_unref_delete(soc, vdev, DP_MOD_ID_CDP);
		return QDF_STATUS_E_INVAL;
	}

	*ppe_vp_num = nss_ppe_ds_wlan_vp_alloc(be_soc->ppeds_handle, dev, vp_arg);
	if (*ppe_vp_num < 0) {
		dp_err("vp alloc failed\n");
		dp_vdev_unref_delete(soc, vdev, DP_MOD_ID_CDP);
		return QDF_STATUS_E_FAULT;
	}

	/*
	 * Extract the VP profile from the BE vap.
	 */
	be_vdev = dp_get_be_vdev_from_dp_vdev(vdev);
	vp_profile = &be_vdev->ppe_vp_profile;

	/*
	 * Test and set the PPE-DS enabled value.
	 */
	if (be_vdev->ppe_vp_enabled == PPE_VP_USER_TYPE_DS) {
		dp_err("%p: PPE DS is already enabled on this vdev_id:%d",
		       be_soc, vdev_id);
		nss_ppe_ds_wlan_vp_free(be_soc->ppeds_handle, *ppe_vp_num);
		dp_vdev_unref_delete(soc, vdev, DP_MOD_ID_CDP);
		return QDF_STATUS_E_ALREADY;
	} else
		be_vdev->ppe_vp_enabled = PPE_VP_USER_TYPE_DS;

	ppe_vp_idx = dp_ppeds_alloc_vp_tbl_entry_be(be_soc, be_vdev);
	if (ppe_vp_idx < 0) {
		dp_err("%p: Failed to allocate PPE VP idx for vdev_id:%d",
		       be_soc, vdev->vdev_id);
		nss_ppe_ds_wlan_vp_free(be_soc->ppeds_handle, *ppe_vp_num);
		be_vdev->ppe_vp_enabled = 0;
		dp_vdev_unref_delete(soc, vdev, DP_MOD_ID_CDP);
		return QDF_STATUS_E_RESOURCES;
	}

	vp_profile->vp_num = *ppe_vp_num;
	vp_profile->ppe_vp_num_idx = ppe_vp_idx;
	vp_profile->to_fw = 0;
	vp_profile->use_ppe_int_pri = 0;
	vp_profile->drop_prec_enable = 0;

	/*
	 * For the sta mode fill up the index reg number.
	 */
	dp_ppeds_setup_vp_entry_be(be_soc, be_vdev);
	dp_vdev_unref_delete(soc, vdev, DP_MOD_ID_CDP);
	return QDF_STATUS_SUCCESS;
}

/**
 * dp_ppeds_detach_vdev_be() - Detach the PPE DS port from vdev
 * @soc_hdl: CDP SoC Tx/Rx handle
 * @vdev_id: vdev id
 *
 * Detach the PPE DS port from BE VAP
 *
 * Return: void
 */
void dp_ppeds_detach_vdev_be(struct cdp_soc_t *soc_hdl, uint8_t vdev_id)
{
	struct dp_soc *soc = cdp_soc_t_to_dp_soc(soc_hdl);
	struct dp_soc_be *be_soc;
	struct dp_ppe_vp_profile *vp_profile;
	struct dp_vdev *vdev;
	struct dp_vdev_be *be_vdev;

	if (!soc) {
		dp_err("CDP soch handle is NULL");
		return;
	}

	be_soc = dp_get_be_soc_from_dp_soc(soc);
	vdev = dp_vdev_get_ref_by_id(soc, vdev_id, DP_MOD_ID_CDP);
	if (!vdev) {
		dp_err("%p: Could not find vdev for id:%d", be_soc, vdev_id);
		return;
	}

	/*
	 * Extract the VP profile from the BE vap.
	 */
	be_vdev = dp_get_be_vdev_from_dp_vdev(vdev);
	vp_profile = &be_vdev->ppe_vp_profile;

	if (vp_profile->ppe_vp_num_idx == -1) {
		dp_err("%p: Invalid PPE VP num for vdev_id:%d",
			be_vdev, vdev_id);
		dp_vdev_unref_delete(soc, vdev, DP_MOD_ID_CDP);
		return;
	}

	nss_ppe_ds_wlan_vp_free(be_soc->ppeds_handle, vp_profile->ppe_vp_num_idx);

	dp_ppeds_dealloc_vp_tbl_entry_be(be_soc, vp_profile->ppe_vp_num_idx);

	/*
	 * Test and reset PPE-DS enabled value
	 */
	if (be_vdev->ppe_vp_enabled != PPE_VP_USER_TYPE_DS)
		dp_err("%p: Invalid vdev got detached with vdev_id:%d",
			be_vdev, vdev_id);
	else
		be_vdev->ppe_vp_enabled = 0;

	dp_vdev_unref_delete(soc, vdev, DP_MOD_ID_CDP);
}

/*
 * dp_ppeds_set_int_pri2tid_be() - Set up INT_PRI to TID
 * @soc_hdl: CDP SoC Tx/Rx handle
 * @pri2tid: Priority to TID table
 *
 * Setup the INT_PRI to TID table
 *
 * Return: void
 */
void dp_ppeds_set_int_pri2tid_be(struct cdp_soc_t *soc_hdl,
				  uint8_t *pri2tid)
{
	struct dp_soc *soc = cdp_soc_t_to_dp_soc(soc_hdl);
	struct dp_soc_be *be_soc;
	union hal_tx_ppe_pri2tid_map0_config map0 = {0};
	union hal_tx_ppe_pri2tid_map1_config map1 = {0};
	uint32_t value = 0;

	if (!soc) {
		dp_err("CDP soch handle is NULL");
		return;
	}

	be_soc = dp_get_be_soc_from_dp_soc(soc);

	map0.int_pri0 = pri2tid[0];
	map0.int_pri1 = pri2tid[1];
	map0.int_pri2 = pri2tid[2];
	map0.int_pri3 = pri2tid[3];
	map0.int_pri4 = pri2tid[4];
	map0.int_pri5 = pri2tid[5];
	map0.int_pri6 = pri2tid[6];
	map0.int_pri7 = pri2tid[7];
	map0.int_pri8 = pri2tid[8];
	map0.int_pri9 = pri2tid[9];

	value = map0.val;

	hal_tx_set_int_pri2tid(be_soc->soc.hal_soc, value, 0);
	map1.int_pri10 = pri2tid[10];
	map1.int_pri11 = pri2tid[11];
	map1.int_pri12 = pri2tid[12];
	map1.int_pri13 = pri2tid[13];
	map1.int_pri14 = pri2tid[14];
	map1.int_pri15 = pri2tid[15];

	value = map1.val;

	hal_tx_set_int_pri2tid(be_soc->soc.hal_soc, value, 1);
}

/**
 * dp_ppeds_update_int_pri2tid_be() - Update INT_PRI to TID
 * @soc_hdl: CDP SoC Tx/Rx handle
 * @pri: Priority value
 * @tid: TID mapped to the priority value
 *
 * Update the tid for a specific INT_PRI
 *
 * Return: void
 */
void dp_ppeds_update_int_pri2tid_be(struct cdp_soc_t *soc_hdl,
				     uint8_t pri, uint8_t tid)
{
	struct dp_soc *soc = cdp_soc_t_to_dp_soc(soc_hdl);
	struct dp_soc_be *be_soc;

	if (!soc) {
		dp_err("CDP soch handle is NULL");
		return;
	}

	be_soc = dp_get_be_soc_from_dp_soc(soc);
	hal_tx_update_int_pri2tid(be_soc->soc.hal_soc, pri, tid);
}

/**
 * dp_ppeds_dump_ppe_vp_tbl_be() - Dump the PPE VP entries
 * @soc_hdl: CDP SoC Tx/Rx handle
 *
 * Dump the PPE VP entries of a SoC
 *
 * Return: void
 */
void dp_ppeds_dump_ppe_vp_tbl_be(struct cdp_soc_t *soc_hdl)
{
	struct dp_soc *soc = cdp_soc_t_to_dp_soc(soc_hdl);
	struct dp_soc_be *be_soc;

	if (!soc) {
		dp_err("CDP soch handle is NULL");
		return;
	}

	be_soc = dp_get_be_soc_from_dp_soc(soc);

	qdf_mutex_acquire(&be_soc->ppe_vp_tbl_lock);
	hal_tx_dump_ppe_vp_entry(be_soc->soc.hal_soc);
	qdf_mutex_release(&be_soc->ppe_vp_tbl_lock);
}

/**
 * dp_ppeds_vdev_enable_pri2tid_be() - Enable PPE PRI2TID conversion for a vap
 * @soc_hdl: CDP SoC Tx/Rx handle
 * @vdev_id : vdev id
 * @val: Boolean to enable/disable PRI2TID mapping
 *
 * Return: QDF_STATUS
 */
QDF_STATUS dp_ppeds_vdev_enable_pri2tid_be(struct cdp_soc_t *soc_hdl,
				       uint8_t vdev_id, bool val)
{
	struct dp_soc *soc = cdp_soc_t_to_dp_soc(soc_hdl);
	struct dp_soc_be *be_soc;
	struct dp_vdev *vdev;
	struct dp_vdev_be *be_vdev;
	struct dp_ppe_vp_profile *vp_profile;
	int8_t ppe_vp_idx;

	if (!soc) {
		dp_err("CDP soc handle is NULL");
		return QDF_STATUS_E_FAULT;
	}

	be_soc = dp_get_be_soc_from_dp_soc(soc);
	vdev = dp_vdev_get_ref_by_id(soc, vdev_id, DP_MOD_ID_CDP);
	if (!vdev) {
		dp_err("%p: Could not find vdev for id:%d", be_soc, vdev_id);
		return QDF_STATUS_E_FAULT;
	}

	/*
	 * Extract the VP profile from the BE vap.
	 */
	be_vdev = dp_get_be_vdev_from_dp_vdev(vdev);
	vp_profile = &be_vdev->ppe_vp_profile;

	/*
	 * Test PPE-DS enabled value
	 */
	if (be_vdev->ppe_vp_enabled != PPE_VP_USER_TYPE_DS) {
		dp_err("%p: PPE-DS is not enabled on this vdev_id:%d",
		       be_soc, vdev_id);
		dp_vdev_unref_delete(soc, vdev, DP_MOD_ID_CDP);
		return QDF_STATUS_E_FAULT;
	}

	ppe_vp_idx = vp_profile->ppe_vp_num_idx;

	hal_tx_enable_pri2tid_map(be_soc->soc.hal_soc, val, ppe_vp_idx);
	dp_vdev_unref_delete(soc, vdev, DP_MOD_ID_CDP);
	return QDF_STATUS_SUCCESS;
}

/**
 * dp_ppeds_stop_soc_be() - Stop the PPE-DS instance
 * @soc: DP SoC
 *
 * Return: void
 */
void dp_ppeds_stop_soc_be(struct dp_soc *soc)
{
	struct dp_soc_be *be_soc = dp_get_be_soc_from_dp_soc(soc);

	if (!be_soc->ppeds_handle)
		return;

	nss_ppe_ds_wlan_inst_stop(be_soc->ppeds_handle);
}

/**
 * dp_ppeds_start_soc_be() - Start the PPE-DS instance
 * @soc: DP SoC
 *
 * Return: void
 */
QDF_STATUS dp_ppeds_start_soc_be(struct dp_soc *soc)
{
	struct dp_soc_be *be_soc = dp_get_be_soc_from_dp_soc(soc);

	if (!be_soc->ppeds_handle) {
		dp_err("%p: ppeds not allocated", soc);
		return QDF_STATUS_SUCCESS;
	}

	if (nss_ppe_ds_wlan_inst_start(be_soc->ppeds_handle) != 0) {
		dp_err("%p: ppeds start failed", soc);
		return QDF_STATUS_SUCCESS;
	}

	return QDF_STATUS_SUCCESS;
}

/**
 * dp_ppeds_register_soc_be() - Register the PPE-DS instance
 * @be_soc: BE SoC
 *
 * Return: QDF_STATUS
 */
QDF_STATUS dp_ppeds_register_soc_be(struct dp_soc_be *be_soc)
{
	struct ppeds_wlan_reg_info reg_info;
	struct dp_soc *soc = DP_SOC_BE_GET_SOC(be_soc);

	if (!be_soc->ppeds_handle) {
		dp_err("%p: ppeds not attached", soc);
		return QDF_STATUS_SUCCESS;
	}

	reg_info.ppe2tcl_ba = be_soc->ppe2tcl_ring.base_paddr_aligned;
	reg_info.reo2ppe_ba = be_soc->reo2ppe_ring.base_paddr_aligned;
	reg_info.ppe2tcl_num_desc = be_soc->ppe2tcl_ring.num_entries;
	reg_info.reo2ppe_num_desc = be_soc->reo2ppe_ring.num_entries;
	if (nss_ppe_ds_wlan_inst_register(be_soc->ppeds_handle, &reg_info) != true) {
		dp_err("%p: ppeds registeration failed", be_soc);
		return QDF_STATUS_SUCCESS;
	}

	/*
	 * Implicit tx to tx complete mapping for PPE2TCL ring
	 */
	hal_tx_config_rbm_mapping_be(soc->hal_soc, be_soc->ppe2tcl_ring.hal_srng, 6);

	return QDF_STATUS_SUCCESS;
}

/**
 * dp_ppeds_detach_soc_be() - De-attach the PPE-DS instance
 * @be_soc: BE SoC
 *
 * Return: void
 */
void dp_ppeds_detach_soc_be(struct dp_soc_be *be_soc)
{
	if (!be_soc->ppeds_handle)
		return;

	nss_ppe_ds_wlan_inst_del(be_soc->ppeds_handle);
	be_soc->ppeds_handle = NULL;

	dp_ppeds_deinit_ppe_vp_tbl_be(be_soc);
}

/**
 * dp_ppeds_attach_soc_be() - Attach the PPE-DS instance
 * @be_soc: BE SoC
 *
 * Return: QDF_STATUS
 */
QDF_STATUS dp_ppeds_attach_soc_be(struct dp_soc_be *be_soc)
{
	struct dp_soc_be **besocptr;

	if (dp_ppeds_init_ppe_vp_tbl_be(be_soc) != QDF_STATUS_SUCCESS) {
		dp_err("%p: Failed to init ppe vp tbl", be_soc);
		goto fail2;
	}

	be_soc->ppeds_handle =
			nss_ppe_ds_wlan_inst_alloc(&ppeds_ops,
						   sizeof(struct dp_soc_be *));
	if (!be_soc->ppeds_handle) {
		dp_err("%p: Failed to allocate ppeds soc instance", be_soc);
		goto fail3;
	}

	 besocptr = (struct dp_soc_be **)ppeds_wlan_priv(be_soc->ppeds_handle);
	 *besocptr = be_soc;

	 return QDF_STATUS_SUCCESS;

fail3:
	dp_ppeds_deinit_ppe_vp_tbl_be(be_soc);
fail2:
	return QDF_STATUS_E_FAILURE;

}
