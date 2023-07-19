/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

/*
 * DOC: contains ML STA link force active/inactive related functionality
 */
#include "wlan_mlo_link_force.h"
#include "wlan_mlo_mgr_sta.h"

void
ml_nlink_convert_linkid_bitmap_to_vdev_bitmap(
			struct wlan_objmgr_psoc *psoc,
			struct wlan_objmgr_vdev *vdev,
			uint32_t link_bitmap,
			uint32_t *associated_bitmap,
			uint32_t *vdev_id_bitmap_sz,
			uint32_t vdev_id_bitmap[MLO_VDEV_BITMAP_SZ],
			uint8_t *vdev_id_num,
			uint8_t vdev_ids[WLAN_MLO_MAX_VDEVS])
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct wlan_mlo_sta *sta_ctx;
	uint8_t i, j, bitmap_sz = 0, num_vdev = 0;
	uint16_t link_id;
	uint8_t vdev_id;
	uint32_t associated_link_bitmap = 0;

	*vdev_id_bitmap_sz = 0;
	*vdev_id_num = 0;
	qdf_mem_zero(vdev_id_bitmap,
		     sizeof(vdev_id_bitmap[0]) * MLO_VDEV_BITMAP_SZ);
	qdf_mem_zero(vdev_ids,
		     sizeof(vdev_ids[0]) * WLAN_MLO_MAX_VDEVS);
	if (associated_bitmap)
		*associated_bitmap = 0;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	mlo_dev_lock_acquire(mlo_dev_ctx);
	sta_ctx = mlo_dev_ctx->sta_ctx;
	for (i = 0; i < WLAN_UMAC_MLO_MAX_VDEVS; i++) {
		/*todo: add standby link */
		if (!mlo_dev_ctx->wlan_vdev_list[i])
			continue;
		vdev_id = wlan_vdev_get_id(mlo_dev_ctx->wlan_vdev_list[i]);
		if (!qdf_test_bit(i, sta_ctx->wlan_connected_links)) {
			mlo_debug("vdev %d is not connected", vdev_id);
			continue;
		}

		link_id = wlan_vdev_get_link_id(
					mlo_dev_ctx->wlan_vdev_list[i]);
		if (link_id >= MAX_MLO_LINK_ID) {
			mlo_err("invalid link id %d", link_id);
			continue;
		}
		associated_link_bitmap |= 1 << link_id;
		/* If the link_id is not interested one which is specified
		 * in "link_bitmap", continue the search.
		 */
		if (!(link_bitmap & (1 << link_id)))
			continue;
		j = vdev_id / 32;
		if (j >= MLO_VDEV_BITMAP_SZ)
			break;
		vdev_id_bitmap[j] |= 1 << (vdev_id % 32);
		if (j + 1 > bitmap_sz)
			bitmap_sz = j + 1;

		if (num_vdev >= WLAN_MLO_MAX_VDEVS)
			break;
		vdev_ids[num_vdev++] = vdev_id;
	}
	mlo_dev_lock_release(mlo_dev_ctx);

	*vdev_id_bitmap_sz = bitmap_sz;
	*vdev_id_num = num_vdev;
	if (associated_bitmap)
		*associated_bitmap = associated_link_bitmap;

	mlo_debug("vdev %d link bitmap 0x%x vdev_bitmap 0x%x sz %d num %d assoc 0x%x for bitmap 0x%x",
		  wlan_vdev_get_id(vdev), link_bitmap & associated_link_bitmap,
		  vdev_id_bitmap[0], *vdev_id_bitmap_sz, num_vdev,
		  associated_link_bitmap, link_bitmap);
}

void
ml_nlink_convert_vdev_bitmap_to_linkid_bitmap(
				struct wlan_objmgr_psoc *psoc,
				struct wlan_objmgr_vdev *vdev,
				uint32_t vdev_id_bitmap_sz,
				uint32_t *vdev_id_bitmap,
				uint32_t *link_bitmap,
				uint32_t *associated_bitmap)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct wlan_mlo_sta *sta_ctx;
	uint8_t i, j;
	uint16_t link_id;
	uint8_t vdev_id;
	uint32_t associated_link_bitmap = 0;

	*link_bitmap = 0;
	if (associated_bitmap)
		*associated_bitmap = 0;
	if (!vdev_id_bitmap_sz) {
		mlo_debug("vdev_id_bitmap_sz 0");
		return;
	}

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	sta_ctx = mlo_dev_ctx->sta_ctx;
	mlo_dev_lock_acquire(mlo_dev_ctx);
	for (i = 0; i < WLAN_UMAC_MLO_MAX_VDEVS; i++) {
		if (!mlo_dev_ctx->wlan_vdev_list[i])
			continue;
		vdev_id = wlan_vdev_get_id(mlo_dev_ctx->wlan_vdev_list[i]);
		if (!qdf_test_bit(i, sta_ctx->wlan_connected_links)) {
			mlo_debug("vdev %d is not connected", vdev_id);
			continue;
		}

		link_id = wlan_vdev_get_link_id(
					mlo_dev_ctx->wlan_vdev_list[i]);
		if (link_id >= MAX_MLO_LINK_ID) {
			mlo_err("invalid link id %d", link_id);
			continue;
		}
		associated_link_bitmap |= 1 << link_id;
		j = vdev_id / 32;
		if (j >= vdev_id_bitmap_sz) {
			mlo_err("invalid vdev id %d", vdev_id);
			continue;
		}
		/* If the vdev_id is not interested one which is specified
		 * in "vdev_id_bitmap", continue the search.
		 */
		if (!(vdev_id_bitmap[j] & (1 << (vdev_id % 32))))
			continue;

		*link_bitmap |= 1 << link_id;
	}
	mlo_dev_lock_release(mlo_dev_ctx);

	if (associated_bitmap)
		*associated_bitmap = associated_link_bitmap;
	mlo_debug("vdev %d link bitmap 0x%x vdev_bitmap 0x%x sz %d assoc 0x%x",
		  wlan_vdev_get_id(vdev), *link_bitmap, vdev_id_bitmap[0],
		  vdev_id_bitmap_sz, associated_link_bitmap);
}

