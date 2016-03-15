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
#ifndef _HIF_USB_INTERNAL_H
#define _HIF_USB_INTERNAL_H

#include <cdf_nbuf.h>
#include "a_types.h"
#include "athdefs.h"
#include "a_osapi.h"
#include "a_usb_defs.h"
#include <ol_if_athvar.h>
#include <linux/usb.h>
#include "hif.h"
#define TX_URB_COUNT    32
#define RX_URB_COUNT    32

#define HIF_USB_RX_BUFFER_SIZE  (1792 + 8)
#define HIF_USB_RX_BUNDLE_ONE_PKT_SIZE  (1792 + 8)

#ifdef HIF_USB_TASKLET
#define HIF_USB_SCHEDULE_WORK(pipe)\
	tasklet_schedule(&pipe->io_complete_tasklet);

#define HIF_USB_INIT_WORK(pipe)\
		tasklet_init(&pipe->io_complete_tasklet,\
				usb_hif_io_comp_tasklet,\
				(long unsigned int)pipe);

#define HIF_USB_FLUSH_WORK(pipe) flush_work(&pipe->io_complete_work);
#else
#define HIF_USB_SCHEDULE_WORK(pipe) schedule_work(&pipe->io_complete_work);
#define HIF_USB_INIT_WORK(pipe)\
		INIT_WORK(&pipe->io_complete_work,\
				usb_hif_io_comp_work);
#define HIF_USB_FLUSH_WORK(pipe)
#endif

/* USB Endpoint definition */
typedef enum {
	HIF_TX_CTRL_PIPE = 0,
	HIF_TX_DATA_LP_PIPE,
	HIF_TX_DATA_MP_PIPE,
	HIF_TX_DATA_HP_PIPE,
	HIF_RX_CTRL_PIPE,
	HIF_RX_DATA_PIPE,
	HIF_RX_DATA2_PIPE,
	HIF_RX_INT_PIPE,
	HIF_USB_PIPE_MAX
} HIF_USB_PIPE_ID;

#define HIF_USB_PIPE_INVALID HIF_USB_PIPE_MAX
/* debug masks */
#define USB_HIF_DEBUG_CTRL_TRANS            ATH_DEBUG_MAKE_MODULE_MASK(0)
#define USB_HIF_DEBUG_BULK_IN               ATH_DEBUG_MAKE_MODULE_MASK(1)
#define USB_HIF_DEBUG_BULK_OUT              ATH_DEBUG_MAKE_MODULE_MASK(2)
#define USB_HIF_DEBUG_ENUM                  ATH_DEBUG_MAKE_MODULE_MASK(3)
#define USB_HIF_DEBUG_DUMP_DATA             ATH_DEBUG_MAKE_MODULE_MASK(4)
#define USB_HIF_SUSPEND                     ATH_DEBUG_MAKE_MODULE_MASK(5)
#define USB_HIF_ISOC_SUPPORT                ATH_DEBUG_MAKE_MODULE_MASK(6)

struct _HIF_USB_PIPE;

typedef struct _HIF_URB_CONTEXT {
	DL_LIST link;
	struct _HIF_USB_PIPE *pipe;
	cdf_nbuf_t buf;
	struct urb *urb;
	struct hif_usb_send_context *send_context;
} HIF_URB_CONTEXT;

#define HIF_USB_PIPE_FLAG_TX    (1 << 0)

typedef struct _HIF_USB_PIPE {
	DL_LIST urb_list_head;
	DL_LIST urb_pending_list;
	int32_t urb_alloc;
	int32_t urb_cnt;
	int32_t urb_cnt_thresh;
	unsigned int usb_pipe_handle;
	uint32_t flags;
	uint8_t ep_address;
	uint8_t logical_pipe_num;
	struct _HIF_DEVICE_USB *device;
	uint16_t max_packet_size;
#ifdef HIF_USB_TASKLET
	struct tasklet_struct io_complete_tasklet;
#else
	struct work_struct io_complete_work;
#endif
	struct sk_buff_head io_comp_queue;
	struct usb_endpoint_descriptor *ep_desc;
	int32_t urb_prestart_cnt;
} HIF_USB_PIPE;

typedef struct _HIF_DEVICE_USB {
	cdf_spinlock_t cs_lock;
	cdf_spinlock_t tx_lock;
	cdf_spinlock_t rx_lock;
	struct hif_msg_callbacks htc_callbacks;
	struct usb_device *udev;
	struct usb_interface *interface;
	HIF_USB_PIPE pipes[HIF_USB_PIPE_MAX];
	uint8_t surprise_removed;
	uint8_t *diag_cmd_buffer;
	uint8_t *diag_resp_buffer;
	void *claimed_context;
	struct hif_usb_softc *sc;
	A_BOOL is_bundle_enabled;
	uint16_t rx_bundle_cnt;
	uint32_t rx_bundle_buf_len;
} HIF_DEVICE_USB;

/*
 * Data structure to record required sending context data
 */
struct hif_usb_send_context {
	A_BOOL new_alloc;
	HIF_DEVICE_USB *hif_usb_device;
	cdf_nbuf_t netbuf;
	unsigned int transfer_id;
	unsigned int head_data_len;
};

extern unsigned int hif_usb_disable_rxdata2;

extern CDF_STATUS usb_hif_submit_ctrl_in(HIF_DEVICE_USB *macp,
				uint8_t req,
				uint16_t value,
				uint16_t index,
				void *data, uint32_t size);

extern CDF_STATUS usb_hif_submit_ctrl_out(HIF_DEVICE_USB *macp,
					uint8_t req,
					uint16_t value,
					uint16_t index,
					void *data, uint32_t size);

int hif_enable_usb(struct usb_interface *interface,
	struct hif_usb_softc *sc);
void hif_disable_usb(struct usb_interface *interface,
					uint8_t surprise_removed);
CDF_STATUS usb_hif_setup_pipe_resources(HIF_DEVICE_USB *device);
void usb_hif_cleanup_pipe_resources(HIF_DEVICE_USB *device);
void usb_hif_prestart_recv_pipes(HIF_DEVICE_USB *device);
void usb_hif_start_recv_pipes(HIF_DEVICE_USB *device);
void usb_hif_flush_all(HIF_DEVICE_USB *device);
void usb_hif_cleanup_transmit_urb(HIF_URB_CONTEXT *urb_context);
void usb_hif_enqueue_pending_transfer(HIF_USB_PIPE *pipe,
						HIF_URB_CONTEXT *urb_context);
void usb_hif_remove_pending_transfer(HIF_URB_CONTEXT *urb_context);
HIF_URB_CONTEXT *usb_hif_alloc_urb_from_pipe(HIF_USB_PIPE *pipe);
#ifdef HIF_USB_TASKLET
void usb_hif_io_comp_tasklet(long unsigned int context);
#else
void usb_hif_io_comp_work(struct work_struct *work);
#endif
CDF_STATUS hif_diag_write_warm_reset(struct usb_interface *interface,
			uint32_t address, uint32_t data);
#endif
