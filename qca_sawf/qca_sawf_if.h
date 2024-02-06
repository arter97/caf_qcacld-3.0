/*
 * Copyright (c) 2022, Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _DP_SAWF_H__
#define _DP_SAWF_H_

#define QCA_SAWF_SVID_VALID 0x1
#define QCA_SAWF_DSCP_VALID 0x2
#define QCA_SAWF_PCP_VALID  0x4
#define QCA_SAWF_SVC_ID_INVALID 0xFF
#define FLOW_START 1
#define FLOW_STOP  2
#define QCA_SAWF_MCAST_IF_MAX 16

/* qca_sawf_metadata_param
 *
 * @netdev : Netdevice
 * @peer_mac : Destination peer mac address
 * @service_id : Service class id
 * @dscp : Differentiated Services Code Point
 * @rule_id : Rule id
 * @sawf_rule_type: Rule type
 * @pcp: pcp value
 * @valid_flag: flag to indicate if pcp is valid or not
 * @mcast_flag: flag to indicate if query is for multicast
 */
struct qca_sawf_metadata_param {
	struct net_device *netdev;
	uint8_t *peer_mac;
	uint32_t service_id;
	uint32_t dscp;
	uint32_t rule_id;
	uint8_t sawf_rule_type;
	uint32_t pcp;
	uint32_t valid_flag;
	uint8_t mcast_flag:1;
};

/*
 * qca_sawf_connection_sync_param
 *
 * @dst_dev: Destination netdev
 * @src_dev: Source netdev
 * @dst_mac: Destination MAC address
 * @src_mac: Source MAC address
 * @fw_service_id: Service ID matching in forward direction
 * @rv_service_id: Service ID matching in reverse direction
 * @start_or_stop: Connection stat or stop indication 1:start, 2:stop
 * @fw_mark_metadata: forward metadata
 * @rv_mark_metadata: reverse metadata
 */
struct qca_sawf_connection_sync_param {
	struct net_device *dst_dev;
	struct net_device *src_dev;
	uint8_t *dst_mac;
	uint8_t *src_mac;
	uint8_t fw_service_id;
	uint8_t rv_service_id;
	uint8_t start_or_stop;
	uint32_t fw_mark_metadata;
	uint32_t rv_mark_metadata;
};

/*
 * qca_sawf_ip_addr
 *
 * @v4_addr: ipv4 IP address
 * @v6_addr: ipv6 IP addr
 */
union qca_sawf_ip_addr {
		u_int32_t v4_addr;
		struct in6_addr v6_addr;
};

/*
 * qca_sawf_mcast_connection_sync_param
 *
 * @src: src IP addr
 * @dest: src IP addr
 * @src_dev_name: source netdev name
 * @dest_dev_name: destination netdev names
 * @dest_dev_count: destination netdev count
 * @add_or_sub: 1:start, 2:stop
 * @ip_version: ipv6/ipv4
 */
typedef struct qca_sawf_mcast_connection_sync_param {
	union qca_sawf_ip_addr src;
	union qca_sawf_ip_addr dest;
	int32_t src_ifindex;
	int32_t dest_ifindex[QCA_SAWF_MCAST_IF_MAX];
	uint32_t dest_dev_count;
	uint8_t add_or_sub;
	uint16_t ip_version;
} qca_sawf_mcast_sync_param_t;

uint16_t qca_sawf_get_msduq(struct net_device *netdev,
			    uint8_t *peer_mac, uint32_t service_id);
uint16_t qca_sawf_get_msduq_v2(struct net_device *netdev, uint8_t *peer_mac,
			       uint32_t service_id, uint32_t dscp_pcp,
			       uint32_t rule_id, uint8_t sawf_rule_type,
			       bool pcp);
uint32_t qca_sawf_get_mark_metadata(
		struct qca_sawf_metadata_param *params);
uint16_t qca_sawf_get_msdu_queue(struct net_device *netdev,
				 uint8_t *peer_mac, uint32_t service_id,
				 uint32_t dscp, uint32_t rule_id);

/*
 * qca_sawf_config_ul() - Set Uplink QoS parameters
 *
 * @dst_dev: Destination netdev
 * @src_dev: Source netdev
 * @dst_mac: Destination MAC address
 * @src_mac: Source MAC address
 * @fw_service_id: Service class ID in forward direction
 * @rv_service_id: Service class ID in reverse direction
 * @add_or_sub: Add or Sub param
 * @fw_mark_metadata: Forward direction metadata
 * @rv_mark_metadata: Reverse direction metadata
 *
 * Return: void
 */
void qca_sawf_config_ul(struct net_device *dst_dev, struct net_device *src_dev,
			uint8_t *dst_mac, uint8_t *src_mac,
			uint8_t fw_service_id, uint8_t rv_service_id,
			uint8_t add_or_sub, uint32_t fw_mark_metadata,
			uint32_t rv_mark_metadata);

/*
 * qca_sawf_connection_sync() - Update connection status
 *
 * @param: connection sync parameters
 *
 * Return: void
 */
void qca_sawf_connection_sync(struct qca_sawf_connection_sync_param *param);

/*
 * qca_sawf_mcast_connection_sync() - Update connection status
 *
 * @param: connection sync parameters
 *
 * Return: void
 */
void qca_sawf_mcast_connection_sync(qca_sawf_mcast_sync_param_t *params);
#endif
