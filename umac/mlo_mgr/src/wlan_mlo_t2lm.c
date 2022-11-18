/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * DOC: contains T2LM APIs
 */

#include <wlan_objmgr_pdev_obj.h>
#include <wlan_objmgr_vdev_obj.h>
#include <wlan_objmgr_peer_obj.h>
#include <wlan_mlo_mgr_public_structs.h>
#include <wlan_mlo_t2lm.h>
#include <wlan_mlo_mgr_cmn.h>
#include <qdf_util.h>

/**
 * wlan_mlo_get_hw_link_id_mask() - Return hw_link_id mask for a given
 * ieee_link_id_mask.
 * @wlan_vdev_list: vdev list
 * @vdev_count: vdev count
 * @ieee_link_id_mask: IEEE link id mask
 *
 * Return: hw_link_id mask
 */
static uint8_t wlan_mlo_get_hw_link_id_mask(
		struct wlan_objmgr_vdev **wlan_vdev_list,
		uint16_t vdev_count, uint16_t ieee_link_id_mask)
{
	struct wlan_objmgr_pdev *pdev;
	struct wlan_objmgr_vdev *vdev;
	uint8_t link_id;
	uint16_t ieee_link_id;
	uint16_t hw_link_id_mask = 0;
	int i;

	for (link_id = 0; link_id < WLAN_T2LM_MAX_NUM_LINKS; link_id++) {
		ieee_link_id = ieee_link_id_mask & BIT(link_id);
		if (!ieee_link_id)
			continue;

		for (i = 0; i < vdev_count; i++) {
			vdev = wlan_vdev_list[i];
			if (!vdev) {
				t2lm_err("vdev is null");
				continue;
			}

			if (ieee_link_id != BIT(vdev->vdev_mlme.mlo_link_id))
				continue;

			pdev = wlan_vdev_get_pdev(vdev);
			if (!pdev) {
				t2lm_err("pdev is null");
				continue;
			}

			hw_link_id_mask |=
				BIT(wlan_mlo_get_pdev_hw_link_id(pdev));
			break;
		}
	}

	return hw_link_id_mask;
}

