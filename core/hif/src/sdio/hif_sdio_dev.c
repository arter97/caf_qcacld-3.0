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

#define ATH_MODULE_NAME hif
#include "a_debug.h"

#include <cdf_types.h>
#include <cdf_status.h>
#include <cdf_softirq_timer.h>
#include <cdf_time.h>
#include <cdf_lock.h>
#include <cdf_memory.h>
#include <cdf_util.h>
#include <cdf_defer.h>
#include <cdf_atomic.h>
#include <cdf_nbuf.h>
#include <athdefs.h>
#include <cdf_net_types.h>
#include <a_types.h>
#include <athdefs.h>
#include <a_osapi.h>
#include <hif.h>
#include <htc_services.h>
#include "hif_sdio_internal.h"
#include "if_sdio.h"
#include "regtable_sdio.h"

/* under HL SDIO, with Interface Memory support, we have
 * the following reasons to support 2 mboxs:
 * a) we need place different buffers in different
 * mempool, for example, data using Interface Memory,
 * desc and other using DRAM, they need different SDIO
 * mbox channels.
 * b) currently, tx mempool in LL case is seperated from
 * main mempool, the structure (descs at the beginning
 * of every pool buffer) is different, because they only
 * need store tx desc from host. To align with LL case,
 * we also need 2 mbox support just as PCIe LL cases.
 */

#define INVALID_MAILBOX_NUMBER 0xFF
/**
 * hif_dev_map_pipe_to_mail_box() - maps pipe id to mailbox.
 * @pDev: sdio device context
 * @pipeid: pipe index
 *
 *
 * Return: mailbox index
 */
A_UINT8 hif_dev_map_pipe_to_mail_box(HIF_SDIO_DEVICE *pDev,
			A_UINT8 pipeid)
{
	/* TODO: temp version, should not hardcoded here, will be
	 * updated after HIF design */
	if (2 == pipeid || 3 == pipeid)
		return 1;
	else if (0 == pipeid || 1 == pipeid)
		return 0;
	else {
		AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
			("%s: pipeid=%d,should not happen\n",
			 __func__, pipeid));
		cdf_assert(0);
		return INVALID_MAILBOX_NUMBER;
	}
}

/**
 * hif_dev_map_mail_box_to_pipe() - map sdio mailbox to htc pipe.
 * @pdev: sdio device
 * @mboxIndex: mailbox index
 * @upload: boolean to decide mailbox index
 *
 * Disable hif device interrupts and destroy hif context
 *
 * Return: none
 */
A_UINT8 hif_dev_map_mail_box_to_pipe(HIF_SDIO_DEVICE *pDev,
			A_UINT8 mboxIndex,
				     A_BOOL upload)
{
	/* TODO: temp version, should not hardcoded here, will be
	 * updated after HIF design */
	if (mboxIndex == 0) {
		return upload ? 1 : 0;
	} else if (mboxIndex == 1) {
		return upload ? 3 : 2;
	} else {
		AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
			("%s:--------------------mboxIndex=%d,upload=%d,"
			 " should not happen\n",
			__func__, mboxIndex, upload));
		cdf_assert(0);
		return 0xff;
	}
}

/**
 * hif_dev_map_service_to_pipe() - maps ul/dl pipe to service id.
 * @pDev: sdio device context
 * @ServiceId: sevice index
 * @ULPipe: uplink pipe id
 * @DLPipe: down-linklink pipe id
 * @SwapMapping: mailbox swap mapping
 *
 * Return: int
 */
A_STATUS hif_dev_map_service_to_pipe(HIF_SDIO_DEVICE *pDev,
				     A_UINT16 ServiceId,
				     A_UINT8 *ULPipe, A_UINT8 *DLPipe,
				     A_BOOL SwapMapping)
{
	A_STATUS status = EOK;
	switch (ServiceId) {
	case HTT_DATA_MSG_SVC:
		if (SwapMapping) {
			*ULPipe = 1;
			*DLPipe = 0;
		} else {
		*ULPipe = 3;
		*DLPipe = 2;
		}
		break;

	case HTC_CTRL_RSVD_SVC:
	case HTC_RAW_STREAMS_SVC:
		*ULPipe = 1;
		*DLPipe = 0;
		break;

	case WMI_DATA_BE_SVC:
	case WMI_DATA_BK_SVC:
	case WMI_DATA_VI_SVC:
	case WMI_DATA_VO_SVC:
		*ULPipe = 1;
		*DLPipe = 0;
		break;

	case WMI_CONTROL_SVC:
		if (SwapMapping) {
			*ULPipe = 3;
			*DLPipe = 2;
		} else {
		*ULPipe = 1;
		*DLPipe = 0;
		}
		break;

	default:
		status = !EOK;
		break;
	}
	return status;
}

