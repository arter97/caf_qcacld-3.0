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

#include "wlan_dp_priv.h"

#define NUM_CPUS_FOR_LOAD_BALANCE 2

/**
 * wlan_dp_load_balancer_deinit() - deinitialize load balancer module
 * @psoc: psoc handle
 *
 * Return: None
 */
void wlan_dp_load_balancer_deinit(struct wlan_objmgr_psoc *psoc)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_psoc_get_priv(psoc);
	struct wlan_dp_lb_data *lb_data = &dp_ctx->lb_data;

	qdf_spinlock_destroy(&lb_data->load_balance_lock);
}

/**
 * wlan_dp_load_balancer_init() - initialize load balancer module
 *
 * By default balance the IRQs on little clusters. Hence update the default
 * cpu mask in the load balance data with the little cluster mask.
 *
 * def_cpumask: Default cpu mask on which IRQs will get balanced until all the
 * cpus of the def_cpumask goes offline. This def_cpumask will get used later
 * to restore the cpumask in the preferred cpu mask in case of cpu mask change
 * due to cpu hotplug or audio affinity manager.
 *
 * preferred_cpu_mask: This is the global cpumask which will get updated when
 * the cpu cluster get changed. For example,
 * after driver load, preferred_cpu_mask = 0x3.
 * If all the cpus from the preferred_cpu_mask goes offline or if
 * qdf_walt_get_cpus_taken() return 0x3 then the preferred_cpu_mask will get
 * updated to 0xC.
 * If cpu0/cpu1 comes back online or qdf_walt_get_cpus_taken() return empty
 * then preferred_cpu_mask will get updated with the def_cpumask i.e 0x3.
 *
 * curr_cpu_mask: current cpumask is the subset of preferred_cpu_mask which
 * will be used for the load balance.
 * Example: after driver load, preferred_cpu_mask = 0x3 and curr_cpu_mask = 0x3
 * If cpu0 goes offline, preferred_cpu_mask = 0x3 and curr_cpu_mask = 0x2
 * If cpu0 comes back online, preferred_cpu_mask = 0x3 curr_cpu_mask = 0x3
 *
 * @psoc: psoc handle
 *
 * Return: None
 */
void wlan_dp_load_balancer_init(struct wlan_objmgr_psoc *psoc)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_psoc_get_priv(psoc);
	struct wlan_dp_lb_data *lb_data;
	unsigned int cpus;
	int package_id;
	int num_cpus = 0;

	lb_data = &dp_ctx->lb_data;
	qdf_spinlock_create(&lb_data->load_balance_lock);

	qdf_for_each_possible_cpu(cpus) {
		package_id = qdf_topology_physical_package_id(cpus);
		if (package_id == CPU_CLUSTER_TYPE_LITTLE) {
			qdf_cpumask_set_cpu(cpus, &lb_data->def_cpumask);
			num_cpus++;
			if (num_cpus >= NUM_CPUS_FOR_LOAD_BALANCE)
				break;
		}
	}

	qdf_cpumask_copy(&lb_data->preferred_cpu_mask,
			 &lb_data->def_cpumask);
	qdf_cpumask_copy(&lb_data->curr_cpu_mask,
			 &lb_data->def_cpumask);

	dp_info("default cpus for load balance: %*pbl",
		qdf_cpumask_pr_args(&lb_data->def_cpumask));
}
