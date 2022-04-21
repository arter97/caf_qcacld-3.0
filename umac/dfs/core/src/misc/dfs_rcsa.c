/*
 * Copyright (c) 2017-2020 The Linux Foundation. All rights reserved.
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
 * DOC: This file has DFS_RCSA Functions.
 *
 */

#include <dfs.h>
#include <dfs_process_radar_found_ind.h>
#include <wlan_dfs_mlme_api.h>
#include <wlan_dfs_utils_api.h>
#include "../dfs_precac_forest.h"

#if defined(QCA_DFS_RCSA_SUPPORT)
/**
 * dfs_convert_range2nol_ie_vars() - Convert the NOL range to NOL IE contents.
 * @dfs: Pointer to wlan_dfs.
 * @nol_range: Range of NOL affected frequencies.
 *
 * If subchannel marking is enabled or if the current mode is 5/10MHz,
 * the NOL will have range spanning from 5-20MHz. This range needs to be
 * represented in a variable BW bitmap in comparison to BW > 20, which can
 * be represented by 20MHz spaced bitmap.
 *
 * Return: True if range is converted to NOL IE, else false.
 */
static bool dfs_convert_range2nol_ie_vars(struct wlan_dfs *dfs,
					  struct dfs_freq_range nol_range)
{
	if (dfs->dfs_use_nol_subchannel_marking ||
	    WLAN_IS_CHAN_MODE_5(dfs->dfs_curchan) ||
	    WLAN_IS_CHAN_MODE_10(dfs->dfs_curchan)) {
		dfs->dfs_nol_ie_bandwidth =
			nol_range.end_freq - nol_range.start_freq;
		dfs->dfs_nol_ie_startfreq =
			(nol_range.start_freq + nol_range.end_freq) / 2;
		dfs->dfs_nol_ie_bitmap = 0x01;
		return true;
	}
	return false;
}

/* dfs_prepare_nol_ie_bitmap: Create a Bitmap from the radar found subchannels
 * to be sent along with RCSA.
 * @dfs: Pointer to wlan_dfs.
 * @radar_found: Pointer to radar_found_info.
 * @in_sub_channels: Pointer to Sub-channels.
 * @n_in_sub_channels: Number of sub-channels.
 * @nol_range: Range of NOL affected frequencies.
 */
#ifdef CONFIG_CHAN_FREQ_API
static void
dfs_prepare_nol_ie_bitmap_for_freq(struct wlan_dfs *dfs,
				   struct radar_found_info *radar_found,
				   uint16_t *in_sub_channels,
				   uint8_t n_in_sub_channels,
				   struct dfs_freq_range nol_range)
{
	uint16_t cur_subchans[NUM_CHANNELS_160MHZ];
	uint8_t n_cur_subchans;
	uint8_t i;
	uint8_t j;
	uint8_t bits = 0x01;

	if (dfs_convert_range2nol_ie_vars(dfs, nol_range))
		goto exit;

	n_cur_subchans =
	    dfs_get_bonding_channels_for_freq(dfs, dfs->dfs_curchan,
					      radar_found->segment_id,
					      radar_found->detector_id,
					      cur_subchans);
	dfs->dfs_nol_ie_bandwidth = MIN_DFS_SUBCHAN_BW;
	dfs->dfs_nol_ie_startfreq = cur_subchans[0];

	/* Search through the array list of radar affected subchannels
	 * to find if the subchannel in our current channel has radar hit.
	 * Break if found to reduce loop count.
	 */
	for (i = 0; i < n_cur_subchans; i++) {
		for (j = 0; j < n_in_sub_channels; j++) {
			if (cur_subchans[i] == in_sub_channels[j]) {
				dfs->dfs_nol_ie_bitmap |= bits;
				break;
			}
		}
		bits <<= 1;
	}

exit:
	dfs_debug(dfs, WLAN_DEBUG_DFS_NOL,
		  "RCSA info: start_freq %d, bw %d, bitmap %x",
		  dfs->dfs_nol_ie_startfreq, dfs->dfs_nol_ie_bandwidth,
		  dfs->dfs_nol_ie_bitmap);
}
#endif

