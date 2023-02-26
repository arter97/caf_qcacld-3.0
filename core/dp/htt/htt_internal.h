/*
 * Copyright (c) 2011, 2014-2020 The Linux Foundation. All rights reserved.
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef _HTT_INTERNAL__H_
#define _HTT_INTERNAL__H_

#include <athdefs.h>            /* A_STATUS */
#include <qdf_nbuf.h>           /* qdf_nbuf_t */
#include <qdf_util.h>           /* qdf_assert */
#include <htc_api.h>            /* HTC_PACKET */

#include <htt_types.h>

/* htt_rx.c */
#define RX_MSDU_END_4_FIRST_MSDU_MASK \
	(pdev->targetdef->d_RX_MSDU_END_4_FIRST_MSDU_MASK)
#define RX_MSDU_END_4_FIRST_MSDU_LSB \
	(pdev->targetdef->d_RX_MSDU_END_4_FIRST_MSDU_LSB)
#define RX_MPDU_START_0_RETRY_LSB  \
	(pdev->targetdef->d_RX_MPDU_START_0_RETRY_LSB)
#define RX_MPDU_START_0_RETRY_MASK  \
	(pdev->targetdef->d_RX_MPDU_START_0_RETRY_MASK)
#define RX_MPDU_START_0_SEQ_NUM_MASK \
	(pdev->targetdef->d_RX_MPDU_START_0_SEQ_NUM_MASK)
#define RX_MPDU_START_0_SEQ_NUM_LSB \
	(pdev->targetdef->d_RX_MPDU_START_0_SEQ_NUM_LSB)
#define RX_MPDU_START_2_PN_47_32_LSB \
	(pdev->targetdef->d_RX_MPDU_START_2_PN_47_32_LSB)
#define RX_MPDU_START_2_PN_47_32_MASK \
	(pdev->targetdef->d_RX_MPDU_START_2_PN_47_32_MASK)
#define RX_MPDU_START_2_TID_LSB  \
	(pdev->targetdef->d_RX_MPDU_START_2_TID_LSB)
#define RX_MPDU_START_2_TID_MASK  \
	(pdev->targetdef->d_RX_MPDU_START_2_TID_MASK)
#define RX_MSDU_END_1_KEY_ID_OCT_MASK \
	(pdev->targetdef->d_RX_MSDU_END_1_KEY_ID_OCT_MASK)
#define RX_MSDU_END_1_KEY_ID_OCT_LSB \
	(pdev->targetdef->d_RX_MSDU_END_1_KEY_ID_OCT_LSB)
#define RX_MSDU_END_1_EXT_WAPI_PN_63_48_MASK \
	(pdev->targetdef->d_RX_MSDU_END_1_EXT_WAPI_PN_63_48_MASK)
#define RX_MSDU_END_1_EXT_WAPI_PN_63_48_LSB \
	(pdev->targetdef->d_RX_MSDU_END_1_EXT_WAPI_PN_63_48_LSB)
#define RX_MSDU_END_4_LAST_MSDU_MASK \
	(pdev->targetdef->d_RX_MSDU_END_4_LAST_MSDU_MASK)
#define RX_MSDU_END_4_LAST_MSDU_LSB \
	(pdev->targetdef->d_RX_MSDU_END_4_LAST_MSDU_LSB)
#define RX_ATTENTION_0_MCAST_BCAST_MASK \
	(pdev->targetdef->d_RX_ATTENTION_0_MCAST_BCAST_MASK)
#define RX_ATTENTION_0_MCAST_BCAST_LSB \
	(pdev->targetdef->d_RX_ATTENTION_0_MCAST_BCAST_LSB)
#define RX_ATTENTION_0_FRAGMENT_MASK \
	(pdev->targetdef->d_RX_ATTENTION_0_FRAGMENT_MASK)
#define RX_ATTENTION_0_FRAGMENT_LSB \
	(pdev->targetdef->d_RX_ATTENTION_0_FRAGMENT_LSB)
#define RX_ATTENTION_0_MPDU_LENGTH_ERR_MASK \
	(pdev->targetdef->d_RX_ATTENTION_0_MPDU_LENGTH_ERR_MASK)
#define RX_FRAG_INFO_0_RING2_MORE_COUNT_MASK \
	(pdev->targetdef->d_RX_FRAG_INFO_0_RING2_MORE_COUNT_MASK)
#define RX_FRAG_INFO_0_RING2_MORE_COUNT_LSB \
	(pdev->targetdef->d_RX_FRAG_INFO_0_RING2_MORE_COUNT_LSB)
#define RX_MSDU_START_0_MSDU_LENGTH_MASK \
	(pdev->targetdef->d_RX_MSDU_START_0_MSDU_LENGTH_MASK)
#define RX_MSDU_START_0_MSDU_LENGTH_LSB \
	(pdev->targetdef->d_RX_MSDU_START_0_MSDU_LENGTH_LSB)
#define RX_MPDU_START_0_ENCRYPTED_MASK \
	(pdev->targetdef->d_RX_MPDU_START_0_ENCRYPTED_MASK)
#define RX_MPDU_START_0_ENCRYPTED_LSB \
	(pdev->targetdef->d_RX_MPDU_START_0_ENCRYPTED_LSB)
#define RX_ATTENTION_0_MORE_DATA_MASK \
	(pdev->targetdef->d_RX_ATTENTION_0_MORE_DATA_MASK)
#define RX_ATTENTION_0_MSDU_DONE_MASK \
	(pdev->targetdef->d_RX_ATTENTION_0_MSDU_DONE_MASK)
#define RX_ATTENTION_0_TCP_UDP_CHKSUM_FAIL_MASK \
	(pdev->targetdef->d_RX_ATTENTION_0_TCP_UDP_CHKSUM_FAIL_MASK)
#define RX_MSDU_START_2_DECAP_FORMAT_OFFSET \
	(pdev->targetdef->d_RX_MSDU_START_2_DECAP_FORMAT_OFFSET)
#define RX_MSDU_START_2_DECAP_FORMAT_LSB \
	(pdev->targetdef->d_RX_MSDU_START_2_DECAP_FORMAT_LSB)
#define RX_MSDU_START_2_DECAP_FORMAT_MASK \
	(pdev->targetdef->d_RX_MSDU_START_2_DECAP_FORMAT_MASK)
/* end */

