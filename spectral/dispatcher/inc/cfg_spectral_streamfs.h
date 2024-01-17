/**
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * DOC: This file contains centralized cfg definitions of Spectral component
 * with streamfs data transport functionality.
 */
#ifndef __CONFIG_SPECTRAL_STREAMFS_H
#define __CONFIG_SPECTRAL_STREAMFS_H

/*
 * <ini>
 * spectral_streamfs_num_bufs - Maximum no. of sub-buffers allocated.
 * @min: 512
 * @max: 2048
 * @Default: 512
 *
 * This ini is used to set the maximum number of streamfs sub-buffers in a
 * channel buffer.
 *
 * Related: None
 *
 * Supported Feature: Spectral
 *
 * Usage: External
 *
 * </ini>
 */

#define CFG_SPECTRAL_STREAMFS_NUM_BUFFERS \
	CFG_INI_UINT("spectral_streamfs_num_bufs", 512, \
			2048, 512, \
			CFG_VALUE_OR_DEFAULT, \
			"Spectral streamfs no. of buffers")

/*
 * <ini>
 * spectral_streamfs_databuf_size - Maximum size of sub-buffer
 * allocated in bytes.
 * @min: 16
 * @max: 4096
 * @Default: 512
 *
 * This ini is used to set the maximum size of each streamfs sub-buffer.
 *
 * Related: None
 *
 * Supported Feature: Spectral
 *
 * Usage: External
 *
 * </ini>
 */

#define CFG_SPECTRAL_STREAMFS_BUFFER_SIZE \
	CFG_INI_UINT("spectral_streamfs_databuf_size", 16, \
			4096, 512, \
			CFG_VALUE_OR_DEFAULT, \
			"Spectral streamfs buffer size")



#define CFG_SPECTRAL_STREAMFS_ALL \
	CFG(CFG_SPECTRAL_STREAMFS_NUM_BUFFERS) \
	CFG(CFG_SPECTRAL_STREAMFS_BUFFER_SIZE)

#endif
