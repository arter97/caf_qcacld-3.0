/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
#ifndef __DP_RH_H
#define __DP_RH_H

#include <dp_types.h>
#include <dp_mon.h>
#include <hal_rh_tx.h>
#include <hal_rh_rx.h>
#include <qdf_pkt_add_timestamp.h>

/**
 * struct dp_soc_rh - Extended DP soc for RH targets
 * @soc: dp soc structure
 */
struct dp_soc_rh {
	struct dp_soc soc;
};

/**
 * struct dp_pdev_rh - Extended DP pdev for RH targets
 * @pdev: dp_pdev structure
 */
struct dp_pdev_rh {
	struct dp_pdev pdev;
};

/**
 * struct dp_vdev_rh - Extended DP vdev for RH targets
 * @vdev: dp_vdev structure
 */
struct dp_vdev_rh {
	struct dp_vdev vdev;
};

/**
 * struct dp_peer_rh - Extended DP peer for RH targets
 * @peer: dp_peer structure
 */
struct dp_peer_rh {
	struct dp_peer peer;
};

/**
 * struct dp_mon_soc_rh - Extended DP mon soc for RH targets
 * @mon_soc: dp_mon_soc structure
 */
struct dp_mon_soc_rh {
	struct dp_mon_soc mon_soc;
};

/**
 * struct dp_mon_pdev_rh - Extended DP mon pdev for RH targets
 * @mon_pdev: dp_mon_pdev structure
 */
struct dp_mon_pdev_rh {
	struct dp_mon_pdev mon_pdev;
};

/**
 * dp_get_soc_context_size_rh() - get context size for dp_soc_rh
 *
 * Return: value in bytes for RH specific soc structure
 */
qdf_size_t dp_get_soc_context_size_rh(void);

/**
 * dp_initialize_arch_ops_rh() - initialize RH specific arch ops
 * @arch_ops: arch ops pointer
 *
 * Return: none
 */
void dp_initialize_arch_ops_rh(struct dp_arch_ops *arch_ops);

/**
 * dp_get_context_size_rh() - get RH specific size for peer/vdev/pdev/soc
 * @context_type: Context type to get size
 *
 * Return: size in bytes for the context_type
 */

qdf_size_t dp_get_context_size_rh(enum dp_context_type context_type);

/**
 * dp_mon_get_context_size_rh() - get RH specific size for mon pdev/soc
 * @context_type: Mon context type to get size
 *
 * Return: size in bytes for the context_type
 */

qdf_size_t dp_mon_get_context_size_rh(enum dp_context_type context_type);

#endif
