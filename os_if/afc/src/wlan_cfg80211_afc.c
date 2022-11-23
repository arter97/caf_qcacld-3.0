/*
 * Copyright (c) 2022, Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.

 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * DOC: Defines afc utility functions
 */

#include <pld_common.h>
#include <wlan_cfg80211.h>
#include <wlan_cfg80211_afc.h>
#include <wlan_reg_ucfg_api.h>
#include <wlan_objmgr_pdev_obj.h>
#include <wlan_osif_priv.h>
#include <wlan_hdd_main.h>
#include <wlan_hdd_object_manager.h>
#include <osif_psoc_sync.h>

#ifdef CONFIG_AFC_SUPPORT
const struct nla_policy
wlan_cfg80211_afc_response_policy[QCA_WLAN_VENDOR_ATTR_AFC_RESP_MAX + 1] = {
	[QCA_WLAN_VENDOR_ATTR_AFC_RESP_TIME_TO_LIVE] = { .type = NLA_U32 },
	[QCA_WLAN_VENDOR_ATTR_AFC_RESP_REQ_ID] = { .type = NLA_U32 },
	[QCA_WLAN_VENDOR_ATTR_AFC_RESP_EXP_DATE] = { .type = NLA_U32 },
	[QCA_WLAN_VENDOR_ATTR_AFC_RESP_EXP_TIME] = { .type = NLA_U32 },
	[QCA_WLAN_VENDOR_ATTR_AFC_RESP_AFC_SERVER_RESP_CODE] = {
							.type = NLA_S32 },
	[QCA_WLAN_VENDOR_ATTR_AFC_RESP_FREQ_PSD_INFO] = { .type = NLA_NESTED },
	[QCA_WLAN_VENDOR_ATTR_AFC_RESP_OPCLASS_CHAN_EIRP_INFO] = {
							.type = NLA_NESTED },
	[QCA_WLAN_VENDOR_ATTR_AFC_RESP_DATA] = { .type = NLA_STRING,
				.len = QCA_NL80211_AFC_RESP_BUF_MAX_SIZE },
};

/**
 * get_afc_evt_data_len() - Function to get vendor event buffer length needed
 * @event_type: AFC event type
 * @afc_data: AFC data pass from regulatory component
 *
 * Return: Buffer length needed, negative if error
 */
