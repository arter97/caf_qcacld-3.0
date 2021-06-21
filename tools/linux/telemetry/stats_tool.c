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

#define DESC_FIELD_PROVISIONING         31

#define STATS_64(fp, descr, x) \
	fprintf(fp, "%-*s = %ju\n", DESC_FIELD_PROVISIONING, (descr), (x))

#define STATS_32(fp, descr, x) \
	fprintf(fp, "%-*s = %u\n", DESC_FIELD_PROVISIONING, (descr), (x))

#define STATS_16(fp, descr, x) \
	fprintf(fp, "%-*s = %hu\n", DESC_FIELD_PROVISIONING, (descr), (x))

#define STATS_16_SIGNED(fp, descr, x) \
	fprintf(fp, "%-*s = %hi\n", DESC_FIELD_PROVISIONING, (descr), (x))

#define STATS_8(fp, descr, x) \
	fprintf(fp, "%-*s = %hhu\n", DESC_FIELD_PROVISIONING, (descr), (x))

#define STATS_FLT(fp, descr, x, precsn) \
	fprintf(fp, "%-*s = %.*f\n", DESC_FIELD_PROVISIONING, (descr), \
	(precsn), (x))

#define STATS_UNVLBL(fp, descr, msg) \
	fprintf(fp, "%-*s = %s\n", DESC_FIELD_PROVISIONING, (descr), (msg))

/* WLAN MAC Address length */
#define USER_MAC_ADDR_LEN     (ETH_ALEN * 3)

static const char *opt_string = "badArvsDcf:i:m:Rh?";

static const struct option long_opts[] = {
	{ "basic", no_argument, NULL, 'b' },
	{ "advance", no_argument, NULL, 'a' },
	{ "debug", no_argument, NULL, 'd' },
	{ "ap", no_argument, NULL, 'A' },
	{ "radio", no_argument, NULL, 'r' },
	{ "vap", no_argument, NULL, 'v' },
	{ "sta", no_argument, NULL, 's' },
	{ "data", no_argument, NULL, 'D' },
	{ "ctrl", no_argument, NULL, 'c' },
	{ "feature", required_argument, NULL, 'f' },
	{ "ifname", required_argument, NULL, 'i' },
	{ "stamacaddr", required_argument, NULL, 'm' },
	{ "recursive", no_argument, NULL, 'R' },
	{ "help", no_argument, NULL, 'h' },
	{ NULL, no_argument, NULL, 0 },
};

