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
#include "hal_hw_headers.h"
#include "dp_types.h"
#include "dp_rx.h"
#include "dp_rh_rx.h"
#include "dp_peer.h"
#include "hal_rx.h"
#include "hal_rh_rx.h"
#include "hal_api.h"
#include "hal_rh_api.h"
#include "qdf_nbuf.h"
#include "dp_internal.h"
#ifdef WIFI_MONITOR_SUPPORT
#include <dp_mon.h>
#endif
#ifdef FEATURE_WDS
#include "dp_txrx_wds.h"
#endif
#include "dp_hist.h"
#include "dp_rx_buffer_pool.h"
#include "dp_rh.h"

static inline
bool is_sa_da_idx_valid(uint32_t max_ast,
			qdf_nbuf_t nbuf, struct hal_rx_msdu_metadata msdu_info)
{
	if ((qdf_nbuf_is_sa_valid(nbuf) && (msdu_info.sa_idx > max_ast)) ||
	    (!qdf_nbuf_is_da_mcbc(nbuf) && qdf_nbuf_is_da_valid(nbuf) &&
	     (msdu_info.da_idx > max_ast)))
		return false;

	return true;
}

#if defined(FEATURE_MCL_REPEATER) && defined(FEATURE_MEC)
/**
 * dp_rx_mec_check_wrapper() - wrapper to dp_rx_mcast_echo_check
 * @soc: core DP main context
 * @txrx_peer: dp peer handler
 * @rx_tlv_hdr: start of the rx TLV header
 * @nbuf: pkt buffer
 *
 * Return: bool (true if it is a looped back pkt else false)
 */
static inline bool dp_rx_mec_check_wrapper(struct dp_soc *soc,
					   struct dp_txrx_peer *txrx_peer,
					   uint8_t *rx_tlv_hdr,
					   qdf_nbuf_t nbuf)
{
	return dp_rx_mcast_echo_check(soc, txrx_peer, rx_tlv_hdr, nbuf);
}
#else
static inline bool dp_rx_mec_check_wrapper(struct dp_soc *soc,
					   struct dp_txrx_peer *txrx_peer,
					   uint8_t *rx_tlv_hdr,
					   qdf_nbuf_t nbuf)
{
	return false;
}
#endif

static bool
dp_rx_intrabss_ucast_check_rh(struct dp_soc *soc, qdf_nbuf_t nbuf,
			      struct dp_txrx_peer *ta_txrx_peer,
			      struct hal_rx_msdu_metadata *msdu_metadata,
			      uint8_t *p_tx_vdev_id)
{
	uint16_t da_peer_id;
	struct dp_txrx_peer *da_peer;
	struct dp_ast_entry *ast_entry;
	dp_txrx_ref_handle txrx_ref_handle = NULL;

	if (!qdf_nbuf_is_da_valid(nbuf) || qdf_nbuf_is_da_mcbc(nbuf))
		return false;

	ast_entry = soc->ast_table[msdu_metadata->da_idx];
	if (!ast_entry)
		return false;

	if (ast_entry->type == CDP_TXRX_AST_TYPE_DA) {
		ast_entry->is_active = TRUE;
		return false;
	}

	da_peer_id = ast_entry->peer_id;
	/* TA peer cannot be same as peer(DA) on which AST is present
	 * this indicates a change in topology and that AST entries
	 * are yet to be updated.
	 */
	if (da_peer_id == ta_txrx_peer->peer_id ||
	    da_peer_id == HTT_INVALID_PEER)
		return false;

	da_peer = dp_txrx_peer_get_ref_by_id(soc, da_peer_id,
					     &txrx_ref_handle, DP_MOD_ID_RX);
	if (!da_peer)
		return false;

	*p_tx_vdev_id = da_peer->vdev->vdev_id;
	/* If the source or destination peer in the isolation
	 * list then dont forward instead push to bridge stack.
	 */
	if (dp_get_peer_isolation(ta_txrx_peer) ||
	    dp_get_peer_isolation(da_peer) ||
	    da_peer->vdev->vdev_id != ta_txrx_peer->vdev->vdev_id) {
		dp_txrx_peer_unref_delete(txrx_ref_handle, DP_MOD_ID_RX);
		return false;
	}

	if (da_peer->bss_peer) {
		dp_txrx_peer_unref_delete(txrx_ref_handle, DP_MOD_ID_RX);
		return false;
	}

	dp_txrx_peer_unref_delete(txrx_ref_handle, DP_MOD_ID_RX);
	return true;
}

/*
 * dp_rx_intrabss_fwd_rh() - Implements the Intra-BSS forwarding logic
 *
 * @soc: core txrx main context
 * @ta_txrx_peer	: source peer entry
 * @rx_tlv_hdr	: start address of rx tlvs
 * @nbuf	: nbuf that has to be intrabss forwarded
 *
 * Return: bool: true if it is forwarded else false
 */
static bool
dp_rx_intrabss_fwd_rh(struct dp_soc *soc,
		      struct dp_txrx_peer *ta_txrx_peer,
		      uint8_t *rx_tlv_hdr,
		      qdf_nbuf_t nbuf,
		      struct hal_rx_msdu_metadata msdu_metadata,
		      struct cdp_tid_rx_stats *tid_stats)
{
	uint8_t tx_vdev_id;

	/* if it is a broadcast pkt (eg: ARP) and it is not its own
	 * source, then clone the pkt and send the cloned pkt for
	 * intra BSS forwarding and original pkt up the network stack
	 * Note: how do we handle multicast pkts. do we forward
	 * all multicast pkts as is or let a higher layer module
	 * like igmpsnoop decide whether to forward or not with
	 * Mcast enhancement.
	 */
	if (qdf_nbuf_is_da_mcbc(nbuf) && !ta_txrx_peer->bss_peer)
		return dp_rx_intrabss_mcbc_fwd(soc, ta_txrx_peer, rx_tlv_hdr,
					       nbuf, tid_stats);

	if (dp_rx_intrabss_eapol_drop_check(soc, ta_txrx_peer, rx_tlv_hdr,
					    nbuf))
		return true;

	if (dp_rx_intrabss_ucast_check_rh(soc, nbuf, ta_txrx_peer,
					  &msdu_metadata, &tx_vdev_id))
		return dp_rx_intrabss_ucast_fwd(soc, ta_txrx_peer, tx_vdev_id,
						rx_tlv_hdr, nbuf, tid_stats);

	return false;
}

QDF_STATUS dp_rx_desc_pool_init_rh(struct dp_soc *soc,
				   struct rx_desc_pool *rx_desc_pool,
				   uint32_t pool_id)
{
	return dp_rx_desc_pool_init_generic(soc, rx_desc_pool, pool_id);
}

void dp_rx_desc_pool_deinit_rh(struct dp_soc *soc,
			       struct rx_desc_pool *rx_desc_pool,
			       uint32_t pool_id)
{
}
