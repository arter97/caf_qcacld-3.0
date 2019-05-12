/*
 * Copyright (c) 2016-2018 The Linux Foundation. All rights reserved.
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

#include "dp_types.h"
#include "dp_rx.h"
#include "dp_peer.h"
#include "hal_rx.h"
#include "hal_api.h"
#include "qdf_nbuf.h"
#ifdef MESH_MODE_SUPPORT
#include "if_meta_hdr.h"
#endif
#include "dp_internal.h"
#include "dp_rx_mon.h"

#ifdef RX_DESC_DEBUG_CHECK
static inline void dp_rx_desc_prep(struct dp_rx_desc *rx_desc, qdf_nbuf_t nbuf)
{
	rx_desc->magic = DP_RX_DESC_MAGIC;
	rx_desc->nbuf = nbuf;
}
#else
static inline void dp_rx_desc_prep(struct dp_rx_desc *rx_desc, qdf_nbuf_t nbuf)
{
	rx_desc->nbuf = nbuf;
}
#endif

#ifdef CONFIG_WIN
static inline bool dp_rx_check_ap_bridge(struct dp_vdev *vdev)
{
	return vdev->ap_bridge_enabled;
}
#else
static inline bool dp_rx_check_ap_bridge(struct dp_vdev *vdev)
{
	if (vdev->opmode != wlan_op_mode_sta)
		return true;
	else
		return false;
}
#endif

/*
 * dp_rx_dump_info_and_assert() - dump RX Ring info and Rx Desc info
 *
 * @soc: core txrx main context
 * @hal_ring: opaque pointer to the HAL Rx Ring, which will be serviced
 * @ring_desc: opaque pointer to the RX ring descriptor
 * @rx_desc: host rs descriptor
 *
 * Return: void
 */
void dp_rx_dump_info_and_assert(struct dp_soc *soc, void *hal_ring,
				void *ring_desc, struct dp_rx_desc *rx_desc)
{
	void *hal_soc = soc->hal_soc;
	dp_rx_desc_dump(rx_desc);
	hal_srng_dump_ring_desc(hal_soc, hal_ring, ring_desc);
	hal_srng_dump_ring(hal_soc, hal_ring);
}

/*
 * dp_rx_buffers_replenish() - replenish rxdma ring with rx nbufs
 *			       called during dp rx initialization
 *			       and at the end of dp_rx_process.
 *
 * @soc: core txrx main context
 * @mac_id: mac_id which is one of 3 mac_ids
 * @dp_rxdma_srng: dp rxdma circular ring
 * @rx_desc_pool: Poiter to free Rx descriptor pool
 * @num_req_buffers: number of buffer to be replenished
 * @desc_list: list of descs if called from dp_rx_process
 *	       or NULL during dp rx initialization or out of buffer
 *	       interrupt.
 * @tail: tail of descs list
 * @owner: who owns the nbuf (host, NSS etc...)
 * Return: return success or failure
 */
QDF_STATUS dp_rx_buffers_replenish(struct dp_soc *dp_soc, uint32_t mac_id,
				struct dp_srng *dp_rxdma_srng,
				struct rx_desc_pool *rx_desc_pool,
				uint32_t num_req_buffers,
				union dp_rx_desc_list_elem_t **desc_list,
				union dp_rx_desc_list_elem_t **tail,
				uint8_t owner)
{
	uint32_t num_alloc_desc;
	uint16_t num_desc_to_free = 0;
	struct dp_pdev *dp_pdev = dp_soc->pdev_list[mac_id];
	uint32_t num_entries_avail;
	uint32_t count;
	int sync_hw_ptr = 1;
	qdf_dma_addr_t paddr;
	qdf_nbuf_t rx_netbuf;
	void *rxdma_ring_entry;
	union dp_rx_desc_list_elem_t *next;
	QDF_STATUS ret;

	void *rxdma_srng;

	rxdma_srng = dp_rxdma_srng->hal_srng;

	if (!rxdma_srng) {
		QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_ERROR,
			"rxdma srng not initialized");
		DP_STATS_INC(dp_pdev, replenish.rxdma_err, num_req_buffers);
		return QDF_STATUS_E_FAILURE;
	}

	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,
		"requested %d buffers for replenish", num_req_buffers);

	hal_srng_access_start(dp_soc->hal_soc, rxdma_srng);
	num_entries_avail = hal_srng_src_num_avail(dp_soc->hal_soc,
						   rxdma_srng,
						   sync_hw_ptr);

	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,
		"no of availble entries in rxdma ring: %d",
		num_entries_avail);

	if (!(*desc_list) && (num_entries_avail >
		((dp_rxdma_srng->num_entries * 3) / 4))) {
		num_req_buffers = num_entries_avail;
	} else if (num_entries_avail < num_req_buffers) {
		num_desc_to_free = num_req_buffers - num_entries_avail;
		num_req_buffers = num_entries_avail;
	}

	if (qdf_unlikely(!num_req_buffers)) {
		num_desc_to_free = num_req_buffers;
		hal_srng_access_end(dp_soc->hal_soc, rxdma_srng);
		goto free_descs;
	}

	/*
	 * if desc_list is NULL, allocate the descs from freelist
	 */
	if (!(*desc_list)) {
		num_alloc_desc = dp_rx_get_free_desc_list(dp_soc, mac_id,
							  rx_desc_pool,
							  num_req_buffers,
							  desc_list,
							  tail);

		if (!num_alloc_desc) {
			QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_ERROR,
				"no free rx_descs in freelist");
			DP_STATS_INC(dp_pdev, err.desc_alloc_fail,
					num_req_buffers);
			hal_srng_access_end(dp_soc->hal_soc, rxdma_srng);
			return QDF_STATUS_E_NOMEM;
		}

		QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,
			"%d rx desc allocated", num_alloc_desc);
		num_req_buffers = num_alloc_desc;
	}


	count = 0;

	while (count < num_req_buffers) {
		rx_netbuf = qdf_nbuf_alloc(dp_soc->osdev,
					RX_BUFFER_SIZE,
					RX_BUFFER_RESERVATION,
					RX_BUFFER_ALIGNMENT,
					FALSE);

		if (rx_netbuf == NULL) {
			DP_STATS_INC(dp_pdev, replenish.nbuf_alloc_fail, 1);
			continue;
		}

		ret = qdf_nbuf_map_single(dp_soc->osdev, rx_netbuf,
				    QDF_DMA_FROM_DEVICE);
		if (qdf_unlikely(QDF_IS_STATUS_ERROR(ret))) {
			qdf_nbuf_free(rx_netbuf);
			DP_STATS_INC(dp_pdev, replenish.map_err, 1);
			continue;
		}

		paddr = qdf_nbuf_get_frag_paddr(rx_netbuf, 0);

		/*
		 * check if the physical address of nbuf->data is
		 * less then 0x50000000 then free the nbuf and try
		 * allocating new nbuf. We can try for 100 times.
		 * this is a temp WAR till we fix it properly.
		 */
		ret = check_x86_paddr(dp_soc, &rx_netbuf, &paddr, dp_pdev);
		if (ret == QDF_STATUS_E_FAILURE) {
			DP_STATS_INC(dp_pdev, replenish.x86_fail, 1);
			break;
		}

		count++;

		rxdma_ring_entry = hal_srng_src_get_next(dp_soc->hal_soc,
								rxdma_srng);
		qdf_assert_always(rxdma_ring_entry);

		next = (*desc_list)->next;

		dp_rx_desc_prep(&((*desc_list)->rx_desc), rx_netbuf);
		(*desc_list)->rx_desc.in_use = 1;

		QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,
				"rx_netbuf=%pK, buf=%pK, paddr=0x%llx, cookie=%d\n",
			rx_netbuf, qdf_nbuf_data(rx_netbuf),
			(unsigned long long)paddr, (*desc_list)->rx_desc.cookie);

		hal_rxdma_buff_addr_info_set(rxdma_ring_entry, paddr,
						(*desc_list)->rx_desc.cookie,
						owner);

		*desc_list = next;
	}

	hal_srng_access_end(dp_soc->hal_soc, rxdma_srng);

	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,
		"successfully replenished %d buffers", num_req_buffers);
	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,
		"%d rx desc added back to free list", num_desc_to_free);

	DP_STATS_INC_PKT(dp_pdev, replenish.pkts, num_req_buffers,
			(RX_BUFFER_SIZE * num_req_buffers));

free_descs:
	DP_STATS_INC(dp_pdev, buf_freelist, num_desc_to_free);
	/*
	 * add any available free desc back to the free list
	 */
	if (*desc_list)
		dp_rx_add_desc_list_to_free_list(dp_soc, desc_list, tail,
			mac_id, rx_desc_pool);

	return QDF_STATUS_SUCCESS;
}

/*
 * dp_rx_deliver_raw() - process RAW mode pkts and hand over the
 *				pkts to RAW mode simulation to
 *				decapsulate the pkt.
 *
 * @vdev: vdev on which RAW mode is enabled
 * @nbuf_list: list of RAW pkts to process
 * @peer: peer object from which the pkt is rx
 *
 * Return: void
 */
void
dp_rx_deliver_raw(struct dp_vdev *vdev, qdf_nbuf_t nbuf_list,
					struct dp_peer *peer)
{
	qdf_nbuf_t deliver_list_head = NULL;
	qdf_nbuf_t deliver_list_tail = NULL;
	qdf_nbuf_t nbuf;

