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

#include <qcatools_lib.h>
#include <cdp_txrx_stats_struct.h>
#include <wlan_stats_define.h>
#include <stats_lib.h>
#if UMAC_SUPPORT_CFG80211
#ifndef BUILD_X86
#include <netlink/genl/genl.h>
#endif
#endif

#define FL "%s(): %d:"

#define STATS_ERR(fmt, args...) \
	fprintf(stderr, FL "stats: Error: "fmt, __func__, __LINE__, ## args)
#define STATS_WARN(fmt, args...) \
	fprintf(stdout, FL "stats: Warn: "fmt, __func__, __LINE__, ## args)
#define STATS_MSG(fmt, args...) \
	fprintf(stdout, FL "stats: "fmt, __func__, __LINE__, ## args)

#define STATS_PRINT(fmt, args...) \
	fprintf(stdout, "stats: "fmt, ## args)

/* This path is created if phy is brought up in cfg80211 mode */
#define CFG80211_MODE_FILE_PATH      "/sys/class/net/wifi%d/phy80211/"

/* NL80211 command and event socket IDs */
#define STATS_NL80211_CMD_SOCK_ID    DEFAULT_NL80211_CMD_SOCK_ID
#define STATS_NL80211_EVENT_SOCK_ID  DEFAULT_NL80211_EVENT_SOCK_ID

#define SET_FLAG(_flg, _mask)              \
	do {                               \
		if (!(_flg))               \
			(_flg) = (_mask);  \
		else                       \
			(_flg) &= (_mask); \
	} while (0)

/* Global socket context to create nl80211 command and event interface */
static struct socket_context g_sock_ctx = {0};

/**
 * struct feat_parser_t: Defines feature name and corresponding ID
 * @name: Name of the feature passed from userspace
 * @id:   Feature flag corresponding to name
 */
struct feat_parser_t {
	char *name;
	u_int64_t id;
};

static struct feat_parser_t g_feat[] = {
	{ "ALL", STATS_FEAT_FLG_ALL },
	{ "TX", STATS_FEAT_FLG_TX },
	{ "RX", STATS_FEAT_FLG_RX },
	{ "AST", STATS_FEAT_FLG_AST },
	{ "CFR", STATS_FEAT_FLG_CFR },
	{ "FWD", STATS_FEAT_FLG_FWD },
	{ "HTT", STATS_FEAT_FLG_HTT },
	{ "RAW", STATS_FEAT_FLG_RAW },
	{ "RDK", STATS_FEAT_FLG_RDK },
	{ "TSO", STATS_FEAT_FLG_TSO },
	{ "TWT", STATS_FEAT_FLG_TWT },
	{ "VOW", STATS_FEAT_FLG_VOW },
	{ "WDI", STATS_FEAT_FLG_WDI },
	{ "WMI", STATS_FEAT_FLG_WMI },
	{ "IGMP", STATS_FEAT_FLG_IGMP },
	{ "LINK", STATS_FEAT_FLG_LINK },
	{ "MESH", STATS_FEAT_FLG_MESH },
	{ "RATE", STATS_FEAT_FLG_RATE },
	{ "DELAY", STATS_FEAT_FLG_DELAY },
	{ "ME", STATS_FEAT_FLG_ME },
	{ "NAWDS", STATS_FEAT_FLG_NAWDS },
	{ "TXCAP", STATS_FEAT_FLG_TXCAP },
	{ "MONITOR", STATS_FEAT_FLG_MONITOR },
	{ NULL, 0 },
};

struct nla_policy g_policy[QCA_WLAN_VENDOR_ATTR_FEAT_MAX] = {
	[QCA_WLAN_VENDOR_ATTR_OBJ_ID] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_FEAT_ME] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_FEAT_TX] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_FEAT_RX] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_FEAT_FWD] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_FEAT_RAW] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_FEAT_TSO] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_FEAT_TWT] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_FEAT_IGMP] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_FEAT_LINK] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_FEAT_MESH] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_FEAT_RATE] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_FEAT_NAWDS] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_RECURSIVE] = { .type = NLA_FLAG },
};

static int libstats_is_radio_ifname_valid(const char *ifname)
{
	int i;

	assert(ifname);
	/*
	 * At this step, we only validate if the string makes sense.
	 * If the interface doesn't actually exist, we'll throw an
	 * error at the place where we make system calls to try and
	 * use the interface.
	 * Reduces the no. of ioctl calls.
	 */
	if (strncmp(ifname, "wifi", 4) != 0)
		return 0;
	if (!ifname[4] || !isdigit(ifname[4]))
		return 0;
	/*
	 * We don't make any assumptions on max no. of radio interfaces,
	 * at this step.
	 */
	for (i = 5; i < IFNAMSIZ; i++) {
		if (!ifname[i])
			break;
		if (!isdigit(ifname[i]))
			return 0;
	}

	return 1;
}

static int libstats_is_vap_ifname_valid(const char *ifname)
{
	char path[100];
	FILE *fp;
	ssize_t bufsize = sizeof(path);

	assert(ifname);

	if ((strlcpy(path, PATH_SYSNET_DEV, bufsize) >= bufsize) ||
	    (strlcat(path, ifname, bufsize) >= bufsize) ||
	    (strlcat(path, "/parent", bufsize) >= bufsize)) {
		STATS_ERR("Error creating pathname\n");
		return -EINVAL;
	}
	fp = fopen(path, "r");
	if (fp) {
		fclose(fp);
		return 1;
	}

	return 0;
}

static int libstats_is_cfg80211_mode_enabled(void)
{
	FILE *fp;
	char filename[32] = {0};
	int i = 0;
	int ret = 0;

	for (i = 0; i < MAX_RADIO_NUM; i++) {
		snprintf(filename, sizeof(filename),
			 CFG80211_MODE_FILE_PATH, i);
		fp = fopen(filename, "r");
		if (fp) {
			ret = 1;
			fclose(fp);
		}
	}

	return ret;
}

static int libstats_socket_init(void)
{
	return init_socket_context(&g_sock_ctx, STATS_NL80211_CMD_SOCK_ID,
				   STATS_NL80211_EVENT_SOCK_ID);
}

static int libstats_init(void)
{
	g_sock_ctx.cfg80211 = libstats_is_cfg80211_mode_enabled();

	if (!g_sock_ctx.cfg80211) {
		STATS_ERR("CFG80211 mode is disabled!\n");
		return -EINVAL;
	}

	if (libstats_socket_init()) {
		STATS_ERR("Failed to initialise socket\n");
		return -EIO;
	}
	/* There are few generic IOCTLS, so we need to have sockfd */
	g_sock_ctx.sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (g_sock_ctx.sock_fd < 0) {
		errx(1, "socket creation failed");
		return -EIO;
	}

	return 0;
}

static void libstats_deinit(void)
{
	if (!g_sock_ctx.cfg80211)
		return;
	destroy_socket_context(&g_sock_ctx);
	close(g_sock_ctx.sock_fd);
}

