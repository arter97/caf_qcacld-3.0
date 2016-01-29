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

#ifndef _HIF_INTERNAL_H_
#define _HIF_INTERNAL_H_

#include "athdefs.h"
#include "a_types.h"
#include "a_osapi.h"
#include <cdf_types.h>          /* cdf_device_t, cdf_print */
#include <cdf_time.h>           /* cdf_system_ticks, etc. */
#include <cdf_status.h>
#include <cdf_softirq_timer.h>
#include <cdf_atomic.h>
#include "hif.h"
#include "hif_debug.h"
#include "hif_sdio_common.h"
#include <linux/scatterlist.h>
#define HIF_LINUX_MMC_SCATTER_SUPPORT

#define BUS_REQUEST_MAX_NUM                64

#define SDIO_CLOCK_FREQUENCY_DEFAULT       25000000
#define SDWLAN_ENABLE_DISABLE_TIMEOUT      20
#define FLAGS_CARD_ENAB                    0x02
#define FLAGS_CARD_IRQ_UNMSK               0x04

#define HIF_MBOX_BLOCK_SIZE                HIF_DEFAULT_IO_BLOCK_SIZE
#define HIF_MBOX0_BLOCK_SIZE               1
#define HIF_MBOX1_BLOCK_SIZE               HIF_MBOX_BLOCK_SIZE
#define HIF_MBOX2_BLOCK_SIZE               HIF_MBOX_BLOCK_SIZE
#define HIF_MBOX3_BLOCK_SIZE               HIF_MBOX_BLOCK_SIZE

struct HIF_SCATTER_REQ_PRIV;

struct bus_request {
	struct bus_request *next;       /* link list of available requests */
	struct bus_request *inusenext;  /* link list of in use requests */
	struct semaphore sem_req;
	A_UINT32 address;       /* request data */
	A_UCHAR *buffer;
	A_UINT32 length;
	A_UINT32 request;
	void *context;
	A_STATUS status;
	struct HIF_SCATTER_REQ_PRIV *pScatterReq;
};

struct hif_sdio_dev {
	struct sdio_func *func;
	spinlock_t asynclock;
	struct task_struct *async_task; /* task to handle async commands */
	struct semaphore sem_async;     /* wake up for async task */
	int async_shutdown;     /* stop the async task */
	struct completion async_completion;     /* thread completion */
	struct bus_request *asyncreq;  /* request for async tasklet */
	struct bus_request *taskreq;   /*  async tasklet data */
	spinlock_t lock;
	struct bus_request *s_busRequestFreeQueue;     /* free list */
	struct bus_request busRequest[BUS_REQUEST_MAX_NUM]; /* bus requests */
	void *claimedContext;
	struct htc_callbacks htcCallbacks;
	A_UINT8 *dma_buffer;
	DL_LIST ScatterReqHead; /* scatter request list head */
	A_BOOL scatter_enabled; /* scatter enabled flag */
	A_BOOL is_suspend;
	A_BOOL is_disabled;
	atomic_t irqHandling;
	HIF_DEVICE_POWER_CHANGE_TYPE powerConfig;
	HIF_DEVICE_STATE DeviceState;
	const struct sdio_device_id *id;
	struct mmc_host *host;
	void *htcContext;
	A_BOOL swap_mailbox;
};

#define HIF_DMA_BUFFER_SIZE (4 * 1024)
#define CMD53_FIXED_ADDRESS 1
#define CMD53_INCR_ADDRESS  2

struct bus_request *hif_allocate_bus_request(struct hif_sdio_dev *device);
void hif_free_bus_request(struct hif_sdio_dev *device,
			  struct bus_request *busrequest);
void add_to_async_list(struct hif_sdio_dev *device,
		       struct bus_request *busrequest);
void hif_dump_cccr(struct hif_sdio_dev *hif_device);

#ifdef HIF_LINUX_MMC_SCATTER_SUPPORT

#define MAX_SCATTER_REQUESTS             4
#define MAX_SCATTER_ENTRIES_PER_REQ      16
#define MAX_SCATTER_REQ_TRANSFER_SIZE    (32*1024)

struct HIF_SCATTER_REQ_PRIV {
	struct _HIF_SCATTER_REQ *pHifScatterReq;
	struct hif_sdio_dev *device;     /* this device */
	struct bus_request *busrequest;
	/* scatter list for linux */
	struct scatterlist sgentries[MAX_SCATTER_ENTRIES_PER_REQ];
};

#define ATH_DEBUG_SCATTER  ATH_DEBUG_MAKE_MODULE_MASK(0)

A_STATUS setup_hif_scatter_support(struct hif_sdio_dev *device,
		   struct HIF_DEVICE_SCATTER_SUPPORT_INFO *pInfo);
void cleanup_hif_scatter_resources(struct hif_sdio_dev *device);
A_STATUS do_hif_read_write_scatter(struct hif_sdio_dev *device,
				   struct bus_request *busrequest);

#else                           /* HIF_LINUX_MMC_SCATTER_SUPPORT */

static inline A_STATUS setup_hif_scatter_support(struct hif_sdio_dev *device,
				struct HIF_DEVICE_SCATTER_SUPPORT_INFO *pInfo)
{
	return A_ENOTSUP;
}

static inline A_STATUS do_hif_read_write_scatter(struct hif_sdio_dev *device,
					 struct bus_request *busrequest)
{
	return A_ENOTSUP;
}

#define cleanup_hif_scatter_resources(d) { }

#endif /* HIF_LINUX_MMC_SCATTER_SUPPORT */

#endif /* _HIF_INTERNAL_H_ */