static void display_help(void)
{
	STATS_PRINT("\nstats : Displays Statistics of Access Point\n");
	STATS_PRINT("\nUsage:\n"
		    "stats [Level] [Object] [FeatureFlag] [[-i interface_name] | [-m StationMACAddress]] [-R] [-h | ?]\n"
		    "\n"
		    "One of the 3 Levels can be selected. The Levels are as follows (catagorised based on purpose or\n"
		    "the level of details):\n"
		    "Basic, Advance and Debug. The default is Basic where it provides very minimal stats.\n"
		    "\n"
		    "One of the 4 Objects can be selected. The objects are as follows (from top to bottom of the hierarchy):\n"
		    "Entire Access Point, Radio, Vap and Station (STA). The default is Entire Access Point.\n"
		    "Stats for sub-levels below the selected Object can be displayed by choosing -R (recursive) option (not\n"
		    "applicable for the STA Object, where information for a single STA in displayed).\n"
		    "\n"
		    "Feature flag specifies various specific feature for which Stats is to be displayed. By default ALL will\n"
		    "be selected. List of feature flags are as follows:\n"
		    "ALL,RX,TX,AST,CFR,FWD,HTT,RAW,RDK,TSO,TWT,VOW,WDI,WMI,IGMP,LINK,MESH,RATE,DELAY,ME,NAWDS,TXCAP and MONITOR.\n"
		    "\n"
		    "Type specifies the stats category. Catagories are:\n"
		    "Control and Data. Default is Data category.\n"
		    "\n"
		    "Levels:\n"
		    "\n"
		    "-b or --basic\n"
		    "    Only Basic level stats are displyed. This is the default level\n"
		    "-a or --advance\n"
		    "    Advance level stats are displayed.\n"
		    "-d or --debug\n"
		    "    Debug level stats are displayed.\n"
		    "\n"
		    "Objects:\n"
		    "\n"
		    "-A or --ap\n"
		    "    Entire Access Point stats are displayed. No need to specify interface and STA MAC.\n"
		    "-r or --radio\n"
		    "    Radio object stats are displayed. Radio interface Name needs to be provided.\n"
		    "-v or --vap\n"
		    "    Vap object stats are displayed. Vap interface Name need to be provided.\n"
		    "-s or --sta\n"
		    "    STA object stats are displayed. STA MAC Address need to be provided.\n"
		    "\n"
		    "Feature Flags:\n"
		    "\n"
		    "-f <flag1[,flag2,...,flagN]> or --feature=<flag1[,flag2,...,flagN]>\n"
		    "    Feature flag carries the flags for requested features. Multiple flags can be added with ',' separation.\n"
		    "\n"
		    "Types:\n"
		    "\n"
		    "-D or --data\n"
		    "    Stats for data is displayed\n"
		    "-c or --ctrl\n"
		    "    Stats for control is displayed\n"
		    "\n"
		    "Interface:\n"
		    "\n"
		    "If Radio object is selected:\n"
		    "-i wifiX or --ifname=wifiX\n"
		    "\n"
		    "If VAP object is selected:\n"
		    "-i <VAP_name> or --ifname=<VAP_name>\n"
		    "\n"
		    "STA MAC Address:\n"
		    "\n"
		    "Required if STA object is selected:\n"
		    "-m xx:xx:xx:xx:xx:xx or --stamacaddr xx:xx:xx:xx:xx:xx\n"
		    "\n"
		    "OTHER OPTIONS:\n"
		    "-R or --recursive\n"
		    "    Recursive display\n"
		    "-h or --help\n"
		    "    Usage display\n");
}

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

u_int32_t print_basic_sta_data(struct reply_buffer *reply, u_int32_t index)
{
	u_int8_t *mac = NULL;
	struct basic_peer_data_tx *tx = NULL;
	struct basic_peer_data_rx *rx = NULL;
	struct basic_peer_data_link *link = NULL;
	struct basic_peer_data_rate *rate = NULL;

	STATS_PRINT("\t\tBasic STA Data Stats:\n");
	if (!reply) {
		STATS_ERR("Invalid Reply!\n");
		return index;
	}
	while (index < reply->db_count) {
		if (reply->db[index].obj != STATS_OBJ_STA)
			break;
		if (reply->db[index].type == STATS_OBJECT_ID) {
			mac = &reply->data[reply->db[index].index];
			STATS_PRINT("STATS For Peer %s\n", macaddr_to_str(mac));
		} else if (reply->db[index].type == STATS_FEAT_TX) {
			tx = (struct basic_peer_data_tx *)
			      &reply->data[reply->db[index].index];
			STATS_PRINT("Tx Stats\n");
			STATS_32(stdout, "Tx Success", tx->tx_success.num);
			STATS_64(stdout, "Tx Success Bytes",
				 tx->tx_success.bytes);
			STATS_32(stdout, "Tx Complete", tx->comp_pkt.num);
			STATS_64(stdout, "Tx Complete Bytes",
				 tx->comp_pkt.bytes);
			STATS_32(stdout, "Tx Failed", tx->tx_failed);
			STATS_64(stdout, "Tx Dropped Count", tx->dropped_count);
		} else if (reply->db[index].type == STATS_FEAT_RX) {
			rx = (struct basic_peer_data_rx *)
			      &reply->data[reply->db[index].index];
			STATS_PRINT("Rx Stats\n");
			STATS_32(stdout, "Rx To Stack", rx->to_stack.num);
			STATS_64(stdout, "Rx To Stack Bytes",
				 rx->to_stack.bytes);
			STATS_64(stdout, "Rx Total Packets",
				 rx->total_pkt_rcvd);
			STATS_64(stdout, "Rx Total Bytes",
				 rx->total_bytes_rcvd);
			STATS_64(stdout, "Rx Error Count", rx->rx_error_count);
		} else if (reply->db[index].type == STATS_FEAT_LINK) {
			link = (struct basic_peer_data_link *)
				&reply->data[reply->db[index].index];
			STATS_PRINT("Link Stats\n");
			STATS_32(stdout, "SNR Avrage", link->avg_snr);
			STATS_8(stdout, "SNR", link->snr);
			STATS_8(stdout, "SNR Last", link->last_snr);
		} else if (reply->db[index].type == STATS_FEAT_RATE) {
			rate = (struct basic_peer_data_rate *)
				&reply->data[reply->db[index].index];
			STATS_PRINT("Rate Stats\n");
			STATS_32(stdout, "Rx Rate", rate->rx_rate);
			STATS_32(stdout, "Rx Last Rate", rate->last_rx_rate);
			STATS_32(stdout, "Tx Rate", rate->tx_rate);
			STATS_32(stdout, "Tx Last Rate", rate->last_tx_rate);
		}
		index++;
	}

	return index;
}

