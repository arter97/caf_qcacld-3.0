/*
 * Copyright (c) 2012-2019 The Linux Foundation. All rights reserved.
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
 * This file lim_send_sme_rspMessages.cc contains the functions
 * for sending SME response/notification messages to applications
 * above MAC software.
 * Author:        Chandra Modumudi
 * Date:          02/13/02
 * History:-
 * Date           Modified by    Modification Information
 * --------------------------------------------------------------------
 */

#include "qdf_types.h"
#include "wni_api.h"
#include "sir_common.h"
#include "ani_global.h"

#include "wni_cfg.h"
#include "sys_def.h"
#include "cfg_api.h"

#include "sch_api.h"
#include "utils_api.h"
#include "lim_utils.h"
#include "lim_security_utils.h"
#include "lim_ser_des_utils.h"
#include "lim_send_sme_rsp_messages.h"
#include "lim_ibss_peer_mgmt.h"
#include "lim_session_utils.h"
#include "lim_types.h"
#include "sir_api.h"
#include "cds_regdomain.h"
#include "lim_send_messages.h"
#include "nan_datapath.h"
#include "lim_assoc_utils.h"
#include "wlan_reg_services_api.h"
#include "wlan_utility.h"

#include "wlan_tdls_tgt_api.h"
#include "lim_process_fils.h"
#include "wma.h"

static void lim_handle_join_rsp_status(struct mac_context *mac_ctx,
	struct pe_session *session_entry, tSirResultCodes result_code,
	tpSirSmeJoinRsp sme_join_rsp);

/**
 * lim_send_sme_rsp() - Send Response to upper layers
 * @mac_ctx:          Pointer to Global MAC structure
 * @msg_type:         Indicates message type
 * @result_code:       Indicates the result of previously issued
 *                    eWNI_SME_msg_type_REQ message
 *
 * This function is called by lim_process_sme_req_messages() to send
 * eWNI_SME_START_RSP, eWNI_SME_STOP_BSS_RSP
 * or eWNI_SME_SWITCH_CHL_RSP messages to applications above MAC
 * Software.
 *
 * Return: None
 */

void
lim_send_sme_rsp(struct mac_context *mac_ctx, uint16_t msg_type,
	 tSirResultCodes result_code, uint8_t sme_session_id,
	 uint16_t sme_transaction_id)
{
	struct scheduler_msg msg = {0};
	tSirSmeRsp *sme_rsp;

	pe_debug("Sending message: %s with reasonCode: %s",
		lim_msg_str(msg_type), lim_result_code_str(result_code));

	sme_rsp = qdf_mem_malloc(sizeof(tSirSmeRsp));
	if (!sme_rsp)
		return;

	sme_rsp->messageType = msg_type;
	sme_rsp->length = sizeof(tSirSmeRsp);
	sme_rsp->statusCode = result_code;

	sme_rsp->sessionId = sme_session_id;
	sme_rsp->transactionId = sme_transaction_id;

	msg.type = msg_type;
	msg.bodyptr = sme_rsp;
	msg.bodyval = 0;
	MTRACE(mac_trace(mac_ctx, TRACE_CODE_TX_SME_MSG,
			 sme_session_id, msg.type));

#ifdef FEATURE_WLAN_DIAG_SUPPORT_LIM    /* FEATURE_WLAN_DIAG_SUPPORT */
	switch (msg_type) {
	case eWNI_SME_STOP_BSS_RSP:
		lim_diag_event_report(mac_ctx, WLAN_PE_DIAG_STOP_BSS_RSP_EVENT,
				NULL, (uint16_t) result_code, 0);
		break;
	}
#endif /* FEATURE_WLAN_DIAG_SUPPORT */
	lim_sys_process_mmh_msg_api(mac_ctx, &msg);
}

/**
 * lim_get_max_rate_flags() - Get rate flags
 * @mac_ctx: Pointer to global MAC structure
 * @sta_ds: Pointer to station ds structure
 *
 * This function is called to get the rate flags for a connection
 * from the station ds structure depending on the ht and the vht
 * channel width supported.
 *
 * Return: Returns the populated rate_flags
 */
uint32_t lim_get_max_rate_flags(struct mac_context *mac_ctx, tpDphHashNode sta_ds)
{
	uint32_t rate_flags = 0;

	if (sta_ds == NULL) {
		pe_err("sta_ds is NULL");
		return rate_flags;
	}

	if (!sta_ds->mlmStaContext.htCapability &&
	    !sta_ds->mlmStaContext.vhtCapability) {
		rate_flags |= TX_RATE_LEGACY;
	} else {
		if (sta_ds->mlmStaContext.vhtCapability) {
			if (WNI_CFG_VHT_CHANNEL_WIDTH_80MHZ ==
				sta_ds->vhtSupportedChannelWidthSet) {
				rate_flags |= TX_RATE_VHT80;
			} else if (WNI_CFG_VHT_CHANNEL_WIDTH_20_40MHZ ==
					sta_ds->vhtSupportedChannelWidthSet) {
				if (sta_ds->htSupportedChannelWidthSet)
					rate_flags |= TX_RATE_VHT40;
				else
					rate_flags |= TX_RATE_VHT20;
			}
		} else if (sta_ds->mlmStaContext.htCapability) {
			if (sta_ds->htSupportedChannelWidthSet)
				rate_flags |= TX_RATE_HT40;
			else
				rate_flags |= TX_RATE_HT20;
		}
	}

	if (sta_ds->htShortGI20Mhz || sta_ds->htShortGI40Mhz)
		rate_flags |= TX_RATE_SGI;

	return rate_flags;
}

/**
 * lim_send_sme_join_reassoc_rsp_after_resume() - Send Response to SME
 * @mac_ctx          Pointer to Global MAC structure
 * @status           Resume link status
 * @ctx              context passed while calling resmune link.
 *                   (join response to be sent)
 *
 * This function is called to send Join/Reassoc rsp
 * message to SME after the resume link.
 *
 * Return: None
 */
static void lim_send_sme_join_reassoc_rsp_after_resume(struct mac_context *mac_ctx,
	QDF_STATUS status, uint32_t *ctx)
{
	struct scheduler_msg msg = {0};
	tpSirSmeJoinRsp sme_join_rsp = (tpSirSmeJoinRsp) ctx;

	msg.type = sme_join_rsp->messageType;
	msg.bodyptr = sme_join_rsp;
	msg.bodyval = 0;
	MTRACE(mac_trace(mac_ctx, TRACE_CODE_TX_SME_MSG, NO_SESSION, msg.type));
	lim_sys_process_mmh_msg_api(mac_ctx, &msg);
}

/**
 * lim_handle_join_rsp_status() - Handle the response.
 * @mac_ctx:            Pointer to Global MAC structure
 * @session_entry:      PE Session Info
 * @result_code:        Indicates the result of previously issued
 *                      eWNI_SME_msgType_REQ message
 * @sme_join_rsp        The received response.
 *
 * This function will handle both the success and failure status
 * of the received response.
 *
 * Return: None
 */
static void lim_handle_join_rsp_status(struct mac_context *mac_ctx,
	struct pe_session *session_entry, tSirResultCodes result_code,
	tpSirSmeJoinRsp sme_join_rsp)
{
	uint16_t bss_ie_len;
	void *bss_ies;
	bool is_vendor_ap_1_present;
	tpSirSmeJoinReq join_reassoc_req = NULL;

#ifdef FEATURE_WLAN_MCC_TO_SCC_SWITCH
	tSirSmeHTProfile *ht_profile;
#endif
	if (result_code == eSIR_SME_SUCCESS) {
		if (session_entry->beacon != NULL) {
			sme_join_rsp->beaconLength = session_entry->bcnLen;
			qdf_mem_copy(sme_join_rsp->frames,
				session_entry->beacon,
				sme_join_rsp->beaconLength);
			qdf_mem_free(session_entry->beacon);
			session_entry->beacon = NULL;
			session_entry->bcnLen = 0;
			pe_debug("Beacon: %d",
				sme_join_rsp->beaconLength);
		}
		if (session_entry->assocReq != NULL) {
			sme_join_rsp->assocReqLength =
				session_entry->assocReqLen;
			qdf_mem_copy(sme_join_rsp->frames +
				sme_join_rsp->beaconLength,
				session_entry->assocReq,
				sme_join_rsp->assocReqLength);
			qdf_mem_free(session_entry->assocReq);
			session_entry->assocReq = NULL;
			session_entry->assocReqLen = 0;
			pe_debug("AssocReq: %d",
				sme_join_rsp->assocReqLength);
		}
		if (session_entry->assocRsp != NULL) {
			sme_join_rsp->assocRspLength =
				session_entry->assocRspLen;
			qdf_mem_copy(sme_join_rsp->frames +
				sme_join_rsp->beaconLength +
				sme_join_rsp->assocReqLength,
				session_entry->assocRsp,
				sme_join_rsp->assocRspLength);
			qdf_mem_free(session_entry->assocRsp);
			session_entry->assocRsp = NULL;
			session_entry->assocRspLen = 0;
		}
		if (session_entry->ricData != NULL) {
			sme_join_rsp->parsedRicRspLen =
				session_entry->RICDataLen;
			qdf_mem_copy(sme_join_rsp->frames +
				sme_join_rsp->beaconLength +
				sme_join_rsp->assocReqLength +
				sme_join_rsp->assocRspLength,
				session_entry->ricData,
				sme_join_rsp->parsedRicRspLen);
			qdf_mem_free(session_entry->ricData);
			session_entry->ricData = NULL;
			session_entry->RICDataLen = 0;
			pe_debug("RicLength: %d",
				sme_join_rsp->parsedRicRspLen);
		}
#ifdef FEATURE_WLAN_ESE
		if (session_entry->tspecIes != NULL) {
			sme_join_rsp->tspecIeLen =
				session_entry->tspecLen;
			qdf_mem_copy(sme_join_rsp->frames +
				sme_join_rsp->beaconLength +
				sme_join_rsp->assocReqLength +
				sme_join_rsp->assocRspLength +
				sme_join_rsp->parsedRicRspLen,
				session_entry->tspecIes,
				sme_join_rsp->tspecIeLen);
			qdf_mem_free(session_entry->tspecIes);
			session_entry->tspecIes = NULL;
			session_entry->tspecLen = 0;
			pe_debug("ESE-TspecLen: %d",
				sme_join_rsp->tspecIeLen);
		}
#endif
		sme_join_rsp->aid = session_entry->limAID;
		pe_debug("AssocRsp: %d",
			sme_join_rsp->assocRspLength);
		sme_join_rsp->vht_channel_width =
			session_entry->ch_width;
#ifdef FEATURE_WLAN_MCC_TO_SCC_SWITCH
		if (session_entry->cc_switch_mode !=
				QDF_MCC_TO_SCC_SWITCH_DISABLE) {
			ht_profile = &sme_join_rsp->HTProfile;
			ht_profile->htSupportedChannelWidthSet =
				session_entry->htSupportedChannelWidthSet;
			ht_profile->htRecommendedTxWidthSet =
				session_entry->htRecommendedTxWidthSet;
			ht_profile->htSecondaryChannelOffset =
				session_entry->htSecondaryChannelOffset;
			ht_profile->dot11mode = session_entry->dot11mode;
			ht_profile->htCapability = session_entry->htCapability;
			ht_profile->vhtCapability =
				session_entry->vhtCapability;
			ht_profile->apCenterChan = session_entry->ch_center_freq_seg0;
			ht_profile->apChanWidth = session_entry->ch_width;
		}
#endif
		pe_debug("pLimJoinReq:%pK, pLimReAssocReq:%pK",
			session_entry->pLimJoinReq,
			session_entry->pLimReAssocReq);

		if (session_entry->pLimJoinReq)
			join_reassoc_req = session_entry->pLimJoinReq;

		if (session_entry->pLimReAssocReq)
			join_reassoc_req = session_entry->pLimReAssocReq;

		if (!join_reassoc_req) {
			pe_err("both  pLimJoinReq and pLimReAssocReq NULL");
			return;
		}

		bss_ie_len = lim_get_ielen_from_bss_description(
				&join_reassoc_req->bssDescription);
		bss_ies = &join_reassoc_req->bssDescription.ieFields;
		is_vendor_ap_1_present = (wlan_get_vendor_ie_ptr_from_oui(
			SIR_MAC_VENDOR_AP_1_OUI, SIR_MAC_VENDOR_AP_1_OUI_LEN,
			bss_ies, bss_ie_len) != NULL);

		if (mac_ctx->roam.configParam.is_force_1x1 &&
		    is_vendor_ap_1_present && (session_entry->nss == 2) &&
		    (mac_ctx->lteCoexAntShare == 0 ||
				IS_5G_CH(session_entry->currentOperChannel))) {
			/* SET vdev param */
			pe_debug("sending SMPS intolrent vdev_param");
			wma_cli_set_command(session_entry->smeSessionId,
					   (int)WMI_VDEV_PARAM_SMPS_INTOLERANT,
					    1, VDEV_CMD);

		}
	} else {
		if (session_entry->beacon != NULL) {
			qdf_mem_free(session_entry->beacon);
			session_entry->beacon = NULL;
			session_entry->bcnLen = 0;
		}
		if (session_entry->assocReq != NULL) {
			qdf_mem_free(session_entry->assocReq);
			session_entry->assocReq = NULL;
			session_entry->assocReqLen = 0;
		}
		if (session_entry->assocRsp != NULL) {
			qdf_mem_free(session_entry->assocRsp);
			session_entry->assocRsp = NULL;
			session_entry->assocRspLen = 0;
		}
		if (session_entry->ricData != NULL) {
			qdf_mem_free(session_entry->ricData);
			session_entry->ricData = NULL;
			session_entry->RICDataLen = 0;
		}
#ifdef FEATURE_WLAN_ESE
		if (session_entry->tspecIes != NULL) {
			qdf_mem_free(session_entry->tspecIes);
			session_entry->tspecIes = NULL;
			session_entry->tspecLen = 0;
		}
#endif
	}
}

