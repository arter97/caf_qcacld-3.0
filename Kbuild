# We can build either as part of a standalone Kernel build or as
# an external module.  Determine which mechanism is being used
ifeq ($(MODNAME),)
	KERNEL_BUILD := y
else
	KERNEL_BUILD := n
endif

ifeq ($(KERNEL_BUILD), y)
	# These are provided in external module based builds
	# Need to explicitly define for Kernel-based builds
	MODNAME := wlan
	WLAN_ROOT := drivers/staging/qcacld-3.0
	WLAN_COMMON_ROOT := cmn
	WLAN_COMMON_INC := $(WLAN_ROOT)/$(WLAN_COMMON_ROOT)
	WLAN_FW_API := $(WLAN_ROOT)/../fw-api/
	WLAN_PROFILE := default
endif

WLAN_COMMON_ROOT ?= cmn
WLAN_COMMON_INC ?= $(WLAN_ROOT)/$(WLAN_COMMON_ROOT)
WLAN_FW_API ?= $(WLAN_ROOT)/../fw-api/
WLAN_PROFILE ?= default
CONFIG_QCA_CLD_WLAN_PROFILE ?= $(WLAN_PROFILE)
DEVNAME ?= wlan
WLAN_PLATFORM_INC ?= $(WLAN_ROOT)/../platform/inc
DATA_IPA_INC ?= $(WLAN_ROOT)/../dataipa/drivers/platform/msm/include
DATA_IPA_UAPI_INC ?= $(DATA_IPA_INC)/uapi

ifeq ($(KERNEL_BUILD), n)
ifneq ($(ANDROID_BUILD_TOP),)
      ANDROID_BUILD_TOP_REL := $(shell python -c "import os.path; print(os.path.relpath('$(ANDROID_BUILD_TOP)'))")
      override WLAN_ROOT := $(ANDROID_BUILD_TOP_REL)/$(WLAN_ROOT)
      override WLAN_COMMON_INC := $(ANDROID_BUILD_TOP_REL)/$(WLAN_COMMON_INC)
      override WLAN_FW_API := $(ANDROID_BUILD_TOP_REL)/$(WLAN_FW_API)
else ifneq ($(LINUX_BUILD_TOP),)
      LINUX_BUILD_TOP_REL := $(shell python -c "import os.path; print(os.path.relpath('$(LINUX_BUILD_TOP)'))")
      $(warning "LINUX_BUILD_TOP_REL=: $(LINUX_BUILD_TOP_REL)")
      override WLAN_ROOT := $(LINUX_BUILD_TOP_REL)/qcacld-3.0
      override WLAN_COMMON_ROOT ?= cmn
      override WLAN_COMMON_INC ?= $(WLAN_ROOT)/$(WLAN_COMMON_ROOT)
      override WLAN_FW_API := $(WLAN_ROOT)/../fw-api
endif
endif

include $(WLAN_ROOT)/configs/$(CONFIG_QCA_CLD_WLAN_PROFILE)_defconfig

# add configurations in WLAN_CFG_OVERRIDE
$(foreach cfg, $(WLAN_CFG_OVERRIDE), \
	$(eval $(cfg)) \
	$(warning "Overriding WLAN config with: $(cfg)"))

# KERNEL_SUPPORTS_NESTED_COMPOSITES := y is used to enable nested
# composite support. The nested composite support is available in some
# MSM kernels, and is available in 5.10 GKI kernels beginning with
# 5.10.20, but unfortunately is not available in any upstream kernel.
#
# When the feature is present in an MSM kernel, the flag is explicitly
# set in the kernel sources.  When a GKI kernel is used, there isn't a
# flag set in the sources, so set the flag here if we are building
# with GKI kernel where the feature is present
KERNEL_VERSION = $(shell echo $$(( ( $1 << 16 ) + ( $2 << 8 ) + $3 )))
LINUX_CODE := $(call KERNEL_VERSION,$(VERSION),$(PATCHLEVEL),$(SUBLEVEL))

# Comosite feature was added to GKI 5.10.20
COMPOSITE_CODE_ADDED := 330260 # hardcoded $(call KERNEL_VERSION,5,10,20)

# Comosite feature was not ported beyond 5.10.x
COMPOSITE_CODE_REMOVED := 330496 # hardcoded $(call KERNEL_VERSION,5,11,0)

ifeq ($(KERNEL_SUPPORTS_NESTED_COMPOSITES),)
  #flag is not explicitly present
  ifneq ($(findstring gki,$(CONFIG_LOCALVERSION))$(findstring qki,$(CONFIG_LOCALVERSION)),)
    # GKI kernel
    ifeq ($(shell test $(LINUX_CODE) -ge $(COMPOSITE_CODE_ADDED); echo $$?),0)
      # version >= 5.10.20
      ifeq ($(shell test $(LINUX_CODE) -lt $(COMPOSITE_CODE_REMOVED); echo $$?),0)
        # version < 5.11.0
        KERNEL_SUPPORTS_NESTED_COMPOSITES := y
      endif
    endif
  endif
endif

OBJS :=
OBJS_DIRS :=

define add-wlan-objs
$(eval $(_add-wlan-objs))
endef

define _add-wlan-objs
  ifneq ($(2),)
    ifeq ($(KERNEL_SUPPORTS_NESTED_COMPOSITES),y)
      OBJS_DIRS += $(dir $(2))
      OBJS += $(1).o
      $(1)-y := $(2)
    else
      OBJS += $(2)
    endif
  endif
endef

############ UAPI ############
UAPI_DIR :=	uapi
UAPI_INC :=	-I$(WLAN_ROOT)/$(UAPI_DIR)/linux

############ COMMON ############
COMMON_DIR :=	core/common
COMMON_INC :=	-I$(WLAN_ROOT)/$(COMMON_DIR)

############ HDD ############
HDD_DIR :=	core/hdd
HDD_INC_DIR :=	$(HDD_DIR)/inc
HDD_SRC_DIR :=	$(HDD_DIR)/src

HDD_INC := 	-I$(WLAN_ROOT)/$(HDD_INC_DIR) \
		-I$(WLAN_ROOT)/$(HDD_SRC_DIR)

HDD_OBJS := 	$(HDD_SRC_DIR)/wlan_hdd_assoc.o \
		$(HDD_SRC_DIR)/wlan_hdd_cfg.o \
		$(HDD_SRC_DIR)/wlan_hdd_cfg80211.o \
		$(HDD_SRC_DIR)/wlan_hdd_data_stall_detection.o \
		$(HDD_SRC_DIR)/wlan_hdd_driver_ops.o \
		$(HDD_SRC_DIR)/wlan_hdd_ftm.o \
		$(HDD_SRC_DIR)/wlan_hdd_hostapd.o \
		$(HDD_SRC_DIR)/wlan_hdd_ioctl.o \
		$(HDD_SRC_DIR)/wlan_hdd_main.o \
		$(HDD_SRC_DIR)/wlan_hdd_object_manager.o \
		$(HDD_SRC_DIR)/wlan_hdd_oemdata.o \
		$(HDD_SRC_DIR)/wlan_hdd_p2p.o \
		$(HDD_SRC_DIR)/wlan_hdd_power.o \
		$(HDD_SRC_DIR)/wlan_hdd_regulatory.o \
		$(HDD_SRC_DIR)/wlan_hdd_scan.o \
		$(HDD_SRC_DIR)/wlan_hdd_softap_tx_rx.o \
		$(HDD_SRC_DIR)/wlan_hdd_sta_info.o \
		$(HDD_SRC_DIR)/wlan_hdd_stats.o \
		$(HDD_SRC_DIR)/wlan_hdd_trace.o \
		$(HDD_SRC_DIR)/wlan_hdd_tx_rx.o \
		$(HDD_SRC_DIR)/wlan_hdd_wmm.o \
		$(HDD_SRC_DIR)/wlan_hdd_wowl.o\
		$(HDD_SRC_DIR)/wlan_hdd_ll_lt_sap.o\

ifeq ($(CONFIG_UNIT_TEST), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_unit_test.o
endif

ifeq ($(CONFIG_WLAN_WEXT_SUPPORT_ENABLE), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_wext.o \
	    $(HDD_SRC_DIR)/wlan_hdd_hostapd_wext.o
endif

ifeq ($(CONFIG_AFC_SUPPORT), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_afc.o
endif

ifeq ($(CONFIG_DCS), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_dcs.o
endif

ifeq ($(CONFIG_FEATURE_WLAN_EXTSCAN), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_ext_scan.o
endif

ifeq ($(CONFIG_WLAN_DEBUGFS), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_debugfs.o
ifeq ($(CONFIG_WLAN_FEATURE_LINK_LAYER_STATS), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_debugfs_llstat.o
endif
ifeq ($(CONFIG_WLAN_FEATURE_MIB_STATS), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_debugfs_mibstat.o
endif
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_debugfs_csr.o
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_debugfs_offload.o
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_debugfs_roam.o
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_debugfs_config.o
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_debugfs_unit_test.o
ifeq ($(CONFIG_WLAN_MWS_INFO_DEBUGFS), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_debugfs_coex.o
endif
endif

ifeq ($(CONFIG_WLAN_CONV_SPECTRAL_ENABLE),y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_spectralscan.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_DSRC), y)
HDD_OBJS+=	$(HDD_SRC_DIR)/wlan_hdd_ocb.o
endif

ifeq ($(CONFIG_FEATURE_MEMDUMP_ENABLE), y)
HDD_OBJS+=	$(HDD_SRC_DIR)/wlan_hdd_memdump.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_FIPS), y)
HDD_OBJS+=	$(HDD_SRC_DIR)/wlan_hdd_fips.o
endif

ifeq ($(CONFIG_QCACLD_FEATURE_GREEN_AP), y)
HDD_OBJS+=	$(HDD_SRC_DIR)/wlan_hdd_green_ap.o
endif

ifeq ($(CONFIG_QCACLD_FEATURE_APF), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_apf.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_LPSS), y)
HDD_OBJS +=	$(HDD_SRC_DIR)/wlan_hdd_lpass.o
endif

ifeq ($(CONFIG_WLAN_NAPI), y)
HDD_OBJS +=     $(HDD_SRC_DIR)/wlan_hdd_napi.o
endif

ifeq ($(CONFIG_IPA_OFFLOAD), y)
HDD_OBJS +=	$(HDD_SRC_DIR)/wlan_hdd_ipa.o
endif

ifeq ($(CONFIG_QCACLD_FEATURE_SON), y)
HDD_OBJS +=	$(HDD_SRC_DIR)/wlan_hdd_son.o
endif

ifeq ($(CONFIG_QCACLD_FEATURE_NAN), y)
HDD_OBJS +=	$(HDD_SRC_DIR)/wlan_hdd_nan.o
HDD_OBJS +=	$(HDD_SRC_DIR)/wlan_hdd_nan_datapath.o
endif

ifeq ($(CONFIG_QCOM_TDLS), y)
HDD_OBJS +=	$(HDD_SRC_DIR)/wlan_hdd_tdls.o
endif

ifeq ($(CONFIG_WLAN_SYNC_TSF_PLUS), y)
CONFIG_WLAN_SYNC_TSF := y
endif

ifeq ($(CONFIG_WLAN_SYNC_TSF), y)
HDD_OBJS +=	$(HDD_SRC_DIR)/wlan_hdd_tsf.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_DISA), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_disa.o
endif

ifeq ($(CONFIG_LFR_SUBNET_DETECTION), y)
HDD_OBJS +=	$(HDD_SRC_DIR)/wlan_hdd_subnet_detect.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_11AX), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_he.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_11BE), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_eht.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_TWT), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_twt.o
endif

ifeq ($(CONFIG_FEATURE_MONITOR_MODE_SUPPORT), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_rx_monitor.o
endif

ifeq ($(CONFIG_WLAN_NUD_TRACKING), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_nud_tracking.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_PACKET_FILTERING), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_packet_filter.o
endif

ifeq ($(CONFIG_FEATURE_RSSI_MONITOR), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_rssi_monitor.o
endif

ifeq ($(CONFIG_FEATURE_BSS_TRANSITION), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_bss_transition.o
endif

ifeq ($(CONFIG_FEATURE_STATION_INFO), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_station_info.o
endif

ifeq ($(CONFIG_FEATURE_TX_POWER), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_tx_power.o
endif

ifeq ($(CONFIG_FEATURE_OTA_TEST), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_ota_test.o
endif

ifeq ($(CONFIG_FEATURE_ACTIVE_TOS), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_active_tos.o
endif

ifeq ($(CONFIG_FEATURE_SAR_LIMITS), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sar_limits.o
endif

ifeq ($(CONFIG_FEATURE_CONCURRENCY_MATRIX), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_concurrency_matrix.o
endif

ifeq ($(CONFIG_FEATURE_SAP_COND_CHAN_SWITCH), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sap_cond_chan_switch.o
endif

ifeq ($(CONFIG_FEATURE_P2P_LISTEN_OFFLOAD), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_p2p_listen_offload.o
endif

ifeq ($(CONFIG_WLAN_SYSFS), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs.o
ifeq ($(CONFIG_WLAN_SYSFS_CHANNEL), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_channel.o
endif
ifeq ($(CONFIG_WLAN_SYSFS_FW_MODE_CFG), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_fw_mode_config.o
endif
ifeq ($(CONFIG_WLAN_REASSOC), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_reassoc.o
endif
ifeq ($(CONFIG_WLAN_SYSFS_STA_INFO), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_sta_info.o
endif
ifeq ($(CONFIG_WLAN_DEBUG_CRASH_INJECT), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_crash_inject.o
endif
ifeq ($(CONFIG_FEATURE_UNIT_TEST_SUSPEND), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_suspend_resume.o
endif
ifeq ($(CONFIG_WLAN_SYSFS_MEM_STATS), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_mem_stats.o
endif
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_unit_test.o
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_modify_acl.o
ifeq ($(CONFIG_WLAN_SYSFS_CONNECT_INFO), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_connect_info.o
endif
ifeq ($(CONFIG_WLAN_SCAN_DISABLE), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_scan_disable.o
endif
ifeq ($(CONFIG_WLAN_SYSFS_DCM), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_dcm.o
endif
ifeq ($(CONFIG_WLAN_WOW_ITO), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_wow_ito.o
endif
ifeq ($(CONFIG_WLAN_WOWL_ADD_PTRN), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_wowl_add_ptrn.o
endif
ifeq ($(CONFIG_WLAN_WOWL_DEL_PTRN), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_wowl_del_ptrn.o
endif
ifeq ($(CONFIG_WLAN_SYSFS_TX_STBC), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_tx_stbc.o
endif
ifeq ($(CONFIG_WLAN_SYSFS_SCAN_CFG), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_scan_config.o
endif
ifeq ($(CONFIG_WLAN_SYSFS_MONITOR_MODE_CHANNEL), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_monitor_mode_channel.o
endif
ifeq ($(CONFIG_WLAN_SYSFS_RANGE_EXT), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_range_ext.o
endif
ifeq ($(CONFIG_WLAN_SYSFS_RADAR), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_radar.o
endif
ifeq ($(CONFIG_WLAN_SYSFS_RTS_CTS), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_rts_cts.o
endif
ifeq ($(CONFIG_WLAN_SYSFS_HE_BSS_COLOR), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_he_bss_color.o
endif
ifeq ($(CONFIG_WLAN_TXRX_FW_STATS), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_txrx_fw_stats.o
endif
ifeq ($(CONFIG_WLAN_TXRX_STATS), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_txrx_stats.o
endif
ifeq ($(CONFIG_WLAN_SYSFS_DP_TRACE), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_dp_trace.o
endif
ifeq ($(CONFIG_WLAN_SYSFS_STATS), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_stats.o
endif
ifeq ($(CONFIG_WLAN_SYSFS_TDLS_PEERS), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_tdls_peers.o
endif
ifeq ($(CONFIG_WLAN_SYSFS_TEMPERATURE), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_temperature.o
endif
ifeq ($(CONFIG_WLAN_THERMAL_CFG), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_thermal_cfg.o
endif
ifeq ($(CONFIG_FEATURE_MOTION_DETECTION), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_motion_detection.o
endif
ifeq ($(CONFIG_WLAN_SYSFS_WLAN_DBG), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_wlan_dbg.o
endif
ifeq ($(CONFIG_WLAN_TXRX_FW_ST_RST), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_txrx_fw_st_rst.o
endif
ifeq ($(CONFIG_WLAN_GTX_BW_MASK), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_gtx_bw_mask.o
endif
ifeq ($(CONFIG_IPA_OFFLOAD), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_ipa.o
endif
ifeq ($(CONFIG_REMOVE_PKT_LOG), n)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_pktlog.o
endif
ifeq ($(CONFIG_WLAN_DL_MODES), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_dl_modes.o
endif
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_policy_mgr.o
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_dp_aggregation.o
ifeq ($(CONFIG_DP_SWLM), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_swlm.o
endif
ifeq ($(CONFIG_WLAN_DUMP_IN_PROGRESS), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_dump_in_progress.o
endif
ifeq ($(CONFIG_FEATURE_SET), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_wifi_features.o
endif
ifeq ($(CONFIG_WLAN_BMISS), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_bmiss.o
endif
ifeq ($(CONFIG_WLAN_FREQ_LIST), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_get_freq_for_pwr.o
endif
ifeq ($(CONFIG_WLAN_SYSFS_DP_STATS), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_txrx_stats_console.o
endif

ifeq ($(CONFIG_DP_PKT_ADD_TIMESTAMP), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_add_timestamp.o
endif

ifeq ($(CONFIG_DP_TRAFFIC_END_INDICATION), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_dp_traffic_end_indication.o
endif

ifeq ($(CONFIG_DP_HW_TX_DELAY_STATS_ENABLE), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_dp_tx_delay_stats.o
endif

ifeq ($(CONFIG_WLAN_SYSFS_EHT_RATE), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_eht_rate.o
endif

ifeq ($(CONFIG_WLAN_SYSFS_BITRATES), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_bitrates.o
endif

ifeq ($(CONFIG_FEATURE_DIRECT_LINK), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_direct_link_ut_cmd.o
endif

ifeq ($(CONFIG_BUS_AUTO_SUSPEND), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_runtime_pm.o
endif

ifeq ($(CONFIG_WLAN_SYSFS_LOG_BUFFER), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_log_buffer.o
endif

ifeq ($(CONFIG_WLAN_SYSFS_DFSNOL), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_dfsnol.o
endif

ifeq ($(CONFIG_WLAN_SYSFS_WDS_MODE), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_wds_mode.o
endif

ifeq ($(CONFIG_WLAN_SYSFS_ROAM_TRIGGER_BITMAP), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_roam_trigger_bitmap.o
endif

ifeq ($(CONFIG_WLAN_SYSFS_RF_TEST_MODE), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_sysfs_rf_test_mode.o
endif

endif # CONFIG_WLAN_SYSFS

ifeq ($(CONFIG_QCACLD_FEATURE_FW_STATE), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_fw_state.o
endif

ifeq ($(CONFIG_QCACLD_FEATURE_COEX_CONFIG), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_coex_config.o
endif

ifeq ($(CONFIG_QCACLD_FEATURE_MPTA_HELPER), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_mpta_helper.o
endif

ifeq ($(CONFIG_WLAN_BCN_RECV_FEATURE), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_bcn_recv.o
endif

ifeq ($(CONFIG_QCACLD_FEATURE_HW_CAPABILITY), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_hw_capability.o
endif

ifeq ($(CONFIG_FW_THERMAL_THROTTLE), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_thermal.o
endif

ifeq ($(CONFIG_QCACLD_FEATURE_BTC_CHAIN_MODE), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_btc_chain_mode.o
endif

ifeq ($(CONFIG_FEATURE_WLAN_TIME_SYNC_FTM), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_ftm_time_sync.o
endif

ifeq ($(CONFIG_WLAN_HANG_EVENT), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_hang_event.o
endif

ifeq ($(CONFIG_WLAN_CFR_ENABLE), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_cfr.o
endif

ifeq ($(CONFIG_FEATURE_GPIO_CFG),y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_gpio.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_MEDIUM_ASSESS), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_medium_assess.o
endif

ifeq ($(CONFIG_WLAN_ENABLE_GPIO_WAKEUP),y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_gpio_wakeup.o
endif

HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_cm_connect.o
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_cm_disconnect.o


ifeq ($(CONFIG_WLAN_BOOTUP_MARKER), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_bootup_marker.o
endif

ifeq ($(CONFIG_FEATURE_WLAN_CH_AVOID_EXT),y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_avoid_freq_ext.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_11BE_MLO), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_mlo.o
endif


ifeq ($(CONFIG_WLAN_FEATURE_MDNS_OFFLOAD),y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_mdns_offload.o
endif

ifeq ($(CONFIG_QCACLD_WLAN_CONNECTIVITY_DIAG_EVENT), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_connectivity_logging.o
else ifeq ($(CONFIG_QCACLD_WLAN_CONNECTIVITY_LOGGING), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_connectivity_logging.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_MCC_QUOTA), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_mcc_quota.o
endif

ifeq ($(CONFIG_FEATURE_WDS), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_wds.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_PEER_TXQ_FLUSH_CONF), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_peer_txq_flush.o
endif

ifeq ($(CONFIG_WIFI_POS_CONVERGED), y)
ifeq ($(CONFIG_WIFI_POS_PASN), y)
HDD_OBJS += $(HDD_SRC_DIR)/wlan_hdd_wifi_pos_pasn.o
endif
endif

$(call add-wlan-objs,hdd,$(HDD_OBJS))

###### OSIF_SYNC ########
SYNC_DIR := os_if/sync
SYNC_INC_DIR := $(SYNC_DIR)/inc
SYNC_SRC_DIR := $(SYNC_DIR)/src

SYNC_INC := \
	-I$(WLAN_ROOT)/$(SYNC_INC_DIR) \
	-I$(WLAN_ROOT)/$(SYNC_SRC_DIR) \

SYNC_OBJS := \
	$(SYNC_SRC_DIR)/osif_sync.o \
	$(SYNC_SRC_DIR)/osif_driver_sync.o \
	$(SYNC_SRC_DIR)/osif_psoc_sync.o \
	$(SYNC_SRC_DIR)/osif_vdev_sync.o \

$(call add-wlan-objs,sync,$(SYNC_OBJS))

########### Driver Synchronization Core (DSC) ###########
DSC_DIR := components/dsc
DSC_INC_DIR := $(DSC_DIR)/inc
DSC_SRC_DIR := $(DSC_DIR)/src
DSC_TEST_DIR := $(DSC_DIR)/test

DSC_INC := \
	-I$(WLAN_ROOT)/$(DSC_INC_DIR) \
	-I$(WLAN_ROOT)/$(DSC_SRC_DIR) \
	-I$(WLAN_ROOT)/$(DSC_TEST_DIR)

DSC_OBJS := \
	$(DSC_SRC_DIR)/__wlan_dsc.o \
	$(DSC_SRC_DIR)/wlan_dsc_driver.o \
	$(DSC_SRC_DIR)/wlan_dsc_psoc.o \
	$(DSC_SRC_DIR)/wlan_dsc_vdev.o

ifeq ($(CONFIG_DSC_TEST), y)
	DSC_OBJS += $(DSC_TEST_DIR)/wlan_dsc_test.o
endif

$(call add-wlan-objs,dsc,$(DSC_OBJS))

########### HOST DIAG LOG ###########
HOST_DIAG_LOG_DIR :=	$(WLAN_COMMON_ROOT)/utils/host_diag_log

HOST_DIAG_LOG_INC_DIR :=	$(HOST_DIAG_LOG_DIR)/inc
HOST_DIAG_LOG_SRC_DIR :=	$(HOST_DIAG_LOG_DIR)/src

HOST_DIAG_LOG_INC :=	-I$(WLAN_ROOT)/$(HOST_DIAG_LOG_INC_DIR) \
			-I$(WLAN_ROOT)/$(HOST_DIAG_LOG_SRC_DIR)

ifeq ($(CONFIG_WLAN_DIAG_VERSION), y)
HOST_DIAG_LOG_OBJS +=	$(HOST_DIAG_LOG_SRC_DIR)/host_diag_log.o
endif

$(call add-wlan-objs,host_diag_log,$(HOST_DIAG_LOG_OBJS))

############ EPPING ############
EPPING_DIR :=	$(WLAN_COMMON_ROOT)/utils/epping
EPPING_INC_DIR :=	$(EPPING_DIR)/inc
EPPING_SRC_DIR :=	$(EPPING_DIR)/src

EPPING_INC := 	-I$(WLAN_ROOT)/$(EPPING_INC_DIR)

ifeq ($(CONFIG_FEATURE_EPPING), y)
EPPING_OBJS := $(EPPING_SRC_DIR)/epping_main.o \
		$(EPPING_SRC_DIR)/epping_txrx.o \
		$(EPPING_SRC_DIR)/epping_tx.o \
		$(EPPING_SRC_DIR)/epping_rx.o \
		$(EPPING_SRC_DIR)/epping_helper.o
endif

$(call add-wlan-objs,epping,$(EPPING_OBJS))

############ SYS ############
CMN_SYS_DIR :=	$(WLAN_COMMON_ROOT)/utils/sys
CMN_SYS_INC_DIR := 	$(CMN_SYS_DIR)
CMN_SYS_INC :=	-I$(WLAN_ROOT)/$(CMN_SYS_INC_DIR)

############ MAC ############
MAC_DIR :=	core/mac
MAC_INC_DIR :=	$(MAC_DIR)/inc
MAC_SRC_DIR :=	$(MAC_DIR)/src

MAC_INC := 	-I$(WLAN_ROOT)/$(MAC_INC_DIR) \
		-I$(WLAN_ROOT)/$(MAC_SRC_DIR)/dph \
		-I$(WLAN_ROOT)/$(MAC_SRC_DIR)/include \
		-I$(WLAN_ROOT)/$(MAC_SRC_DIR)/pe/include \
		-I$(WLAN_ROOT)/$(MAC_SRC_DIR)/pe/lim \
		-I$(WLAN_ROOT)/$(MAC_SRC_DIR)/pe/nan

MAC_DPH_OBJS :=	$(MAC_SRC_DIR)/dph/dph_hash_table.o

ifeq ($(KERNEL_SUPPORTS_NESTED_COMPOSITES),y)
MAC_LIM_OBJS := $(MAC_SRC_DIR)/pe/lim/lim_aid_mgmt.o \
		$(MAC_SRC_DIR)/pe/lim/lim_admit_control.o \
		$(MAC_SRC_DIR)/pe/lim/lim_api.o \
		$(MAC_SRC_DIR)/pe/lim/lim_assoc_utils.o \
		$(MAC_SRC_DIR)/pe/lim/lim_ft.o \
		$(MAC_SRC_DIR)/pe/lim/lim_link_monitoring_algo.o \
		$(MAC_SRC_DIR)/pe/lim/lim_process_action_frame.o \
		$(MAC_SRC_DIR)/pe/lim/lim_process_assoc_req_frame.o \
		$(MAC_SRC_DIR)/pe/lim/lim_process_assoc_rsp_frame.o \
		$(MAC_SRC_DIR)/pe/lim/lim_process_auth_frame.o \
		$(MAC_SRC_DIR)/pe/lim/lim_process_beacon_frame.o \
		$(MAC_SRC_DIR)/pe/lim/lim_process_cfg_updates.o \
		$(MAC_SRC_DIR)/pe/lim/lim_process_deauth_frame.o \
		$(MAC_SRC_DIR)/pe/lim/lim_process_disassoc_frame.o \
		$(MAC_SRC_DIR)/pe/lim/lim_process_message_queue.o \
		$(MAC_SRC_DIR)/pe/lim/lim_process_mlm_req_messages.o \
		$(MAC_SRC_DIR)/pe/lim/lim_process_mlm_rsp_messages.o \
		$(MAC_SRC_DIR)/pe/lim/lim_process_probe_req_frame.o \
		$(MAC_SRC_DIR)/pe/lim/lim_process_probe_rsp_frame.o \
		$(MAC_SRC_DIR)/pe/lim/lim_process_sme_req_messages.o \
		$(MAC_SRC_DIR)/pe/lim/lim_prop_exts_utils.o \
		$(MAC_SRC_DIR)/pe/lim/lim_scan_result_utils.o \
		$(MAC_SRC_DIR)/pe/lim/lim_security_utils.o \
		$(MAC_SRC_DIR)/pe/lim/lim_send_management_frames.o \
		$(MAC_SRC_DIR)/pe/lim/lim_send_messages.o \
		$(MAC_SRC_DIR)/pe/lim/lim_send_sme_rsp_messages.o \
		$(MAC_SRC_DIR)/pe/lim/lim_session.o \
		$(MAC_SRC_DIR)/pe/lim/lim_session_utils.o \
		$(MAC_SRC_DIR)/pe/lim/lim_sme_req_utils.o \
		$(MAC_SRC_DIR)/pe/lim/lim_timer_utils.o \
		$(MAC_SRC_DIR)/pe/lim/lim_trace.o \
		$(MAC_SRC_DIR)/pe/lim/lim_utils.o
else
#composite of all of the above is in lim.c
MAC_LIM_OBJS := $(MAC_SRC_DIR)/pe/lim/lim.o
endif

ifeq ($(CONFIG_QCOM_TDLS), y)
MAC_LIM_OBJS += $(MAC_SRC_DIR)/pe/lim/lim_process_tdls.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_FILS), y)
MAC_LIM_OBJS += $(MAC_SRC_DIR)/pe/lim/lim_process_fils.o
endif

ifeq ($(CONFIG_QCACLD_FEATURE_NAN), y)
MAC_NDP_OBJS += $(MAC_SRC_DIR)/pe/nan/nan_datapath.o
endif

ifeq ($(CONFIG_QCACLD_WLAN_LFR2), y)
	MAC_LIM_OBJS += $(MAC_SRC_DIR)/pe/lim/lim_process_mlm_host_roam.o \
		$(MAC_SRC_DIR)/pe/lim/lim_send_frames_host_roam.o \
		$(MAC_SRC_DIR)/pe/lim/lim_roam_timer_utils.o \
		$(MAC_SRC_DIR)/pe/lim/lim_ft_preauth.o \
		$(MAC_SRC_DIR)/pe/lim/lim_reassoc_utils.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_11BE_MLO), y)
	MAC_LIM_OBJS += $(MAC_SRC_DIR)/pe/lim/lim_mlo.o
endif

MAC_SCH_OBJS := $(MAC_SRC_DIR)/pe/sch/sch_api.o \
		$(MAC_SRC_DIR)/pe/sch/sch_beacon_gen.o \
		$(MAC_SRC_DIR)/pe/sch/sch_beacon_process.o \
		$(MAC_SRC_DIR)/pe/sch/sch_message.o

MAC_RRM_OBJS :=	$(MAC_SRC_DIR)/pe/rrm/rrm_api.o

MAC_OBJS := 	$(MAC_CFG_OBJS) \
		$(MAC_DPH_OBJS) \
		$(MAC_LIM_OBJS) \
		$(MAC_SCH_OBJS) \
		$(MAC_RRM_OBJS) \
		$(MAC_NDP_OBJS)

$(call add-wlan-objs,mac,$(MAC_OBJS))

############ SAP ############
SAP_DIR :=	core/sap
SAP_INC_DIR :=	$(SAP_DIR)/inc
SAP_SRC_DIR :=	$(SAP_DIR)/src

SAP_INC := 	-I$(WLAN_ROOT)/$(SAP_INC_DIR) \
		-I$(WLAN_ROOT)/$(SAP_SRC_DIR)

SAP_OBJS :=	$(SAP_SRC_DIR)/sap_api_link_cntl.o \
		$(SAP_SRC_DIR)/sap_ch_select.o \
		$(SAP_SRC_DIR)/sap_fsm.o \
		$(SAP_SRC_DIR)/sap_module.o

$(call add-wlan-objs,sap,$(SAP_OBJS))

############ CFG ############
CFG_REL_DIR := $(WLAN_COMMON_ROOT)/cfg
CFG_DIR := $(WLAN_ROOT)/$(CFG_REL_DIR)
CFG_INC := \
	-I$(CFG_DIR)/inc \
	-I$(CFG_DIR)/dispatcher/inc \
	-I$(WLAN_ROOT)/components/cfg
CFG_OBJS := \
	$(CFG_REL_DIR)/src/cfg.o

$(call add-wlan-objs,cfg,$(CFG_OBJS))

############ DFS ############
DFS_DIR :=     $(WLAN_COMMON_ROOT)/umac/dfs
DFS_CORE_INC_DIR := $(DFS_DIR)/core/inc
DFS_CORE_SRC_DIR := $(DFS_DIR)/core/src

DFS_DISP_INC_DIR := $(DFS_DIR)/dispatcher/inc
DFS_DISP_SRC_DIR := $(DFS_DIR)/dispatcher/src
DFS_TARGET_INC_DIR := $(WLAN_COMMON_ROOT)/target_if/dfs/inc
DFS_CMN_SERVICES_INC_DIR := $(WLAN_COMMON_ROOT)/umac/cmn_services/dfs/inc

DFS_INC :=	-I$(WLAN_ROOT)/$(DFS_DISP_INC_DIR) \
		-I$(WLAN_ROOT)/$(DFS_TARGET_INC_DIR) \
		-I$(WLAN_ROOT)/$(DFS_CMN_SERVICES_INC_DIR)

ifeq ($(CONFIG_WLAN_DFS_MASTER_ENABLE), y)

DFS_OBJS :=	$(DFS_CORE_SRC_DIR)/misc/dfs.o \
		$(DFS_CORE_SRC_DIR)/misc/dfs_nol.o \
		$(DFS_CORE_SRC_DIR)/misc/dfs_random_chan_sel.o \
		$(DFS_CORE_SRC_DIR)/misc/dfs_process_radar_found_ind.o \
		$(DFS_DISP_SRC_DIR)/wlan_dfs_init_deinit_api.o \
		$(DFS_DISP_SRC_DIR)/wlan_dfs_lmac_api.o \
		$(DFS_DISP_SRC_DIR)/wlan_dfs_mlme_api.o \
		$(DFS_DISP_SRC_DIR)/wlan_dfs_tgt_api.o \
		$(DFS_DISP_SRC_DIR)/wlan_dfs_ucfg_api.o \
		$(DFS_DISP_SRC_DIR)/wlan_dfs_tgt_api.o \
		$(DFS_DISP_SRC_DIR)/wlan_dfs_utils_api.o \
		$(WLAN_COMMON_ROOT)/target_if/dfs/src/target_if_dfs.o

ifeq ($(CONFIG_WLAN_FEATURE_DFS_OFFLOAD), y)
DFS_OBJS +=	$(WLAN_COMMON_ROOT)/target_if/dfs/src/target_if_dfs_full_offload.o
else
DFS_OBJS +=	$(WLAN_COMMON_ROOT)/target_if/dfs/src/target_if_dfs_partial_offload.o \
		$(DFS_CORE_SRC_DIR)/filtering/dfs_fcc_bin5.o \
		$(DFS_CORE_SRC_DIR)/filtering/dfs_bindetects.o \
		$(DFS_CORE_SRC_DIR)/filtering/dfs_debug.o \
		$(DFS_CORE_SRC_DIR)/filtering/dfs_init.o \
		$(DFS_CORE_SRC_DIR)/filtering/dfs_misc.o \
		$(DFS_CORE_SRC_DIR)/filtering/dfs_phyerr_tlv.o \
		$(DFS_CORE_SRC_DIR)/filtering/dfs_process_phyerr.o \
		$(DFS_CORE_SRC_DIR)/filtering/dfs_process_radarevent.o \
		$(DFS_CORE_SRC_DIR)/filtering/dfs_staggered.o \
		$(DFS_CORE_SRC_DIR)/filtering/dfs_radar.o \
		$(DFS_CORE_SRC_DIR)/filtering/dfs_partial_offload_radar.o \
		$(DFS_CORE_SRC_DIR)/misc/dfs_filter_init.o
endif
endif

$(call add-wlan-objs,dfs,$(DFS_OBJS))

############ SME ############
SME_DIR :=	core/sme
SME_INC_DIR :=	$(SME_DIR)/inc
SME_SRC_DIR :=	$(SME_DIR)/src

SME_INC := 	-I$(WLAN_ROOT)/$(SME_INC_DIR) \
		-I$(WLAN_ROOT)/$(SME_SRC_DIR)/csr \
		-I$(WLAN_ROOT)/$(SME_SRC_DIR)/qos \
		-I$(WLAN_ROOT)/$(SME_SRC_DIR)/common \
		-I$(WLAN_ROOT)/$(SME_SRC_DIR)/rrm \
		-I$(WLAN_ROOT)/$(SME_SRC_DIR)/nan

ifeq ($(KERNEL_SUPPORTS_NESTED_COMPOSITES),y)
SME_CSR_OBJS := $(SME_SRC_DIR)/csr/csr_api_roam.o \
		$(SME_SRC_DIR)/csr/csr_api_scan.o \
		$(SME_SRC_DIR)/csr/csr_cmd_process.o \
		$(SME_SRC_DIR)/csr/csr_link_list.o \
		$(SME_SRC_DIR)/csr/csr_util.o \

SME_QOS_OBJS := $(SME_SRC_DIR)/qos/sme_qos.o

SME_CMN_OBJS := $(SME_SRC_DIR)/common/sme_api.o \
		$(SME_SRC_DIR)/common/sme_power_save.o \
		$(SME_SRC_DIR)/common/sme_trace.o

SME_RRM_OBJS := $(SME_SRC_DIR)/rrm/sme_rrm.o

ifeq ($(CONFIG_QCACLD_FEATURE_NAN), y)
SME_NDP_OBJS += $(SME_SRC_DIR)/nan/nan_datapath_api.o
endif

SME_OBJS :=	$(SME_CMN_OBJS) \
		$(SME_CSR_OBJS) \
		$(SME_QOS_OBJS) \
		$(SME_RRM_OBJS) \
		$(SME_NAN_OBJS) \
		$(SME_NDP_OBJS)
else # KERNEL_SUPPORTS_NESTED_COMPOSITES
SME_OBJS := $(SME_SRC_DIR)/sme.o
endif
$(call add-wlan-objs,sme,$(SME_OBJS))

############ NLINK ############
NLINK_DIR     :=	$(WLAN_COMMON_ROOT)/utils/nlink
NLINK_INC_DIR :=	$(NLINK_DIR)/inc
NLINK_SRC_DIR :=	$(NLINK_DIR)/src

NLINK_INC     := 	-I$(WLAN_ROOT)/$(NLINK_INC_DIR)
NLINK_OBJS    :=	$(NLINK_SRC_DIR)/wlan_nlink_srv.o

$(call add-wlan-objs,nlink,$(NLINK_OBJS))

############ PTT ############
PTT_DIR     :=	$(WLAN_COMMON_ROOT)/utils/ptt
PTT_INC_DIR :=	$(PTT_DIR)/inc
PTT_SRC_DIR :=	$(PTT_DIR)/src

PTT_INC     := 	-I$(WLAN_ROOT)/$(PTT_INC_DIR)
PTT_OBJS    :=	$(PTT_SRC_DIR)/wlan_ptt_sock_svc.o

$(call add-wlan-objs,ptt,$(PTT_OBJS))

############ WLAN_LOGGING ############
WLAN_LOGGING_DIR     :=	$(WLAN_COMMON_ROOT)/utils/logging
WLAN_LOGGING_INC_DIR :=	$(WLAN_LOGGING_DIR)/inc
WLAN_LOGGING_SRC_DIR :=	$(WLAN_LOGGING_DIR)/src

WLAN_LOGGING_INC     := -I$(WLAN_ROOT)/$(WLAN_LOGGING_INC_DIR)
WLAN_LOGGING_OBJS    := $(WLAN_LOGGING_SRC_DIR)/wlan_logging_sock_svc.o \
		$(WLAN_LOGGING_SRC_DIR)/wlan_roam_debug.o

$(call add-wlan-objs,wlan_logging,$(WLAN_LOGGING_OBJS))

############ SYS ############
SYS_DIR :=	core/mac/src/sys

SYS_INC := 	-I$(WLAN_ROOT)/$(SYS_DIR)/common/inc \
		-I$(WLAN_ROOT)/$(SYS_DIR)/legacy/src/platform/inc \
		-I$(WLAN_ROOT)/$(SYS_DIR)/legacy/src/system/inc \
		-I$(WLAN_ROOT)/$(SYS_DIR)/legacy/src/utils/inc

SYS_COMMON_SRC_DIR := $(SYS_DIR)/common/src
SYS_LEGACY_SRC_DIR := $(SYS_DIR)/legacy/src
SYS_OBJS :=	$(SYS_COMMON_SRC_DIR)/wlan_qct_sys.o \
		$(SYS_LEGACY_SRC_DIR)/platform/src/sys_wrapper.o \
		$(SYS_LEGACY_SRC_DIR)/system/src/mac_init_api.o \
		$(SYS_LEGACY_SRC_DIR)/system/src/sys_entry_func.o \
		$(SYS_LEGACY_SRC_DIR)/utils/src/dot11f.o \
		$(SYS_LEGACY_SRC_DIR)/utils/src/mac_trace.o \
		$(SYS_LEGACY_SRC_DIR)/utils/src/parser_api.o \
		$(SYS_LEGACY_SRC_DIR)/utils/src/utils_parser.o

$(call add-wlan-objs,sys,$(SYS_OBJS))

############ Qcacld WMI ###################
WMI_DIR := components/wmi

CLD_WMI_INC  :=	-I$(WLAN_ROOT)/$(WMI_DIR)/inc

ifeq ($(CONFIG_WMI_ROAM_SUPPORT), y)
CLD_WMI_ROAM_OBJS +=	$(WMI_DIR)/src/wmi_unified_roam_tlv.o \
			$(WMI_DIR)/src/wmi_unified_roam_api.o
endif

ifeq ($(CONFIG_CP_STATS), y)
CLD_WMI_MC_CP_STATS_OBJS :=	$(WMI_DIR)/src/wmi_unified_mc_cp_stats_tlv.o \
				$(WMI_DIR)/src/wmi_unified_mc_cp_stats_api.o
endif

ifeq ($(CONFIG_QCA_TARGET_IF_MLME), y)
CLD_WMI_MLME_OBJS += $(WMI_DIR)/src/wmi_unified_mlme_tlv.o \
		     $(WMI_DIR)/src/wmi_unified_mlme_api.o
endif

CLD_WMI_OBJS :=	$(CLD_WMI_ROAM_OBJS) \
		$(CLD_WMI_MC_CP_STATS_OBJS) \
		$(CLD_WMI_MLME_OBJS)

$(call add-wlan-objs,cld_wmi,$(CLD_WMI_OBJS))

############ Qca-wifi-host-cmn ############
QDF_OS_DIR :=	qdf
QDF_OS_INC_DIR := $(QDF_OS_DIR)/inc
QDF_OS_SRC_DIR := $(QDF_OS_DIR)/src
QDF_OS_LINUX_SRC_DIR := $(QDF_OS_DIR)/linux/src
QDF_OBJ_DIR := $(WLAN_COMMON_ROOT)/$(QDF_OS_SRC_DIR)
QDF_LINUX_OBJ_DIR := $(WLAN_COMMON_ROOT)/$(QDF_OS_LINUX_SRC_DIR)
QDF_TEST_DIR := $(QDF_OS_DIR)/test
QDF_TEST_OBJ_DIR := $(WLAN_COMMON_ROOT)/$(QDF_TEST_DIR)

QDF_INC := \
	-I$(WLAN_COMMON_INC)/$(QDF_OS_INC_DIR) \
	-I$(WLAN_COMMON_INC)/$(QDF_OS_LINUX_SRC_DIR) \
	-I$(WLAN_COMMON_INC)/$(QDF_TEST_DIR)

QDF_OBJS := \
	$(QDF_LINUX_OBJ_DIR)/qdf_crypto.o \
	$(QDF_LINUX_OBJ_DIR)/qdf_defer.o \
	$(QDF_LINUX_OBJ_DIR)/qdf_delayed_work.o \
	$(QDF_LINUX_OBJ_DIR)/qdf_event.o \
	$(QDF_LINUX_OBJ_DIR)/qdf_file.o \
	$(QDF_LINUX_OBJ_DIR)/qdf_func_tracker.o \
	$(QDF_LINUX_OBJ_DIR)/qdf_idr.o \
	$(QDF_LINUX_OBJ_DIR)/qdf_list.o \
	$(QDF_LINUX_OBJ_DIR)/qdf_lock.o \
	$(QDF_LINUX_OBJ_DIR)/qdf_mc_timer.o \
	$(QDF_LINUX_OBJ_DIR)/qdf_mem.o \
	$(QDF_LINUX_OBJ_DIR)/qdf_nbuf.o \
	$(QDF_LINUX_OBJ_DIR)/qdf_periodic_work.o \
	$(QDF_LINUX_OBJ_DIR)/qdf_status.o \
	$(QDF_LINUX_OBJ_DIR)/qdf_threads.o \
	$(QDF_LINUX_OBJ_DIR)/qdf_trace.o \
	$(QDF_LINUX_OBJ_DIR)/qdf_nbuf_frag.o \
	$(QDF_OBJ_DIR)/qdf_flex_mem.o \
	$(QDF_OBJ_DIR)/qdf_parse.o \
	$(QDF_OBJ_DIR)/qdf_platform.o \
	$(QDF_OBJ_DIR)/qdf_str.o \
	$(QDF_OBJ_DIR)/qdf_talloc.o \
	$(QDF_OBJ_DIR)/qdf_types.o \

ifeq ($(CONFIG_CNSS2_SSR_DRIVER_DUMP), y)
QDF_OBJS += $(QDF_LINUX_OBJ_DIR)/qdf_ssr_driver_dump.o
endif

ifeq ($(CONFIG_WLAN_DEBUGFS), y)
QDF_OBJS += $(QDF_LINUX_OBJ_DIR)/qdf_debugfs.o
endif

ifeq ($(CONFIG_WLAN_TRACEPOINTS), y)
QDF_OBJS += $(QDF_LINUX_OBJ_DIR)/qdf_tracepoint.o
endif

ifeq ($(CONFIG_WLAN_STREAMFS), y)
QDF_OBJS += $(QDF_LINUX_OBJ_DIR)/qdf_streamfs.o
endif

ifeq ($(CONFIG_IPA_OFFLOAD), y)
QDF_OBJS += $(QDF_LINUX_OBJ_DIR)/qdf_ipa.o
endif

ifeq ($(CONFIG_DP_PKT_ADD_TIMESTAMP), y)
QDF_OBJS += $(QDF_LINUX_OBJ_DIR)/qdf_pkt_add_timestamp.o
endif

# enable CPU hotplug support if SMP is enabled
ifeq ($(CONFIG_SMP), y)
	QDF_OBJS += $(QDF_OBJ_DIR)/qdf_cpuhp.o
	QDF_OBJS += $(QDF_LINUX_OBJ_DIR)/qdf_cpuhp.o
endif

ifeq ($(CONFIG_LEAK_DETECTION), y)
	QDF_OBJS += $(QDF_OBJ_DIR)/qdf_debug_domain.o
	QDF_OBJS += $(QDF_OBJ_DIR)/qdf_tracker.o
endif

ifeq ($(CONFIG_WLAN_HANG_EVENT), y)
	QDF_OBJS += $(QDF_OBJ_DIR)/qdf_notifier.o
endif

ifeq ($(CONFIG_QDF_TEST), y)
	QDF_OBJS += $(QDF_TEST_OBJ_DIR)/qdf_delayed_work_test.o
	QDF_OBJS += $(QDF_TEST_OBJ_DIR)/qdf_hashtable_test.o
	QDF_OBJS += $(QDF_TEST_OBJ_DIR)/qdf_periodic_work_test.o
	QDF_OBJS += $(QDF_TEST_OBJ_DIR)/qdf_ptr_hash_test.o
	QDF_OBJS += $(QDF_TEST_OBJ_DIR)/qdf_slist_test.o
	QDF_OBJS += $(QDF_TEST_OBJ_DIR)/qdf_talloc_test.o
	QDF_OBJS += $(QDF_TEST_OBJ_DIR)/qdf_tracker_test.o
	QDF_OBJS += $(QDF_TEST_OBJ_DIR)/qdf_types_test.o
endif

ifeq ($(CONFIG_WLAN_HANG_EVENT), y)
	QDF_OBJS += $(QDF_OBJ_DIR)/qdf_hang_event_notifier.o
endif

ifeq ($(CONFIG_WLAN_LRO), y)
QDF_OBJS +=     $(QDF_LINUX_OBJ_DIR)/qdf_lro.o
endif

$(call add-wlan-objs,qdf,$(QDF_OBJS))

############ WBUFF ############
WBUFF_OS_DIR :=	wbuff
WBUFF_OS_INC_DIR := $(WBUFF_OS_DIR)/inc
WBUFF_OS_SRC_DIR := $(WBUFF_OS_DIR)/src
WBUFF_OBJ_DIR := $(WLAN_COMMON_ROOT)/$(WBUFF_OS_SRC_DIR)

WBUFF_INC :=	-I$(WLAN_COMMON_INC)/$(WBUFF_OS_INC_DIR) \

ifeq ($(CONFIG_WLAN_WBUFF), y)
WBUFF_OBJS += 	$(WBUFF_OBJ_DIR)/wbuff.o
endif

$(call add-wlan-objs,wbuff,$(WBUFF_OBJS))

##########QAL #######
QAL_OS_DIR :=	qal
QAL_OS_INC_DIR := $(QAL_OS_DIR)/inc
QAL_OS_LINUX_SRC_DIR := $(QAL_OS_DIR)/linux/src

QAL_INC :=	-I$(WLAN_COMMON_INC)/$(QAL_OS_INC_DIR) \
		-I$(WLAN_COMMON_INC)/$(QAL_OS_LINUX_SRC_DIR)


##########OS_IF #######
OS_IF_DIR := $(WLAN_COMMON_ROOT)/os_if

OS_IF_INC += -I$(WLAN_COMMON_INC)/os_if/linux \
            -I$(WLAN_COMMON_INC)/os_if/linux/scan/inc \
            -I$(WLAN_COMMON_INC)/os_if/linux/spectral/inc \
            -I$(WLAN_COMMON_INC)/os_if/linux/crypto/inc \
            -I$(WLAN_COMMON_INC)/os_if/linux/mlme/inc \
            -I$(WLAN_COMMON_INC)/os_if/linux/gpio/inc

OS_IF_OBJ += $(OS_IF_DIR)/linux/wlan_osif_request_manager.o \
	     $(OS_IF_DIR)/linux/crypto/src/wlan_nl_to_crypto_params.o \
	     $(OS_IF_DIR)/linux/mlme/src/osif_cm_util.o \
	     $(OS_IF_DIR)/linux/mlme/src/osif_cm_connect_rsp.o \
	     $(OS_IF_DIR)/linux/mlme/src/osif_cm_disconnect_rsp.o \
	     $(OS_IF_DIR)/linux/mlme/src/osif_cm_req.o \
	     $(OS_IF_DIR)/linux/mlme/src/osif_cm_roam_rsp.o \
	     $(OS_IF_DIR)/linux/mlme/src/osif_vdev_mgr_util.o

OS_IF_OBJ += $(OS_IF_DIR)/linux/crypto/src/wlan_cfg80211_crypto.o

$(call add-wlan-objs,os_if,$(OS_IF_OBJ))

############ UMAC_DISP ############
UMAC_DISP_DIR := umac/global_umac_dispatcher/lmac_if
UMAC_DISP_INC_DIR := $(UMAC_DISP_DIR)/inc
UMAC_DISP_SRC_DIR := $(UMAC_DISP_DIR)/src
UMAC_DISP_OBJ_DIR := $(WLAN_COMMON_ROOT)/$(UMAC_DISP_SRC_DIR)

UMAC_DISP_INC := -I$(WLAN_COMMON_INC)/$(UMAC_DISP_INC_DIR)

UMAC_DISP_OBJS := $(UMAC_DISP_OBJ_DIR)/wlan_lmac_if.o

$(call add-wlan-objs,umac_disp,$(UMAC_DISP_OBJS))

############# UMAC_SCAN ############
UMAC_SCAN_DIR := umac/scan
UMAC_SCAN_DISP_INC_DIR := $(UMAC_SCAN_DIR)/dispatcher/inc
UMAC_SCAN_CORE_DIR := $(WLAN_COMMON_ROOT)/$(UMAC_SCAN_DIR)/core/src
UMAC_SCAN_DISP_DIR := $(WLAN_COMMON_ROOT)/$(UMAC_SCAN_DIR)/dispatcher/src
UMAC_TARGET_SCAN_INC := -I$(WLAN_COMMON_INC)/target_if/scan/inc

UMAC_SCAN_INC := -I$(WLAN_COMMON_INC)/$(UMAC_SCAN_DISP_INC_DIR)
UMAC_SCAN_OBJS := $(UMAC_SCAN_CORE_DIR)/wlan_scan_cache_db.o \
		$(UMAC_SCAN_CORE_DIR)/wlan_scan_11d.o \
		$(UMAC_SCAN_CORE_DIR)/wlan_scan_filter.o \
		$(UMAC_SCAN_CORE_DIR)/wlan_scan_main.o \
		$(UMAC_SCAN_CORE_DIR)/wlan_scan_manager.o \
		$(UMAC_SCAN_DISP_DIR)/wlan_scan_tgt_api.o \
		$(UMAC_SCAN_DISP_DIR)/wlan_scan_ucfg_api.o \
		$(UMAC_SCAN_DISP_DIR)/wlan_scan_api.o \
		$(UMAC_SCAN_DISP_DIR)/wlan_scan_utils_api.o \
		$(WLAN_COMMON_ROOT)/os_if/linux/scan/src/wlan_cfg80211_scan.o \
		$(WLAN_COMMON_ROOT)/os_if/linux/wlan_cfg80211.o \
		$(WLAN_COMMON_ROOT)/target_if/scan/src/target_if_scan.o

ifeq ($(CONFIG_FEATURE_WLAN_EXTSCAN), y)
UMAC_SCAN_OBJS += $(UMAC_SCAN_DISP_DIR)/wlan_extscan_api.o
endif

ifeq ($(CONFIG_BAND_6GHZ), y)
UMAC_SCAN_OBJS += $(UMAC_SCAN_CORE_DIR)/wlan_scan_manager_6ghz.o
endif

$(call add-wlan-objs,umac_scan,$(UMAC_SCAN_OBJS))

############# UMAC_SPECTRAL_SCAN ############
UMAC_SPECTRAL_DIR := spectral
UMAC_SPECTRAL_DISP_INC_DIR := $(UMAC_SPECTRAL_DIR)/dispatcher/inc
UMAC_SPECTRAL_CORE_INC_DIR := $(UMAC_SPECTRAL_DIR)/core
UMAC_SPECTRAL_CORE_DIR := $(WLAN_COMMON_ROOT)/$(UMAC_SPECTRAL_DIR)/core
UMAC_SPECTRAL_DISP_DIR := $(WLAN_COMMON_ROOT)/$(UMAC_SPECTRAL_DIR)/dispatcher/src
UMAC_TARGET_SPECTRAL_INC := -I$(WLAN_COMMON_INC)/target_if/spectral

UMAC_SPECTRAL_INC := -I$(WLAN_COMMON_INC)/$(UMAC_SPECTRAL_DISP_INC_DIR) \
			-I$(WLAN_COMMON_INC)/$(UMAC_SPECTRAL_CORE_INC_DIR) \
			-I$(WLAN_COMMON_INC)/target_if/direct_buf_rx/inc
ifeq ($(CONFIG_WLAN_CONV_SPECTRAL_ENABLE),y)
UMAC_SPECTRAL_OBJS := $(UMAC_SPECTRAL_CORE_DIR)/spectral_offload.o \
		$(UMAC_SPECTRAL_CORE_DIR)/spectral_common.o \
		$(UMAC_SPECTRAL_DISP_DIR)/wlan_spectral_ucfg_api.o \
		$(UMAC_SPECTRAL_DISP_DIR)/wlan_spectral_utils_api.o \
		$(UMAC_SPECTRAL_DISP_DIR)/wlan_spectral_tgt_api.o \
		$(WLAN_COMMON_ROOT)/os_if/linux/spectral/src/wlan_cfg80211_spectral.o \
		$(WLAN_COMMON_ROOT)/os_if/linux/spectral/src/os_if_spectral_netlink.o \
		$(WLAN_COMMON_ROOT)/target_if/spectral/target_if_spectral_netlink.o \
		$(WLAN_COMMON_ROOT)/target_if/spectral/target_if_spectral_phyerr.o \
		$(WLAN_COMMON_ROOT)/target_if/spectral/target_if_spectral.o \
		$(WLAN_COMMON_ROOT)/target_if/spectral/target_if_spectral_sim.o
endif

$(call add-wlan-objs,umac_spectral,$(UMAC_SPECTRAL_OBJS))

############# WLAN_CFR ############
WLAN_CFR_DIR := umac/cfr
WLAN_CFR_DISP_INC_DIR := $(WLAN_CFR_DIR)/dispatcher/inc
WLAN_CFR_CORE_INC_DIR := $(WLAN_CFR_DIR)/core/inc
WLAN_CFR_CORE_DIR := $(WLAN_COMMON_ROOT)/$(WLAN_CFR_DIR)/core/src
WLAN_CFR_DISP_DIR := $(WLAN_COMMON_ROOT)/$(WLAN_CFR_DIR)/dispatcher/src
WLAN_CFR_TARGET_INC_DIR := target_if/cfr/inc

WLAN_CFR_INC := -I$(WLAN_COMMON_INC)/$(WLAN_CFR_DISP_INC_DIR) \
			-I$(WLAN_COMMON_INC)/$(WLAN_CFR_CORE_INC_DIR) \
			-I$(WLAN_COMMON_INC)/$(WLAN_CFR_TARGET_INC_DIR)
ifeq ($(CONFIG_WLAN_CFR_ENABLE),y)
WLAN_CFR_OBJS := $(WLAN_CFR_CORE_DIR)/cfr_common.o \
                $(WLAN_CFR_DISP_DIR)/wlan_cfr_tgt_api.o \
                $(WLAN_CFR_DISP_DIR)/wlan_cfr_ucfg_api.o \
                $(WLAN_CFR_DISP_DIR)/wlan_cfr_utils_api.o \
		$(WLAN_COMMON_ROOT)/target_if/cfr/src/target_if_cfr.o \
		$(WLAN_COMMON_ROOT)/target_if/cfr/src/target_if_cfr_6490.o
ifeq ($(CONFIG_WLAN_ENH_CFR_ENABLE),y)
WLAN_CFR_OBJS += $(WLAN_COMMON_ROOT)/target_if/cfr/src/target_if_cfr_enh.o
endif
ifeq ($(CONFIG_WLAN_CFR_ADRASTEA),y)
WLAN_CFR_OBJS += $(WLAN_COMMON_ROOT)/target_if/cfr/src/target_if_cfr_adrastea.o
endif
ifeq ($(CONFIG_WLAN_CFR_DBR),y)
WLAN_CFR_OBJS += $(WLAN_COMMON_ROOT)/target_if/cfr/src/target_if_cfr_dbr.o
endif
endif

$(call add-wlan-objs,wlan_cfr,$(WLAN_CFR_OBJS))

############# GPIO_CFG ############
UMAC_GPIO_DIR := gpio
UMAC_GPIO_DISP_INC_DIR := $(UMAC_GPIO_DIR)/dispatcher/inc
UMAC_GPIO_CORE_INC_DIR := $(UMAC_GPIO_DIR)/core/inc
UMAC_GPIO_CORE_DIR := $(WLAN_COMMON_ROOT)/$(UMAC_GPIO_DIR)/core/src
UMAC_GPIO_DISP_DIR := $(WLAN_COMMON_ROOT)/$(UMAC_GPIO_DIR)/dispatcher/src
UMAC_TARGET_GPIO_INC := -I$(WLAN_COMMON_INC)/target_if/gpio

UMAC_GPIO_INC := -I$(WLAN_COMMON_INC)/$(UMAC_GPIO_DISP_INC_DIR) \
			-I$(WLAN_COMMON_INC)/$(UMAC_GPIO_CORE_INC_DIR)

ifeq ($(CONFIG_FEATURE_GPIO_CFG),y)
UMAC_GPIO_OBJS := $(UMAC_GPIO_DISP_DIR)/wlan_gpio_tgt_api.o \
		$(UMAC_GPIO_DISP_DIR)/wlan_gpio_ucfg_api.o \
		$(UMAC_GPIO_CORE_DIR)/wlan_gpio_api.o \
		$(WLAN_COMMON_ROOT)/os_if/linux/gpio/src/wlan_cfg80211_gpio.o \
		$(WLAN_COMMON_ROOT)/target_if/gpio/target_if_gpio.o
endif

$(call add-wlan-objs,umac_gpio,$(UMAC_GPIO_OBJS))

############# UMAC_GREEN_AP ############
UMAC_GREEN_AP_DIR := umac/green_ap
UMAC_GREEN_AP_DISP_INC_DIR := $(UMAC_GREEN_AP_DIR)/dispatcher/inc
UMAC_GREEN_AP_CORE_DIR := $(WLAN_COMMON_ROOT)/$(UMAC_GREEN_AP_DIR)/core/src
UMAC_GREEN_AP_DISP_DIR := $(WLAN_COMMON_ROOT)/$(UMAC_GREEN_AP_DIR)/dispatcher/src
UMAC_TARGET_GREEN_AP_INC := -I$(WLAN_COMMON_INC)/target_if/green_ap/inc

UMAC_GREEN_AP_INC := -I$(WLAN_COMMON_INC)/$(UMAC_GREEN_AP_DISP_INC_DIR)

ifeq ($(CONFIG_QCACLD_FEATURE_GREEN_AP), y)
UMAC_GREEN_AP_OBJS := $(UMAC_GREEN_AP_CORE_DIR)/wlan_green_ap_main.o \
		$(UMAC_GREEN_AP_DISP_DIR)/wlan_green_ap_api.o \
                $(UMAC_GREEN_AP_DISP_DIR)/wlan_green_ap_ucfg_api.o \
                $(WLAN_COMMON_ROOT)/target_if/green_ap/src/target_if_green_ap.o
endif

$(call add-wlan-objs,umac_green_ap,$(UMAC_GREEN_AP_OBJS))

############# WLAN_CONV_CRYPTO_SUPPORTED ############
UMAC_CRYPTO_DIR := umac/cmn_services/crypto
UMAC_CRYPTO_CORE_DIR := $(WLAN_COMMON_ROOT)/$(UMAC_CRYPTO_DIR)/src
UMAC_CRYPTO_INC := -I$(WLAN_COMMON_INC)/$(UMAC_CRYPTO_DIR)/inc \
		-I$(WLAN_COMMON_INC)/$(UMAC_CRYPTO_DIR)/src

UMAC_CRYPTO_OBJS := $(UMAC_CRYPTO_CORE_DIR)/wlan_crypto_global_api.o \
		$(UMAC_CRYPTO_CORE_DIR)/wlan_crypto_ucfg_api.o \
		$(UMAC_CRYPTO_CORE_DIR)/wlan_crypto_main.o \
		$(UMAC_CRYPTO_CORE_DIR)/wlan_crypto_obj_mgr.o \
		$(UMAC_CRYPTO_CORE_DIR)/wlan_crypto_param_handling.o

$(call add-wlan-objs,umac_crypto,$(UMAC_CRYPTO_OBJS))

############# FTM CORE ############
FTM_CORE_DIR := ftm
TARGET_IF_FTM_DIR := target_if/ftm
OS_IF_LINUX_FTM_DIR := os_if/linux/ftm

FTM_CORE_SRC := $(WLAN_COMMON_ROOT)/$(FTM_CORE_DIR)/core/src
FTM_DISP_SRC := $(WLAN_COMMON_ROOT)/$(FTM_CORE_DIR)/dispatcher/src
TARGET_IF_FTM_SRC := $(WLAN_COMMON_ROOT)/$(TARGET_IF_FTM_DIR)/src
OS_IF_FTM_SRC := $(WLAN_COMMON_ROOT)/$(OS_IF_LINUX_FTM_DIR)/src

FTM_CORE_INC := $(WLAN_COMMON_INC)/$(FTM_CORE_DIR)/core/src
FTM_DISP_INC := $(WLAN_COMMON_INC)/$(FTM_CORE_DIR)/dispatcher/inc
TARGET_IF_FTM_INC := $(WLAN_COMMON_INC)/$(TARGET_IF_FTM_DIR)/inc
OS_IF_FTM_INC := $(WLAN_COMMON_INC)/$(OS_IF_LINUX_FTM_DIR)/inc

FTM_INC := -I$(FTM_DISP_INC)	\
	   -I$(FTM_CORE_INC)	\
	   -I$(OS_IF_FTM_INC)	\
	   -I$(TARGET_IF_FTM_INC)

ifeq ($(CONFIG_QCA_WIFI_FTM), y)
FTM_OBJS := $(FTM_DISP_SRC)/wlan_ftm_init_deinit.o \
	    $(FTM_DISP_SRC)/wlan_ftm_ucfg_api.o \
	    $(FTM_CORE_SRC)/wlan_ftm_svc.o \
	    $(TARGET_IF_FTM_SRC)/target_if_ftm.o

ifeq ($(QCA_WIFI_FTM_NL80211), y)
FTM_OBJS += $(OS_IF_FTM_SRC)/wlan_cfg80211_ftm.o
endif

ifeq ($(CONFIG_LINUX_QCMBR), y)
FTM_OBJS += $(OS_IF_FTM_SRC)/wlan_ioctl_ftm.o
endif

endif

$(call add-wlan-objs,ftm,$(FTM_OBJS))

############# UMAC_CMN_SERVICES ############
UMAC_COMMON_INC := -I$(WLAN_COMMON_INC)/umac/cmn_services/cmn_defs/inc \
		-I$(WLAN_COMMON_INC)/umac/cmn_services/utils/inc
UMAC_COMMON_OBJS := $(WLAN_COMMON_ROOT)/umac/cmn_services/utils/src/wlan_utility.o

$(call add-wlan-objs,umac_common,$(UMAC_COMMON_OBJS))

############ CDS (Connectivity driver services) ############
CDS_DIR :=	core/cds
CDS_INC_DIR :=	$(CDS_DIR)/inc
CDS_SRC_DIR :=	$(CDS_DIR)/src

CDS_INC := 	-I$(WLAN_ROOT)/$(CDS_INC_DIR) \
		-I$(WLAN_ROOT)/$(CDS_SRC_DIR)

CDS_OBJS :=	$(CDS_SRC_DIR)/cds_api.o \
		$(CDS_SRC_DIR)/cds_reg_service.o \
		$(CDS_SRC_DIR)/cds_packet.o \
		$(CDS_SRC_DIR)/cds_regdomain.o \
		$(CDS_SRC_DIR)/cds_sched.o \
		$(CDS_SRC_DIR)/cds_utils.o

$(call add-wlan-objs,cds,$(CDS_OBJS))

###### UMAC OBJMGR ########
UMAC_OBJMGR_DIR := $(WLAN_COMMON_ROOT)/umac/cmn_services/obj_mgr

UMAC_OBJMGR_INC := -I$(WLAN_COMMON_INC)/umac/cmn_services/obj_mgr/inc \
		-I$(WLAN_COMMON_INC)/umac/cmn_services/obj_mgr/src \
		-I$(WLAN_COMMON_INC)/umac/cmn_services/inc

UMAC_OBJMGR_OBJS := $(UMAC_OBJMGR_DIR)/src/wlan_objmgr_global_obj.o \
		$(UMAC_OBJMGR_DIR)/src/wlan_objmgr_pdev_obj.o \
		$(UMAC_OBJMGR_DIR)/src/wlan_objmgr_peer_obj.o \
		$(UMAC_OBJMGR_DIR)/src/wlan_objmgr_psoc_obj.o \
		$(UMAC_OBJMGR_DIR)/src/wlan_objmgr_vdev_obj.o

ifeq ($(CONFIG_WLAN_OBJMGR_DEBUG), y)
UMAC_OBJMGR_OBJS += $(UMAC_OBJMGR_DIR)/src/wlan_objmgr_debug.o
endif

$(call add-wlan-objs,umac_objmgr,$(UMAC_OBJMGR_OBJS))

###########  UMAC MGMT TXRX ##########
UMAC_MGMT_TXRX_DIR := $(WLAN_COMMON_ROOT)/umac/cmn_services/mgmt_txrx

UMAC_MGMT_TXRX_INC := -I$(WLAN_COMMON_INC)/umac/cmn_services/mgmt_txrx/dispatcher/inc \

UMAC_MGMT_TXRX_OBJS := $(UMAC_MGMT_TXRX_DIR)/core/src/wlan_mgmt_txrx_main.o \
	$(UMAC_MGMT_TXRX_DIR)/dispatcher/src/wlan_mgmt_txrx_utils_api.o \
	$(UMAC_MGMT_TXRX_DIR)/dispatcher/src/wlan_mgmt_txrx_tgt_api.o

$(call add-wlan-objs,umac_mgmt_txrx,$(UMAC_MGMT_TXRX_OBJS))

###### UMAC INTERFACE_MGR ########
UMAC_INTERFACE_MGR_COMP_DIR :=	components/cmn_services/interface_mgr
UMAC_INTERFACE_MGR_CMN_DIR := $(WLAN_COMMON_ROOT)/umac/cmn_services/interface_mgr

UMAC_INTERFACE_MGR_INC := -I$(WLAN_COMMON_INC)/umac/cmn_services/interface_mgr/inc \
			 -I$(WLAN_ROOT)/components/cmn_services/interface_mgr/inc

UMAC_INTERFACE_MGR_OBJS := $(UMAC_INTERFACE_MGR_CMN_DIR)/src/wlan_if_mgr_main.o \
			  $(UMAC_INTERFACE_MGR_CMN_DIR)/src/wlan_if_mgr_core.o \
			  $(UMAC_INTERFACE_MGR_COMP_DIR)/src/wlan_if_mgr_sta.o \
			  $(UMAC_INTERFACE_MGR_COMP_DIR)/src/wlan_if_mgr_sap.o \
			  $(UMAC_INTERFACE_MGR_COMP_DIR)/src/wlan_if_mgr_roam.o

$(call add-wlan-objs,umac_ifmgr,$(UMAC_INTERFACE_MGR_OBJS))

###### UMAC MLO_MGR ########
UMAC_MLO_MGR_CMN_DIR :=	$(WLAN_COMMON_ROOT)/umac/mlo_mgr
MLO_MGR_TARGET_IF_DIR := $(WLAN_COMMON_ROOT)/target_if/mlo_mgr

UMAC_MLO_MGR_CLD_DIR := components/umac/mlme/mlo_mgr
UMAC_MLO_MGR_CLD_INC := -I$(WLAN_ROOT)/$(UMAC_MLO_MGR_CLD_DIR)/inc \
			-I$(WLAN_ROOT)/$(UMAC_MLO_MGR_CLD_DIR)/dispatcher/inc \

UMAC_MLO_MGR_INC := -I$(WLAN_COMMON_INC)/umac/mlo_mgr/inc \
		    -I$(WLAN_COMMON_INC)/target_if/mlo_mgr/inc

ifeq ($(CONFIG_WLAN_FEATURE_11BE_MLO), y)
UMAC_MLO_MGR_OBJS := $(UMAC_MLO_MGR_CMN_DIR)/src/wlan_mlo_mgr_main.o \
			  $(UMAC_MLO_MGR_CMN_DIR)/src/wlan_mlo_mgr_cmn.o \
			  $(UMAC_MLO_MGR_CMN_DIR)/src/wlan_mlo_mgr_sta.o \
			  $(UMAC_MLO_MGR_CMN_DIR)/src/wlan_mlo_mgr_op.o \
			  $(UMAC_MLO_MGR_CMN_DIR)/src/utils_mlo.o \
			  $(UMAC_MLO_MGR_CMN_DIR)/src/wlan_mlo_mgr_ap.o \
			  $(UMAC_MLO_MGR_CMN_DIR)/src/wlan_mlo_mgr_peer_list.o \
			  $(UMAC_MLO_MGR_CMN_DIR)/src/wlan_mlo_mgr_aid.o \
			  $(UMAC_MLO_MGR_CMN_DIR)/src/wlan_mlo_mgr_peer.o \
			  $(UMAC_MLO_MGR_CMN_DIR)/src/wlan_mlo_mgr_msgq.o \
			  $(UMAC_MLO_MGR_CMN_DIR)/src/wlan_mlo_mgr_primary_umac.o \
			  $(MLO_MGR_TARGET_IF_DIR)/src/target_if_mlo_mgr.o \
			  $(UMAC_MLO_MGR_CLD_DIR)/src/wlan_mlo_link_force.o \
			  $(UMAC_MLO_MGR_CLD_DIR)/src/wlan_mlo_mgr_roam.o \
			  $(UMAC_MLO_MGR_CLD_DIR)/src/wlan_t2lm_api.o \
			  $(UMAC_MLO_MGR_CMN_DIR)/src/wlan_mlo_t2lm.o \
			  $(UMAC_MLO_MGR_CLD_DIR)/src/wlan_epcs_api.o \
			  $(UMAC_MLO_MGR_CMN_DIR)/src/wlan_mlo_epcs.o \
			  $(UMAC_MLO_MGR_CLD_DIR)/dispatcher/src/wlan_mlo_epcs_ucfg_api.o \
			  $(UMAC_MLO_MGR_CMN_DIR)/src/wlan_mlo_mgr_link_switch.o \

$(call add-wlan-objs,umac_mlomgr,$(UMAC_MLO_MGR_OBJS))
endif
########## POWER MANAGEMENT OFFLOADS (PMO) ##########
PMO_DIR :=	components/pmo
PMO_INC :=	-I$(WLAN_ROOT)/$(PMO_DIR)/core/inc \
			-I$(WLAN_ROOT)/$(PMO_DIR)/dispatcher/inc \

ifeq ($(CONFIG_POWER_MANAGEMENT_OFFLOAD), y)
PMO_OBJS :=     $(PMO_DIR)/core/src/wlan_pmo_main.o \
		$(PMO_DIR)/core/src/wlan_pmo_apf.o \
		$(PMO_DIR)/core/src/wlan_pmo_arp.o \
		$(PMO_DIR)/core/src/wlan_pmo_gtk.o \
		$(PMO_DIR)/core/src/wlan_pmo_mc_addr_filtering.o \
		$(PMO_DIR)/core/src/wlan_pmo_static_config.o \
		$(PMO_DIR)/core/src/wlan_pmo_wow.o \
		$(PMO_DIR)/core/src/wlan_pmo_lphb.o \
		$(PMO_DIR)/core/src/wlan_pmo_suspend_resume.o \
		$(PMO_DIR)/core/src/wlan_pmo_hw_filter.o \
		$(PMO_DIR)/dispatcher/src/wlan_pmo_obj_mgmt_api.o \
		$(PMO_DIR)/dispatcher/src/wlan_pmo_ucfg_api.o \
		$(PMO_DIR)/dispatcher/src/wlan_pmo_tgt_arp.o \
		$(PMO_DIR)/dispatcher/src/wlan_pmo_tgt_gtk.o \
		$(PMO_DIR)/dispatcher/src/wlan_pmo_tgt_wow.o \
		$(PMO_DIR)/dispatcher/src/wlan_pmo_tgt_static_config.o \
		$(PMO_DIR)/dispatcher/src/wlan_pmo_tgt_mc_addr_filtering.o \
		$(PMO_DIR)/dispatcher/src/wlan_pmo_tgt_lphb.o \
		$(PMO_DIR)/dispatcher/src/wlan_pmo_tgt_suspend_resume.o \
		$(PMO_DIR)/dispatcher/src/wlan_pmo_tgt_hw_filter.o \

ifeq ($(CONFIG_WLAN_FEATURE_PACKET_FILTERING), y)
PMO_OBJS +=	$(PMO_DIR)/core/src/wlan_pmo_pkt_filter.o \
		$(PMO_DIR)/dispatcher/src/wlan_pmo_tgt_pkt_filter.o
endif
endif

ifeq ($(CONFIG_WLAN_NS_OFFLOAD), y)
PMO_OBJS +=     $(PMO_DIR)/core/src/wlan_pmo_ns.o \
		$(PMO_DIR)/dispatcher/src/wlan_pmo_tgt_ns.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_ICMP_OFFLOAD), y)
PMO_OBJS +=     $(PMO_DIR)/core/src/wlan_pmo_icmp.o \
		$(PMO_DIR)/dispatcher/src/wlan_pmo_tgt_icmp.o
endif

$(call add-wlan-objs,pmo,$(PMO_OBJS))

########## DISA (ENCRYPTION TEST) ##########

DISA_DIR :=	components/disa
DISA_INC :=	-I$(WLAN_ROOT)/$(DISA_DIR)/core/inc \
		-I$(WLAN_ROOT)/$(DISA_DIR)/dispatcher/inc

ifeq ($(CONFIG_WLAN_FEATURE_DISA), y)
DISA_OBJS :=	$(DISA_DIR)/core/src/wlan_disa_main.o \
		$(DISA_DIR)/dispatcher/src/wlan_disa_obj_mgmt_api.o \
		$(DISA_DIR)/dispatcher/src/wlan_disa_tgt_api.o \
		$(DISA_DIR)/dispatcher/src/wlan_disa_ucfg_api.o
endif

$(call add-wlan-objs,disa,$(DISA_OBJS))

######## OCB ##############
OCB_DIR := components/ocb
OCB_INC := -I$(WLAN_ROOT)/$(OCB_DIR)/core/inc \
		-I$(WLAN_ROOT)/$(OCB_DIR)/dispatcher/inc

ifeq ($(CONFIG_WLAN_FEATURE_DSRC), y)
OCB_OBJS :=	$(OCB_DIR)/dispatcher/src/wlan_ocb_ucfg_api.o \
		$(OCB_DIR)/dispatcher/src/wlan_ocb_tgt_api.o \
		$(OCB_DIR)/core/src/wlan_ocb_main.o
endif

$(call add-wlan-objs,ocb,$(OCB_OBJS))

######## IPA ##############
IPA_DIR := $(WLAN_COMMON_ROOT)/ipa
IPA_INC := -I$(WLAN_ROOT)/$(IPA_DIR)/core/inc \
		-I$(WLAN_ROOT)/$(IPA_DIR)/dispatcher/inc

ifeq ($(CONFIG_IPA_OFFLOAD), y)
IPA_OBJS :=	$(IPA_DIR)/dispatcher/src/wlan_ipa_ucfg_api.o \
		$(IPA_DIR)/dispatcher/src/wlan_ipa_obj_mgmt_api.o \
		$(IPA_DIR)/dispatcher/src/wlan_ipa_tgt_api.o \
		$(IPA_DIR)/core/src/wlan_ipa_main.o \
		$(IPA_DIR)/core/src/wlan_ipa_core.o \
		$(IPA_DIR)/core/src/wlan_ipa_stats.o \
		$(IPA_DIR)/core/src/wlan_ipa_rm.o
endif

$(call add-wlan-objs,ipa,$(IPA_OBJS))

######## FWOL ##########
FWOL_CORE_INC := components/fw_offload/core/inc
FWOL_CORE_SRC := components/fw_offload/core/src
FWOL_DISPATCHER_INC := components/fw_offload/dispatcher/inc
FWOL_DISPATCHER_SRC := components/fw_offload/dispatcher/src
FWOL_TARGET_IF_INC := components/target_if/fw_offload/inc
FWOL_TARGET_IF_SRC := components/target_if/fw_offload/src
FWOL_OS_IF_INC := os_if/fw_offload/inc
FWOL_OS_IF_SRC := os_if/fw_offload/src

FWOL_INC := -I$(WLAN_ROOT)/$(FWOL_CORE_INC) \
	    -I$(WLAN_ROOT)/$(FWOL_DISPATCHER_INC) \
	    -I$(WLAN_ROOT)/$(FWOL_TARGET_IF_INC) \
	    -I$(WLAN_ROOT)/$(FWOL_OS_IF_INC) \
	    -I$(WLAN_COMMON_INC)/umac/thermal/dispatcher/inc

ifeq ($(CONFIG_WLAN_FW_OFFLOAD), y)
FWOL_OBJS :=	$(FWOL_CORE_SRC)/wlan_fw_offload_main.o \
		$(FWOL_DISPATCHER_SRC)/wlan_fwol_ucfg_api.o \
		$(FWOL_DISPATCHER_SRC)/wlan_fwol_tgt_api.o \
		$(FWOL_TARGET_IF_SRC)/target_if_fwol.o \
		$(FWOL_OS_IF_SRC)/os_if_fwol.o
endif

$(call add-wlan-objs,fwol,$(FWOL_OBJS))

######## SM FRAMEWORK  ##############
UMAC_SM_DIR := umac/cmn_services/sm_engine
UMAC_SM_INC := -I$(WLAN_COMMON_INC)/$(UMAC_SM_DIR)/inc

UMAC_SM_OBJS := $(WLAN_COMMON_ROOT)/$(UMAC_SM_DIR)/src/wlan_sm_engine.o

ifeq ($(CONFIG_SM_ENG_HIST), y)
UMAC_SM_OBJS +=	$(WLAN_COMMON_ROOT)/$(UMAC_SM_DIR)/src/wlan_sm_engine_dbg.o
endif

$(call add-wlan-objs,umac_sm,$(UMAC_SM_OBJS))

######## COMMON MLME ##############
UMAC_MLME_INC := -I$(WLAN_COMMON_INC)/umac/mlme \
		-I$(WLAN_COMMON_INC)/umac/mlme/mlme_objmgr/dispatcher/inc \
		-I$(WLAN_COMMON_INC)/umac/mlme/vdev_mgr/dispatcher/inc \
		-I$(WLAN_COMMON_INC)/umac/mlme/pdev_mgr/dispatcher/inc \
		-I$(WLAN_COMMON_INC)/umac/mlme/psoc_mgr/dispatcher/inc \
		-I$(WLAN_COMMON_INC)/umac/mlme/connection_mgr/dispatcher/inc \
		-I$(WLAN_COMMON_INC)/umac/mlme/connection_mgr/utf/inc \
		-I$(WLAN_COMMON_INC)/umac/mlme/include \
		-I$(WLAN_COMMON_INC)/umac/mlme/mlme_utils/

UMAC_MLME_OBJS := $(WLAN_COMMON_ROOT)/umac/mlme/mlme_objmgr/dispatcher/src/wlan_vdev_mlme_main.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/vdev_mgr/core/src/vdev_mlme_sm.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/vdev_mgr/dispatcher/src/wlan_vdev_mlme_api.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/vdev_mgr/core/src/vdev_mgr_ops.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/vdev_mgr/dispatcher/src/wlan_vdev_mgr_tgt_if_rx_api.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/vdev_mgr/dispatcher/src/wlan_vdev_mgr_tgt_if_tx_api.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/vdev_mgr/dispatcher/src/wlan_vdev_mgr_ucfg_api.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/vdev_mgr/dispatcher/src/wlan_vdev_mgr_api.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/vdev_mgr/dispatcher/src/wlan_vdev_mgr_utils_api.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/mlme_objmgr/dispatcher/src/wlan_cmn_mlme_main.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/mlme_objmgr/dispatcher/src/wlan_pdev_mlme_main.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/pdev_mgr/dispatcher/src/wlan_pdev_mlme_api.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/mlme_objmgr/dispatcher/src/wlan_psoc_mlme_main.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/psoc_mgr/dispatcher/src/wlan_psoc_mlme_api.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/psoc_mgr/dispatcher/src/wlan_psoc_mlme_ucfg_api.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/connection_mgr/core/src/wlan_cm_bss_scoring.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/connection_mgr/core/src/wlan_cm_main.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/connection_mgr/core/src/wlan_cm_sm.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/connection_mgr/core/src/wlan_cm_roam_sm.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/connection_mgr/core/src/wlan_cm_connect.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/connection_mgr/core/src/wlan_cm_connect_scan.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/connection_mgr/core/src/wlan_cm_disconnect.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/connection_mgr/core/src/wlan_cm_util.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/connection_mgr/dispatcher/src/wlan_cm_ucfg_api.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/connection_mgr/dispatcher/src/wlan_cm_api.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/mlme_utils/wlan_vdev_mlme_ser_if.o
ifeq ($(CONFIG_CM_UTF_ENABLE), y)
UMAC_MLME_OBJS += $(WLAN_COMMON_ROOT)/umac/mlme/connection_mgr/utf/src/wlan_cm_utf_main.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/connection_mgr/utf/src/wlan_cm_utf_scan.o
endif

ifeq ($(CONFIG_QCACLD_WLAN_LFR3), y)
# Add LFR3/FW roam specific connection manager files here
UMAC_MLME_OBJS += $(WLAN_COMMON_ROOT)/umac/mlme/connection_mgr/core/src/wlan_cm_roam_util.o

endif
ifeq ($(CONFIG_QCACLD_WLAN_LFR2), y)
# Add LFR2/host roam specific connection manager files here
UMAC_MLME_OBJS += $(WLAN_COMMON_ROOT)/umac/mlme/connection_mgr/core/src/wlan_cm_roam_util.o \
		$(WLAN_COMMON_ROOT)/umac/mlme/connection_mgr/core/src/wlan_cm_host_roam.o
endif

$(call add-wlan-objs,umac_mlme,$(UMAC_MLME_OBJS))

######## MLME ##############
MLME_DIR := components/mlme
MLME_INC := -I$(WLAN_ROOT)/$(MLME_DIR)/core/inc \
		-I$(WLAN_ROOT)/$(MLME_DIR)/dispatcher/inc \

MLME_OBJS :=	$(MLME_DIR)/core/src/wlan_mlme_main.o \
		$(MLME_DIR)/dispatcher/src/wlan_mlme_api.o \
		$(MLME_DIR)/dispatcher/src/wlan_mlme_ucfg_api.o

MLME_OBJS += $(MLME_DIR)/core/src/wlan_mlme_vdev_mgr_interface.o

ifeq ($(CONFIG_WLAN_FEATURE_TWT), y)
MLME_OBJS += $(MLME_DIR)/core/src/wlan_mlme_twt_api.o
MLME_OBJS += $(MLME_DIR)/dispatcher/src/wlan_mlme_twt_ucfg_api.o
endif

CM_DIR := components/umac/mlme/connection_mgr
CM_TGT_IF_DIR := components/target_if/connection_mgr

CM_INC := -I$(WLAN_ROOT)/$(CM_DIR)/dispatcher/inc \
		-I$(WLAN_ROOT)/$(CM_DIR)/utf/inc \
		-I$(WLAN_ROOT)/$(CM_TGT_IF_DIR)/inc

MLME_INC += $(CM_INC)

MLME_OBJS +=    $(CM_DIR)/dispatcher/src/wlan_cm_tgt_if_tx_api.o \
		$(CM_DIR)/dispatcher/src/wlan_cm_roam_api.o \
		$(CM_DIR)/dispatcher/src/wlan_cm_roam_ucfg_api.o \
		$(CM_TGT_IF_DIR)/src/target_if_cm_roam_offload.o \
		$(CM_TGT_IF_DIR)/src/target_if_cm_roam_event.o \
		$(CM_DIR)/core/src/wlan_cm_roam_offload.o \
		$(CM_DIR)/core/src/wlan_cm_vdev_connect.o \
		$(CM_DIR)/core/src/wlan_cm_vdev_disconnect.o

ifeq ($(CONFIG_CM_UTF_ENABLE), y)
MLME_OBJS +=    $(CM_DIR)/utf/src/cm_utf.o
endif

ifeq ($(CONFIG_QCACLD_WLAN_LFR3), y)
MLME_OBJS +=    $(CM_DIR)/core/src/wlan_cm_roam_fw_sync.o \
		$(CM_DIR)/core/src/wlan_cm_roam_offload_event.o
endif

ifeq ($(CONFIG_QCACLD_WLAN_LFR2), y)
# Add LFR2/host roam specific connection manager files here
MLME_OBJS +=    $(CM_DIR)/core/src/wlan_cm_host_roam_preauth.o \
		$(CM_DIR)/core/src/wlan_cm_host_util.o
endif

####### WFA_CONFIG ########

WFA_DIR := components/umac/mlme/wfa_config
WFA_TGT_IF_DIR := components/target_if/wfa_config

WFA_INC := -I$(WLAN_ROOT)/$(WFA_DIR)/dispatcher/inc \
		-I$(WLAN_ROOT)/$(WFA_TGT_IF_DIR)/inc

MLME_INC += $(WFA_INC)

MLME_OBJS += $(WFA_TGT_IF_DIR)/src/target_if_wfa_testcmd.o \
		$(WFA_DIR)/dispatcher/src/wlan_wfa_tgt_if_tx_api.o

####### LL_SAP #######
LL_SAP_DIR := components/umac/mlme/sap/ll_sap
LL_SAP_OS_IF_DIR := os_if/mlme/sap/ll_sap
LL_SAP_TARGET_IF_DIR := components/target_if/sap/ll_sap
LL_SAP_WMI_DIR := components/wmi/

LL_SAP_INC := -I$(WLAN_ROOT)/$(LL_SAP_DIR)/dispatcher/inc \
		-I$(WLAN_ROOT)/$(LL_SAP_OS_IF_DIR)/inc \
		-I$(WLAN_ROOT)/$(LL_SAP_TARGET_IF_DIR)/inc \
		-I$(WLAN_ROOT)/$(LL_SAP_WMI_DIR)/inc

MLME_INC += $(LL_SAP_INC)

ifeq ($(CONFIG_WLAN_FEATURE_LL_LT_SAP), y)
MLME_OBJS += $(LL_SAP_DIR)/dispatcher/src/wlan_ll_sap_ucfg_api.o \
		$(LL_SAP_DIR)/dispatcher/src/wlan_ll_sap_api.o \
		$(LL_SAP_DIR)/core/src/wlan_ll_sap_main.o \
		$(LL_SAP_DIR)/core/src/wlan_ll_lt_sap_main.o \
		$(LL_SAP_DIR)/core/src/wlan_ll_lt_sap_bearer_switch.o \
		$(LL_SAP_OS_IF_DIR)/src/os_if_ll_sap.o \
		$(LL_SAP_TARGET_IF_DIR)/src/target_if_ll_sap.o \
		$(LL_SAP_WMI_DIR)/src/wmi_unified_ll_sap_api.o \
		$(LL_SAP_WMI_DIR)/src/wmi_unified_ll_sap_tlv.o
endif

$(call add-wlan-objs,mlme,$(MLME_OBJS))

####### DENYLIST_MGR ########

DLM_DIR := components/denylist_mgr
DLM_INC := -I$(WLAN_ROOT)/$(DLM_DIR)/core/inc \
                -I$(WLAN_ROOT)/$(DLM_DIR)/dispatcher/inc
ifeq ($(CONFIG_FEATURE_DENYLIST_MGR), y)
DLM_OBJS :=    $(DLM_DIR)/core/src/wlan_dlm_main.o \
                $(DLM_DIR)/core/src/wlan_dlm_core.o \
                $(DLM_DIR)/dispatcher/src/wlan_dlm_ucfg_api.o \
                $(DLM_DIR)/dispatcher/src/wlan_dlm_tgt_api.o
endif

$(call add-wlan-objs,dlm,$(DLM_OBJS))

######### CONNECTIVITY_LOGGING #########
CONN_LOGGING_DIR := components/cmn_services/logging
CONN_LOGGING_INC := -I$(WLAN_ROOT)/$(CONN_LOGGING_DIR)/inc

ifeq ($(CONFIG_QCACLD_WLAN_CONNECTIVITY_DIAG_EVENT), y)
CONN_LOGGING_OBJS := $(CONN_LOGGING_DIR)/src/wlan_connectivity_logging.o
else ifeq ($(CONFIG_QCACLD_WLAN_CONNECTIVITY_LOGGING), y)
CONN_LOGGING_OBJS := $(CONN_LOGGING_DIR)/src/wlan_connectivity_logging.o
endif

$(call add-wlan-objs,conn_logging,$(CONN_LOGGING_OBJS))

########## ACTION OUI ##########

ACTION_OUI_DIR := components/action_oui
ACTION_OUI_INC := -I$(WLAN_ROOT)/$(ACTION_OUI_DIR)/core/inc \
		  -I$(WLAN_ROOT)/$(ACTION_OUI_DIR)/dispatcher/inc

ifeq ($(CONFIG_WLAN_FEATURE_ACTION_OUI), y)
ACTION_OUI_OBJS := $(ACTION_OUI_DIR)/core/src/wlan_action_oui_main.o \
		$(ACTION_OUI_DIR)/core/src/wlan_action_oui_parse.o \
		$(ACTION_OUI_DIR)/dispatcher/src/wlan_action_oui_tgt_api.o \
		$(ACTION_OUI_DIR)/dispatcher/src/wlan_action_oui_ucfg_api.o
endif

$(call add-wlan-objs,action_oui,$(ACTION_OUI_OBJS))

######## PACKET CAPTURE ########

PKT_CAPTURE_DIR := components/pkt_capture
PKT_CAPTURE_OS_IF_DIR := os_if/pkt_capture
PKT_CAPTURE_TARGET_IF_DIR := components/target_if/pkt_capture/
PKT_CAPTURE_INC := -I$(WLAN_ROOT)/$(PKT_CAPTURE_DIR)/core/inc \
		  -I$(WLAN_ROOT)/$(PKT_CAPTURE_DIR)/dispatcher/inc \
		  -I$(WLAN_ROOT)/$(PKT_CAPTURE_TARGET_IF_DIR)/inc \
		  -I$(WLAN_ROOT)/$(PKT_CAPTURE_OS_IF_DIR)/inc

ifeq ($(CONFIG_WLAN_FEATURE_PKT_CAPTURE), y)
PKT_CAPTURE_OBJS := $(PKT_CAPTURE_DIR)/core/src/wlan_pkt_capture_main.o \
		$(PKT_CAPTURE_DIR)/core/src/wlan_pkt_capture_mon_thread.o \
		$(PKT_CAPTURE_DIR)/core/src/wlan_pkt_capture_mgmt_txrx.o \
		$(PKT_CAPTURE_DIR)/core/src/wlan_pkt_capture_data_txrx.o \
		$(PKT_CAPTURE_DIR)/dispatcher/src/wlan_pkt_capture_ucfg_api.o \
		$(PKT_CAPTURE_DIR)/dispatcher/src/wlan_pkt_capture_tgt_api.o \
		$(PKT_CAPTURE_DIR)/dispatcher/src/wlan_pkt_capture_api.o \
		$(PKT_CAPTURE_TARGET_IF_DIR)/src/target_if_pkt_capture.o \
		$(PKT_CAPTURE_OS_IF_DIR)/src/os_if_pkt_capture.o
endif

$(call add-wlan-objs,pkt_capture,$(PKT_CAPTURE_OBJS))

########## FTM TIME SYNC ##########

FTM_TIME_SYNC_DIR := components/ftm_time_sync
FTM_TIME_SYNC_INC := -I$(WLAN_ROOT)/$(FTM_TIME_SYNC_DIR)/core/inc \
		  -I$(WLAN_ROOT)/$(FTM_TIME_SYNC_DIR)/dispatcher/inc

ifeq ($(CONFIG_FEATURE_WLAN_TIME_SYNC_FTM), y)
FTM_TIME_SYNC_OBJS := $(FTM_TIME_SYNC_DIR)/core/src/ftm_time_sync_main.o \
		$(FTM_TIME_SYNC_DIR)/dispatcher/src/ftm_time_sync_ucfg_api.o \
		$(FTM_TIME_SYNC_DIR)/dispatcher/src/wlan_ftm_time_sync_tgt_api.o
endif

$(call add-wlan-objs,ftm_time_sync,$(FTM_TIME_SYNC_OBJS))

########## WLAN PRE_CAC ##########

WLAN_PRE_CAC_DIR := components/pre_cac
PRE_CAC_OSIF_DIR := os_if/pre_cac
WLAN_PRE_CAC_INC := -I$(WLAN_ROOT)/$(WLAN_PRE_CAC_DIR)/dispatcher/inc \
		  -I$(WLAN_ROOT)/$(WLAN_PRE_CAC_DIR)/core/src \
		  -I$(WLAN_ROOT)/$(PRE_CAC_OSIF_DIR)/inc

ifeq ($(CONFIG_FEATURE_WLAN_PRE_CAC), y)
WLAN_PRE_CAC_OBJS := $(HDD_SRC_DIR)/wlan_hdd_pre_cac.o \
		$(WLAN_PRE_CAC_DIR)/core/src/wlan_pre_cac_main.o \
		$(WLAN_PRE_CAC_DIR)/dispatcher/src/wlan_pre_cac_ucfg_api.o \
		$(WLAN_PRE_CAC_DIR)/dispatcher/src/wlan_pre_cac_api.o \
		$(PRE_CAC_OSIF_DIR)/src/osif_pre_cac.o
endif

$(call add-wlan-objs,wlan_pre_cac,$(WLAN_PRE_CAC_OBJS))

########## CLD TARGET_IF #######
CLD_TARGET_IF_DIR := components/target_if

CLD_TARGET_IF_INC := -I$(WLAN_ROOT)/$(CLD_TARGET_IF_DIR)/pmo/inc \
		     -I$(WLAN_ROOT)/$(CLD_TARGET_IF_DIR)/mlme/inc \

ifeq ($(CONFIG_QCA_TARGET_IF_MLME), y)
CLD_TARGET_IF_OBJ := $(CLD_TARGET_IF_DIR)/mlme/src/target_if_mlme.o
endif

ifeq ($(CONFIG_POWER_MANAGEMENT_OFFLOAD), y)
CLD_TARGET_IF_OBJ += $(CLD_TARGET_IF_DIR)/pmo/src/target_if_pmo_arp.o \
		$(CLD_TARGET_IF_DIR)/pmo/src/target_if_pmo_gtk.o \
		$(CLD_TARGET_IF_DIR)/pmo/src/target_if_pmo_hw_filter.o \
		$(CLD_TARGET_IF_DIR)/pmo/src/target_if_pmo_lphb.o \
		$(CLD_TARGET_IF_DIR)/pmo/src/target_if_pmo_main.o \
		$(CLD_TARGET_IF_DIR)/pmo/src/target_if_pmo_mc_addr_filtering.o \
		$(CLD_TARGET_IF_DIR)/pmo/src/target_if_pmo_static_config.o \
		$(CLD_TARGET_IF_DIR)/pmo/src/target_if_pmo_suspend_resume.o \
		$(CLD_TARGET_IF_DIR)/pmo/src/target_if_pmo_wow.o
ifeq ($(CONFIG_WLAN_NS_OFFLOAD), y)
CLD_TARGET_IF_OBJ += $(CLD_TARGET_IF_DIR)/pmo/src/target_if_pmo_ns.o
endif
ifeq ($(CONFIG_WLAN_FEATURE_PACKET_FILTERING), y)
CLD_TARGET_IF_OBJ += $(CLD_TARGET_IF_DIR)/pmo/src/target_if_pmo_pkt_filter.o
endif
ifeq ($(CONFIG_WLAN_FEATURE_ICMP_OFFLOAD), y)
CLD_TARGET_IF_OBJ += $(CLD_TARGET_IF_DIR)/pmo/src/target_if_pmo_icmp.o
endif
endif

ifeq ($(CONFIG_WLAN_FEATURE_DSRC), y)
CLD_TARGET_IF_INC += -I$(WLAN_ROOT)/$(CLD_TARGET_IF_DIR)/ocb/inc
CLD_TARGET_IF_OBJ += $(CLD_TARGET_IF_DIR)/ocb/src/target_if_ocb.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_DISA), y)
CLD_TARGET_IF_INC += -I$(WLAN_ROOT)/$(CLD_TARGET_IF_DIR)/disa/inc
CLD_TARGET_IF_OBJ += $(CLD_TARGET_IF_DIR)/disa/src/target_if_disa.o
endif

ifeq ($(CONFIG_FEATURE_DENYLIST_MGR), y)
CLD_TARGET_IF_INC += -I$(WLAN_ROOT)/$(CLD_TARGET_IF_DIR)/denylist_mgr/inc
CLD_TARGET_IF_OBJ += $(CLD_TARGET_IF_DIR)/denylist_mgr/src/target_if_dlm.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_ACTION_OUI), y)
CLD_TARGET_IF_INC += -I$(WLAN_ROOT)/$(CLD_TARGET_IF_DIR)/action_oui/inc
CLD_TARGET_IF_OBJ += $(CLD_TARGET_IF_DIR)/action_oui/src/target_if_action_oui.o
endif

ifeq ($(CONFIG_FEATURE_WLAN_TIME_SYNC_FTM), y)
CLD_TARGET_IF_INC += -I$(WLAN_ROOT)/$(CLD_TARGET_IF_DIR)/ftm_time_sync/inc
CLD_TARGET_IF_OBJ += $(CLD_TARGET_IF_DIR)/ftm_time_sync/src/target_if_ftm_time_sync.o
endif

$(call add-wlan-objs,cld_target_if,$(CLD_TARGET_IF_OBJ))

############## UMAC P2P ###########
P2P_DIR := components/p2p
P2P_CORE_OBJ_DIR := $(P2P_DIR)/core/src
P2P_DISPATCHER_DIR := $(P2P_DIR)/dispatcher
P2P_DISPATCHER_INC_DIR := $(P2P_DISPATCHER_DIR)/inc
P2P_DISPATCHER_OBJ_DIR := $(P2P_DISPATCHER_DIR)/src
P2P_OS_IF_INC := os_if/p2p/inc
P2P_OS_IF_SRC := os_if/p2p/src
P2P_TARGET_IF_INC := components/target_if/p2p/inc
P2P_TARGET_IF_SRC := components/target_if/p2p/src
P2P_INC := -I$(WLAN_ROOT)/$(P2P_DISPATCHER_INC_DIR) \
	   -I$(WLAN_ROOT)/$(P2P_OS_IF_INC) \
	   -I$(WLAN_ROOT)/$(P2P_TARGET_IF_INC)
P2P_OBJS := $(P2P_DISPATCHER_OBJ_DIR)/wlan_p2p_ucfg_api.o \
	    $(P2P_DISPATCHER_OBJ_DIR)/wlan_p2p_tgt_api.o \
	    $(P2P_DISPATCHER_OBJ_DIR)/wlan_p2p_cfg.o \
	    $(P2P_DISPATCHER_OBJ_DIR)/wlan_p2p_api.o \
	    $(P2P_CORE_OBJ_DIR)/wlan_p2p_main.o \
	    $(P2P_CORE_OBJ_DIR)/wlan_p2p_roc.o \
	    $(P2P_CORE_OBJ_DIR)/wlan_p2p_off_chan_tx.o \
	    $(P2P_OS_IF_SRC)/wlan_cfg80211_p2p.o \
	    $(P2P_TARGET_IF_SRC)/target_if_p2p.o
ifeq ($(CONFIG_WLAN_FEATURE_MCC_QUOTA), y)
P2P_OBJS += $(P2P_DISPATCHER_OBJ_DIR)/wlan_p2p_mcc_quota_tgt_api.o \
	    $(P2P_CORE_OBJ_DIR)/wlan_p2p_mcc_quota.o \
	    $(P2P_TARGET_IF_SRC)/target_if_p2p_mcc_quota.o
endif
$(call add-wlan-objs,p2p,$(P2P_OBJS))

###### UMAC POLICY MGR ########
POLICY_MGR_DIR := components/cmn_services/policy_mgr

POLICY_MGR_INC := -I$(WLAN_ROOT)/$(POLICY_MGR_DIR)/inc \
		  -I$(WLAN_ROOT)/$(POLICY_MGR_DIR)/src

POLICY_MGR_OBJS := $(POLICY_MGR_DIR)/src/wlan_policy_mgr_action.o \
	$(POLICY_MGR_DIR)/src/wlan_policy_mgr_core.o \
	$(POLICY_MGR_DIR)/src/wlan_policy_mgr_get_set_utils.o \
	$(POLICY_MGR_DIR)/src/wlan_policy_mgr_init_deinit.o \
	$(POLICY_MGR_DIR)/src/wlan_policy_mgr_ucfg.o \
	$(POLICY_MGR_DIR)/src/wlan_policy_mgr_pcl.o
ifeq ($(CONFIG_WLAN_FEATURE_LL_LT_SAP), y)
POLICY_MGR_OBJS += $(POLICY_MGR_DIR)/src/wlan_policy_mgr_ll_sap.o
endif

$(call add-wlan-objs,policy_mgr,$(POLICY_MGR_OBJS))

###### UMAC TDLS ########
TDLS_DIR := components/tdls

TDLS_OS_IF_INC := os_if/tdls/inc
TDLS_OS_IF_SRC := os_if/tdls/src
TDLS_TARGET_IF_INC := components/target_if/tdls/inc
TDLS_TARGET_IF_SRC := components/target_if/tdls/src
TDLS_INC := -I$(WLAN_ROOT)/$(TDLS_DIR)/dispatcher/inc \
	    -I$(WLAN_ROOT)/$(TDLS_DIR)/core/src \
	    -I$(WLAN_ROOT)/$(TDLS_OS_IF_INC) \
	    -I$(WLAN_ROOT)/$(TDLS_TARGET_IF_INC)

ifeq ($(CONFIG_QCOM_TDLS), y)
TDLS_OBJS := $(TDLS_DIR)/core/src/wlan_tdls_main.o \
       $(TDLS_DIR)/core/src/wlan_tdls_cmds_process.o \
       $(TDLS_DIR)/core/src/wlan_tdls_peer.o \
       $(TDLS_DIR)/core/src/wlan_tdls_mgmt.o \
       $(TDLS_DIR)/core/src/wlan_tdls_ct.o \
       $(TDLS_DIR)/dispatcher/src/wlan_tdls_tgt_api.o \
       $(TDLS_DIR)/dispatcher/src/wlan_tdls_ucfg_api.o \
       $(TDLS_DIR)/dispatcher/src/wlan_tdls_utils_api.o \
       $(TDLS_DIR)/dispatcher/src/wlan_tdls_cfg.o \
       $(TDLS_DIR)/dispatcher/src/wlan_tdls_api.o \
       $(TDLS_OS_IF_SRC)/wlan_cfg80211_tdls.o \
       $(TDLS_TARGET_IF_SRC)/target_if_tdls.o
endif

$(call add-wlan-objs,tdls,$(TDLS_OBJS))

########### BMI ###########
BMI_DIR := core/bmi

BMI_INC := -I$(WLAN_ROOT)/$(BMI_DIR)/inc

ifeq ($(CONFIG_WLAN_FEATURE_BMI), y)
BMI_OBJS := $(BMI_DIR)/src/bmi.o \
            $(BMI_DIR)/src/bmi_1.o \
            $(BMI_DIR)/src/ol_fw.o \
            $(BMI_DIR)/src/ol_fw_common.o
endif

$(call add-wlan-objs,bmi,$(BMI_OBJS))

##########  TARGET_IF #######
TARGET_IF_DIR := $(WLAN_COMMON_ROOT)/target_if

TARGET_IF_INC := -I$(WLAN_COMMON_INC)/target_if/core/inc \
		 -I$(WLAN_COMMON_INC)/target_if/init_deinit/inc \
		 -I$(WLAN_COMMON_INC)/target_if/crypto/inc \
		 -I$(WLAN_COMMON_INC)/target_if/regulatory/inc \
		 -I$(WLAN_COMMON_INC)/target_if/mlme/vdev_mgr/inc \
		 -I$(WLAN_COMMON_INC)/target_if/dispatcher/inc \
		 -I$(WLAN_COMMON_INC)/target_if/mlme/psoc/inc \
		 -I$(WLAN_COMMON_INC)/target_if/ipa/inc

TARGET_IF_OBJ := $(TARGET_IF_DIR)/core/src/target_if_main.o \
		$(TARGET_IF_DIR)/regulatory/src/target_if_reg.o \
		$(TARGET_IF_DIR)/regulatory/src/target_if_reg_lte.o \
		$(TARGET_IF_DIR)/regulatory/src/target_if_reg_11d.o \
		$(TARGET_IF_DIR)/init_deinit/src/init_cmd_api.o \
		$(TARGET_IF_DIR)/init_deinit/src/init_deinit_lmac.o \
		$(TARGET_IF_DIR)/init_deinit/src/init_event_handler.o \
		$(TARGET_IF_DIR)/init_deinit/src/service_ready_util.o \
		$(TARGET_IF_DIR)/mlme/vdev_mgr/src/target_if_vdev_mgr_tx_ops.o \
		$(TARGET_IF_DIR)/mlme/vdev_mgr/src/target_if_vdev_mgr_rx_ops.o \
		$(TARGET_IF_DIR)/mlme/psoc/src/target_if_psoc_timer_tx_ops.o

ifeq ($(CONFIG_FEATURE_VDEV_OPS_WAKELOCK), y)
TARGET_IF_OBJ += $(TARGET_IF_DIR)/mlme/psoc/src/target_if_psoc_wake_lock.o
endif

TARGET_IF_OBJ += $(TARGET_IF_DIR)/crypto/src/target_if_crypto.o

ifeq ($(CONFIG_IPA_OFFLOAD), y)
TARGET_IF_OBJ += $(TARGET_IF_DIR)/ipa/src/target_if_ipa.o
endif

$(call add-wlan-objs,target_if,$(TARGET_IF_OBJ))

########### GLOBAL_LMAC_IF ##########
GLOBAL_LMAC_IF_DIR := $(WLAN_COMMON_ROOT)/global_lmac_if

GLOBAL_LMAC_IF_INC := -I$(WLAN_COMMON_INC)/global_lmac_if/inc \

GLOBAL_LMAC_IF_OBJ := $(GLOBAL_LMAC_IF_DIR)/src/wlan_global_lmac_if.o

$(call add-wlan-objs,global_lmac_if,$(GLOBAL_LMAC_IF_OBJ))

########### WMI ###########
WMI_ROOT_DIR := wmi

WMI_SRC_DIR := $(WMI_ROOT_DIR)/src
WMI_INC_DIR := $(WMI_ROOT_DIR)/inc
WMI_OBJ_DIR := $(WLAN_COMMON_ROOT)/$(WMI_SRC_DIR)

WMI_INC := -I$(WLAN_COMMON_INC)/$(WMI_INC_DIR)

WMI_OBJS := $(WMI_OBJ_DIR)/wmi_unified.o \
	    $(WMI_OBJ_DIR)/wmi_tlv_helper.o \
	    $(WMI_OBJ_DIR)/wmi_unified_tlv.o \
	    $(WMI_OBJ_DIR)/wmi_unified_api.o \
	    $(WMI_OBJ_DIR)/wmi_unified_reg_api.o \
	    $(WMI_OBJ_DIR)/wmi_unified_vdev_api.o \
	    $(WMI_OBJ_DIR)/wmi_unified_vdev_tlv.o \
	    $(WMI_OBJ_DIR)/wmi_unified_crypto_api.o

ifeq ($(CONFIG_POWER_MANAGEMENT_OFFLOAD), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_pmo_api.o
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_pmo_tlv.o
endif

ifeq ($(CONFIG_QCACLD_FEATURE_APF), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_apf_tlv.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_ACTION_OUI), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_action_oui_tlv.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_DSRC), y)
ifeq ($(CONFIG_OCB_UT_FRAMEWORK), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_ocb_ut.o
endif
endif

ifeq ($(CONFIG_WLAN_DFS_MASTER_ENABLE), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_dfs_api.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_TWT), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_twt_api.o
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_twt_tlv.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_DSRC), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_ocb_api.o
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_ocb_tlv.o
endif

ifeq ($(CONFIG_FEATURE_WLAN_EXTSCAN), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_extscan_api.o
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_extscan_tlv.o
endif

ifeq ($(CONFIG_FEATURE_INTEROP_ISSUES_AP), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_interop_issues_ap_api.o
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_interop_issues_ap_tlv.o
endif

ifeq ($(CONFIG_DCS), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_dcs_api.o
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_dcs_tlv.o
endif

ifeq ($(CONFIG_QCACLD_FEATURE_NAN), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_nan_api.o
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_nan_tlv.o
endif

ifeq ($(CONFIG_CONVERGED_P2P_ENABLE), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_p2p_api.o
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_p2p_tlv.o
endif

ifeq ($(CONFIG_WMI_CONCURRENCY_SUPPORT), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_concurrency_api.o
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_concurrency_tlv.o
endif

ifeq ($(CONFIG_WMI_STA_SUPPORT), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_sta_api.o
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_sta_tlv.o
endif

ifeq ($(CONFIG_WMI_BCN_OFFLOAD), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_bcn_api.o
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_bcn_tlv.o
endif

ifeq ($(CONFIG_WLAN_FW_OFFLOAD), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_fwol_api.o
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_fwol_tlv.o
endif

ifeq ($(CONFIG_WLAN_HANG_EVENT), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_hang_event.o
endif

ifeq ($(CONFIG_WLAN_CFR_ENABLE), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_cfr_tlv.o
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_cfr_api.o
endif

ifeq ($(CONFIG_CP_STATS), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_cp_stats_api.o
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_cp_stats_tlv.o
endif

ifeq ($(CONFIG_FEATURE_GPIO_CFG), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_gpio_api.o
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_gpio_tlv.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_11BE_MLO), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_11be_tlv.o
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_11be_api.o
endif

ifeq ($(CONFIG_FEATURE_WDS), y)
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_wds_api.o
WMI_OBJS += $(WMI_OBJ_DIR)/wmi_unified_wds_tlv.o
endif

$(call add-wlan-objs,wmi,$(WMI_OBJS))

########### FWLOG ###########
FWLOG_DIR := $(WLAN_COMMON_ROOT)/utils/fwlog

FWLOG_INC := -I$(WLAN_ROOT)/$(FWLOG_DIR)

ifeq ($(CONFIG_FEATURE_FW_LOG_PARSING), y)
FWLOG_OBJS := $(FWLOG_DIR)/dbglog_host.o
endif

$(call add-wlan-objs,fwlog,$(FWLOG_OBJS))

############ TXRX ############
TXRX_DIR :=     core/dp/txrx
TXRX_INC :=     -I$(WLAN_ROOT)/$(TXRX_DIR)

TXRX_OBJS :=
ifeq ($(CONFIG_WDI_EVENT_ENABLE), y)
TXRX_OBJS += 	$(TXRX_DIR)/ol_txrx_event.o
endif

ifneq (y,$(filter y,$(CONFIG_LITHIUM) $(CONFIG_BERYLLIUM) $(CONFIG_RHINE)))
TXRX_OBJS += $(TXRX_DIR)/ol_txrx.o \
		$(TXRX_DIR)/ol_cfg.o \
                $(TXRX_DIR)/ol_rx.o \
                $(TXRX_DIR)/ol_rx_fwd.o \
                $(TXRX_DIR)/ol_txrx.o \
                $(TXRX_DIR)/ol_rx_defrag.o \
                $(TXRX_DIR)/ol_tx_desc.o \
                $(TXRX_DIR)/ol_tx.o \
                $(TXRX_DIR)/ol_rx_reorder_timeout.o \
                $(TXRX_DIR)/ol_rx_reorder.o \
                $(TXRX_DIR)/ol_rx_pn.o \
                $(TXRX_DIR)/ol_txrx_peer_find.o \
                $(TXRX_DIR)/ol_txrx_encap.o \
                $(TXRX_DIR)/ol_tx_send.o

ifeq ($(CONFIG_LL_DP_SUPPORT), y)

TXRX_OBJS +=     $(TXRX_DIR)/ol_tx_ll.o

ifeq ($(CONFIG_WLAN_FASTPATH), y)
TXRX_OBJS +=     $(TXRX_DIR)/ol_tx_ll_fastpath.o
else
TXRX_OBJS +=     $(TXRX_DIR)/ol_tx_ll_legacy.o
endif

ifeq ($(CONFIG_WLAN_TX_FLOW_CONTROL_V2), y)
TXRX_OBJS +=     $(TXRX_DIR)/ol_txrx_flow_control.o
endif

endif #CONFIG_LL_DP_SUPPORT

ifeq ($(CONFIG_HL_DP_SUPPORT), y)
TXRX_OBJS +=     $(TXRX_DIR)/ol_tx_hl.o
TXRX_OBJS +=     $(TXRX_DIR)/ol_tx_classify.o
TXRX_OBJS +=     $(TXRX_DIR)/ol_tx_sched.o
TXRX_OBJS +=     $(TXRX_DIR)/ol_tx_queue.o
endif #CONFIG_HL_DP_SUPPORT

ifeq ($(CONFIG_WLAN_TX_FLOW_CONTROL_LEGACY), y)
TXRX_OBJS +=     $(TXRX_DIR)/ol_txrx_legacy_flow_control.o
endif

ifeq ($(CONFIG_IPA_OFFLOAD), y)
TXRX_OBJS +=     $(TXRX_DIR)/ol_txrx_ipa.o
endif

ifeq ($(CONFIG_QCA_SUPPORT_TX_THROTTLE), y)
TXRX_OBJS +=     $(TXRX_DIR)/ol_tx_throttle.o
endif
endif #LITHIUM/BERYLLIUM/RHINE

$(call add-wlan-objs,txrx,$(TXRX_OBJS))

ifeq (y,$(filter y,$(CONFIG_LITHIUM) $(CONFIG_BERYLLIUM) $(CONFIG_RHINE)))
############ DP 3.0 ############
DP_INC := -I$(WLAN_COMMON_INC)/dp/inc \
	-I$(WLAN_COMMON_INC)/dp/wifi3.0 \
	-I$(WLAN_COMMON_INC)/target_if/dp/inc \
	-I$(WLAN_COMMON_INC)/dp/cmn_dp_api

DP_SRC := $(WLAN_COMMON_ROOT)/dp/wifi3.0
DP_OBJS := $(DP_SRC)/dp_main.o \
		$(DP_SRC)/dp_tx.o \
		$(DP_SRC)/dp_arch_ops.o \
		$(DP_SRC)/dp_tx_desc.o \
		$(DP_SRC)/dp_rx.o \
		$(DP_SRC)/dp_htt.o \
		$(DP_SRC)/dp_peer.o \
		$(DP_SRC)/dp_rx_desc.o \
		$(DP_SRC)/dp_rx_defrag.o \
		$(DP_SRC)/dp_stats.o \
		$(WLAN_COMMON_ROOT)/target_if/dp/src/target_if_dp.o

ifneq ($(CONFIG_RHINE), y)
DP_OBJS += $(DP_SRC)/dp_rings_main.o
DP_OBJS += $(DP_SRC)/dp_reo.o
DP_OBJS += $(DP_SRC)/dp_rx_err.o
DP_OBJS += $(DP_SRC)/dp_rx_tid.o
endif

ifeq ($(CONFIG_WIFI_MONITOR_SUPPORT), y)
DP_INC += -I$(WLAN_COMMON_INC)/dp/wifi3.0/monitor \
	-I$(WLAN_COMMON_INC)/dp/wifi3.0/monitor/1.0 \
	-I$(WLAN_COMMON_INC)/dp/wifi3.0/monitor/2.0 \

DP_OBJS += $(DP_SRC)/monitor/dp_mon.o \
		$(DP_SRC)/monitor/dp_mon_filter.o \
		$(DP_SRC)/monitor/dp_rx_mon.o \
		$(DP_SRC)/monitor/1.0/dp_rx_mon_dest_1.0.o \
		$(DP_SRC)/monitor/1.0/dp_rx_mon_status_1.0.o \
		$(DP_SRC)/monitor/1.0/dp_mon_filter_1.0.o \
		$(DP_SRC)/monitor/1.0/dp_mon_1.0.o
endif

DP_OBJS += $(DP_SRC)/../cmn_dp_api/dp_ratetable.o

ifeq ($(CONFIG_BERYLLIUM), y)
DP_INC += -I$(WLAN_COMMON_INC)/dp/wifi3.0/be

DP_OBJS += $(DP_SRC)/be/dp_be.o
DP_OBJS += $(DP_SRC)/be/dp_be_tx.o
DP_OBJS += $(DP_SRC)/be/dp_be_rx.o

ifeq ($(CONFIG_WIFI_MONITOR_SUPPORT), y)
ifeq ($(CONFIG_WLAN_TX_MON_2_0), y)
DP_OBJS += $(DP_SRC)/monitor/2.0/dp_mon_2.0.o \
		$(DP_SRC)/monitor/2.0/dp_mon_filter_2.0.o
DP_OBJS += $(DP_SRC)/monitor/2.0/dp_tx_mon_2.0.o \
		$(DP_SRC)/monitor/2.0/dp_tx_mon_status_2.0.o
ccflags-$(CONFIG_WLAN_TX_MON_2_0) += -DWLAN_PKT_CAPTURE_TX_2_0
ccflags-y += -DWLAN_TX_PKT_CAPTURE_ENH_BE
ccflags-y += -DQDF_FRAG_CACHE_SUPPORT
endif
endif
endif

ifeq ($(CONFIG_LITHIUM), y)
DP_OBJS += $(DP_SRC)/li/dp_li.o
DP_OBJS += $(DP_SRC)/li/dp_li_tx.o
DP_OBJS += $(DP_SRC)/li/dp_li_rx.o
endif

ifeq ($(CONFIG_RHINE), y)
DP_OBJS += $(DP_SRC)/rh/dp_rh.o
DP_OBJS += $(DP_SRC)/rh/dp_rh_tx.o
DP_OBJS += $(DP_SRC)/rh/dp_rh_rx.o
DP_OBJS += $(DP_SRC)/rh/dp_rh_htt.o
endif

ifeq ($(CONFIG_WLAN_TX_FLOW_CONTROL_V2), y)
DP_OBJS += $(DP_SRC)/dp_tx_flow_control.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_RX_BUFFER_POOL), y)
DP_OBJS += $(DP_SRC)/dp_rx_buffer_pool.o
endif

ifeq ($(CONFIG_IPA_OFFLOAD), y)
DP_OBJS +=     $(DP_SRC)/dp_ipa.o
endif

ifeq ($(CONFIG_WDI_EVENT_ENABLE), y)
DP_OBJS +=     $(DP_SRC)/dp_wdi_event.o
endif

ifeq ($(CONFIG_FEATURE_MEC), y)
DP_OBJS += $(DP_SRC)/dp_txrx_wds.o
endif

ifeq ($(CONFIG_QCACLD_FEATURE_SON), y)
DP_OBJS += $(WLAN_COMMON_ROOT)/dp/cmn_dp_api/dp_ratetable.o
DP_INC += -I$(WLAN_COMMON_INC)/dp/cmn_dp_api
endif

endif #LITHIUM

$(call add-wlan-objs,dp,$(DP_OBJS))

############ CFG ############
WCFG_DIR := wlan_cfg
WCFG_INC := -I$(WLAN_COMMON_INC)/$(WCFG_DIR)
WCFG_SRC := $(WLAN_COMMON_ROOT)/$(WCFG_DIR)

ifeq (y,$(filter y,$(CONFIG_LITHIUM) $(CONFIG_BERYLLIUM) $(CONFIG_RHINE)))
WCFG_OBJS := $(WCFG_SRC)/wlan_cfg.o
endif

$(call add-wlan-objs,wcfg,$(WCFG_OBJS))

############ OL ############
OL_DIR :=     core/dp/ol
OL_INC :=     -I$(WLAN_ROOT)/$(OL_DIR)/inc

############ CDP ############
CDP_ROOT_DIR := dp
CDP_INC_DIR := $(CDP_ROOT_DIR)/inc
CDP_INC := -I$(WLAN_COMMON_INC)/$(CDP_INC_DIR)

############ PKTLOG ############
PKTLOG_DIR :=      $(WLAN_COMMON_ROOT)/utils/pktlog
PKTLOG_INC :=      -I$(WLAN_ROOT)/$(PKTLOG_DIR)/include

ifeq ($(CONFIG_REMOVE_PKT_LOG), n)
PKTLOG_OBJS :=	$(PKTLOG_DIR)/pktlog_ac.o \
		$(PKTLOG_DIR)/pktlog_internal.o \
		$(PKTLOG_DIR)/linux_ac.o

ifeq ($(CONFIG_PKTLOG_LEGACY), y)
	PKTLOG_OBJS  += $(PKTLOG_DIR)/pktlog_wifi2.o
else
	PKTLOG_OBJS  += $(PKTLOG_DIR)/pktlog_wifi3.o
endif

endif


$(call add-wlan-objs,pktlog,$(PKTLOG_OBJS))

############ HTT ############
HTT_DIR :=      core/dp/htt
HTT_INC :=      -I$(WLAN_ROOT)/$(HTT_DIR)

ifneq (y,$(filter y,$(CONFIG_LITHIUM) $(CONFIG_BERYLLIUM) $(CONFIG_RHINE)))
HTT_OBJS := $(HTT_DIR)/htt_tx.o \
            $(HTT_DIR)/htt.o \
            $(HTT_DIR)/htt_t2h.o \
            $(HTT_DIR)/htt_h2t.o \
            $(HTT_DIR)/htt_fw_stats.o \
            $(HTT_DIR)/htt_rx.o

ifeq ($(CONFIG_FEATURE_MONITOR_MODE_SUPPORT), y)
HTT_OBJS += $(HTT_DIR)/htt_monitor_rx.o
endif

ifeq ($(CONFIG_LL_DP_SUPPORT), y)
HTT_OBJS += $(HTT_DIR)/htt_rx_ll.o
endif

ifeq ($(CONFIG_HL_DP_SUPPORT), y)
HTT_OBJS += $(HTT_DIR)/htt_rx_hl.o
endif
endif

$(call add-wlan-objs,htt,$(HTT_OBJS))

############## INIT-DEINIT ###########
INIT_DEINIT_DIR := init_deinit/dispatcher
INIT_DEINIT_INC_DIR := $(INIT_DEINIT_DIR)/inc
INIT_DEINIT_SRC_DIR := $(INIT_DEINIT_DIR)/src
INIT_DEINIT_OBJ_DIR := $(WLAN_COMMON_ROOT)/$(INIT_DEINIT_SRC_DIR)
INIT_DEINIT_INC := -I$(WLAN_COMMON_INC)/$(INIT_DEINIT_INC_DIR)
INIT_DEINIT_OBJS := $(INIT_DEINIT_OBJ_DIR)/dispatcher_init_deinit.o

$(call add-wlan-objs,init_deinit,$(INIT_DEINIT_OBJS))

############## REGULATORY ###########
REGULATORY_DIR := umac/regulatory
REGULATORY_CORE_SRC_DIR := $(REGULATORY_DIR)/core/src
REG_DISPATCHER_INC_DIR := $(REGULATORY_DIR)/dispatcher/inc
REG_DISPATCHER_SRC_DIR := $(REGULATORY_DIR)/dispatcher/src
REG_CORE_OBJ_DIR := $(WLAN_COMMON_ROOT)/$(REGULATORY_CORE_SRC_DIR)
REG_DISPATCHER_OBJ_DIR := $(WLAN_COMMON_ROOT)/$(REG_DISPATCHER_SRC_DIR)
REGULATORY_INC := -I$(WLAN_COMMON_INC)/$(REGULATORY_CORE_SRC_DIR)
REGULATORY_INC += -I$(WLAN_COMMON_INC)/$(REG_DISPATCHER_INC_DIR)
REGULATORY_INC += -I$(WLAN_COMMON_INC)/umac/cmn_services/regulatory/inc
REGULATORY_OBJS := $(REG_CORE_OBJ_DIR)/reg_build_chan_list.o \
		    $(REG_CORE_OBJ_DIR)/reg_callbacks.o \
		    $(REG_CORE_OBJ_DIR)/reg_db.o \
		    $(REG_CORE_OBJ_DIR)/reg_db_parser.o \
		    $(REG_CORE_OBJ_DIR)/reg_utils.o \
		    $(REG_CORE_OBJ_DIR)/reg_lte.o \
		    $(REG_CORE_OBJ_DIR)/reg_offload_11d_scan.o \
		    $(REG_CORE_OBJ_DIR)/reg_opclass.o \
		    $(REG_CORE_OBJ_DIR)/reg_priv_objs.o \
		    $(REG_DISPATCHER_OBJ_DIR)/wlan_reg_services_api.o \
		    $(REG_CORE_OBJ_DIR)/reg_services_common.o \
		    $(REG_DISPATCHER_OBJ_DIR)/wlan_reg_tgt_api.o \
		    $(REG_DISPATCHER_OBJ_DIR)/wlan_reg_ucfg_api.o
ifeq ($(CONFIG_HOST_11D_SCAN), y)
REGULATORY_OBJS += $(REG_CORE_OBJ_DIR)/reg_host_11d.o
endif

$(call add-wlan-objs,regulatory,$(REGULATORY_OBJS))

############## Control path common scheduler ##########
SCHEDULER_DIR := scheduler
SCHEDULER_INC_DIR := $(SCHEDULER_DIR)/inc
SCHEDULER_SRC_DIR := $(SCHEDULER_DIR)/src
SCHEDULER_OBJ_DIR := $(WLAN_COMMON_ROOT)/$(SCHEDULER_SRC_DIR)
SCHEDULER_INC := -I$(WLAN_COMMON_INC)/$(SCHEDULER_INC_DIR)
SCHEDULER_OBJS := $(SCHEDULER_OBJ_DIR)/scheduler_api.o \
                  $(SCHEDULER_OBJ_DIR)/scheduler_core.o

$(call add-wlan-objs,scheduler,$(SCHEDULER_OBJS))

###### UMAC SERIALIZATION ########
UMAC_SER_DIR := umac/cmn_services/serialization
UMAC_SER_INC_DIR := $(UMAC_SER_DIR)/inc
UMAC_SER_SRC_DIR := $(UMAC_SER_DIR)/src
UMAC_SER_OBJ_DIR := $(WLAN_COMMON_ROOT)/$(UMAC_SER_SRC_DIR)

UMAC_SER_INC := -I$(WLAN_COMMON_INC)/$(UMAC_SER_INC_DIR)
UMAC_SER_OBJS := $(UMAC_SER_OBJ_DIR)/wlan_serialization_main.o \
		 $(UMAC_SER_OBJ_DIR)/wlan_serialization_api.o \
		 $(UMAC_SER_OBJ_DIR)/wlan_serialization_utils.o \
		 $(UMAC_SER_OBJ_DIR)/wlan_serialization_legacy_api.o \
		 $(UMAC_SER_OBJ_DIR)/wlan_serialization_rules.o \
		 $(UMAC_SER_OBJ_DIR)/wlan_serialization_internal.o \
		 $(UMAC_SER_OBJ_DIR)/wlan_serialization_non_scan.o \
		 $(UMAC_SER_OBJ_DIR)/wlan_serialization_queue.o \
		 $(UMAC_SER_OBJ_DIR)/wlan_serialization_scan.o

$(call add-wlan-objs,umac_ser,$(UMAC_SER_OBJS))

###### WIFI POS ########
WIFI_POS_OS_IF_DIR := $(WLAN_COMMON_ROOT)/os_if/linux/wifi_pos/src
WIFI_POS_OS_IF_INC := -I$(WLAN_COMMON_INC)/os_if/linux/wifi_pos/inc
WIFI_POS_TGT_DIR := $(WLAN_COMMON_ROOT)/target_if/wifi_pos/src
WIFI_POS_TGT_INC := -I$(WLAN_COMMON_INC)/target_if/wifi_pos/inc
WIFI_POS_CORE_DIR := $(WLAN_COMMON_ROOT)/umac/wifi_pos/src
WIFI_POS_API_INC := -I$(WLAN_COMMON_INC)/umac/wifi_pos/inc


ifeq ($(CONFIG_WIFI_POS_CONVERGED), y)

WIFI_POS_CLD_DIR := components/wifi_pos
WIFI_POS_CLD_CORE_DIR := $(WIFI_POS_CLD_DIR)/core
WIFI_POS_CLD_CORE_SRC := $(WIFI_POS_CLD_CORE_DIR)/src
WIFI_POS_CLD_DISP_DIR := $(WIFI_POS_CLD_DIR)/dispatcher

WIFI_POS_OBJS := $(WIFI_POS_CORE_DIR)/wifi_pos_api.o \
		 $(WIFI_POS_CORE_DIR)/wifi_pos_main.o \
		 $(WIFI_POS_CORE_DIR)/wifi_pos_ucfg.o \
		 $(WIFI_POS_CORE_DIR)/wifi_pos_utils.o \
		 $(WIFI_POS_CLD_DISP_DIR)/src/wifi_pos_ucfg_api.o \
		 $(WIFI_POS_OS_IF_DIR)/os_if_wifi_pos.o \
		 $(WIFI_POS_OS_IF_DIR)/os_if_wifi_pos_utils.o \
		 $(WIFI_POS_OS_IF_DIR)/wlan_cfg80211_wifi_pos.o \
		 $(WIFI_POS_TGT_DIR)/target_if_wifi_pos.o \
		 $(WIFI_POS_TGT_DIR)/target_if_wifi_pos_rx_ops.o \
		 $(WIFI_POS_TGT_DIR)/target_if_wifi_pos_tx_ops.o

ifeq ($(CONFIG_WIFI_POS_PASN), y)
WIFI_POS_OBJS += $(WIFI_POS_CORE_DIR)/wifi_pos_pasn_api.o
WIFI_POS_OBJS += $(WIFI_POS_CLD_CORE_SRC)/wlan_wifi_pos_interface.o
endif

WIFI_POS_CLD_INC :=	-I$(WLAN_ROOT)/$(WIFI_POS_CLD_CORE_DIR)/inc \
			-I$(WLAN_ROOT)/$(WIFI_POS_CLD_DISP_DIR)/inc
endif

$(call add-wlan-objs,wifi_pos,$(WIFI_POS_OBJS))

###### TWT CONVERGED ########
TWT_CONV_CMN_OSIF_SRC := $(WLAN_COMMON_ROOT)/os_if/linux/twt/src
TWT_CONV_CMN_DISPATCHER_SRC := $(WLAN_COMMON_ROOT)/umac/twt/dispatcher/src
TWT_CONV_CMN_CORE_SRC := $(WLAN_COMMON_ROOT)/umac/twt/core/src
TWT_CONV_CMN_TGT_SRC := $(WLAN_COMMON_ROOT)/target_if/twt/src
TWT_CONV_OSIF_SRC := os_if/twt/src
TWT_CONV_DISPATCHER_SRC := components/umac/twt/dispatcher/src
TWT_CONV_CORE_SRC := components/umac/twt/core/src
TWT_CONV_TGT_SRC := components/target_if/twt/src

TWT_CONV_INCS := -I$(WLAN_COMMON_INC)/umac \
		 -I$(WLAN_ROOT)/components/umac \
		 -I$(WLAN_COMMON_INC)/os_if/linux/twt/inc \
		 -I$(WLAN_COMMON_INC)/umac/twt/dispatcher/inc \
		 -I$(WLAN_COMMON_INC)/target_if/twt/inc \
		 -I$(WLAN_ROOT)/os_if/twt/inc \
		 -I$(WLAN_ROOT)/components/umac/twt/dispatcher/inc \
		 -I$(WLAN_ROOT)/components/target_if/twt/inc


ifeq ($(CONFIG_WLAN_TWT_CONVERGED), y)
TWT_CONV_OBJS := $(TWT_CONV_CMN_OSIF_SRC)/osif_twt_req.o \
		 $(TWT_CONV_CMN_OSIF_SRC)/osif_twt_rsp.o \
		 $(TWT_CONV_CMN_DISPATCHER_SRC)/wlan_twt_api.o \
		 $(TWT_CONV_CMN_DISPATCHER_SRC)/wlan_twt_tgt_if_rx_api.o \
		 $(TWT_CONV_CMN_DISPATCHER_SRC)/wlan_twt_tgt_if_tx_api.o \
		 $(TWT_CONV_CMN_DISPATCHER_SRC)/wlan_twt_ucfg_api.o \
		 $(TWT_CONV_CMN_CORE_SRC)/wlan_twt_common.o \
		 $(TWT_CONV_CMN_CORE_SRC)/wlan_twt_objmgr.o \
		 $(TWT_CONV_CMN_TGT_SRC)/target_if_twt_cmd.o \
		 $(TWT_CONV_CMN_TGT_SRC)/target_if_twt_evt.o \
		 $(TWT_CONV_CMN_TGT_SRC)/target_if_twt.o \
		 $(TWT_CONV_OSIF_SRC)/osif_twt_ext_req.o \
		 $(TWT_CONV_OSIF_SRC)/osif_twt_ext_rsp.o \
		 $(TWT_CONV_OSIF_SRC)/osif_twt_ext_util.o \
		 $(TWT_CONV_DISPATCHER_SRC)/wlan_twt_ucfg_ext_api.o \
		 $(TWT_CONV_DISPATCHER_SRC)/wlan_twt_cfg_ext_api.o \
		 $(TWT_CONV_DISPATCHER_SRC)/wlan_twt_tgt_if_ext_rx_api.o \
		 $(TWT_CONV_DISPATCHER_SRC)/wlan_twt_tgt_if_ext_tx_api.o \
		 $(TWT_CONV_CORE_SRC)/wlan_twt_cfg.o \
		 $(TWT_CONV_CORE_SRC)/wlan_twt_main.o \
		 $(TWT_CONV_TGT_SRC)/target_if_ext_twt_cmd.o \
		 $(TWT_CONV_TGT_SRC)/target_if_ext_twt_evt.o
endif

$(call add-wlan-objs,twt_conv,$(TWT_CONV_OBJS))

###### CP STATS ########
CP_MC_STATS_OS_IF_SRC           := os_if/cp_stats/src
CP_STATS_TGT_SRC                := $(WLAN_COMMON_ROOT)/target_if/cp_stats/src
CP_STATS_CORE_SRC               := $(WLAN_COMMON_ROOT)/umac/cp_stats/core/src
CP_STATS_DISPATCHER_SRC         := $(WLAN_COMMON_ROOT)/umac/cp_stats/dispatcher/src
CP_MC_STATS_COMPONENT_SRC       := components/cp_stats/dispatcher/src
CP_MC_STATS_COMPONENT_TGT_SRC   := $(CLD_TARGET_IF_DIR)/cp_stats/src

CP_STATS_OS_IF_INC              := -I$(WLAN_COMMON_INC)/os_if/linux/cp_stats/inc
CP_STATS_TGT_INC                := -I$(WLAN_COMMON_INC)/target_if/cp_stats/inc
CP_STATS_DISPATCHER_INC         := -I$(WLAN_COMMON_INC)/umac/cp_stats/dispatcher/inc
CP_MC_STATS_COMPONENT_INC       := -I$(WLAN_ROOT)/components/cp_stats/dispatcher/inc
CP_STATS_CFG80211_OS_IF_INC     := -I$(WLAN_ROOT)/os_if/cp_stats/inc

ifeq ($(CONFIG_CP_STATS), y)
CP_STATS_OBJS := $(CP_MC_STATS_COMPONENT_SRC)/wlan_cp_stats_mc_tgt_api.o	\
		 $(CP_MC_STATS_COMPONENT_SRC)/wlan_cp_stats_mc_ucfg_api.o	\
		 $(CP_MC_STATS_COMPONENT_TGT_SRC)/target_if_mc_cp_stats.o	\
		 $(CP_STATS_CORE_SRC)/wlan_cp_stats_comp_handler.o		\
		 $(CP_STATS_CORE_SRC)/wlan_cp_stats_obj_mgr_handler.o		\
		 $(CP_STATS_CORE_SRC)/wlan_cp_stats_ol_api.o			\
		 $(CP_MC_STATS_OS_IF_SRC)/wlan_cfg80211_mc_cp_stats.o		\
		 $(CP_STATS_DISPATCHER_SRC)/wlan_cp_stats_utils_api.o		\
		 $(WLAN_COMMON_ROOT)/target_if/cp_stats/src/target_if_cp_stats.o	\
		 $(CP_STATS_DISPATCHER_SRC)/wlan_cp_stats_ucfg_api.o

endif

$(call add-wlan-objs,cp_stats,$(CP_STATS_OBJS))

###### DCS ######
DCS_TGT_IF_SRC := $(WLAN_COMMON_ROOT)/target_if/dcs/src
DCS_CORE_SRC   := $(WLAN_COMMON_ROOT)/umac/dcs/core/src
DCS_DISP_SRC   := $(WLAN_COMMON_ROOT)/umac/dcs/dispatcher/src

DCS_TGT_IF_INC := -I$(WLAN_COMMON_INC)/target_if/dcs/inc
DCS_DISP_INC   := -I$(WLAN_COMMON_INC)/umac/dcs/dispatcher/inc

ifeq ($(CONFIG_DCS), y)
DCS_OBJS := $(DCS_TGT_IF_SRC)/target_if_dcs.o \
	$(DCS_CORE_SRC)/wlan_dcs.o \
	$(DCS_DISP_SRC)/wlan_dcs_init_deinit_api.o \
	$(DCS_DISP_SRC)/wlan_dcs_ucfg_api.o \
	$(DCS_DISP_SRC)/wlan_dcs_tgt_api.o
endif

$(call add-wlan-objs,dcs,$(DCS_OBJS))

####### AFC ######
AFC_CMN_OSIF_SRC  := $(WLAN_COMMON_ROOT)/os_if/linux/afc/src
AFC_CMN_CORE_SRC  := $(WLAN_COMMON_ROOT)/umac/afc/core/src
AFC_CMN_DISP_SRC  := $(WLAN_COMMON_ROOT)/umac/afc/dispatcher/src

AFC_CMN_OSIF_INC  := -I$(WLAN_COMMON_INC)/os_if/linux/afc/inc
AFC_CMN_DISP_INC  := -I$(WLAN_COMMON_INC)/umac/afc/dispatcher/inc
AFC_CMN_CORE_INC  := -I$(WLAN_COMMON_INC)/umac/afc/core/inc

ifeq ($(CONFIG_AFC_SUPPORT), y)
AFC_OBJS := $(AFC_CMN_OSIF_SRC)/wlan_cfg80211_afc.o \
	    $(AFC_CMN_CORE_SRC)/wlan_afc_main.o \
	    $(AFC_CMN_DISP_SRC)/wlan_afc_ucfg_api.o
endif

$(call add-wlan-objs,afc,$(AFC_OBJS))

###### INTEROP ISSUES AP ########
INTEROP_ISSUES_AP_OS_IF_SRC      := os_if/interop_issues_ap/src
INTEROP_ISSUES_AP_TGT_SRC        := components/target_if/interop_issues_ap/src
INTEROP_ISSUES_AP_CORE_SRC       := components/interop_issues_ap/core/src
INTEROP_ISSUES_AP_DISPATCHER_SRC := components/interop_issues_ap/dispatcher/src

INTEROP_ISSUES_AP_OS_IF_INC      := -I$(WLAN_ROOT)/os_if/interop_issues_ap/inc
INTEROP_ISSUES_AP_TGT_INC        := -I$(WLAN_ROOT)/components/target_if/interop_issues_ap/inc
INTEROP_ISSUES_AP_DISPATCHER_INC := -I$(WLAN_ROOT)/components/interop_issues_ap/dispatcher/inc
INTEROP_ISSUES_AP_CORE_INC       := -I$(WLAN_ROOT)/components/interop_issues_ap/core/inc

ifeq ($(CONFIG_FEATURE_INTEROP_ISSUES_AP), y)
INTEROP_ISSUES_AP_OBJS := $(INTEROP_ISSUES_AP_TGT_SRC)/target_if_interop_issues_ap.o \
		$(INTEROP_ISSUES_AP_CORE_SRC)/wlan_interop_issues_ap_api.o \
		$(INTEROP_ISSUES_AP_OS_IF_SRC)/wlan_cfg80211_interop_issues_ap.o \
		$(INTEROP_ISSUES_AP_DISPATCHER_SRC)/wlan_interop_issues_ap_tgt_api.o \
		$(INTEROP_ISSUES_AP_DISPATCHER_SRC)/wlan_interop_issues_ap_ucfg_api.o
endif

$(call add-wlan-objs,interop_issues_ap,$(INTEROP_ISSUES_AP_OBJS))

######################### NAN #########################
NAN_CORE_DIR := components/nan/core/src
NAN_CORE_INC := -I$(WLAN_ROOT)/components/nan/core/inc
NAN_UCFG_DIR := components/nan/dispatcher/src
NAN_UCFG_INC := -I$(WLAN_ROOT)/components/nan/dispatcher/inc
NAN_TGT_DIR  := components/target_if/nan/src
NAN_TGT_INC  := -I$(WLAN_ROOT)/components/target_if/nan/inc

NAN_OS_IF_DIR  := os_if/nan/src
NAN_OS_IF_INC  := -I$(WLAN_ROOT)/os_if/nan/inc

ifeq ($(CONFIG_QCACLD_FEATURE_NAN), y)
WLAN_NAN_OBJS := $(NAN_CORE_DIR)/nan_main.o \
		 $(NAN_CORE_DIR)/nan_api.o \
		 $(NAN_UCFG_DIR)/nan_ucfg_api.o \
		 $(NAN_UCFG_DIR)/wlan_nan_api.o \
		 $(NAN_UCFG_DIR)/cfg_nan.o \
		 $(NAN_TGT_DIR)/target_if_nan.o \
		 $(NAN_OS_IF_DIR)/os_if_nan.o
endif

$(call add-wlan-objs,nan,$(WLAN_NAN_OBJS))

#######################################################

######################### DP_COMPONENT #########################
DP_COMP_CORE_DIR := components/dp/core/src
DP_COMP_UCFG_DIR := components/dp/dispatcher/src
DP_COMP_TGT_DIR  := components/target_if/dp/src
DP_COMP_OS_IF_DIR  := os_if/dp/src

DP_COMP_INC	:= -I$(WLAN_ROOT)/components/dp/core/inc	\
		-I$(WLAN_ROOT)/components/dp/core/src		\
		-I$(WLAN_ROOT)/components/dp/dispatcher/inc	\
		-I$(WLAN_ROOT)/components/target_if/dp/inc	\
		-I$(WLAN_ROOT)/os_if/dp/inc

WLAN_DP_COMP_OBJS := $(DP_COMP_CORE_DIR)/wlan_dp_main.o \
		 $(DP_COMP_UCFG_DIR)/wlan_dp_ucfg_api.o \
		$(DP_COMP_UCFG_DIR)/wlan_dp_api.o \
		 $(DP_COMP_OS_IF_DIR)/os_if_dp.o \
		 $(DP_COMP_OS_IF_DIR)/os_if_dp_txrx.o \
		 $(DP_COMP_CORE_DIR)/wlan_dp_bus_bandwidth.o \
		 $(DP_COMP_CORE_DIR)/wlan_dp_softap_txrx.o \
		 $(DP_COMP_CORE_DIR)/wlan_dp_txrx.o \
		 $(DP_COMP_TGT_DIR)/target_if_dp_comp.o

ifeq ($(CONFIG_WLAN_LRO), y)
WLAN_DP_COMP_OBJS += $(DP_COMP_OS_IF_DIR)/os_if_dp_lro.o
endif

ifeq ($(CONFIG_WLAN_NUD_TRACKING), y)
WLAN_DP_COMP_OBJS += $(DP_COMP_CORE_DIR)/wlan_dp_nud_tracking.o
endif

ifeq ($(CONFIG_WLAN_FEATURE_PERIODIC_STA_STATS), y)
WLAN_DP_COMP_OBJS += $(DP_COMP_CORE_DIR)/wlan_dp_periodic_sta_stats.o
endif

ifeq ($(CONFIG_DP_SWLM), y)
WLAN_DP_COMP_OBJS += $(DP_COMP_CORE_DIR)/wlan_dp_swlm.o
endif

ifeq (y,$(filter y,$(CONFIG_LITHIUM) $(CONFIG_BERYLLIUM) $(CONFIG_RHINE)))
WLAN_DP_COMP_OBJS += $(DP_COMP_CORE_DIR)/wlan_dp_prealloc.o

ifeq ($(CONFIG_WLAN_TX_MON_2_0), y)
ifeq ($(CONFIG_WLAN_DP_LOCAL_PKT_CAPTURE), y)
WLAN_DP_COMP_OBJS += $(DP_COMP_OS_IF_DIR)/os_if_dp_local_pkt_capture.o
endif #CONFIG_WLAN_DP_LOCAL_PKT_CAPTURE
endif #CONFIG_WLAN_TX_MON_2_0
endif

ifeq ($(CONFIG_WLAN_FEATURE_DP_RX_THREADS), y)
WLAN_DP_COMP_OBJS += $(DP_COMP_CORE_DIR)/wlan_dp_rx_thread.o
endif

ifeq ($(CONFIG_RX_FISA), y)
WLAN_DP_COMP_OBJS += $(DP_COMP_CORE_DIR)/wlan_dp_fisa_rx.o
WLAN_DP_COMP_OBJS += $(DP_COMP_CORE_DIR)/wlan_dp_rx_fst.o
endif

ifeq ($(CONFIG_FEATURE_DIRECT_LINK), y)
WLAN_DP_COMP_OBJS += $(DP_COMP_CORE_DIR)/wlan_dp_wfds.o
endif

ifeq ($(CONFIG_WLAN_SUPPORT_FLOW_PRIORTIZATION), y)
WLAN_DP_COMP_OBJS += $(DP_COMP_UCFG_DIR)/wlan_dp_flow_ucfg_api.o
WLAN_DP_COMP_OBJS += $(DP_COMP_CORE_DIR)/wlan_dp_fim.o
WLAN_DP_COMP_OBJS += $(DP_COMP_CORE_DIR)/wlan_dp_fpm.o
WLAN_DP_COMP_OBJS += $(DP_COMP_OS_IF_DIR)/wlan_osif_fpm.o
endif

ifeq ($(CONFIG_WLAN_SUPPORT_SERVICE_CLASS), y)
WLAN_DP_COMP_OBJS += $(DP_COMP_CORE_DIR)/wlan_dp_svc.o
WLAN_DP_COMP_OBJS += $(DP_COMP_OS_IF_DIR)/os_if_dp_svc.o
WLAN_DP_COMP_OBJS += $(DP_COMP_UCFG_DIR)/wlan_dp_svc_ucfg_api.o
endif

ifeq ($(CONFIG_WLAN_SUPPORT_LAPB), y)
WLAN_DP_COMP_OBJS += $(DP_COMP_CORE_DIR)/wlan_dp_lapb_flow.o
endif

ifeq ($(CONFIG_WLAN_DP_LOAD_BALANCE_SUPPORT), y)
WLAN_DP_COMP_OBJS += $(DP_COMP_CORE_DIR)/wlan_dp_load_balance.o
endif

ifeq ($(CONFIG_WLAN_DP_FLOW_BALANCE_SUPPORT), y)
WLAN_DP_COMP_OBJS += $(DP_COMP_CORE_DIR)/wlan_dp_flow_balance.o
endif

$(call add-wlan-objs,dp_comp,$(WLAN_DP_COMP_OBJS))

#######################################################

######################### QMI_COMPONENT #########################
QMI_COMP_CORE_DIR := components/qmi/core/src
QMI_COMP_UCFG_DIR := components/qmi/dispatcher/src
QMI_COMP_OS_IF_DIR := os_if/qmi/src

QMI_COMP_INC := -I$(WLAN_ROOT)/components/qmi/core/inc       \
		-I$(WLAN_ROOT)/components/qmi/core/src       \
		-I$(WLAN_ROOT)/components/qmi/dispatcher/inc \
		-I$(WLAN_ROOT)/os_if/qmi/inc

ifeq ($(CONFIG_QMI_COMPONENT_ENABLE), y)
WLAN_QMI_COMP_OBJS := $(QMI_COMP_CORE_DIR)/wlan_qmi_main.o \
		 $(QMI_COMP_UCFG_DIR)/wlan_qmi_ucfg_api.o  \
		 $(QMI_COMP_OS_IF_DIR)/os_if_qmi.o

ifeq ($(CONFIG_QMI_WFDS), y)
WLAN_QMI_COMP_OBJS += $(QMI_COMP_OS_IF_DIR)/os_if_qmi_wifi_driver_service_v01.o
WLAN_QMI_COMP_OBJS += $(QMI_COMP_OS_IF_DIR)/os_if_qmi_wfds.o
WLAN_QMI_COMP_OBJS += $(QMI_COMP_UCFG_DIR)/wlan_qmi_wfds_api.o
endif
endif

$(call add-wlan-objs,qmi_comp,$(WLAN_QMI_COMP_OBJS))

#######################################################

######################### SON #########################
#SON_CORE_DIR := components/son/core/src
#SON_CORE_INC := -I$(WLAN_ROOT)/components/son/core/inc
SON_UCFG_DIR := components/son/dispatcher/src
SON_UCFG_INC := -I$(WLAN_ROOT)/components/son/dispatcher/inc
SON_TGT_DIR  := $(WLAN_COMMON_ROOT)/target_if/son/src
SON_TGT_INC  := -I$(WLAN_COMMON_INC)/target_if/son/inc/

SON_OS_IF_DIR  := os_if/son/src
SON_OS_IF_INC  := -I$(WLAN_ROOT)/os_if/son/inc

ifeq ($(CONFIG_QCACLD_FEATURE_SON), y)
WLAN_SON_OBJS := $(SON_UCFG_DIR)/son_ucfg_api.o \
		 $(SON_UCFG_DIR)/son_api.o \
		 $(SON_OS_IF_DIR)/os_if_son.o \
		 $(SON_TGT_DIR)/target_if_son.o
endif

$(call add-wlan-objs,son,$(WLAN_SON_OBJS))

#######################################################

######################### SPATIAL_REUSE #########################
SR_UCFG_DIR := components/spatial_reuse/dispatcher/src
SR_UCFG_INC := -I$(WLAN_ROOT)/components/spatial_reuse/dispatcher/inc
SR_TGT_DIR  := $(WLAN_COMMON_ROOT)/target_if/spatial_reuse/src
SR_TGT_INC  := -I$(WLAN_COMMON_INC)/target_if/spatial_reuse/inc/

ifeq ($(CONFIG_WLAN_FEATURE_SR), y)
WLAN_SR_OBJS := $(SR_UCFG_DIR)/spatial_reuse_ucfg_api.o \
		 $(SR_UCFG_DIR)/spatial_reuse_api.o \
		 $(SR_TGT_DIR)/target_if_spatial_reuse.o
endif

$(call add-wlan-objs,spatial_reuse,$(WLAN_SR_OBJS))

#######################################################

###### COEX ########
COEX_OS_IF_SRC      := os_if/coex/src
COEX_TGT_SRC        := components/target_if/coex/src
COEX_CORE_SRC       := components/coex/core/src
COEX_DISPATCHER_SRC := components/coex/dispatcher/src

COEX_OS_IF_INC      := -I$(WLAN_ROOT)/os_if/coex/inc
COEX_TGT_INC        := -I$(WLAN_ROOT)/components/target_if/coex/inc
COEX_DISPATCHER_INC := -I$(WLAN_ROOT)/components/coex/dispatcher/inc
COEX_CORE_INC       := -I$(WLAN_ROOT)/components/coex/core/inc
COEX_STRUCT_INC     := -I$(WLAN_COMMON_INC)/coex/dispatcher/inc

ifeq ($(CONFIG_FEATURE_COEX), y)
COEX_OBJS := $(COEX_TGT_SRC)/target_if_coex.o                 \
		 $(COEX_CORE_SRC)/wlan_coex_main.o                 \
		 $(COEX_OS_IF_SRC)/wlan_cfg80211_coex.o           \
		 $(COEX_DISPATCHER_SRC)/wlan_coex_tgt_api.o       \
		 $(COEX_DISPATCHER_SRC)/wlan_coex_utils_api.o       \
		 $(COEX_DISPATCHER_SRC)/wlan_coex_ucfg_api.o
endif

$(call add-wlan-objs,coex,$(COEX_OBJS))

###### COAP ########
ifeq ($(CONFIG_WLAN_FEATURE_COAP), y)
COAP_HDD_SRC := core/hdd/src
COAP_OS_IF_SRC := os_if/coap/src
COAP_TGT_SRC := components/target_if/coap/src
COAP_CORE_SRC  := components/coap/core/src
COAP_DISPATCHER_SRC := components/coap/dispatcher/src
COAP_WMI_SRC := components/wmi/src

COAP_OS_IF_INC  := -I$(WLAN_ROOT)/os_if/coap/inc
COAP_TGT_INC := -I$(WLAN_ROOT)/components/target_if/coap/inc
COAP_DISPATCHER_INC := -I$(WLAN_ROOT)/components/coap/dispatcher/inc
COAP_CORE_INC := -I$(WLAN_ROOT)/components/coap/core/inc
COAP_WMI_INC := -I$(WLAN_ROOT)/components/wmi/inc

COAP_OBJS := \
	$(COAP_HDD_SRC)/wlan_hdd_coap.o \
	$(COAP_OS_IF_SRC)/wlan_cfg80211_coap.o \
	$(COAP_TGT_SRC)/target_if_coap.o  \
	$(COAP_CORE_SRC)/wlan_coap_main.o  \
	$(COAP_DISPATCHER_SRC)/wlan_coap_tgt_api.o \
	$(COAP_DISPATCHER_SRC)/wlan_coap_ucfg_api.o \
	$(COAP_WMI_SRC)/wmi_unified_coap_tlv.o
$(call add-wlan-objs,coap,$(COAP_OBJS))
endif

############## HTC ##########
HTC_DIR := htc
HTC_INC := -I$(WLAN_COMMON_INC)/$(HTC_DIR)

HTC_OBJS := $(WLAN_COMMON_ROOT)/$(HTC_DIR)/htc.o \
            $(WLAN_COMMON_ROOT)/$(HTC_DIR)/htc_send.o \
            $(WLAN_COMMON_ROOT)/$(HTC_DIR)/htc_recv.o \
            $(WLAN_COMMON_ROOT)/$(HTC_DIR)/htc_services.o

ifeq ($(CONFIG_FEATURE_HTC_CREDIT_HISTORY), y)
HTC_OBJS += $(WLAN_COMMON_ROOT)/$(HTC_DIR)/htc_credit_history.o
endif

ifeq ($(CONFIG_WLAN_HANG_EVENT), y)
HTC_OBJS += $(WLAN_COMMON_ROOT)/$(HTC_DIR)/htc_hang_event.o
endif

$(call add-wlan-objs,htc,$(HTC_OBJS))

########### HIF ###########
HIF_DIR := hif
HIF_CE_DIR := $(HIF_DIR)/src/ce

HIF_DISPATCHER_DIR := $(HIF_DIR)/src/dispatcher

HIF_PCIE_DIR := $(HIF_DIR)/src/pcie
HIF_IPCIE_DIR := $(HIF_DIR)/src/ipcie
HIF_SNOC_DIR := $(HIF_DIR)/src/snoc
HIF_USB_DIR := $(HIF_DIR)/src/usb
HIF_SDIO_DIR := $(HIF_DIR)/src/sdio

HIF_SDIO_NATIVE_DIR := $(HIF_SDIO_DIR)/native_sdio
HIF_SDIO_NATIVE_INC_DIR := $(HIF_SDIO_NATIVE_DIR)/include
HIF_SDIO_NATIVE_SRC_DIR := $(HIF_SDIO_NATIVE_DIR)/src

HIF_INC := -I$(WLAN_COMMON_INC)/$(HIF_DIR)/inc \
	   -I$(WLAN_COMMON_INC)/$(HIF_DIR)/src

ifeq ($(CONFIG_HIF_PCI), y)
HIF_INC += -I$(WLAN_COMMON_INC)/$(HIF_DISPATCHER_DIR)
HIF_INC += -I$(WLAN_COMMON_INC)/$(HIF_PCIE_DIR)
HIF_INC += -I$(WLAN_COMMON_INC)/$(HIF_CE_DIR)
endif

ifeq ($(CONFIG_HIF_IPCI), y)
HIF_INC += -I$(WLAN_COMMON_INC)/$(HIF_DISPATCHER_DIR)
HIF_INC += -I$(WLAN_COMMON_INC)/$(HIF_IPCIE_DIR)
HIF_INC += -I$(WLAN_COMMON_INC)/$(HIF_CE_DIR)
endif

ifeq ($(CONFIG_HIF_SNOC), y)
HIF_INC += -I$(WLAN_COMMON_INC)/$(HIF_DISPATCHER_DIR)
HIF_INC += -I$(WLAN_COMMON_INC)/$(HIF_SNOC_DIR)
HIF_INC += -I$(WLAN_COMMON_INC)/$(HIF_CE_DIR)
endif

ifeq ($(CONFIG_HIF_USB), y)
HIF_INC += -I$(WLAN_COMMON_INC)/$(HIF_DISPATCHER_DIR)
HIF_INC += -I$(WLAN_COMMON_INC)/$(HIF_USB_DIR)
HIF_INC += -I$(WLAN_COMMON_INC)/$(HIF_CE_DIR)
endif

ifeq ($(CONFIG_HIF_SDIO), y)
HIF_INC += -I$(WLAN_COMMON_INC)/$(HIF_DISPATCHER_DIR)
HIF_INC += -I$(WLAN_COMMON_INC)/$(HIF_SDIO_DIR)
HIF_INC += -I$(WLAN_COMMON_INC)/$(HIF_SDIO_NATIVE_INC_DIR)
HIF_INC += -I$(WLAN_COMMON_INC)/$(HIF_CE_DIR)
endif

HIF_COMMON_OBJS := $(WLAN_COMMON_ROOT)/$(HIF_DIR)/src/ath_procfs.o \
		   $(WLAN_COMMON_ROOT)/$(HIF_DIR)/src/hif_main.o \
		   $(WLAN_COMMON_ROOT)/$(HIF_DIR)/src/hif_runtime_pm.o \
		   $(WLAN_COMMON_ROOT)/$(HIF_DIR)/src/hif_exec.o

ifneq (y,$(filter y,$(CONFIG_LITHIUM) $(CONFIG_BERYLLIUM)))
HIF_COMMON_OBJS += $(WLAN_COMMON_ROOT)/$(HIF_DIR)/src/hif_main_legacy.o
endif

ifeq ($(CONFIG_WLAN_NAPI), y)
HIF_COMMON_OBJS += $(WLAN_COMMON_ROOT)/$(HIF_DIR)/src/hif_irq_affinity.o
endif

HIF_CE_OBJS :=  $(WLAN_COMMON_ROOT)/$(HIF_CE_DIR)/ce_diag.o \
                $(WLAN_COMMON_ROOT)/$(HIF_CE_DIR)/ce_main.o \
                $(WLAN_COMMON_ROOT)/$(HIF_CE_DIR)/ce_service.o \
                $(WLAN_COMMON_ROOT)/$(HIF_CE_DIR)/ce_tasklet.o \
                $(WLAN_COMMON_ROOT)/$(HIF_DIR)/src/mp_dev.o \
                $(WLAN_COMMON_ROOT)/$(HIF_DIR)/src/regtable.o

ifeq ($(CONFIG_WLAN_FEATURE_BMI), y)
HIF_CE_OBJS +=  $(WLAN_COMMON_ROOT)/$(HIF_CE_DIR)/ce_bmi.o
endif

ifeq ($(CONFIG_LITHIUM), y)
ifeq ($(CONFIG_CNSS_QCA6290), y)
HIF_CE_OBJS +=  $(WLAN_COMMON_ROOT)/$(HIF_DIR)/src/qca6290def.o
endif

ifeq ($(CONFIG_CNSS_QCA6390), y)
HIF_CE_OBJS +=  $(WLAN_COMMON_ROOT)/$(HIF_DIR)/src/qca6390def.o
endif

ifeq ($(CONFIG_CNSS_QCA6490), y)
HIF_CE_OBJS +=  $(WLAN_COMMON_ROOT)/$(HIF_DIR)/src/qca6490def.o
endif

ifeq ($(CONFIG_CNSS_QCA6750), y)
HIF_CE_OBJS +=  $(WLAN_COMMON_ROOT)/$(HIF_DIR)/src/qca6750def.o
endif

HIF_CE_OBJS +=  $(WLAN_COMMON_ROOT)/$(HIF_CE_DIR)/ce_service_srng.o
else ifeq ($(CONFIG_BERYLLIUM), y)
ifeq (y,$(findstring y,$(CONFIG_CNSS_KIWI) $(CONFIG_CNSS_KIWI_V2) $CONFIG_CNSS_PEACH))
HIF_CE_OBJS +=  $(WLAN_COMMON_ROOT)/$(HIF_DIR)/src/kiwidef.o
endif

ifeq ($(CONFIG_CNSS_WCN7750), y)
HIF_CE_OBJS +=  $(WLAN_COMMON_ROOT)/$(HIF_DIR)/src/wcn7750def.o
endif

HIF_CE_OBJS +=  $(WLAN_COMMON_ROOT)/$(HIF_CE_DIR)/ce_service_srng.o
else
HIF_CE_OBJS +=  $(WLAN_COMMON_ROOT)/$(HIF_CE_DIR)/ce_service_legacy.o
endif

HIF_USB_OBJS := $(WLAN_COMMON_ROOT)/$(HIF_USB_DIR)/usbdrv.o \
                $(WLAN_COMMON_ROOT)/$(HIF_USB_DIR)/hif_usb.o \
                $(WLAN_COMMON_ROOT)/$(HIF_USB_DIR)/if_usb.o \
                $(WLAN_COMMON_ROOT)/$(HIF_USB_DIR)/regtable_usb.o

HIF_SDIO_OBJS := $(WLAN_COMMON_ROOT)/$(HIF_SDIO_DIR)/hif_diag_reg_access.o \
                 $(WLAN_COMMON_ROOT)/$(HIF_SDIO_DIR)/hif_sdio_dev.o \
                 $(WLAN_COMMON_ROOT)/$(HIF_SDIO_DIR)/hif_sdio.o \
                 $(WLAN_COMMON_ROOT)/$(HIF_SDIO_DIR)/regtable_sdio.o \
                 $(WLAN_COMMON_ROOT)/$(HIF_SDIO_DIR)/transfer/transfer.o

ifeq ($(CONFIG_WLAN_FEATURE_BMI), y)
HIF_SDIO_OBJS += $(WLAN_COMMON_ROOT)/$(HIF_SDIO_DIR)/hif_bmi_reg_access.o
endif

ifeq ($(CONFIG_SDIO_TRANSFER), adma)
HIF_SDIO_OBJS += $(WLAN_COMMON_ROOT)/$(HIF_SDIO_DIR)/transfer/adma.o
else
HIF_SDIO_OBJS += $(WLAN_COMMON_ROOT)/$(HIF_SDIO_DIR)/transfer/mailbox.o
endif

HIF_SDIO_NATIVE_OBJS := $(WLAN_COMMON_ROOT)/$(HIF_SDIO_NATIVE_SRC_DIR)/hif.o \
                        $(WLAN_COMMON_ROOT)/$(HIF_SDIO_NATIVE_SRC_DIR)/hif_scatter.o \
                        $(WLAN_COMMON_ROOT)/$(HIF_SDIO_NATIVE_SRC_DIR)/dev_quirks.o

ifeq ($(CONFIG_WLAN_NAPI), y)
HIF_OBJS += $(WLAN_COMMON_ROOT)/$(HIF_DIR)/src/hif_napi.o
endif

ifeq ($(CONFIG_FEATURE_UNIT_TEST_SUSPEND), y)
	HIF_OBJS += $(WLAN_COMMON_ROOT)/$(HIF_DIR)/src/hif_unit_test_suspend.o
endif

HIF_PCIE_OBJS := $(WLAN_COMMON_ROOT)/$(HIF_PCIE_DIR)/if_pci.o
HIF_IPCIE_OBJS := $(WLAN_COMMON_ROOT)/$(HIF_IPCIE_DIR)/if_ipci.o
HIF_SNOC_OBJS := $(WLAN_COMMON_ROOT)/$(HIF_SNOC_DIR)/if_snoc.o
HIF_SDIO_OBJS += $(WLAN_COMMON_ROOT)/$(HIF_SDIO_DIR)/if_sdio.o

HIF_OBJS += $(WLAN_COMMON_ROOT)/$(HIF_DISPATCHER_DIR)/multibus.o
HIF_OBJS += $(WLAN_COMMON_ROOT)/$(HIF_DISPATCHER_DIR)/dummy.o
HIF_OBJS += $(HIF_COMMON_OBJS)

ifeq ($(CONFIG_HIF_PCI), y)
HIF_OBJS += $(HIF_PCIE_OBJS)
HIF_OBJS += $(HIF_CE_OBJS)
HIF_OBJS += $(WLAN_COMMON_ROOT)/$(HIF_DISPATCHER_DIR)/multibus_pci.o
endif

ifeq ($(CONFIG_HIF_IPCI), y)
HIF_OBJS += $(HIF_IPCIE_OBJS)
HIF_OBJS += $(HIF_CE_OBJS)
HIF_OBJS += $(WLAN_COMMON_ROOT)/$(HIF_DISPATCHER_DIR)/multibus_ipci.o
endif

ifeq ($(CONFIG_HIF_SNOC), y)
HIF_OBJS += $(HIF_SNOC_OBJS)
HIF_OBJS += $(HIF_CE_OBJS)
HIF_OBJS += $(WLAN_COMMON_ROOT)/$(HIF_DISPATCHER_DIR)/multibus_snoc.o
endif

ifeq ($(CONFIG_HIF_SDIO), y)
HIF_OBJS += $(HIF_SDIO_OBJS)
HIF_OBJS += $(HIF_SDIO_NATIVE_OBJS)
HIF_OBJS += $(WLAN_COMMON_ROOT)/$(HIF_DISPATCHER_DIR)/multibus_sdio.o
endif

ifeq ($(CONFIG_HIF_USB), y)
HIF_OBJS += $(HIF_USB_OBJS)
HIF_OBJS += $(WLAN_COMMON_ROOT)/$(HIF_DISPATCHER_DIR)/multibus_usb.o
endif

$(call add-wlan-objs,hif,$(HIF_OBJS))

############ HAL ############
ifeq (y,$(filter y,$(CONFIG_LITHIUM) $(CONFIG_BERYLLIUM) $(CONFIG_RHINE)))
HAL_DIR :=	hal
HAL_INC :=	-I$(WLAN_COMMON_INC)/$(HAL_DIR)/inc \
		-I$(WLAN_COMMON_INC)/$(HAL_DIR)/wifi3.0

#TODO fix hal_reo for RHINE
HAL_OBJS :=	$(WLAN_COMMON_ROOT)/$(HAL_DIR)/wifi3.0/hal_srng.o \
		$(WLAN_COMMON_ROOT)/$(HAL_DIR)/wifi3.0/hal_reo.o

ifeq ($(CONFIG_RX_FISA), y)
HAL_OBJS += $(WLAN_COMMON_ROOT)/$(HAL_DIR)/wifi3.0/hal_rx_flow.o
endif
endif #### CONFIG LITHIUM/BERYLLIUM/RHINE ####

ifeq ($(CONFIG_LITHIUM), y)
HAL_INC += 	-I$(WLAN_COMMON_INC)/$(HAL_DIR)/wifi3.0/li

HAL_OBJS +=	$(WLAN_COMMON_ROOT)/$(HAL_DIR)/wifi3.0/li/hal_li_generic_api.o

HAL_OBJS +=	$(WLAN_COMMON_ROOT)/$(HAL_DIR)/wifi3.0/li/hal_li_reo.o

ifeq ($(CONFIG_CNSS_QCA6290), y)
HAL_INC += -I$(WLAN_COMMON_INC)/$(HAL_DIR)/wifi3.0/qca6290
HAL_OBJS += $(WLAN_COMMON_ROOT)/$(HAL_DIR)/wifi3.0/qca6290/hal_6290.o
else ifeq ($(CONFIG_CNSS_QCA6390), y)
HAL_INC += -I$(WLAN_COMMON_INC)/$(HAL_DIR)/wifi3.0/qca6390
HAL_OBJS += $(WLAN_COMMON_ROOT)/$(HAL_DIR)/wifi3.0/qca6390/hal_6390.o
else ifeq ($(CONFIG_CNSS_QCA6490), y)
HAL_INC += -I$(WLAN_COMMON_INC)/$(HAL_DIR)/wifi3.0/qca6490
HAL_OBJS += $(WLAN_COMMON_ROOT)/$(HAL_DIR)/wifi3.0/qca6490/hal_6490.o
else ifeq ($(CONFIG_CNSS_QCA6750), y)
HAL_INC += -I$(WLAN_COMMON_INC)/$(HAL_DIR)/wifi3.0/qca6750
HAL_OBJS += $(WLAN_COMMON_ROOT)/$(HAL_DIR)/wifi3.0/qca6750/hal_6750.o
else
#error "Not 11ax"
endif

endif #####CONFIG_LITHIUM####

ifeq ($(CONFIG_BERYLLIUM), y)
HAL_INC += 	-I$(WLAN_COMMON_INC)/$(HAL_DIR)/wifi3.0/be

HAL_OBJS +=	$(WLAN_COMMON_ROOT)/$(HAL_DIR)/wifi3.0/be/hal_be_generic_api.o

HAL_OBJS +=	$(WLAN_COMMON_ROOT)/$(HAL_DIR)/wifi3.0/be/hal_be_reo.o \

ifeq (y,$(findstring y,$(CONFIG_INCLUDE_HAL_PEACH)))
HAL_INC += -I$(WLAN_COMMON_INC)/$(HAL_DIR)/wifi3.0/peach
HAL_OBJS += $(WLAN_COMMON_ROOT)/$(HAL_DIR)/wifi3.0/peach/hal_peach.o
ccflags-y += -DINCLUDE_HAL_PEACH
else ifeq (y,$(findstring y,$(CONFIG_INCLUDE_HAL_KIWI)))
HAL_INC += -I$(WLAN_COMMON_INC)/$(HAL_DIR)/wifi3.0/kiwi
HAL_OBJS += $(WLAN_COMMON_ROOT)/$(HAL_DIR)/wifi3.0/kiwi/hal_kiwi.o
ccflags-y += -DINCLUDE_HAL_KIWI
else ifeq (y,$(findstring y,$(CONFIG_QCA_WIFI_WCN7750)))
HAL_INC += -I$(WLAN_COMMON_INC)/$(HAL_DIR)/wifi3.0/wcn7750
HAL_OBJS += $(WLAN_COMMON_ROOT)/$(HAL_DIR)/wifi3.0/wcn7750/hal_wcn7750.o
endif

endif #### CONFIG_BERYLLIUM ####

ifeq ($(CONFIG_RHINE), y)
HAL_INC += 	-I$(WLAN_COMMON_INC)/$(HAL_DIR)/wifi3.0/rh

HAL_OBJS +=	$(WLAN_COMMON_ROOT)/$(HAL_DIR)/wifi3.0/rh/hal_rh_generic_api.o

ifeq ($(CONFIG_CNSS_WCN6450), y)
#TODO fix this for RHINE
HAL_INC += -I$(WLAN_COMMON_INC)/$(HAL_DIR)/wifi3.0/wcn6450
HAL_OBJS += $(WLAN_COMMON_ROOT)/$(HAL_DIR)/wifi3.0/wcn6450/hal_wcn6450.o
else
#error "Not RHINE"
endif

endif #####CONFIG_RHINE####

$(call add-wlan-objs,hal,$(HAL_OBJS))

############ WMA ############
WMA_DIR :=	core/wma

WMA_INC_DIR :=  $(WMA_DIR)/inc
WMA_SRC_DIR :=  $(WMA_DIR)/src

WMA_INC :=	-I$(WLAN_ROOT)/$(WMA_INC_DIR) \
		-I$(WLAN_ROOT)/$(WMA_SRC_DIR)

ifeq ($(CONFIG_QCACLD_FEATURE_NAN), y)
WMA_NDP_OBJS += $(WMA_SRC_DIR)/wma_nan_datapath.o
endif

WMA_OBJS :=	$(WMA_SRC_DIR)/wma_main.o \
		$(WMA_SRC_DIR)/wma_scan_roam.o \
		$(WMA_SRC_DIR)/wma_dev_if.o \
		$(WMA_SRC_DIR)/wma_mgmt.o \
		$(WMA_SRC_DIR)/wma_power.o \
		$(WMA_SRC_DIR)/wma_data.o \
		$(WMA_SRC_DIR)/wma_utils.o \
		$(WMA_SRC_DIR)/wma_features.o \
		$(WMA_SRC_DIR)/wlan_qct_wma_legacy.o\
		$(WMA_NDP_OBJS)

ifeq ($(CONFIG_WLAN_FEATURE_11BE), y)
WMA_OBJS +=	$(WMA_SRC_DIR)/wma_eht.o
endif
ifeq ($(CONFIG_WLAN_FEATURE_DSRC), y)
WMA_OBJS+=	$(WMA_SRC_DIR)/wma_ocb.o
endif
ifeq ($(CONFIG_WLAN_FEATURE_FIPS), y)
WMA_OBJS+=	$(WMA_SRC_DIR)/wma_fips_api.o
endif
ifeq ($(CONFIG_WLAN_FEATURE_11AX), y)
WMA_OBJS+=	$(WMA_SRC_DIR)/wma_he.o
endif
ifeq ($(CONFIG_WLAN_FEATURE_TWT), y)
WMA_OBJS +=	$(WMA_SRC_DIR)/wma_twt.o
endif
ifeq ($(CONFIG_QCACLD_FEATURE_FW_STATE), y)
WMA_OBJS +=	$(WMA_SRC_DIR)/wma_fw_state.o
endif
ifeq ($(CONFIG_WLAN_MWS_INFO_DEBUGFS), y)
WMA_OBJS +=	$(WMA_SRC_DIR)/wma_coex.o
endif

WMA_OBJS +=	$(WMA_SRC_DIR)/wma_pasn_peer_api.o

$(call add-wlan-objs,wma,$(WMA_OBJS))

#######DIRECT_BUFFER_RX#########
ifeq ($(CONFIG_DIRECT_BUF_RX_ENABLE), y)
DBR_DIR = $(WLAN_COMMON_ROOT)/target_if/direct_buf_rx
UMAC_DBR_INC := -I$(WLAN_COMMON_INC)/target_if/direct_buf_tx/inc
UMAC_DBR_OBJS := $(DBR_DIR)/src/target_if_direct_buf_rx_api.o \
		 $(DBR_DIR)/src/target_if_direct_buf_rx_main.o \
		 $(WLAN_COMMON_ROOT)/wmi/src/wmi_unified_dbr_api.o \
		 $(WLAN_COMMON_ROOT)/wmi/src/wmi_unified_dbr_tlv.o
endif

$(call add-wlan-objs,umac_dbr,$(UMAC_DBR_OBJS))

############## PLD ##########
PLD_DIR := core/pld
PLD_INC_DIR := $(PLD_DIR)/inc
PLD_SRC_DIR := $(PLD_DIR)/src

PLD_INC :=	-I$(WLAN_ROOT)/$(PLD_INC_DIR) \
		-I$(WLAN_ROOT)/$(PLD_SRC_DIR)

PLD_OBJS :=	$(PLD_SRC_DIR)/pld_common.o

ifeq ($(CONFIG_IPCIE_FW_SIM), y)
PLD_OBJS +=     $(PLD_SRC_DIR)/pld_pcie_fw_sim.o
endif
ifeq ($(CONFIG_PCIE_FW_SIM), y)
PLD_OBJS +=     $(PLD_SRC_DIR)/pld_pcie_fw_sim.o
else ifeq ($(CONFIG_HIF_PCI), y)
PLD_OBJS +=     $(PLD_SRC_DIR)/pld_pcie.o
endif
ifeq ($(CONFIG_SNOC_FW_SIM),y)
PLD_OBJS +=     $(PLD_SRC_DIR)/pld_snoc_fw_sim.o
else ifeq (y,$(findstring y, $(CONFIG_ICNSS) $(CONFIG_PLD_SNOC_ICNSS_FLAG)))
PLD_OBJS +=     $(PLD_SRC_DIR)/pld_snoc.o
else ifeq (y,$(findstring y, $(CONFIG_PLD_IPCI_ICNSS_FLAG)))
PLD_OBJS +=     $(PLD_SRC_DIR)/pld_ipci.o
endif

ifeq ($(CONFIG_QCA_WIFI_SDIO), y)
PLD_OBJS +=	$(PLD_SRC_DIR)/pld_sdio.o
endif
ifeq ($(CONFIG_HIF_USB), y)
PLD_OBJS +=	$(PLD_SRC_DIR)/pld_usb.o
endif

$(call add-wlan-objs,pld,$(PLD_OBJS))


TARGET_INC := 	-I$(WLAN_FW_API)/fw

ifeq ($(CONFIG_CNSS_QCA6290), y)
ifeq ($(CONFIG_QCA6290_11AX), y)
TARGET_INC +=	-I$(WLAN_FW_API)/hw/qca6290/11ax/v2
else
TARGET_INC +=	-I$(WLAN_FW_API)/hw/qca6290/v2
endif
endif

ifeq ($(CONFIG_CNSS_QCA6390), y)
TARGET_INC +=	-I$(WLAN_FW_API)/hw/qca6390/v1
endif

ifeq ($(CONFIG_CNSS_QCA6490), y)
TARGET_INC +=	-I$(WLAN_FW_API)/hw/qca6490/v1
endif

ifeq ($(CONFIG_CNSS_QCA6750), y)
TARGET_INC +=	-I$(WLAN_FW_API)/hw/qca6750/v1
endif

ifeq ($(CONFIG_CNSS_WCN6450), y)
TARGET_INC +=	-I$(WLAN_FW_API)/hw/wcn6450/v1
endif

ifeq ($(CONFIG_CNSS_PEACH), y)
TARGET_INC +=	-I$(WLAN_FW_API)/hw/peach/v1/
else
ifeq ($(CONFIG_CNSS_KIWI_V2), y)
TARGET_INC +=	-I$(WLAN_FW_API)/hw/kiwi/v2/
else
ifeq ($(CONFIG_CNSS_KIWI), y)
TARGET_INC +=	-I$(WLAN_FW_API)/hw/kiwi/v1/
endif

ifeq ($(CONFIG_CNSS_WCN7750), y)
TARGET_INC +=   -I$(WLAN_FW_API)/hw/wcn7750/v1/
endif

endif
endif

LINUX_INC :=	-Iinclude

INCS :=		$(HDD_INC) \
		$(SYNC_INC) \
		$(DSC_INC) \
		$(EPPING_INC) \
		$(LINUX_INC) \
		$(MAC_INC) \
		$(SAP_INC) \
		$(SME_INC) \
		$(SYS_INC) \
		$(CLD_WMI_INC) \
		$(QAL_INC) \
		$(QDF_INC) \
		$(WBUFF_INC) \
		$(CDS_INC) \
		$(CFG_INC) \
		$(DFS_INC) \
		$(TARGET_IF_INC) \
		$(CLD_TARGET_IF_INC) \
		$(OS_IF_INC) \
		$(GLOBAL_LMAC_IF_INC) \
		$(FTM_INC)

INCS +=		$(WMA_INC) \
		$(UAPI_INC) \
		$(COMMON_INC) \
		$(WMI_INC) \
		$(FWLOG_INC) \
		$(TXRX_INC) \
		$(OL_INC) \
		$(CDP_INC) \
		$(PKTLOG_INC) \
		$(HTT_INC) \
		$(INIT_DEINIT_INC) \
		$(SCHEDULER_INC) \
		$(REGULATORY_INC) \
		$(HTC_INC) \
		$(WCFG_INC)

INCS +=		$(HIF_INC) \
		$(BMI_INC) \
		$(CMN_SYS_INC)

ifeq (y,$(filter y,$(CONFIG_LITHIUM) $(CONFIG_BERYLLIUM) $(CONFIG_RHINE)))
INCS += 	$(HAL_INC) \
		$(DP_INC)
endif

################ WIFI POS ################
INCS +=		$(WIFI_POS_CLD_INC)
INCS +=		$(WIFI_POS_API_INC)
INCS +=		$(WIFI_POS_TGT_INC)
INCS +=		$(WIFI_POS_OS_IF_INC)
################ CP STATS ################
INCS +=		$(CP_STATS_OS_IF_INC)
INCS +=		$(CP_STATS_TGT_INC)
INCS +=		$(CP_STATS_DISPATCHER_INC)
INCS +=		$(CP_MC_STATS_COMPONENT_INC)
INCS +=		$(CP_STATS_CFG80211_OS_IF_INC)
################ TWT CONVERGED ################
INCS +=		$(TWT_CONV_INCS)
################ Dynamic ACS ####################
INCS +=		$(DCS_TGT_IF_INC)
INCS +=		$(DCS_DISP_INC)
################ AFC #################
INCS +=		$(AFC_CMN_OSIF_INC)
INCS +=		$(AFC_CMN_DISP_INC)
INCS +=		$(AFC_CMN_CORE_INC)
################ INTEROP ISSUES AP ################
INCS +=		$(INTEROP_ISSUES_AP_OS_IF_INC)
INCS +=		$(INTEROP_ISSUES_AP_TGT_INC)
INCS +=		$(INTEROP_ISSUES_AP_DISPATCHER_INC)
INCS +=		$(INTEROP_ISSUES_AP_CORE_INC)
################ NAN POS ################
INCS +=		$(NAN_CORE_INC)
INCS +=		$(NAN_UCFG_INC)
INCS +=		$(NAN_TGT_INC)
INCS +=		$(NAN_OS_IF_INC)
###########DP_COMPONENT ####################
INCS +=		$(DP_COMP_INC)
###########QMI_COMPONENT ####################
INCS +=		$(QMI_COMP_INC)
################ SON ################
INCS +=		$(SON_CORE_INC)
INCS +=		$(SON_UCFG_INC)
INCS +=		$(SON_TGT_INC)
INCS +=		$(SON_OS_IF_INC)
################ SPATIAL_REUSE ################
INCS +=		$(SR_UCFG_INC)
INCS +=		$(SR_TGT_INC)
##########################################

INCS +=		$(UMAC_OBJMGR_INC)
INCS +=		$(UMAC_MGMT_TXRX_INC)
INCS +=		$(PMO_INC)
INCS +=		$(P2P_INC)
INCS +=		$(POLICY_MGR_INC)
INCS +=		$(TARGET_INC)
INCS +=		$(TDLS_INC)
INCS +=		$(UMAC_SER_INC)
INCS +=		$(NLINK_INC) \
		$(PTT_INC) \
		$(WLAN_LOGGING_INC)

INCS +=		$(PLD_INC)
INCS +=		$(OCB_INC)

INCS +=		$(IPA_INC)
INCS +=		$(UMAC_SM_INC)
INCS +=		$(UMAC_MLME_INC)
INCS +=		$(MLME_INC)
INCS +=		$(FWOL_INC)
INCS +=		$(DLM_INC)
INCS +=		$(CONN_LOGGING_INC)

ifeq ($(CONFIG_REMOVE_PKT_LOG), n)
INCS +=		$(PKTLOG_INC)
endif

INCS +=		$(HOST_DIAG_LOG_INC)

INCS +=		$(DISA_INC)
INCS +=		$(ACTION_OUI_INC)
INCS +=		$(PKT_CAPTURE_INC)
INCS +=		$(FTM_TIME_SYNC_INC)
INCS +=		$(WLAN_PRE_CAC_INC)

INCS +=		$(UMAC_DISP_INC)
INCS +=		$(UMAC_SCAN_INC)
INCS +=		$(UMAC_TARGET_SCAN_INC)
INCS +=		$(UMAC_GREEN_AP_INC)
INCS +=		$(UMAC_TARGET_GREEN_AP_INC)
INCS +=		$(UMAC_COMMON_INC)
INCS +=		$(UMAC_SPECTRAL_INC)
INCS +=		$(WLAN_CFR_INC)
INCS +=		$(UMAC_TARGET_SPECTRAL_INC)
INCS +=		$(UMAC_GPIO_INC)
INCS +=		$(UMAC_TARGET_GPIO_INC)
INCS +=		$(UMAC_DBR_INC)
INCS +=		$(UMAC_CRYPTO_INC)
INCS +=		$(UMAC_INTERFACE_MGR_INC)
INCS +=		$(UMAC_MLO_MGR_INC)
INCS +=		$(UMAC_MLO_MGR_CLD_INC)
INCS +=		$(COEX_OS_IF_INC)
INCS +=		$(COEX_TGT_INC)
INCS +=		$(COEX_DISPATCHER_INC)
INCS +=		$(COEX_CORE_INC)
INCS +=		$(COEX_STRUCT_INC)
################ COAP ################
INCS +=		$(COAP_OS_IF_INC)
INCS +=		$(COAP_TGT_INC)
INCS +=		$(COAP_DISPATCHER_INC)
INCS +=		$(COAP_CORE_INC)
INCS +=		$(COAP_WMI_INC)

ccflags-y += $(INCS)

ccflags-y += -include $(WLAN_ROOT)/configs/default_config.h

# CFG80211_MLO_KEY_OPERATION_SUPPORT
# Used to indicate the Linux Kernel contains support for ML key operation
# support.
#
# This feature was backported to Android Common Kernel 5.15 via:
# https://android-review.googlesource.com/c/kernel/common/+/2173923
found = $(shell if grep -qF "nl80211_validate_key_link_id" $(srctree)/net/wireless/nl80211.c; then echo "yes" ;else echo "no" ;fi;)
ifeq ($(findstring yes, $(found)), yes)
ccflags-y += -DCFG80211_MLO_KEY_OPERATION_SUPPORT
endif

found = $(shell if grep -qF "struct link_station_parameters" $(srctree)/include/net/cfg80211.h; then echo "yes"; else echo "no"; fi;)
ifeq ($(findstring yes, $(found)), yes)
ccflags-y += -DCFG80211_LINK_STA_PARAMS_PRESENT
endif

found = $(shell if grep -qF "NL80211_EXT_FEATURE_PUNCT" $(srctree)/include/uapi/linux/nl80211.h; then echo "yes" ;else echo "no" ;fi;)
ifeq ($(findstring yes, $(found)), yes)
ccflags-y += -DNL80211_EXT_FEATURE_PUNCT_SUPPORT
endif

found = $(shell if grep -qF "unsigned int link_id, u16 punct_bitmap" $(srctree)/include/net/cfg80211.h; then echo "yes" ;else echo "no" ;fi;)
ifeq ($(findstring yes, $(found)), yes)
ccflags-y += -DCFG80211_RU_PUNCT_NOTIFY
endif

found = $(shell if grep -qF "NL80211_EXT_FEATURE_AUTH_AND_DEAUTH_RANDOM_TA" $(srctree)/include/uapi/linux/nl80211.h; then echo "yes"; else echo "no"; fi;)
ifeq ($(findstring yes, $(found)), yes)
ccflags-y += -DCFG80211_EXT_FEATURE_AUTH_AND_DEAUTH_RANDOM_TA
endif

# CFG80211_EXTERNAL_AUTH_MLO_SUPPORT
# Used to indicate Linux kernel contains support for ML external auth support.
#
# This feature was backported to Android Common Kernel 5.15 via:
# https://android-review.googlesource.com/c/kernel/common/+/2450264
found = $(shell if grep -qF "MLD address of the peer. Used by the authentication request event" $(srctree)/include/net/cfg80211.h; then echo "yes" ;else echo "no" ;fi;)
ifeq ($(findstring yes, $(found)), yes)
ccflags-y += -DCFG80211_EXTERNAL_AUTH_MLO_SUPPORT
endif

found = $(shell if grep -qF "NL80211_CMD_SET_TID_TO_LINK_MAPPING" $(srctree)/include/uapi/linux/nl80211.h; then echo "yes" ;else echo "no" ;fi;)
ifeq ($(findstring yes, $(found)), yes)
ccflags-y += -DWLAN_FEATURE_11BE_MLO_TTLM
endif

found = $(shell if grep -qF "NL80211_EXT_FEATURE_SECURE_NAN" $(srctree)/include/uapi/linux/nl80211.h; then echo "yes"; else echo "no"; fi;)
ifeq ($(findstring yes, $(found)), yes)
ccflags-y += -DCFG80211_EXT_FEATURE_SECURE_NAN
endif

found = $(shell if grep -qF "bool mlo_params_valid;" $(srctree)/include/net/cfg80211.h; then echo "yes" ;else echo "no" ;fi;)
ifeq ($(findstring yes, $(found)), yes)
ccflags-y += -DCFG80211_MLD_AP_STA_CONNECT_UPSTREAM_SUPPORT
endif

found = $(shell if grep -qF "CFG80211_MULTI_LINK_SAP" $(srctree)/include/net/cfg80211.h; then echo "yes" ;else echo "no" ;fi;)
ifeq ($(findstring yes, $(found)), yes)
ifeq ($(CONFIG_WLAN_FEATURE_MULTI_LINK_SAP_TEST_CONFIG), y)
CONFIG_WLAN_FEATURE_MULTI_LINK_SAP := y
endif
endif

ifeq ($(CONFIG_WLAN_FEATURE_MULTI_LINK_SAP), y)
CONFIG_WLAN_DP_MLO_DEV_CTX := y
CONFIG_QCA_DP_TX_FW_METADATA_V2 := y
CONFIG_WLAN_DP_TXPOOL_SHARE := y
CONFIG_WLAN_MCAST_MLO_SAP := y
endif

ifeq (qca_cld3, $(WLAN_WEAR_CHIPSET))
	ccflags-y += -DWLAN_WEAR_CHIPSET
endif

ccflags-$(CONFIG_ONE_MSI_VECTOR) += -DWLAN_ONE_MSI_VECTOR

ccflags-$(CONFIG_DSC_DEBUG) += -DWLAN_DSC_DEBUG
ccflags-$(CONFIG_DSC_TEST) += -DWLAN_DSC_TEST

ifeq ($(CONFIG_LITHIUM), y)
ccflags-y += -DCONFIG_LITHIUM
endif

ifeq ($(CONFIG_BERYLLIUM), y)
ccflags-y += -DCONFIG_BERYLLIUM
ccflags-y += -DDP_OFFLOAD_FRAME_WITH_SW_EXCEPTION
endif

ifeq ($(CONFIG_RHINE), y)
ccflags-y += -DCONFIG_RHINE
ccflags-y += -DDP_OFFLOAD_FRAME_WITH_SW_EXCEPTION
endif

ccflags-$(CONFIG_TALLOC_DEBUG) += -DWLAN_TALLOC_DEBUG
ccflags-$(CONFIG_QDF_TEST) += -DWLAN_DELAYED_WORK_TEST
ccflags-$(CONFIG_QDF_TEST) += -DWLAN_HASHTABLE_TEST
ccflags-$(CONFIG_QDF_TEST) += -DWLAN_PERIODIC_WORK_TEST
ccflags-$(CONFIG_QDF_TEST) += -DWLAN_PTR_HASH_TEST
ccflags-$(CONFIG_QDF_TEST) += -DWLAN_SLIST_TEST
ccflags-$(CONFIG_QDF_TEST) += -DWLAN_TALLOC_TEST
ccflags-$(CONFIG_QDF_TEST) += -DWLAN_TRACKER_TEST
ccflags-$(CONFIG_QDF_TEST) += -DWLAN_TYPES_TEST
ccflags-$(CONFIG_WLAN_HANG_EVENT) += -DWLAN_HANG_EVENT

#Flag to enable pre_cac
ccflags-$(CONFIG_FEATURE_WLAN_PRE_CAC)  += -DPRE_CAC_SUPPORT

ccflags-$(CONFIG_WIFI_POS_PASN) += -DWLAN_FEATURE_RTT_11AZ_SUPPORT

ifeq ($(CONFIG_DIRECT_BUF_RX_ENABLE), y)
ifeq ($(CONFIG_DBR_HOLD_LARGE_MEM), y)
ccflags-y += -DDBR_HOLD_LARGE_MEM
endif
endif

ccflags-$(CONFIG_QCA_DMA_PADDR_CHECK) += -DQCA_DMA_PADDR_CHECK
ccflags-$(CONFIG_PADDR_CHECK_ON_3RD_PARTY_PLATFORM) += -DQCA_PADDR_CHECK_ON_3RD_PARTY_PLATFORM
ccflags-$(CONFIG_DP_TRAFFIC_END_INDICATION) += -DDP_TRAFFIC_END_INDICATION
ccflags-$(CONFIG_THERMAL_STATS_SUPPORT) += -DTHERMAL_STATS_SUPPORT
ccflags-$(CONFIG_PTT_SOCK_SVC_ENABLE) += -DPTT_SOCK_SVC_ENABLE
ccflags-$(CONFIG_FEATURE_WLAN_WAPI) += -DFEATURE_WLAN_WAPI
ccflags-$(CONFIG_FEATURE_WLAN_WAPI) += -DATH_SUPPORT_WAPI
ccflags-$(CONFIG_SOFTAP_CHANNEL_RANGE) += -DSOFTAP_CHANNEL_RANGE
ccflags-$(CONFIG_FEATURE_WLAN_SCAN_PNO) += -DFEATURE_WLAN_SCAN_PNO
ccflags-$(CONFIG_WLAN_FEATURE_PACKET_FILTERING) += -DWLAN_FEATURE_PACKET_FILTERING
ccflags-$(CONFIG_DHCP_SERVER_OFFLOAD) += -DDHCP_SERVER_OFFLOAD
ccflags-$(CONFIG_WLAN_NS_OFFLOAD) += -DWLAN_NS_OFFLOAD
ccflags-$(CONFIG_QCA_TARGET_IF_MLME) += -DQCA_TARGET_IF_MLME
ccflags-$(CONFIG_WLAN_DYNAMIC_ARP_NS_OFFLOAD) += -DFEATURE_WLAN_DYNAMIC_ARP_NS_OFFLOAD
ccflags-$(CONFIG_WLAN_FEATURE_ICMP_OFFLOAD) += -DWLAN_FEATURE_ICMP_OFFLOAD
ccflags-$(CONFIG_FEATURE_WLAN_RA_FILTERING) += -DFEATURE_WLAN_RA_FILTERING
ccflags-$(CONFIG_FEATURE_WLAN_LPHB) += -DFEATURE_WLAN_LPHB
ccflags-$(CONFIG_QCA_SUPPORT_TX_THROTTLE) += -DQCA_SUPPORT_TX_THROTTLE
ccflags-$(CONFIG_WMI_INTERFACE_EVENT_LOGGING) += -DWMI_INTERFACE_EVENT_LOGGING
ccflags-$(CONFIG_WLAN_FEATURE_LINK_LAYER_STATS) += -DWLAN_FEATURE_LINK_LAYER_STATS
ccflags-$(CONFIG_FEATURE_CLUB_LL_STATS_AND_GET_STATION) += -DFEATURE_CLUB_LL_STATS_AND_GET_STATION
ccflags-$(CONFIG_WLAN_FEATURE_MIB_STATS) += -DWLAN_FEATURE_MIB_STATS
ccflags-$(CONFIG_FEATURE_WLAN_EXTSCAN) += -DFEATURE_WLAN_EXTSCAN
ccflags-$(CONFIG_160MHZ_SUPPORT) += -DCONFIG_160MHZ_SUPPORT
ccflags-$(CONFIG_REG_CLIENT) += -DCONFIG_REG_CLIENT
ccflags-$(CONFIG_WLAN_PMO_ENABLE) += -DWLAN_PMO_ENABLE
ccflags-$(CONFIG_CONVERGED_P2P_ENABLE) += -DCONVERGED_P2P_ENABLE
ccflags-$(CONFIG_WLAN_POLICY_MGR_ENABLE) += -DWLAN_POLICY_MGR_ENABLE
ccflags-$(CONFIG_FEATURE_DENYLIST_MGR) += -DFEATURE_DENYLIST_MGR
ccflags-$(CONFIG_WAPI_BIG_ENDIAN) += -DFEATURE_WAPI_BIG_ENDIAN
ccflags-$(CONFIG_SUPPORT_11AX) += -DSUPPORT_11AX
ccflags-$(CONFIG_HDD_INIT_WITH_RTNL_LOCK) += -DCONFIG_HDD_INIT_WITH_RTNL_LOCK
ccflags-$(CONFIG_WLAN_CONV_SPECTRAL_ENABLE) += -DWLAN_CONV_SPECTRAL_ENABLE
ccflags-$(CONFIG_WLAN_CFR_ENABLE) += -DWLAN_CFR_ENABLE
ccflags-$(CONFIG_WLAN_ENH_CFR_ENABLE) += -DWLAN_ENH_CFR_ENABLE
ccflags-$(CONFIG_WLAN_ENH_CFR_ENABLE) += -DWLAN_CFR_PM
ccflags-$(CONFIG_WLAN_CFR_ADRASTEA) += -DWLAN_CFR_ADRASTEA
ccflags-$(CONFIG_WLAN_CFR_DBR) += -DWLAN_CFR_DBR
ccflags-$(CONFIG_WLAN_CFR_ENABLE) += -DCFR_USE_FIXED_FOLDER
ccflags-$(CONFIG_WLAN_FEATURE_MEDIUM_ASSESS) += -DWLAN_FEATURE_MEDIUM_ASSESS
ccflags-$(CONFIG_FEATURE_RADAR_HISTORY) += -DFEATURE_RADAR_HISTORY
ccflags-$(CONFIG_DIRECT_BUF_RX_ENABLE) += -DDIRECT_BUF_RX_ENABLE
ccflags-$(CONFIG_WMI_DBR_SUPPORT) += -DWMI_DBR_SUPPORT
ifneq ($(CONFIG_CNSS_QCA6750), y)
ccflags-$(CONFIG_DIRECT_BUF_RX_ENABLE) += -DDBR_MULTI_SRNG_ENABLE
endif
ifneq ($(CONFIG_CNSS_WCN6450), y)
ccflags-$(CONFIG_DIRECT_BUF_RX_ENABLE) += -DDBR_MULTI_SRNG_ENABLE
endif
ccflags-$(CONFIG_WMI_CMD_STRINGS) += -DWMI_CMD_STRINGS
ccflags-$(CONFIG_WLAN_FEATURE_TWT) += -DWLAN_SUPPORT_TWT
ifeq ($(CONFIG_WLAN_FEATURE_11BE_MLO), y)
ifeq ($(CONFIG_DP_USE_REDUCED_PEER_ID_FIELD_WIDTH), y)
ccflags-y += -DDP_USE_REDUCED_PEER_ID_FIELD_WIDTH
endif
endif
ccflags-$(CONFIG_DP_MULTIPASS_SUPPORT) += -DQCA_MULTIPASS_SUPPORT
ccflags-$(CONFIG_DP_MULTIPASS_SUPPORT) += -DWLAN_REPEATER_NOT_SUPPORTED
ccflags-$(CONFIG_DP_MULTIPASS_SUPPORT) += -DQCA_SUPPORT_PEER_ISOLATION
ccflags-$(CONFIG_WLAN_DP_PROFILE_SUPPORT) += -DWLAN_DP_PROFILE_SUPPORT

ifdef CONFIG_WLAN_TWT_SAP_STA_COUNT
WLAN_TWT_SAP_STA_COUNT ?= 32
ccflags-y += -DWLAN_TWT_SAP_STA_COUNT=$(WLAN_TWT_SAP_STA_COUNT)
endif

ccflags-$(CONFIG_ENABLE_LOW_POWER_MODE) += -DCONFIG_ENABLE_LOW_POWER_MODE
ccflags-$(CONFIG_WLAN_TWT_SAP_PDEV_COUNT) += -DWLAN_TWT_AP_PDEV_COUNT_NUM_PHY
ccflags-$(CONFIG_WLAN_DISABLE_EXPORT_SYMBOL) += -DWLAN_DISABLE_EXPORT_SYMBOL
ccflags-$(CONFIG_WIFI_POS_CONVERGED) += -DWIFI_POS_CONVERGED
ccflags-$(CONFIG_WLAN_TWT_CONVERGED) += -DWLAN_TWT_CONV_SUPPORTED
ccflags-$(CONFIG_WIFI_POS_LEGACY) += -DFEATURE_OEM_DATA_SUPPORT
ccflags-$(CONFIG_FEATURE_HTC_CREDIT_HISTORY) += -DFEATURE_HTC_CREDIT_HISTORY
ccflags-$(CONFIG_WLAN_FEATURE_P2P_DEBUG) += -DWLAN_FEATURE_P2P_DEBUG
ccflags-$(CONFIG_WLAN_WEXT_SUPPORT_ENABLE) += -DWLAN_WEXT_SUPPORT_ENABLE
ccflags-$(CONFIG_WLAN_LOGGING_SOCK_SVC) += -DWLAN_LOGGING_SOCK_SVC_ENABLE
ccflags-$(CONFIG_WLAN_LOGGING_BUFFERS_DYNAMICALLY) += -DWLAN_LOGGING_BUFFERS_DYNAMICALLY
ccflags-$(CONFIG_WLAN_FEATURE_FILS) += -DWLAN_FEATURE_FILS_SK
ccflags-$(CONFIG_CP_STATS) += -DWLAN_SUPPORT_INFRA_CTRL_PATH_STATS
ccflags-$(CONFIG_CP_STATS) += -DQCA_SUPPORT_CP_STATS
ccflags-$(CONFIG_CP_STATS) += -DQCA_SUPPORT_MC_CP_STATS
ccflags-$(CONFIG_CP_STATS) += -DWLAN_SUPPORT_LEGACY_CP_STATS_HANDLERS
ccflags-$(CONFIG_DCS) += -DDCS_INTERFERENCE_DETECTION
ccflags-$(CONFIG_FEATURE_INTEROP_ISSUES_AP) += -DWLAN_FEATURE_INTEROP_ISSUES_AP
ccflags-$(CONFIG_FEATURE_MEMDUMP_ENABLE) += -DWLAN_FEATURE_MEMDUMP_ENABLE
ccflags-$(CONFIG_FEATURE_FW_LOG_PARSING) += -DFEATURE_FW_LOG_PARSING
ccflags-$(CONFIG_FEATURE_OEM_DATA) += -DFEATURE_OEM_DATA
ccflags-$(CONFIG_FEATURE_MOTION_DETECTION) += -DWLAN_FEATURE_MOTION_DETECTION
ccflags-$(CONFIG_WLAN_FW_OFFLOAD) += -DWLAN_FW_OFFLOAD
ccflags-$(CONFIG_WLAN_FEATURE_ELNA) += -DWLAN_FEATURE_ELNA
ccflags-$(CONFIG_FEATURE_COEX) += -DFEATURE_COEX
ccflags-$(CONFIG_HOST_WAKEUP_OVER_QMI) += -DHOST_WAKEUP_OVER_QMI
ccflags-$(CONFIG_DISABLE_STATUS_RING_TIMER_WAR) += -DWLAN_DISABLE_STATUS_RING_TIMER_WAR
ccflags-$(CONFIG_CE_DISABLE_SRNG_TIMER_IRQ) += -DWLAN_WAR_CE_DISABLE_SRNG_TIMER_IRQ

ccflags-$(CONFIG_PLD_IPCI_ICNSS_FLAG) += -DCONFIG_PLD_IPCI_ICNSS
ccflags-$(CONFIG_PLD_SDIO_CNSS_FLAG) += -DCONFIG_PLD_SDIO_CNSS
ccflags-$(CONFIG_WLAN_RESIDENT_DRIVER) += -DFEATURE_WLAN_RESIDENT_DRIVER
ccflags-$(CONFIG_FEATURE_GPIO_CFG) += -DWLAN_FEATURE_GPIO_CFG
ccflags-$(CONFIG_FEATURE_BUS_BANDWIDTH_MGR) += -DFEATURE_BUS_BANDWIDTH_MGR
ccflags-$(CONFIG_DP_BE_WAR) += -DDP_BE_WAR

ifeq ($(CONFIG_IPCIE_FW_SIM), y)
ccflags-y += -DCONFIG_PLD_IPCIE_FW_SIM
endif
ifeq ($(CONFIG_PLD_PCIE_CNSS_FLAG), y)
ifeq ($(CONFIG_PCIE_FW_SIM), y)
ccflags-y += -DCONFIG_PLD_PCIE_FW_SIM
else
ccflags-y += -DCONFIG_PLD_PCIE_CNSS
endif
endif

ccflags-$(CONFIG_PLD_PCIE_INIT_FLAG) += -DCONFIG_PLD_PCIE_INIT
ccflags-$(CONFIG_WLAN_FEATURE_DP_RX_THREADS) += -DFEATURE_WLAN_DP_RX_THREADS
ccflags-$(CONFIG_WLAN_DP_LOCAL_PKT_CAPTURE) += -DWLAN_FEATURE_LOCAL_PKT_CAPTURE
ccflags-$(CONFIG_WLAN_FEATURE_RX_SOFTIRQ_TIME_LIMIT) += -DWLAN_FEATURE_RX_SOFTIRQ_TIME_LIMIT
ccflags-$(CONFIG_FEATURE_HIF_LATENCY_PROFILE_ENABLE) += -DHIF_LATENCY_PROFILE_ENABLE
ccflags-$(CONFIG_FEATURE_HAL_DELAYED_REG_WRITE) += -DFEATURE_HAL_DELAYED_REG_WRITE
ccflags-$(CONFIG_FEATURE_HAL_RECORD_SUSPEND_WRITE) += -DFEATURE_HAL_RECORD_SUSPEND_WRITE
ccflags-$(CONFIG_QCA_OL_DP_SRNG_LOCK_LESS_ACCESS) += -DQCA_OL_DP_SRNG_LOCK_LESS_ACCESS
ccflags-$(CONFIG_SHADOW_WRITE_DELAY) += -DSHADOW_WRITE_DELAY
ccflags-$(CONFIG_WLAN_DP_LOAD_BALANCE_SUPPORT) += -DWLAN_DP_LOAD_BALANCE_SUPPORT
ccflags-$(CONFIG_WLAN_DP_FLOW_BALANCE_SUPPORT) += -DWLAN_DP_FLOW_BALANCE_SUPPORT

ccflags-$(CONFIG_PLD_USB_CNSS) += -DCONFIG_PLD_USB_CNSS
ccflags-$(CONFIG_PLD_SDIO_CNSS2) += -DCONFIG_PLD_SDIO_CNSS2
ccflags-$(CONFIG_WLAN_RECORD_RX_PADDR) += -DHIF_RECORD_RX_PADDR
ccflags-$(CONFIG_FEATURE_WLAN_TIME_SYNC_FTM) += -DFEATURE_WLAN_TIME_SYNC_FTM

ccflags-$(CONFIG_WLAN_FEATURE_LRO_CTX_IN_CB) += -DWLAN_FEATURE_LRO_CTX_IN_CB

#For both legacy and lithium chip's monitor mode config
ifeq ($(CONFIG_FEATURE_MONITOR_MODE_SUPPORT), y)
ccflags-y += -DFEATURE_MONITOR_MODE_SUPPORT
ccflags-$(CONFIG_DP_CON_MON_MSI_ENABLED) += -DDP_CON_MON_MSI_ENABLED
ccflags-$(CONFIG_WLAN_RX_MON_PARSE_CMN_USER_INFO) += -DWLAN_RX_MON_PARSE_CMN_USER_INFO
ccflags-$(CONFIG_DP_CON_MON_MSI_SKIP_SET) += -DDP_CON_MON_MSI_SKIP_SET
ccflags-$(CONFIG_QCA_WIFI_MONITOR_MODE_NO_MSDU_START_TLV_SUPPORT) += -DQCA_WIFI_MONITOR_MODE_NO_MSDU_START_TLV_SUPPORT
ccflags-$(CONFIG_FEATURE_ML_MONITOR_MODE_SUPPORT) += -DFEATURE_ML_MONITOR_MODE_SUPPORT
else
ccflags-y += -DDISABLE_MON_CONFIG
endif

ifeq ($(CONFIG_SMP), y)
ifneq (y,$(filter y,$(CONFIG_LITHIUM) $(CONFIG_BERYLLIUM) $(CONFIG_RHINE)))
ccflags-y += -DWLAN_DP_LEGACY_OL_RX_THREAD
endif
endif

#Enable NL80211 test mode
ccflags-$(CONFIG_NL80211_TESTMODE) += -DWLAN_NL80211_TESTMODE

# Flag to enable bus auto suspend
ifeq ($(CONFIG_BUS_AUTO_SUSPEND), y)
ccflags-y += -DFEATURE_RUNTIME_PM
endif

ifeq (y,$(findstring y, $(CONFIG_ICNSS) $(CONFIG_ICNSS_MODULE)))
ifeq ($(CONFIG_SNOC_FW_SIM), y)
ccflags-y += -DCONFIG_PLD_SNOC_FW_SIM
else
ccflags-y += -DCONFIG_PLD_SNOC_ICNSS
endif
endif

ccflags-$(CONFIG_PLD_SNOC_ICNSS_FLAG) += -DCONFIG_PLD_SNOC_ICNSS
ccflags-$(CONFIG_ICNSS2_HELIUM) += -DCONFIG_PLD_SNOC_ICNSS2

ccflags-$(CONFIG_WIFI_3_0_ADRASTEA) += -DQCA_WIFI_3_0_ADRASTEA
ccflags-$(CONFIG_WIFI_3_0_ADRASTEA) += -DQCA_WIFI_3_0
ccflags-$(CONFIG_ADRASTEA_SHADOW_REGISTERS) += -DADRASTEA_SHADOW_REGISTERS
ccflags-$(CONFIG_ADRASTEA_RRI_ON_DDR) += -DADRASTEA_RRI_ON_DDR

ifeq ($(CONFIG_QMI_SUPPORT), n)
ccflags-y += -DCONFIG_BYPASS_QMI
endif

ccflags-$(CONFIG_WLAN_FASTPATH) +=	-DWLAN_FEATURE_FASTPATH

ccflags-$(CONFIG_FEATURE_PKTLOG) +=     -DFEATURE_PKTLOG

ccflags-$(CONFIG_CONNECTIVITY_PKTLOG) += -DCONNECTIVITY_PKTLOG

ifeq ($(CONFIG_WLAN_NAPI), y)
ccflags-y += -DFEATURE_NAPI
ccflags-y += -DHIF_IRQ_AFFINITY
ifeq ($(CONFIG_WLAN_NAPI_DEBUG), y)
ccflags-y += -DFEATURE_NAPI_DEBUG
endif
endif

ifeq (y,$(findstring y,$(CONFIG_ARCH_MSM) $(CONFIG_ARCH_QCOM)))
ccflags-y += -DMSM_PLATFORM
endif

ccflags-$(CONFIG_CNSS_OUT_OF_TREE) += -DCONFIG_CNSS_OUT_OF_TREE
ccflags-$(CONFIG_CNSS_OUT_OF_TREE) += -I$(WLAN_PLATFORM_INC)
ccflags-$(CONFIG_IPA_OUT_OF_TREE) += -I$(DATA_IPA_INC)
ccflags-$(CONFIG_IPA_OUT_OF_TREE) += -I$(DATA_IPA_UAPI_INC)

ccflags-$(CONFIG_WLAN_FEATURE_DP_BUS_BANDWIDTH) += -DWLAN_FEATURE_DP_BUS_BANDWIDTH
ccflags-$(CONFIG_WLAN_FEATURE_PERIODIC_STA_STATS) += -DWLAN_FEATURE_PERIODIC_STA_STATS

ccflags-$(CONFIG_WLAN_TX_FLOW_CONTROL_V2) += -DQCA_LL_TX_FLOW_CONTROL_V2
ccflags-$(CONFIG_WLAN_TX_FLOW_CONTROL_V2) += -DQCA_LL_TX_FLOW_GLOBAL_MGMT_POOL
ccflags-$(CONFIG_WLAN_TX_FLOW_CONTROL_LEGACY) += -DQCA_LL_LEGACY_TX_FLOW_CONTROL
ccflags-$(CONFIG_WLAN_PDEV_TX_FLOW_CONTROL) += -DQCA_LL_PDEV_TX_FLOW_CONTROL

ifeq ($(CONFIG_WLAN_DEBUG_VERSION), y)
ccflags-y +=	-DWLAN_DEBUG
ifeq ($(CONFIG_TRACE_RECORD_FEATURE), y)
ccflags-y +=	-DTRACE_RECORD \
		-DLIM_TRACE_RECORD \
		-DSME_TRACE_RECORD \
		-DHDD_TRACE_RECORD
endif
endif
ccflags-$(CONFIG_UNIT_TEST) += -DWLAN_UNIT_TEST
ccflags-$(CONFIG_WLAN_DEBUG_CRASH_INJECT) += -DCONFIG_WLAN_DEBUG_CRASH_INJECT
ccflags-$(CONFIG_WLAN_SYSFS_FW_MODE_CFG) += -DCONFIG_WLAN_SYSFS_FW_MODE_CFG
ccflags-$(CONFIG_WLAN_REASSOC) += -DCONFIG_WLAN_REASSOC
ccflags-$(CONFIG_WLAN_SCAN_DISABLE) += -DCONFIG_WLAN_SCAN_DISABLE
ccflags-$(CONFIG_WLAN_WOW_ITO) += -DCONFIG_WLAN_WOW_ITO
ccflags-$(CONFIG_WLAN_WOWL_ADD_PTRN) += -DCONFIG_WLAN_WOWL_ADD_PTRN
ccflags-$(CONFIG_WLAN_WOWL_DEL_PTRN) += -DCONFIG_WLAN_WOWL_DEL_PTRN
ccflags-$(CONFIG_WLAN_SYSFS_TX_STBC) += -DCONFIG_WLAN_SYSFS_TX_STBC
ccflags-$(CONFIG_WLAN_GET_STATS) += -DCONFIG_WLAN_GET_STATS
ccflags-$(CONFIG_WLAN_SYSFS_WLAN_DBG) += -DCONFIG_WLAN_SYSFS_WLAN_DBG
ccflags-$(CONFIG_WLAN_TXRX_FW_ST_RST) += -DCONFIG_WLAN_TXRX_FW_ST_RST
ccflags-$(CONFIG_WLAN_GTX_BW_MASK) += -DCONFIG_WLAN_GTX_BW_MASK
ccflags-$(CONFIG_WLAN_SYSFS_SCAN_CFG) += -DCONFIG_WLAN_SYSFS_SCAN_CFG
ccflags-$(CONFIG_WLAN_SYSFS_MONITOR_MODE_CHANNEL) += -DCONFIG_WLAN_SYSFS_MONITOR_MODE_CHANNEL
ccflags-$(CONFIG_WLAN_SYSFS_RADAR) += -DCONFIG_WLAN_SYSFS_RADAR
ccflags-$(CONFIG_WLAN_SYSFS_RTS_CTS) += -DWLAN_SYSFS_RTS_CTS
ccflags-$(CONFIG_WLAN_TXRX_FW_STATS) += -DCONFIG_WLAN_TXRX_FW_STATS
ccflags-$(CONFIG_WLAN_TXRX_STATS) += -DCONFIG_WLAN_TXRX_STATS
ccflags-$(CONFIG_WLAN_SYSFS_DP_TRACE) += -DWLAN_SYSFS_DP_TRACE
ccflags-$(CONFIG_WLAN_SYSFS_STATS) += -DWLAN_SYSFS_STATS
ccflags-$(CONFIG_WLAN_SYSFS_TEMPERATURE) += -DCONFIG_WLAN_SYSFS_TEMPERATURE
ccflags-$(CONFIG_WLAN_THERMAL_CFG) += -DCONFIG_WLAN_THERMAL_CFG
ccflags-$(CONFIG_FEATURE_UNIT_TEST_SUSPEND) += -DWLAN_SUSPEND_RESUME_TEST
ccflags-$(CONFIG_FEATURE_WLM_STATS) += -DFEATURE_WLM_STATS
ccflags-$(CONFIG_WLAN_SYSFS_MEM_STATS) += -DCONFIG_WLAN_SYSFS_MEM_STATS
ccflags-$(CONFIG_WLAN_SYSFS_DCM) += -DWLAN_SYSFS_DCM
ccflags-$(CONFIG_WLAN_SYSFS_HE_BSS_COLOR) += -DWLAN_SYSFS_HE_BSS_COLOR
ccflags-$(CONFIG_WLAN_SYSFS_STA_INFO) += -DWLAN_SYSFS_STA_INFO
ccflags-$(CONFIG_WLAN_DL_MODES) += -DCONFIG_WLAN_DL_MODES
ccflags-$(CONFIG_WLAN_THERMAL_MULTI_CLIENT_SUPPORT) += -DFEATURE_WPSS_THERMAL_MITIGATION
ccflags-$(CONFIG_WLAN_DUMP_IN_PROGRESS) += -DCONFIG_WLAN_DUMP_IN_PROGRESS
ccflags-$(CONFIG_WLAN_BMISS) += -DCONFIG_WLAN_BMISS
ccflags-$(CONFIG_WLAN_SYSFS_DP_STATS) += -DWLAN_SYSFS_DP_STATS
ccflags-$(CONFIG_WLAN_FREQ_LIST) += -DCONFIG_WLAN_FREQ_LIST

ccflags-$(CONFIG_WIFI_MONITOR_SUPPORT) += -DWIFI_MONITOR_SUPPORT
ccflags-$(CONFIG_QCA_MONITOR_PKT_SUPPORT) += -DQCA_MONITOR_PKT_SUPPORT
ccflags-$(CONFIG_MONITOR_MODULARIZED_ENABLE) += -DMONITOR_MODULARIZED_ENABLE
ccflags-$(CONFIG_DP_PKT_ADD_TIMESTAMP) += -DCONFIG_DP_PKT_ADD_TIMESTAMP
ccflags-$(CONFIG_WLAN_PDEV_VDEV_SEND_MULTI_PARAM) += -DWLAN_PDEV_VDEV_SEND_MULTI_PARAM
ccflags-$(CONFIG_WLAN_SYSFS_LOG_BUFFER) += -DFEATURE_SYSFS_LOG_BUFFER
ccflags-$(CONFIG_ENABLE_VALLOC_REPLACE_MALLOC) += -DENABLE_VALLOC_REPLACE_MALLOC
ccflags-$(CONFIG_WLAN_SYSFS_DFSNOL) += -DCONFIG_WLAN_SYSFS_DFSNOL
ccflags-$(CONFIG_WLAN_SYSFS_WDS_MODE) += -DFEATURE_SYSFS_WDS_MODE
ccflags-$(CONFIG_WLAN_SYSFS_ROAM_TRIGGER_BITMAP) += -DFEATURE_SYSFS_ROAM_TRIGGER_BITMAP
cppflags-$(CONFIG_BCN_RATECODE_ENABLE) += -DWLAN_BCN_RATECODE_ENABLE
ccflags-$(CONFIG_WLAN_SYSFS_RF_TEST_MODE) += -DFEATURE_SYSFS_RF_TEST_MODE

ifeq ($(CONFIG_LEAK_DETECTION), y)
ccflags-y += \
	-DCONFIG_HALT_KMEMLEAK \
	-DCONFIG_LEAK_DETECTION \
	-DMEMORY_DEBUG \
	-DNBUF_MEMORY_DEBUG \
	-DNBUF_MAP_UNMAP_DEBUG \
	-DTIMER_MANAGER \
	-DWLAN_DELAYED_WORK_DEBUG \
	-DWLAN_WAKE_LOCK_DEBUG \
	-DWLAN_PERIODIC_WORK_DEBUG
endif

cppflags-$(CONFIG_ALLOC_CONTIGUOUS_MULTI_PAGE) += -DALLOC_CONTIGUOUS_MULTI_PAGE

ifeq ($(CONFIG_QCOM_VOWIFI_11R), y)
ccflags-y += -DKERNEL_SUPPORT_11R_CFG80211
ccflags-y += -DUSE_80211_WMMTSPEC_FOR_RIC
endif

ifeq ($(CONFIG_QCOM_ESE), y)
ccflags-y += -DFEATURE_WLAN_ESE
endif

#normally, TDLS negative behavior is not needed
ccflags-$(CONFIG_QCOM_TDLS) += -DFEATURE_WLAN_TDLS
ccflags-$(CONFIG_QCOM_TDLS) += -DWLAN_FEATURE_TDLS_CONCURRENCIES

ifeq (y,$(filter y,$(CONFIG_LITHIUM) $(CONFIG_BERYLLIUM)))
ccflags-$(CONFIG_QCOM_TDLS) += -DTDLS_WOW_ENABLED
endif

ccflags-$(CONFIG_WLAN_SYSFS_TDLS_PEERS) += -DWLAN_SYSFS_TDLS_PEERS
ccflags-$(CONFIG_WLAN_SYSFS_RANGE_EXT) += -DWLAN_SYSFS_RANGE_EXT

ccflags-$(CONFIG_QCACLD_WLAN_LFR2) += -DWLAN_FEATURE_PREAUTH_ENABLE

ifeq ($(CONFIG_CM_UTF_ENABLE), y)
ccflags-y += -DFEATURE_CM_UTF_ENABLE
endif

ccflags-$(CONFIG_QCACLD_WLAN_LFR3) += -DWLAN_FEATURE_ROAM_OFFLOAD
ccflags-$(CONFIG_WLAN_FEATURE_ROAM_INFO_STATS) += -DWLAN_FEATURE_ROAM_INFO_STATS
ccflags-$(CONFIG_QCACLD_WLAN_CONNECTIVITY_LOGGING) += -DWLAN_FEATURE_CONNECTIVITY_LOGGING
ccflags-$(CONFIG_QCACLD_WLAN_CONNECTIVITY_DIAG_EVENT) += -DCONNECTIVITY_DIAG_EVENT
ccflags-$(CONFIG_OFDM_SCRAMBLER_SEED) += -DWLAN_FEATURE_OFDM_SCRAMBLER_SEED

ccflags-$(CONFIG_WLAN_FEATURE_MBSSID) += -DWLAN_FEATURE_MBSSID
ccflags-$(CONFIG_WLAN_FEATURE_P2P_P2P_STA) += -DWLAN_FEATURE_P2P_P2P_STA

ifeq (y,$(findstring y, $(CONFIG_CNSS_GENL) $(CONFIG_CNSS_GENL_MODULE)))
ccflags-y += -DCNSS_GENL
endif

ifeq (y,$(findstring y, $(CONFIG_CNSS_UTILS) $(CONFIG_CNSS_UTILS_MODULE)))
ccflags-y += -DCNSS_UTILS
endif

ifeq (y,$(findstring y, $(CONFIG_WCNSS_MEM_PRE_ALLOC) $(CONFIG_WCNSS_MEM_PRE_ALLOC_MODULE)))
ccflags-y += -DCNSS_MEM_PRE_ALLOC
endif

ccflags-$(CONFIG_QCACLD_WLAN_LFR2) += -DWLAN_FEATURE_HOST_ROAM

ccflags-$(CONFIG_FEATURE_ROAM_DEBUG) += -DFEATURE_ROAM_DEBUG

ccflags-$(CONFIG_WLAN_POWER_DEBUG) += -DWLAN_POWER_DEBUG

ccflags-$(CONFIG_WLAN_MWS_INFO_DEBUGFS) += -DWLAN_MWS_INFO_DEBUGFS

ifeq ($(CONFIG_WLAN_DEBUG_LINK_VOTE), y)
ccflags-$(CONFIG_WLAN_DEBUG_LINK_VOTE) += -DWLAN_DEBUG_LINK_VOTE
endif
# Enable object manager reference count debug infrastructure
ccflags-$(CONFIG_WLAN_OBJMGR_DEBUG) += -DWLAN_OBJMGR_DEBUG
ccflags-$(CONFIG_WLAN_OBJMGR_DEBUG) += -DWLAN_OBJMGR_REF_ID_DEBUG
ccflags-$(CONFIG_WLAN_OBJMGR_REF_ID_TRACE) += -DWLAN_OBJMGR_REF_ID_TRACE
ccflags-$(CONFIG_FEATURE_DELAYED_PEER_OBJ_DESTROY) += -DFEATURE_DELAYED_PEER_OBJ_DESTROY

ccflags-$(CONFIG_WLAN_FEATURE_SAE) += -DWLAN_FEATURE_SAE

ifeq ($(CONFIG_WLAN_DIAG_VERSION), y)
ccflags-y += -DFEATURE_WLAN_DIAG_SUPPORT
ccflags-y += -DFEATURE_WLAN_DIAG_SUPPORT_CSR
ccflags-y += -DFEATURE_WLAN_DIAG_SUPPORT_LIM
ifeq ($(CONFIG_HIF_PCI), y)
ccflags-y += -DCONFIG_ATH_PROCFS_DIAG_SUPPORT
endif
ifeq ($(CONFIG_HIF_IPCI), y)
ccflags-y += -DCONFIG_ATH_PROCFS_DIAG_SUPPORT
endif
endif

ifeq ($(CONFIG_HIF_USB), y)
ccflags-y += -DCONFIG_ATH_PROCFS_DIAG_SUPPORT
ccflags-y += -DQCA_SUPPORT_OL_RX_REORDER_TIMEOUT
ccflags-y += -DCONFIG_ATH_PCIE_MAX_PERF=0 -DCONFIG_ATH_PCIE_AWAKE_WHILE_DRIVER_LOAD=0 -DCONFIG_DISABLE_CDC_MAX_PERF_WAR=0
endif

ccflags-$(CONFIG_QCA_SUPPORT_TXRX_DRIVER_TCP_DEL_ACK) += -DQCA_SUPPORT_TXRX_DRIVER_TCP_DEL_ACK

ccflags-$(CONFIG_QCA_TXDESC_SANITY_CHECKS) += -DQCA_SUPPORT_TXDESC_SANITY_CHECKS

ccflags-$(CONFIG_QCOM_LTE_COEX) += -DFEATURE_WLAN_CH_AVOID

ccflags-$(CONFIG_WLAN_FEATURE_LPSS) += -DWLAN_FEATURE_LPSS

ccflags-$(CONFIG_DESC_DUP_DETECT_DEBUG) += -DDESC_DUP_DETECT_DEBUG
ccflags-$(CONFIG_DEBUG_RX_RING_BUFFER) += -DDEBUG_RX_RING_BUFFER

ccflags-$(CONFIG_DESC_TIMESTAMP_DEBUG_INFO) += -DDESC_TIMESTAMP_DEBUG_INFO

ccflags-$(PANIC_ON_BUG) += -DPANIC_ON_BUG

ccflags-$(WLAN_WARN_ON_ASSERT) += -DWLAN_WARN_ON_ASSERT

ccflags-$(CONFIG_POWER_MANAGEMENT_OFFLOAD) += -DWLAN_POWER_MANAGEMENT_OFFLOAD

ccflags-$(CONFIG_WLAN_LOG_FATAL) += -DWLAN_LOG_FATAL
ccflags-$(CONFIG_WLAN_LOG_ERROR) += -DWLAN_LOG_ERROR
ccflags-$(CONFIG_WLAN_LOG_WARN) += -DWLAN_LOG_WARN
ccflags-$(CONFIG_WLAN_LOG_INFO) += -DWLAN_LOG_INFO
ccflags-$(CONFIG_WLAN_LOG_DEBUG) += -DWLAN_LOG_DEBUG
ccflags-$(CONFIG_WLAN_LOG_ENTER) += -DWLAN_LOG_ENTER
ccflags-$(CONFIG_WLAN_LOG_EXIT) += -DWLAN_LOG_EXIT
ccflags-$(WLAN_OPEN_SOURCE) += -DWLAN_OPEN_SOURCE
ccflags-$(CONFIG_FEATURE_STATS_EXT) += -DWLAN_FEATURE_STATS_EXT
ccflags-$(CONFIG_QCACLD_FEATURE_NAN) += -DWLAN_FEATURE_NAN
ccflags-$(CONFIG_QCACLD_FEATURE_SON) += -DWLAN_FEATURE_SON
ccflags-$(CONFIG_NDP_SAP_CONCURRENCY_ENABLE) += -DNDP_SAP_CONCURRENCY_ENABLE
ccflags-$(CONFIG_ENFORCE_PLD_REMOVE) += -DENFORCE_PLD_REMOVE

ifeq ($(CONFIG_DFS_FCC_TYPE4_DURATION_CHECK), y)
ccflags-$(CONFIG_DFS_FCC_TYPE4_DURATION_CHECK) += -DDFS_FCC_TYPE4_DURATION_CHECK
endif

ccflags-$(CONFIG_WLAN_SYSFS) += -DWLAN_SYSFS
ccflags-$(CONFIG_WLAN_SYSFS_CHANNEL) += -DWLAN_SYSFS_CHANNEL
ccflags-$(CONFIG_FEATURE_BECN_STATS) += -DWLAN_FEATURE_BEACON_RECEPTION_STATS

ccflags-$(CONFIG_WLAN_SYSFS_CONNECT_INFO) += -DWLAN_SYSFS_CONNECT_INFO
ccflags-$(CONFIG_WLAN_SYSFS_EHT_RATE) += -DWLAN_SYSFS_EHT_RATE
ccflags-$(CONFIG_WLAN_SYSFS_BITRATES) += -DWLAN_SYSFS_BITRATES

#Set RX_PERFORMANCE
ccflags-$(CONFIG_RX_PERFORMANCE) += -DRX_PERFORMANCE

#Set MULTI_IF_LOG
ccflags-$(CONFIG_MULTI_IF_LOG) += -DMULTI_IF_LOG

#Set SLUB_MEM_OPTIMIZE
ccflags-$(CONFIG_SLUB_MEM_OPTIMIZE) += -DSLUB_MEM_OPTIMIZE

ifeq ($(CONFIG_ARCH_SDXBAAGHA), y)
ccflags-$(CONFIG_WLAN_MEMORY_OPT) += -DWLAN_MEMORY_OPT
endif

#Set DFS_PRI_MULTIPLIER
ccflags-$(CONFIG_DFS_PRI_MULTIPLIER) += -DDFS_PRI_MULTIPLIER

#Set DFS_OVERRIDE_RF_THRESHOLD
ccflags-$(CONFIG_DFS_OVERRIDE_RF_THRESHOLD) += -DDFS_OVERRIDE_RF_THRESHOLD

#Enable OL debug and wmi unified functions
ccflags-$(CONFIG_ATH_PERF_PWR_OFFLOAD) += -DATH_PERF_PWR_OFFLOAD

#Disable packet log
ccflags-$(CONFIG_REMOVE_PKT_LOG) += -DREMOVE_PKT_LOG

#Enable 11AC TX
ccflags-$(CONFIG_ATH_11AC_TXCOMPACT) += -DATH_11AC_TXCOMPACT

#ENABLE HTT HTC tx completion
ccflags-$(ENABLE_CE4_COMP_DISABLE_HTT_HTC_MISC_LIST) += -DENABLE_CE4_COMP_DISABLE_HTT_HTC_MISC_LIST

#Enable PCI specific APIS (dma, etc)
ccflags-$(CONFIG_HIF_PCI) += -DHIF_PCI

ccflags-$(CONFIG_HIF_IPCI) += -DHIF_IPCI

ccflags-$(CONFIG_HIF_SNOC) += -DHIF_SNOC

ccflags-$(CONFIG_HL_DP_SUPPORT) += -DCONFIG_HL_SUPPORT
ccflags-$(CONFIG_HL_DP_SUPPORT) += -DWLAN_PARTIAL_REORDER_OFFLOAD
ccflags-$(CONFIG_HL_DP_SUPPORT) += -DQCA_COMPUTE_TX_DELAY
ccflags-$(CONFIG_HL_DP_SUPPORT) += -DQCA_COMPUTE_TX_DELAY_PER_TID
ccflags-$(CONFIG_LL_DP_SUPPORT) += -DCONFIG_LL_DP_SUPPORT
ccflags-$(CONFIG_LL_DP_SUPPORT) += -DWLAN_FULL_REORDER_OFFLOAD
ccflags-$(CONFIG_WLAN_FEATURE_BIG_DATA_STATS) += -DWLAN_FEATURE_BIG_DATA_STATS
ifeq ($(CONFIG_WLAN_FEATURE_11AX), y)
ccflags-$(CONFIG_WLAN_FEATURE_SR) += -DWLAN_FEATURE_SR
ccflags-$(CONFIG_OBSS_PD) += -DOBSS_PD
endif
ccflags-$(CONFIG_WLAN_FEATURE_IGMP_OFFLOAD) += -DWLAN_FEATURE_IGMP_OFFLOAD
ccflags-$(CONFIG_WLAN_FEATURE_GET_USABLE_CHAN_LIST) += -DWLAN_FEATURE_GET_USABLE_CHAN_LIST

# For PCIe GEN switch
ccflags-$(CONFIG_PCIE_GEN_SWITCH) += -DPCIE_GEN_SWITCH

# For OOB testing
ccflags-$(CONFIG_WLAN_FEATURE_WOW_PULSE) += -DWLAN_FEATURE_WOW_PULSE

#Enable High Latency related Flags
ifeq ($(CONFIG_QCA_WIFI_SDIO), y)
ccflags-y += -DCONFIG_AR6320_SUPPORT \
            -DSDIO_3_0 \
            -DHIF_SDIO \
            -DCONFIG_DISABLE_CDC_MAX_PERF_WAR=0 \
            -DCONFIG_ATH_PROCFS_DIAG_SUPPORT \
            -DHIF_MBOX_SLEEP_WAR \
            -DDEBUG_HL_LOGGING \
            -DQCA_BAD_PEER_TX_FLOW_CL \
            -DCONFIG_SDIO \
            -DFEATURE_WLAN_FORCE_SAP_SCC

ifeq ($(CONFIG_SDIO_TRANSFER), adma)
ccflags-y += -DCONFIG_SDIO_TRANSFER_ADMA
else
ccflags-y += -DCONFIG_SDIO_TRANSFER_MAILBOX
endif
endif

ccflags-$(CONFIG_AR6320_SUPPORT) += -DCONFIG_AR6320_SUPPORT

ifeq ($(CONFIG_WLAN_FEATURE_DSRC), y)
ccflags-y += -DWLAN_FEATURE_DSRC
ifeq ($(CONFIG_OCB_UT_FRAMEWORK), y)
ccflags-y += -DWLAN_OCB_UT
endif

else ifeq ($(CONFIG_WLAN_REG_AUTO), y)
ccflags-y += -DWLAN_REG_AUTO
endif

ccflags-$(CONFIG_FEATURE_SKB_PRE_ALLOC) += -DFEATURE_SKB_PRE_ALLOC

#Enable USB specific APIS
ifeq ($(CONFIG_HIF_USB), y)
ccflags-y += -DHIF_USB \
            -DDEBUG_HL_LOGGING
endif

#Enable Genoa specific features.
ccflags-$(CONFIG_QCA_HL_NETDEV_FLOW_CONTROL) += -DQCA_HL_NETDEV_FLOW_CONTROL
ccflags-$(CONFIG_FEATURE_HL_GROUP_CREDIT_FLOW_CONTROL) += -DFEATURE_HL_GROUP_CREDIT_FLOW_CONTROL
ccflags-$(CONFIG_FEATURE_HL_DBS_GROUP_CREDIT_SHARING) += -DFEATURE_HL_DBS_GROUP_CREDIT_SHARING
ccflags-$(CONFIG_CREDIT_REP_THROUGH_CREDIT_UPDATE) += -DCONFIG_CREDIT_REP_THROUGH_CREDIT_UPDATE
ccflags-$(CONFIG_RX_PN_CHECK_OFFLOAD) += -DCONFIG_RX_PN_CHECK_OFFLOAD

ccflags-$(CONFIG_WLAN_SYNC_TSF_TIMER) += -DWLAN_FEATURE_TSF_TIMER_SYNC
ccflags-$(CONFIG_WLAN_SYNC_TSF_PTP) += -DWLAN_FEATURE_TSF_PTP
ccflags-$(CONFIG_WLAN_SYNC_TSF_PLUS_EXT_GPIO_IRQ) += -DWLAN_FEATURE_TSF_PLUS_EXT_GPIO_IRQ
ccflags-$(CONFIG_WLAN_SYNC_TSF_PLUS_EXT_GPIO_SYNC) += -DWLAN_FEATURE_TSF_PLUS_EXT_GPIO_SYNC
ccflags-$(CONFIG_TX_DESC_HI_PRIO_RESERVE) += -DCONFIG_TX_DESC_HI_PRIO_RESERVE

#Enable power management suspend/resume functionality
ccflags-$(CONFIG_ATH_BUS_PM) += -DATH_BUS_PM

#Enable FLOWMAC module support
ccflags-$(CONFIG_ATH_SUPPORT_FLOWMAC_MODULE) += -DATH_SUPPORT_FLOWMAC_MODULE

#Enable spectral support
ccflags-$(CONFIG_ATH_SUPPORT_SPECTRAL) += -DATH_SUPPORT_SPECTRAL

#Enable legacy pktlog
ccflags-$(CONFIG_PKTLOG_LEGACY) += -DPKTLOG_LEGACY

#Enable WDI Event support
ccflags-$(CONFIG_WDI_EVENT_ENABLE) += -DWDI_EVENT_ENABLE

#Enable the type_specific_data in the struct ath_pktlog_arg
ccflags-$(CONFIG_PKTLOG_HAS_SPECIFIC_DATA) += -DPKTLOG_HAS_SPECIFIC_DATA

#Endianness selection
ifeq ($(CONFIG_LITTLE_ENDIAN), y)
ccflags-y += -DANI_LITTLE_BYTE_ENDIAN
ccflags-y += -DANI_LITTLE_BIT_ENDIAN
ccflags-y += -DDOT11F_LITTLE_ENDIAN_HOST
else
ccflags-y += -DANI_BIG_BYTE_ENDIAN
ccflags-y += -DBIG_ENDIAN_HOST
endif

#Enable TX reclaim support
ccflags-$(CONFIG_TX_CREDIT_RECLAIM_SUPPORT) += -DTX_CREDIT_RECLAIM_SUPPORT

#Enable FTM support
ccflags-$(CONFIG_QCA_WIFI_FTM) += -DQCA_WIFI_FTM
ccflags-$(CONFIG_NL80211_TESTMODE) += -DQCA_WIFI_FTM_NL80211
ccflags-$(CONFIG_LINUX_QCMBR) += -DLINUX_QCMBR -DQCA_WIFI_FTM_IOCTL

#Enable Checksum Offload support
ccflags-$(CONFIG_CHECKSUM_OFFLOAD) += -DCHECKSUM_OFFLOAD

#Enable IPA Offload support
ccflags-$(CONFIG_IPA_OFFLOAD) += -DIPA_OFFLOAD
ifeq ($(CONFIG_IPA_OFFLOAD), y)
ccflags-$(CONFIG_IPA_WDS_EASYMESH) += -DIPA_WDS_EASYMESH_FEATURE
endif

#Enable IPA optional Wifi datapath
ifeq ($(CONFIG_IPA_OPT_WIFI_DP), y)
ifeq ($(CONFIG_IPA_OFFLOAD), y)
ccflags-$(CONFIG_IPA_OPT_WIFI_DP) += -DIPA_OPT_WIFI_DP
endif
endif

ccflags-$(CONFIG_WDI3_IPA_OVER_GSI) += -DIPA_WDI3_GSI
ccflags-$(CONFIG_WDI2_IPA_OVER_GSI) += -DIPA_WDI2_GSI

#Enable WMI DIAG log over CE7
ccflags-$(CONFIG_WLAN_FEATURE_WMI_DIAG_OVER_CE7) += -DWLAN_FEATURE_WMI_DIAG_OVER_CE7

ifdef CONFIG_WLAN_DP_FEATURE_DEFERRED_REO_QDESC_DESTROY
ccflags-y += -DWLAN_DP_FEATURE_DEFERRED_REO_QDESC_DESTROY
endif

ifeq ($(CONFIG_ARCH_SDX20), y)
ccflags-y += -DSYNC_IPA_READY
endif

ifeq ($(CONFIG_ARCH_SDXPOORWILLS), y)
CONFIG_FEATURE_SG := y
endif

ifeq ($(CONFIG_ARCH_MSM8996), y)
ifneq ($(CONFIG_QCN7605_SUPPORT), y)
CONFIG_FEATURE_SG := y
CONFIG_RX_THREAD_PRIORITY := y
endif
endif

ifeq ($(CONFIG_FEATURE_SG), y)
ccflags-y += -DFEATURE_SG
endif

ifeq ($(CONFIG_RX_THREAD_PRIORITY), y)
ccflags-y += -DRX_THREAD_PRIORITY
endif

ifeq ($(CONFIG_SUPPORT_P2P_BY_ONE_INTF_WLAN), y)
#sta support to tx P2P action frames
ccflags-y += -DSUPPORT_P2P_BY_ONE_INTF_WLAN
else
#Open P2P device interface only for non-Mobile router use cases
ccflags-$(CONFIG_WLAN_OPEN_P2P_INTERFACE) += -DWLAN_OPEN_P2P_INTERFACE
endif

ccflags-$(CONFIG_WMI_BCN_OFFLOAD) += -DWLAN_WMI_BCN

#Enable wbuff
ccflags-$(CONFIG_WLAN_WBUFF) += -DWLAN_FEATURE_WBUFF

#Enable GTK Offload
ccflags-$(CONFIG_GTK_OFFLOAD) += -DWLAN_FEATURE_GTK_OFFLOAD

#Enable External WoW
ccflags-$(CONFIG_EXT_WOW) += -DWLAN_FEATURE_EXTWOW_SUPPORT

#Mark it as SMP Kernel
ccflags-$(CONFIG_SMP) += -DQCA_CONFIG_SMP

#CONFIG_RPS default Y, but depend on CONFIG_SMP
ccflags-$(CONFIG_RPS) += -DQCA_CONFIG_RPS

ccflags-$(CONFIG_CHNL_MATRIX_RESTRICTION) += -DWLAN_ENABLE_CHNL_MATRIX_RESTRICTION

#Enable ICMP packet disable powersave feature
ccflags-$(CONFIG_ICMP_DISABLE_PS) += -DWLAN_ICMP_DISABLE_PS

#enable MCC TO SCC switch
ccflags-$(CONFIG_FEATURE_WLAN_MCC_TO_SCC_SWITCH) += -DFEATURE_WLAN_MCC_TO_SCC_SWITCH

#enable wlan auto shutdown feature
ccflags-$(CONFIG_FEATURE_WLAN_AUTO_SHUTDOWN) += -DFEATURE_WLAN_AUTO_SHUTDOWN

#enable AP-AP ACS Optimization
ccflags-$(CONFIG_FEATURE_WLAN_AP_AP_ACS_OPTIMIZE) += -DFEATURE_WLAN_AP_AP_ACS_OPTIMIZE

#enable DCS feature in vdev level
ccflags-$(CONFIG_WLAN_FEATURE_VDEV_DCS) += -DWLAN_FEATURE_VDEV_DCS

#Enable 4address scheme
ccflags-$(CONFIG_FEATURE_WLAN_STA_4ADDR_SCHEME) += -DFEATURE_WLAN_STA_4ADDR_SCHEME

#Optimize GC connection speed by skipping JOIN
ccflags-$(CONFIG_FEATURE_WLAN_GC_SKIP_JOIN) += -DFEATURE_WLAN_GC_SKIP_JOIN

#enable MDM/SDX special config
ccflags-$(CONFIG_MDM_PLATFORM) += -DMDM_PLATFORM

#Disable STA-AP Mode DFS support
ccflags-$(CONFIG_FEATURE_WLAN_STA_AP_MODE_DFS_DISABLE) += -DFEATURE_WLAN_STA_AP_MODE_DFS_DISABLE

#Enable 2.4 GHz social channels in 5 GHz only mode for p2p usage
ccflags-$(CONFIG_WLAN_ENABLE_SOCIAL_CHANNELS_5G_ONLY) += -DWLAN_ENABLE_SOCIAL_CHANNELS_5G_ONLY

#Green AP feature
ccflags-$(CONFIG_QCACLD_FEATURE_GREEN_AP) += -DWLAN_SUPPORT_GREEN_AP

ccflags-$(CONFIG_QCACLD_FEATURE_GAP_LL_PS_MODE) += -DWLAN_SUPPORT_GAP_LL_PS_MODE

ccflags-$(CONFIG_QCACLD_FEATURE_APF) += -DFEATURE_WLAN_APF

ccflags-$(CONFIG_WLAN_FEATURE_SARV1_TO_SARV2) += -DWLAN_FEATURE_SARV1_TO_SARV2

ccflags-$(CONFIG_FEATURE_WLAN_FT_IEEE8021X) += -DFEATURE_WLAN_FT_IEEE8021X
ccflags-$(CONFIG_FEATURE_WLAN_FT_PSK) += -DFEATURE_WLAN_FT_PSK

#Enable host 11d scan
ccflags-$(CONFIG_HOST_11D_SCAN) += -DHOST_11D_SCAN

#Stats & Quota Metering feature
ifeq ($(CONFIG_IPA_OFFLOAD), y)
ifeq ($(CONFIG_QCACLD_FEATURE_METERING), y)
ccflags-y += -DFEATURE_METERING
endif
endif

#Define Max IPA interface
ifeq ($(CONFIG_IPA_OFFLOAD), y)
ifdef CONFIG_NUM_IPA_IFACE
ccflags-y += -DMAX_IPA_IFACE=$(CONFIG_NUM_IPA_IFACE)
else
NUM_IPA_IFACE ?= 3
ccflags-y += -DMAX_IPA_IFACE=$(NUM_IPA_IFACE)
endif
endif


ccflags-$(CONFIG_TUFELLO_DUAL_FW_SUPPORT) += -DCONFIG_TUFELLO_DUAL_FW_SUPPORT
ccflags-$(CONFIG_CHANNEL_HOPPING_ALL_BANDS) += -DCHANNEL_HOPPING_ALL_BANDS

#Enable Signed firmware support for split binary format
ccflags-$(CONFIG_QCA_SIGNED_SPLIT_BINARY_SUPPORT) += -DQCA_SIGNED_SPLIT_BINARY_SUPPORT

#Enable single firmware binary format
ccflags-$(CONFIG_QCA_SINGLE_BINARY_SUPPORT) += -DQCA_SINGLE_BINARY_SUPPORT

#Enable collecting target RAM dump after kernel panic
ccflags-$(CONFIG_TARGET_RAMDUMP_AFTER_KERNEL_PANIC) += -DTARGET_RAMDUMP_AFTER_KERNEL_PANIC

#Enable/disable secure firmware feature
ccflags-$(CONFIG_FEATURE_SECURE_FIRMWARE) += -DFEATURE_SECURE_FIRMWARE

ccflags-$(CONFIG_ATH_PCIE_ACCESS_DEBUG) += -DCONFIG_ATH_PCIE_ACCESS_DEBUG

# Enable feature support for Linux version QCMBR
ccflags-$(CONFIG_LINUX_QCMBR) += -DLINUX_QCMBR

# Enable feature sync tsf between multi devices
ccflags-$(CONFIG_WLAN_SYNC_TSF) += -DWLAN_FEATURE_TSF

ifeq ($(CONFIG_WLAN_SYNC_TSF_PLUS), y)
ccflags-y += -DWLAN_FEATURE_TSF_PLUS

ccflags-$(CONFIG_WLAN_SYNC_TSF_ACCURACY) += -DWLAN_FEATURE_TSF_ACCURACY

ifneq ($(CONFIG_WLAN_SYNC_TSF_PLUS_DISABLE_SOCK_TS), y)
ccflags-y += -DWLAN_FEATURE_TSF_PLUS_SOCK_TS
endif

endif

# Enable feature sync tsf for chips based on Adrastea arch
ccflags-$(CONFIG_WLAN_SYNC_TSF_PLUS_NOIRQ) += -DWLAN_FEATURE_TSF_PLUS_NOIRQ

ifeq ($(CONFIG_WLAN_TSF_UPLINK_DELAY), y)
# Enable uplink delay report feature
ccflags-y += -DWLAN_FEATURE_TSF_UPLINK_DELAY
CONFIG_WLAN_TSF_AUTO_REPORT := y
endif

# Enable tx latency stats feature
ifeq ($(CONFIG_WLAN_TX_LATENCY_STATS), y)
ccflags-y += -DWLAN_FEATURE_TX_LATENCY_STATS
CONFIG_WLAN_TSF_AUTO_REPORT := y
endif

# Enable TSF auto report feature
ccflags-$(CONFIG_WLAN_TSF_AUTO_REPORT) += -DWLAN_FEATURE_TSF_AUTO_REPORT

ccflags-$(CONFIG_ATH_PROCFS_DIAG_SUPPORT) += -DCONFIG_ATH_PROCFS_DIAG_SUPPORT

ccflags-$(CONFIG_HELIUMPLUS) += -DHELIUMPLUS
ccflags-$(CONFIG_RX_OL) += -DRECEIVE_OFFLOAD
ccflags-$(CONFIG_TX_TID_OVERRIDE) += -DATH_TX_PRI_OVERRIDE
ccflags-$(CONFIG_AR900B) += -DAR900B
ccflags-$(CONFIG_HTT_PADDR64) += -DHTT_PADDR64
ccflags-$(CONFIG_OL_RX_INDICATION_RECORD) += -DOL_RX_INDICATION_RECORD
ccflags-$(CONFIG_TSOSEG_DEBUG) += -DTSOSEG_DEBUG
ccflags-$(CONFIG_ALLOW_PKT_DROPPING) += -DFEATURE_ALLOW_PKT_DROPPING

# Enable feature for athdiag live debug mode
ccflags-$(CONFIG_ATH_DIAG_EXT_DIRECT) += -DATH_DIAG_EXT_DIRECT

ccflags-$(CONFIG_ENABLE_DEBUG_ADDRESS_MARKING) += -DENABLE_DEBUG_ADDRESS_MARKING
ccflags-$(CONFIG_FEATURE_TSO) += -DFEATURE_TSO
ccflags-$(CONFIG_FEATURE_TSO_DEBUG) += -DFEATURE_TSO_DEBUG
ccflags-$(CONFIG_FEATURE_TSO_STATS) += -DFEATURE_TSO_STATS
ccflags-$(CONFIG_FEATURE_FORCE_WAKE) += -DFORCE_WAKE
ccflags-$(CONFIG_WLAN_LRO) += -DFEATURE_LRO

ccflags-$(CONFIG_FEATURE_AP_MCC_CH_AVOIDANCE) += -DFEATURE_AP_MCC_CH_AVOIDANCE

ccflags-$(CONFIG_FEATURE_EPPING) += -DWLAN_FEATURE_EPPING

ccflags-$(CONFIG_WLAN_OFFLOAD_PACKETS) += -DWLAN_FEATURE_OFFLOAD_PACKETS

ccflags-$(CONFIG_WLAN_FEATURE_DISA) += -DWLAN_FEATURE_DISA

ccflags-$(CONFIG_WLAN_FEATURE_ACTION_OUI) += -DWLAN_FEATURE_ACTION_OUI

ccflags-$(CONFIG_WLAN_FEATURE_FIPS) += -DWLAN_FEATURE_FIPS

ccflags-$(CONFIG_LFR_SUBNET_DETECTION) += -DFEATURE_LFR_SUBNET_DETECTION

ccflags-$(CONFIG_MCC_TO_SCC_SWITCH) += -DFEATURE_WLAN_MCC_TO_SCC_SWITCH

ccflags-$(CONFIG_FEATURE_WLAN_D0WOW) += -DFEATURE_WLAN_D0WOW

ccflags-$(CONFIG_WLAN_FEATURE_PKT_CAPTURE) += -DWLAN_FEATURE_PKT_CAPTURE

ccflags-$(CONFIG_WLAN_FEATURE_PKT_CAPTURE_V2) += -DWLAN_FEATURE_PKT_CAPTURE_V2

ccflags-$(CONFIG_DP_RX_UDP_OVER_PEER_ROAM) += -DDP_RX_UDP_OVER_PEER_ROAM

cppflags-$(CONFIG_WLAN_BOOST_CPU_FREQ_IN_ROAM) += -DWLAN_BOOST_CPU_FREQ_IN_ROAM

ccflags-$(CONFIG_QCA_WIFI_EMULATION) += -DQCA_WIFI_EMULATION
ccflags-$(CONFIG_SAP_MULTI_LINK_EMULATION) += -DSAP_MULTI_LINK_EMULATION
ccflags-$(CONFIG_SHADOW_V2) += -DCONFIG_SHADOW_V2
ccflags-$(CONFIG_SHADOW_V3) += -DCONFIG_SHADOW_V3
ccflags-$(CONFIG_QCA6290_HEADERS_DEF) += -DQCA6290_HEADERS_DEF
ccflags-$(CONFIG_QCA_WIFI_QCA6290) += -DQCA_WIFI_QCA6290
ccflags-$(CONFIG_QCA6390_HEADERS_DEF) += -DQCA6390_HEADERS_DEF
ccflags-$(CONFIG_QCA6750_HEADERS_DEF) += -DQCA6750_HEADERS_DEF
ccflags-$(CONFIG_QCA_WIFI_QCA6390) += -DQCA_WIFI_QCA6390
ccflags-$(CONFIG_QCA6490_HEADERS_DEF) += -DQCA6490_HEADERS_DEF
ccflags-$(CONFIG_KIWI_HEADERS_DEF) += -DKIWI_HEADERS_DEF
ccflags-$(CONFIG_WCN6450_HEADERS_DEF) += -DWCN6450_HEADERS_DEF
ccflags-$(CONFIG_WCN7750_HEADERS_DEF) += -DWCN7750_HEADERS_DEF
ccflags-$(CONFIG_QCA_WIFI_QCA6490) += -DQCA_WIFI_QCA6490
ccflags-$(CONFIG_QCA_WIFI_QCA6750) += -DQCA_WIFI_QCA6750
ccflags-$(CONFIG_QCA_WIFI_WCN7750) += -DQCA_WIFI_WCN7750
ccflags-$(CONFIG_QCA_WIFI_KIWI) += -DQCA_WIFI_KIWI
ccflags-$(CONFIG_QCA_WIFI_WCN6450) += -DQCA_WIFI_WCN6450
ccflags-$(CONFIG_QCA_WIFI_WCN6450) += -DWLAN_40BIT_ADDRESSING_SUPPORT
ccflags-$(CONFIG_QCA_WIFI_WCN6450) += -DWLAN_64BIT_DATA_SUPPORT
ccflags-$(CONFIG_CE_LEGACY_MSI_SUPPORT) += -DCE_LEGACY_MSI_SUPPORT
ccflags-$(CONFIG_HIF_HAL_REG_ACCESS_SUPPORT) += -DHIF_HAL_REG_ACCESS_SUPPORT
ccflags-$(CONFIG_FEATURE_HIF_DELAYED_REG_WRITE) += -DFEATURE_HIF_DELAYED_REG_WRITE
ccflags-$(CONFIG_CNSS_KIWI_V2) += -DQCA_WIFI_KIWI_V2
ccflags-$(CONFIG_CNSS_MANGO) += -DQCA_WIFI_MANGO
ccflags-$(CONFIG_CNSS_PEACH) += -DQCA_WIFI_PEACH
ccflags-$(CONFIG_QCA_WIFI_QCA8074) += -DQCA_WIFI_QCA8074
ccflags-$(CONFIG_SCALE_INCLUDES) += -DSCALE_INCLUDES
ccflags-$(CONFIG_QCA_WIFI_QCA8074_VP) += -DQCA_WIFI_QCA8074_VP
ccflags-$(CONFIG_DP_INTR_POLL_BASED) += -DDP_INTR_POLL_BASED
ccflags-$(CONFIG_TX_PER_PDEV_DESC_POOL) += -DTX_PER_PDEV_DESC_POOL
ccflags-$(CONFIG_DP_TRACE) += -DCONFIG_DP_TRACE
ccflags-$(CONFIG_FEATURE_TSO) += -DFEATURE_TSO
ccflags-$(CONFIG_TSO_DEBUG_LOG_ENABLE) += -DTSO_DEBUG_LOG_ENABLE
ccflags-$(CONFIG_DP_LFR) += -DDP_LFR
ccflags-$(CONFIG_DUP_RX_DESC_WAR) += -DDUP_RX_DESC_WAR
ccflags-$(CONFIG_DP_MEM_PRE_ALLOC) += -DDP_MEM_PRE_ALLOC
ccflags-$(CONFIG_DP_TXRX_SOC_ATTACH) += -DDP_TXRX_SOC_ATTACH
ccflags-$(CONFIG_WLAN_FEATURE_BMI) += -DWLAN_FEATURE_BMI
ccflags-$(CONFIG_QCA_TX_PADDING_CREDIT_SUPPORT) += -DQCA_TX_PADDING_CREDIT_SUPPORT
ccflags-$(CONFIG_QCN7605_SUPPORT) += -DQCN7605_SUPPORT -DPLATFORM_GENOA
ccflags-$(CONFIG_HIF_REG_WINDOW_SUPPORT) += -DHIF_REG_WINDOW_SUPPORT
ccflags-$(CONFIG_WLAN_ALLOCATE_GLOBAL_BUFFERS_DYNAMICALLY) += -DWLAN_ALLOCATE_GLOBAL_BUFFERS_DYNAMICALLY
ccflags-$(CONFIG_HIF_CE_DEBUG_DATA_BUF) += -DHIF_CE_DEBUG_DATA_BUF
ccflags-$(CONFIG_IPA_DISABLE_OVERRIDE) += -DIPA_DISABLE_OVERRIDE
ccflags-$(CONFIG_QCA_LL_TX_FLOW_CONTROL_RESIZE) += -DQCA_LL_TX_FLOW_CONTROL_RESIZE
ccflags-$(CONFIG_HIF_PCI) += -DCE_SVC_CMN_INIT
ccflags-$(CONFIG_HIF_IPCI) += -DCE_SVC_CMN_INIT
ccflags-$(CONFIG_HIF_SNOC) += -DCE_SVC_CMN_INIT
ccflags-$(CONFIG_RX_DESC_SANITY_WAR) += -DRX_DESC_SANITY_WAR
ccflags-$(CONFIG_WBM_IDLE_LSB_WR_CNF_WAR) += -DWBM_IDLE_LSB_WRITE_CONFIRM_WAR
ccflags-$(CONFIG_DYNAMIC_RX_AGGREGATION) += -DWLAN_FEATURE_DYNAMIC_RX_AGGREGATION
ccflags-$(CONFIG_DP_FEATURE_HW_COOKIE_CONVERSION) += -DDP_FEATURE_HW_COOKIE_CONVERSION
ccflags-$(CONFIG_DP_HW_COOKIE_CONVERT_EXCEPTION) += -DDP_HW_COOKIE_CONVERT_EXCEPTION
ccflags-$(CONFIG_TX_ADDR_INDEX_SEARCH) += -DTX_ADDR_INDEX_SEARCH
ccflags-$(CONFIG_QCA_SUPPORT_TX_MIN_RATES_FOR_SPECIAL_FRAMES) += -DQCA_SUPPORT_TX_MIN_RATES_FOR_SPECIAL_FRAMES
ccflags-$(CONFIG_QCA_GET_TSF_VIA_REG) += -DQCA_GET_TSF_VIA_REG
ccflags-$(CONFIG_DP_TX_COMP_RING_DESC_SANITY_CHECK) += -DDP_TX_COMP_RING_DESC_SANITY_CHECK
ccflags-$(CONFIG_HAL_SRNG_REG_HIS_DEBUG) += -DHAL_SRNG_REG_HIS_DEBUG
ccflags-$(CONFIG_DP_MLO_LINK_STATS_SUPPORT) += -DDP_MLO_LINK_STATS_SUPPORT
ccflags-$(CONFIG_DP_RX_BUFFER_OPTIMIZATION) += -DDP_RX_BUFFER_OPTIMIZATION
ccflags-$(CONFIG_WORD_BASED_TLV) += -DBE_NON_AP_COMPACT_TLV

ccflags-$(CONFIG_RX_HASH_DEBUG) += -DRX_HASH_DEBUG
ccflags-$(CONFIG_DP_PKT_STATS_PER_LMAC) += -DDP_PKT_STATS_PER_LMAC
ccflags-$(CONFIG_NO_RX_PKT_HDR_TLV) += -DNO_RX_PKT_HDR_TLV
ccflags-$(CONFIG_DP_TX_PACKET_INSPECT_FOR_ILP) += -DDP_TX_PACKET_INSPECT_FOR_ILP

ifeq ($(CONFIG_QCA6290_11AX), y)
ccflags-y += -DQCA_WIFI_QCA6290_11AX -DQCA_WIFI_QCA6290_11AX_MU_UL
endif

ccflags-$(CONFIG_WLAN_TX_FLOW_CONTROL_V2) += -DQCA_AC_BASED_FLOW_CONTROL

# Enable Low latency optimisation mode
ccflags-$(CONFIG_FEATURE_NO_DBS_INTRABAND_MCC_SUPPORT) += -DFEATURE_NO_DBS_INTRABAND_MCC_SUPPORT
ccflags-$(CONFIG_HAL_DISABLE_NON_BA_2K_JUMP_ERROR) += -DHAL_DISABLE_NON_BA_2K_JUMP_ERROR
ccflags-$(CONFIG_ENABLE_HAL_SOC_STATS) += -DENABLE_HAL_SOC_STATS
ccflags-$(CONFIG_ENABLE_HAL_REG_WR_HISTORY) += -DENABLE_HAL_REG_WR_HISTORY
ccflags-$(CONFIG_DP_RX_DESC_COOKIE_INVALIDATE) += -DDP_RX_DESC_COOKIE_INVALIDATE
ccflags-$(CONFIG_MON_ENABLE_DROP_FOR_MAC) += -DMON_ENABLE_DROP_FOR_MAC
ccflags-$(CONFIG_MON_ENABLE_DROP_FOR_NON_MON_PMAC) += -DMON_ENABLE_DROP_FOR_NON_MON_PMAC
ccflags-$(CONFIG_DP_WAR_INVALID_FIRST_MSDU_FLAG) += -DDP_WAR_INVALID_FIRST_MSDU_FLAG
ccflags-$(CONFIG_LITHIUM) += -DDISABLE_MON_RING_MSI_CFG
ccflags-$(CONFIG_LITHIUM) += -DFEATURE_IRQ_AFFINITY
ccflags-$(CONFIG_RHINE) += -DFEATURE_IRQ_AFFINITY
ccflags-$(CONFIG_RHINE) += -DWLAN_SOFTUMAC_SUPPORT
ccflags-$(CONFIG_BERYLLIUM) += -DFEATURE_IRQ_AFFINITY
ccflags-$(CONFIG_TX_MULTIQ_PER_AC) += -DTX_MULTIQ_PER_AC
ccflags-$(CONFIG_PCI_LINK_STATUS_SANITY) += -DPCI_LINK_STATUS_SANITY
ccflags-$(CONFIG_DDP_MON_RSSI_IN_DBM) += -DDP_MON_RSSI_IN_DBM
ccflags-$(CONFIG_SYSTEM_PM_CHECK) += -DSYSTEM_PM_CHECK
ccflags-$(CONFIG_DISABLE_EAPOL_INTRABSS_FWD) += -DDISABLE_EAPOL_INTRABSS_FWD
ccflags-$(CONFIG_TX_AGGREGATION_SIZE_ENABLE) += -DTX_AGGREGATION_SIZE_ENABLE
ccflags-$(CONFIG_TX_MULTI_TCL) += -DTX_MULTI_TCL
ccflags-$(CONFIG_WLAN_DP_DISABLE_TCL_CMD_CRED_SRNG) += -DWLAN_DP_DISABLE_TCL_CMD_CRED_SRNG
ccflags-$(CONFIG_WLAN_DP_DISABLE_TCL_STATUS_SRNG) += -DWLAN_DP_DISABLE_TCL_STATUS_SRNG
ccflags-$(CONFIG_DP_WAR_VALIDATE_RX_ERR_MSDU_COOKIE) += -DDP_WAR_VALIDATE_RX_ERR_MSDU_COOKIE
ccflags-$(CONFIG_WLAN_DP_SRNG_USAGE_WM_TRACKING) += -DWLAN_DP_SRNG_USAGE_WM_TRACKING
ccflags-$(CONFIG_WLAN_FEATURE_DP_CFG_EVENT_HISTORY) += -DWLAN_FEATURE_DP_CFG_EVENT_HISTORY
ccflags-$(CONFIG_WLAN_DP_VDEV_NO_SELF_PEER) += -DWLAN_DP_VDEV_NO_SELF_PEER
ccflags-$(CONFIG_DP_RX_MSDU_DONE_FAIL_HISTORY) += -DDP_RX_MSDU_DONE_FAIL_HISTORY
ccflags-$(CONFIG_DP_RX_PEEK_MSDU_DONE_WAR) += -DDP_RX_PEEK_MSDU_DONE_WAR

# Enable Low latency
ccflags-$(CONFIG_WLAN_FEATURE_LL_MODE) += -DWLAN_FEATURE_LL_MODE

# Enable PCI low power interrupt register configuration
ccflags-$(CONFIG_PCI_LOW_POWER_INT_REG) += -DCONFIG_PCI_LOW_POWER_INT_REG

ccflags-$(CONFIG_WLAN_CLD_PM_QOS) += -DCLD_PM_QOS
ccflags-$(CONFIG_WLAN_CLD_DEV_PM_QOS) += -DCLD_DEV_PM_QOS
ccflags-$(CONFIG_REO_DESC_DEFER_FREE) += -DREO_DESC_DEFER_FREE
ccflags-$(CONFIG_WLAN_FEATURE_11AX) += -DWLAN_FEATURE_11AX
ccflags-$(CONFIG_WLAN_FEATURE_11AX) += -DWLAN_FEATURE_11AX_BSS_COLOR
ccflags-$(CONFIG_WLAN_FEATURE_11AX) += -DSUPPORT_11AX_D3
ccflags-$(CONFIG_RXDMA_ERR_PKT_DROP) += -DRXDMA_ERR_PKT_DROP
ccflags-$(CONFIG_MAX_ALLOC_PAGE_SIZE) += -DMAX_ALLOC_PAGE_SIZE
ccflags-$(CONFIG_DELIVERY_TO_STACK_STATUS_CHECK) += -DDELIVERY_TO_STACK_STATUS_CHECK
ccflags-$(CONFIG_WLAN_TRACE_HIDE_MAC_ADDRESS) += -DWLAN_TRACE_HIDE_MAC_ADDRESS
ccflags-$(CONFIG_WLAN_TRACE_HIDE_SSID) += -DWLAN_TRACE_HIDE_SSID
ccflags-$(CONFIG_WLAN_DP_MLO_DEV_CTX) += -DWLAN_DP_MLO_DEV_CTX
ccflags-$(CONFIG_WLAN_FEATURE_11BE) += -DWLAN_FEATURE_11BE
ccflags-$(CONFIG_WLAN_FEATURE_11BE_MLO) += -DWLAN_FEATURE_11BE_MLO
ccflags-$(CONFIG_WLAN_FEATURE_11BE_MLO) += -DWLAN_FEATURE_11BE_MLO_ADV_FEATURE
ccflags-$(CONFIG_WLAN_HDD_MULTI_VDEV_SINGLE_NDEV) += -DWLAN_HDD_MULTI_VDEV_SINGLE_NDEV
ifeq ($(CONFIG_WLAN_FEATURE_11BE_MLO), y)
ccflags-$(CONFIG_WLAN_FEATURE_MULTI_LINK_SAP) += -DWLAN_FEATURE_MULTI_LINK_SAP
endif
ccflags-$(CONFIG_WLAN_FEATURE_11BE_MLO) += -DWLAN_SUPPORT_11BE_D3_0
ccflags-$(CONFIG_WLAN_MCAST_MLO_SAP) += -DWLAN_MCAST_MLO_SAP
ccflags-$(CONFIG_FIX_TXDMA_LIMITATION) += -DFIX_TXDMA_LIMITATION
ccflags-$(CONFIG_FEATURE_AST) += -DFEATURE_AST
ccflags-$(CONFIG_PEER_PROTECTED_ACCESS) += -DPEER_PROTECTED_ACCESS
ccflags-$(CONFIG_SERIALIZE_QUEUE_SETUP) += -DSERIALIZE_QUEUE_SETUP
ccflags-$(CONFIG_DP_RX_PKT_NO_PEER_DELIVER) += -DDP_RX_PKT_NO_PEER_DELIVER
ccflags-$(CONFIG_DP_RX_DROP_RAW_FRM) += -DDP_RX_DROP_RAW_FRM
ccflags-$(CONFIG_FEATURE_ALIGN_STATS_FROM_DP) += -DFEATURE_ALIGN_STATS_FROM_DP
ccflags-$(CONFIG_DP_RX_SPECIAL_FRAME_NEED) += -DDP_RX_SPECIAL_FRAME_NEED
ccflags-$(CONFIG_FEATURE_STATS_EXT_V2) += -DFEATURE_STATS_EXT_V2
ccflags-$(CONFIG_WLAN_DP_TXPOOL_SHARE) += -DWLAN_DP_TXPOOL_SHARE
ccflags-$(CONFIG_WLAN_FEATURE_CAL_FAILURE_TRIGGER) += -DWLAN_FEATURE_CAL_FAILURE_TRIGGER
ccflags-$(CONFIG_WLAN_FEATURE_DYNAMIC_MAC_ADDR_UPDATE) += -DWLAN_FEATURE_DYNAMIC_MAC_ADDR_UPDATE
ccflags-$(CONFIG_WLAN_FEATURE_SAP_ACS_OPTIMIZE) += -DWLAN_FEATURE_SAP_ACS_OPTIMIZE
ccflags-$(CONFIG_QCA_DP_TX_FW_METADATA_V2)+= -DQCA_DP_TX_FW_METADATA_V2

ccflags-$(CONFIG_VERBOSE_DEBUG) += -DENABLE_VERBOSE_DEBUG
ccflags-$(CONFIG_RX_DESC_DEBUG_CHECK) += -DRX_DESC_DEBUG_CHECK
ccflags-$(CONFIG_REGISTER_OP_DEBUG) += -DHAL_REGISTER_WRITE_DEBUG
ccflags-$(CONFIG_ENABLE_QDF_PTR_HASH_DEBUG) += -DENABLE_QDF_PTR_HASH_DEBUG
#Enable STATE MACHINE HISTORY
ccflags-$(CONFIG_SM_ENG_HIST) += -DSM_ENG_HIST_ENABLE
ccflags-$(CONFIG_FEATURE_VDEV_OPS_WAKELOCK) += -DFEATURE_VDEV_OPS_WAKELOCK

# Vendor Commands
ccflags-$(CONFIG_FEATURE_RSSI_MONITOR) += -DFEATURE_RSSI_MONITOR
ccflags-$(CONFIG_FEATURE_BSS_TRANSITION) += -DFEATURE_BSS_TRANSITION
ccflags-$(CONFIG_FEATURE_STATION_INFO) += -DFEATURE_STATION_INFO
ccflags-$(CONFIG_FEATURE_TX_POWER) += -DFEATURE_TX_POWER
ccflags-$(CONFIG_FEATURE_OTA_TEST) += -DFEATURE_OTA_TEST
ccflags-$(CONFIG_FEATURE_ACTIVE_TOS) += -DFEATURE_ACTIVE_TOS
ccflags-$(CONFIG_FEATURE_SAR_LIMITS) += -DFEATURE_SAR_LIMITS
ccflags-$(CONFIG_FEATURE_CONCURRENCY_MATRIX) += -DFEATURE_CONCURRENCY_MATRIX
ccflags-$(CONFIG_FEATURE_SAP_COND_CHAN_SWITCH) += -DFEATURE_SAP_COND_CHAN_SWITCH
ccflags-$(CONFIG_FEATURE_WLAN_CH_AVOID_EXT) += -DFEATURE_WLAN_CH_AVOID_EXT
ccflags-$(CONFIG_WLAN_FEATURE_MDNS_OFFLOAD) += -DWLAN_FEATURE_MDNS_OFFLOAD

#if converged p2p is enabled
ifeq ($(CONFIG_CONVERGED_P2P_ENABLE), y)
ccflags-$(CONFIG_FEATURE_P2P_LISTEN_OFFLOAD) += -DFEATURE_P2P_LISTEN_OFFLOAD
endif

#Enable support to get ANI value
ifeq ($(CONFIG_ANI_LEVEL_REQUEST), y)
ccflags-y += -DFEATURE_ANI_LEVEL_REQUEST
endif

#Flags to enable/disable WMI APIs
ccflags-$(CONFIG_WMI_ROAM_SUPPORT) += -DWMI_ROAM_SUPPORT
ccflags-$(CONFIG_WMI_CONCURRENCY_SUPPORT) += -DWMI_CONCURRENCY_SUPPORT
ccflags-$(CONFIG_WMI_STA_SUPPORT) += -DWMI_STA_SUPPORT

ifdef CONFIG_HIF_LARGE_CE_RING_HISTORY
ccflags-y += -DHIF_CE_HISTORY_MAX=$(CONFIG_HIF_LARGE_CE_RING_HISTORY)
endif

ccflags-$(CONFIG_WLAN_HANG_EVENT) += -DHIF_CE_LOG_INFO
ccflags-$(CONFIG_WLAN_HANG_EVENT) += -DHIF_BUS_LOG_INFO
ccflags-$(CONFIG_WLAN_HANG_EVENT) += -DDP_SUPPORT_RECOVERY_NOTIFY

ccflags-$(CONFIG_ENABLE_SIZE_OPTIMIZE) += -Os

# DFS component
ccflags-$(CONFIG_WLAN_DFS_STATIC_MEM_ALLOC) += -DWLAN_DFS_STATIC_MEM_ALLOC
ccflags-$(CONFIG_WLAN_DFS_MASTER_ENABLE) += -DMOBILE_DFS_SUPPORT
ifeq ($(CONFIG_WLAN_FEATURE_DFS_OFFLOAD), y)
ccflags-$(CONFIG_WLAN_DFS_MASTER_ENABLE) += -DWLAN_DFS_FULL_OFFLOAD
else
ccflags-$(CONFIG_WLAN_DFS_MASTER_ENABLE) += -DWLAN_DFS_PARTIAL_OFFLOAD
endif
ccflags-$(CONFIG_WLAN_DFS_MASTER_ENABLE) += -DDFS_COMPONENT_ENABLE
ccflags-$(CONFIG_WLAN_DFS_MASTER_ENABLE) += -DQCA_DFS_USE_POLICY_MANAGER
ccflags-$(CONFIG_WLAN_DFS_MASTER_ENABLE) += -DQCA_DFS_NOL_PLATFORM_DRV_SUPPORT
ccflags-$(CONFIG_QCA_DFS_BW_PUNCTURE) += -DQCA_DFS_BW_PUNCTURE

ccflags-$(CONFIG_WLAN_DEBUGFS) += -DWLAN_DEBUGFS
ccflags-$(CONFIG_WLAN_DEBUGFS) += -DWLAN_DBGLOG_DEBUGFS
ccflags-$(CONFIG_WLAN_STREAMFS) += -DWLAN_STREAMFS

ccflags-$(CONFIG_DYNAMIC_DEBUG) += -DFEATURE_MULTICAST_HOST_FW_MSGS

ccflags-$(CONFIG_ENABLE_SMMU_S1_TRANSLATION) += -DENABLE_SMMU_S1_TRANSLATION

#Flag to enable/disable Line number logging
ccflags-$(CONFIG_LOG_LINE_NUMBER) += -DLOG_LINE_NUMBER

#Flag to enable/disable MTRACE feature
ccflags-$(CONFIG_ENABLE_MTRACE_LOG) += -DENABLE_MTRACE_LOG

ccflags-$(CONFIG_FUNC_CALL_MAP) += -DFUNC_CALL_MAP

#Flag to enable/disable Adaptive 11r feature
ccflags-$(CONFIG_ADAPTIVE_11R) += -DWLAN_ADAPTIVE_11R

#Flag to enable/disable sae single pmk feature feature
ccflags-$(CONFIG_SAE_SINGLE_PMK) += -DWLAN_SAE_SINGLE_PMK

#Flag to enable/disable multi client low latency feature support
ccflags-$(CONFIG_MULTI_CLIENT_LL_SUPPORT) += -DMULTI_CLIENT_LL_SUPPORT

#Flag to enable/disable vendor handoff control feature support
ccflags-$(CONFIG_WLAN_VENDOR_HANDOFF_CONTROL) += -DWLAN_VENDOR_HANDOFF_CONTROL

#Flag to enable/disable mscs feature
ccflags-$(CONFIG_FEATURE_MSCS) += -DWLAN_FEATURE_MSCS

#Flag to enable NUD tracking
ccflags-$(CONFIG_WLAN_NUD_TRACKING) += -DWLAN_NUD_TRACKING

#Flag to enable set and get disable channel list feature
ccflags-$(CONFIG_DISABLE_CHANNEL_LIST) += -DDISABLE_CHANNEL_LIST

#Flag to enable/disable WIPS feature
ccflags-$(CONFIG_WLAN_BCN_RECV_FEATURE) += -DWLAN_BCN_RECV_FEATURE

#Flag to enable/disable thermal mitigation
ccflags-$(CONFIG_FW_THERMAL_THROTTLE) += -DFW_THERMAL_THROTTLE

#Flag to enable/disable LTE COEX support
ccflags-$(CONFIG_LTE_COEX) += -DLTE_COEX

#Flag to enable/disable HOST_OPCLASS
ccflags-$(CONFIG_HOST_OPCLASS) += -DHOST_OPCLASS
ccflags-$(CONFIG_HOST_OPCLASS) += -DHOST_OPCLASS_EXT

#Flag to enable/disable TARGET_11D_SCAN
ccflags-$(CONFIG_TARGET_11D_SCAN) += -DTARGET_11D_SCAN

#Flag to enable/disable avoid acs frequency list feature
ccflags-$(CONFIG_SAP_AVOID_ACS_FREQ_LIST) += -DSAP_AVOID_ACS_FREQ_LIST

#Flag to enable Dynamic Voltage WDCVS (Config Voltage Mode)
ccflags-$(CONFIG_WLAN_DYNAMIC_CVM) += -DFEATURE_WLAN_DYNAMIC_CVM

#Flag to enable get firmware state feature
ccflags-$(CONFIG_QCACLD_FEATURE_FW_STATE) += -DFEATURE_FW_STATE

#Flag to enable set coex configuration feature
ccflags-$(CONFIG_QCACLD_FEATURE_COEX_CONFIG) += -DFEATURE_COEX_CONFIG

#Flag to enable MPTA helper feature
ccflags-$(CONFIG_QCACLD_FEATURE_MPTA_HELPER) += -DFEATURE_MPTA_HELPER

#Flag to enable get hw capability
ccflags-$(CONFIG_QCACLD_FEATURE_HW_CAPABILITY) += -DFEATURE_HW_CAPABILITY

#Flag to enable set btc chain mode feature
ccflags-$(CONFIG_QCACLD_FEATURE_BTC_CHAIN_MODE) += -DFEATURE_BTC_CHAIN_MODE

ccflags-$(CONFIG_DATA_CE_SW_INDEX_NO_INLINE_UPDATE) += -DDATA_CE_SW_INDEX_NO_INLINE_UPDATE

#Flag to enable Multi page memory allocation for RX descriptor pool
ccflags-$(CONFIG_QCACLD_RX_DESC_MULTI_PAGE_ALLOC) += -DRX_DESC_MULTI_PAGE_ALLOC

#Flag to enable SAR Safety Feature
ccflags-$(CONFIG_SAR_SAFETY_FEATURE) += -DSAR_SAFETY_FEATURE

ccflags-$(CONFIG_CONNECTION_ROAMING_CFG) += -DCONNECTION_ROAMING_CFG
ccflags-$(CONFIG_FEATURE_SET) += -DFEATURE_SET
ifeq ($(CONFIG_WLAN_FEATURE_LL_LT_SAP), y)
ccflags-y += -DWLAN_FEATURE_LL_LT_SAP
ccflags-y += -DWLAN_FEATURE_VDEV_DCS
endif

ccflags-$(CONFIG_WLAN_FEATURE_NEAR_FULL_IRQ) += -DWLAN_FEATURE_NEAR_FULL_IRQ
ccflags-$(CONFIG_WLAN_FEATURE_DP_EVENT_HISTORY) += -DWLAN_FEATURE_DP_EVENT_HISTORY
ccflags-$(CONFIG_WLAN_FEATURE_DP_RX_RING_HISTORY) += -DWLAN_FEATURE_DP_RX_RING_HISTORY
ccflags-$(CONFIG_WLAN_FEATURE_DP_MON_STATUS_RING_HISTORY) += -DWLAN_FEATURE_DP_MON_STATUS_RING_HISTORY
ccflags-$(CONFIG_WLAN_FEATURE_DP_TX_DESC_HISTORY) += -DWLAN_FEATURE_DP_TX_DESC_HISTORY
ccflags-$(CONFIG_REO_QDESC_HISTORY) += -DREO_QDESC_HISTORY
ccflags-$(CONFIG_DP_TX_HW_DESC_HISTORY) += -DDP_TX_HW_DESC_HISTORY
ifdef CONFIG_QDF_NBUF_HISTORY_SIZE
ccflags-y += -DQDF_NBUF_HISTORY_SIZE=$(CONFIG_QDF_NBUF_HISTORY_SIZE)
endif
ccflags-$(CONFIG_WLAN_DP_PER_RING_TYPE_CONFIG) += -DWLAN_DP_PER_RING_TYPE_CONFIG
ccflags-$(CONFIG_WLAN_CE_INTERRUPT_THRESHOLD_CONFIG) += -DWLAN_CE_INTERRUPT_THRESHOLD_CONFIG
ccflags-$(CONFIG_SAP_DHCP_FW_IND) += -DSAP_DHCP_FW_IND
ccflags-$(CONFIG_WLAN_DP_PENDING_MEM_FLUSH) += -DWLAN_DP_PENDING_MEM_FLUSH
ccflags-$(CONFIG_WLAN_SUPPORT_DATA_STALL) += -DWLAN_SUPPORT_DATA_STALL
ccflags-$(CONFIG_WLAN_SUPPORT_TXRX_HL_BUNDLE) += -DWLAN_SUPPORT_TXRX_HL_BUNDLE
ccflags-$(CONFIG_QCN7605_PCIE_SHADOW_REG_SUPPORT) += -DQCN7605_PCIE_SHADOW_REG_SUPPORT
ccflags-$(CONFIG_QCN7605_PCIE_GOLBAL_RESET_SUPPORT) += -DQCN7605_PCIE_GOLBAL_RESET_SUPPORT
ccflags-$(CONFIG_MARK_ICMP_REQ_TO_FW) += -DWLAN_DP_FEATURE_MARK_ICMP_REQ_TO_FW
ccflags-$(CONFIG_EMULATION_2_0) += -DCONFIG_KIWI_EMULATION_2_0
ccflags-$(CONFIG_WORD_BASED_TLV) += -DCONFIG_WORD_BASED_TLV
ccflags-$(CONFIG_4_BYTES_TLV_TAG) += -DCONFIG_4_BYTES_TLV_TAG
ccflags-$(CONFIG_WLAN_SKIP_BAR_UPDATE) += -DWLAN_SKIP_BAR_UPDATE
ccflags-$(CONFIG_WLAN_TRACEPOINTS) += -DWLAN_TRACEPOINTS

ccflags-$(CONFIG_QCACLD_FEATURE_SON) += -DFEATURE_PERPKT_INFO
ccflags-$(CONFIG_QCACLD_FEATURE_SON) += -DQCA_ENHANCED_STATS_SUPPORT
ccflags-$(CONFIG_WLAN_FEATURE_CE_RX_BUFFER_REUSE) += -DWLAN_FEATURE_CE_RX_BUFFER_REUSE

CONFIG_NUM_SOC_PERF_CLUSTER ?= 1
ccflags-y += -DNUM_SOC_PERF_CLUSTER=$(CONFIG_NUM_SOC_PERF_CLUSTER)

ifeq ($(CONFIG_QMI_COMPONENT_ENABLE), y)
ccflags-y += -DQMI_COMPONENT_ENABLE
ifeq ($(CONFIG_QMI_WFDS), y)
ccflags-y += -DQMI_WFDS
endif
endif

ifdef CONFIG_MAX_LOGS_PER_SEC
ccflags-y += -DWLAN_MAX_LOGS_PER_SEC=$(CONFIG_MAX_LOGS_PER_SEC)
endif

ifeq ($(CONFIG_NON_QC_PLATFORM), y)
ccflags-y += -DWLAN_DUMP_LOG_BUF_CNT=$(CONFIG_DUMP_LOG_BUF_CNT)
endif

ifdef CONFIG_SCHED_HISTORY_SIZE
ccflags-y += -DWLAN_SCHED_HISTORY_SIZE=$(CONFIG_SCHED_HISTORY_SIZE)
endif

ifdef CONFIG_QDF_TIMER_MULTIPLIER_FRAC
ccflags-y += -DQDF_TIMER_MULTIPLIER_FRAC=$(CONFIG_QDF_TIMER_MULTIPLIER_FRAC)
endif

ifdef CONFIG_DP_LEGACY_MODE_CSM_DEFAULT_DISABLE
ccflags-y += -DDP_LEGACY_MODE_CSM_DEFAULT_DISABLE=$(CONFIG_DP_LEGACY_MODE_CSM_DEFAULT_DISABLE)
endif

ifdef CONFIG_HANDLE_RX_REROUTE_ERR
ccflags-y += -DHANDLE_RX_REROUTE_ERR
endif

# configure log buffer size
ifdef CONFIG_CFG_NUM_DP_TRACE_RECORD
ccflags-y += -DMAX_QDF_DP_TRACE_RECORDS=$(CONFIG_CFG_NUM_DP_TRACE_RECORD)
endif

ifdef CONFIG_CFG_NUM_HTC_CREDIT_HISTORY
ccflags-y += -DHTC_CREDIT_HISTORY_MAX=$(CONFIG_CFG_NUM_HTC_CREDIT_HISTORY)
endif

ifdef CONFIG_CFG_NUM_WMI_EVENT_HISTORY
ccflags-y += -DWMI_EVENT_DEBUG_MAX_ENTRY=$(CONFIG_CFG_NUM_WMI_EVENT_HISTORY)
endif

ifdef CONFIG_CFG_NUM_WMI_MGMT_EVENT_HISTORY
ccflags-y += -DWMI_MGMT_EVENT_DEBUG_MAX_ENTRY=$(CONFIG_CFG_NUM_WMI_MGMT_EVENT_HISTORY)
endif

ifdef CONFIG_CFG_NUM_TX_RX_HISTOGRAM
ccflags-y += -DNUM_TX_RX_HISTOGRAM=$(CONFIG_CFG_NUM_TX_RX_HISTOGRAM)
endif

ifdef CONFIG_CFG_NUM_RX_IND_RECORD
ccflags-y += -DOL_RX_INDICATION_MAX_RECORDS=$(CONFIG_CFG_NUM_RX_IND_RECORD)
endif

ifdef CONFIG_CFG_NUM_ROAM_DEBUG_RECORD
ccflags-y += -DWLAN_ROAM_DEBUG_MAX_REC=$(CONFIG_CFG_NUM_ROAM_DEBUG_RECORD)
endif

ifdef CONFIG_CFG_PMO_WOW_FILTERS_MAX
ccflags-y += -DPMO_WOW_FILTERS_MAX=$(CONFIG_CFG_PMO_WOW_FILTERS_MAX)
endif

ifdef CONFIG_CFG_GTK_OFFLOAD_MAX_VDEV
ccflags-y += -DCFG_TGT_DEFAULT_GTK_OFFLOAD_MAX_VDEV=$(CONFIG_CFG_GTK_OFFLOAD_MAX_VDEV)
endif

ifdef CONFIG_TGT_NUM_MSDU_DESC
ccflags-y += -DCFG_TGT_NUM_MSDU_DESC=$(CONFIG_TGT_NUM_MSDU_DESC)
endif

ifdef CONFIG_HTC_MAX_MSG_PER_BUNDLE_TX
ccflags-y += -DCFG_HTC_MAX_MSG_PER_BUNDLE_TX=$(CONFIG_HTC_MAX_MSG_PER_BUNDLE_TX)
endif

ifdef CONFIG_CFG_BMISS_OFFLOAD_MAX_VDEV
ccflags-y += -DCFG_TGT_DEFAULT_BMISS_OFFLOAD_MAX_VDEV=$(CONFIG_CFG_BMISS_OFFLOAD_MAX_VDEV)
endif

ifdef CONFIG_WLAN_UMAC_MLO_MAX_DEV
ccflags-y += -DWLAN_UMAC_MLO_MAX_DEV=$(CONFIG_WLAN_UMAC_MLO_MAX_DEV)
endif

ifdef CONFIG_CFG_ROAM_OFFLOAD_MAX_VDEV
ccflags-y += -DCFG_TGT_DEFAULT_ROAM_OFFLOAD_MAX_VDEV=$(CONFIG_CFG_ROAM_OFFLOAD_MAX_VDEV)
endif

ifdef CONFIG_CFG_MAX_PERIODIC_TX_PTRNS
ccflags-y += -DMAXNUM_PERIODIC_TX_PTRNS=$(CONFIG_CFG_MAX_PERIODIC_TX_PTRNS)
endif

ifdef CONFIG_CFG_MAX_STA_VDEVS
ccflags-y += -DCFG_TGT_DEFAULT_MAX_STA_VDEVS=$(CONFIG_CFG_MAX_STA_VDEVS)
endif

ifdef CONFIG_CFG_NUM_OF_ADDITIONAL_FW_PEERS
ccflags-y += -DNUM_OF_ADDITIONAL_FW_PEERS=$(CONFIG_CFG_NUM_OF_ADDITIONAL_FW_PEERS)
endif

ifdef CONFIG_CFG_NUM_OF_TDLS_CONN_TABLE_ENTRIES
ccflags-y += -DCFG_TGT_NUM_TDLS_CONN_TABLE_ENTRIES=$(CONFIG_CFG_NUM_OF_TDLS_CONN_TABLE_ENTRIES)
endif

ifdef CONFIG_CFG_TGT_AST_SKID_LIMIT
ccflags-y += -DCFG_TGT_AST_SKID_LIMIT=$(CONFIG_CFG_TGT_AST_SKID_LIMIT)
endif

ifdef CONFIG_TX_RESOURCE_HIGH_TH_IN_PER
ccflags-y += -DTX_RESOURCE_HIGH_TH_IN_PER=$(CONFIG_TX_RESOURCE_HIGH_TH_IN_PER)
endif

ifdef CONFIG_TX_RESOURCE_LOW_TH_IN_PER
ccflags-y += -DTX_RESOURCE_LOW_TH_IN_PER=$(CONFIG_TX_RESOURCE_LOW_TH_IN_PER)
endif

CONFIG_WLAN_MAX_PSOCS ?= 1
ccflags-y += -DWLAN_MAX_PSOCS=$(CONFIG_WLAN_MAX_PSOCS)

CONFIG_WLAN_MAX_PDEVS ?= 1
ccflags-y += -DWLAN_MAX_PDEVS=$(CONFIG_WLAN_MAX_PDEVS)

ifeq ($(CONFIG_WLAN_FEATURE_11BE_MLO), y)
CONFIG_WLAN_MAX_ML_VDEVS ?= 3
else
CONFIG_WLAN_MAX_ML_VDEVS ?= 0
endif
ccflags-y += -DWLAN_MAX_ML_VDEVS=$(CONFIG_WLAN_MAX_ML_VDEVS)

CONFIG_WLAN_MAX_VDEVS ?= 6
ccflags-y += -DWLAN_MAX_VDEVS=$(CONFIG_WLAN_MAX_VDEVS)

ifdef CONFIG_WLAN_FEATURE_11BE_MLO
CONFIG_WLAN_MAX_MLD ?= 2
else
CONFIG_WLAN_MAX_MLD ?= 1
endif
ccflags-y += -DWLAN_MAX_MLD=$(CONFIG_WLAN_MAX_MLD)

ifdef CONFIG_WLAN_FEATURE_11BE_MLO
CONFIG_WLAN_MAX_ML_DEFAULT_LINK ?= 2
else
CONFIG_WLAN_MAX_ML_DEFAULT_LINK  ?= 1
endif
ccflags-y += -DWLAN_MAX_ML_DEFAULT_LINK=$(CONFIG_WLAN_MAX_ML_DEFAULT_LINK)

ifdef CONFIG_WLAN_FEATURE_11BE_MLO
ifndef CONFIG_WLAN_DEFAULT_REC_LINK_VALUE
CONFIG_WLAN_DEFAULT_REC_LINK_VALUE  ?= 2
endif
else
ifndef CONFIG_WLAN_DEFAULT_REC_LINK_VALUE
CONFIG_WLAN_DEFAULT_REC_LINK_VALUE  ?= 2
endif
endif
ccflags-y += -DWLAN_DEFAULT_REC_LINK_VALUE=$(CONFIG_WLAN_DEFAULT_REC_LINK_VALUE)

ifdef CONFIG_WLAN_FEATURE_11BE_MLO
CONFIG_WLAN_MAX_ML_BSS_LINKS ?= 3
else
CONFIG_WLAN_MAX_ML_BSS_LINKS ?= 1
endif
ccflags-y += -DWLAN_MAX_ML_BSS_LINKS=$(CONFIG_WLAN_MAX_ML_BSS_LINKS)

ifdef CONFIG_WLAN_FEATURE_EMLSR
CONFIG_WLAN_EMLSR_ENABLE ?= 1
else
CONFIG_WLAN_EMLSR_ENABLE ?= 0
endif
ccflags-y += -DWLAN_EMLSR_ENABLE=$(CONFIG_WLAN_EMLSR_ENABLE)

#Maximum pending commands for a vdev is calculated in vdev create handler
#by WLAN_SER_MAX_PENDING_CMDS/WLAN_SER_MAX_VDEVS. For SAP case, we will need
#to accommodate 32 Pending commands to handle multiple STA sending
#deauth/disassoc at the same time and for STA vdev,4 non scan pending commands
#are supported. So calculate WLAN_SER_MAX_PENDING_COMMANDS based on the SAP
#modes supported and no of STA vdev total non scan pending queue. Reserve
#additional 3 pending commands for WLAN_SER_MAX_PENDING_CMDS_AP to account for
#other commands like hardware mode change.

ifdef CONFIG_SIR_SAP_MAX_NUM_PEERS
CONFIG_WLAN_SER_MAX_PENDING_CMDS_AP ?=$(CONFIG_SIR_SAP_MAX_NUM_PEERS)
else
CONFIG_WLAN_SER_MAX_PENDING_CMDS_AP ?=32
endif
ccflags-y += -DWLAN_SER_MAX_PENDING_CMDS_AP=$(CONFIG_WLAN_SER_MAX_PENDING_CMDS_AP)+3

CONFIG_WLAN_SER_MAX_PENDING_CMDS_STA ?= 4
ccflags-y += -DWLAN_SER_MAX_PENDING_CMDS_STA=$(CONFIG_WLAN_SER_MAX_PENDING_CMDS_STA)

CONFIG_WLAN_MAX_PENDING_CMDS ?= $(CONFIG_WLAN_SER_MAX_PENDING_CMDS_AP)*3+$(CONFIG_WLAN_SER_MAX_PENDING_CMDS_STA)*2

ccflags-y += -DWLAN_SER_MAX_PENDING_CMDS=$(CONFIG_WLAN_MAX_PENDING_CMDS)

CONFIG_WLAN_PDEV_MAX_VDEVS ?= $(CONFIG_WLAN_MAX_VDEVS)
ccflags-y += -DWLAN_PDEV_MAX_VDEVS=$(CONFIG_WLAN_PDEV_MAX_VDEVS)

CONFIG_WLAN_PSOC_MAX_VDEVS ?= $(CONFIG_WLAN_MAX_VDEVS)
ccflags-y += -DWLAN_PSOC_MAX_VDEVS=$(CONFIG_WLAN_PSOC_MAX_VDEVS)

CONFIG_MAX_SCAN_CACHE_SIZE ?= 500
ccflags-y += -DMAX_SCAN_CACHE_SIZE=$(CONFIG_MAX_SCAN_CACHE_SIZE)
CONFIG_SCAN_MAX_REST_TIME ?= 0
ccflags-y += -DSCAN_MAX_REST_TIME=$(CONFIG_SCAN_MAX_REST_TIME)
CONFIG_SCAN_MIN_REST_TIME ?= 0
ccflags-y += -DSCAN_MIN_REST_TIME=$(CONFIG_SCAN_MIN_REST_TIME)
CONFIG_SCAN_BURST_DURATION ?= 0
ccflags-y += -DSCAN_BURST_DURATION=$(CONFIG_SCAN_BURST_DURATION)
CONFIG_SCAN_PROBE_SPACING_TIME ?= 0
ccflags-y += -DSCAN_PROBE_SPACING_TIME=$(CONFIG_SCAN_PROBE_SPACING_TIME)
CONFIG_SCAN_PROBE_DELAY ?= 0
ccflags-y += -DSCAN_PROBE_DELAY=$(CONFIG_SCAN_PROBE_DELAY)
CONFIG_SCAN_MAX_SCAN_TIME ?= 30000
ccflags-y += -DSCAN_MAX_SCAN_TIME=$(CONFIG_SCAN_MAX_SCAN_TIME)
CONFIG_SCAN_NETWORK_IDLE_TIMEOUT ?= 0
ccflags-y += -DSCAN_NETWORK_IDLE_TIMEOUT=$(CONFIG_SCAN_NETWORK_IDLE_TIMEOUT)
CONFIG_HIDDEN_SSID_TIME ?= 0xFFFFFFFF
ccflags-y += -DHIDDEN_SSID_TIME=$(CONFIG_HIDDEN_SSID_TIME)
CONFIG_SCAN_CHAN_STATS_EVENT_ENAB ?= false
ccflags-y += -DSCAN_CHAN_STATS_EVENT_ENAB=$(CONFIG_SCAN_CHAN_STATS_EVENT_ENAB)
CONFIG_MAX_BCN_PROBE_IN_SCAN_QUEUE ?= 150
ccflags-y += -DMAX_BCN_PROBE_IN_SCAN_QUEUE=$(CONFIG_MAX_BCN_PROBE_IN_SCAN_QUEUE)

#CONFIG_RX_DIAG_WQ_MAX_SIZE maximum number FW diag events that can be queued in
#FW diag events work queue. Host driver will discard the all diag events after
#this limit is reached.
#
# Value 0 represents no limit and any non zero value represents the maximum
# size of the work queue.
CONFIG_RX_DIAG_WQ_MAX_SIZE ?= 1000
ccflags-y += -DRX_DIAG_WQ_MAX_SIZE=$(CONFIG_RX_DIAG_WQ_MAX_SIZE)

CONFIG_MGMT_DESC_POOL_MAX ?= 64
ccflags-y += -DMGMT_DESC_POOL_MAX=$(CONFIG_MGMT_DESC_POOL_MAX)

ifdef CONFIG_SIR_SAP_MAX_NUM_PEERS
ccflags-y += -DSIR_SAP_MAX_NUM_PEERS=$(CONFIG_SIR_SAP_MAX_NUM_PEERS)
endif

ifdef CONFIG_BEACON_TX_OFFLOAD_MAX_VDEV
ccflags-y += -DCFG_TGT_DEFAULT_BEACON_TX_OFFLOAD_MAX_VDEV=$(CONFIG_BEACON_TX_OFFLOAD_MAX_VDEV)
endif

ifdef CONFIG_LIMIT_IPA_TX_BUFFER
ccflags-y += -DLIMIT_IPA_TX_BUFFER=$(CONFIG_LIMIT_IPA_TX_BUFFER)
endif

ifdef CONFIG_LOCK_STATS_ON
ccflags-y += -DQDF_LOCK_STATS=1
ccflags-y += -DQDF_LOCK_STATS_DESTROY_PRINT=0
ifneq ($(CONFIG_ARCH_SDXPRAIRIE), y)
ccflags-y += -DQDF_LOCK_STATS_BUG_ON=1
endif
ccflags-$(CONFIG_VCPU_TIMESTOLEN) += -DVCPU_TIMESTOLEN
ccflags-y += -DQDF_LOCK_STATS_LIST=1
ccflags-y += -DQDF_LOCK_STATS_LIST_SIZE=256
endif

ifdef CONFIG_FW_THERMAL_THROTTLE
ccflags-y += -DFW_THERMAL_THROTTLE_SUPPORT
endif

ccflags-$(CONFIG_FEATURE_RX_LINKSPEED_ROAM_TRIGGER) += -DFEATURE_RX_LINKSPEED_ROAM_TRIGGER
#DP_RATETABLE_SUPPORT is enabled when CONFIG_FEATURE_RX_LINKSPEED_ROAM_TRIGGER is enabled
ccflags-$(CONFIG_FEATURE_RX_LINKSPEED_ROAM_TRIGGER) += -DDP_RATETABLE_SUPPORT

ccflags-$(CONFIG_BAND_6GHZ) += -DCONFIG_BAND_6GHZ
ccflags-$(CONFIG_6G_SCAN_CHAN_SORT_ALGO) += -DFEATURE_6G_SCAN_CHAN_SORT_ALGO
ccflags-$(CONFIG_AFC_SUPPORT) += -DCONFIG_AFC_SUPPORT
ccflags-$(CONFIG_WLAN_FEATURE_AFC_DCS_SKIP_ACS_RANGE) += -DWLAN_FEATURE_AFC_DCS_SKIP_ACS_RANGE

ccflags-$(CONFIG_RX_FISA) += -DWLAN_SUPPORT_RX_FISA
ccflags-$(CONFIG_RX_FISA_HISTORY) += -DWLAN_SUPPORT_RX_FISA_HIST

ccflags-$(CONFIG_DP_SWLM) += -DWLAN_DP_FEATURE_SW_LATENCY_MGR

ccflags-$(CONFIG_RX_DEFRAG_DO_NOT_REINJECT) += -DRX_DEFRAG_DO_NOT_REINJECT

ccflags-$(CONFIG_HANDLE_BC_EAP_TX_FRM) += -DHANDLE_BROADCAST_EAPOL_TX_FRAME

ccflags-$(CONFIG_MORE_TX_DESC) += -DTX_TO_NPEERS_INC_TX_DESCS

ccflags-$(CONFIG_HASTINGS_BT_WAR) += -DHASTINGS_BT_WAR

ccflags-$(CONFIG_HIF_DEBUG) += -DHIF_CONFIG_SLUB_DEBUG_ON
ccflags-$(CONFIG_HAL_DEBUG) += -DHAL_CONFIG_SLUB_DEBUG_ON

ccflags-$(CONFIG_FOURTH_CONNECTION) += -DFEATURE_FOURTH_CONNECTION
ccflags-$(CONFIG_FOURTH_CONNECTION_AUTO) += -DFOURTH_CONNECTION_AUTO
ccflags-$(CONFIG_WMI_SEND_RECV_QMI) += -DWLAN_FEATURE_WMI_SEND_RECV_QMI

ccflags-$(CONFIG_WDI3_STATS_UPDATE) += -DWDI3_STATS_UPDATE
ccflags-$(CONFIG_WDI3_STATS_BW_MONITOR) += -DWDI3_STATS_BW_MONITOR

ccflags-$(CONFIG_IPA_P2P_SUPPORT) += -DIPA_P2P_SUPPORT

ccflags-$(CONFIG_WLAN_CUSTOM_DSCP_UP_MAP) += -DWLAN_CUSTOM_DSCP_UP_MAP
ccflags-$(CONFIG_WLAN_SEND_DSCP_UP_MAP_TO_FW) += -DWLAN_SEND_DSCP_UP_MAP_TO_FW

ccflags-$(CONFIG_SMMU_S1_UNMAP) += -DCONFIG_SMMU_S1_UNMAP
ccflags-$(CONFIG_HIF_CPU_PERF_AFFINE_MASK) += -DHIF_CPU_PERF_AFFINE_MASK
ccflags-$(CONFIG_HIF_CPU_CLEAR_AFFINITY) += -DHIF_CPU_CLEAR_AFFINITY

ccflags-$(CONFIG_GENERIC_SHADOW_REGISTER_ACCESS_ENABLE) += -DGENERIC_SHADOW_REGISTER_ACCESS_ENABLE
ccflags-$(CONFIG_IPA_SET_RESET_TX_DB_PA) += -DIPA_SET_RESET_TX_DB_PA
ccflags-$(CONFIG_DEVICE_FORCE_WAKE_ENABLE) += -DDEVICE_FORCE_WAKE_ENABLE
ccflags-$(CONFIG_WINDOW_REG_PLD_LOCK_ENABLE) += -DWINDOW_REG_PLD_LOCK_ENABLE
ccflags-$(CONFIG_DUMP_REO_QUEUE_INFO_IN_DDR) += -DDUMP_REO_QUEUE_INFO_IN_DDR
ccflags-$(CONFIG_DP_RX_REFILL_CPU_PERF_AFFINE_MASK) += -DDP_RX_REFILL_CPU_PERF_AFFINE_MASK
ccflags-$(CONFIG_WLAN_FEATURE_AFFINITY_MGR) += -DWLAN_FEATURE_AFFINITY_MGR
ccflags-$(CONFIG_FEATURE_ENABLE_CE_DP_IRQ_AFFINE) += -DFEATURE_ENABLE_CE_DP_IRQ_AFFINE
ccflags-$(CONFIG_WLAN_SUPPORT_SERVICE_CLASS) += -DWLAN_SUPPORT_SERVICE_CLASS
ccflags-$(CONFIG_WLAN_SUPPORT_FLOW_PRIORTIZATION) += -DWLAN_SUPPORT_FLOW_PRIORTIZATION
ccflags-$(CONFIG_WLAN_SUPPORT_LAPB) += -DWLAN_SUPPORT_LAPB
found = $(shell if grep -qF "walt_get_cpus_taken" $(srctree)/kernel/sched/walt/walt.c; then echo "yes" ;else echo "no" ;fi;)
ifeq ($(findstring yes, $(found)), yes)
ccflags-y += -DWALT_GET_CPU_TAKEN_SUPPORT
endif

ifdef CONFIG_MAX_CLIENTS_ALLOWED
ccflags-y += -DWLAN_MAX_CLIENTS_ALLOWED=$(CONFIG_MAX_CLIENTS_ALLOWED)
endif

ifeq ($(CONFIG_WLAN_FEATURE_RX_BUFFER_POOL), y)
ccflags-y += -DWLAN_FEATURE_RX_PREALLOC_BUFFER_POOL
ifdef CONFIG_DP_RX_BUFFER_POOL_SIZE
ccflags-y += -DDP_RX_BUFFER_POOL_SIZE=$(CONFIG_DP_RX_BUFFER_POOL_SIZE)
endif
ifdef CONFIG_DP_RX_BUFFER_POOL_ALLOC_THRES
ccflags-y += -DDP_RX_BUFFER_POOL_ALLOC_THRES=$(CONFIG_DP_RX_BUFFER_POOL_ALLOC_THRES)
endif
ifdef CONFIG_DP_RX_REFILL_BUFF_POOL_SIZE
ccflags-y += -DDP_RX_REFILL_BUFF_POOL_SIZE=$(CONFIG_DP_RX_REFILL_BUFF_POOL_SIZE)
endif
ifdef CONFIG_DP_RX_REFILL_THRD_THRESHOLD
ccflags-y += -DDP_RX_REFILL_THRD_THRESHOLD=$(CONFIG_DP_RX_REFILL_THRD_THRESHOLD)
endif
endif

ccflags-$(CONFIG_DP_FT_LOCK_HISTORY) += -DDP_FT_LOCK_HISTORY

ccflags-$(CONFIG_INTRA_BSS_FWD_OFFLOAD) += -DINTRA_BSS_FWD_OFFLOAD
ccflags-$(CONFIG_GET_DRIVER_MODE) += -DFEATURE_GET_DRIVER_MODE

ifeq ($(CONFIG_FEATURE_IPA_PIPE_CHANGE_WDI1), y)
ccflags-y += -DFEATURE_IPA_PIPE_CHANGE_WDI1
endif

ccflags-$(CONFIG_WLAN_BOOTUP_MARKER) += -DWLAN_BOOTUP_MARKER
ifdef CONFIG_WLAN_PLACEMARKER_PREFIX
ccflags-y += -DWLAN_PLACEMARKER_PREFIX=\"$(CONFIG_WLAN_PLACEMARKER_PREFIX)\"
endif

ccflags-$(CONFIG_FEATURE_STA_MODE_VOTE_LINK) += -DFEATURE_STA_MODE_VOTE_LINK
ccflags-$(CONFIG_WLAN_ENABLE_GPIO_WAKEUP) += -DWLAN_ENABLE_GPIO_WAKEUP
ccflags-$(CONFIG_WLAN_MAC_ADDR_UPDATE_DISABLE) += -DWLAN_MAC_ADDR_UPDATE_DISABLE

ifeq ($(CONFIG_SMP), y)
ifeq ($(CONFIG_HIF_DETECTION_LATENCY_ENABLE), y)
ccflags-y += -DHIF_DETECTION_LATENCY_ENABLE
ccflags-y += -DDETECTION_TIMER_TIMEOUT=4000
ccflags-y += -DDETECTION_LATENCY_THRESHOLD=3900
endif
endif

#Flags to enable/disable WDS specific features
ccflags-$(CONFIG_FEATURE_WDS) += -DFEATURE_WDS
ccflags-$(CONFIG_FEATURE_MEC) += -DFEATURE_MEC
ccflags-$(CONFIG_FEATURE_MCL_REPEATER) += -DFEATURE_MCL_REPEATER
ccflags-$(CONFIG_WDS_CONV_TARGET_IF_OPS_ENABLE) += -DWDS_CONV_TARGET_IF_OPS_ENABLE
ccflags-$(CONFIG_BYPASS_WDS_OL_OPS) += -DBYPASS_OL_OPS

ccflags-$(CONFIG_IPA_WDI3_TX_TWO_PIPES) += -DIPA_WDI3_TX_TWO_PIPES

ccflags-$(CONFIG_DP_TX_TRACKING) += -DDP_TX_TRACKING

ifdef CONFIG_CHIP_VERSION
ccflags-y += -DCHIP_VERSION=$(CONFIG_CHIP_VERSION)
endif

ccflags-$(CONFIG_WLAN_FEATURE_MARK_FIRST_WAKEUP_PACKET) += -DWLAN_FEATURE_MARK_FIRST_WAKEUP_PACKET

ccflags-$(CONFIG_SHUTDOWN_WLAN_IN_SYSTEM_SUSPEND) += -DSHUTDOWN_WLAN_IN_SYSTEM_SUSPEND

ifeq ($(CONFIG_WLAN_FEATURE_MCC_QUOTA), y)
ccflags-y += -DWLAN_FEATURE_MCC_QUOTA
ifdef CONFIG_WLAN_MCC_MIN_CHANNEL_QUOTA
ccflags-y += -DWLAN_MCC_MIN_CHANNEL_QUOTA=$(CONFIG_WLAN_MCC_MIN_CHANNEL_QUOTA)
endif
endif

ccflags-$(CONFIG_WLAN_FEATURE_PEER_TXQ_FLUSH_CONF) += -DWLAN_FEATURE_PEER_TXQ_FLUSH_CONF

ifeq ($(CONFIG_DP_HW_TX_DELAY_STATS_ENABLE), y)
ccflags-y += -DHW_TX_DELAY_STATS_ENABLE
endif

# Config MAX SAP interface number
ifdef CONFIG_QDF_MAX_NO_OF_SAP_MODE
ccflags-y += -DQDF_MAX_NO_OF_SAP_MODE=$(CONFIG_QDF_MAX_NO_OF_SAP_MODE)
endif

#Flags to enable/disable Dynamic WLAN interface control feature
ifeq ($(CONFIG_CNSS_HW_SECURE_DISABLE), y)
ccflags-y += -DFEATURE_CNSS_HW_SECURE_DISABLE
endif

#DBAM feature needs COEX feature to be enabled
ifeq ($(CONFIG_FEATURE_COEX), y)
ccflags-$(CONFIG_WLAN_FEATURE_COEX_DBAM) += -DWLAN_FEATURE_DBAM_CONFIG
endif

# Flag to enable Constrained Application Protocol feature
ccflags-$(CONFIG_WLAN_FEATURE_COAP) += -DWLAN_FEATURE_COAP

# SSR driver dump config
ccflags-$(CONFIG_CNSS2_SSR_DRIVER_DUMP) += -DWLAN_FEATURE_SSR_DRIVER_DUMP

# Currently, for versions of gcc which support it, the kernel Makefile
# is disabling the maybe-uninitialized warning.  Re-enable it for the
# WLAN driver.  Note that we must use ccflags-y here so that it
# will override the kernel settings.
ifeq ($(call cc-option-yn, -Wmaybe-uninitialized), y)
ccflags-y += -Wmaybe-uninitialized
ifneq (y,$(CONFIG_ARCH_MSM))
ccflags-y += -Wframe-larger-than=4096
endif
endif
ccflags-y += -Wmissing-prototypes

ifeq ($(call cc-option-yn, -Wheader-guard), y)
ccflags-y += -Wheader-guard
endif
# If the module name is not "wlan", then the define MULTI_IF_NAME to be the
# same a the QCA CHIP name. The host driver will then append MULTI_IF_NAME to
# any string that must be unique for all instances of the driver on the system.
# This allows multiple instances of the driver with different module names.
# If the module name is wlan, leave MULTI_IF_NAME undefined and the code will
# treat the driver as the primary driver.
#
# If DYNAMIC_SINGLE_CHIP is defined and MULTI_IF_NAME is undefined, which means
# there are multiple possible drivers, but only 1 driver will be loaded at
# a time(WLAN dynamic detect), no matter what the module name is, then
# host driver will only append DYNAMIC_SINGLE_CHIP to the path of
# firmware/mac/ini file.
#
# If DYNAMIC_SINGLE_CHIP & MULTI_IF_NAME defined, which means there are
# multiple possible drivers, we also can load multiple drivers together.
# And we can use DYNAMIC_SINGLE_CHIP to distinguish the ko name, and use
# MULTI_IF_NAME to make cnss2 platform driver to figure out which wlanhost
# driver attached. Moreover, as the first priority, host driver will only
# append DYNAMIC_SINGLE_CHIP to the path of firmware/mac/ini file.

ifneq ($(DYNAMIC_SINGLE_CHIP),)
ccflags-y += -DDYNAMIC_SINGLE_CHIP=\"$(DYNAMIC_SINGLE_CHIP)\"
ifneq ($(MULTI_IF_NAME),)
ccflags-y += -DMULTI_IF_NAME=\"$(MULTI_IF_NAME)\"
endif
#
else

ifneq ($(MODNAME), wlan)
CHIP_NAME ?= $(MODNAME)
ccflags-y += -DMULTI_IF_NAME=\"$(CHIP_NAME)\"
endif

endif #DYNAMIC_SINGLE_CHIP

# WLAN_HDD_ADAPTER_MAGIC must be unique for all instances of the driver on the
# system. If it is not defined, then the host driver will use the first 4
# characters (including NULL) of MULTI_IF_NAME to construct
# WLAN_HDD_ADAPTER_MAGIC.
ifdef WLAN_HDD_ADAPTER_MAGIC
ccflags-y += -DWLAN_HDD_ADAPTER_MAGIC=$(WLAN_HDD_ADAPTER_MAGIC)
endif

# Determine if we are building against an arm architecture host
ifeq ($(findstring arm, $(ARCH)),)
	ccflags-y += -DWLAN_HOST_ARCH_ARM=0
else
	ccflags-y += -DWLAN_HOST_ARCH_ARM=1
endif

# Android wifi state control interface
ifneq ($(WLAN_CTRL_NAME),)
ccflags-y += -DWLAN_CTRL_NAME=\"$(WLAN_CTRL_NAME)\"
endif

# inject some build related information
ifeq ($(CONFIG_BUILD_TAG), y)
CLD_CHECKOUT = $(shell cd "$(WLAN_ROOT)" && \
	git reflog | grep -vm1 "}: cherry-pick: " | grep -oE ^[0-f]+)
CLD_IDS = $(shell cd "$(WLAN_ROOT)" && \
	git log -50 $(CLD_CHECKOUT)~..HEAD | \
		sed -nE 's/^\s*Change-Id: (I[0-f]{10})[0-f]{30}\s*$$/\1/p' | \
		paste -sd "," -)

CMN_CHECKOUT = $(shell cd "$(WLAN_COMMON_INC)" && \
	git reflog | grep -vm1 "}: cherry-pick: " | grep -oE ^[0-f]+)
CMN_IDS = $(shell cd "$(WLAN_COMMON_INC)" && \
	git log -50 $(CMN_CHECKOUT)~..HEAD | \
		sed -nE 's/^\s*Change-Id: (I[0-f]{10})[0-f]{30}\s*$$/\1/p' | \
		paste -sd "," -)
BUILD_TAG = "cld:$(CLD_IDS); cmn:$(CMN_IDS); dev:$(DEVNAME)"
ccflags-y += -DBUILD_TAG=\"$(BUILD_TAG)\"
endif

ifeq ($(CONFIG_ARCH_PINEAPPLE), y)
ccflags-y += -gdwarf-4
endif

# Module information used by KBuild framework
obj-$(CONFIG_QCA_CLD_WLAN) += $(MODNAME).o
ifeq ($(CONFIG_WLAN_RESIDENT_DRIVER), y)
$(MODNAME)-y := $(HDD_SRC_DIR)/wlan_hdd_main_module.o
obj-$(CONFIG_QCA_CLD_WLAN) += wlan_resident.o
wlan_resident-y := $(OBJS)
else
$(MODNAME)-y := $(OBJS)
endif
OBJS_DIRS += $(dir $(OBJS)) \
	     $(WLAN_COMMON_ROOT)/$(HIF_CE_DIR)/ \
	     $(QDF_OBJ_DIR)/ \
	     $(WLAN_COMMON_ROOT)/$(HIF_PCIE_DIR)/ \
	     $(WLAN_COMMON_ROOT)/$(HIF_SNOC_DIR)/ \
	     $(WLAN_COMMON_ROOT)/$(HIF_SDIO_DIR)/
CLEAN_DIRS := $(addsuffix *.o,$(sort $(OBJS_DIRS))) \
	      $(addsuffix .*.o.cmd,$(sort $(OBJS_DIRS)))
clean-files := $(CLEAN_DIRS)
