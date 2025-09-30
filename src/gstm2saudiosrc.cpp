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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <m2s_api.h>
#include "gstm2saudiosrc.h"

#define DBG_MSG(format, args...) printf("[m2saudiosrc] " format, ## args)

GST_DEBUG_CATEGORY_STATIC (m2saudiosrc_debug);
#define GST_CAT_DEFAULT m2saudiosrc_debug

#define DEFAULT_HW_HITLESS               (TRUE)
#define DEFAULT_GPU_NUM                  (0)
#define DEFAULT_L2_CPU_NUM               (-1)
#define DEFAULT_L1_CPU_NUM               (-1)
#define DEFAULT_SAMPLES_PER_BUFFER       1024
#define DEFAULT_FREQ                     440.0
#define DEFAULT_IS_LIVE                  FALSE
#define DEFAULT_TIMESTAMP_OFFSET         G_GINT64_CONSTANT (0)
#define DEFAULT_TIME_BETWEEN_TICKS       GST_SECOND
#define DEFAULT_CAN_ACTIVATE_PULL        FALSE

#define DEFAULT_P_DST_ADDRESS            "239.1.1.1"
#define DEFAULT_S_DST_ADDRESS            "0.0.0.0"
#define DEFAULT_P_IF_ADDRESS             "192.168.0.1"
#define DEFAULT_S_IF_ADDRESS             "0.0.0.0"
#define DEFAULT_P_SRC_ADDRESS            "192.168.1.1"
#define DEFAULT_S_SRC_ADDRESS            "0.0.0.0"
#define DEFAULT_P_DST_PORT               (50000)
#define DEFAULT_S_DST_PORT               (50001)
#define DEFAULT_P_SRC_PORT               (0)
#define DEFAULT_S_SRC_PORT               (0)
#define DEFAULT_PAYLOAD_TYPE             (97)
#define DEFAULT_PLAYOUT_DELAY_MS         (0)
#define DEFAULT_DEBUG_MESSAGE_INTERVAL   (10)
#define DEFAULT_PACKET_TIME              (1)

enum
{
	PROP_0,
	PROP_HW_HITLESS,
	PROP_GPU_NUM,
	PROP_L2_CPU_NUM,
	PROP_L1_CPU_NUM,
	PROP_P_DST_ADDRESS,
	PROP_S_DST_ADDRESS,
	PROP_P_IF_ADDRESS,
	PROP_S_IF_ADDRESS,
	PROP_P_SRC_ADDRESS,
	PROP_S_SRC_ADDRESS,
	PROP_P_DST_PORT,
	PROP_S_DST_PORT,
	PROP_P_SRC_PORT,
	PROP_S_SRC_PORT,
	PROP_PAYLOAD_TYPE,
	PROP_PLAYOUT_DELAY_MS,
	PROP_DEBUG_MESSAGE_INTERVAL,
	PROP_SAMPLES_PER_BUFFER,
	PROP_IS_LIVE,
	PROP_TIMESTAMP_OFFSET,
	PROP_PACKET_TIME,
};

#define DEFAULT_FORMAT_STR GST_AUDIO_NE ("S16")

static GstStaticPadTemplate gst_m2saudiosrc_src_template =
	GST_STATIC_PAD_TEMPLATE ("src",
	                         GST_PAD_SRC,
	                         GST_PAD_ALWAYS,
	                         GST_STATIC_CAPS ("audio/x-raw,format={S24BE},rate=[1,max],"
	                                          "channels=[1,max],layout={interleaved}")
	);

#define gst_m2saudiosrc_parent_class parent_class
G_DEFINE_TYPE (GstM2saudiosrc, gst_m2saudiosrc, GST_TYPE_BASE_SRC);


static void gst_m2saudiosrc_set_hw_hitless (GstM2saudiosrc *m2saudiosrc, bool hw_hitless);
static void gst_m2saudiosrc_set_gpu_num (GstM2saudiosrc *m2saudiosrc, uint8_t gpu_num);
static void gst_m2saudiosrc_set_l2_cpu_num (GstM2saudiosrc *m2saudiosrc, int32_t cpu_num);
static void gst_m2saudiosrc_set_l1_cpu_num (GstM2saudiosrc *m2saudiosrc, int32_t cpu_num);
static void gst_m2saudiosrc_set_p_dst_address (GstM2saudiosrc *m2saudiosrc, const char *p_address);
static void gst_m2saudiosrc_set_s_dst_address (GstM2saudiosrc *m2saudiosrc, const char *p_address);
static void gst_m2saudiosrc_set_p_if_address (GstM2saudiosrc *m2saudiosrc, const char *p_address);
static void gst_m2saudiosrc_set_s_if_address (GstM2saudiosrc *m2saudiosrc, const char *p_address);
static void gst_m2saudiosrc_set_p_src_address (GstM2saudiosrc *m2saudiosrc, const char *p_address);
static void gst_m2saudiosrc_set_s_src_address (GstM2saudiosrc *m2saudiosrc, const char *p_address);
static void gst_m2saudiosrc_set_p_dst_port (GstM2saudiosrc *m2saudiosrc, uint16_t port);
static void gst_m2saudiosrc_set_s_dst_port (GstM2saudiosrc *m2saudiosrc, uint16_t port);
static void gst_m2saudiosrc_set_p_src_port (GstM2saudiosrc *m2saudiosrc, uint16_t port);
static void gst_m2saudiosrc_set_s_src_port (GstM2saudiosrc *m2saudiosrc, uint16_t port);
static void gst_m2saudiosrc_set_payload_type (GstM2saudiosrc *m2saudiosrc, uint8_t payload_type);
static void gst_m2saudiosrc_set_playout_delay_ms (GstM2saudiosrc *m2saudiosrc, int32_t playout_delay_ms);
static void gst_m2saudiosrc_set_debug_message_interval (GstM2saudiosrc *m2saudiosrc, uint16_t interval);
static void gst_m2saudiosrc_set_packet_time (GstM2saudiosrc *m2saudiosrc, uint8_t packet_time);

static void gst_m2saudiosrc_finalize (GObject * object);

