/*
 * Copyright (c) 2012-2014, 2017-2019 The Linux Foundation. All rights reserved.
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

#ifndef _WDI_EVENT_API_H_
#define _WDI_EVENT_API_H_

#include "wdi_event.h"
#include <cdp_txrx_handle.h>
struct ol_txrx_pdev_t;

#ifdef WDI_EVENT_ENABLE
/**
 * @brief Subscribe to a specified WDI event.
 * @details
 *  This function adds the provided wdi_event_subscribe object to a list of
 *  subscribers for the specified WDI event.
 *  When the event in question happens, each subscriber for the event will
 *  have their callback function invoked.
 *  The order in which callback functions from multiple subscribers are
 *  invoked is unspecified.
 *
 * @param pdev - the event physical device, that maintains the event lists
 * @param event_cb_sub - the callback and context for the event subscriber
 * @param event - which event's notifications are being subscribed to
 * @return error code, or A_OK for success
 */
A_STATUS wdi_event_sub(struct cdp_pdev *ppdev,
		       void *event_cb_sub,
		       uint32_t event);

/**
 * @brief Unsubscribe from a specified WDI event.
 * @details
 *  This function removes the provided event subscription object from the
 *  list of subscribers for its event.
 *  This function shall only be called if there was a successful prior call
 *  to event_sub() on the same wdi_event_subscribe object.
 *
 * @param pdev - the event physical device with the list of event subscribers
 * @param event_cb_sub - the event subscription object
 * @param event - which event is being unsubscribed
 * @return error code, or 0 for success
 */
int wdi_event_unsub(struct cdp_pdev *ppdev,
		    void *event_cb_sub,
		    uint32_t event);


void wdi_event_handler(enum WDI_EVENT event,
		       struct cdp_pdev *txrx_pdev, void *data);
A_STATUS wdi_event_attach(struct ol_txrx_pdev_t *txrx_pdev);
A_STATUS wdi_event_detach(struct ol_txrx_pdev_t *txrx_pdev);

#else

static inline void wdi_event_handler(enum WDI_EVENT event,
		       struct cdp_pdev *txrx_pdev, void *data)
{
}

static inline A_STATUS wdi_event_attach(struct ol_txrx_pdev_t *txrx_pdev)
{
	return A_OK;
}

static inline A_STATUS wdi_event_detach(struct ol_txrx_pdev_t *txrx_pdev)
{
	return A_OK;
}

static inline A_STATUS wdi_event_sub(struct cdp_pdev *ppdev, void *event_cb_sub,
				     uint32_t event)
{
	return A_OK;
}

static inline int wdi_event_unsub(struct cdp_pdev *ppdev,
				  void *event_cb_sub,
				  uint32_t event)
{
	return 0;
}
#endif /* WDI_EVENT_ENABLE */

#endif /* _WDI_EVENT_API_H_ */
