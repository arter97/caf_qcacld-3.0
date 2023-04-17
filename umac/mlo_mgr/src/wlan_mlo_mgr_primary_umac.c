/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "wlan_mlo_mgr_main.h"
#include "qdf_types.h"
#include "wlan_cmn.h"
#include "wlan_mlo_mgr_peer.h"
#include <wlan_mlo_mgr_ap.h>
#include <wlan_mlo_mgr_setup.h>
#include <wlan_utility.h>
#include <wlan_reg_services_api.h>

/**
 * struct mlpeer_data: PSOC peers MLO data
 * @total_rssi:  sum of RSSI of all ML peers
 * @num_ml_peers: Number of ML peer's with this PSOC as TQM
 * @max_ml_peers: Max ML peers can have this PSOC as TQM
 *                (it is to distribute peers across all PSOCs)
 * @num_non_ml_peers: Non MLO peers of this PSOC
 */
struct mlpeer_data {
	int32_t total_rssi;
	uint16_t num_ml_peers;
	uint16_t max_ml_peers;
	uint16_t num_non_ml_peers;
};

/**
 * struct mlo_all_link_rssi: structure to collect TQM params for all PSOCs
 * @psoc_tqm_parms:  It collects peer data for all PSOCs
 * @num_psocs:       Number of PSOCs in the system
 * @current_psoc_id: current psoc id, it is for iterator
 */
struct mlo_all_link_rssi {
	struct mlpeer_data psoc_tqm_parms[WLAN_OBJMGR_MAX_DEVICES];
	uint8_t num_psocs;
	uint8_t current_psoc_id;
};

/* Invalid TQM/PSOC ID */
#define ML_INVALID_PRIMARY_TQM   0xff
/* Congestion value */
#define ML_PRIMARY_TQM_CONGESTION 30

static void wlan_mlo_peer_get_rssi(struct wlan_objmgr_psoc *psoc,
				   void *obj, void *args)
{
	struct wlan_mlo_peer_context *mlo_peer_ctx;
	struct wlan_objmgr_peer *peer = (struct wlan_objmgr_peer *)obj;
	struct mlo_all_link_rssi *rssi_data = (struct mlo_all_link_rssi *)args;
	struct mlpeer_data *tqm_params = NULL;
	uint8_t index;

	mlo_peer_ctx = peer->mlo_peer_ctx;
	index = rssi_data->current_psoc_id;
	tqm_params = &rssi_data->psoc_tqm_parms[index];

	if (!wlan_peer_is_mlo(peer) && !mlo_peer_ctx) {
		if (wlan_peer_get_peer_type(peer) == WLAN_PEER_STA)
			tqm_params->num_non_ml_peers += 1;
		return;
	}

	if (!mlo_peer_ctx)
		return;

	/* If this psoc is new primary UMAC after migration,
	 * account RSSI on new link
	 */
	if (mlo_peer_ctx->migrate_primary_umac_psoc_id ==
			rssi_data->current_psoc_id) {
		tqm_params->total_rssi += mlo_peer_ctx->avg_link_rssi;
		tqm_params->num_ml_peers += 1;
		return;
	}

	/* If this psoc is not primary UMAC or if TQM migration is happening
	 * from current primary psoc, don't account RSSI
	 */
	if (mlo_peer_ctx->primary_umac_psoc_id == rssi_data->current_psoc_id &&
	    mlo_peer_ctx->migrate_primary_umac_psoc_id ==
	    ML_INVALID_PRIMARY_TQM) {
		tqm_params->total_rssi += mlo_peer_ctx->avg_link_rssi;
		tqm_params->num_ml_peers += 1;
	}
}