#ifndef offsetof
#define offsetof(type, field)   ((size_t)(&((type *)0)->field))
#endif

#undef MS
#define MS(_v, _f) (((_v) & _f ## _MASK) >> _f ## _LSB)
#undef SM
#define SM(_v, _f) (((_v) << _f ## _LSB) & _f ## _MASK)
#undef WO
#define WO(_f)      ((_f ## _OFFSET) >> 2)

#define GET_FIELD(_addr, _f) MS(*((A_UINT32 *)(_addr) + WO(_f)), _f)

#include <rx_desc.h>
#include <wal_rx_desc.h>        /* struct rx_attention, etc */

struct htt_host_fw_desc_base {
	union {
		struct fw_rx_desc_base val;
		A_UINT32 dummy_pad;     /* make sure it is DOWRD aligned */
	} u;
};


/*
 * This struct defines the basic descriptor information used by host,
 * which is written either by the 11ac HW MAC into the host Rx data
 * buffer ring directly or generated by FW and copied from Rx indication
 */
struct htt_host_rx_desc_base {
	struct htt_host_fw_desc_base fw_desc;
	struct rx_attention attention;
	struct rx_frag_info frag_info;
	struct rx_mpdu_start mpdu_start;
	struct rx_msdu_start msdu_start;
	struct rx_msdu_end msdu_end;
	struct rx_mpdu_end mpdu_end;
	struct rx_ppdu_start ppdu_start;
	struct rx_ppdu_end ppdu_end;
#ifdef QCA_WIFI_3_0_ADRASTEA
/* Increased to support some of offload features */
#define RX_HTT_HDR_STATUS_LEN 256
#else
#define RX_HTT_HDR_STATUS_LEN 64
#endif
	char rx_hdr_status[RX_HTT_HDR_STATUS_LEN];
};

#define RX_DESC_ATTN_MPDU_LEN_ERR_BIT   0x08000000

#define RX_STD_DESC_ATTN_OFFSET	\
	(offsetof(struct htt_host_rx_desc_base, attention))
#define RX_STD_DESC_FRAG_INFO_OFFSET \
	(offsetof(struct htt_host_rx_desc_base, frag_info))
#define RX_STD_DESC_MPDU_START_OFFSET \
	(offsetof(struct htt_host_rx_desc_base, mpdu_start))
#define RX_STD_DESC_MSDU_START_OFFSET \
	(offsetof(struct htt_host_rx_desc_base, msdu_start))
#define RX_STD_DESC_MSDU_END_OFFSET \
	(offsetof(struct htt_host_rx_desc_base, msdu_end))
#define RX_STD_DESC_MPDU_END_OFFSET \
	(offsetof(struct htt_host_rx_desc_base, mpdu_end))
#define RX_STD_DESC_PPDU_START_OFFSET \
	(offsetof(struct htt_host_rx_desc_base, ppdu_start))
#define RX_STD_DESC_PPDU_END_OFFSET \
	(offsetof(struct htt_host_rx_desc_base, ppdu_end))
#define RX_STD_DESC_HDR_STATUS_OFFSET \
	(offsetof(struct htt_host_rx_desc_base, rx_hdr_status))

#define RX_STD_DESC_FW_MSDU_OFFSET \
	(offsetof(struct htt_host_rx_desc_base, fw_desc))

#define RX_STD_DESC_SIZE (sizeof(struct htt_host_rx_desc_base))

#define RX_DESC_ATTN_OFFSET32       (RX_STD_DESC_ATTN_OFFSET >> 2)
#define RX_DESC_FRAG_INFO_OFFSET32  (RX_STD_DESC_FRAG_INFO_OFFSET >> 2)
#define RX_DESC_MPDU_START_OFFSET32 (RX_STD_DESC_MPDU_START_OFFSET >> 2)
#define RX_DESC_MSDU_START_OFFSET32 (RX_STD_DESC_MSDU_START_OFFSET >> 2)
#define RX_DESC_MSDU_END_OFFSET32   (RX_STD_DESC_MSDU_END_OFFSET >> 2)
#define RX_DESC_MPDU_END_OFFSET32   (RX_STD_DESC_MPDU_END_OFFSET >> 2)
#define RX_DESC_PPDU_START_OFFSET32 (RX_STD_DESC_PPDU_START_OFFSET >> 2)
#define RX_DESC_PPDU_END_OFFSET32   (RX_STD_DESC_PPDU_END_OFFSET >> 2)
#define RX_DESC_HDR_STATUS_OFFSET32 (RX_STD_DESC_HDR_STATUS_OFFSET >> 2)

#define RX_STD_DESC_SIZE_DWORD      (RX_STD_DESC_SIZE >> 2)

/*
 * Make sure there is a minimum headroom provided in the rx netbufs
 * for use by the OS shim and OS and rx data consumers.
 */
#define HTT_RX_BUF_OS_MIN_HEADROOM 32
#define HTT_RX_STD_DESC_RESERVATION  \
	((HTT_RX_BUF_OS_MIN_HEADROOM > RX_STD_DESC_SIZE) ? \
	 HTT_RX_BUF_OS_MIN_HEADROOM : RX_STD_DESC_SIZE)
#define HTT_RX_DESC_RESERVATION32 \
	(HTT_RX_STD_DESC_RESERVATION >> 2)

#define HTT_RX_DESC_ALIGN_MASK 7        /* 8-byte alignment */

#ifdef DEBUG_RX_RING_BUFFER
#ifdef MSM_PLATFORM
#define HTT_ADDRESS_MASK   0xfffffffffffffffe
#else
#define HTT_ADDRESS_MASK   0xfffffffe
#endif /* MSM_PLATFORM */

/**
 * rx_buf_debug: rx_ring history
 *
 * There are three types of entries in history:
 * 1) rx-descriptors posted (and received)
 *    Both of these events are stored on the same entry
 *    @paddr : physical address posted on the ring
 *    @nbuf  : virtual address of nbuf containing data
 *    @ndata : virual address of data (corresponds to physical address)
 *    @posted: time-stamp when the buffer is posted to the ring
 *    @recved: time-stamp when the buffer is received (rx_in_order_ind)
 *           : or 0, if the buffer has not been received yet
 * 2) ring alloc-index (fill-index) updates
 *    @paddr : = 0
 *    @nbuf  : = 0
 *    @ndata : = 0
 *    posted : time-stamp when alloc index was updated
 *    recved : value of alloc index
 * 3) htt_rx_in_order_indication reception
 *    @paddr : = 0
 *    @nbuf  : = 0
 *    @ndata : msdu_cnt
 *    @posted: time-stamp when HTT message is recived
 *    @recvd : 0x48545452584D5367 ('HTTRXMSG')
 */