u_int32_t print_basic_sta_ctrl(struct reply_buffer *reply, u_int32_t index)
{
	u_int8_t *mac = NULL;
	struct basic_peer_ctrl_tx *tx = NULL;
	struct basic_peer_ctrl_rx *rx = NULL;
	struct basic_peer_ctrl_link *link = NULL;
	struct basic_peer_ctrl_rate *rate = NULL;

	STATS_PRINT("\t\tBasic STA Control Stats:\n");
	if (!reply) {
		STATS_ERR("Invalid Reply!\n");
		return index;
	}
	while (index < reply->db_count) {
		if (reply->db[index].obj != STATS_OBJ_STA)
			break;
		if (reply->db[index].type == STATS_OBJECT_ID) {
			mac = &reply->data[reply->db[index].index];
			STATS_PRINT("STATS For Peer %s\n", macaddr_to_str(mac));
		} else if (reply->db[index].type == STATS_FEAT_TX) {
			tx = (struct basic_peer_ctrl_tx *)
			      &reply->data[reply->db[index].index];
			STATS_PRINT("Tx Stats\n");
			STATS_32(stdout, "Tx Management", tx->cs_tx_mgmt);
			STATS_32(stdout, "Tx Not Ok", tx->cs_is_tx_not_ok);
		} else if (reply->db[index].type == STATS_FEAT_RX) {
			rx = (struct basic_peer_ctrl_rx *)
			      &reply->data[reply->db[index].index];
			STATS_PRINT("Rx Stats\n");
			STATS_32(stdout, "Rx Management", rx->cs_rx_mgmt);
			STATS_32(stdout, "Rx Decrypt Crc",
				 rx->cs_rx_decryptcrc);
			STATS_32(stdout, "Rx Security Failure",
				 rx->cs_rx_security_failure);
		} else if (reply->db[index].type == STATS_FEAT_LINK) {
			link = (struct basic_peer_ctrl_link *)
				&reply->data[reply->db[index].index];
			STATS_PRINT("Link Stats\n");
			STATS_8(stdout, "Rx Management SNR",
				link->cs_rx_mgmt_snr);
		} else if (reply->db[index].type == STATS_FEAT_RATE) {
			rate = (struct basic_peer_ctrl_rate *)
				&reply->data[reply->db[index].index];
			STATS_PRINT("Rate Stats\n");
			STATS_32(stdout, "Rx Management Rate",
				 rate->cs_rx_mgmt_rate);
		}
		index++;
	}

	return index;
}