static bool is_interface_active(const char *if_name, enum stats_object_e obj)
{
	struct ifreq  dev = {0};

	dev.ifr_addr.sa_family = AF_INET;
	memset(dev.ifr_name, '\0', IFNAMSIZ);
	if (strlcpy(dev.ifr_name, if_name, IFNAMSIZ) > IFNAMSIZ)
		return false;

	if (ioctl(g_sock_ctx.sock_fd, SIOCGIFFLAGS, &dev) < 0)
		return false;

	if ((dev.ifr_flags & IFF_RUNNING) &&
	    ((obj == STATS_OBJ_AP) || (obj == STATS_OBJ_RADIO) ||
	    (dev.ifr_flags & IFF_UP)))
		return true;

	return false;
}

static void set_valid_feature_request(struct stats_command *cmd)
{
	if (cmd->feat_flag == STATS_FEAT_FLG_ALL)
		return;

	switch (cmd->lvl) {
	case STATS_LVL_BASIC:
		{
			switch (cmd->obj) {
			case STATS_OBJ_AP:
				if (cmd->type & STATS_TYPE_CTRL) {
					SET_FLAG(cmd->feat_flag,
						 STATS_BASIC_AP_CTRL_MASK);
				} else {
					SET_FLAG(cmd->feat_flag,
						 STATS_BASIC_AP_DATA_MASK);
				}
				break;
			case STATS_OBJ_RADIO:
				if (cmd->type & STATS_TYPE_CTRL) {
					SET_FLAG(cmd->feat_flag,
						 STATS_BASIC_RADIO_CTRL_MASK);
				} else {
					SET_FLAG(cmd->feat_flag,
						 STATS_BASIC_RADIO_DATA_MASK);
				}
				break;
			case STATS_OBJ_VAP:
				if (cmd->type & STATS_TYPE_CTRL) {
					SET_FLAG(cmd->feat_flag,
						 STATS_BASIC_VAP_CTRL_MASK);
				} else {
					SET_FLAG(cmd->feat_flag,
						 STATS_BASIC_VAP_DATA_MASK);
				}
				break;
			case STATS_OBJ_STA:
				if (cmd->type & STATS_TYPE_CTRL) {
					SET_FLAG(cmd->feat_flag,
						 STATS_BASIC_STA_CTRL_MASK);
				} else {
					SET_FLAG(cmd->feat_flag,
						 STATS_BASIC_STA_DATA_MASK);
				}
				break;
			}
		}
		break;
#if WLAN_ADVANCE_TELEMETRY
	case STATS_LVL_ADVANCE:
		{
			switch (cmd->obj) {
			case STATS_OBJ_AP:
				if (cmd->type & STATS_TYPE_CTRL) {
					SET_FLAG(cmd->feat_flag,
						 STATS_ADVANCE_AP_CTRL_MASK);
				} else {
					SET_FLAG(cmd->feat_flag,
						 STATS_ADVANCE_AP_DATA_MASK);
				}
				break;
			case STATS_OBJ_RADIO:
				if (cmd->type & STATS_TYPE_CTRL) {
					SET_FLAG(cmd->feat_flag,
						 STATS_ADVANCE_RADIO_CTRL_MASK);
				} else {
					SET_FLAG(cmd->feat_flag,
						 STATS_ADVANCE_RADIO_DATA_MASK);
				}
				break;
			case STATS_OBJ_VAP:
				if (cmd->type & STATS_TYPE_CTRL) {
					SET_FLAG(cmd->feat_flag,
						 STATS_ADVANCE_VAP_CTRL_MASK);
				} else {
					SET_FLAG(cmd->feat_flag,
						 STATS_ADVANCE_VAP_DATA_MASK);
				}
				break;
			case STATS_OBJ_STA:
				if (cmd->type & STATS_TYPE_CTRL) {
					SET_FLAG(cmd->feat_flag,
						 STATS_ADVANCE_STA_CTRL_MASK);
				} else {
					SET_FLAG(cmd->feat_flag,
						 STATS_ADVANCE_STA_DATA_MASK);
				}
				break;
			}
		}
		break;
#endif /* WLAN_ADVANCE_TELEMETRY */
#if WLAN_DEBUG_TELEMETRY
	case STATS_LVL_DEBUG:
		{
			switch (cmd->obj) {
			case STATS_OBJ_AP:
				if (cmd->type & STATS_TYPE_CTRL) {
					SET_FLAG(cmd->feat_flag,
						 STATS_DEBUG_AP_CTRL_MASK);
				} else {
					SET_FLAG(cmd->feat_flag,
						 STATS_DEBUG_AP_DATA_MASK);
				}
				break;
			case STATS_OBJ_RADIO:
				if (cmd->type & STATS_TYPE_CTRL) {
					SET_FLAG(cmd->feat_flag,
						 STATS_DEBUG_RADIO_CTRL_MASK);
				} else {
					SET_FLAG(cmd->feat_flag,
						 STATS_DEBUG_RADIO_DATA_MASK);
				}
				break;
			case STATS_OBJ_VAP:
				if (cmd->type & STATS_TYPE_CTRL) {
					SET_FLAG(cmd->feat_flag,
						 STATS_DEBUG_VAP_CTRL_MASK);
				} else {
					SET_FLAG(cmd->feat_flag,
						 STATS_DEBUG_VAP_DATA_MASK);
				}
				break;
			case STATS_OBJ_STA:
				if (cmd->type & STATS_TYPE_CTRL) {
					SET_FLAG(cmd->feat_flag,
						 STATS_DEBUG_STA_CTRL_MASK);
				} else {
					SET_FLAG(cmd->feat_flag,
						 STATS_DEBUG_STA_DATA_MASK);
				}
				break;
			}
		}
		break;
#endif /* WLAN_DEBUG_TELEMETRY */
	default:
		cmd->feat_flag = 0;
	}
}

static int is_valid_cmd(struct stats_command *cmd)
{
	u_int8_t *sta_mac = cmd->sta_mac.ether_addr_octet;

	switch (cmd->obj) {
	case STATS_OBJ_AP:
		if (sta_mac[0])
			STATS_WARN("Ignore STA MAC for AP stats\n");
		break;
	case STATS_OBJ_RADIO:
		if (!cmd->if_name[0]) {
			STATS_ERR("Radio Interface name not configured.\n");
			return -EINVAL;
		}
		if (!libstats_is_radio_ifname_valid(cmd->if_name)) {
			STATS_ERR("Radio Interface name invalid.\n");
			return -EINVAL;
		}
		if (sta_mac[0])
			STATS_WARN("Ignore STA MAC address input for Radio\n");
		break;
	case STATS_OBJ_VAP:
		if (!cmd->if_name[0]) {
			STATS_ERR("VAP Interface name not configured.\n");
			return -EINVAL;
		}
		if (!libstats_is_vap_ifname_valid(cmd->if_name)) {
			STATS_ERR("VAP Interface name invalid.\n");
			return -EINVAL;
		}
		if (sta_mac[0])
			STATS_WARN("Ignore STA MAC address input for VAP\n");
		break;
	case STATS_OBJ_STA:
		if (cmd->recursive)
			STATS_WARN("Ignore recursive display for STA\n");
		cmd->recursive = false;
		break;
	default:
		STATS_ERR("Unknown Stats object.\n");
		return -EINVAL;
	}
	set_valid_feature_request(cmd);
	if (!cmd->feat_flag) {
		STATS_ERR("Invalid Feature Request!\n");
		return -EINVAL;
	}

	return 0;
}

