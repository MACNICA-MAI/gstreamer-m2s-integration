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
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <gst/gst.h>
#include <gst/base/gstbasesink.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiosink.h>
#include <m2s_api.h>
#include <tr_offset.h>
#include "gstm2saudiosink.h"

#define DBG_MSG(format, args...) printf("[m2saudiosink] " format, ## args)

GST_DEBUG_CATEGORY_STATIC (gst_m2saudiosink_debug_category);
#define GST_CAT_DEFAULT gst_m2saudiosink_debug_category

#define DEFAULT_GPU_NUM                  (0)
#define DEFAULT_CPU_NUM                  (-1)
#define DEFAULT_P_DST_ADDRESS            "239.1.1.1"
#define DEFAULT_S_DST_ADDRESS            "0.0.0.0"
#define DEFAULT_P_SRC_ADDRESS            "192.168.0.1"
#define DEFAULT_S_SRC_ADDRESS            "0.0.0.0"
#define DEFAULT_P_DST_PORT               (50000)
#define DEFAULT_S_DST_PORT               (50001)
#define DEFAULT_P_SRC_PORT               (0)
#define DEFAULT_S_SRC_PORT               (0)
#define DEFAULT_PAYLOAD_TYPE             (97)
#define DEFAULT_DEBUG_MESSAGE_INTERVAL   (10)
#define DEFAULT_PACKET_TIME              (1)
#define DEFAULT_TX_DELAY_MS              (200)

/* prototypes */

static void gst_m2saudiosink_set_gpu_num (GstM2saudiosink *m2saudiosink, uint8_t gpu_num);
static void gst_m2saudiosink_set_cpu_num (GstM2saudiosink *m2saudiosink, int32_t cpu_num);
static void gst_m2saudiosink_set_p_dst_address (GstM2saudiosink *m2saudiosink, const char *p_address);
static void gst_m2saudiosink_set_s_dst_address (GstM2saudiosink *m2saudiosink, const char *p_address);
static void gst_m2saudiosink_set_p_src_address (GstM2saudiosink *m2saudiosink, const char *p_address);
static void gst_m2saudiosink_set_s_src_address (GstM2saudiosink *m2saudiosink, const char *p_address);
static void gst_m2saudiosink_set_p_dst_port (GstM2saudiosink *m2saudiosink, uint16_t port);
static void gst_m2saudiosink_set_s_dst_port (GstM2saudiosink *m2saudiosink, uint16_t port);
static void gst_m2saudiosink_set_p_src_port (GstM2saudiosink *m2saudiosink, uint16_t port);
static void gst_m2saudiosink_set_s_src_port (GstM2saudiosink *m2saudiosink, uint16_t port);
static void gst_m2saudiosink_set_payload_type (GstM2saudiosink *m2saudiosink, uint8_t payload_type);
static void gst_m2saudiosink_set_debug_message_interval (GstM2saudiosink *m2saudiosink, uint16_t interval);
static void gst_m2saudiosink_set_packet_time (GstM2saudiosink *m2saudiosink, uint8_t packet_time);
static void gst_m2saudiosink_set_tx_delay_ms (GstM2saudiosink *m2saudiosink, int32_t tx_delay_ms);
static void gst_m2saudiosink_set_property (GObject * object,
                                           guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_m2saudiosink_get_property (GObject * object,
                                           guint property_id, GValue * value, GParamSpec * pspec);
static void gst_m2saudiosink_dispose (GObject * object);
static void gst_m2saudiosink_finalize (GObject * object);

static GstStateChangeReturn gst_m2saudiosink_change_state (GstElement * element, GstStateChange transition);

static gboolean gst_m2saudiosink_set_caps (GstBaseSink * sink, GstCaps * caps);
static GstCaps *gst_m2saudiosink_fixate (GstBaseSink * sink, GstCaps * caps);
static GstFlowReturn gst_m2saudiosink_render (GstBaseSink * sink,
                                              GstBuffer * buffer);

enum
{
	PROP_0,
	PROP_GPU_NUM,
	PROP_CPU_NUM,
	PROP_P_DST_ADDRESS,
	PROP_S_DST_ADDRESS,
	PROP_P_SRC_ADDRESS,
	PROP_S_SRC_ADDRESS,
	PROP_P_DST_PORT,
	PROP_S_DST_PORT,
	PROP_P_SRC_PORT,
	PROP_S_SRC_PORT,
	PROP_PAYLOAD_TYPE,
	PROP_DEBUG_MESSAGE_INTERVAL,
	PROP_PACKET_TIME,
	PROP_TX_DELAY_MS,
};

static int32_t g_start_time_offset_ns = 0;
/* pad templates */

static GstStaticPadTemplate gst_m2saudiosink_sink_template =
	GST_STATIC_PAD_TEMPLATE ("sink",
	                         GST_PAD_SINK,
	                         GST_PAD_ALWAYS,
	                         GST_STATIC_CAPS ("audio/x-raw,format={S24LE,U24LE,S24BE,U24BE},rate=[1,max],"
	                                          "channels=[1,max],layout={interleaved,non-interleaved}")
	);


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstM2saudiosink, gst_m2saudiosink, GST_TYPE_BASE_SINK,
                         GST_DEBUG_CATEGORY_INIT (gst_m2saudiosink_debug_category, "m2saudiosink", 0,
                                                  "debug category for m2saudiosink element"));

