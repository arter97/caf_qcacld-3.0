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

/* This path is for network interfaces */
#define PATH_SYSNET_DEV              "/sys/class/net/"

#define MAX_IF_NUM                     3

/* NL80211 command and event socket IDs */
#define STATS_NL80211_CMD_SOCK_ID    DEFAULT_NL80211_CMD_SOCK_ID
#define STATS_NL80211_EVENT_SOCK_ID  DEFAULT_NL80211_EVENT_SOCK_ID

#define STATS_BASIC_AP_CTRL_MASK       0
#define STATS_BASIC_AP_DATA_MASK       (STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX)
#define STATS_BASIC_RADIO_CTRL_MASK                    \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_LINK)
#define STATS_BASIC_RADIO_DATA_MASK    (STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX)
#define STATS_BASIC_VAP_CTRL_MASK      (STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX)
#define STATS_BASIC_VAP_DATA_MASK      (STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX)
#define STATS_BASIC_STA_CTRL_MASK                      \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_LINK | STATS_FEAT_FLG_RATE)
#define STATS_BASIC_STA_DATA_MASK                      \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_LINK | STATS_FEAT_FLG_RATE)

#define STATS_ADVANCE_AP_CTRL_MASK                     \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_LINK | STATS_FEAT_FLG_RATE)
#define STATS_ADVANCE_AP_DATA_MASK                     \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_LINK | STATS_FEAT_FLG_RATE)
#define STATS_ADVANCE_RADIO_CTRL_MASK                  \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_LINK)
#define STATS_ADVANCE_RADIO_DATA_MASK                  \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_RAW | STATS_FEAT_FLG_TSO |     \
	 STATS_FEAT_FLG_VOW)
#define STATS_ADVANCE_VAP_CTRL_MASK                    \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX)
#define STATS_ADVANCE_VAP_DATA_MASK                    \
	(STATS_FEAT_FLG_ME | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_RAW | STATS_FEAT_FLG_TSO |     \
	 STATS_FEAT_FLG_IGMP | STATS_FEAT_FLG_MESH |   \
	 STATS_FEAT_FLG_NAWDS)
#define STATS_ADVANCE_STA_CTRL_MASK                    \
	(STATS_FEAT_FLG_TX | STATS_FEAT_FLG_TWT)
#define STATS_ADVANCE_STA_DATA_MASK                    \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_FWD | STATS_FEAT_FLG_TWT |     \
	 STATS_FEAT_FLG_RAW | STATS_FEAT_FLG_RDK |     \
	 STATS_FEAT_FLG_NAWDS | STATS_FEAT_FLG_DELAY | \
	 STATS_FEAT_FLG_LINK | STATS_FEAT_FLG_RATE)

#define STATS_DEBUG_AP_CTRL_MASK                       \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_LINK | STATS_FEAT_FLG_RATE)
#define STATS_DEBUG_AP_DATA_MASK                       \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_LINK | STATS_FEAT_FLG_RATE)
#define STATS_DEBUG_RADIO_CTRL_MASK                    \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_LINK)
#define STATS_DEBUG_RADIO_DATA_MASK    (STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX)
#define STATS_DEBUG_VAP_CTRL_MASK      (STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX)
#define STATS_DEBUG_VAP_DATA_MASK      (STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX)
#define STATS_DEBUG_STA_CTRL_MASK                      \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_LINK | STATS_FEAT_FLG_RATE)
#define STATS_DEBUG_STA_DATA_MASK                      \
	(STATS_FEAT_FLG_RX | STATS_FEAT_FLG_TX |       \
	 STATS_FEAT_FLG_LINK | STATS_FEAT_FLG_RATE)

#define SET_FLAG(_flg, _mask)              \
	do {                               \
		if (!(_flg))               \
			(_flg) = (_mask);  \
		else                       \
			(_flg) &= (_mask); \
	} while (0)

/* Global cfg80211 flag. For cfg80211 mode set to 1 during init */
static int g_cfg_flag;
/* Global socket context to create nl80211 command and event interface */
static struct socket_context g_sock_ctx = {0};

