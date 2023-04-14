/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <htt.h>
#include "dp_types.h"
#include "dp_internal.h"
#include "dp_rh_htt.h"
#include "dp_rh_rx.h"
#include "qdf_mem.h"
#include "cdp_txrx_cmn_struct.h"

#define HTT_MSG_BUF_SIZE(msg_bytes) \
	((msg_bytes) + HTC_HEADER_LEN + HTC_HDR_ALIGNMENT_PADDING)

#define HTT_T2H_MSG_BUF_REINIT(_buf, dev)				\
	do {								\
		QDF_NBUF_CB_PADDR(_buf) -= (HTC_HEADER_LEN +		\
					HTC_HDR_ALIGNMENT_PADDING);	\
		qdf_nbuf_init_fast((_buf));				\
		qdf_mem_dma_sync_single_for_device(dev,			\
					(QDF_NBUF_CB_PADDR(_buf)),	\
					(skb_end_pointer(_buf) -	\
					(_buf)->data),			\
					PCI_DMA_FROMDEVICE);		\
	} while (0)

/*
 * dp_htt_h2t_send_complete_free_netbuf() - Free completed buffer
 * @soc:	SOC handle
 * @status:	Completion status
 * @netbuf:	HTT buffer
 */
static void
dp_htt_h2t_send_complete_free_netbuf(
	void *soc, A_STATUS status, qdf_nbuf_t netbuf)
{
	qdf_nbuf_free(netbuf);
}

static void
dp_htt_rx_addba_handler_rh(struct dp_soc *soc, uint16_t peer_id,
			   uint8_t tid, uint16_t win_sz)
{
}

static QDF_STATUS
dp_htt_rx_delba_ind_handler_rh(void *soc_handle, uint16_t peer_id,
			       uint8_t tid, uint16_t win_sz)
{
	return QDF_STATUS_SUCCESS;
}

/**
 * dp_htt_t2h_msg_handler_fast() -  Fastpath specific message handler
 * @context: HTT context
 * @cmpl_msdus: netbuf completions
 * @num_cmpls: number of completions to be handled
 *
 * Return: None
 */
static void
dp_htt_t2h_msg_handler_fast(void *context, qdf_nbuf_t *cmpl_msdus,
			    uint32_t num_cmpls)
{
	struct htt_soc *soc = (struct htt_soc *)context;
	qdf_nbuf_t htt_t2h_msg;
	uint32_t *msg_word;
	uint32_t i;
	enum htt_t2h_msg_type msg_type;
	uint32_t msg_len;

	for (i = 0; i < num_cmpls; i++) {
		htt_t2h_msg = cmpl_msdus[i];
		msg_len = qdf_nbuf_len(htt_t2h_msg);

		/*
		 * Move the data pointer to point to HTT header
		 * past the HTC header + HTC header alignment padding
		 */
		qdf_nbuf_pull_head(htt_t2h_msg, HTC_HEADER_LEN +
				   HTC_HDR_ALIGNMENT_PADDING);

		msg_word = (uint32_t *)qdf_nbuf_data(htt_t2h_msg);
		msg_type = HTT_T2H_MSG_TYPE_GET(*msg_word);

		switch (msg_type) {
		case HTT_T2H_MSG_TYPE_RX_DATA_IND:
		{
			break;
		}
		/* TODO add support for TX completion handling */
		case HTT_T2H_MSG_TYPE_RX_PN_IND:
		{
			/* TODO check and add PN IND handling */
			break;
		}
		case HTT_T2H_MSG_TYPE_RX_ADDBA:
		{
			uint16_t peer_id;
			uint8_t tid;
			uint16_t win_sz;

			/*
			 * Update REO Queue Desc with new values
			 */
			peer_id = HTT_RX_ADDBA_PEER_ID_GET(*msg_word);
			tid = HTT_RX_ADDBA_TID_GET(*msg_word);
			win_sz = HTT_RX_ADDBA_WIN_SIZE_GET(*msg_word);

			/*
			 * Window size needs to be incremented by 1
			 * since fw needs to represent a value of 256
			 * using just 8 bits
			 */
			dp_htt_rx_addba_handler_rh(soc->dp_soc, peer_id,
						   tid, win_sz + 1);
			break;
		}
		case HTT_T2H_MSG_TYPE_RX_DELBA:
		{
			uint16_t peer_id;
			uint8_t tid;
			uint8_t win_sz;
			QDF_STATUS status;

			peer_id = HTT_RX_DELBA_PEER_ID_GET(*msg_word);
			tid = HTT_RX_DELBA_TID_GET(*msg_word);
			win_sz = HTT_RX_DELBA_WIN_SIZE_GET(*msg_word);

			status = dp_htt_rx_delba_ind_handler_rh(soc->dp_soc,
								peer_id, tid,
								win_sz);

			dp_htt_info("DELBA PeerID %d BAW %d TID %d stat %d",
				    peer_id, win_sz, tid, status);
			break;
		}
		case HTT_T2H_MSG_TYPE_PPDU_STATS_IND:
		{
			qdf_nbuf_t nbuf_copy;
			HTC_PACKET htc_pkt = {0};

			nbuf_copy = qdf_nbuf_copy(htt_t2h_msg);
			if (qdf_unlikely(!nbuf_copy)) {
				dp_htt_err("NBUF copy failed for PPDU stats msg");
				break;
			}
			htc_pkt.Status = QDF_STATUS_SUCCESS;
			htc_pkt.pPktContext = (void *)nbuf_copy;
			dp_htt_t2h_msg_handler(context, &htc_pkt);
			break;
		}
		default:
		{
			HTC_PACKET htc_pkt = {0};

			htc_pkt.Status = QDF_STATUS_SUCCESS;
			htc_pkt.pPktContext = (void *)htt_t2h_msg;
			/*
			 * Increment user count to protect buffer
			 * from generic handler free count will be
			 * reset to 1 during MSG_BUF_REINIT
			 */
			qdf_nbuf_inc_users(htt_t2h_msg);
			dp_htt_t2h_msg_handler(context, &htc_pkt);
			break;
		}

		/* Re-initialize the indication buffer */
		HTT_T2H_MSG_BUF_REINIT(htt_t2h_msg, soc->osdev);
		qdf_nbuf_set_pktlen(htt_t2h_msg, 0);
		}
	}
}