#ifdef CONFIG_SLUB_DEBUG_ON
#define HTT_RX_RING_BUFF_DBG_LIST          (8 * 1024)
#else
#define HTT_RX_RING_BUFF_DBG_LIST          (4 * 1024)
#endif
struct rx_buf_debug {
	qdf_dma_addr_t paddr;
	qdf_nbuf_t     nbuf;
	void          *nbuf_data;
	uint64_t       posted; /* timetamp */
	uint64_t       recved; /* timestamp */
	int            cpu;

};
#endif

static inline struct htt_host_rx_desc_base *htt_rx_desc(qdf_nbuf_t msdu)
{
	return (struct htt_host_rx_desc_base *)
	       (((size_t) (qdf_nbuf_head(msdu) + HTT_RX_DESC_ALIGN_MASK)) &
		~HTT_RX_DESC_ALIGN_MASK);
}

#if defined(HELIUMPLUS)
/**
 * htt_print_rx_desc_lro() - print LRO information in the rx
 * descriptor
 * @rx_desc: HTT rx descriptor
 *
 * Prints the LRO related fields in the HTT rx descriptor
 *
 * Return: none
 */
static inline void htt_print_rx_desc_lro(struct htt_host_rx_desc_base *rx_desc)
{
	qdf_nofl_info
		("----------------------RX DESC LRO----------------------\n");
	qdf_nofl_info("msdu_end.lro_eligible:0x%x\n",
		      rx_desc->msdu_end.lro_eligible);
	qdf_nofl_info("msdu_start.tcp_only_ack:0x%x\n",
		      rx_desc->msdu_start.tcp_only_ack);
	qdf_nofl_info("msdu_end.tcp_udp_chksum:0x%x\n",
		      rx_desc->msdu_end.tcp_udp_chksum);
	qdf_nofl_info("msdu_end.tcp_seq_number:0x%x\n",
		      rx_desc->msdu_end.tcp_seq_number);
	qdf_nofl_info("msdu_end.tcp_ack_number:0x%x\n",
		      rx_desc->msdu_end.tcp_ack_number);
	qdf_nofl_info("msdu_start.tcp_proto:0x%x\n",
		      rx_desc->msdu_start.tcp_proto);
	qdf_nofl_info("msdu_start.ipv6_proto:0x%x\n",
		      rx_desc->msdu_start.ipv6_proto);
	qdf_nofl_info("msdu_start.ipv4_proto:0x%x\n",
		      rx_desc->msdu_start.ipv4_proto);
	qdf_nofl_info("msdu_start.l3_offset:0x%x\n",
		      rx_desc->msdu_start.l3_offset);
	qdf_nofl_info("msdu_start.l4_offset:0x%x\n",
		      rx_desc->msdu_start.l4_offset);
	qdf_nofl_info("msdu_start.flow_id_toeplitz:0x%x\n",
		      rx_desc->msdu_start.flow_id_toeplitz);
	qdf_nofl_info
		("---------------------------------------------------------\n");
}

/**
 * htt_print_rx_desc_lro() - extract LRO information from the rx
 * descriptor
 * @msdu: network buffer
 * @rx_desc: HTT rx descriptor
 *
 * Extracts the LRO related fields from the HTT rx descriptor
 * and stores them in the network buffer's control block
 *
 * Return: none
 */
static inline void htt_rx_extract_lro_info(qdf_nbuf_t msdu,
	 struct htt_host_rx_desc_base *rx_desc)
{
	if (rx_desc->attention.tcp_udp_chksum_fail)
		QDF_NBUF_CB_RX_LRO_ELIGIBLE(msdu) = 0;
	else
		QDF_NBUF_CB_RX_LRO_ELIGIBLE(msdu) =
			rx_desc->msdu_end.lro_eligible;

	if (QDF_NBUF_CB_RX_LRO_ELIGIBLE(msdu)) {
		QDF_NBUF_CB_RX_TCP_PURE_ACK(msdu) =
			rx_desc->msdu_start.tcp_only_ack;
		QDF_NBUF_CB_RX_TCP_CHKSUM(msdu) =
			rx_desc->msdu_end.tcp_udp_chksum;
		QDF_NBUF_CB_RX_TCP_SEQ_NUM(msdu) =
			rx_desc->msdu_end.tcp_seq_number;
		QDF_NBUF_CB_RX_TCP_ACK_NUM(msdu) =
			rx_desc->msdu_end.tcp_ack_number;
		QDF_NBUF_CB_RX_TCP_WIN(msdu) =
			rx_desc->msdu_end.window_size;
		QDF_NBUF_CB_RX_TCP_PROTO(msdu) =
			rx_desc->msdu_start.tcp_proto;
		QDF_NBUF_CB_RX_IPV6_PROTO(msdu) =
			rx_desc->msdu_start.ipv6_proto;
		QDF_NBUF_CB_RX_TCP_OFFSET(msdu) =
			rx_desc->msdu_start.l4_offset;
		QDF_NBUF_CB_RX_FLOW_ID(msdu) =
			rx_desc->msdu_start.flow_id_toeplitz;
	}
}
#else
static inline void htt_print_rx_desc_lro(struct htt_host_rx_desc_base *rx_desc)
{}
static inline void htt_rx_extract_lro_info(qdf_nbuf_t msdu,
	 struct htt_host_rx_desc_base *rx_desc) {}
#endif /* HELIUMPLUS */

