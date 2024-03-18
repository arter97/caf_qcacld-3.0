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

#ifndef _WLAN_DP_LOAD_BALACE_H_
#define _WLAN_DP_LOAD_BALACE_H_

struct wlan_dp_psoc_context;

/**
 * struct cpu_irq_load - structure holding cpu irq load
 * @irq_time_prev: previous CPU irq time in ns
 * @wlan_irq_time_prev: previous wlan irq time in ns
 * @wlan_ksoftirqd_time_prev: previous wlan ksoftirqd time in ns
 * @last_avg_calc_time: time at which last average computed
 * @cpu_id: cpu id
 * @total_cpu_avg_load: Total average cpu load in percentage
 * @wlan_avg_load: wlan average load on cpu in percentage
 */
struct cpu_irq_load {
	uint64_t irq_time_prev;
	uint64_t wlan_irq_time_prev;
	uint64_t wlan_ksoftirqd_time_prev;
	uint64_t last_avg_calc_time;
	uint32_t cpu_id;
	uint8_t total_cpu_avg_load;
	uint8_t wlan_avg_load;
};

/**
 * struct wlan_dp_lb_data - structure holding data for load balance
 * @cpu_load: cpu irq load for NR_CPUS
 * @def_cpumask: default cpu mask used for load balance
 * @preferred_cpu_mask: preferred cpu mask for the load balance which will get
 * changed if all the cups of current preferred mask goes offline or current
 * preferred mask intersect with the audio used cpumask.
 * @curr_cpu_mask: cpus which are available for load balance currently,
 * subset of preferred_cpu_mask
 * @load_balance_lock: lock to protect load balace from multiple contexts
 */
struct wlan_dp_lb_data {
	struct cpu_irq_load cpu_load[NR_CPUS];
	qdf_cpu_mask def_cpumask;
	qdf_cpu_mask preferred_cpu_mask;
	qdf_cpu_mask curr_cpu_mask;
	qdf_spinlock_t load_balance_lock;
};

#ifdef WLAN_DP_LOAD_BALANCE_SUPPORT
/**
 * wlan_dp_load_balancer_init() - initialize load balancer module
 * @psoc: psoc handle
 *
 * Return: None
 */
void wlan_dp_load_balancer_init(struct wlan_objmgr_psoc *psoc);

/**
 * wlan_dp_load_balancer_deinit() - deinitialize load balancer module
 * @psoc: psoc handle
 *
 * Return: None
 */
void wlan_dp_load_balancer_deinit(struct wlan_objmgr_psoc *psoc);
#else
static inline void wlan_dp_load_balancer_init(struct wlan_objmgr_psoc *psoc)
{
}

static inline void wlan_dp_load_balancer_deinit(struct wlan_objmgr_psoc *psoc)
{
}
#endif
#endif /* _WLAN_DP_LOAD_BALACE_H_ */