static int32_t libstats_prepare_request(struct nl_msg *nlmsg,
					struct stats_command *cmd)
{
	int32_t ret = 0;

	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_TELEMETRIC_LEVEL,
		       cmd->lvl)) {
		STATS_ERR("failed to put stats level\n");
		return -EIO;
	}
	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_TELEMETRIC_OBJECT,
		       cmd->obj)) {
		STATS_ERR("failed to put stats object\n");
		return -EIO;
	}
	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_TELEMETRIC_TYPE,
		       cmd->type)) {
		STATS_ERR("failed to put stats category type\n");
		return -EIO;
	}
	if (cmd->recursive &&
	    nla_put_flag(nlmsg, QCA_WLAN_VENDOR_ATTR_TELEMETRIC_RECURSIVE)) {
		STATS_ERR("failed to put recursive flag\n");
		return -EIO;
	}
	if (nla_put_u64(nlmsg, QCA_WLAN_VENDOR_ATTR_TELEMETRIC_FEATURE_FLAG,
			cmd->feat_flag)) {
		STATS_ERR("failed to put feature flag\n");
		return -EIO;
	}
	if (cmd->obj == STATS_OBJ_STA) {
		if (nla_put(nlmsg, QCA_WLAN_VENDOR_ATTR_TELEMETRIC_STA_MAC,
			    ETH_ALEN, cmd->sta_mac.ether_addr_octet)) {
			STATS_ERR("failed to put sta MAC\n");
			return -EIO;
		}
	}

	return ret;
}

static struct stats_vap *find_parent_vap(struct reply_buffer *reply)
{
	struct stats_vap *vap = reply->obj_list.vap_list;

	if (!vap)
		return NULL;
	while (vap->mgr.next)
		vap = vap->mgr.next;

	return vap;
}

static struct stats_radio *find_parent_radio(struct reply_buffer *reply)
{
	struct stats_radio *radio = reply->obj_list.radio_list;

	if (!radio)
		return NULL;
	while (radio->mgr.next)
		radio = radio->mgr.next;

	return radio;
}

static struct stats_ap *find_parent_ap(struct reply_buffer *reply)
{
	struct stats_ap *ap = reply->obj_list.ap_list;

	if (!ap)
		return NULL;
	while (ap->mgr.next)
		ap = ap->mgr.next;

	return ap;
}

static struct stats_sta *add_stats_sta(struct reply_buffer *reply)
{
	struct stats_sta *sta = NULL;
	struct stats_sta *temp = NULL;
	struct stats_vap *vap = NULL;

	if (STATS_OBJ_STA != reply->root_obj) {
		vap = find_parent_vap(reply);
		if (!vap) {
			STATS_ERR("Unable to find Parent vap\n");
			return NULL;
		}
	}
	sta = (struct stats_sta *)malloc(sizeof(struct stats_sta));
	if (!sta) {
		STATS_ERR("Unable to allocate stats_sta!\n");
		return NULL;
	}
	memset(sta, 0, sizeof(struct stats_sta));
	sta->mgr.parent = vap;
	if (vap && !vap->mgr.child_root)
		vap->mgr.child_root = sta;
	if (!reply->obj_list.sta_list) {
		reply->obj_list.sta_list = sta;
	} else {
		temp = reply->obj_list.sta_list;
		while (temp->mgr.next)
			temp = temp->mgr.next;
		temp->mgr.next = sta;
	}

	return sta;
}

static struct stats_vap *add_stats_vap(struct reply_buffer *reply)
{
	struct stats_vap *vap = NULL;
	struct stats_vap *temp = NULL;
	struct stats_radio *radio = NULL;

	if (STATS_OBJ_VAP != reply->root_obj) {
		radio = find_parent_radio(reply);
		if (!radio) {
			STATS_ERR("Unable to find Parent radio\n");
			return NULL;
		}
	}
	vap = (struct stats_vap *)malloc(sizeof(struct stats_vap));
	if (!vap) {
		STATS_ERR("Unable to allocate stats_vap!\n");
		return NULL;
	}
	memset(vap, 0, sizeof(struct stats_vap));
	vap->id = reply->vap_count;
	reply->vap_count++;
	vap->mgr.parent = radio;
	if (radio && !radio->mgr.child_root)
		radio->mgr.child_root = vap;
	if (!reply->obj_list.vap_list) {
		reply->obj_list.vap_list = vap;
	} else {
		temp = reply->obj_list.vap_list;
		while (temp->mgr.next)
			temp = temp->mgr.next;
		temp->mgr.next = vap;
	}

	return vap;
}

static struct stats_radio *add_stats_radio(struct reply_buffer *reply)
{
	struct stats_radio *radio = NULL;
	struct stats_radio *temp = NULL;
	struct stats_ap *ap = NULL;

	if (STATS_OBJ_RADIO != reply->root_obj) {
		ap = find_parent_ap(reply);
		if (!ap) {
			STATS_ERR("Unable to find Parent ap\n");
			return NULL;
		}
	}
	radio = (struct stats_radio *)malloc(sizeof(struct stats_radio));
	if (!radio) {
		STATS_ERR("Unable to allocate stats_radio!\n");
		return NULL;
	}
	memset(radio, 0, sizeof(struct stats_radio));
	radio->id = reply->radio_count;
	reply->radio_count++;
	radio->mgr.parent = ap;
	if (ap && !ap->mgr.child_root)
		ap->mgr.child_root = radio;
	if (!reply->obj_list.radio_list) {
		reply->obj_list.radio_list = radio;
	} else {
		temp = reply->obj_list.radio_list;
		while (temp->mgr.next)
			temp = temp->mgr.next;
		temp->mgr.next = radio;
	}

	return radio;
}

static struct stats_ap *add_stats_ap(struct reply_buffer *reply)
{
	struct stats_ap *ap = NULL;
	struct stats_ap *temp = NULL;

	if (STATS_OBJ_AP != reply->root_obj) {
		STATS_ERR("Invalid Request\n");
		return NULL;
	}
	ap = (struct stats_ap *)malloc(sizeof(struct stats_ap));
	if (!ap) {
		STATS_ERR("Unable to allocate stats_ap!\n");
		return NULL;
	}
	memset(ap, 0, sizeof(struct stats_ap));
	ap->id = reply->ap_count;
	reply->ap_count++;
	if (!reply->obj_list.ap_list) {
		reply->obj_list.ap_list = ap;
	} else {
		temp = reply->obj_list.ap_list;
		while (temp->mgr.next)
			temp = temp->mgr.next;
		temp->mgr.next = ap;
	}

	return ap;
}