static inline void htt_print_rx_desc(struct htt_host_rx_desc_base *rx_desc)
{
	qdf_nofl_info
		("----------------------RX DESC----------------------------\n");
	qdf_nofl_info("attention: %#010x\n",
		      (unsigned int)(*(uint32_t *)&rx_desc->attention));
	qdf_nofl_info("frag_info: %#010x\n",
		      (unsigned int)(*(uint32_t *)&rx_desc->frag_info));
	qdf_nofl_info("mpdu_start: %#010x %#010x %#010x\n",
		      (unsigned int)(((uint32_t *)&rx_desc->mpdu_start)[0]),
		      (unsigned int)(((uint32_t *)&rx_desc->mpdu_start)[1]),
		      (unsigned int)(((uint32_t *)&rx_desc->mpdu_start)[2]));
	qdf_nofl_info("msdu_start: %#010x %#010x %#010x\n",
		      (unsigned int)(((uint32_t *)&rx_desc->msdu_start)[0]),
		      (unsigned int)(((uint32_t *)&rx_desc->msdu_start)[1]),
		      (unsigned int)(((uint32_t *)&rx_desc->msdu_start)[2]));
	qdf_nofl_info("msdu_end: %#010x %#010x %#010x %#010x %#010x\n",
		      (unsigned int)(((uint32_t *)&rx_desc->msdu_end)[0]),
		      (unsigned int)(((uint32_t *)&rx_desc->msdu_end)[1]),
		      (unsigned int)(((uint32_t *)&rx_desc->msdu_end)[2]),
		      (unsigned int)(((uint32_t *)&rx_desc->msdu_end)[3]),
		      (unsigned int)(((uint32_t *)&rx_desc->msdu_end)[4]));
	qdf_nofl_info("mpdu_end: %#010x\n",
		      (unsigned int)(*(uint32_t *)&rx_desc->mpdu_end));
	qdf_nofl_info("ppdu_start: %#010x %#010x %#010x %#010x %#010x\n"
		      "%#010x %#010x %#010x %#010x %#010x\n",
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_start)[0]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_start)[1]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_start)[2]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_start)[3]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_start)[4]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_start)[5]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_start)[6]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_start)[7]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_start)[8]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_start)[9]));
	qdf_nofl_info("ppdu_end: %#010x %#010x %#010x %#010x %#010x\n"
		      "%#010x %#010x %#010x %#010x %#010x\n"
		      "%#010x,%#010x %#010x %#010x %#010x\n"
		      "%#010x %#010x %#010x %#010x %#010x\n" "%#010x %#010x\n",
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[0]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[1]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[2]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[3]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[4]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[5]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[6]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[7]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[8]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[9]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[10]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[11]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[12]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[13]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[14]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[15]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[16]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[17]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[18]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[19]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[20]),
		      (unsigned int)(((uint32_t *)&rx_desc->ppdu_end)[21]));
	qdf_nofl_info
		("---------------------------------------------------------\n");
}

#ifndef HTT_ASSERT_LEVEL
#define HTT_ASSERT_LEVEL 3
#endif

#define HTT_ASSERT_ALWAYS(condition) qdf_assert_always((condition))

#define HTT_ASSERT0(condition) qdf_assert((condition))
#if HTT_ASSERT_LEVEL > 0
#define HTT_ASSERT1(condition) qdf_assert((condition))
#else
#define HTT_ASSERT1(condition)
#endif

#if HTT_ASSERT_LEVEL > 1
#define HTT_ASSERT2(condition) qdf_assert((condition))
#else
#define HTT_ASSERT2(condition)
#endif

#if HTT_ASSERT_LEVEL > 2
#define HTT_ASSERT3(condition) qdf_assert((condition))
#else
#define HTT_ASSERT3(condition)
#endif

/*
 * HTT_MAX_SEND_QUEUE_DEPTH -
 * How many packets HTC should allow to accumulate in a send queue
 * before calling the EpSendFull callback to see whether to retain
 * or drop packets.
 * This is not relevant for LL, where tx descriptors should be immediately
 * downloaded to the target.
 * This is not very relevant for HL either, since it is anticipated that
 * the HL tx download scheduler will not work this far in advance - rather,
 * it will make its decisions just-in-time, so it can be responsive to
 * changing conditions.
 * Hence, this queue depth threshold spec is mostly just a formality.
 */
#define HTT_MAX_SEND_QUEUE_DEPTH 64

#define IS_PWR2(value) (((value) ^ ((value)-1)) == ((value) << 1) - 1)

/*
 * HTT_RX_PRE_ALLOC_POOL_SIZE -
 * How many Rx Buffer should be there in pre-allocated pool of buffers.
 * This is mainly for low memory condition where kernel fails to alloc
 * SKB buffer to the Rx ring.
 */
#define HTT_RX_PRE_ALLOC_POOL_SIZE 64
/* Max rx MSDU size including L2 headers */
#define MSDU_SIZE 1560
/* Rounding up to a cache line size. */
#define HTT_RX_BUF_SIZE  roundup(MSDU_SIZE +				\
				 sizeof(struct htt_host_rx_desc_base),	\
				 QDF_CACHE_LINE_SZ)
#define MAX_RX_PAYLOAD_SZ (HTT_RX_BUF_SIZE - RX_STD_DESC_SIZE)
/*
 * DMA_MAP expects the buffer to be an integral number of cache lines.
 * Rather than checking the actual cache line size, this code makes a
 * conservative estimate of what the cache line size could be.
 */
#define HTT_LOG2_MAX_CACHE_LINE_SIZE 7  /* 2^7 = 128 */
#define HTT_MAX_CACHE_LINE_SIZE_MASK ((1 << HTT_LOG2_MAX_CACHE_LINE_SIZE) - 1)

#ifdef BIG_ENDIAN_HOST
/*
 * big-endian: bytes within a 4-byte "word" are swapped:
 * pre-swap  post-swap
 *  index     index
 *    0         3
 *    1         2
 *    2         1
 *    3         0
 *    4         7
 *    5         6
 * etc.
 * To compute the post-swap index from the pre-swap index, compute
 * the byte offset for the start of the word (index & ~0x3) and add
 * the swapped byte offset within the word (3 - (index & 0x3)).
 */
#define HTT_ENDIAN_BYTE_IDX_SWAP(idx) (((idx) & ~0x3) + (3 - ((idx) & 0x3)))
#else
/* little-endian: no adjustment needed */
#define HTT_ENDIAN_BYTE_IDX_SWAP(idx) idx
#endif

#define HTT_TX_MUTEX_INIT(_mutex)			\
	qdf_spinlock_create(_mutex)

#define HTT_TX_MUTEX_ACQUIRE(_mutex)			\
	qdf_spin_lock_bh(_mutex)

#define HTT_TX_MUTEX_RELEASE(_mutex)			\
	qdf_spin_unlock_bh(_mutex)

#define HTT_TX_MUTEX_DESTROY(_mutex)			\
	qdf_spinlock_destroy(_mutex)

