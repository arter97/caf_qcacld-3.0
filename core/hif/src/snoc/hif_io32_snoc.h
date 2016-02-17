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

/**
 * DOC: hif_io32_snoc.h
 *
 * snoc specific implementations and configurations
 */

#ifndef __HIF_IO32_SNOC_H__
#define __HIF_IO32_SNOC_H__

#ifdef HIF_PCI
#error snoc and pci cannot be supported in parrallel at this time
#endif

#include "hif.h"
#include "regtable.h"
#include "ce_reg.h"
#include "cdf_atomic.h"
#include <soc/qcom/icnss.h>
#include "hif_main.h"
#include "hif_debug.h"

/**
 * Following features are not supported for snoc bus
 * Force 0 and consider moving corresponding code into
 * pci specific files
 */
#define CONFIG_ATH_PCIE_MAX_PERF 0
#define CONFIG_ATH_PCIE_AWAKE_WHILE_DRIVER_LOAD 0
#define CONFIG_ATH_PCIE_ACCESS_LIKELY 0
#define CONFIG_PCIE_ENABLE_L1_CLOCK_GATE 0

#define A_TARGET_ACCESS_LIKELY(scn)
#define A_TARGET_ACCESS_UNLIKELY(scn)
#define A_TARGET_ACCESS_BEGIN_RET_PTR(scn)
#define A_TARGET_ACCESS_END_RET_PTR(scn)
#define A_TARGET_ACCESS_BEGIN(scn)
#define A_TARGET_ACCESS_END(scn)
#define A_TARGET_ACCESS_BEGIN_RET(scn)
#define A_TARGET_ACCESS_END_RET(scn)
#define A_TARGET_ACCESS_BEGIN_RET_EXT(scn, val)
#define A_TARGET_ACCESS_END_RET_EXT(scn, val)

#define Q_TARGET_ACCESS_BEGIN(scn) 0
#define Q_TARGET_ACCESS_END(scn) 0

static inline void hif_pci_cancel_deferred_target_sleep(struct ol_softc *scn)
{
	return;
}

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

/**
 * hif_write32_mb - SNOC write 32
 * @addr: physical address
 * @value: value
 *
 * Return: N/A
 */
static inline void hif_write32_mb(void __iomem *addr, uint32_t value)
{
	wmb(); /* write memory barrier */
	writel_relaxed((value), (addr));
	wmb(); /* write memory barrier */
}

/**
 * hif_read32_mb - SNOC read 32
 * @addr: physical address
 *
 * Return: N/A
 */
static inline uint32_t hif_read32_mb(void __iomem *addr)
{
	uint32_t tmp;
	rmb(); /* read memory barrier */
	tmp = readl_relaxed(addr);
	rmb(); /* read memory barrier */
	return tmp;
}

#define A_TARGET_READ(scn, offset) \
	hif_read32_mb(scn->mem + (offset))
#define A_TARGET_WRITE(scn, offset, value) \
	hif_write32_mb((scn->mem + offset), (value))

static inline void ce_enable_irq_in_individual_register(struct ol_softc *scn,
		int ce_id)
{
	uint32_t offset;
	offset = HOST_IE_ADDRESS + CE_BASE_ADDRESS(ce_id);
	hif_write32_mb(scn->mem + offset, 1);
	hif_read32_mb(scn->mem + offset);
}

static inline void ce_disable_irq_in_individual_register(struct ol_softc *scn,
		int ce_id)
{
	uint32_t offset;
	offset = HOST_IE_ADDRESS + CE_BASE_ADDRESS(ce_id);
	hif_write32_mb(scn->mem + offset, 0);
	hif_read32_mb(scn->mem + offset);
}

/**
 * ce_irq_enable() - enable copy engine IRQ
 * @scn: struct ol_softc
 * @ce_id: ce_id
 *
 * Return: N/A
 */
static inline void ce_irq_enable(struct ol_softc *scn,
		int ce_id)
{
	ce_enable_irq_in_individual_register(scn, ce_id);
}

/**
 * ce_irq_disable() - disable copy engine IRQ
 * @scn: struct ol_softc
 * @ce_id: ce_id
 *
 * Return: N/A
 */
static inline void ce_irq_disable(struct ol_softc *scn, int ce_id)
{
	ce_disable_irq_in_individual_register(scn, ce_id);
}
#endif
