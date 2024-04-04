/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

#ifndef _WLAN_DP_SPM_H_
#define _WLAN_DP_SPM_H_

#include <qdf_status.h>
#include <qdf_nbuf.h>
#include "wlan_dp_main.h"

#define WLAN_DP_SPM_FLOW_REC_TBL_MAX 256
#define WLAN_DP_SPM_MAX_SERVICE_CLASS_SUPPORT 32
#define WLAN_DP_SPM_INVALID_METADATA 0xFF

/**
 * struct wlan_dp_spm_flow_tbl_stats - Flow tables statistics
 * @total: Total flows tracked count
 * @active: Current active flows count
 * @deleted: Number of flows deleted
 * @mapped: Number of flows currently mapped
 */
struct wlan_dp_spm_flow_tbl_stats {
	uint32_t total;
	uint32_t active;
	uint32_t deleted;
	uint32_t mapped;
};

/**
 * struct wlan_dp_spm_flow_info - Record of flow which will be tracked
 * @node: List node
 * @id: Flow ID
 * @info: Flow details
 * @guid: Global unique identifier
 * @peer_id: Peer ID
 * @svc_id: Service ID
 * @svc_metadata: Service metadata
 * @cookie: sock address or skb hash
 * @flags: flow flags
 * @active_ts: last active timestamp
 */
struct wlan_dp_spm_flow_info {
	qdf_list_node_t node;
	uint16_t id;
	struct flow_info info;
	uint64_t guid;
	uint16_t peer_id;
	uint16_t svc_id;
	uint8_t svc_metadata;
	uint64_t cookie;
	uint32_t flags;
	uint64_t active_ts;
};

/**
 * struct wlan_dp_spm_intf_context - SPM context per dp interface
 * @origin_aft: Active flow table for originating traffic
 * @flow_rec_base: Flow records base address
 * @o_flow_rec_freelist: Flow records freelist
 * @o_stats: Flow table stats for originating traffic
 */
struct wlan_dp_spm_intf_context {
	struct wlan_dp_spm_flow_info *origin_aft[WLAN_DP_SPM_FLOW_REC_TBL_MAX];
	struct wlan_dp_spm_flow_info *flow_rec_base;
	qdf_list_t o_flow_rec_freelist;
	struct wlan_dp_spm_flow_tbl_stats o_stats;
};

/**
 * struct wlan_dp_spm_policy_info: Policy information
 * @node: List node
 * @flow: Policy flow details
 * @id: policy ID
 * @is_5tuple: 2 tuple or 5 tuple
 * @svc_id: Service ID for this policy
 * @flows_attached: Number of flows attached
 */
struct wlan_dp_spm_policy_info {
	qdf_list_node_t node;
	struct flow_info flow;
	uint64_t id;
	bool is_5tuple;
	uint16_t svc_id;
	uint16_t flows_attached;
};

/**
 * struct wlan_dp_spm_svc_queue_info - Metadata of msdu queue per tid per peer
 * @node: List node
 * @peer_id: peer ID
 * @metadata: Queue metadata per peer
 */
struct wlan_dp_spm_svc_queue_info {
	qdf_list_node_t node;
	uint16_t peer_id;
	uint8_t metadata;
};

/**
 * struct wlan_dp_spm_svc_class - Service class information
 * @id: Service id
 * @tid: Traffic identifier
 * @msdu_loss_rate: Loss rate per ppm of this flow
 * @policy_list: list of Policies mapped to the service
 * @queue_info: MSDU queue metadata for the service
 */
struct wlan_dp_spm_svc_class {
	uint16_t id;
	uint8_t tid;
	uint32_t msdu_loss_rate;
	qdf_list_t policy_list;
	/* TBD: Converted to qdf_list_t when concurrency is supported. */
	struct wlan_dp_spm_svc_queue_info queue_info;
};

/**
 * struct wlan_dp_spm_context - SPM context
 * @svc_class_db: Service database
 * @max_supported_svc_class: Supported service classes
 * @spm_wq: SPM Workqueue
 * @spm_work: Work context of SPM
 * @evt_list: Event list
 * @evt_lock: SPM Event list lock
 * @flow_guid_gen: Ever incrementing count to give unique ID to flows
 * @spm_intf: Interface level SPM context containing flow tables
 * @svc_cnt: Number of Service classes present
 * @policy_cnt: Number of policies attached
 */
struct wlan_dp_spm_context {
	struct wlan_dp_spm_svc_class **svc_class_db;
	uint32_t max_supported_svc_class;
	qdf_workqueue_t *spm_wq;
	qdf_work_t spm_work;
	qdf_list_t evt_list;
	qdf_spinlock_t evt_lock;
	/* TBD: Converted to TAILQ when concurrency is supported. */
	struct wlan_dp_spm_intf_context *spm_intf;
	uint32_t svc_cnt;
	uint32_t policy_cnt;
};

#ifdef WLAN_FEATURE_SAWFISH
/**
 * wlan_dp_spm_ctx_init() - Initialize SPM context
 * @dp_ctx: PSOC DP context
 * @queues_per_tid: Number of queues per tid
 * @num_tids: number of TIDs
 *
 * Return: Success if initialized successfully
 */
QDF_STATUS wlan_dp_spm_ctx_init(struct wlan_dp_psoc_context *dp_ctx,
				uint32_t queues_per_tid, uint32_t num_tids);

/**
 * wlan_dp_spm_ctx_deinit(): Deinitialize SPM context
 * @dp_ctx: PSOC DP context
 *
 * Return: None
 */
void wlan_dp_spm_ctx_deinit(struct wlan_dp_psoc_context *dp_ctx);

#else
static inline
QDF_STATUS wlan_dp_spm_ctx_init(struct wlan_dp_psoc_context *dp_ctx,
				uint32_t queues_per_tid, uint32_t num_tids)
{
	return QDF_STATUS_SUCCESS;
}

static inline
QDF_STATUS wlan_dp_spm_ctx_deinit(struct wlan_dp_psoc_context *dp_ctx)
{
	return QDF_STATUS_SUCCESS;
}
#endif

#if defined(WLAN_FEATURE_SAWFISH) || defined(WLAN_FEATURE_MLSTC)
/**
 * wlan_dp_spm_intf_ctx_init(): Initialize per interface SPM context
 * @dp_intf: DP interface
 *
 * Return: Success if initialized, else failure code
 */
QDF_STATUS wlan_dp_spm_intf_ctx_init(struct wlan_dp_intf *dp_intf);

/**
 * wlan_dp_spm_intf_ctx_deinit(): De-initialize per interface SPM context
 * @dp_intf: DP interface
 *
 * Return: None
 */
void wlan_dp_spm_intf_ctx_deinit(struct wlan_dp_intf *dp_intf);
#else
static inline
QDF_STATUS wlan_dp_spm_intf_ctx_init(struct wlan_dp_intf *dp_intf)
{
	return QDF_STATUS_SUCCESS;
}

static inline
void wlan_dp_spm_intf_ctx_deinit(struct wlan_dp_intf *dp_intf)
{
}
#endif
#endif