void
ml_nlink_get_curr_force_state(struct wlan_objmgr_psoc *psoc,
			      struct wlan_objmgr_vdev *vdev,
			      struct ml_link_force_state *force_cmd)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	mlo_dev_lock_acquire(mlo_dev_ctx);
	qdf_mem_copy(force_cmd,
		     &mlo_dev_ctx->sta_ctx->link_force_ctx.force_state,
		     sizeof(*force_cmd));
	mlo_dev_lock_release(mlo_dev_ctx);
}

void
ml_nlink_clr_force_state(struct wlan_objmgr_psoc *psoc,
			 struct wlan_objmgr_vdev *vdev)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct ml_link_force_state *force_state;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	mlo_dev_lock_acquire(mlo_dev_ctx);
	force_state = &mlo_dev_ctx->sta_ctx->link_force_ctx.force_state;
	qdf_mem_zero(force_state, sizeof(*force_state));
	ml_nlink_dump_force_state(force_state, "");
	mlo_dev_lock_release(mlo_dev_ctx);
}

static void
ml_nlink_update_link_bitmap(uint16_t *curr_link_bitmap,
			    uint16_t link_bitmap,
			    enum set_curr_control ctrl)
{
	switch (ctrl) {
	case LINK_OVERWRITE:
		*curr_link_bitmap = link_bitmap;
		break;
	case LINK_CLR:
		*curr_link_bitmap &= ~link_bitmap;
		break;
	case LINK_ADD:
		*curr_link_bitmap |= link_bitmap;
		break;
	default:
		mlo_err("unknown update ctrl %d", ctrl);
		return;
	}
}

void
ml_nlink_set_curr_force_active_state(struct wlan_objmgr_psoc *psoc,
				     struct wlan_objmgr_vdev *vdev,
				     uint16_t link_bitmap,
				     enum set_curr_control ctrl)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct ml_link_force_state *force_state;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	mlo_dev_lock_acquire(mlo_dev_ctx);
	force_state = &mlo_dev_ctx->sta_ctx->link_force_ctx.force_state;
	ml_nlink_update_link_bitmap(&force_state->force_active_bitmap,
				    link_bitmap, ctrl);
	ml_nlink_dump_force_state(force_state, ":ctrl %d bitmap 0x%x",
				  ctrl, link_bitmap);
	mlo_dev_lock_release(mlo_dev_ctx);
}

void
ml_nlink_set_curr_force_inactive_state(struct wlan_objmgr_psoc *psoc,
				       struct wlan_objmgr_vdev *vdev,
				       uint16_t link_bitmap,
				       enum set_curr_control ctrl)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct ml_link_force_state *force_state;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	mlo_dev_lock_acquire(mlo_dev_ctx);
	force_state = &mlo_dev_ctx->sta_ctx->link_force_ctx.force_state;
	ml_nlink_update_link_bitmap(&force_state->force_inactive_bitmap,
				    link_bitmap, ctrl);
	ml_nlink_dump_force_state(force_state, ":ctrl %d bitmap 0x%x", ctrl,
				  link_bitmap);
	mlo_dev_lock_release(mlo_dev_ctx);
}

void
ml_nlink_set_curr_force_active_num_state(struct wlan_objmgr_psoc *psoc,
					 struct wlan_objmgr_vdev *vdev,
					 uint8_t link_num,
					 uint16_t link_bitmap)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct ml_link_force_state *force_state;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	mlo_dev_lock_acquire(mlo_dev_ctx);
	force_state = &mlo_dev_ctx->sta_ctx->link_force_ctx.force_state;
	force_state->force_active_num = link_num;
	force_state->force_active_num_bitmap = link_bitmap;
	ml_nlink_dump_force_state(force_state, ":num %d bitmap 0x%x",
				  link_num, link_bitmap);
	mlo_dev_lock_release(mlo_dev_ctx);
}

void
ml_nlink_set_curr_force_inactive_num_state(struct wlan_objmgr_psoc *psoc,
					   struct wlan_objmgr_vdev *vdev,
					   uint8_t link_num,
					   uint16_t link_bitmap)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct ml_link_force_state *force_state;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	mlo_dev_lock_acquire(mlo_dev_ctx);
	force_state = &mlo_dev_ctx->sta_ctx->link_force_ctx.force_state;
	force_state->force_inactive_num = link_num;
	force_state->force_inactive_num_bitmap = link_bitmap;
	ml_nlink_dump_force_state(force_state, ":num %d bitmap 0x%x",
				  link_num, link_bitmap);
	mlo_dev_lock_release(mlo_dev_ctx);
}
