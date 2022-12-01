/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * This file contains the API definitions for the Unified Wireless Module
 * Interface (WMI) specific to 11be.
 */

#ifndef _WMI_UNIFIED_11BE_API_H_
#define _WMI_UNIFIED_11BE_API_H_

#include <wmi_unified_api.h>
#include <wmi_unified_priv.h>

#ifdef WLAN_FEATURE_11BE_MLO
/**
 * wmi_extract_mlo_link_set_active_resp() - extract mlo link set active
 *  response event
 * @wmi: wmi handle
 * @evt_buf: pointer to event buffer
 * @evt: Pointer to hold mlo link set active response event
 *
 * This function gets called to extract mlo link set active response event
 *
 * Return: QDF_STATUS_SUCCESS on success and QDF_STATUS_E_FAILURE for failure
 */
QDF_STATUS
wmi_extract_mlo_link_set_active_resp(wmi_unified_t wmi,
				     void *evt_buf,
				     struct mlo_link_set_active_resp *evt);

/**
 * wmi_send_mlo_link_set_active_cmd() - send mlo link set active command
 * @wmi: WMI handle for this pdev
 * @param: Pointer to mlo link set active param
 *
 * Return: QDF_STATUS code
 */
QDF_STATUS
wmi_send_mlo_link_set_active_cmd(wmi_unified_t wmi_handle,
				 struct mlo_link_set_active_param *param);

/**
 * wmi_extract_mgmt_rx_ml_cu_params() - extract mlo cu params from event
 * @wmi_handle: wmi handle
 * @evt_buf: pointer to event buffer
 * @cu_params: Pointer to mlo CU params
 *
 * Return: QDF_STATUS_SUCCESS on success and QDF_STATUS_E_FAILURE for failure
 */
QDF_STATUS
wmi_extract_mgmt_rx_ml_cu_params(wmi_unified_t wmi_handle, void *evt_buf,
				 struct mlo_mgmt_ml_info *cu_params);
#endif /*WLAN_FEATURE_11BE_MLO*/

#ifdef WLAN_FEATURE_11BE
/**
 * wmi_send_mlo_peer_tid_to_link_map_cmd() - send TID-to-link mapping command
 * @wmi: WMI handle for this pdev
 * @params: Pointer to TID-to-link mapping params
 */
QDF_STATUS wmi_send_mlo_peer_tid_to_link_map_cmd(
		wmi_unified_t wmi,
		struct wmi_host_tid_to_link_map_params *params);

/**
 * wmi_send_mlo_vdev_tid_to_link_map_cmd() - send TID-to-link mapping command
 *                                           per vdev
 * @wmi: WMI handle for this pdev
 * @params: Pointer to TID-to-link mapping params
 */
QDF_STATUS wmi_send_mlo_vdev_tid_to_link_map_cmd(
		wmi_unified_t wmi,
		struct wmi_host_tid_to_link_map_ap_params *params);

/**
 * wmi_extract_mlo_vdev_tid_to_link_map_event() - extract mlo t2lm info for vdev
 * @wmi: wmi handle
 * @evt_buf: pointer to event buffer
 * @resp: Pointer to host structure to get the t2lm info
 *
 * This function gets called to extract mlo t2lm info for particular pdev
 *
 * Return: QDF_STATUS_SUCCESS on success and QDF_STATUS_E_FAILURE for failure
 */
QDF_STATUS
wmi_extract_mlo_vdev_tid_to_link_map_event(
		wmi_unified_t wmi, void *evt_buf,
		struct mlo_vdev_host_tid_to_link_map_resp *resp);

/**
 * wmi_extract_mlo_vdev_bcast_tid_to_link_map_event() - extract bcast mlo t2lm
 *                                                      info for vdev
 * @wmi: wmi handle
 * @evt_buf: pointer to event buffer
 * @bcast: Pointer to host structure to get the t2lm bcast info
 *
 * This function gets called to extract bcast mlo t2lm info for particular pdev
 *
 * Return: QDF_STATUS_SUCCESS on success and QDF_STATUS_E_FAILURE for failure
 */
QDF_STATUS
wmi_extract_mlo_vdev_bcast_tid_to_link_map_event(
				     wmi_unified_t wmi,
				     void *evt_buf,
				     struct wmi_host_bcast_t2lm_info *bcast);
#endif /* WLAN_FEATURE_11BE */

#endif /*_WMI_UNIFIED_11BE_API_H_*/