static void wlan_get_rssi_data_each_psoc(struct wlan_objmgr_psoc *psoc,
					 void *arg, uint8_t index)
{
	struct mlo_all_link_rssi *rssi_data = (struct mlo_all_link_rssi *)arg;
	struct mlpeer_data *tqm_params = NULL;

	tqm_params = &rssi_data->psoc_tqm_parms[index];

	tqm_params->total_rssi = 0;
	tqm_params->num_ml_peers = 0;
	tqm_params->num_non_ml_peers = 0;
	tqm_params->max_ml_peers = MAX_MLO_PEER;

	rssi_data->current_psoc_id = index;
	rssi_data->num_psocs++;

	wlan_objmgr_iterate_obj_list(psoc, WLAN_PEER_OP,
				     wlan_mlo_peer_get_rssi, rssi_data, 0,
				     WLAN_MLO_MGR_ID);
}

static QDF_STATUS mld_get_link_rssi(struct mlo_all_link_rssi *rssi_data)
{
	rssi_data->num_psocs = 0;

	wlan_objmgr_iterate_psoc_list(wlan_get_rssi_data_each_psoc,
				      rssi_data, WLAN_MLO_MGR_ID);

	return QDF_STATUS_SUCCESS;
}

uint8_t
wlan_mld_get_best_primary_umac_w_rssi(struct wlan_mlo_peer_context *ml_peer,
				      struct wlan_objmgr_vdev *link_vdevs[])
{
	struct mlo_all_link_rssi rssi_data;
	uint8_t i;
	int32_t avg_rssi[WLAN_OBJMGR_MAX_DEVICES] = {0};
	int32_t diff_rssi[WLAN_OBJMGR_MAX_DEVICES] = {0};
	int32_t diff_low;
	bool mld_sta_links[WLAN_OBJMGR_MAX_DEVICES] = {0};
	bool mld_no_sta[WLAN_OBJMGR_MAX_DEVICES] = {0};
	uint8_t prim_link, id, prim_link_hi;
	uint8_t num_psocs;
	struct mlpeer_data *tqm_params = NULL;
	struct wlan_channel *channel;
	enum phy_ch_width sec_hi_bw, hi_bw;
	uint8_t cong = ML_PRIMARY_TQM_CONGESTION;
	uint16_t mld_ml_sta_count[WLAN_OBJMGR_MAX_DEVICES] = {0};
	enum phy_ch_width mld_ch_width[WLAN_OBJMGR_MAX_DEVICES];
	uint8_t psoc_w_nosta;
	uint16_t ml_sta_count = 0;
	uint32_t total_cap, cap;
	uint16_t bw;
	bool group_full[WLAN_OBJMGR_MAX_DEVICES] = {0};
	uint16_t group_size[WLAN_OBJMGR_MAX_DEVICES] = {0};
	uint16_t grp_size = 0;
	uint16_t group_full_count = 0;

	mld_get_link_rssi(&rssi_data);

	for (i = 0; i < rssi_data.num_psocs; i++) {
		tqm_params = &rssi_data.psoc_tqm_parms[i];

		if (tqm_params->num_ml_peers)
			avg_rssi[i] = (tqm_params->total_rssi /
				       tqm_params->num_ml_peers);
	}

	/**
	 * If MLD STA associated to a set of links, choose primary UMAC
	 * from those links only
	 */
	num_psocs = 0;
	psoc_w_nosta = 0;
	for (i = 0; i < WLAN_UMAC_MLO_MAX_VDEVS; i++)
		mld_ch_width[i] = CH_WIDTH_INVALID;

