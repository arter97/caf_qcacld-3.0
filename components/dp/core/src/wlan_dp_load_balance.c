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
#include "wlan_dp_fisa_rx.h"

#define NUM_CPUS_FOR_LOAD_BALANCE 2

/* weightage in percentage */
#define OLDER_SAMPLE_WTG 25
#define LATEST_SAMPLE_WTG (100 - OLDER_SAMPLE_WTG)

/* time in ns to averge the data sample */
#define SAMPLING_AVERAGE_TIME_THRS 1000000000

/*time in ns to make the load balance */
#define LOAD_BALANCE_TIME_THRS 5000000000

/**
 * wlan_dp_lb_sort_ring_weightages() - function to sort the ring weightages
 *	from high to low
 * @e1: first element
 * @e2: second element
 *
 * Return: -1, if element1 weight is greater than the element2 weight
 *	1, if element1 weight is lesser than the element2 weight
 *	0, if both weights are equal
 */
int wlan_dp_lb_sort_ring_weightages(const void *e1, const void *e2)
{
	struct wlan_dp_rx_ring_wtg *w1 = (struct wlan_dp_rx_ring_wtg *)e1;
	struct wlan_dp_rx_ring_wtg *w2 = (struct wlan_dp_rx_ring_wtg *)e2;

	if (w1->weight > w2->weight)
		return -1;
	else if (w1->weight < w2->weight)
		return 1;
	else
		return 0;
}

/**
 * wlan_dp_lb_irq_balance_handler() - handler for irq balancing
 * Sort the ring weightages from high to low since the cpu list is
 * sorted from high to low based on the allowed load. Idea is to take
 * highest load allowed cpu first and put ring weightages until the allowed
 * weightage.
 * @dp_ctx: dp context
 * @cpu_load_avg: cpu load average in percentage for NR_CPUS
 * @num_cpus: Number of available CPUs
 * @weightages: weightage of each rx ring in percentage
 *
 * Return: None
 */
static void
wlan_dp_lb_irq_balance_handler(struct wlan_dp_psoc_context *dp_ctx,
			       struct cpu_load *cpu_load_avg,
			       uint32_t num_cpus,
			       struct wlan_dp_rx_ring_wtg *weightages)
{
	int cpu, i = 0;
	int grp_id;

	/* sort ring weightages from high to low */
	qdf_sort(weightages, MAX_REO_DEST_RINGS, sizeof(*weightages),
		 wlan_dp_lb_sort_ring_weightages, NULL);

	cpu = 0;

	while (weightages[i].weight) {
		dp_debug("Applying ring%d weight %d on cpu%d",
			 weightages[i].ring_id, weightages[i].weight,
			 cpu_load_avg[cpu].cpu_id);

		grp_id = cdp_get_ext_grp_id_from_reo_num(dp_ctx->cdp_soc,
							 weightages[i].ring_id);
		if (grp_id < 0) {
			dp_err("failed to get grp id");
			continue;
		}

		hif_check_and_apply_irq_affinity(dp_ctx->hif_handle, grp_id,
						 cpu_load_avg[cpu].cpu_id);

		cpu_load_avg[cpu].allowed_wtg -= weightages[i++].weight;

		if (i >= MAX_REO_DEST_RINGS)
			break;

		if ((cpu_load_avg[cpu].allowed_wtg + ALLOWED_BUFFER_WT) >=
		    (int)weightages[i].weight)
			continue;

		cpu++;
		if (cpu >= num_cpus)
			cpu = 0;
	}
}

/**
 * wlan_dp_lb_sort_cpu_load() - function to sort the cpu loads
 *	from high to low
 * @elem1: first element
 * @elem2: second element
 *
 * Return: -1, if element1 weight is greater than the element2 weight
 *	1, if element1 weight is lesser than the element2 weight
 *	0, if both weights are equal
 */
