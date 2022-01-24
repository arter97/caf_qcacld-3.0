/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
 * Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

static int is_radio_ifname_valid(const char *ifname)
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

static int is_vap_ifname_valid(const char *ifname)
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

static int is_cfg80211_mode_enabled(void)
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

static int stats_lib_socket_init(void)
{
	return init_socket_context(&g_sock_ctx, STATS_NL80211_CMD_SOCK_ID,
				   STATS_NL80211_EVENT_SOCK_ID);
}

static int stats_lib_init(void)
{
	g_sock_ctx.cfg80211 = is_cfg80211_mode_enabled();

	if (!g_sock_ctx.cfg80211) {
		STATS_ERR("CFG80211 mode is disabled!\n");
		return -EINVAL;
	}

	if (stats_lib_socket_init()) {
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

static void stats_lib_deinit(void)
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
		if (!is_radio_ifname_valid(cmd->if_name)) {
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
		if (!is_vap_ifname_valid(cmd->if_name)) {
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

static int32_t prepare_request(struct nl_msg *nlmsg, struct stats_command *cmd)
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

static struct stats_obj *add_stats_obj(struct reply_buffer *reply)
{
	struct stats_obj *obj;

	obj = (struct stats_obj *)malloc(sizeof(struct stats_obj));
	if (!obj) {
		STATS_ERR("Unable to allocate stats_obj!\n");
		return NULL;
	}
	memset(obj, 0, sizeof(struct stats_obj));
	if (!reply->obj_head) {
		reply->obj_head = obj;
		reply->obj_last = obj;
	} else {
		reply->obj_last->next = obj;
		reply->obj_last = obj;
	}

	return obj;
}

static void *parse_basic_sta(struct nlattr *rattr, enum stats_type_e type)
{
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_FEAT_MAX + 1] = {0};
	struct nlattr *attr = NULL;
	struct basic_peer_data *data = NULL;
	struct basic_peer_ctrl *ctrl = NULL;
	void *dest = NULL;

	if (!rattr || nla_parse_nested(tb, QCA_WLAN_VENDOR_ATTR_FEAT_MAX,
				       rattr, g_policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return NULL;
	}
	switch (type) {
	case STATS_TYPE_DATA:
		data = malloc(sizeof(struct basic_peer_data));
		if (!data)
			return NULL;
		memset(data, 0, sizeof(struct basic_peer_data));
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX];
			data->tx = malloc(sizeof(struct basic_peer_data_tx));
			if (data->tx)
				memcpy(data->tx, nla_data(attr),
				       sizeof(struct basic_peer_data_tx));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX];
			data->rx = malloc(sizeof(struct basic_peer_data_rx));
			if (data->rx)
				memcpy(data->rx, nla_data(attr),
				       sizeof(struct basic_peer_data_rx));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_LINK]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_LINK];
			data->link =
				malloc(sizeof(struct basic_peer_data_link));
			if (data->link)
				memcpy(data->link, nla_data(attr),
				       sizeof(struct basic_peer_data_link));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RATE]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_RATE];
			data->rate =
				malloc(sizeof(struct basic_peer_data_rate));
			if (data->rate)
				memcpy(data->rate, nla_data(attr),
				       sizeof(struct basic_peer_data_rate));
		}
		dest = data;
		break;
	case STATS_TYPE_CTRL:
		ctrl = malloc(sizeof(struct basic_peer_ctrl));
		if (!ctrl)
			return NULL;
		memset(ctrl, 0, sizeof(struct basic_peer_ctrl));
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX];
			ctrl->tx = malloc(sizeof(struct basic_peer_ctrl_tx));
			if (ctrl->tx)
				memcpy(ctrl->tx, nla_data(attr),
				       sizeof(struct basic_peer_ctrl_tx));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX];
			ctrl->rx = malloc(sizeof(struct basic_peer_ctrl_rx));
			if (ctrl->rx)
				memcpy(ctrl->rx, nla_data(attr),
				       sizeof(struct basic_peer_ctrl_rx));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_LINK]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_LINK];
			ctrl->link =
				malloc(sizeof(struct basic_peer_ctrl_link));
			if (ctrl->link)
				memcpy(ctrl->link, nla_data(attr),
				       sizeof(struct basic_peer_ctrl_link));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RATE]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_RATE];
			ctrl->rate =
				malloc(sizeof(struct basic_peer_ctrl_rate));
			if (ctrl->rate)
				memcpy(ctrl->rate, nla_data(attr),
				       sizeof(struct basic_peer_ctrl_rate));
		}
		dest = ctrl;
		break;
	}

	return dest;
}

