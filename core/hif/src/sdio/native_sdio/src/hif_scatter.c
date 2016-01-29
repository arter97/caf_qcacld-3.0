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

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio.h>
#include <linux/kthread.h>
#include "hif_internal.h"
#include <cdf_memory.h>
#include "dl_list.h"
#define ATH_MODULE_NAME hif
#include "a_debug.h"

#ifdef HIF_LINUX_MMC_SCATTER_SUPPORT

#define _CMD53_ARG_READ          0
#define _CMD53_ARG_WRITE         1
#define _CMD53_ARG_BLOCK_BASIS   1
#define _CMD53_ARG_FIXED_ADDRESS 0
#define _CMD53_ARG_INCR_ADDRESS  1

#define SDIO_SET_CMD53_ARG(arg, rw, func, mode, opcode, address, bytes_blocks) \
		((arg) = (((rw) & 1) << 31) | \
		((func & 0x7) << 28) | \
		(((mode) & 1) << 27) | \
		(((opcode) & 1) << 26) | \
		(((address) & 0x1FFFF) << 9) | \
		((bytes_blocks) & 0x1FF))

/**
 * free_scatter_req() - free scattered request.
 * @device: hif device context
 * @pReq: scatter list node
 *
 * Return: none
 */
static void free_scatter_req(struct hif_sdio_dev *device,
		struct _HIF_SCATTER_REQ *pReq)
{
	unsigned long flag;

	spin_lock_irqsave(&device->lock, flag);

	dl_list_insert_tail(&device->scatter_req_head, &pReq->list_link);

	spin_unlock_irqrestore(&device->lock, flag);

}

/**
 * alloc_scatter_req() - allocate scattered request.
 * @device: hif device context
 *
 *
 * Return: pointer to allocated scatter list node
 */
static struct _HIF_SCATTER_REQ *alloc_scatter_req(struct hif_sdio_dev *device)
{
	DL_LIST *pItem;
	unsigned long flag;

	spin_lock_irqsave(&device->lock, flag);

	pItem = dl_list_remove_item_from_head(&device->scatter_req_head);

	spin_unlock_irqrestore(&device->lock, flag);

	if (pItem != NULL)
		return A_CONTAINING_STRUCT(pItem,
			struct _HIF_SCATTER_REQ, list_link);

	return NULL;
}

/**
 * do_hif_read_write_scatter() - rd/wr scattered operation.
 * @device: hif device context
 * @busrequest: rd/wr bus request
 *
 * called by async task to perform the operation synchronously
 * using direct MMC APIs
 * Return: int
 */
A_STATUS do_hif_read_write_scatter(struct hif_sdio_dev *device,
		struct bus_request *busrequest)
{
	int i;
	A_UINT8 rw;
	A_UINT8 opcode;
	struct mmc_request mmcreq;
	struct mmc_command cmd;
	struct mmc_data data;
	struct HIF_SCATTER_REQ_PRIV *pReqPriv;
	struct _HIF_SCATTER_REQ *pReq;
	A_STATUS status = A_OK;
	struct scatterlist *pSg;

	HIF_ENTER();

	pReqPriv = busrequest->scatter_req;

	A_ASSERT(pReqPriv != NULL);

	pReq = pReqPriv->hif_scatter_req;

	memset(&mmcreq, 0, sizeof(struct mmc_request));
	memset(&cmd, 0, sizeof(struct mmc_command));
	memset(&data, 0, sizeof(struct mmc_data));

	data.blksz = HIF_MBOX_BLOCK_SIZE;
	data.blocks = pReq->total_length / HIF_MBOX_BLOCK_SIZE;

	AR_DEBUG_PRINTF(ATH_DEBUG_SCATTER,
			("HIF-SCATTER: (%s) Address: 0x%X, (BlockLen: %d, "
			 "BlockCount: %d), (tot:%d,sg:%d)\n",
			 (pReq->request & HIF_SDIO_WRITE) ? "WRITE" : "READ",
			 pReq->address, data.blksz, data.blocks,
			 pReq->total_length, pReq->valid_scatter_entries));

