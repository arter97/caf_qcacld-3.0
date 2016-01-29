/*
 * Copyright (c) 2013-2016 The Linux Foundation. All rights reserved.
 *
 * Previously licensed under the ISC license by Qualcomm Atheros, Inc.
 *
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

/*
 * This file was originally distributed by Qualcomm Atheros, Inc.
 * under proprietary terms before Copyright ownership was assigned
 * to the Linux Foundation.
 */

#include <cdf_types.h>
#include <cdf_status.h>
#include <cdf_softirq_timer.h>
#include <cdf_time.h>
#include <cdf_lock.h>
#include <cdf_memory.h>
#include <cdf_util.h>
#include <cdf_defer.h>
#include <cdf_atomic.h>
#include <cdf_nbuf.h>
#include <athdefs.h>
#include "cdf_net_types.h"
#include "a_types.h"
#include "athdefs.h"
#include "a_osapi.h"
#include <hif.h>
#include <htc_services.h>
#include <a_debug.h>
#include "hif_sdio_dev.h"
#include "if_sdio.h"
#include "regtable_sdio.h"

#define ATH_MODULE_NAME hif_sdio

/**
 * hif_start() - start hif bus interface.
 * @scn: HIF context
 *
 * Enables hif device interrupts
 *
 * Return: int
 */
uint32_t hif_start(struct ol_softc *scn)
{
	struct hif_sdio_dev *hif_device = (struct hif_sdio_dev *)scn->hif_hdl;
	HIF_SDIO_DEVICE *htc_sdio_device = hif_dev_from_hif(hif_device);
	HIF_ENTER();
	hif_dev_enable_interrupts(htc_sdio_device);
	HIF_EXIT();
	return A_OK;
}

/**
 * hif_flush_surprise_remove() - remove hif bus interface.
 * @scn: HIF context
 *
 *
 * Return: none
 */
void hif_flush_surprise_remove(struct ol_softc *scn)
{

}

/**
 * hif_stop() - stop hif bus interface.
 * @scn: HIF context
 *
 * Disable hif device interrupts and destroy hif context
 *
 * Return: none
 */
void hif_stop(struct ol_softc *scn)
{
	struct hif_sdio_dev *hif_device = (struct hif_sdio_dev *)scn->hif_hdl;
	HIF_SDIO_DEVICE *htc_sdio_device = hif_dev_from_hif(hif_device);
	HIF_ENTER();
	if (htc_sdio_device != NULL) {
		hif_dev_disable_interrupts(htc_sdio_device);
		hif_dev_destroy(htc_sdio_device);
	}
	HIF_EXIT();
}

/**
 * hif_send_head() - send data on hif bus interface.
 * @scn: HIF context
 *
 * send tx data on a given pipe id
 *
 * Return: int
 */
CDF_STATUS hif_send_head(struct ol_softc *scn, uint8_t pipe,
		uint32_t transferID, uint32_t nbytes, cdf_nbuf_t buf,
		uint32_t data_attr)
{
	struct hif_sdio_dev *hif_device = (struct hif_sdio_dev *)scn->hif_hdl;
	HIF_SDIO_DEVICE *htc_sdio_device = hif_dev_from_hif(hif_device);
	return hif_dev_send_buffer(htc_sdio_device,
				transferID, pipe,
				nbytes, buf);

}

/**
 * hif_map_service_to_pipe() - maps ul/dl pipe to service id.
 * @scn: HIF context
 * @ServiceId: sevice index
 * @ULPipe: uplink pipe id
 * @DLPipe: down-linklink pipe id
 * @ul_is_polled: if ul is polling based
 * @ul_is_polled: if dl is polling based
 *
 * Return: int
 */
