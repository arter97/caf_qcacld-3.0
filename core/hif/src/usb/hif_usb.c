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
#include <cdf_time.h>
#include <cdf_lock.h>
#include <cdf_memory.h>
#include <cdf_util.h>
#include <cdf_defer.h>
#include <cdf_atomic.h>
#include <cdf_nbuf.h>
#include "cdf_net_types.h"
#include <hif_usb_internal.h>
#include <htc_services.h>
#include <hif_debug.h>
#define ATH_MODULE_NAME hif
#include <a_debug.h>

#define USB_HIF_USE_SINGLE_PIPE_FOR_DATA
#define USB_HIF_TARGET_CREDIT_SIZE  1664
#ifdef DEBUG
static ATH_DEBUG_MASK_DESCRIPTION g_hif_debug_description[] = {
	{USB_HIF_DEBUG_CTRL_TRANS, "Control Transfers"},
	{USB_HIF_DEBUG_BULK_IN, "BULK In Transfers"},
	{USB_HIF_DEBUG_BULK_OUT, "BULK Out Transfers"},
	{USB_HIF_DEBUG_DUMP_DATA, "Dump data"},
	{USB_HIF_DEBUG_ENUM, "Enumeration"},
};

ATH_DEBUG_INSTANTIATE_MODULE_VAR(hif,
				 "hif",
				 "USB Host Interface",
				 ATH_DEBUG_MASK_DEFAULTS | ATH_DEBUG_INFO |
				 USB_HIF_DEBUG_ENUM,
				 ATH_DEBUG_DESCRIPTION_COUNT
				 (g_hif_debug_description),
				 g_hif_debug_description);

#endif

#ifdef USB_ISOC_SUPPORT
unsigned int hif_usb_isoch_vo = 1;
#else
unsigned int hif_usb_isoch_vo;
#endif
unsigned int hif_usb_disable_rxdata2 = 1;

static void hif_usb_destroy(HIF_DEVICE_USB *device);
static HIF_DEVICE_USB *hif_usb_create(struct usb_interface *interface);
/**
 * hif_enable_usb() - hif_enable_usb
 * @interface: pointer to usb_interface structure
 * @sc: pointer to hif_usb_softc
 *
 * Return: int 0 on success and errno on failure.
 */
int hif_enable_usb(struct usb_interface *interface, struct hif_usb_softc *sc)
{
	HIF_DEVICE_USB *device = NULL;
	int retval = -1;
	struct usb_device *udev = interface_to_usbdev(interface);

	HIF_TRACE("+%s", __func__);

	HIF_TRACE("BCDDevice : %x", udev->descriptor.bcdDevice);

	do {
		device = hif_usb_create(interface);
		if (NULL == device) {
			HIF_ERROR("device is NULL");
			break;
		}
		device->sc = sc;
		retval = 0;

	} while (false);

	if ((device != NULL) && (retval < 0)) {
		HIF_ERROR("abnormal condition");
		hif_usb_destroy(device);
	}

	HIF_TRACE("-%s", __func__);
	return retval;
}

/**
 * hif_disable_usb() - cleanup usb resources, destroy HIF USB device
 * @interface: pointer to usb_interface structure
 * @surprise_removed: indicates that device is  being removed
 *
 * Return: int 0 on success and errno on failure.
 */
void hif_disable_usb(struct usb_interface *interface, uint8_t surprise_removed)
{
	HIF_DEVICE_USB *device;

	HIF_TRACE("+%s", __func__);

	do {
		device = (HIF_DEVICE_USB *) usb_get_intfdata(interface);
		if (NULL == device) {
			HIF_ERROR("device is NULL");
			break;
		}
		device->surprise_removed = surprise_removed;
		hif_usb_destroy(device);
	} while (false);

	HIF_TRACE("-%s", __func__);

}

/**
 * usb_hif_usb_transmit_complete() - completion routing for tx urb's
 * @urb: pointer to urb for which tx completion is called
 *
 * Return: none
 */
static void usb_hif_usb_transmit_complete(struct urb *urb)
{
	HIF_URB_CONTEXT *urb_context = (HIF_URB_CONTEXT *) urb->context;
	cdf_nbuf_t buf;
	HIF_USB_PIPE *pipe = urb_context->pipe;
	struct hif_usb_send_context *send_context;

	HIF_DBG("+%s: pipe: %d, stat:%d, len:%d", __func__,
		pipe->logical_pipe_num, urb->status, urb->actual_length);

	/* this urb is not pending anymore */
	usb_hif_remove_pending_transfer(urb_context);

	if (urb->status != 0) {
		HIF_ERROR("%s:  pipe: %d, failed:%d",
			  __func__, pipe->logical_pipe_num, urb->status);
	}

	buf = urb_context->buf;
	send_context = urb_context->send_context;

	if (send_context->new_alloc)
		cdf_mem_free(send_context);
	else
		cdf_nbuf_pull_head(buf, send_context->head_data_len);

	urb_context->buf = NULL;
	usb_hif_cleanup_transmit_urb(urb_context);

	/* note: queue implements a lock */
	skb_queue_tail(&pipe->io_comp_queue, buf);
	HIF_USB_SCHEDULE_WORK(pipe)

	HIF_DBG("-%s", __func__);
}