static void *parse_basic_vap(struct nlattr *rattr, enum stats_type_e type)
{
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_FEAT_MAX + 1] = {0};
	struct nlattr *attr = NULL;
	struct basic_vdev_data *data = NULL;
	struct basic_vdev_ctrl *ctrl = NULL;
	void *dest = NULL;

	if (!rattr || nla_parse_nested(tb, QCA_WLAN_VENDOR_ATTR_FEAT_MAX,
				       rattr, g_policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return NULL;
	}

	switch (type) {
	case STATS_TYPE_DATA:
		data = malloc(sizeof(struct basic_vdev_data));
		if (!data)
			return NULL;
		memset(data, 0, sizeof(struct basic_vdev_data));
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX];
			data->tx = malloc(sizeof(struct basic_vdev_data_tx));
			if (data->tx)
				memcpy(data->tx, nla_data(attr),
				       sizeof(struct basic_vdev_data_tx));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX];
			data->rx = malloc(sizeof(struct basic_vdev_data_rx));
			if (data->rx)
				memcpy(data->rx, nla_data(attr),
				       sizeof(struct basic_vdev_data_rx));
		}
		dest = data;
		break;
	case STATS_TYPE_CTRL:
		ctrl = malloc(sizeof(struct basic_vdev_ctrl));
		if (!ctrl)
			return NULL;
		memset(ctrl, 0, sizeof(struct basic_vdev_ctrl));
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX];
			ctrl->tx = malloc(sizeof(struct basic_vdev_ctrl_tx));
			if (ctrl->tx)
				memcpy(ctrl->tx, nla_data(attr),
				       sizeof(struct basic_vdev_ctrl_tx));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX];
			ctrl->rx = malloc(sizeof(struct basic_vdev_ctrl_rx));
			if (ctrl->rx)
				memcpy(ctrl->rx, nla_data(attr),
				       sizeof(struct basic_vdev_ctrl_rx));
		}
		dest = ctrl;
		break;
	}

	return dest;
}

static void *parse_basic_radio(struct nlattr *rattr, enum stats_type_e type)
{
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_FEAT_MAX + 1] = {0};
	struct nlattr *attr = NULL;
	struct basic_pdev_data *data = NULL;
	struct basic_pdev_ctrl *ctrl = NULL;
	void *dest = NULL;

	if (!rattr || nla_parse_nested(tb, QCA_WLAN_VENDOR_ATTR_FEAT_MAX,
				       rattr, g_policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return NULL;
	}
	switch (type) {
	case STATS_TYPE_DATA:
		data = malloc(sizeof(struct basic_pdev_data));
		if (!data)
			return NULL;
		memset(data, 0, sizeof(struct basic_pdev_data));
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX];
			data->tx = malloc(sizeof(struct basic_pdev_data_tx));
			if (data->tx)
				memcpy(data->tx, nla_data(attr),
				       sizeof(struct basic_pdev_data_tx));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX];
			data->rx = malloc(sizeof(struct basic_pdev_data_rx));
			if (data->rx)
				memcpy(data->rx, nla_data(attr),
				       sizeof(struct basic_pdev_data_rx));
		}
		dest = data;
		break;
	case STATS_TYPE_CTRL:
		ctrl = malloc(sizeof(struct basic_pdev_ctrl));
		if (!ctrl)
			return NULL;
		memset(ctrl, 0, sizeof(struct basic_pdev_ctrl));
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX];
			ctrl->tx = malloc(sizeof(struct basic_pdev_ctrl_tx));
			if (ctrl->tx)
				memcpy(ctrl->tx, nla_data(attr),
				       sizeof(struct basic_pdev_ctrl_tx));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX];
			ctrl->rx = malloc(sizeof(struct basic_pdev_ctrl_rx));
			if (ctrl->rx)
				memcpy(ctrl->rx, nla_data(attr),
				       sizeof(struct basic_pdev_ctrl_rx));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_LINK]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_LINK];
			ctrl->link =
				malloc(sizeof(struct basic_pdev_ctrl_link));
			if (ctrl->link)
				memcpy(ctrl->link, nla_data(attr),
				       sizeof(struct basic_pdev_ctrl_link));
		}
		dest = ctrl;
		break;
	}

	return dest;
}

static void *parse_basic_ap(struct nlattr *rattr, enum stats_type_e type)
{
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_FEAT_MAX + 1] = {0};
	struct basic_psoc_data *data = NULL;

	if (!rattr || nla_parse_nested(tb, QCA_WLAN_VENDOR_ATTR_FEAT_MAX,
				       rattr, g_policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return NULL;
	}
	if (type == STATS_TYPE_CTRL)
		return NULL;

	data = malloc(sizeof(struct basic_psoc_data));
	if (!data)
		return NULL;
	memset(data, 0, sizeof(struct basic_psoc_data));

	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
		data->tx = malloc(sizeof(struct basic_psoc_data_tx));
		if (data->tx)
			memcpy(data->tx,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]),
			       sizeof(struct basic_psoc_data_tx));
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
		data->rx = malloc(sizeof(struct basic_psoc_data_rx));
		if (data->rx)
			memcpy(data->rx,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]),
			       sizeof(struct basic_psoc_data_rx));
	}

	return data;
}

