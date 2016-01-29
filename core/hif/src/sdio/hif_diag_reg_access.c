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

#include "athdefs.h"
#include "a_types.h"
#include "a_osapi.h"
#define ATH_MODULE_NAME hif
#include "a_debug.h"

#include "targaddrs.h"
#include "hif.h"
#include "if_sdio.h"
#include "regtable_sdio.h"

#define CPU_DBG_SEL_ADDRESS                      0x00000483
#define CPU_DBG_ADDRESS                          0x00000484

/* set the window address register (using 4-byte register access ).
* This mitigates host interconnect issues with non-4byte aligned bus requests,
* some interconnects use bus adapters that impose strict limitations.
* Since diag window access is not intended for performance critical operations,
* the 4byte mode should be satisfactory as it generates 4X the bus activity.  */
static A_STATUS
ar6000_set_address_window_register(struct hif_sdio_dev *hifDevice,
						   A_UINT32 RegisterAddr,
						   A_UINT32 Address)
{
	A_STATUS status;
	static A_UINT32 address;
	address = Address;

	/*AR6320,just write the 4-byte address to window register*/
	status = hif_read_write(hifDevice,
				RegisterAddr,
				(A_UCHAR *) (&address),
				4, HIF_WR_SYNC_BYTE_INC, NULL);

	if (status != A_OK) {
		AR_DEBUG_PRINTF(ATH_LOG_ERR,
			("Cannot write 0x%x to window reg: 0x%X\n",
			 Address, RegisterAddr));
		return status;
	}

	return A_OK;
}

/*
 * Read from the AR6000 through its diagnostic window.
 * No cooperation from the Target is required for this.
 */
CDF_STATUS
hif_diag_read_access(struct ol_softc *scn, A_UINT32 address,
		 A_UINT32 *data)
{
	A_STATUS status;
	static A_UINT32 readvalue;
	struct hif_sdio_dev *hif_device = (struct hif_sdio_dev *)scn->hif_hdl;

	if (address & 0x03) {
		AR_DEBUG_PRINTF(ATH_LOG_ERR,
			("[%s]addr is not 4 bytes align.addr[0x%08x]\n",
			 __func__, address));
		return A_BAD_ADDRESS;
	}

	/* set window register to start read cycle */
	status = ar6000_set_address_window_register(hif_device,
						    WINDOW_READ_ADDR_ADDRESS,
						    address);

	if (status != A_OK)
		return status;

	/* read the data */
	status = hif_read_write(hif_device,
				WINDOW_DATA_ADDRESS,
				(A_UCHAR *) &readvalue,
				sizeof(A_UINT32), HIF_RD_SYNC_BYTE_INC, NULL);
	if (status != A_OK) {
		AR_DEBUG_PRINTF(ATH_LOG_ERR,
			("Cannot read from WINDOW_DATA_ADDRESS\n"));
		return status;
	}

	*data = readvalue;
	return status;
}

/*
 * Write to the AR6000 through its diagnostic window.
 * No cooperation from the Target is required for this.
 */
CDF_STATUS hif_diag_write_access(struct ol_softc *scn, A_UINT32 address,
			       A_UINT32 data)
{
	A_STATUS status;
	static A_UINT32 writeValue;
	struct hif_sdio_dev *hif_device = (struct hif_sdio_dev *)scn->hif_hdl;

	if (address & 0x03) {
		AR_DEBUG_PRINTF(ATH_LOG_ERR,
			("[%s]addr is not 4 bytes align.addr[0x%08x]\n",
			 __func__, address));
		return A_BAD_ADDRESS;
	}

	writeValue = data;

	/* set write data */
	status = hif_read_write(hif_device,
				WINDOW_DATA_ADDRESS,
				(A_UCHAR *) &writeValue,
				sizeof(A_UINT32), HIF_WR_SYNC_BYTE_INC, NULL);
	if (status != A_OK) {
		AR_DEBUG_PRINTF(ATH_LOG_ERR,
			("Cannot write 0x%x to WINDOW_DATA_ADDRESS\n",
			 data));
		return status;
	}

	/* set window register, which starts the write cycle */
	return ar6000_set_address_window_register(hif_device,
						  WINDOW_WRITE_ADDR_ADDRESS,
						  address);
}

/*
 * Write a block data to the AR6000 through its diagnostic window.
 * This function may take some time.
 * No cooperation from the Target is required for this.
 */