static void monitoring_thread_main(GstM2saudiosink *p_m2saudiosink)
{
	m2s_status_t status;
	std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now();
	std::unique_lock<std::mutex> lock(p_m2saudiosink->mon_lock);

	while(1)
	{
		tp += std::chrono::seconds(p_m2saudiosink->debug_message_interval);
		p_m2saudiosink->mon_cond.wait_until(lock, tp);

		if (!p_m2saudiosink->mon_running)
		{
			break;
		}

		m2s_get_status(p_m2saudiosink->strm_id, &status, true);

		printf("[M2S_STATUS: TX_AUDIO(dst_ip[0]=%s)]\n"
			   " (Stream) reset=%u\n"
			   " (APP_FIFO) enqueue=%u dequeue=%u stored=%u\n"
			   " (RTP_FIFO) enqueue=%u dequeue=%u stored=%u\n"
			   " (Packet) snd=%u zeroed=%u discontinuous=%u\n"
			   " (Debug) cpu_load=%f\n",
			   p_m2saudiosink->dst_ip[0].c_str(),
			   status.tx.reset,
			   status.tx.app_fifo_enqueue,
			   status.tx.app_fifo_dequeue,
			   status.tx.app_fifo_stored,
			   status.tx.rtp_fifo_enqueue,
			   status.tx.rtp_fifo_dequeue,
			   status.tx.rtp_fifo_stored,
			   status.tx.packet_snd,
			   status.tx.packet_zeroed_timeout,
			   status.tx.packet_discontinuous,
			   status.tx.cpu_load);
		printf("\n");
	}
}

static void start_monitoring_timer(GstM2saudiosink *p_m2saudiosink)
{
	p_m2saudiosink->mon_running = true;
	p_m2saudiosink->p_mon_thread = new std::thread(&monitoring_thread_main, p_m2saudiosink);
}

static void stop_monitoring_timer(GstM2saudiosink *p_m2saudiosink)
{
	{
		std::unique_lock<std::mutex> lock(p_m2saudiosink->mon_lock);
		p_m2saudiosink->mon_running = false;
		p_m2saudiosink->mon_cond.notify_all();
	}
	p_m2saudiosink->p_mon_thread->join();
	delete p_m2saudiosink->p_mon_thread;
}

static void gst_m2saudiosink_set_gpu_num (GstM2saudiosink *m2saudiosink, uint8_t gpu_num)
{
	m2saudiosink->gpu_num = gpu_num;
}

static void gst_m2saudiosink_set_cpu_num (GstM2saudiosink *m2saudiosink, int32_t cpu_num)
{
	m2saudiosink->cpu_num = cpu_num;
}

static void gst_m2saudiosink_set_p_dst_address (GstM2saudiosink *m2saudiosink, const char *p_address)
{
	m2saudiosink->dst_ip[0] = p_address;
}

static void gst_m2saudiosink_set_s_dst_address (GstM2saudiosink *m2saudiosink, const char *p_address)
{
	m2saudiosink->dst_ip[1] = p_address;
}

static void gst_m2saudiosink_set_p_src_address (GstM2saudiosink *m2saudiosink, const char *p_address)
{
	m2saudiosink->src_ip[0] = p_address;
}