	for (i = 0; i < WLAN_UMAC_MLO_MAX_VDEVS; i++) {
		if (!link_vdevs[i])
			continue;

		id = wlan_vdev_get_psoc_id(link_vdevs[i]);
		if (id >= WLAN_OBJMGR_MAX_DEVICES)
			continue;

		if (wlan_vdev_skip_pumac(link_vdevs[i])) {
			mlo_err("Skip Radio for Primary MLO umac");
			mld_sta_links[id] = false;
			continue;
		}

		tqm_params = &rssi_data.psoc_tqm_parms[id];
		mld_sta_links[id] = true;

		channel = wlan_vdev_mlme_get_bss_chan(link_vdevs[i]);
		mld_ch_width[id] = channel->ch_width;

		if ((tqm_params->num_ml_peers +
		     tqm_params->num_non_ml_peers) == 0) {
			/* If this PSOC has no stations */
			mld_no_sta[id] = true;
			psoc_w_nosta++;
		}

		mld_ml_sta_count[id] = tqm_params->num_ml_peers;
		/* Update total MLO STA count */
		ml_sta_count += tqm_params->num_ml_peers;

		num_psocs++;

		/* If no stations are associated, derive diff rssi
		 * based on psoc id {0-20, 20-40, 40 } so that
		 * stations are distributed across TQMs
		 */
		if (!avg_rssi[id]) {
			diff_rssi[id] = (id * 20);
			continue;
		}
		diff_rssi[id] = (ml_peer->avg_link_rssi >= avg_rssi[id]) ?
				(ml_peer->avg_link_rssi - avg_rssi[id]) :
				(avg_rssi[id] - ml_peer->avg_link_rssi);

	}

	prim_link = ML_INVALID_PRIMARY_TQM;

	/* If one of the PSOCs doesn't have any station select that PSOC as
	 * primary TQM. If more than one PSOC have no stations as Primary TQM
	 * the vdev with less bw needs to be selected as Primary TQM
	 */
	if (psoc_w_nosta == 1) {
		for (i = 0; i < WLAN_OBJMGR_MAX_DEVICES; i++) {
			if (mld_no_sta[i]) {
				prim_link = i;
				break;
			}
		}
	} else if (psoc_w_nosta > 1) {
		hi_bw = CH_WIDTH_INVALID;
		sec_hi_bw = CH_WIDTH_INVALID;
		for (i = 0; i < WLAN_OBJMGR_MAX_DEVICES; i++) {
			if (!mld_no_sta[i])
				continue;

			if (hi_bw == CH_WIDTH_INVALID) {
				prim_link_hi = i;
				hi_bw = mld_ch_width[i];
				continue;
			}
			/* if bw is 320MHZ mark it as highest ch width */
			if (mld_ch_width[i] == CH_WIDTH_320MHZ) {
				prim_link = prim_link_hi;
				sec_hi_bw = hi_bw;
				hi_bw = mld_ch_width[i];
				prim_link_hi = i;
			}
			/* If bw is less than or equal to 160 MHZ
			 * and chwidth is greater than than other link
			 * Mark this link as primary link
			 */
			if (mld_ch_width[i] <= CH_WIDTH_160MHZ) {
				if (hi_bw < mld_ch_width[i]) {
					/* move high bw to second high bw */
					prim_link = prim_link_hi;
					sec_hi_bw = hi_bw;

					hi_bw = mld_ch_width[i];
					prim_link_hi = i;
				} else if ((sec_hi_bw == CH_WIDTH_INVALID) ||
					   (sec_hi_bw < mld_ch_width[i])) {
					/* update sec high bw */
					sec_hi_bw = mld_ch_width[i];
					prim_link = i;
				}
			}
		}
	} else {
		total_cap = 0;
		for (i = 0; i < WLAN_OBJMGR_MAX_DEVICES; i++) {
			bw = wlan_reg_get_bw_value(mld_ch_width[i]);
			total_cap += bw * (100 - cong);
		}

		group_full_count = 0;
		for (i = 0; i < WLAN_OBJMGR_MAX_DEVICES; i++) {
			if (!mld_sta_links[i])
				continue;

			bw = wlan_reg_get_bw_value(mld_ch_width[i]);
			cap = bw * (100 - cong);
			grp_size = (ml_sta_count) * ((cap * 100) / total_cap);
			group_size[i] = grp_size / 100;
			if (group_size[i] <=  mld_ml_sta_count[i]) {
				group_full[i] = true;
				group_full_count++;
			}
		}

		if ((num_psocs - group_full_count) == 1) {
			for (i = 0; i < WLAN_OBJMGR_MAX_DEVICES; i++) {
				if (!mld_sta_links[i])
					continue;

				if (group_full[i])
					continue;

				prim_link = i;
				break;
			}
		} else {
			diff_low = 0;
			/* find min diff, based on it, allocate primary umac */
			for (i = 0; i < WLAN_OBJMGR_MAX_DEVICES; i++) {
				if (!mld_sta_links[i])
					continue;

				/* First iteration */
				if (diff_low == 0) {
					diff_low = diff_rssi[i];
					prim_link = i;
				} else if (diff_low > diff_rssi[i]) {
					diff_low = diff_rssi[i];
					prim_link = i;
				}
			}
		}
	}