#if WLAN_ADVANCE_TELEMETRY
static void *parse_advance_sta(struct nlattr *rattr, enum stats_type_e type)
{
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_FEAT_MAX + 1] = {0};
	struct nlattr *attr = NULL;
	struct advance_peer_data *data = NULL;
	struct advance_peer_ctrl *ctrl = NULL;
	void *dest = NULL;

	if (!rattr || nla_parse_nested(tb, QCA_WLAN_VENDOR_ATTR_FEAT_MAX,
				       rattr, g_policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return NULL;
	}

	switch (type) {
	case STATS_TYPE_DATA:
		data = malloc(sizeof(struct advance_peer_data));
		if (!data)
			return NULL;
		memset(data, 0, sizeof(struct advance_peer_data));
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX];
			data->tx = malloc(sizeof(struct advance_peer_data_tx));
			if (data->tx)
				memcpy(data->tx, nla_data(attr),
				       sizeof(struct advance_peer_data_tx));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX];
			data->rx = malloc(sizeof(struct advance_peer_data_rx));
			if (data->rx)
				memcpy(data->rx, nla_data(attr),
				       sizeof(struct advance_peer_data_rx));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RAW]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_RAW];
			data->raw =
				malloc(sizeof(struct advance_peer_data_raw));
			if (data->raw)
				memcpy(data->raw, nla_data(attr),
				       sizeof(struct advance_peer_data_raw));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_FWD]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_FWD];
			data->fwd =
				malloc(sizeof(struct advance_peer_data_fwd));
			if (data->fwd)
				memcpy(data->fwd, nla_data(attr),
				       sizeof(struct advance_peer_data_fwd));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TWT]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_TWT];
			data->twt =
				malloc(sizeof(struct advance_peer_data_twt));
			if (data->twt)
				memcpy(data->twt, nla_data(attr),
				       sizeof(struct advance_peer_data_twt));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_LINK]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_LINK];
			data->link =
				malloc(sizeof(struct advance_peer_data_link));
			if (data->link)
				memcpy(data->link, nla_data(attr),
				       sizeof(struct advance_peer_data_link));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RATE]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_RATE];
			data->rate =
				malloc(sizeof(struct advance_peer_data_rate));
			if (data->rate)
				memcpy(data->rate, nla_data(attr),
				       sizeof(struct advance_peer_data_rate));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_NAWDS]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_NAWDS];
			data->nawds =
				malloc(sizeof(struct advance_peer_data_nawds));
			if (data->nawds)
				memcpy(data->nawds, nla_data(attr),
				       sizeof(struct advance_peer_data_nawds));
		}
		dest = data;
		break;
	case STATS_TYPE_CTRL:
		ctrl = malloc(sizeof(struct advance_peer_ctrl));
		if (!ctrl)
			return NULL;
		memset(ctrl, 0, sizeof(struct advance_peer_ctrl));
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX];
			ctrl->tx = malloc(sizeof(struct advance_peer_ctrl_tx));
			if (ctrl->tx)
				memcpy(ctrl->tx, nla_data(attr),
				       sizeof(struct advance_peer_ctrl_tx));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX];
			ctrl->rx = malloc(sizeof(struct advance_peer_ctrl_rx));
			if (ctrl->rx)
				memcpy(ctrl->rx, nla_data(attr),
				       sizeof(struct advance_peer_ctrl_rx));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TWT]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_TWT];
			ctrl->twt =
				malloc(sizeof(struct advance_peer_ctrl_twt));
			if (ctrl->twt)
				memcpy(ctrl->twt, nla_data(attr),
				       sizeof(struct advance_peer_ctrl_twt));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_LINK]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_LINK];
			ctrl->link =
				malloc(sizeof(struct advance_peer_ctrl_link));
			if (ctrl->link)
				memcpy(ctrl->link, nla_data(attr),
				       sizeof(struct advance_peer_ctrl_link));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RATE]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_RATE];
			ctrl->rate =
				malloc(sizeof(struct advance_peer_ctrl_rate));
			if (ctrl->rate)
				memcpy(ctrl->rate, nla_data(attr),
				       sizeof(struct advance_peer_ctrl_rate));
		}
		dest = ctrl;
		break;
	}

	return dest;
}

