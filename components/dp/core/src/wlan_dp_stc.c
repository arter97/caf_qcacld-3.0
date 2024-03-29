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
  * DOC: wlan_dp_stc.c
  *
  *
  */

#include "wlan_dp_stc.h"

#ifdef WLAN_DP_FEATURE_STC

/* API to be called from RX Flow table entry add */
/* API to be called from TX Flow table entry add */

static void wlan_dp_stc_flow_monitor_work_handler(void *arg)
{
	/*
	 * 1) Monitor TX/RX flows
	 * 2) Add long flow to Sampling flow table
	 * 3) Flow inactivity detection
	 * 4) Ping inactivity detection
	 */
}

static void wlan_dp_stc_flow_sampling_timer(void *arg)
{
	/*
	 * 1) Walk over the Sampling flow table
	 * 2) Get stats for the flows in Sampling flow table
	 * 3) Get delta stats against the last cached value.
	 * 4) Store the sample in Sampling flow table, against the flow entry
	 * 5) Mark Sampling complete
	 */
}

QDF_STATUS wlan_dp_stc_attach(struct wlan_dp_psoc_context *dp_ctx)
{
	struct wlan_dp_stc *dp_stc;
	QDF_STATUS status;

	dp_info("STC: attach");
	dp_stc = qdf_mem_malloc(sizeof(*dp_stc));
	if (!dp_stc) {
		dp_err("STC context alloc failed");
		return QDF_STATUS_E_NOMEM;
	}

	dp_ctx->dp_stc = dp_stc;
	dp_stc->dp_ctx = dp_ctx;

	/* Init periodic work */
	status = qdf_periodic_work_create(&dp_stc->flow_monitor_work,
					  wlan_dp_stc_flow_monitor_work_handler,
					  dp_stc);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_info("STC flow monitor work creation failed");
		goto periodic_work_creation_fail;
	}

	dp_stc->flow_monitor_interval = 100;
	dp_stc->periodic_work_state = WLAN_DP_STC_WORK_INIT;

	/* Init timer */
	status = qdf_timer_init(dp_ctx->qdf_dev, &dp_stc->flow_sampling_timer,
				wlan_dp_stc_flow_sampling_timer, dp_stc,
				QDF_TIMER_TYPE_SW);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_info("STC flow sampling timer init failed");
		goto timer_init_fail;
	}

	dp_stc->sample_timer_state = WLAN_DP_STC_TIMER_INIT;
	dp_fisa_rx_add_tcp_flow_to_fst(dp_ctx);

	return QDF_STATUS_SUCCESS;

timer_init_fail:
	qdf_periodic_work_destroy(&dp_stc->flow_monitor_work);

periodic_work_creation_fail:
	qdf_mem_free(dp_stc);

	return status;
}

QDF_STATUS wlan_dp_stc_detach(struct wlan_dp_psoc_context *dp_ctx)
{
	struct wlan_dp_stc *dp_stc = dp_ctx->dp_stc;

	dp_info("STC: detach");
	qdf_timer_sync_cancel(&dp_stc->flow_sampling_timer);
	qdf_periodic_work_destroy(&dp_stc->flow_monitor_work);
	qdf_mem_free(dp_ctx->dp_stc);

	return QDF_STATUS_SUCCESS;
}
#endif