/**
 * hif_dev_alloc_rx_buffer() - allocate rx buffer.
 * @pDev: sdio device context
 *
 *
 * Return: htc buffer pointer
 */
HTC_PACKET *hif_dev_alloc_rx_buffer(HIF_SDIO_DEVICE *pDev)
{
	HTC_PACKET *pPacket;
	cdf_nbuf_t netbuf;
	A_UINT32 bufsize = 0, headsize = 0;

	bufsize = HIF_SDIO_RX_BUFFER_SIZE + HIF_SDIO_RX_DATA_OFFSET;
	headsize = sizeof(HTC_PACKET);
	netbuf = cdf_nbuf_alloc(NULL, bufsize + headsize, 0, 4, false);
	if (netbuf == NULL) {
		AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
				("(%s)Allocate netbuf failed\n", __func__));
		return NULL;
	}
	pPacket = (HTC_PACKET *) cdf_nbuf_data(netbuf);
	cdf_nbuf_reserve(netbuf, headsize);

	SET_HTC_PACKET_INFO_RX_REFILL(pPacket,
				      pDev,
				      cdf_nbuf_data(netbuf),
				      bufsize, ENDPOINT_0);
	SET_HTC_PACKET_NET_BUF_CONTEXT(pPacket, netbuf);
	return pPacket;
}

/**
 * hif_dev_create() - create hif device after probe.
 * @scn: HIF context
 * @callbacks: htc callbacks
 * @target: HIF target
 *
 *
 * Return: int
 */
HIF_SDIO_DEVICE *hif_dev_create(struct hif_sdio_dev *hif_device,
			struct hif_msg_callbacks *callbacks, void *target)
{

	A_STATUS status;
	HIF_SDIO_DEVICE *pDev;

	pDev = cdf_mem_malloc(sizeof(HIF_SDIO_DEVICE));
	if (!pDev) {
		A_ASSERT(false);
		return NULL;
	}

	A_MEMZERO(pDev, sizeof(HIF_SDIO_DEVICE));
	cdf_spinlock_init(&pDev->Lock);
	cdf_spinlock_init(&pDev->TxLock);
	cdf_spinlock_init(&pDev->RxLock);

	pDev->HIFDevice = hif_device;
	pDev->pTarget = target;
	status = hif_configure_device(hif_device,
				      HIF_DEVICE_SET_HTC_CONTEXT,
				      (void *)pDev, sizeof(pDev));
	if (status != A_OK) {
		AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
				("(%s)HIF_DEVICE_SET_HTC_CONTEXT failed!!!\n",
				 __func__));
	}

	A_MEMCPY(&pDev->hif_callbacks, callbacks, sizeof(*callbacks));

	return pDev;
}

/**
 * hif_dev_destroy() - destroy hif device.
 * @pDev: sdio device context
 *
 *
 * Return: none
 */
void hif_dev_destroy(HIF_SDIO_DEVICE *pDev)
{
	A_STATUS status;

	status = hif_configure_device(pDev->HIFDevice,
				      HIF_DEVICE_SET_HTC_CONTEXT,
				      (void *)NULL, 0);
	if (status != A_OK) {
		AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
				("(%s)HIF_DEVICE_SET_HTC_CONTEXT failed!!!\n",
				 __func__));
	}
	cdf_mem_free(pDev);
}

/**
 * hif_dev_from_hif() - get sdio device from hif device.
 * @pDev: hif device context
 *
 *
 * Return: hif sdio device context
 */
HIF_SDIO_DEVICE *hif_dev_from_hif(struct hif_sdio_dev *hif_device)
{
	HIF_SDIO_DEVICE *pDev = NULL;
	A_STATUS status;
	status = hif_configure_device(hif_device,
				HIF_DEVICE_GET_HTC_CONTEXT,
				(void **)&pDev, sizeof(HIF_SDIO_DEVICE));
	if (status != A_OK) {
		AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
				("(%s)HTC_SDIO_CONTEXT is NULL!!!\n",
				 __func__));
	}
	return pDev;
}

/**
 * hif_dev_disable_interrupts() - disable hif device interrupts.
 * @pDev: sdio device context
 *
 *
 * Return: int
 */