/**
 * wlan_mlo_parse_t2lm_provisioned_links() - Parse the T2LM provisioned links
 * from the T2LM IE.
 * @peer: Pointer to peer structure
 * @link_mapping_presence_ind: This indicates whether the link mapping of TIDs
 *                             present in the T2LM IE.
 * @link_mapping_of_tids: Link mapping of each TID
 * @t2lm: Pointer to a T2LM structure
 * @dir: DL/UL/BIDI direction
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS wlan_mlo_parse_t2lm_provisioned_links(
		struct wlan_objmgr_peer *peer,
		uint8_t link_mapping_presence_ind,
		uint8_t *link_mapping_of_tids,
		struct wlan_t2lm_onging_negotiation_info *t2lm,
		enum wlan_t2lm_direction dir)
{
	struct wlan_objmgr_vdev *vdev;
	struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {0};
	uint16_t vdev_count = 0;
	uint16_t hw_link_id_mask;
	uint8_t tid_num;
	uint16_t i;

	vdev = wlan_peer_get_vdev(peer);
	if (!vdev) {
		t2lm_err("vdev is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	mlo_get_ml_vdev_list(vdev, &vdev_count, wlan_vdev_list);
	if (!vdev_count) {
		t2lm_err("Number of VDEVs under MLD is reported as 0");
		return QDF_STATUS_E_NULL_VALUE;
	}

	for (tid_num = 0; tid_num < T2LM_MAX_NUM_TIDS; tid_num++) {
		if (link_mapping_presence_ind & BIT(tid_num)) {
			hw_link_id_mask = wlan_mlo_get_hw_link_id_mask(
					wlan_vdev_list, vdev_count,
					*(uint16_t *)link_mapping_of_tids);
			t2lm->t2lm_info[dir].hw_link_map_tid[tid_num] =
				hw_link_id_mask;
			link_mapping_of_tids += sizeof(uint16_t);
		}
	}

	for (i = 0; i < vdev_count; i++)
		mlo_release_vdev_ref(wlan_vdev_list[i]);

	return QDF_STATUS_SUCCESS;
}

/**
 * wlan_mlo_parse_t2lm_info() - Parse T2LM IE fields
 * @peer: Pointer to peer structure
 * @ie: Pointer to T2LM IE
 * @t2lm: Pointer to T2LM structure
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS wlan_mlo_parse_t2lm_info(
		struct wlan_objmgr_peer *peer, uint8_t *ie,
		struct wlan_t2lm_onging_negotiation_info *t2lm)
{
	enum wlan_t2lm_direction dir;
	uint8_t *t2lm_control_field;
	uint16_t t2lm_control;
	uint8_t link_mapping_presence_ind;
	uint8_t *link_mapping_of_tids;
	uint8_t tid_num;
	QDF_STATUS ret;

	t2lm_control_field = ie + sizeof(struct wlan_ie_tid_to_link_mapping);
	if (!t2lm_control_field) {
		t2lm_err("t2lm_control_field is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	t2lm_control = *(uint16_t *)t2lm_control_field;

	dir = QDF_GET_BITS(t2lm_control, WLAN_T2LM_CONTROL_DIRECTION_IDX,
			   WLAN_T2LM_CONTROL_DIRECTION_BITS);
	if (dir > WLAN_T2LM_BIDI_DIRECTION) {
		t2lm_err("Invalid direction");
		return QDF_STATUS_E_NULL_VALUE;
	}

	t2lm->t2lm_info[dir].direction = dir;
	t2lm->t2lm_info[dir].default_link_mapping =
		QDF_GET_BITS(t2lm_control,
			     WLAN_T2LM_CONTROL_DEFAULT_LINK_MAPPING_IDX,
			     WLAN_T2LM_CONTROL_DEFAULT_LINK_MAPPING_BITS);

	t2lm_debug("direction:%d default_link_mapping:%d",
		   t2lm->t2lm_info[dir].direction,
		   t2lm->t2lm_info[dir].default_link_mapping);

	/* Link mapping of TIDs are not present when default mapping is set */
	if (t2lm->t2lm_info[dir].default_link_mapping)
		return QDF_STATUS_SUCCESS;

	link_mapping_presence_ind = QDF_GET_BITS(
		t2lm_control,
		WLAN_T2LM_CONTROL_LINK_MAPPING_PRESENCE_INDICATOR_IDX,
		WLAN_T2LM_CONTROL_LINK_MAPPING_PRESENCE_INDICATOR_BITS);

	link_mapping_of_tids = t2lm_control_field + sizeof(t2lm_control);

	ret = wlan_mlo_parse_t2lm_provisioned_links(peer,
						    link_mapping_presence_ind,
						    link_mapping_of_tids, t2lm,
						    dir);
	if (ret) {
		t2lm_err("failed to populate provisioned links");
		return ret;
	}

	for (tid_num = 0; tid_num < T2LM_MAX_NUM_TIDS; tid_num++) {
		if (link_mapping_presence_ind & BIT(tid_num))
			t2lm_debug("link mapping of TID%d is %x", tid_num,
				   t2lm->t2lm_info[dir].hw_link_map_tid[tid_num]);
	}

	return ret;
}

QDF_STATUS wlan_mlo_parse_t2lm_ie(
		struct wlan_objmgr_peer *peer,
		struct wlan_t2lm_onging_negotiation_info *t2lm, uint8_t *ie)
{
	struct extn_ie_header *ext_ie_hdr;
	enum wlan_t2lm_direction dir;
	QDF_STATUS retval;

	if (!peer) {
		t2lm_err("null peer");
		return QDF_STATUS_E_NULL_VALUE;
	}

	for (dir = 0; dir < WLAN_T2LM_MAX_DIRECTION; dir++)
		t2lm->t2lm_info[dir].direction = WLAN_T2LM_INVALID_DIRECTION;

	for (dir = 0; dir < WLAN_T2LM_MAX_DIRECTION; dir++) {
		if (!ie) {
			t2lm_err("ie is null");
			return QDF_STATUS_E_NULL_VALUE;
		}

		ext_ie_hdr = (struct extn_ie_header *)ie;

		if (ext_ie_hdr->ie_id == WLAN_ELEMID_EXTN_ELEM &&
		    ext_ie_hdr->ie_extn_id == WLAN_EXTN_ELEMID_T2LM) {
			retval = wlan_mlo_parse_t2lm_info(peer, ie, t2lm);
			if (retval) {
				t2lm_err("Failed to parse the T2LM IE");
				return retval;
			}
			ie += ext_ie_hdr->ie_len + sizeof(struct ie_header);
		}
	}

