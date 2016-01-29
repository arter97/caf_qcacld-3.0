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

#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif

#include <osdep.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/if_arp.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sd.h>
#include <linux/wait.h>
#include <cdf_memory.h>
#include "bmi_msg.h"            /* TARGET_TYPE_ */
#include "if_sdio.h"
#include <cdf_trace.h>
#include <cds_api.h>
#include "regtable_sdio.h"
#include <hif_debug.h>
#ifndef REMOVE_PKT_LOG
#include "ol_txrx_types.h"
#include "pktlog_ac_api.h"
#include "pktlog_ac.h"
#endif
#include "epping_main.h"

#ifndef ATH_BUS_PM
#ifdef CONFIG_PM
#define ATH_BUS_PM
#endif /* CONFIG_PM */
#endif /* ATH_BUS_PM */

#ifndef REMOVE_PKT_LOG
struct ol_pl_os_dep_funcs *g_ol_pl_os_dep_funcs = NULL;
#endif
#define HIF_SDIO_LOAD_TIMEOUT 1000

struct ath_hif_sdio_softc *scn = NULL;
struct ol_softc *ol_sc;
static atomic_t hif_sdio_load_state;
/* Wait queue for MC thread */
wait_queue_head_t sync_wait_queue;

/**
 * hif_sdio_probe() - configure sdio device
 * @context: sdio device context
 * @hif_handle: pointer to hif handle
 *
 * Return: 0 for success and non-zero for failure
 */