static void libstats_parse_basic_sta(struct cfg80211_data *buffer,
				     enum stats_type_e type,
				     struct nlattr *rattr,
				     struct reply_buffer *reply)
{
	u_int32_t size = 0;
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_FEAT_MAX + 1] = {0};
	struct stats_sta *sta = NULL;
	void *dest = NULL;

	if (!rattr || nla_parse_nested(tb, QCA_WLAN_VENDOR_ATTR_FEAT_MAX,
				       rattr, g_policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return;
	}
	sta = add_stats_sta(reply);
	if (!sta)
		return;

	if (tb[QCA_WLAN_VENDOR_ATTR_OBJ_ID])
		memcpy(sta->mac_addr,
		       nla_data(tb[QCA_WLAN_VENDOR_ATTR_OBJ_ID]),
		       ETH_ALEN);
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
		if (type == STATS_TYPE_DATA) {
			size = sizeof(struct basic_peer_data_tx);
			dest = malloc(size);
			sta->u_basic.data.tx = dest;
		} else {
			size = sizeof(struct basic_peer_ctrl_tx);
			dest = malloc(size);
			sta->u_basic.ctrl.tx = dest;
		}
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
		if (type == STATS_TYPE_DATA) {
			size = sizeof(struct basic_peer_data_rx);
			dest = malloc(size);
			sta->u_basic.data.rx = dest;
		} else {
			size = sizeof(struct basic_peer_ctrl_rx);
			dest = malloc(size);
			sta->u_basic.ctrl.rx = dest;
		}
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_LINK]) {
		if (type == STATS_TYPE_DATA) {
			size = sizeof(struct basic_peer_data_link);
			dest = malloc(size);
			sta->u_basic.data.link = dest;
		} else {
			size = sizeof(struct basic_peer_ctrl_link);
			dest = malloc(size);
			sta->u_basic.ctrl.link = dest;
		}
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_LINK]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RATE]) {
		if (type == STATS_TYPE_DATA) {
			size = sizeof(struct basic_peer_data_rate);
			dest = malloc(size);
			sta->u_basic.data.rate = dest;
		} else {
			size = sizeof(struct basic_peer_ctrl_rate);
			dest = malloc(size);
			sta->u_basic.ctrl.rate = dest;
		}
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_RATE]),
			       size);
	}
}

static void libstats_parse_basic_vap(struct cfg80211_data *buffer,
				     enum stats_type_e type,
				     struct nlattr *rattr,
				     struct reply_buffer *reply)
{
	u_int32_t size = 0;
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_FEAT_MAX + 1] = {0};
	struct stats_vap *vap = NULL;
	void *dest = NULL;

	if (!rattr || nla_parse_nested(tb, QCA_WLAN_VENDOR_ATTR_FEAT_MAX,
				       rattr, g_policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return;
	}
	vap = add_stats_vap(reply);
	if (!vap)
		return;

	if (tb[QCA_WLAN_VENDOR_ATTR_OBJ_ID])
		strlcpy(vap->vap_name,
			nla_get_string(tb[QCA_WLAN_VENDOR_ATTR_OBJ_ID]),
			IFNAME_LEN);
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
		if (type == STATS_TYPE_DATA) {
			size = sizeof(struct basic_vdev_data_tx);
			dest = malloc(size);
			vap->u_basic.data.tx = dest;
		} else {
			size = sizeof(struct basic_vdev_ctrl_tx);
			dest = malloc(size);
			vap->u_basic.ctrl.tx = dest;
		}
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
		if (type == STATS_TYPE_DATA) {
			size = sizeof(struct basic_vdev_data_rx);
			dest = malloc(size);
			vap->u_basic.data.rx = dest;
		} else {
			size = sizeof(struct basic_vdev_ctrl_rx);
			dest = malloc(size);
			vap->u_basic.ctrl.rx = dest;
		}
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_RECURSIVE])
		vap->recursive = true;
}

static void libstats_parse_basic_radio(struct cfg80211_data *buffer,
				       enum stats_type_e type,
				       struct nlattr *rattr,
				       struct reply_buffer *reply)
{
	u_int32_t size = 0;
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_FEAT_MAX + 1] = {0};
	struct stats_radio *radio = NULL;
	void *dest = NULL;

	if (!rattr || nla_parse_nested(tb, QCA_WLAN_VENDOR_ATTR_FEAT_MAX,
				       rattr, g_policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return;
	}
	radio = add_stats_radio(reply);
	if (!radio)
		return;

	if (tb[QCA_WLAN_VENDOR_ATTR_OBJ_ID])
		snprintf(radio->radio_name, IFNAME_LEN, "wifi%d",
			 nla_get_u8(tb[QCA_WLAN_VENDOR_ATTR_OBJ_ID]));
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
		if (type == STATS_TYPE_DATA) {
			size = sizeof(struct basic_pdev_data_tx);
			dest = malloc(size);
			radio->u_basic.data.tx = dest;
		} else {
			size = sizeof(struct basic_pdev_ctrl_tx);
			dest = malloc(size);
			radio->u_basic.ctrl.tx = dest;
		}
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
		if (type == STATS_TYPE_DATA) {
			size = sizeof(struct basic_pdev_data_rx);
			dest = malloc(size);
			radio->u_basic.data.rx = dest;
		} else {
			size = sizeof(struct basic_pdev_ctrl_rx);
			dest = malloc(size);
			radio->u_basic.ctrl.rx = dest;
		}
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_LINK]) {
		size = sizeof(struct basic_pdev_ctrl_link);
		dest = malloc(size);
		radio->u_basic.ctrl.link = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_LINK]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_RECURSIVE])
		radio->recursive = true;
}

static void libstats_parse_basic_ap(struct cfg80211_data *buffer,
				    enum stats_type_e type,
				    struct nlattr *rattr,
				    struct reply_buffer *reply)
{
	u_int32_t size = 0;
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_FEAT_MAX + 1] = {0};
	struct stats_ap *ap = NULL;
	void *dest = NULL;

	if (!rattr || nla_parse_nested(tb, QCA_WLAN_VENDOR_ATTR_FEAT_MAX,
				       rattr, g_policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return;
	}
	ap = add_stats_ap(reply);
	if (!ap)
		return;

	if (tb[QCA_WLAN_VENDOR_ATTR_OBJ_ID])
		snprintf(ap->ap_name, IFNAME_LEN, "Soc%d",
			 nla_get_u8(tb[QCA_WLAN_VENDOR_ATTR_OBJ_ID]));
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
		size = sizeof(struct basic_psoc_data_tx);
		dest = malloc(size);
		ap->u.b_data.tx = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
		size = sizeof(struct basic_psoc_data_rx);
		dest = malloc(size);
		ap->u.b_data.rx = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_RECURSIVE])
		ap->recursive = true;
}