CDF_STATUS
hif_diag_write_mem(struct ol_softc *scn, A_UINT32 address, A_UINT8 *data,
		   int nbytes)
{
	CDF_STATUS status;
	A_INT32 i;
	A_UINT32 tmp_data;

	if ((address & 0x03) || (nbytes & 0x03)) {
		AR_DEBUG_PRINTF(ATH_LOG_ERR,
			("[%s]addr or length is not 4 bytes"
			 " align.addr[0x%08x] len[0x%08x]\n",
			 __func__, address, nbytes));
		return A_BAD_ADDRESS;
	}

	for (i = 0; i < nbytes; i += 4) {
		tmp_data =
			data[i] | (data[i + 1] << 8) | (data[i + 2] << 16) |
			(data[i + 3] << 24);
		status = hif_diag_write_access(scn, address + i, tmp_data);
		if (status != CDF_STATUS_SUCCESS) {
			AR_DEBUG_PRINTF(ATH_LOG_ERR,
				("Diag Write mem failed.addr[0x%08x]"
				 " value[0x%08x]\n",
				 address + i, tmp_data));
			return status;
		}
	}

	return CDF_STATUS_SUCCESS;
}

/*
 * Reaad a block data to the AR6000 through its diagnostic window.
 * This function may take some time.
 * No cooperation from the Target is required for this.
 */
CDF_STATUS
hif_diag_read_mem(struct ol_softc *scn, A_UINT32 address, A_UINT8 *data,
		  int nbytes)
{
	CDF_STATUS status;
	A_INT32 i;
	A_UINT32 tmp_data;

	if ((address & 0x03) || (nbytes & 0x03)) {
		AR_DEBUG_PRINTF(ATH_LOG_ERR,
			("[%s]addr or length is not 4 bytes"
			" align.addr[0x%08x] len[0x%08x]\n",
			 __func__, address, nbytes));
		return A_BAD_ADDRESS;
	}

	for (i = 0; i < nbytes; i += 4) {
		status = hif_diag_read_access(scn, address + i, &tmp_data);
		if (status != CDF_STATUS_SUCCESS) {
			AR_DEBUG_PRINTF(ATH_LOG_ERR,
					("Diag Write mem failed.addr[0x%08x]"
					 " value[0x%08x]\n",
					 address + i, tmp_data));
			return status;
		}
		data[i] = tmp_data & 0xff;
		data[i + 1] = tmp_data >> 8 & 0xff;
		data[i + 2] = tmp_data >> 16 & 0xff;
		data[i + 3] = tmp_data >> 24 & 0xff;
	}

	return CDF_STATUS_SUCCESS;
}

A_STATUS
ar6k_read_target_register(struct hif_sdio_dev *hifDevice, int regsel,
		 A_UINT32 *regval)
{
	A_STATUS status;
	A_UCHAR vals[4];
	A_UCHAR register_selection[4];

	register_selection[0] = register_selection[1] = register_selection[2] =
					register_selection[3] = (regsel & 0xff);
	status =
	      hif_read_write(hifDevice, CPU_DBG_SEL_ADDRESS, register_selection,
			       4, HIF_WR_SYNC_BYTE_FIX, NULL);

	if (status != A_OK) {
		AR_DEBUG_PRINTF(ATH_LOG_ERR,
			("Cannot write CPU_DBG_SEL (%d)\n", regsel));
		return status;
	}

	status = hif_read_write(hifDevice,
				CPU_DBG_ADDRESS,
				(A_UCHAR *) vals,
				sizeof(vals), HIF_RD_SYNC_BYTE_INC, NULL);
	if (status != A_OK) {
		AR_DEBUG_PRINTF(ATH_LOG_ERR,
				("Cannot read from CPU_DBG_ADDRESS\n"));
		return status;
	}

	*regval = vals[0] << 0 | vals[1] << 8 |
			vals[2] << 16 | vals[3] << 24;

	return status;
}

void ar6k_fetch_target_regs(struct hif_sdio_dev *hifDevice,
		 A_UINT32 *targregs)
{
	int i;
	A_UINT32 val;

	for (i = 0; i < AR6003_FETCH_TARG_REGS_COUNT; i++) {
		val = 0xffffffff;
		(void)ar6k_read_target_register(hifDevice, i, &val);
		targregs[i] = val;
	}
}
