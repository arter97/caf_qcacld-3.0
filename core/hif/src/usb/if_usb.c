/*
 * Copyright (c) 2013-2016 The Linux Foundation. All rights reserved.
 *
 * Previously licensed under the ISC license by Qualcomm Atheros, Inc.
 *
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

/*
 * This file was originally distributed by Qualcomm Atheros, Inc.
 * under proprietary terms before Copyright ownership was assigned
 * to the Linux Foundation.
 */

#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include "if_usb.h"
#include "hif_usb_internal.h"
#include "bmi_msg.h"		/* TARGET_TYPE_ */
#include "regtable_usb.h"
#include "ol_fw.h"
#include "hif_debug.h"
#include "epping_main.h"
#include "hif_main.h"

#if !defined(REMOVE_PKT_LOG)
#include "ol_txrx_types.h"
#include "pktlog_ac_api.h"
#include "pktlog_ac.h"
#include "cds_concurrency.h"
#define USB_PKT_LOG_MOD_INIT(ol_sc) \
{ \
	ol_txrx_pdev_handle pdev_txrx_handle; \
	pdev_txrx_handle = cds_get_context(CDF_MODULE_ID_TXRX); \
	if (cds_get_conparam() != CDF_GLOBAL_FTM_MODE && \
		!WLAN_IS_EPPING_ENABLED(cds_get_conparam())) { \
		ol_pl_sethandle(&pdev_txrx_handle->pl_dev, ol_sc); \
		if (pktlogmod_init(ol_sc)) \
			HIF_ERROR("%s: pktlogmod_init failed", __func__); \
	} \
}
#else
#define USB_PKT_LOG_MOD_INIT(ol_sc)
#endif
#define DELAY_FOR_TARGET_READY 200	/* 200ms */
static int hif_usb_unload_dev_num = -1;
struct hif_usb_softc *usb_sc = NULL;

/**
 * hif_diag_write_cold_reset() - reset SOC by sending a diag command
 * @scn: pointer to ol_softc structure
 *
 * Return: CDF_STATUS_SUCCESS if success else an appropriate CDF_STATUS error
 */
static inline CDF_STATUS
hif_diag_write_cold_reset(struct ol_softc *scn)
{

	HIF_DBG("%s: resetting SOC", __func__);

	return hif_diag_write_access(scn,
				(ROME_USB_SOC_RESET_CONTROL_COLD_RST_LSB |
				ROME_USB_RTC_SOC_BASE_ADDRESS),
				SOC_RESET_CONTROL_COLD_RST_SET(1));
}

/**
 * hif_usb_configure() - create HIF_USB_DEVICE, populate hif_hdl, init procfs
 * @scn: pointer to ol_softc structure
 * @hif_hdl: pointer to hif_handle_t structure to be populated
 * @interface: pointer to usb_interface being initialized
 *
 * Return: int 0 if success else an appropriate error number
 */
static int
hif_usb_configure(struct hif_usb_softc *sc, hif_handle_t *hif_hdl,
		struct usb_interface *interface)
{
	int ret = 0;
	struct usb_device *dev = interface_to_usbdev(interface);

	HIF_ENTER();
	if (hif_enable_usb(interface, sc)) {
		HIF_ERROR("ath: %s: Target probe failed", __func__);
		ret = -EIO;
		goto err_stalled;
	}

	if (athdiag_procfs_init(sc->ol_sc) != 0) {
		HIF_ERROR("athdiag_procfs_init failed");
		return A_ERROR;
	}
	sc->ol_sc->athdiag_procfs_inited = true;

	hif_usb_unload_dev_num = dev->devnum;
	*hif_hdl = (hif_handle_t *)(usb_get_intfdata(interface));
	HIF_EXIT();
	return 0;

err_stalled:

	return ret;
}

/**
 * hif_nointrs(): disable IRQ
 *
 * This function stops interrupt(s)
 *
 * @scn: struct ol_softc
 *
 * Return: none
 */
void hif_nointrs(struct ol_softc *scn)
{

}

/**
 * hif_usb_reboot() - called at reboot time to reset WLAN SOC
 * @nb: pointer to notifier_block registered during register_reboot_notifier
 * @val: code indicating reboot reason
 * @v: unused pointer
 *
 * Return: int 0 if success else an appropriate error number
 */
