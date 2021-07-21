/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
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
 * DOC: mbss_utils.c
 * This file implements APIs for all multi vdev
 * functions
 */

#include "mbss_utils.h"

bool mbss_acs_in_progress(struct wlan_objmgr_vdev *vdev)
{
	bool status = false;
	struct mbss_pdev *mbss_ctx;

	mbss_ctx = mbss_get_ctx(vdev);
	if (!mbss_ctx) {
		mbss_err("MBSS ctx in null");
		goto exit;
	}

	mbss_lock(mbss_ctx);
	status = mbss_ctx->acs_in_progress;
	mbss_unlock(mbss_ctx);
exit:
	return status;
}

bool mbss_ht40_in_progress(struct wlan_objmgr_vdev *vdev)
{
	bool status = false;
	struct mbss_pdev *mbss_ctx;

	mbss_ctx = mbss_get_ctx(vdev);
	if (!mbss_ctx) {
		mbss_err("MBSS ctx in null");
		goto exit;
	}

	mbss_lock(mbss_ctx);
	status = mbss_ctx->ht40_in_progress;
	mbss_unlock(mbss_ctx);
exit:
	return status;
}

bool mbss_vdev_acs_in_progress(struct wlan_objmgr_vdev *vdev,
			       enum wlan_mbss_acs_source acs_src)
{
	bool status = false;
	struct mbss_pdev *mbss_ctx;
	struct mbss_acs_data *data;
	mbss_bitmap_type *bitmap;

	mbss_ctx = mbss_get_ctx(vdev);
	if (!mbss_ctx) {
		mbss_err("MBSS ctx in null");
		goto exit;
	}

	if (acs_src >= MBSS_ACS_SRC_MAX)
		goto exit;

	mbss_lock(mbss_ctx);

	data = &mbss_ctx->mbss_acs.data[acs_src];
	bitmap = data->vdevs_waiting_acs;

	if (mbss_check_vdev_bit(wlan_vdev_get_id(vdev), bitmap))
		status = true;

	mbss_unlock(mbss_ctx);
exit:
	return status;
}

bool mbss_sta_connecting(struct wlan_objmgr_vdev *vdev)
{
	bool status = false;
	struct mbss_pdev *mbss_ctx;
	mbss_bitmap_type *bitmap;

	mbss_ctx = mbss_get_ctx(vdev);
	if (!mbss_ctx) {
		mbss_err("MBSS ctx in null");
		goto exit;
	}

	mbss_lock(mbss_ctx);
	bitmap = mbss_ctx->vdev_bitmaps.connecting_vdevs;

	if (mbss_check_vdev_bit(wlan_vdev_get_id(vdev), bitmap))
		status = true;

	mbss_unlock(mbss_ctx);
exit:
	return status;
}

uint8_t mbss_num_sta_up(struct wlan_objmgr_pdev *pdev)
{
	uint8_t num = 0;

	return num;
}

uint8_t mbss_num_ap_up(struct wlan_objmgr_pdev *pdev)
{
	uint8_t num = 0;

	return num;
}

uint8_t mbss_num_sta_connecting(struct wlan_objmgr_pdev *pdev)
{
	uint8_t num = 0;
	struct mbss_pdev *mbss_ctx;

	mbss_ctx = mbss_get_pdev_ctx(pdev);
	if (!mbss_ctx) {
		mbss_err("MBSS ctx in null");
		goto exit;
	}

	mbss_lock(mbss_ctx);
	mbss_unlock(mbss_ctx);
exit:
	return num;
}

uint8_t mbss_num_sta(struct wlan_objmgr_pdev *pdev)
{
	uint8_t num = 0;
	struct mbss_pdev *mbss_ctx;

	mbss_ctx = mbss_get_pdev_ctx(pdev);
	if (!mbss_ctx) {
		mbss_err("MBSS ctx in null");
		goto exit;
	}

	mbss_lock(mbss_ctx);
	num = mbss_ctx->num_sta_vdev;
	mbss_unlock(mbss_ctx);
exit:
	return num;
}