	if (prim_link != ML_INVALID_PRIMARY_TQM)
		return prim_link;

	/* If primary link id is not found, return id of 1st available link */
	for (i = 0; i < WLAN_UMAC_MLO_MAX_VDEVS; i++) {
		if (!link_vdevs[i])
			continue;

		if (wlan_vdev_skip_pumac(link_vdevs[i])) {
			mlo_debug("Skip Radio for Primary MLO umac");
			continue;
		}
		id = wlan_vdev_get_psoc_id(link_vdevs[i]);
		if (id >= WLAN_OBJMGR_MAX_DEVICES)
			continue;

		return wlan_vdev_get_psoc_id(link_vdevs[i]);
	}

	return ML_INVALID_PRIMARY_TQM;
}

void mlo_peer_assign_primary_umac(
		struct wlan_mlo_peer_context *ml_peer,
		struct wlan_mlo_link_peer_entry *peer_entry)
{
	struct wlan_mlo_link_peer_entry *peer_ent_iter;
	uint8_t i;
	uint8_t primary_umac_set = 0;

	/* If MLD is within single SOC, then assoc link becomes
	 * primary umac
	 */
	if (ml_peer->primary_umac_psoc_id == ML_PRIMARY_UMAC_ID_INVAL) {
		if (wlan_peer_mlme_is_assoc_peer(peer_entry->link_peer)) {
			peer_entry->is_primary = true;
			ml_peer->primary_umac_psoc_id =
				wlan_peer_get_psoc_id(peer_entry->link_peer);
		} else {
			peer_entry->is_primary = false;
		}
	} else {
		/* If this peer PSOC is not derived as Primary PSOC,
		 * mark is_primary as false
		 */
		if (wlan_peer_get_psoc_id(peer_entry->link_peer) !=
				ml_peer->primary_umac_psoc_id) {
			peer_entry->is_primary = false;
			return;
		}

		/* For single SOC, check whether is_primary is set for
		 * other partner peer, then mark is_primary false for this peer
		 */
		for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
			peer_ent_iter = &ml_peer->peer_list[i];

			if (!peer_ent_iter->link_peer)
				continue;

			/* Check for other link peers */
			if (peer_ent_iter == peer_entry)
				continue;

			if (wlan_peer_get_psoc_id(peer_ent_iter->link_peer) !=
					ml_peer->primary_umac_psoc_id)
				continue;

			if (peer_ent_iter->is_primary)
				primary_umac_set = 1;
		}

		if (primary_umac_set)
			peer_entry->is_primary = false;
		else
			peer_entry->is_primary = true;
	}
}