/**
 * struct feat_parser_t: Defines feature name and corresponding ID
 * @name: Name of the feature passed from userspace
 * @id:   Feature flag corresponding to name
 */
struct feat_parser_t {
	char *name;
	u_int32_t id;
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

static char *g_ifnames[MAX_IF_NUM] = { "wifi0", "wifi1", "wifi2" };

static char *macaddr_to_str(u_int8_t *addr)
{
	static char string[3 * ETHER_ADDR_LEN];

	memset(string, 0, sizeof(string));
	if (addr) {
		snprintf(string, sizeof(string),
			 "%02x:%02x:%02x:%02x:%02x:%02x",
			 addr[0], addr[1], addr[2],
			 addr[3], addr[4], addr[5]);
	}

	return string;
}

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

	for (i = 0; i < MAX_IF_NUM; i++) {
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
	g_cfg_flag = libstats_is_cfg80211_mode_enabled();
	g_sock_ctx.cfg80211 = g_cfg_flag;

	if (!g_cfg_flag) {
		STATS_ERR("CFG80211 mode is disabled!\n");
		return -EINVAL;
	}

	if (libstats_socket_init()) {
		STATS_ERR("Failed to initialise socket\n");
		return -EINVAL;
	}

	return 0;
}

static void libstats_deinit(void)
{
	destroy_socket_context(&g_sock_ctx);
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
	}
}

static int is_valid_cmd(struct stats_command *cmd)
{
	u_int8_t *sta_mac = cmd->sta_mac.ether_addr_octet;

	switch (cmd->obj) {
	case STATS_OBJ_AP:
		if (cmd->if_name[0])
			STATS_WARN("Ignore Interface Name input for AP\n");
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
		if (sta_mac[0] != '\0') {
			STATS_ERR("STA MAC address not configured.\n");
			return -EINVAL;
		}
		if (cmd->if_name[0])
			STATS_WARN("Ignore Interface name input for STA\n");
		if (cmd->recursive)
			STATS_WARN("Ignore recursive display for STA\n");
		cmd->recursive = 0;
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
	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_TELEMETRIC_RECURSIVE,
		       cmd->recursive)) {
		STATS_ERR("failed to put recursive flag\n");
		return -EIO;
	}
	if (nla_put_u32(nlmsg, QCA_WLAN_VENDOR_ATTR_TELEMETRIC_FEATURE_FLAG,
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

#define UPDATE_REPLY(_inx, _len, _type, _size, _obj)   \
	do {                                           \
		reply->db[(_inx)].index = (_len);      \
		reply->db[(_inx)].type = (_type);      \
		reply->db[(_inx)].obj = (_obj);        \
		(_len) += (_size);                     \
		reply->length = (_len);                \
		(_inx)++;                              \
		reply->db_count++;                     \
	} while (0)

static void libstats_parse_basic_sta(struct cfg80211_data *buffer,
				     enum stats_type_e type,
				     bool recursive,
				     struct nlattr *rattr,
				     struct reply_buffer *reply)
{
	u_int32_t len = 0;
	u_int32_t index = 0;
	u_int32_t size = 0;
	u_int8_t *mac = NULL;
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_BASIC_PEER_FEAT_MAX + 1] = {0};
	struct nla_policy policy[QCA_WLAN_VENDOR_ATTR_BASIC_PEER_FEAT_MAX] = {
	[QCA_WLAN_VENDOR_ATTR_BASIC_PEER_MAC_ADDR] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_BASIC_PEER_FEAT_TX] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_BASIC_PEER_FEAT_RX] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_BASIC_PEER_FEAT_LINK] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_BASIC_PEER_FEAT_RATE] = { .type = NLA_UNSPEC },
	};

	if (!reply) {
		STATS_ERR("Invalid reply buffer\n");
		return;
	}
	len = reply->length;
	index = reply->db_count;
	if (recursive && rattr) {
		if (nla_parse(tb, QCA_WLAN_VENDOR_ATTR_BASIC_PEER_FEAT_MAX,
			      nla_data(rattr), nla_len(rattr), NULL)) {
			STATS_ERR("NLA Recursive Parsing failed\n");
			return;
		}
	} else if (nla_parse(tb, QCA_WLAN_VENDOR_ATTR_BASIC_PEER_FEAT_MAX,
			     buffer->nl_vendordata, buffer->nl_vendordata_len,
			     policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return;
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_BASIC_PEER_MAC_ADDR]) {
		mac = nla_data(tb[QCA_WLAN_VENDOR_ATTR_BASIC_PEER_MAC_ADDR]);
		memcpy(&reply->data[len], mac, ETH_ALEN);
		size = ETH_ALEN;
		UPDATE_REPLY(index, len, STATS_OBJECT_ID, size, STATS_OBJ_STA);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_BASIC_PEER_FEAT_TX]) {
		if (type == STATS_TYPE_DATA)
			size = sizeof(struct basic_peer_data_tx);
		else
			size = sizeof(struct basic_peer_ctrl_tx);
		memcpy(&reply->data[len],
		       nla_data(tb[QCA_WLAN_VENDOR_ATTR_BASIC_PEER_FEAT_TX]),
		       size);
		UPDATE_REPLY(index, len, STATS_FEAT_TX, size, STATS_OBJ_STA);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_BASIC_PEER_FEAT_RX]) {
		if (type == STATS_TYPE_DATA)
			size = sizeof(struct basic_peer_data_rx);
		else
			size = sizeof(struct basic_peer_ctrl_rx);
		memcpy(&reply->data[len],
		       nla_data(tb[QCA_WLAN_VENDOR_ATTR_BASIC_PEER_FEAT_RX]),
		       size);
		UPDATE_REPLY(index, len, STATS_FEAT_RX, size, STATS_OBJ_STA);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_BASIC_PEER_FEAT_LINK]) {
		if (type == STATS_TYPE_DATA)
			size = sizeof(struct basic_peer_data_link);
		else
			size = sizeof(struct basic_peer_ctrl_link);
		memcpy(&reply->data[len],
		       nla_data(tb[QCA_WLAN_VENDOR_ATTR_BASIC_PEER_FEAT_LINK]),
		       size);
		UPDATE_REPLY(index, len, STATS_FEAT_LINK, size, STATS_OBJ_STA);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_BASIC_PEER_FEAT_RATE]) {
		if (type == STATS_TYPE_DATA)
			size = sizeof(struct basic_peer_data_rate);
		else
			size = sizeof(struct basic_peer_ctrl_rate);
		memcpy(&reply->data[len],
		       nla_data(tb[QCA_WLAN_VENDOR_ATTR_BASIC_PEER_FEAT_RATE]),
		       size);
		UPDATE_REPLY(index, len, STATS_FEAT_RATE, size, STATS_OBJ_STA);
	}
}

