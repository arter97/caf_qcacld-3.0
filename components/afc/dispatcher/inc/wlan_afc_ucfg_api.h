/*
 * Copyright (c) 2022, Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.

 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * DOC: wlan_afc_ucfg_api.h
 */

#ifndef __WLAN_AFC_UCFG_API_H__
#define __WLAN_AFC_UCFG_API_H__

#ifdef CONFIG_AFC_SUPPORT
/**
 * ucfg_afc_init() - API to initialize AFC component
 *
 * Return: QDF STATUS
 */
QDF_STATUS ucfg_afc_init(void);

/**
 * ucfg_afc_deinit() - API to de-initialize AFC component
 *
 * Return: QDF STATUS
 */
QDF_STATUS ucfg_afc_deinit(void);
#else
static inline QDF_STATUS ucfg_afc_init(void) { return QDF_STATUS_SUCCESS; }
static inline QDF_STATUS ucfg_afc_deinit(void) { return QDF_STATUS_SUCCESS; }
#endif
#endif