static A_STATUS hif_sdio_probe(void *context, void *hif_handle)
{
	int ret = 0;
	struct HIF_DEVICE_OS_DEVICE_INFO os_dev_info;
	struct sdio_func *func = NULL;
	const struct sdio_device_id *id;
	uint32_t target_type;
	HIF_ENTER();

	scn = (struct ath_hif_sdio_softc *)cdf_mem_malloc(sizeof(*scn));
	if (!scn) {
		ret = -ENOMEM;
		goto err_alloc;
	}
	A_MEMZERO(scn, sizeof(*scn));

	scn->hif_handle = hif_handle;
	hif_configure_device(hif_handle, HIF_DEVICE_GET_OS_DEVICE,
			     &os_dev_info,
			     sizeof(os_dev_info));

	scn->aps_osdev.device = os_dev_info.os_dev;
	scn->aps_osdev.bc.bc_bustype = HAL_BUS_TYPE_SDIO;
	spin_lock_init(&scn->target_lock);
	ol_sc = cdf_mem_malloc(sizeof(*ol_sc));
	if (!ol_sc) {
		ret = -ENOMEM;
		goto err_attach;
	}
	OS_MEMZERO(ol_sc, sizeof(*ol_sc));

	{
		/*
		 * Attach Target register table. This is needed early on
		 * even before BMI since PCI and HIF initialization
		 * directly access Target registers.
		 *
		 * TBDXXX: targetdef should not be global -- should be stored
		 * in per-device struct so that we can support multiple
		 * different Target types with a single Host driver.
		 * The whole notion of an "hif type" -- (not as in the hif
		 * module, but generic "Host Interface Type") is bizarre.
		 * At first, one one expect it to be things like SDIO, USB, PCI.
		 * But instead, it's an actual platform type. Inexplicably, the
		 * values used for HIF platform types are *different* from the
		 * values used for Target Types.
		 */

#if defined(CONFIG_AR9888_SUPPORT)
		hif_register_tbl_attach(ol_sc, HIF_TYPE_AR9888);
		target_register_tbl_attach(ol_sc, TARGET_TYPE_AR9888);
		target_type = TARGET_TYPE_AR9888;
#elif defined(CONFIG_AR6320_SUPPORT)
		id = ((struct hif_sdio_dev *) hif_handle)->id;
		if ((id->device & MANUFACTURER_ID_AR6K_BASE_MASK) ==
				MANUFACTURER_ID_QCA9377_BASE) {
			hif_register_tbl_attach(ol_sc, HIF_TYPE_AR6320V2);
			target_register_tbl_attach(ol_sc, TARGET_TYPE_AR6320V2);
		} else if ((id->device & MANUFACTURER_ID_AR6K_BASE_MASK) ==
				MANUFACTURER_ID_AR6320_BASE) {
			int ar6kid = id->device & MANUFACTURER_ID_AR6K_REV_MASK;
			if (ar6kid >= 1) {
				/* v2 or higher silicon */
				hif_register_tbl_attach(ol_sc,
					HIF_TYPE_AR6320V2);
				target_register_tbl_attach(ol_sc,
					  TARGET_TYPE_AR6320V2);
			} else {
				/* legacy v1 silicon */
				hif_register_tbl_attach(ol_sc,
					HIF_TYPE_AR6320);
				target_register_tbl_attach(ol_sc,
					  TARGET_TYPE_AR6320);
			}
		}
		target_type = TARGET_TYPE_AR6320;

#endif
	}
	func = ((struct hif_sdio_dev *) hif_handle)->func;
	scn->targetdef =  ol_sc->targetdef;
	scn->hostdef =  ol_sc->hostdef;
	scn->aps_osdev.bdev = func;
	ol_sc->aps_osdev.bdev = func;
	ol_sc->aps_osdev.device = scn->aps_osdev.device;
	ol_sc->aps_osdev.bc.bc_handle = scn->aps_osdev.bc.bc_handle;
	ol_sc->aps_osdev.bc.bc_bustype = scn->aps_osdev.bc.bc_bustype;
	ol_sc->hif_sc = (void *)scn;
	scn->ol_sc = ol_sc;
	ol_sc->target_type = target_type;
	ol_sc->enableuartprint = 1;
	ol_sc->enablefwlog = 0;
	ol_sc->max_no_of_peers = 1;
	ol_sc->hif_hdl = hif_handle;

#ifndef TARGET_DUMP_FOR_NON_QC_PLATFORM
	ol_sc->ramdump_base = ioremap(RAMDUMP_ADDR, RAMDUMP_SIZE);
	ol_sc->ramdump_size = RAMDUMP_SIZE;
	if (ol_sc->ramdump_base == NULL) {
		ol_sc->ramdump_base = 0;
		ol_sc->ramdump_size = 0;
	}
#endif

	if (athdiag_procfs_init(scn) != 0) {
		CDF_TRACE(CDF_MODULE_ID_HIF, CDF_TRACE_LEVEL_ERROR,
			  "%s athdiag_procfs_init failed", __func__);
		ret = A_ERROR;
		goto err_attach1;
	}

	atomic_set(&hif_sdio_load_state, true);
	wake_up_interruptible(&sync_wait_queue);

	return 0;

err_attach1:
	cdf_mem_free(ol_sc);
err_attach:
	cdf_mem_free(scn);
	scn = NULL;
err_alloc:
	return ret;
}

/**
 * ol_ath_sdio_configure() - configure sdio device
 * @hif_sc: pointer to sdio softc structure
 * @dev: pointer to net device
 * @hif_handle: pointer to sdio function
 *
 * Return: 0 for success and non-zero for failure
 */
int
ol_ath_sdio_configure(void *hif_sc, struct net_device *dev,
		      hif_handle_t *hif_hdl)
{
	struct ath_hif_sdio_softc *sc = (struct ath_hif_sdio_softc *)hif_sc;
	int ret = 0;

	sc->aps_osdev.netdev = dev;
	*hif_hdl = sc->hif_handle;

	return ret;
}

/**
 * hif_sdio_remove() - remove sdio device
 * @conext: sdio device context
 * @hif_handle: pointer to sdio function
 *
 * Return: 0 for success and non-zero for failure
 */
