/*
 * Copyright (c) 2015 The Linux Foundation. All rights reserved.
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

#ifndef __HIF_IO32_H__
#define __HIF_IO32_H__

#include <linux/io.h>
#include "ol_if_athvar.h"
#include "hif.h"
#ifdef HIF_PCI
#include "hif_io32_pci.h"
#elif defined(HIF_SDIO)
/**
 * hif_pci_cancel_deferred_target_sleep() - defer bus target sleep
 * @scn: ol_softc
 *
 * Return: void
 */
static inline void hif_pci_cancel_deferred_target_sleep(struct ol_softc *scn)
{
	return;
}

/**
 * hif_target_sleep_state_adjust() - on-demand sleep/wake
 * @scn: ol_softc pointer.
 * @sleep_ok: bool
 * @wait_for_it: bool
 *
 * Output the pipe error counts of each pipe to log file
 *
 * Return: none
 */
static inline void hif_target_sleep_state_adjust(struct ol_softc *scn,
						bool sleep_ok, bool wait_for_it)
{
	return;
}

/**
 * soc_wake_reset() - soc_wake_reset
 * @scn: ol_softc
 *
 * Return: void
 */
static inline void soc_wake_reset(struct ol_softc *scn)
{
}
#else
#include "hif_io32_snoc.h"
#endif /* HIF_PCI */
#endif /* __HIF_IO32_H__ */