#ifdef ATH_11AC_TXCOMPACT

#define HTT_TX_NBUF_QUEUE_MUTEX_INIT(_pdev)		\
	qdf_spinlock_create(&_pdev->txnbufq_mutex)

#define HTT_TX_NBUF_QUEUE_MUTEX_DESTROY(_pdev)	       \
	HTT_TX_MUTEX_DESTROY(&_pdev->txnbufq_mutex)

#define HTT_TX_NBUF_QUEUE_REMOVE(_pdev, _msdu)	do {	\
	HTT_TX_MUTEX_ACQUIRE(&_pdev->txnbufq_mutex);	\
	_msdu =  qdf_nbuf_queue_remove(&_pdev->txnbufq);\
	HTT_TX_MUTEX_RELEASE(&_pdev->txnbufq_mutex);    \
	} while (0)

#define HTT_TX_NBUF_QUEUE_ADD(_pdev, _msdu) do {	\
	HTT_TX_MUTEX_ACQUIRE(&_pdev->txnbufq_mutex);	\
	qdf_nbuf_queue_add(&_pdev->txnbufq, _msdu);     \
	HTT_TX_MUTEX_RELEASE(&_pdev->txnbufq_mutex);    \
	} while (0)

#define HTT_TX_NBUF_QUEUE_INSERT_HEAD(_pdev, _msdu) do {   \
	HTT_TX_MUTEX_ACQUIRE(&_pdev->txnbufq_mutex);	   \
	qdf_nbuf_queue_insert_head(&_pdev->txnbufq, _msdu);\
	HTT_TX_MUTEX_RELEASE(&_pdev->txnbufq_mutex);       \
	} while (0)
#else

#define HTT_TX_NBUF_QUEUE_MUTEX_INIT(_pdev)
#define HTT_TX_NBUF_QUEUE_REMOVE(_pdev, _msdu)
#define HTT_TX_NBUF_QUEUE_ADD(_pdev, _msdu)
#define HTT_TX_NBUF_QUEUE_INSERT_HEAD(_pdev, _msdu)
#define HTT_TX_NBUF_QUEUE_MUTEX_DESTROY(_pdev)

#endif

#ifdef CONFIG_HL_SUPPORT

static inline void htt_tx_resume_handler(void *context)
{
}
#else

void htt_tx_resume_handler(void *context);
#endif

#ifdef ATH_11AC_TXCOMPACT
#define HTT_TX_SCHED htt_tx_sched
#else
#define HTT_TX_SCHED(pdev)      /* no-op */
#endif

int htt_tx_attach(struct htt_pdev_t *pdev, int desc_pool_elems);

void htt_tx_detach(struct htt_pdev_t *pdev);

int htt_rx_attach(struct htt_pdev_t *pdev);

#if defined(CONFIG_HL_SUPPORT)

static inline void htt_rx_detach(struct htt_pdev_t *pdev)
{
}
#else

void htt_rx_detach(struct htt_pdev_t *pdev);
#endif

int htt_htc_attach(struct htt_pdev_t *pdev, uint16_t service_id);

void htt_t2h_msg_handler(void *context, HTC_PACKET *pkt);
#ifdef WLAN_FEATURE_FASTPATH
void htt_t2h_msg_handler_fast(void *htt_pdev, qdf_nbuf_t *cmpl_msdus,
			      uint32_t num_cmpls);
#else
static inline void htt_t2h_msg_handler_fast(void *htt_pdev,
					   qdf_nbuf_t *cmpl_msdus,
					   uint32_t num_cmpls)
{
}
#endif

void htt_h2t_send_complete(void *context, HTC_PACKET *pkt);

QDF_STATUS htt_h2t_ver_req_msg(struct htt_pdev_t *pdev);

int htt_tx_padding_credit_update_handler(void *context, int pad_credit);

#if defined(HELIUMPLUS)
QDF_STATUS
htt_h2t_frag_desc_bank_cfg_msg(struct htt_pdev_t *pdev);
#endif /* defined(HELIUMPLUS) */

QDF_STATUS htt_h2t_rx_ring_cfg_msg_ll(struct htt_pdev_t *pdev);

QDF_STATUS htt_h2t_rx_ring_rfs_cfg_msg_ll(struct htt_pdev_t *pdev);

QDF_STATUS htt_h2t_rx_ring_rfs_cfg_msg_hl(struct htt_pdev_t *pdev);

QDF_STATUS htt_h2t_rx_ring_cfg_msg_hl(struct htt_pdev_t *pdev);

extern QDF_STATUS (*htt_h2t_rx_ring_cfg_msg)(struct htt_pdev_t *pdev);

enum htc_send_full_action htt_h2t_full(void *context, HTC_PACKET *pkt);

struct htt_htc_pkt *htt_htc_pkt_alloc(struct htt_pdev_t *pdev);

void htt_htc_pkt_free(struct htt_pdev_t *pdev, struct htt_htc_pkt *pkt);

void htt_htc_pkt_pool_free(struct htt_pdev_t *pdev);

#ifdef ATH_11AC_TXCOMPACT
void htt_htc_misc_pkt_list_trim(struct htt_pdev_t *pdev, int level);

void
htt_htc_misc_pkt_list_add(struct htt_pdev_t *pdev, struct htt_htc_pkt *pkt);

void htt_htc_misc_pkt_pool_free(struct htt_pdev_t *pdev);
#endif

#ifdef WLAN_FULL_REORDER_OFFLOAD
int
htt_rx_hash_list_insert(struct htt_pdev_t *pdev,
			qdf_dma_addr_t paddr,
			qdf_nbuf_t netbuf);
#else
static inline int
htt_rx_hash_list_insert(struct htt_pdev_t *pdev,
			qdf_dma_addr_t paddr,
			qdf_nbuf_t netbuf)
{
	return 0;
}
#endif

qdf_nbuf_t
htt_rx_hash_list_lookup(struct htt_pdev_t *pdev, qdf_dma_addr_t paddr);

#ifdef IPA_OFFLOAD
int
htt_tx_ipa_uc_attach(struct htt_pdev_t *pdev,
		     unsigned int uc_tx_buf_sz,
		     unsigned int uc_tx_buf_cnt,
		     unsigned int uc_tx_partition_base);

int
htt_rx_ipa_uc_attach(struct htt_pdev_t *pdev, unsigned int rx_ind_ring_size);