static A_STATUS hif_sdio_remove(void *context, void *hif_handle)
{
	HIF_ENTER();

	if (!scn) {
		CDF_TRACE(CDF_MODULE_ID_HIF, CDF_TRACE_LEVEL_ERROR,
			  "Global SDIO context is NULL");
		return A_ERROR;
	}

	atomic_set(&hif_sdio_load_state, false);
	athdiag_procfs_remove();

#ifndef TARGET_DUMP_FOR_NON_QC_PLATFORM
	iounmap(scn->ol_sc->ramdump_base);
#endif

	if (scn && scn->ol_sc) {
		hif_deinit_cdf_ctx(scn->ol_sc);
		cdf_mem_free(scn->ol_sc);
		scn->ol_sc = NULL;
	}
	if (scn) {
		cdf_mem_free(scn);
		scn = NULL;
	}

	HIF_EXIT();

	return 0;
}

/**
 * hif_sdio_suspend() - sdio suspend routine
 * @context: sdio device context
 *
 * Return: 0 for success and non-zero for failure
 */
static A_STATUS hif_sdio_suspend(void *context)
{
	return 0;
}

/**
 * hif_sdio_resume() - sdio resume routine
 * @context: sdio device context
 *
 * Return: 0 for success and non-zero for failure
 */
static A_STATUS hif_sdio_resume(void *context)
{
	return 0;
}

/**
 * hif_sdio_power_change() - change power state of sdio bus
 * @conext: sdio device context
 * @config: power state configurartion
 *
 * Return: 0 for success and non-zero for failure
 */
static A_STATUS hif_sdio_power_change(void *context, A_UINT32 config)
{
	return 0;
}

/*
 * Module glue.
 */
#include <linux/version.h>
static char *version = "HIF (Atheros/multi-bss)";
static char *dev_info = "ath_hif_sdio";

/**
 * init_ath_hif_sdio() - initialize hif sdio callbacks
 * @param: none
 *
 * Return: 0 for success and non-zero for failure
 */
static int init_ath_hif_sdio(void)
{
	static int probed;
	A_STATUS status;
	struct osdrv_callbacks osdrv_callbacks;
	HIF_ENTER();

	A_MEMZERO(&osdrv_callbacks, sizeof(osdrv_callbacks));
	osdrv_callbacks.device_inserted_handler = hif_sdio_probe;
	osdrv_callbacks.device_removed_handler = hif_sdio_remove;
	osdrv_callbacks.device_suspend_handler = hif_sdio_suspend;
	osdrv_callbacks.device_resume_handler = hif_sdio_resume;
	osdrv_callbacks.device_power_change_handler = hif_sdio_power_change;

	if (probed)
		return -ENODEV;
	probed++;

	CDF_TRACE(CDF_MODULE_ID_HIF, CDF_TRACE_LEVEL_INFO, "%s %d", __func__,
		  __LINE__);
	status = hif_init(&osdrv_callbacks);
	if (status != A_OK) {
		CDF_TRACE(CDF_MODULE_ID_HIF, CDF_TRACE_LEVEL_FATAL,
			  "%s hif_init failed!", __func__);
		return -ENODEV;
	}
	CDF_TRACE(CDF_MODULE_ID_HIF, CDF_TRACE_LEVEL_ERROR,
		 "%s: %s\n", dev_info, version);

	return 0;
}

/**
 * hif_register_driver() - initialize hif sdio
 * @param: none
 *
 * Return: 0 for success and non-zero for failure
 */
int hif_register_driver(void)
{
	int status = 0;

	HIF_ENTER();
	status = init_ath_hif_sdio();
	HIF_EXIT("status = %d", status);

	return status;

}

void hif_unregister_driver(void)
{
	HIF_ENTER();
	hif_shutdown_device(NULL);
	HIF_EXIT();

	return;
}

/**
 * hif_bus_prevent_linkdown(): prevent linkdown
 *
 * Dummy function for busses and platforms that do not support
 * link down.  This may need to be replaced with a wakelock.
 *
 * This is duplicated here because CONFIG_CNSS can be defined
 * even though it is not used for the sdio bus.
 */