A_STATUS hif_dev_disable_interrupts(HIF_SDIO_DEVICE *pDev)
{
	struct MBOX_IRQ_ENABLE_REGISTERS regs;
	A_STATUS status = A_OK;
	HIF_ENTER();

	LOCK_HIF_DEV(pDev);
	/* Disable all interrupts */
	pDev->IrqEnableRegisters.int_status_enable = 0;
	pDev->IrqEnableRegisters.cpu_int_status_enable = 0;
	pDev->IrqEnableRegisters.error_status_enable = 0;
	pDev->IrqEnableRegisters.counter_int_status_enable = 0;
	/* copy into our temp area */
	A_MEMCPY(&regs,
		 &pDev->IrqEnableRegisters, sizeof(pDev->IrqEnableRegisters));

	UNLOCK_HIF_DEV(pDev);

	/* always synchronous */
	status = hif_read_write(pDev->HIFDevice,
				INT_STATUS_ENABLE_ADDRESS,
				(A_UCHAR *) &regs,
				sizeof(struct MBOX_IRQ_ENABLE_REGISTERS),
				HIF_WR_SYNC_BYTE_INC, NULL);

	if (status != A_OK) {
		/* Can't write it for some reason */
		AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
			("Failed to update interrupt control registers err: %d",
			 status));
	}

    /* To Do mask the host controller interrupts */
	hif_mask_interrupt(pDev->HIFDevice);
	HIF_EXIT("status :%d", status);
	return status;
}

/**
 * hif_dev_enable_interrupts() - enables hif device interrupts.
 * @pDev: sdio device context
 *
 *
 * Return: int
 */
A_STATUS hif_dev_enable_interrupts(HIF_SDIO_DEVICE *pDev)
{
	A_STATUS status;
	struct MBOX_IRQ_ENABLE_REGISTERS regs;
	HIF_ENTER();

	/* for good measure, make sure interrupt are disabled
	 * before unmasking at the HIF layer.
	 * The rationale here is that between device insertion
	 * (where we clear the interrupts the first time)
	 * and when HTC is finally ready to handle interrupts,
	 * other software can perform target "soft" resets.
	 * The AR6K interrupt enables reset back to an "enabled"
	 * state when this happens. */
	hif_dev_disable_interrupts(pDev);

	/* Unmask the host controller interrupts */
	hif_un_mask_interrupt(pDev->HIFDevice);

	LOCK_HIF_DEV(pDev);

	/* Enable all the interrupts except for the internal
	 * AR6000 CPU interrupt */
	pDev->IrqEnableRegisters.int_status_enable =
		INT_STATUS_ENABLE_ERROR_SET(0x01) |
			INT_STATUS_ENABLE_CPU_SET(0x01)
		| INT_STATUS_ENABLE_COUNTER_SET(0x01);

		/* enable 2 mboxs INT */
	pDev->IrqEnableRegisters.int_status_enable |=
			INT_STATUS_ENABLE_MBOX_DATA_SET(0x01) |
			INT_STATUS_ENABLE_MBOX_DATA_SET(0x02);

	/* Set up the CPU Interrupt Status Register, enable
	 * CPU sourced interrupt #0, #1.
	 * #0 is used for report assertion from target
	 * #1 is used for inform host that credit arrived
	 * */
	pDev->IrqEnableRegisters.cpu_int_status_enable = 0x03;

	/* Set up the Error Interrupt Status Register */
	pDev->IrqEnableRegisters.error_status_enable =
		(ERROR_STATUS_ENABLE_RX_UNDERFLOW_SET(0x01)
		 | ERROR_STATUS_ENABLE_TX_OVERFLOW_SET(0x01)) >> 16;

	/* Set up the Counter Interrupt Status Register
	 * (only for debug interrupt to catch fatal errors) */
	pDev->IrqEnableRegisters.counter_int_status_enable =
	   (COUNTER_INT_STATUS_ENABLE_BIT_SET(AR6K_TARGET_DEBUG_INTR_MASK)) >>
		24;

	/* copy into our temp area */
	A_MEMCPY(&regs,
		 &pDev->IrqEnableRegisters,
		 sizeof(struct MBOX_IRQ_ENABLE_REGISTERS));

	UNLOCK_HIF_DEV(pDev);

	/* always synchronous */
	status = hif_read_write(pDev->HIFDevice,
				INT_STATUS_ENABLE_ADDRESS,
				(A_UCHAR *) &regs,
				sizeof(struct MBOX_IRQ_ENABLE_REGISTERS),
				HIF_WR_SYNC_BYTE_INC, NULL);

	if (status != A_OK) {
		/* Can't write it for some reason */
		AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
		  ("Failed to update interrupt control registers err: %d\n",
				 status));

	}
	HIF_EXIT();
	return status;
}

