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
#ifndef _GST_M2SVIDEOSINK_H_
#define _GST_M2SVIDEOSINK_H_

#include <gst/video/video.h>
#include <gst/video/gstvideosink.h>

G_BEGIN_DECLS

#define GST_TYPE_M2SVIDEOSINK   (gst_m2svideosink_get_type())
#define GST_M2SVIDEOSINK(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_M2SVIDEOSINK,GstM2svideosink))
#define GST_M2SVIDEOSINK_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_M2SVIDEOSINK,GstM2svideosinkClass))
#define GST_IS_M2SVIDEOSINK(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_M2SVIDEOSINK))
#define GST_IS_M2SVIDEOSINK_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_M2SVIDEOSINK))

typedef struct _GstM2svideosink GstM2svideosink;
typedef struct _GstM2svideosinkClass GstM2svideosinkClass;

struct _GstM2svideosink
{
	GstVideoSink base_m2svideosink;
	GstVideoInfo info; /* protected by the object or stream lock */

	std::thread *p_mon_thread;
	std::mutex mon_lock;
	std::condition_variable mon_cond;
	bool mon_running;

	m2s_strm_id_t strm_id;
	uint8_t gpu_num;
	int32_t cpu_num;
	std::string dst_ip[2];
	std::string src_ip[2];
	uint16_t dst_port[2];
	uint16_t src_port[2];
	uint8_t payload_type;
	uint16_t debug_message_interval;
	uint8_t scan;
	m2s_video_rtp_format_t rtp_format;
	float bpp;
	char ipx_license[128];
	bool box_mode;
	uint8_t box_size;
	bool gpudirect;
	int32_t tx_delay_ms;

	bool done_first_set_contents;
	uint64_t start_time;
	uint64_t frame_offset;
	m2s_frame_rate_t m2s_frame_rate;
};

struct _GstM2svideosinkClass
{
	GstVideoSinkClass base_m2svideosink_class;
};

GType gst_m2svideosink_get_type (void);

G_END_DECLS

#endif