int htt_tx_ipa_uc_detach(struct htt_pdev_t *pdev);

int htt_rx_ipa_uc_detach(struct htt_pdev_t *pdev);

#else
/**
 * htt_tx_ipa_uc_attach() - attach htt ipa uc tx resource
 * @pdev: htt context
 * @uc_tx_buf_sz: single tx buffer size
 * @uc_tx_buf_cnt: total tx buffer count
 * @uc_tx_partition_base: tx buffer partition start
 *
 * Return: 0 success
 */
static inline int
htt_tx_ipa_uc_attach(struct htt_pdev_t *pdev,
		     unsigned int uc_tx_buf_sz,
		     unsigned int uc_tx_buf_cnt,
		     unsigned int uc_tx_partition_base)
{
	return 0;
}

/**
 * htt_rx_ipa_uc_attach() - attach htt ipa uc rx resource
 * @pdev: htt context
 * @rx_ind_ring_size: rx ring size
 *
 * Return: 0 success
 */
static inline int
htt_rx_ipa_uc_attach(struct htt_pdev_t *pdev, unsigned int rx_ind_ring_size)
{
	return 0;
}

static inline int htt_tx_ipa_uc_detach(struct htt_pdev_t *pdev)
{
	return 0;
}

static inline int htt_rx_ipa_uc_detach(struct htt_pdev_t *pdev)
{
	return 0;
}

#endif /* IPA_OFFLOAD */

/* Maximum Outstanding Bus Download */
#define HTT_MAX_BUS_CREDIT 33

#ifdef CONFIG_HL_SUPPORT

/**
 * htt_tx_credit_update() - check for diff in bus delta and target delta
 * @pdev: pointer to htt device.
 *
 * Return: min of bus delta and target delta
 */
int
htt_tx_credit_update(struct htt_pdev_t *pdev);
#else

static inline int
htt_tx_credit_update(struct htt_pdev_t *pdev)
{
	return 0;
}
#endif


#ifdef FEATURE_HL_GROUP_CREDIT_FLOW_CONTROL

#define HTT_TX_GROUP_INDEX_OFFSET \
(sizeof(struct htt_txq_group) / sizeof(u_int32_t))

void htt_tx_group_credit_process(struct htt_pdev_t *pdev, u_int32_t *msg_word);
#else

static inline
void htt_tx_group_credit_process(struct htt_pdev_t *pdev, u_int32_t *msg_word)
{
}
#endif

#ifdef DEBUG_RX_RING_BUFFER
/**
 * htt_rx_dbg_rxbuf_init() - init debug rx buff list
 * @pdev: pdev handle
 *
 * Allocation is done from bss segment. This uses vmalloc and has a bit
 * of an overhead compared to kmalloc (which qdf_mem_alloc wraps). The impact
 * of the overhead to performance will need to be quantified.
 *
 * Return: none
 */
static struct rx_buf_debug rx_buff_list_bss[HTT_RX_RING_BUFF_DBG_LIST];
static inline
void htt_rx_dbg_rxbuf_init(struct htt_pdev_t *pdev)
{
	pdev->rx_buff_list = rx_buff_list_bss;
	qdf_spinlock_create(&(pdev->rx_buff_list_lock));
	pdev->rx_buff_index = 0;
	pdev->rx_buff_posted_cum = 0;
	pdev->rx_buff_recvd_cum  = 0;
	pdev->rx_buff_recvd_err  = 0;
	pdev->refill_retry_timer_starts = 0;
	pdev->refill_retry_timer_calls = 0;
	pdev->refill_retry_timer_doubles = 0;
}

/**
 * htt_display_rx_buf_debug() - display debug rx buff list and some counters
 * @pdev: pdev handle
 *
 * Return: Success
 */
static inline int htt_display_rx_buf_debug(struct htt_pdev_t *pdev)
{
	int i;
	struct rx_buf_debug *buf;

	if ((pdev) &&
	    (pdev->rx_buff_list)) {
		buf = pdev->rx_buff_list;
		for (i = 0; i < HTT_RX_RING_BUFF_DBG_LIST; i++) {
			if (buf[i].posted != 0)
				qdf_nofl_info("[%d][0x%x] %pK %lu %pK %llu %llu",
					      i, buf[i].cpu,
					      buf[i].nbuf_data,
					      (unsigned long)buf[i].paddr,
					      buf[i].nbuf,
					      buf[i].posted,
					      buf[i].recved);
		}

		qdf_nofl_info("rxbuf_idx %d all_posted: %d all_recvd: %d recv_err: %d",
			      pdev->rx_buff_index,
			      pdev->rx_buff_posted_cum,
			      pdev->rx_buff_recvd_cum,
			      pdev->rx_buff_recvd_err);

		qdf_nofl_info("timer kicks :%d actual  :%d restarts:%d debtors: %d fill_n: %d",
			      pdev->refill_retry_timer_starts,
			      pdev->refill_retry_timer_calls,
			      pdev->refill_retry_timer_doubles,
			      pdev->rx_buff_debt_invoked,
			      pdev->rx_buff_fill_n_invoked);
	} else
		return -EINVAL;
	return 0;
}

/**
 * htt_rx_dbg_rxbuf_set() - set element of rx buff list
 * @pdev: pdev handle
 * @paddr: physical address of netbuf
 * @rx_netbuf: received netbuf
 *
 * Return: none
 */
static inline
void htt_rx_dbg_rxbuf_set(struct htt_pdev_t *pdev, qdf_dma_addr_t paddr,
			  qdf_nbuf_t rx_netbuf)
{
	if (pdev->rx_buff_list) {
		qdf_spin_lock_bh(&(pdev->rx_buff_list_lock));
		pdev->rx_buff_list[pdev->rx_buff_index].paddr = paddr;
		pdev->rx_buff_list[pdev->rx_buff_index].nbuf  = rx_netbuf;
		pdev->rx_buff_list[pdev->rx_buff_index].nbuf_data =
							rx_netbuf->data;
		pdev->rx_buff_list[pdev->rx_buff_index].posted =
						qdf_get_log_timestamp();
		pdev->rx_buff_posted_cum++;
		pdev->rx_buff_list[pdev->rx_buff_index].recved = 0;
		pdev->rx_buff_list[pdev->rx_buff_index].cpu =
				(1 << qdf_get_cpu());
		QDF_NBUF_CB_RX_MAP_IDX(rx_netbuf) = pdev->rx_buff_index;
		if (++pdev->rx_buff_index >=
				HTT_RX_RING_BUFF_DBG_LIST)
			pdev->rx_buff_index = 0;
		qdf_spin_unlock_bh(&(pdev->rx_buff_list_lock));
	}
}