/**
 * hif_send_internal() - HIF internal routine to prepare and submit tx urbs
 * @hif_usb_device: pointer to HIF_DEVICE_USB structure
 * @pipe_id: HIF pipe on which data is to be sent
 * @hdr_buf: any header buf to be prepended, currently ignored
 * @buf: cdf_nbuf_t containing data to be transmitted
 * @nbytes: number of bytes to be transmitted
 *
 * Return: CDF_STATUS_SUCCESS on success and error CDF status on failure
 */
static CDF_STATUS hif_send_internal(HIF_DEVICE_USB *hif_usb_device,
				    uint8_t pipe_id,
				    cdf_nbuf_t hdr_buf,
				    cdf_nbuf_t buf, unsigned int nbytes)
{
	CDF_STATUS status = CDF_STATUS_SUCCESS;
	HIF_DEVICE_USB *device = hif_usb_device;
	HIF_USB_PIPE *pipe = &device->pipes[pipe_id];
	HIF_URB_CONTEXT *urb_context;
	uint8_t *data;
	uint32_t len;
	struct urb *urb;
	int usb_status;
	int i;
	struct hif_usb_send_context *send_context;
	int frag_count = 0, head_data_len, tmp_frag_count = 0;
	unsigned char *data_ptr;

	HIF_DBG("+%s pipe : %d, buf:0x%p nbytes %u",
		__func__, pipe_id, buf, nbytes);

	frag_count = cdf_nbuf_get_num_frags(buf);
	if (frag_count > 1) {	/* means have extra fragment buf in skb */
		/* header data length should be total sending length substract
		 * internal data length of netbuf
		 * | hif_usb_send_context | fragments except internal buffer |
		 * netbuf->data
		 */
		head_data_len = sizeof(struct hif_usb_send_context);
		while (tmp_frag_count < (frag_count - 1)) {
			head_data_len =
			    head_data_len +
			    cdf_nbuf_get_frag_len(buf, tmp_frag_count);
			tmp_frag_count = tmp_frag_count + 1;
		}
	} else {
		/*
		 * | hif_usb_send_context | netbuf->data
		 */
		head_data_len = sizeof(struct hif_usb_send_context);
	}

	/* Check whether head room is enough to save extra head data */
	if (head_data_len <= cdf_nbuf_headroom(buf)) {
		send_context = (struct hif_usb_send_context *)
		    cdf_nbuf_push_head(buf, head_data_len);
		send_context->new_alloc = false;
	} else {
		send_context =
		    cdf_mem_malloc(sizeof(struct hif_usb_send_context)
				   + head_data_len + nbytes);
		if (send_context == NULL) {
			HIF_ERROR("%s: cdf_mem_malloc failed", __func__);
			status = CDF_STATUS_E_NOMEM;
			goto err;
		}
		send_context->new_alloc = true;
	}
	send_context->netbuf = buf;
	send_context->hif_usb_device = hif_usb_device;
	send_context->transfer_id = pipe_id;
	send_context->head_data_len = head_data_len;
	/*
	 * Copy data to head part of netbuf or head of allocated buffer.
	 * if buffer is new allocated, the last buffer should be copied also.
	 * It assume last fragment is internal buffer of netbuf
	 * sometime total length of fragments larger than nbytes
	 */
	data_ptr = (unsigned char *)send_context +
				sizeof(struct hif_usb_send_context);
	for (i = 0;
	     i < (send_context->new_alloc ? frag_count : frag_count - 1); i++) {
		int frag_len = cdf_nbuf_get_frag_len(buf, i);
		unsigned char *frag_addr = cdf_nbuf_get_frag_vaddr(buf, i);
		cdf_mem_copy(data_ptr, frag_addr, frag_len);
		data_ptr += frag_len;
	}
	/* Reset pData pointer and send out */
	data_ptr = (unsigned char *)send_context +
				sizeof(struct hif_usb_send_context);

	do {
		urb_context = usb_hif_alloc_urb_from_pipe(pipe);
		if (NULL == urb_context) {
			/* TODO : note, it is possible to run out of urbs if 2
			 * endpoints map to the same pipe ID
			 */
			HIF_ERROR("%s pipe:%d no urbs left. URB Cnt : %d",
				__func__, pipe_id, pipe->urb_cnt);
			status = CDF_STATUS_E_RESOURCES;
			break;
		}
		urb_context->send_context = send_context;
		urb = urb_context->urb;
		urb_context->buf = buf;
		data = data_ptr;
		len = nbytes;

		usb_fill_bulk_urb(urb,
				  device->udev,
				  pipe->usb_pipe_handle,
				  data,
				  (len % pipe->max_packet_size) ==
				  0 ? (len + 1) : len,
				  usb_hif_usb_transmit_complete, urb_context);

		if ((len % pipe->max_packet_size) == 0)
			/* hit a max packet boundary on this pipe */

		HIF_DBG
		    ("athusb bulk send submit:%d, 0x%X (ep:0x%2.2X), %d bytes",
		     pipe->logical_pipe_num, pipe->usb_pipe_handle,
		     pipe->ep_address, nbytes);

		usb_hif_enqueue_pending_transfer(pipe, urb_context);
		usb_status = usb_submit_urb(urb, GFP_ATOMIC);
		if (usb_status) {
			if (send_context->new_alloc)
				cdf_mem_free(send_context);
			else
				cdf_nbuf_pull_head(buf, head_data_len);
			urb_context->buf = NULL;
			HIF_ERROR("athusb : usb bulk transmit failed %d",
					usb_status);
			usb_hif_remove_pending_transfer(urb_context);
			usb_hif_cleanup_transmit_urb(urb_context);
			status = CDF_STATUS_E_FAILURE;
			break;
		}

	} while (false);
err:
	if (!CDF_IS_STATUS_SUCCESS(status) &&
				(status != CDF_STATUS_E_RESOURCES)) {
		HIF_ERROR("athusb send failed %d", status);
	}

	HIF_DBG("-%s pipe : %d", __func__, pipe_id);

	return status;
}