static void libstats_parse_basic_vap(struct cfg80211_data *buffer,
				     enum stats_type_e type,
				     bool recursive,
				     struct nlattr *rattr,
				     struct reply_buffer *reply)
{
	u_int32_t len = 0;
	u_int32_t index = 0;
	u_int32_t size = 0;
	int tmp = 0;
	struct nlattr *attr = NULL;
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_BASIC_VDEV_FEAT_MAX + 1] = {0};
	struct nla_policy policy[QCA_WLAN_VENDOR_ATTR_BASIC_VDEV_FEAT_MAX] = {
	[QCA_WLAN_VENDOR_ATTR_BASIC_VDEV_IF_NAME] = { .type = NLA_STRING },
	[QCA_WLAN_VENDOR_ATTR_BASIC_VDEV_FEAT_TX] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_BASIC_VDEV_FEAT_RX] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_BASIC_VDEV_R_PEER] = { .type = NLA_NESTED },
	};

	if (!reply) {
		STATS_ERR("Invalid reply buffer\n");
		return;
	}
	len = reply->length;
	index = reply->db_count;
	if (recursive && rattr) {
		if (nla_parse(tb, QCA_WLAN_VENDOR_ATTR_BASIC_VDEV_FEAT_MAX,
			      nla_data(rattr), nla_len(rattr), NULL)) {
			STATS_ERR("NLA Recursive Parsing failed\n");
			return;
		}
	} else if (nla_parse(tb, QCA_WLAN_VENDOR_ATTR_BASIC_VDEV_FEAT_MAX,
			     buffer->nl_vendordata, buffer->nl_vendordata_len,
			     policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return;
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_BASIC_VDEV_IF_NAME]) {
		size = IFNAMSIZ;
		memcpy(&reply->data[len],
		       nla_get_string(
		       tb[QCA_WLAN_VENDOR_ATTR_BASIC_VDEV_IF_NAME]),
		       size);
		UPDATE_REPLY(index, len, STATS_OBJECT_ID, size, STATS_OBJ_VAP);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_BASIC_VDEV_FEAT_TX]) {
		if (type == STATS_TYPE_DATA)
			size = sizeof(struct basic_vdev_data_tx);
		else
			size = sizeof(struct basic_vdev_ctrl_tx);
		memcpy(&reply->data[len],
		       nla_data(tb[QCA_WLAN_VENDOR_ATTR_BASIC_VDEV_FEAT_TX]),
		       size);
		UPDATE_REPLY(index, len, STATS_FEAT_TX, size, STATS_OBJ_VAP);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_BASIC_VDEV_FEAT_RX]) {
		if (type == STATS_TYPE_DATA)
			size = sizeof(struct basic_vdev_data_rx);
		else
			size = sizeof(struct basic_vdev_ctrl_rx);
		memcpy(&reply->data[len],
		       nla_data(tb[QCA_WLAN_VENDOR_ATTR_BASIC_VDEV_FEAT_RX]),
		       size);
		UPDATE_REPLY(index, len, STATS_FEAT_RX, size, STATS_OBJ_VAP);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_BASIC_VDEV_R_PEER]) {
		UPDATE_REPLY(index, len, STATS_RECURSIVE, 0, STATS_OBJ_VAP);
		nla_for_each_nested(attr,
				    tb[QCA_WLAN_VENDOR_ATTR_BASIC_VDEV_R_PEER],
				    tmp) {
			libstats_parse_basic_sta(buffer, type, recursive,
						 attr, reply);
		}
	}
}

