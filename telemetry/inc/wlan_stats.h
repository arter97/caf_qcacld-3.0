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

struct stats_config {
	enum stats_level_e     lvl;
	enum stats_object_e    obj;
	enum stats_type_e      type;
	u_int8_t               recursive;
	u_int32_t              feat;
	QDF_STATUS             status;
	struct sk_buff         *reply_skb;
};

struct unified_stats {
	void *tx;
	void *rx;
	void *link;
	void *rate;
	u_int32_t tx_size;
	u_int32_t rx_size;
	u_int32_t link_size;
	u_int32_t rate_size;
};

QDF_STATUS wlan_stats_get_peer_stats(struct wlan_objmgr_psoc *psoc,
				     struct wlan_objmgr_peer *peer,
				     struct stats_config *cfg,
				     struct unified_stats *stats);

QDF_STATUS wlan_stats_get_vdev_stats(struct wlan_objmgr_psoc *psoc,
				     struct wlan_objmgr_vdev *vdev,
				     struct stats_config *cfg,
				     struct unified_stats *stats);

QDF_STATUS wlan_stats_get_pdev_stats(struct wlan_objmgr_psoc *psoc,
				     struct wlan_objmgr_pdev *pdev,
				     struct stats_config *cfg,
				     struct unified_stats *stats);

QDF_STATUS wlan_stats_get_psoc_stats(struct wlan_objmgr_psoc *psoc,
				     struct stats_config *cfg,
				     struct unified_stats *stats);

void wlan_stats_free_unified_stats(struct unified_stats *stats);

#endif /* _WLAN_STATS_H_ */