void dfs_fetch_nol_ie_info(struct wlan_dfs *dfs,
			   uint8_t *nol_ie_bandwidth,
			   uint16_t *nol_ie_startfreq,
			   uint8_t *nol_ie_bitmap)
{
	if (nol_ie_bandwidth)
		*nol_ie_bandwidth = dfs->dfs_nol_ie_bandwidth;
	if (nol_ie_startfreq)
		*nol_ie_startfreq = dfs->dfs_nol_ie_startfreq;
	if (nol_ie_bitmap)
		*nol_ie_bitmap = dfs->dfs_nol_ie_bitmap;
}

void dfs_get_rcsa_flags(struct wlan_dfs *dfs, bool *is_rcsa_ie_sent,
			bool *is_nol_ie_sent)
{
	if (is_rcsa_ie_sent)
		*is_rcsa_ie_sent = dfs->dfs_is_rcsa_ie_sent;
	if (is_nol_ie_sent)
		*is_nol_ie_sent = dfs->dfs_is_nol_ie_sent;
}

void dfs_set_rcsa_flags(struct wlan_dfs *dfs, bool is_rcsa_ie_sent,
			bool is_nol_ie_sent)
{
	dfs->dfs_is_rcsa_ie_sent = is_rcsa_ie_sent;
	dfs->dfs_is_nol_ie_sent = is_nol_ie_sent;
}

static void dfs_reset_nol_ie_bitmap(struct wlan_dfs *dfs)
{
	dfs->dfs_nol_ie_bitmap = 0;
}

/**
 * dfs_create_range_from_nol_bit() - API to convert the NOL IE bitmap to NOL
 * range.
 * @dfs: Pointer to wlan_dfs (has the NOL IE contents stored for sharing).
 * @is_radar_on_precac_chan: Boolean to indicate if NOL IE is on preCAC channel.
 *
 * Return: Range of the NOL received in NOL IE.
 */
static struct dfs_freq_range
dfs_create_range_from_nol_bit(struct wlan_dfs *dfs,
			      bool *is_radar_on_precac_chan)
{
	qdf_freq_t cur_freq = dfs->dfs_nol_ie_startfreq;
	qdf_freq_t start_freq = 0, end_freq = 0;
	uint8_t bw = dfs->dfs_nol_ie_bandwidth;
	struct dfs_freq_range nol_range;
	uint8_t i;
	uint8_t bits = 0x01;
	uint8_t agile_bw = wlan_reg_get_bw_value(dfs->dfs_precac_chwidth);

	for (i = 0; i < NUM_CHANNELS_160MHZ; i++) {
		/*
		 * For 5/10/20MHz BW channels only one subchannel will be
		 * present. For 40/80/160MHz channels more than one
		 * subchannels can be present.
		 */
		if (dfs->dfs_nol_ie_bitmap & bits) {
			if (!start_freq)
				start_freq = cur_freq - bw / 2;
			if (IS_WITHIN_RANGE(cur_freq,
					    dfs->dfs_agile_precac_freq_mhz,
					    (agile_bw / 2)))
				*is_radar_on_precac_chan = true;
		} else {
			if (start_freq && !end_freq)
				end_freq = cur_freq - bw / 2;
		}
		bits <<= 1;
		cur_freq += bw;
	}
	if (start_freq && !end_freq)
		end_freq = cur_freq - bw / 2;

	dfs_debug(dfs, WLAN_DEBUG_DFS_NOL,
		  "Freq range received from RCSA is: start %d end %d",
		  start_freq, end_freq);

	nol_range.start_freq = start_freq;
	nol_range.end_freq = end_freq;
	return nol_range;
}

static inline bool dfs_is_range_invalid(struct dfs_freq_range freq_range)
{
	return !freq_range.start_freq || !freq_range.end_freq ||
		(freq_range.start_freq > freq_range.end_freq);
}