static int8_t wlan_vdev_derive_link_rssi(struct wlan_objmgr_vdev *vdev,
					 struct wlan_objmgr_vdev *assoc_vdev,
					 int8_t rssi)
{
	struct wlan_channel *channel, *assoc_channel;
	uint16_t ch_freq, assoc_freq;
	uint8_t tx_pow, assoc_tx_pow;
	int8_t diff_txpow;
	struct wlan_objmgr_pdev *pdev, *assoc_pdev;
	uint8_t log10_freq;
	uint8_t derived_rssi;
	int16_t ten_derived_rssi;
	int8_t ten_diff_pl = 0;

	pdev = wlan_vdev_get_pdev(vdev);
	assoc_pdev = wlan_vdev_get_pdev(assoc_vdev);

	channel = wlan_vdev_get_active_channel(vdev);
	if (channel)
		ch_freq = channel->ch_freq;
	else
		ch_freq = 1;

	assoc_channel = wlan_vdev_get_active_channel(assoc_vdev);
	if (assoc_channel)
		assoc_freq = assoc_channel->ch_freq;
	else
		assoc_freq = 1;

	/*
	 *  diff of path loss (of two links) = log10(freq1) - log10(freq2)
	 *                       (since distance is constant)
	 *  since log10 is not available, we cameup with approximate ranges
	 */
	log10_freq = (ch_freq * 10) / assoc_freq;
	if ((log10_freq >= 20) && (log10_freq < 30))
		ten_diff_pl = 4;  /* 0.4 *10 */
	else if ((log10_freq >= 11) && (log10_freq < 20))
		ten_diff_pl = 1;  /* 0.1 *10 */
	else if ((log10_freq >= 8) && (log10_freq < 11))
		ten_diff_pl = 0; /* 0 *10 */
	else if ((log10_freq >= 4) && (log10_freq < 8))
		ten_diff_pl = -1; /* -0.1 * 10 */
	else if ((log10_freq >= 1) && (log10_freq < 4))
		ten_diff_pl = -4;  /* -0.4 * 10 */

	assoc_tx_pow = wlan_reg_get_channel_reg_power_for_freq(assoc_pdev,
							       assoc_freq);
	tx_pow = wlan_reg_get_channel_reg_power_for_freq(pdev, ch_freq);

	diff_txpow = tx_pow -  assoc_tx_pow;

	ten_derived_rssi = (diff_txpow * 10) - ten_diff_pl + (rssi * 10);
	derived_rssi = ten_derived_rssi / 10;

	return derived_rssi;
}

static void mlo_peer_calculate_avg_rssi(
		struct wlan_mlo_dev_context *ml_dev,
		struct wlan_mlo_peer_context *ml_peer,
		int8_t rssi,
		struct wlan_objmgr_vdev *assoc_vdev)
{
	int32_t total_rssi = 0;
	uint8_t num_psocs = 0;
	uint8_t i;
	struct wlan_objmgr_vdev *vdev;

	for (i = 0; i < WLAN_UMAC_MLO_MAX_VDEVS; i++) {
		vdev = ml_dev->wlan_vdev_list[i];
		if (!vdev)
			continue;

		num_psocs++;
		if (vdev == assoc_vdev)
			total_rssi += rssi;
		else
			total_rssi += wlan_vdev_derive_link_rssi(vdev,
								 assoc_vdev,
								 rssi);
	}

	if (!num_psocs)
		return;

	ml_peer->avg_link_rssi = total_rssi / num_psocs;
}

#ifdef WLAN_MLO_MULTI_CHIP
int8_t mlo_get_central_umac_id(
		uint8_t *psoc_ids)
{
	uint8_t prim_psoc_id = -1;
	uint8_t adjacent = 0;

	/* Some 3 link RDPs have restriction on the primary umac.
	 * Only the link that is adjacent to both the links can be
	 * a primary umac.
	 * Note: it means umac migration is also restricted.
	 */
	mlo_chip_adjacent(psoc_ids[0], psoc_ids[1], &adjacent);
	if (!adjacent) {
		prim_psoc_id = psoc_ids[2];
	} else {
		mlo_chip_adjacent(psoc_ids[0], psoc_ids[2], &adjacent);
		if (!adjacent) {
			prim_psoc_id = psoc_ids[1];
		} else {
			/* If all links are adjacent to each other,
			 * no need to restrict the primary umac.
			 * return failure the caller will handle.
			 */
			mlo_chip_adjacent(psoc_ids[1], psoc_ids[2],
					  &adjacent);
			if (!adjacent)
				prim_psoc_id = psoc_ids[0];
			else
				return prim_psoc_id;
		}
	}

	return prim_psoc_id;
}

static QDF_STATUS mlo_set_3_link_primary_umac(
		struct wlan_mlo_peer_context *ml_peer,
		struct wlan_objmgr_vdev *link_vdevs[])
{
	uint8_t psoc_ids[WLAN_UMAC_MLO_MAX_VDEVS];
	int8_t central_umac_id;

