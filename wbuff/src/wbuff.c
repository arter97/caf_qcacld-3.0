/*
 * Copyright (c) 2018-2019 The Linux Foundation. All rights reserved.
 * Copyright (c) 2021, 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

/**
 * DOC: wbuff.c
 * wbuff buffer management APIs
 */

#include <wbuff.h>
#include "i_wbuff.h"

/*
 * Allocation holder array for all wbuff registered modules
 */
struct wbuff_holder wbuff;

/**
 * wbuff_get_pool_slot_from_len() - get pool_id from length
 * @len: length of the buffer
 *
 * Return: pool_id
 */
static uint8_t wbuff_get_pool_slot_from_len(uint16_t len)
{
	if ((len > 0) && (len <= WBUFF_LEN_POOL0))
		return WBUFF_POOL_0;
	else if ((len > WBUFF_LEN_POOL0) && (len <= WBUFF_LEN_POOL1))
		return WBUFF_POOL_1;
	else if ((len > WBUFF_LEN_POOL1) && (len <= WBUFF_LEN_POOL2))
		return WBUFF_POOL_2;
	else
		return WBUFF_POOL_3;
}

/**
 * wbuff_get_len_from_pool_slot() - get len from pool_id
 * @pool_id: pool ID
 *
 * Return: nbuf length from pool_id
 */
static uint32_t wbuff_get_len_from_pool_slot(uint16_t pool_id)
{
	uint32_t len = 0;

	switch (pool_id) {
	case 0:
		len = WBUFF_LEN_POOL0;
		break;
	case 1:
		len = WBUFF_LEN_POOL1;
		break;
	case 2:
		len = WBUFF_LEN_POOL2;
		break;
	case 3:
		len = WBUFF_LEN_POOL3;
		break;
	default:
		len = 0;
	}

	return len;
}

/**
 * wbuff_get_free_mod_slot() - get free module_id
 *
 * Return: module_id
 */
static uint8_t wbuff_get_free_mod_slot(void)
{
	uint8_t module_id = 0;

	for (module_id = 0; module_id < WBUFF_MAX_MODULES; module_id++) {
		qdf_spin_lock_bh(&wbuff.mod[module_id].lock);
		if (!wbuff.mod[module_id].registered) {
			wbuff.mod[module_id].registered = true;
			qdf_spin_unlock_bh(&wbuff.mod[module_id].lock);
			break;
		}
		qdf_spin_unlock_bh(&wbuff.mod[module_id].lock);
	}

	return module_id;
}

/**
 * wbuff_is_valid_alloc_req() - validate alloc  request
 * @req: allocation request from registered module
 * @num: number of pools required
 *
 * Return: true if valid wbuff_alloc_request
 *         false if invalid wbuff_alloc_request
 */
static bool wbuff_is_valid_alloc_req(struct wbuff_alloc_request *req,
				     uint8_t num)
{
	uint16_t psize = 0;
	uint8_t alloc = 0, pool_id = 0;

	for (alloc = 0; alloc < num; alloc++) {
		pool_id = req[alloc].slot;
		psize = req[alloc].size;
		if ((pool_id > WBUFF_MAX_POOLS - 1) ||
		    (psize > wbuff_alloc_max[pool_id]))
			return false;
	}

	return true;
}

/**
 * wbuff_prepare_nbuf() - allocate nbuf
 * @module_id: module ID
 * @pool_id: pool ID
 * @len: length of the buffer
 * @reserve: nbuf headroom to start with
 * @align: alignment for the nbuf
 *
 * Return: nbuf if success
 *         NULL if failure
 */
static qdf_nbuf_t wbuff_prepare_nbuf(uint8_t module_id, uint8_t pool_id,
				     uint32_t len, int reserve, int align)
{
	qdf_nbuf_t buf;
	unsigned long dev_scratch = 0;

	buf = qdf_nbuf_alloc(NULL, roundup(len + reserve, align), reserve,
			     align, false);
	if (!buf)
		return NULL;
	dev_scratch = module_id;
	dev_scratch <<= WBUFF_MODULE_ID_SHIFT;
	dev_scratch |= ((pool_id << WBUFF_POOL_ID_SHIFT) | 1);
	qdf_nbuf_set_dev_scratch(buf, dev_scratch);

	return buf;
}

/**
 * wbuff_is_valid_handle() - validate wbuff handle
 * @handle: wbuff handle passed by module
 *
 * Return: true - valid wbuff_handle
 *         false - invalid wbuff_handle
 */
static bool wbuff_is_valid_handle(struct wbuff_handle *handle)
{
	if ((handle) && (handle->id < WBUFF_MAX_MODULES) &&
	    (wbuff.mod[handle->id].registered))
		return true;

	return false;
}