/**
 * hif_send_head() - HIF routine exposed to upper layers to send data
 * @scn: pointer to ol_softc structure
 * @pipe_id: HIF pipe on which data is to be sent
 * @transfer_id: endpoint ID on which data is to be sent
 * @nbytes: number of bytes to be transmitted
 * @wbuf: cdf_nbuf_t containing data to be transmitted
 * @hdr_buf: any header buf to be prepended, currently ignored
 * @data_attr: data_attr field from cvg_nbuf_cb of wbuf
 *
 * Return: CDF_STATUS_SUCCESS on success and error CDF status on failure
 */
CDF_STATUS hif_send_head(struct ol_softc *scn, uint8_t pipe_id,
				uint32_t transfer_id, uint32_t nbytes,
				cdf_nbuf_t wbuf, uint32_t data_attr)
{
	CDF_STATUS status = EOK;

	HIF_TRACE("+%s", __func__);
	status = hif_send_internal(scn->hif_hdl, pipe_id, NULL, wbuf, nbytes);
	HIF_TRACE("-%s", __func__);
	return status;
}

/**
 * hif_get_free_queue_number() - get # of free TX resources in a given HIF pipe
 * @scn: pointer to ol_softc structure
 * @pipe_id: HIF pipe which is being polled for free resources
 *
 * Return: # of free resources in pipe_id
 */
uint16_t hif_get_free_queue_number(struct ol_softc *scn, uint8_t pipe_id)
{
	HIF_DEVICE_USB *device = (HIF_DEVICE_USB *) scn->hif_hdl;

	return device->pipes[pipe_id].urb_cnt;
}

/**
 * hif_get_max_queue_number() - max # of free TX resources in a given HIF pipe
 * @scn: pointer to ol_softc structure
 * @pipe_id: HIF pipe on which is being polled for free resources
 *
 * Return: maximum # of free resources in pipe_id
 */
uint16_t hif_get_max_queue_number(struct ol_softc *scn, uint8_t pipe_id)
{
	HIF_DEVICE_USB *device = (HIF_DEVICE_USB *) scn->hif_hdl;

	return device->pipes[pipe_id].urb_alloc;
}

/**
 * hif_post_init() - copy HTC callbacks to HIF
 * @scn: pointer to ol_softc structure
 * @target: pointer to HTC_TARGET structure
 * @callbacks: htc callbacks
 *
 * Return: none
 */
void hif_post_init(struct ol_softc *scn, void *target,
		struct hif_msg_callbacks *callbacks)
{
	HIF_DEVICE_USB *device = (HIF_DEVICE_USB *) scn->hif_hdl;

	cdf_mem_copy(&device->htc_callbacks, callbacks,
			sizeof(device->htc_callbacks));
}

/**
 * hif_detach_htc() - remove HTC callbacks from HIF
 * @scn: pointer to ol_softc structure
 *
 * Return: none
 */
void hif_detach_htc(struct ol_softc *scn)
{
	HIF_DEVICE_USB *device = (HIF_DEVICE_USB *) scn->hif_hdl;

	usb_hif_flush_all(device);
	cdf_mem_zero(&device->htc_callbacks, sizeof(device->htc_callbacks));
}

