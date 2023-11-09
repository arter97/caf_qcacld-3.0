/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef _WLAN_DP_METADATA_H_
#define _WLAN_DP_METADATA_H_

/* 32-bit Metadata definition stored in skb->mark field.
 *
 * |31   28|27   24|23    16|15      0|
 * +-------+-------+--------+---------+
 * | Flags |  TAG  | svc_id | Reserve |
 * +-------+-------+--------+---------+
 *
 * bit 31-28: Flags
 * bit 27-24: TAG
 * bit 23-16: service_class_id
 * bit 15-0 : reserved
 */

#define INVALID_TAG		(0xF)
#define LAPB_VALID_TAG		(0xB)

#define METADATA_FLAGS_LAPB_FLUSH_IND_SHIFT (28)
#define METADATA_FLAGS_LAPB_FLUSH_IND_MASK \
		(1 << METADATA_FLAGS_LAPB_FLUSH_IND_SHIFT)

#define METADATA_TAG_SHIFT (24)
#define METADATA_TAG_SIZE  (0x0F)
#define METADATA_TAG_MASK  (METADATA_TAG_SIZE << METADATA_TAG_SHIFT)

#define METADATA_SERVICE_CLASS_ID_SHIFT	(16)
#define METADATA_SERVICE_CLASS_ID_SIZE	(0xFF)
#define METADATA_SERVICE_CLASS_ID_MASK \
	(METADATA_SERVICE_CLASS_ID_SIZE << METADATA_SERVICE_CLASS_ID_SHIFT)

#define IS_LAPB_FRAME(x) \
	((((x) & METADATA_TAG_MASK) >> METADATA_TAG_SHIFT) == LAPB_VALID_TAG)

#define GET_SERVICE_CLASS_ID(x) \
	(((x) & METADATA_SERVICE_CLASS_ID_MASK) >> \
	 METADATA_SERVICE_CLASS_ID_SHIFT)

#define GET_FLAGS_LAPB_FLUSH_IND(x) \
	(((x) & METADATA_FLAGS_LAPB_FLUSH_IND_MASK) >> \
	 METADATA_FLAGS_LAPB_FLUSH_IND_SHIFT)

#define SET_FLAGS_LAPB_FLUSH_IND(x) \
	((x) |= (1 << METADATA_FLAGS_LAPB_FLUSH_IND_SHIFT))

#define PREPARE_METADATA(tag, svc_id) \
	(((tag) << METADATA_TAG_SHIFT) | \
	(((svc_id) & METADATA_SERVICE_CLASS_ID_SIZE) << \
		METADATA_SERVICE_CLASS_ID_SHIFT))

#endif /* end of _WLAN_DP_METADATA_H_ */