static void gst_m2saudiosrc_set_property (GObject * object,
                                          guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_m2saudiosrc_get_property (GObject * object,
                                          guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_m2saudiosrc_setcaps (GstBaseSrc * basesrc,
                                         GstCaps * caps);
static GstCaps *gst_m2saudiosrc_fixate (GstBaseSrc * bsrc, GstCaps * caps);

static gboolean gst_m2saudiosrc_is_seekable (GstBaseSrc * basesrc);
static gboolean gst_m2saudiosrc_do_seek (GstBaseSrc * basesrc,
                                         GstSegment * segment);
static gboolean gst_m2saudiosrc_query (GstBaseSrc * basesrc,
                                       GstQuery * query);

static void gst_m2saudiosrc_get_times (GstBaseSrc * basesrc,
                                       GstBuffer * buffer, GstClockTime * start, GstClockTime * end);
static gboolean gst_m2saudiosrc_start (GstBaseSrc * basesrc);
static gboolean gst_m2saudiosrc_stop (GstBaseSrc * basesrc);
static GstFlowReturn gst_m2saudiosrc_fill (GstBaseSrc * basesrc,
                                           guint64 offset, guint length, GstBuffer * buffer);

static GstStateChangeReturn gst_m2saudiosrc_change_state (GstElement * element, GstStateChange transition);

static void monitoring_thread_main(GstM2saudiosrc *p_m2saudiosrc)
{
	m2s_status_t status;
	std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now();
	std::unique_lock<std::mutex> lock(p_m2saudiosrc->mon_lock);

	while(1)
	{
		tp += std::chrono::seconds(p_m2saudiosrc->debug_message_interval);
		p_m2saudiosrc->mon_cond.wait_until(lock, tp);

		if (!p_m2saudiosrc->mon_running)
		{
			break;
		}

		m2s_get_status(p_m2saudiosrc->strm_id, &status, true);

		printf("[M2S_STATUS: RX_AUDIO(dst_ip[0]=%s)]\n"
			   " (Stream) active=%d/%d ditect=%u/%u lost=%u/%u reset=%u\n"
			   " (APP_FIFO) enqueue=%u dequeue=%u stored=%u\n"
			   " (RTP_FIFO) enqueue=%u dequeue=%u stored=%u\n"
			   " (Hitless_FIFO) enqueue=%u/%u dequeue=%u/%u stored=%u/%u\n"
			   " (Packet-7) rcv=%u lost=%u discontinuous=%u\n"
			   " (Packet) rcv=%u/%u lost=%u/%u discontinuous=%u/%u underflow=%u/%u overflow=%u/%u abnormal_seqnum=%u/%u async=%d\n"
			   " (Header) hdr_extension=%u padding=%u\n"
			   " (Debug) l2_cpu_load=%f l1_cpu_load=%f\n",
			   p_m2saudiosrc->dst_ip[0].c_str(),
			   status.rx.active[0],
			   status.rx.active[1],
			   status.rx.detect[0],
			   status.rx.detect[1],
			   status.rx.lost[0],
			   status.rx.lost[1],
			   status.rx.reset,
			   status.rx.app_fifo_enqueue,
			   status.rx.app_fifo_dequeue,
			   status.rx.app_fifo_stored,
			   status.rx.rtp_fifo_enqueue,
			   status.rx.rtp_fifo_dequeue,
			   status.rx.rtp_fifo_stored,
			   status.rx.hitless_fifo_enqueue[0],
			   status.rx.hitless_fifo_enqueue[1],
			   status.rx.hitless_fifo_dequeue[0],
			   status.rx.hitless_fifo_dequeue[1],
			   status.rx.hitless_fifo_stored[0],
			   status.rx.hitless_fifo_stored[1],
			   status.rx.packet_7_rcv,
			   status.rx.packet_7_lost,
			   status.rx.packet_7_discontinuous,
			   status.rx.packet_rcv[0],
			   status.rx.packet_rcv[1],
			   status.rx.packet_lost[0],
			   status.rx.packet_lost[1],
			   status.rx.packet_discontinuous[0],
			   status.rx.packet_discontinuous[1],
			   status.rx.packet_underflow[0],
			   status.rx.packet_underflow[1],
			   status.rx.packet_overflow[0],
			   status.rx.packet_overflow[1],
			   status.rx.abnormal_seqnum[0],
			   status.rx.abnormal_seqnum[1],
			   status.rx.async_stream,
			   status.rx.hdr_extension,
			   status.rx.padding,
			   status.rx.l2_cpu_load,
			   status.rx.l1_cpu_load);
		printf("\n");
	}
}

static void start_monitoring_timer(GstM2saudiosrc *p_m2saudiosrc)
{
	p_m2saudiosrc->mon_running = true;
	p_m2saudiosrc->p_mon_thread = new std::thread(&monitoring_thread_main, p_m2saudiosrc);
}

static void stop_monitoring_timer(GstM2saudiosrc *p_m2saudiosrc)
{
	{
		std::unique_lock<std::mutex> lock(p_m2saudiosrc->mon_lock);
		p_m2saudiosrc->mon_running = false;
		p_m2saudiosrc->mon_cond.notify_all();
	}
	p_m2saudiosrc->p_mon_thread->join();
	delete p_m2saudiosrc->p_mon_thread;
}

static void gst_m2saudiosrc_set_hw_hitless (GstM2saudiosrc *m2saudiosrc, bool hw_hitless)
{
	m2saudiosrc->hw_hitless = hw_hitless;
}

static void gst_m2saudiosrc_set_gpu_num (GstM2saudiosrc *m2saudiosrc, uint8_t gpu_num)
{
	m2saudiosrc->gpu_num = gpu_num;
}

static void gst_m2saudiosrc_set_l2_cpu_num (GstM2saudiosrc *m2saudiosrc, int32_t cpu_num)
{
	m2saudiosrc->l2_cpu_num = cpu_num;
}

static void gst_m2saudiosrc_set_l1_cpu_num (GstM2saudiosrc *m2saudiosrc, int32_t cpu_num)
{
	m2saudiosrc->l1_cpu_num = cpu_num;
}

static void gst_m2saudiosrc_set_p_dst_address (GstM2saudiosrc *p_m2saudiosrc, const char *p_address)
{
	p_m2saudiosrc->dst_ip[0] = p_address;
}

static void gst_m2saudiosrc_set_s_dst_address (GstM2saudiosrc *p_m2saudiosrc, const char *p_address)
{
	p_m2saudiosrc->dst_ip[1] = p_address;
}

static void gst_m2saudiosrc_set_p_if_address (GstM2saudiosrc *p_m2saudiosrc, const char *p_address)
{
	p_m2saudiosrc->if_ip[0] = p_address;
}

static void gst_m2saudiosrc_set_s_if_address (GstM2saudiosrc *p_m2saudiosrc, const char *p_address)
{
	p_m2saudiosrc->if_ip[1] = p_address;
}

static void gst_m2saudiosrc_set_p_src_address (GstM2saudiosrc *p_m2saudiosrc, const char *p_address)
{
	p_m2saudiosrc->src_ip[0] = p_address;
}

static void gst_m2saudiosrc_set_s_src_address (GstM2saudiosrc *p_m2saudiosrc, const char *p_address)
{
	p_m2saudiosrc->src_ip[1] = p_address;
}

static void gst_m2saudiosrc_set_p_dst_port (GstM2saudiosrc *p_m2saudiosrc, uint16_t port)
{
	p_m2saudiosrc->dst_port[0] = port;
}

static void gst_m2saudiosrc_set_s_dst_port (GstM2saudiosrc *p_m2saudiosrc, uint16_t port)
{
	p_m2saudiosrc->dst_port[1] = port;
}

static void gst_m2saudiosrc_set_p_src_port (GstM2saudiosrc *p_m2saudiosrc, uint16_t port)
{
	p_m2saudiosrc->src_port[0] = port;
}

static void gst_m2saudiosrc_set_s_src_port (GstM2saudiosrc *p_m2saudiosrc, uint16_t port)
{
	p_m2saudiosrc->src_port[1] = port;
}

static void gst_m2saudiosrc_set_payload_type (GstM2saudiosrc *m2saudiosrc, uint8_t payload_type)
{
	m2saudiosrc->payload_type = payload_type;
}

static void gst_m2saudiosrc_set_playout_delay_ms (GstM2saudiosrc *p_m2saudiosrc, int32_t playout_delay_ms)
{
	p_m2saudiosrc->playout_delay_ms = playout_delay_ms;
}

static void gst_m2saudiosrc_set_debug_message_interval (GstM2saudiosrc *m2saudiosrc, uint16_t interval)
{
	std::unique_lock<std::mutex> lock(m2saudiosrc->mon_lock);
	m2saudiosrc->debug_message_interval = interval;
}

static void gst_m2saudiosrc_set_packet_time (GstM2saudiosrc *m2saudiosrc, uint8_t packet_time)
{
	m2saudiosrc->packet_time = packet_time;
}

static void
gst_m2saudiosrc_class_init (GstM2saudiosrcClass * klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;
	GstBaseSrcClass *gstbasesrc_class;

	gobject_class = (GObjectClass *) klass;
	gstelement_class = (GstElementClass *) klass;
	gstbasesrc_class = (GstBaseSrcClass *) klass;

	gobject_class->set_property = gst_m2saudiosrc_set_property;
	gobject_class->get_property = gst_m2saudiosrc_get_property;
	gobject_class->finalize = gst_m2saudiosrc_finalize;

	g_object_class_install_property (gobject_class, PROP_HW_HITLESS,
	                                 g_param_spec_boolean ("hw-hitless", "HW Hitless ",
	                                                       "HW Hitless", DEFAULT_HW_HITLESS,
	                                                       (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_GPU_NUM,
	                                 g_param_spec_uint ("gpu-num", "GPU Number",
	                                                    "GPU Number", 0, 255, DEFAULT_GPU_NUM,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_L2_CPU_NUM,
	                                 g_param_spec_int ("l2-cpu-num", "L2 CPU Number",
	                                                   "L2 CPU Number", -1, 1000, DEFAULT_L2_CPU_NUM,
	                                                   (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_L1_CPU_NUM,
	                                 g_param_spec_int ("l1-cpu-num", "L1 CPU Number",
	                                                   "L1 CPU Number", -1, 1000, DEFAULT_L1_CPU_NUM,
	                                                   (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_P_DST_ADDRESS,
	                                 g_param_spec_string ("p-dst-address", "Primary Destination Address",
	                                                      "Address to receive packets for. This is equivalent to the "
	                                                      "multicast-group property for now", DEFAULT_P_DST_ADDRESS,
	                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_S_DST_ADDRESS,
	                                 g_param_spec_string ("s-dst-address", "Secondary Destination Address",
	                                                      "Address to receive packets for. This is equivalent to the "
	                                                      "multicast-group property for now", DEFAULT_S_DST_ADDRESS,
	                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_P_IF_ADDRESS,
	                                 g_param_spec_string ("p-if-address", "Primary Interface Address",
	                                                      "Interface Address", DEFAULT_P_IF_ADDRESS,
	                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_S_IF_ADDRESS,
	                                 g_param_spec_string ("s-if-address", "SecondaryInterface Address",
	                                                      "Interface Address", DEFAULT_S_IF_ADDRESS,
	                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_P_SRC_ADDRESS,
	                                 g_param_spec_string ("p-src-address", "Primary Source Address",
	                                                      "Source Address", DEFAULT_P_SRC_ADDRESS,
	                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_S_SRC_ADDRESS,
	                                 g_param_spec_string ("s-src-address", "Secondary Source Address",
	                                                      "Source Address", DEFAULT_S_SRC_ADDRESS,
	                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_P_DST_PORT,
	                                 g_param_spec_uint ("p-dst-port", "Primary Destination Port",
	                                                    "Destination Port", 0, 65535, DEFAULT_P_DST_PORT,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_S_DST_PORT,
	                                 g_param_spec_uint ("s-dst-port", "Secondary Destination Port",
	                                                    "Destination Port", 0, 65535, DEFAULT_S_DST_PORT,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_P_SRC_PORT,
	                                 g_param_spec_uint ("p-src-port", "Primary Source Port",
	                                                    "Source Port", 0, 65535, DEFAULT_P_SRC_PORT,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_S_SRC_PORT,
	                                 g_param_spec_uint ("s-src-port", "Secondary Source Port",
	                                                    "Source Port", 0, 65535, DEFAULT_S_SRC_PORT,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_PAYLOAD_TYPE,
	                                 g_param_spec_uint ("payload-type", "Payload Type",
	                                                    "Payload Type", 0, 127, DEFAULT_PAYLOAD_TYPE,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_PLAYOUT_DELAY_MS,
	                                 g_param_spec_int  ("playout-delay-ms", "Playout delay milliseconds",
	                                                    "Playout delay ms", 0x80000000, 0x7fffffff, DEFAULT_PLAYOUT_DELAY_MS,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_DEBUG_MESSAGE_INTERVAL,
	                                 g_param_spec_uint ("debug-message-interval", "Debug message interval",
	                                                    "Debug message interval", 0, 65535, DEFAULT_DEBUG_MESSAGE_INTERVAL,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_SAMPLES_PER_BUFFER,
	                                 g_param_spec_int ("samplesperbuffer", "Samples per buffer",
	                                                   "Number of samples in each outgoing buffer",
	                                                   1, G_MAXINT, DEFAULT_SAMPLES_PER_BUFFER,
	                                                   (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
	g_object_class_install_property (gobject_class, PROP_IS_LIVE,
	                                 g_param_spec_boolean ("is-live", "Is Live",
	                                                       "Whether to act as a live source", DEFAULT_IS_LIVE,
	                                                       (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
	g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_TIMESTAMP_OFFSET,
	                                 g_param_spec_int64 ("timestamp-offset",
	                                                     "Timestamp offset",
	                                                     "An offset added to timestamps set on buffers (in ns)", G_MININT64,
	                                                     G_MAXINT64, DEFAULT_TIMESTAMP_OFFSET,
	                                                     (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_PACKET_TIME,
	                                 g_param_spec_uint ("packet-time", "Packet Time",
	                                                    "Packet Time 0:1ms, 1:125us", 0, 1, DEFAULT_PACKET_TIME,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	gst_element_class_add_static_pad_template (gstelement_class,
	                                           &gst_m2saudiosrc_src_template);

	gst_element_class_set_static_metadata (gstelement_class,
	                                       "FIXME Long name", "Generic", "FIXME Description",
	                                       "FIXME <fixme@example.com>");

	gstbasesrc_class->set_caps = GST_DEBUG_FUNCPTR (gst_m2saudiosrc_setcaps);
	gstbasesrc_class->fixate = GST_DEBUG_FUNCPTR (gst_m2saudiosrc_fixate);
	gstbasesrc_class->is_seekable =
		GST_DEBUG_FUNCPTR (gst_m2saudiosrc_is_seekable);
	gstbasesrc_class->do_seek = GST_DEBUG_FUNCPTR (gst_m2saudiosrc_do_seek);
	gstbasesrc_class->query = GST_DEBUG_FUNCPTR (gst_m2saudiosrc_query);
	gstbasesrc_class->get_times =
		GST_DEBUG_FUNCPTR (gst_m2saudiosrc_get_times);
	gstbasesrc_class->start = GST_DEBUG_FUNCPTR (gst_m2saudiosrc_start);
	gstbasesrc_class->stop = GST_DEBUG_FUNCPTR (gst_m2saudiosrc_stop);
	gstbasesrc_class->fill = GST_DEBUG_FUNCPTR (gst_m2saudiosrc_fill);

	gstelement_class->change_state = gst_m2saudiosrc_change_state;
}

static void
gst_m2saudiosrc_init (GstM2saudiosrc * p_m2saudiosrc)
{
	p_m2saudiosrc->freq = DEFAULT_FREQ;

	/* we operate in time */
	gst_base_src_set_format (GST_BASE_SRC (p_m2saudiosrc), GST_FORMAT_TIME);
	gst_base_src_set_live (GST_BASE_SRC (p_m2saudiosrc), DEFAULT_IS_LIVE);

	p_m2saudiosrc->samples_per_buffer = DEFAULT_SAMPLES_PER_BUFFER;
	p_m2saudiosrc->generate_samples_per_buffer = p_m2saudiosrc->samples_per_buffer;
	p_m2saudiosrc->timestamp_offset = DEFAULT_TIMESTAMP_OFFSET;
	p_m2saudiosrc->can_activate_pull = DEFAULT_CAN_ACTIVATE_PULL;

	p_m2saudiosrc->fifo_is_almost_empty = true;
	gst_m2saudiosrc_set_hw_hitless(p_m2saudiosrc, DEFAULT_HW_HITLESS);
	gst_m2saudiosrc_set_gpu_num(p_m2saudiosrc, DEFAULT_GPU_NUM);
	gst_m2saudiosrc_set_l2_cpu_num(p_m2saudiosrc, DEFAULT_L2_CPU_NUM);
	gst_m2saudiosrc_set_l1_cpu_num(p_m2saudiosrc, DEFAULT_L1_CPU_NUM);
	gst_m2saudiosrc_set_p_if_address(p_m2saudiosrc, DEFAULT_P_IF_ADDRESS);
	gst_m2saudiosrc_set_s_if_address(p_m2saudiosrc, DEFAULT_S_IF_ADDRESS);
	gst_m2saudiosrc_set_p_dst_address(p_m2saudiosrc, DEFAULT_P_DST_ADDRESS);
	gst_m2saudiosrc_set_s_dst_address(p_m2saudiosrc, DEFAULT_S_DST_ADDRESS);
	gst_m2saudiosrc_set_p_src_address(p_m2saudiosrc, DEFAULT_P_SRC_ADDRESS);
	gst_m2saudiosrc_set_s_src_address(p_m2saudiosrc, DEFAULT_S_SRC_ADDRESS);
	gst_m2saudiosrc_set_p_dst_port(p_m2saudiosrc, DEFAULT_P_DST_PORT);
	gst_m2saudiosrc_set_s_dst_port(p_m2saudiosrc, DEFAULT_S_DST_PORT);
	gst_m2saudiosrc_set_p_src_port(p_m2saudiosrc, DEFAULT_P_SRC_PORT);
	gst_m2saudiosrc_set_s_src_port(p_m2saudiosrc, DEFAULT_S_SRC_PORT);
	gst_m2saudiosrc_set_payload_type(p_m2saudiosrc, DEFAULT_PAYLOAD_TYPE);
	gst_m2saudiosrc_set_playout_delay_ms(p_m2saudiosrc, DEFAULT_PLAYOUT_DELAY_MS);
	gst_m2saudiosrc_set_debug_message_interval(p_m2saudiosrc, DEFAULT_DEBUG_MESSAGE_INTERVAL);
	gst_m2saudiosrc_set_packet_time(p_m2saudiosrc, DEFAULT_PACKET_TIME);

	gst_base_src_set_blocksize (GST_BASE_SRC (p_m2saudiosrc), -1);
}

static void
gst_m2saudiosrc_finalize (GObject * object)
{
	GstM2saudiosrc *src = GST_M2SAUDIOSRC (object);

	g_free (src->tmp);
	src->tmp = NULL;
	src->tmpsize = 0;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GstCaps *
gst_m2saudiosrc_fixate (GstBaseSrc * bsrc, GstCaps * caps)
{
	GstM2saudiosrc *src = GST_M2SAUDIOSRC (bsrc);
	GstStructure *structure;
	gint channels, rate;

	caps = gst_caps_make_writable (caps);

	structure = gst_caps_get_structure (caps, 0);

	GST_DEBUG_OBJECT (src, "fixating samplerate to %d", GST_AUDIO_DEF_RATE);

	rate = MAX (GST_AUDIO_DEF_RATE, src->freq * 2);
	gst_structure_fixate_field_nearest_int (structure, "rate", rate);

	gst_structure_fixate_field_string (structure, "format", DEFAULT_FORMAT_STR);

	gst_structure_fixate_field_string (structure, "layout", "interleaved");

	/* fixate to mono unless downstream requires stereo, for backwards compat */
	gst_structure_fixate_field_nearest_int (structure, "channels", 1);

	if (gst_structure_get_int (structure, "channels", &channels) && channels > 2) {
		if (!gst_structure_has_field_typed (structure, "channel-mask",
		                                    GST_TYPE_BITMASK))
			gst_structure_set (structure, "channel-mask", GST_TYPE_BITMASK, 0ULL,
			                   NULL);
	}

	caps = GST_BASE_SRC_CLASS (parent_class)->fixate (bsrc, caps);

	return caps;
}

static gboolean
gst_m2saudiosrc_setcaps (GstBaseSrc * basesrc, GstCaps * caps)
{
	GstM2saudiosrc *p_m2saudiosrc = GST_M2SAUDIOSRC (basesrc);
	GstAudioInfo info;

	if (!gst_audio_info_from_caps (&info, caps))
		goto invalid_caps;

	GST_DEBUG_OBJECT (p_m2saudiosrc, "negotiated to caps %" GST_PTR_FORMAT, caps);

	p_m2saudiosrc->info = info;

	gst_base_src_set_blocksize (basesrc,
	                            GST_AUDIO_INFO_BPF (&info) * p_m2saudiosrc->samples_per_buffer);

	m2s_media_conf_t media_conf;
	m2s_ip_conf_t ip_conf;
	memset(&media_conf, 0, sizeof(media_conf));
	memset(&ip_conf, 0, sizeof(ip_conf));

	for (int i = 0; i < 2; i++)
	{
		ip_conf.rx_only.if_ip[i] = m2s_conv_ip_address_from_string(p_m2saudiosrc->if_ip[i].c_str());
		ip_conf.dst_ip[i] = m2s_conv_ip_address_from_string(p_m2saudiosrc->dst_ip[i].c_str());
		ip_conf.src_ip[i] = m2s_conv_ip_address_from_string(p_m2saudiosrc->src_ip[i].c_str());
		ip_conf.dst_port[i] = p_m2saudiosrc->dst_port[i];
		ip_conf.src_port[i] = p_m2saudiosrc->src_port[i];
		ip_conf.payload_type[i] = p_m2saudiosrc->payload_type;
		ip_conf.rtp_enabled[i] = (ip_conf.rx_only.if_ip[i] == 0) ? false : true;
	}
	ip_conf.rx_only.playout_delay_ms = p_m2saudiosrc->playout_delay_ms;

	media_conf.audio.rtp_caps.sample_rate = M2S_AUDIO_SAMPLE_RATE_48KHz;
	media_conf.audio.rtp_caps.channels = GST_AUDIO_INFO_CHANNELS(&p_m2saudiosrc->info);
	media_conf.audio.rtp_caps.packet_time = (m2s_audio_packet_time_t)p_m2saudiosrc->packet_time;

	media_conf.audio.app_caps.sample_rate = M2S_AUDIO_SAMPLE_RATE_48KHz;
	media_conf.audio.app_caps.channels = media_conf.audio.rtp_caps.channels;
	media_conf.audio.app_caps.format = M2S_AUDIO_APP_FORMAT_S24BE;
	media_conf.audio.app_caps.layout = M2S_AUDIO_LAYOUT_INTERLEAVED;

	m2s_set_media_conf(p_m2saudiosrc->strm_id, &media_conf);
	m2s_set_ip_conf(p_m2saudiosrc->strm_id, &ip_conf);

	p_m2saudiosrc->raw_element_length = 2880 * media_conf.audio.app_caps.channels;
	DBG_MSG("channels=%u\n", media_conf.audio.app_caps.channels);
	DBG_MSG("raw_element_length=%u\n", p_m2saudiosrc->raw_element_length);

	return TRUE;

	/* ERROR */
 invalid_caps:
	{
		GST_ERROR_OBJECT (basesrc, "received invalid caps");
		return FALSE;
	}
}

static gboolean
gst_m2saudiosrc_query (GstBaseSrc * basesrc, GstQuery * query)
{
	GstM2saudiosrc *src = GST_M2SAUDIOSRC (basesrc);
	gboolean res = FALSE;

	switch (GST_QUERY_TYPE (query)) {
	case GST_QUERY_CONVERT:
	{
		GstFormat src_fmt, dest_fmt;
		gint64 src_val, dest_val;

		gst_query_parse_convert (query, &src_fmt, &src_val, &dest_fmt, &dest_val);

		if (!gst_audio_info_convert (&src->info, src_fmt, src_val, dest_fmt,
		                             &dest_val))
			goto error;

		gst_query_set_convert (query, src_fmt, src_val, dest_fmt, dest_val);
		res = TRUE;
		break;
	}
	case GST_QUERY_SCHEDULING:
	{
		/* if we can operate in pull mode */
		gst_query_set_scheduling (query, GST_SCHEDULING_FLAG_SEEKABLE, 1, -1, 0);
		gst_query_add_scheduling_mode (query, GST_PAD_MODE_PUSH);
		if (src->can_activate_pull)
			gst_query_add_scheduling_mode (query, GST_PAD_MODE_PULL);

		res = TRUE;
		break;
	}
	case GST_QUERY_LATENCY:
	{
		if (src->info.rate > 0) {
			GstClockTime latency;

			latency =
				gst_util_uint64_scale (src->generate_samples_per_buffer, GST_SECOND,
				                       src->info.rate);
			gst_query_set_latency (query,
			                       gst_base_src_is_live (GST_BASE_SRC_CAST (src)), latency,
			                       GST_CLOCK_TIME_NONE);
			GST_DEBUG_OBJECT (src, "Reporting latency of %" GST_TIME_FORMAT,
			                  GST_TIME_ARGS (latency));
			res = TRUE;
		}
		break;
	}
	default:
		res = GST_BASE_SRC_CLASS (parent_class)->query (basesrc, query);
		break;
	}

	return res;
	/* ERROR */
 error:
	{
		GST_DEBUG_OBJECT (src, "query failed");
		return FALSE;
	}
}

static void
gst_m2saudiosrc_get_times (GstBaseSrc * basesrc, GstBuffer * buffer,
                           GstClockTime * start, GstClockTime * end)
{
	/* for live sources, sync on the timestamp of the buffer */
	if (gst_base_src_is_live (basesrc)) {
		GstClockTime timestamp = GST_BUFFER_TIMESTAMP (buffer);

		if (GST_CLOCK_TIME_IS_VALID (timestamp)) {
			/* get duration to calculate end time */
			GstClockTime duration = GST_BUFFER_DURATION (buffer);

			if (GST_CLOCK_TIME_IS_VALID (duration)) {
				*end = timestamp + duration;
			}
			*start = timestamp;
		}
	} else {
		*start = -1;
		*end = -1;
	}
}

static gboolean
gst_m2saudiosrc_start (GstBaseSrc * basesrc)
{
	GstM2saudiosrc *src = GST_M2SAUDIOSRC (basesrc);

	src->next_sample = 0;
	src->next_byte = 0;
	src->next_time = 0;
	src->check_seek_stop = FALSE;
	src->eos_reached = FALSE;
	src->tags_pushed = FALSE;

	return TRUE;
}

static gboolean
gst_m2saudiosrc_stop (GstBaseSrc * basesrc)
{
	return TRUE;
}

/* seek to time, will be called when we operate in push mode. In pull mode we
 * get the requested byte offset. */
static gboolean
gst_m2saudiosrc_do_seek (GstBaseSrc * basesrc, GstSegment * segment)
{
	GstM2saudiosrc *src = GST_M2SAUDIOSRC (basesrc);
	GstClockTime time;
	gint samplerate, bpf;
	gint64 next_sample;

	GST_DEBUG_OBJECT (src, "seeking %" GST_SEGMENT_FORMAT, segment);

	time = segment->position;
	src->reverse = (segment->rate < 0.0);

	samplerate = GST_AUDIO_INFO_RATE (&src->info);
	bpf = GST_AUDIO_INFO_BPF (&src->info);

	/* now move to the time indicated, don't seek to the sample *after* the time */
	next_sample = gst_util_uint64_scale_int (time, samplerate, GST_SECOND);
	src->next_byte = next_sample * bpf;
	if (samplerate == 0)
		src->next_time = 0;
	else
		src->next_time =
			gst_util_uint64_scale_round (next_sample, GST_SECOND, samplerate);

	GST_DEBUG_OBJECT (src, "seeking next_sample=%" G_GINT64_FORMAT
	                  " next_time=%" GST_TIME_FORMAT, next_sample,
	                  GST_TIME_ARGS (src->next_time));

	g_assert (src->next_time <= time);

	src->next_sample = next_sample;

	if (segment->rate > 0 && GST_CLOCK_TIME_IS_VALID (segment->stop)) {
		time = segment->stop;
		src->sample_stop =
			gst_util_uint64_scale_round (time, samplerate, GST_SECOND);
		src->check_seek_stop = TRUE;
	} else if (segment->rate < 0) {
		time = segment->start;
		src->sample_stop =
			gst_util_uint64_scale_round (time, samplerate, GST_SECOND);
		src->check_seek_stop = TRUE;
	} else {
		src->check_seek_stop = FALSE;
	}
	src->eos_reached = FALSE;

	return TRUE;
}

static gboolean
gst_m2saudiosrc_is_seekable (GstBaseSrc * basesrc)
{
	/* we're seekable... */
	return FALSE;
}

static GstFlowReturn
gst_m2saudiosrc_fill (GstBaseSrc * basesrc, guint64 offset,
                      guint length, GstBuffer * buffer)
{
	GstM2saudiosrc *src;
	GstClockTime next_time;
	gint64 next_sample, next_byte;
	gint bytes, samples;
	GstElementClass *eclass;
	GstMapInfo map;
	gint samplerate, bpf;
	m2s_media_t media;
	m2s_media_size_t size;
	m2s_media_size_t size_max;
	uint32_t rtp_timestamp;

	src = GST_M2SAUDIOSRC (basesrc);

	/* example for tagging generated data */
	if (!src->tags_pushed) {
		GstTagList *taglist;

		taglist = gst_tag_list_new (GST_TAG_DESCRIPTION, "audiotest wave", NULL);

		eclass = GST_ELEMENT_CLASS (parent_class);
		if (eclass->send_event)
			eclass->send_event (GST_ELEMENT_CAST (basesrc),
			                    gst_event_new_tag (taglist));
		else
			gst_tag_list_unref (taglist);
		src->tags_pushed = TRUE;
	}

	if (src->eos_reached) {
		GST_INFO_OBJECT (src, "eos");
		return GST_FLOW_EOS;
	}

	samplerate = GST_AUDIO_INFO_RATE (&src->info);
	bpf = GST_AUDIO_INFO_BPF (&src->info);

	/* if no length was given, use our default length in samples otherwise convert
	 * the length in bytes to samples. */
	if (length == (guint)-1)
		samples = src->samples_per_buffer;
	else
		samples = length / bpf;

	/* if no offset was given, use our next logical byte */
	if (offset == (guint64)-1)
		offset = src->next_byte;

	/* now see if we are at the byteoffset we think we are */
	if ((gint64)offset != src->next_byte) {
	  GST_DEBUG_OBJECT (src, "seek to new offset %" G_GUINT64_FORMAT, offset);
	  /* we have a discont in the expected sample offset, do a 'seek' */
	  src->next_sample = offset / bpf;
	  src->next_time =
		  gst_util_uint64_scale_int (src->next_sample, GST_SECOND, samplerate);
	  src->next_byte = offset;
	}

	/* check for eos */
	if (src->check_seek_stop && !src->reverse &&
	    (src->sample_stop > src->next_sample) &&
	    (src->sample_stop < src->next_sample + samples)
		) {
		/* calculate only partial buffer */
		src->generate_samples_per_buffer = src->sample_stop - src->next_sample;
		next_sample = src->sample_stop;
		src->eos_reached = TRUE;
	} else if (src->check_seek_stop && src->reverse &&
	           (src->sample_stop >= (src->next_sample - samples))
			   ) {
		/* calculate only partial buffer */
		src->generate_samples_per_buffer = src->next_sample - src->sample_stop;
		next_sample = src->sample_stop;
		src->eos_reached = TRUE;
	} else {
		/* calculate full buffer */
		src->generate_samples_per_buffer = samples;
		next_sample = src->next_sample + (src->reverse ? (-samples) : samples);
	}

	bytes = src->generate_samples_per_buffer * bpf;

	next_byte = src->next_byte + (src->reverse ? (-bytes) : bytes);
	next_time = gst_util_uint64_scale_int (next_sample, GST_SECOND, samplerate);

	GST_LOG_OBJECT (src, "samplerate %d", samplerate);
	GST_LOG_OBJECT (src, "next_sample %" G_GINT64_FORMAT ", ts %" GST_TIME_FORMAT,
	                next_sample, GST_TIME_ARGS (next_time));

	gst_buffer_set_size (buffer, bytes);

	GST_BUFFER_OFFSET (buffer) = src->next_sample;
	GST_BUFFER_OFFSET_END (buffer) = next_sample;
	if (!src->reverse) {
		GST_BUFFER_TIMESTAMP (buffer) = src->timestamp_offset + src->next_time;
		GST_BUFFER_DURATION (buffer) = next_time - src->next_time;
	} else {
		GST_BUFFER_TIMESTAMP (buffer) = src->timestamp_offset + next_time;
		GST_BUFFER_DURATION (buffer) = src->next_time - next_time;
	}

	gst_object_sync_values (GST_OBJECT (src), GST_BUFFER_TIMESTAMP (buffer));

	src->next_time = next_time;
	src->next_sample = next_sample;
	src->next_byte = next_byte;

	GST_LOG_OBJECT (src, "generating %u samples at ts %" GST_TIME_FORMAT,
	                src->generate_samples_per_buffer,
	                GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buffer)));

	gst_buffer_map (buffer, &map, GST_MAP_WRITE);
	if (src->pack_func) {
		gsize tmpsize;

		tmpsize =
			src->generate_samples_per_buffer * GST_AUDIO_INFO_CHANNELS (&src->info)
			* src->pack_size;

		if (tmpsize > src->tmpsize) {
			src->tmp = g_realloc (src->tmp, tmpsize);
			src->tmpsize = tmpsize;
		}
		src->process (src, (guint8 *)src->tmp);
		// src->pack_func (src->info.finfo, (GstAudioPackFlags)0, src->tmp, map.data,
		//     src->generate_samples_per_buffer *
		//     GST_AUDIO_INFO_CHANNELS (&src->info));
	} else {
		// src->process (src, map.data);
	}

	auto &vec_raw = src->vec_raw;
	while (vec_raw.size() < map.size)
	{
		m2s_status_t status;
		if (m2s_get_status(src->strm_id, &status, false) == M2S_RET_SUCCESS)
		{
			if (src->fifo_is_almost_empty)
			{
				if (status.rx.app_fifo_stored >= 5)
				{
					src->fifo_is_almost_empty = false;
				}
			}
			else
			{
				if (status.rx.app_fifo_stored < 2)
				{
					src->fifo_is_almost_empty = true;
				}
			}
		}
		else
		{
			src->fifo_is_almost_empty = true;
		}

		if (src->fifo_is_almost_empty)
		{
			break;
		}
		else
		{
			size_max.audio.raw_size = src->raw_element_length;
			vec_raw.resize(vec_raw.size() + src->raw_element_length);
			media.audio.p_raw = &vec_raw[vec_raw.size() - src->raw_element_length];
			m2s_read(src->strm_id, &rtp_timestamp, &media, &size, &size_max);
		}
	}

	if (!src->fifo_is_almost_empty && (vec_raw.size() >= map.size))
	{
		memcpy(map.data, &vec_raw[0], map.size);
		vec_raw.erase(vec_raw.begin(), vec_raw.begin() + map.size);
	}
	else
	{
		memset(map.data, 0, map.size);
	}

	gst_buffer_unmap (buffer, &map);

	if (GST_AUDIO_INFO_LAYOUT (&src->info) == GST_AUDIO_LAYOUT_NON_INTERLEAVED) {
		// gst_buffer_add_audio_meta (buffer, &src->info,
		//     src->generate_samples_per_buffer, NULL);
	}

	return GST_FLOW_OK;
}

static void
gst_m2saudiosrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
	GstM2saudiosrc *p_m2saudiosrc = GST_M2SAUDIOSRC (object);

	switch (prop_id) {

	case PROP_HW_HITLESS:
		gst_m2saudiosrc_set_hw_hitless (p_m2saudiosrc, g_value_get_boolean (value));
		break;
	case PROP_GPU_NUM:
		gst_m2saudiosrc_set_gpu_num (p_m2saudiosrc, g_value_get_uint (value));
		break;
	case PROP_L2_CPU_NUM:
		gst_m2saudiosrc_set_l2_cpu_num (p_m2saudiosrc, g_value_get_int (value));
		break;
	case PROP_L1_CPU_NUM:
		gst_m2saudiosrc_set_l1_cpu_num (p_m2saudiosrc, g_value_get_int (value));
		break;
	case PROP_P_DST_ADDRESS:
		gst_m2saudiosrc_set_p_dst_address (p_m2saudiosrc, g_value_get_string (value));
		break;
	case PROP_S_DST_ADDRESS:
		gst_m2saudiosrc_set_s_dst_address (p_m2saudiosrc, g_value_get_string (value));
		break;
	case PROP_P_IF_ADDRESS:
		gst_m2saudiosrc_set_p_if_address (p_m2saudiosrc, g_value_get_string (value));
		break;
	case PROP_S_IF_ADDRESS:
		gst_m2saudiosrc_set_s_if_address (p_m2saudiosrc, g_value_get_string (value));
		break;
	case PROP_P_SRC_ADDRESS:
		gst_m2saudiosrc_set_p_src_address (p_m2saudiosrc, g_value_get_string (value));
		break;
	case PROP_S_SRC_ADDRESS:
		gst_m2saudiosrc_set_s_src_address (p_m2saudiosrc, g_value_get_string (value));
		break;
	case PROP_P_DST_PORT:
		gst_m2saudiosrc_set_p_dst_port (p_m2saudiosrc, g_value_get_uint (value));
		break;
	case PROP_S_DST_PORT:
		gst_m2saudiosrc_set_s_dst_port (p_m2saudiosrc, g_value_get_uint (value));
		break;
	case PROP_P_SRC_PORT:
		gst_m2saudiosrc_set_p_src_port (p_m2saudiosrc, g_value_get_uint (value));
		break;
	case PROP_S_SRC_PORT:
		gst_m2saudiosrc_set_s_src_port (p_m2saudiosrc, g_value_get_uint (value));
		break;
	case PROP_PAYLOAD_TYPE:
		gst_m2saudiosrc_set_payload_type (p_m2saudiosrc, g_value_get_uint (value));
		break;
	case PROP_PLAYOUT_DELAY_MS:
		gst_m2saudiosrc_set_playout_delay_ms (p_m2saudiosrc, g_value_get_int (value));
		break;
	case PROP_DEBUG_MESSAGE_INTERVAL:
		gst_m2saudiosrc_set_debug_message_interval (p_m2saudiosrc, g_value_get_uint (value));
		break;
	case PROP_PACKET_TIME:
		gst_m2saudiosrc_set_packet_time (p_m2saudiosrc, g_value_get_uint (value));
		break;

	case PROP_SAMPLES_PER_BUFFER:
		p_m2saudiosrc->samples_per_buffer = g_value_get_int (value);
		gst_base_src_set_blocksize (GST_BASE_SRC_CAST (p_m2saudiosrc),
		                            GST_AUDIO_INFO_BPF (&p_m2saudiosrc->info) * p_m2saudiosrc->samples_per_buffer);
		break;
	case PROP_IS_LIVE:
		gst_base_src_set_live (GST_BASE_SRC (p_m2saudiosrc), g_value_get_boolean (value));
		break;
	case PROP_TIMESTAMP_OFFSET:
		p_m2saudiosrc->timestamp_offset = g_value_get_int64 (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gst_m2saudiosrc_get_property (GObject * object, guint prop_id,
                              GValue * value, GParamSpec * pspec)
{
	GstM2saudiosrc *p_m2saudiosrc = GST_M2SAUDIOSRC (object);

	switch (prop_id) {

	case PROP_HW_HITLESS:
		g_value_set_boolean (value, p_m2saudiosrc->hw_hitless);
		break;
	case PROP_GPU_NUM:
		g_value_set_uint (value, p_m2saudiosrc->gpu_num);
		break;
	case PROP_L2_CPU_NUM:
		g_value_set_int (value, p_m2saudiosrc->l2_cpu_num);
		break;
	case PROP_L1_CPU_NUM:
		g_value_set_int (value, p_m2saudiosrc->l1_cpu_num);
		break;
	case PROP_P_DST_ADDRESS:
		g_value_set_string (value, p_m2saudiosrc->dst_ip[0].c_str());
		break;
	case PROP_S_DST_ADDRESS:
		g_value_set_string (value, p_m2saudiosrc->dst_ip[1].c_str());
		break;
	case PROP_P_IF_ADDRESS:
		g_value_set_string (value, p_m2saudiosrc->if_ip[0].c_str());
		break;
	case PROP_S_IF_ADDRESS:
		g_value_set_string (value, p_m2saudiosrc->if_ip[1].c_str());
		break;
	case PROP_P_SRC_ADDRESS:
		g_value_set_string (value, p_m2saudiosrc->src_ip[0].c_str());
		break;
	case PROP_S_SRC_ADDRESS:
		g_value_set_string (value, p_m2saudiosrc->src_ip[1].c_str());
		break;
	case PROP_P_DST_PORT:
		g_value_set_uint (value, p_m2saudiosrc->dst_port[0]);
		break;
	case PROP_S_DST_PORT:
		g_value_set_uint (value, p_m2saudiosrc->dst_port[1]);
		break;
	case PROP_P_SRC_PORT:
		g_value_set_uint (value, p_m2saudiosrc->src_port[0]);
		break;
	case PROP_S_SRC_PORT:
		g_value_set_uint (value, p_m2saudiosrc->src_port[1]);
		break;
	case PROP_PAYLOAD_TYPE:
		g_value_set_uint (value, p_m2saudiosrc->payload_type);
		break;
	case PROP_PLAYOUT_DELAY_MS:
		g_value_set_int (value, p_m2saudiosrc->playout_delay_ms);
		break;
	case PROP_DEBUG_MESSAGE_INTERVAL:
		g_value_set_uint (value, p_m2saudiosrc->debug_message_interval);
		break;
	case PROP_PACKET_TIME:
		g_value_set_uint (value, p_m2saudiosrc->packet_time);
		break;

	case PROP_SAMPLES_PER_BUFFER:
		g_value_set_int (value, p_m2saudiosrc->samples_per_buffer);
		break;
	case PROP_IS_LIVE:
		g_value_set_boolean (value, gst_base_src_is_live (GST_BASE_SRC (p_m2saudiosrc)));
		break;
	case PROP_TIMESTAMP_OFFSET:
		g_value_set_int64 (value, p_m2saudiosrc->timestamp_offset);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static GstStateChangeReturn
gst_m2saudiosrc_change_state (GstElement * element, GstStateChange transition)
{
	GstM2saudiosrc *p_m2saudiosrc = GST_M2SAUDIOSRC (element);
	GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

	switch (transition)
	{
	case GST_STATE_CHANGE_NULL_TO_READY:
		m2s_open_conf_t open_conf;
		open_conf.cuda_dev_num = p_m2saudiosrc->gpu_num;
		open_conf.p_ipx_license_file = nullptr;
		m2s_open(&open_conf);

		m2s_cpu_affinity_t cpu_affinity;
		cpu_affinity.rx.l2_num = p_m2saudiosrc->l2_cpu_num;
		cpu_affinity.rx.l1_num = p_m2saudiosrc->l1_cpu_num;
		m2s_create(&p_m2saudiosrc->strm_id, M2S_IO_TYPE_RX, M2S_MEDIA_TYPE_AUDIO, M2S_MEMORY_MODE_CPU, &cpu_affinity, NULL, p_m2saudiosrc->hw_hitless);

		break;

	case GST_STATE_CHANGE_READY_TO_PAUSED:
		break;

	case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
		m2s_start(p_m2saudiosrc->strm_id);
		start_monitoring_timer(p_m2saudiosrc);
		break;

	case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
		stop_monitoring_timer(p_m2saudiosrc);
		m2s_stop(p_m2saudiosrc->strm_id);
		break;

	case GST_STATE_CHANGE_PAUSED_TO_READY:
		break;

	case GST_STATE_CHANGE_READY_TO_NULL:
		m2s_delete(p_m2saudiosrc->strm_id);
		//m2s_close();
		break;

	default:
		break;
	}

	ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
	return ret;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
	gboolean ret = FALSE;
	ret |= gst_element_register (plugin, "m2saudiosrc", GST_RANK_NONE, GST_TYPE_M2SAUDIOSRC);
	return ret;
}

#ifndef VERSION
#define VERSION "2.12.1"
#endif
#ifndef PACKAGE
#define PACKAGE "FIXME_package"
#endif
#ifndef GST_PACKAGE_NAME
#define GST_PACKAGE_NAME "FIXME_package_name"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://FIXME.org/"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
                   GST_VERSION_MINOR,
                   m2saudiosrc,
                   "FIXME plugin description",
                   plugin_init, VERSION, GST_LICENSE_UNKNOWN, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
