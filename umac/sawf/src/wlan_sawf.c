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
 * DOC: wlan_sawf.c
 * This file defines functions needed by the SAWF framework.
 */

#include "wlan_sawf.h"

static struct sawf_ctx *g_wlan_sawf_ctx;

QDF_STATUS wlan_sawf_init(void)
{
	if (g_wlan_sawf_ctx) {
		qdf_err("SAWF global context is already allocated");
		return QDF_STATUS_E_FAILURE;
	}

	g_wlan_sawf_ctx = qdf_mem_malloc(sizeof(struct sawf_ctx));
	if (!g_wlan_sawf_ctx) {
		qdf_err("Mem alloc failed for SAWF context");
		return QDF_STATUS_E_FAILURE;
	}

	qdf_info("SAWF: SAWF ctx is initialized");
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_sawf_deinit(void)
{
	if (!g_wlan_sawf_ctx) {
		qdf_err("SAWF gloabl context is already freed");
		return QDF_STATUS_E_FAILURE;
	}

	qdf_mem_free(g_wlan_sawf_ctx);

	return QDF_STATUS_SUCCESS;
}

struct sawf_ctx *wlan_get_sawf_ctx(void)
{
	return g_wlan_sawf_ctx;
}

void wlan_print_service_class(struct wlan_sawf_scv_class_params *params)
{
	qdf_info("Service ID       :%d", params->svc_id);
	qdf_info("App Name         :%s", params->app_name);
	qdf_info("Min througput    :%d", params->min_thruput_rate);
	qdf_info("Max throughput   :%d", params->max_thruput_rate);
	qdf_info("Burst Size       :%d", params->burst_size);
	qdf_info("Service Interval :%d", params->service_interval);
	qdf_info("Delay Bound      :%d", params->delay_bound);
	qdf_info("MSDU TTL         :%d", params->msdu_ttl);
	qdf_info("Priority         :%d", params->priority);
	qdf_info("TID              :%d", params->tid);
	qdf_info("MSDU Loss Rate   :%d", params->msdu_rate_loss);
}

bool wlan_service_id_valid(uint8_t svc_id)
{
	if (svc_id <  SAWF_SVC_CLASS_MIN || svc_id > SAWF_SVC_CLASS_MAX)
		return false;
	else
		return true;
}

bool wlan_service_id_configured(uint8_t svc_id)
{
	struct sawf_ctx *sawf;

	sawf = wlan_get_sawf_ctx();
	if (!sawf) {
		qdf_err("SAWF ctx is invalid");
		return false;
	}

	if (!(sawf->svc_classes[svc_id].configured))
		return false;

	return true;
}