uint8_t mbss_num_ap(struct wlan_objmgr_pdev *pdev)
{
	uint8_t num = 0;
	struct mbss_pdev *mbss_ctx;

	mbss_ctx = mbss_get_pdev_ctx(pdev);
	if (!mbss_ctx) {
		mbss_err("MBSS ctx in null");
		goto exit;
	}

	mbss_lock(mbss_ctx);
	num = mbss_ctx->num_ap_vdev;
	mbss_unlock(mbss_ctx);
exit:
	return num;
}

uint8_t mbss_num_monitor(struct wlan_objmgr_pdev *pdev)
{
	uint8_t num = 0;
	struct mbss_pdev *mbss_ctx;

	mbss_ctx = mbss_get_pdev_ctx(pdev);
	if (!mbss_ctx) {
		mbss_err("MBSS ctx in null");
		goto exit;
	}

	mbss_lock(mbss_ctx);
	num = mbss_ctx->num_monitor_vdev;
	mbss_unlock(mbss_ctx);
exit:
	return num;
}

uint8_t mbss_num_vdev(struct wlan_objmgr_pdev *pdev)
{
	uint8_t num = 0;
	struct mbss_pdev *mbss_ctx;

	mbss_ctx = mbss_get_pdev_ctx(pdev);
	if (!mbss_ctx) {
		mbss_err("MBSS ctx in null");
		goto exit;
	}

	mbss_lock(mbss_ctx);
	num = mbss_ctx->num_vdev;
	mbss_unlock(mbss_ctx);
exit:
	return num;
}

QDF_STATUS
mbss_start_vdevs(struct wlan_objmgr_pdev *pdev, void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct mbss_pdev *mbss_ctx;
	wlan_objmgr_pdev_op_handler handler;
	struct wlan_mbss_ext_cb *ext_ops;

	mbss_ctx = mbss_get_pdev_ctx(pdev);
	if (!mbss_ctx) {
		mbss_err("MBSS ctx in null");
		goto err;
	}

	ext_ops = wlan_mbss_get_ext_ops();
	if (ext_ops && ext_ops->mbss_start_vdevs_cb)
		handler = ext_ops->mbss_start_vdevs_cb;
	else
		goto err;

	return wlan_objmgr_pdev_iterate_obj_list(pdev,
						   WLAN_VDEV_OP,
						   handler,
						   arg, 0,
						   WLAN_MBSS_ID);
err:
	status = QDF_STATUS_E_FAILURE;
	return status;
}

QDF_STATUS
mbss_start_ap_vdevs(struct wlan_objmgr_pdev *pdev, void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct mbss_pdev *mbss_ctx;
	wlan_objmgr_pdev_op_handler handler;
	struct wlan_mbss_ext_cb *ext_ops;

	mbss_ctx = mbss_get_pdev_ctx(pdev);
	if (!mbss_ctx) {
		mbss_err("MBSS ctx in null");
		goto err;
	}

	ext_ops = wlan_mbss_get_ext_ops();
	if (ext_ops && ext_ops->mbss_start_ap_vdevs_cb)
		handler = ext_ops->mbss_start_ap_vdevs_cb;
	else
		goto err;

	return wlan_objmgr_pdev_iterate_obj_list(pdev,
						   WLAN_VDEV_OP,
						   handler,
						   arg, 0,
						   WLAN_MBSS_ID);
err:
	status = QDF_STATUS_E_FAILURE;
	return status;
}

QDF_STATUS
mbss_start_sta_vdevs(struct wlan_objmgr_pdev *pdev, void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct mbss_pdev *mbss_ctx;
	wlan_objmgr_pdev_op_handler handler;
	struct wlan_mbss_ext_cb *ext_ops;

	mbss_ctx = mbss_get_pdev_ctx(pdev);
	if (!mbss_ctx) {
		mbss_err("MBSS ctx in null");
		goto err;
	}

	ext_ops = wlan_mbss_get_ext_ops();
	if (ext_ops && ext_ops->mbss_start_sta_vdevs_cb)
		handler = ext_ops->mbss_start_sta_vdevs_cb;
	else
		goto err;

	return wlan_objmgr_pdev_iterate_obj_list(pdev,
						   WLAN_VDEV_OP,
						   handler,
						   arg, 0,
						   WLAN_MBSS_ID);
err:
	status = QDF_STATUS_E_FAILURE;
	return status;
}