/**
 * lim_add_bss_info() - copy data from session entry to join rsp
 * @sta_ds: Station dph entry
 * @sme_join_rsp: Join response buffer to be filled up
 *
 * Return: None
 */
static void lim_add_bss_info(tpDphHashNode sta_ds, tpSirSmeJoinRsp sme_join_rsp)
{
	struct parsed_ies *parsed_ies = &sta_ds->parsed_ies;

	if (parsed_ies->hs20vendor_ie.present)
		sme_join_rsp->hs20vendor_ie = parsed_ies->hs20vendor_ie;
	if (parsed_ies->vht_caps.present)
		sme_join_rsp->vht_caps = parsed_ies->vht_caps;
	if (parsed_ies->ht_caps.present)
		sme_join_rsp->ht_caps = parsed_ies->ht_caps;
	if (parsed_ies->ht_operation.present)
		sme_join_rsp->ht_operation = parsed_ies->ht_operation;
	if (parsed_ies->vht_operation.present)
		sme_join_rsp->vht_operation = parsed_ies->vht_operation;
}

#ifdef WLAN_FEATURE_FILS_SK
static void lim_update_fils_seq_num(tpSirSmeJoinRsp sme_join_rsp,
				    struct pe_session *session_entry)
{
	sme_join_rsp->fils_seq_num =
		session_entry->fils_info->sequence_number;
}
#else
static inline void lim_update_fils_seq_num(tpSirSmeJoinRsp sme_join_rsp,
					   struct pe_session *session_entry)
{}
#endif
/**
 * lim_send_sme_join_reassoc_rsp() - Send Response to Upper Layers
 * @mac_ctx:            Pointer to Global MAC structure
 * @msg_type:           Indicates message type
 * @result_code:        Indicates the result of previously issued
 *                      eWNI_SME_msgType_REQ message
 * @prot_status_code:   Protocol Status Code
 * @session_entry:      PE Session Info
 * @sme_session_id:     SME Session ID
 * @sme_transaction_id: SME Transaction ID
 *
 * This function is called by lim_process_sme_req_messages() to send
 * eWNI_SME_JOIN_RSP or eWNI_SME_REASSOC_RSP messages to applications
 * above MAC Software.
 *
 * Return: None
 */

void
lim_send_sme_join_reassoc_rsp(struct mac_context *mac_ctx, uint16_t msg_type,
	tSirResultCodes result_code, uint16_t prot_status_code,
	struct pe_session *session_entry, uint8_t sme_session_id,
	uint16_t sme_transaction_id)
{
	tpSirSmeJoinRsp sme_join_rsp;
	uint32_t rsp_len;
	tpDphHashNode sta_ds = NULL;
#ifdef FEATURE_WLAN_DIAG_SUPPORT_LIM    /* FEATURE_WLAN_DIAG_SUPPORT */
	if (msg_type == eWNI_SME_REASSOC_RSP)
		lim_diag_event_report(mac_ctx, WLAN_PE_DIAG_REASSOC_RSP_EVENT,
			session_entry, (uint16_t) result_code, 0);
	else
		lim_diag_event_report(mac_ctx, WLAN_PE_DIAG_JOIN_RSP_EVENT,
			session_entry, (uint16_t) result_code, 0);
#endif /* FEATURE_WLAN_DIAG_SUPPORT */

	pe_debug("Sending message: %s with reasonCode: %s",
		lim_msg_str(msg_type), lim_result_code_str(result_code));

	if (session_entry == NULL) {
		rsp_len = sizeof(tSirSmeJoinRsp);
		sme_join_rsp = qdf_mem_malloc(rsp_len);
		if (!sme_join_rsp)
			return;

		sme_join_rsp->beaconLength = 0;
		sme_join_rsp->assocReqLength = 0;
		sme_join_rsp->assocRspLength = 0;
	} else {
		rsp_len = session_entry->assocReqLen +
			session_entry->assocRspLen + session_entry->bcnLen +
			session_entry->RICDataLen +
#ifdef FEATURE_WLAN_ESE
			session_entry->tspecLen +
#endif
			sizeof(tSirSmeJoinRsp) - sizeof(uint8_t);
		sme_join_rsp = qdf_mem_malloc(rsp_len);
		if (!sme_join_rsp)
			return;

		if (lim_is_fils_connection(session_entry)) {
			sme_join_rsp->is_fils_connection = true;
			lim_update_fils_seq_num(sme_join_rsp,
						session_entry);
		}

		if (result_code == eSIR_SME_SUCCESS) {
			sta_ds = dph_get_hash_entry(mac_ctx,
				DPH_STA_HASH_INDEX_PEER,
				&session_entry->dph.dphHashTable);
			if (sta_ds == NULL) {
				pe_err("Get Self Sta Entry fail");
			} else {
				/* Pass the peer's staId */
				sme_join_rsp->staId = sta_ds->staIndex;
				sme_join_rsp->timingMeasCap =
					sta_ds->timingMeasCap;
#ifdef FEATURE_WLAN_TDLS
				sme_join_rsp->tdls_prohibited =
					session_entry->tdls_prohibited;
				sme_join_rsp->tdls_chan_swit_prohibited =
				   session_entry->tdls_chan_swit_prohibited;
#endif
				sme_join_rsp->nss = sta_ds->nss;
				sme_join_rsp->max_rate_flags =
					lim_get_max_rate_flags(mac_ctx, sta_ds);
				lim_add_bss_info(sta_ds, sme_join_rsp);

				/* Copy FILS params only for Successful join */
				populate_fils_connect_params(mac_ctx,
						session_entry, sme_join_rsp);
			}
		}

		sme_join_rsp->beaconLength = 0;
		sme_join_rsp->assocReqLength = 0;
		sme_join_rsp->assocRspLength = 0;
		sme_join_rsp->parsedRicRspLen = 0;
#ifdef FEATURE_WLAN_ESE
		sme_join_rsp->tspecIeLen = 0;
#endif
		lim_handle_join_rsp_status(mac_ctx, session_entry, result_code,
			sme_join_rsp);

		/* Send supported NSS 1x1 to SME */
		sme_join_rsp->supported_nss_1x1 =
			session_entry->supported_nss_1x1;
		pe_debug("SME Join Rsp is supported NSS 1X1: %d",
		       sme_join_rsp->supported_nss_1x1);
	}

	sme_join_rsp->messageType = msg_type;
	sme_join_rsp->length = (uint16_t) rsp_len;
	sme_join_rsp->statusCode = result_code;
	sme_join_rsp->protStatusCode = prot_status_code;

	sme_join_rsp->sessionId = sme_session_id;
	sme_join_rsp->transactionId = sme_transaction_id;

	lim_send_sme_join_reassoc_rsp_after_resume(mac_ctx, QDF_STATUS_SUCCESS,
			(uint32_t *)sme_join_rsp);
}

/**
 * lim_send_sme_start_bss_rsp()
 *
 ***FUNCTION:
 * This function is called to send eWNI_SME_START_BSS_RSP
 * message to applications above MAC Software.
 *
 ***PARAMS:
 *
 ***LOGIC:
 *
 ***ASSUMPTIONS:
 * NA
 *
 ***NOTE:
 * NA
 *
 * @param mac         Pointer to Global MAC structure
 * @param msgType      Indicates message type
 * @param resultCode   Indicates the result of previously issued
 *                     eWNI_SME_msgType_REQ message
 *
 * @return None
 */

void
lim_send_sme_start_bss_rsp(struct mac_context *mac,
			   uint16_t msgType, tSirResultCodes resultCode,
			   struct pe_session *pe_session, uint8_t smesessionId,
			   uint16_t smetransactionId)
{

	uint16_t size = 0;
	struct scheduler_msg mmhMsg = {0};
	tSirSmeStartBssRsp *pSirSmeRsp;
	uint16_t ieLen;
	uint16_t ieOffset, curLen;

	pe_debug("Sending message: %s with reasonCode: %s",
		       lim_msg_str(msgType), lim_result_code_str(resultCode));

	size = sizeof(tSirSmeStartBssRsp);

	if (pe_session == NULL) {
		pSirSmeRsp = qdf_mem_malloc(size);
		if (!pSirSmeRsp)
			return;
	} else {
		/* subtract size of beaconLength + Mac Hdr + Fixed Fields before SSID */
		ieOffset = sizeof(tAniBeaconStruct) + SIR_MAC_B_PR_SSID_OFFSET;
		ieLen = pe_session->schBeaconOffsetBegin
			+ pe_session->schBeaconOffsetEnd - ieOffset;
		/* calculate the memory size to allocate */
		size += ieLen;

		pSirSmeRsp = qdf_mem_malloc(size);
		if (!pSirSmeRsp)
			return;
		size = sizeof(tSirSmeStartBssRsp);
		if (resultCode == eSIR_SME_SUCCESS) {

			sir_copy_mac_addr(pSirSmeRsp->bssDescription.bssId,
					  pe_session->bssId);

			/* Read beacon interval from session */
			pSirSmeRsp->bssDescription.beaconInterval =
				(uint16_t) pe_session->beaconParams.
				beaconInterval;
			pSirSmeRsp->bssType = pe_session->bssType;

			if (cfg_get_capability_info
				    (mac, &pSirSmeRsp->bssDescription.capabilityInfo,
				    pe_session)
			    != QDF_STATUS_SUCCESS)
				pe_err("could not retrieve Capabilities value");

			lim_get_phy_mode(mac,
					 (uint32_t *) &pSirSmeRsp->bssDescription.
					 nwType, pe_session);

			pSirSmeRsp->bssDescription.channelId =
				pe_session->currentOperChannel;

		if (!LIM_IS_NDI_ROLE(pe_session)) {
			curLen = pe_session->schBeaconOffsetBegin - ieOffset;
			qdf_mem_copy((uint8_t *) &pSirSmeRsp->bssDescription.
				     ieFields,
				     pe_session->pSchBeaconFrameBegin +
				     ieOffset, (uint32_t) curLen);

			qdf_mem_copy(((uint8_t *) &pSirSmeRsp->bssDescription.
				      ieFields) + curLen,
				     pe_session->pSchBeaconFrameEnd,
				     (uint32_t) pe_session->
				     schBeaconOffsetEnd);

			pSirSmeRsp->bssDescription.length = (uint16_t)
				(offsetof(tSirBssDescription, ieFields[0])
				- sizeof(pSirSmeRsp->bssDescription.length)
				+ ieLen);
			/* This is the size of the message, subtracting the size of the pointer to ieFields */
			size += ieLen - sizeof(uint32_t);
		}
#ifdef FEATURE_WLAN_MCC_TO_SCC_SWITCH
			if (pe_session->cc_switch_mode
			    != QDF_MCC_TO_SCC_SWITCH_DISABLE) {
				pSirSmeRsp->HTProfile.
				htSupportedChannelWidthSet =
					pe_session->htSupportedChannelWidthSet;
				pSirSmeRsp->HTProfile.htRecommendedTxWidthSet =
					pe_session->htRecommendedTxWidthSet;
				pSirSmeRsp->HTProfile.htSecondaryChannelOffset =
					pe_session->htSecondaryChannelOffset;
				pSirSmeRsp->HTProfile.dot11mode =
					pe_session->dot11mode;
				pSirSmeRsp->HTProfile.htCapability =
					pe_session->htCapability;
				pSirSmeRsp->HTProfile.vhtCapability =
					pe_session->vhtCapability;
				pSirSmeRsp->HTProfile.apCenterChan =
					pe_session->ch_center_freq_seg0;
				pSirSmeRsp->HTProfile.apChanWidth =
					pe_session->ch_width;
			}
#endif
		}
	}
	pSirSmeRsp->messageType = msgType;
	pSirSmeRsp->length = size;

	/* Update SME session Id and transaction Id */
	pSirSmeRsp->sessionId = smesessionId;
	pSirSmeRsp->transactionId = smetransactionId;
	pSirSmeRsp->statusCode = resultCode;
	if (pe_session != NULL)
		pSirSmeRsp->staId = pe_session->staId;       /* else it will be always zero smeRsp StaID = 0 */

	mmhMsg.type = msgType;
	mmhMsg.bodyptr = pSirSmeRsp;
	mmhMsg.bodyval = 0;
	if (pe_session == NULL) {
		MTRACE(mac_trace(mac, TRACE_CODE_TX_SME_MSG,
				 NO_SESSION, mmhMsg.type));
	} else {
		MTRACE(mac_trace(mac, TRACE_CODE_TX_SME_MSG,
				 pe_session->peSessionId, mmhMsg.type));
	}
#ifdef FEATURE_WLAN_DIAG_SUPPORT_LIM    /* FEATURE_WLAN_DIAG_SUPPORT */
	lim_diag_event_report(mac, WLAN_PE_DIAG_START_BSS_RSP_EVENT,
			      pe_session, (uint16_t) resultCode, 0);
#endif /* FEATURE_WLAN_DIAG_SUPPORT */

	lim_sys_process_mmh_msg_api(mac, &mmhMsg);
} /*** end lim_send_sme_start_bss_rsp() ***/

