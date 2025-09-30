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
#ifndef __GST_M2SAUDIOSRC_H__
#define __GST_M2SAUDIOSRC_H__

#include <gst/gst.h>
#include <gst/base/gstbasesrc.h>
#include <gst/audio/audio.h>

G_BEGIN_DECLS

#define GST_TYPE_M2SAUDIOSRC (gst_m2saudiosrc_get_type())
G_DECLARE_FINAL_TYPE (GstM2saudiosrc, gst_m2saudiosrc, GST, M2SAUDIOSRC,
    GstBaseSrc)

typedef void (*ProcessFunc) (GstM2saudiosrc*, guint8 *);

/**
 * GstM2saudiosrc:
 *
 * m2saudiosrc object structure.
 */
struct _GstM2saudiosrc {
	GstBaseSrc parent;

	ProcessFunc process;
	GstAudioFormatPack pack_func;
	gint pack_size;
	gpointer tmp;
	gsize tmpsize;

	/* parameters */
	gdouble freq;

	/* audio parameters */
	GstAudioInfo info;
	gint samples_per_buffer;

	/*< private >*/
	gboolean tags_pushed;			/* send tags just once ? */
	GstClockTimeDiff timestamp_offset;    /* base offset */
	GstClockTime next_time;               /* next timestamp */
	gint64 next_sample;                   /* next sample to send */
	gint64 next_byte;                     /* next byte to send */
	gint64 sample_stop;
	gboolean check_seek_stop;
	gboolean eos_reached;
	gint generate_samples_per_buffer;	/* used to generate a partial buffer */
	gboolean can_activate_pull;
	gboolean reverse;                  /* play backwards */

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
	std::string if_ip[2];
	std::string dst_ip[2];
	std::string src_ip[2];
	uint16_t dst_port[2];
	uint16_t src_port[2];
	uint8_t payload_type;
	int32_t playout_delay_ms;
	uint16_t debug_message_interval;
	uint8_t packet_time;

	std::vector<uint8_t> vec_raw;
	uint32_t raw_element_length;
};

G_END_DECLS

#endif /* __GST_M2SAUDIOSRC_H__ */