	nbuf = nbuf_list;
	while (nbuf) {
		qdf_nbuf_t next = qdf_nbuf_next(nbuf);

		DP_RX_LIST_APPEND(deliver_list_head, deliver_list_tail, nbuf);

		DP_STATS_INC(vdev->pdev, rx_raw_pkts, 1);
		DP_STATS_INC_PKT(peer, rx.raw, 1, qdf_nbuf_len(nbuf));

		/*
		 * reset the chfrag_start and chfrag_end bits in nbuf cb
		 * as this is a non-amsdu pkt and RAW mode simulation expects
		 * these bit s to be 0 for non-amsdu pkt.
		 */
		if (qdf_nbuf_is_rx_chfrag_start(nbuf) &&
			 qdf_nbuf_is_rx_chfrag_end(nbuf)) {
			qdf_nbuf_set_rx_chfrag_start(nbuf, 0);
			qdf_nbuf_set_rx_chfrag_end(nbuf, 0);
		}

		nbuf = next;
	}

	vdev->osif_rsim_rx_decap(vdev->osif_vdev, &deliver_list_head,
				 &deliver_list_tail, (struct cdp_peer*) peer);

	vdev->osif_rx(vdev->osif_vdev, deliver_list_head);
}


#ifdef DP_LFR
/*
 * In case of LFR, data of a new peer might be sent up
 * even before peer is added.
 */
static inline struct dp_vdev *
dp_get_vdev_from_peer(struct dp_soc *soc,
			uint16_t peer_id,
			struct dp_peer *peer,
			struct hal_rx_mpdu_desc_info mpdu_desc_info)
{
	struct dp_vdev *vdev;
	uint8_t vdev_id;

	if (unlikely(!peer)) {
		if (peer_id != HTT_INVALID_PEER) {
			vdev_id = DP_PEER_METADATA_ID_GET(
					mpdu_desc_info.peer_meta_data);
			QDF_TRACE(QDF_MODULE_ID_DP,
				QDF_TRACE_LEVEL_DEBUG,
				FL("PeerID %d not found use vdevID %d"),
				peer_id, vdev_id);
			vdev = dp_get_vdev_from_soc_vdev_id_wifi3(soc,
							vdev_id);
		} else {
			QDF_TRACE(QDF_MODULE_ID_DP,
				QDF_TRACE_LEVEL_DEBUG,
				FL("Invalid PeerID %d"),
				peer_id);
			return NULL;
		}
	} else {
		vdev = peer->vdev;
	}
	return vdev;
}
#else
static inline struct dp_vdev *
dp_get_vdev_from_peer(struct dp_soc *soc,
			uint16_t peer_id,
			struct dp_peer *peer,
			struct hal_rx_mpdu_desc_info mpdu_desc_info)
{
	if (unlikely(!peer)) {
		QDF_TRACE(QDF_MODULE_ID_DP,
			QDF_TRACE_LEVEL_DEBUG,
			FL("Peer not found for peerID %d"),
			peer_id);
		return NULL;
	} else {
		return peer->vdev;
	}
}
#endif

/**
 * dp_rx_da_learn() - Add AST entry based on DA lookup
 *			This is a WAR for HK 1.0 and will
 *			be removed in HK 2.0
 *
 * @soc: core txrx main context
 * @rx_tlv_hdr	: start address of rx tlvs
 * @sa_peer	: source peer entry
 * @nbuf	: nbuf to retrive destination mac for which AST will be added
 *
 */
static void
dp_rx_da_learn(struct dp_soc *soc,
		uint8_t *rx_tlv_hdr,
		struct dp_peer *ta_peer,
		qdf_nbuf_t nbuf)
{
	struct dp_peer *peer = NULL;

	if (unlikely(!ta_peer))
		return;

	if (ta_peer->vdev->opmode != wlan_op_mode_ap)
		return;

	if (!soc->enable_da)
		return;

	if (qdf_unlikely(!qdf_nbuf_is_da_valid(nbuf) &&
			 !qdf_nbuf_is_da_mcbc(nbuf))) {
		/* find peer in table by da */
		peer = dp_peer_find_hash_find(soc, qdf_nbuf_data(nbuf), 0, DP_VDEV_ALL);
		if (peer) {
			QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_INFO_HIGH,
				  "Found peer with DA %s, return without adding\n",
				  ether_sprintf(qdf_nbuf_data(nbuf)));
			return;
		}
		dp_peer_add_ast(soc,
				ta_peer->vdev->vap_bss_peer,
				qdf_nbuf_data(nbuf),
				CDP_TXRX_AST_TYPE_DA,
				IEEE80211_NODE_F_WDS_HM);
	}
}


/**
 * dp_rx_intrabss_fwd() - Implements the Intra-BSS forwarding logic
 *
 * @soc: core txrx main context
 * @sa_peer	: source peer entry
 * @rx_tlv_hdr	: start address of rx tlvs
 * @nbuf	: nbuf that has to be intrabss forwarded
 *
 * Return: bool: true if it is forwarded else false
 */
static bool
dp_rx_intrabss_fwd(struct dp_soc *soc,
			struct dp_peer *sa_peer,
			uint8_t *rx_tlv_hdr,
			qdf_nbuf_t nbuf)
{
	uint16_t da_idx;
	uint16_t len;
	struct dp_peer *da_peer;
	struct dp_ast_entry *ast_entry;
	qdf_nbuf_t nbuf_copy;

	/* check if the destination peer is available in peer table
	 * and also check if the source peer and destination peer
	 * belong to the same vap and destination peer is not bss peer.
	 */

	if ((qdf_nbuf_is_da_valid(nbuf) &&
	   !qdf_nbuf_is_da_mcbc(nbuf))) {
		da_idx = hal_rx_msdu_end_da_idx_get(rx_tlv_hdr);

		ast_entry = soc->ast_table[da_idx];
		if (!ast_entry)
			return false;

		if (ast_entry->type == CDP_TXRX_AST_TYPE_DA) {
			ast_entry->is_active = TRUE;
			return false;
		}

		da_peer = ast_entry->peer;

		if (!da_peer)
			return false;

		if (da_peer->vdev == sa_peer->vdev && !da_peer->bss_peer) {
			memset(nbuf->cb, 0x0, sizeof(nbuf->cb));
			len = qdf_nbuf_len(nbuf);

			/* linearize the nbuf just before we send to
			 * dp_tx_send()
			 */
			if (qdf_unlikely(qdf_nbuf_is_nonlinear(nbuf))) {
				if (qdf_nbuf_linearize(nbuf) == -ENOMEM)
					return false;

				nbuf = qdf_nbuf_unshare(nbuf);
				if (!nbuf) {
					DP_STATS_INC_PKT(sa_peer,
							 rx.intra_bss.fail,
							 1,
							 len);
					/* return true even though the pkt is
					 * not forwarded. Basically skb_unshare
					 * failed and we want to continue with
					 * next nbuf.
					 */
					return true;
				}
			}

			if (!dp_tx_send(sa_peer->vdev, nbuf)) {
				DP_STATS_INC_PKT(sa_peer, rx.intra_bss.pkts,
						1, len);
				return true;
			} else {
				DP_STATS_INC_PKT(sa_peer, rx.intra_bss.fail, 1,
						len);
				return false;
			}
		}
	}
	/* if it is a broadcast pkt (eg: ARP) and it is not its own
	 * source, then clone the pkt and send the cloned pkt for
	 * intra BSS forwarding and original pkt up the network stack
	 * Note: how do we handle multicast pkts. do we forward
	 * all multicast pkts as is or let a higher layer module
	 * like igmpsnoop decide whether to forward or not with
	 * Mcast enhancement.
	 */
	else if (qdf_unlikely((qdf_nbuf_is_da_mcbc(nbuf) &&
		!sa_peer->bss_peer))) {
		nbuf_copy = qdf_nbuf_copy(nbuf);
		if (!nbuf_copy)
			return false;
		memset(nbuf_copy->cb, 0x0, sizeof(nbuf_copy->cb));
		len = qdf_nbuf_len(nbuf_copy);

		if (dp_tx_send(sa_peer->vdev, nbuf_copy)) {
			DP_STATS_INC_PKT(sa_peer, rx.intra_bss.fail, 1, len);
			qdf_nbuf_free(nbuf_copy);
		} else
			DP_STATS_INC_PKT(sa_peer, rx.intra_bss.pkts, 1, len);
	}
	/* return false as we have to still send the original pkt
	 * up the stack
	 */
	return false;
}

#ifdef MESH_MODE_SUPPORT

/**
 * dp_rx_fill_mesh_stats() - Fills the mesh per packet receive stats
 *
 * @vdev: DP Virtual device handle
 * @nbuf: Buffer pointer
 * @rx_tlv_hdr: start of rx tlv header
 * @peer: pointer to peer
 *
 * This function allocated memory for mesh receive stats and fill the
 * required stats. Stores the memory address in skb cb.
 *
 * Return: void
 */