	if ((t2lm->t2lm_info[WLAN_T2LM_DL_DIRECTION].direction ==
			WLAN_T2LM_DL_DIRECTION ||
	     t2lm->t2lm_info[WLAN_T2LM_UL_DIRECTION].direction ==
			WLAN_T2LM_UL_DIRECTION) &&
	    t2lm->t2lm_info[WLAN_T2LM_BIDI_DIRECTION].direction ==
			WLAN_T2LM_BIDI_DIRECTION) {
		t2lm_err("Both DL/UL and BIDI T2LM IEs should not be present at the same time");

		qdf_mem_zero(t2lm, sizeof(*t2lm));
		for (dir = 0; dir < WLAN_T2LM_MAX_DIRECTION; dir++) {
			t2lm->t2lm_info[dir].direction =
			    WLAN_T2LM_INVALID_DIRECTION;
		}

		return QDF_STATUS_E_FAILURE;
	}

	return QDF_STATUS_SUCCESS;
}

/**
 * wlan_get_ieee_link_id_mask() - Get ieee_link_id mask from hw_link_id mask
 * @wlan_vdev_list: Pointer to vdev list
 * @vdev_count: vdev count
 * @hw_link_id_mask: hw_link_id mask
 *
 * Return: IEEE link ID mask
 */
static uint8_t wlan_get_ieee_link_id_mask(
		struct wlan_objmgr_vdev **wlan_vdev_list,
		uint16_t vdev_count, uint16_t hw_link_id_mask)
{
	struct wlan_objmgr_pdev *pdev;
	struct wlan_objmgr_vdev *vdev;
	uint8_t link_id;
	uint16_t hw_link_id;
	uint16_t ieee_link_id_mask = 0;
	int i;

	for (link_id = 0; link_id < WLAN_T2LM_MAX_NUM_LINKS; link_id++) {
		hw_link_id = hw_link_id_mask & BIT(link_id);
		if (!hw_link_id)
			continue;

		for (i = 0; i < vdev_count; i++) {
			vdev = wlan_vdev_list[i];
			if (!vdev) {
				t2lm_err("vdev is null");
				continue;
			}

			pdev = wlan_vdev_get_pdev(vdev);
			if (!pdev) {
				t2lm_err("pdev is null");
				continue;
			}

			if (hw_link_id != BIT(wlan_mlo_get_pdev_hw_link_id(pdev)))
				continue;

			ieee_link_id_mask |= BIT(vdev->vdev_mlme.mlo_link_id);
			break;
		}
	}

	return ieee_link_id_mask;
}

/**
 * wlan_mlo_add_t2lm_provisioned_links() - API to add provisioned links in the
 * T2LM IE
 * @peer: Pointer to peer structure
 * @link_mapping_presence_indicator: This indicates whether the link mapping of
 *                                   TIDs present in the T2LM IE.
 * @link_mapping_of_tids: Link mapping of each TID
 * @t2lm: Pointer to a T2LM structure
 * @num_tids: Pointer to save num TIDs
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS wlan_mlo_add_t2lm_provisioned_links(
		struct wlan_objmgr_peer *peer,
		uint8_t link_mapping_presence_indicator,
		uint8_t *link_mapping_of_tids, struct wlan_t2lm_info *t2lm,
		uint8_t *num_tids)
{
	struct wlan_objmgr_vdev *vdev;
	struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {0};
	uint16_t vdev_count = 0;
	uint16_t hw_link_id_mask;
	uint16_t ieee_link_id_mask;
	uint8_t tid_num;
	uint16_t i;

	vdev = wlan_peer_get_vdev(peer);
	if (!vdev) {
		t2lm_err("vdev is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	mlo_get_ml_vdev_list(vdev, &vdev_count, wlan_vdev_list);
	if (!vdev_count) {
		t2lm_err("Number of VDEVs under MLD is reported as 0");
		return QDF_STATUS_E_NULL_VALUE;
	}

	for (tid_num = 0; tid_num < T2LM_MAX_NUM_TIDS; tid_num++) {
		ieee_link_id_mask = 0;

		if (t2lm->hw_link_map_tid[tid_num]) {
			hw_link_id_mask = t2lm->hw_link_map_tid[tid_num];
			ieee_link_id_mask = wlan_get_ieee_link_id_mask(
					wlan_vdev_list, vdev_count,
					hw_link_id_mask);
		}

		*(uint16_t *)link_mapping_of_tids = ieee_link_id_mask;
		t2lm_info("link mapping of TID%d is %x", tid_num,
			  ieee_link_id_mask);
		link_mapping_of_tids += sizeof(uint16_t);
		(*num_tids)++;
	}

	for (i = 0; i < vdev_count; i++)
		mlo_release_vdev_ref(wlan_vdev_list[i]);

	return QDF_STATUS_SUCCESS;
}

/**
 * wlan_add_t2lm_info_ie() - Add T2LM IE for UL/DL/Bidirection
 * @frm: Pointer to buffer
 * @peer: Pointer to peer structure
 * @t2lm: Pointer to t2lm mapping structure
 *
 * Return: Updated frame pointer
 */
