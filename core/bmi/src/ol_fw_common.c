/*
 * Copyright (c) 2014-2016 The Linux Foundation. All rights reserved.
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

#include "ol_if_athvar.h"
#include "targaddrs.h"
#include "ol_cfg.h"
#include "i_ar6320v2_regtable.h"
#include "ol_fw.h"
#ifdef HIF_PCI
#include "ce_reg.h"
#elif defined(HIF_SDIO)
#include "if_sdio.h"
#include "regtable_sdio.h"
#endif
#if  defined(CONFIG_CNSS)
#include <net/cnss.h>
#endif
#include "i_bmi.h"

#if defined(HIF_SDIO) || defined(HIF_USB)
static struct ol_fw_files FW_FILES_QCA6174_FW_1_1 = {
	"qwlan11.bin", "bdwlan11.bin", "otp11.bin", "utf11.bin",
	"utfbd11.bin", "qsetup11.bin", "epping11.bin"};
static struct ol_fw_files FW_FILES_QCA6174_FW_2_0 = {
	"qwlan20.bin", "bdwlan20.bin", "otp20.bin", "utf20.bin",
	"utfbd20.bin", "qsetup20.bin", "epping20.bin"};
static struct ol_fw_files FW_FILES_QCA6174_FW_1_3 = {
	"qwlan13.bin", "bdwlan13.bin", "otp13.bin", "utf13.bin",
	"utfbd13.bin", "qsetup13.bin", "epping13.bin"};
static struct ol_fw_files FW_FILES_QCA6174_FW_3_0 = {
	"qwlan30.bin", "bdwlan30.bin", "otp30.bin", "utf30.bin",
	"utfbd30.bin", "qsetup30.bin", "epping30.bin"};
static struct ol_fw_files FW_FILES_DEFAULT = {
	"qwlan.bin", "bdwlan.bin", "otp.bin", "utf.bin",
	"utfbd.bin", "qsetup.bin", "epping.bin"};

static int ol_get_fw_files_for_target(struct ol_fw_files *pfw_files,
		uint32_t target_version)
{
	if (!pfw_files)
		return -ENODEV;

	switch (target_version) {
	case AR6320_REV1_VERSION:
	case AR6320_REV1_1_VERSION:
		memcpy(pfw_files, &FW_FILES_QCA6174_FW_1_1, sizeof(*pfw_files));
		break;
	case AR6320_REV1_3_VERSION:
		memcpy(pfw_files, &FW_FILES_QCA6174_FW_1_3, sizeof(*pfw_files));
		break;
	case AR6320_REV2_1_VERSION:
		memcpy(pfw_files, &FW_FILES_QCA6174_FW_2_0, sizeof(*pfw_files));
		break;
	case AR6320_REV3_VERSION:
	case AR6320_REV3_2_VERSION:
		memcpy(pfw_files, &FW_FILES_QCA6174_FW_3_0, sizeof(*pfw_files));
		break;
	case QCA9377_REV1_1_VERSION:
#ifdef CONFIG_TUFELLO_DUAL_FW_SUPPORT
	memcpy(pfw_files, &FW_FILES_DEFAULT, sizeof(*pfw_files));
#else
	memcpy(pfw_files, &FW_FILES_QCA6174_FW_3_0, sizeof(*pfw_files));
#endif
	break;
	default:
		memcpy(pfw_files, &FW_FILES_DEFAULT, sizeof(*pfw_files));
		BMI_ERR("%s version mismatch 0x%X ",
			__func__, target_version);
	break;
	}

	return 0;
}
#else
static inline int ol_get_fw_files_for_target(struct cnss_fw_files *pfw_files,
		uint32_t target_version)
{
	return 0;
}
#endif
#if defined(CONFIG_CNSS) && defined(HIF_PCI)
int ol_get_fw_files(struct ol_softc *scn)
{
	int status = 0;

	switch (scn->aps_osdev.bc.bc_bustype) {
	case HAL_BUS_TYPE_PCI:
		if (0 !=
		   cnss_get_fw_files_for_target((struct cnss_fw_files *)&scn->fw_files,
						  scn->target_type,
						  scn->target_version)) {
			BMI_ERR("%s: No FW files from CNSS driver", __func__);
			status = CDF_STATUS_E_FAILURE;
		}
		break;
	default:
		BMI_ERR("%s: No FW files from CNSS driver for bus type %u",
			__func__, scn->aps_osdev.bc.bc_bustype);
		status = CDF_STATUS_E_FAILURE;
		break;
	}

	return status;
}
#else
int ol_get_fw_files(struct ol_softc *scn)
{
	int status = 0;

	switch (scn->aps_osdev.bc.bc_bustype) {
	case HAL_BUS_TYPE_SDIO:
	case HAL_BUS_TYPE_USB:
		if (0 != ol_get_fw_files_for_target(&scn->fw_files,
				scn->target_version)) {
			BMI_ERR("%s: No FW files from driver\n", __func__);
			status = CDF_STATUS_E_FAILURE;
		}
		break;
	default:
		BMI_ERR("%s: No FW files for bus type %u", __func__,
			scn->aps_osdev.bc.bc_bustype);
		status = CDF_STATUS_E_FAILURE;
		break;
	}
	return status;
}
#endif
/* AXI Start Address */
#define TARGET_ADDR (0xa0000)
#ifdef CONFIG_CODESWAP_FEATURE
void ol_transfer_codeswap_struct(struct ol_softc *scn)
{
	struct codeswap_codeseg_info wlan_codeswap;
	CDF_STATUS rv;

	if (scn->aps_osdev.bc.bc_bustype != HAL_BUS_TYPE_PCI)
		return;

	if (!scn) {
		BMI_ERR("%s: ol_softc is null", __func__);
		return;
	}
	if (cnss_get_codeswap_struct(&wlan_codeswap)) {
		BMI_ERR("%s: failed to get codeswap structure",
				__func__);
		return;
	}

	rv = bmi_write_memory(TARGET_ADDR,
		(uint8_t *) &wlan_codeswap, sizeof(wlan_codeswap),
			scn);

	if (rv != CDF_STATUS_SUCCESS) {
		BMI_ERR("Failed to Write 0xa0000 to Target");
		return;
	}
	BMI_INFO("codeswap structure is successfully downloaded");
}
#endif