void dp_rx_fill_mesh_stats(struct dp_vdev *vdev, qdf_nbuf_t nbuf,
				uint8_t *rx_tlv_hdr, struct dp_peer *peer)
{
	struct mesh_recv_hdr_s *rx_info = NULL;
	uint32_t pkt_type;
	uint32_t nss;
	uint32_t rate_mcs;
	uint32_t bw;

	/* fill recv mesh stats */
	rx_info = qdf_mem_malloc(sizeof(struct mesh_recv_hdr_s));

	/* upper layers are resposible to free this memory */

	if (rx_info == NULL) {
		QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_ERROR,
			"Memory allocation failed for mesh rx stats");
		DP_STATS_INC(vdev->pdev, mesh_mem_alloc, 1);
		return;
	}

	rx_info->rs_flags = MESH_RXHDR_VER1;
	if (qdf_nbuf_is_rx_chfrag_start(nbuf))
		rx_info->rs_flags |= MESH_RX_FIRST_MSDU;

	if (qdf_nbuf_is_rx_chfrag_end(nbuf))
		rx_info->rs_flags |= MESH_RX_LAST_MSDU;

	if (hal_rx_attn_msdu_get_is_decrypted(rx_tlv_hdr)) {
		rx_info->rs_flags |= MESH_RX_DECRYPTED;
		rx_info->rs_keyix = hal_rx_msdu_get_keyid(rx_tlv_hdr);
		if (vdev->osif_get_key)
			vdev->osif_get_key(vdev->osif_vdev,
					&rx_info->rs_decryptkey[0],
					&peer->mac_addr.raw[0],
					rx_info->rs_keyix);
	}

	rx_info->rs_rssi = hal_rx_msdu_start_get_rssi(rx_tlv_hdr);
	rx_info->rs_channel = hal_rx_msdu_start_get_freq(rx_tlv_hdr);
	pkt_type = hal_rx_msdu_start_get_pkt_type(rx_tlv_hdr);
	rate_mcs = hal_rx_msdu_start_rate_mcs_get(rx_tlv_hdr);
	bw = hal_rx_msdu_start_bw_get(rx_tlv_hdr);
	nss = hal_rx_msdu_start_nss_get(rx_tlv_hdr);
	rx_info->rs_ratephy1 = rate_mcs | (nss << 0x8) | (pkt_type << 16) |
				(bw << 24);

	qdf_nbuf_set_rx_fctx_type(nbuf, (void *)rx_info, CB_FTYPE_MESH_RX_INFO);

	QDF_TRACE(QDF_MODULE_ID_TXRX, QDF_TRACE_LEVEL_INFO_MED,
		FL("Mesh rx stats: flags %x, rssi %x, chn %x, rate %x, kix %x"),
						rx_info->rs_flags,
						rx_info->rs_rssi,
						rx_info->rs_channel,
						rx_info->rs_ratephy1,
						rx_info->rs_keyix);

}

/**
 * dp_rx_filter_mesh_packets() - Filters mesh unwanted packets
 *
 * @vdev: DP Virtual device handle
 * @nbuf: Buffer pointer
 * @rx_tlv_hdr: start of rx tlv header
 *
 * This checks if the received packet is matching any filter out
 * catogery and and drop the packet if it matches.
 *
 * Return: status(0 indicates drop, 1 indicate to no drop)
 */

QDF_STATUS dp_rx_filter_mesh_packets(struct dp_vdev *vdev, qdf_nbuf_t nbuf,
					uint8_t *rx_tlv_hdr)
{
	union dp_align_mac_addr mac_addr;

	if (qdf_unlikely(vdev->mesh_rx_filter)) {
		if (vdev->mesh_rx_filter & MESH_FILTER_OUT_FROMDS)
			if (hal_rx_mpdu_get_fr_ds(rx_tlv_hdr))
				return  QDF_STATUS_SUCCESS;

		if (vdev->mesh_rx_filter & MESH_FILTER_OUT_TODS)
			if (hal_rx_mpdu_get_to_ds(rx_tlv_hdr))
				return  QDF_STATUS_SUCCESS;

		if (vdev->mesh_rx_filter & MESH_FILTER_OUT_NODS)
			if (!hal_rx_mpdu_get_fr_ds(rx_tlv_hdr)
				&& !hal_rx_mpdu_get_to_ds(rx_tlv_hdr))
				return  QDF_STATUS_SUCCESS;

		if (vdev->mesh_rx_filter & MESH_FILTER_OUT_RA) {
			if (hal_rx_mpdu_get_addr1(rx_tlv_hdr,
					&mac_addr.raw[0]))
				return QDF_STATUS_E_FAILURE;

			if (!qdf_mem_cmp(&mac_addr.raw[0],
					&vdev->mac_addr.raw[0],
					DP_MAC_ADDR_LEN))
				return  QDF_STATUS_SUCCESS;
		}

		if (vdev->mesh_rx_filter & MESH_FILTER_OUT_TA) {
			if (hal_rx_mpdu_get_addr2(rx_tlv_hdr,
					&mac_addr.raw[0]))
				return QDF_STATUS_E_FAILURE;

			if (!qdf_mem_cmp(&mac_addr.raw[0],
					&vdev->mac_addr.raw[0],
					DP_MAC_ADDR_LEN))
				return  QDF_STATUS_SUCCESS;
		}
	}

	return QDF_STATUS_E_FAILURE;
}

#else
void dp_rx_fill_mesh_stats(struct dp_vdev *vdev, qdf_nbuf_t nbuf,
				uint8_t *rx_tlv_hdr, struct dp_peer *peer)
{
}

QDF_STATUS dp_rx_filter_mesh_packets(struct dp_vdev *vdev, qdf_nbuf_t nbuf,
					uint8_t *rx_tlv_hdr)
{
	return QDF_STATUS_E_FAILURE;
}

#endif

#ifdef CONFIG_WIN
/**
 * dp_rx_nac_filter(): Function to perform filtering of non-associated
 * clients
 * @pdev: DP pdev handle
 * @rx_pkt_hdr: Rx packet Header
 *
 * return: dp_vdev*
 */
static
struct dp_vdev *dp_rx_nac_filter(struct dp_pdev *pdev,
		uint8_t *rx_pkt_hdr)
{
	struct ieee80211_frame *wh;
	struct dp_neighbour_peer *peer = NULL;

	wh = (struct ieee80211_frame *)rx_pkt_hdr;

	if ((wh->i_fc[1] & IEEE80211_FC1_DIR_MASK) != IEEE80211_FC1_DIR_TODS)
		return NULL;

	qdf_spin_lock_bh(&pdev->neighbour_peer_mutex);
	TAILQ_FOREACH(peer, &pdev->neighbour_peers_list,
				neighbour_peer_list_elem) {
		if (qdf_mem_cmp(&peer->neighbour_peers_macaddr.raw[0],
				wh->i_addr2, DP_MAC_ADDR_LEN) == 0) {
			QDF_TRACE(
				QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,
				FL("NAC configuration matched for mac-%2x:%2x:%2x:%2x:%2x:%2x"),
				peer->neighbour_peers_macaddr.raw[0],
				peer->neighbour_peers_macaddr.raw[1],
				peer->neighbour_peers_macaddr.raw[2],
				peer->neighbour_peers_macaddr.raw[3],
				peer->neighbour_peers_macaddr.raw[4],
				peer->neighbour_peers_macaddr.raw[5]);

				qdf_spin_unlock_bh(&pdev->neighbour_peer_mutex);

			return pdev->monitor_vdev;
		}
	}
	qdf_spin_unlock_bh(&pdev->neighbour_peer_mutex);

	return NULL;
}

/**
 * dp_rx_process_invalid_peer(): Function to pass invalid peer list to umac
 * @soc: DP SOC handle
 * @mpdu: mpdu for which peer is invalid
 *
 * return: integer type
 */
uint8_t dp_rx_process_invalid_peer(struct dp_soc *soc, qdf_nbuf_t mpdu)
{
	struct dp_invalid_peer_msg msg;
	struct dp_vdev *vdev = NULL;
	struct dp_pdev *pdev = NULL;
	struct ieee80211_frame *wh;
	uint8_t i;
	uint8_t *rx_pkt_hdr;
	qdf_nbuf_t curr_nbuf, next_nbuf;

	rx_pkt_hdr = hal_rx_pkt_hdr_get(qdf_nbuf_data(mpdu));
	wh = (struct ieee80211_frame *)rx_pkt_hdr;

	if (!DP_FRAME_IS_DATA(wh)) {
		QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,
				"NAWDS valid only for data frames");
		goto free;
	}

	if (qdf_nbuf_len(mpdu) < sizeof(struct ieee80211_frame)) {
		QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_ERROR,
				"Invalid nbuf length");
		goto free;
	}


	for (i = 0; i < MAX_PDEV_CNT; i++) {
		pdev = soc->pdev_list[i];
		if (!pdev) {
			QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_ERROR,
					"PDEV not found");
			continue;
		}

		if (pdev->filter_neighbour_peers) {
			/* Next Hop scenario not yet handle */
			vdev = dp_rx_nac_filter(pdev, rx_pkt_hdr);
			if (vdev) {
				dp_rx_mon_deliver(soc, i,
						pdev->invalid_peer_head_msdu,
						pdev->invalid_peer_tail_msdu);

				pdev->invalid_peer_head_msdu = NULL;
				pdev->invalid_peer_tail_msdu = NULL;

				return 0;
			}
		}
		TAILQ_FOREACH(vdev, &pdev->vdev_list, vdev_list_elem) {
			if (qdf_mem_cmp(wh->i_addr1, vdev->mac_addr.raw,
						DP_MAC_ADDR_LEN) == 0) {
				goto out;
			}
		}
	}

	if (!vdev) {
		QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_ERROR,
				"VDEV not found");
		goto free;
	}

out:
	msg.wh = wh;
	qdf_nbuf_pull_head(mpdu, RX_PKT_TLVS_LEN);
	msg.nbuf = mpdu;
	msg.vdev_id = vdev->vdev_id;
	if (pdev->soc->cdp_soc.ol_ops->rx_invalid_peer)
		pdev->soc->cdp_soc.ol_ops->rx_invalid_peer(pdev->osif_pdev, &msg);

free:
	/* Drop and free packet */
	curr_nbuf = mpdu;
	while (curr_nbuf) {
		next_nbuf = qdf_nbuf_next(curr_nbuf);
		qdf_nbuf_free(curr_nbuf);
		curr_nbuf = next_nbuf;
	}

	return 0;
}

/**
 * dp_rx_process_invalid_peer_wrapper(): Function to wrap invalid peer handler
 * @soc: DP SOC handle
 * @mpdu: mpdu for which peer is invalid
 * @mpdu_done: if an mpdu is completed
 *
 * return: integer type
 */
