//==============================================================================
// Copyright (C) 2023 Macnica Inc. All Rights Reserved.
//
// Use in source and binary forms, with or without modification, are permitted
// provided by agreeing to the following terms and conditions:
//
// REDISTRIBUTIONS OR SUBLICENSING IN SOURCE AND BINARY FORM ARE NOT ALLOWED.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//------------------------------------------------------------------------------
//! @file
//! @brief
//==============================================================================
#ifndef __GST_M2SVIDEOSRC_H__
#define __GST_M2SVIDEOSRC_H__

G_BEGIN_DECLS

#define GST_TYPE_M2SVIDEOSRC (gst_m2svideosrc_get_type())
G_DECLARE_FINAL_TYPE (GstM2svideosrc, gst_m2svideosrc, GST, M2SVIDEOSRC,
                      GstPushSrc)

/**
 * GstM2svideosrc:
 *
 * Opaque data structure.
 */
struct _GstM2svideosrc {
	GstPushSrc element;

	/*< private >*/

	/* video state */
	GstVideoInfo info; /* protected by the object or stream lock */
	GstVideoChromaResample *subsample;

	/* private */
	/* FIXME 2.0: Change type to GstClockTime */
	gint64 timestamp_offset;              /* base offset */

	std::thread *p_mon_thread;
	std::mutex mon_lock;
	std::condition_variable mon_cond;
	bool mon_running;

	m2s_strm_id_t strm_id;
	bool fifo_is_almost_empty;
	bool hw_hitless;
	uint8_t gpu_num;
	int32_t l2_cpu_num;
	int32_t l1_cpu_num;
	char if_ip[2][32];
	char dst_ip[2][32];
	char src_ip[2][32];
	uint16_t dst_port[2];
	uint16_t src_port[2];
	uint8_t payload_type;
	int32_t playout_delay_ms;
	uint16_t debug_message_interval;
	uint8_t scan;
	m2s_video_rtp_format_t rtp_format;
	char ipx_license[128];
	bool box_mode;
	uint8_t box_size;
	bool async_rtp_timestamp;
	uint8_t *p_frame_from_m2s;
	uint32_t frame_size_from_m2s;
	uint8_t fifo_under_threshold;
	uint8_t fifo_middle;
	uint8_t fifo_over_threshold;
	uint8_t under_count;
	uint8_t under_count_max;
	bool gpudirect;

	/* running time and frames for current caps */
	GstClockTime running_time;            /* total running time */
	gint64 n_frames;                      /* total frames sent */
	gboolean reverse;

	/* previous caps running time and frames */
	GstClockTime accum_rtime;              /* accumulated running_time */
	gint64 accum_frames;                  /* accumulated frames */

	guint n_lines;
	gint offset;
	gpointer *lines;
};

G_END_DECLS

#endif /* __GST_M2SVIDEOSRC_H__ */
