/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * DOC: contains ll_sap definitions specific to the ll_sap module
 */

#ifndef __OS_IF_LL_SAP_H__
#define __OS_IF_LL_SAP_H__

#include "qdf_types.h"
#include "qca_vendor.h"

#ifdef WLAN_FEATURE_LL_LT_SAP

/**
 * osif_ll_sap_register_cb() - Register ll_sap osif callbacks
 *
 * Return: QDF_STATUS
 */
QDF_STATUS osif_ll_sap_register_cb(void);

/**
 * osif_ll_sap_unregister_cb() - un-register ll_sap osif callbacks
 *
 * Return: QDF_STATUS
 */
void osif_ll_sap_unregister_cb(void);

QDF_STATUS osif_ll_lt_sap_request_for_audio_transport_switch(
			enum qca_wlan_audio_transport_switch_type req_type);

#else
static inline QDF_STATUS osif_ll_sap_register_cb(void)
{
	return QDF_STATUS_SUCCESS;
}

static inline void osif_ll_sap_unregister_cb(void) {}

static inline QDF_STATUS
osif_ll_lt_sap_request_for_audio_transport_switch(
			enum qca_wlan_audio_transport_switch_type req_type)
{
	return QDF_STATUS_E_INVAL;
}
#endif /* WLAN_FEATURE_LL_LT_SAP */
#endif /* __OS_IF_LL_SAP_H__*/
