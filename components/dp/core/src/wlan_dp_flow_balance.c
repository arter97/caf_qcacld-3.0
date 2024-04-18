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

#include "wlan_dp_fisa_rx.h"
#include "wlan_dp_flow_balance.h"
#include <cdp_txrx_ctrl.h>
#include "wlan_dp_main.h"

/* units in percentage(%) */
#define FLOW_THRASH_DETECT_THRES 90
#define RING_OVERLOAD_THRESH 70

static void
wlan_dp_fb_do_flow_balance(struct wlan_dp_psoc_context *dp_ctx,
			   struct wlan_dp_rx_ring_fm_tbl *map_tbl,
			   uint32_t *targeted_weightages,
			   uint8_t num_rings,
			   struct wlan_dp_rx_ring_wtg *balanced_wtgs)
{
}

static inline int
wlan_dp_fb_get_num_rings_for_flow_balance(uint8_t num_rx_rings,
					  int total_num_flows)
{
	if (total_num_flows >= num_rx_rings)
		return num_rx_rings;
	else
		return total_num_flows;
}

/**
 * wlan_dp_fb_get_per_ring_targeted_weightage() - Gives the per ring targeted
 * weightages
 * Calculated the number of rings will going to assign to each cpu and
 * targeted weightage of each ring based on the allowed wlan load of the
 * respective CPU.
 * Example: num_cpus = 2, num_rings = 4, allowed_load_on_cpu1 = 60%,
 *	allowed_load_on_cpu2 = 40% then,
 * targeted_weightages will be ring0 = 30%, ring1 = 30%,
 *	ring2 = 20% and ring4 = 20%.
 *
 * @cpu_load_avg: per cpu load average in percentage
 * @num_cpus: num cpus available for balance
 * @num_rings: number of rings for flow balance
 * @targeted_weightages: targeted weightages result
 */
static void
wlan_dp_fb_get_per_ring_targeted_weightage(struct cpu_load *cpu_load_avg,
					   int num_cpus, int num_rings,
					   uint32_t *targeted_weightages)
{
	int index = 0;
	int num_rings_per_cpu;
	int i, j, rem;

	rem = num_rings % num_cpus;

	for (i = 0; i < num_cpus; i++) {
		num_rings_per_cpu = num_rings / num_cpus;
		if (rem) {
			num_rings_per_cpu++;
			rem--;
		}

		for (j = 0; j < num_rings_per_cpu; j++)
			targeted_weightages[index++] =
				cpu_load_avg[i].allowed_wtg / num_rings_per_cpu;
	}
}

/**
 * wlan_dp_fb_cal_per_flow_weightage() - calculate the per flow weightage in
 * percentage
 * weightage = flow throughput / total throughput.
 * @map_tbl: rx ring flow map table which holds the per ring per flow info
 * @total_flow_pkt_avg: sum of packets per second avg of all the flows
 *
 * Return: None
 */
static void
wlan_dp_fb_cal_per_flow_weightage(struct wlan_dp_rx_ring_fm_tbl *map_tbl,
				  uint32_t total_flow_pkt_avg)
{
	struct wlan_dp_rx_ring_fm_tbl *ring;
	struct wlan_dp_flow *flow_info;
	int i, flow;

	for (i = 0; i < MAX_REO_DEST_RINGS; i++) {
		ring = &map_tbl[i];

		if (!ring->total_avg_pkts)
			continue;

		for (flow = 0; flow < ring->num_flows; flow++) {
			flow_info = &ring->flow_list[flow];
			flow_info->flow_wtg = (flow_info->avg_pkt_cnt * 100) /
						total_flow_pkt_avg;
			ring->ring_weightage += flow_info->flow_wtg;
		}
	}
}

/**
 * wlan_dp_fb_handler() - handler for flow balance called from load balance
 * handler
 * 1. Builds the flow_map table which holds the per ring flow details
 * 2. Calculates the per flow weightage and detects the flow thrashing by
 *    comparing the flow map table ring weightage with the per ring packet
 *    average computed from per reo stats.
 * 3. Calculate the num rings required for flow balance based on the num flows
 * 4. Calculate the per ring targeted weightage, so that the flows will be
 *     moved across the rings based on this targeted weightage.
 * 5. Do the flow balance and update the balanced ring weightages.
 *
 * @dp_ctx: dp context
 * @per_ring_pkt_avg: per ring packet average per second
 * @total_avg_pkts: sum of all rings packet average per second
 * @cpu_load: cpu load details in percentage
 * @num_cpus: number of cpus available for load balance
 * @balanced_wtgs: ring weightage after flow balance
 *
 * Return: returns the wlan_dp_fb_status
 */
