/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * DOC: contains EPCS APIs
 */

#ifndef _WLAN_MLO_EPCS_H_
#define _WLAN_MLO_EPCS_H_

#include <wlan_cmn_ieee80211.h>
#include <wlan_mlo_mgr_public_structs.h>
#ifdef WMI_AP_SUPPORT
#include <wlan_cmn.h>
#endif

struct wlan_mlo_peer_context;

/**
 * enum wlan_epcs_category - epcs category
 *
 * @WLAN_EPCS_CATEGORY_NONE: none
 * @WLAN_EPCS_CATEGORY_REQUEST: EPCS request
 * @WLAN_EPCS_CATEGORY_RESPONSE: EPCS response
 * @WLAN_EPCS_CATEGORY_TEARDOWN: EPCS teardown
 * @WLAN_EPCS_CATEGORY_INVALID: Invalid
 */
enum wlan_epcs_category {
	WLAN_EPCS_CATEGORY_NONE = 0,
	WLAN_EPCS_CATEGORY_REQUEST = 3,
	WLAN_EPCS_CATEGORY_RESPONSE = 4,
	WLAN_EPCS_CATEGORY_TEARDOWN = 5,
	WLAN_EPCS_CATEGORY_INVALID,
};

/**
 * struct wlan_epcs_info - EPCS information of frame
 * @cat: frame category
 * @dialog_token: dialog token
 * @status: status
 */
struct wlan_epcs_info {
	enum wlan_epcs_category cat;
	uint8_t dialog_token;
	uint16_t status;
};

/**
 * enum peer_epcs_state - epcs stat of peer
 * @EPCS_DOWN: EPCS state down
 * @EPCS_ENABLE: EPCS state enabled
 */
enum peer_epcs_state {
	EPCS_DOWN,
	EPCS_ENABLE
};

/**
 * struct wlan_mlo_peer_epcs_info - Peer EPCS information
 * @state: EPCS state of peer
 * @self_gen_dialog_token: selfgenerated dialog token
 */
struct wlan_mlo_peer_epcs_info {
	enum peer_epcs_state state;
	uint8_t self_gen_dialog_token;
};

/**
 * struct wlan_epcs_context - EPCS context if MLD
 */
struct wlan_epcs_context {
};

/**
 * struct epcs_frm - EPCS action frame format
 * @category: category
 * @protected_eht_action: Protected EHT Action
 * @dialog_token: Dialog Token
 * @status_code: Status Code
 * @req: Request frame
 * @resp: Response frame
 * @bytes: Priority Access Multi-Link element bytes
 */
struct epcs_frm {
	uint8_t category;
	uint8_t protected_eht_action;
	uint8_t dialog_token;
	union {
		struct {
			uint8_t bytes[0];
		} req;
		struct {
			uint8_t status_code[2];
			uint8_t bytes[0];
		} resp;
	};
};

#define epcs_alert(format, args...) \
		QDF_TRACE_FATAL(QDF_MODULE_ID_EPCS, format, ## args)

#define epcs_err(format, args...) \
		QDF_TRACE_ERROR(QDF_MODULE_ID_EPCS, format, ## args)

#define epcs_warn(format, args...) \
		QDF_TRACE_WARN(QDF_MODULE_ID_EPCS, format, ## args)

#define epcs_info(format, args...) \
		QDF_TRACE_INFO(QDF_MODULE_ID_EPCS, format, ## args)

#define epcs_debug(format, args...) \
		QDF_TRACE_DEBUG(QDF_MODULE_ID_EPCS, format, ## args)

#define epcs_rl_debug(format, args...) \
		QDF_TRACE_DEBUG_RL(QDF_MODULE_ID_EPCS, format, ## args)

/**
 * wlan_mlo_add_epcs_action_frame() - API to add EPCS action frame
 * @frm: Pointer to a frame to add EPCS information
 * @args: EPCS action frame related info
 * @buf: Pointer to EPCS IE values
 *
 * Return: Pointer to the updated frame buffer
 */
uint8_t *wlan_mlo_add_epcs_action_frame(uint8_t *frm,
					struct wlan_action_frame_args *args,
					uint8_t *buf);

/**
 * wlan_mlo_parse_epcs_action_frame() - API to parse EPCS action frame
 * @epcs: Pointer to EPCS information
 * @action_frm: EPCS action frame
 * @frm_len: frame length
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
wlan_mlo_parse_epcs_action_frame(struct wlan_epcs_info *epcs,
				 struct wlan_action_frame *action_frm,
				 uint32_t frm_len);

/**
 * wlan_mlo_peer_rcv_cmd() - API to process EPCS command
 * @ml_peer: Pointer to ML peer received
 * @epcs: Pointer to EPCS information
 * @updparam: pointer to fill update parameters
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
wlan_mlo_peer_rcv_cmd(struct wlan_mlo_peer_context *ml_peer,
		      struct wlan_epcs_info *epcs,
		      bool *updparam);

/**
 * wlan_mlo_peer_rcv_action_frame() - API to process EPCS frame receive event
 * @ml_peer: Pointer to ML peer received
 * @epcs: Pointer to EPCS information
 * @respond: pointer to fill response required or not
 * @updparam: pointer to fill update parameters
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
wlan_mlo_peer_rcv_action_frame(struct wlan_mlo_peer_context *ml_peer,
			       struct wlan_epcs_info *epcs,
			       bool *respond,
			       bool *updparam);

#endif /* _WLAN_MLO_EPCS_H_ */