void lim_send_sme_disassoc_deauth_ntf(struct mac_context *mac,
				      QDF_STATUS status, uint32_t *pCtx)
{
	struct scheduler_msg mmhMsg = {0};
	struct scheduler_msg *pMsg = (struct scheduler_msg *) pCtx;

	mmhMsg.type = pMsg->type;
	mmhMsg.bodyptr = pMsg;
	mmhMsg.bodyval = 0;

	MTRACE(mac_trace(mac, TRACE_CODE_TX_SME_MSG, NO_SESSION, mmhMsg.type));

	lim_sys_process_mmh_msg_api(mac, &mmhMsg);
}

/**
 * lim_send_sme_disassoc_ntf()
 *
 ***FUNCTION:
 * This function is called by limProcessSmeMessages() to send
 * eWNI_SME_DISASSOC_RSP/IND message to host
 *
 ***PARAMS:
 *
 ***LOGIC:
 *
 ***ASSUMPTIONS:
 * NA
 *
 ***NOTE:
 * This function is used for sending eWNI_SME_DISASSOC_CNF,
 * or eWNI_SME_DISASSOC_IND to host depending on
 * disassociation trigger.
 *
 * @param peerMacAddr       Indicates the peer MAC addr to which
 *                          disassociate was initiated
 * @param reasonCode        Indicates the reason for Disassociation
 * @param disassocTrigger   Indicates the trigger for Disassociation
 * @param aid               Indicates the STAID. This parameter is
 *                          present only on AP.
 *
 * @return None
 */
void
lim_send_sme_disassoc_ntf(struct mac_context *mac,
			  tSirMacAddr peerMacAddr,
			  tSirResultCodes reasonCode,
			  uint16_t disassocTrigger,
			  uint16_t aid,
			  uint8_t smesessionId,
			  uint16_t smetransactionId, struct pe_session *pe_session)
{

	uint8_t *pBuf;
	tSirSmeDisassocRsp *pSirSmeDisassocRsp;
	tSirSmeDisassocInd *pSirSmeDisassocInd;
	uint32_t *pMsg = NULL;
	bool failure = false;
	struct pe_session *session = NULL;
	uint16_t i, assoc_id;
	tpDphHashNode sta_ds = NULL;
	QDF_STATUS status;

	pe_debug("Disassoc Ntf with trigger : %d reasonCode: %d",
		disassocTrigger, reasonCode);

	switch (disassocTrigger) {
	case eLIM_DUPLICATE_ENTRY:
		/*
		 * Duplicate entry is removed at LIM.
		 * Initiate new entry for other session
		 */
		pe_debug("Rcvd eLIM_DUPLICATE_ENTRY for " MAC_ADDRESS_STR,
			MAC_ADDR_ARRAY(peerMacAddr));

		for (i = 0; i < mac->lim.maxBssId; i++) {
			if ((&mac->lim.gpSession[i] != NULL) &&
					(mac->lim.gpSession[i].valid) &&
					(mac->lim.gpSession[i].pePersona ==
								QDF_SAP_MODE)) {
				/* Find the sta ds entry in another session */
				session = &mac->lim.gpSession[i];
				sta_ds = dph_lookup_hash_entry(mac,
						peerMacAddr, &assoc_id,
						&session->dph.dphHashTable);
				if (sta_ds)
					break;
			}
		}
		if (sta_ds
#ifdef WLAN_FEATURE_11W
			&& (!sta_ds->rmfEnabled)
#endif
		) {
			if (lim_add_sta(mac, sta_ds, false, session) !=
					QDF_STATUS_SUCCESS)
					pe_err("could not Add STA with assocId: %d",
					sta_ds->assocId);
		}
		status = lim_prepare_disconnect_done_ind(mac, &pMsg,
							 smesessionId,
							 reasonCode,
							 &peerMacAddr[0]);
		if (!QDF_IS_STATUS_SUCCESS(status)) {
			pe_err("Failed to prepare message");
			return;
		}
		break;

	case eLIM_HOST_DISASSOC:
		/**
		 * Disassociation response due to
		 * host triggered disassociation
		 */

		pSirSmeDisassocRsp = qdf_mem_malloc(sizeof(tSirSmeDisassocRsp));
		if (!pSirSmeDisassocRsp) {
			failure = true;
			goto error;
		}
		pe_debug("send eWNI_SME_DISASSOC_RSP with retCode: %d for " MAC_ADDRESS_STR,
			reasonCode, MAC_ADDR_ARRAY(peerMacAddr));
		pSirSmeDisassocRsp->messageType = eWNI_SME_DISASSOC_RSP;
		pSirSmeDisassocRsp->length = sizeof(tSirSmeDisassocRsp);
		/* sessionId */
		pBuf = (uint8_t *) &pSirSmeDisassocRsp->sessionId;
		*pBuf = smesessionId;
		pBuf++;

		/* transactionId */
		lim_copy_u16(pBuf, smetransactionId);
		pBuf += sizeof(uint16_t);

		/* statusCode */
		lim_copy_u32(pBuf, reasonCode);
		pBuf += sizeof(tSirResultCodes);

		/* peerMacAddr */
		qdf_mem_copy(pBuf, peerMacAddr, sizeof(tSirMacAddr));
		pBuf += sizeof(tSirMacAddr);

		/* Clear Station Stats */
		/* for sta, it is always 1, IBSS is handled at halInitSta */

#ifdef FEATURE_WLAN_DIAG_SUPPORT_LIM    /* FEATURE_WLAN_DIAG_SUPPORT */

		lim_diag_event_report(mac, WLAN_PE_DIAG_DISASSOC_RSP_EVENT,
				      pe_session, (uint16_t) reasonCode, 0);
#endif
		pMsg = (uint32_t *) pSirSmeDisassocRsp;
		break;

	case eLIM_PEER_ENTITY_DISASSOC:
	case eLIM_LINK_MONITORING_DISASSOC:
		status = lim_prepare_disconnect_done_ind(mac, &pMsg,
						smesessionId,
						reasonCode, &peerMacAddr[0]);
		if (!QDF_IS_STATUS_SUCCESS(status)) {
			pe_err("Failed to prepare message");
			return;
		}
		break;

	default:
		/**
		 * Disassociation indication due to Disassociation
		 * frame reception from peer entity or due to
		 * loss of link with peer entity.
		 */
		pSirSmeDisassocInd = qdf_mem_malloc(sizeof(tSirSmeDisassocInd));
		if (!pSirSmeDisassocInd) {
			failure = true;
			goto error;
		}
		pe_debug("send eWNI_SME_DISASSOC_IND with retCode: %d for " MAC_ADDRESS_STR,
			reasonCode, MAC_ADDR_ARRAY(peerMacAddr));
		pSirSmeDisassocInd->messageType = eWNI_SME_DISASSOC_IND;
		pSirSmeDisassocInd->length = sizeof(tSirSmeDisassocInd);

		/* Update SME session Id and Transaction Id */
		pSirSmeDisassocInd->sessionId = smesessionId;
		pSirSmeDisassocInd->transactionId = smetransactionId;
		pSirSmeDisassocInd->reasonCode = reasonCode;
		pBuf = (uint8_t *) &pSirSmeDisassocInd->statusCode;

		lim_copy_u32(pBuf, reasonCode);
		pBuf += sizeof(tSirResultCodes);

		qdf_mem_copy(pBuf, pe_session->bssId, sizeof(tSirMacAddr));
		pBuf += sizeof(tSirMacAddr);

		qdf_mem_copy(pBuf, peerMacAddr, sizeof(tSirMacAddr));

#ifdef FEATURE_WLAN_DIAG_SUPPORT_LIM    /* FEATURE_WLAN_DIAG_SUPPORT */
		lim_diag_event_report(mac, WLAN_PE_DIAG_DISASSOC_IND_EVENT,
				      pe_session, (uint16_t) reasonCode, 0);
#endif
		pMsg = (uint32_t *) pSirSmeDisassocInd;

		break;
	}

error:
	/* Delete the PE session Created */
	if ((pe_session != NULL) && LIM_IS_STA_ROLE(pe_session))
		pe_delete_session(mac, pe_session);

	if (false == failure)
		lim_send_sme_disassoc_deauth_ntf(mac, QDF_STATUS_SUCCESS,
						 (uint32_t *) pMsg);
} /*** end lim_send_sme_disassoc_ntf() ***/

/** -----------------------------------------------------------------
   \brief lim_send_sme_disassoc_ind() - sends SME_DISASSOC_IND

   After receiving disassociation frame from peer entity, this
   function sends a eWNI_SME_DISASSOC_IND to SME with a specific
   reason code.

   \param mac - global mac structure
   \param pStaDs - station dph hash node
   \return none
   \sa
   ----------------------------------------------------------------- */
void
lim_send_sme_disassoc_ind(struct mac_context *mac, tpDphHashNode pStaDs,
			  struct pe_session *pe_session)
{
	struct scheduler_msg mmhMsg = {0};
	tSirSmeDisassocInd *pSirSmeDisassocInd;

	pSirSmeDisassocInd = qdf_mem_malloc(sizeof(tSirSmeDisassocInd));
	if (!pSirSmeDisassocInd)
		return;

	pSirSmeDisassocInd->messageType = eWNI_SME_DISASSOC_IND;
	pSirSmeDisassocInd->length = sizeof(tSirSmeDisassocInd);

	pSirSmeDisassocInd->sessionId = pe_session->smeSessionId;
	pSirSmeDisassocInd->transactionId = pe_session->transactionId;
	pSirSmeDisassocInd->statusCode = eSIR_SME_DEAUTH_STATUS;
	pSirSmeDisassocInd->reasonCode = pStaDs->mlmStaContext.disassocReason;

	qdf_mem_copy(pSirSmeDisassocInd->bssid.bytes, pe_session->bssId,
		     QDF_MAC_ADDR_SIZE);

	qdf_mem_copy(pSirSmeDisassocInd->peer_macaddr.bytes, pStaDs->staAddr,
		     QDF_MAC_ADDR_SIZE);