/**
 * hif_dev_setup() - set up sdio device.
 * @pDev: sdio device context
 *
 *
 * Return: int
 */
A_STATUS hif_dev_setup(HIF_SDIO_DEVICE *pDev)
{
	A_STATUS status;
	A_UINT32 blocksizes[MAILBOX_COUNT];
	HTC_CALLBACKS htc_callbacks;
	struct hif_sdio_dev *hif_device = pDev->HIFDevice;

	HIF_ENTER();

	status = hif_configure_device(hif_device,
				      HIF_DEVICE_GET_MBOX_ADDR,
				      &pDev->MailBoxInfo,
				      sizeof(pDev->MailBoxInfo));

	if (status != A_OK) {
		AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
				("(%s)HIF_DEVICE_GET_MBOX_ADDR failed!!!\n",
				 __func__));
		A_ASSERT(false);
	}

	status = hif_configure_device(hif_device,
				      HIF_DEVICE_GET_MBOX_BLOCK_SIZE,
				      blocksizes, sizeof(blocksizes));
	if (status != A_OK) {
		AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
			("(%s)HIF_DEVICE_GET_MBOX_BLOCK_SIZE failed!!!\n",
				 __func__));
		A_ASSERT(false);
	}

	pDev->BlockSize = blocksizes[MAILBOX_FOR_BLOCK_SIZE];
	pDev->BlockMask = pDev->BlockSize - 1;
	A_ASSERT((pDev->BlockSize & pDev->BlockMask) == 0);

	/* assume we can process HIF interrupt events asynchronously */
	pDev->HifIRQProcessingMode = HIF_DEVICE_IRQ_ASYNC_SYNC;

	/* see if the HIF layer overrides this assumption */
	hif_configure_device(hif_device,
			     HIF_DEVICE_GET_IRQ_PROC_MODE,
			     &pDev->HifIRQProcessingMode,
			     sizeof(pDev->HifIRQProcessingMode));

	switch (pDev->HifIRQProcessingMode) {
	case HIF_DEVICE_IRQ_SYNC_ONLY:
		AR_DEBUG_PRINTF(ATH_DEBUG_WARN,
			("HIF Interrupt processing is SYNC ONLY\n"));
		/* see if HIF layer wants HTC to yield */
		hif_configure_device(hif_device,
				     HIF_DEVICE_GET_IRQ_YIELD_PARAMS,
				     &pDev->HifIRQYieldParams,
				     sizeof(pDev->HifIRQYieldParams));

		if (pDev->HifIRQYieldParams.recv_packet_yield_count > 0) {
			AR_DEBUG_PRINTF(ATH_DEBUG_WARN,
				("HIF req of DSR yield per %d RECV packets\n",
				 pDev->HifIRQYieldParams.
				 recv_packet_yield_count));
			pDev->DSRCanYield = true;
		}
		break;
	case HIF_DEVICE_IRQ_ASYNC_SYNC:
		AR_DEBUG_PRINTF(ATH_DEBUG_TRC,
			("HIF Interrupt processing is ASYNC and SYNC\n"));
		break;
	default:
		A_ASSERT(false);
		break;
	}

	pDev->HifMaskUmaskRecvEvent = NULL;

	/* see if the HIF layer implements the mask/unmask recv
	 * events function  */
	hif_configure_device(hif_device,
			     HIF_DEVICE_GET_RECV_EVENT_MASK_UNMASK_FUNC,
			     &pDev->HifMaskUmaskRecvEvent,
			     sizeof(pDev->HifMaskUmaskRecvEvent));

	status = hif_dev_disable_interrupts(pDev);

	A_MEMZERO(&htc_callbacks, sizeof(HTC_CALLBACKS));
	/* the device layer handles these */
	htc_callbacks.rwCompletionHandler = hif_dev_rw_completion_handler;
	htc_callbacks.dsrHandler = hif_dev_dsr_handler;
	htc_callbacks.context = pDev;
	status = hif_attach_htc(pDev->HIFDevice, &htc_callbacks);

	HIF_EXIT();
	return status;
}