void dp_rx_process_invalid_peer_wrapper(struct dp_soc *soc,
					qdf_nbuf_t mpdu, bool mpdu_done)
{
	/* Only trigger the process when mpdu is completed */
	if (mpdu_done)
		dp_rx_process_invalid_peer(soc, mpdu);
}
#else
uint8_t dp_rx_process_invalid_peer(struct dp_soc *soc, qdf_nbuf_t mpdu)
{
	qdf_nbuf_t curr_nbuf, next_nbuf;
	struct dp_pdev *pdev;
	uint8_t i;

	curr_nbuf = mpdu;
	while (curr_nbuf) {
		next_nbuf = qdf_nbuf_next(curr_nbuf);
		/* Drop and free packet */
		DP_STATS_INC_PKT(soc, rx.err.rx_invalid_peer, 1,
				qdf_nbuf_len(curr_nbuf));
		qdf_nbuf_free(curr_nbuf);
		curr_nbuf = next_nbuf;
	}

	/* reset the head and tail pointers */
	for (i = 0; i < MAX_PDEV_CNT; i++) {
		pdev = soc->pdev_list[i];
		if (!pdev) {
			QDF_TRACE(QDF_MODULE_ID_DP,
				QDF_TRACE_LEVEL_ERROR,
				"PDEV not found");
			continue;
		}

		pdev->invalid_peer_head_msdu = NULL;
		pdev->invalid_peer_tail_msdu = NULL;
	}
	return 0;
}

void dp_rx_process_invalid_peer_wrapper(struct dp_soc *soc,
					qdf_nbuf_t mpdu, bool mpdu_done)
{
	/* To avoid compiler warning */
	mpdu_done = mpdu_done;

	/* Process the nbuf */
	dp_rx_process_invalid_peer(soc, mpdu);
}
#endif

#if defined(FEATURE_LRO)
static void dp_rx_print_lro_info(uint8_t *rx_tlv)
{
	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,
	FL("----------------------RX DESC LRO----------------------\n"));
	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,
		FL("lro_eligible 0x%x"), HAL_RX_TLV_GET_LRO_ELIGIBLE(rx_tlv));
	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,
		FL("pure_ack 0x%x"), HAL_RX_TLV_GET_TCP_PURE_ACK(rx_tlv));
	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,
		FL("chksum 0x%x"), HAL_RX_TLV_GET_TCP_CHKSUM(rx_tlv));
	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,
		FL("TCP seq num 0x%x"), HAL_RX_TLV_GET_TCP_SEQ(rx_tlv));
	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,
		FL("TCP ack num 0x%x"), HAL_RX_TLV_GET_TCP_ACK(rx_tlv));
	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,
		FL("TCP window 0x%x"), HAL_RX_TLV_GET_TCP_WIN(rx_tlv));
	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,
		FL("TCP protocol 0x%x"), HAL_RX_TLV_GET_TCP_PROTO(rx_tlv));
	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,
		FL("TCP offset 0x%x"), HAL_RX_TLV_GET_TCP_OFFSET(rx_tlv));
	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,
		FL("toeplitz 0x%x"), HAL_RX_TLV_GET_FLOW_ID_TOEPLITZ(rx_tlv));
	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,
	FL("---------------------------------------------------------\n"));
}

/**
 * dp_rx_lro() - LRO related processing
 * @rx_tlv: TLV data extracted from the rx packet
 * @peer: destination peer of the msdu
 * @msdu: network buffer
 * @ctx: LRO context
 *
 * This function performs the LRO related processing of the msdu
 *
 * Return: true: LRO enabled false: LRO is not enabled
 */
static void dp_rx_lro(uint8_t *rx_tlv, struct dp_peer *peer,
	 qdf_nbuf_t msdu, qdf_lro_ctx_t ctx)
{
	if (!peer || !peer->vdev || !peer->vdev->lro_enable) {
		QDF_TRACE(QDF_MODULE_ID_TXRX, QDF_TRACE_LEVEL_DEBUG,
			 FL("no peer, no vdev or LRO disabled"));
		QDF_NBUF_CB_RX_LRO_ELIGIBLE(msdu) = 0;
		return;
	}
	qdf_assert(rx_tlv);
	dp_rx_print_lro_info(rx_tlv);

	QDF_NBUF_CB_RX_LRO_ELIGIBLE(msdu) =
		 HAL_RX_TLV_GET_LRO_ELIGIBLE(rx_tlv);

	QDF_NBUF_CB_RX_TCP_PURE_ACK(msdu) =
			HAL_RX_TLV_GET_TCP_PURE_ACK(rx_tlv);

	QDF_NBUF_CB_RX_TCP_CHKSUM(msdu) =
			 HAL_RX_TLV_GET_TCP_CHKSUM(rx_tlv);
	QDF_NBUF_CB_RX_TCP_SEQ_NUM(msdu) =
			 HAL_RX_TLV_GET_TCP_SEQ(rx_tlv);
	QDF_NBUF_CB_RX_TCP_ACK_NUM(msdu) =
			 HAL_RX_TLV_GET_TCP_ACK(rx_tlv);
	QDF_NBUF_CB_RX_TCP_WIN(msdu) =
			 HAL_RX_TLV_GET_TCP_WIN(rx_tlv);
	QDF_NBUF_CB_RX_TCP_PROTO(msdu) =
			 HAL_RX_TLV_GET_TCP_PROTO(rx_tlv);
	QDF_NBUF_CB_RX_IPV6_PROTO(msdu) =
			 HAL_RX_TLV_GET_IPV6(rx_tlv);
	QDF_NBUF_CB_RX_TCP_OFFSET(msdu) =
			 HAL_RX_TLV_GET_TCP_OFFSET(rx_tlv);
	QDF_NBUF_CB_RX_FLOW_ID(msdu) =
			 HAL_RX_TLV_GET_FLOW_ID_TOEPLITZ(rx_tlv);
	QDF_NBUF_CB_RX_LRO_CTX(msdu) = (unsigned char *)ctx;

}
#else
static void dp_rx_lro(uint8_t *rx_tlv, struct dp_peer *peer,
	 qdf_nbuf_t msdu, qdf_lro_ctx_t ctx)
{
}
#endif

/*
 * dp_rx_adjust_nbuf_len() - set appropriate msdu length in nbuf.
 *
 * @soc: core txrx main context
 * @nbuf: pointer to msdu.
 * @mpdu_len: mpdu length
 *
 * Return: returns true if nbuf is last msdu of mpdu else retuns false.
 */
static inline bool dp_rx_adjust_nbuf_len(struct dp_soc *soc, qdf_nbuf_t nbuf,
					 uint16_t *mpdu_len)
{
	bool last_nbuf;
	uint16_t length;

	if (*mpdu_len >= (RX_BUFFER_SIZE - RX_PKT_TLVS_LEN)) {
		length = RX_BUFFER_SIZE;
		last_nbuf = false;
	} else {
		length = *mpdu_len + RX_PKT_TLVS_LEN;
		last_nbuf = true;
	}

	qdf_nbuf_unmap_nbytes_single(soc->osdev, nbuf, QDF_DMA_FROM_DEVICE,
				     length);
	qdf_nbuf_set_pktlen(nbuf, length);

	*mpdu_len -= (RX_BUFFER_SIZE - RX_PKT_TLVS_LEN);

	return last_nbuf;
}

/**
 * dp_rx_sg_create() - create a frag_list for MSDUs which are spread across
 *		     multiple nbufs.
 *
 * @soc: core txrx main context
 * @nbuf: pointer to the first msdu of an amsdu.
 * @mpdu_len: total length of mpdu
 *
 * This function implements the creation of RX frag_list for cases
 * where an MSDU is spread across multiple nbufs.
 *
 * Return: returns the head nbuf which contains complete frag_list.
 */
qdf_nbuf_t dp_rx_sg_create(struct dp_soc *soc, qdf_nbuf_t nbuf,
			  uint16_t mpdu_len)
{
	qdf_nbuf_t parent, next, frag_list;
	uint16_t frag_list_len = 0;
	bool last_nbuf;

	/*
	 * this is a case where the complete msdu fits in one single nbuf.
	 * in this case HW sets both start and end bit and we only need to
	 * reset these bits for RAW mode simulator to decap the pkt
	 */
	if (qdf_nbuf_is_rx_chfrag_start(nbuf) &&
					qdf_nbuf_is_rx_chfrag_end(nbuf)) {
		qdf_nbuf_unmap_nbytes_single(soc->osdev, nbuf, QDF_DMA_FROM_DEVICE,
					     mpdu_len + RX_PKT_TLVS_LEN);
		qdf_nbuf_set_pktlen (nbuf, mpdu_len + RX_PKT_TLVS_LEN);
		return nbuf;
	}

	/*
	 * This is a case where we have multiple msdus (A-MSDU) spread across
	 * multiple nbufs. here we create a fraglist out of these nbufs.
	 *
	 * the moment we encounter a nbuf with continuation bit set we
	 * know for sure we have an MSDU which is spread across multiple
	 * nbufs. We loop through and reap nbufs till we reach last nbuf.
	 */
	parent = nbuf;
	frag_list = nbuf->next;
	nbuf = nbuf->next;

	/*
	 * set the start bit in the first nbuf we encounter with continuation
	 * bit set. This has the proper mpdu length set as it is the first
	 * msdu of the mpdu. this becomes the parent nbuf and the subsequent
	 * nbufs will form the frag_list of the parent nbuf.
	 */
	qdf_nbuf_set_rx_chfrag_start(parent, 1);
	last_nbuf = dp_rx_adjust_nbuf_len(soc, parent, &mpdu_len);

	/*
	 * this is where we set the length of the fragments which are
	 * associated to the parent nbuf. We iterate through the frag_list
	 * till we hit the last_nbuf of the list.
	 */
	do {
		last_nbuf = dp_rx_adjust_nbuf_len(soc, nbuf, &mpdu_len);
		qdf_nbuf_pull_head(nbuf, RX_PKT_TLVS_LEN);
		frag_list_len += qdf_nbuf_len(nbuf);

		if (last_nbuf) {
			next = nbuf->next;
			nbuf->next = NULL;
			break;
		}

		nbuf = nbuf->next;
	} while (!last_nbuf);

	qdf_nbuf_set_rx_chfrag_start(nbuf, 0);
	qdf_nbuf_append_ext_list(parent, frag_list, frag_list_len);
	parent->next = next;

	return parent;
}