static int get_afc_evt_data_len(enum qca_wlan_vendor_afc_event_type event_type,
				void *afc_data)
{
	uint32_t len = NLMSG_HDRLEN;
	struct wlan_afc_host_partial_request *afc_req = NULL;
	struct wlan_afc_frange_list *afc_frange_list = NULL;
	struct wlan_afc_num_opclasses *afc_num_opclasses = NULL;
	struct wlan_afc_opclass_obj *afc_opclass_obj = NULL;
	struct reg_fw_afc_power_event *pow_evt = NULL;
	struct afc_chan_obj *pow_evt_chan_info = NULL;
	uint8_t *temp = NULL;
	uint16_t opclass_len = 0;
	int i = 0;

	/* QCA_WLAN_VENDOR_ATTR_AFC_EVENT_TYPE */
	len += nla_total_size(sizeof(u8));

	switch (event_type) {
	case QCA_WLAN_VENDOR_AFC_EVENT_TYPE_EXPIRY:
		afc_req = (struct wlan_afc_host_partial_request *)afc_data;
		len += /* QCA_WLAN_VENDOR_ATTR_AFC_EVENT_REQ_ID */
			nla_total_size(sizeof(u32)) +
			/* QCA_WLAN_VENDOR_ATTR_AFC_EVENT_AFC_WFA_VERSION */
			nla_total_size(sizeof(u32)) +
			/* QCA_WLAN_VENDOR_ATTR_AFC_EVENT_MIN_DES_POWER */
			nla_total_size(sizeof(u16)) +
			/* QCA_WLAN_VENDOR_ATTR_AFC_EVENT_AP_DEPLOYMENT */
			nla_total_size(sizeof(u8));

		temp = (uint8_t *)&afc_req->fixed_params;
		temp += sizeof(struct wlan_afc_host_req_fixed_params);
		afc_frange_list = (struct wlan_afc_frange_list *)temp;
		len +=
		/* QCA_WLAN_VENDOR_ATTR_AFC_FREQ_PSD_INFO_RANGE_START */
		(nla_total_size(sizeof(u32)) * afc_frange_list->num_ranges) +
		/* QCA_WLAN_VENDOR_ATTR_AFC_FREQ_PSD_INFO_RANGE_END */
		(nla_total_size(sizeof(u32)) * afc_frange_list->num_ranges);

		temp += (sizeof(struct wlan_afc_frange_list) +
			(sizeof(struct wlan_afc_freq_range_obj) *
			afc_frange_list->num_ranges));
		afc_num_opclasses = (struct wlan_afc_num_opclasses *)temp;

		len += /* QCA_WLAN_VENDOR_ATTR_AFC_OPCLASS_INFO_OPCLASS */
			(nla_total_size(sizeof(u8)) *
			afc_num_opclasses->num_opclasses);

		temp += sizeof(struct wlan_afc_num_opclasses);
		afc_opclass_obj = (struct wlan_afc_opclass_obj *)temp;

		for (i = 0; i < afc_num_opclasses->num_opclasses; i++) {
			/* QCA_WLAN_VENDOR_ATTR_AFC_CHAN_EIRP_INFO_CHAN_NUM */
			len += (nla_total_size(sizeof(u8)) *
			       afc_opclass_obj->opclass_num_cfis);

			opclass_len = sizeof(struct wlan_afc_opclass_obj);
			opclass_len += afc_opclass_obj->opclass_num_cfis *
				       sizeof(uint8_t);
			temp = (uint8_t *)afc_opclass_obj + opclass_len;
			afc_opclass_obj = (struct wlan_afc_opclass_obj *)temp;
		}
		break;
	case QCA_WLAN_VENDOR_AFC_EVENT_TYPE_POWER_UPDATE_COMPLETE:
		pow_evt = (struct reg_fw_afc_power_event *)afc_data;
		len += nla_total_size(
			/* QCA_WLAN_VENDOR_ATTR_AFC_EVENT_REQ_ID */
			nla_total_size(sizeof(u32)) +
			/* QCA_WLAN_VENDOR_ATTR_AFC_EVENT_STATUS_CODE */
			nla_total_size(sizeof(u8)) +
			/* QCA_WLAN_VENDOR_ATTR_AFC_EVENT_SERVER_RESP_CODE */
			nla_total_size(sizeof(s32)) +
			/* QCA_WLAN_VENDOR_ATTR_AFC_EVENT_EXP_DATE */
			nla_total_size(sizeof(u32)) +
			/* QCA_WLAN_VENDOR_ATTR_AFC_EVENT_EXP_TIME */
			nla_total_size(sizeof(u32)));
		len += nla_total_size(
			/* QCA_WLAN_VENDOR_ATTR_AFC_FREQ_PSD_INFO_RANGE_START */
			(nla_total_size(sizeof(u32)) * pow_evt->num_freq_objs) +
			/* QCA_WLAN_VENDOR_ATTR_AFC_FREQ_PSD_INFO_RANGE_END */
			(nla_total_size(sizeof(u32)) * pow_evt->num_freq_objs) +
			/* QCA_WLAN_VENDOR_ATTR_AFC_FREQ_PSD_INFO_PSD */
			(nla_total_size(sizeof(u32)) * pow_evt->num_freq_objs));

		len += nla_total_size(
			/* QCA_WLAN_VENDOR_ATTR_AFC_OPCLASS_INFO_OPCLASS */
			nla_total_size(sizeof(u8)) *  pow_evt->num_chan_objs);

		pow_evt_chan_info = pow_evt->afc_chan_info;
		for (i = 0; i < pow_evt->num_chan_objs; i++) {
			len += nla_total_size(
			/* QCA_WLAN_VENDOR_ATTR_AFC_CHAN_EIRP_INFO_CHAN_NUM */
			nla_total_size((nla_total_size(sizeof(u8)) *
			pow_evt_chan_info[i].num_chans) +
			/* QCA_WLAN_VENDOR_ATTR_AFC_CHAN_EIRP_INFO_EIRP */
			(nla_total_size(sizeof(u32)) *
			pow_evt_chan_info[i].num_chans)));
		}
		break;
	default:
		osif_err("Invalid AFC event type: %d", event_type);
			return -EINVAL;
		break;
	}
	return len;
}

/**
 * afc_expiry_event_update() - Function to fill vendor event buffer with
 * partial request
 * @vendor_event: Pointer to vendor event SK buffer structure
 * @afc_data: Pointer to AFC partial request
 *
 * Return: 0 if success, otherwise error code
 */