QDF_STATUS wbuff_module_init(void)
{
	struct wbuff_module *mod = NULL;
	uint8_t module_id = 0, pool_id = 0;

	if (!qdf_nbuf_is_dev_scratch_supported()) {
		wbuff.initialized = false;
		return QDF_STATUS_E_NOSUPPORT;
	}

	for (module_id = 0; module_id < WBUFF_MAX_MODULES; module_id++) {
		mod = &wbuff.mod[module_id];
		qdf_spinlock_create(&mod->lock);
		for (pool_id = 0; pool_id < WBUFF_MAX_POOLS; pool_id++)
			mod->pool[pool_id] = NULL;
		mod->registered = false;
	}
	wbuff.initialized = true;

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS wbuff_module_deinit(void)
{
	struct wbuff_module *mod = NULL;
	uint8_t module_id = 0;

	if (!wbuff.initialized)
		return QDF_STATUS_E_INVAL;

	wbuff.initialized = false;
	for (module_id = 0; module_id < WBUFF_MAX_MODULES; module_id++) {
		mod = &wbuff.mod[module_id];
		if (mod->registered)
			wbuff_module_deregister((struct wbuff_mod_handle *)
						&mod->handle);
		qdf_spinlock_destroy(&mod->lock);
	}

	return QDF_STATUS_SUCCESS;
}

struct wbuff_mod_handle *
wbuff_module_register(struct wbuff_alloc_request *req, uint8_t num,
		      int reserve, int align)
{
	struct wbuff_module *mod = NULL;
	qdf_nbuf_t buf = NULL;
	uint32_t len = 0;
	uint16_t idx = 0, psize = 0;
	uint8_t alloc = 0, module_id = 0, pool_id = 0;

	if (!wbuff.initialized)
		return NULL;

	if ((num == 0) || (num > WBUFF_MAX_POOLS))
		return NULL;

	if (!wbuff_is_valid_alloc_req(req, num))
		return NULL;

	module_id = wbuff_get_free_mod_slot();
	if (module_id == WBUFF_MAX_MODULES)
		return NULL;

	mod = &wbuff.mod[module_id];

	mod->handle.id = module_id;

	for (alloc = 0; alloc < num; alloc++) {
		pool_id = req[alloc].slot;
		psize = req[alloc].size;
		len = wbuff_get_len_from_pool_slot(pool_id);
		/**
		 * Allocate pool_cnt number of buffers for
		 * the pool given by pool_id
		 */
		for (idx = 0; idx < psize; idx++) {
			buf = wbuff_prepare_nbuf(module_id, pool_id, len,
						 reserve, align);
			if (!buf)
				continue;
			if (!mod->pool[pool_id]) {
				qdf_nbuf_set_next(buf, NULL);
				mod->pool[pool_id] = buf;
			} else {
				qdf_nbuf_set_next(buf, mod->pool[pool_id]);
				mod->pool[pool_id] = buf;
			}
		}
	}
	mod->reserve = reserve;
	mod->align = align;

	return (struct wbuff_mod_handle *)&mod->handle;
}

QDF_STATUS wbuff_module_deregister(struct wbuff_mod_handle *hdl)
{
	struct wbuff_handle *handle;
	struct wbuff_module *mod = NULL;
	uint8_t module_id = 0, pool_id = 0;
	qdf_nbuf_t first = NULL, buf = NULL;

	handle = (struct wbuff_handle *)hdl;

	if ((!wbuff.initialized) || (!wbuff_is_valid_handle(handle)))
		return QDF_STATUS_E_INVAL;

	module_id = handle->id;
	mod = &wbuff.mod[module_id];

	qdf_spin_lock_bh(&mod->lock);
	for (pool_id = 0; pool_id < WBUFF_MAX_POOLS; pool_id++) {
		first = mod->pool[pool_id];
		while (first) {
			buf = first;
			first = qdf_nbuf_next(buf);
			qdf_nbuf_free(buf);
		}
	}
	mod->registered = false;
	qdf_spin_unlock_bh(&mod->lock);

	return QDF_STATUS_SUCCESS;
}

qdf_nbuf_t wbuff_buff_get(struct wbuff_mod_handle *hdl, uint32_t len,
			  const char *func_name, uint32_t line_num)
{
	struct wbuff_handle *handle;
	struct wbuff_module *mod = NULL;
	uint8_t module_id = 0;
	uint8_t pool_id = 0;
	qdf_nbuf_t buf = NULL;

	handle = (struct wbuff_handle *)hdl;

	if ((!wbuff.initialized) || (!wbuff_is_valid_handle(handle)) || !len ||
	    (len > WBUFF_MAX_BUFFER_SIZE))
		return NULL;

	module_id = handle->id;
	pool_id = wbuff_get_pool_slot_from_len(len);
	mod = &wbuff.mod[module_id];

	qdf_spin_lock_bh(&mod->lock);
	if (mod->pool[pool_id]) {
		buf = mod->pool[pool_id];
		mod->pool[pool_id] = qdf_nbuf_next(buf);
		mod->pending_returns++;
	}
	qdf_spin_unlock_bh(&mod->lock);
	if (buf) {
		qdf_nbuf_set_next(buf, NULL);
		qdf_net_buf_debug_update_node(buf, func_name, line_num);
	}

	return buf;
}

qdf_nbuf_t wbuff_buff_put(qdf_nbuf_t buf)
{
	qdf_nbuf_t buffer = buf;
	unsigned long pool_info = 0;
	uint8_t module_id = 0, pool_id = 0;

	if (!wbuff.initialized)
		return buffer;

	pool_info = qdf_nbuf_get_dev_scratch(buf);
	if (!pool_info)
		return buffer;

	module_id = (pool_info & WBUFF_MODULE_ID_BITMASK) >>
			WBUFF_MODULE_ID_SHIFT;
	pool_id = (pool_info & WBUFF_POOL_ID_BITMASK) >> WBUFF_POOL_ID_SHIFT;

	if (module_id >= WBUFF_MAX_MODULES || pool_id >= WBUFF_MAX_POOLS)
		return NULL;

	qdf_nbuf_reset(buffer, wbuff.mod[module_id].reserve,
		       wbuff.mod[module_id].align);

	qdf_spin_lock_bh(&wbuff.mod[module_id].lock);
	if (wbuff.mod[module_id].registered) {
		qdf_nbuf_set_next(buffer, wbuff.mod[module_id].pool[pool_id]);
		wbuff.mod[module_id].pool[pool_id] = buffer;
		wbuff.mod[module_id].pending_returns--;
		buffer = NULL;
	}
	qdf_spin_unlock_bh(&wbuff.mod[module_id].lock);

	return buffer;
}