static inline void dp_rx_deliver_to_stack(struct dp_vdev *vdev,
						struct dp_peer *peer,
						qdf_nbuf_t nbuf_head,
						qdf_nbuf_t nbuf_tail)
{
	/*
	 * highly unlikely to have a vdev without a registerd rx
	 * callback function. if so let us free the nbuf_list.
	 */
	if (qdf_unlikely(!vdev->osif_rx)) {
		qdf_nbuf_t nbuf;
		do {
			nbuf = nbuf_head;
			nbuf_head = nbuf_head->next;
			qdf_nbuf_free(nbuf);
		} while (nbuf_head);

		return;
	}

	if (qdf_unlikely(vdev->rx_decap_type == htt_cmn_pkt_type_raw) ||
			(vdev->rx_decap_type == htt_cmn_pkt_type_native_wifi)) {
		vdev->osif_rsim_rx_decap(vdev->osif_vdev, &nbuf_head,
				&nbuf_tail, (struct cdp_peer *) peer);
	}

	vdev->osif_rx(vdev->osif_vdev, nbuf_head);

}

/**
 * dp_rx_cksum_offload() - set the nbuf checksum as defined by hardware.
 * @nbuf: pointer to the first msdu of an amsdu.
 * @rx_tlv_hdr: pointer to the start of RX TLV headers.
 *
 * The ipsumed field of the skb is set based on whether HW validated the
 * IP/TCP/UDP checksum.
 *
 * Return: void
 */
static inline void dp_rx_cksum_offload(struct dp_pdev *pdev,
				       qdf_nbuf_t nbuf,
				       uint8_t *rx_tlv_hdr)
{
	qdf_nbuf_rx_cksum_t cksum = {0};
	bool ip_csum_err = hal_rx_attn_ip_cksum_fail_get(rx_tlv_hdr);
	bool tcp_udp_csum_er = hal_rx_attn_tcp_udp_cksum_fail_get(rx_tlv_hdr);

	if (qdf_likely(!ip_csum_err && !tcp_udp_csum_er)) {
		cksum.l4_result = QDF_NBUF_RX_CKSUM_TCP_UDP_UNNECESSARY;
		qdf_nbuf_set_rx_cksum(nbuf, &cksum);
	} else {
		DP_STATS_INCC(pdev, err.ip_csum_err, 1, ip_csum_err);
		DP_STATS_INCC(pdev, err.tcp_udp_csum_err, 1, tcp_udp_csum_er);
	}
}

/**
 * dp_rx_msdu_stats_update() - update per msdu stats.
 * @soc: core txrx main context
 * @nbuf: pointer to the first msdu of an amsdu.
 * @rx_tlv_hdr: pointer to the start of RX TLV headers.
 * @peer: pointer to the peer object.
 * ring_id: reo dest ring number on which pkt is reaped.
 *
 * update all the per msdu stats for that nbuf.
 * Return: void
 */
static inline void dp_rx_msdu_stats_update(struct dp_soc *soc,
						qdf_nbuf_t nbuf,
						uint8_t *rx_tlv_hdr,
						struct dp_peer *peer,
						uint8_t ring_id)
{
	bool is_ampdu, is_not_amsdu;
	uint16_t peer_id;
	uint32_t sgi, mcs, tid, nss, bw, reception_type, pkt_type;
	struct dp_vdev *vdev = peer->vdev;
	struct ether_header *eh;
	uint16_t msdu_len = qdf_nbuf_len(nbuf);

	is_not_amsdu = qdf_nbuf_is_rx_chfrag_start(nbuf) &
			qdf_nbuf_is_rx_chfrag_end(nbuf);

	DP_STATS_INC_PKT(peer, rx.rcvd_reo[ring_id], 1, msdu_len);
	DP_STATS_INCC(peer, rx.non_amsdu_cnt, 1, is_not_amsdu);
	DP_STATS_INCC(peer, rx.amsdu_cnt, 1, !is_not_amsdu);

	if (qdf_unlikely(qdf_nbuf_is_da_mcbc(nbuf) &&
			 (vdev->rx_decap_type == htt_cmn_pkt_type_ethernet))) {
		eh = (struct ether_header *)qdf_nbuf_data(nbuf);
		DP_STATS_INC_PKT(peer, rx.multicast, 1, msdu_len);
		if (IEEE80211_IS_BROADCAST(eh->ether_dhost)) {
			DP_STATS_INC_PKT(peer, rx.bcast, 1, msdu_len);

		}
	}

	/* currently we can return from here as we have similar stats
	 * updated at per ppdu level instead of msdu level */
	if (qdf_likely(!soc->process_rx_status))
		return;

	is_ampdu = hal_rx_mpdu_info_ampdu_flag_get(rx_tlv_hdr);
	DP_STATS_INCC(peer, rx.ampdu_cnt, 1, is_ampdu);
	DP_STATS_INCC(peer, rx.non_ampdu_cnt, 1, !(is_ampdu));

	sgi = hal_rx_msdu_start_sgi_get(rx_tlv_hdr);
	mcs = hal_rx_msdu_start_rate_mcs_get(rx_tlv_hdr);
	tid = hal_rx_mpdu_start_tid_get(rx_tlv_hdr);
	bw = hal_rx_msdu_start_bw_get(rx_tlv_hdr);
	reception_type = hal_rx_msdu_start_reception_type_get(rx_tlv_hdr);
	nss = hal_rx_msdu_start_nss_get(rx_tlv_hdr);
	pkt_type = hal_rx_msdu_start_get_pkt_type(rx_tlv_hdr);

	DP_STATS_INC(vdev->pdev, rx.bw[bw], 1);
	DP_STATS_INC(vdev->pdev, rx.reception_type[reception_type], 1);
	DP_STATS_INCC(vdev->pdev, rx.nss[nss], 1,
			((reception_type == REPT_MU_MIMO) ||
			 (reception_type == REPT_MU_OFDMA_MIMO))
			);
	DP_STATS_INC(peer, rx.sgi_count[sgi], 1);
	DP_STATS_INCC(peer, rx.err.mic_err, 1,
			hal_rx_mpdu_end_mic_err_get(rx_tlv_hdr));
	DP_STATS_INCC(peer, rx.err.decrypt_err, 1,
			hal_rx_mpdu_end_decrypt_err_get(rx_tlv_hdr));

	DP_STATS_INC(peer, rx.wme_ac_type[TID_TO_WME_AC(tid)], 1);
	DP_STATS_INC(peer, rx.bw[bw], 1);
	DP_STATS_INC(peer, rx.reception_type[reception_type], 1);

	DP_STATS_INCC(peer, rx.pkt_type[pkt_type].
			mcs_count[MAX_MCS-1], 1,
			((mcs >= MAX_MCS_11A) && (pkt_type
				== DOT11_A)));
	DP_STATS_INCC(peer, rx.pkt_type[pkt_type].
			mcs_count[mcs], 1,
			((mcs <= MAX_MCS_11A) && (pkt_type
				== DOT11_A)));
	DP_STATS_INCC(peer, rx.pkt_type[pkt_type].
			mcs_count[MAX_MCS-1], 1,
			((mcs >= MAX_MCS_11B)
			 && (pkt_type == DOT11_B)));
	DP_STATS_INCC(peer, rx.pkt_type[pkt_type].
			mcs_count[mcs], 1,
			((mcs <= MAX_MCS_11B)
			 && (pkt_type == DOT11_B)));
	DP_STATS_INCC(peer, rx.pkt_type[pkt_type].
			mcs_count[MAX_MCS-1], 1,
			((mcs >= MAX_MCS_11A)
			 && (pkt_type == DOT11_N)));
	DP_STATS_INCC(peer, rx.pkt_type[pkt_type].
			mcs_count[mcs], 1,
			((mcs <= MAX_MCS_11A)
			 && (pkt_type == DOT11_N)));
	DP_STATS_INCC(peer, rx.pkt_type[pkt_type].
			mcs_count[MAX_MCS-1], 1,
			((mcs >= MAX_MCS_11AC)
			 && (pkt_type == DOT11_AC)));
	DP_STATS_INCC(peer, rx.pkt_type[pkt_type].
			mcs_count[mcs], 1,
			((mcs <= MAX_MCS_11AC)
			 && (pkt_type == DOT11_AC)));
	DP_STATS_INCC(peer, rx.pkt_type[pkt_type].
			mcs_count[MAX_MCS-1], 1,
			((mcs >= MAX_MCS)
			 && (pkt_type == DOT11_AX)));
	DP_STATS_INCC(peer, rx.pkt_type[pkt_type].
			mcs_count[mcs], 1,
			((mcs < MAX_MCS)
			 && (pkt_type == DOT11_AX)));

	if (!vdev->pdev || !vdev->pdev->osif_pdev)
		return;

	if ((soc->process_rx_status) &&
		hal_rx_attn_first_mpdu_get(rx_tlv_hdr)) {

		peer_id = peer->peer_ids[0];

		if (soc->cdp_soc.ol_ops &&
				soc->cdp_soc.ol_ops->update_dp_stats) {
			soc->cdp_soc.ol_ops->update_dp_stats(
					vdev->pdev->osif_pdev,
					&peer->stats,
					peer_id,
					UPDATE_PEER_STATS);
		}
	}
}