/**
 * hif_usb_destroy() - clean up usb resources, delete HIF_DEVICE_USB
 * @device: pointer to HIF_DEVICE_USB structure
 *
 * Return: none
 */
static void hif_usb_destroy(HIF_DEVICE_USB *device)
{
	HIF_TRACE("+%s", __func__);

	usb_hif_flush_all(device);

	usb_hif_cleanup_pipe_resources(device);

	usb_set_intfdata(device->interface, NULL);

	if (device->diag_cmd_buffer != NULL)
		cdf_mem_free(device->diag_cmd_buffer);

	if (device->diag_resp_buffer != NULL)
		cdf_mem_free(device->diag_resp_buffer);

	cdf_mem_free(device);
	HIF_TRACE("-%s", __func__);
}

/**
 * hif_usb_create() - create HIF_DEVICE_USB, setup pipe resources
 * @interface: pointer to usb_interface structure
 *
 * Return: pointer to HIF_DEVICE_USB created
 */
static HIF_DEVICE_USB *hif_usb_create(struct usb_interface *interface)
{
	HIF_DEVICE_USB *device;
	struct usb_device *dev = interface_to_usbdev(interface);
	CDF_STATUS status = CDF_STATUS_SUCCESS;
	int i;
	HIF_USB_PIPE *pipe;

	HIF_TRACE("+%s", __func__);

	do {
		device = cdf_mem_malloc(sizeof(*device));
		if (NULL == device) {
			HIF_ERROR("device is NULL");
			break;
		}

		usb_set_intfdata(interface, device);
		cdf_spinlock_init(&(device->cs_lock));
		cdf_spinlock_init(&(device->rx_lock));
		cdf_spinlock_init(&(device->tx_lock));
		device->udev = dev;
		device->interface = interface;

		for (i = 0; i < HIF_USB_PIPE_MAX; i++) {
			pipe = &device->pipes[i];

			HIF_USB_INIT_WORK(pipe);

			skb_queue_head_init(&pipe->io_comp_queue);
		}

		device->diag_cmd_buffer =
			cdf_mem_malloc(USB_CTRL_MAX_DIAG_CMD_SIZE);
		if (NULL == device->diag_cmd_buffer) {
			status = CDF_STATUS_E_NOMEM;
			break;
		}
		device->diag_resp_buffer =
			cdf_mem_malloc(USB_CTRL_MAX_DIAG_RESP_SIZE);
		if (NULL == device->diag_resp_buffer) {
			status = CDF_STATUS_E_NOMEM;
			break;
		}

		status = usb_hif_setup_pipe_resources(device);

	} while (false);

	if (status != CDF_STATUS_SUCCESS) {
		HIF_ERROR("%s: abnormal condition", __func__);
		hif_usb_destroy(device);
		device = NULL;
	}

	HIF_TRACE("+%s", __func__);
	return device;
}

/**
 * hif_start() - Enable HIF TX and RX
 * @scn: pointer to ol_softc structure
 *
 * Return: CDF_STATUS_SUCCESS if success else an appropriate CDF_STATUS error
 */
CDF_STATUS hif_start(struct ol_softc *scn)
{
	HIF_DEVICE_USB *device = (HIF_DEVICE_USB *)scn->hif_hdl;
	int i;

	HIF_TRACE("+%s", __func__);
	usb_hif_prestart_recv_pipes(device);

	/* set the TX resource avail threshold for each TX pipe */
	for (i = HIF_TX_CTRL_PIPE; i <= HIF_TX_DATA_HP_PIPE; i++) {
		device->pipes[i].urb_cnt_thresh =
		    device->pipes[i].urb_alloc / 2;
	}

	HIF_TRACE("-%s", __func__);
	return CDF_STATUS_SUCCESS;
}

/**
 * hif_stop() - Stop/flush all HIF communication
 * @scn: pointer to ol_softc structure
 *
 * Return: none
 */
void hif_stop(struct ol_softc *scn)
{
	HIF_DEVICE_USB *device = (HIF_DEVICE_USB *)scn->hif_hdl;

	HIF_TRACE("+%s", __func__);

	usb_hif_flush_all(device);

	HIF_TRACE("-%s", __func__);
}

/**
 * hif_get_default_pipe() - get default pipes for HIF TX/RX
 * @scn: pointer to ol_softc structure
 * @ul_pipe: pointer to TX pipe
 * @ul_pipe: pointer to TX pipe
 *
 * Return: none
 */
void hif_get_default_pipe(struct ol_softc *scn, uint8_t *ul_pipe,
			  uint8_t *dl_pipe)
{
	*ul_pipe = HIF_TX_CTRL_PIPE;
	*dl_pipe = HIF_RX_CTRL_PIPE;
}