static int hif_usb_reboot(struct notifier_block *nb, unsigned long val,
				void *v)
{
	struct hif_usb_softc *sc;

	HIF_ENTER();
	sc = container_of(nb, struct hif_usb_softc, reboot_notifier);
	/* do cold reset */
	hif_diag_write_cold_reset(sc->ol_sc);
	HIF_EXIT();
	return NOTIFY_DONE;
}

/**
 * hif_usb_disable_lpm() - Disable lpm feature of usb2.0
 * @udev: pointer to usb_device for which LPM is to be disabled
 *
 * LPM needs to be disabled to avoid usb2.0 probe timeout
 *
 * Return: int 0 if success else an appropriate error number
 */
static int hif_usb_disable_lpm(struct usb_device *udev)
{
	struct usb_hcd *hcd;
	int ret = -EPERM;

	HIF_ENTER();
	if (!udev || !udev->bus) {
		HIF_ERROR("Invalid input parameters");
	} else {
		hcd = bus_to_hcd(udev->bus);
		if (udev->usb2_hw_lpm_enabled) {
			if (hcd->driver->set_usb2_hw_lpm) {
				ret = hcd->driver->set_usb2_hw_lpm(hcd,
							udev, false);
				if (!ret) {
					udev->usb2_hw_lpm_enabled = false;
					udev->usb2_hw_lpm_capable = false;
					HIF_TRACE("%s: LPM is disabled",
								__func__);
				} else {
					HIF_TRACE("%s: Fail to disable LPM",
								__func__);
				}
			} else {
				HIF_TRACE("%s: hcd doesn't support LPM",
							__func__);
			}
		} else {
			HIF_TRACE("%s: LPM isn't enabled", __func__);
		}
	}

	HIF_EXIT();
	return ret;
}

/**
 * hif_enable_bus() - enable usb bus
 * @ol_sc: soft_sc struct
 * @dev: device pointer
 * @bdev: bus dev pointer
 * @bid: bus id pointer
 * @type: enum hif_enable_type such as HIF_ENABLE_TYPE_PROBE
 *
 * Return: CDF_STATUS_SUCCESS on success and error CDF status on failure
 */
CDF_STATUS hif_enable_bus(struct ol_softc *ol_sc,
			struct device *dev, void *bdev,
			const hif_bus_id *bid,
			enum hif_enable_type type)

{
	struct usb_interface *interface = (struct usb_interface *)bdev;
	struct usb_device_id *id = (struct usb_device_id *)bid;
	int ret = 0;
	struct hif_usb_softc *sc;
	struct usb_device *pdev = interface_to_usbdev(interface);
	int vendor_id, product_id;

	HIF_ENTER();

	if (!ol_sc) {
		HIF_ERROR("%s: hif_ctx is NULL", __func__);
		goto err_alloc;
	}

	sc = ol_sc->hif_sc;

	if (!sc) {
		HIF_ERROR("%s: usb hif_ctx is NULL", __func__);
		goto err_alloc;
	}

	usb_get_dev(pdev);
	vendor_id = cdf_le16_to_cpu(pdev->descriptor.idVendor);
	product_id = cdf_le16_to_cpu(pdev->descriptor.idProduct);

	ret = 0;

	sc->pdev = (void *)pdev;
	sc->dev = &pdev->dev;

	ol_sc->aps_osdev.bdev = bdev;
	ol_sc->aps_osdev.device = &pdev->dev;

	sc->devid = id->idProduct;
	if ((usb_control_msg(pdev, usb_sndctrlpipe(pdev, 0),
			USB_REQ_SET_CONFIGURATION, 0, 1, 0, NULL, 0,
			HZ)) < 0) {
		HIF_ERROR("%s[%d]", __func__, __LINE__);
	}
	usb_set_interface(pdev, 0, 0);
	/* disable lpm to avoid usb2.0 probe timeout */
	hif_usb_disable_lpm(pdev);

	if (hif_usb_configure(sc, &ol_sc->hif_hdl, interface))
		goto err_config;