u_int32_t print_basic_vap_data(struct reply_buffer *reply, u_int32_t index)
{
	struct basic_vdev_data_tx *tx = NULL;
	struct basic_vdev_data_rx *rx = NULL;
	struct pkt_info *pkt = NULL;

	STATS_PRINT("\t\tBasic VAP Data Stats:\n");
	if (!reply) {
		STATS_ERR("Invalid Reply!\n");
		return index;
	}
	while (index < reply->db_count) {
		if (reply->db[index].obj != STATS_OBJ_VAP)
			break;
		if (reply->db[index].type == STATS_OBJECT_ID) {
			STATS_PRINT("STATS for Vap %s\n",
				    &reply->data[reply->db[index].index]);
		} else if (reply->db[index].type == STATS_FEAT_TX) {
			tx = (struct basic_vdev_data_tx *)
			      &reply->data[reply->db[index].index];
			STATS_PRINT("Tx Stats\n");
			pkt = &tx->rcvd;
			STATS_32(stdout, "Tx Ingress Received", pkt->num);
			STATS_64(stdout, "Tx Ingress Received Bytes",
				 pkt->bytes);
			pkt = &tx->processed;
			STATS_32(stdout, "Tx Ingress Processed", pkt->num);
			STATS_64(stdout, "Tx Ingress Processed Bytes",
				 pkt->bytes);
			pkt = &tx->dropped;
			STATS_32(stdout, "Tx Ingress Dropped", pkt->num);
			STATS_64(stdout, "Tx Ingress Dropped Bytes",
				 pkt->bytes);
			pkt = &tx->tx_success;
			STATS_32(stdout, "Tx Success", pkt->num);
			STATS_64(stdout, "Tx Success Bytes", pkt->bytes);
			pkt = &tx->comp_pkt;
			STATS_32(stdout, "Tx Complete", pkt->num);
			STATS_64(stdout, "Tx Complete Bytes", pkt->bytes);
			STATS_32(stdout, "Tx Failed", tx->tx_failed);
			STATS_64(stdout, "Tx Dropped Count", tx->dropped_count);
		} else if (reply->db[index].type == STATS_FEAT_RX) {
			rx = (struct basic_vdev_data_rx *)
			      &reply->data[reply->db[index].index];
			STATS_PRINT("Rx Stats\n");
			pkt = &rx->to_stack;
			STATS_32(stdout, "Rx To Stack", pkt->num);
			STATS_64(stdout, "Rx To Stack Bytes", pkt->bytes);
			STATS_64(stdout, "Rx Total Packets",
				 rx->total_pkt_rcvd);
			STATS_64(stdout, "Rx Total Bytes",
				 rx->total_bytes_rcvd);
			STATS_64(stdout, "Rx Error Count", rx->rx_error_count);
		} else if (reply->db[index].type == STATS_RECURSIVE) {
			index = print_basic_sta_data(reply, index + 1);
			index--;
		}
		index++;
	}

	return index;
}

u_int32_t print_basic_vap_ctrl(struct reply_buffer *reply, u_int32_t index)
{
	struct basic_vdev_ctrl_tx *tx = NULL;
	struct basic_vdev_ctrl_rx *rx = NULL;

	STATS_PRINT("\t\tBasic VAP Control Stats:\n");
	if (!reply) {
		STATS_ERR("Invalid Reply!\n");
		return index;
	}
	while (index < reply->db_count) {
		if (reply->db[index].obj != STATS_OBJ_VAP)
			break;
		if (reply->db[index].type == STATS_OBJECT_ID) {
			STATS_PRINT("STATS for Vap %s\n",
				    &reply->data[reply->db[index].index]);
		} else if (reply->db[index].type == STATS_FEAT_TX) {
			tx = (struct basic_vdev_ctrl_tx *)
			      &reply->data[reply->db[index].index];
			STATS_PRINT("Tx Stats\n");
			STATS_64(stdout, "Tx Management", tx->cs_tx_mgmt);
			STATS_64(stdout, "Tx Error Count",
				 tx->cs_tx_error_counter);
			STATS_64(stdout, "Tx Discard", tx->cs_tx_discard);
		} else if (reply->db[index].type == STATS_FEAT_RX) {
			rx = (struct basic_vdev_ctrl_rx *)
			      &reply->data[reply->db[index].index];
			STATS_PRINT("Rx Stats\n");
			STATS_64(stdout, "Rx Management", rx->cs_rx_mgmt);
			STATS_64(stdout, "Rx Error Count",
				 rx->cs_rx_error_counter);
			STATS_64(stdout, "Rx Management Discard",
				 rx->cs_rx_mgmt_discard);
			STATS_64(stdout, "Rx Control", rx->cs_rx_ctl);
			STATS_64(stdout, "Rx Discard", rx->cs_rx_discard);
			STATS_64(stdout, "Rx Security Failure",
				 rx->cs_rx_security_failure);
		} else if (reply->db[index].type == STATS_RECURSIVE) {
			index = print_basic_sta_ctrl(reply, index + 1);
			index--;
		}
		index++;
	}

	return index;
}