static uint8_t *wlan_add_t2lm_info_ie(uint8_t *frm,
				      struct wlan_objmgr_peer *peer,
				      struct wlan_t2lm_info *t2lm)
{
	struct wlan_ie_tid_to_link_mapping *t2lm_ie;
	uint16_t t2lm_control = 0;
	uint8_t *t2lm_control_field;
	uint8_t *link_mapping_of_tids;
	uint8_t tid_num;
	uint8_t num_tids = 0;
	uint8_t link_mapping_presence_indicator = 0;
	uint8_t *efrm = frm;
	int ret;

	t2lm_ie = (struct wlan_ie_tid_to_link_mapping *)frm;
	t2lm_ie->elem_id = WLAN_ELEMID_EXTN_ELEM;
	t2lm_ie->elem_id_extn = WLAN_EXTN_ELEMID_T2LM;

	t2lm_ie->elem_len = sizeof(*t2lm_ie) - sizeof(struct ie_header);

	t2lm_control_field = t2lm_ie->data;

	QDF_SET_BITS(t2lm_control, WLAN_T2LM_CONTROL_DIRECTION_IDX,
		     WLAN_T2LM_CONTROL_DIRECTION_BITS, t2lm->direction);

	QDF_SET_BITS(t2lm_control, WLAN_T2LM_CONTROL_DEFAULT_LINK_MAPPING_IDX,
		     WLAN_T2LM_CONTROL_DEFAULT_LINK_MAPPING_BITS,
		     t2lm->default_link_mapping);

	if (t2lm->default_link_mapping) {
		/* Link mapping of TIDs are not present when default mapping is
		 * set. Hence, the size of TID-To-Link mapping control is one
		 * octet.
		 */
		*t2lm_control_field = (uint8_t)t2lm_control;

		t2lm_ie->elem_len += sizeof(uint8_t);

		t2lm_info("T2LM IE added, default_link_mapping: %d dir:%d",
			  t2lm->default_link_mapping, t2lm->direction);

		frm += sizeof(*t2lm_ie) + sizeof(uint8_t);
	} else {
		for (tid_num = 0; tid_num < T2LM_MAX_NUM_TIDS; tid_num++)
			if (t2lm->hw_link_map_tid[tid_num])
				link_mapping_presence_indicator |= BIT(tid_num);

		QDF_SET_BITS(t2lm_control,
			     WLAN_T2LM_CONTROL_LINK_MAPPING_PRESENCE_INDICATOR_IDX,
			     WLAN_T2LM_CONTROL_LINK_MAPPING_PRESENCE_INDICATOR_BITS,
			     link_mapping_presence_indicator);
		t2lm_info("T2LM IE added, direction:%d link_mapping_presence_indicator:%x",
			  t2lm->direction, link_mapping_presence_indicator);

		/* The size of TID-To-Link mapping control is two octets when
		 * default link mapping is not set.
		 */
		*(uint16_t *)t2lm_control_field = htole16(t2lm_control);

		link_mapping_of_tids = t2lm_control_field + sizeof(uint16_t);

		ret = wlan_mlo_add_t2lm_provisioned_links(
			peer, link_mapping_presence_indicator,
			link_mapping_of_tids, t2lm, &num_tids);
		if (ret) {
			t2lm_err("Failed to add t2lm provisioned links");
			return efrm;
		}

		t2lm_ie->elem_len += sizeof(uint16_t) +
				     (num_tids * sizeof(uint16_t));

		frm += sizeof(*t2lm_ie) + sizeof(uint16_t) +
		       (num_tids * sizeof(uint16_t));
	}

