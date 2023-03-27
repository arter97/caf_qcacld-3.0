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

uint16_t qca_sawf_get_msduq(struct net_device *netdev,
			    uint8_t *peer_mac, uint32_t service_id);
uint16_t qca_sawf_get_msduq_v2(struct net_device *netdev, uint8_t *peer_mac,
			       uint32_t service_id, uint32_t dscp,
			       uint32_t rule_id, uint8_t sawf_rule_type);
uint16_t qca_sawf_get_msdu_queue(struct net_device *netdev,
				 uint8_t *peer_mac, uint32_t service_id,
				 uint32_t dscp, uint32_t rule_id);

/*
 * qca_sawf_config_ul() - Set Uplink QoS parameters
 *
 * @dst_mac: Destination MAC address
 * @src_mac: Source MAC address
 * @fw_service_id: Service class ID in forward direction
 * @rv_service_id: Service class ID in reverse direction
 * @add_or_sub: Add or Sub param
 *
 * Return: void
 */
void qca_sawf_config_ul(uint8_t *dst_mac, uint8_t *src_mac,
			uint8_t fw_service_id, uint8_t rv_service_id,
			uint8_t add_or_sub);

/*
 * qca_sawf_config_ul_v2() - Set Uplink QoS parameters
 *
 * @dst_dev: Destination netdev
 * @dst_mac: Destination MAC address
 * @src_dev: Source netdev
 * @src_mac: Source MAC address
 * @fw_service_id: Service class ID in forward direction
 * @rv_service_id: Service class ID in reverse direction
 * @add_or_sub: Add or Sub param
 *
 * Return: void
 */
void qca_sawf_config_ul_v2(struct net_device *dst_dev, uint8_t *dst_mac,
			   struct net_device *src_dev, uint8_t *src_mac,
			   uint8_t fw_service_id, uint8_t rv_service_id,
			   uint8_t add_or_sub);
#endif