	if (pReq->request & HIF_SDIO_WRITE) {
		rw = _CMD53_ARG_WRITE;
		data.flags = MMC_DATA_WRITE;
	} else {
		rw = _CMD53_ARG_READ;
		data.flags = MMC_DATA_READ;
	}

	if (pReq->request & HIF_FIXED_ADDRESS)
		opcode = _CMD53_ARG_FIXED_ADDRESS;
	else
		opcode = _CMD53_ARG_INCR_ADDRESS;

	/* fill SG entries */
	pSg = pReqPriv->sgentries;
	sg_init_table(pSg, pReq->valid_scatter_entries);

	/* assemble SG list */
	for (i = 0; i < pReq->valid_scatter_entries; i++, pSg++) {
		/* setup each sg entry */
		if ((unsigned long)pReq->scatter_list[i].buffer & 0x3) {
			/* note some scatter engines can handle unaligned
			 * buffers, print this as informational only */
			AR_DEBUG_PRINTF(ATH_DEBUG_SCATTER,
				("HIF: (%s) Scatter Buf is unaligned 0x%lx\n",
				 pReq->
				 request & HIF_SDIO_WRITE ? "WRITE" : "READ",
				 (unsigned long)pReq->scatter_list[i].
				 buffer));
		}

		AR_DEBUG_PRINTF(ATH_DEBUG_SCATTER,
				("  %d:  Addr:0x%lX, Len:%d\n", i,
				 (unsigned long)pReq->scatter_list[i].buffer,
				 pReq->scatter_list[i].length));

		sg_set_buf(pSg, pReq->scatter_list[i].buffer,
			   pReq->scatter_list[i].length);
	}
	/* set scatter-gather table for request */
	data.sg = pReqPriv->sgentries;
	data.sg_len = pReq->valid_scatter_entries;
	/* set command argument */
	SDIO_SET_CMD53_ARG(cmd.arg,
			   rw,
			   device->func->num,
			   _CMD53_ARG_BLOCK_BASIS,
			   opcode, pReq->address, data.blocks);

	cmd.opcode = SD_IO_RW_EXTENDED;
	cmd.flags = MMC_RSP_SPI_R5 | MMC_RSP_R5 | MMC_CMD_ADTC;

	mmcreq.cmd = &cmd;
	mmcreq.data = &data;

	mmc_set_data_timeout(&data, device->func->card);
	/* synchronous call to process request */
	mmc_wait_for_req(device->func->card->host, &mmcreq);

	if (cmd.error) {
		status = A_ERROR;
		AR_DEBUG_PRINTF(ATH_DEBUG_ERROR,
				("HIF-SCATTER: cmd error: %d\n", cmd.error));
	}

	if (data.error) {
		status = A_ERROR;
		AR_DEBUG_PRINTF(ATH_DEBUG_ERROR,
				("HIF-SCATTER: data error: %d\n", data.error));
	}

	if (A_FAILED(status)) {
		AR_DEBUG_PRINTF(ATH_DEBUG_ERROR,
			("HIF-SCATTER: FAILED!!! (%s) Address: 0x%X, "
			 "Block mode (BlockLen: %d, BlockCount: %d)\n",
			 (pReq->request & HIF_SDIO_WRITE) ? "WRITE" : "READ",
			 pReq->address, data.blksz, data.blocks));
	}

	/* set completion status, fail or success */
	pReq->completion_status = status;

	if (pReq->request & HIF_ASYNCHRONOUS) {
		AR_DEBUG_PRINTF(ATH_DEBUG_SCATTER,
				("HIF-SCATTER: async_task completion routine "
				 "req: 0x%lX (%d)\n",
				 (unsigned long)busrequest, status));
		/* complete the request */
		A_ASSERT(pReq->completion_routine != NULL);
		pReq->completion_routine(pReq);
	} else {
		AR_DEBUG_PRINTF(ATH_DEBUG_SCATTER,
			("HIF-SCATTER async_task upping busreq : 0x%lX (%d)\n",
			 (unsigned long)busrequest, status));
		/* signal wait */
		up(&busrequest->sem_req);
	}
	HIF_EXIT();

	return status;
}

/**
 * alloc_scatter_req() - callback to issue a read-write
 * scatter request.
 * @device: hif device context
 * @pReq: rd/wr scatter request
 *
 * Return: int
 */