static void *parse_advance_vap(struct nlattr *rattr, enum stats_type_e type)
{
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_FEAT_MAX + 1] = {0};
	struct nlattr *attr = NULL;
	struct advance_vdev_data *data = NULL;
	struct advance_vdev_ctrl *ctrl = NULL;
	void *dest = NULL;

	if (!rattr || nla_parse_nested(tb, QCA_WLAN_VENDOR_ATTR_FEAT_MAX,
				       rattr, g_policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return NULL;
	}

	switch (type) {
	case STATS_TYPE_DATA:
		data = malloc(sizeof(struct advance_vdev_data));
		if (!data)
			return NULL;
		memset(data, 0, sizeof(struct advance_vdev_data));
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX];
			data->tx = malloc(sizeof(struct advance_vdev_data_tx));
			if (data->tx)
				memcpy(data->tx, nla_data(attr),
				       sizeof(struct advance_vdev_data_tx));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX];
			data->rx = malloc(sizeof(struct advance_vdev_data_rx));
			if (data->rx)
				memcpy(data->rx, nla_data(attr),
				       sizeof(struct advance_vdev_data_rx));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_ME]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_ME];
			data->me = malloc(sizeof(struct advance_vdev_data_me));
			if (data->me)
				memcpy(data->me, nla_data(attr),
				       sizeof(struct advance_vdev_data_me));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RAW]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_RAW];
			data->raw =
				malloc(sizeof(struct advance_vdev_data_raw));
			if (data->raw)
				memcpy(data->raw, nla_data(attr),
				       sizeof(struct advance_vdev_data_raw));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TSO]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_TSO];
			data->tso =
				malloc(sizeof(struct advance_vdev_data_tso));
			if (data->tso)
				memcpy(data->tso, nla_data(attr),
				       sizeof(struct advance_vdev_data_tso));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_IGMP]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_IGMP];
			data->igmp =
				malloc(sizeof(struct advance_vdev_data_igmp));
			if (data->igmp)
				memcpy(data->igmp, nla_data(attr),
				       sizeof(struct advance_vdev_data_igmp));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_MESH]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_MESH];
			data->mesh =
				malloc(sizeof(struct advance_vdev_data_mesh));
			if (data->mesh)
				memcpy(data->mesh, nla_data(attr),
				       sizeof(struct advance_vdev_data_mesh));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_NAWDS]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_NAWDS];
			data->nawds =
				malloc(sizeof(struct advance_vdev_data_nawds));
			if (data->nawds)
				memcpy(data->nawds, nla_data(attr),
				       sizeof(struct advance_vdev_data_nawds));
		}
		dest = data;
		break;
	case STATS_TYPE_CTRL:
		ctrl = malloc(sizeof(struct advance_vdev_ctrl));
		if (!ctrl)
			return NULL;
		memset(ctrl, 0, sizeof(struct advance_vdev_ctrl));
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX];
			ctrl->tx = malloc(sizeof(struct advance_vdev_ctrl_tx));
			if (ctrl->tx)
				memcpy(ctrl->tx, nla_data(attr),
				       sizeof(struct advance_vdev_ctrl_tx));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX];
			ctrl->rx = malloc(sizeof(struct advance_vdev_ctrl_rx));
			if (ctrl->rx)
				memcpy(ctrl->rx, nla_data(attr),
				       sizeof(struct advance_vdev_ctrl_rx));
		}
		dest = ctrl;
		break;
	}

	return dest;
}