#if WLAN_ADVANCE_TELEMETRY
static void libstats_parse_advance_sta(struct cfg80211_data *buffer,
				       enum stats_type_e type,
				       struct nlattr *rattr,
				       struct reply_buffer *reply)
{
	u_int32_t size = 0;
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_FEAT_MAX + 1] = {0};
	struct stats_sta *sta = NULL;
	void *dest = NULL;

	if (!rattr || nla_parse_nested(tb, QCA_WLAN_VENDOR_ATTR_FEAT_MAX,
				       rattr, g_policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return;
	}
	sta = add_stats_sta(reply);
	if (!sta)
		return;

	if (tb[QCA_WLAN_VENDOR_ATTR_OBJ_ID])
		memcpy(sta->mac_addr,
		       nla_data(tb[QCA_WLAN_VENDOR_ATTR_OBJ_ID]),
		       ETH_ALEN);
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
		if (type == STATS_TYPE_DATA) {
			size = sizeof(struct advance_peer_data_tx);
			dest = malloc(size);
			sta->u_advance.data.tx = dest;
		} else {
			size = sizeof(struct advance_peer_ctrl_tx);
			dest = malloc(size);
			sta->u_advance.ctrl.tx = dest;
		}
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
		if (type == STATS_TYPE_DATA) {
			size = sizeof(struct advance_peer_data_rx);
			dest = malloc(size);
			sta->u_advance.data.rx = dest;
		} else {
			size = sizeof(struct advance_peer_ctrl_rx);
			dest = malloc(size);
			sta->u_advance.ctrl.rx = dest;
		}
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_FWD]) {
		size = sizeof(struct advance_peer_data_fwd);
		dest = malloc(size);
		sta->u_advance.data.fwd = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_FWD]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RAW]) {
		size = sizeof(struct advance_peer_data_raw);
		dest = malloc(size);
		sta->u_advance.data.raw = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_RAW]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TWT]) {
		if (type == STATS_TYPE_DATA) {
			size = sizeof(struct advance_peer_data_twt);
			dest = malloc(size);
			sta->u_advance.data.twt = dest;
		} else {
			size = sizeof(struct advance_peer_ctrl_twt);
			dest = malloc(size);
			sta->u_advance.ctrl.twt = dest;
		}
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_TWT]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_LINK]) {
		if (type == STATS_TYPE_DATA) {
			size = sizeof(struct advance_peer_data_link);
			dest = malloc(size);
			sta->u_advance.data.link = dest;
		} else {
			size = sizeof(struct advance_peer_ctrl_link);
			dest = malloc(size);
			sta->u_advance.ctrl.link = dest;
		}
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_LINK]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RATE]) {
		if (type == STATS_TYPE_DATA) {
			size = sizeof(struct advance_peer_data_rate);
			dest = malloc(size);
			sta->u_advance.data.rate = dest;
		} else {
			size = sizeof(struct advance_peer_ctrl_rate);
			dest = malloc(size);
			sta->u_advance.ctrl.rate = dest;
		}
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_RATE]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_NAWDS]) {
		size = sizeof(struct advance_peer_data_nawds);
		dest = malloc(size);
		sta->u_advance.data.nawds = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_NAWDS]),
			       size);
	}
}

static void libstats_parse_advance_vap(struct cfg80211_data *buffer,
				       enum stats_type_e type,
				       struct nlattr *rattr,
				       struct reply_buffer *reply)
{
	u_int32_t size = 0;
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_FEAT_MAX + 1] = {0};
	struct stats_vap *vap = NULL;
	void *dest = NULL;

	if (!rattr || nla_parse_nested(tb, QCA_WLAN_VENDOR_ATTR_FEAT_MAX,
				       rattr, g_policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return;
	}
	vap = add_stats_vap(reply);
	if (!vap)
		return;

	if (tb[QCA_WLAN_VENDOR_ATTR_OBJ_ID])
		strlcpy(vap->vap_name,
			nla_get_string(
			tb[QCA_WLAN_VENDOR_ATTR_OBJ_ID]),
			IFNAME_LEN);
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_ME]) {
		size = sizeof(struct advance_vdev_data_me);
		dest = malloc(size);
		vap->u_advance.data.me = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_ME]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
		if (type == STATS_TYPE_DATA) {
			size = sizeof(struct advance_vdev_data_tx);
			dest = malloc(size);
			vap->u_advance.data.tx = dest;
		} else {
			size = sizeof(struct advance_vdev_ctrl_tx);
			dest = malloc(size);
			vap->u_advance.ctrl.tx = dest;
		}
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
		if (type == STATS_TYPE_DATA) {
			size = sizeof(struct advance_vdev_data_rx);
			dest = malloc(size);
			vap->u_advance.data.rx = dest;
		} else {
			size = sizeof(struct advance_vdev_ctrl_rx);
			dest = malloc(size);
			vap->u_advance.ctrl.rx = dest;
		}
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RAW]) {
		size = sizeof(struct advance_vdev_data_raw);
		dest = malloc(size);
		vap->u_advance.data.raw = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_RAW]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TSO]) {
		size = sizeof(struct advance_vdev_data_tso);
		dest = malloc(size);
		vap->u_advance.data.tso = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_TSO]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_IGMP]) {
		size = sizeof(struct advance_vdev_data_igmp);
		dest = malloc(size);
		vap->u_advance.data.igmp = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_IGMP]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_MESH]) {
		size = sizeof(struct advance_vdev_data_mesh);
		dest = malloc(size);
		vap->u_advance.data.mesh = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_MESH]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_NAWDS]) {
		size = sizeof(struct advance_vdev_data_nawds);
		dest = malloc(size);
		vap->u_advance.data.nawds = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_NAWDS]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_RECURSIVE])
		vap->recursive = true;
}

static void libstats_parse_advance_radio(struct cfg80211_data *buffer,
					 enum stats_type_e type,
					 struct nlattr *rattr,
					 struct reply_buffer *reply)
{
	u_int32_t size = 0;
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_FEAT_MAX + 1] = {0};
	struct stats_radio *radio = NULL;
	void *dest = NULL;

	if (!rattr || nla_parse_nested(tb, QCA_WLAN_VENDOR_ATTR_FEAT_MAX,
				       rattr, g_policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return;
	}
	radio = add_stats_radio(reply);
	if (!radio)
		return;

	if (tb[QCA_WLAN_VENDOR_ATTR_OBJ_ID])
		snprintf(radio->radio_name, IFNAME_LEN, "wifi%d",
			 nla_get_u8(tb[QCA_WLAN_VENDOR_ATTR_OBJ_ID]));
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
		if (type == STATS_TYPE_DATA) {
			size = sizeof(struct advance_pdev_data_tx);
			dest = malloc(size);
			radio->u_advance.data.tx = dest;
		} else {
			size = sizeof(struct advance_pdev_ctrl_tx);
			dest = malloc(size);
			radio->u_advance.ctrl.tx = dest;
		}
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
		if (type == STATS_TYPE_DATA) {
			size = sizeof(struct advance_pdev_data_rx);
			dest = malloc(size);
			radio->u_advance.data.rx = dest;
		} else {
			size = sizeof(struct advance_pdev_ctrl_rx);
			dest = malloc(size);
			radio->u_advance.ctrl.rx = dest;
		}
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_ME]) {
		size = sizeof(struct advance_pdev_data_me);
		dest = malloc(size);
		radio->u_advance.data.me = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_ME]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RAW]) {
		size = sizeof(struct advance_pdev_data_raw);
		dest = malloc(size);
		radio->u_advance.data.raw = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_RAW]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TSO]) {
		size = sizeof(struct advance_pdev_data_tso);
		dest = malloc(size);
		radio->u_advance.data.tso = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_TSO]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_IGMP]) {
		size = sizeof(struct advance_pdev_data_igmp);
		dest = malloc(size);
		radio->u_advance.data.igmp = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_IGMP]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_MESH]) {
		size = sizeof(struct advance_pdev_data_mesh);
		dest = malloc(size);
		radio->u_advance.data.mesh = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_MESH]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_NAWDS]) {
		size = sizeof(struct advance_pdev_data_nawds);
		dest = malloc(size);
		radio->u_advance.data.nawds = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_NAWDS]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_LINK]) {
		size = sizeof(struct advance_pdev_ctrl_link);
		dest = malloc(size);
		radio->u_advance.ctrl.link = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_LINK]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_RECURSIVE])
		radio->recursive = true;
}

