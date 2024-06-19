/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

#ifdef FEATURE_MGMT_RX_OVER_SRNG

#include "wlan_cmn.h"
#include "wlan_objmgr_cmn.h"
#include "wlan_objmgr_vdev_obj.h"
#include "wlan_objmgr_global_obj.h"
#include <wlan_mgmt_rx_srng_public_structs.h>

/**
 * struct mgmt_rx_srng_psoc_priv - mgmt rx srng component psoc priv obj
 * @psoc: pointer to psoc object
 * @mgmt_rx_srng_is_enable: is feature enabled by both host and target
 * @tx_ops: TX ops registered with target_if for southbound WMI cmds
 * @rx_ops: RX ops registered with target_if for northbound WMI events
 */
struct mgmt_rx_srng_psoc_priv {
	struct wlan_objmgr_psoc *psoc;
	bool mgmt_rx_srng_is_enable;
	struct wlan_mgmt_rx_srng_tx_ops tx_ops;
	struct wlan_mgmt_rx_srng_rx_ops rx_ops;
};

/**
 * struct mgmt_rx_srng_pdev_priv - mgmt_rx_srng component pdev priv
 * @pdev: pdev obj
 * @osdev: os dev
 * @hal_soc: opaque hal object
 * @read_idx: read index
 * @write_idx: write index
 */
struct mgmt_rx_srng_pdev_priv {
	struct wlan_objmgr_pdev *pdev;
	qdf_device_t osdev;
	void *hal_soc;
	int read_idx;
	int write_idx;
};

#endif