	return frm;
}

uint8_t *wlan_mlo_add_t2lm_ie(uint8_t *frm, struct wlan_objmgr_peer *peer,
			      struct wlan_t2lm_onging_negotiation_info *t2lm)
{
	uint8_t dir;

	if (!frm) {
		t2lm_err("frm is null");
		return NULL;
	}

	if (!t2lm) {
		t2lm_err("t2lm is null");
		return NULL;
	}

	/* As per spec, the frame should include one or two T2LM IEs. When it is
	 * two, then direction should DL and UL.
	 */
	if ((t2lm->t2lm_info[WLAN_T2LM_DL_DIRECTION].direction ==
			WLAN_T2LM_DL_DIRECTION ||
	     t2lm->t2lm_info[WLAN_T2LM_UL_DIRECTION].direction ==
			WLAN_T2LM_UL_DIRECTION) &&
	    t2lm->t2lm_info[WLAN_T2LM_BIDI_DIRECTION].direction ==
			WLAN_T2LM_BIDI_DIRECTION) {
		t2lm_err("Both DL/UL and BIDI T2LM IEs should not be present at the same time");
		return NULL;
	}

	for (dir = 0; dir < WLAN_T2LM_MAX_DIRECTION; dir++) {
		if (t2lm->t2lm_info[dir].direction !=
			WLAN_T2LM_INVALID_DIRECTION)
			frm = wlan_add_t2lm_info_ie(frm, peer,
						    &t2lm->t2lm_info[dir]);
	}

	return frm;
}

/**
 * wlan_mlo_parse_t2lm_request_action_frame() - API to parse T2LM request action
 * frame.
 * @peer: Pointer to peer structure
 * @t2lm: Pointer to T2LM structure
 * @action_frm: Pointer to action frame
 * @category: T2LM action frame category
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS wlan_mlo_parse_t2lm_request_action_frame(
		struct wlan_objmgr_peer *peer,
		struct wlan_t2lm_onging_negotiation_info *t2lm,
		struct wlan_action_frame *action_frm,
		enum wlan_t2lm_category category)
{
	uint8_t *t2lm_action_frm;

	t2lm->category = category;

	/*
	 * T2LM request action frame
	 *
	 *   1-byte     1-byte     1-byte   variable
	 *-------------------------------------------
	 * |         |           |        |         |
	 * | Category| Protected | Dialog | T2LM IE |
	 * |         |    EHT    | token  |         |
	 * |         |  Action   |        |         |
	 *-------------------------------------------
	 */

	t2lm_action_frm = (uint8_t *)action_frm + sizeof(*action_frm);

	t2lm->dialog_token = *t2lm_action_frm;

	return wlan_mlo_parse_t2lm_ie(peer, t2lm,
				      t2lm_action_frm + sizeof(uint8_t));
}

/**
 * wlan_mlo_parse_t2lm_response_action_frame() - API to parse T2LM response
 * action frame.
 * @peer: Pointer to peer structure
 * @t2lm: Pointer to T2LM structure
 * @action_frm: Pointer to action frame
 * @category: T2LM action frame category
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS wlan_mlo_parse_t2lm_response_action_frame(
		struct wlan_objmgr_peer *peer,
		struct wlan_t2lm_onging_negotiation_info *t2lm,
		struct wlan_action_frame *action_frm,
		enum wlan_t2lm_category category)
{
	uint8_t *t2lm_action_frm;
	QDF_STATUS ret_val = QDF_STATUS_SUCCESS;

	t2lm->category = WLAN_T2LM_CATEGORY_RESPONSE;
	/*
	 * T2LM response action frame
	 *
	 *   1-byte     1-byte     1-byte   1-byte   variable
	 *----------------------------------------------------
	 * |         |           |        |        |         |
	 * | Category| Protected | Dialog | Status | T2LM IE |
	 * |         |    EHT    | token  |  code  |         |
	 * |         |  Action   |        |        |         |
	 *----------------------------------------------------
	 */

	t2lm_action_frm = (uint8_t *)action_frm + sizeof(*action_frm);

	t2lm->dialog_token = *t2lm_action_frm;
	t2lm->t2lm_resp_type = *(t2lm_action_frm + sizeof(uint8_t));

	if (t2lm->t2lm_resp_type ==
			WLAN_T2LM_RESP_TYPE_PREFERRED_TID_TO_LINK_MAPPING) {
		t2lm_action_frm += sizeof(uint8_t) + sizeof(uint8_t);
		ret_val = wlan_mlo_parse_t2lm_ie(peer, t2lm, t2lm_action_frm);
	}

	return ret_val;
}