static int afc_expiry_event_update(struct sk_buff *vendor_event, void *afc_data)
{
	struct wlan_afc_host_partial_request *afc_req =
			(struct wlan_afc_host_partial_request *)afc_data;
	struct wlan_afc_frange_list *afc_frange_list;
	struct wlan_afc_num_opclasses *afc_num_opclasses;
	struct wlan_afc_opclass_obj *afc_opclass_obj;
	uint16_t opclass_len;
	uint8_t *p_temp, *p_temp_next;
	struct nlattr *nla_attr;
	struct nlattr *freq_info;
	struct nlattr *opclass_info;
	struct nlattr *chan_list;
	struct nlattr *chan_info;
	int i, j;

	/* Update the fixed parameters from the Expiry event */
	if (nla_put_u32(vendor_event,
			QCA_WLAN_VENDOR_ATTR_AFC_EVENT_REQ_ID,
			afc_req->fixed_params.req_id)) {
		osif_err("QCA_WLAN_VENDOR_ATTR_AFC_REQ_ID put fail");
		goto fail;
	}

	if (nla_put_u32(vendor_event,
			QCA_WLAN_VENDOR_ATTR_AFC_EVENT_AFC_WFA_VERSION,
			(afc_req->fixed_params.version_major << 16) |
			afc_req->fixed_params.version_minor)) {
		osif_err("AFC EVENT WFA version put fail");
		goto fail;
	}

	if (nla_put_u16(vendor_event,
			QCA_WLAN_VENDOR_ATTR_AFC_EVENT_MIN_DES_POWER,
			afc_req->fixed_params.min_des_power)) {
		osif_err("QCA_WLAN_VENDOR_ATTR_AFC_REQ_MIN_DES_PWR put fail");
		goto fail;
	}

	/* Mobile device deployment is unknown */
	if (nla_put_u8(vendor_event,
		       QCA_WLAN_VENDOR_ATTR_AFC_EVENT_AP_DEPLOYMENT,
		       QCA_WLAN_VENDOR_AFC_AP_DEPLOYMENT_TYPE_UNKNOWN)) {
		osif_err("AFC EVENT AP deployment put fail");
		goto fail;
	}

	/* Update the frequency range list from the Expiry event */
	nla_attr = nla_nest_start(
			vendor_event,
			QCA_WLAN_VENDOR_ATTR_AFC_EVENT_FREQ_RANGE_LIST);
	if (!nla_attr)
		goto fail;

	p_temp = (uint8_t *)&afc_req->fixed_params;
	p_temp += sizeof(struct wlan_afc_host_req_fixed_params);
	afc_frange_list = (struct wlan_afc_frange_list *)p_temp;

	for (i = 0; i < afc_frange_list->num_ranges; i++) {
		freq_info = nla_nest_start(vendor_event, i);
		if (!freq_info)
			goto fail;

		if (nla_put_u32(
			vendor_event,
			QCA_WLAN_VENDOR_ATTR_AFC_FREQ_PSD_INFO_RANGE_START,
			afc_frange_list->range_objs[i].lowfreq) ||
		    nla_put_u32(
			vendor_event,
			QCA_WLAN_VENDOR_ATTR_AFC_FREQ_PSD_INFO_RANGE_END,
			afc_frange_list->range_objs[i].highfreq)) {
			osif_err("AFC REQ FREQ RANGE LIST put fail");
			goto fail;
		}

		nla_nest_end(vendor_event, freq_info);
	}
	nla_nest_end(vendor_event, nla_attr);

	/* Update the Operating class and channel list */
	nla_attr = nla_nest_start(
			vendor_event,
			QCA_WLAN_VENDOR_ATTR_AFC_EVENT_OPCLASS_CHAN_LIST);
	if (!nla_attr)
		goto fail;

	p_temp += (sizeof(struct wlan_afc_frange_list) +
		  (sizeof(struct wlan_afc_freq_range_obj) *
		  afc_frange_list->num_ranges));
	afc_num_opclasses = (struct wlan_afc_num_opclasses *)p_temp;

	p_temp += sizeof(struct wlan_afc_num_opclasses);
	afc_opclass_obj = (struct wlan_afc_opclass_obj *)p_temp;

	for (i = 0; i < afc_num_opclasses->num_opclasses; i++) {
		opclass_info = nla_nest_start(vendor_event, i);
		if (!opclass_info)
			goto fail;

		if (nla_put_u8(vendor_event,
			       QCA_WLAN_VENDOR_ATTR_AFC_OPCLASS_INFO_OPCLASS,
			       afc_opclass_obj->opclass)) {
			osif_err("AFC OPCLASS INFO OPCLASS put fail");
			goto fail;
		}

		chan_list = nla_nest_start(
			vendor_event,
			QCA_WLAN_VENDOR_ATTR_AFC_OPCLASS_INFO_CHAN_LIST);
		if (!chan_list)
			goto fail;

		for (j = 0; j < afc_opclass_obj->opclass_num_cfis; j++) {
			chan_info = nla_nest_start(vendor_event, j);
			if (!chan_info)
				goto fail;

			if (nla_put_u8(
			    vendor_event,
			    QCA_WLAN_VENDOR_ATTR_AFC_CHAN_EIRP_INFO_CHAN_NUM,
			    afc_opclass_obj->cfis[j])) {
				osif_err("AFC EIRP INFO CHAN NUM put fail");
				goto fail;
			}

			nla_nest_end(vendor_event, chan_info);
		}
		nla_nest_end(vendor_event, chan_list);

		nla_nest_end(vendor_event, opclass_info);

		opclass_len = sizeof(struct wlan_afc_opclass_obj);
		opclass_len +=
			(afc_opclass_obj->opclass_num_cfis * sizeof(uint8_t));
		p_temp_next = (uint8_t *)afc_opclass_obj + opclass_len;
		afc_opclass_obj = (struct wlan_afc_opclass_obj *)p_temp_next;
	}

	nla_nest_end(vendor_event, nla_attr);

	return QDF_STATUS_SUCCESS;

fail:
	return -EINVAL;
}

/**
 * afc_power_event_update() - Function to fill vendor event buffer with AFC
 * power update event
 * @vendor_event: Pointer to vendor event SK buffer
 * @afc_data: Pointer to AFC power event
 *
 * Return: 0 if success, otherwise error code
 */