#ifdef WDS_VENDOR_EXTENSION
int dp_wds_rx_policy_check(
		uint8_t *rx_tlv_hdr,
		struct dp_vdev *vdev,
		struct dp_peer *peer
		)
{
	struct dp_peer *bss_peer;
	int fr_ds, to_ds, rx_3addr, rx_4addr;
	int rx_policy_ucast, rx_policy_mcast;
	int rx_mcast = hal_rx_msdu_end_da_is_mcbc_get(rx_tlv_hdr);

	if (vdev->opmode == wlan_op_mode_ap) {
		TAILQ_FOREACH(bss_peer, &vdev->peer_list, peer_list_elem) {
			if (bss_peer->bss_peer) {
				/* if wds policy check is not enabled on this vdev, accept all frames */
				if (!bss_peer->wds_ecm.wds_rx_filter) {
					return 1;
				}
				break;
			}
		}
		rx_policy_ucast = bss_peer->wds_ecm.wds_rx_ucast_4addr;
		rx_policy_mcast = bss_peer->wds_ecm.wds_rx_mcast_4addr;
	} else {             /* sta mode */
		if (!peer->wds_ecm.wds_rx_filter) {
			return 1;
		}
		rx_policy_ucast = peer->wds_ecm.wds_rx_ucast_4addr;
		rx_policy_mcast = peer->wds_ecm.wds_rx_mcast_4addr;
	}

	/* ------------------------------------------------
	 *                       self
	 * peer-             rx  rx-
	 * wds  ucast mcast dir policy accept note
	 * ------------------------------------------------
	 * 1     1     0     11  x1     1      AP configured to accept ds-to-ds Rx ucast from wds peers, constraint met; so, accept
	 * 1     1     0     01  x1     0      AP configured to accept ds-to-ds Rx ucast from wds peers, constraint not met; so, drop
	 * 1     1     0     10  x1     0      AP configured to accept ds-to-ds Rx ucast from wds peers, constraint not met; so, drop
	 * 1     1     0     00  x1     0      bad frame, won't see it
	 * 1     0     1     11  1x     1      AP configured to accept ds-to-ds Rx mcast from wds peers, constraint met; so, accept
	 * 1     0     1     01  1x     0      AP configured to accept ds-to-ds Rx mcast from wds peers, constraint not met; so, drop
	 * 1     0     1     10  1x     0      AP configured to accept ds-to-ds Rx mcast from wds peers, constraint not met; so, drop
	 * 1     0     1     00  1x     0      bad frame, won't see it
	 * 1     1     0     11  x0     0      AP configured to accept from-ds Rx ucast from wds peers, constraint not met; so, drop
	 * 1     1     0     01  x0     0      AP configured to accept from-ds Rx ucast from wds peers, constraint not met; so, drop
	 * 1     1     0     10  x0     1      AP configured to accept from-ds Rx ucast from wds peers, constraint met; so, accept
	 * 1     1     0     00  x0     0      bad frame, won't see it
	 * 1     0     1     11  0x     0      AP configured to accept from-ds Rx mcast from wds peers, constraint not met; so, drop
	 * 1     0     1     01  0x     0      AP configured to accept from-ds Rx mcast from wds peers, constraint not met; so, drop
	 * 1     0     1     10  0x     1      AP configured to accept from-ds Rx mcast from wds peers, constraint met; so, accept
	 * 1     0     1     00  0x     0      bad frame, won't see it
	 *
	 * 0     x     x     11  xx     0      we only accept td-ds Rx frames from non-wds peers in mode.
	 * 0     x     x     01  xx     1
	 * 0     x     x     10  xx     0
	 * 0     x     x     00  xx     0      bad frame, won't see it
	 * ------------------------------------------------
	 */

	fr_ds = hal_rx_mpdu_get_fr_ds(rx_tlv_hdr);
	to_ds = hal_rx_mpdu_get_to_ds(rx_tlv_hdr);
	rx_3addr = fr_ds ^ to_ds;
	rx_4addr = fr_ds & to_ds;

	if (vdev->opmode == wlan_op_mode_ap) {
		if ((!peer->wds_enabled && rx_3addr && to_ds) ||
				(peer->wds_enabled && !rx_mcast && (rx_4addr == rx_policy_ucast)) ||
				(peer->wds_enabled && rx_mcast && (rx_4addr == rx_policy_mcast))) {
			return 1;
		}
	} else {           /* sta mode */
		if ((!rx_mcast && (rx_4addr == rx_policy_ucast)) ||
				(rx_mcast && (rx_4addr == rx_policy_mcast))) {
			return 1;
		}
	}
	return 0;
}
#else
int dp_wds_rx_policy_check(
		uint8_t *rx_tlv_hdr,
		struct dp_vdev *vdev,
		struct dp_peer *peer
		)
{
	return 1;
}
#endif

/**
 * dp_rx_process() - Brain of the Rx processing functionality
 *		     Called from the bottom half (tasklet/NET_RX_SOFTIRQ)
 * @soc: core txrx main context
 * @hal_ring: opaque pointer to the HAL Rx Ring, which will be serviced
 * @quota: No. of units (packets) that can be serviced in one shot.
 *
 * This function implements the core of Rx functionality. This is
 * expected to handle only non-error frames.
 *
 * Return: uint32_t: No. of elements processed
 */
