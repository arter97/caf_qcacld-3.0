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

#define t2lm_rl_debug(format, args...) \
	QDF_TRACE_DEBUG_RL(QDF_MODULE_ID_T2LM, format, ## args)

#define WLAN_T2LM_MAX_NUM_LINKS 16

#ifdef WLAN_MLO_USE_SPINLOCK
/**
 * t2lm_dev_lock_create - Create T2LM device mutex/spinlock
 * @t2lm_ctx: T2LM context
 *
 * Creates mutex/spinlock
 *
 * Return: void
 */
static inline void
t2lm_dev_lock_create(struct wlan_t2lm_context *t2lm_ctx)
{
	qdf_spinlock_create(&t2lm_ctx->t2lm_dev_lock);
}

/**
 * t2lm_dev_lock_destroy - Destroy T2LM mutex/spinlock
 * @t2lm_ctx: T2LM context
 *
 * Destroy mutex/spinlock
 *
 * Return: void
 */
static inline void
t2lm_dev_lock_destroy(struct wlan_t2lm_context *t2lm_ctx)
{
	qdf_spinlock_destroy(&t2lm_ctx->t2lm_dev_lock);
}

/**
 * t2lm_dev_lock_acquire - acquire T2LM mutex/spinlock
 * @t2lm_ctx: T2LM context
 *
 * acquire mutex/spinlock
 *
 * return: void
 */
static inline
void t2lm_dev_lock_acquire(struct wlan_t2lm_context *t2lm_ctx)
{
	qdf_spin_lock_bh(&t2lm_ctx->t2lm_dev_lock);
}

/**
 * t2lm_dev_lock_release - release T2LM dev mutex/spinlock
 * @t2lm_ctx: T2LM context
 *
 * release mutex/spinlock
 *
 * return: void
 */
static inline
void t2lm_dev_lock_release(struct wlan_t2lm_context *t2lm_ctx)
{
	qdf_spin_unlock_bh(&t2lm_ctx->t2lm_dev_lock);
}
#else /* WLAN_MLO_USE_SPINLOCK */
static inline
void t2lm_dev_lock_create(struct wlan_t2lm_context *t2lm_ctx)
{
	qdf_mutex_create(&t2lm_ctx->t2lm_dev_lock);
}

static inline
void t2lm_dev_lock_destroy(struct wlan_t2lm_context *t2lm_ctx)
{
	qdf_mutex_destroy(&t2lm_ctx->t2lm_dev_lock);
}

static inline void t2lm_dev_lock_acquire(struct wlan_t2lm_context *t2lm_ctx)
{
	qdf_mutex_acquire(&t2lm_ctx->t2lm_dev_lock);
}

static inline void t2lm_dev_lock_release(struct wlan_t2lm_context *t2lm_ctx)
{
	qdf_mutex_release(&t2lm_ctx->t2lm_dev_lock);
}
#endif

/**
 * wlan_mlo_parse_t2lm_ie() - API to parse the T2LM IE
 * @t2lm: Pointer to T2LM structure
 * @ie: Pointer to T2LM IE
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_mlo_parse_t2lm_ie(
	struct wlan_t2lm_onging_negotiation_info *t2lm, uint8_t *ie);

/**
 * wlan_mlo_add_t2lm_ie() - API to add TID-to-link mapping IE
 * @frm: Pointer to buffer
 * @t2lm: Pointer to t2lm mapping structure
 *
 * Return: Updated frame pointer
 */
uint8_t *wlan_mlo_add_t2lm_ie(uint8_t *frm,
			      struct wlan_t2lm_onging_negotiation_info *t2lm);

/**
 * wlan_mlo_vdev_tid_to_link_map_event() - API to process the revceived T2LM
 * event.
 * @psoc: psoc object
 * @event: Pointer to received T2LM info
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_mlo_vdev_tid_to_link_map_event(
			struct wlan_objmgr_psoc *psoc,
			struct mlo_vdev_host_tid_to_link_map_resp *event);

/**
 * wlan_mlo_parse_t2lm_action_frame() - API to parse T2LM action frame
 * @t2lm: Pointer to T2LM structure
 * @action_frm: Pointer to action frame
 * @category: T2LM action frame category
 *
 * Return: 0 - success, else failure
 */
int wlan_mlo_parse_t2lm_action_frame(
		struct wlan_t2lm_onging_negotiation_info *t2lm,
		struct wlan_action_frame *action_frm,
		enum wlan_t2lm_category category);

/**
 * wlan_mlo_add_t2lm_action_frame() - API to add T2LM action frame
 * @frm: Pointer to a frame to add T2LM IE
 * @args: T2LM action frame related info
 * @buf: Pointer to T2LM IE values
 * @category: T2LM action frame category
 *
 * Return: Pointer to the updated frame buffer
 */
uint8_t *wlan_mlo_add_t2lm_action_frame(
		uint8_t *frm, struct wlan_action_frame_args *args,
		uint8_t *buf, enum wlan_t2lm_category category);

/**
 * wlan_mlo_parse_bcn_prbresp_t2lm_ie() - API to parse the T2LM IE from beacon/
 * probe response frame
 * @t2lm_ctx: T2LM context
 * @ie: Pointer to T2LM IE
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_mlo_parse_bcn_prbresp_t2lm_ie(
		struct wlan_t2lm_context *t2lm_ctx, uint8_t *ie);

/**
 * wlan_mlo_add_t2lm_info_ie() - Add T2LM IE for UL/DL/Bidirection
 * @frm: Pointer to buffer
 * @t2lm: Pointer to t2lm mapping structure
 *
 * Return: Updated frame pointer
 */
uint8_t *wlan_mlo_add_t2lm_info_ie(uint8_t *frm, struct wlan_t2lm_info *t2lm);

/**
 * wlan_mlo_t2lm_timer_init() - API to add TID-to-link mapping IE
 * @vdev: Pointer to vdev
 *
 * Return: qdf status
 */
QDF_STATUS
wlan_mlo_t2lm_timer_init(struct wlan_objmgr_vdev *vdev);

/**
 * wlan_mlo_t2lm_timer_start() - API to start T2LM timer
 * @vdev: Pointer to vdev
 * @interval: T2LM timer interval
 * @t2lm_ie_index: T2LM IE index
 *
 * Return: qdf status
 */
QDF_STATUS
wlan_mlo_t2lm_timer_start(struct wlan_objmgr_vdev *vdev,
			  uint32_t interval, uint8_t t2lm_ie_index);

/**
 * wlan_mlo_t2lm_timer_stop() - API to stop TID-to-link mapping timer
 * @vdev: Pointer to vdev
 *
 * Return: qdf status
 */
QDF_STATUS
wlan_mlo_t2lm_timer_stop(struct wlan_objmgr_vdev *vdev);

/**
 * wlan_mlo_t2lm_timer_expiry_handler() - API to handle t2lm timer expiry
 * @vdev: Pointer to vdev structure
 *
 * Return: none
 */
void
wlan_mlo_t2lm_timer_expiry_handler(void *vdev);

/**
 * wlan_handle_t2lm_timer() - API to handle TID-to-link mapping timer
 * @vdev: Pointer to vdev
 * @ie_idx: ie index value
 *
 * Return: qdf status
 */
QDF_STATUS
wlan_handle_t2lm_timer(struct wlan_objmgr_vdev *vdev, uint8_t ie_idx);

/**
 * wlan_process_bcn_prbrsp_t2lm_ie() - API to process the received T2LM IE from
 * beacon/probe response.
 * @vdev: Pointer to vdev
 * @rx_t2lm_ie: Received T2LM IE
 * @tsf: Local TSF value
 *
 * Return QDF_STATUS
 */
QDF_STATUS wlan_process_bcn_prbrsp_t2lm_ie(struct wlan_objmgr_vdev *vdev,
					   struct wlan_t2lm_context *rx_t2lm_ie,
					   uint64_t tsf);

#else
static inline QDF_STATUS wlan_mlo_parse_t2lm_ie(
	struct wlan_t2lm_onging_negotiation_info *t2lm, uint8_t *ie)
{
	return QDF_STATUS_E_FAILURE;
}

static inline
int8_t *wlan_mlo_add_t2lm_ie(uint8_t *frm,
			     struct wlan_t2lm_onging_negotiation_info *t2lm)
{
	return frm;
}

static inline
int wlan_mlo_parse_t2lm_action_frame(
		struct wlan_t2lm_onging_negotiation_info *t2lm,
		struct wlan_action_frame *action_frm,
		enum wlan_t2lm_category category)
{
	return 0;
}

static inline
uint8_t *wlan_mlo_add_t2lm_action_frame(
		uint8_t *frm, struct wlan_action_frame_args *args,
		uint8_t *buf, enum wlan_t2lm_category category)
{
	return frm;
}

static inline
QDF_STATUS wlan_mlo_parse_bcn_prbresp_t2lm_ie(
		struct wlan_t2lm_context *t2lm_ctx, uint8_t *ie)
{
	return QDF_STATUS_E_FAILURE;
}

static inline
uint8_t *wlan_mlo_add_t2lm_info_ie(uint8_t *frm, struct wlan_t2lm_info *t2lm)
{
	return frm;
}

static inline QDF_STATUS
wlan_mlo_t2lm_timer_init(struct wlan_objmgr_vdev *vdev)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline QDF_STATUS
wlan_mlo_t2lm_timer_start(struct wlan_objmgr_vdev *vdev,
			  uint32_t interval, uint8_t t2lm_ie_index)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline QDF_STATUS
wlan_mlo_t2lm_timer_stop(struct wlan_objmgr_vdev *vdev)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline void
wlan_mlo_t2lm_timer_expiry_handler(void *vdev)
{}

static inline QDF_STATUS
wlan_handle_t2lm_timer(struct wlan_objmgr_vdev *vdev, uint8_t ie_idx)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline QDF_STATUS
wlan_process_bcn_prbrsp_t2lm_ie(struct wlan_objmgr_vdev *vdev,
				struct wlan_t2lm_context *rx_t2lm_ie,
				uint64_t tsf)
{
	return QDF_STATUS_SUCCESS;
}
#endif /* WLAN_FEATURE_11BE */
#endif /* _WLAN_MLO_T2LM_H_ */