static int afc_power_event_update(struct sk_buff *vendor_event, void *afc_data)
{
	struct reg_fw_afc_power_event *pwr_evt =
				(struct reg_fw_afc_power_event *)afc_data;
	struct afc_chan_obj *pow_evt_chan_info = NULL;
	struct chan_eirp_obj *pow_evt_eirp_info = NULL;
	struct nlattr *nla_attr;
	struct nlattr *freq_info;
	struct nlattr *opclass_info;
	struct nlattr *chan_list;
	struct nlattr *chan_info;
	int i, j;

	/* Update the fixed parameters from the Power event */
	if (nla_put_u32(vendor_event,
			QCA_WLAN_VENDOR_ATTR_AFC_EVENT_REQ_ID,
			pwr_evt->resp_id)) {
		osif_err("QCA_WLAN_VENDOR_ATTR_AFC_EVENT_REQ_ID put fail");
		goto fail;
	}

	if (nla_put_u8(vendor_event,
		       QCA_WLAN_VENDOR_ATTR_AFC_EVENT_STATUS_CODE,
		       pwr_evt->fw_status_code)) {
		osif_err("AFC EVENT STATUS CODE put fail");
		goto fail;
	}

	if (nla_put_s32(vendor_event,
			QCA_WLAN_VENDOR_ATTR_AFC_EVENT_SERVER_RESP_CODE,
			pwr_evt->serv_resp_code)) {
		osif_err("AFC EVENT SERVER RESP CODE put fail");
		goto fail;
	}

	if (nla_put_u32(vendor_event,
			QCA_WLAN_VENDOR_ATTR_AFC_EVENT_EXP_DATE,
			pwr_evt->avail_exp_time_d)) {
		osif_err("AFC EVENT EXPIRE DATE put fail");
		goto fail;
	}

	if (nla_put_u32(vendor_event,
			QCA_WLAN_VENDOR_ATTR_AFC_EVENT_EXP_TIME,
			pwr_evt->avail_exp_time_t)) {
		osif_err("AFC EVENT EXPIRE TIME put fail");
		goto fail;
	}

	/* Update the Frequency and corresponding PSD info */
	nla_attr = nla_nest_start(
			vendor_event,
			QCA_WLAN_VENDOR_ATTR_AFC_EVENT_FREQ_RANGE_LIST);
	if (!nla_attr)
		goto fail;

	for (i = 0; i < pwr_evt->num_freq_objs; i++) {
		freq_info = nla_nest_start(vendor_event, i);
		if (!freq_info)
			goto fail;

		if (nla_put_u32(
			vendor_event,
			QCA_WLAN_VENDOR_ATTR_AFC_FREQ_PSD_INFO_RANGE_START,
			pwr_evt->afc_freq_info[i].low_freq) ||
		    nla_put_u32(
			vendor_event,
			QCA_WLAN_VENDOR_ATTR_AFC_FREQ_PSD_INFO_RANGE_END,
			pwr_evt->afc_freq_info[i].high_freq) ||
		    nla_put_u32(
			vendor_event,
			QCA_WLAN_VENDOR_ATTR_AFC_FREQ_PSD_INFO_PSD,
			pwr_evt->afc_freq_info[i].max_psd)) {
			osif_err("AFC FREQUENCY PSD INFO put failed");
			goto fail;
		}

		nla_nest_end(vendor_event, freq_info);
	}
	nla_nest_end(vendor_event, nla_attr);

	/* Update the Operating class, channel list and EIRP info */
	nla_attr = nla_nest_start(
			vendor_event,
			QCA_WLAN_VENDOR_ATTR_AFC_EVENT_OPCLASS_CHAN_LIST);
	if (!nla_attr)
		goto fail;

	pow_evt_chan_info = pwr_evt->afc_chan_info;

	for (i = 0; i < pwr_evt->num_chan_objs; i++) {
		opclass_info = nla_nest_start(vendor_event, i);
		if (!opclass_info)
			goto fail;

		if (nla_put_u8(vendor_event,
			       QCA_WLAN_VENDOR_ATTR_AFC_OPCLASS_INFO_OPCLASS,
			       pow_evt_chan_info[i].global_opclass)) {
			osif_err("AFC OPCLASS INFO put fail");
			goto fail;
		}

		chan_list = nla_nest_start(
			vendor_event,
			QCA_WLAN_VENDOR_ATTR_AFC_OPCLASS_INFO_CHAN_LIST);
		if (!chan_list)
			goto fail;

		pow_evt_eirp_info = pow_evt_chan_info[i].chan_eirp_info;

		for (j = 0; j < pow_evt_chan_info[i].num_chans; j++) {
			chan_info = nla_nest_start(vendor_event, j);
			if (!chan_info)
				goto fail;

			if (nla_put_u8(
			    vendor_event,
			    QCA_WLAN_VENDOR_ATTR_AFC_CHAN_EIRP_INFO_CHAN_NUM,
			    pow_evt_eirp_info[j].cfi) ||
			    nla_put_u32(
			    vendor_event,
			    QCA_WLAN_VENDOR_ATTR_AFC_CHAN_EIRP_INFO_EIRP,
			    pow_evt_eirp_info[j].eirp_power)) {
				osif_err("AFC CHAN EIRP_INFO put fail");
				goto fail;
			}

			nla_nest_end(vendor_event, chan_info);
		}
		nla_nest_end(vendor_event, chan_list);

		nla_nest_end(vendor_event, opclass_info);
	}

	nla_nest_end(vendor_event, nla_attr);

	return QDF_STATUS_SUCCESS;

fail:
	return -EINVAL;
}

/**
 * wlan_cfg80211_trigger_afc_event - Handler for AFC events. Construct message
 *                                   for AFC event and post it to application.
 * @pdev: Pointer to pdev object
 * @event_type: Type of AFC event
 * @afc_data: Pointer to data to be sent to user application
 *
 * Return: 0 on success, negative errno on failure
 */