	pSirSmeDisassocInd->staId = pStaDs->staIndex;

	mmhMsg.type = eWNI_SME_DISASSOC_IND;
	mmhMsg.bodyptr = pSirSmeDisassocInd;
	mmhMsg.bodyval = 0;

	MTRACE(mac_trace(mac, TRACE_CODE_TX_SME_MSG,
			 pe_session->peSessionId, mmhMsg.type));
#ifdef FEATURE_WLAN_DIAG_SUPPORT_LIM    /* FEATURE_WLAN_DIAG_SUPPORT */
	lim_diag_event_report(mac, WLAN_PE_DIAG_DISASSOC_IND_EVENT, pe_session,
			      0, (uint16_t) pStaDs->mlmStaContext.disassocReason);
#endif /* FEATURE_WLAN_DIAG_SUPPORT */

	lim_sys_process_mmh_msg_api(mac, &mmhMsg);

} /*** end lim_send_sme_disassoc_ind() ***/

/** -----------------------------------------------------------------
   \brief lim_send_sme_deauth_ind() - sends SME_DEAUTH_IND

   After receiving deauthentication frame from peer entity, this
   function sends a eWNI_SME_DEAUTH_IND to SME with a specific
   reason code.

   \param mac - global mac structure
   \param pStaDs - station dph hash node
   \return none
   \sa
   ----------------------------------------------------------------- */
void
lim_send_sme_deauth_ind(struct mac_context *mac, tpDphHashNode pStaDs,
			struct pe_session *pe_session)
{
	struct scheduler_msg mmhMsg = {0};
	tSirSmeDeauthInd *pSirSmeDeauthInd;

	pSirSmeDeauthInd = qdf_mem_malloc(sizeof(tSirSmeDeauthInd));
	if (!pSirSmeDeauthInd)
		return;

	pSirSmeDeauthInd->messageType = eWNI_SME_DEAUTH_IND;
	pSirSmeDeauthInd->length = sizeof(tSirSmeDeauthInd);

	pSirSmeDeauthInd->sessionId = pe_session->smeSessionId;
	pSirSmeDeauthInd->transactionId = pe_session->transactionId;
	if (eSIR_INFRA_AP_MODE == pe_session->bssType) {
		pSirSmeDeauthInd->statusCode =
			(tSirResultCodes) pStaDs->mlmStaContext.cleanupTrigger;
	} else {
		/* Need to indicatet he reascon code over the air */
		pSirSmeDeauthInd->statusCode =
			(tSirResultCodes) pStaDs->mlmStaContext.disassocReason;
	}
	/* BSSID */
	qdf_mem_copy(pSirSmeDeauthInd->bssid.bytes, pe_session->bssId,
		     QDF_MAC_ADDR_SIZE);
	/* peerMacAddr */
	qdf_mem_copy(pSirSmeDeauthInd->peer_macaddr.bytes, pStaDs->staAddr,
		     QDF_MAC_ADDR_SIZE);
	pSirSmeDeauthInd->reasonCode = pStaDs->mlmStaContext.disassocReason;

	pSirSmeDeauthInd->staId = pStaDs->staIndex;
	if (eSIR_MAC_PEER_STA_REQ_LEAVING_BSS_REASON ==
		pStaDs->mlmStaContext.disassocReason)
		pSirSmeDeauthInd->rssi = pStaDs->del_sta_ctx_rssi;

	mmhMsg.type = eWNI_SME_DEAUTH_IND;
	mmhMsg.bodyptr = pSirSmeDeauthInd;
	mmhMsg.bodyval = 0;

	MTRACE(mac_trace_msg_tx(mac, pe_session->peSessionId, mmhMsg.type));
#ifdef FEATURE_WLAN_DIAG_SUPPORT_LIM    /* FEATURE_WLAN_DIAG_SUPPORT */
	lim_diag_event_report(mac, WLAN_PE_DIAG_DEAUTH_IND_EVENT, pe_session,
			      0, pStaDs->mlmStaContext.cleanupTrigger);
#endif /* FEATURE_WLAN_DIAG_SUPPORT */

	lim_sys_process_mmh_msg_api(mac, &mmhMsg);
	return;
} /*** end lim_send_sme_deauth_ind() ***/

#ifdef FEATURE_WLAN_TDLS
/**
 * lim_send_sme_tdls_del_sta_ind()
 *
 ***FUNCTION:
 * This function is called to send the TDLS STA context deletion to SME.
 *
 ***LOGIC:
 *
 ***ASSUMPTIONS:
 *
 ***NOTE:
 * NA
 *
 * @param  mac   - Pointer to global MAC structure
 * @param  pStaDs - Pointer to internal STA Datastructure
 * @param  pe_session - Pointer to the session entry
 * @param  reasonCode - Reason for TDLS sta deletion
 * @return None
 */
void
lim_send_sme_tdls_del_sta_ind(struct mac_context *mac, tpDphHashNode pStaDs,
			      struct pe_session *pe_session, uint16_t reasonCode)
{
	struct tdls_event_info info;

	pe_debug("Delete TDLS Peer "MAC_ADDRESS_STR "with reason code: %d",
			MAC_ADDR_ARRAY(pStaDs->staAddr), reasonCode);
	info.vdev_id = pe_session->smeSessionId;
	qdf_mem_copy(info.peermac.bytes, pStaDs->staAddr, QDF_MAC_ADDR_SIZE);
	info.message_type = TDLS_PEER_DISCONNECTED;
	info.peer_reason = TDLS_DISCONNECTED_PEER_DELETE;

	tgt_tdls_event_handler(mac->psoc, &info);

	return;
} /*** end lim_send_sme_tdls_del_sta_ind() ***/

/**
 * lim_send_sme_mgmt_tx_completion()
 *
 ***FUNCTION:
 * This function is called to send the eWNI_SME_MGMT_FRM_TX_COMPLETION_IND
 * message to SME.
 *
 ***LOGIC:
 *
 ***ASSUMPTIONS:
 *
 ***NOTE:
 * NA
 *
 * @param  mac   - Pointer to global MAC structure
 * @param  pe_session - Pointer to the session entry
 * @param  txCompleteStatus - TX Complete Status of Mgmt Frames
 * @return None
 */
void
lim_send_sme_mgmt_tx_completion(struct mac_context *mac,
				uint32_t sme_session_id,
				uint32_t txCompleteStatus)
{
	struct scheduler_msg msg = {0};
	struct tdls_mgmt_tx_completion_ind *mgmt_tx_completion_ind;
	QDF_STATUS status;

	mgmt_tx_completion_ind =
		qdf_mem_malloc(sizeof(*mgmt_tx_completion_ind));
	if (!mgmt_tx_completion_ind)
		return;

	/* sessionId */
	mgmt_tx_completion_ind->session_id = sme_session_id;

	mgmt_tx_completion_ind->tx_complete_status = txCompleteStatus;

	msg.type = eWNI_SME_MGMT_FRM_TX_COMPLETION_IND;
	msg.bodyptr = mgmt_tx_completion_ind;
	msg.bodyval = 0;

	mgmt_tx_completion_ind->psoc = mac->psoc;
	msg.callback = tgt_tdls_send_mgmt_tx_completion;
	status = scheduler_post_message(QDF_MODULE_ID_PE,
			       QDF_MODULE_ID_TDLS,
			       QDF_MODULE_ID_TARGET_IF, &msg);
	if (QDF_IS_STATUS_ERROR(status)) {
		pe_err("post msg fail, %d", status);
		qdf_mem_free(mgmt_tx_completion_ind);
	}
} /*** end lim_send_sme_tdls_delete_all_peer_ind() ***/

#endif /* FEATURE_WLAN_TDLS */

QDF_STATUS lim_prepare_disconnect_done_ind(struct mac_context *mac_ctx,
					   uint32_t **msg,
					   uint8_t session_id,
					   tSirResultCodes reason_code,
					   uint8_t *peer_mac_addr)
{
	struct sir_sme_discon_done_ind *sir_sme_dis_ind;

	sir_sme_dis_ind = qdf_mem_malloc(sizeof(*sir_sme_dis_ind));
	if (!sir_sme_dis_ind)
		return QDF_STATUS_E_FAILURE;

	pe_debug("Prepare eWNI_SME_DISCONNECT_DONE_IND withretCode: %d",
		 reason_code);

	sir_sme_dis_ind->message_type = eWNI_SME_DISCONNECT_DONE_IND;
	sir_sme_dis_ind->length = sizeof(*sir_sme_dis_ind);
	sir_sme_dis_ind->session_id = session_id;
	if (peer_mac_addr)
		qdf_mem_copy(sir_sme_dis_ind->peer_mac,
			     peer_mac_addr, ETH_ALEN);

	/*
	 * Instead of sending deauth reason code as 505 which is
	 * internal value(eSIR_SME_LOST_LINK_WITH_PEER_RESULT_CODE)
	 * Send reason code as zero to Supplicant
	 */
	if (reason_code == eSIR_SME_LOST_LINK_WITH_PEER_RESULT_CODE)
		sir_sme_dis_ind->reason_code = 0;
	else
		sir_sme_dis_ind->reason_code = reason_code;

	*msg = (uint32_t *)sir_sme_dis_ind;

	return QDF_STATUS_SUCCESS;
}

/**
 * lim_send_sme_deauth_ntf()
 *
 ***FUNCTION:
 * This function is called by limProcessSmeMessages() to send
 * eWNI_SME_DISASSOC_RSP/IND message to host
 *
 ***PARAMS:
 *
 ***LOGIC:
 *
 ***ASSUMPTIONS:
 * NA
 *
 ***NOTE:
 * This function is used for sending eWNI_SME_DEAUTH_CNF or
 * eWNI_SME_DEAUTH_IND to host depending on deauthentication trigger.
 *
 * @param peerMacAddr       Indicates the peer MAC addr to which
 *                          deauthentication was initiated
 * @param reasonCode        Indicates the reason for Deauthetication
 * @param deauthTrigger     Indicates the trigger for Deauthetication
 * @param aid               Indicates the STAID. This parameter is present
 *                          only on AP.
 *
 * @return None
 */