#ifdef CONFIG_DISABLE_SLEEP_BMI_OPTION
static inline void ol_sdio_disable_sleep(struct ol_softc *scn)
{
	uint32_t value;

	BMI_ERR("prevent ROME from sleeping");
	bmi_read_soc_register(MBOX_BASE_ADDRESS + LOCAL_SCRATCH_OFFSET,
		/* this address should be 0x80C0 for ROME*/
		&value,
		scn);

	value |= SOC_OPTION_SLEEP_DISABLE;

	bmi_write_soc_register(MBOX_BASE_ADDRESS + LOCAL_SCRATCH_OFFSET,
				 value,
				 scn);
}
#else
static inline void ol_sdio_disable_sleep(struct ol_softc *scn)
{
}
#endif


/*Setting SDIO block size, mbox ISR yield limit for SDIO based HIF*/
static int ol_sdio_extra_initialization(struct ol_softc *scn)
{

	int status;
	uint32_t param;

	uint32_t blocksizes[HTC_MAILBOX_NUM_MAX];
	uint32_t MboxIsrYieldValue = 99;
	/* get the block sizes */
	status = hif_configure_device(scn->hif_hdl,
				HIF_DEVICE_GET_MBOX_BLOCK_SIZE,
				blocksizes, sizeof(blocksizes));
	if (status != EOK) {
		BMI_ERR("Failed to get block size info from HIF layer");
		goto exit;
	}
	/* note: we actually get the block size for mailbox 1,
	 * for SDIO the block size on mailbox 0 is artificially
	 * set to 1 must be a power of 2 */
	cdf_assert((blocksizes[1] & (blocksizes[1] - 1)) == 0);

	/* set the host interest area for the block size */
	status = bmi_write_memory(hif_hia_item_address(scn->target_type,
				 offsetof(struct host_interest_s,
				 hi_mbox_io_block_sz)),
				(uint8_t *)&blocksizes[1],
				4,
				scn);

	if (status != EOK) {
		BMI_ERR("BMIWriteMemory for IO block size failed");
		goto exit;
	}

	if (MboxIsrYieldValue != 0) {
		/* set the host for the mbox ISR yield limit */
		status =
		bmi_write_memory(hif_hia_item_address(scn->target_type,
				offsetof(struct host_interest_s,
				hi_mbox_isr_yield_limit)),
				(uint8_t *)&MboxIsrYieldValue,
				4,
				scn);

		if (status != EOK) {
			BMI_ERR("BMI write for yield limit failed\n");
			goto exit;
		}
	}
	ol_sdio_disable_sleep(scn);
	status = bmi_read_memory(hif_hia_item_address(scn->target_type,
			offsetof(struct host_interest_s,
			hi_acs_flags)),
			(uint8_t *)&param,
			4,
			scn);
	if (status != EOK) {
		BMI_ERR("BMIReadMemory for hi_acs_flags failed");
		goto exit;
	}

	param |= (HI_ACS_FLAGS_SDIO_SWAP_MAILBOX_SET|
			 HI_ACS_FLAGS_SDIO_REDUCE_TX_COMPL_SET|
			 HI_ACS_FLAGS_ALT_DATA_CREDIT_SIZE);

	bmi_write_memory(hif_hia_item_address(scn->target_type,
			offsetof(struct host_interest_s,
			hi_acs_flags)),
			(uint8_t *)&param, 4, scn);
exit:
	return status;
}

