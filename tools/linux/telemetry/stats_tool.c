/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
 * Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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

#define FL "%s(): %d:"

#define STATS_ERR(fmt, args...) \
	fprintf(stderr, FL "stats: Error: "fmt, __func__, __LINE__, ## args)
#define STATS_WARN(fmt, args...) \
	fprintf(stdout, FL "stats: Warn: "fmt, __func__, __LINE__, ## args)
#define STATS_MSG(fmt, args...) \
	fprintf(stdout, FL "stats: "fmt, __func__, __LINE__, ## args)

#define STATS_PRINT(fmt, args...) \
	fprintf(stdout, fmt, ## args)

#define DESC_FIELD_PROVISIONING         31

#define STATS_64(fp, descr, x) \
	fprintf(fp, "\t%-*s = %ju\n", DESC_FIELD_PROVISIONING, (descr), (x))

#define STATS_32(fp, descr, x) \
	fprintf(fp, "\t%-*s = %u\n", DESC_FIELD_PROVISIONING, (descr), (x))

#define STATS_16(fp, descr, x) \
	fprintf(fp, "\t%-*s = %hu\n", DESC_FIELD_PROVISIONING, (descr), (x))

#define STATS_16_SIGNED(fp, descr, x) \
	fprintf(fp, "\t%-*s = %hi\n", DESC_FIELD_PROVISIONING, (descr), (x))

#define STATS_8(fp, descr, x) \
	fprintf(fp, "\t%-*s = %hhu\n", DESC_FIELD_PROVISIONING, (descr), (x))

#define STATS_FLT(fp, descr, x, precsn) \
	fprintf(fp, "\t%-*s = %.*f\n", DESC_FIELD_PROVISIONING, (descr), \
	(precsn), (x))

#define STATS_UNVLBL(fp, descr, msg) \
	fprintf(fp, "\t%-*s = %s\n", DESC_FIELD_PROVISIONING, (descr), (msg))

/* WLAN MAC Address length */
#define USER_MAC_ADDR_LEN     (ETH_ALEN * 3)

static const char *opt_string = "BADarvsdcf:i:m:Rh?";

static const struct option long_opts[] = {
	{ "basic", no_argument, NULL, 'B' },
	{ "advance", no_argument, NULL, 'A' },
	{ "debug", no_argument, NULL, 'D' },
	{ "ap", no_argument, NULL, 'a' },
	{ "radio", no_argument, NULL, 'r' },
	{ "vap", no_argument, NULL, 'v' },
	{ "sta", no_argument, NULL, 's' },
	{ "data", no_argument, NULL, 'd' },
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
		    "stats [Level] [Object] [StatsType] [FeatureFlag] [[-i interface_name] | [-m StationMACAddress]] [-R] [-h | ?]\n"
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
		    "StatsType specifies the stats category. Catagories are:\n"
		    "Control and Data. Default is Data category.\n"
		    "\n"
		    "Feature flag specifies various specific feature for which Stats is to be displayed. By default ALL will\n"
		    "be selected. List of feature flags are as follows:\n"
		    "ALL,RX,TX,AST,CFR,FWD,HTT,RAW,RDK,TSO,TWT,VOW,WDI,WMI,IGMP,LINK,MESH,RATE,DELAY,ME,NAWDS,TXCAP and MONITOR.\n"
		    "\n"
		    "Levels:\n"
		    "\n"
		    "-B or --basic\n"
		    "    Only Basic level stats are displyed. This is the default level\n"
		    "-A or --advance\n"
		    "    Advance level stats are displayed.\n"
		    "-D or --debug\n"
		    "    Debug level stats are displayed.\n"
		    "\n"
		    "Objects:\n"
		    "\n"
		    "-a or --ap\n"
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
		    "-d or --data\n"
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

void print_basic_data_tx_stats(struct basic_data_tx_stats *tx)
{
	STATS_64(stdout, "Tx Success", tx->tx_success.num);
	STATS_64(stdout, "Tx Success Bytes", tx->tx_success.bytes);
	STATS_64(stdout, "Tx Complete", tx->comp_pkt.num);
	STATS_64(stdout, "Tx Complete Bytes", tx->comp_pkt.bytes);
	STATS_64(stdout, "Tx Failed", tx->tx_failed);
	STATS_64(stdout, "Tx Dropped Count", tx->dropped_count);
}

void print_basic_data_rx_stats(struct basic_data_rx_stats *rx)
{
	STATS_64(stdout, "Rx To Stack", rx->to_stack.num);
	STATS_64(stdout, "Rx To Stack Bytes", rx->to_stack.bytes);
	STATS_64(stdout, "Rx Total Packets", rx->total_rcvd.num);
	STATS_64(stdout, "Rx Total Bytes", rx->total_rcvd.bytes);
	STATS_64(stdout, "Rx Error Count", rx->rx_error_count);
}

void print_basic_sta_data_tx(struct basic_peer_data_tx *tx)
{
	print_basic_data_tx_stats(&tx->tx);
}

void print_basic_sta_data_rx(struct basic_peer_data_rx *rx)
{
	print_basic_data_rx_stats(&rx->rx);
}

void print_basic_sta_data_link(struct basic_peer_data_link *link)
{
	STATS_8(stdout, "SNR", link->snr);
	STATS_8(stdout, "Last SNR", link->last_snr);
	STATS_32(stdout, "Avrage SNR", link->avg_snr);
}

void print_basic_sta_data_rate(struct basic_peer_data_rate *rate)
{
	STATS_32(stdout, "Rx Rate", rate->rx_rate);
	STATS_32(stdout, "Last Rx Rate", rate->last_rx_rate);
	STATS_32(stdout, "Tx Rate", rate->tx_rate);
	STATS_32(stdout, "Last Tx Rate", rate->last_tx_rate);
}

void print_basic_sta_ctrl_tx(struct basic_peer_ctrl_tx *tx)
{
	STATS_32(stdout, "Tx Management", tx->cs_tx_mgmt);
	STATS_32(stdout, "Tx Not Ok", tx->cs_is_tx_not_ok);
}

void print_basic_sta_ctrl_rx(struct basic_peer_ctrl_rx *rx)
{
	STATS_32(stdout, "Rx Management", rx->cs_rx_mgmt);
	STATS_32(stdout, "Rx Decrypt Crc", rx->cs_rx_decryptcrc);
	STATS_32(stdout, "Rx Security Failure", rx->cs_rx_security_failure);
}

void print_basic_sta_ctrl_link(struct basic_peer_ctrl_link *link)
{
	STATS_8(stdout, "Rx Management SNR", link->cs_rx_mgmt_snr);
}

void print_basic_sta_ctrl_rate(struct basic_peer_ctrl_rate *rate)
{
	STATS_32(stdout, "Rx Management Rate", rate->cs_rx_mgmt_rate);
}

void print_basic_vap_data_tx(struct basic_vdev_data_tx *tx)
{
	struct pkt_info *pkt = NULL;

	print_basic_data_tx_stats(&tx->tx);
	pkt = &tx->ingress;
	STATS_64(stdout, "Tx Ingress Received", pkt->num);
	STATS_64(stdout, "Tx Ingress Received Bytes", pkt->bytes);
	pkt = &tx->processed;
	STATS_64(stdout, "Tx Ingress Processed", pkt->num);
	STATS_64(stdout, "Tx Ingress Processed Bytes", pkt->bytes);
	pkt = &tx->dropped;
	STATS_64(stdout, "Tx Ingress Dropped", pkt->num);
	STATS_64(stdout, "Tx Ingress Dropped Bytes", pkt->bytes);
}

void print_basic_vap_data_rx(struct basic_vdev_data_rx *rx)
{
	print_basic_data_rx_stats(&rx->rx);
}

void print_basic_vap_ctrl_tx(struct basic_vdev_ctrl_tx *tx)
{
	STATS_64(stdout, "Tx Management", tx->cs_tx_mgmt);
	STATS_64(stdout, "Tx Error Count", tx->cs_tx_error_counter);
	STATS_64(stdout, "Tx Discard", tx->cs_tx_discard);
}

void print_basic_vap_ctrl_rx(struct basic_vdev_ctrl_rx *rx)
{
	STATS_64(stdout, "Rx Management", rx->cs_rx_mgmt);
	STATS_64(stdout, "Rx Error Count", rx->cs_rx_error_counter);
	STATS_64(stdout, "Rx Management Discard", rx->cs_rx_mgmt_discard);
	STATS_64(stdout, "Rx Control", rx->cs_rx_ctl);
	STATS_64(stdout, "Rx Discard", rx->cs_rx_discard);
	STATS_64(stdout, "Rx Security Failure", rx->cs_rx_security_failure);
}

void print_basic_radio_data_tx(struct basic_pdev_data_tx *tx)
{
	struct pkt_info *pkt = NULL;

	print_basic_data_tx_stats(&tx->tx);
	pkt = &tx->ingress;
	STATS_64(stdout, "Tx Ingress Received", pkt->num);
	STATS_64(stdout, "Tx Ingress Received Bytes", pkt->bytes);
	pkt = &tx->processed;
	STATS_64(stdout, "Tx Ingress Processed", pkt->num);
	STATS_64(stdout, "Tx Ingress Processed Bytes", pkt->bytes);
	pkt = &tx->dropped;
	STATS_64(stdout, "Tx Ingress Dropped", pkt->num);
	STATS_64(stdout, "Tx Ingress Dropped Bytes", pkt->bytes);
}

void print_basic_radio_data_rx(struct basic_pdev_data_rx *rx)
{
	print_basic_data_rx_stats(&rx->rx);
	STATS_64(stdout, "Dropped Count", rx->dropped_count);
	STATS_64(stdout, "Error Count", rx->err_count);
}

void print_basic_radio_ctrl_tx(struct basic_pdev_ctrl_tx *tx)
{
	STATS_32(stdout, "Lithium_cycle_counts: Tx Frame Count",
		 tx->cs_tx_frame_count);
	STATS_32(stdout, "Tx Management", tx->cs_tx_mgmt);
}

void print_basic_radio_ctrl_rx(struct basic_pdev_ctrl_rx *rx)
{
	STATS_32(stdout, "Lithium_cycle_counts: Rx Frame Count",
		 rx->cs_rx_frame_count);
	STATS_32(stdout, "Rx Management", rx->cs_rx_mgmt);
	STATS_32(stdout, "Rx Number of Mnagement", rx->cs_rx_num_mgmt);
	STATS_32(stdout, "Rx Number of Control", rx->cs_rx_num_ctl);
	STATS_32(stdout, "Rx Error Sum", rx->cs_rx_error_sum);
}

void print_basic_radio_ctrl_link(struct basic_pdev_ctrl_link *link)
{
	STATS_32(stdout, "Channel Tx Power", link->cs_chan_tx_pwr);
	STATS_32(stdout, "Rx Rssi Combind", link->cs_rx_rssi_comb);
	STATS_16_SIGNED(stdout, "Channel NF", link->cs_chan_nf);
	STATS_16_SIGNED(stdout, "Channel NF Sec80", link->cs_chan_nf_sec80);
	STATS_8(stdout, "DCS Total Util", link->dcs_total_util);
}

void print_basic_ap_data_tx(struct basic_psoc_data_tx *tx)
{
	STATS_64(stdout, "Tx Egress Pkts", tx->egress.num);
	STATS_64(stdout, "Tx Egress Bytes", tx->egress.bytes);
}

void print_basic_ap_data_rx(struct basic_psoc_data_rx *rx)
{
	STATS_64(stdout, "Rx Ingress Pkts", rx->ingress.num);
	STATS_64(stdout, "Rx Ingress Bytes", rx->ingress.bytes);
}

#if WLAN_ADVANCE_TELEMETRY
void print_advance_data_tx_stats(struct advance_data_tx_stats *tx)
{
	u_int8_t inx = 0;

	STATS_64(stdout, "Tx Unicast Packets", tx->ucast.num);
	STATS_64(stdout, "Tx Unicast Bytes", tx->ucast.bytes);
	STATS_64(stdout, "Tx Multicast Packets", tx->mcast.num);
	STATS_64(stdout, "Tx Multicast Bytes", tx->mcast.bytes);
	STATS_64(stdout, "Tx Broadcast Packets", tx->bcast.num);
	STATS_64(stdout, "Tx Broadcast Bytes", tx->bcast.bytes);
	STATS_PRINT("\tTx SGI = 0.8us %u, 0.4us %u, 1.6us %u, 3.2us %u\n",
		    tx->sgi_count[0], tx->sgi_count[1],
		    tx->sgi_count[2], tx->sgi_count[3]);
	STATS_PRINT("\tTx NSS (1-%d)\n\t", STATS_IF_SS_COUNT);
	for (inx = 0; inx < STATS_IF_SS_COUNT; inx++)
		STATS_PRINT(" %u = %u ", (inx + 1), tx->nss[inx]);
	STATS_PRINT("\n\tTx BW Counts = 20MHZ %u 40MHZ %u 80MHZ %u 160MHZ %u\n",
		    tx->bw[0], tx->bw[1], tx->bw[2], tx->bw[3]);
	STATS_32(stdout, "Tx Retries", tx->retries);
	STATS_PRINT("\tTx Aggregation:\n");
	STATS_32(stdout, "MSDU's Part of AMPDU", tx->ampdu_cnt);
	STATS_32(stdout, "MSDU's With No MPDU Level Aggregation",
		 tx->non_ampdu_cnt);
	STATS_32(stdout, "MSDU's Part of AMSDU", tx->amsdu_cnt);
	STATS_32(stdout, "MSDU's With No MSDU Level Aggregation",
		 tx->non_amsdu_cnt);
}

void print_advance_data_rx_stats(struct advance_data_rx_stats *rx)
{
	u_int8_t inx = 0;

	STATS_64(stdout, "Rx Unicast Packets", rx->unicast.num);
	STATS_64(stdout, "Rx Unicast Bytes", rx->unicast.bytes);
	STATS_64(stdout, "Rx Multicast Packets", rx->multicast.num);
	STATS_64(stdout, "Rx Multicast Bytes", rx->multicast.bytes);
	STATS_64(stdout, "Rx Broadcast Packets", rx->bcast.num);
	STATS_64(stdout, "Rx Broadcast Bytes", rx->bcast.bytes);
	STATS_32(stdout, "Rx Retries", rx->rx_retries);
	STATS_32(stdout, "Rx Multipass Packet Drop", rx->multipass_rx_pkt_drop);
	STATS_32(stdout, "Rx BAR Reaceive Count", rx->bar_recv_cnt);
	STATS_32(stdout, "Rx MPDU FCS Ok Count", rx->mpdu_cnt_fcs_ok);
	STATS_32(stdout, "Rx MPDU FCS Error Count", rx->mpdu_cnt_fcs_err);
	STATS_PRINT("\tRx PPDU Counts\n");
	for (inx = 0; inx < STATS_IF_MAX_MCS; inx++) {
		if (!rate_string[STATS_IF_DOT11_AX][inx].valid)
			continue;
		STATS_PRINT("\t\t%s = %u\n",
			    rate_string[STATS_IF_DOT11_AX][inx].mcs_type,
			    rx->su_ax_ppdu_cnt[inx]);
	}
	STATS_PRINT("\tRx SGI = 0.8us %u, 0.4us %u, 1.6us %u, 3.2us %u\n",
		    rx->sgi_count[0], rx->sgi_count[1],
		    rx->sgi_count[2], rx->sgi_count[3]);
	STATS_PRINT("\tRx MSDU Counts for NSS (1-%u)\n\t", STATS_IF_SS_COUNT);
	for (inx = 0; inx < STATS_IF_SS_COUNT; inx++)
		STATS_PRINT(" %u = %u ", (inx + 1), rx->nss[inx]);
	STATS_PRINT("\n\tRx PPDU Counts for NSS (1-%u) in SU mode\n\t",
		    STATS_IF_SS_COUNT);
	for (inx = 0; inx < STATS_IF_SS_COUNT; inx++)
		STATS_PRINT(" %u = %u ", (inx + 1), rx->ppdu_nss[inx]);
	STATS_PRINT("\n\tRx BW Counts = 20MHZ %u 40MHZ %u 80MHZ %u 160MHZ %u\n",
		    rx->bw[0], rx->bw[1], rx->bw[2], rx->bw[3]);
	STATS_PRINT("\tRx Data Packets per AC\n");
	STATS_32(stdout, "     Best effort",
		 rx->wme_ac_type[STATS_IF_WME_AC_BE]);
	STATS_32(stdout, "      Background",
		 rx->wme_ac_type[STATS_IF_WME_AC_BK]);
	STATS_32(stdout, "           Video",
		 rx->wme_ac_type[STATS_IF_WME_AC_VI]);
	STATS_32(stdout, "           Voice",
		 rx->wme_ac_type[STATS_IF_WME_AC_VO]);
	STATS_PRINT("\tRx Aggregation:\n");
	STATS_32(stdout, "MSDU's Part of AMPDU", rx->ampdu_cnt);
	STATS_32(stdout, "MSDU's With No MPDU Level Aggregation",
		 rx->non_ampdu_cnt);
	STATS_32(stdout, "MSDU's Part of AMSDU", rx->amsdu_cnt);
	STATS_32(stdout, "MSDU's With No MSDU Level Aggregation",
		 rx->non_amsdu_cnt);
}

void print_advance_sta_data_tx(struct advance_peer_data_tx *tx)
{
	print_basic_sta_data_tx(&tx->b_tx);
	print_advance_data_tx_stats(&tx->adv_tx);
}

void print_advance_sta_data_rx(struct advance_peer_data_rx *rx)
{
	print_basic_sta_data_rx(&rx->b_rx);
	print_advance_data_rx_stats(&rx->adv_rx);
}

void print_advance_sta_data_fwd(struct advance_peer_data_fwd *fwd)
{
	STATS_64(stdout, "Intra BSS Packets", fwd->pkts.num);
	STATS_64(stdout, "Intra BSS Bytes", fwd->pkts.bytes);
	STATS_64(stdout, "Intra BSS Fail Packets", fwd->fail.num);
	STATS_64(stdout, "Intra BSS Fail Bytes", fwd->fail.bytes);
	STATS_32(stdout, "Intra BSS MDNS No FWD", fwd->mdns_no_fwd);
}

void print_advance_sta_data_raw(struct advance_peer_data_raw *raw)
{
	STATS_64(stdout, "Raw Packets", raw->raw.num);
	STATS_64(stdout, "Raw Bytes", raw->raw.bytes);
}

void print_advance_sta_data_twt(struct advance_peer_data_twt *twt)
{
	STATS_64(stdout, "Rx TWT Packets", twt->to_stack_twt.num);
	STATS_64(stdout, "Rx TWT Bytes", twt->to_stack_twt.bytes);
	STATS_64(stdout, "Tx TWT Packets", twt->tx_success_twt.num);
	STATS_64(stdout, "Tx TWT Bytes", twt->tx_success_twt.bytes);
}

void print_advance_sta_data_link(struct advance_peer_data_link *link)
{
	print_basic_sta_data_link(&link->b_link);
	STATS_32(stdout, "Rx SNR Time", link->rx_snr_measured_time);
}

void print_advance_sta_data_rate(struct advance_peer_data_rate *rate)
{
	print_basic_sta_data_rate(&rate->b_rate);
	STATS_32(stdout, "Avg ppdu Rx rate(kbps)", rate->rnd_avg_rx_rate);
	STATS_32(stdout, "Avg ppdu Rx rate", rate->avg_rx_rate);
	STATS_32(stdout, "Avg ppdu Tx rate(kbps)", rate->rnd_avg_tx_rate);
	STATS_32(stdout, "Avg ppdu Tx rate", rate->avg_tx_rate);
}

void print_advance_sta_data_nawds(struct advance_peer_data_nawds *nawds)
{
	STATS_64(stdout, "Multicast Packets", nawds->nawds_mcast.num);
	STATS_64(stdout, "Multicast Bytes", nawds->nawds_mcast.bytes);
	STATS_32(stdout, "NAWDS Tx Drop Count", nawds->nawds_mcast_tx_drop);
	STATS_32(stdout, "NAWDS Rx Drop Count", nawds->nawds_mcast_rx_drop);
}

void print_advance_sta_ctrl_tx(struct advance_peer_ctrl_tx *tx)
{
	print_basic_sta_ctrl_tx(&tx->b_tx);
	STATS_32(stdout, "Assocition Tx Count", tx->cs_tx_assoc);
	STATS_32(stdout, "Assocition Tx Failed Count", tx->cs_tx_assoc_fail);
}

void print_advance_sta_ctrl_rx(struct advance_peer_ctrl_rx *rx)
{
	print_basic_sta_ctrl_rx(&rx->b_rx);
}

void print_advance_sta_ctrl_twt(struct advance_peer_ctrl_twt *twt)
{
	STATS_32(stdout, "TWT Event Type", twt->cs_twt_event_type);
	STATS_32(stdout, "TWT Flow ID", twt->cs_twt_flow_id);
	STATS_32(stdout, "Broadcast TWT", twt->cs_twt_bcast);
	STATS_32(stdout, "TWT Trigger", twt->cs_twt_trig);
	STATS_32(stdout, "TWT Announcement", twt->cs_twt_announ);
	STATS_32(stdout, "TWT Dialog ID", twt->cs_twt_dialog_id);
	STATS_32(stdout, "TWT Wake duration (us)", twt->cs_twt_wake_dura_us);
	STATS_32(stdout, "TWT Wake interval (us)", twt->cs_twt_wake_intvl_us);
	STATS_32(stdout, "TWT SP offset (us)", twt->cs_twt_sp_offset_us);
}

void print_advance_sta_ctrl_link(struct advance_peer_ctrl_link *link)
{
	print_basic_sta_ctrl_link(&link->b_link);
}

void print_advance_sta_ctrl_rate(struct advance_peer_ctrl_rate *rate)
{
	print_basic_sta_ctrl_rate(&rate->b_rate);
}

void print_advance_vap_data_me(struct advance_vdev_data_me *me)
{
	STATS_64(stdout, "Multicast Packets", me->mcast_pkt.num);
	STATS_64(stdout, "Multicast Bytes", me->mcast_pkt.bytes);
	STATS_32(stdout, "Unicast Packets", me->ucast);
}

void print_advance_vap_data_tx(struct advance_vdev_data_tx *tx)
{
	print_basic_vap_data_tx(&tx->b_tx);
	print_advance_data_tx_stats(&tx->adv_tx);
	STATS_64(stdout, "Tx Reinject Packets", tx->reinject_pkts.num);
	STATS_64(stdout, "Tx Reinject Bytes", tx->reinject_pkts.bytes);
	STATS_64(stdout, "Tx Inspect Packets", tx->inspect_pkts.num);
	STATS_64(stdout, "Tx Inspect Bytes", tx->inspect_pkts.bytes);
	STATS_32(stdout, "Tx CCE Classified", tx->cce_classified);
}

void print_advance_vap_data_rx(struct advance_vdev_data_rx *rx)
{
	print_basic_vap_data_rx(&rx->b_rx);
	print_advance_data_rx_stats(&rx->adv_rx);
}

void print_advance_vap_data_raw(struct advance_vdev_data_raw *raw)
{
	STATS_64(stdout, "RAW Rx Packets", raw->rx_raw.num);
	STATS_64(stdout, "RAW Rx Bytes", raw->rx_raw.bytes);
	STATS_64(stdout, "RAW Tx Packets", raw->tx_raw_pkt.num);
	STATS_64(stdout, "RAW Tx Bytes", raw->tx_raw_pkt.bytes);
	STATS_32(stdout, "RAW Tx Classified by CCE", raw->cce_classified_raw);
}

void print_advance_vap_data_tso(struct advance_vdev_data_tso *tso)
{
	STATS_64(stdout, "SG Packets", tso->sg_pkt.num);
	STATS_64(stdout, "SG Bytess", tso->sg_pkt.bytes);
	STATS_64(stdout, "Non-SG Packets", tso->non_sg_pkts.num);
	STATS_64(stdout, "Non-SG Bytess", tso->non_sg_pkts.bytes);
	STATS_64(stdout, "TSO Packets", tso->num_tso_pkts.num);
	STATS_64(stdout, "TSO Bytess", tso->num_tso_pkts.bytes);
}

void print_advance_vap_data_igmp(struct advance_vdev_data_igmp *igmp)
{
	STATS_32(stdout, "IGMP Received", igmp->igmp_rcvd);
	STATS_32(stdout, "IGMP Converted Unicast", igmp->igmp_ucast_converted);
}

void print_advance_vap_data_mesh(struct advance_vdev_data_mesh *mesh)
{
	STATS_32(stdout, "MESH FW Completion Count", mesh->completion_fw);
	STATS_32(stdout, "MESH FW Exception Count", mesh->exception_fw);
}

void print_advance_vap_data_nawds(struct advance_vdev_data_nawds *nawds)
{
	STATS_64(stdout, "Multicast Tx Packets", nawds->tx_nawds_mcast.num);
	STATS_64(stdout, "Multicast Tx Bytes", nawds->tx_nawds_mcast.bytes);
	STATS_32(stdout, "NAWDS Tx Drop Count", nawds->nawds_mcast_tx_drop);
	STATS_32(stdout, "NAWDS Rx Drop Count", nawds->nawds_mcast_rx_drop);
}

void print_advance_vap_ctrl_tx(struct advance_vdev_ctrl_tx *tx)
{
	print_basic_vap_ctrl_tx(&tx->b_tx);
	STATS_64(stdout, "Tx Off Channel Management Count",
		 tx->cs_tx_offchan_mgmt);
	STATS_64(stdout, "Tx Off Channel Data Count", tx->cs_tx_offchan_data);
	STATS_64(stdout, "Tx Off Channel Fail Count", tx->cs_tx_offchan_fail);
	STATS_64(stdout, "Tx Beacon Count", tx->cs_tx_bcn_success);
	STATS_64(stdout, "Tx Beacon Outage Count", tx->cs_tx_bcn_outage);
	STATS_64(stdout, "Tx FILS Frame Sent Count", tx->cs_fils_frames_sent);
	STATS_64(stdout, "Tx FILS Frame Sent Fail",
		 tx->cs_fils_frames_sent_fail);
	STATS_64(stdout, "Tx Offload Probe Response Success Count",
		 tx->cs_tx_offload_prb_resp_succ_cnt);
	STATS_64(stdout, "Tx Offload Probe Response Fail Count",
		 tx->cs_tx_offload_prb_resp_fail_cnt);
}

void print_advance_vap_ctrl_rx(struct advance_vdev_ctrl_rx *rx)
{
	print_basic_vap_ctrl_rx(&rx->b_rx);
	STATS_64(stdout, "Rx Action Frame Count", rx->cs_rx_action);
	STATS_64(stdout, "Rx MLME Auth Attempt", rx->cs_mlme_auth_attempt);
	STATS_64(stdout, "Rx MLME Auth Success", rx->cs_mlme_auth_success);
	STATS_64(stdout, "Authorization Attempt", rx->cs_authorize_attempt);
	STATS_64(stdout, "Authorization Success", rx->cs_authorize_success);
	STATS_64(stdout, "Probe Request Drops", rx->cs_prob_req_drops);
	STATS_64(stdout, "OOB Probe Requests", rx->cs_oob_probe_req_count);
	STATS_64(stdout, "Wildcard probe requests drops",
		 rx->cs_wc_probe_req_drops);
	STATS_64(stdout, "Connections refuse Radio limit",
		 rx->cs_sta_xceed_rlim);
	STATS_64(stdout, "Connections refuse Vap limit", rx->cs_sta_xceed_vlim);
}

void print_advance_radio_data_me(struct advance_pdev_data_me *me)
{
	STATS_64(stdout, "Multicast Packets", me->mcast_pkt.num);
	STATS_64(stdout, "Multicast Bytes", me->mcast_pkt.bytes);
	STATS_32(stdout, "Unicast Packets", me->ucast);
}

void print_histogram_stats(struct histogram_stats *hist)
{
	STATS_32(stdout, " Single Packets", hist->pkts_1);
	STATS_32(stdout, "   2-20 Packets", hist->pkts_2_20);
	STATS_32(stdout, "  21-40 Packets", hist->pkts_21_40);
	STATS_32(stdout, "  41-60 Packets", hist->pkts_41_60);
	STATS_32(stdout, "  61-80 Packets", hist->pkts_61_80);
	STATS_32(stdout, " 81-100 Packets", hist->pkts_81_100);
	STATS_32(stdout, "101-200 Packets", hist->pkts_101_200);
	STATS_32(stdout, "   200+ Packets", hist->pkts_201_plus);
}

void print_advance_radio_data_tx(struct advance_pdev_data_tx *tx)
{
	print_basic_radio_data_tx(&tx->b_tx);
	print_advance_data_tx_stats(&tx->adv_tx);
	STATS_64(stdout, "Tx Reinject Packets", tx->reinject_pkts.num);
	STATS_64(stdout, "Tx Reinject Bytes", tx->reinject_pkts.bytes);
	STATS_64(stdout, "Tx Inspect Packets", tx->inspect_pkts.num);
	STATS_64(stdout, "Tx Inspect Bytes", tx->inspect_pkts.bytes);
	STATS_32(stdout, "Tx CCE Classified", tx->cce_classified);
	STATS_PRINT("\tTx packets sent per interrupt\n");
	print_histogram_stats(&tx->tx_hist);
}

void print_advance_radio_data_rx(struct advance_pdev_data_rx *rx)
{
	print_basic_radio_data_rx(&rx->b_rx);
	print_advance_data_rx_stats(&rx->adv_rx);
	STATS_PRINT("\tRx packets sent per interrupt\n");
	print_histogram_stats(&rx->rx_hist);
}

void print_advance_radio_data_raw(struct advance_pdev_data_raw *raw)
{
	STATS_64(stdout, "RAW Rx Packets", raw->rx_raw.num);
	STATS_64(stdout, "RAW Rx Bytes", raw->rx_raw.bytes);
	STATS_64(stdout, "RAW Tx Packets", raw->tx_raw_pkt.num);
	STATS_64(stdout, "RAW Tx Bytes", raw->tx_raw_pkt.bytes);
	STATS_32(stdout, "RAW Tx Classified by CCE", raw->cce_classified_raw);
	STATS_32(stdout, "RAW Rx Packets", raw->rx_raw_pkts);
}

void print_advance_radio_data_tso(struct advance_pdev_data_tso *tso)
{
	STATS_64(stdout, "SG Packets", tso->sg_pkt.num);
	STATS_64(stdout, "SG Bytess", tso->sg_pkt.bytes);
	STATS_64(stdout, "Non-SG Packets", tso->non_sg_pkts.num);
	STATS_64(stdout, "Non-SG Bytess", tso->non_sg_pkts.bytes);
	STATS_64(stdout, "TSO Packets", tso->num_tso_pkts.num);
	STATS_64(stdout, "TSO Bytess", tso->num_tso_pkts.bytes);
	STATS_32(stdout, "TSO Complete", tso->tso_comp);
	STATS_PRINT("\tTSO Histogram\n");
	STATS_64(stdout, "    Single", tso->segs_1);
	STATS_64(stdout, "  2-5 segs", tso->segs_2_5);
	STATS_64(stdout, " 6-10 segs", tso->segs_6_10);
	STATS_64(stdout, "11-15 segs", tso->segs_11_15);
	STATS_64(stdout, "16-20 segs", tso->segs_16_20);
	STATS_64(stdout, "  20+ segs", tso->segs_20_plus);
}

void print_advance_radio_data_igmp(struct advance_pdev_data_igmp *igmp)
{
	STATS_32(stdout, "IGMP Received", igmp->igmp_rcvd);
	STATS_32(stdout, "IGMP Converted Unicast", igmp->igmp_ucast_converted);
}

void print_advance_radio_data_mesh(struct advance_pdev_data_mesh *mesh)
{
	STATS_32(stdout, "MESH FW Completion Count", mesh->completion_fw);
	STATS_32(stdout, "MESH FW Exception Count", mesh->exception_fw);
}

void print_advance_radio_data_nawds(struct advance_pdev_data_nawds *nawds)
{
	STATS_64(stdout, "Multicast Tx Packets", nawds->tx_nawds_mcast.num);
	STATS_64(stdout, "Multicast Tx Bytes", nawds->tx_nawds_mcast.bytes);
	STATS_32(stdout, "NAWDS Tx Drop Count", nawds->nawds_mcast_tx_drop);
	STATS_32(stdout, "NAWDS Rx Drop Count", nawds->nawds_mcast_rx_drop);
}

void print_advance_radio_ctrl_tx(struct advance_pdev_ctrl_tx *tx)
{
	print_basic_radio_ctrl_tx(&tx->b_tx);
	STATS_64(stdout, "Tx Beacon Count", tx->cs_tx_beacon);
	STATS_64(stdout, "Tx Retries Count", tx->cs_tx_retries);
}

void print_advance_radio_ctrl_rx(struct advance_pdev_ctrl_rx *rx)
{
	print_basic_radio_ctrl_rx(&rx->b_rx);
	STATS_32(stdout, "Rx Mgmt Frames dropped (RSSI too low)",
		 rx->cs_rx_mgmt_rssi_drop);
}

void print_advance_radio_ctrl_link(struct advance_pdev_ctrl_link *link)
{
	print_basic_radio_ctrl_link(&link->b_link);
	STATS_8(stdout, "DCS Tx Utilized", link->dcs_ap_tx_util);
	STATS_8(stdout, "DCS Rx Utilized", link->dcs_ap_rx_util);
	STATS_8(stdout, "DCS Self BSS Utilized", link->dcs_self_bss_util);
	STATS_8(stdout, "DCS OBSS Utilized", link->dcs_obss_util);
	STATS_8(stdout, "DCS OBSS Rx Utilised", link->dcs_obss_rx_util);
	STATS_8(stdout, "DCS Free Medium", link->dcs_free_medium);
	STATS_8(stdout, "DCS Non-WiFi Utilized", link->dcs_non_wifi_util);
	STATS_32(stdout, "DCS Under SS Utilized", link->dcs_ss_under_util);
	STATS_32(stdout, "DCS Secondary 20 Utilized", link->dcs_sec_20_util);
	STATS_32(stdout, "DCS Secondary 40 Utilized", link->dcs_sec_40_util);
	STATS_32(stdout, "DCS Secondary 80 Utilized", link->dcs_sec_80_util);
	STATS_8(stdout, "Rx RSSI (Chain 0) Pri 20", link->rx_rssi_chain0_pri20);
	STATS_8(stdout, "Rx RSSI (Chain 0) Sec 20", link->rx_rssi_chain0_sec20);
	STATS_8(stdout, "Rx RSSI (Chain 0) Sec 40", link->rx_rssi_chain0_sec40);
	STATS_8(stdout, "Rx RSSI (Chain 0) Sec 80", link->rx_rssi_chain0_sec80);
	STATS_8(stdout, "Rx RSSI (Chain 1) Pri 20", link->rx_rssi_chain1_pri20);
	STATS_8(stdout, "Rx RSSI (Chain 1) Sec 20", link->rx_rssi_chain1_sec20);
	STATS_8(stdout, "Rx RSSI (Chain 1) Sec 40", link->rx_rssi_chain1_sec40);
	STATS_8(stdout, "Rx RSSI (Chain 1) Sec 80", link->rx_rssi_chain1_sec80);
	STATS_8(stdout, "Rx RSSI (Chain 2) Pri 20", link->rx_rssi_chain2_pri20);
	STATS_8(stdout, "Rx RSSI (Chain 2) Sec 20", link->rx_rssi_chain2_sec20);
	STATS_8(stdout, "Rx RSSI (Chain 2) Sec 40", link->rx_rssi_chain2_sec40);
	STATS_8(stdout, "Rx RSSI (Chain 2) Sec 80", link->rx_rssi_chain2_sec80);
	STATS_8(stdout, "Rx RSSI (Chain 3) Pri 20", link->rx_rssi_chain3_pri20);
	STATS_8(stdout, "Rx RSSI (Chain 3) Sec 20", link->rx_rssi_chain3_sec20);
	STATS_8(stdout, "Rx RSSI (Chain 3) Sec 40", link->rx_rssi_chain3_sec40);
	STATS_8(stdout, "Rx RSSI (Chain 3) Sec 80", link->rx_rssi_chain3_sec80);
	STATS_32(stdout, "Tx RSSI", link->cs_tx_rssi);
}

void print_advance_ap_data_tx(struct advance_psoc_data_tx *tx)
{
	print_basic_ap_data_tx(&tx->b_tx);
}

void print_advance_ap_data_rx(struct advance_psoc_data_rx *rx)
{
	print_basic_ap_data_rx(&rx->b_rx);
	STATS_32(stdout, "Rx Ring Error Packets", rx->err_ring_pkts);
	STATS_32(stdout, "Rx Fragments", rx->rx_frags);
	STATS_32(stdout, "Rx HW Reinject", rx->rx_hw_reinject);
	STATS_32(stdout, "Rx BAR Frames", rx->bar_frame);
	STATS_32(stdout, "Rx Rejected", rx->rejected);
	STATS_32(stdout, "Rx RAW Frame Drop", rx->raw_frm_drop);
}
#endif /* WLAN_ADVANCE_TELEMETRY */

void print_basic_sta_data(struct stats_obj *sta)
{
	struct basic_peer_data *data = sta->stats;

	STATS_PRINT("Basic Data STATS For STA %s (Parent %s)\n",
		    macaddr_to_str(sta->u_id.mac_addr), sta->pif_name);
	if (data->tx) {
		STATS_PRINT("Tx Stats\n");
		print_basic_sta_data_tx(data->tx);
	}
	if (data->rx) {
		STATS_PRINT("Rx Stats\n");
		print_basic_sta_data_rx(data->rx);
	}
	if (data->link) {
		STATS_PRINT("Link Stats\n");
		print_basic_sta_data_link(data->link);
	}
	if (data->rate) {
		STATS_PRINT("Rate Stats\n");
		print_basic_sta_data_rate(data->rate);
	}
}

void print_basic_sta_ctrl(struct stats_obj *sta)
{
	struct basic_peer_ctrl *ctrl = sta->stats;

	STATS_PRINT("Basic Control STATS For STA %s (Parent %s)\n",
		    macaddr_to_str(sta->u_id.mac_addr), sta->pif_name);
	if (ctrl->tx) {
		STATS_PRINT("Tx Stats\n");
		print_basic_sta_ctrl_tx(ctrl->tx);
	}
	if (ctrl->rx) {
		STATS_PRINT("Rx Stats\n");
		print_basic_sta_ctrl_rx(ctrl->rx);
	}
	if (ctrl->link) {
		STATS_PRINT("Link Stats\n");
		print_basic_sta_ctrl_link(ctrl->link);
	}
	if (ctrl->rate) {
		STATS_PRINT("Rate Stats\n");
		print_basic_sta_ctrl_rate(ctrl->rate);
	}
}

void print_basic_vap_data(struct stats_obj *vap)
{
	struct basic_vdev_data *data = vap->stats;

	STATS_PRINT("Basic Data STATS For VAP %s (Parent %s)\n",
		    vap->u_id.if_name, vap->pif_name);
	if (data->tx) {
		STATS_PRINT("Tx Stats\n");
		print_basic_vap_data_tx(data->tx);
	}
	if (data->rx) {
		STATS_PRINT("Rx Stats\n");
		print_basic_vap_data_rx(data->rx);
	}
}

void print_basic_vap_ctrl(struct stats_obj *vap)
{
	struct basic_vdev_ctrl *ctrl = vap->stats;

	STATS_PRINT("Basic Control STATS For VAP %s (Parent %s)\n",
		    vap->u_id.if_name, vap->pif_name);
	if (ctrl->tx) {
		STATS_PRINT("Tx Stats\n");
		print_basic_vap_ctrl_tx(ctrl->tx);
	}
	if (ctrl->rx) {
		STATS_PRINT("Rx Stats\n");
		print_basic_vap_ctrl_rx(ctrl->rx);
	}
}

void print_basic_radio_data(struct stats_obj *radio)
{
	struct basic_pdev_data *data = radio->stats;

	STATS_PRINT("Basic Data STATS for Radio %s (Parent %s)\n",
		    radio->u_id.if_name, radio->pif_name);
	if (data->tx) {
		STATS_PRINT("Tx Stats\n");
		print_basic_radio_data_tx(data->tx);
	}
	if (data->rx) {
		STATS_PRINT("Rx Stats\n");
		print_basic_radio_data_rx(data->rx);
	}
}

void print_basic_radio_ctrl(struct stats_obj *radio)
{
	struct basic_pdev_ctrl *ctrl = radio->stats;

	STATS_PRINT("Basic Control STATS for Radio %s (Parent %s)\n",
		    radio->u_id.if_name, radio->pif_name);
	if (ctrl->tx) {
		STATS_PRINT("Tx Stats\n");
		print_basic_radio_ctrl_tx(ctrl->tx);
	}
	if (ctrl->rx) {
		STATS_PRINT("Rx Stats\n");
		print_basic_radio_ctrl_rx(ctrl->rx);
	}
	if (ctrl->link) {
		STATS_PRINT("Link Stats\n");
		print_basic_radio_ctrl_link(ctrl->link);
	}
}

void print_basic_ap_data(struct stats_obj *ap)
{
	struct basic_psoc_data *data = ap->stats;

	STATS_PRINT("Basic Data STATS for AP %s\n", ap->u_id.if_name);
	if (data->tx) {
		STATS_PRINT("Tx Stats\n");
		print_basic_ap_data_tx(data->tx);
	}
	if (data->rx) {
		STATS_PRINT("Rx Stats\n");
		print_basic_ap_data_rx(data->rx);
	}
}

#if WLAN_ADVANCE_TELEMETRY
void print_advance_sta_data(struct stats_obj *sta)
{
	struct advance_peer_data *data = sta->stats;

	STATS_PRINT("Advance Data STATS For STA %s (Parent %s)\n",
		    macaddr_to_str(sta->u_id.mac_addr), sta->pif_name);
	if (data->tx) {
		STATS_PRINT("Tx Stats\n");
		print_advance_sta_data_tx(data->tx);
	}
	if (data->rx) {
		STATS_PRINT("Rx Stats\n");
		print_advance_sta_data_rx(data->rx);
	}
	if (data->fwd) {
		STATS_PRINT("Intra Bss (FWD) Stats\n");
		print_advance_sta_data_fwd(data->fwd);
	}
	if (data->raw) {
		STATS_PRINT("RAW Stats\n");
		print_advance_sta_data_raw(data->raw);
	}
	if (data->twt) {
		STATS_PRINT("TWT Stats\n");
		print_advance_sta_data_twt(data->twt);
	}
	if (data->link) {
		STATS_PRINT("Link Stats\n");
		print_advance_sta_data_link(data->link);
	}
	if (data->rate) {
		STATS_PRINT("Rate Stats\n");
		print_advance_sta_data_rate(data->rate);
	}
	if (data->nawds) {
		STATS_PRINT("NAWDS Stats\n");
		print_advance_sta_data_nawds(data->nawds);
	}
}

void print_advance_sta_ctrl(struct stats_obj *sta)
{
	struct advance_peer_ctrl *ctrl = sta->stats;

	STATS_PRINT("Advance Control STATS For STA %s (Parent %s)\n",
		    macaddr_to_str(sta->u_id.mac_addr), sta->pif_name);
	if (ctrl->tx) {
		STATS_PRINT("Tx Stats\n");
		print_advance_sta_ctrl_tx(ctrl->tx);
	}
	if (ctrl->rx) {
		STATS_PRINT("Rx Stats\n");
		print_advance_sta_ctrl_rx(ctrl->rx);
	}
	if (ctrl->twt) {
		STATS_PRINT("TWT Stats\n");
		print_advance_sta_ctrl_twt(ctrl->twt);
	}
	if (ctrl->link) {
		STATS_PRINT("Link Stats\n");
		print_advance_sta_ctrl_link(ctrl->link);
	}
	if (ctrl->rate) {
		STATS_PRINT("Rate Stats\n");
		print_advance_sta_ctrl_rate(ctrl->rate);
	}
}

void print_advance_vap_data(struct stats_obj *vap)
{
	struct advance_vdev_data *data = vap->stats;

	STATS_PRINT("Advance Data STATS for Vap %s (Parent %s)\n",
		    vap->u_id.if_name, vap->pif_name);
	if (data->me) {
		STATS_PRINT("ME Stats\n");
		print_advance_vap_data_me(data->me);
	}
	if (data->tx) {
		STATS_PRINT("Tx Stats\n");
		print_advance_vap_data_tx(data->tx);
	}
	if (data->rx) {
		STATS_PRINT("Rx Stats\n");
		print_advance_vap_data_rx(data->rx);
	}
	if (data->raw) {
		STATS_PRINT("RAW Stats\n");
		print_advance_vap_data_raw(data->raw);
	}
	if (data->tso) {
		STATS_PRINT("TSO Stats\n");
		print_advance_vap_data_tso(data->tso);
	}
	if (data->igmp) {
		STATS_PRINT("IGMP Stats\n");
		print_advance_vap_data_igmp(data->igmp);
	}
	if (data->mesh) {
		STATS_PRINT("MESH Stats\n");
		print_advance_vap_data_mesh(data->mesh);
	}
	if (data->nawds) {
		STATS_PRINT("NAWDS Stats\n");
		print_advance_vap_data_nawds(data->nawds);
	}
}

void print_advance_vap_ctrl(struct stats_obj *vap)
{
	struct advance_vdev_ctrl *ctrl = vap->stats;

	STATS_PRINT("Advance Control STATS for Vap %s (Parent %s)\n",
		    vap->u_id.if_name, vap->pif_name);
	if (ctrl->tx) {
		STATS_PRINT("Tx Stats\n");
		print_advance_vap_ctrl_tx(ctrl->tx);
	}
	if (ctrl->rx) {
		STATS_PRINT("Rx Stats\n");
		print_advance_vap_ctrl_rx(ctrl->rx);
	}
}

void print_advance_radio_data(struct stats_obj *radio)
{
	struct advance_pdev_data *data = radio->stats;

	STATS_PRINT("Advance Data STATS for Radio %s (Parent %s)\n",
		    radio->u_id.if_name, radio->pif_name);
	if (data->me) {
		STATS_PRINT("ME Stats\n");
		print_advance_radio_data_me(data->me);
	}
	if (data->tx) {
		STATS_PRINT("Tx Stats\n");
		print_advance_radio_data_tx(data->tx);
	}
	if (data->rx) {
		STATS_PRINT("Rx Stats\n");
		print_advance_radio_data_rx(data->rx);
	}
	if (data->raw) {
		STATS_PRINT("RAW Stats\n");
		print_advance_radio_data_raw(data->raw);
	}
	if (data->tso) {
		STATS_PRINT("TSO Stats\n");
		print_advance_radio_data_tso(data->tso);
	}
	if (data->igmp) {
		STATS_PRINT("IGMP Stats\n");
		print_advance_radio_data_igmp(data->igmp);
	}
	if (data->mesh) {
		STATS_PRINT("MESH Stats\n");
		print_advance_radio_data_mesh(data->mesh);
	}
	if (data->nawds) {
		STATS_PRINT("NAWDS Stats\n");
		print_advance_radio_data_nawds(data->nawds);
	}
}

void print_advance_radio_ctrl(struct stats_obj *radio)
{
	struct advance_pdev_ctrl *ctrl = radio->stats;

	STATS_PRINT("Advance Control STATS for Radio %s (Parent %s)\n",
		    radio->u_id.if_name, radio->pif_name);
	if (ctrl->tx) {
		STATS_PRINT("Tx Stats\n");
		print_advance_radio_ctrl_tx(ctrl->tx);
	}
	if (ctrl->rx) {
		STATS_PRINT("Rx Stats\n");
		print_advance_radio_ctrl_rx(ctrl->rx);
	}
	if (ctrl->link) {
		STATS_PRINT("Link Stats\n");
		print_advance_radio_ctrl_link(ctrl->link);
	}
}

void print_advance_ap_data(struct stats_obj *ap)
{
	struct advance_psoc_data *data = ap->stats;

	STATS_PRINT("Advance Data STATS for AP %s\n", ap->u_id.if_name);
	if (data->tx) {
		STATS_PRINT("Tx Stats\n");
		print_advance_ap_data_tx(data->tx);
	}
	if (data->rx) {
		STATS_PRINT("Rx Stats\n");
		print_advance_ap_data_rx(data->rx);
	}
}
#endif /* WLAN_ADVANCE_TELEMETRY */

void print_basic_stats(struct stats_obj *obj)
{
	switch (obj->obj_type) {
	case STATS_OBJ_STA:
		if (obj->type == STATS_TYPE_DATA)
			print_basic_sta_data(obj);
		else
			print_basic_sta_ctrl(obj);
		break;
	case STATS_OBJ_VAP:
		if (obj->type == STATS_TYPE_DATA)
			print_basic_vap_data(obj);
		else
			print_basic_vap_ctrl(obj);
		break;
	case STATS_OBJ_RADIO:
		if (obj->type == STATS_TYPE_DATA)
			print_basic_radio_data(obj);
		else
			print_basic_radio_ctrl(obj);
		break;
	case STATS_OBJ_AP:
		if (obj->type == STATS_TYPE_DATA)
			print_basic_ap_data(obj);
		break;
	default:
		STATS_ERR("Invalid object option\n");
	}
}

#if WLAN_ADVANCE_TELEMETRY
void print_advance_stats(struct stats_obj *obj)
{
	switch (obj->obj_type) {
	case STATS_OBJ_STA:
		if (obj->type == STATS_TYPE_DATA)
			print_advance_sta_data(obj);
		else
			print_advance_sta_ctrl(obj);
		break;
	case STATS_OBJ_VAP:
		if (obj->type == STATS_TYPE_DATA)
			print_advance_vap_data(obj);
		else
			print_advance_vap_ctrl(obj);
		break;
	case STATS_OBJ_RADIO:
		if (obj->type == STATS_TYPE_DATA)
			print_advance_radio_data(obj);
		else
			print_advance_radio_ctrl(obj);
		break;
	case STATS_OBJ_AP:
		if (obj->type == STATS_TYPE_DATA)
			print_advance_ap_data(obj);
		break;
	default:
		STATS_ERR("Invalid object option\n");
	}
}
#endif /* WLAN_ADVANCE_TELEMETRY */

void print_response(struct reply_buffer *reply)
{
	struct stats_obj *obj = reply->obj_head;

	while (obj) {
		switch (obj->lvl) {
		case STATS_LVL_BASIC:
			print_basic_stats(obj);
			break;
#if WLAN_ADVANCE_TELEMETRY
		case STATS_LVL_ADVANCE:
			print_advance_stats(obj);
			break;
#endif /* WLAN_ADVANCE_TELEMETRY */
#if WLAN_DEBUG_TELEMETRY
		case STATS_LVL_DEBUG:
			break;
#endif /* WLAN_DEBUG_TELEMETRY */
		default:
			STATS_ERR("Invalid Level!\n");
		}
		obj = obj->next;
	}
}

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

static void free_interface_list(struct interface_list *if_list)
{
	u_int8_t inx = 0;

	for (inx = 0; inx < MAX_RADIO_NUM; inx++) {
		if (if_list->r_names[inx])
			free(if_list->r_names[inx]);
		if_list->r_names[inx] = NULL;
	}
	if_list->r_count = 0;
	for (inx = 0; inx < MAX_VAP_NUM; inx++) {
		if (if_list->v_names[inx])
			free(if_list->v_names[inx]);
		if_list->v_names[inx] = NULL;
	}
	if_list->v_count = 0;
}

static int fetch_all_interfaces(struct interface_list *if_list)
{
	char temp_name[IFNAME_LEN] = {'\0'};
	DIR *dir = NULL;
	u_int8_t rinx = 0;
	u_int8_t vinx = 0;

	dir = opendir(PATH_SYSNET_DEV);
	if (!dir) {
		perror(PATH_SYSNET_DEV);
		return -ENOENT;
	}
	while (1) {
		struct dirent *entry;
		char *d_name;

		entry = readdir(dir);
		if (!entry)
			break;
		d_name = entry->d_name;
		if (entry->d_type & (DT_DIR | DT_LNK)) {
			if (strlcpy(temp_name, d_name, IFNAME_LEN) >=
				    IFNAME_LEN) {
				STATS_ERR("Unable to fetch interface name\n");
				closedir(dir);
				return -EIO;
			}
		} else {
			continue;
		}
		if (is_radio_ifname_valid(temp_name)) {
			if (rinx >= MAX_RADIO_NUM) {
				STATS_WARN("Radio Interfaces exceeded limit\n");
				continue;
			}
			if_list->r_names[rinx] = (char *)malloc(IFNAME_LEN);
			if (!if_list->r_names[rinx]) {
				STATS_ERR("Unable to Allocate Memory!\n");
				closedir(dir);
				return -ENOMEM;
			}
			strlcpy(if_list->r_names[rinx], temp_name, IFNAME_LEN);
			rinx++;
		} else if (is_vap_ifname_valid(temp_name)) {
			if (vinx >= MAX_VAP_NUM) {
				STATS_WARN("Vap Interfaces exceeded limit\n");
				continue;
			}
			if_list->v_names[vinx] = (char *)malloc(IFNAME_LEN);
			if (!if_list->v_names[vinx]) {
				STATS_ERR("Unable to Allocate Memory!\n");
				closedir(dir);
				return -ENOMEM;
			}
			strlcpy(if_list->v_names[vinx], temp_name, IFNAME_LEN);
			vinx++;
		}
	}

	closedir(dir);
	if_list->r_count = rinx;
	if_list->v_count = vinx;

	return 0;
}

int main(int argc, char *argv[])
{
	struct stats_command cmd;
	struct reply_buffer *reply = NULL;
	enum stats_level_e level_temp = STATS_LVL_BASIC;
	enum stats_object_e obj_temp = STATS_OBJ_AP;
	enum stats_type_e type_temp = STATS_TYPE_DATA;
	u_int64_t feat_temp = STATS_FEAT_FLG_ALL;
	int ret = 0;
	int long_index = 0;
	u_int8_t is_obj_set = 0;
	u_int8_t is_type_set = 0;
	u_int8_t is_feat_set = 0;
	u_int8_t is_level_set = 0;
	u_int8_t is_ifname_set = 0;
	u_int8_t is_stamacaddr_set = 0;
	u_int8_t is_option_selected = 0;
	u_int8_t inx = 0;
	bool recursion_temp = false;
	char feat_flags[128] = {0};
	char ifname_temp[IFNAME_LEN] = {0};
	char stamacaddr_temp[USER_MAC_ADDR_LEN] = {0};
	struct ether_addr *ret_eth_addr = NULL;
	struct interface_list if_list;

	memset(&cmd, 0, sizeof(struct stats_command));
	memset(&if_list, 0, sizeof(struct interface_list));

	ret = getopt_long(argc, argv, opt_string, long_opts, &long_index);
	while (ret != -1) {
		switch (ret) {
		case 'B':
			if (is_level_set) {
				STATS_ERR("Multiple Stats Level Arguments\n");
				display_help();
				return -EINVAL;
			}
			level_temp = STATS_LVL_BASIC;
			is_level_set = 1;
			is_option_selected = 1;
			break;
		case 'A':
			if (is_level_set) {
				STATS_ERR("Multiple Stats Level Arguments\n");
				display_help();
				return -EINVAL;
			}
			level_temp = STATS_LVL_ADVANCE;
			is_level_set = 1;
			is_option_selected = 1;
			break;
		case 'D':
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
		case 'a':
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
		case 'd':
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
			recursion_temp = true;
			break;
		default:
			STATS_ERR("Unrecognized option\n");
			display_help();
			is_option_selected = 1;
			return -EINVAL;
		}
		ret = getopt_long(argc, argv, opt_string,
				  long_opts, &long_index);
	}

	if (!is_option_selected)
		STATS_WARN("No valid option selected\n"
			   "Will display only default stats\n");

	if (feat_flags[0])
		feat_temp = libstats_get_feature_flag(feat_flags);
	if (!feat_temp)
		return -EINVAL;

	cmd.lvl = level_temp;
	cmd.obj = obj_temp;
	cmd.type = type_temp;
	cmd.feat_flag = feat_temp;
	cmd.recursive = recursion_temp;

	if (ifname_temp[0])
		strlcpy(cmd.if_name, ifname_temp, IFNAME_LEN);
	ret = fetch_all_interfaces(&if_list);
	if (ret < 0) {
		STATS_ERR("Unbale to fetch Interfaces!\n");
		free_interface_list(&if_list);
		return ret;
	}

	if (stamacaddr_temp[0] != '\0') {
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
	cmd.reply = reply;

	if ((cmd.obj == STATS_OBJ_AP) && !cmd.if_name[0]) {
		for (inx = 0; inx < if_list.r_count; inx++) {
			strlcpy(cmd.if_name, if_list.r_names[inx], IFNAME_LEN);
			ret = libstats_request_handle(&cmd);
			if (ret < 0)
				break;
		}
	} else {
		ret = libstats_request_handle(&cmd);
	}

	/* Print Output */
	if (!ret)
		print_response(cmd.reply);

	/* Cleanup */
	free_interface_list(&if_list);
	libstats_free_reply_buffer(&cmd);

	return 0;
}