void hif_bus_prevent_linkdown(bool flag)
{
	HIF_ERROR("wlan: %s power collapse ignored",
			(flag ? "disable" : "enable"));
}

/**
 * hif_targ_is_awake(): check if target is awake
 *
 * This function returns true if the target is awake
 *
 * @scn: struct ol_softc
 * @mem: mapped mem base
 *
 * Return: bool
 */
bool hif_targ_is_awake(struct ol_softc *scn, void *__iomem *mem)
{
	return true;
}

/**
 * hif_reset_soc(): reset soc
 *
 * this function resets soc
 *
 * @hif_ctx: HIF context
 *
 * Return: void
 */
/* Function to reset SoC */
void hif_reset_soc(void *hif_ctx)
{
}

/**
 * hif_disable_isr(): disable isr
 *
 * This function disables isr and kills tasklets
 *
 * @hif_ctx: struct ol_softc
 *
 * Return: void
 */
void hif_disable_isr(void *hif_ctx)
{

}

/**
 * hif_bus_suspend() - suspend the bus
 *
 * This function suspends the bus, but sdio doesn't need to suspend.
 * Therefore do nothing.
 *
 * Return: 0 for success and non-zero for failure
 */
int hif_bus_suspend(void)
{
	struct hif_sdio_dev *device = scn->hif_handle;

	struct device *dev = &device->func->dev;

	hif_device_suspend(dev);

	return 0;
}

/**
 * hif_bus_resume() - hif resume API
 *
 * This function resumes the bus. but sdio doesn't need to resume.
 * Therefore do nothing.
 *
 * Return: 0 for success and non-zero for failure
 */
int hif_bus_resume(void)
{
	struct hif_sdio_dev *device = scn->hif_handle;

	struct device *dev = &device->func->dev;

	hif_device_resume(dev);

	return 0;
}

/**
 * hif_enable_power_gating() - enable HW power gating
 *
 * Return: n/a
 */
void hif_enable_power_gating(void *hif_ctx)
{
}

/**
 * hif_disable_aspm() - hif_disable_aspm
 *
 * Return: n/a
 */
void hif_disable_aspm(void)
{
}

/**
 * hif_bus_close() - hif_bus_close
 *
 * Return: n/a
 */
void hif_bus_close(struct ol_softc *ol_sc)
{
	struct ath_hif_sdio_softc *sc;

	if (ol_sc == NULL) {
		HIF_ERROR("%s: ol_softc is NULL", __func__);
		return;
	}
	sc = ol_sc->hif_sc;
	if (sc == NULL)
		return;
	cdf_mem_free(sc);
	ol_sc->hif_sc = NULL;
}

/**
 * hif_bus_open() - hif_bus_open
 * @scn: scn
 * @bus_type: bus type
 *
 * Return: n/a
 */
CDF_STATUS hif_bus_open(struct ol_softc *ol_sc, enum ath_hal_bus_type bus_type)
{
	struct ath_hif_sdio_softc *sc;
	A_STATUS status;

	status = init_ath_hif_sdio();
	sc = cdf_mem_malloc(sizeof(*sc));
	if (!sc) {
		HIF_ERROR("%s: no mem", __func__);
		return CDF_STATUS_E_NOMEM;
	}
	ol_sc->hif_sc = (void *)sc;
	sc->ol_sc = ol_sc;
	ol_sc->bus_type = bus_type;

	return CDF_STATUS_SUCCESS;
}

/**
 * hif_get_target_type() - Get the target type
 *
 * This function is used to query the target type.
 *
 * @ol_sc: ol_softc struct pointer
 * @dev: device pointer
 * @bdev: bus dev pointer
 * @bid: bus id pointer
 * @hif_type: HIF type such as HIF_TYPE_QCA6180
 * @target_type: target type such as TARGET_TYPE_QCA6180
 *
 * Return: 0 for success
 */
