/*
 * Copyright (c) 2012-2014, 2016 The Linux Foundation. All rights reserved.
 *
 * Previously licensed under the ISC license by Qualcomm Atheros, Inc.
 *
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

/*
 * This file was originally distributed by Qualcomm Atheros, Inc.
 * under proprietary terms before Copyright ownership was assigned
 * to the Linux Foundation.
 */

#ifndef _OL_TXRX__H_
#define _OL_TXRX__H_

#include <cdf_nbuf.h>           /* cdf_nbuf_t */
#include <ol_txrx_types.h>      /* ol_txrx_vdev_t, etc. */
#include <ol_ctrl_api.h>        /* ol_pdev_handle */
#include "cds_sched.h"

void ol_txrx_peer_unref_delete(struct ol_txrx_peer_t *peer);

/**
 * ol_tx_desc_pool_size_hl() - allocate tx descriptor pool size for HL systems
 * @ctrl_pdev: the control pdev handle
 *
 * Return: allocated pool size
 */
u_int16_t
ol_tx_desc_pool_size_hl(ol_pdev_handle ctrl_pdev);

#ifndef OL_TX_AVG_FRM_BYTES
#define OL_TX_AVG_FRM_BYTES 1000
#endif

#ifndef OL_TX_DESC_POOL_SIZE_MIN_HL
#define OL_TX_DESC_POOL_SIZE_MIN_HL 500
#endif

#ifndef OL_TX_DESC_POOL_SIZE_MAX_HL
#define OL_TX_DESC_POOL_SIZE_MAX_HL 5000
#endif


#ifdef CONFIG_PER_VDEV_TX_DESC_POOL
#define TXRX_HL_TX_FLOW_CTRL_VDEV_LOW_WATER_MARK 400
#define TXRX_HL_TX_FLOW_CTRL_MGMT_RESERVED 100
#endif

#ifdef CONFIG_TX_DESC_HI_PRIO_RESERVE
#define TXRX_HL_TX_DESC_HI_PRIO_RESERVED 20
#endif

#if defined(CONFIG_HL_SUPPORT) && defined(FEATURE_WLAN_TDLS)

void
ol_txrx_hl_tdls_flag_reset(struct ol_txrx_vdev_t *vdev, bool flag);
#else

static inline void
ol_txrx_hl_tdls_flag_reset(struct ol_txrx_vdev_t *vdev, bool flag)
{
	return;
}
#endif

#ifdef CONFIG_HL_SUPPORT

void
ol_txrx_copy_mac_addr_raw(ol_txrx_vdev_handle vdev, uint8_t *bss_addr);

void
ol_txrx_add_last_real_peer(ol_txrx_pdev_handle pdev,
			   ol_txrx_vdev_handle vdev,
			   uint8_t *peer_id);

bool
is_vdev_restore_last_peer(struct ol_txrx_peer_t *peer);

void
ol_txrx_update_last_real_peer(
	ol_txrx_pdev_handle pdev,
	struct ol_txrx_peer_t *peer,
	uint8_t *peer_id, bool restore_last_peer);
#else

static inline void
ol_txrx_copy_mac_addr_raw(ol_txrx_vdev_handle vdev, uint8_t *bss_addr)
{
	return;
}

static inline void
ol_txrx_add_last_real_peer(ol_txrx_pdev_handle pdev,
			   ol_txrx_vdev_handle vdev, uint8_t *peer_id)
{
	return;
}

static inline bool
is_vdev_restore_last_peer(struct ol_txrx_peer_t *peer)
{
	return  false;
}

static inline void
ol_txrx_update_last_real_peer(
	ol_txrx_pdev_handle pdev,
	struct ol_txrx_peer_t *peer,
	uint8_t *peer_id, bool restore_last_peer)

{
	return;
}
#endif


/**
 * ol_txrx_get_vdev_from_vdev_id() - get vdev from vdev_id
 * @vdev_id: vdev_id
 *
 * Return: vdev handle
 *            NULL if not found.
 */
static inline ol_txrx_vdev_handle ol_txrx_get_vdev_from_vdev_id(uint8_t vdev_id)
{
	ol_txrx_pdev_handle pdev = cds_get_context(CDF_MODULE_ID_TXRX);
	ol_txrx_vdev_handle vdev = NULL;

	if (cdf_unlikely(!pdev)) {
		return NULL;
	}

	TAILQ_FOREACH(vdev, &pdev->vdev_list, vdev_list_elem) {
		if (vdev->vdev_id == vdev_id)
			break;
	}

	return vdev;
}

#endif /* _OL_TXRX__H_ */