void
lim_send_sme_deauth_ntf(struct mac_context *mac, tSirMacAddr peerMacAddr,
			tSirResultCodes reasonCode, uint16_t deauthTrigger,
			uint16_t aid, uint8_t smesessionId,
			uint16_t smetransactionId)
{
	uint8_t *pBuf;
	tSirSmeDeauthRsp *pSirSmeDeauthRsp;
	tSirSmeDeauthInd *pSirSmeDeauthInd;
	struct pe_session *pe_session;
	uint8_t sessionId;
	uint32_t *pMsg = NULL;
	QDF_STATUS status;

	pe_session = pe_find_session_by_bssid(mac, peerMacAddr, &sessionId);
	switch (deauthTrigger) {
	case eLIM_HOST_DEAUTH:
		/**
		 * Deauthentication response to host triggered
		 * deauthentication.
		 */
		pSirSmeDeauthRsp = qdf_mem_malloc(sizeof(tSirSmeDeauthRsp));
		if (!pSirSmeDeauthRsp)
			return;
		pe_debug("send eWNI_SME_DEAUTH_RSP with retCode: %d for" MAC_ADDRESS_STR,
			reasonCode, MAC_ADDR_ARRAY(peerMacAddr));
		pSirSmeDeauthRsp->messageType = eWNI_SME_DEAUTH_RSP;
		pSirSmeDeauthRsp->length = sizeof(tSirSmeDeauthRsp);
		pSirSmeDeauthRsp->statusCode = reasonCode;
		pSirSmeDeauthRsp->sessionId = smesessionId;
		pSirSmeDeauthRsp->transactionId = smetransactionId;

		pBuf = (uint8_t *) pSirSmeDeauthRsp->peer_macaddr.bytes;
		qdf_mem_copy(pBuf, peerMacAddr, sizeof(tSirMacAddr));

#ifdef FEATURE_WLAN_DIAG_SUPPORT_LIM    /* FEATURE_WLAN_DIAG_SUPPORT */
		lim_diag_event_report(mac, WLAN_PE_DIAG_DEAUTH_RSP_EVENT,
				      pe_session, 0, (uint16_t) reasonCode);
#endif
		pMsg = (uint32_t *) pSirSmeDeauthRsp;

		break;

	case eLIM_PEER_ENTITY_DEAUTH:
	case eLIM_LINK_MONITORING_DEAUTH:
		status = lim_prepare_disconnect_done_ind(mac, &pMsg,
						smesessionId, reasonCode,
						&peerMacAddr[0]);
		if (!QDF_IS_STATUS_SUCCESS(status)) {
			pe_err("Failed to prepare message");
			return;
		}
		break;
	default:
		/**
		 * Deauthentication indication due to Deauthentication
		 * frame reception from peer entity or due to
		 * loss of link with peer entity.
		 */
		pSirSmeDeauthInd = qdf_mem_malloc(sizeof(tSirSmeDeauthInd));
		if (!pSirSmeDeauthInd)
			return;
		pe_debug("send eWNI_SME_DEAUTH_IND with retCode: %d for " MAC_ADDRESS_STR,
			reasonCode, MAC_ADDR_ARRAY(peerMacAddr));
		pSirSmeDeauthInd->messageType = eWNI_SME_DEAUTH_IND;
		pSirSmeDeauthInd->length = sizeof(tSirSmeDeauthInd);
		pSirSmeDeauthInd->reasonCode = eSIR_MAC_UNSPEC_FAILURE_REASON;

		/* sessionId */
		pBuf = (uint8_t *) &pSirSmeDeauthInd->sessionId;
		*pBuf++ = smesessionId;

		/* transaction ID */
		lim_copy_u16(pBuf, smetransactionId);
		pBuf += sizeof(uint16_t);

		/* status code */
		lim_copy_u32(pBuf, reasonCode);
		pBuf += sizeof(tSirResultCodes);

		/* bssId */
		qdf_mem_copy(pBuf, pe_session->bssId, sizeof(tSirMacAddr));
		pBuf += sizeof(tSirMacAddr);

		/* peerMacAddr */
		qdf_mem_copy(pSirSmeDeauthInd->peer_macaddr.bytes, peerMacAddr,
			     QDF_MAC_ADDR_SIZE);

#ifdef FEATURE_WLAN_DIAG_SUPPORT_LIM    /* FEATURE_WLAN_DIAG_SUPPORT */
		lim_diag_event_report(mac, WLAN_PE_DIAG_DEAUTH_IND_EVENT,
				      pe_session, 0, (uint16_t) reasonCode);
#endif /* FEATURE_WLAN_DIAG_SUPPORT */
		pMsg = (uint32_t *) pSirSmeDeauthInd;

		break;
	}

	/*Delete the PE session  created */
	if (pe_session != NULL) {
		pe_delete_session(mac, pe_session);
	}

	lim_send_sme_disassoc_deauth_ntf(mac, QDF_STATUS_SUCCESS,
					 (uint32_t *) pMsg);

} /*** end lim_send_sme_deauth_ntf() ***/

/**
 * lim_send_sme_wm_status_change_ntf() - Send Notification
 * @mac_ctx:             Global MAC Context
 * @status_change_code:  Indicates the change in the wireless medium.
 * @status_change_info:  Indicates the information associated with
 *                       change in the wireless medium.
 * @info_len:            Indicates the length of status change information
 *                       being sent.
 * @session_id           SessionID
 *
 * This function is called by limProcessSmeMessages() to send
 * eWNI_SME_WM_STATUS_CHANGE_NTF message to host.
 *
 * Return: None
 */
void
lim_send_sme_wm_status_change_ntf(struct mac_context *mac_ctx,
	tSirSmeStatusChangeCode status_change_code,
	uint32_t *status_change_info, uint16_t info_len, uint8_t session_id)
{
	struct scheduler_msg msg = {0};
	tSirSmeWmStatusChangeNtf *wm_status_change_ntf;
	uint32_t max_info_len;

	wm_status_change_ntf = qdf_mem_malloc(sizeof(tSirSmeWmStatusChangeNtf));
	if (!wm_status_change_ntf)
		return;

	msg.type = eWNI_SME_WM_STATUS_CHANGE_NTF;
	msg.bodyval = 0;
	msg.bodyptr = wm_status_change_ntf;

	switch (status_change_code) {
	case eSIR_SME_AP_CAPS_CHANGED:
		max_info_len = sizeof(tSirSmeApNewCaps);
		break;
	case eSIR_SME_JOINED_NEW_BSS:
		max_info_len = sizeof(tSirSmeNewBssInfo);
		break;
	default:
		max_info_len = sizeof(wm_status_change_ntf->statusChangeInfo);
		break;
	}

	switch (status_change_code) {
	case eSIR_SME_RADAR_DETECTED:
		break;
	default:
		wm_status_change_ntf->messageType =
			eWNI_SME_WM_STATUS_CHANGE_NTF;
		wm_status_change_ntf->statusChangeCode = status_change_code;
		wm_status_change_ntf->length = sizeof(tSirSmeWmStatusChangeNtf);
		wm_status_change_ntf->sessionId = session_id;
		if (info_len <= max_info_len && status_change_info) {
			qdf_mem_copy(
			    (uint8_t *) &wm_status_change_ntf->statusChangeInfo,
			    (uint8_t *) status_change_info, info_len);
		}
		pe_debug("StatusChg code: 0x%x length: %d",
			status_change_code, info_len);
		break;
	}

	MTRACE(mac_trace(mac_ctx, TRACE_CODE_TX_SME_MSG, session_id, msg.type));
	if (QDF_STATUS_SUCCESS != lim_sys_process_mmh_msg_api(mac_ctx, &msg)) {
		qdf_mem_free(wm_status_change_ntf);
		pe_err("lim_sys_process_mmh_msg_api failed");
	}

} /*** end lim_send_sme_wm_status_change_ntf() ***/

/**
 * lim_send_sme_set_context_rsp()
 *
 ***FUNCTION:
 * This function is called by limProcessSmeMessages() to send
 * eWNI_SME_SETCONTEXT_RSP message to host
 *
 ***PARAMS:
 *
 ***LOGIC:
 *
 ***ASSUMPTIONS:
 * NA
 *
 ***NOTE:
 *
 * @param mac         Pointer to Global MAC structure
 * @param peerMacAddr  Indicates the peer MAC addr to which
 *                     setContext was performed
 * @param aid          Indicates the aid corresponding to the peer MAC
 *                     address
 * @param resultCode   Indicates the result of previously issued
 *                     eWNI_SME_SETCONTEXT_RSP message
 *
 * @return None
 */
void
lim_send_sme_set_context_rsp(struct mac_context *mac,
			     struct qdf_mac_addr peer_macaddr, uint16_t aid,
			     tSirResultCodes resultCode,
			     struct pe_session *pe_session, uint8_t smesessionId,
			     uint16_t smetransactionId)
{
	struct scheduler_msg mmhMsg = {0};
	tSirSmeSetContextRsp *pSirSmeSetContextRsp;

	pSirSmeSetContextRsp = qdf_mem_malloc(sizeof(tSirSmeSetContextRsp));
	if (!pSirSmeSetContextRsp)
		return;

	pSirSmeSetContextRsp->messageType = eWNI_SME_SETCONTEXT_RSP;
	pSirSmeSetContextRsp->length = sizeof(tSirSmeSetContextRsp);
	pSirSmeSetContextRsp->statusCode = resultCode;

	qdf_copy_macaddr(&pSirSmeSetContextRsp->peer_macaddr, &peer_macaddr);

	/* Update SME session and transaction Id */
	pSirSmeSetContextRsp->sessionId = smesessionId;
	pSirSmeSetContextRsp->transactionId = smetransactionId;

	mmhMsg.type = eWNI_SME_SETCONTEXT_RSP;
	mmhMsg.bodyptr = pSirSmeSetContextRsp;
	mmhMsg.bodyval = 0;
	if (NULL == pe_session) {
		MTRACE(mac_trace(mac, TRACE_CODE_TX_SME_MSG,
				 NO_SESSION, mmhMsg.type));
	} else {
		MTRACE(mac_trace(mac, TRACE_CODE_TX_SME_MSG,
				 pe_session->peSessionId, mmhMsg.type));
	}

#ifdef FEATURE_WLAN_DIAG_SUPPORT_LIM    /* FEATURE_WLAN_DIAG_SUPPORT */
	lim_diag_event_report(mac, WLAN_PE_DIAG_SETCONTEXT_RSP_EVENT,
			      pe_session, (uint16_t) resultCode, 0);
#endif /* FEATURE_WLAN_DIAG_SUPPORT */

	mac->lim.sme_msg_callback(mac, &mmhMsg);
} /*** end lim_send_sme_set_context_rsp() ***/

/** -----------------------------------------------------------------
   \brief lim_send_sme_addts_rsp() - sends SME ADDTS RSP
 \      This function sends a eWNI_SME_ADDTS_RSP to SME.
 \      SME only looks at rc and tspec field.
   \param mac - global mac structure
   \param rspReqd - is SmeAddTsRsp required
   \param status - status code of SME_ADD_TS_RSP
   \return tspec
   \sa
   ----------------------------------------------------------------- */
void
lim_send_sme_addts_rsp(struct mac_context *mac, uint8_t rspReqd,
		       uint32_t status,
		       struct pe_session *pe_session, struct mac_tspec_ie tspec,
		       uint8_t smesessionId, uint16_t smetransactionId)
{
	tpSirAddtsRsp rsp;
	struct scheduler_msg mmhMsg = {0};

	if (!rspReqd)
		return;

	rsp = qdf_mem_malloc(sizeof(tSirAddtsRsp));
	if (!rsp)
		return;

	rsp->messageType = eWNI_SME_ADDTS_RSP;
	rsp->rc = status;
	rsp->rsp.status = (enum eSirMacStatusCodes)status;
	rsp->rsp.tspec = tspec;
	/* Update SME session Id and transcation Id */
	rsp->sessionId = smesessionId;
	rsp->transactionId = smetransactionId;

	mmhMsg.type = eWNI_SME_ADDTS_RSP;
	mmhMsg.bodyptr = rsp;
	mmhMsg.bodyval = 0;
	if (NULL == pe_session) {
		MTRACE(mac_trace(mac, TRACE_CODE_TX_SME_MSG,
				 NO_SESSION, mmhMsg.type));
	} else {
		MTRACE(mac_trace(mac, TRACE_CODE_TX_SME_MSG,
				 pe_session->peSessionId, mmhMsg.type));
	}
#ifdef FEATURE_WLAN_DIAG_SUPPORT_LIM    /* FEATURE_WLAN_DIAG_SUPPORT */
	lim_diag_event_report(mac, WLAN_PE_DIAG_ADDTS_RSP_EVENT, pe_session, 0,
			      0);
#endif /* FEATURE_WLAN_DIAG_SUPPORT */

	lim_sys_process_mmh_msg_api(mac, &mmhMsg);
	return;
}

void
lim_send_sme_delts_rsp(struct mac_context *mac, tpSirDeltsReq delts, uint32_t status,
		       struct pe_session *pe_session, uint8_t smesessionId,
		       uint16_t smetransactionId)
{
	tpSirDeltsRsp rsp;
	struct scheduler_msg mmhMsg = {0};

	pe_debug("SendSmeDeltsRsp aid: %d tsid: %d up: %d status: %d",
		delts->aid,
		delts->req.tsinfo.traffic.tsid,
		delts->req.tsinfo.traffic.userPrio, status);
	if (!delts->rspReqd)
		return;

	rsp = qdf_mem_malloc(sizeof(tSirDeltsRsp));
	if (!rsp)
		return;

	if (pe_session != NULL) {

		rsp->aid = delts->aid;
		qdf_copy_macaddr(&rsp->macaddr, &delts->macaddr);
		qdf_mem_copy((uint8_t *) &rsp->rsp, (uint8_t *) &delts->req,
			     sizeof(struct delts_req_info));
	}

	rsp->messageType = eWNI_SME_DELTS_RSP;
	rsp->rc = status;

	/* Update SME session Id and transcation Id */
	rsp->sessionId = smesessionId;
	rsp->transactionId = smetransactionId;

	mmhMsg.type = eWNI_SME_DELTS_RSP;
	mmhMsg.bodyptr = rsp;
	mmhMsg.bodyval = 0;
	if (NULL == pe_session) {
		MTRACE(mac_trace(mac, TRACE_CODE_TX_SME_MSG,
				 NO_SESSION, mmhMsg.type));
	} else {
		MTRACE(mac_trace(mac, TRACE_CODE_TX_SME_MSG,
				 pe_session->peSessionId, mmhMsg.type));
	}
#ifdef FEATURE_WLAN_DIAG_SUPPORT_LIM    /* FEATURE_WLAN_DIAG_SUPPORT */
	lim_diag_event_report(mac, WLAN_PE_DIAG_DELTS_RSP_EVENT, pe_session,
			      (uint16_t) status, 0);
#endif /* FEATURE_WLAN_DIAG_SUPPORT */

	lim_sys_process_mmh_msg_api(mac, &mmhMsg);
}