/**
 * htt_rx_dbg_rxbuf_set() - reset element of rx buff list
 * @pdev: pdev handle
 * @netbuf: rx sk_buff
 * Return: none
 */
static inline
void htt_rx_dbg_rxbuf_reset(struct htt_pdev_t *pdev,
				qdf_nbuf_t netbuf)
{
	uint32_t index;

	if (pdev->rx_buff_list) {
		qdf_spin_lock_bh(&(pdev->rx_buff_list_lock));
		index = QDF_NBUF_CB_RX_MAP_IDX(netbuf);
		if (index < HTT_RX_RING_BUFF_DBG_LIST) {
			pdev->rx_buff_list[index].recved =
				qdf_get_log_timestamp();
			pdev->rx_buff_recvd_cum++;
		} else {
			pdev->rx_buff_recvd_err++;
		}
		pdev->rx_buff_list[pdev->rx_buff_index].cpu |=
				(1 << qdf_get_cpu());
		qdf_spin_unlock_bh(&(pdev->rx_buff_list_lock));
	}
}
/**
 * htt_rx_dbg_rxbuf_indupd() - add a record for alloc index update
 * @pdev: pdev handle
 * @idx : value of the index
 *
 * Return: none
 */
static inline
void htt_rx_dbg_rxbuf_indupd(struct htt_pdev_t *pdev, int alloc_index)
{
	if (pdev->rx_buff_list) {
		qdf_spin_lock_bh(&(pdev->rx_buff_list_lock));
		pdev->rx_buff_list[pdev->rx_buff_index].paddr = 0;
		pdev->rx_buff_list[pdev->rx_buff_index].nbuf  = 0;
		pdev->rx_buff_list[pdev->rx_buff_index].nbuf_data = 0;
		pdev->rx_buff_list[pdev->rx_buff_index].posted =
						qdf_get_log_timestamp();
		pdev->rx_buff_list[pdev->rx_buff_index].recved =
			(uint64_t)alloc_index;
		pdev->rx_buff_list[pdev->rx_buff_index].cpu =
				(1 << qdf_get_cpu());
		if (++pdev->rx_buff_index >=
				HTT_RX_RING_BUFF_DBG_LIST)
			pdev->rx_buff_index = 0;
		qdf_spin_unlock_bh(&(pdev->rx_buff_list_lock));
	}
}
/**
 * htt_rx_dbg_rxbuf_httrxind() - add a record for recipt of htt rx_ind msg
 * @pdev: pdev handle
 *
 * Return: none
 */
static inline
void htt_rx_dbg_rxbuf_httrxind(struct htt_pdev_t *pdev, unsigned int msdu_cnt)
{
	if (pdev->rx_buff_list) {
		qdf_spin_lock_bh(&(pdev->rx_buff_list_lock));
		pdev->rx_buff_list[pdev->rx_buff_index].paddr = msdu_cnt;
		pdev->rx_buff_list[pdev->rx_buff_index].nbuf  = 0;
		pdev->rx_buff_list[pdev->rx_buff_index].nbuf_data = 0;
		pdev->rx_buff_list[pdev->rx_buff_index].posted =
						qdf_get_log_timestamp();
		pdev->rx_buff_list[pdev->rx_buff_index].recved =
			(uint64_t)0x48545452584D5347; /* 'HTTRXMSG' */
		pdev->rx_buff_list[pdev->rx_buff_index].cpu =
				(1 << qdf_get_cpu());
		if (++pdev->rx_buff_index >=
				HTT_RX_RING_BUFF_DBG_LIST)
			pdev->rx_buff_index = 0;
		qdf_spin_unlock_bh(&(pdev->rx_buff_list_lock));
	}
}

/**
 * htt_rx_dbg_rxbuf_deinit() - deinit debug rx buff list
 * @pdev: pdev handle
 *
 * Return: none
 */
static inline
void htt_rx_dbg_rxbuf_deinit(struct htt_pdev_t *pdev)
{
	if (pdev->rx_buff_list)
		pdev->rx_buff_list = NULL;
	qdf_spinlock_destroy(&(pdev->rx_buff_list_lock));
}
#else
static inline
void htt_rx_dbg_rxbuf_init(struct htt_pdev_t *pdev)
{
}
static inline int htt_display_rx_buf_debug(struct htt_pdev_t *pdev)
{
	return 0;
}

static inline
void htt_rx_dbg_rxbuf_set(struct htt_pdev_t *pdev,
				uint32_t paddr,
				qdf_nbuf_t rx_netbuf)
{
}
static inline
void htt_rx_dbg_rxbuf_reset(struct htt_pdev_t *pdev,
				qdf_nbuf_t netbuf)
{
}
static inline
void htt_rx_dbg_rxbuf_indupd(struct htt_pdev_t *pdev,
			     int    alloc_index)
{
}
static inline
void htt_rx_dbg_rxbuf_httrxind(struct htt_pdev_t *pdev,
			       unsigned int msdu_cnt)
{
}
static inline
void htt_rx_dbg_rxbuf_deinit(struct htt_pdev_t *pdev)
{
	return;
}
#endif

#ifndef HTT_RX_RING_SIZE_MIN
#define HTT_RX_RING_SIZE_MIN 128        /* slightly > than one large A-MPDU */
#endif

#ifndef HTT_RX_RING_SIZE_MAX
#define HTT_RX_RING_SIZE_MAX 2048       /* ~20 ms @ 1 Gbps of 1500B MSDUs */
#endif

#ifndef HTT_RX_RING_SIZE_1x1
#define HTT_RX_RING_SIZE_1x1 1024      /* ~20 ms @ 400 Mbps of 1500B MSDUs */
#endif

#ifndef HTT_RX_AVG_FRM_BYTES
#define HTT_RX_AVG_FRM_BYTES 1000
#endif

#define HTT_FCS_LEN (4)

#ifdef HTT_DEBUG_DATA
#define HTT_PKT_DUMP(x) x
#else
#define HTT_PKT_DUMP(x) /* no-op */
#endif

