/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * DOC: wlan_sawf.h
 * This file defines data structure &  prototypes of the functions
 * needed by the SAWF framework.
 */

#ifndef _WLAN_SAWF_H_
#define _WLAN_SAWF_H_

#include <qdf_status.h>

#define SAWF_SVC_CLASS_MIN 1
#define SAWF_SVC_CLASS_MAX 128
#define WLAN_MAX_SVC_CLASS_NAME 64

#define SAWF_LINE_FORMAT "================================================"
/**
 * struct wlan_sawf_scv_class_params- Service Class Parameters
 * @svc_id: Service ID
 * @app_name: Service class name
 * @min_thruput_rate: min throughput in kilobits per second
 * @max_thruput_rate: max throughput in kilobits per second
 * @burst_size:  burst size in bytes
 * @service_interval: service interval
 * @delay_bound: delay bound in in milli seconds
 * @msdu_ttl: MSDU Time-To-Live
 * @priority: Priority
 * @tid: TID
 * @msdu_rate_loss: MSDU loss rate in parts per million
 * @configured: indicating if the serivice class is configured.
 */

struct wlan_sawf_scv_class_params {
	uint8_t svc_id;
	char app_name[WLAN_MAX_SVC_CLASS_NAME];
	uint32_t min_thruput_rate;
	uint32_t max_thruput_rate;
	uint32_t burst_size;
	uint32_t service_interval;
	uint32_t delay_bound;
	uint32_t msdu_ttl;
	uint32_t priority;
	uint32_t tid;
	uint32_t msdu_rate_loss;
	bool configured;
};

/**
 * struct sawf_ctx- SAWF context
 * @svc_classes: List of all service classes
 */
struct sawf_ctx {
	struct wlan_sawf_scv_class_params svc_classes[SAWF_SVC_CLASS_MAX];
};

/* wlan_sawf_init() - Initialize SAWF subsytem
 *
 * Initialize the SAWF context
 *
 * Return: QDF_STATUS
 */

QDF_STATUS wlan_sawf_init(void);

/* wlan_sawf_deinit() - Deinitialize SAWF subsystem
 *
 * Deinnitialize the SAWF context
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_sawf_deinit(void);

/* wlan_get_sawf_ctx() - Get service aware wifi context
 *
 * Get Service Aware Wifi Context
 *
 * Return: SAWF context
 */
struct sawf_ctx *wlan_get_sawf_ctx(void);

/* wlan_service_id_valid() - Validate the service ID
 *
 * Validate the service ID
 *
 * Return: true or false
 */
bool wlan_service_id_valid(uint8_t svc_id);

/* wlan_service_id_configured() - Is service ID configured
 *
 * Is the service ID configured
 *
 * Return: true or false
 */
bool wlan_service_id_configured(uint8_t svc_id);

/* wlan_print_service_class() - Print service class params
 *
 * Print service class params
 *
 * Return: none
 */
void wlan_print_service_class(struct wlan_sawf_scv_class_params *params);
#endif