static A_STATUS hif_read_write_scatter(struct hif_sdio_dev *device,
				   struct _HIF_SCATTER_REQ *pReq)
{
	A_STATUS status = A_EINVAL;
	A_UINT32 request = pReq->request;
	struct HIF_SCATTER_REQ_PRIV *pReqPriv =
		(struct HIF_SCATTER_REQ_PRIV *) pReq->hif_private[0];

	do {

		A_ASSERT(pReqPriv != NULL);

		AR_DEBUG_PRINTF(ATH_DEBUG_SCATTER,
			("HIF-SCATTER: total len: %d Scatter Entries: %d\n",
				 pReq->total_length,
				 pReq->valid_scatter_entries));

		if (!(request & HIF_EXTENDED_IO)) {
			AR_DEBUG_PRINTF(ATH_DEBUG_ERROR,
				("HIF-SCATTER: Invalid command type: 0x%08x\n",
					 request));
			break;
		}

		if (!(request & (HIF_SYNCHRONOUS | HIF_ASYNCHRONOUS))) {
			AR_DEBUG_PRINTF(ATH_DEBUG_ERROR,
				("HIF-SCATTER: Invalid mode: 0x%08x\n",
					 request));
			break;
		}

		if (!(request & HIF_BLOCK_BASIS)) {
			AR_DEBUG_PRINTF(ATH_DEBUG_ERROR,
				("HIF-SCATTER: Invalid data mode: 0x%08x\n",
					 request));
			break;
		}

		if (pReq->total_length > MAX_SCATTER_REQ_TRANSFER_SIZE) {
			AR_DEBUG_PRINTF(ATH_DEBUG_ERROR,
				("HIF-SCATTER: Invalid length: %d\n",
					 pReq->total_length));
			break;
		}

		if (pReq->total_length == 0) {
			A_ASSERT(false);
			break;
		}

		/* add bus request to the async list for the async
		 * I/O thread to process */
		add_to_async_list(device, pReqPriv->busrequest);

		if (request & HIF_SYNCHRONOUS) {
			AR_DEBUG_PRINTF(ATH_DEBUG_SCATTER,
				("HIF-SCATTER: queued sync req: 0x%lX\n",
					 (unsigned long)pReqPriv->busrequest));
			/* signal thread and wait */
			up(&device->sem_async);
			if (down_interruptible(&pReqPriv->busrequest->sem_req)
			    != 0) {
				AR_DEBUG_PRINTF(ATH_DEBUG_ERROR,
					("HIF-SCATTER: interrupted!\n"));
				/* interrupted, exit */
				status = A_ERROR;
				break;
			} else {
				status = pReq->completion_status;
			}
		} else {
			AR_DEBUG_PRINTF(ATH_DEBUG_SCATTER,
				("HIF-SCATTER: queued async req: 0x%lX\n",
					 (unsigned long)pReqPriv->busrequest));
			/* wake thread, it will process and then take
			 * care of the async callback */
			up(&device->sem_async);
			status = A_OK;
		}

	} while (false);

	if (A_FAILED(status) && (request & HIF_ASYNCHRONOUS)) {
		pReq->completion_status = status;
		pReq->completion_routine(pReq);
		status = A_OK;
	}

	return status;
}

/**
 * setup_hif_scatter_support() - setup of HIF scatter resources
 * scatter request.
 * @device: hif device context
 * @pInfo: scatter info
 *
 * Return: int
 */
A_STATUS setup_hif_scatter_support(struct hif_sdio_dev *device,
			   struct HIF_DEVICE_SCATTER_SUPPORT_INFO *pInfo)
{
	A_STATUS status = A_ERROR;
	int i;
	struct HIF_SCATTER_REQ_PRIV *pReqPriv;
	struct bus_request *busrequest;

	do {
		if (device->func->card->host->max_segs <
		    MAX_SCATTER_ENTRIES_PER_REQ) {
			AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
				("host only supports scatter of : %d entries,"
				 "need: %d\n",
				 device->func->card->host->max_segs,
				 MAX_SCATTER_ENTRIES_PER_REQ));
			status = A_ENOTSUP;
			break;
		}

