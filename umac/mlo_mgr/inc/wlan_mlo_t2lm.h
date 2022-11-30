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

#ifndef _WLAN_MLO_T2LM_H_
#define _WLAN_MLO_T2LM_H_

#include <wlan_cmn_ieee80211.h>

#ifdef WLAN_FEATURE_11BE

#define t2lm_alert(format, args...) \
	QDF_TRACE_FATAL(QDF_MODULE_ID_T2LM, format, ## args)

#define t2lm_err(format, args...) \
	QDF_TRACE_ERROR(QDF_MODULE_ID_T2LM, format, ## args)

#define t2lm_warn(format, args...) \
	QDF_TRACE_WARN(QDF_MODULE_ID_T2LM, format, ## args)

#define t2lm_info(format, args...) \
	QDF_TRACE_INFO(QDF_MODULE_ID_T2LM, format, ## args)

#define t2lm_debug(format, args...) \
	QDF_TRACE_DEBUG(QDF_MODULE_ID_T2LM, format, ## args)

#define WLAN_T2LM_MAX_NUM_LINKS 16

/**
 * wlan_mlo_parse_t2lm_ie() - API to parse the T2LM IE
 * @peer: Pointer to peer structure
 * @t2lm: Pointer to T2LM structure
 * @ie: Pointer to T2LM IE
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_mlo_parse_t2lm_ie(
	struct wlan_objmgr_peer *peer,
	struct wlan_t2lm_onging_negotiation_info *t2lm, uint8_t *ie);

/**
 * wlan_mlo_add_t2lm_ie() - API to add TID-to-link mapping IE
 * @frm: Pointer to buffer
 * @peer: Pointer to peer structure
 * @t2lm: Pointer to t2lm mapping structure
 *
 * Return: Updated frame pointer
 */
uint8_t *wlan_mlo_add_t2lm_ie(uint8_t *frm, struct wlan_objmgr_peer *peer,
			      struct wlan_t2lm_onging_negotiation_info *t2lm);

/**
 * wlan_mlo_parse_t2lm_action_frame() - API to parse T2LM action frame
 * @peer: Pointer to peer structure
 * @t2lm: Pointer to T2LM structure
 * @action_frm: Pointer to action frame
 * @category: T2LM action frame category
 *
 * Return: 0 - success, else failure
 */
int wlan_mlo_parse_t2lm_action_frame(
		struct wlan_objmgr_peer *peer,
		struct wlan_t2lm_onging_negotiation_info *t2lm,
		struct wlan_action_frame *action_frm,
		enum wlan_t2lm_category category);

/**
 * wlan_mlo_add_t2lm_action_frame() - API to add T2LM action frame
 * @peer: Pointer to peer structure
 * @frm: Pointer to a frame to add T2LM IE
 * @args: T2LM action frame related info
 * @buf: Pointer to T2LM IE values
 * @category: T2LM action frame category
 *
 * Return: Pointer to the updated frame buffer
 */
uint8_t *wlan_mlo_add_t2lm_action_frame(
		struct wlan_objmgr_peer *peer,
		uint8_t *frm, struct wlan_action_frame_args *args,
		uint8_t *buf, enum wlan_t2lm_category category);
#else
static inline QDF_STATUS wlan_mlo_parse_t2lm_ie(
	struct wlan_objmgr_peer *peer,
	struct wlan_t2lm_onging_negotiation_info *t2lm, uint8_t *ie)
{
	return QDF_STATUS_E_FAILURE;
}

static inline
int8_t *wlan_mlo_add_t2lm_ie(uint8_t *frm, struct wlan_objmgr_peer *peer,
			     struct wlan_t2lm_onging_negotiation_info *t2lm)
{
	return frm;
}

static inline
int wlan_mlo_parse_t2lm_action_frame(
		struct wlan_objmgr_peer *peer,
		struct wlan_t2lm_onging_negotiation_info *t2lm,
		struct wlan_action_frame *action_frm,
		enum wlan_t2lm_category category)
{
	return 0;
}

static inline
uint8_t *wlan_mlo_add_t2lm_action_frame(
		struct wlan_objmgr_peer *peer,
		uint8_t *frm, struct wlan_action_frame_args *args,
		uint8_t *buf, enum wlan_t2lm_category category)
{
	return frm;
}
#endif /* WLAN_FEATURE_11BE */
#endif /* _WLAN_MLO_T2LM_H_ */
