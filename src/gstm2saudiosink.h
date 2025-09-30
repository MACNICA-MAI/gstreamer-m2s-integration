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
#ifndef _GST_M2SAUDIOSINK_H_
#define _GST_M2SAUDIOSINK_H_

#include <gst/base/gstbasesink.h>

G_BEGIN_DECLS

#define GST_TYPE_M2SAUDIOSINK   (gst_m2saudiosink_get_type())
#define GST_M2SAUDIOSINK(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_M2SAUDIOSINK,GstM2saudiosink))
#define GST_M2SAUDIOSINK_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_M2SAUDIOSINK,GstM2saudiosinkClass))
#define GST_IS_M2SAUDIOSINK(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_M2SAUDIOSINK))
#define GST_IS_M2SAUDIOSINK_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_M2SAUDIOSINK))

typedef struct _GstM2saudiosink GstM2saudiosink;
typedef struct _GstM2saudiosinkClass GstM2saudiosinkClass;

struct _GstM2saudiosink
{
	GstBaseSink base_m2saudiosink;
	GstAudioInfo info; /* protected by the object or stream lock */

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
	uint8_t  payload_type;
	uint16_t debug_message_interval;
	uint8_t packet_time;
	int32_t tx_delay_ms;

	bool done_first_set_contents;
	uint64_t start_time;
	uint64_t raw_offset;

	std::vector<uint8_t> vec_raw;
	uint32_t raw_element_length;
};

struct _GstM2saudiosinkClass
{
	GstBaseSinkClass base_m2saudiosink_class;
};

GType gst_m2saudiosink_get_type (void);

G_END_DECLS

#endif