static void libstats_parse_basic_radio(struct cfg80211_data *buffer,
				       enum stats_type_e type,
				       bool recursive,
				       struct nlattr *rattr,
				       struct reply_buffer *reply)
{
	u_int32_t len = 0;
	u_int32_t index = 0;
	u_int32_t size = 0;
	int tmp = 0;
	struct nlattr *attr = NULL;
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_BASIC_PDEV_FEAT_MAX + 1] = {0};
	struct nla_policy policy[QCA_WLAN_VENDOR_ATTR_BASIC_PDEV_FEAT_MAX] = {
	[QCA_WLAN_VENDOR_ATTR_BASIC_PDEV_ID] = { .type = NLA_U8 },
	[QCA_WLAN_VENDOR_ATTR_BASIC_PDEV_FEAT_TX] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_BASIC_PDEV_FEAT_RX] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_BASIC_PDEV_FEAT_LINK] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_BASIC_PDEV_R_VDEV] = { .type = NLA_NESTED },
	};

	if (!reply) {
		STATS_ERR("Invalid reply buffer\n");
		return;
	}
	len = reply->length;
	index = reply->db_count;
	if (recursive && rattr) {
		if (nla_parse(tb, QCA_WLAN_VENDOR_ATTR_BASIC_PDEV_FEAT_MAX,
			      nla_data(rattr), nla_len(rattr), NULL)) {
			STATS_ERR("NLA Recursive Parsing failed\n");
			return;
		}
	} else if (nla_parse(tb, QCA_WLAN_VENDOR_ATTR_BASIC_PDEV_FEAT_MAX,
			     buffer->nl_vendordata, buffer->nl_vendordata_len,
			     policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return;
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_BASIC_PDEV_ID]) {
		reply->data[len] =
			nla_get_u8(tb[QCA_WLAN_VENDOR_ATTR_BASIC_PDEV_ID]);
		size = sizeof(u_int8_t);
		UPDATE_REPLY(index, len, STATS_OBJECT_ID, size,
			     STATS_OBJ_RADIO);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_BASIC_PDEV_FEAT_TX]) {
		if (type == STATS_TYPE_DATA)
			size = sizeof(struct basic_pdev_data_tx);
		else
			size = sizeof(struct basic_pdev_ctrl_tx);
		memcpy(&reply->data[len],
		       nla_data(tb[QCA_WLAN_VENDOR_ATTR_BASIC_PDEV_FEAT_TX]),
		       size);
		UPDATE_REPLY(index, len, STATS_FEAT_TX, size, STATS_OBJ_RADIO);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_BASIC_PDEV_FEAT_RX]) {
		if (type == STATS_TYPE_DATA)
			size = sizeof(struct basic_pdev_data_rx);
		else
			size = sizeof(struct basic_pdev_ctrl_rx);
		memcpy(&reply->data[len],
		       nla_data(tb[QCA_WLAN_VENDOR_ATTR_BASIC_PDEV_FEAT_RX]),
		       size);
		UPDATE_REPLY(index, len, STATS_FEAT_RX, size, STATS_OBJ_RADIO);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_BASIC_PDEV_FEAT_LINK]) {
		if (type == STATS_TYPE_CTRL)
			size = sizeof(struct basic_pdev_ctrl_rx);
		memcpy(&reply->data[len],
		       nla_data(tb[QCA_WLAN_VENDOR_ATTR_BASIC_PDEV_FEAT_LINK]),
		       size);
		UPDATE_REPLY(index, len, STATS_FEAT_LINK,
			     size, STATS_OBJ_RADIO);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_BASIC_PDEV_R_VDEV]) {
		UPDATE_REPLY(index, len, STATS_RECURSIVE, 0, STATS_OBJ_RADIO);
		nla_for_each_nested(attr,
				    tb[QCA_WLAN_VENDOR_ATTR_BASIC_PDEV_R_VDEV],
				    tmp) {
			libstats_parse_basic_vap(buffer, type, recursive,
						 attr, reply);
		}
	}
}

