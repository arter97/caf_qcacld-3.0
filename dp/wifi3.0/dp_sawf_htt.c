/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <dp_types.h>
#include <dp_peer.h>
#include <dp_internal.h>
#include <dp_htt.h>
#include <dp_sawf.h>
#include <dp_sawf_htt.h>
#include "cdp_txrx_cmn_reg.h"

#define HTT_MSG_BUF_SIZE(msg_bytes) \
	((msg_bytes) + HTC_HEADER_LEN + HTC_HDR_ALIGNMENT_PADDING)

static void
dp_htt_h2t_send_complete_free_netbuf(void *soc, A_STATUS status,
				     qdf_nbuf_t netbuf)
{
	qdf_nbuf_free(netbuf);
}

QDF_STATUS
dp_htt_sawf_get_host_queue(struct dp_soc *soc, uint32_t *msg_word,
			   uint8_t *host_queue)
{
	uint8_t cl_info, fl_override, hlos_tid;
	uint16_t ast_idx;
	hlos_tid = HTT_T2H_SAWF_MSDUQ_INFO_HTT_HLOS_TID_GET(*msg_word);
	ast_idx = HTT_T2H_SAWF_MSDUQ_INFO_HTT_AST_LIST_IDX_GET(*msg_word);

	qdf_assert_always((hlos_tid >= 0) || (hlos_tid <= DP_SAWF_TID_MAX));

	switch (soc->arch_id) {
	case CDP_ARCH_TYPE_LI:
		*host_queue = DP_SAWF_DEFAULT_Q_MAX +
			      (ast_idx - 2) * DP_SAWF_TID_MAX + hlos_tid;
		qdf_assert_always((ast_idx == 2) || (ast_idx == 3));
		break;
	case CDP_ARCH_TYPE_BE:
		cl_info =
		HTT_T2H_SAWF_MSDUQ_INFO_HTT_WHO_CLASS_INFO_SEL_GET(*msg_word);
		fl_override =
		HTT_T2H_SAWF_MSDUQ_INFO_HTT_FLOW_OVERRIDE_GET(*msg_word);
		*host_queue = (cl_info * DP_SAWF_DEFAULT_Q_MAX) + (fl_override)
			       + (hlos_tid << 1);
		break;
	default:
		dp_err("unkonwn arch_id 0x%x", soc->arch_id);
		return QDF_STATUS_E_FAILURE;
	}
	qdf_info("|AST idx: %d|HLOS TID:%d", ast_idx, hlos_tid);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
dp_htt_h2t_sawf_def_queues_map_req(struct htt_soc *soc,
				   uint8_t svc_class_id, uint16_t peer_id)
{
	qdf_nbuf_t htt_msg;
	uint32_t *msg_word;
	uint8_t *htt_logger_bufp;
	struct dp_htt_htc_pkt *pkt;
	QDF_STATUS status;

	htt_msg = qdf_nbuf_alloc(
			soc->osdev,
			HTT_MSG_BUF_SIZE(HTT_SAWF_DEF_QUEUES_MAP_REQ_BYTES),
			HTC_HEADER_LEN + HTC_HDR_ALIGNMENT_PADDING, 4, TRUE);

	if (!htt_msg) {
		dp_htt_err("Fail to allocate htt msg buffer");
		return QDF_STATUS_E_NOMEM;
	}

	if (!qdf_nbuf_put_tail(htt_msg, HTT_SAWF_DEF_QUEUES_MAP_REQ_BYTES)) {
		dp_htt_err("Fail to expand head for SAWF_DEF_QUEUES_MAP_REQ");
		qdf_nbuf_free(htt_msg);
		return QDF_STATUS_E_FAILURE;
	}

	msg_word = (uint32_t *)qdf_nbuf_data(htt_msg);

	qdf_nbuf_push_head(htt_msg, HTC_HDR_ALIGNMENT_PADDING);

	/* word 0 */
	htt_logger_bufp = (uint8_t *)msg_word;
	*msg_word = 0;

	HTT_H2T_MSG_TYPE_SET(*msg_word,
			     HTT_H2T_SAWF_DEF_QUEUES_MAP_REQ);
	HTT_RX_SAWF_DEF_QUEUES_MAP_REQ_SVC_CLASS_ID_SET(*msg_word,
							svc_class_id);
	HTT_RX_SAWF_DEF_QUEUES_MAP_REQ_PEER_ID_SET(*msg_word, peer_id);

	pkt = htt_htc_pkt_alloc(soc);
	if (!pkt) {
		dp_htt_err("Fail to allocate dp_htt_htc_pkt buffer");
		qdf_nbuf_free(htt_msg);
		return QDF_STATUS_E_NOMEM;
	}

	pkt->soc_ctxt = NULL;

	/* macro to set packet parameters for TX */
	SET_HTC_PACKET_INFO_TX(
			&pkt->htc_pkt,
			dp_htt_h2t_send_complete_free_netbuf,
			qdf_nbuf_data(htt_msg),
			qdf_nbuf_len(htt_msg),
			soc->htc_endpoint,
			HTC_TX_PACKET_TAG_RUNTIME_PUT);

	SET_HTC_PACKET_NET_BUF_CONTEXT(&pkt->htc_pkt, htt_msg);

	status = DP_HTT_SEND_HTC_PKT(soc, pkt,
				     HTT_H2T_SAWF_DEF_QUEUES_MAP_REQ,
				     htt_logger_bufp);

	if (status != QDF_STATUS_SUCCESS) {
		qdf_nbuf_free(htt_msg);
		htt_htc_pkt_free(soc, pkt);
	}

	return status;
}

QDF_STATUS
dp_htt_h2t_sawf_def_queues_unmap_req(struct htt_soc *soc,
				     uint8_t svc_id, uint16_t peer_id)
{
	qdf_nbuf_t htt_msg;
	uint32_t *msg_word;
	uint8_t *htt_logger_bufp;
	struct dp_htt_htc_pkt *pkt;
	QDF_STATUS status;

	htt_msg = qdf_nbuf_alloc(
			soc->osdev,
			HTT_MSG_BUF_SIZE(HTT_SAWF_DEF_QUEUES_UNMAP_REQ_BYTES),
			HTC_HEADER_LEN + HTC_HDR_ALIGNMENT_PADDING, 4, TRUE);

	if (!htt_msg) {
		dp_htt_err("Fail to allocate htt msg buffer");
		return QDF_STATUS_E_NOMEM;
	}

	if (!qdf_nbuf_put_tail(htt_msg, HTT_SAWF_DEF_QUEUES_UNMAP_REQ_BYTES)) {
		dp_htt_err("Fail to expand head for SAWF_DEF_QUEUES_UNMAP_REQ");
		qdf_nbuf_free(htt_msg);
		return QDF_STATUS_E_FAILURE;
	}

	msg_word = (uint32_t *)qdf_nbuf_data(htt_msg);

	qdf_nbuf_push_head(htt_msg, HTC_HDR_ALIGNMENT_PADDING);

	/* word 0 */
	htt_logger_bufp = (uint8_t *)msg_word;
	*msg_word = 0;

	HTT_H2T_MSG_TYPE_SET(*msg_word,
			     HTT_H2T_SAWF_DEF_QUEUES_UNMAP_REQ);
	HTT_RX_SAWF_DEF_QUEUES_UNMAP_REQ_SVC_CLASS_ID_SET(*msg_word, svc_id);
	HTT_RX_SAWF_DEF_QUEUES_UNMAP_REQ_PEER_ID_SET(*msg_word, peer_id);

	pkt = htt_htc_pkt_alloc(soc);
	if (!pkt) {
		dp_htt_err("Fail to allocate dp_htt_htc_pkt buffer");
		qdf_nbuf_free(htt_msg);
		return QDF_STATUS_E_NOMEM;
	}

	pkt->soc_ctxt = NULL;

	/* macro to set packet parameters for TX */
	SET_HTC_PACKET_INFO_TX(
			&pkt->htc_pkt,
			dp_htt_h2t_send_complete_free_netbuf,
			qdf_nbuf_data(htt_msg),
			qdf_nbuf_len(htt_msg),
			soc->htc_endpoint,
			HTC_TX_PACKET_TAG_RUNTIME_PUT);

	SET_HTC_PACKET_NET_BUF_CONTEXT(&pkt->htc_pkt, htt_msg);

	status = DP_HTT_SEND_HTC_PKT(
			soc, pkt,
			HTT_H2T_SAWF_DEF_QUEUES_UNMAP_REQ, htt_logger_bufp);

	if (status != QDF_STATUS_SUCCESS) {
		qdf_nbuf_free(htt_msg);
		htt_htc_pkt_free(soc, pkt);
	}

	return status;
}

QDF_STATUS
dp_htt_h2t_sawf_def_queues_map_report_req(struct htt_soc *soc,
					  uint16_t peer_id, uint8_t tid_mask)
{
	qdf_nbuf_t htt_msg;
	uint32_t *msg_word;
	uint8_t *htt_logger_bufp;
	struct dp_htt_htc_pkt *pkt;
	QDF_STATUS status;

	htt_msg = qdf_nbuf_alloc(
			soc->osdev,
			HTT_MSG_BUF_SIZE(HTT_SAWF_DEF_QUEUES_MAP_REPORT_REQ_BYTES),
			HTC_HEADER_LEN + HTC_HDR_ALIGNMENT_PADDING, 4, TRUE);

	if (!htt_msg) {
		dp_htt_err("Fail to allocate htt msg buffer");
		return QDF_STATUS_E_NOMEM;
	}

	if (!qdf_nbuf_put_tail(htt_msg,
			       HTT_SAWF_DEF_QUEUES_MAP_REPORT_REQ_BYTES)) {
		dp_htt_err("Fail to expand head for SAWF_DEF_QUEUES_MAP_REQ");
		qdf_nbuf_free(htt_msg);
		return QDF_STATUS_E_FAILURE;
	}

	msg_word = (uint32_t *)qdf_nbuf_data(htt_msg);

	qdf_nbuf_push_head(htt_msg, HTC_HDR_ALIGNMENT_PADDING);

	/* word 0 */
	htt_logger_bufp = (uint8_t *)msg_word;
	*msg_word = 0;

	HTT_H2T_MSG_TYPE_SET(*msg_word, HTT_H2T_SAWF_DEF_QUEUES_MAP_REPORT_REQ);
	HTT_RX_SAWF_DEF_QUEUES_MAP_REPORT_REQ_TID_MASK_SET(*msg_word, tid_mask);
	HTT_RX_SAWF_DEF_QUEUES_MAP_REPORT_REQ_PEER_ID_SET(*msg_word, peer_id);

	pkt = htt_htc_pkt_alloc(soc);
	if (!pkt) {
		dp_htt_err("Fail to allocate dp_htt_htc_pkt buffer");
		qdf_nbuf_free(htt_msg);
		return QDF_STATUS_E_NOMEM;
	}

	pkt->soc_ctxt = NULL; /* not used during send-done callback */

	/* macro to set packet parameters for TX */
	SET_HTC_PACKET_INFO_TX(
			&pkt->htc_pkt,
			dp_htt_h2t_send_complete_free_netbuf,
			qdf_nbuf_data(htt_msg),
			qdf_nbuf_len(htt_msg),
			soc->htc_endpoint,
			HTC_TX_PACKET_TAG_RUNTIME_PUT);

	SET_HTC_PACKET_NET_BUF_CONTEXT(&pkt->htc_pkt, htt_msg);

	status = DP_HTT_SEND_HTC_PKT(
			soc, pkt,
			HTT_H2T_SAWF_DEF_QUEUES_MAP_REPORT_REQ,
			htt_logger_bufp);

	if (status != QDF_STATUS_SUCCESS) {
		qdf_nbuf_free(htt_msg);
		htt_htc_pkt_free(soc, pkt);
	}

	return status;
}

QDF_STATUS
dp_htt_sawf_def_queues_map_report_conf(struct htt_soc *soc,
				       uint32_t *msg_word,
				       qdf_nbuf_t htt_t2h_msg)
{
	int tid_report_bytes, rem_bytes;
	uint16_t peer_id;
	uint8_t tid, svc_class_id;
	struct dp_peer *peer;
	struct dp_peer_sawf *sawf_ctx;

	peer_id = HTT_T2H_SAWF_DEF_QUEUES_MAP_REPORT_CONF_PEER_ID_GET(*msg_word);

	peer = dp_peer_get_ref_by_id(soc->dp_soc, peer_id,
				     DP_MOD_ID_HTT);
	if (!peer) {
		qdf_info("Invalid peer");
		return QDF_STATUS_E_FAILURE;
	}

	sawf_ctx = dp_peer_sawf_ctx_get(peer);
	if (!sawf_ctx) {
		dp_peer_unref_delete(peer, DP_MOD_ID_CDP);
		qdf_err("Invalid SAWF ctx");
		return QDF_STATUS_E_FAILURE;
	}

	/*
	 * Target can send multiple tid reports in the response
	 * based on the report request, tid_report_bytes is the length of
	 * all tid reports in bytes.
	 */
	tid_report_bytes = qdf_nbuf_len(htt_t2h_msg) -
		HTT_SAWF_DEF_QUEUES_MAP_REPORT_CONF_HDR_BYTES;

	for (rem_bytes = tid_report_bytes; rem_bytes > 0;
		rem_bytes -= HTT_SAWF_DEF_QUEUES_MAP_REPORT_CONF_ELEM_BYTES) {
		++msg_word;
		tid = HTT_T2H_SAWF_DEF_QUEUES_MAP_REPORT_CONF_TID_GET(*msg_word);
		svc_class_id = HTT_T2H_SAWF_DEF_QUEUES_MAP_REPORT_CONF_SVC_CLASS_ID_GET(*msg_word);
		qdf_info("peer id %u tid 0x%x svc_class_id %u",
			 peer_id, tid, svc_class_id);
		qdf_assert(tid < DP_SAWF_MAX_TIDS);
		/* update tid report */
		sawf_ctx->tid_reports[tid].svc_class_id = svc_class_id;
	}

	dp_peer_unref_delete(peer, DP_MOD_ID_HTT);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
dp_htt_sawf_msduq_map(struct htt_soc *soc, uint32_t *msg_word,
		      qdf_nbuf_t htt_t2h_msg)
{
	uint16_t peer_id;
	struct dp_peer *peer;
	struct dp_peer_sawf *sawf_ctx;
	uint8_t tid_queue;
	uint8_t host_queue, remapped_tid;
	struct dp_sawf_msduq *msduq;
	struct dp_sawf_msduq_tid_map msduq_map;
	uint8_t host_tid_queue;
	uint8_t msduq_index = 0;
	struct dp_peer *primary_link_peer = NULL;

	peer_id = HTT_T2H_SAWF_MSDUQ_INFO_HTT_PEER_ID_GET(*msg_word);
	tid_queue = HTT_T2H_SAWF_MSDUQ_INFO_HTT_QTYPE_GET(*msg_word);

	peer = dp_peer_get_ref_by_id(soc->dp_soc, peer_id,
				     DP_MOD_ID_SAWF);
	if (!peer) {
		qdf_err("Invalid peer");
		return QDF_STATUS_E_FAILURE;
	}

	if (IS_MLO_DP_MLD_PEER(peer)) {
		primary_link_peer = dp_get_primary_link_peer_by_id(soc->dp_soc,
					peer->peer_id, DP_MOD_ID_SAWF);
		if (!primary_link_peer) {
			dp_peer_unref_delete(peer, DP_MOD_ID_SAWF);
			qdf_err("Invalid peer");
			return QDF_STATUS_E_FAILURE;
		}

		/*
		 * Release the MLD-peer reference.
		 * Hold only primary link ref now.
		 */
		dp_peer_unref_delete(peer, DP_MOD_ID_SAWF);
		peer = primary_link_peer;
	}

	sawf_ctx = dp_peer_sawf_ctx_get(peer);
	if (!sawf_ctx) {
		dp_peer_unref_delete(peer, DP_MOD_ID_SAWF);
		qdf_err("Invalid SAWF ctx");
		return QDF_STATUS_E_FAILURE;
	}

	msg_word++;
	remapped_tid = HTT_T2H_SAWF_MSDUQ_INFO_HTT_REMAP_TID_GET(*msg_word);

	if (dp_htt_sawf_get_host_queue(soc->dp_soc, msg_word, &host_queue) ==
		QDF_STATUS_E_FAILURE) {
		dp_peer_unref_delete(peer, DP_MOD_ID_SAWF);
		qdf_err("Invalid host queue");
		return QDF_STATUS_E_FAILURE;
	}

	qdf_info("|TID Q:%d|Remapped TID:%d|Host Q:%d|",
		 tid_queue, remapped_tid, host_queue);

	host_tid_queue = tid_queue - DP_SAWF_DEFAULT_Q_PTID_MAX;

	if (remapped_tid < DP_SAWF_TID_MAX &&
	    host_tid_queue < DP_SAWF_DEFINED_Q_PTID_MAX) {
		msduq_map = sawf_ctx->msduq_map[remapped_tid][host_tid_queue];
		msduq_map.host_queue_id = host_queue;
	}

	msduq_index = host_queue - DP_SAWF_DEFAULT_Q_MAX;

	if (msduq_index < DP_SAWF_Q_MAX) {
		msduq = &sawf_ctx->msduq[msduq_index];
		msduq->remapped_tid = remapped_tid;
		msduq->htt_msduq = host_tid_queue;
	}

	dp_peer_unref_delete(peer, DP_MOD_ID_SAWF);

	return QDF_STATUS_SUCCESS;
}