int
wlan_cfg80211_trigger_afc_event(struct wlan_objmgr_pdev *pdev,
				enum qca_wlan_vendor_afc_event_type event_type,
				void *afc_data)
{
	int ret_val = 0;
	struct sk_buff *vendor_event;
	struct pdev_osif_priv *osif_priv;
	int len;

	osif_priv = wlan_pdev_get_ospriv(pdev);
	if (!osif_priv) {
		osif_err("PDEV OS private structure is NULL");
		return -EINVAL;
	}

	if (!afc_data) {
		osif_err("AFC data buffer is NULL! Event Type: %d", event_type);
		return -EINVAL;
	}

	len = get_afc_evt_data_len(event_type, afc_data);

	if (len <= 0) {
		osif_err("Invalid length");
		return -EINVAL;
	}

	/* Allocate vendor event for AFC */
	vendor_event =
		wlan_cfg80211_vendor_event_alloc(
				osif_priv->wiphy, NULL,
				len,
				QCA_NL80211_VENDOR_SUBCMD_AFC_EVENT_INDEX,
				GFP_ATOMIC);
	if (!vendor_event) {
		osif_err("cfg80211_vendor_event_alloc failed");
		return -ENOMEM;
	}

	/* Update the AFC Event Type */
	ret_val = nla_put_u8(vendor_event, QCA_WLAN_VENDOR_ATTR_AFC_EVENT_TYPE,
			     event_type);
	if (ret_val) {
		osif_err("QCA_WLAN_VENDOR_ATTR_AFC_EVENT_TYPE put fail");
		wlan_cfg80211_vendor_free_skb(vendor_event);
		return ret_val;
	}

	switch (event_type) {
	case QCA_WLAN_VENDOR_AFC_EVENT_TYPE_EXPIRY:
		/* Copy over the AFC Event data */
		ret_val = afc_expiry_event_update(vendor_event, afc_data);
		break;
	case QCA_WLAN_VENDOR_AFC_EVENT_TYPE_POWER_UPDATE_COMPLETE:
		/* Copy over the AFC Power event data */
		ret_val = afc_power_event_update(vendor_event, afc_data);
		break;
	default:
		osif_err("Invalid event type %d", event_type);
		ret_val = -EINVAL;
	}

	if (ret_val) {
		osif_err("AFC event update fail");
		wlan_cfg80211_vendor_free_skb(vendor_event);
		return ret_val;
	}

	osif_debug("Sending AFC event: %s to user application",
		   (event_type == QCA_WLAN_VENDOR_AFC_EVENT_TYPE_EXPIRY) ?
		   "Expiry event" :
		   (event_type ==
		    QCA_WLAN_VENDOR_AFC_EVENT_TYPE_POWER_UPDATE_COMPLETE) ?
		   "Power update complete event" : "");

	/* Send AFC event to user space application */
	wlan_cfg80211_vendor_event(vendor_event, GFP_ATOMIC);

	return ret_val;
}

/**
 * afc_response_buffer_display() - Function to display AFC response information
 * @afc_rsp: Pointer to AFC response structure
 *
 * Return: None
 */
static void afc_response_buffer_display(struct wlan_afc_host_resp *afc_rsp)
{
	struct wlan_afc_bin_resp_data *afc_bin = NULL;
	struct wlan_afc_resp_freq_psd_info *freq_obj = NULL;
	struct wlan_afc_resp_opclass_info *opclass_obj = NULL;
	struct wlan_afc_resp_eirp_info *eirp_obj = NULL;
	uint8_t *tmp_ptr = NULL;
	int iter, iter_j;

	/* Display the AFC Response Fixed parameters */
	osif_debug("Header: %u Status: %u TTL: %u Length: %u Format: %u",
		   afc_rsp->header,
		   afc_rsp->status,
		   afc_rsp->time_to_live,
		   afc_rsp->length,
		   afc_rsp->resp_format);

	afc_bin = (struct wlan_afc_bin_resp_data *)&afc_rsp->afc_resp[0];

	osif_debug("Loc Err: %u Ver: 0x%x WFA Ver: 0x%x Req ID: %u Date: 0x%x",
		   afc_bin->local_err_code,
		   afc_bin->version,
		   afc_bin->afc_wfa_version,
		   afc_bin->request_id,
		   afc_bin->avail_exp_time_d);
	osif_debug("Time: 0x%x Ser Resp: %u Freq objs: %u Opclass objs: %u",
		   afc_bin->avail_exp_time_t,
		   afc_bin->afc_serv_resp_code,
		   afc_bin->num_frequency_obj,
		   afc_bin->num_channel_obj);

	/* Display Frequency/PSD info from AFC Response */
	freq_obj = (struct wlan_afc_resp_freq_psd_info *)
			((uint8_t *)afc_bin +
			sizeof(struct wlan_afc_bin_resp_data));
	tmp_ptr = (uint8_t *)freq_obj;

	for (iter = 0; iter < afc_bin->num_frequency_obj; iter++) {
		osif_debug("Freq Info[%d]: 0x%x Max PSD[%d]: %u ",
			   iter, freq_obj->freq_info, iter, freq_obj->max_psd);
		freq_obj = (struct wlan_afc_resp_freq_psd_info *)
				((uint8_t *)freq_obj +
				sizeof(struct wlan_afc_resp_freq_psd_info));
	}

	/* Display Opclass and channel EIRP info from AFC Response */
	opclass_obj = (struct wlan_afc_resp_opclass_info *)
			((uint8_t *)tmp_ptr +
			(afc_bin->num_frequency_obj *
			sizeof(struct wlan_afc_resp_freq_psd_info)));
	for (iter = 0; iter < afc_bin->num_channel_obj; iter++) {
		osif_debug("Opclass[%d]: %u Num channels[%d]: %u",
			   iter, opclass_obj->opclass,
			   iter, opclass_obj->num_channels);

		eirp_obj = (struct wlan_afc_resp_eirp_info *)
			((uint8_t *)opclass_obj +
			sizeof(struct wlan_afc_resp_opclass_info));
		for (iter_j = 0; iter_j < opclass_obj->num_channels; iter_j++) {
			osif_debug("Channel Info[%d]:CFI: %u EIRP: %u ",
				   iter_j, eirp_obj->channel_cfi,
				   eirp_obj->max_eirp_pwr);
			eirp_obj = (struct wlan_afc_resp_eirp_info *)
					((uint8_t *)eirp_obj +
					sizeof(struct wlan_afc_resp_eirp_info));
		}
		opclass_obj = (struct wlan_afc_resp_opclass_info *)
				((uint8_t *)opclass_obj +
				sizeof(struct wlan_afc_resp_opclass_info) +
				(opclass_obj->num_channels *
				 sizeof(struct wlan_afc_resp_eirp_info)));
	}
}