QDF_STATUS
mbss_stop_vdevs(struct wlan_objmgr_pdev *pdev, void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct mbss_pdev *mbss_ctx;
	wlan_objmgr_pdev_op_handler handler;
	struct wlan_mbss_ext_cb *ext_ops;

	mbss_ctx = mbss_get_pdev_ctx(pdev);
	if (!mbss_ctx) {
		mbss_err("MBSS ctx in null");
		goto err;
	}

	ext_ops = wlan_mbss_get_ext_ops();
	if (ext_ops && ext_ops->mbss_stop_vdevs_cb)
		handler = ext_ops->mbss_stop_vdevs_cb;
	else
		goto err;

	return wlan_objmgr_pdev_iterate_obj_list(pdev,
						   WLAN_VDEV_OP,
						   handler,
						   arg, 0,
						   WLAN_MBSS_ID);
err:
	status = QDF_STATUS_E_FAILURE;
	return status;
}

QDF_STATUS
mbss_stop_ap_vdevs(struct wlan_objmgr_pdev *pdev, void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct mbss_pdev *mbss_ctx;
	wlan_objmgr_pdev_op_handler handler;
	struct wlan_mbss_ext_cb *ext_ops;

	mbss_ctx = mbss_get_pdev_ctx(pdev);
	if (!mbss_ctx) {
		mbss_err("MBSS ctx in null");
		goto err;
	}

	ext_ops = wlan_mbss_get_ext_ops();
	if (ext_ops && ext_ops->mbss_stop_ap_vdevs_cb)
		handler = ext_ops->mbss_stop_ap_vdevs_cb;
	else
		goto err;

	return wlan_objmgr_pdev_iterate_obj_list(pdev,
						   WLAN_VDEV_OP,
						   handler,
						   arg, 0,
						   WLAN_MBSS_ID);
err:
	status = QDF_STATUS_E_FAILURE;
	return status;
}

QDF_STATUS
mbss_stop_sta_vdevs(struct wlan_objmgr_pdev *pdev, void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct mbss_pdev *mbss_ctx;
	wlan_objmgr_pdev_op_handler handler;
	struct wlan_mbss_ext_cb *ext_ops;

	mbss_ctx = mbss_get_pdev_ctx(pdev);
	if (!mbss_ctx) {
		mbss_err("MBSS ctx in null");
		goto err;
	}

	ext_ops = wlan_mbss_get_ext_ops();
	if (ext_ops && ext_ops->mbss_stop_sta_vdevs_cb)
		handler = ext_ops->mbss_stop_sta_vdevs_cb;
	else
		goto err;

	return wlan_objmgr_pdev_iterate_obj_list(pdev,
						   WLAN_VDEV_OP,
						   handler,
						   arg, 0,
						   WLAN_MBSS_ID);
err:
	status = QDF_STATUS_E_FAILURE;
	return status;
}

QDF_STATUS
mbss_stop_ap_monitor_vdevs(struct wlan_objmgr_pdev *pdev, void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct mbss_pdev *mbss_ctx;
	wlan_objmgr_pdev_op_handler handler;
	struct wlan_mbss_ext_cb *ext_ops;

	mbss_ctx = mbss_get_pdev_ctx(pdev);
	if (!mbss_ctx) {
		mbss_err("MBSS ctx in null");
		goto err;
	}

	ext_ops = wlan_mbss_get_ext_ops();
	if (ext_ops && ext_ops->mbss_stop_ap_monitor_vdevs_cb)
		handler = ext_ops->mbss_stop_ap_monitor_vdevs_cb;
	else
		goto err;

	return wlan_objmgr_pdev_iterate_obj_list(pdev,
						   WLAN_VDEV_OP,
						   handler,
						   arg, 0,
						   WLAN_MBSS_ID);
err:
	status = QDF_STATUS_E_FAILURE;
	return status;
}