static void libstats_parse_advance_ap(struct cfg80211_data *buffer,
				      enum stats_type_e type,
				      struct nlattr *rattr,
				      struct reply_buffer *reply)
{
	u_int32_t size = 0;
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_FEAT_MAX + 1] = {0};
	struct stats_ap *ap = NULL;
	void *dest = NULL;

	if (!rattr || nla_parse_nested(tb, QCA_WLAN_VENDOR_ATTR_FEAT_MAX,
				       rattr, g_policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return;
	}
	ap = add_stats_ap(reply);
	if (!ap)
		return;

	if (tb[QCA_WLAN_VENDOR_ATTR_OBJ_ID])
		snprintf(ap->ap_name, IFNAME_LEN, "Soc%d",
			 nla_get_u8(tb[QCA_WLAN_VENDOR_ATTR_OBJ_ID]));
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
		size = sizeof(struct advance_psoc_data_tx);
		dest = malloc(size);
		ap->u.a_data.tx = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
		size = sizeof(struct advance_psoc_data_rx);
		dest = malloc(size);
		ap->u.a_data.rx = dest;
		if (dest)
			memcpy(dest,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]),
			       size);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_RECURSIVE])
		ap->recursive = true;
}
#endif /* WLAN_ADVANCE_TELEMETRY */

static void libstats_response_handler(struct cfg80211_data *buffer)
{
	struct stats_command *cmd = NULL;
	struct reply_buffer *reply = NULL;
	struct nlattr *attr = NULL;
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_STATS_MAX + 1] = {0};
	u_int8_t obj = 0;
	struct nla_policy policy[QCA_WLAN_VENDOR_ATTR_STATS_MAX] = {
	[QCA_WLAN_VENDOR_ATTR_STATS_OBJECT] = { .type = NLA_U8 },
	[QCA_WLAN_VENDOR_ATTR_STATS_RECURSIVE] = { .type = NLA_NESTED },
	};

	if (!buffer) {
		STATS_ERR("Buffer not valid\n");
		return;
	}
	if (!buffer->nl_vendordata) {
		STATS_ERR("Vendor Data is null\n");
		return;
	}
	if (!buffer->data) {
		STATS_ERR("User Data is null\n");
		return;
	}
	cmd = buffer->data;
	reply = cmd->reply;
	if (!reply) {
		STATS_ERR("User reply buffer in null\n");
		return;
	}
	reply->root_obj = cmd->obj;
	if (nla_parse(tb, QCA_WLAN_VENDOR_ATTR_STATS_MAX,
		      buffer->nl_vendordata, buffer->nl_vendordata_len,
		      policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return;
	}
	if (!tb[QCA_WLAN_VENDOR_ATTR_STATS_OBJECT]) {
		STATS_ERR("NLA Parsing failed for Stats Object\n");
		return;
	}
	if (!tb[QCA_WLAN_VENDOR_ATTR_STATS_RECURSIVE]) {
		STATS_ERR("NLA Parsing failed for Stats Recursive\n");
		return;
	}
	obj = nla_get_u8(tb[QCA_WLAN_VENDOR_ATTR_STATS_OBJECT]);
	attr = tb[QCA_WLAN_VENDOR_ATTR_STATS_RECURSIVE];
	if (cmd->lvl == STATS_LVL_BASIC) {
		switch (obj) {
		case STATS_OBJ_STA:
			libstats_parse_basic_sta(buffer, cmd->type,
						 attr, reply);
			break;
		case STATS_OBJ_VAP:
			libstats_parse_basic_vap(buffer, cmd->type,
						 attr, reply);
			break;
		case STATS_OBJ_RADIO:
			libstats_parse_basic_radio(buffer, cmd->type,
						   attr, reply);
			break;
		case STATS_OBJ_AP:
			libstats_parse_basic_ap(buffer, cmd->type,
						attr, reply);
			break;
		default:
			STATS_ERR("Unexpected Object\n");
		}
#if WLAN_ADVANCE_TELEMETRY
	} else if (cmd->lvl == STATS_LVL_ADVANCE) {
		switch (obj) {
		case STATS_OBJ_STA:
			libstats_parse_advance_sta(buffer, cmd->type,
						   attr, reply);
			break;
		case STATS_OBJ_VAP:
			libstats_parse_advance_vap(buffer, cmd->type,
						   attr, reply);
			break;
		case STATS_OBJ_RADIO:
			libstats_parse_advance_radio(buffer, cmd->type,
						     attr, reply);
			break;
		case STATS_OBJ_AP:
			libstats_parse_advance_ap(buffer, cmd->type,
						  attr, reply);
			break;
		default:
			STATS_ERR("Unexpected Object\n");
		}
#endif /* WLAN_ADVANCE_TELEMETRY */
#if WLAN_DEBUG_TELEMETRY
	} else if (cmd->lvl == STATS_LVL_DEBUG) {
#endif /* WLAN_DEBUG_TELEMETRY */
	} else {
		STATS_ERR("Level Not Supported!\n");
	}
}

static int32_t libstats_send_nl_command(struct stats_command *cmd,
					struct cfg80211_data *buffer,
					const char *ifname)
{
	struct nl_msg *nlmsg = NULL;
	struct nlattr *nl_ven_data = NULL;
	int32_t ret = 0;

	nlmsg = wifi_cfg80211_prepare_command(&g_sock_ctx.cfg80211_ctxt,
		QCA_NL80211_VENDOR_SUBCMD_TELEMETRIC_DATA, ifname);
	if (nlmsg) {
		nl_ven_data = (struct nlattr *)start_vendor_data(nlmsg);
		if (!nl_ven_data) {
			STATS_ERR("failed to start vendor data\n");
			nlmsg_free(nlmsg);
			return -EIO;
		}
		ret = libstats_prepare_request(nlmsg, cmd);
		if (ret) {
			STATS_ERR("failed to prepare stats request\n");
			nlmsg_free(nlmsg);
			return -EINVAL;
		}
		end_vendor_data(nlmsg, nl_ven_data);

		ret = send_nlmsg(&g_sock_ctx.cfg80211_ctxt, nlmsg, buffer);
		if (ret < 0)
			STATS_ERR("Couldn't send NL command, ret = %d\n", ret);
	} else {
		ret = -EINVAL;
		STATS_ERR("Unable to prepare NL command!\n");
	}

	return ret;
}

static int32_t libstats_send(struct stats_command *cmd)
{
	struct cfg80211_data buffer;
	int32_t ret = 0;

	if (!g_sock_ctx.cfg80211) {
		STATS_ERR("cfg80211 interface is not enabled\n");
		return -EPERM;
	}

	if (is_valid_cmd(cmd)) {
		STATS_ERR("Invalid command\n");
		return -EINVAL;
	}

	memset(&buffer, 0, sizeof(struct cfg80211_data));
	buffer.data = cmd;
	buffer.length = sizeof(*cmd);
	buffer.callback = libstats_response_handler;
	buffer.parse_data = 1;

	if (is_interface_active(cmd->if_name, cmd->obj))
		ret = libstats_send_nl_command(cmd, &buffer, cmd->if_name);

	return ret;
}