#if defined(USB_MULTI_IN_TEST) || defined(USB_ISOC_TEST)
/**
 * hif_map_service_to_pipe() - maps ul/dl pipe to service id.
 * @scn: HIF context
 * @svc_id: sevice index
 * @ul_pipe: pointer to uplink pipe id
 * @dl_pipe: pointer to down-linklink pipe id
 * @ul_is_polled: if ul is polling based
 * @ul_is_polled: if dl is polling based
 *
 * Return: CDF_STATUS_SUCCESS if success else an appropriate CDF_STATUS error
 */
int hif_map_service_to_pipe(struct ol_softc *scn, uint16_t svc_id,
			    uint8_t *ul_pipe, uint8_t *dl_pipe,
			    int *ul_is_polled, int *dl_is_polled)
{
	CDF_STATUS status = CDF_STATUS_SUCCESS;

	switch (svc_id) {
	case HTC_CTRL_RSVD_SVC:
	case WMI_CONTROL_SVC:
	case HTC_RAW_STREAMS_SVC:
		*ul_pipe = HIF_TX_CTRL_PIPE;
		*dl_pipe = HIF_RX_DATA_PIPE;
		break;
	case WMI_DATA_BE_SVC:
		*ul_pipe = HIF_TX_DATA_LP_PIPE;
		*dl_pipe = HIF_RX_DATA_PIPE;
		break;
	case WMI_DATA_BK_SVC:
		*ul_pipe = HIF_TX_DATA_MP_PIPE;
		*dl_pipe = HIF_RX_DATA2_PIPE;
		break;
	case WMI_DATA_VI_SVC:
		*ul_pipe = HIF_TX_DATA_HP_PIPE;
		*dl_pipe = HIF_RX_DATA_PIPE;
		break;
	case WMI_DATA_VO_SVC:
		*ul_pipe = HIF_TX_DATA_LP_PIPE;
		*dl_pipe = HIF_RX_DATA_PIPE;
		break;
	default:
		status = CDF_STATUS_E_FAILURE;
		break;
	}

	return status;
}
#else

#ifdef QCA_TX_HTT2_SUPPORT
#define USB_TX_CHECK_HTT2_SUPPORT 1
#else
#define USB_TX_CHECK_HTT2_SUPPORT 0
#endif

/**
 * hif_map_service_to_pipe() - maps ul/dl pipe to service id.
 * @scn: HIF context
 * @svc_id: sevice index
 * @ul_pipe: pointer to uplink pipe id
 * @dl_pipe: pointer to down-linklink pipe id
 * @ul_is_polled: if ul is polling based
 * @ul_is_polled: if dl is polling based
 *
 * Return: CDF_STATUS_SUCCESS if success else an appropriate CDF_STATUS error
 */
int hif_map_service_to_pipe(struct ol_softc *scn, uint16_t svc_id,
			    uint8_t *ul_pipe, uint8_t *dl_pipe,
			    int *ul_is_polled, int *dl_is_polled)
{
	CDF_STATUS status = CDF_STATUS_SUCCESS;

	switch (svc_id) {
	case HTC_CTRL_RSVD_SVC:
	case WMI_CONTROL_SVC:
		*ul_pipe = HIF_TX_CTRL_PIPE;
		*dl_pipe = HIF_RX_DATA_PIPE;
		break;
	case WMI_DATA_BE_SVC:
	case WMI_DATA_BK_SVC:
		*ul_pipe = HIF_TX_DATA_LP_PIPE;
		if (hif_usb_disable_rxdata2)
			*dl_pipe = HIF_RX_DATA_PIPE;
		else
			*dl_pipe = HIF_RX_DATA2_PIPE;
		break;
	case WMI_DATA_VI_SVC:
		*ul_pipe = HIF_TX_DATA_MP_PIPE;
		if (hif_usb_disable_rxdata2)
			*dl_pipe = HIF_RX_DATA_PIPE;
		else
			*dl_pipe = HIF_RX_DATA2_PIPE;
		break;
	case WMI_DATA_VO_SVC:
		*ul_pipe = HIF_TX_DATA_HP_PIPE;
		if (hif_usb_disable_rxdata2)
			*dl_pipe = HIF_RX_DATA_PIPE;
		else
			*dl_pipe = HIF_RX_DATA2_PIPE;
		break;
	case HTC_RAW_STREAMS_SVC:
		*ul_pipe = HIF_TX_CTRL_PIPE;
		*dl_pipe = HIF_RX_DATA_PIPE;
		break;
	case HTT_DATA_MSG_SVC:
		*ul_pipe = HIF_TX_DATA_LP_PIPE;
		if (hif_usb_disable_rxdata2)
			*dl_pipe = HIF_RX_DATA_PIPE;
		else
			*dl_pipe = HIF_RX_DATA2_PIPE;
		break;
	case HTT_DATA2_MSG_SVC:
		if (USB_TX_CHECK_HTT2_SUPPORT) {
			*ul_pipe = HIF_TX_DATA_HP_PIPE;
			if (hif_usb_disable_rxdata2)
				*dl_pipe = HIF_RX_DATA_PIPE;
			else
				*dl_pipe = HIF_RX_DATA2_PIPE;
			}
		break;
	default:
		status = CDF_STATUS_E_FAILURE;
		break;
	}

	return status;
}
#endif