static void gst_m2saudiosink_set_s_src_address (GstM2saudiosink *m2saudiosink, const char *p_address)
{
	m2saudiosink->src_ip[1] = p_address;
}

static void gst_m2saudiosink_set_p_dst_port (GstM2saudiosink *m2saudiosink, uint16_t port)
{
	m2saudiosink->dst_port[0] = port;
}

static void gst_m2saudiosink_set_s_dst_port (GstM2saudiosink *m2saudiosink, uint16_t port)
{
	m2saudiosink->dst_port[1] = port;
}

static void gst_m2saudiosink_set_p_src_port (GstM2saudiosink *m2saudiosink, uint16_t port)
{
	m2saudiosink->src_port[0] = port;
}

static void gst_m2saudiosink_set_s_src_port (GstM2saudiosink *m2saudiosink, uint16_t port)
{
	m2saudiosink->src_port[1] = port;
}

static void gst_m2saudiosink_set_payload_type (GstM2saudiosink *m2saudiosink, uint8_t payload_type)
{
	m2saudiosink->payload_type = payload_type;
}

static void gst_m2saudiosink_set_debug_message_interval (GstM2saudiosink *m2saudiosink, uint16_t interval)
{
	std::unique_lock<std::mutex> lock(m2saudiosink->mon_lock);
	m2saudiosink->debug_message_interval = interval;
}

static void gst_m2saudiosink_set_packet_time (GstM2saudiosink *m2saudiosink, uint8_t packet_time)
{
	m2saudiosink->packet_time = packet_time;
}

static void gst_m2saudiosink_set_tx_delay_ms (GstM2saudiosink *m2saudiosink, int32_t tx_delay_ms)
{
	m2saudiosink->tx_delay_ms = tx_delay_ms;
}

