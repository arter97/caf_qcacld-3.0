/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

#ifndef WLAN_MGMT_RX_SRNG_CFG_H__
#define WLAN_MGMT_RX_SRNG_CFG_H__

#ifdef FEATURE_MGMT_RX_OVER_SRNG
/*
 * <ini>
 * mgmt_rx_srng_enable- Control management packets over SRNG path
 * @Min: 0
 * @Max: 1
 * @Default: 0
 *
 * This ini is used to enable reception of mgmt packets over SRNG instead of CE
 *
 * Supported Feature: Management over SRNG
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_FEATURE_MGMT_RX_SRNG_ENABLE \
	CFG_INI_BOOL("mgmt_rx_srng_enable", false, \
		     "Enable/Disable mgmt packets reception over SRING path")

#define CFG_MGMT_RX_SRNG \
	CFG(CFG_FEATURE_MGMT_RX_SRNG_ENABLE) \
#else
#define CFG_MGMT_RX_SRNG
#endif

#endif /* WLAN_MGMT_RX_SRNG_CFG_H__*/