static QDF_STATUS
dp_htt_htc_attach(struct htt_soc *soc, uint16_t service_id)
{
	struct htc_service_connect_req connect;
	struct htc_service_connect_resp response;
	QDF_STATUS status;

	qdf_mem_zero(&connect, sizeof(connect));
	qdf_mem_zero(&response, sizeof(response));

	connect.pMetaData = NULL;
	connect.MetaDataLength = 0;
	connect.EpCallbacks.pContext = soc;
	connect.EpCallbacks.EpTxComplete = dp_htt_h2t_send_complete;
	connect.EpCallbacks.EpTxCompleteMultiple = NULL;
	/* fastpath handler will be used instead */
	connect.EpCallbacks.EpRecv = NULL;

	/* rx buffers currently are provided by HIF, not by EpRecvRefill */
	connect.EpCallbacks.EpRecvRefill = NULL;
	/* N/A, fill is done by HIF */
	connect.EpCallbacks.RecvRefillWaterMark = 1;

	connect.EpCallbacks.EpSendFull = dp_htt_h2t_full;
	/*
	 * Specify how deep to let a queue get before htc_send_pkt will
	 * call the EpSendFull function due to excessive send queue depth.
	 */
	connect.MaxSendQueueDepth = DP_HTT_MAX_SEND_QUEUE_DEPTH;

	/* disable flow control for HTT data message service */
	connect.ConnectionFlags |= HTC_CONNECT_FLAGS_DISABLE_CREDIT_FLOW_CTRL;

	/* connect to control service */
	connect.service_id = service_id;

	status = htc_connect_service(soc->htc_soc, &connect, &response);

	if (status != QDF_STATUS_SUCCESS) {
		dp_htt_err("HTC connect svc failed for id:%u", service_id);
		return status;
	}

	if (service_id == HTT_DATA_MSG_SVC)
		soc->htc_endpoint = response.Endpoint;

	/*
	 * TODO do we need to set tx_svc end point
	 * hif_save_htc_htt_config_endpoint(dpsoc->hif_handle,
	 * soc->htc_endpoint);
	 */
	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
dp_htt_htc_soc_attach_all(struct htt_soc *soc)
{
	struct dp_soc *dp_soc = soc->dp_soc;
	int svc_list[3] = {HTT_DATA_MSG_SVC, HTT_DATA2_MSG_SVC,
		HTT_DATA3_MSG_SVC};
	QDF_STATUS status;
	int i;

	for (i = 0; i < QDF_ARRAY_SIZE(svc_list); i++) {
		status = dp_htt_htc_attach(soc, svc_list[i]);
		if (QDF_IS_STATUS_ERROR(status))
			return status;
	}

	dp_hif_update_pipe_callback(dp_soc, (void *)soc,
				    dp_htt_hif_t2h_hp_callback,
				    DP_HTT_T2H_HP_PIPE);

	/* Register fastpath cb handlers for RX CE's */
	if (hif_ce_fastpath_cb_register(dp_soc->hif_handle,
					dp_htt_t2h_msg_handler_fast, soc)) {
		dp_htt_err("failed to register fastpath callback");
		return QDF_STATUS_E_FAILURE;
	}

	return QDF_STATUS_SUCCESS;
}

/*
 * dp_htt_soc_initialize_rh() - SOC level HTT initialization
 * @htt_soc: Opaque htt SOC handle
 * @ctrl_psoc: Opaque ctrl SOC handle
 * @htc_soc: SOC level HTC handle
 * @hal_soc: Opaque HAL SOC handle
 * @osdev: QDF device
 *
 * Return: HTT handle on success; NULL on failure
 */
void *
dp_htt_soc_initialize_rh(struct htt_soc *htt_soc,
			 struct cdp_ctrl_objmgr_psoc *ctrl_psoc,
			 HTC_HANDLE htc_soc,
			 hal_soc_handle_t hal_soc_hdl, qdf_device_t osdev)
{
	struct htt_soc *soc = (struct htt_soc *)htt_soc;

	soc->osdev = osdev;
	soc->ctrl_psoc = ctrl_psoc;
	soc->htc_soc = htc_soc;
	soc->hal_soc = hal_soc_hdl;

	if (dp_htt_htc_soc_attach_all(soc))
		goto fail2;

	return soc;

fail2:
	return NULL;
}