int wlan_dp_lb_sort_cpu_load(const void *elem1, const void *elem2)
{
	struct cpu_load *c1 = (struct cpu_load *)elem1;
	struct cpu_load *c2 = (struct cpu_load *)elem2;

	if (c1->allowed_wtg > c2->allowed_wtg)
		return -1;
	else if (c1->allowed_wtg < c2->allowed_wtg)
		return 1;
	else
		return 0;
}

/**
 * wlan_dp_lb_update_cur_ring_wtgs() - calculate and update the per ring
 *	weightage in percentage
 * @per_ring_pkt_avg: per ring packet average per second
 * @total_avg_pkts: total average pkts per second received across all the rings
 * @cur_ring_wtg: Array of structures to store the current ring weightage info
 *
 * Return: none
 */
static void
wlan_dp_lb_update_cur_ring_wtgs(uint32_t *per_ring_pkt_avg,
				uint32_t total_avg_pkts,
				struct wlan_dp_rx_ring_wtg *cur_ring_wtg)
{
	struct wlan_dp_rx_ring_wtg *ring_wtg;
	int ring;

	for (ring = 0; ring < MAX_REO_DEST_RINGS; ring++) {
		ring_wtg = &cur_ring_wtg[ring];
		ring_wtg->ring_id = ring;
		ring_wtg->weight = (per_ring_pkt_avg[ring] * 100) /
					total_avg_pkts;
	}
}

/**
 * wlan_dp_lb_handler() - handler for load balance
 * Calculates the per CPU allowed wlan tput weightage(throughput in percentage).
 * If flow balance support is enabled, get the per ring throughput weightage in
 * percentage after flow balance else calculate the per ring tput weightage
 * from the REO stats.
 * Calls the IRQ balance handler with the per ring tput weightages to place the
 * IRQs on the available CPUs based on it's allowed wlan tput weightage.
 *
 * Per cpu allowed wlan weightage calculation: for example cpu0 and cpu1 are
 * considered for load balance,
 * cpu0 non-wlan irq load = cpu0 total irq load - cpu0 wlan irq load
 * cpu1 non-wlan irq load = cpu1 total irq load - cpu1 wlan irq load
 * overall cpu irq load = cpu0 total irq load + cpu1 total irq load
 * overall wlan irq load = cpu0 wlan irq load + cpu1 wlan irq load
 *
 * per cpu targeted load  = overall cpu irq load / 2(number of cpus) to make
 * load is balanced among cpu0 and cpu1.
 *
 * Let's assume 100% of throughput is contributing overall wlan irq load of x%
 * 1% of cpu = 100/x percentage of throughput
 * cpu0 allowed wlan load = per cpu targeted load - cpu0 non-wlan irq load
 * cpu0 allowed wlan tput weightage = cpu0 allowed wlan load * 100/x
 * cpu1 allowed wlan load = per cpu targeted load - cpu1 non-wlan irq load
 * cpu1 allowed wlan tput weightage = cpu1allowed wlan load * 100/x
 *
 * wlan_dp_lb_handler is get called for every load balance time threshold
 * (5 sec), cpu hotplug, and preferred cpu mask change due to audio affinity
 * manager.
 *
 * @dp_ctx: dp context
 *
 * Return: None
 */
