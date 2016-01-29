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
#include <a_debug.h>
#include "hif_sdio_internal.h"

/*
 * Data structure to record required sending context data
 */
struct hif_sendContext {
	A_BOOL bNewAlloc;
	HIF_SDIO_DEVICE *pDev;
	cdf_nbuf_t netbuf;
	unsigned int transferID;
	unsigned int head_data_len;
};

/**
 * hif_dev_rw_completion_handler() - Completion routine
 * for ALL HIF layer async I/O
 * @context: hif send context
 * @status: completion routine sync/async context
 *
 * Return: 0 for success and non-zero for failure
 */
A_STATUS hif_dev_rw_completion_handler(void *context, A_STATUS status)
{
	struct hif_sendContext *pSendContext =
				(struct hif_sendContext *)context;
	unsigned int transferID = pSendContext->transferID;
	HIF_SDIO_DEVICE *pDev = pSendContext->pDev;
	cdf_nbuf_t buf = pSendContext->netbuf;
	/* Fix Me: Do we need toeplitz_hash_result for SDIO */
	uint32_t toeplitz_hash_result = 0;

	if (pSendContext->bNewAlloc)
		cdf_mem_free((void *)pSendContext);
	else
		cdf_nbuf_pull_head(buf, pSendContext->head_data_len);
	if (pDev->hif_callbacks.txCompletionHandler)
		pDev->hif_callbacks.txCompletionHandler(pDev->hif_callbacks.
					Context, buf,
					transferID, toeplitz_hash_result);

	return A_OK;
}

/**
 * hif_dev_send_buffer() - send buffer to sdio device
 * @pDev: sdio function
 * @transferID: transfer id
 * @pipe: ul/dl pipe
 * @nbytes: no of bytes to transfer
 * @buf: pointer to buffer
 *
 * Return: 0 for success and non-zero for failure
 */
A_STATUS hif_dev_send_buffer(HIF_SDIO_DEVICE *pDev, unsigned int transferID,
			     uint8_t pipe, unsigned int nbytes, cdf_nbuf_t buf)
{
	A_STATUS status;
	A_UINT32 paddedLength;
	int frag_count = 0, i, head_data_len;
	struct hif_sendContext *pSendContext;
	unsigned char *pData;
	A_UINT32 request = HIF_WR_ASYNC_BLOCK_INC;
	A_UINT8 mboxIndex = hif_dev_map_pipe_to_mail_box(pDev, pipe);

	paddedLength = DEV_CALC_SEND_PADDED_LEN(pDev, nbytes);
	A_ASSERT(paddedLength - nbytes < HIF_DUMMY_SPACE_MASK + 1);
	/*
	 * two most significant bytes to save dummy data count
	 * data written into the dummy space will not put into
	 * the final mbox FIFO.
	 */
	request |= ((paddedLength - nbytes) << 16);

	frag_count = cdf_nbuf_get_num_frags(buf);

	if (frag_count > 1) {
		/* header data length should be total sending length substract
		 * internal data length of netbuf */
		head_data_len = sizeof(struct hif_sendContext) +
			(nbytes - cdf_nbuf_get_frag_len(buf, frag_count - 1));
	} else {
		/*
		 * | hif_sendContext | netbuf->data
		 */
		head_data_len = sizeof(struct hif_sendContext);
	}

	/* Check whether head room is enough to save extra head data */
	if ((head_data_len <= cdf_nbuf_headroom(buf)) &&
	    (cdf_nbuf_tailroom(buf) >= (paddedLength - nbytes))) {
		pSendContext =
			(struct hif_sendContext *)cdf_nbuf_push_head(buf,
						     head_data_len);
		pSendContext->bNewAlloc = false;
	} else {
		pSendContext =
			(struct hif_sendContext *)
			cdf_mem_malloc(sizeof(struct hif_sendContext) +
				       paddedLength);
		pSendContext->bNewAlloc = true;
	}

	pSendContext->netbuf = buf;
	pSendContext->pDev = pDev;
	pSendContext->transferID = transferID;
	pSendContext->head_data_len = head_data_len;
	/*
	 * Copy data to head part of netbuf or head of allocated buffer.
	 * if buffer is new allocated, the last buffer should be copied also.
	 * It assume last fragment is internal buffer of netbuf
	 * sometime total length of fragments larger than nbytes
	 */
	pData = (unsigned char *)pSendContext + sizeof(struct hif_sendContext);
	for (i = 0; i < (pSendContext->bNewAlloc ? frag_count : frag_count - 1);
	     i++) {
		int frag_len = cdf_nbuf_get_frag_len(buf, i);
		unsigned char *frag_addr = cdf_nbuf_get_frag_vaddr(buf, i);
		if (frag_len > nbytes)
			frag_len = nbytes;
		memcpy(pData, frag_addr, frag_len);
		pData += frag_len;
		nbytes -= frag_len;
		if (nbytes <= 0)
			break;
	}

	/* Reset pData pointer and send out */
	pData = (unsigned char *)pSendContext + sizeof(struct hif_sendContext);
	status = hif_read_write(pDev->HIFDevice,
				pDev->MailBoxInfo.MboxProp[mboxIndex].
				ExtendedAddress, (char *)pData, paddedLength,
				request, (void *)pSendContext);

	if (status == A_PENDING)
		/*
		 * it will return A_PENDING in native HIF implementation,
		 * which should be treated as successful result here.
		 */
		status = A_OK;
	/* release buffer or move back data pointer when failed */
	if (status != A_OK) {
		if (pSendContext->bNewAlloc)
			cdf_mem_free(pSendContext);
		else
			cdf_nbuf_pull_head(buf, head_data_len);
	}

	return status;
}