	ol_sc->enableuartprint = 1;
	ol_sc->enablefwlog = 0;
	ol_sc->max_no_of_peers = 1;

	sc->interface = interface;
	sc->reboot_notifier.notifier_call = hif_usb_reboot;
	register_reboot_notifier(&sc->reboot_notifier);

	usb_sc = sc;
	HIF_EXIT();
	return 0;

err_config:
	hif_diag_write_cold_reset(sc->ol_sc);
	cdf_mem_free(ol_sc);
	ret = CDF_STATUS_E_FAILURE;
	usb_sc = NULL;
	cdf_mem_free(sc);
err_alloc:
	usb_put_dev(pdev);

	return ret;
}


/**
 * hif_bus_close(): close bus, delete hif_sc
 * @ol_sc: soft_sc struct
 *
 * Return: none
 */
void hif_bus_close(struct ol_softc *ol_sc)
{
	struct hif_usb_softc *sc;

	if (ol_sc == NULL) {
		HIF_ERROR("%s: ol_softc is NULL", __func__);
		return;
	}
	sc = ol_sc->hif_sc;
	if (sc == NULL)
		return;
	cdf_mem_free(sc);
	ol_sc->hif_sc = NULL;
	usb_sc = NULL;
}

/**
 * hif_disable_bus(): This function disables the bus
 * @bdev: pointer to usb_interface
 *
 * Return: none
 */
void hif_disable_bus(void *bdev)
{
	struct usb_interface *interface = (struct usb_interface *)bdev;
	HIF_DEVICE_USB *device = usb_get_intfdata(interface);
	struct usb_device *udev = interface_to_usbdev(interface);
	struct hif_usb_softc *sc = device->sc;
	struct ol_softc *scn = sc->ol_sc;

	/* Attach did not succeed, all resources have been
	 * freed in error handler
	 */
	if (!sc)
		return;

	HIF_TRACE("%s: trying to remove hif_usb!", __func__);

	/* disable lpm to avoid following cold reset will
	 * cause xHCI U1/U2 timeout
	 */
	usb_disable_lpm(udev);

	/* wait for disable lpm */
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(msecs_to_jiffies(DELAY_FOR_TARGET_READY));
	set_current_state(TASK_RUNNING);

	/* do cold reset */
	hif_diag_write_cold_reset(scn);

	if (usb_sc->suspend_state)
		hif_bus_resume();

	unregister_reboot_notifier(&sc->reboot_notifier);
	usb_put_dev(interface_to_usbdev(interface));

	hif_disable_usb(interface, 1);

	HIF_TRACE("%s hif_usb removed !!!!!!", __func__);
}

/**
 * hif_bus_suspend() - suspend the bus
 *
 * This function suspends the bus, but usb doesn't need to suspend.
 * Therefore just remove all the pending urb transactions
 *
 * Return: 0 for success and non-zero for failure
 */
int hif_bus_suspend(void)
{
	struct ol_softc *scn = cds_get_context(CDF_MODULE_ID_HIF);
	HIF_DEVICE_USB *device = scn->hif_hdl;
	struct hif_usb_softc *sc = device->sc;

	HIF_ENTER();
	sc->suspend_state = 1;
	usb_hif_flush_all(device);
	HIF_EXIT();
	return 0;
}

/**
 * hif_bus_resume() - hif resume API
 *
 * This function resumes the bus. but usb doesn't need to resume.
 * Post recv urbs for RX data pipe
 *
 * Return: 0 for success and non-zero for failure
 */
int hif_bus_resume(void)
{
	struct ol_softc *scn = cds_get_context(CDF_MODULE_ID_HIF);
	HIF_DEVICE_USB *device = scn->hif_hdl;
	struct hif_usb_softc *sc = device->sc;

	HIF_ENTER();
	sc->suspend_state = 0;
	usb_hif_start_recv_pipes(device);

	HIF_EXIT();
	return 0;
}

/**
 * hif_bus_reset_resume() - resume the bus after reset
 *
 * This function is called to tell the driver that USB device has been resumed
 * and it has also been reset. The driver should redo any necessary
 * initialization. This function resets WLAN SOC.
 *
 * Return: int 0 for success, non zero for failure
 */