/**
 * hif_ctrl_msg_exchange() - send usb ctrl message and receive response
 * @macp: pointer to HIF_DEVICE_USB
 * @send_req_val: USB send message request value
 * @send_msg: pointer to data to send
 * @len: length in bytes of the data to send
 * @response_req_val: USB response message request value
 * @response_msg: pointer to response msg
 * @response_len: length of the response message
 *
 * Return: CDF_STATUS_SUCCESS if success else an appropriate CDF_STATUS error
 */
static CDF_STATUS hif_ctrl_msg_exchange(HIF_DEVICE_USB *macp,
				uint8_t send_req_val,
				uint8_t *send_msg,
				uint32_t len,
				uint8_t response_req_val,
				uint8_t *response_msg,
				uint32_t *response_len)
{
	CDF_STATUS status;

	do {

		/* send command */
		status = usb_hif_submit_ctrl_out(macp, send_req_val, 0, 0,
						 send_msg, len);

		if (!CDF_IS_STATUS_SUCCESS(status))
			break;

		if (NULL == response_msg) {
			/* no expected response */
			break;
		}

		/* get response */
		status = usb_hif_submit_ctrl_in(macp, response_req_val, 0, 0,
						response_msg, *response_len);

		if (!CDF_IS_STATUS_SUCCESS(status))
			break;

	} while (false);

	return status;
}

/**
 * hif_exchange_bmi_msg() - send/recev ctrl message of type BMI_CMD/BMI_RESP
 * @scn: pointer to ol_softc
 * @bmi_request: pointer to data to send
 * @request_length: length in bytes of the data to send
 * @bmi_response: pointer to response msg
 * @bmi_response_length: length of the response message
 * @timeout_ms: timeout to wait for response (ignored in current implementation)
 *
 * Return: CDF_STATUS_SUCCESS if success else an appropriate CDF_STATUS error
 */
CDF_STATUS hif_exchange_bmi_msg(struct ol_softc *scn,
				uint8_t *bmi_request,
				uint32_t request_length,
				uint8_t *bmi_response,
				uint32_t *bmi_response_lengthp,
				uint32_t timeout_ms)
{
	HIF_DEVICE_USB *macp = (HIF_DEVICE_USB *) scn->hif_hdl;

	return hif_ctrl_msg_exchange(macp,
				USB_CONTROL_REQ_SEND_BMI_CMD,
				bmi_request,
				request_length,
				USB_CONTROL_REQ_RECV_BMI_RESP,
				bmi_response, bmi_response_lengthp);
}

/**
 * hif_diag_read_access() - Read data from target memory or register
 * @scn: pointer to ol_softc
 * @address: register address to read from
 * @data: pointer to buffer to store the value read from the register
 *
 * Return: CDF_STATUS_SUCCESS if success else an appropriate CDF_STATUS error
 */
CDF_STATUS hif_diag_read_access(struct ol_softc *scn, uint32_t address,
					uint32_t *data)
{
	HIF_DEVICE_USB *macp = (HIF_DEVICE_USB *)scn->hif_hdl;
	CDF_STATUS status;
	USB_CTRL_DIAG_CMD_READ *cmd;
	uint32_t respLength;

	cmd = (USB_CTRL_DIAG_CMD_READ *) macp->diag_cmd_buffer;

	cdf_mem_zero(cmd, sizeof(*cmd));
	cmd->Cmd = USB_CTRL_DIAG_CC_READ;
	cmd->Address = address;
	respLength = sizeof(USB_CTRL_DIAG_RESP_READ);

	status = hif_ctrl_msg_exchange(macp,
				USB_CONTROL_REQ_DIAG_CMD,
				(uint8_t *) cmd,
				sizeof(*cmd),
				USB_CONTROL_REQ_DIAG_RESP,
				macp->diag_resp_buffer, &respLength);

	if (CDF_IS_STATUS_SUCCESS(status)) {
		USB_CTRL_DIAG_RESP_READ *pResp =
			(USB_CTRL_DIAG_RESP_READ *) macp->diag_resp_buffer;
		*data = pResp->ReadValue;
		status = CDF_STATUS_SUCCESS;
	} else {
		status = CDF_STATUS_E_FAILURE;
	}

	return status;
}