static void *parse_advance_radio(struct nlattr *rattr, enum stats_type_e type)
{
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_FEAT_MAX + 1] = {0};
	struct nlattr *attr = NULL;
	struct advance_pdev_data *data = NULL;
	struct advance_pdev_ctrl *ctrl = NULL;
	void *dest = NULL;

	if (!rattr || nla_parse_nested(tb, QCA_WLAN_VENDOR_ATTR_FEAT_MAX,
				       rattr, g_policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return NULL;
	}

	switch (type) {
	case STATS_TYPE_DATA:
		data = malloc(sizeof(struct advance_pdev_data));
		if (!data)
			return NULL;
		memset(data, 0, sizeof(struct advance_pdev_data));
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX];
			data->tx = malloc(sizeof(struct advance_pdev_data_tx));
			if (data->tx)
				memcpy(data->tx, nla_data(attr),
				       sizeof(struct advance_pdev_data_tx));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX];
			data->rx = malloc(sizeof(struct advance_pdev_data_rx));
			if (data->rx)
				memcpy(data->rx, nla_data(attr),
				       sizeof(struct advance_pdev_data_rx));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_ME]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_ME];
			data->me = malloc(sizeof(struct advance_pdev_data_me));
			if (data->me)
				memcpy(data->me, nla_data(attr),
				       sizeof(struct advance_pdev_data_me));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RAW]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_RAW];
			data->raw =
				malloc(sizeof(struct advance_pdev_data_raw));
			if (data->raw)
				memcpy(data->raw, nla_data(attr),
				       sizeof(struct advance_pdev_data_raw));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TSO]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_TSO];
			data->tso =
				malloc(sizeof(struct advance_pdev_data_tso));
			if (data->tso)
				memcpy(data->tso, nla_data(attr),
				       sizeof(struct advance_pdev_data_tso));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_IGMP]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_IGMP];
			data->igmp =
				malloc(sizeof(struct advance_pdev_data_igmp));
			if (data->igmp)
				memcpy(data->igmp, nla_data(attr),
				       sizeof(struct advance_pdev_data_igmp));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_MESH]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_MESH];
			data->mesh =
				malloc(sizeof(struct advance_pdev_data_mesh));
			if (data->mesh)
				memcpy(data->mesh, nla_data(attr),
				       sizeof(struct advance_pdev_data_mesh));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_NAWDS]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_NAWDS];
			data->nawds =
				malloc(sizeof(struct advance_pdev_data_nawds));
			if (data->nawds)
				memcpy(data->nawds, nla_data(attr),
				       sizeof(struct advance_pdev_data_nawds));
		}
		dest = data;
		break;
	case STATS_TYPE_CTRL:
		ctrl = malloc(sizeof(struct advance_pdev_ctrl));
		if (!ctrl)
			return NULL;
		memset(ctrl, 0, sizeof(struct advance_pdev_ctrl));
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX];
			ctrl->tx = malloc(sizeof(struct advance_pdev_ctrl_tx));
			if (ctrl->tx)
				memcpy(ctrl->tx, nla_data(attr),
				       sizeof(struct advance_pdev_ctrl_tx));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX];
			ctrl->rx = malloc(sizeof(struct advance_pdev_ctrl_rx));
			if (ctrl->rx)
				memcpy(ctrl->rx, nla_data(attr),
				       sizeof(struct advance_pdev_ctrl_rx));
		}
		if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_LINK]) {
			attr = tb[QCA_WLAN_VENDOR_ATTR_FEAT_LINK];
			ctrl->link =
				malloc(sizeof(struct advance_pdev_ctrl_link));
			if (ctrl->link)
				memcpy(ctrl->link, nla_data(attr),
				       sizeof(struct advance_pdev_ctrl_link));
		}
		dest = ctrl;
		break;
	}

	return dest;
}

static void *parse_advance_ap(struct nlattr *rattr, enum stats_type_e type)
{
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_FEAT_MAX + 1] = {0};
	struct advance_psoc_data *data = NULL;

	if (!rattr || nla_parse_nested(tb, QCA_WLAN_VENDOR_ATTR_FEAT_MAX,
				       rattr, g_policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return NULL;
	}

	if (type == STATS_TYPE_CTRL)
		return NULL;

	data = malloc(sizeof(struct advance_psoc_data));
	if (!data)
		return NULL;
	memset(data, 0, sizeof(struct advance_psoc_data));

	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]) {
		data->tx = malloc(sizeof(struct advance_psoc_data_tx));
		if (data->tx)
			memcpy(data->tx,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_TX]),
			       sizeof(struct advance_psoc_data_tx));
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]) {
		data->rx = malloc(sizeof(struct advance_psoc_data_rx));
		if (data->rx)
			memcpy(data->rx,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_FEAT_RX]),
			       sizeof(struct advance_psoc_data_rx));
	}

	return data;
}
#endif /* WLAN_ADVANCE_TELEMETRY */