		AR_DEBUG_PRINTF(ATH_DEBUG_ANY,
				("max scatter req : %d entries: %d\n",
				 MAX_SCATTER_REQUESTS,
				 MAX_SCATTER_ENTRIES_PER_REQ));

		for (i = 0; i < MAX_SCATTER_REQUESTS; i++) {
			/* allocate the private request blob */
			pReqPriv =
				(struct HIF_SCATTER_REQ_PRIV *)
				cdf_mem_malloc(sizeof(struct HIF_SCATTER_REQ_PRIV));
			if (NULL == pReqPriv)
				break;
			A_MEMZERO(pReqPriv, sizeof(struct HIF_SCATTER_REQ_PRIV));
			/* save the device instance */
			pReqPriv->device = device;
			/* allocate the scatter request */
			pReqPriv->hif_scatter_req =
				(struct _HIF_SCATTER_REQ *)
				cdf_mem_malloc(sizeof(struct _HIF_SCATTER_REQ) +
					       (MAX_SCATTER_ENTRIES_PER_REQ -
				       1) * (sizeof(struct _HIF_SCATTER_ITEM)));

			if (NULL == pReqPriv->hif_scatter_req) {
				cdf_mem_free(pReqPriv);
				break;
			}
			/* just zero the main part of the scatter request */
			A_MEMZERO(pReqPriv->hif_scatter_req,
				  sizeof(struct _HIF_SCATTER_REQ));
			/* back pointer to the private struct */
			pReqPriv->hif_scatter_req->hif_private[0] = pReqPriv;
			/* allocate a bus request for this scatter request */
			busrequest = hif_allocate_bus_request(device);
			if (NULL == busrequest) {
				cdf_mem_free(pReqPriv->hif_scatter_req);
				cdf_mem_free(pReqPriv);
				break;
			}
			/* assign the scatter request to this bus request */
			busrequest->scatter_req = pReqPriv;
			/* point back to the request */
			pReqPriv->busrequest = busrequest;
			/* add it to the scatter pool */
			free_scatter_req(device, pReqPriv->hif_scatter_req);
		}

		if (i != MAX_SCATTER_REQUESTS) {
			status = A_NO_MEMORY;
			AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
				("failed to alloc scatter resources !\n"));
			break;
		}

		/* set scatter function pointers */
		pInfo->allocate_req_func = alloc_scatter_req;
		pInfo->free_req_func = free_scatter_req;
		pInfo->read_write_scatter_func = hif_read_write_scatter;
		pInfo->max_scatter_entries = MAX_SCATTER_ENTRIES_PER_REQ;
		pInfo->max_tx_size_per_scatter_req =
			MAX_SCATTER_REQ_TRANSFER_SIZE;

		status = A_OK;

	} while (false);

	if (A_FAILED(status))
		cleanup_hif_scatter_resources(device);

	return status;
}

/**
 * cleanup_hif_scatter_resources() - cleanup HIF scatter resources
 * scatter request.
 * @device: hif device context
 *
 *
 * Return: none
 */
void cleanup_hif_scatter_resources(struct hif_sdio_dev *device)
{
	struct HIF_SCATTER_REQ_PRIV *pReqPriv;
	struct _HIF_SCATTER_REQ *pReq;

	/* empty the free list */

	while (true) {

		pReq = alloc_scatter_req(device);

		if (NULL == pReq)
			break;

		pReqPriv = (struct HIF_SCATTER_REQ_PRIV *) pReq->hif_private[0];
		A_ASSERT(pReqPriv != NULL);

		if (pReqPriv->busrequest != NULL) {
			pReqPriv->busrequest->scatter_req = NULL;
			/* free bus request */
			hif_free_bus_request(device, pReqPriv->busrequest);
			pReqPriv->busrequest = NULL;
		}

		if (pReqPriv->hif_scatter_req != NULL) {
			cdf_mem_free(pReqPriv->hif_scatter_req);
			pReqPriv->hif_scatter_req = NULL;
		}

		cdf_mem_free(pReqPriv);
	}
}

#endif /* HIF_LINUX_MMC_SCATTER_SUPPORT */