static void libstats_parse_basic_ap(struct cfg80211_data *buffer,
				    enum stats_type_e type,
				    bool recursive,
				    struct reply_buffer *reply)
{
	u_int32_t len = 0;
	u_int32_t index = 0;
	u_int32_t size = 0;
	int tmp = 0;
	struct nlattr *attr = NULL;
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_BASIC_PSOC_FEAT_MAX + 1] = {0};
	struct nla_policy policy[QCA_WLAN_VENDOR_ATTR_BASIC_PSOC_FEAT_MAX] = {
	[QCA_WLAN_VENDOR_ATTR_BASIC_PSOC_FEAT_TX] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_BASIC_PSOC_FEAT_RX] = { .type = NLA_UNSPEC },
	[QCA_WLAN_VENDOR_ATTR_BASIC_PSOC_R_PDEV] = { .type = NLA_NESTED },
	};

	if (!reply) {
		STATS_ERR("Invalid reply buffer\n");
		return;
	}
	len = reply->length;
	index = reply->db_count;
	if (nla_parse(tb, QCA_WLAN_VENDOR_ATTR_BASIC_PSOC_FEAT_MAX,
		      buffer->nl_vendordata, buffer->nl_vendordata_len,
		      policy)) {
		STATS_ERR("NLA Parsing failed\n");
		return;
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_BASIC_PSOC_FEAT_TX]) {
		if (type == STATS_TYPE_DATA) {
			size = sizeof(struct basic_psoc_data_tx);
		} else {
			STATS_ERR("Invalid type!");
			return;
		}
		memcpy(&reply->data[len],
		       nla_data(tb[QCA_WLAN_VENDOR_ATTR_BASIC_PSOC_FEAT_TX]),
		       size);
		UPDATE_REPLY(index, len, STATS_FEAT_TX, size, STATS_OBJ_AP);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_BASIC_PSOC_FEAT_RX]) {
		if (type == STATS_TYPE_DATA) {
			size = sizeof(struct basic_psoc_data_rx);
		} else {
			STATS_ERR("Invalid type!");
			return;
		}
		memcpy(&reply->data[len],
		       nla_data(tb[QCA_WLAN_VENDOR_ATTR_BASIC_PSOC_FEAT_RX]),
		       size);
		UPDATE_REPLY(index, len, STATS_FEAT_RX, size, STATS_OBJ_AP);
	}
	if (tb[QCA_WLAN_VENDOR_ATTR_BASIC_PSOC_R_PDEV]) {
		UPDATE_REPLY(index, len, STATS_RECURSIVE, 0, STATS_OBJ_AP);
		nla_for_each_nested(attr,
				    tb[QCA_WLAN_VENDOR_ATTR_BASIC_PSOC_R_PDEV],
				    tmp) {
			libstats_parse_basic_radio(buffer, type, recursive,
						   attr, reply);
		}
	}
}