int hif_bus_reset_resume(void)
{
	struct ol_softc *scn = cds_get_context(CDF_MODULE_ID_HIF);
	int ret = 0;

	HIF_ENTER();
	if (hif_diag_write_cold_reset(scn) != CDF_STATUS_SUCCESS)
		ret = 1;

	HIF_EXIT();
	return ret;
}

/**
 * hif_bus_open()- initialization routine for usb bus
 * @ol_sc: ol_sc
 * @bus_type: bus type
 *
 * Return: CDF_STATUS_SUCCESS on success and error CDF status on failure
 */
CDF_STATUS hif_bus_open(struct ol_softc *ol_sc,
				enum ath_hal_bus_type bus_type)
{
	struct hif_usb_softc *sc;

	sc = cdf_mem_malloc(sizeof(*sc));
	if (!sc) {
		HIF_ERROR("%s: no mem", __func__);
		return CDF_STATUS_E_NOMEM;
	}
	ol_sc->hif_sc = (void *)sc;
	sc->ol_sc = ol_sc;
	ol_sc->bus_type = bus_type;
	ol_sc->aps_osdev.bc.bc_bustype = bus_type;

	return CDF_STATUS_SUCCESS;
}
void hif_disable_isr(void *ol_sc)
{
	/* TODO */
}

/* Function to reset SoC */
void hif_reset_soc(void *ol_sc)
{
	/* TODO */
}

/**
 * hif_usb_reg_tbl_attach()- attach hif, target register tables
 * @scn: pointer to ol_softc structure
 *
 * Attach host and target register tables based on target_type, target_version
 *
 * Return: none
 */
void hif_usb_reg_tbl_attach(struct ol_softc *ol_sc)
{
	u_int32_t hif_type, target_type;
	int32_t ret = 0;
	uint32_t chip_id;
	CDF_STATUS rv;
	struct ol_softc *scn = ol_sc;

	if (ol_sc->hostdef == NULL && ol_sc->targetdef == NULL) {
		switch (((struct ol_softc *)ol_sc)->target_type) {
		case TARGET_TYPE_AR6320:
			switch (((struct ol_softc *)ol_sc)->target_version) {
			case AR6320_REV1_VERSION:
			case AR6320_REV1_1_VERSION:
			case AR6320_REV1_3_VERSION:
				hif_type = HIF_TYPE_AR6320;
				target_type = TARGET_TYPE_AR6320;
				break;
			case AR6320_REV2_1_VERSION:
			case AR6320_REV3_VERSION:
			case QCA9377_REV1_1_VERSION:
				hif_type = HIF_TYPE_AR6320V2;
				target_type = TARGET_TYPE_AR6320V2;
				break;
			default:
				ret = -1;
				break;
			}
			break;
		default:
			ret = -1;
			break;
		}

		if (!ret) {
			/* assign target register table if we find
			corresponding type */
			hif_register_tbl_attach(ol_sc, hif_type);
			target_register_tbl_attach(ol_sc, target_type);
			/* read the chip revision*/
			rv = hif_diag_read_access(ol_sc,
						(CHIP_ID_ADDRESS |
						RTC_SOC_BASE_ADDRESS),
						&chip_id);
			if (rv != CDF_STATUS_SUCCESS) {
				HIF_ERROR("%s: get chip id val (%d)", __func__,
					rv);
			}
			((struct ol_softc *)ol_sc)->target_revision =
						CHIP_ID_REVISION_GET(chip_id);
		}
	}

}

/**
 * hif_usb_get_hw_info()- returns target_version, target_revision
 * @scn: pointer to ol_softc structure
 * @version: pointer to version information to be populated
 * @revision: pointer to revision information to be populated

 * This function is also used to attach the host and target register tables.
 * Ideally, we should not attach register tables as a part of this function.
 * There is scope of cleanup to move register table attach during
 * initialization for USB bus.
 *
 * The reason we are doing register table attach for USB here is that, it relies
 * on ol_sc->target_type and ol_sc->target_version, which get populated during
 * bmi_firmware_download. "hif_get_fw_info" is the only initialization related
 * call into HIF there after.
 *
 * To fix this, we can move the "get target info, functionality currently in
 * bmi_firmware_download into hif initialization functions. This change will
 * affect all buses. Can be taken up as a part of convergence.
 *
 * Return: none
 */