uint32_t
dp_rx_process(struct dp_intr *int_ctx, void *hal_ring, uint32_t quota)
{
	void *hal_soc;
	void *ring_desc;
	struct dp_rx_desc *rx_desc = NULL;
	qdf_nbuf_t nbuf, next;
	union dp_rx_desc_list_elem_t *head[MAX_PDEV_CNT] = { NULL };
	union dp_rx_desc_list_elem_t *tail[MAX_PDEV_CNT] = { NULL };
	uint32_t rx_bufs_used = 0, rx_buf_cookie = 0;
	uint32_t l2_hdr_offset = 0;
	uint16_t msdu_len = 0;
	uint16_t peer_id = HTT_INVALID_PEER;
	uint16_t old_peerid = HTT_INVALID_PEER;
	struct dp_peer *peer = NULL;
	struct dp_vdev *vdev = NULL;
	uint32_t pkt_len = 0;
	struct hal_rx_mpdu_desc_info mpdu_desc_info = { 0 };
	struct hal_rx_msdu_desc_info msdu_desc_info = { 0 };
	enum hal_reo_error_status error;
	uint32_t peer_mdata;
	uint8_t *rx_tlv_hdr;
	uint32_t rx_bufs_reaped[MAX_PDEV_CNT] = { 0 };
	uint32_t total_rx_bufs_reaped = 0;
	uint8_t mac_id = 0;
	struct dp_pdev *pdev;
	struct dp_srng *dp_rxdma_srng;
	struct rx_desc_pool *rx_desc_pool;
	struct dp_soc *soc = int_ctx->soc;
	uint8_t ring_id = 0;
	qdf_nbuf_t nbuf_head = NULL;
	qdf_nbuf_t nbuf_tail = NULL;
	qdf_nbuf_t deliver_list_head = NULL;
	qdf_nbuf_t deliver_list_tail = NULL;
	int32_t tid = 0;

	DP_HIST_INIT();
	/* Debug -- Remove later */
	qdf_assert(soc && hal_ring);

	hal_soc = soc->hal_soc;

	/* Debug -- Remove later */
	qdf_assert(hal_soc);

	hif_pm_runtime_mark_last_busy(soc->osdev->dev);

	if (qdf_unlikely(hal_srng_access_start(hal_soc, hal_ring))) {

		/*
		 * Need API to convert from hal_ring pointer to
		 * Ring Type / Ring Id combo
		 */
		DP_STATS_INC(soc, rx.err.hal_ring_access_fail, 1);
		QDF_TRACE(QDF_MODULE_ID_TXRX, QDF_TRACE_LEVEL_ERROR,
			FL("HAL RING Access Failed -- %pK"), hal_ring);
		hal_srng_access_end(hal_soc, hal_ring);
		return rx_bufs_used;
	}

	ring_id = hal_srng_ring_id_get(hal_ring);
	/*
	 * start reaping the buffers from reo ring and queue
	 * them in per vdev queue.
	 * Process the received pkts in a different per vdev loop.
	 */
	while (qdf_likely(quota)) {

		ring_desc = hal_srng_dst_get_next(hal_soc, hal_ring);

		/*
		 * in case HW has updated hp after we cached the hp
		 * ring_desc can be NULL even there are entries
		 * available in the ring. Update the cached_hp
		 * and reap the buffers available to read complete
		 * mpdu in one reap
		 */
		if (qdf_unlikely(ring_desc == NULL)) {
			hal_srng_access_start_unlocked(hal_soc, hal_ring);
			ring_desc = hal_srng_dst_get_next(hal_soc, hal_ring);
			if (!ring_desc) {
				break;
			}
			DP_STATS_INC(soc, rx.hp_oos, 1);
		}

		error = HAL_RX_ERROR_STATUS_GET(ring_desc);

		if (qdf_unlikely(error == HAL_REO_ERROR_DETECTED)) {
			QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_ERROR,
			FL("HAL RING 0x%pK:error %d"), hal_ring, error);
			DP_STATS_INC(soc, rx.err.hal_reo_error[ring_id], 1);
			/* Don't know how to deal with this -- assert */
			qdf_assert(0);
		}

		rx_buf_cookie = HAL_RX_REO_BUF_COOKIE_GET(ring_desc);

		rx_desc = dp_rx_cookie_2_va_rxdma_buf(soc, rx_buf_cookie);
		qdf_assert(rx_desc);

		/*
		 * this is a unlikely scenario where the host is reaping
		 * a descriptor which it already reaped just a while ago
		 * but is yet to replenish it back to HW.
		 * In this case host will dump the last 128 descriptors
		 * including the software descriptor rx_desc and assert.
		 */
		if (qdf_unlikely(!rx_desc->in_use)) {
			DP_STATS_INC(soc, rx.err.hal_reo_dest_dup, 1);
			dp_rx_dump_info_and_assert(soc, hal_ring,
						   ring_desc, rx_desc);
			continue;
		}

		rx_bufs_reaped[rx_desc->pool_id]++;

		/* Get MPDU DESC info */
		hal_rx_mpdu_desc_info_get(ring_desc, &mpdu_desc_info);

		peer_mdata = mpdu_desc_info.peer_meta_data;
		QDF_NBUF_CB_RX_PEER_ID(rx_desc->nbuf) =
			DP_PEER_METADATA_PEER_ID_GET(peer_mdata);

		/* Get MSDU DESC info */
		hal_rx_msdu_desc_info_get(ring_desc, &msdu_desc_info);

		/*
		 * save msdu flags first, last and continuation msdu in
		 * nbuf->cb
		 */
		if (msdu_desc_info.msdu_flags & HAL_MSDU_F_FIRST_MSDU_IN_MPDU)
			qdf_nbuf_set_rx_chfrag_start(rx_desc->nbuf, 1);

		if (msdu_desc_info.msdu_flags & HAL_MSDU_F_MSDU_CONTINUATION)
			qdf_nbuf_set_rx_chfrag_cont(rx_desc->nbuf, 1);

		if (msdu_desc_info.msdu_flags & HAL_MSDU_F_LAST_MSDU_IN_MPDU)
			qdf_nbuf_set_rx_chfrag_end(rx_desc->nbuf, 1);

		if (msdu_desc_info.msdu_flags & HAL_MSDU_F_DA_IS_MCBC)
			qdf_nbuf_set_da_mcbc(rx_desc->nbuf, 1);

		if (msdu_desc_info.msdu_flags & HAL_MSDU_F_DA_IS_VALID)
			qdf_nbuf_set_da_valid(rx_desc->nbuf, 1);

		if (msdu_desc_info.msdu_flags & HAL_MSDU_F_SA_IS_VALID)
			qdf_nbuf_set_sa_valid(rx_desc->nbuf, 1);

		QDF_NBUF_CB_RX_PKT_LEN(rx_desc->nbuf) = msdu_desc_info.msdu_len;

		DP_RX_LIST_APPEND(nbuf_head, nbuf_tail, rx_desc->nbuf);

		/*
		 * if continuation bit is set then we have MSDU spread
		 * across multiple buffers, let us not decrement quota
		 * till we reap all buffers of that MSDU.
		 */
		if (qdf_likely(!qdf_nbuf_is_rx_chfrag_cont(rx_desc->nbuf)))
			quota -= 1;


		dp_rx_add_to_free_desc_list(&head[rx_desc->pool_id],
						&tail[rx_desc->pool_id],
						rx_desc);
	}
	hal_srng_access_end(hal_soc, hal_ring);

	/* Update histogram statistics by looping through pdev's */
	DP_RX_HIST_STATS_PER_PDEV();

	for (mac_id = 0; mac_id < MAX_PDEV_CNT; mac_id++) {
		/*
		 * continue with next mac_id if no pkts were reaped
		 * from that pool
		 */
		if (!rx_bufs_reaped[mac_id])
			continue;

		pdev = soc->pdev_list[mac_id];
		dp_rxdma_srng = &pdev->rx_refill_buf_ring;
		rx_desc_pool = &soc->rx_desc_buf[mac_id];

		dp_rx_buffers_replenish(soc, mac_id, dp_rxdma_srng,
					rx_desc_pool, rx_bufs_reaped[mac_id],
					&head[mac_id], &tail[mac_id],
					HAL_RX_BUF_RBM_SW3_BM);

		total_rx_bufs_reaped += rx_bufs_reaped[mac_id];
	}

	qdf_atomic_add(total_rx_bufs_reaped,
		       &soc->batch_intr->pkt_to_stack_per_sec);
	/* Peer can be NULL is case of LFR */
	if (qdf_likely(peer != NULL))
		vdev = NULL;

	/*
	 * BIG loop where each nbuf is dequeued from global queue,
	 * processed and queued back on a per vdev basis. These nbufs
	 * are sent to stack as and when we run out of nbufs
	 * or a new nbuf dequeued from global queue has a different
	 * vdev when compared to previous nbuf.
	 */
	nbuf = nbuf_head;
	while (nbuf) {
		next = nbuf->next;

		peer_id = QDF_NBUF_CB_RX_PEER_ID(nbuf);
		if (qdf_unlikely(old_peerid != peer_id))
			peer = dp_peer_find_by_id(soc, peer_id);
		old_peerid = peer_id;

		rx_bufs_used++;

		if (deliver_list_head && peer && (vdev != peer->vdev)) {
			dp_rx_deliver_to_stack(vdev, peer, deliver_list_head,
					deliver_list_tail);
			deliver_list_head = NULL;
			deliver_list_tail = NULL;
		}

		if (qdf_likely((peer != NULL) && !peer->delete_in_progress)) {
			vdev = peer->vdev;
		} else {
			DP_STATS_INC_PKT(soc, rx.err.rx_invalid_peer, 1,
					 QDF_NBUF_CB_RX_PKT_LEN(nbuf));
			qdf_nbuf_free(nbuf);
			nbuf = next;
			continue;
		}

		if (qdf_unlikely(vdev == NULL)) {
			qdf_nbuf_free(nbuf);
			nbuf = next;
			DP_STATS_INC(soc, rx.err.invalid_vdev, 1);
			continue;
		}


		DP_HIST_PACKET_COUNT_INC(vdev->pdev->pdev_id);
		/*
		 * First IF condition:
		 * 802.11 Fragmented pkts are reinjected to REO
		 * HW block as SG pkts and for these pkts we only
		 * need to pull the RX TLVS header length.
		 * Second IF condition:
		 * The below condition happens when an MSDU is spread
		 * across multiple buffers. This can happen in two cases
		 * 1. The nbuf size is smaller then the received msdu.
		 *    ex: we have set the nbuf size to 2048 during
		 *        nbuf_alloc. but we received an msdu which is
		 *        2304 bytes in size then this msdu is spread
		 *        across 2 nbufs.
		 *
		 * 2. AMSDUs when RAW mode is enabled.
		 *    ex: 1st MSDU is in 1st nbuf and 2nd MSDU is spread
		 *        across 1st nbuf and 2nd nbuf and last MSDU is
		 *        spread across 2nd nbuf and 3rd nbuf.
		 *
		 * for these scenarios let us create a skb frag_list and
		 * append these buffers till the last MSDU of the AMSDU
		 * Third condition:
		 * This is the most likely case, we receive 802.3 pkts
		 * decapsulated by HW, here we need to set the pkt length.
		 */
		if (qdf_unlikely(qdf_nbuf_is_frag(nbuf))) {
			qdf_nbuf_unmap_single(soc->osdev, nbuf, QDF_DMA_FROM_DEVICE);
			rx_tlv_hdr = qdf_nbuf_data(nbuf);

			qdf_nbuf_set_da_mcbc(nbuf,
					hal_rx_msdu_end_da_is_mcbc_get(rx_tlv_hdr));

			qdf_nbuf_set_da_valid(nbuf,
					hal_rx_msdu_end_da_is_valid_get(rx_tlv_hdr));

			qdf_nbuf_set_sa_valid(nbuf,
					hal_rx_msdu_end_sa_is_valid_get(rx_tlv_hdr));

			qdf_nbuf_pull_head(nbuf, RX_PKT_TLVS_LEN);
		}
		else if (qdf_unlikely(vdev->rx_decap_type ==
				htt_cmn_pkt_type_raw)) {
			msdu_len = QDF_NBUF_CB_RX_PKT_LEN(nbuf);
			nbuf = dp_rx_sg_create(soc, nbuf, msdu_len);

			DP_STATS_INC(vdev->pdev, rx_raw_pkts, 1);
			DP_STATS_INC_PKT(peer, rx.raw, 1, msdu_len);

			rx_tlv_hdr = qdf_nbuf_data(nbuf);
			qdf_nbuf_pull_head(nbuf, RX_PKT_TLVS_LEN);
			next = nbuf->next;
		} else {
			msdu_len = QDF_NBUF_CB_RX_PKT_LEN(nbuf);
			qdf_nbuf_unmap_nbytes_single(soc->osdev, nbuf, QDF_DMA_FROM_DEVICE,
					     msdu_len + RX_PKT_TLVS_LEN
					     + MAX_L2_HDR_OFFSET);
			rx_tlv_hdr = qdf_nbuf_data(nbuf);

			l2_hdr_offset = ETH_L2_HDR_OFFSET;

			pkt_len = msdu_len + l2_hdr_offset + RX_PKT_TLVS_LEN;

			qdf_nbuf_set_pktlen(nbuf, pkt_len);
			qdf_nbuf_pull_head(nbuf,
					RX_PKT_TLVS_LEN +
					l2_hdr_offset);
		}

		if (!dp_wds_rx_policy_check(rx_tlv_hdr, vdev, peer)) {
			QDF_TRACE(QDF_MODULE_ID_DP,
					QDF_TRACE_LEVEL_ERROR,
					FL("Policy Check Drop pkt"));
			/* Drop & free packet */
			qdf_nbuf_free(nbuf);
			/* Statistics */
			nbuf = next;
			continue;
		}

		if (qdf_unlikely(peer->bss_peer)) {
			QDF_TRACE(QDF_MODULE_ID_DP,
				QDF_TRACE_LEVEL_ERROR,
				FL("received pkt with same src MAC"));
			DP_STATS_INC_PKT(peer, rx.mec_drop, 1, msdu_len);

			/* Drop & free packet */
			qdf_nbuf_free(nbuf);
			/* Statistics */
			nbuf = next;
			continue;
		}

		if (qdf_unlikely((peer->nawds_enabled == true) &&
			(qdf_nbuf_is_da_mcbc(nbuf)) &&
			(hal_rx_get_mpdu_mac_ad4_valid(rx_tlv_hdr) == false))) {
			DP_STATS_INC(peer, rx.nawds_mcast_drop, 1);
			qdf_nbuf_free(nbuf);
			nbuf = next;
			continue;
		}

		if (soc->process_rx_status)
			dp_rx_cksum_offload(vdev->pdev, nbuf, rx_tlv_hdr);

		dp_set_rx_queue(nbuf, ring_id);

		dp_rx_msdu_stats_update(soc, nbuf, rx_tlv_hdr, peer, ring_id);

		if (qdf_unlikely(vdev->mesh_vdev)) {
			if (dp_rx_filter_mesh_packets(vdev, nbuf,
							rx_tlv_hdr)
					== QDF_STATUS_SUCCESS) {
				QDF_TRACE(QDF_MODULE_ID_DP,
					QDF_TRACE_LEVEL_INFO_MED,
					FL("mesh pkt filtered"));
			DP_STATS_INC(vdev->pdev, dropped.mesh_filter,
					1);

				qdf_nbuf_free(nbuf);
				nbuf = next;
				continue;
			}
			dp_rx_fill_mesh_stats(vdev, nbuf, rx_tlv_hdr, peer);
		}

#ifdef QCA_WIFI_NAPIER_EMULATION_DBG /* Debug code, remove later */
		QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_ERROR,
			"p_id %d msdu_len %d hdr_off %d",
			peer_id, msdu_len, l2_hdr_offset);

		print_hex_dump(KERN_ERR,
			       "\t Pkt Data:", DUMP_PREFIX_NONE, 32, 4,
				qdf_nbuf_data(nbuf), 128, false);