/**
 * hif_diag_write_access() - write data to target memory or register
 * @scn: pointer to ol_softc
 * @address: register address to write to
 * @data: value to be written to the address
 *
 * Return: CDF_STATUS_SUCCESS if success else an appropriate CDF_STATUS error
 */
CDF_STATUS hif_diag_write_access(struct ol_softc *scn,
					uint32_t address,
					uint32_t data)
{
	HIF_DEVICE_USB *macp = (HIF_DEVICE_USB *)scn->hif_hdl;
	USB_CTRL_DIAG_CMD_WRITE *cmd;

	cmd = (USB_CTRL_DIAG_CMD_WRITE *) macp->diag_cmd_buffer;

	cdf_mem_zero(cmd, sizeof(*cmd));
	cmd->Cmd = USB_CTRL_DIAG_CC_WRITE;
	cmd->Address = address;
	cmd->Value = data;

	return hif_ctrl_msg_exchange(macp,
				USB_CONTROL_REQ_DIAG_CMD,
				(uint8_t *) cmd,
				sizeof(*cmd), 0, NULL, 0);
}

/**
 * hif_release_device() - release access to a particular HIF device
 * @scn: pointer to ol_softc
 *
 * Return: none
 */
void
hif_release_device(struct ol_softc *scn)
{
	HIF_DEVICE_USB *device = (HIF_DEVICE_USB *) scn->hif_hdl;

	HIF_TRACE("+%s", __func__);
	device->claimed_context = NULL;
	HIF_TRACE("-%s", __func__);
}

/**
 * hif_claim_device() - claim access to a particular HIF device
 * @scn: pointer to ol_softc
 *
 * Return: none
 */
void hif_claim_device(struct ol_softc *scn, void *claimedContext)
{
	HIF_DEVICE_USB *device = (HIF_DEVICE_USB *) scn->hif_hdl;

	device->claimed_context = claimedContext;
}

/**
 * hif_dump_info() - dump info about all HIF pipes and endpoints
 * @scn: pointer to ol_softc
 *
 * Return: none
 */
void hif_dump_info(struct ol_softc *scn)
{
	HIF_DEVICE_USB *device = (HIF_DEVICE_USB *)scn->hif_hdl;
	HIF_USB_PIPE *pipe = NULL;
	struct usb_host_interface *iface_desc = NULL;
	struct usb_endpoint_descriptor *ep_desc;
	uint8_t i = 0;

	for (i = 0; i < HIF_USB_PIPE_MAX; i++) {
		pipe = &device->pipes[i];
		HIF_ERROR("PipeIndex : %d URB Cnt : %d PipeHandle : %x",
			i, pipe->urb_cnt,
			pipe->usb_pipe_handle);
		if (usb_pipeisoc(pipe->usb_pipe_handle))
			HIF_INFO("Pipe Type ISOC");
		else if (usb_pipebulk(pipe->usb_pipe_handle))
			HIF_INFO("Pipe Type BULK");
		else if (usb_pipeint(pipe->usb_pipe_handle))
			HIF_INFO("Pipe Type INT");
		else if (usb_pipecontrol(pipe->usb_pipe_handle))
			HIF_INFO("Pipe Type control");
	}

	for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
		ep_desc = &iface_desc->endpoint[i].desc;
		if (ep_desc) {
			HIF_INFO
				("ep_desc : %p Index : %d: DescType : %d Addr : %d Maxp : %d Atrrib : %d",
				ep_desc, i, ep_desc->bDescriptorType,
				ep_desc->bEndpointAddress,
				ep_desc->wMaxPacketSize,
				ep_desc->bmAttributes);
			if ((ep_desc) && (usb_endpoint_type(ep_desc) ==
						USB_ENDPOINT_XFER_ISOC)) {
				HIF_INFO("ISOC EP Detected");
			}
		}
	}

}

/**
 * hif_dump_target_memory() - dump target memory.
 * @scn: HIF context
 * @ramdump_base: buffer location
 * @address: ramdump base address
 * @size: dump size
 *
 * Return: int
 */
void hif_dump_target_memory(struct ol_softc *scn, void *ramdump_base,
			uint32_t address, uint32_t size)
{
/* TO DO... */
}


/**
 * hif_flush_surprise_remove() - Cleanup residual buffers for device shutdown
 * @scn: HIF context
 *
 * Not applicable to USB bus
 *
 * Return: none
 */
void hif_flush_surprise_remove(struct ol_softc *scn)
{
/* TO DO... */
}

/**
 * hif_diag_read_mem() -read nbytes of data from target memory or register
 * @scn: pointer to ol_softc
 * @address: register address to read from
 * @data: buffer to store the value read
 * @nbytes: number of bytes to be read from 'address'
 *
 * Return: CDF_STATUS_SUCCESS if success else an appropriate CDF_STATUS error
 */