int wlan_mlo_parse_t2lm_action_frame(
		struct wlan_objmgr_peer *peer,
		struct wlan_t2lm_onging_negotiation_info *t2lm,
		struct wlan_action_frame *action_frm,
		enum wlan_t2lm_category category)
{
	QDF_STATUS ret_val = QDF_STATUS_SUCCESS;

	switch (category) {
	case WLAN_T2LM_CATEGORY_REQUEST:
		{
			ret_val = wlan_mlo_parse_t2lm_request_action_frame(
					peer, t2lm, action_frm, category);
			return qdf_status_to_os_return(ret_val);
		}
	case WLAN_T2LM_CATEGORY_RESPONSE:
		{
			ret_val = wlan_mlo_parse_t2lm_response_action_frame(
					peer, t2lm, action_frm, category);

			return qdf_status_to_os_return(ret_val);
		}
	case WLAN_T2LM_CATEGORY_TEARDOWN:
			/* Nothing to parse from T2LM teardown frame, just reset
			 * the mapping to default mapping.
			 *
			 * T2LM teardown action frame
			 *
			 *   1-byte     1-byte
			 *------------------------
			 * |         |           |
			 * | Category| Protected |
			 * |         |    EHT    |
			 * |         |  Action   |
			 *------------------------
			 */
			break;
	default:
			t2lm_err("Invalid category:%d", category);
	}

	return ret_val;
}

static uint8_t *wlan_mlo_add_t2lm_request_action_frame(
		struct wlan_objmgr_peer *peer, uint8_t *frm,
		struct wlan_action_frame_args *args, uint8_t *buf,
		enum wlan_t2lm_category category)
{
	*frm++ = args->category;
	*frm++ = args->action;
	/* Dialog token*/
	*frm++ = args->arg1;

	t2lm_info("T2LM request frame: category:%d action:%d dialog_token:%d",
		  args->category, args->action, args->arg1);
	return wlan_mlo_add_t2lm_ie(frm, peer, (void *)buf);
}

static uint8_t *wlan_mlo_add_t2lm_response_action_frame(
		struct wlan_objmgr_peer *peer, uint8_t *frm,
		struct wlan_action_frame_args *args, uint8_t *buf,
		enum wlan_t2lm_category category)
{
	*frm++ = args->category;
	*frm++ = args->action;
	/* Dialog token*/
	*frm++ = args->arg1;
	/* Status code */
	*frm++ = args->arg2;

	t2lm_info("T2LM response frame: category:%d action:%d dialog_token:%d status_code:%d",
		  args->category, args->action, args->arg1, args->arg2);

	if (args->arg2 == WLAN_T2LM_RESP_TYPE_PREFERRED_TID_TO_LINK_MAPPING)
		frm = wlan_mlo_add_t2lm_ie(frm, peer, (void *)buf);

	return frm;
}

uint8_t *wlan_mlo_add_t2lm_action_frame(
		struct wlan_objmgr_peer *peer, uint8_t *frm,
		struct wlan_action_frame_args *args, uint8_t *buf,
		enum wlan_t2lm_category category)
{
	if (!peer) {
		t2lm_err("null peer");
		return NULL;
	}

	switch (category) {
	case WLAN_T2LM_CATEGORY_REQUEST:
		return wlan_mlo_add_t2lm_request_action_frame(peer, frm, args,
							      buf, category);
	case WLAN_T2LM_CATEGORY_RESPONSE:
		return wlan_mlo_add_t2lm_response_action_frame(peer, frm, args,
							      buf, category);
	case WLAN_T2LM_CATEGORY_TEARDOWN:
		*frm++ = args->category;
		*frm++ = args->action;
		return frm;
	default:
		t2lm_err("Invalid category:%d", category);
	}

	return frm;
}