static void libstats_response_handler(struct cfg80211_data *buffer)
{
	struct stats_command *cmd = NULL;

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
	if (cmd->lvl == STATS_LVL_BASIC) {
		if (cmd->obj == STATS_OBJ_STA)
			libstats_parse_basic_sta(buffer, cmd->type,
						 cmd->recursive, NULL,
						 cmd->reply);
		else if (cmd->obj == STATS_OBJ_VAP)
			libstats_parse_basic_vap(buffer, cmd->type,
						 cmd->recursive, NULL,
						 cmd->reply);
		else if (cmd->obj == STATS_OBJ_RADIO)
			libstats_parse_basic_radio(buffer, cmd->type,
						   cmd->recursive, NULL,
						   cmd->reply);
		else if (cmd->obj == STATS_OBJ_AP)
			libstats_parse_basic_ap(buffer, cmd->type,
						cmd->recursive, cmd->reply);
	} else if (cmd->lvl == STATS_LVL_ADVANCE) {
	} else if (cmd->lvl == STATS_LVL_DEBUG) {
	}
}

static void libstats_send_nl_command(struct stats_command *cmd,
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
			return;
		}
		ret = libstats_prepare_request(nlmsg, cmd);
		if (ret) {
			STATS_ERR("failed to prepare stats request\n");
			nlmsg_free(nlmsg);
			return;
		}
		end_vendor_data(nlmsg, nl_ven_data);

		ret = send_nlmsg(&g_sock_ctx.cfg80211_ctxt, nlmsg, buffer);
		if (ret < 0) {
			STATS_ERR("Couldn't send NL command, ret = %d\n", ret);
			return;
		}
	}
}

static void libstats_send(struct stats_command *cmd)
{
	struct cfg80211_data buffer;
	u_int8_t index = 0;

	if (!g_sock_ctx.cfg80211) {
		STATS_ERR("cfg80211 interface is not enabled\n");
		return;
	}

	if (is_valid_cmd(cmd)) {
		STATS_ERR("Invalid command\n");
		return;
	}

	memset(&buffer, 0, sizeof(struct cfg80211_data));
	buffer.data = cmd;
	buffer.length = sizeof(*cmd);
	buffer.callback = libstats_response_handler;
	buffer.parse_data = 1;

	if (cmd->obj == STATS_OBJ_AP) {
		for (index = 0; index < MAX_IF_NUM; index++)
			libstats_send_nl_command(cmd, &buffer,
						 g_ifnames[index]);
	} else {
		libstats_send_nl_command(cmd, &buffer, cmd->if_name);
	}
}

u_int32_t libstats_get_feature_flag(char *feat_flags)
{
	u_int32_t feat_flag = 0;
	u_int8_t index = 0;
	char *flag = NULL;

	flag = strtok_r(feat_flags, ",", &feat_flags);
	while (flag) {
		for (index = 0; index < STATS_FEAT_MAX; index++) {
			if (!g_feat[index].name) {
				STATS_WARN("Invalid Feature Identifier %s\n"
					  "Continue with ALL feature\n", flag);
				/* default feature value */
				return 0;
			}
			if (!strncmp(flag, g_feat[index].name, strlen(flag))) {
				/* ALL is specified in Feature list */
				if (!index)
					return 0;
				feat_flag |= g_feat[index].id;
				break;
			}
		}
		flag = strtok_r(NULL, ",", &feat_flags);
	}

	return feat_flag;
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

	libstats_send(cmd);
	libstats_deinit();

	return ret;
}
