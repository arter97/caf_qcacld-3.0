/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
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

#ifndef _WLAN_STATS_H_
#define _WLAN_STATS_H_

#include <wlan_stats_define.h>

/**
 * struct stats_config: Structure to hold user configurations
 * @wiphy:  Pointer to wiphy structure which came as part of User request
 * @feat:  Feat flag set to dedicated bit of this field
 * @lvl:  Requested level of Stats (i.e. Basic, Advance or Debug)
 * @obj:  Requested stats for object (i.e. AP, Radio, Vap or STA)
 * @type:  Requested stats category
 * @status:  Status of Stats acumulation in recursive call
 * @recursive:  Flag for Recursiveness of request
 */
struct stats_config {
	struct wiphy           *wiphy;
	u_int64_t              feat;
	enum stats_level_e     lvl;
	enum stats_object_e    obj;
	enum stats_type_e      type;
	QDF_STATUS             status;
	bool                   recursive;
};

/**
 * struct unified_stats: Structure to carry all feature specific stats in driver
 *                       level for stats response setup
 * All features are void pointers and its corresponding sizes.
 * This can hold Basic or Advance or Debug structures independently.
 */
struct unified_stats {
	void *me;
	void *rx;
	void *tx;
	void *fwd;
	void *raw;
	void *tso;
	void *twt;
	void *igmp;
	void *link;
	void *mesh;
	void *rate;
	void *nawds;
	u_int32_t me_size;
	u_int32_t rx_size;
	u_int32_t tx_size;
	u_int32_t fwd_size;
	u_int32_t raw_size;
	u_int32_t tso_size;
	u_int32_t twt_size;
	u_int32_t igmp_size;
	u_int32_t link_size;
	u_int32_t mesh_size;
	u_int32_t rate_size;
	u_int32_t nawds_size;
};

/**
 * wlan_stats_get_peer_stats(): Function to get peer specific stats
 * @psoc:  Pointer to Psoc object
 * @peer:  Pointer to Peer object
 * @cfg:  Pointer to stats config came as part of user request
 * @stats:  Pointer to unified stats object
 *
 * Return: QDF_STATUS_SUCCESS for success and Error code for failure
 */
QDF_STATUS wlan_stats_get_peer_stats(struct wlan_objmgr_psoc *psoc,
				     struct wlan_objmgr_peer *peer,
				     struct stats_config *cfg,
				     struct unified_stats *stats);

/**
 * wlan_stats_get_vdev_stats(): Function to get vdev specific stats
 * @psoc:  Pointer to Psoc object
 * @vdev:  Pointer to Vdev object
 * @cfg:  Pointer to stats config came as part of user request
 * @stats:  Pointer to unified stats object
 *
 * Return: QDF_STATUS_SUCCESS for success and Error code for failure
 */
QDF_STATUS wlan_stats_get_vdev_stats(struct wlan_objmgr_psoc *psoc,
				     struct wlan_objmgr_vdev *vdev,
				     struct stats_config *cfg,
				     struct unified_stats *stats);

/**
 * wlan_stats_get_pdev_stats(): Function to get pdev specific stats
 * @psoc:  Pointer to Psoc object
 * @pdev:  Pointer to Pdev object
 * @cfg:  Pointer to stats config came as part of user request
 * @stats:  Pointer to unified stats object
 *
 * Return: QDF_STATUS_SUCCESS for success and Error code for failure
 */
QDF_STATUS wlan_stats_get_pdev_stats(struct wlan_objmgr_psoc *psoc,
				     struct wlan_objmgr_pdev *pdev,
				     struct stats_config *cfg,
				     struct unified_stats *stats);

/**
 * wlan_stats_get_psoc_stats(): Function to get psoc specific stats
 * @psoc:  Pointer to Psoc object
 * @cfg:  Pointer to stats config came as part of user request
 * @stats:  Pointer to unified stats object
 *
 * Return: QDF_STATUS_SUCCESS for success and Error code for failure
 */
QDF_STATUS wlan_stats_get_psoc_stats(struct wlan_objmgr_psoc *psoc,
				     struct stats_config *cfg,
				     struct unified_stats *stats);

/**
 * wlan_stats_is_recursive_valid(): Function to check recursiveness
 * @cfg:  Pointer to stats config came as part of user request
 * @obj:  The object for which recursiveness is being checked
 *
 * So, this function will check if requested feature is valid for
 * underneath objects.
 *
 * Return: True if Recursive is possible or false if not
 */
bool wlan_stats_is_recursive_valid(struct stats_config *cfg,
				   enum stats_object_e obj);

/**
 * wlan_stats_free_unified_stats(): Function to free all feature holder pointers
 * @stats:  Pointer to unified stats object
 *
 * Return: None
 */
void wlan_stats_free_unified_stats(struct unified_stats *stats);

#endif /* _WLAN_STATS_H_ */