#ifdef RX_HASH_DEBUG
#define HTT_RX_CHECK_MSDU_COUNT(msdu_count) HTT_ASSERT_ALWAYS(msdu_count)
#else
#define HTT_RX_CHECK_MSDU_COUNT(msdu_count)     /* no-op */
#endif

#if HTT_PADDR64
#define NEXT_FIELD_OFFSET_IN32 2
#else /* ! HTT_PADDR64 */
#define NEXT_FIELD_OFFSET_IN32 1
#endif /* HTT_PADDR64 */

#define RX_PADDR_MAGIC_PATTERN 0xDEAD0000

#if HTT_PADDR64
static inline qdf_dma_addr_t htt_paddr_trim_to_37(qdf_dma_addr_t paddr)
{
	qdf_dma_addr_t ret = paddr;

	if (sizeof(paddr) > 4)
		ret &= 0x1fffffffff;
	return ret;
}
#else /* not 64 bits */
static inline qdf_dma_addr_t htt_paddr_trim_to_37(qdf_dma_addr_t paddr)
{
	return paddr;
}
#endif /* HTT_PADDR64 */

#ifdef WLAN_FULL_REORDER_OFFLOAD
#ifdef ENABLE_DEBUG_ADDRESS_MARKING
static inline qdf_dma_addr_t
htt_rx_paddr_unmark_high_bits(qdf_dma_addr_t paddr)
{
	uint32_t markings;

	if (sizeof(qdf_dma_addr_t) > 4) {
		markings = (uint32_t)((paddr >> 16) >> 16);
		/*
		 * check if it is marked correctly:
		 * See the mark_high_bits function above for the expected
		 * pattern.
		 * the LS 5 bits are the high bits of physical address
		 * padded (with 0b0) to 8 bits
		 */
		if ((markings & 0xFFFF0000) != RX_PADDR_MAGIC_PATTERN) {
			qdf_print("paddr not marked correctly: 0x%pK!\n",
				  (void *)paddr);
			HTT_ASSERT_ALWAYS(0);
		}

		/* clear markings  for further use */
		paddr = htt_paddr_trim_to_37(paddr);
	}
	return paddr;
}

static inline
qdf_dma_addr_t htt_rx_in_ord_paddr_get(uint32_t *u32p)
{
	qdf_dma_addr_t paddr = 0;

	paddr = (qdf_dma_addr_t)HTT_RX_IN_ORD_PADDR_IND_PADDR_GET(*u32p);
	if (sizeof(qdf_dma_addr_t) > 4) {
		u32p++;
		/* 32 bit architectures dont like <<32 */
		paddr |= (((qdf_dma_addr_t)
			  HTT_RX_IN_ORD_PADDR_IND_PADDR_GET(*u32p))
			  << 16 << 16);
	}
	paddr = htt_rx_paddr_unmark_high_bits(paddr);

	return paddr;
}
#else
#if HTT_PADDR64
static inline
qdf_dma_addr_t htt_rx_in_ord_paddr_get(uint32_t *u32p)
{
	qdf_dma_addr_t paddr = 0;

	paddr = (qdf_dma_addr_t)HTT_RX_IN_ORD_PADDR_IND_PADDR_GET(*u32p);
	if (sizeof(qdf_dma_addr_t) > 4) {
		u32p++;
		/* 32 bit architectures dont like <<32 */
		paddr |= (((qdf_dma_addr_t)
			  HTT_RX_IN_ORD_PADDR_IND_PADDR_GET(*u32p))
			  << 16 << 16);
	}
	return paddr;
}
#else
static inline
qdf_dma_addr_t htt_rx_in_ord_paddr_get(uint32_t *u32p)
{
	return HTT_RX_IN_ORD_PADDR_IND_PADDR_GET(*u32p);
}
#endif
#endif /* ENABLE_DEBUG_ADDRESS_MARKING */

static inline
unsigned int htt_rx_in_order_ring_elems(struct htt_pdev_t *pdev)
{
	return (*pdev->rx_ring.alloc_idx.vaddr -
		*pdev->rx_ring.target_idx.vaddr) &
		pdev->rx_ring.size_mask;
}

static inline qdf_nbuf_t
htt_rx_in_order_netbuf_pop(htt_pdev_handle pdev, qdf_dma_addr_t paddr)
{
	HTT_ASSERT1(htt_rx_in_order_ring_elems(pdev) != 0);
	qdf_atomic_dec(&pdev->rx_ring.fill_cnt);
	paddr = htt_paddr_trim_to_37(paddr);
	return htt_rx_hash_list_lookup(pdev, paddr);
}

#else
static inline
qdf_dma_addr_t htt_rx_in_ord_paddr_get(uint32_t *u32p)
{
	return 0;
}

static inline qdf_nbuf_t
htt_rx_in_order_netbuf_pop(htt_pdev_handle pdev, qdf_dma_addr_t paddr)
{
	return NULL;
}
#endif

#if defined(FEATURE_MONITOR_MODE_SUPPORT) && defined(WLAN_FULL_REORDER_OFFLOAD)
int htt_rx_mon_amsdu_rx_in_order_pop_ll(htt_pdev_handle pdev,
					qdf_nbuf_t rx_ind_msg,
					qdf_nbuf_t *head_msdu,
					qdf_nbuf_t *tail_msdu,
					uint32_t *replenish_cnt);

/**
 * htt_rx_mon_get_rx_status() - Update information about the rx status,
 * which is used later for radiotap updation.
 * @pdev: Pointer to pdev handle
 * @rx_desc: Pointer to struct htt_host_rx_desc_base
 * @rx_status: Return variable updated with rx_status
 *
 * Return: None
 */
void htt_rx_mon_get_rx_status(htt_pdev_handle pdev,
			      struct htt_host_rx_desc_base *rx_desc,
			      struct mon_rx_status *rx_status);
#else
static inline
int htt_rx_mon_amsdu_rx_in_order_pop_ll(htt_pdev_handle pdev,
					qdf_nbuf_t rx_ind_msg,
					qdf_nbuf_t *head_msdu,
					qdf_nbuf_t *tail_msdu,
					uint32_t *replenish_cnt)
{
	return 0;
}

static inline
void htt_rx_mon_get_rx_status(htt_pdev_handle pdev,
			      struct htt_host_rx_desc_base *rx_desc,
			      struct mon_rx_status *rx_status)
{
}
#endif

#endif /* _HTT_INTERNAL__H_ */