enum wlan_dp_fb_status
wlan_dp_fb_handler(struct wlan_dp_psoc_context *dp_ctx,
		   uint32_t *per_ring_pkt_avg,
		   uint32_t total_avg_pkts,
		   struct cpu_load *cpu_load,
		   uint32_t num_cpus,
		   struct wlan_dp_rx_ring_wtg *balanced_wtgs)
{
	struct dp_rx_fst *rx_fst = dp_ctx->rx_fst;
	struct wlan_dp_rx_ring_fm_tbl flow_map_tbl[MAX_REO_DEST_RINGS] = {0};
	struct wlan_dp_rx_ring_fm_tbl *ring;
	uint32_t targeted_weightages[MAX_REO_DEST_RINGS] = {0};
	uint32_t total_flow_pkt_avg = 0;
	uint32_t total_num_flows = 0;
	int i;
	uint8_t total_num_rings = dp_ctx->lb_data.num_wlan_used_rx_rings;
	uint8_t num_rings;
	bool flow_thrash = false;
	bool ring_overloaded = false;
	bool flow_balance_not_eligible = false;

	for (i = 0; i <  MAX_REO_DEST_RINGS; i++) {
		ring = &flow_map_tbl[i];
		ring->ring_id = i;
		ring->flow_list =
		(struct wlan_dp_flow *)qdf_mem_malloc(rx_fst->max_entries *
						      sizeof(struct wlan_dp_flow));
		if (!ring->flow_list) {
			dp_err("failed to allocate flow list for ring %d", i);
			flow_balance_not_eligible = true;
			goto free_flow_list;
		}
	}

	dp_fisa_flow_balance_build_flow_map_tbl(dp_ctx, &flow_map_tbl[0],
						&total_flow_pkt_avg,
						&total_num_flows);

	if (total_num_flows <= 1) {
		flow_balance_not_eligible = true;
		goto free_flow_list;
	}

	wlan_dp_fb_cal_per_flow_weightage(&flow_map_tbl[0], total_flow_pkt_avg);

	/* Do the flow balance only in the following cases,
	 * 1. No flow thrashing detected
	 * 2. Most of the flows are mapped to single ring
	 */
	for (i = 0; i <  MAX_REO_DEST_RINGS; i++) {
		ring = &flow_map_tbl[i];
		if (ring->num_flows &&
		    (ring->total_avg_pkts <
		     ((per_ring_pkt_avg[i] * FLOW_THRASH_DETECT_THRES) / 100))) {
			dp_debug("flow thrashing detected on ring %d flows_avg_pkts %u ring_avg_pkts %u",
				 i, ring->total_avg_pkts, per_ring_pkt_avg[i]);
			flow_thrash = true;
			break;
		}

		if (ring->ring_weightage > RING_OVERLOAD_THRESH) {
			dp_debug("ring%d is overloaded weightage %u",
				 i, ring->ring_weightage);
			ring_overloaded = true;
		}
	}

	if (flow_thrash || !ring_overloaded) {
		dp_debug("flow balance is not eligible flow_trash %s ring_overloaded %s",
			 flow_thrash ? "true" : "false",
			 ring_overloaded ? "true" : "false");
		flow_balance_not_eligible = true;
		goto free_flow_list;
	}

	num_rings = wlan_dp_fb_get_num_rings_for_flow_balance(total_num_rings,
							      total_num_flows);

	dp_debug("flow_balance: num_rings %u num_flows %u",
		 num_rings, total_num_flows);

	wlan_dp_fb_get_per_ring_targeted_weightage(cpu_load,
						   num_cpus, num_rings,
						   targeted_weightages);

	wlan_dp_fb_do_flow_balance(dp_ctx, &flow_map_tbl[0],
				   targeted_weightages, num_rings,
				   balanced_wtgs);
free_flow_list:
	for (i = 0; i <  MAX_REO_DEST_RINGS; i++) {
		ring = &flow_map_tbl[i];
		if (ring->flow_list)
			qdf_mem_free(ring->flow_list);
	}

	if (flow_balance_not_eligible)
		return FLOW_BALANCE_NOT_ELIGIBLE;
	else
		return FLOW_BALANCE_SUCCESS;
}

void wlan_dp_fb_update_num_rx_rings(struct wlan_objmgr_psoc *psoc)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_psoc_get_priv(psoc);
	struct wlan_dp_lb_data *lb_data = &dp_ctx->lb_data;
	cdp_config_param_type soc_param;
	QDF_STATUS status;
	int i;

	status = cdp_txrx_get_psoc_param(dp_ctx->cdp_soc,
					 CDP_CFG_REO_RINGS_MAPPING, &soc_param);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("Unable to fetch rx rings mapping");
		return;
	}

	for (i = 0; i < MAX_REO_DEST_RINGS; i++) {
		if ((BIT(i) & soc_param.cdp_reo_rings_mapping) &&
		    !wlan_dp_check_is_ring_ipa_rx(dp_ctx->cdp_soc, i))
			lb_data->num_wlan_used_rx_rings++;
	}
}