	if (ml_peer->max_links != 3)
		return QDF_STATUS_E_FAILURE;

	/* Some 3 link RDPs have restriction on the primary umac.
	 * Only the link that is adjacent to both the links can be
	 * a primary umac.
	 * Note: it means umac migration is also restricted.
	 */
	psoc_ids[0] = wlan_vdev_get_psoc_id(link_vdevs[0]);
	psoc_ids[1] = wlan_vdev_get_psoc_id(link_vdevs[1]);
	psoc_ids[2] = wlan_vdev_get_psoc_id(link_vdevs[2]);

	central_umac_id = mlo_get_central_umac_id(psoc_ids);
	if (central_umac_id != -1)
		ml_peer->primary_umac_psoc_id = central_umac_id;
	else
		return QDF_STATUS_E_FAILURE;

	mlo_peer_assign_primary_umac(ml_peer,
				     &ml_peer->peer_list[0]);

	return QDF_STATUS_SUCCESS;
}
#else
static QDF_STATUS mlo_set_3_link_primary_umac(
		struct wlan_mlo_peer_context *ml_peer,
		struct wlan_objmgr_vdev *link_vdevs[])
{
	return QDF_STATUS_E_FAILURE;
}
#endif

QDF_STATUS mlo_peer_allocate_primary_umac(
		struct wlan_mlo_dev_context *ml_dev,
		struct wlan_mlo_peer_context *ml_peer,
		struct wlan_objmgr_vdev *link_vdevs[])
{
	struct wlan_mlo_link_peer_entry *peer_entry;
	struct wlan_objmgr_peer *assoc_peer = NULL;
	int32_t rssi;
	struct mlo_mgr_context *mlo_ctx = wlan_objmgr_get_mlo_ctx();
	uint8_t first_link_id = 0;
	bool primary_umac_set = false;
	uint8_t i, psoc_id;

	peer_entry = &ml_peer->peer_list[0];
	assoc_peer = peer_entry->link_peer;
	if (!assoc_peer)
		return QDF_STATUS_E_FAILURE;

	/* For Station mode, assign assoc peer as primary umac */
	if (wlan_peer_get_peer_type(assoc_peer) == WLAN_PEER_AP) {
		mlo_peer_assign_primary_umac(ml_peer, peer_entry);
		mlo_info("MLD ID %d ML Peer " QDF_MAC_ADDR_FMT " primary umac soc %d ",
			 ml_dev->mld_id,
			 QDF_MAC_ADDR_REF(ml_peer->peer_mld_addr.bytes),
			 ml_peer->primary_umac_psoc_id);

		return QDF_STATUS_SUCCESS;
	}

	/* Select assoc peer's PSOC as primary UMAC in Multi-chip solution,
	 * 1) for single link MLO connection
	 * 2) if MLD is single chip MLO
	 */
	if ((mlo_ctx->force_non_assoc_prim_umac) &&
	    (ml_peer->max_links >= 1)) {
		for (i = 0; i < WLAN_UMAC_MLO_MAX_VDEVS; i++) {
			if (!link_vdevs[i])
				continue;

			if (wlan_peer_get_vdev(assoc_peer) == link_vdevs[i])
				continue;
			psoc_id = wlan_vdev_get_psoc_id(link_vdevs[i]);
			ml_peer->primary_umac_psoc_id = psoc_id;
			break;
		}

		mlo_peer_assign_primary_umac(ml_peer, peer_entry);
		mlo_info("MLD ID %d ML Peer " QDF_MAC_ADDR_FMT
			 " primary umac soc %d ", ml_dev->mld_id,
			 QDF_MAC_ADDR_REF(ml_peer->peer_mld_addr.bytes),
			 ml_peer->primary_umac_psoc_id);

		return QDF_STATUS_SUCCESS;
	}