QDF_STATUS
mbss_start_ap_monitor_vdevs(struct wlan_objmgr_pdev *pdev, void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct mbss_pdev *mbss_ctx;
	wlan_objmgr_pdev_op_handler handler;
	struct wlan_mbss_ext_cb *ext_ops;

	mbss_ctx = mbss_get_pdev_ctx(pdev);
	if (!mbss_ctx) {
		mbss_err("MBSS ctx in null");
		goto err;
	}

	ext_ops = wlan_mbss_get_ext_ops();
	if (ext_ops && ext_ops->mbss_start_ap_monitor_vdevs_cb)
		handler = ext_ops->mbss_start_ap_monitor_vdevs_cb;
	else
		goto err;

	return wlan_objmgr_pdev_iterate_obj_list(pdev,
						   WLAN_VDEV_OP,
						   handler,
						   arg, 0,
						   WLAN_MBSS_ID);
err:
	status = QDF_STATUS_E_FAILURE;
	return status;
}

QDF_STATUS
mbss_stop_start_ap_vdevs(struct wlan_objmgr_pdev *pdev, void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct mbss_pdev *mbss_ctx;
	wlan_objmgr_pdev_op_handler handler;
	struct wlan_mbss_ext_cb *ext_ops;

	mbss_ctx = mbss_get_pdev_ctx(pdev);
	if (!mbss_ctx) {
		mbss_err("MBSS ctx in null");
		goto err;
	}

	ext_ops = wlan_mbss_get_ext_ops();
	if (ext_ops && ext_ops->mbss_stop_start_ap_vdevs_cb)
		handler = ext_ops->mbss_stop_start_ap_vdevs_cb;
	else
		goto err;

	return wlan_objmgr_pdev_iterate_obj_list(pdev,
						   WLAN_VDEV_OP,
						   handler,
						   arg, 0,
						   WLAN_MBSS_ID);
err:
	status = QDF_STATUS_E_FAILURE;
	return status;
}

QDF_STATUS
mbss_stop_start_ap_monitor_vdevs(struct wlan_objmgr_pdev *pdev, void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct mbss_pdev *mbss_ctx;
	wlan_objmgr_pdev_op_handler handler;
	struct wlan_mbss_ext_cb *ext_ops;

	mbss_ctx = mbss_get_pdev_ctx(pdev);
	if (!mbss_ctx) {
		mbss_err("MBSS ctx in null");
		goto err;
	}

	ext_ops = wlan_mbss_get_ext_ops();
	if (ext_ops && ext_ops->mbss_stop_start_ap_monitor_vdevs_cb)
		handler = ext_ops->mbss_stop_start_ap_monitor_vdevs_cb;
	else
		goto err;

	return wlan_objmgr_pdev_iterate_obj_list(pdev,
						   WLAN_VDEV_OP,
						   handler,
						   arg, 0,
						   WLAN_MBSS_ID);
err:
	status = QDF_STATUS_E_FAILURE;
	return status;
}

QDF_STATUS
mbss_start_restart_ap_monitor_vdevs(struct wlan_objmgr_pdev *pdev, void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct mbss_pdev *mbss_ctx;
	wlan_objmgr_pdev_op_handler handler;
	struct wlan_mbss_ext_cb *ext_ops;

	mbss_ctx = mbss_get_pdev_ctx(pdev);
	if (!mbss_ctx) {
		mbss_err("MBSS ctx in null");
		goto err;
	}

	ext_ops = wlan_mbss_get_ext_ops();
	if (ext_ops && ext_ops->mbss_start_restart_ap_monitor_vdevs_cb)
		handler = ext_ops->mbss_start_restart_ap_monitor_vdevs_cb;
	else
		goto err;

	return wlan_objmgr_pdev_iterate_obj_list(pdev,
						   WLAN_VDEV_OP,
						   handler,
						   arg, 0,
						   WLAN_MBSS_ID);
err:
	status = QDF_STATUS_E_FAILURE;
	return status;
}