u_int32_t print_basic_radio_data(struct reply_buffer *reply, u_int32_t index)
{
	struct basic_pdev_data_tx *tx = NULL;
	struct basic_pdev_data_rx *rx = NULL;
	struct pkt_info *pkt = NULL;
	u_int8_t id = 0;

	STATS_PRINT("\t\tBasic Radio Data Stats:\n");
	if (!reply) {
		STATS_ERR("Invalid Reply!\n");
		return index;
	}
	while (index < reply->db_count) {
		if (reply->db[index].obj != STATS_OBJ_RADIO)
			break;
		if (reply->db[index].type == STATS_OBJECT_ID) {
			id = reply->data[reply->db[index].index];
			STATS_PRINT("STATS for Wifi %d\n", id);
		} else if (reply->db[index].type == STATS_FEAT_TX) {
			tx = (struct basic_pdev_data_tx *)
			      &reply->data[reply->db[index].index];
			STATS_PRINT("Tx Stats\n");
			pkt = &tx->rcvd;
			STATS_32(stdout, "Tx Ingress Received", pkt->num);
			STATS_64(stdout, "Tx Ingress Received Bytes",
				 pkt->bytes);
			pkt = &tx->processed;
			STATS_32(stdout, "Tx Ingress Processed", pkt->num);
			STATS_64(stdout, "Tx Ingress Processed Bytes",
				 pkt->bytes);
			pkt = &tx->dropped;
			STATS_32(stdout, "Tx Ingress Dropped", pkt->num);
			STATS_64(stdout, "Tx Ingress Dropped Bytes",
				 pkt->bytes);
			pkt = &tx->tx_success;
			STATS_32(stdout, "Tx Success", pkt->num);
			STATS_64(stdout, "Tx Success Bytes", pkt->bytes);
			pkt = &tx->comp_pkt;
			STATS_32(stdout, "Tx Complete", pkt->num);
			STATS_64(stdout, "Tx Complete Bytes", pkt->bytes);
			STATS_32(stdout, "Tx Failed", tx->tx_failed);
			STATS_64(stdout, "Tx Dropped Count", tx->dropped_count);
		} else if (reply->db[index].type == STATS_FEAT_RX) {
			rx = (struct basic_pdev_data_rx *)
			      &reply->data[reply->db[index].index];
			STATS_PRINT("Rx Stats\n");
			pkt = &rx->to_stack;
			STATS_32(stdout, "Rx To Stack", pkt->num);
			STATS_64(stdout, "Rx To Stack Bytes", pkt->bytes);
			STATS_64(stdout, "Rx Total Packets",
				 rx->total_pkt_rcvd);
			STATS_64(stdout, "Rx Total Bytes",
				 rx->total_bytes_rcvd);
			STATS_64(stdout, "Rx Error Count", rx->rx_error_count);
			STATS_64(stdout, "Dropped Count", rx->dropped_count);
			STATS_64(stdout, "Error Count", rx->err_count);
		} else if (reply->db[index].type == STATS_RECURSIVE) {
			index = print_basic_vap_data(reply, index + 1);
			index--;
		}
		index++;
	}

	return index;
}