void
lim_send_sme_delts_ind(struct mac_context *mac, struct delts_req_info *delts,
		       uint16_t aid, struct pe_session *pe_session)
{
	tpSirDeltsRsp rsp;
	struct scheduler_msg mmhMsg = {0};

	pe_debug("SendSmeDeltsInd aid: %d tsid: %d up: %d",
		aid, delts->tsinfo.traffic.tsid, delts->tsinfo.traffic.userPrio);

	rsp = qdf_mem_malloc(sizeof(tSirDeltsRsp));
	if (!rsp)
		return;

	rsp->messageType = eWNI_SME_DELTS_IND;
	rsp->rc = QDF_STATUS_SUCCESS;
	rsp->aid = aid;
	qdf_mem_copy((uint8_t *) &rsp->rsp, (uint8_t *) delts, sizeof(*delts));

	/* Update SME  session Id and SME transaction Id */

	rsp->sessionId = pe_session->smeSessionId;
	rsp->transactionId = pe_session->transactionId;

	mmhMsg.type = eWNI_SME_DELTS_IND;
	mmhMsg.bodyptr = rsp;
	mmhMsg.bodyval = 0;
	MTRACE(mac_trace_msg_tx(mac, pe_session->peSessionId, mmhMsg.type));
#ifdef FEATURE_WLAN_DIAG_SUPPORT_LIM    /* FEATURE_WLAN_DIAG_SUPPORT */
	lim_diag_event_report(mac, WLAN_PE_DIAG_DELTS_IND_EVENT, pe_session, 0,
			      0);
#endif /* FEATURE_WLAN_DIAG_SUPPORT */

	lim_sys_process_mmh_msg_api(mac, &mmhMsg);
}

#ifndef QCA_SUPPORT_CP_STATS
/**
 * lim_send_sme_pe_statistics_rsp()
 *
 ***FUNCTION:
 * This function is called to send 802.11 statistics response to HDD.
 * This function posts the result back to HDD. This is a response to
 * HDD's request for statistics.
 *
 ***PARAMS:
 *
 ***LOGIC:
 *
 ***ASSUMPTIONS:
 * NA
 *
 ***NOTE:
 * NA
 *
 * @param mac         Pointer to Global MAC structure
 * @param p80211Stats  Statistics sent in response
 * @param resultCode   TODO:
 *
 *
 * @return none
 */

void
lim_send_sme_pe_statistics_rsp(struct mac_context *mac, uint16_t msgType, void *stats)
{
	struct scheduler_msg mmhMsg = {0};
	uint8_t sessionId;
	tAniGetPEStatsRsp *pPeStats = (tAniGetPEStatsRsp *) stats;
	struct pe_session *pPeSessionEntry;

	/* Get the Session Id based on Sta Id */
	pPeSessionEntry =
		pe_find_session_by_sta_id(mac, pPeStats->staId, &sessionId);

	/* Fill the Session Id */
	if (NULL != pPeSessionEntry) {
		/* Fill the Session Id */
		pPeStats->sessionId = pPeSessionEntry->smeSessionId;
	}

	pPeStats->msgType = eWNI_SME_GET_STATISTICS_RSP;

	/* msgType should be WMA_GET_STATISTICS_RSP */
	mmhMsg.type = eWNI_SME_GET_STATISTICS_RSP;

	mmhMsg.bodyptr = stats;
	mmhMsg.bodyval = 0;
	MTRACE(mac_trace(mac, TRACE_CODE_TX_SME_MSG, NO_SESSION, mmhMsg.type));
	lim_sys_process_mmh_msg_api(mac, &mmhMsg);

	return;

} /*** end lim_send_sme_pe_statistics_rsp() ***/
#endif

#ifdef FEATURE_WLAN_ESE
/**
 * lim_send_sme_pe_ese_tsm_rsp() - send tsm response
 * @mac:   Pointer to global mac structure
 * @pStats: Pointer to TSM Stats
 *
 * This function is called to send tsm stats response to HDD.
 * This function posts the result back to HDD. This is a response to
 * HDD's request to get tsm stats.
 *
 * Return: None
 */
void lim_send_sme_pe_ese_tsm_rsp(struct mac_context *mac,
				 tAniGetTsmStatsRsp *pStats)
{
	struct scheduler_msg mmhMsg = {0};
	uint8_t sessionId;
	tAniGetTsmStatsRsp *pPeStats = (tAniGetTsmStatsRsp *) pStats;
	struct pe_session *pPeSessionEntry = NULL;

	/* Get the Session Id based on Sta Id */
	pPeSessionEntry =
		pe_find_session_by_sta_id(mac, pPeStats->staId, &sessionId);

	/* Fill the Session Id */
	if (NULL != pPeSessionEntry) {
		/* Fill the Session Id */
		pPeStats->sessionId = pPeSessionEntry->smeSessionId;
	} else {
		pe_err("Session not found for the Sta id: %d",
		       pPeStats->staId);
		qdf_mem_free(pPeStats->tsmStatsReq);
		qdf_mem_free(pPeStats);
		return;
	}

	pPeStats->msgType = eWNI_SME_GET_TSM_STATS_RSP;
	pPeStats->tsmMetrics.RoamingCount
		= pPeSessionEntry->eseContext.tsm.tsmMetrics.RoamingCount;
	pPeStats->tsmMetrics.RoamingDly
		= pPeSessionEntry->eseContext.tsm.tsmMetrics.RoamingDly;

	mmhMsg.type = eWNI_SME_GET_TSM_STATS_RSP;
	mmhMsg.bodyptr = pStats;
	mmhMsg.bodyval = 0;
	MTRACE(mac_trace(mac, TRACE_CODE_TX_SME_MSG, sessionId, mmhMsg.type));
	lim_sys_process_mmh_msg_api(mac, &mmhMsg);

	return;
} /*** end lim_send_sme_pe_ese_tsm_rsp() ***/

#endif /* FEATURE_WLAN_ESE */

void
lim_send_sme_ibss_peer_ind(struct mac_context *mac,
			   tSirMacAddr peerMacAddr,
			   uint16_t staIndex,
			   uint8_t *beacon,
			   uint16_t beaconLen, uint16_t msgType, uint8_t sessionId)
{
	struct scheduler_msg mmhMsg = {0};
	tSmeIbssPeerInd *pNewPeerInd;

	pNewPeerInd = qdf_mem_malloc(sizeof(tSmeIbssPeerInd) + beaconLen);
	if (!pNewPeerInd)
		return;

	qdf_mem_copy((uint8_t *) pNewPeerInd->peer_addr.bytes,
		     peerMacAddr, QDF_MAC_ADDR_SIZE);
	pNewPeerInd->staId = staIndex;
	pNewPeerInd->mesgLen = sizeof(tSmeIbssPeerInd) + beaconLen;
	pNewPeerInd->mesgType = msgType;
	pNewPeerInd->sessionId = sessionId;

	if (beacon != NULL) {
		qdf_mem_copy((void *)((uint8_t *) pNewPeerInd +
				      sizeof(tSmeIbssPeerInd)), (void *)beacon,
			     beaconLen);
	}

	mmhMsg.type = msgType;
	mmhMsg.bodyptr = pNewPeerInd;
	MTRACE(mac_trace(mac, TRACE_CODE_TX_SME_MSG, sessionId, mmhMsg.type));
	lim_sys_process_mmh_msg_api(mac, &mmhMsg);

}

/**
 * lim_process_csa_wbw_ie() - Process CSA Wide BW IE
 * @mac_ctx:         pointer to global adapter context
 * @csa_params:      pointer to CSA parameters
 * @chnl_switch_info:pointer to channel switch parameters
 * @session_entry:   session pointer
 *
 * Return: None
 */
static QDF_STATUS lim_process_csa_wbw_ie(struct mac_context *mac_ctx,
		struct csa_offload_params *csa_params,
		tLimWiderBWChannelSwitchInfo *chnl_switch_info,
		struct pe_session *session_entry)
{
	struct ch_params ch_params = {0};
	uint8_t ap_new_ch_width;
	bool new_ch_width_dfn = false;
	uint8_t center_freq_diff;
	uint32_t fw_vht_ch_wd = wma_get_vht_ch_width() + 1;

	ap_new_ch_width = csa_params->new_ch_width + 1;

	pe_debug("new channel: %d new_ch_width: %d seg0: %d seg1: %d",
		 csa_params->channel, ap_new_ch_width,
		 csa_params->new_ch_freq_seg1,
		 csa_params->new_ch_freq_seg2);

	if ((ap_new_ch_width != CH_WIDTH_80MHZ) &&
			(ap_new_ch_width != CH_WIDTH_160MHZ) &&
			(ap_new_ch_width != CH_WIDTH_80P80MHZ)) {
		pe_err("CSA wide BW IE has wrong ch_width %d",
				csa_params->new_ch_width);
		return QDF_STATUS_E_INVAL;
	}

	if (!csa_params->new_ch_freq_seg1 && !csa_params->new_ch_freq_seg2) {
		pe_err("CSA wide BW IE has invalid center freq");
		return QDF_STATUS_E_INVAL;
	}
	if ((ap_new_ch_width == CH_WIDTH_80MHZ) &&
			csa_params->new_ch_freq_seg2) {
		new_ch_width_dfn = true;
		if (csa_params->new_ch_freq_seg2 >
				csa_params->new_ch_freq_seg1)
			center_freq_diff = csa_params->new_ch_freq_seg2 -
				csa_params->new_ch_freq_seg1;
		else
			center_freq_diff = csa_params->new_ch_freq_seg1 -
				csa_params->new_ch_freq_seg2;
		if (center_freq_diff == CENTER_FREQ_DIFF_160MHz)
			ap_new_ch_width = CH_WIDTH_160MHZ;
		else if (center_freq_diff > CENTER_FREQ_DIFF_80P80MHz)
			ap_new_ch_width = CH_WIDTH_80P80MHZ;
		else
			ap_new_ch_width = CH_WIDTH_80MHZ;
	}
	session_entry->gLimChannelSwitch.state =
		eLIM_CHANNEL_SWITCH_PRIMARY_AND_SECONDARY;
	if ((ap_new_ch_width == CH_WIDTH_160MHZ) &&
			!new_ch_width_dfn) {
		if (csa_params->new_ch_freq_seg1 != csa_params->channel +
				CH_TO_CNTR_FREQ_DIFF_160MHz) {
			pe_err("CSA wide BW IE has invalid center freq");
			return QDF_STATUS_E_INVAL;
		}

		if (ap_new_ch_width > fw_vht_ch_wd) {
			pe_debug("New BW is not supported, setting BW to %d",
				 fw_vht_ch_wd);
			ap_new_ch_width = fw_vht_ch_wd;
		}
		ch_params.ch_width = ap_new_ch_width ;
		wlan_reg_set_channel_params(mac_ctx->pdev,
					    csa_params->channel, 0, &ch_params);
		ap_new_ch_width = ch_params.ch_width;
		csa_params->new_ch_freq_seg1 = ch_params.center_freq_seg0;
		csa_params->new_ch_freq_seg2 = ch_params.center_freq_seg1;
	} else if (!new_ch_width_dfn) {
		if (ap_new_ch_width > fw_vht_ch_wd) {
			pe_debug("New BW is not supported, setting BW to %d",
				 fw_vht_ch_wd);
			ap_new_ch_width = fw_vht_ch_wd;
		}
		if (csa_params->new_ch_freq_seg1 != csa_params->channel +
				CH_TO_CNTR_FREQ_DIFF_80MHz) {
			pe_err("CSA wide BW IE has invalid center freq");
			return QDF_STATUS_E_INVAL;
		}
		csa_params->new_ch_freq_seg2 = 0;
	}
	if (new_ch_width_dfn) {
		if (csa_params->new_ch_freq_seg1 != csa_params->channel +
				CH_TO_CNTR_FREQ_DIFF_80MHz) {
			pe_err("CSA wide BW IE has invalid center freq");
			return QDF_STATUS_E_INVAL;
		}
		if (ap_new_ch_width > fw_vht_ch_wd) {
			pe_debug("New width is not supported, setting BW to %d",
				 fw_vht_ch_wd);
			ap_new_ch_width = fw_vht_ch_wd;
		}
		if ((ap_new_ch_width == CH_WIDTH_160MHZ) &&
				(csa_params->new_ch_freq_seg1 !=
				 csa_params->channel +
				 CH_TO_CNTR_FREQ_DIFF_160MHz)) {
			pe_err("wide BW IE has invalid 160M center freq");
			csa_params->new_ch_freq_seg2 = 0;
			ap_new_ch_width = CH_WIDTH_80MHZ;
		}
	}
	chnl_switch_info->newChanWidth = ap_new_ch_width;
	chnl_switch_info->newCenterChanFreq0 = csa_params->new_ch_freq_seg1;
	chnl_switch_info->newCenterChanFreq1 = csa_params->new_ch_freq_seg2;