static void stats_response_handler(struct cfg80211_data *buffer)
{
	struct stats_command *cmd;
	struct reply_buffer *reply;
	struct nlattr *attr;
	struct stats_obj *obj;
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_STATS_MAX + 1] = {0};
	struct nla_policy policy[QCA_WLAN_VENDOR_ATTR_STATS_MAX] = {
	[QCA_WLAN_VENDOR_ATTR_STATS_LEVEL] = { .type = NLA_U8 },
	[QCA_WLAN_VENDOR_ATTR_STATS_OBJECT] = { .type = NLA_U8 },
	[QCA_WLAN_VENDOR_ATTR_STATS_OBJ_ID] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_STATS_PARENT_IF] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_STATS_TYPE] = { .type = NLA_U8 },
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
	if (nla_parse(tb, QCA_WLAN_VENDOR_ATTR_STATS_MAX,
		      buffer->nl_vendordata, buffer->nl_vendordata_len,
		      policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return;
	}
	if (!tb[QCA_WLAN_VENDOR_ATTR_STATS_LEVEL]) {
		STATS_ERR("NLA Parsing failed for Stats Object\n");
		return;
	}
	if (!tb[QCA_WLAN_VENDOR_ATTR_STATS_OBJECT]) {
		STATS_ERR("NLA Parsing failed for Stats Object\n");
		return;
	}
	if (!tb[QCA_WLAN_VENDOR_ATTR_STATS_OBJ_ID]) {
		STATS_ERR("NLA Parsing failed for Stats Object ID\n");
		return;
	}
	if (!tb[QCA_WLAN_VENDOR_ATTR_STATS_TYPE]) {
		STATS_ERR("NLA Parsing failed for Stats Type\n");
		return;
	}
	if (!tb[QCA_WLAN_VENDOR_ATTR_STATS_RECURSIVE]) {
		STATS_ERR("NLA Parsing failed for Stats Recursive\n");
		return;
	}
	obj = add_stats_obj(reply);
	if (!obj)
		return;
	obj->lvl = nla_get_u8(tb[QCA_WLAN_VENDOR_ATTR_STATS_LEVEL]);
	obj->obj_type = nla_get_u8(tb[QCA_WLAN_VENDOR_ATTR_STATS_OBJECT]);
	obj->type = nla_get_u8(tb[QCA_WLAN_VENDOR_ATTR_STATS_TYPE]);
	if (tb[QCA_WLAN_VENDOR_ATTR_STATS_PARENT_IF])
		strlcpy(obj->pif_name, nla_get_string(
			tb[QCA_WLAN_VENDOR_ATTR_STATS_PARENT_IF]),
			IFNAME_LEN);
	if (tb[QCA_WLAN_VENDOR_ATTR_STATS_OBJ_ID]) {
		if (obj->obj_type == STATS_OBJ_STA)
			memcpy(obj->u_id.mac_addr,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_STATS_OBJ_ID]),
			       ETH_ALEN);
		else
			memcpy(obj->u_id.if_name,
			       nla_data(tb[QCA_WLAN_VENDOR_ATTR_STATS_OBJ_ID]),
			       IFNAME_LEN);
	}
	attr = tb[QCA_WLAN_VENDOR_ATTR_STATS_RECURSIVE];
	if (obj->lvl == STATS_LVL_BASIC) {
		switch (obj->obj_type) {
		case STATS_OBJ_STA:
			obj->stats = parse_basic_sta(attr, obj->type);
			break;
		case STATS_OBJ_VAP:
			obj->stats = parse_basic_vap(attr, obj->type);
			break;
		case STATS_OBJ_RADIO:
			obj->stats = parse_basic_radio(attr, obj->type);
			break;
		case STATS_OBJ_AP:
			obj->stats = parse_basic_ap(attr, obj->type);
			break;
		default:
			STATS_ERR("Unexpected Object\n");
		}
#if WLAN_ADVANCE_TELEMETRY
	} else if (obj->lvl == STATS_LVL_ADVANCE) {
		switch (obj->obj_type) {
		case STATS_OBJ_STA:
			obj->stats = parse_advance_sta(attr, obj->type);
			break;
		case STATS_OBJ_VAP:
			obj->stats = parse_advance_vap(attr, obj->type);
			break;
		case STATS_OBJ_RADIO:
			obj->stats = parse_advance_radio(attr, obj->type);
			break;
		case STATS_OBJ_AP:
			obj->stats = parse_advance_ap(attr, obj->type);
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

static int32_t send_nl_command(struct stats_command *cmd,
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
		ret = prepare_request(nlmsg, cmd);
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

static int32_t process_and_send_stats_request(struct stats_command *cmd)
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
	buffer.callback = stats_response_handler;
	buffer.parse_data = 1;

	if (is_interface_active(cmd->if_name, cmd->obj))
		ret = send_nl_command(cmd, &buffer, cmd->if_name);

	return ret;
}

static void free_basic_sta(struct stats_obj *sta)
{
	switch (sta->type) {
	case STATS_TYPE_DATA:
		{
			struct basic_peer_data *data = sta->stats;

			if (data->tx)
				free(data->tx);
			if (data->rx)
				free(data->rx);
			if (data->link)
				free(data->link);
			if (data->rate)
				free(data->rate);
		}
		break;
	case STATS_TYPE_CTRL:
		{
			struct basic_peer_ctrl *ctrl = sta->stats;

			if (ctrl->tx)
				free(ctrl->tx);
			if (ctrl->rx)
				free(ctrl->rx);
			if (ctrl->link)
				free(ctrl->link);
			if (ctrl->rate)
				free(ctrl->rate);
		}
		break;
	default:
		STATS_ERR("Unexpected Type %d!\n", sta->type);
	}

	free(sta->stats);
}

static void free_basic_vap(struct stats_obj *vap)
{
	switch (vap->type) {
	case STATS_TYPE_DATA:
		{
			struct basic_vdev_data *data = vap->stats;

			if (data->tx)
				free(data->tx);
			if (data->rx)
				free(data->rx);
		}
		break;
	case STATS_TYPE_CTRL:
		{
			struct basic_vdev_ctrl *ctrl = vap->stats;

			if (ctrl->tx)
				free(ctrl->tx);
			if (ctrl->rx)
				free(ctrl->rx);
		}
		break;
	default:
		STATS_ERR("Unexpected Type %d!\n", vap->type);
	}

	free(vap->stats);
}

static void free_basic_radio(struct stats_obj *radio)
{
	switch (radio->type) {
	case STATS_TYPE_DATA:
		{
			struct basic_pdev_data *data = radio->stats;

			if (data->tx)
				free(data->tx);
			if (data->rx)
				free(data->rx);
		}
		break;
	case STATS_TYPE_CTRL:
		{
			struct basic_pdev_ctrl *ctrl = radio->stats;

			if (ctrl->tx)
				free(ctrl->tx);
			if (ctrl->rx)
				free(ctrl->rx);
			if (ctrl->link)
				free(ctrl->link);
		}
		break;
	default:
		STATS_ERR("Unexpected Type %d!\n", radio->type);
	}

	free(radio->stats);
}

static void free_basic_ap(struct stats_obj *ap)
{
	struct basic_psoc_data *data = ap->stats;

	if (!data)
		return;

	if (data->tx)
		free(data->tx);
	if (data->rx)
		free(data->rx);

	free(ap->stats);
}

#if WLAN_ADVANCE_TELEMETRY
static void aggr_advance_pdev_ctrl(struct advance_pdev_ctrl *pdev_ctrl,
				   struct advance_vdev_ctrl *vdev_ctrl)
{
	if (!pdev_ctrl || !vdev_ctrl)
		return;

	if (pdev_ctrl->tx && vdev_ctrl->tx)
		pdev_ctrl->tx->cs_tx_beacon +=
					vdev_ctrl->tx->cs_tx_bcn_success +
					vdev_ctrl->tx->cs_tx_bcn_outage;
}

static void aggregate_advance_stats(struct stats_obj *obj)
{
	struct stats_obj *radio = NULL;

	while (obj) {
		if (obj->obj_type == STATS_OBJ_RADIO)
			radio = obj;

		if ((obj->obj_type == STATS_OBJ_VAP) && (radio)) {
			switch (obj->type) {
			case STATS_TYPE_DATA:
				break;
			case STATS_TYPE_CTRL:
				aggr_advance_pdev_ctrl(radio->stats,
						       obj->stats);
				break;
			}
		}
		obj = obj->next;
	}
}

static void accumulate_advance_stats(struct stats_command *cmd)
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
	case STATS_OBJ_RADIO:
		aggregate_advance_stats(cmd->reply->obj_head);
		break;
	case STATS_OBJ_VAP:
		break;
	case STATS_OBJ_STA:
	default:
		break;
	}
}