u_int32_t print_basic_radio_ctrl(struct reply_buffer *reply, u_int32_t index)
{
	struct basic_pdev_ctrl_tx *tx = NULL;
	struct basic_pdev_ctrl_rx *rx = NULL;
	struct basic_pdev_ctrl_link *link = NULL;
	u_int8_t id = 0;

	STATS_PRINT("\t\tBasic Radio Control Stats:\n");
	if (!reply) {
		STATS_ERR("Invalid Reply!\n");
		return index;
	}
	while (index < reply->db_count) {
		if (reply->db[index].obj != STATS_OBJ_RADIO)
			break;
		if (reply->db[index].type == STATS_OBJECT_ID) {
			id = reply->data[reply->db[index].index];
			STATS_PRINT("STATS for Wifi %d\n", id);
		} else if (reply->db[index].type == STATS_FEAT_TX) {
			tx = (struct basic_pdev_ctrl_tx *)
			      &reply->data[reply->db[index].index];
			STATS_PRINT("Tx Stats\n");
			STATS_32(stdout, "Tx Management", tx->cs_tx_mgmt);
			STATS_32(stdout, "Tx Frame Count",
				 tx->cs_tx_frame_count);
		} else if (reply->db[index].type == STATS_FEAT_RX) {
			rx = (struct basic_pdev_ctrl_rx *)
				&reply->data[reply->db[index].index];
			STATS_PRINT("Rx Stats\n");
			STATS_32(stdout, "Rx Management", rx->cs_rx_mgmt);
			STATS_32(stdout, "Rx Number of Mnagement",
				 rx->cs_rx_num_mgmt);
			STATS_32(stdout, "Rx Number of Control",
				 rx->cs_rx_num_ctl);
			STATS_32(stdout, "Rx Frame Count",
				 rx->cs_rx_frame_count);
			STATS_32(stdout, "Rx Error Sum",
				 rx->cs_rx_error_sum);
		} else if (reply->db[index].type == STATS_FEAT_LINK) {
			link = (struct basic_pdev_ctrl_link *)
				&reply->data[reply->db[index].index];
			STATS_PRINT("Link Stats\n");
			STATS_32(stdout, "Channel Tx Power",
				 link->cs_chan_tx_pwr);
			STATS_32(stdout, "Rx Rssi Combind",
				 link->cs_rx_rssi_comb);
			STATS_16_SIGNED(stdout, "Channel NF", link->cs_chan_nf);
			STATS_16_SIGNED(stdout, "Channel NF Sec80",
					link->cs_chan_nf_sec80);
			STATS_8(stdout, "DCS Total Util", link->dcs_total_util);
		} else if (reply->db[index].type == STATS_RECURSIVE) {
			index = print_basic_vap_ctrl(reply, index + 1);
			index--;
		}
		index++;
	}

	return index;
}

u_int32_t print_basic_ap_data(struct reply_buffer *reply, u_int32_t index)
{
	struct basic_psoc_data_tx *tx = NULL;
	struct basic_psoc_data_rx *rx = NULL;
	bool reprint = true;

	if (!reply) {
		STATS_ERR("Invalid Reply!\n");
		return index;
	}
	while (index < reply->db_count) {
		if (reply->db[index].obj != STATS_OBJ_AP)
			break;
		if (reprint) {
			STATS_PRINT("\t\tBasic AP Data Stats\n");
			reprint = false;
		}
		if (reply->db[index].type == STATS_FEAT_TX) {
			tx = (struct basic_psoc_data_tx *)
			      &reply->data[reply->db[index].index];
			STATS_PRINT("Tx Stats\n");
			STATS_32(stdout, "Tx Egress Pkts", tx->egress.num);
			STATS_64(stdout, "Tx Egress Bytes", tx->egress.bytes);
		} else if (reply->db[index].type == STATS_FEAT_RX) {
			rx = (struct basic_psoc_data_rx *)
			      &reply->data[reply->db[index].index];
			STATS_PRINT("Rx Stats\n");
			STATS_32(stdout, "Rx Ingress Pkts", rx->ingress.num);
			STATS_64(stdout, "Rx Ingress Bytes", rx->ingress.bytes);
		} else if (reply->db[index].type == STATS_RECURSIVE) {
			index = print_basic_radio_data(reply, index + 1);
			reprint = true;
			index--;
		}
		index++;
	}

	return index;
}

u_int32_t print_basic_ap_ctrl(struct reply_buffer *reply, u_int32_t index)
{
	STATS_PRINT("\t\tBasic AP Control Stats");
	if (!reply) {
		STATS_ERR("Invalid Reply!\n");
		return index;
	}
	while (index < reply->db_count) {
		if (reply->db[index].obj != STATS_OBJ_AP)
			break;
		if (reply->db[index].type == STATS_RECURSIVE) {
			index = print_basic_radio_ctrl(reply, index + 1);
			index--;
		}
		index++;
	}

	return index;
}