#endif /* NAPIER_EMULATION */

		if (qdf_likely(vdev->rx_decap_type ==
					htt_cmn_pkt_type_ethernet) &&
				(qdf_likely(!vdev->mesh_vdev)) &&
				(vdev->wds_enabled)) {
			dp_rx_da_learn(soc, rx_tlv_hdr, peer, nbuf);

			/* Due to HW issue, sometimes we see that the sa_idx
			 * and da_idx are invalid with sa_valid and da_valid
			 * bits set
			 *
			 * in this case we also see that value of
			 * sa_sw_peer_id is set as 0
			 *
			 * Drop the packet if sa_idx and da_idx OOB or
			 * sa_sw_peerid is 0
			 */
			if ((qdf_nbuf_is_sa_valid(nbuf) &&
			    (!hal_rx_msdu_end_sa_sw_peer_id_get(rx_tlv_hdr) ||
			     (hal_rx_msdu_end_sa_idx_get(rx_tlv_hdr) >
				wlan_cfg_get_max_ast_idx(soc->wlan_cfg_ctx)))) ||
			    (qdf_nbuf_is_da_valid(nbuf) &&
			     (hal_rx_msdu_end_da_idx_get(rx_tlv_hdr) >
			      wlan_cfg_get_max_ast_idx(soc->wlan_cfg_ctx)))) {

				qdf_nbuf_free(nbuf);
				nbuf = next;
				DP_STATS_INC(soc, rx.err.invalid_sa_da_idx, 1);
				continue;
			}
			/* WDS Source Port Learning */
			dp_rx_wds_srcport_learn(soc,
						rx_tlv_hdr,
						peer,
						nbuf);

			/* Intrabss-fwd */
			if (dp_rx_check_ap_bridge(vdev))
				if (dp_rx_intrabss_fwd(soc,
							peer,
							rx_tlv_hdr,
							nbuf)) {
					nbuf = next;
					continue; /* Get next desc */
				}
		}

		/* Get TID from first msdu per MPDU, save to skb->priority */
		if (qdf_nbuf_is_rx_chfrag_start(nbuf)) {
			tid = hal_rx_mpdu_start_tid_get(rx_tlv_hdr);
		}
		DP_RX_TID_SAVE(nbuf, tid);

		dp_rx_lro(rx_tlv_hdr, peer, nbuf, int_ctx->lro_ctx);

		DP_RX_LIST_APPEND(deliver_list_head,
					deliver_list_tail,
					nbuf);

		DP_STATS_INC_PKT(peer, rx.to_stack, 1,
				QDF_NBUF_CB_RX_PKT_LEN(nbuf));

		nbuf = next;
	}

	if (deliver_list_head)
		dp_rx_deliver_to_stack(vdev, peer, deliver_list_head,
				deliver_list_tail);

	return rx_bufs_used; /* Assume no scale factor for now */
}

/**
 * dp_rx_detach() - detach dp rx
 * @pdev: core txrx pdev context
 *
 * This function will detach DP RX into main device context
 * will free DP Rx resources.
 *
 * Return: void
 */
void
dp_rx_pdev_detach(struct dp_pdev *pdev)
{
	uint8_t pdev_id = pdev->pdev_id;
	struct dp_soc *soc = pdev->soc;
	struct rx_desc_pool *rx_desc_pool;

	rx_desc_pool = &soc->rx_desc_buf[pdev_id];

	if (rx_desc_pool->pool_size != 0) {
		dp_rx_desc_pool_free(soc, pdev_id, rx_desc_pool);
		qdf_spinlock_destroy(&soc->rx_desc_mutex[pdev_id]);
	}

	return;
}

/**
 * dp_rx_attach() - attach DP RX
 * @pdev: core txrx pdev context
 *
 * This function will attach a DP RX instance into the main
 * device (SOC) context. Will allocate dp rx resource and
 * initialize resources.
 *
 * Return: QDF_STATUS_SUCCESS: success
 *         QDF_STATUS_E_RESOURCES: Error return
 */
QDF_STATUS
dp_rx_pdev_attach(struct dp_pdev *pdev)
{
	uint8_t pdev_id = pdev->pdev_id;
	struct dp_soc *soc = pdev->soc;
	struct dp_srng rxdma_srng;
	uint32_t rxdma_entries;
	union dp_rx_desc_list_elem_t *desc_list = NULL;
	union dp_rx_desc_list_elem_t *tail = NULL;
	struct dp_srng *dp_rxdma_srng;
	struct rx_desc_pool *rx_desc_pool;

	if (wlan_cfg_get_dp_pdev_nss_enabled(pdev->wlan_cfg_ctx)) {
		QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_INFO,
			  "nss-wifi<4> skip Rx refil %d", pdev_id);
		return QDF_STATUS_SUCCESS;
	}

	qdf_spinlock_create(&soc->rx_desc_mutex[pdev_id]);
	pdev = soc->pdev_list[pdev_id];
	rxdma_srng = pdev->rx_refill_buf_ring;
	soc->process_rx_status = CONFIG_PROCESS_RX_STATUS;
	rxdma_entries = rxdma_srng.alloc_size/hal_srng_get_entrysize(
						     soc->hal_soc, RXDMA_BUF);

	rx_desc_pool = &soc->rx_desc_buf[pdev_id];

	dp_rx_desc_pool_alloc(soc, pdev_id, rxdma_entries*3, rx_desc_pool);
	/* For Rx buffers, WBM release ring is SW RING 3,for all pdev's */
	dp_rxdma_srng = &pdev->rx_refill_buf_ring;
	dp_rx_buffers_replenish(soc, pdev_id, dp_rxdma_srng, rx_desc_pool,
		0, &desc_list, &tail, HAL_RX_BUF_RBM_SW3_BM);

	return QDF_STATUS_SUCCESS;
}

/*
 * dp_rx_nbuf_prepare() - prepare RX nbuf
 * @soc: core txrx main context
 * @pdev: core txrx pdev context
 *
 * This function alloc & map nbuf for RX dma usage, retry it if failed
 * until retry times reaches max threshold or succeeded.
 *
 * Return: qdf_nbuf_t pointer if succeeded, NULL if failed.
 */
qdf_nbuf_t
dp_rx_nbuf_prepare(struct dp_soc *soc, struct dp_pdev *pdev)
{
	uint8_t *buf;
	int32_t nbuf_retry_count;
	QDF_STATUS ret;
	qdf_nbuf_t nbuf = NULL;

	for (nbuf_retry_count = 0; nbuf_retry_count <
		QDF_NBUF_ALLOC_MAP_RETRY_THRESHOLD;
			nbuf_retry_count++) {
		/* Allocate a new skb */
		nbuf = qdf_nbuf_alloc(soc->osdev,
					RX_BUFFER_SIZE,
					RX_BUFFER_RESERVATION,
					RX_BUFFER_ALIGNMENT,
					FALSE);

		if (nbuf == NULL) {
			DP_STATS_INC(pdev,
				replenish.nbuf_alloc_fail, 1);
			continue;
		}

		buf = qdf_nbuf_data(nbuf);

		memset(buf, 0, RX_BUFFER_SIZE);

		ret = qdf_nbuf_map_single(soc->osdev, nbuf,
				    QDF_DMA_FROM_DEVICE);

		/* nbuf map failed */
		if (qdf_unlikely(QDF_IS_STATUS_ERROR(ret))) {
			qdf_nbuf_free(nbuf);
			DP_STATS_INC(pdev, replenish.map_err, 1);
			continue;
		}
		/* qdf_nbuf alloc and map succeeded */
		break;
	}

	/* qdf_nbuf still alloc or map failed */
	if (qdf_unlikely(nbuf_retry_count >=
			QDF_NBUF_ALLOC_MAP_RETRY_THRESHOLD))
		return NULL;

	return nbuf;
}