	if ((ml_peer->max_links == 1) ||
	    (mlo_vdevs_check_single_soc(link_vdevs, ml_peer->max_links))) {
		mlo_peer_assign_primary_umac(ml_peer, peer_entry);
		mlo_info("MLD ID %d Assoc peer " QDF_MAC_ADDR_FMT
			 " primary umac soc %d ", ml_dev->mld_id,
			 QDF_MAC_ADDR_REF(ml_peer->peer_mld_addr.bytes),
			 ml_peer->primary_umac_psoc_id);

		return QDF_STATUS_SUCCESS;
	}

	if (mlo_set_3_link_primary_umac(ml_peer, link_vdevs) ==
	    QDF_STATUS_SUCCESS) {
		/* If success then the primary umac is restricted and assigned.
		 * if not, there is no restriction, so just fallthrough
		 */
		mlo_info("MLD ID %d ML Peer " QDF_MAC_ADDR_FMT
			 " center primary umac soc %d ",
			 ml_dev->mld_id,
			 QDF_MAC_ADDR_REF(ml_peer->peer_mld_addr.bytes),
			 ml_peer->primary_umac_psoc_id);

		return QDF_STATUS_SUCCESS;
	}

	if (mlo_ctx->mlo_is_force_primary_umac) {
		for (i = 0; i < WLAN_UMAC_MLO_MAX_VDEVS; i++) {
			if (!link_vdevs[i])
				continue;

			psoc_id = wlan_vdev_get_psoc_id(link_vdevs[i]);
			if (!first_link_id)
				first_link_id = psoc_id;

			if (psoc_id == mlo_ctx->mlo_forced_primary_umac_id) {
				ml_peer->primary_umac_psoc_id = psoc_id;
				primary_umac_set = true;
				break;
			}
		}

		if (!primary_umac_set)
			ml_peer->primary_umac_psoc_id = first_link_id;

		mlo_peer_assign_primary_umac(ml_peer, peer_entry);
		mlo_info("MLD ID %d ML Peer " QDF_MAC_ADDR_FMT " primary umac soc %d ",
			 ml_dev->mld_id,
			 QDF_MAC_ADDR_REF(ml_peer->peer_mld_addr.bytes),
			 ml_peer->primary_umac_psoc_id);

		return QDF_STATUS_SUCCESS;
	}

	rssi = wlan_peer_get_rssi(assoc_peer);
	mlo_peer_calculate_avg_rssi(ml_dev, ml_peer, rssi,
				    wlan_peer_get_vdev(assoc_peer));

	ml_peer->primary_umac_psoc_id =
		wlan_mld_get_best_primary_umac_w_rssi(ml_peer, link_vdevs);

	mlo_peer_assign_primary_umac(ml_peer, peer_entry);

	mlo_info("MLD ID %d ML Peer " QDF_MAC_ADDR_FMT " avg RSSI %d primary umac soc %d ",
		 ml_dev->mld_id,
		 QDF_MAC_ADDR_REF(ml_peer->peer_mld_addr.bytes),
		 ml_peer->avg_link_rssi, ml_peer->primary_umac_psoc_id);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlo_peer_free_primary_umac(
		struct wlan_mlo_dev_context *ml_dev,
		struct wlan_mlo_peer_context *ml_peer)
{
	return QDF_STATUS_SUCCESS;
}

#ifdef QCA_SUPPORT_PRIMARY_LINK_MIGRATE
void wlan_objmgr_mlo_update_primary_info(struct wlan_objmgr_peer *peer)
{
	struct wlan_mlo_peer_context *ml_peer = NULL;
	struct wlan_mlo_link_peer_entry *peer_ent_iter;
	uint8_t i;

	ml_peer = peer->mlo_peer_ctx;
	ml_peer->primary_umac_psoc_id = wlan_peer_get_psoc_id(peer);

	for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
		peer_ent_iter = &ml_peer->peer_list[i];

		if (!peer_ent_iter->link_peer)
			continue;

		if (peer_ent_iter->is_primary)
			peer_ent_iter->is_primary = false;

		if (peer_ent_iter->link_peer == peer)
			peer_ent_iter->is_primary = true;
	}
}

qdf_export_symbol(wlan_objmgr_mlo_update_primary_info);
#endif