int hif_map_service_to_pipe(struct ol_softc *scn, uint16_t ServiceId,
			    uint8_t *ULPipe, uint8_t *DLPipe, int *ul_is_polled,
			    int *dl_is_polled)
{
	struct hif_sdio_dev *hif_device = (struct hif_sdio_dev *)scn->hif_hdl;
	HIF_SDIO_DEVICE *htc_sdio_device = hif_dev_from_hif(hif_device);

	return hif_dev_map_service_to_pipe(htc_sdio_device,
					   ServiceId, ULPipe, DLPipe,
					   hif_device->swap_mailbox);
}

/**
 * hif_map_service_to_pipe() - maps ul/dl pipe to service id.
 * @scn: HIF context
 * @ServiceId: sevice index
 * @ULPipe: uplink pipe id
 * @DLPipe: down-linklink pipe id
 * @ul_is_polled: if ul is polling based
 * @ul_is_polled: if dl is polling based
 *
 * Return: int
 */
void hif_get_default_pipe(struct ol_softc *scn, uint8_t *ULPipe,
			  uint8_t *DLPipe)
{
	hif_map_service_to_pipe(scn,
				HTC_CTRL_RSVD_SVC, ULPipe, DLPipe, NULL, NULL);
}

/**
 * hif_post_init() - create hif device after probe.
 * @scn: HIF context
 * @target: HIF target
 * @callbacks: htc callbacks
 *
 *
 * Return: int
 */
void hif_post_init(struct ol_softc *scn, void *target,
		   struct hif_msg_callbacks *callbacks)
{
	struct hif_sdio_dev *hif_device = (struct hif_sdio_dev *)scn->hif_hdl;
	HIF_SDIO_DEVICE *htc_sdio_device = hif_dev_from_hif(hif_device);
	if (htc_sdio_device == NULL)
		htc_sdio_device = hif_dev_create(hif_device, callbacks, target);

	if (htc_sdio_device)
		hif_dev_setup(htc_sdio_device);

	return;
}

/**
 * hif_get_free_queue_number() - create hif device after probe.
 * @scn: HIF context
 * @pipe: pipe id
 *
 * SDIO uses credit based flow control at the HTC layer
 * so transmit resource checks are bypassed
 * Return: int
 */
uint16_t hif_get_free_queue_number(struct ol_softc *scn, uint8_t pipe)
{
	uint16_t rv;
	rv = 1;

	return rv;
}

/**
 * hif_send_complete_check() - check tx complete on a given pipe.
 * @scn: HIF context
 * @pipe: HIF target
 * @force: check if need to pool for completion
 * Decide whether to actually poll for completions, or just
 * wait for a later chance.
 *
 * Return: int
 */
void hif_send_complete_check(struct ol_softc *scn, uint8_t pipe,
				int force)
{

}

/**
 * hif_set_bundle_mode() - set bundling mode.
 * @scn: HIF context
 * @enabled: enable/disable bundling
 * @rx_bundle_cnt: bundling count
 *
 * Return: none
 */
void hif_set_bundle_mode(struct ol_softc *scn, bool enabled,
			uint64_t rx_bundle_cnt)
{

}

/**
 * hif_check_soc_status() - create hif device state.
 * @scn: HIF context
 *
 *
 * Return: int
 */
int hif_check_soc_status(struct ol_softc *scn)
{
	return 0;
}

/**
 * hif_dump_target_memory() - dump target memory.
 * @scn: HIF context
 * @ramdump_base: buffer location
 * @rx_bundle_cnt: ramdump base address
 * @size: dump size
 *
 * Return: int
 */
void hif_dump_target_memory(struct ol_softc *scn, void *ramdump_base,
		uint32_t address, uint32_t size)
{
}

/**
 * hif_ipa_get_ce_resource() - gets ipa resources.
 * @scn: HIF context
 * @sr_base_paddr: source base address
 * @sr_ring_size: source ring size
 * @reg_paddr: bus physical address
 *
 * Return: int
 */
void hif_ipa_get_ce_resource(struct ol_softc *scn,
		uint32_t *sr_base_paddr,
		uint32_t *sr_ring_size,
		cdf_dma_addr_t *reg_paddr)
{
}
