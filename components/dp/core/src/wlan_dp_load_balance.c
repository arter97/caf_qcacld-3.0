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

#include "hif.h"
#include "wlan_dp_priv.h"

#define NUM_CPUS_FOR_LOAD_BALANCE 2

/* weightage in percentage */
#define OLDER_SAMPLE_WTG 25
#define LATEST_SAMPLE_WTG (100 - OLDER_SAMPLE_WTG)

/* time in ns to averge the data sample */
#define SAMPLING_AVERAGE_TIME_THRS 1000000000

/**
 * wlan_dp_lb_compute_cpu_load_average() - function called from
 * wlan_dp_lb_compute_stats_average() and it does the following,
 * 1. Fetch the per CPU irq/softirq load(time in ns) from the kernel and
 * calculate the total cpu irq load percentage of the current sample.
 * 2. Fetch the per CPU wlan irq/softirq load and per cpu ksoftirqd load
 * (time in ns) from the wlan driver and calculate the wlan irq load percentage
 * of the current sample.
 * 3. Calculate the total cpu irq load average and cpu wlan load average.
 *
 * @dp_ctx: dp context
 *
 * Return: none
 */
static void
wlan_dp_lb_compute_cpu_load_average(struct wlan_dp_psoc_context *dp_ctx)
{
	struct kernel_cpustat kcpustat;
	struct wlan_dp_lb_data *lb_data = &dp_ctx->lb_data;
	struct cpu_irq_load *cpu_load;
	void *hif_ctx = dp_ctx->hif_handle;
	uint64_t wlan_irq_time[NR_CPUS] = {0};
	uint64_t wlan_ksoftirqd_time[NR_CPUS] = {0};
	uint64_t curr_time_in_ns;
	uint64_t total_irq_time, total_irq_time_cur;
	uint64_t wlan_irq_time_cur;
	uint64_t wlan_ksoftirqd_time_cur;
	uint64_t total_cpu_load, wlan_load;
	int cpu;

	hif_get_wlan_rx_time_stats(hif_ctx, wlan_irq_time, wlan_ksoftirqd_time);

	qdf_for_each_online_cpu(cpu) {
		cpu_load = &lb_data->cpu_load[cpu];
		kcpustat_cpu_fetch(&kcpustat, cpu);

		curr_time_in_ns = qdf_sched_clock();
		total_irq_time = kcpustat.cpustat[CPUTIME_SOFTIRQ];
		total_irq_time += kcpustat.cpustat[CPUTIME_IRQ];

		/* If the cpu goes offline and comes back online, the current
		 * irq time will be lesser than the previous irq time. Similarly
		 * when the wlan driver unloaded and loaded back, wlan irq time
		 * will be lesser than the previous wlan irq time. Update the
		 * current irq time in the previous irq time and return.
		 */
		if (!cpu_load->irq_time_prev ||
		    (cpu_load->irq_time_prev > total_irq_time) ||
		    (cpu_load->wlan_irq_time_prev > wlan_irq_time[cpu])) {
			cpu_load->irq_time_prev = total_irq_time;
			cpu_load->wlan_irq_time_prev = wlan_irq_time[cpu];
			cpu_load->wlan_ksoftirqd_time_prev =
						wlan_ksoftirqd_time[cpu];
			cpu_load->last_avg_calc_time = curr_time_in_ns;
			cpu_load->cpu_id = cpu;
			continue;
		}

		total_irq_time_cur = total_irq_time - cpu_load->irq_time_prev;
		wlan_irq_time_cur = wlan_irq_time[cpu] -
					cpu_load->wlan_irq_time_prev;
		wlan_ksoftirqd_time_cur = wlan_ksoftirqd_time[cpu] -
					cpu_load->wlan_ksoftirqd_time_prev;

		if (wlan_irq_time_cur > total_irq_time_cur) {
			dp_debug("cpu_irq_time %llu wlan_irq_time %llu cpu %d",
				 total_irq_time_cur, wlan_irq_time_cur, cpu);
			continue;
		}

		total_cpu_load =
			((total_irq_time_cur + wlan_ksoftirqd_time_cur) * 100) /
			(curr_time_in_ns - cpu_load->last_avg_calc_time);
		wlan_load =
			((wlan_irq_time_cur + wlan_ksoftirqd_time_cur) * 100) /
			(curr_time_in_ns - cpu_load->last_avg_calc_time);

		if (!cpu_load->total_cpu_avg_load)
			cpu_load->total_cpu_avg_load = total_cpu_load;
		else
			cpu_load->total_cpu_avg_load =
			((cpu_load->total_cpu_avg_load * OLDER_SAMPLE_WTG) / 100) +
			((total_cpu_load * LATEST_SAMPLE_WTG) / 100);

		if (!cpu_load->wlan_avg_load)
			cpu_load->wlan_avg_load = wlan_load;
		else
			cpu_load->wlan_avg_load =
			((cpu_load->wlan_avg_load * OLDER_SAMPLE_WTG) / 100) +
			((wlan_load * LATEST_SAMPLE_WTG) / 100);

		cpu_load->irq_time_prev = total_irq_time;
		cpu_load->wlan_irq_time_prev = wlan_irq_time[cpu];
		cpu_load->wlan_ksoftirqd_time_prev = wlan_ksoftirqd_time[cpu];
		cpu_load->last_avg_calc_time = curr_time_in_ns;
	}
}

/**
 * wlan_dp_lb_compute_stats_average() - Called from the bus bandwidth
 * handler to compute the stats average for every sampling threshold time(1sec).
 *
 * @dp_ctx: dp context
 *
 * Return: none
 */
void wlan_dp_lb_compute_stats_average(struct wlan_dp_psoc_context *dp_ctx)
{
	struct wlan_dp_lb_data *lb_data;
	uint64_t curr_time_in_ns = qdf_sched_clock();
	uint64_t time_delta_ns;

	lb_data = &dp_ctx->lb_data;
	if (!lb_data->last_stats_avg_comp_time) {
		lb_data->last_stats_avg_comp_time = curr_time_in_ns;
		return;
	}

	time_delta_ns = curr_time_in_ns - lb_data->last_stats_avg_comp_time;
	if (time_delta_ns >= SAMPLING_AVERAGE_TIME_THRS) {
		wlan_dp_lb_compute_cpu_load_average(dp_ctx);
		cdp_calculate_per_ring_pkt_avg(dp_ctx->cdp_soc);
		lb_data->last_stats_avg_comp_time = curr_time_in_ns;
	}
}

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