static void free_basic_sta(struct stats_sta *sta, enum stats_type_e type)
{
	switch (type) {
	case STATS_TYPE_DATA:
		if (sta->u_basic.data.tx)
			free(sta->u_basic.data.tx);
		if (sta->u_basic.data.rx)
			free(sta->u_basic.data.rx);
		if (sta->u_basic.data.link)
			free(sta->u_basic.data.link);
		if (sta->u_basic.data.rate)
			free(sta->u_basic.data.rate);
		break;
	case STATS_TYPE_CTRL:
		if (sta->u_basic.ctrl.tx)
			free(sta->u_basic.ctrl.tx);
		if (sta->u_basic.ctrl.rx)
			free(sta->u_basic.ctrl.rx);
		if (sta->u_basic.ctrl.link)
			free(sta->u_basic.ctrl.link);
		if (sta->u_basic.ctrl.rate)
			free(sta->u_basic.ctrl.rate);
		break;
	default:
		STATS_ERR("Unexpected Type %d!\n", type);
	}
}

static void free_basic_vap(struct stats_vap *vap, enum stats_type_e type)
{
	switch (type) {
	case STATS_TYPE_DATA:
		if (vap->u_basic.data.tx)
			free(vap->u_basic.data.tx);
		if (vap->u_basic.data.rx)
			free(vap->u_basic.data.rx);
		break;
	case STATS_TYPE_CTRL:
		if (vap->u_basic.ctrl.tx)
			free(vap->u_basic.ctrl.tx);
		if (vap->u_basic.ctrl.rx)
			free(vap->u_basic.ctrl.rx);
		break;
	default:
		STATS_ERR("Unexpected Type %d!\n", type);
	}
}

static void free_basic_radio(struct stats_radio *radio, enum stats_type_e type)
{
	switch (type) {
	case STATS_TYPE_DATA:
		if (radio->u_basic.data.tx)
			free(radio->u_basic.data.tx);
		if (radio->u_basic.data.rx)
			free(radio->u_basic.data.rx);
		break;
	case STATS_TYPE_CTRL:
		if (radio->u_basic.ctrl.tx)
			free(radio->u_basic.ctrl.tx);
		if (radio->u_basic.ctrl.rx)
			free(radio->u_basic.ctrl.rx);
		if (radio->u_basic.ctrl.link)
			free(radio->u_basic.ctrl.link);
		break;
	default:
		STATS_ERR("Unexpected Type %d!\n", type);
	}
}

static void free_basic_ap(struct stats_ap *ap, enum stats_type_e type)
{
	switch (type) {
	case STATS_TYPE_DATA:
		if (ap->u.b_data.tx)
			free(ap->u.b_data.tx);
		if (ap->u.b_data.rx)
			free(ap->u.b_data.rx);
		break;
	case STATS_TYPE_CTRL:
		break;
	default:
		STATS_ERR("Unexpected Type %d!\n", type);
	}
}

#if WLAN_ADVANCE_TELEMETRY
static void free_advance_sta(struct stats_sta *sta, enum stats_type_e type)
{
	switch (type) {
	case STATS_TYPE_DATA:
		if (sta->u_advance.data.tx)
			free(sta->u_advance.data.tx);
		if (sta->u_advance.data.rx)
			free(sta->u_advance.data.rx);
		if (sta->u_advance.data.fwd)
			free(sta->u_advance.data.fwd);
		if (sta->u_advance.data.raw)
			free(sta->u_advance.data.raw);
		if (sta->u_advance.data.twt)
			free(sta->u_advance.data.twt);
		if (sta->u_advance.data.link)
			free(sta->u_advance.data.link);
		if (sta->u_advance.data.rate)
			free(sta->u_advance.data.rate);
		if (sta->u_advance.data.nawds)
			free(sta->u_advance.data.nawds);
		break;
	case STATS_TYPE_CTRL:
		if (sta->u_advance.ctrl.tx)
			free(sta->u_advance.ctrl.tx);
		if (sta->u_advance.ctrl.rx)
			free(sta->u_advance.ctrl.rx);
		if (sta->u_advance.ctrl.twt)
			free(sta->u_advance.ctrl.twt);
		if (sta->u_advance.ctrl.link)
			free(sta->u_advance.ctrl.link);
		if (sta->u_advance.ctrl.rate)
			free(sta->u_advance.ctrl.rate);
		break;
	default:
		STATS_ERR("Unexpected Type %d!\n", type);
	}
}

static void free_advance_vap(struct stats_vap *vap, enum stats_type_e type)
{
	switch (type) {
	case STATS_TYPE_DATA:
		if (vap->u_advance.data.tx)
			free(vap->u_advance.data.tx);
		if (vap->u_advance.data.rx)
			free(vap->u_advance.data.rx);
		if (vap->u_advance.data.me)
			free(vap->u_advance.data.me);
		if (vap->u_advance.data.raw)
			free(vap->u_advance.data.raw);
		if (vap->u_advance.data.tso)
			free(vap->u_advance.data.tso);
		if (vap->u_advance.data.igmp)
			free(vap->u_advance.data.igmp);
		if (vap->u_advance.data.mesh)
			free(vap->u_advance.data.mesh);
		if (vap->u_advance.data.nawds)
			free(vap->u_advance.data.nawds);
		break;
	case STATS_TYPE_CTRL:
		if (vap->u_advance.ctrl.tx)
			free(vap->u_advance.ctrl.tx);
		if (vap->u_advance.ctrl.rx)
			free(vap->u_advance.ctrl.rx);
		break;
	default:
		STATS_ERR("Unexpected Type %d!\n", type);
	}
}

static void free_advance_radio(struct stats_radio *ra, enum stats_type_e type)
{
	switch (type) {
	case STATS_TYPE_DATA:
		if (ra->u_advance.data.tx)
			free(ra->u_advance.data.tx);
		if (ra->u_advance.data.rx)
			free(ra->u_advance.data.rx);
		if (ra->u_advance.data.me)
			free(ra->u_advance.data.me);
		if (ra->u_advance.data.raw)
			free(ra->u_advance.data.raw);
		if (ra->u_advance.data.tso)
			free(ra->u_advance.data.tso);
		if (ra->u_advance.data.igmp)
			free(ra->u_advance.data.igmp);
		if (ra->u_advance.data.mesh)
			free(ra->u_advance.data.mesh);
		if (ra->u_advance.data.nawds)
			free(ra->u_advance.data.nawds);
		break;
	case STATS_TYPE_CTRL:
		if (ra->u_advance.ctrl.tx)
			free(ra->u_advance.ctrl.tx);
		if (ra->u_advance.ctrl.rx)
			free(ra->u_advance.ctrl.rx);
		if (ra->u_advance.ctrl.link)
			free(ra->u_advance.ctrl.link);
		break;
	default:
		STATS_ERR("Unexpected Type %d!\n", type);
	}
}

static void free_advance_ap(struct stats_ap *ap, enum stats_type_e type)
{
	switch (type) {
	case STATS_TYPE_DATA:
		if (ap->u.a_data.tx)
			free(ap->u.a_data.tx);
		if (ap->u.a_data.rx)
			free(ap->u.a_data.rx);
		break;
	case STATS_TYPE_CTRL:
		break;
	default:
		STATS_ERR("Unexpected Type %d!\n", type);
	}
}