static void free_advance_sta(struct stats_obj *sta)
{
	switch (sta->type) {
	case STATS_TYPE_DATA:
		{
			struct advance_peer_data *data = sta->stats;

			if (data->tx)
				free(data->tx);
			if (data->rx)
				free(data->rx);
			if (data->fwd)
				free(data->fwd);
			if (data->raw)
				free(data->raw);
			if (data->twt)
				free(data->twt);
			if (data->link)
				free(data->link);
			if (data->rate)
				free(data->rate);
			if (data->nawds)
				free(data->nawds);
		}
		break;
	case STATS_TYPE_CTRL:
		{
			struct advance_peer_ctrl *ctrl = sta->stats;

			if (ctrl->tx)
				free(ctrl->tx);
			if (ctrl->rx)
				free(ctrl->rx);
			if (ctrl->twt)
				free(ctrl->twt);
			if (ctrl->link)
				free(ctrl->link);
			if (ctrl->rate)
				free(ctrl->rate);
		}
		break;
	default:
		STATS_ERR("Unexpected Type %d!\n", sta->type);
	}

	free(sta->stats);
}

static void free_advance_vap(struct stats_obj *vap)
{
	switch (vap->type) {
	case STATS_TYPE_DATA:
		{
			struct advance_vdev_data *data = vap->stats;

			if (data->tx)
				free(data->tx);
			if (data->rx)
				free(data->rx);
			if (data->me)
				free(data->me);
			if (data->raw)
				free(data->raw);
			if (data->tso)
				free(data->tso);
			if (data->igmp)
				free(data->igmp);
			if (data->mesh)
				free(data->mesh);
			if (data->nawds)
				free(data->nawds);
		}
		break;
	case STATS_TYPE_CTRL:
		{
			struct advance_vdev_ctrl *ctrl = vap->stats;

			if (ctrl->tx)
				free(ctrl->tx);
			if (ctrl->rx)
				free(ctrl->rx);
		}
		break;
	default:
		STATS_ERR("Unexpected Type %d!\n", vap->type);
	}

	free(vap->stats);
}