#ifdef CONFIG_CHAN_FREQ_API
bool dfs_process_nol_ie_bitmap(struct wlan_dfs *dfs, uint8_t nol_ie_bandwidth,
			       uint16_t nol_ie_startfreq, uint8_t nol_ie_bitmap)
{
	uint16_t nol_freq_list[NUM_CHANNELS_160MHZ];
	bool should_nol_ie_be_sent = true;
	bool is_radar_on_precac_chan = false;
	uint8_t num_subchans;

	if (!dfs->dfs_use_nol_subchannel_marking) {
		uint16_t radar_subchans[NUM_CHANNELS_160MHZ];

		/* Since subchannel marking is disabled, disregard
		 * NOL IE and set NOL IE flag as false, so it
		 * can't be sent to uplink.
		 */
		qdf_mem_zero(radar_subchans, sizeof(radar_subchans));
		num_subchans =
		    dfs_get_bonding_channels_for_freq(dfs,
						      dfs->dfs_curchan,
						      SEG_ID_PRIMARY,
						      DETECTOR_ID_0,
						      radar_subchans);
		should_nol_ie_be_sent = false;
		dfs_radar_add_channel_list_to_nol_for_freq(dfs, radar_subchans,
							   nol_freq_list,
							   &num_subchans);
	} else {
		struct dfs_freq_range nol_range;

		/* Add the NOL IE information in DFS structure so that RCSA
		 * and NOL IE can be sent to uplink if uplink exists.
		 */
		dfs->dfs_nol_ie_bandwidth = nol_ie_bandwidth;
		dfs->dfs_nol_ie_startfreq = nol_ie_startfreq;
		dfs->dfs_nol_ie_bitmap = nol_ie_bitmap;

		nol_range = dfs_create_range_from_nol_bit(
				dfs, &is_radar_on_precac_chan);

		if (dfs_is_range_invalid(nol_range)) {
			dfs_err(dfs, WLAN_DEBUG_DFS_ALWAYS,
				"Invalid NOL range from NOL IE!");
			return false;
		}

		dfs_radar_add_channel_range_to_nol(dfs, nol_range);

		num_subchans =
			dfs_convert_rangelist_2_reg_freqlist(dfs, &nol_range,
							     1, nol_freq_list);
		dfs_mark_precac_nol_for_freq(dfs, 0, 0,
					     nol_freq_list,
					     num_subchans, &nol_range, 1);
		if (is_radar_on_precac_chan)
			utils_dfs_agile_sm_deliver_evt(
				dfs->dfs_pdev_obj, DFS_AGILE_SM_EV_ADFS_RADAR);
	}

	return should_nol_ie_be_sent;
}
#endif
#endif /* QCA_DFS_RCSA_SUPPORT */

#if defined(QCA_DFS_RCSA_SUPPORT)
/**
 * dfs_send_nol_ie_and_rcsa()- Send NOL IE and RCSA action frames.
 * @dfs: Pointer to wlan_dfs structure.
 * @radar_found: Pointer to radar found structure.
 * @nol_freq_list: List of 20MHz frequencies on which radar has been detected.
 * @num_channels: number of radar affected channels.
 * @nol_range: Range of NOL affected frequencies.
 * @wait_for_csa: indicates if the repeater AP should take DFS action or wait
 * for CSA
 *
 * Return: void.
 */
void dfs_send_nol_ie_and_rcsa(struct wlan_dfs *dfs,
			      struct radar_found_info *radar_found,
			      uint16_t *nol_freq_list,
			      uint8_t num_channels,
			      struct dfs_freq_range nol_range,
			      bool *wait_for_csa)
{
	dfs->dfs_is_nol_ie_sent = false;
	(dfs->is_radar_during_precac ||
	 radar_found->detector_id == dfs_get_agile_detector_id(dfs)) ?
		(dfs->dfs_is_rcsa_ie_sent = false) :
		(dfs->dfs_is_rcsa_ie_sent = true);
	if (dfs->dfs_use_nol_subchannel_marking) {
		dfs_reset_nol_ie_bitmap(dfs);
		dfs_prepare_nol_ie_bitmap_for_freq(dfs, radar_found,
						   nol_freq_list,
						   num_channels,
						   nol_range);
		dfs->dfs_is_nol_ie_sent = true;
	}

	/*
	 * This calls into the umac DFS code, which sets the umac
	 * related radar flags and begins the channel change
	 * machinery.

	 * Even during precac, this API is called, but with a flag
	 * saying not to send RCSA, but only the radar affected subchannel
	 * information.
	 */
	dfs_mlme_start_rcsa(dfs->dfs_pdev_obj, wait_for_csa);
}
#endif /* QCA_DFS_RCSA_SUPPORT */