void hif_usb_get_hw_info(struct ol_softc *ol_sc, uint32_t *version,
							uint32_t *revision)
{

	hif_usb_reg_tbl_attach(ol_sc);

	/* we need to get chip revision here */
	*version = ((struct ol_softc *)ol_sc)->target_version;
	/* Chip version should be supported, set to 0 for now */
	*revision = ((struct ol_softc *)ol_sc)->target_revision;
}

/**
 * hif_target_sleep_state_adjust() - on-demand sleep/wake
 * @scn: ol_softc pointer.
 * @sleep_ok: bool
 * @wait_for_it: bool
 *
 * Return: int
 */
void hif_target_sleep_state_adjust(struct ol_softc *scn,
						bool sleep_ok, bool wait_for_it)
{
	return;
}

#ifndef QCA_WIFI_3_0
/**
 * hif_check_fw_reg(): hif_check_fw_reg
 * @scn: pointer to ol_softc structure
 *
 * Return: int
 */
int hif_check_fw_reg(struct ol_softc *scn)
{
	return 0;
}
#endif

/**
 * hif_bus_prevent_linkdown(): allow or permit linkdown
 * @scn: pointer to ol_softc structure
 * @flag: true prevents linkdown, false allows
 *
 * Return: n/a
 */
void hif_bus_prevent_linkdown(struct ol_softc *scn, bool flag)
{

}

/**
 * hif_pkt_log_init(): initialize pkt log proc directory entry
 * @hif_ctx: pointer to ol_softc structure
 *
 * Return: n/a
 */
void hif_pkt_log_init(void *hif_ctx)
{
	struct ol_softc *scn = hif_ctx;

	if (NULL == scn) {
		HIF_ERROR("%s: Could not enable pkt log, scn is null",
			__func__);
		return;
	}

	if (scn->pkt_log_init == false) {
		USB_PKT_LOG_MOD_INIT(scn);
		scn->pkt_log_init = true;
	}
}

/**
 * hif_enable_power_gating() - enable HW power gating
 * @hif_ctx: pointer to ol_softc structure
 *
 * No power gating needed for USB. This function is also used to initialize pkt
 * log proc entry
 *
 * Return: n/a
 */
void hif_enable_power_gating(void *hif_ctx)
{
	hif_pkt_log_init(hif_ctx);
}

/**
 * hif_bus_pkt_dl_len_set() set the HTT packet download length
 * @hif_sc: HIF context
 * @pkt_download_len: download length
 *
 * Return: None
 */
void hif_bus_pkt_dl_len_set(void *hif_sc, u_int32_t pkt_download_len)
{

}

/**
 * hif_disable_aspm(): hif_disable_aspm
 *
 * Return: n/a
 */
void hif_disable_aspm(void)
{
}

/**
 * hif_trigger_dump() - trigger various dump cmd
 * @scn: struct ol_softc
 * @cmd_id: dump command id
 * @start: start/stop dump
 *
 * Return: None
 */
void hif_trigger_dump(struct ol_softc *scn, uint8_t cmd_id, bool start)
{
}

/**
 * hif_bus_configure() - configure the bus
 * @scn: pointer to the hif context.
 *
 * return: 0 for success. nonzero for failure.
 */
int hif_bus_configure(struct ol_softc *scn)
{
	return 0;
}

/**
 * hif_wlan_disable() - call the platform driver to disable wlan
 *
 * This function passes the con_mode to platform driver to disable
 * wlan.
 *
 * Return: void
 */
void hif_wlan_disable(void)
{
}

/**
 * hif_shut_down_device() - This function shuts down the device
 * @scn: pointer to ol_softc structure
 *
 * Return: void
 */
void hif_shutdown_device(struct ol_softc *scn)
{
}

/**
 * hif_disable_power_management() - disable power management
 * @hif_ctx: hif handle
 *
 * Return: none
 */
void hif_disable_power_management(void *hif_ctx)
{
}

/**
 * hif_enable_power_management() - disable power management
 * @hif_ctx: hif handle
 *
 * Return: none
 */
void hif_enable_power_management(void *hif_ctx)
{
}
