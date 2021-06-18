/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
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

/**
 * DOC: This file has main repeater structures.
 */

#ifndef _WLAN_REPEATER_INTERNAL_H_
#define _WLAN_REPEATER_INTERNAL_H_

#include <wlan_repeater_api.h>

#define RPTR_MAX_RADIO_CNT 3

#define RPTR_GLOBAL_LOCK(_x)  qdf_spin_lock_bh(_x)
#define RPTR_GLOBAL_UNLOCK(_x)  qdf_spin_unlock_bh(_x)

#define ROOTAP_ACCESS_MASK 0x0F
#define STAVAP_CONNECTION_MASK 0xF0

/**
 * AP preference type for same ssid feature
 * 0 - No preference
 * 1 - connect to RootAP
 * 2 - connect to RE, whose bssid matches with preferred_list_stavap
 */
enum ap_preference_type {
    ap_preference_type_init = 0,
    ap_preference_type_root = 1,
    ap_preference_type_rptr = 2,
};

typedef struct same_ssid_feature {
	u8      extender_info;
	/* 0x00->no stavap connection and RootAP access on Repeater AP
	 * 0x0F->invalid state
	 * 0xF0->stavap connection is there,but no rootap access on Repeater AP
	 * 0xFF->stavap connection and rootap access is there on Repeater AP
	 */
	u8      num_rptr_clients;
	u8 preferred_bssid_list[RPTR_MAX_RADIO_CNT][QDF_MAC_ADDR_SIZE];
	/*list of AP bssid mac to which RE STAVAP has to connect*/
	u8 denied_client_list[RPTR_MAX_RADIO_CNT][QDF_MAC_ADDR_SIZE];
	/*list of RE STAVAP mac to which AP has to deauth*/
	enum ap_preference_type ap_preference;
	bool         same_ssid_disable;     /*disable same ssid feature */
	systime_t    rootap_access_downtime;
} same_ssid_feature_t;

struct wlan_rptr_global_priv {
	u32 global_feature_caps;
	same_ssid_feature_t   ss_info;
	u8     num_stavaps_up;
	struct rptr_ext_cbacks ext_cbacks;    /*Repeater extended cbacks */
	qdf_spinlock_t  rptr_global_lock;
};

struct wlan_rptr_psoc_priv {
	struct wlan_objmgr_psoc *psoc;
	u32 psoc_feature_caps;
};

/**
 * Extender connection type for same ssid feature
 * 0 - init value
 * 1 - connecting RE has no RootAP access
 * 2 - connecting RE has RootAP access
 */
enum ext_connection_type {
    ext_connection_type_init = 0,
    ext_connection_type_no_root_access = 1,
    ext_connection_type_root_access = 2,
};

struct wlan_rptr_pdev_priv {
	struct wlan_objmgr_pdev  *pdev;
	u32    pdev_feature_caps;
	enum ext_connection_type  extender_connection;
	u8     preferred_bssid[QDF_MAC_ADDR_SIZE];/*for same ssid feature*/
	u8     preferredUplink;   /*for Tri-radio fastlane feature*/
	u8     nscanpsta;
};

struct wlan_rptr_vdev_priv {
	struct wlan_objmgr_vdev *vdev;
	u32    vdev_feature_caps;
	u8     vdev_allow_events;
};

struct wlan_rptr_peer_priv {
	struct wlan_objmgr_peer *peer;
	u8     is_extender_client; /*for same ssid feature*/
};

QDF_STATUS
wlan_rptr_core_register_ext_cb(struct rptr_ext_cbacks *ext_cbacks);
void wlan_rptr_core_vdev_feat_cap_set(struct wlan_objmgr_vdev *vdev,
				      uint32_t cap);
void wlan_rptr_core_vdev_feat_cap_clear(struct wlan_objmgr_vdev *vdev,
					uint32_t cap);
uint8_t wlan_rptr_core_vdev_feat_cap_get(struct wlan_objmgr_vdev *vdev,
					 uint32_t cap);
void wlan_rptr_core_pdev_feat_cap_set(struct wlan_objmgr_pdev *pdev,
				      uint32_t cap);
void wlan_rptr_core_pdev_feat_cap_clear(struct wlan_objmgr_pdev *pdev,
					uint32_t cap);
uint8_t wlan_rptr_core_pdev_feat_cap_get(struct wlan_objmgr_pdev *pdev,
					 uint32_t cap);
void wlan_rptr_core_psoc_feat_cap_set(struct wlan_objmgr_psoc *psoc,
				      uint32_t cap);
void wlan_rptr_core_psoc_feat_cap_clear(struct wlan_objmgr_psoc *psoc,
					uint32_t cap);
uint8_t wlan_rptr_core_psoc_feat_cap_get(struct wlan_objmgr_psoc *psoc,
					 uint32_t cap);
void wlan_rptr_core_global_feat_cap_set(uint32_t cap);
void wlan_rptr_core_global_feat_cap_clear(uint32_t cap);
uint8_t wlan_rptr_core_global_feat_cap_get(uint32_t cap);
void wlan_rptr_core_reset_pdev_flags(struct wlan_objmgr_pdev *pdev);
void wlan_rptr_core_reset_global_flags(void);
struct wlan_rptr_global_priv *wlan_rptr_get_global_ctx(void);
void wlan_rptr_core_global_same_ssid_disable(u_int32_t value);
void wlan_rptr_core_ss_parse_scan_entries(struct wlan_objmgr_vdev *vdev,
					  struct scan_event *event);
QDF_STATUS
wlan_rptr_core_validate_stavap_bssid(struct wlan_objmgr_vdev *vdev,
				     u_int8_t *bssid);
struct wlan_rptr_pdev_priv *
wlan_rptr_get_pdev_priv(struct wlan_objmgr_pdev *pdev);
struct wlan_rptr_psoc_priv *
wlan_rptr_get_psoc_priv(struct wlan_objmgr_psoc *psoc);
struct wlan_rptr_vdev_priv *
wlan_rptr_get_vdev_priv(struct wlan_objmgr_vdev *vdev);
struct wlan_rptr_peer_priv *
wlan_rptr_get_peer_priv(struct wlan_objmgr_peer *peer);
#endif