static void aggr_advance_pdev_ctrl(struct advance_pdev_ctrl *pdev_ctrl,
				   struct advance_vdev_ctrl *vdev_ctrl)
{
	if (pdev_ctrl->tx && vdev_ctrl->tx)
		pdev_ctrl->tx->cs_tx_beacon +=
					vdev_ctrl->tx->cs_tx_bcn_success +
					vdev_ctrl->tx->cs_tx_bcn_outage;
}

static void aggregate_radio_stats(struct reply_buffer *reply,
				  enum stats_type_e type)
{
	struct stats_radio *radio;
	struct stats_vap *vap;

	if (!reply)
		return;

	radio = reply->obj_list.radio_list;
	while (radio) {
		vap = radio->mgr.child_root;
		while (vap) {
			if (vap->mgr.parent != radio)
				break;
			switch (type) {
			case STATS_TYPE_DATA:
				break;
			case STATS_TYPE_CTRL:
				aggr_advance_pdev_ctrl(&radio->u_advance.ctrl,
						       &vap->u_advance.ctrl);
				break;
			}
			vap = vap->mgr.next;
		}
		radio = radio->mgr.next;
	}
}

static void libstats_accumulate(struct stats_command *cmd)
{
	if (!cmd->recursive)
		return;

	/**
	 * Aggregation should be handled in each lower node of hierarchy first
	 * and then in top node. That should be handled in each object level
	 * aggreagation functions.
	 *
	 * In STA, aggregation is not required. So, treated as default case.
	 **/
	switch (cmd->obj) {
	case STATS_OBJ_AP:
		break;
	case STATS_OBJ_RADIO:
		aggregate_radio_stats(cmd->reply, cmd->type);
		break;
	case STATS_OBJ_VAP:
		break;
	case STATS_OBJ_STA:
	default:
		break;
	}
}
#endif /* WLAN_ADVANCE_TELEMETRY */

static void free_sta(struct stats_sta *sta, struct stats_command *cmd)
{
	switch (cmd->lvl) {
	case STATS_LVL_BASIC:
		free_basic_sta(sta, cmd->type);
		break;
#if WLAN_ADVANCE_TELEMETRY
	case STATS_LVL_ADVANCE:
		free_advance_sta(sta, cmd->type);
		break;
#endif /* WLAN_ADVANCE_TELEMETRY */
#if WLAN_DEBUG_TELEMETRY
	case STATS_LVL_DEBUG:
		break;
#endif /* WLAN_DEBUG_TELEMETRY */
	default:
		STATS_ERR("Unexpected Level %d!\n", cmd->lvl);
	}
	free(sta);
}

static void free_vap(struct stats_vap *vap, struct stats_command *cmd)
{
	switch (cmd->lvl) {
	case STATS_LVL_BASIC:
		free_basic_vap(vap, cmd->type);
		break;
#if WLAN_ADVANCE_TELEMETRY
	case STATS_LVL_ADVANCE:
		free_advance_vap(vap, cmd->type);
		break;
#endif /* WLAN_ADVANCE_TELEMETRY */
#if WLAN_DEBUG_TELEMETRY
	case STATS_LVL_DEBUG:
		break;
#endif /* WLAN_DEBUG_TELEMETRY */
	default:
		STATS_ERR("Unexpected Level %d!\n", cmd->lvl);
	}
	free(vap);
}

static void free_radio(struct stats_radio *radio, struct stats_command *cmd)
{
	switch (cmd->lvl) {
	case STATS_LVL_BASIC:
		free_basic_radio(radio, cmd->type);
		break;
#if WLAN_ADVANCE_TELEMETRY
	case STATS_LVL_ADVANCE:
		free_advance_radio(radio, cmd->type);
		break;
#endif /* WLAN_ADVANCE_TELEMETRY */
#if WLAN_DEBUG_TELEMETRY
	case STATS_LVL_DEBUG:
		break;
#endif /* WLAN_DEBUG_TELEMETRY */
	default:
		STATS_ERR("Unexpected Level %d!\n", cmd->lvl);
	}
	free(radio);
}

static void free_ap(struct stats_ap *ap, struct stats_command *cmd)
{
	switch (cmd->lvl) {
	case STATS_LVL_BASIC:
		free_basic_ap(ap, cmd->type);
		break;
#if WLAN_ADVANCE_TELEMETRY
	case STATS_LVL_ADVANCE:
		free_advance_ap(ap, cmd->type);
		break;
#endif /* WLAN_ADVANCE_TELEMETRY */
#if WLAN_DEBUG_TELEMETRY
	case STATS_LVL_DEBUG:
		break;
#endif /* WLAN_DEBUG_TELEMETRY */
	default:
		STATS_ERR("Unexpected Level %d!\n", cmd->lvl);
	}
	free(ap);
}

void libstats_free_reply_buffer(struct stats_command *cmd)
{
	struct stats_sta *sta = NULL;
	struct stats_vap *vap = NULL;
	struct stats_radio *radio = NULL;
	struct stats_ap *ap = NULL;
	struct reply_buffer *reply = cmd->reply;

	if (!reply)
		return;
	while (reply->obj_list.sta_list) {
		sta = reply->obj_list.sta_list;
		reply->obj_list.sta_list = sta->mgr.next;
		sta->mgr.next = NULL;
		free_sta(sta, cmd);
	}
	while (reply->obj_list.vap_list) {
		vap = reply->obj_list.vap_list;
		reply->obj_list.vap_list = vap->mgr.next;
		vap->mgr.next = NULL;
		free_vap(vap, cmd);
	}
	while (reply->obj_list.radio_list) {
		radio = reply->obj_list.radio_list;
		reply->obj_list.radio_list = radio->mgr.next;
		radio->mgr.next = NULL;
		free_radio(radio, cmd);
	}
	while (reply->obj_list.ap_list) {
		ap = reply->obj_list.ap_list;
		reply->obj_list.ap_list = ap->mgr.next;
		ap->mgr.next = NULL;
		free_ap(ap, cmd);
	}
	if (reply) {
		free(reply);
		cmd->reply = NULL;
	}
}

u_int64_t libstats_get_feature_flag(char *feat_flags)
{
	u_int64_t feats = 0;
	u_int8_t index = 0;
	char *flag = NULL;
	bool found = false;

	flag = strtok_r(feat_flags, ",", &feat_flags);
	while (flag) {
		found = false;
		for (index = 0; g_feat[index].name; index++) {
			if (!strncmp(flag, g_feat[index].name, strlen(flag)) &&
			    (strlen(flag) == strlen(g_feat[index].name))) {
				found = true;
				/* ALL is specified in Feature list */
				if (!index)
					return STATS_FEAT_FLG_ALL;
				feats |= g_feat[index].id;
				break;
			}
		}
		if (!found)
			STATS_WARN("%s not in supported list!\n", flag);
		flag = strtok_r(NULL, ",", &feat_flags);
	}

	return feats;
}

int32_t libstats_request_handle(struct stats_command *cmd)
{
	int32_t ret = 0;

	if (!cmd) {
		STATS_ERR("Invalid Input\n");
		return -EINVAL;
	}

	ret = libstats_init();
	if (ret)
		return ret;

	ret = libstats_send(cmd);

	libstats_deinit();

#if WLAN_ADVANCE_TELEMETRY
	libstats_accumulate(cmd);
#endif /* WLAN_ADVANCE_TELEMETRY */

	return ret;
}
