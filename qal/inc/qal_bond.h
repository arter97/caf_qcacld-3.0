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
 * DOC: qal_bond
 * QTI driver framework for bond related APIs prototype
 */

#ifndef __QAL_BOND_H
#define __QAL_BOND_H

#include <qdf_types.h>
#include <net/cfg80211.h>
#include <net/bonding.h>

struct net_device *qal_bond_create(struct net *net,
				   char *name, struct mlo_bond_info *mlo_info);

int qal_bond_create_slave(struct net_device *bond_dev,
			  struct net_device *slave_dev);

int qal_bond_slave_release(struct net_device *bond_dev,
			   struct net_device *slave_dev);

int qal_bond_destroy(struct net_device *bond_dev);

void *qal_bond_get_mlo_ctx(struct net_device *bond_dev);
#endif /* __QAL_BOND_H */