void print_basic_stats(struct stats_command *cmd)
{
	switch (cmd->obj) {
	case STATS_OBJ_STA:
		if (cmd->type == STATS_TYPE_DATA)
			print_basic_sta_data(cmd->reply, 0);
		else
			print_basic_sta_ctrl(cmd->reply, 0);
		break;
	case STATS_OBJ_VAP:
		if (cmd->type == STATS_TYPE_DATA)
			print_basic_vap_data(cmd->reply, 0);
		else
			print_basic_vap_ctrl(cmd->reply, 0);
		break;
	case STATS_OBJ_RADIO:
		if (cmd->type == STATS_TYPE_DATA)
			print_basic_radio_data(cmd->reply, 0);
		else
			print_basic_radio_ctrl(cmd->reply, 0);
		break;
	case STATS_OBJ_AP:
		if (cmd->type == STATS_TYPE_DATA)
			print_basic_ap_data(cmd->reply, 0);
		else
			print_basic_ap_ctrl(cmd->reply, 0);
		break;
	default:
		STATS_ERR("Invalid object option\n");
	}
}

void print_response(struct stats_command *cmd)
{
	if (cmd->lvl == STATS_LVL_BASIC)
		print_basic_stats(cmd);
}

int main(int argc, char *argv[])
{
	struct stats_command cmd;
	struct reply_buffer *reply = NULL;
	enum stats_level_e level_temp = STATS_LVL_BASIC;
	enum stats_object_e obj_temp = STATS_OBJ_AP;
	enum stats_type_e type_temp = STATS_TYPE_DATA;
	u_int32_t feat_temp = STATS_FEAT_FLG_ALL;
	int opt = 0;
	int long_index = 0;
	u_int8_t is_obj_set = 0;
	u_int8_t is_type_set = 0;
	u_int8_t is_feat_set = 0;
	u_int8_t is_level_set = 0;
	u_int8_t is_ifname_set = 0;
	u_int8_t is_stamacaddr_set = 0;
	u_int8_t is_option_selected = 0;
	bool recursion_temp = 0;
	char feat_flags[128] = {0};
	char ifname_temp[IF_NAME_LEN] = {0};
	char stamacaddr_temp[USER_MAC_ADDR_LEN] = {0};
	struct ether_addr *ret_eth_addr = NULL;

	memset(&cmd, 0, sizeof(struct stats_command));

	opt = getopt_long(argc, argv, opt_string, long_opts, &long_index);
	while (opt != -1) {
		switch (opt) {
		case 'b':
			if (is_level_set) {
				STATS_ERR("Multiple Stats Level Arguments\n");
				display_help();
				return -EINVAL;
			}
			level_temp = STATS_LVL_BASIC;
			is_level_set = 1;
			is_option_selected = 1;
			break;
		case 'a':
			if (is_level_set) {
				STATS_ERR("Multiple Stats Level Arguments\n");
				display_help();
				return -EINVAL;
			}
			level_temp = STATS_LVL_ADVANCE;
			is_level_set = 1;
			is_option_selected = 1;
			break;
		case 'd':
			if (is_level_set) {
				STATS_ERR("Multiple Stats Level Arguments\n");
				display_help();
				return -EINVAL;
			}
			level_temp = STATS_LVL_DEBUG;
			is_level_set = 1;
			is_option_selected = 1;
			break;
		case 'i':
			if (is_ifname_set) {
				STATS_ERR("Multiple Stats ifname Arguments\n");
				display_help();
				return -EINVAL;
			}
			memset(ifname_temp, '\0', sizeof(ifname_temp));
			if (strlcpy(ifname_temp, optarg, sizeof(ifname_temp)) >=
			    sizeof(ifname_temp)) {
				STATS_ERR("Error creating ifname\n");
				return -EINVAL;
			}
			is_ifname_set  = 1;
			is_option_selected = 1;
			break;
		case 's':
			if (is_obj_set) {
				STATS_ERR("Multiple Stats Object Arguments\n");
				display_help();
				return -EINVAL;
			}
			obj_temp = STATS_OBJ_STA;
			is_obj_set = 1;
			is_option_selected = 1;
			break;
		case 'v':
			if (is_obj_set) {
				STATS_ERR("Multiple Stats Object Arguments\n");
				display_help();
				return -EINVAL;
			}
			obj_temp = STATS_OBJ_VAP;
			is_obj_set = 1;
			is_option_selected = 1;
			break;
		case 'r':
			if (is_obj_set) {
				STATS_ERR("Multiple Stats Object Arguments\n");
				display_help();
				return -EINVAL;
			}
			obj_temp = STATS_OBJ_RADIO;
			is_obj_set = 1;
			is_option_selected = 1;
			break;
		case 'A':
			if (is_obj_set) {
				STATS_ERR("Multiple Stats Object Arguments\n");
				display_help();
				return -EINVAL;
			}
			obj_temp = STATS_OBJ_AP;
			is_obj_set = 1;
			is_option_selected = 1;
			break;
		case 'm':
			if (is_stamacaddr_set) {
				STATS_ERR("Multiple STA MAC Arguments\n");
				display_help();
				return -EINVAL;
			}
			memset(stamacaddr_temp, '\0', sizeof(stamacaddr_temp));
			if (strlcpy(stamacaddr_temp, optarg,
				    sizeof(stamacaddr_temp)) >=
				    sizeof(stamacaddr_temp)) {
				STATS_ERR("Error copying macaddr\n");
				return -EINVAL;
			}
			is_stamacaddr_set  = 1;
			is_option_selected = 1;
			break;
		case 'f':
			if (is_feat_set) {
				STATS_ERR("Multiple Feature flag Arguments\n");
				display_help();
				return -EINVAL;
			}
			memset(feat_flags, '\0', sizeof(feat_flags));
			if (strlcpy(feat_flags, optarg, sizeof(feat_flags)) >=
			    sizeof(feat_flags)) {
				STATS_ERR("Error copying feature flags\n");
				return -EINVAL;
			}
			is_feat_set = 1;
			is_option_selected = 1;
			break;
		case 'c':
			if (is_type_set) {
				STATS_ERR("Multiple Type Arguments\n");
				display_help();
				return -EINVAL;
			}
			type_temp = STATS_TYPE_CTRL;
			is_type_set = 1;
			is_option_selected = 1;
			break;
		case 'D':
			if (is_type_set) {
				STATS_ERR("Multiple Type Arguments\n");
				display_help();
				return -EINVAL;
			}
			type_temp = STATS_TYPE_DATA;
			is_type_set = 1;
			is_option_selected = 1;
			break;
		case 'h':
		case '?':
			display_help();
			is_option_selected = 1;
			return 0;
		case 'R':
			recursion_temp = 1;
			break;
		default:
			STATS_ERR("Unrecognized option\n");
			display_help();
			is_option_selected = 1;
			return -EINVAL;
		}
		opt = getopt_long(argc, argv, opt_string,
				  long_opts, &long_index);
	}

	if (!is_option_selected)
		STATS_WARN("No valid option selected\n"
			   "Will display only default stats\n");

	if (feat_flags[0])
		feat_temp = libstats_get_feature_flag(feat_flags);
	cmd.lvl = level_temp;
	cmd.obj = obj_temp;
	cmd.type = type_temp;
	cmd.feat_flag = feat_temp;
	cmd.recursive = recursion_temp;
	if (ifname_temp[0])
		strlcpy(cmd.if_name, ifname_temp, IF_NAME_LEN);
	if (stamacaddr_temp[0]) {
		ret_eth_addr = ether_aton_r(stamacaddr_temp, &cmd.sta_mac);
		if (!ret_eth_addr) {
			STATS_ERR("STA MAC address not valid.\n");
			return -EINVAL;
		}
	}
	reply = (struct reply_buffer *)malloc(sizeof(struct reply_buffer));
	if (!reply) {
		STATS_ERR("Failed to allocate memory\n");
		return -ENOMEM;
	}
	memset(reply, 0, sizeof(struct reply_buffer));
	reply->data = (u_int8_t *)malloc(MAX_DATA_LENGTH);
	if (!reply->data) {
		free(reply);
		return -ENOMEM;
	}
	cmd.reply = reply;

	libstats_request_handle(&cmd);

	print_response(&cmd);
	if (reply) {
		if (reply->data)
			free(reply->data);
		free(reply);
		cmd.reply = NULL;
	}

	return 0;
}