static void wlan_dp_lb_handler(struct wlan_dp_psoc_context *dp_ctx)
{
	struct wlan_dp_lb_data *lb_data = &dp_ctx->lb_data;
	struct wlan_dp_rx_ring_wtg weightages[MAX_REO_DEST_RINGS];
	struct cpu_irq_load *cpu_load;
	struct cpu_load cpu_load_avgs[NR_CPUS];
	struct cpu_load *cpu_load_avg;
	uint32_t per_ring_pkt_avg[MAX_REO_DEST_RINGS];
	qdf_cpu_mask *cpu_mask = &lb_data->curr_cpu_mask;
	uint64_t start_time;
	uint32_t num_cpus = 0;
	uint32_t total_cpu_load = 0;
	uint32_t total_wlan_load = 0;
	uint32_t total_avg_pkts = 0;
	int cpu;
	uint8_t targeted_load_per_cpu;
	uint8_t non_wlan_avg_load;

	dp_info("cpu mask for load balance %*pbl ",
		qdf_cpumask_pr_args(cpu_mask));

	/* No need to balance anything if only one cpu is available */
	if (qdf_cpumask_weight(cpu_mask) <= 1)
		return;

	start_time = qdf_sched_clock();
	qdf_spin_lock(&lb_data->load_balance_lock);

	/* Take the copy of cpu load for the CPUs present in the
	 * current cpu mask
	 */
	qdf_for_each_online_cpu(cpu) {
		if (!(qdf_cpumask_test_cpu(cpu, cpu_mask)))
			continue;

		cpu_load = &lb_data->cpu_load[cpu];
		cpu_load_avg = &cpu_load_avgs[num_cpus];
		cpu_load_avg->total_cpu_avg_load =
					cpu_load->total_cpu_avg_load;
		cpu_load_avg->wlan_avg_load =
					cpu_load->wlan_avg_load;
		cpu_load_avg->cpu_id  = cpu_load->cpu_id;

		total_cpu_load += cpu_load_avg->total_cpu_avg_load;
		total_wlan_load += cpu_load_avg->wlan_avg_load;
		num_cpus++;
	}

	targeted_load_per_cpu = total_cpu_load / num_cpus;

	/*calculate and update per cpu allowed wlan tput weightage */
	for (cpu = 0; cpu < num_cpus; cpu++) {
		cpu_load_avg = &cpu_load_avgs[cpu];
		non_wlan_avg_load = cpu_load_avg->total_cpu_avg_load -
					cpu_load_avg->wlan_avg_load;
		if (non_wlan_avg_load >= targeted_load_per_cpu)
			cpu_load_avg->allowed_wtg = 0;
		else
			cpu_load_avg->allowed_wtg =
			((targeted_load_per_cpu - non_wlan_avg_load) * 100) /
			total_wlan_load;
		dp_debug("cpu_id %d total_irq_load %d wlan_irq_load %d allowed_tpu_wtg %d",
			 cpu_load_avg->cpu_id,
			 cpu_load_avg->total_cpu_avg_load,
			 cpu_load_avg->wlan_avg_load,
			 cpu_load_avg->allowed_wtg);
	}

	/* sort cpus based on allowed tput weightages from high to low */
	qdf_sort(cpu_load_avgs, num_cpus, sizeof(struct cpu_load),
		 wlan_dp_lb_sort_cpu_load, NULL);

	cdp_get_per_ring_pkt_avg(dp_ctx->cdp_soc,
				 per_ring_pkt_avg, &total_avg_pkts);

	/* update the current ring weightages which are needed
	 * for irq balance.
	 */
	wlan_dp_lb_update_cur_ring_wtgs(per_ring_pkt_avg,
					total_avg_pkts, &weightages[0]);

	wlan_dp_lb_irq_balance_handler(dp_ctx, &cpu_load_avgs[0],
				       num_cpus, &weightages[0]);

	qdf_spin_unlock(&lb_data->load_balance_lock);
	lb_data->last_load_balanced_time = qdf_sched_clock();

	dp_info("Total time taken for load balance %lluns",
		lb_data->last_load_balanced_time - start_time);
}

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
		lb_data->last_load_balanced_time = curr_time_in_ns;
		return;
	}

	time_delta_ns = curr_time_in_ns - lb_data->last_stats_avg_comp_time;
	if (time_delta_ns >= SAMPLING_AVERAGE_TIME_THRS) {
		wlan_dp_lb_compute_cpu_load_average(dp_ctx);
		cdp_calculate_per_ring_pkt_avg(dp_ctx->cdp_soc);
		dp_fisa_calc_flow_stats_avg(dp_ctx);
		lb_data->last_stats_avg_comp_time = curr_time_in_ns;
	}

	time_delta_ns = curr_time_in_ns - lb_data->last_load_balanced_time;
	if (time_delta_ns >= LOAD_BALANCE_TIME_THRS)
		wlan_dp_lb_handler(dp_ctx);
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