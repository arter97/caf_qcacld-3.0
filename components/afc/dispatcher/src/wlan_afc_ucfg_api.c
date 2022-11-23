/*
 * Copyright (c) 2022, Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.

 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * DOC: This file contains afc north bound interface definitions
 */

#include <qdf_types.h>
#include <wlan_reg_afc.h>
#include <qca_vendor.h>
#include <wlan_objmgr_cmn.h>
#include <wlan_objmgr_global_obj.h>
#include <wlan_objmgr_pdev_obj.h>
#include <wlan_afc_ucfg_api.h>
#include <wlan_cfg80211_afc.h>
#include <wlan_reg_ucfg_api.h>

#ifdef CONFIG_AFC_SUPPORT
/**
 * wlan_send_afc_partial_request() - Callback function to send AFC partial
 * request
 * @pdev: Pointer of pdev object
 * @afc_req: AFC partial request
 * @arg: Callback argument
 *
 * Return: None
 */
static void
wlan_send_afc_partial_request(struct wlan_objmgr_pdev *pdev,
			      struct wlan_afc_host_partial_request *afc_req,
			      void *arg)
{
	wlan_cfg80211_trigger_afc_event(
			pdev,
			QCA_WLAN_VENDOR_AFC_EVENT_TYPE_EXPIRY,
			afc_req);
}

/**
 * wlan_send_afc_power_event() - Callback function to send AFC power event
 * @pdev: Pointer of pdev object
 * @afc_pwr_evt: AFC power update event
 * @arg: Callback argument
 *
 * Return: None
 */
static void
wlan_send_afc_power_event(struct wlan_objmgr_pdev *pdev,
			  struct reg_fw_afc_power_event *afc_pwr_evt,
			  void *arg)
{
	wlan_cfg80211_trigger_afc_event(
			pdev,
			QCA_WLAN_VENDOR_AFC_EVENT_TYPE_POWER_UPDATE_COMPLETE,
			afc_pwr_evt);
}

/**
 * wlan_afc_pdev_obj_create_handler() - AFC callback for pdev create
 * @pdev: Pointer of pdev object
 * @arg: Callback argument
 *
 * Return: QDF STATUS
 */
static QDF_STATUS
wlan_afc_pdev_obj_create_handler(struct wlan_objmgr_pdev *pdev, void *arg)
{
	QDF_STATUS status;

	status = ucfg_reg_register_afc_req_rx_callback(
					pdev,
					wlan_send_afc_partial_request,
					NULL);
	if (QDF_IS_STATUS_ERROR(status))
		return status;

	status = ucfg_reg_register_afc_power_event_callback(
					pdev,
					wlan_send_afc_power_event,
					NULL);
	if (QDF_IS_STATUS_ERROR(status)) {
		ucfg_reg_unregister_afc_req_rx_callback(
					pdev,
					wlan_send_afc_partial_request);
		return status;
	}
	return status;
}

/**
 * wlan_afc_pdev_obj_destroy_handler() - AFC callback for pdev destroy
 * @pdev: Pointer of pdev object
 * @arg: Callback argument
 *
 * Return: QDF STATUS
 */
static QDF_STATUS
wlan_afc_pdev_obj_destroy_handler(struct wlan_objmgr_pdev *pdev, void *arg)
{
	ucfg_reg_unregister_afc_req_rx_callback(
					pdev,
					wlan_send_afc_partial_request);
	ucfg_reg_unregister_afc_power_event_callback(
					pdev,
					wlan_send_afc_power_event);
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS ucfg_afc_init(void)
{
	if (wlan_objmgr_register_pdev_create_handler(
	    WLAN_UMAC_COMP_AFC,
	    wlan_afc_pdev_obj_create_handler, NULL)
	    != QDF_STATUS_SUCCESS)
		return QDF_STATUS_E_FAILURE;

	if (wlan_objmgr_register_pdev_destroy_handler(
	    WLAN_UMAC_COMP_AFC,
	    wlan_afc_pdev_obj_destroy_handler, NULL)
	    != QDF_STATUS_SUCCESS)
		return QDF_STATUS_E_FAILURE;

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS ucfg_afc_deinit(void)
{
	if (wlan_objmgr_unregister_pdev_create_handler(
	    WLAN_UMAC_COMP_AFC,
	    wlan_afc_pdev_obj_create_handler, NULL)
	    != QDF_STATUS_SUCCESS)
		return QDF_STATUS_E_FAILURE;

	if (wlan_objmgr_unregister_pdev_destroy_handler(
	    WLAN_UMAC_COMP_AFC,
	    wlan_afc_pdev_obj_destroy_handler, NULL)
	    != QDF_STATUS_SUCCESS)
		return QDF_STATUS_E_FAILURE;

	return QDF_STATUS_SUCCESS;
}
#endif