CDF_STATUS hif_diag_read_mem(struct ol_softc *scn, uint32_t address,
					uint8_t *data, int nbytes)
{
	CDF_STATUS status = CDF_STATUS_SUCCESS;

	HIF_TRACE("+%s", __func__);

	if ((address & 0x3) || ((uintptr_t)data & 0x3))
		return CDF_STATUS_E_IO;

	while ((nbytes >= 4) &&
		CDF_IS_STATUS_SUCCESS(status =
					hif_diag_read_access(scn,
							address,
							(uint32_t *)data))) {

		nbytes -= sizeof(uint32_t);
		address += sizeof(uint32_t);
		data += sizeof(uint32_t);

	}
	HIF_TRACE("-%s", __func__);
	return status;
}

/**
 * hif_diag_write_mem() -write  nbytes of data to target memory or register
 * @scn: pointer to ol_softc
 * @address: register address to write to
 * @data: buffer containing data to be written
 * @nbytes: number of bytes to be written
 *
 * Return: CDF_STATUS_SUCCESS if success else an appropriate CDF_STATUS error
 */
CDF_STATUS hif_diag_write_mem(struct ol_softc *scn, uint32_t address,
					uint8_t *data, int nbytes)
{
	CDF_STATUS status = CDF_STATUS_SUCCESS;
	HIF_TRACE("+%s", __func__);

	if ((address & 0x3) || ((uintptr_t)data & 0x3))
		return CDF_STATUS_E_IO;

	while (nbytes >= 4 &&
		CDF_IS_STATUS_SUCCESS(status =
					hif_diag_write_access(scn,
						address,
						*((uint32_t *)data)))) {

		nbytes -= sizeof(uint32_t);
		address += sizeof(uint32_t);
		data += sizeof(uint32_t);

	}
	HIF_TRACE("-%s", __func__);
	return status;
}

void hif_send_complete_check(struct ol_softc *scn, uint8_t PipeID,
						int force)
{
	/* NO-OP*/
}

/* diagnostic command defnitions */
#define USB_CTRL_DIAG_CC_READ       0
#define USB_CTRL_DIAG_CC_WRITE      1
#define USB_CTRL_DIAG_CC_WARM_RESET 2

/**
 * hif_diag_write_warm_reset() - send diag command to reset ROME
 * @interface: pointer to usb_interface
 * @address: register address to write to
 * @data: data to be sent
 *
 * Return: CDF_STATUS_SUCCESS if success else an appropriate CDF_STATUS error
 */
CDF_STATUS
hif_diag_write_warm_reset(struct usb_interface *interface,
					uint32_t address, uint32_t data)
{
	HIF_DEVICE_USB *hif_dev;
	CDF_STATUS status = EOK;
	USB_CTRL_DIAG_CMD_WRITE *cmd;

	hif_dev = (HIF_DEVICE_USB *) usb_get_intfdata(interface);
	cmd = (USB_CTRL_DIAG_CMD_WRITE *) hif_dev->diag_cmd_buffer;
	HIF_TRACE("+%s", __func__);
	cdf_mem_zero(cmd, sizeof(USB_CTRL_DIAG_CMD_WRITE));
	cmd->Cmd = USB_CTRL_DIAG_CC_WARM_RESET;
	cmd->Address = address;
	cmd->Value = data;

	status = hif_ctrl_msg_exchange(hif_dev,
					USB_CONTROL_REQ_DIAG_CMD,
					(uint8_t *) cmd,
					sizeof(USB_CTRL_DIAG_CMD_WRITE),
					0, NULL, 0);
	if (!CDF_IS_STATUS_SUCCESS(status))
		HIF_ERROR("-%s HIFCtrlMsgExchange fail", __func__);

	HIF_TRACE("-%s", __func__);
	return status;
}

void hif_suspend_wow(struct ol_softc *scn)
{
	HIF_INFO("HIFsuspendwow - TODO");
}

/**
 * hif_set_bundle_mode() - enable bundling and set default rx bundle cnt
 * @scn: pointer to ol_softc structure
 * @enabled: flag to enable/disable bundling
 * @rx_bundle_cnt: bundle count to be used for RX
 *
 * Return: none
 */
void hif_set_bundle_mode(struct ol_softc *scn, bool enabled,
				int rx_bundle_cnt)
{
	HIF_DEVICE_USB *device = (HIF_DEVICE_USB *) scn->hif_hdl;

	device->is_bundle_enabled = enabled;
	device->rx_bundle_cnt = rx_bundle_cnt;
	if (device->is_bundle_enabled && (device->rx_bundle_cnt == 0))
		device->rx_bundle_cnt = 1;

	device->rx_bundle_buf_len = device->rx_bundle_cnt *
					HIF_USB_RX_BUNDLE_ONE_PKT_SIZE;

	HIF_DBG("athusb bundle %s cnt %d", enabled ? "enabled" : "disabled",
			rx_bundle_cnt);
}