/**
 * extract_afc_resp() - Function to extract AFC response
 * @attr: Pointer to NL attribute array
 * @afc_resp_len: AFC response length extracted
 * @is_bin: AFC response format is BIN or JSON
 *
 * Return: Pointer to AFC response axtracted
 */
static struct wlan_afc_host_resp *extract_afc_resp(struct nlattr **attr,
						   int *afc_resp_len,
						   bool *is_bin)
{
	struct wlan_afc_host_resp *afc_rsp = NULL;
	struct wlan_afc_bin_resp_data *afc_fixed_params;
	struct wlan_afc_resp_freq_psd_info *frange_obj;
	struct wlan_afc_resp_opclass_info *start_opcls, *opclass_list;
	struct wlan_afc_resp_eirp_info *chan_obj;
	struct nlattr *nl, *nl_attr;
	struct nlattr *frange_info[
			QCA_WLAN_VENDOR_ATTR_AFC_FREQ_PSD_INFO_MAX + 1];
	struct nlattr *opclass_info[
			QCA_WLAN_VENDOR_ATTR_AFC_OPCLASS_INFO_MAX + 1];
	struct nlattr *chan_info[
			QCA_WLAN_VENDOR_ATTR_AFC_CHAN_EIRP_INFO_MAX + 1];
	uint32_t num_frange_obj = 0, num_channels = 0;
	uint32_t total_channels = 0;
	uint32_t start_freq = 0, end_freq = 0;
	uint8_t num_opclas_obj = 0;
	uint8_t *temp;
	int rem, iter, i;

	if (attr[QCA_WLAN_VENDOR_ATTR_AFC_RESP_DATA]) {
		nl = attr[QCA_WLAN_VENDOR_ATTR_AFC_RESP_DATA];
		*afc_resp_len = nla_len(nl);
		afc_rsp = (struct wlan_afc_host_resp *)qdf_mem_malloc(
						*afc_resp_len);
		if (!afc_rsp)
			goto fail;

		nla_memcpy(afc_rsp, nl, *afc_resp_len);
		*is_bin = false;
		return afc_rsp;
	}

	if (attr[QCA_WLAN_VENDOR_ATTR_AFC_RESP_FREQ_PSD_INFO]) {
		nla_for_each_nested(
			nl,
			attr[QCA_WLAN_VENDOR_ATTR_AFC_RESP_FREQ_PSD_INFO],
			rem)
			num_frange_obj++;
	}

	if (attr[QCA_WLAN_VENDOR_ATTR_AFC_RESP_OPCLASS_CHAN_EIRP_INFO]) {
		nla_for_each_nested(
			nl, attr[
			QCA_WLAN_VENDOR_ATTR_AFC_RESP_OPCLASS_CHAN_EIRP_INFO],
			rem) {
			num_channels = 0;

			if (wlan_cfg80211_nla_parse(
				opclass_info,
				QCA_WLAN_VENDOR_ATTR_AFC_OPCLASS_INFO_MAX,
				nla_data(nl),
				nla_len(nl),
				NULL))
				goto fail;

			nla_for_each_nested(
			nl_attr,
			opclass_info[
			QCA_WLAN_VENDOR_ATTR_AFC_OPCLASS_INFO_CHAN_LIST],
			iter)
			num_channels++;

			num_opclas_obj++;
			total_channels += num_channels;
		}
	}

	/* Calculate the total length required for AFC response buffer */
	*afc_resp_len = (sizeof(struct wlan_afc_host_resp) +
		sizeof(struct wlan_afc_bin_resp_data) +
		(num_frange_obj * sizeof(struct wlan_afc_resp_freq_psd_info)) +
		(num_opclas_obj * sizeof(struct wlan_afc_resp_opclass_info)) +
		(total_channels * sizeof(struct wlan_afc_resp_eirp_info)));
	afc_rsp = (struct wlan_afc_host_resp *)qdf_mem_malloc(*afc_resp_len);
	if (!afc_rsp)
		goto fail;

	afc_rsp->resp_format = REG_AFC_SERV_RESP_FORMAT_BINARY;
	*is_bin = true;

	if (attr[QCA_WLAN_VENDOR_ATTR_AFC_RESP_TIME_TO_LIVE]) {
		afc_rsp->time_to_live =
		nla_get_u32(attr[QCA_WLAN_VENDOR_ATTR_AFC_RESP_TIME_TO_LIVE]);
	}

	/* Update the AFC fixed parameters from the AFC response */
	afc_fixed_params = (struct wlan_afc_bin_resp_data *)afc_rsp->afc_resp;

	if (attr[QCA_WLAN_VENDOR_ATTR_AFC_RESP_REQ_ID]) {
		afc_fixed_params->request_id =
			nla_get_u32(attr[QCA_WLAN_VENDOR_ATTR_AFC_RESP_REQ_ID]);
	}

	if (attr[QCA_WLAN_VENDOR_ATTR_AFC_RESP_EXP_DATE]) {
		afc_fixed_params->avail_exp_time_d =
			nla_get_u32(
				attr[QCA_WLAN_VENDOR_ATTR_AFC_RESP_EXP_DATE]);
	}

	if (attr[QCA_WLAN_VENDOR_ATTR_AFC_RESP_EXP_TIME]) {
		afc_fixed_params->avail_exp_time_t =
			nla_get_u32(
				attr[QCA_WLAN_VENDOR_ATTR_AFC_RESP_EXP_TIME]);
	}

	if (attr[QCA_WLAN_VENDOR_ATTR_AFC_RESP_AFC_SERVER_RESP_CODE]) {
		afc_fixed_params->afc_serv_resp_code =
		nla_get_s32(attr[
			QCA_WLAN_VENDOR_ATTR_AFC_RESP_AFC_SERVER_RESP_CODE]);
	}

	afc_fixed_params->num_frequency_obj = num_frange_obj;
	afc_fixed_params->num_channel_obj = num_opclas_obj;

	/* Start parsing and updating the frequency range list */
	temp = (uint8_t *)afc_fixed_params;
	frange_obj = (struct wlan_afc_resp_freq_psd_info *)
			(temp + sizeof(struct wlan_afc_bin_resp_data));

	i = 0;

	if (attr[QCA_WLAN_VENDOR_ATTR_AFC_RESP_FREQ_PSD_INFO]) {
		nla_for_each_nested(
			nl,
			attr[QCA_WLAN_VENDOR_ATTR_AFC_RESP_FREQ_PSD_INFO],
			rem) {
			if (wlan_cfg80211_nla_parse(
				frange_info,
				QCA_WLAN_VENDOR_ATTR_AFC_FREQ_PSD_INFO_MAX,
				nla_data(nl),
				nla_len(nl),
				NULL))
				goto fail;

			if (frange_info[
			QCA_WLAN_VENDOR_ATTR_AFC_FREQ_PSD_INFO_RANGE_START])
				start_freq = nla_get_u32(frange_info[
			QCA_WLAN_VENDOR_ATTR_AFC_FREQ_PSD_INFO_RANGE_START]);

			if (frange_info[
			QCA_WLAN_VENDOR_ATTR_AFC_FREQ_PSD_INFO_RANGE_END])
				end_freq = nla_get_u32(frange_info[
			QCA_WLAN_VENDOR_ATTR_AFC_FREQ_PSD_INFO_RANGE_END]);

			frange_obj[i].freq_info = (start_freq & 0x0000FFFF) |
						  (end_freq << 16);

			if (frange_info[
				QCA_WLAN_VENDOR_ATTR_AFC_FREQ_PSD_INFO_PSD])
				frange_obj[i].max_psd = nla_get_u32(frange_info[
				QCA_WLAN_VENDOR_ATTR_AFC_FREQ_PSD_INFO_PSD]);

			i++;
		}
	}

	/*
	 * Start parsing and updating the opclass list and corresponding channel
	 * and EIRP power information.
	 */
	temp = (uint8_t *)frange_obj;
	start_opcls = (struct wlan_afc_resp_opclass_info *)(temp +
		sizeof(struct wlan_afc_resp_freq_psd_info) * num_frange_obj);
	opclass_list = start_opcls;

	if (attr[QCA_WLAN_VENDOR_ATTR_AFC_RESP_OPCLASS_CHAN_EIRP_INFO]) {
		nla_for_each_nested(
			nl, attr[
			QCA_WLAN_VENDOR_ATTR_AFC_RESP_OPCLASS_CHAN_EIRP_INFO],
			rem) {
			if (wlan_cfg80211_nla_parse(
				opclass_info,
				QCA_WLAN_VENDOR_ATTR_AFC_OPCLASS_INFO_MAX,
				nla_data(nl),
				nla_len(nl),
				NULL))
				goto fail;

			if (opclass_info[
				QCA_WLAN_VENDOR_ATTR_AFC_OPCLASS_INFO_OPCLASS])
				opclass_list->opclass = nla_get_u8(
				opclass_info[
				QCA_WLAN_VENDOR_ATTR_AFC_OPCLASS_INFO_OPCLASS]);

			temp = (uint8_t *)opclass_list;
			chan_obj = (struct wlan_afc_resp_eirp_info *)
				(temp +
				sizeof(struct wlan_afc_resp_opclass_info));
			i = 0;
			nla_for_each_nested(
			nl_attr, opclass_info[
			QCA_WLAN_VENDOR_ATTR_AFC_OPCLASS_INFO_CHAN_LIST],
			iter) {
				if (wlan_cfg80211_nla_parse(
				chan_info,
				QCA_WLAN_VENDOR_ATTR_AFC_CHAN_EIRP_INFO_MAX,
				nla_data(nl_attr),
				nla_len(nl_attr),
				NULL))
					goto fail;

				if (chan_info[
				QCA_WLAN_VENDOR_ATTR_AFC_CHAN_EIRP_INFO_CHAN_NUM])
				chan_obj[i].channel_cfi = nla_get_u8(
				chan_info[
				QCA_WLAN_VENDOR_ATTR_AFC_CHAN_EIRP_INFO_CHAN_NUM]);

				if (chan_info[
				QCA_WLAN_VENDOR_ATTR_AFC_CHAN_EIRP_INFO_EIRP])
				chan_obj[i].max_eirp_pwr = nla_get_u32(
				chan_info[
				QCA_WLAN_VENDOR_ATTR_AFC_CHAN_EIRP_INFO_EIRP]);

				i++;
			}

			opclass_list->num_channels = i;
			temp = (uint8_t *)chan_obj;
			opclass_list = (struct wlan_afc_resp_opclass_info *)
				(temp +
				(sizeof(struct wlan_afc_resp_eirp_info) *
				opclass_list->num_channels));
		}
	}

	return afc_rsp;
fail:
	osif_err("Error parsing the AFC response from application");
	if (afc_rsp)
		qdf_mem_free(afc_rsp);

	return NULL;
}

