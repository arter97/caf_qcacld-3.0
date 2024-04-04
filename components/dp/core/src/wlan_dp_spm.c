/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

#include <dp_types.h>
#include <dp_internal.h>
#include "wlan_dp_spm.h"
#include "dp_htt.h"

#define DP_SPM_FLOW_FLAG_IN_USE         BIT(0)
#define DP_SPM_FLOW_FLAG_SVC_METADATA   BIT(1)

#ifdef WLAN_FEATURE_SAWFISH
/**
 * wlan_dp_spm_get_context(): Get SPM context from DP PSOC interface
 *
 * Return: SPM context handle
 */
static inline
struct wlan_dp_spm_context *wlan_dp_spm_get_context(void)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_get_context();

	return dp_ctx ? dp_ctx->spm_ctx : NULL;
}

QDF_STATUS wlan_dp_spm_ctx_init(struct wlan_dp_psoc_context *dp_ctx,
				uint32_t queues_per_tid, uint32_t num_tids)
{
	QDF_STATUS status;
	struct wlan_dp_spm_context *ctx;

	if (dp_ctx->spm_ctx) {
		dp_err("SPM module already initialized");
		return QDF_STATUS_E_ALREADY;
	}

	ctx = (struct wlan_dp_spm_context *)qdf_mem_malloc(sizeof(*ctx));
	if (!ctx) {
		dp_err("Unable to allocate spm ctx");
		goto fail;
	}

	/* Move this below creation of DB to WMI ready msg or new HTT msg */
	ctx->max_supported_svc_class = queues_per_tid * num_tids;

	if (ctx->max_supported_svc_class >
		WLAN_DP_SPM_MAX_SERVICE_CLASS_SUPPORT) {
		dp_err("Wrong config recvd queues per tid: %u, num tids: %u",
		       queues_per_tid, num_tids);
		ctx->max_supported_svc_class =
					WLAN_DP_SPM_MAX_SERVICE_CLASS_SUPPORT;
	}

	ctx->svc_class_db = (struct wlan_dp_spm_svc_class **)
			qdf_mem_malloc(sizeof(struct wlan_dp_spm_svc_class *) *
				       queues_per_tid * num_tids);
	if (!ctx->svc_class_db) {
		dp_err("Unable to allocate SPM service class database");
		goto fail_svc_db_alloc;
	}

	qdf_list_create(&ctx->evt_list, 0);

	status = qdf_create_work(NULL, &ctx->spm_work,
				 wlan_dp_spm_work_handler, dp_ctx);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("Work creation failed");
		goto fail_work_creation;
	}

	ctx->spm_wq = qdf_alloc_unbound_workqueue("spm_wq");
	if (!ctx->spm_wq) {
		dp_err("SPM Workqueue allocation failed");
		goto fail_workqueue_alloc;
	}

	dp_ctx->spm_ctx = ctx;
	dp_info("Initialized SPM context!");

	return QDF_STATUS_SUCCESS;

fail_workqueue_alloc:
	qdf_destroy_work(NULL, &ctx->spm_work);

fail_work_creation:
	qdf_mem_free(ctx->svc_class_db);

fail_svc_db_alloc:
	qdf_mem_free(ctx);

fail:
	return QDF_STATUS_E_FAILURE;
}

void wlan_dp_spm_ctx_deinit(struct wlan_dp_psoc_context *dp_ctx)
{
	struct wlan_dp_spm_context *ctx;
	struct wlan_dp_spm_svc_class **svc_class;
	int i;

	if (!dp_ctx->spm_ctx) {
		dp_err("SPM module not present!");
		return;
	}

	ctx = dp_ctx->spm_ctx;

	if (ctx->spm_intf)
		wlan_dp_spm_intf_ctx_deinit(NULL);

	svc_class = ctx->svc_class_db;
	for (i = 0; i < ctx->max_supported_svc_class; i++) {
		if (svc_class[i])
			wlan_dp_spm_svc_class_delete(i);
	}

	qdf_flush_workqueue(NULL, ctx->spm_wq);
	qdf_destroy_workqueue(NULL, ctx->spm_wq);
	qdf_flush_work(&ctx->spm_work);
	qdf_destroy_work(NULL, &ctx->spm_work);
	qdf_mem_free(ctx);
	dp_ctx->spm_ctx = NULL;
	dp_info("Deinitialized SPM context!");
}
#else
/**
 * wlan_dp_spm_get_context(): Get SPM context from DP PSOC interface
 *
 * Return: SPM context handle
 */
