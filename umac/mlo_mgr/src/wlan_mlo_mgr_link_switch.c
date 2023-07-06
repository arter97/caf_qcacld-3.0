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
 * DOC: contains MLO manager Link Switch related functionality
 */
#include <wlan_mlo_mgr_link_switch.h>
#include <wlan_objmgr_vdev_obj.h>
#include <wlan_mlo_mgr_cmn.h>

void mlo_mgr_update_link_info_mac_addr(struct wlan_objmgr_vdev *vdev,
				       struct wlan_mlo_link_mac_update *ml_mac_update)
{
	struct mlo_link_info *link_info;
	uint8_t link_info_iter;
	struct mlo_vdev_link_mac_info *link_mac_info;

	if (!vdev || !vdev->mlo_dev_ctx || !ml_mac_update)
		return;

	link_mac_info = &ml_mac_update->link_mac_info[0];
	link_info = &vdev->mlo_dev_ctx->link_ctx->links_info[0];

	for (link_info_iter = 0; link_info_iter < WLAN_MAX_ML_BSS_LINKS;
	     link_info_iter++) {
		qdf_mem_copy(&link_info->link_addr,
			     &link_mac_info->link_mac_addr,
			     QDF_MAC_ADDR_SIZE);

		link_info->vdev_id = link_mac_info->vdev_id;
		mlo_debug("link_info_iter:%d vdev_id %d "QDF_MAC_ADDR_FMT,
			  link_info_iter, link_info->vdev_id,
			  QDF_MAC_ADDR_REF(link_info->link_addr.bytes));
		link_mac_info++;
		link_info++;
	}
}

void mlo_mgr_update_ap_link_info(struct wlan_objmgr_vdev *vdev, uint8_t link_id,
				 uint8_t *ap_link_addr,
				 struct wlan_channel channel)
{
	struct mlo_link_info *link_info;
	uint8_t link_info_iter;

	if (!vdev || !vdev->mlo_dev_ctx || !ap_link_addr)
		return;

	link_info = &vdev->mlo_dev_ctx->link_ctx->links_info[0];
	for (link_info_iter = 0; link_info_iter < WLAN_MAX_ML_BSS_LINKS;
	     link_info_iter++) {
		if (qdf_is_macaddr_zero(&link_info->ap_link_addr))
			break;

		link_info++;
	}

	if (link_info_iter == WLAN_MAX_ML_BSS_LINKS)
		return;

	qdf_mem_copy(&link_info->ap_link_addr, ap_link_addr, QDF_MAC_ADDR_SIZE);
	qdf_mem_copy(link_info->link_chan_info, &channel, sizeof(channel));
	link_info->link_id = link_id;
}

void mlo_mgr_update_link_info_reset(struct wlan_mlo_dev_context *ml_dev)
{
	struct mlo_link_info *link_info;
	uint8_t link_info_iter;

	if (!ml_dev)
		return;

	link_info = &ml_dev->link_ctx->links_info[0];

	for (link_info_iter = 0; link_info_iter < WLAN_MAX_ML_BSS_LINKS;
	     link_info_iter++) {
		qdf_mem_zero(&link_info->link_addr, QDF_MAC_ADDR_SIZE);
		qdf_mem_zero(&link_info->ap_link_addr, QDF_MAC_ADDR_SIZE);
		link_info->vdev_id = WLAN_INVALID_VDEV_ID;
		link_info->link_id = WLAN_INVALID_LINK_ID;
		link_info++;
	}
}

struct mlo_link_info
*mlo_mgr_get_ap_link_by_link_id(struct wlan_objmgr_vdev *vdev, int link_id)
{
	struct mlo_link_info *link_info;
	uint8_t link_info_iter;

	if (!vdev || link_id < 0 || link_id > 15)
		return NULL;

	link_info = &vdev->mlo_dev_ctx->link_ctx->links_info[0];
	for (link_info_iter = 0; link_info_iter < WLAN_MAX_ML_BSS_LINKS;
	     link_info_iter++) {
		if (link_info->link_id == link_id)
			return link_info;
		link_info++;
	}

	return NULL;
}

static
void mlo_mgr_alloc_link_info_wmi_chan(struct wlan_mlo_dev_context *ml_dev)
{
	struct mlo_link_info *link_info;
	uint8_t link_info_iter;

	if (!ml_dev)
		return;

	link_info = &ml_dev->link_ctx->links_info[0];
	for (link_info_iter = 0; link_info_iter < WLAN_MAX_ML_BSS_LINKS;
	     link_info_iter++) {
		link_info->link_chan_info =
			qdf_mem_malloc(sizeof(*link_info->link_chan_info));
		if (!link_info->link_chan_info)
			return;
		link_info++;
	}
}

static
void mlo_mgr_free_link_info_wmi_chan(struct wlan_mlo_dev_context *ml_dev)
{
	struct mlo_link_info *link_info;
	uint8_t link_info_iter;

	if (!ml_dev)
		return;

	link_info = &ml_dev->link_ctx->links_info[0];
	for (link_info_iter = 0; link_info_iter < WLAN_MAX_ML_BSS_LINKS;
	     link_info_iter++) {
		if (link_info->link_chan_info)
			qdf_mem_free(link_info->link_chan_info);
		link_info++;
	}
}

QDF_STATUS mlo_mgr_link_switch_init(struct wlan_mlo_dev_context *ml_dev)
{
	ml_dev->link_ctx =
		qdf_mem_malloc(sizeof(struct mlo_link_switch_context));

	if (!ml_dev->link_ctx)
		return QDF_STATUS_E_NOMEM;

	mlo_mgr_alloc_link_info_wmi_chan(ml_dev);
	mlo_mgr_update_link_info_reset(ml_dev);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlo_mgr_link_switch_deinit(struct wlan_mlo_dev_context *ml_dev)
{
	mlo_mgr_free_link_info_wmi_chan(ml_dev);
	qdf_mem_free(ml_dev->link_ctx);
	ml_dev->link_ctx = NULL;
	return QDF_STATUS_SUCCESS;
}