/**
 * __wlan_cfg80211_afc_response() - AFC response command from user application
 * @wiphy: Pointer to wireless wiphy structure
 * @wdev: Pointer to wireless_dev structure
 * @data: Pointer to the data to be passed via vendor interface
 * @data_len: Length of the data to be passed
 *
 * Return: 0 on success, negative errno on failure
 */
static int __wlan_cfg80211_afc_response(struct wiphy *wiphy,
					struct wireless_dev *wdev,
					const void *data,
					int data_len)
{
	struct hdd_context *hdd_ctx = wiphy_priv(wiphy);
	struct wlan_objmgr_psoc *psoc;
	qdf_device_t qdf_dev;
	struct wlan_objmgr_pdev *pdev;
	struct nlattr *attr[QCA_WLAN_VENDOR_ATTR_AFC_RESP_MAX + 1];
	struct wlan_afc_host_resp *afc_rsp = NULL;
	bool is_bin = false;
	struct reg_afc_resp_rx_ind_info afc_ind_obj;
	int afc_resp_len = 0;
	int pdev_id;
	int ret_val = -EINVAL;

	hdd_enter();

	if (!data || !data_len) {
		osif_err("Invalid data length data ptr: %pK ", data);
		return -EINVAL;
	}

	pdev = hdd_ctx->pdev;
	if (!pdev) {
		osif_err("Invalid pdev");
		goto out;
	}

	psoc = hdd_ctx->psoc;
	if (!psoc) {
		osif_err("Invalid psoc");
		goto out;
	}

	qdf_dev = wlan_psoc_get_qdf_dev(psoc);
	if (!qdf_dev) {
		osif_err("Invalid qdf dev");
		goto out;
	}

	pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);

	if (wlan_cfg80211_nla_parse(attr, QCA_WLAN_VENDOR_ATTR_AFC_RESP_MAX,
				    data, data_len,
				    wlan_cfg80211_afc_response_policy)) {
		osif_err("Error parsing NL attributes");
		goto out;
	}

	afc_rsp = extract_afc_resp(attr, &afc_resp_len, &is_bin);

	if (afc_rsp) {
		if (is_bin)
			afc_response_buffer_display(afc_rsp);

		ret_val = pld_send_buffer_to_afcmem(
				qdf_dev->dev, (char *)afc_rsp,
				afc_resp_len, pdev_id);
	} else {
		goto out;
	}

	if (ret_val) {
		osif_err("AFC memory copy failed %d", ret_val);
		qdf_mem_free(afc_rsp);
		goto out;
	} else {
		osif_debug("AFC response copied to AFC memory");
	}

	/* Send WMI cmd to FW to indicate AFC response is copied */
	afc_ind_obj.cmd_type = REG_AFC_CMD_SERV_RESP_READY;
	afc_ind_obj.serv_resp_format = is_bin ?
			REG_AFC_SERV_RESP_FORMAT_BINARY :
			REG_AFC_SERV_RESP_FORMAT_JSON;
	ret_val = ucfg_reg_send_afc_resp_rx_ind(pdev, &afc_ind_obj);

	if (ret_val) {
		osif_err("AFC RX INDICATION FAILED");
		qdf_mem_free(afc_rsp);
		goto out;
	} else {
		osif_debug("AFC response indication sent to FW");
	}

	qdf_mem_free(afc_rsp);

out:
	return ret_val;
}

int wlan_cfg80211_afc_response(struct wiphy *wiphy,
			       struct wireless_dev *wdev,
			       const void *data,
			       int data_len)
{
	struct osif_psoc_sync *psoc_sync;
	int ret;

	ret = osif_psoc_sync_op_start(wiphy_dev(wiphy), &psoc_sync);
	if (ret)
		return ret;

	ret = __wlan_cfg80211_afc_response(wiphy, wdev, data, data_len);

	osif_psoc_sync_op_stop(psoc_sync);
	return ret;
}
#endif