int hif_get_target_type(struct ol_softc *ol_sc, struct device *dev,
	void *bdev, const hif_bus_id *bid, uint32_t *hif_type,
	uint32_t *target_type)
{

	return 0;
}

void hif_get_target_revision(struct ol_softc *ol_sc)
{
	struct ol_softc *ol_sc_local = (struct ol_softc *)ol_sc;
	uint32_t chip_id = 0;
	A_STATUS rv;

	rv = hif_diag_read_access(ol_sc_local->hif_hdl,
			(CHIP_ID_ADDRESS | RTC_SOC_BASE_ADDRESS), &chip_id);
	if (rv != A_OK) {
		HIF_ERROR("%s[%d]: get chip id fail\n", __func__, __LINE__);
	} else {
		ol_sc_local->target_revision =
			CHIP_ID_REVISION_GET(chip_id);
	}
}

/**
 * hif_enable_bus() - hif_enable_bus
 * @dev: dev
 * @bdev: bus dev
 * @bid: bus id
 * @type: bus type
 *
 * Return: CDF_STATUS
 */
CDF_STATUS hif_enable_bus(struct ol_softc *ol_scn,
			  struct device *dev, void *bdev,
			  const hif_bus_id *bid,
			  enum hif_enable_type type)
{
	int ret = 0;
	const struct sdio_device_id *id = bid;

	init_waitqueue_head(&sync_wait_queue);
	if (hif_sdio_device_inserted(dev, id)) {
			HIF_ERROR("wlan: %s hif_sdio_device_inserted"
					  "failed", __func__);
		return CDF_STATUS_E_NOMEM;
	}

	ol_scn = cds_get_context(CDF_MODULE_ID_HIF);
	if (!ol_scn) {
		HIF_ERROR("%s: hif_ctx is NULL", __func__);
		return CDF_STATUS_E_NOMEM;
	}

	wait_event_interruptible_timeout(sync_wait_queue,
			  atomic_read(&hif_sdio_load_state) == true,
			  HIF_SDIO_LOAD_TIMEOUT);
	memcpy(ol_scn, ol_sc, sizeof(*ol_sc));

	return ret;
}

/**
 * hif_disable_bus() - hif_disable_bus
 *
 * This function disables the bus
 *
 * @bdev: bus dev
 *
 * Return: none
 */
void hif_disable_bus(void *bdev)
{
	struct sdio_func *func = bdev;

	hif_sdio_device_removed(func);
}

/**
 * hif_nointrs() - disable IRQ
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
 * hif_dump_registers() - dump bus debug registers
 *
 * This function dumps hif bus debug registers
 *
 * @scn: struct ol_softc
 *
 * Return: 0 for success or error code
 */
int hif_dump_registers(struct ol_softc *scn)
{
	return 0;
}

/**
 * hif_bus_pkt_dl_len_set()- set the HTT packet download length
 * @hif_sc: HIF context
 * @pkt_download_len: download length
 *
 * Return: None
 */
void hif_bus_pkt_dl_len_set(void *hif_sc, u_int32_t pkt_download_len)
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
 * hif_check_fw_reg() - hif_check_fw_reg
 * @scn: scn
 * @state:
 *
 * Return: int
 */
int hif_check_fw_reg(struct ol_softc *scn)
{
	return 0;
}

/**
 * hif_wlan_disable() - call the platform driver to disable wlan
 *
 *
 * Return: void
 */
void hif_wlan_disable(void)
{
}

/**
 * hif_config_target() - configure hif bus
 * @hif_hdl: hif handle
 * @state:
 *
 * Return: int
 */
int hif_config_target(void *hif_hdl)
{
	return 0;
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

/**
 * hif_bus_configure() - configure the sdio bus
 * @scn: pointer to the hif context.
 *
 * return: 0 for success. nonzero for failure.
 */
int hif_bus_configure(struct ol_softc *scn)
{
	return 0;
}

