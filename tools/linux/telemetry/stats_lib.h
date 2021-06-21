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

#ifndef _STATS_LIB_H_
#define _STATS_LIB_H_

#define MAX_DATA_LENGTH           (1024 * 3)

#define MAX_DB_COUNT              128
/* Network Interface name length */
#define IF_NAME_LEN               IFNAMSIZ
/* Flag for recursive stats */
#define RECURSIVE                 1

enum stats_feat_e {
	STATS_FEAT_ALL = 0,
	STATS_OBJECT_ID = 1,
	STATS_FEAT_RX = 2,
	STATS_FEAT_TX = 3,
	STATS_FEAT_AST = 4,
	STATS_FEAT_CFR = 5,
	STATS_FEAT_FWD = 6,
	STATS_FEAT_HTT = 7,
	STATS_FEAT_RAW = 8,
	STATS_FEAT_RDK = 9,
	STATS_FEAT_TSO = 10,
	STATS_FEAT_TWT = 11,
	STATS_FEAT_VOW = 12,
	STATS_FEAT_WDI = 13,
	STATS_FEAT_WMI = 14,
	STATS_FEAT_IGMP = 15,
	STATS_FEAT_LINK = 16,
	STATS_FEAT_MESH = 17,
	STATS_FEAT_RATE = 18,
	STATS_FEAT_DELAY = 19,
	STATS_FEAT_ME = 20,
	STATS_FEAT_NAWDS = 21,
	STATS_FEAT_TXCAP = 22,
	STATS_FEAT_MONITOR = 23,
	STATS_RECURSIVE = 24,
	STATS_FEAT_MAX,
};

struct data_block {
	enum stats_object_e obj;
	enum stats_feat_e type;
	u_int32_t index;
};

struct reply_buffer {
	u_int32_t length;
	u_int32_t db_count;
	struct data_block db[MAX_DB_COUNT];
	u_int8_t *data;
};

/**
 * struct stats_command: Defines interface level command structure
 * @lvl:       Stats level
 * @obj:       Stats object
 * @type:      Stats traffic type
 * @recursive: Stats recursiveness
 * @feat_flag: Stats requested for combination of Features
 * @sta_mac:   Station MAC address if Stats requested for STA object
 * @if_name:   Interface name on which Stats is requested
 */
struct stats_command {
	enum stats_level_e lvl;
	enum stats_object_e obj;
	enum stats_type_e type;
	u_int8_t recursive;
	u_int32_t feat_flag;
	struct ether_addr sta_mac;
	char if_name[IF_NAME_LEN];
	struct reply_buffer *reply;
};

/**
 * libstats_get_feature_flag(): Function to parse Feature flags and return value
 * @feat_flags: String holding feature flag names separted by dilimeter '|'
 *
 * Return: Combination of requested feature flag value or 0
 */
u_int32_t libstats_get_feature_flag(char *feat_flags);

/**
 * libstats_request_handle(): Function to send stats request to driver
 * @cmd:      Filled command structure by Application
 *
 * Return: 0 on Success, -1 on Failure
 */
int32_t libstats_request_handle(struct stats_command *cmd);

#endif /* _STATS_LIB_H_ */