static void free_advance_radio(struct stats_obj *radio)
{
	switch (radio->type) {
	case STATS_TYPE_DATA:
		{
			struct advance_pdev_data *data = radio->stats;

			if (data->tx)
				free(data->tx);
			if (data->rx)
				free(data->rx);
			if (data->me)
				free(data->me);
			if (data->raw)
				free(data->raw);
			if (data->tso)
				free(data->tso);
			if (data->igmp)
				free(data->igmp);
			if (data->mesh)
				free(data->mesh);
			if (data->nawds)
				free(data->nawds);
		}
		break;
	case STATS_TYPE_CTRL:
		{
			struct advance_pdev_ctrl *ctrl = radio->stats;

			if (ctrl->tx)
				free(ctrl->tx);
			if (ctrl->rx)
				free(ctrl->rx);
			if (ctrl->link)
				free(ctrl->link);
		}
		break;
	default:
		STATS_ERR("Unexpected Type %d!\n", radio->type);
	}

	free(radio->stats);
}

static void free_advance_ap(struct stats_obj *ap)
{
	struct advance_psoc_data *data = ap->stats;

	if (!data)
		return;

	if (data->tx)
		free(data->tx);
	if (data->rx)
		free(data->rx);

	free(ap->stats);
}
#endif /* WLAN_ADVANCE_TELEMETRY */

static void free_sta(struct stats_obj *sta)
{
	switch (sta->lvl) {
	case STATS_LVL_BASIC:
		free_basic_sta(sta);
		break;
#if WLAN_ADVANCE_TELEMETRY
	case STATS_LVL_ADVANCE:
		free_advance_sta(sta);
		break;
#endif /* WLAN_ADVANCE_TELEMETRY */
#if WLAN_DEBUG_TELEMETRY
	case STATS_LVL_DEBUG:
		break;
#endif /* WLAN_DEBUG_TELEMETRY */
	default:
		STATS_ERR("Unexpected Level %d!\n", sta->lvl);
	}
	free(sta);
}

static void free_vap(struct stats_obj *vap)
{
	switch (vap->lvl) {
	case STATS_LVL_BASIC:
		free_basic_vap(vap);
		break;
#if WLAN_ADVANCE_TELEMETRY
	case STATS_LVL_ADVANCE:
		free_advance_vap(vap);
		break;
#endif /* WLAN_ADVANCE_TELEMETRY */
#if WLAN_DEBUG_TELEMETRY
	case STATS_LVL_DEBUG:
		break;
#endif /* WLAN_DEBUG_TELEMETRY */
	default:
		STATS_ERR("Unexpected Level %d!\n", vap->lvl);
	}
	free(vap);
}

static void free_radio(struct stats_obj *radio)
{
	switch (radio->lvl) {
	case STATS_LVL_BASIC:
		free_basic_radio(radio);
		break;
#if WLAN_ADVANCE_TELEMETRY
	case STATS_LVL_ADVANCE:
		free_advance_radio(radio);
		break;
#endif /* WLAN_ADVANCE_TELEMETRY */
#if WLAN_DEBUG_TELEMETRY
	case STATS_LVL_DEBUG:
		break;
#endif /* WLAN_DEBUG_TELEMETRY */
	default:
		STATS_ERR("Unexpected Level %d!\n", radio->lvl);
	}
	free(radio);
}

static void free_ap(struct stats_obj *ap)
{
	switch (ap->lvl) {
	case STATS_LVL_BASIC:
		free_basic_ap(ap);
		break;
#if WLAN_ADVANCE_TELEMETRY
	case STATS_LVL_ADVANCE:
		free_advance_ap(ap);
		break;
#endif /* WLAN_ADVANCE_TELEMETRY */
#if WLAN_DEBUG_TELEMETRY
	case STATS_LVL_DEBUG:
		break;
#endif /* WLAN_DEBUG_TELEMETRY */
	default:
		STATS_ERR("Unexpected Level %d!\n", ap->lvl);
	}
	free(ap);
}

void libstats_free_reply_buffer(struct stats_command *cmd)
{
	struct stats_obj *obj;
	struct reply_buffer *reply = cmd->reply;

	if (!reply || !reply->obj_head)
		return;

	while (reply->obj_head) {
		obj = reply->obj_head;
		reply->obj_head = obj->next;
		switch (obj->obj_type) {
		case STATS_OBJ_STA:
			free_sta(obj);
			break;
		case STATS_OBJ_VAP:
			free_vap(obj);
			break;
		case STATS_OBJ_RADIO:
			free_radio(obj);
			break;
		case STATS_OBJ_AP:
			free_ap(obj);
			break;
		}
	}
	reply->obj_last = NULL;
	free(reply);
	cmd->reply = NULL;
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

	ret = stats_lib_init();
	if (ret)
		return ret;

	ret = process_and_send_stats_request(cmd);

	stats_lib_deinit();

#if WLAN_ADVANCE_TELEMETRY
	accumulate_advance_stats(cmd);
#endif /* WLAN_ADVANCE_TELEMETRY */

	return ret;
}