static CDF_STATUS
ol_usb_extra_initialization(struct ol_softc *scn)
{
	CDF_STATUS status = !CDF_STATUS_SUCCESS;
	u_int32_t param = 0;

	param |= HI_ACS_FLAGS_ALT_DATA_CREDIT_SIZE;
	status = bmi_write_memory(
				hif_hia_item_address(scn->target_type,
				offsetof(struct host_interest_s, hi_acs_flags)),
				(u_int8_t *)&param, 4, scn);

	return status;
}
int ol_extra_initialization(struct ol_softc *scn)
{
	if (scn->aps_osdev.bc.bc_bustype == HAL_BUS_TYPE_SDIO)
		return ol_sdio_extra_initialization(scn);
	else if (scn->aps_osdev.bc.bc_bustype == HAL_BUS_TYPE_USB)
		return ol_usb_extra_initialization(scn);
	return 0;
}

void ol_target_ready(struct ol_softc *scn, void *cfg_ctx)
{
	u_int32_t value = 0;
	int status = EOK;

	if (scn->aps_osdev.bc.bc_bustype != HAL_BUS_TYPE_SDIO)
		return;
	status = hif_diag_read_mem(scn,
		hif_hia_item_address(scn->target_type,
		offsetof(struct host_interest_s, hi_acs_flags)),
		(uint8_t *)&value, sizeof(u_int32_t));

	if (status != EOK) {
		BMI_ERR("HIFDiagReadMem failed");
		return;
	}

	if (value & HI_ACS_FLAGS_SDIO_SWAP_MAILBOX_FW_ACK) {
		BMI_ERR("MAILBOX SWAP Service is enabled!");
		hif_set_mailbox_swap(scn->hif_hdl);
	}

	if (value & HI_ACS_FLAGS_SDIO_REDUCE_TX_COMPL_FW_ACK)
		BMI_ERR("Reduced Tx Complete service is enabled!");
}
