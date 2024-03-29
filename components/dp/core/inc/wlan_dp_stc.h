/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 /**
  * DOC: wlan_dp_stc.h
  *
  *
  */
#ifndef __WLAN_DP_STC_H__
#define __WLAN_DP_STC_H__

#include "wlan_dp_main.h"

/**
 * enum wlan_dp_stc_periodic_work_state - Periodic work states
 * @WLAN_DP_STC_WORK_INIT: INIT state
 * @WLAN_DP_STC_WORK_STOPPED: Stopped state
 * @WLAN_DP_STC_WORK_STARTED: Started state
 * @WLAN_DP_STC_WORK_RUNNING: Running state
 */
enum wlan_dp_stc_periodic_work_state {
	WLAN_DP_STC_WORK_INIT,
	WLAN_DP_STC_WORK_STOPPED,
	WLAN_DP_STC_WORK_STARTED,
	WLAN_DP_STC_WORK_RUNNING,
};

/**
 * struct wlan_dp_stc - Smart traffic classifier context
 * @dp_ctx: DP component global context
 * @flow_monitor_work: periodic work to process all the misc work for STC
 * @flow_monitor_interval: periodic flow monitor work interval
 * @periodic_work_state: States of the periodic flow monitor work
 * @flow_sampling_timer: timer to sample all the short-listed flows
 */
struct wlan_dp_stc {
	struct wlan_dp_psoc_context *dp_ctx;
	struct qdf_periodic_work flow_monitor_work;
	uint32_t flow_monitor_interval;
	enum wlan_dp_stc_periodic_work_state periodic_work_state;
	qdf_timer_t flow_sampling_timer;
};

/* Function Declaration - START */

#ifdef WLAN_DP_FEATURE_STC
/**
 * wlan_dp_stc_attach() - STC attach
 * @dp_ctx: DP global psoc context
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_dp_stc_attach(struct wlan_dp_psoc_context *dp_ctx);

/**
 * wlan_dp_stc_detach() - STC detach
 * @dp_ctx: DP global psoc context
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_dp_stc_detach(struct wlan_dp_psoc_context *dp_ctx);
#else
static inline
QDF_STATUS wlan_dp_stc_attach(struct wlan_dp_psoc_context *dp_ctx)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS wlan_dp_stc_detach(struct wlan_dp_psoc_context *dp_ctx)
{
	return QDF_STATUS_SUCCESS;
}
#endif
/* Function Declaration - END */
#endif