	if (session_entry->ch_width == ap_new_ch_width)
		goto prnt_log;

	if (session_entry->ch_width == CH_WIDTH_80MHZ) {
		chnl_switch_info->newChanWidth = CH_WIDTH_80MHZ;
		chnl_switch_info->newCenterChanFreq1 = 0;
	} else {
		session_entry->ch_width = ap_new_ch_width;
		chnl_switch_info->newChanWidth = ap_new_ch_width;
	}
prnt_log:
	pe_debug("new channel: %d new_ch_width: %d seg0: %d seg1: %d",
			csa_params->channel,
			chnl_switch_info->newChanWidth,
			chnl_switch_info->newCenterChanFreq0,
			chnl_switch_info->newCenterChanFreq1);

	return QDF_STATUS_SUCCESS;
}

/**
 * lim_handle_csa_offload_msg() - Handle CSA offload message
 * @mac_ctx:         pointer to global adapter context
 * @msg:             Message pointer.
 *
 * Return: None
 */
void lim_handle_csa_offload_msg(struct mac_context *mac_ctx,
				struct scheduler_msg *msg)
{
	struct pe_session *session_entry;
	struct scheduler_msg mmh_msg = {0};
	struct csa_offload_params *csa_params =
				(struct csa_offload_params *) (msg->bodyptr);
	tpSmeCsaOffloadInd csa_offload_ind;
	tpDphHashNode sta_ds = NULL;
	uint8_t session_id;
	uint16_t aid = 0;
	uint16_t chan_space = 0;
	struct ch_params ch_params = {0};

	tLimWiderBWChannelSwitchInfo *chnl_switch_info = NULL;
	tLimChannelSwitchInfo *lim_ch_switch = NULL;

	pe_debug("handle csa offload msg");

	if (!csa_params) {
		pe_err("limMsgQ body ptr is NULL");
		return;
	}

	csa_offload_ind = qdf_mem_malloc(sizeof(tSmeCsaOffloadInd));
	if (!csa_offload_ind)
		goto err;

	session_entry =
		pe_find_session_by_bssid(mac_ctx,
			csa_params->bssId, &session_id);
	if (!session_entry) {
		pe_err("Session does not exists for %pM",
				csa_params->bssId);
		qdf_mem_free(csa_offload_ind);
		goto err;
	}

	sta_ds = dph_lookup_hash_entry(mac_ctx, session_entry->bssId, &aid,
		&session_entry->dph.dphHashTable);

	if (!sta_ds) {
		pe_err("sta_ds does not exist");
		qdf_mem_free(csa_offload_ind);
		goto err;
	}

	if (!LIM_IS_STA_ROLE(session_entry)) {
		pe_debug("Invalid role to handle CSA");
		qdf_mem_free(csa_offload_ind);
		goto err;
	}

	/*
	 * on receiving channel switch announcement from AP, delete all
	 * TDLS peers before leaving BSS and proceed for channel switch
	 */
	lim_delete_tdls_peers(mac_ctx, session_entry);

	lim_ch_switch = &session_entry->gLimChannelSwitch;
	session_entry->gLimChannelSwitch.switchMode =
		csa_params->switch_mode;
	/* timer already started by firmware, switch immediately */
	session_entry->gLimChannelSwitch.switchCount = 0;
	session_entry->gLimChannelSwitch.primaryChannel =
		csa_params->channel;
	session_entry->gLimChannelSwitch.state =
		eLIM_CHANNEL_SWITCH_PRIMARY_ONLY;
	session_entry->gLimChannelSwitch.ch_width = CH_WIDTH_20MHZ;
	lim_ch_switch->sec_ch_offset =
		session_entry->htSecondaryChannelOffset;
	session_entry->gLimChannelSwitch.ch_center_freq_seg0 = 0;
	session_entry->gLimChannelSwitch.ch_center_freq_seg1 = 0;
	chnl_switch_info =
		&session_entry->gLimWiderBWChannelSwitch;

	pe_debug("vht: %d ht: %d flag: %x chan: %d, sec_ch_offset %d",
		 session_entry->vhtCapability,
		 session_entry->htSupportedChannelWidthSet,
		 csa_params->ies_present_flag,
		 csa_params->channel,
		 csa_params->sec_chan_offset);
	pe_debug("seg1: %d seg2: %d width: %d country: %s class: %d",
		 csa_params->new_ch_freq_seg1,
		 csa_params->new_ch_freq_seg2,
		 csa_params->new_ch_width,
		 mac_ctx->scan.countryCodeCurrent,
		 csa_params->new_op_class);

	if (session_entry->vhtCapability &&
			session_entry->htSupportedChannelWidthSet) {
		if ((csa_params->ies_present_flag & lim_wbw_ie_present) &&
			(QDF_STATUS_SUCCESS == lim_process_csa_wbw_ie(mac_ctx,
					csa_params, chnl_switch_info,
					session_entry))) {
			pe_debug("CSA wide BW IE process successful");
			lim_ch_switch->sec_ch_offset =
				PHY_SINGLE_CHANNEL_CENTERED;
			if (chnl_switch_info->newChanWidth) {
				if (csa_params->channel <
				  csa_params->new_ch_freq_seg1)
					lim_ch_switch->sec_ch_offset =
						PHY_DOUBLE_CHANNEL_LOW_PRIMARY;
				else
					lim_ch_switch->sec_ch_offset =
						PHY_DOUBLE_CHANNEL_HIGH_PRIMARY;
			}
		} else if (csa_params->ies_present_flag
				& lim_xcsa_ie_present) {
			chan_space =
				wlan_reg_dmn_get_chanwidth_from_opclass(
						mac_ctx->scan.countryCodeCurrent,
						csa_params->channel,
						csa_params->new_op_class);
			session_entry->gLimChannelSwitch.state =
				eLIM_CHANNEL_SWITCH_PRIMARY_AND_SECONDARY;

			if (chan_space == 80) {
				chnl_switch_info->newChanWidth =
					CH_WIDTH_80MHZ;
			} else if (chan_space == 40) {
				chnl_switch_info->newChanWidth =
					CH_WIDTH_40MHZ;
			} else {
				chnl_switch_info->newChanWidth =
					CH_WIDTH_20MHZ;
				lim_ch_switch->state =
					eLIM_CHANNEL_SWITCH_PRIMARY_ONLY;
			}

			ch_params.ch_width =
				chnl_switch_info->newChanWidth;
			wlan_reg_set_channel_params(mac_ctx->pdev,
					csa_params->channel, 0, &ch_params);
			chnl_switch_info->newCenterChanFreq0 =
				ch_params.center_freq_seg0;
			/*
			 * This is not applicable for 20/40/80 MHz.
			 * Only used when we support 80+80 MHz operation.
			 * In case of 80+80 MHz, this parameter indicates
			 * center channel frequency index of 80 MHz
			 * channel offrequency segment 1.
			 */
			chnl_switch_info->newCenterChanFreq1 =
				ch_params.center_freq_seg1;
			lim_ch_switch->sec_ch_offset =
				ch_params.sec_ch_offset;

		} else {
			lim_ch_switch->state =
				eLIM_CHANNEL_SWITCH_PRIMARY_AND_SECONDARY;
			ch_params.ch_width = CH_WIDTH_40MHZ;
			wlan_reg_set_channel_params(mac_ctx->pdev,
					csa_params->channel, 0, &ch_params);
			lim_ch_switch->sec_ch_offset =
				ch_params.sec_ch_offset;
			chnl_switch_info->newChanWidth = CH_WIDTH_40MHZ;
			chnl_switch_info->newCenterChanFreq0 =
				ch_params.center_freq_seg0;
			chnl_switch_info->newCenterChanFreq1 = 0;
		}
		session_entry->gLimChannelSwitch.ch_center_freq_seg0 =
			chnl_switch_info->newCenterChanFreq0;
		session_entry->gLimChannelSwitch.ch_center_freq_seg1 =
			chnl_switch_info->newCenterChanFreq1;
		session_entry->gLimChannelSwitch.ch_width =
			chnl_switch_info->newChanWidth;

	} else if (session_entry->htSupportedChannelWidthSet) {
		if (csa_params->ies_present_flag
				& lim_xcsa_ie_present) {
			chan_space =
				wlan_reg_dmn_get_chanwidth_from_opclass(
						mac_ctx->scan.countryCodeCurrent,
						csa_params->channel,
						csa_params->new_op_class);
			lim_ch_switch->state =
				eLIM_CHANNEL_SWITCH_PRIMARY_AND_SECONDARY;
			if (chan_space == 40) {
				lim_ch_switch->ch_width =
					CH_WIDTH_40MHZ;
				chnl_switch_info->newChanWidth =
					CH_WIDTH_40MHZ;
				ch_params.ch_width =
					chnl_switch_info->newChanWidth;
				wlan_reg_set_channel_params(mac_ctx->pdev,
						csa_params->channel,
						0, &ch_params);
				lim_ch_switch->ch_center_freq_seg0 =
					ch_params.center_freq_seg0;
				lim_ch_switch->sec_ch_offset =
					ch_params.sec_ch_offset;
			} else {
				lim_ch_switch->ch_width =
					CH_WIDTH_20MHZ;
				chnl_switch_info->newChanWidth =
					CH_WIDTH_40MHZ;
				lim_ch_switch->state =
					eLIM_CHANNEL_SWITCH_PRIMARY_ONLY;
				lim_ch_switch->sec_ch_offset =
					PHY_SINGLE_CHANNEL_CENTERED;
			}
		} else {
			lim_ch_switch->ch_width =
				CH_WIDTH_40MHZ;
			lim_ch_switch->state =
				eLIM_CHANNEL_SWITCH_PRIMARY_AND_SECONDARY;
			ch_params.ch_width = CH_WIDTH_40MHZ;
			wlan_reg_set_channel_params(mac_ctx->pdev,
					csa_params->channel, 0, &ch_params);
			lim_ch_switch->ch_center_freq_seg0 =
				ch_params.center_freq_seg0;
			lim_ch_switch->sec_ch_offset =
				ch_params.sec_ch_offset;
		}

	}
	pe_debug("new ch width: %d space: %d",
			session_entry->gLimChannelSwitch.ch_width, chan_space);
	if ((session_entry->currentOperChannel == csa_params->channel) &&
		(session_entry->ch_width ==
		 session_entry->gLimChannelSwitch.ch_width)) {
		pe_debug("Ignore CSA, no change in ch and bw");
		qdf_mem_free(csa_offload_ind);
		goto err;
	}

	if (WLAN_REG_IS_24GHZ_CH(csa_params->channel) &&
	    (session_entry->dot11mode == WNI_CFG_DOT11_MODE_11A))
		session_entry->dot11mode = WNI_CFG_DOT11_MODE_11G;
	else if (WLAN_REG_IS_5GHZ_CH(csa_params->channel) &&
		 ((session_entry->dot11mode == WNI_CFG_DOT11_MODE_11G) ||
		 (session_entry->dot11mode == WNI_CFG_DOT11_MODE_11G_ONLY)))
		session_entry->dot11mode = WNI_CFG_DOT11_MODE_11A;

	/* Send RSO Stop to FW before triggering the vdev restart for CSA */
	if (mac_ctx->lim.stop_roaming_callback)
		mac_ctx->lim.stop_roaming_callback(mac_ctx,
						   session_entry->smeSessionId,
						   ecsr_driver_disabled);

	lim_prepare_for11h_channel_switch(mac_ctx, session_entry);