static inline
struct wlan_dp_spm_context *wlan_dp_spm_get_context(void)
{
	return NULL;
}
#endif
/**
 * wlan_dp_spm_flow_retire(): Retire flows in active flow table
 * @spm_intf: SPM interface
 * @clear_tbl: If set, clear entire table
 *
 * Return: None
 */
static void wlan_dp_spm_flow_retire(struct wlan_dp_spm_intf_context *spm_intf,
				    bool clear_tbl)
{
	struct wlan_dp_spm_flow_info *cursor;
	uint64_t curr_ts = qdf_sched_clock();
	int i;

	for (i = 0; i < WLAN_DP_SPM_FLOW_REC_TBL_MAX; i++, cursor++) {
		cursor = spm_intf->origin_aft[i];
		if (!cursor)
			continue;

		if (clear_tbl) {
			qdf_list_insert_back(&spm_intf->o_flow_rec_freelist,
					     &cursor->node);
		} else if ((curr_ts - cursor->active_ts) > (5000 * 1000)) {
			qdf_mem_zero(cursor,
				     sizeof(struct wlan_dp_spm_flow_info));
			cursor->id = i;
			qdf_list_insert_back(&spm_intf->o_flow_rec_freelist,
					     &cursor->node);
			spm_intf->o_stats.active--;
			spm_intf->o_stats.deleted++;
		}
	}
}

QDF_STATUS wlan_dp_spm_intf_ctx_init(struct wlan_dp_intf *dp_intf)
{
	struct wlan_dp_spm_context *spm_ctx = wlan_dp_spm_get_context();
	struct wlan_dp_spm_intf_context *spm_intf;
	struct wlan_dp_spm_flow_info *flow_rec;
	int i;

	if (dp_intf->device_mode != QDF_STA_MODE)
		return QDF_STATUS_E_NOSUPPORT;

	if (dp_intf->spm_intf_ctx) {
		dp_info("Module already initialized!");
		return QDF_STATUS_E_ALREADY;
	}

	spm_intf = (struct wlan_dp_spm_intf_context *)
					qdf_mem_malloc(sizeof(*spm_intf));
	if (!spm_intf) {
		dp_err("Unable to allocate spm interface");
		goto fail_intf_alloc;
	}

	qdf_list_create(&spm_intf->o_flow_rec_freelist,
			WLAN_DP_SPM_FLOW_REC_TBL_MAX);

	spm_intf->flow_rec_base = (struct wlan_dp_spm_flow_info *)
			qdf_mem_malloc(sizeof(struct wlan_dp_spm_flow_info) *
				       WLAN_DP_SPM_FLOW_REC_TBL_MAX);
	if (!spm_intf->flow_rec_base) {
		dp_err("Unable to allocate origin freelist");
		goto fail_flow_rec_freelist;
	}

	flow_rec = spm_intf->flow_rec_base;
	for (i = 0; i < WLAN_DP_SPM_FLOW_REC_TBL_MAX; i++) {
		qdf_mem_zero(flow_rec, sizeof(struct wlan_dp_spm_flow_info));
		flow_rec->id = i;
		qdf_list_insert_back(&spm_intf->o_flow_rec_freelist,
				     &flow_rec->node);
		flow_rec++;
	}

	dp_intf->spm_intf_ctx = spm_intf;
	if (spm_ctx)
		spm_ctx->spm_intf = spm_intf;
	dp_info("SPM interface created");

	return QDF_STATUS_SUCCESS;

fail_flow_rec_freelist:
	qdf_mem_free(spm_intf_ctx);
	spm_intf_ctx = NULL;

fail_intf_alloc:
	return QDF_STATUS_E_FAILURE;
}

void wlan_dp_spm_intf_ctx_deinit(struct wlan_dp_intf *dp_intf)
{
	struct wlan_dp_spm_context *spm_ctx = wlan_dp_spm_get_context();
	struct wlan_dp_spm_intf_context *spm_intf;

	if (dp_intf->device_mode != QDF_STA_MODE)
		return;

	if (!dp_intf->spm_intf_ctx) {
		dp_info("Module already uninitialized!");
		return;
	}

	spm_intf = dp_intf->spm_intf_ctx;

	wlan_dp_spm_flow_retire(spm_intf, true);

	qdf_mem_free(spm_intf->flow_rec_base);

	qdf_mem_free(spm_intf);
	dp_intf->spm_intf_ctx = NULL;

	if (spm_ctx)
		spm_ctx->spm_intf = NULL;
	dp_info("SPM interface deinitialized!");
}