static void
gst_m2saudiosink_class_init (GstM2saudiosinkClass * klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GstBaseSinkClass *base_sink_class = GST_BASE_SINK_CLASS (klass);
	GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

	/* Setting up pads and setting metadata should be moved to
	   base_class_init if you intend to subclass this class. */
	gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
	                                           &gst_m2saudiosink_sink_template);

	gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
	                                       "FIXME Long name", "Generic", "FIXME Description",
	                                       "FIXME <fixme@example.com>");

	gobject_class->set_property = gst_m2saudiosink_set_property;
	gobject_class->get_property = gst_m2saudiosink_get_property;

	g_object_class_install_property (gobject_class, PROP_GPU_NUM,
	                                 g_param_spec_uint ("gpu-num", "GPU Number",
	                                                    "GPU Number", 0, 255, DEFAULT_GPU_NUM,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_CPU_NUM,
	                                 g_param_spec_int ("cpu-num", "CPU Number",
	                                                   "CPU Number", -1, 1000, DEFAULT_CPU_NUM,
	                                                   (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_P_DST_ADDRESS,
	                                 g_param_spec_string ("p-dst-address", "Primary Destination Address",
	                                                      "Address to send packets for. This is equivalent to the "
	                                                      "multicast-group property for now", DEFAULT_P_DST_ADDRESS,
	                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_S_DST_ADDRESS,
	                                 g_param_spec_string ("s-dst-address", "Secondary Destination Address",
	                                                      "Address to send packets for. This is equivalent to the "
	                                                      "multicast-group property for now", DEFAULT_S_DST_ADDRESS,
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

	g_object_class_install_property (gobject_class, PROP_DEBUG_MESSAGE_INTERVAL,
	                                 g_param_spec_uint ("debug-message-interval", "Debug message interval",
	                                                    "Debug message interval", 0, 65535, DEFAULT_DEBUG_MESSAGE_INTERVAL,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_PACKET_TIME,
	                                 g_param_spec_uint ("packet-time", "Packet Time",
	                                                    "Packet Time 0:1ms, 1:125us", 0, 1, DEFAULT_PACKET_TIME,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_TX_DELAY_MS,
	                                 g_param_spec_int  ("tx-delay-ms", "Tx delay milliseconds",
	                                                    "Tx delay ms", 0x80000000, 0x7fffffff, DEFAULT_TX_DELAY_MS,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	gobject_class->dispose = gst_m2saudiosink_dispose;
	gobject_class->finalize = gst_m2saudiosink_finalize;

	element_class->change_state = gst_m2saudiosink_change_state;

	base_sink_class->set_caps = GST_DEBUG_FUNCPTR (gst_m2saudiosink_set_caps);
	base_sink_class->fixate = GST_DEBUG_FUNCPTR (gst_m2saudiosink_fixate);
	base_sink_class->render = GST_DEBUG_FUNCPTR (gst_m2saudiosink_render);
}

static void
gst_m2saudiosink_init (GstM2saudiosink * p_m2saudiosink)
{
	gst_base_sink_set_sync (GST_BASE_SINK (p_m2saudiosink), FALSE);
	gst_m2saudiosink_set_gpu_num(p_m2saudiosink, DEFAULT_GPU_NUM);
	gst_m2saudiosink_set_cpu_num(p_m2saudiosink, DEFAULT_CPU_NUM);
	gst_m2saudiosink_set_p_dst_address(p_m2saudiosink, DEFAULT_P_DST_ADDRESS);
	gst_m2saudiosink_set_s_dst_address(p_m2saudiosink, DEFAULT_S_DST_ADDRESS);
	gst_m2saudiosink_set_p_src_address(p_m2saudiosink, DEFAULT_P_SRC_ADDRESS);
	gst_m2saudiosink_set_s_src_address(p_m2saudiosink, DEFAULT_S_SRC_ADDRESS);
	gst_m2saudiosink_set_p_dst_port(p_m2saudiosink, DEFAULT_P_DST_PORT);
	gst_m2saudiosink_set_s_dst_port(p_m2saudiosink, DEFAULT_S_DST_PORT);
	gst_m2saudiosink_set_p_src_port(p_m2saudiosink, DEFAULT_P_SRC_PORT);
	gst_m2saudiosink_set_s_src_port(p_m2saudiosink, DEFAULT_S_SRC_PORT);
	gst_m2saudiosink_set_payload_type(p_m2saudiosink, DEFAULT_PAYLOAD_TYPE);
	gst_m2saudiosink_set_debug_message_interval(p_m2saudiosink, DEFAULT_DEBUG_MESSAGE_INTERVAL);
	gst_m2saudiosink_set_packet_time(p_m2saudiosink, DEFAULT_PACKET_TIME);
	gst_m2saudiosink_set_tx_delay_ms(p_m2saudiosink, DEFAULT_TX_DELAY_MS);
}

void
gst_m2saudiosink_set_property (GObject * object, guint property_id,
                               const GValue * value, GParamSpec * pspec)
{
	GstM2saudiosink *p_m2saudiosink = GST_M2SAUDIOSINK (object);
	GST_DEBUG_OBJECT (p_m2saudiosink, "set_property");

	switch (property_id) {
	case PROP_GPU_NUM:
		gst_m2saudiosink_set_gpu_num (p_m2saudiosink, g_value_get_uint (value));
		break;
	case PROP_CPU_NUM:
		gst_m2saudiosink_set_cpu_num (p_m2saudiosink, g_value_get_int (value));
		break;
	case PROP_P_DST_ADDRESS:
		gst_m2saudiosink_set_p_dst_address (p_m2saudiosink, g_value_get_string (value));
		break;
	case PROP_S_DST_ADDRESS:
		gst_m2saudiosink_set_s_dst_address (p_m2saudiosink, g_value_get_string (value));
		break;
	case PROP_P_SRC_ADDRESS:
		gst_m2saudiosink_set_p_src_address (p_m2saudiosink, g_value_get_string (value));
		break;
	case PROP_S_SRC_ADDRESS:
		gst_m2saudiosink_set_s_src_address (p_m2saudiosink, g_value_get_string (value));
		break;
	case PROP_P_DST_PORT:
		gst_m2saudiosink_set_p_dst_port (p_m2saudiosink, g_value_get_uint (value));
		break;
	case PROP_S_DST_PORT:
		gst_m2saudiosink_set_s_dst_port (p_m2saudiosink, g_value_get_uint (value));
		break;
	case PROP_P_SRC_PORT:
		gst_m2saudiosink_set_p_src_port (p_m2saudiosink, g_value_get_uint (value));
		break;
	case PROP_S_SRC_PORT:
		gst_m2saudiosink_set_s_src_port (p_m2saudiosink, g_value_get_uint (value));
		break;
	case PROP_PAYLOAD_TYPE:
		gst_m2saudiosink_set_payload_type (p_m2saudiosink, g_value_get_uint (value));
		break;
	case PROP_DEBUG_MESSAGE_INTERVAL:
		gst_m2saudiosink_set_debug_message_interval (p_m2saudiosink, g_value_get_uint (value));
		break;
	case PROP_PACKET_TIME:
		gst_m2saudiosink_set_packet_time (p_m2saudiosink, g_value_get_uint (value));
		break;
	case PROP_TX_DELAY_MS:
		gst_m2saudiosink_set_tx_delay_ms (p_m2saudiosink, g_value_get_int (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

void
gst_m2saudiosink_get_property (GObject * object, guint property_id,
                               GValue * value, GParamSpec * pspec)
{
	GstM2saudiosink *p_m2saudiosink = GST_M2SAUDIOSINK (object);
	GST_DEBUG_OBJECT (p_m2saudiosink, "get_property");

	switch (property_id) {
	case PROP_GPU_NUM:
		g_value_set_uint (value, p_m2saudiosink->gpu_num);
		break;
	case PROP_CPU_NUM:
		g_value_set_int (value, p_m2saudiosink->cpu_num);
		break;
	case PROP_P_DST_ADDRESS:
		g_value_set_string (value, p_m2saudiosink->dst_ip[0].c_str());
		break;
	case PROP_S_DST_ADDRESS:
		g_value_set_string (value, p_m2saudiosink->dst_ip[1].c_str());
		break;
	case PROP_P_SRC_ADDRESS:
		g_value_set_string (value, p_m2saudiosink->src_ip[0].c_str());
		break;
	case PROP_S_SRC_ADDRESS:
		g_value_set_string (value, p_m2saudiosink->src_ip[1].c_str());
		break;
	case PROP_P_DST_PORT:
		g_value_set_uint (value, p_m2saudiosink->dst_port[0]);
		break;
	case PROP_S_DST_PORT:
		g_value_set_uint (value, p_m2saudiosink->dst_port[1]);
		break;
	case PROP_P_SRC_PORT:
		g_value_set_uint (value, p_m2saudiosink->src_port[0]);
		break;
	case PROP_S_SRC_PORT:
		g_value_set_uint (value, p_m2saudiosink->src_port[1]);
		break;
	case PROP_PAYLOAD_TYPE:
		g_value_set_uint (value, p_m2saudiosink->payload_type);
		break;
	case PROP_DEBUG_MESSAGE_INTERVAL:
		g_value_set_uint (value, p_m2saudiosink->debug_message_interval);
		break;
	case PROP_PACKET_TIME:
		g_value_set_uint (value, p_m2saudiosink->packet_time);
		break;
	case PROP_TX_DELAY_MS:
		g_value_set_int (value, p_m2saudiosink->tx_delay_ms);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

void
gst_m2saudiosink_dispose (GObject * object)
{
	GstM2saudiosink *m2saudiosink = GST_M2SAUDIOSINK (object);

	GST_DEBUG_OBJECT (m2saudiosink, "dispose");

	/* clean up as possible.  may be called multiple times */

	G_OBJECT_CLASS (gst_m2saudiosink_parent_class)->dispose (object);
}

static GstStateChangeReturn
gst_m2saudiosink_change_state (GstElement * element, GstStateChange transition)
{
	GstM2saudiosink *p_m2saudiosink = GST_M2SAUDIOSINK (element);
	GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

	switch (transition)
	{
	case GST_STATE_CHANGE_NULL_TO_READY:
		m2s_open_conf_t open_conf;
		open_conf.cuda_dev_num = p_m2saudiosink->gpu_num;
		open_conf.p_ipx_license_file = nullptr;
		m2s_open(&open_conf);

		m2s_cpu_affinity_t cpu_affinity;
		cpu_affinity.tx.num = p_m2saudiosink->cpu_num;
		m2s_create(&p_m2saudiosink->strm_id, M2S_IO_TYPE_TX, M2S_MEDIA_TYPE_AUDIO, M2S_MEMORY_MODE_CPU, &cpu_affinity, NULL, false);

		break;

	case GST_STATE_CHANGE_READY_TO_PAUSED:
		break;

	case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
		m2s_start(p_m2saudiosink->strm_id);
		m2s_enable_select(p_m2saudiosink->strm_id, true);
		start_monitoring_timer(p_m2saudiosink);
		break;

	case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
		stop_monitoring_timer(p_m2saudiosink);
		m2s_enable_select(p_m2saudiosink->strm_id, false);
		m2s_stop(p_m2saudiosink->strm_id);
		break;

	case GST_STATE_CHANGE_PAUSED_TO_READY:
		break;

	case GST_STATE_CHANGE_READY_TO_NULL:
		m2s_delete(p_m2saudiosink->strm_id);
		//m2s_close();
		break;

	default:
		break;
	}

	ret = GST_ELEMENT_CLASS (gst_m2saudiosink_parent_class)->change_state (element, transition);
	return ret;
}

void
gst_m2saudiosink_finalize (GObject * object)
{
	GstM2saudiosink *m2saudiosink = GST_M2SAUDIOSINK (object);

	GST_DEBUG_OBJECT (m2saudiosink, "finalize");

	/* clean up object here */

	G_OBJECT_CLASS (gst_m2saudiosink_parent_class)->finalize (object);
}

/* notify subclass of new caps */
static gboolean
gst_m2saudiosink_set_caps (GstBaseSink * p_sink, GstCaps * p_caps)
{
	GstM2saudiosink *p_m2saudiosink = GST_M2SAUDIOSINK (p_sink);
	GstAudioInfo info;
	GST_DEBUG_OBJECT (p_m2saudiosink, "set_caps");

	if (!gst_audio_info_from_caps (&info, p_caps)) {
		GST_ERROR_OBJECT (p_sink, "Failed to parse caps %" GST_PTR_FORMAT, p_caps);
		return FALSE;
	}

	GST_DEBUG_OBJECT (p_sink, "Setting caps %" GST_PTR_FORMAT, p_caps);
	p_m2saudiosink->info = info;

	m2s_media_conf_t media_conf;
	m2s_ip_conf_t ip_conf;
	memset(&media_conf, 0, sizeof(media_conf));
	memset(&ip_conf, 0, sizeof(ip_conf));

	for (int i = 0; i < 2; i++)
	{
		ip_conf.dst_ip[i] = m2s_conv_ip_address_from_string(p_m2saudiosink->dst_ip[i].c_str());
		ip_conf.src_ip[i] = m2s_conv_ip_address_from_string(p_m2saudiosink->src_ip[i].c_str());
		ip_conf.dst_port[i] = p_m2saudiosink->dst_port[i];
		ip_conf.src_port[i] = p_m2saudiosink->src_port[i];
		ip_conf.payload_type[i] = p_m2saudiosink->payload_type;
		ip_conf.rtp_enabled[i] = (ip_conf.src_ip[i] == 0) ? false : true;
	}

	media_conf.audio.rtp_caps.sample_rate = M2S_AUDIO_SAMPLE_RATE_48KHz;
	media_conf.audio.rtp_caps.channels = GST_AUDIO_INFO_CHANNELS(&p_m2saudiosink->info);
	media_conf.audio.rtp_caps.packet_time = (m2s_audio_packet_time_t)p_m2saudiosink->packet_time;

	media_conf.audio.app_caps.sample_rate = M2S_AUDIO_SAMPLE_RATE_48KHz;
	media_conf.audio.app_caps.channels = media_conf.audio.rtp_caps.channels;
	media_conf.audio.app_caps.format = M2S_AUDIO_APP_FORMAT_S24BE;
	media_conf.audio.app_caps.layout = M2S_AUDIO_LAYOUT_INTERLEAVED;

	m2s_set_media_conf(p_m2saudiosink->strm_id, &media_conf);
	m2s_set_ip_conf(p_m2saudiosink->strm_id, &ip_conf);

	g_start_time_offset_ns = calc_tr_offset(M2S_MEDIA_TYPE_AUDIO, &media_conf);

	p_m2saudiosink->done_first_set_contents = false;
	p_m2saudiosink->raw_offset = 0;
	p_m2saudiosink->raw_element_length = 2880 * media_conf.audio.app_caps.channels;

	return TRUE;
}

/* fixate sink caps during pull-mode negotiation */
static GstCaps *
gst_m2saudiosink_fixate (GstBaseSink * sink, GstCaps * caps)
{
	GstM2saudiosink *m2saudiosink = GST_M2SAUDIOSINK (sink);

	GST_DEBUG_OBJECT (m2saudiosink, "fixate");

	return NULL;
}

static GstFlowReturn
gst_m2saudiosink_render (GstBaseSink * sink, GstBuffer * buffer)
{
	GstFlowReturn ret;
	int32_t ret_m2s;
	GstM2saudiosink *p_m2saudiosink = GST_M2SAUDIOSINK (sink);
	GstMapInfo info;
	m2s_media_t media;
	m2s_media_size_t size;
	m2s_time_info_t time_info;
	uint64_t align_time;
	auto &vec_raw = p_m2saudiosink->vec_raw;

	GST_DEBUG_OBJECT (p_m2saudiosink, "render");

	if (gst_buffer_map(buffer, &info, GST_MAP_READ))
	{
		// Workaround: m2s does not yet support variable length writing.
		vec_raw.resize(vec_raw.size() + info.size);
		memcpy(&vec_raw[vec_raw.size() - info.size], info.data, info.size);

		while (vec_raw.size() >= p_m2saudiosink->raw_element_length)
		{
			size.audio.raw_size = p_m2saudiosink->raw_element_length;
			if ((ret_m2s = m2s_write_select(p_m2saudiosink->strm_id, &size, nullptr)) != 0)
			{
				if ((ret_m2s == M2S_RET_NOT_START) || (ret_m2s == M2S_RET_DISABLED))
				{
					ret = GST_FLOW_OK;
				}
				else
				{
					//DBG_MSG("!!! gst_m2saudiosink_render : m2s_write_select error: ret=%#010x size=%u\n", ret_m2s, size.audio.raw_size);
					ret = GST_FLOW_ERROR;
				}
				goto UNMAP;
			}

			if (!p_m2saudiosink->done_first_set_contents)
			{
				// The first frame will be sent 200 msec after the current time.
				p_m2saudiosink->start_time = m2s_get_current_tai_ns() + ((int64_t)p_m2saudiosink->tx_delay_ms * 1000000);
				p_m2saudiosink->done_first_set_contents = true;
			}
			align_time = m2s_calc_next_audio_alignment_point(p_m2saudiosink->start_time, p_m2saudiosink->raw_offset++);
			time_info.start_time_ns = align_time + g_start_time_offset_ns;
			time_info.rtp_timestamp = m2s_conv_tai_to_rtptime(align_time, M2S_RTP_COUNTER_FREQ_48KHZ);

			media.audio.p_raw = &vec_raw[0];

			if ((ret_m2s = m2s_write(p_m2saudiosink->strm_id, &time_info, &media, &size)) != 0)
			{
				if (ret_m2s == M2S_RET_NOT_START)
				{
					ret = GST_FLOW_OK;
				}
				else
				{
					//DBG_MSG("!!! gst_m2saudiosink_render : m2s_write error: ret=%#010x\n", ret_m2s);
					ret = GST_FLOW_ERROR;
				}
				goto UNMAP;
			}

			vec_raw.erase(vec_raw.begin(), vec_raw.begin() + p_m2saudiosink->raw_element_length);
		}

		ret = GST_FLOW_OK;

	  UNMAP:
		gst_buffer_unmap(buffer, &info);
	}
	else
	{
		ret = GST_FLOW_ERROR;
	}

	return ret;
}

static gboolean
plugin_init (GstPlugin * plugin)
{

	/* FIXME Remember to set the rank if it's an element that is meant
	   to be autoplugged by decodebin. */
	return gst_element_register (plugin, "m2saudiosink", GST_RANK_NONE,
	                             GST_TYPE_M2SAUDIOSINK);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "2.12.1"
#endif
#ifndef PACKAGE
#define PACKAGE "FIXME_package"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "FIXME_package_name"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://FIXME.org/"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
                   GST_VERSION_MINOR,
                   m2saudiosink,
                   "FIXME plugin description",
                   plugin_init, VERSION, GST_LICENSE_UNKNOWN, PACKAGE_NAME, GST_PACKAGE_ORIGIN)