	csa_offload_ind->mesgType = eWNI_SME_CSA_OFFLOAD_EVENT;
	csa_offload_ind->mesgLen = sizeof(tSmeCsaOffloadInd);
	qdf_mem_copy(csa_offload_ind->bssid.bytes, session_entry->bssId,
			QDF_MAC_ADDR_SIZE);
	mmh_msg.type = eWNI_SME_CSA_OFFLOAD_EVENT;
	mmh_msg.bodyptr = csa_offload_ind;
	mmh_msg.bodyval = 0;
	pe_debug("Sending eWNI_SME_CSA_OFFLOAD_EVENT to SME");
	MTRACE(mac_trace_msg_tx
			(mac_ctx, session_entry->peSessionId, mmh_msg.type));
#ifdef FEATURE_WLAN_DIAG_SUPPORT
	lim_diag_event_report(mac_ctx,
			WLAN_PE_DIAG_SWITCH_CHL_IND_EVENT, session_entry,
			QDF_STATUS_SUCCESS, QDF_STATUS_SUCCESS);
#endif
	lim_sys_process_mmh_msg_api(mac_ctx, &mmh_msg);

err:
	qdf_mem_free(csa_params);
}

/*--------------------------------------------------------------------------
   \brief pe_delete_session() - Handle the Delete BSS Response from HAL.

   \param mac                   - pointer to global adapter context
   \param sessionId             - Message pointer.

   \sa
   --------------------------------------------------------------------------*/

void lim_handle_delete_bss_rsp(struct mac_context *mac, struct scheduler_msg *MsgQ)
{
	struct pe_session *pe_session;
	tpDeleteBssParams pDelBss = (tpDeleteBssParams) (MsgQ->bodyptr);

	pe_session =
		pe_find_session_by_session_id(mac, pDelBss->sessionId);
	if (pe_session == NULL) {
		pe_err("Session Does not exist for given sessionID: %d",
			pDelBss->sessionId);
		qdf_mem_free(MsgQ->bodyptr);
		MsgQ->bodyptr = NULL;
		return;
	}

	/*
	 * During DEL BSS handling, the PE Session will be deleted, but it is
	 * better to clear this flag if the session is hanging around due
	 * to some error conditions so that the next DEL_BSS request does
	 * not take the HO_FAIL path
	 */
	pe_session->process_ho_fail = false;
	if (LIM_IS_IBSS_ROLE(pe_session))
		lim_ibss_del_bss_rsp(mac, MsgQ->bodyptr, pe_session);
	else if (LIM_IS_UNKNOWN_ROLE(pe_session))
		lim_process_sme_del_bss_rsp(mac, MsgQ->bodyval, pe_session);
	else if (LIM_IS_NDI_ROLE(pe_session))
		lim_ndi_del_bss_rsp(mac, MsgQ->bodyptr, pe_session);
	else
		lim_process_mlm_del_bss_rsp(mac, MsgQ, pe_session);

}

/** -----------------------------------------------------------------
   \brief lim_send_sme_aggr_qos_rsp() - sends SME FT AGGR QOS RSP
 \      This function sends a eWNI_SME_FT_AGGR_QOS_RSP to SME.
 \      SME only looks at rc and tspec field.
   \param mac - global mac structure
   \param rspReqd - is SmeAddTsRsp required
   \param status - status code of eWNI_SME_FT_AGGR_QOS_RSP
   \return tspec
   \sa
   ----------------------------------------------------------------- */
void
lim_send_sme_aggr_qos_rsp(struct mac_context *mac, tpSirAggrQosRsp aggrQosRsp,
			  uint8_t smesessionId)
{
	struct scheduler_msg mmhMsg = {0};

	mmhMsg.type = eWNI_SME_FT_AGGR_QOS_RSP;
	mmhMsg.bodyptr = aggrQosRsp;
	mmhMsg.bodyval = 0;
	MTRACE(mac_trace(mac, TRACE_CODE_TX_SME_MSG,
			 smesessionId, mmhMsg.type));
	lim_sys_process_mmh_msg_api(mac, &mmhMsg);

	return;
}

void lim_send_sme_max_assoc_exceeded_ntf(struct mac_context *mac, tSirMacAddr peerMacAddr,
					 uint8_t smesessionId)
{
	struct scheduler_msg mmhMsg = {0};
	tSmeMaxAssocInd *pSmeMaxAssocInd;

	pSmeMaxAssocInd = qdf_mem_malloc(sizeof(tSmeMaxAssocInd));
	if (!pSmeMaxAssocInd)
		return;
	qdf_mem_copy((uint8_t *) pSmeMaxAssocInd->peer_mac.bytes,
		     (uint8_t *) peerMacAddr, QDF_MAC_ADDR_SIZE);
	pSmeMaxAssocInd->mesgType = eWNI_SME_MAX_ASSOC_EXCEEDED;
	pSmeMaxAssocInd->mesgLen = sizeof(tSmeMaxAssocInd);
	pSmeMaxAssocInd->sessionId = smesessionId;
	mmhMsg.type = pSmeMaxAssocInd->mesgType;
	mmhMsg.bodyptr = pSmeMaxAssocInd;
	pe_debug("msgType: %s peerMacAddr "MAC_ADDRESS_STR "sme session id %d",
		"eWNI_SME_MAX_ASSOC_EXCEEDED", MAC_ADDR_ARRAY(peerMacAddr),
		pSmeMaxAssocInd->sessionId);
	MTRACE(mac_trace(mac, TRACE_CODE_TX_SME_MSG,
			 smesessionId, mmhMsg.type));
	lim_sys_process_mmh_msg_api(mac, &mmhMsg);

	return;
}

/** -----------------------------------------------------------------
   \brief lim_send_sme_ap_channel_switch_resp() - sends
   eWNI_SME_CHANNEL_CHANGE_RSP
   After receiving WMA_SWITCH_CHANNEL_RSP indication this
   function sends a eWNI_SME_CHANNEL_CHANGE_RSP to SME to notify
   that the Channel change has been done to the specified target
   channel in the Channel change request
   \param mac - global mac structure
   \param pe_session - session info
   \param pChnlParams - Channel switch params
   --------------------------------------------------------------------*/
void
lim_send_sme_ap_channel_switch_resp(struct mac_context *mac,
				    struct pe_session *pe_session,
				    tpSwitchChannelParams pChnlParams)
{
	struct scheduler_msg mmhMsg = {0};
	tpSwitchChannelParams pSmeSwithChnlParams;
	uint8_t channelId;
	bool is_ch_dfs = false;
	enum phy_ch_width ch_width;
	uint8_t ch_center_freq_seg1;

	pSmeSwithChnlParams = qdf_mem_malloc(sizeof(tSwitchChannelParams));
	if (!pSmeSwithChnlParams)
		return;

	qdf_mem_copy(pSmeSwithChnlParams, pChnlParams,
		     sizeof(tSwitchChannelParams));

	channelId = pSmeSwithChnlParams->channelNumber;
	ch_width = pSmeSwithChnlParams->ch_width;
	ch_center_freq_seg1 = pSmeSwithChnlParams->ch_center_freq_seg1;

	/*
	 * Pass the sme sessionID to SME instead
	 * PE session ID.
	 */
	pSmeSwithChnlParams->peSessionId = pe_session->smeSessionId;

	mmhMsg.type = eWNI_SME_CHANNEL_CHANGE_RSP;
	mmhMsg.bodyptr = (void *)pSmeSwithChnlParams;
	mmhMsg.bodyval = 0;
	lim_sys_process_mmh_msg_api(mac, &mmhMsg);

	/*
	 * We should start beacon transmission only if the new
	 * channel after channel change is Non-DFS. For a DFS
	 * channel, PE will receive an explicit request from
	 * upper layers to start the beacon transmission .
	 */

	if (ch_width == CH_WIDTH_160MHZ) {
		is_ch_dfs = true;
	} else if (ch_width == CH_WIDTH_80P80MHZ) {
		if (wlan_reg_get_channel_state(mac->pdev, channelId) ==
				CHANNEL_STATE_DFS ||
		    wlan_reg_get_channel_state(mac->pdev,
			    ch_center_freq_seg1 -
			    SIR_80MHZ_START_CENTER_CH_DIFF) ==
							CHANNEL_STATE_DFS)
			is_ch_dfs = true;
	} else {
		if (wlan_reg_get_channel_state(mac->pdev, channelId) ==
				CHANNEL_STATE_DFS)
			is_ch_dfs = true;
	}

	if (!is_ch_dfs) {
		if (channelId == pe_session->currentOperChannel) {
			lim_apply_configuration(mac, pe_session);
			lim_send_beacon(mac, pe_session);
		} else {
			pe_debug("Failed to Transmit Beacons on channel: %d after AP channel change response",
				       pe_session->bcnLen);
		}

		lim_obss_send_detection_cfg(mac, pe_session, true);
	}
	return;
}

#ifdef WLAN_FEATURE_11AX_BSS_COLOR
/**
 *  lim_send_bss_color_change_ie_update() - update bss color change IE in
 *   beacon template
 *
 *  @mac_ctx: pointer to global adapter context
 *  @session: session pointer
 *
 *  Return: none
 */
static void
lim_send_bss_color_change_ie_update(struct mac_context *mac_ctx,
						struct pe_session *session)
{
	/* Update the beacon template and send to FW */
	if (sch_set_fixed_beacon_fields(mac_ctx, session) != QDF_STATUS_SUCCESS) {
		pe_err("Unable to set BSS color change IE in beacon");
		return;
	}

	/* Send update beacon template message */
	lim_send_beacon_ind(mac_ctx, session, REASON_COLOR_CHANGE);
	pe_debug("Updated BSS color change countdown = %d",
		 session->he_bss_color_change.countdown);
}

static void
lim_handle_bss_color_change_ie(struct mac_context *mac_ctx,
					struct pe_session *session)
{
	tUpdateBeaconParams beacon_params;

	/* handle bss color change IE */
	if (LIM_IS_AP_ROLE(session) &&
			session->he_op.bss_col_disabled) {
		if (session->he_bss_color_change.countdown > 0) {
			session->he_bss_color_change.countdown--;
		} else {
			session->bss_color_changing = 0;
			qdf_mem_zero(&beacon_params, sizeof(beacon_params));
			if (session->he_bss_color_change.new_color != 0) {
				session->he_op.bss_col_disabled = 0;
				session->he_op.bss_color =
					session->he_bss_color_change.new_color;
				beacon_params.paramChangeBitmap |=
					PARAM_BSS_COLOR_CHANGED;
				beacon_params.bss_color_disabled = 0;
				beacon_params.bss_color =
					session->he_op.bss_color;
				lim_send_beacon_params(mac_ctx,
						       &beacon_params,
						       session);
				lim_send_obss_color_collision_cfg(mac_ctx,
						session,
						OBSS_COLOR_COLLISION_DETECTION);
			}
		}

		lim_send_bss_color_change_ie_update(mac_ctx, session);
	}
}

#else
static void
lim_handle_bss_color_change_ie(struct mac_context *mac_ctx,
					struct pe_session *session)
{
}
#endif

void
lim_process_beacon_tx_success_ind(struct mac_context *mac_ctx, uint16_t msgType,
				  void *event)
{
	struct pe_session *session;
	tpSirFirstBeaconTxCompleteInd bcn_ind =
		(tSirFirstBeaconTxCompleteInd *) event;

	session = pe_find_session_by_bss_idx(mac_ctx, bcn_ind->bssIdx);
	if (!session) {
		pe_err("Session Does not exist for given session id");
		return;
	}

	pe_debug("role: %d swIe: %d opIe: %d switch cnt:%d",
		 GET_LIM_SYSTEM_ROLE(session),
		 session->dfsIncludeChanSwIe,
		 session->gLimOperatingMode.present,
		 session->gLimChannelSwitch.switchCount);

	if (!LIM_IS_AP_ROLE(session))
		return;

	if (session->dfsIncludeChanSwIe &&
	    (session->gLimChannelSwitch.switchCount ==
	    mac_ctx->sap.SapDfsInfo.sap_ch_switch_beacon_cnt))
		lim_process_ap_ecsa_timeout(session);


	if (session->gLimOperatingMode.present)
		/* Done with nss update */
		session->gLimOperatingMode.present = 0;

	lim_handle_bss_color_change_ie(mac_ctx, session);

	return;
}
