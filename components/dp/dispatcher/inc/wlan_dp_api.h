/*
 * Copyright (c) 2023, Qualcomm Innovation Center, Inc. All rights reserved.
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
/**
 * DOC: wlan_dp_api.h
 *
 */

#if !defined(_WLAN_DP_API_H_)
#define _WLAN_DP_API_H_

/**
 * wlan_dp_update_peer_map_unmap_version() - update peer map unmap version
 * @version: Peer map unmap version pointer to be updated
 *
 * Return: None
 */
void wlan_dp_update_peer_map_unmap_version(uint8_t *version);

/**
 * wlan_dp_runtime_suspend() - Runtime suspend DP handler
 * @soc: CDP SoC handle
 * @pdev_id: DP PDEV ID
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_dp_runtime_suspend(ol_txrx_soc_handle soc, uint8_t pdev_id);

/**
 * wlan_dp_runtime_resume() - Runtime suspend DP handler
 * @soc: CDP SoC handle
 * @pdev_id: DP PDEV ID
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_dp_runtime_resume(ol_txrx_soc_handle soc, uint8_t pdev_id);
#endif
