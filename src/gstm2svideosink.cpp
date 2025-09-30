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
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideosink.h>
#include <m2s_api.h>
#include <tr_offset.h>
#include "gstm2svideosink.h"

#define DBG_MSG(format, args...) printf("[m2svideosink] " format, ## args)

GST_DEBUG_CATEGORY_STATIC (gst_m2svideosink_debug_category);
#define GST_CAT_DEFAULT gst_m2svideosink_debug_category

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
#define DEFAULT_PAYLOAD_TYPE             (96)
#define DEFAULT_DEBUG_MESSAGE_INTERVAL   (10)
#define DEFAULT_SCAN                     (0)
#define DEFAULT_RTP_FORMAT               M2S_VIDEO_RTP_FORMAT_RAW_YUV422_10bit
#define DEFAULT_IPX_LICENSE              ""
#define DEFAULT_BPP                      (3)
#define DEFAULT_BOX_MODE                 (FALSE)
#define DEFAULT_BOX_SIZE                 (60)
#define DEFAULT_GPUDIRECT                (FALSE)
#define DEFAULT_TX_DELAY_MS              (500)

#define GST_TYPE_M2S_VIDEO_SINK_RTP_FORMAT (gst_m2s_video_sink_rtp_format_get_type ())
static GType gst_m2s_video_sink_rtp_format_get_type (void)
{
	static GType m2s_video_sink_rtp_format = 0;
	if (!m2s_video_sink_rtp_format) {
		static const GEnumValue rtp_formats[] = {
			{M2S_VIDEO_RTP_FORMAT_RAW_YUV422_10bit, "Raw YUV422 10bit", "raw-yuv422-10bit"},
			{M2S_VIDEO_RTP_FORMAT_JXSV_YUV422_8bit, "JPEG-XS YUV422 8bit", "jxsv-yuv422-8bit"},
			{M2S_VIDEO_RTP_FORMAT_JXSV_YUV422_10bit, "JPEG-XS YUV422 10bit", "jxsv-yuv422-10bit"},
			{M2S_VIDEO_RTP_FORMAT_JXSV_BGRA_8bit, "JPEG-XS BGRA 8bit", "jxsv-bgra-8bit"},
			{0, NULL, NULL},
		};
		m2s_video_sink_rtp_format = g_enum_register_static ("GstM2sVideoSinkRtpFormat", rtp_formats);
	}
	return m2s_video_sink_rtp_format;
}

/* prototypes */

static void gst_m2svideosink_set_gpu_num (GstM2svideosink *m2svideosink, uint8_t gpu_num);
static void gst_m2svideosink_set_cpu_num (GstM2svideosink *m2svideosink, int32_t cpu_num);
static void gst_m2svideosink_set_p_dst_address (GstM2svideosink *m2svideosink, const char *p_address);
static void gst_m2svideosink_set_s_dst_address (GstM2svideosink *m2svideosink, const char *p_address);
static void gst_m2svideosink_set_p_src_address (GstM2svideosink *m2svideosink, const char *p_address);
static void gst_m2svideosink_set_s_src_address (GstM2svideosink *m2svideosink, const char *p_address);
static void gst_m2svideosink_set_p_dst_port (GstM2svideosink *m2svideosink, uint16_t port);
static void gst_m2svideosink_set_s_dst_port (GstM2svideosink *m2svideosink, uint16_t port);
static void gst_m2svideosink_set_p_src_port (GstM2svideosink *m2svideosink, uint16_t port);
static void gst_m2svideosink_set_s_src_port (GstM2svideosink *m2svideosink, uint16_t port);
static void gst_m2svideosink_set_payload_type (GstM2svideosink *m2svideosink, uint8_t payoad_type);
static void gst_m2svideosink_set_debug_message_interval (GstM2svideosink *m2svideosink, uint16_t interval);
static void gst_m2svideosink_set_scan (GstM2svideosink *m2svideosink, uint8_t scan);
static void gst_m2svideosink_set_rtp_format (GstM2svideosink *m2svideosink, m2s_video_rtp_format_t rtp_format);
static void gst_m2svideosink_set_ipx_license (GstM2svideosink *m2svideosink, const char *p_file);
static void gst_m2svideosink_set_bpp (GstM2svideosink *m2svideosink, float bpp);
static void gst_m2svideosink_set_box_mode (GstM2svideosink *m2svideosink, bool box_mode);
static void gst_m2svideosink_set_box_size (GstM2svideosink *m2svideosink, uint8_t box_size);
static void gst_m2svideosink_set_gpudirect (GstM2svideosink *m2svideosink, bool gpudirect);
static void gst_m2svideosink_set_tx_delay_ms (GstM2svideosink *m2svideosink, int32_t tx_delay_ms);
static void gst_m2svideosink_set_property (GObject * object,
                                           guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_m2svideosink_get_property (GObject * object,
                                           guint property_id, GValue * value, GParamSpec * pspec);
static void gst_m2svideosink_dispose (GObject * object);
static void gst_m2svideosink_finalize (GObject * object);

static GstStateChangeReturn gst_m2svideosink_change_state (GstElement * element, GstStateChange transition);

static gboolean gst_m2svideosink_set_caps (GstBaseSink * bsink, GstCaps * caps);

static GstFlowReturn gst_m2svideosink_show_frame (GstVideoSink * video_sink,
                                                  GstBuffer * buf);

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
	PROP_SCAN,
	PROP_RTP_FORMAT,
	PROP_IPX_LICENSE,
	PROP_BPP,
	PROP_BOX_MODE,
	PROP_BOX_SIZE,
	PROP_GPUDIRECT,
	PROP_TX_DELAY_MS,
};

static int32_t g_start_time_offset_ns = 0;
/* pad templates */

/* FIXME: add/remove formats you can handle */
#define VIDEO_SINK_CAPS \
	GST_VIDEO_CAPS_MAKE("{ UYVP, UYVY, I420, v210, BGRx }")


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstM2svideosink, gst_m2svideosink, GST_TYPE_VIDEO_SINK,
                         GST_DEBUG_CATEGORY_INIT (gst_m2svideosink_debug_category, "m2svideosink", 0,
                                                  "debug category for m2svideosink element"));

static void monitoring_thread_main(GstM2svideosink *p_m2svideosink)
{
	m2s_status_t status;
	std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now();
	std::unique_lock<std::mutex> lock(p_m2svideosink->mon_lock);

	while(1)
	{
		tp += std::chrono::seconds(p_m2svideosink->debug_message_interval);
		p_m2svideosink->mon_cond.wait_until(lock, tp);

		if (!p_m2svideosink->mon_running)
		{
			break;
		}

		m2s_get_status(p_m2svideosink->strm_id, &status, true);

		printf("[M2S_STATUS: TX_VIDEO(dst_ip[0]=%s)]\n"
			   " (Stream) reset=%u\n"
			   " (APP_FIFO) enqueue=%u dequeue=%u stored=%u\n"
			   " (RTP_FIFO) enqueue=%u dequeue=%u stored=%u\n"
			   " (Packet) snd=%u zeroed=%u discontinuous=%u\n"
			   " (Debug) cpu_load=%f\n",
			   p_m2svideosink->dst_ip[0].c_str(),
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

static void start_monitoring_timer(GstM2svideosink *p_m2svideosink)
{
	p_m2svideosink->mon_running = true;
	p_m2svideosink->p_mon_thread = new std::thread(&monitoring_thread_main, p_m2svideosink);
}

static void stop_monitoring_timer(GstM2svideosink *p_m2svideosink)
{
	{
		std::unique_lock<std::mutex> lock(p_m2svideosink->mon_lock);
		p_m2svideosink->mon_running = false;
		p_m2svideosink->mon_cond.notify_all();
	}
	p_m2svideosink->p_mon_thread->join();
	delete p_m2svideosink->p_mon_thread;
}

static void gst_m2svideosink_set_gpu_num (GstM2svideosink *m2svideosink, uint8_t gpu_num)
{
	m2svideosink->gpu_num = gpu_num;
}

static void gst_m2svideosink_set_cpu_num (GstM2svideosink *m2svideosink, int32_t cpu_num)
{
	m2svideosink->cpu_num = cpu_num;
}

static void gst_m2svideosink_set_p_dst_address (GstM2svideosink *m2svideosink, const char *p_address)
{
	m2svideosink->dst_ip[0] = p_address;
}

static void gst_m2svideosink_set_s_dst_address (GstM2svideosink *m2svideosink, const char *p_address)
{
	m2svideosink->dst_ip[1] = p_address;
}

static void gst_m2svideosink_set_p_src_address (GstM2svideosink *m2svideosink, const char *p_address)
{
	m2svideosink->src_ip[0] = p_address;
}

static void gst_m2svideosink_set_s_src_address (GstM2svideosink *m2svideosink, const char *p_address)
{
	m2svideosink->src_ip[1] = p_address;
}

static void gst_m2svideosink_set_p_dst_port (GstM2svideosink *m2svideosink, uint16_t port)
{
	m2svideosink->dst_port[0] = port;
}

static void gst_m2svideosink_set_s_dst_port (GstM2svideosink *m2svideosink, uint16_t port)
{
	m2svideosink->dst_port[1] = port;
}

static void gst_m2svideosink_set_p_src_port (GstM2svideosink *m2svideosink, uint16_t port)
{
	m2svideosink->src_port[0] = port;
}

static void gst_m2svideosink_set_s_src_port (GstM2svideosink *m2svideosink, uint16_t port)
{
	m2svideosink->src_port[1] = port;
}

static void gst_m2svideosink_set_payload_type (GstM2svideosink *m2svideosink, uint8_t payload_type)
{
	m2svideosink->payload_type = payload_type;
}

static void gst_m2svideosink_set_debug_message_interval (GstM2svideosink *m2svideosink, uint16_t interval)
{
	std::unique_lock<std::mutex> lock(m2svideosink->mon_lock);
	m2svideosink->debug_message_interval = interval;
}

static void gst_m2svideosink_set_scan (GstM2svideosink *m2svideosink, uint8_t scan)
{
	m2svideosink->scan = scan;
}

static void gst_m2svideosink_set_rtp_format (GstM2svideosink *m2svideosink, m2s_video_rtp_format_t rtp_format)
{
	m2svideosink->rtp_format = rtp_format;
}

static void gst_m2svideosink_set_ipx_license (GstM2svideosink *m2svideosink, const char *p_file)
{
	strncpy(m2svideosink->ipx_license, p_file, sizeof(m2svideosink->ipx_license) - 1);
}

static void gst_m2svideosink_set_bpp (GstM2svideosink *m2svideosink, float bpp)
{
	m2svideosink->bpp = bpp;
}

static void gst_m2svideosink_set_box_mode (GstM2svideosink *m2svideosink, bool box_mode)
{
	m2svideosink->box_mode = box_mode;
}

static void gst_m2svideosink_set_box_size (GstM2svideosink *m2svideosink, uint8_t box_size)
{
	m2svideosink->box_size = box_size;
}

static void gst_m2svideosink_set_gpudirect (GstM2svideosink *m2svideosink, bool gpudirect)
{
	m2svideosink->gpudirect = gpudirect;
}

static void gst_m2svideosink_set_tx_delay_ms (GstM2svideosink *m2svideosink, int32_t tx_delay_ms)
{
	m2svideosink->tx_delay_ms = tx_delay_ms;
}

static void
gst_m2svideosink_class_init (GstM2svideosinkClass * klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
	GstBaseSinkClass *basesink_class = (GstBaseSinkClass *) klass;
	GstVideoSinkClass *video_sink_class = GST_VIDEO_SINK_CLASS (klass);

	/* Setting up pads and setting metadata should be moved to
	   base_class_init if you intend to subclass this class. */
	gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
	                                    gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
	                                                          gst_caps_from_string (VIDEO_SINK_CAPS)));

	gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
	                                       "FIXME Long name", "Generic", "FIXME Description",
	                                       "FIXME <fixme@example.com>");

	gobject_class->set_property = gst_m2svideosink_set_property;
	gobject_class->get_property = gst_m2svideosink_get_property;

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
	                                 g_param_spec_uint ("p-src-port", "Primary Destination Port",
	                                                    "Destination Port", 0, 65535, DEFAULT_P_SRC_PORT,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_S_SRC_PORT,
	                                 g_param_spec_uint ("s-src-port", "Secondary Destination Port",
	                                                    "Destination Port", 0, 65535, DEFAULT_S_SRC_PORT,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_PAYLOAD_TYPE,
	                                 g_param_spec_uint ("payload-type", "Payload Type",
	                                                    "Payload Type", 0, 127, DEFAULT_PAYLOAD_TYPE,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_DEBUG_MESSAGE_INTERVAL,
	                                 g_param_spec_uint ("debug-message-interval", "Debug message interval",
	                                                    "Debug message interval", 0, 65535, DEFAULT_DEBUG_MESSAGE_INTERVAL,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_SCAN,
	                                 g_param_spec_uint ("scan", "SCAN",
	                                                    "0:PROGRESSIVE, 1:INTERLACE_TFF, 2:INTERLACE_BFF", 0, 2, DEFAULT_SCAN,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_RTP_FORMAT,
	                                 g_param_spec_enum ("rtp-format", "RTP Format",
	                                                    "RTP Format.", GST_TYPE_M2S_VIDEO_SINK_RTP_FORMAT, DEFAULT_RTP_FORMAT,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_IPX_LICENSE,
	                                 g_param_spec_string ("ipx-license", "IPX LICENSE",
	                                                      "Path to IntoPIX licence file", DEFAULT_IPX_LICENSE,
	                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_BPP,
	                                 g_param_spec_float ("bpp", "BPP",
	                                                     "BPP", 0.1, 8.0, DEFAULT_BPP,
	                                                     (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_BOX_MODE,
	                                 g_param_spec_boolean ("box-mode", "Box Mode",
	                                                       "Box Mode", DEFAULT_BOX_MODE,
	                                                       (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_BOX_SIZE,
	                                 g_param_spec_uint ("box-size", "Box Size",
	                                                    "Box Size", 0, 255, DEFAULT_BOX_SIZE,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_GPUDIRECT,
	                                 g_param_spec_boolean ("gpudirect", "gpu direct",
	                                                       "GPUDirect", DEFAULT_GPUDIRECT,
	                                                       (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_TX_DELAY_MS,
	                                 g_param_spec_int  ("tx-delay-ms", "Tx delay milliseconds",
	                                                    "Tx delay ms", 0x80000000, 0x7fffffff, DEFAULT_TX_DELAY_MS,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	gobject_class->dispose = gst_m2svideosink_dispose;
	gobject_class->finalize = gst_m2svideosink_finalize;

	element_class->change_state = gst_m2svideosink_change_state;

	basesink_class->set_caps = GST_DEBUG_FUNCPTR (gst_m2svideosink_set_caps);

	video_sink_class->show_frame = GST_DEBUG_FUNCPTR (gst_m2svideosink_show_frame);

}

static void
gst_m2svideosink_init (GstM2svideosink * p_m2svideosink)
{
	gst_m2svideosink_set_gpu_num(p_m2svideosink, DEFAULT_GPU_NUM);
	gst_m2svideosink_set_cpu_num(p_m2svideosink, DEFAULT_CPU_NUM);
	gst_m2svideosink_set_p_dst_address(p_m2svideosink, DEFAULT_P_DST_ADDRESS);
	gst_m2svideosink_set_s_dst_address(p_m2svideosink, DEFAULT_S_DST_ADDRESS);
	gst_m2svideosink_set_p_src_address(p_m2svideosink, DEFAULT_P_SRC_ADDRESS);
	gst_m2svideosink_set_s_src_address(p_m2svideosink, DEFAULT_S_SRC_ADDRESS);
	gst_m2svideosink_set_p_dst_port(p_m2svideosink, DEFAULT_P_DST_PORT);
	gst_m2svideosink_set_s_dst_port(p_m2svideosink, DEFAULT_S_DST_PORT);
	gst_m2svideosink_set_p_src_port(p_m2svideosink, DEFAULT_P_SRC_PORT);
	gst_m2svideosink_set_s_src_port(p_m2svideosink, DEFAULT_S_SRC_PORT);
	gst_m2svideosink_set_payload_type(p_m2svideosink, DEFAULT_PAYLOAD_TYPE);
	gst_m2svideosink_set_debug_message_interval(p_m2svideosink, DEFAULT_DEBUG_MESSAGE_INTERVAL);
	gst_m2svideosink_set_scan(p_m2svideosink, DEFAULT_SCAN);
	gst_m2svideosink_set_rtp_format(p_m2svideosink, DEFAULT_RTP_FORMAT);
	gst_m2svideosink_set_ipx_license(p_m2svideosink, DEFAULT_IPX_LICENSE);
	gst_m2svideosink_set_bpp(p_m2svideosink, DEFAULT_BPP);
	gst_m2svideosink_set_box_mode(p_m2svideosink, DEFAULT_BOX_MODE);
	gst_m2svideosink_set_box_size(p_m2svideosink, DEFAULT_BOX_SIZE);
	gst_m2svideosink_set_gpudirect(p_m2svideosink, DEFAULT_GPUDIRECT);
	gst_m2svideosink_set_tx_delay_ms(p_m2svideosink, DEFAULT_TX_DELAY_MS);
}

void
gst_m2svideosink_set_property (GObject * object, guint property_id,
                               const GValue * value, GParamSpec * pspec)
{
	GstM2svideosink *p_m2svideosink = GST_M2SVIDEOSINK (object);

	GST_DEBUG_OBJECT (p_m2svideosink, "set_property");

	switch (property_id) {
	case PROP_GPU_NUM:
		gst_m2svideosink_set_gpu_num (p_m2svideosink, g_value_get_uint (value));
		break;
	case PROP_CPU_NUM:
		gst_m2svideosink_set_cpu_num (p_m2svideosink, g_value_get_int (value));
		break;
	case PROP_P_DST_ADDRESS:
		gst_m2svideosink_set_p_dst_address (p_m2svideosink, g_value_get_string (value));
		break;
	case PROP_S_DST_ADDRESS:
		gst_m2svideosink_set_s_dst_address (p_m2svideosink, g_value_get_string (value));
		break;
	case PROP_P_SRC_ADDRESS:
		gst_m2svideosink_set_p_src_address (p_m2svideosink, g_value_get_string (value));
		break;
	case PROP_S_SRC_ADDRESS:
		gst_m2svideosink_set_s_src_address (p_m2svideosink, g_value_get_string (value));
		break;
	case PROP_P_DST_PORT:
		gst_m2svideosink_set_p_dst_port (p_m2svideosink, g_value_get_uint (value));
		break;
	case PROP_S_DST_PORT:
		gst_m2svideosink_set_s_dst_port (p_m2svideosink, g_value_get_uint (value));
		break;
	case PROP_P_SRC_PORT:
		gst_m2svideosink_set_p_src_port (p_m2svideosink, g_value_get_uint (value));
		break;
	case PROP_S_SRC_PORT:
		gst_m2svideosink_set_s_src_port (p_m2svideosink, g_value_get_uint (value));
		break;
	case PROP_PAYLOAD_TYPE:
		gst_m2svideosink_set_payload_type (p_m2svideosink, g_value_get_uint (value));
		break;
	case PROP_DEBUG_MESSAGE_INTERVAL:
		gst_m2svideosink_set_debug_message_interval (p_m2svideosink, g_value_get_uint (value));
		break;
	case PROP_SCAN:
		gst_m2svideosink_set_scan (p_m2svideosink, g_value_get_uint (value));
		break;
	case PROP_RTP_FORMAT:
		gst_m2svideosink_set_rtp_format (p_m2svideosink, (m2s_video_rtp_format_t)g_value_get_enum (value));
		break;
	case PROP_IPX_LICENSE:
		gst_m2svideosink_set_ipx_license (p_m2svideosink, g_value_get_string (value));
		break;
	case PROP_BPP:
		gst_m2svideosink_set_bpp (p_m2svideosink, g_value_get_float (value));
		break;
	case PROP_BOX_MODE:
		gst_m2svideosink_set_box_mode (p_m2svideosink, g_value_get_boolean (value));
		break;
	case PROP_BOX_SIZE:
		gst_m2svideosink_set_box_size (p_m2svideosink, g_value_get_uint (value));
		break;
	case PROP_GPUDIRECT:
		gst_m2svideosink_set_gpudirect (p_m2svideosink, g_value_get_boolean (value));
		break;
	case PROP_TX_DELAY_MS:
		gst_m2svideosink_set_tx_delay_ms (p_m2svideosink, g_value_get_int (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

void
gst_m2svideosink_get_property (GObject * object, guint property_id,
                               GValue * value, GParamSpec * pspec)
{
	GstM2svideosink *p_m2svideosink = GST_M2SVIDEOSINK (object);

	GST_DEBUG_OBJECT (p_m2svideosink, "get_property");

	switch (property_id) {
	case PROP_GPU_NUM:
		g_value_set_uint (value, p_m2svideosink->gpu_num);
		break;
	case PROP_CPU_NUM:
		g_value_set_int (value, p_m2svideosink->cpu_num);
		break;
	case PROP_P_DST_ADDRESS:
		g_value_set_string (value, p_m2svideosink->dst_ip[0].c_str());
		break;
	case PROP_S_DST_ADDRESS:
		g_value_set_string (value, p_m2svideosink->dst_ip[1].c_str());
		break;
	case PROP_P_SRC_ADDRESS:
		g_value_set_string (value, p_m2svideosink->src_ip[0].c_str());
		break;
	case PROP_S_SRC_ADDRESS:
		g_value_set_string (value, p_m2svideosink->src_ip[1].c_str());
		break;
	case PROP_P_DST_PORT:
		g_value_set_uint (value, p_m2svideosink->dst_port[0]);
		break;
	case PROP_S_DST_PORT:
		g_value_set_uint (value, p_m2svideosink->dst_port[1]);
		break;
	case PROP_P_SRC_PORT:
		g_value_set_uint (value, p_m2svideosink->src_port[0]);
		break;
	case PROP_S_SRC_PORT:
		g_value_set_uint (value, p_m2svideosink->src_port[1]);
		break;
	case PROP_PAYLOAD_TYPE:
		g_value_set_uint (value, p_m2svideosink->payload_type);
		break;
	case PROP_DEBUG_MESSAGE_INTERVAL:
		g_value_set_uint (value, p_m2svideosink->debug_message_interval);
		break;
	case PROP_SCAN:
		g_value_set_uint (value, p_m2svideosink->scan);
		break;
	case PROP_RTP_FORMAT:
		g_value_set_enum (value, p_m2svideosink->rtp_format);
		break;
	case PROP_IPX_LICENSE:
		g_value_set_string (value, p_m2svideosink->ipx_license);
		break;
	case PROP_BPP:
		g_value_set_float (value, p_m2svideosink->bpp);
		break;
	case PROP_BOX_MODE:
		g_value_set_boolean (value, p_m2svideosink->box_mode);
		break;
	case PROP_BOX_SIZE:
		g_value_set_uint (value, p_m2svideosink->box_size);
		break;
	case PROP_GPUDIRECT:
		g_value_set_boolean (value, p_m2svideosink->gpudirect);
		break;
	case PROP_TX_DELAY_MS:
		g_value_set_int (value, p_m2svideosink->tx_delay_ms);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

void
gst_m2svideosink_dispose (GObject * object)
{
	GstM2svideosink *m2svideosink = GST_M2SVIDEOSINK (object);

	GST_DEBUG_OBJECT (m2svideosink, "dispose");

	/* clean up as possible.  may be called multiple times */

	G_OBJECT_CLASS (gst_m2svideosink_parent_class)->dispose (object);
}

void
gst_m2svideosink_finalize (GObject * object)
{
	GstM2svideosink *m2svideosink = GST_M2SVIDEOSINK (object);

	GST_DEBUG_OBJECT (m2svideosink, "finalize");

	/* clean up object here */

	G_OBJECT_CLASS (gst_m2svideosink_parent_class)->finalize (object);
}

static GstStateChangeReturn
gst_m2svideosink_change_state (GstElement * element, GstStateChange transition)
{
	GstM2svideosink *p_m2svideosink = GST_M2SVIDEOSINK (element);
	GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

	switch (transition)
	{
	case GST_STATE_CHANGE_NULL_TO_READY:
		m2s_open_conf_t open_conf;
		open_conf.cuda_dev_num = p_m2svideosink->gpu_num;
		open_conf.p_ipx_license_file = p_m2svideosink->ipx_license;
		m2s_open(&open_conf);

		m2s_cpu_affinity_t cpu_affinity;
		cpu_affinity.tx.num = p_m2svideosink->cpu_num;
		m2s_create(&p_m2svideosink->strm_id, M2S_IO_TYPE_TX, M2S_MEDIA_TYPE_VIDEO, M2S_MEMORY_MODE_CPU, &cpu_affinity, NULL, false);

		break;

	case GST_STATE_CHANGE_READY_TO_PAUSED:
		break;

	case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
		m2s_start(p_m2svideosink->strm_id);
		m2s_enable_select(p_m2svideosink->strm_id, true);
		start_monitoring_timer(p_m2svideosink);
		break;

	case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
		stop_monitoring_timer(p_m2svideosink);
		m2s_enable_select(p_m2svideosink->strm_id, false);
		m2s_stop(p_m2svideosink->strm_id);
		break;

	case GST_STATE_CHANGE_PAUSED_TO_READY:
		break;

	case GST_STATE_CHANGE_READY_TO_NULL:
		m2s_delete(p_m2svideosink->strm_id);
		//m2s_close();
		break;

	default:
		break;
	}

	ret = GST_ELEMENT_CLASS (gst_m2svideosink_parent_class)->change_state (element, transition);
	return ret;
}

static gboolean gst_m2svideosink_set_caps (GstBaseSink * p_bsink, GstCaps * p_caps)
{
	GstVideoSink *p_vsink;
	GstM2svideosink *p_m2svideosink;
	GstVideoInfo info;
	uint32_t frame_pixel_num;
	m2s_sys_conf_t sys_conf;
	m2s_media_conf_t media_conf;
	m2s_ip_conf_t ip_conf;
	memset(&sys_conf, 0, sizeof(sys_conf));
	memset(&media_conf, 0, sizeof(media_conf));
	memset(&ip_conf, 0, sizeof(ip_conf));

	p_vsink = GST_VIDEO_SINK_CAST (p_bsink);
	p_m2svideosink = GST_M2SVIDEOSINK (p_vsink);

	if (!gst_video_info_from_caps (&info, p_caps)) {
		GST_ERROR_OBJECT (p_bsink, "Failed to parse caps %" GST_PTR_FORMAT, p_caps);
		return FALSE;
	}

	GST_DEBUG_OBJECT (p_bsink, "Setting caps %" GST_PTR_FORMAT, p_caps);
	p_m2svideosink->info = info;

	DBG_MSG("framerate %u/%u\n", GST_VIDEO_INFO_FPS_N(&p_m2svideosink->info), GST_VIDEO_INFO_FPS_D(&p_m2svideosink->info));

	frame_pixel_num = GST_VIDEO_INFO_WIDTH(&p_m2svideosink->info) * GST_VIDEO_INFO_HEIGHT(&p_m2svideosink->info);
	DBG_MSG("frame_pixel_num %u\n", frame_pixel_num);

	for (int i = 0; i < 2; i++)
	{
		ip_conf.dst_ip[i] = m2s_conv_ip_address_from_string(p_m2svideosink->dst_ip[i].c_str());
		DBG_MSG("dst_ip[%u]=%s\n", i, p_m2svideosink->dst_ip[i].c_str());
		ip_conf.src_ip[i] = m2s_conv_ip_address_from_string(p_m2svideosink->src_ip[i].c_str());
		DBG_MSG("src_ip[%u]=%s\n", i, p_m2svideosink->src_ip[i].c_str());
		ip_conf.dst_port[i] = p_m2svideosink->dst_port[i];
		DBG_MSG("dst_port[%u]=%u\n", i, ip_conf.dst_port[i]);
		ip_conf.src_port[i] = p_m2svideosink->src_port[i];
		DBG_MSG("src_port[%u]=%u\n", i, ip_conf.src_port[i]);
		ip_conf.payload_type[i] = p_m2svideosink->payload_type;
		DBG_MSG("payload_type[%u]=%u\n", i, ip_conf.payload_type[i]);
		ip_conf.rtp_enabled[i] = (ip_conf.src_ip[i] == 0) ? false : true;
	}

	if ((GST_VIDEO_INFO_WIDTH(&p_m2svideosink->info) == 3840) &&
		(GST_VIDEO_INFO_HEIGHT(&p_m2svideosink->info) == 2160))
	{
		media_conf.video.app_caps.resolution = M2S_VIDEO_RESOLUTION_3840x2160;
	}
	else if ((GST_VIDEO_INFO_WIDTH(&p_m2svideosink->info) == 1920) &&
			 (GST_VIDEO_INFO_HEIGHT(&p_m2svideosink->info) == 1080))
	{
		media_conf.video.app_caps.resolution = M2S_VIDEO_RESOLUTION_1920x1080;
	}
	else
	{
		DBG_MSG("!!! unsupported video resolution !!!\n");
	}

	if (GST_VIDEO_INFO_FORMAT(&p_m2svideosink->info) == GST_VIDEO_FORMAT_I420)
	{
		DBG_MSG("GST_VIDEO_FORMAT_I420\n");
		media_conf.video.app_caps.format = M2S_VIDEO_APP_FORMAT_I420;
	}
	else if (GST_VIDEO_INFO_FORMAT(&p_m2svideosink->info) == GST_VIDEO_FORMAT_UYVP)
	{
		DBG_MSG("GST_VIDEO_FORMAT_UYVP\n");
		media_conf.video.app_caps.format = M2S_VIDEO_APP_FORMAT_UYVP;
	}
	else if (GST_VIDEO_INFO_FORMAT(&p_m2svideosink->info) == GST_VIDEO_FORMAT_UYVY)
	{
		DBG_MSG("GST_VIDEO_FORMAT_UYVY\n");
		media_conf.video.app_caps.format = M2S_VIDEO_APP_FORMAT_UYVY;
	}
	else if (GST_VIDEO_INFO_FORMAT(&p_m2svideosink->info) == GST_VIDEO_FORMAT_v210)
	{
		DBG_MSG("GST_VIDEO_FORMAT_V210\n");
		media_conf.video.app_caps.format = M2S_VIDEO_APP_FORMAT_V210;
	}
	else if (GST_VIDEO_INFO_FORMAT(&p_m2svideosink->info) == GST_VIDEO_FORMAT_BGRx)
	{
		DBG_MSG("GST_VIDEO_FORMAT_BGRx\n");
		media_conf.video.app_caps.format = M2S_VIDEO_APP_FORMAT_BGRx;
	}
	else
	{
		DBG_MSG("!!! unknown video format !!!\n");
	}

	if ((GST_VIDEO_INFO_FPS_N(&p_m2svideosink->info) == 60000) &&
		(GST_VIDEO_INFO_FPS_D(&p_m2svideosink->info) == 1001))
	{
		DBG_MSG("M2S_FRAME_RATE_60000_1001\n");
		media_conf.video.app_caps.frame_rate = M2S_FRAME_RATE_60000_1001;
	}
	else if ((GST_VIDEO_INFO_FPS_N(&p_m2svideosink->info) == 30000) &&
			 (GST_VIDEO_INFO_FPS_D(&p_m2svideosink->info) == 1001))
	{
		DBG_MSG("M2S_FRAME_RATE_30000_1001\n");
		media_conf.video.app_caps.frame_rate = M2S_FRAME_RATE_30000_1001;
	}
	else if ((GST_VIDEO_INFO_FPS_N(&p_m2svideosink->info) == 50) &&
			 (GST_VIDEO_INFO_FPS_D(&p_m2svideosink->info) == 1))
	{
		DBG_MSG("M2S_FRAME_RATE_50_1\n");
		media_conf.video.app_caps.frame_rate = M2S_FRAME_RATE_50_1;
	}
	else if ((GST_VIDEO_INFO_FPS_N(&p_m2svideosink->info) == 25) &&
			 (GST_VIDEO_INFO_FPS_D(&p_m2svideosink->info) == 1))
	{
		DBG_MSG("M2S_FRAME_RATE_25_1\n");
		media_conf.video.app_caps.frame_rate = M2S_FRAME_RATE_25_1;
	}
	else
	{
		DBG_MSG("!!! unsupported framerate !!!\n");
	}

	media_conf.video.rtp_caps.format = p_m2svideosink->rtp_format;
	media_conf.video.rtp_caps.scan = (m2s_video_scan_t)p_m2svideosink->scan;
	media_conf.video.rtp_caps.frame_rate = media_conf.video.app_caps.frame_rate;
	media_conf.video.rtp_caps.resolution = media_conf.video.app_caps.resolution;
	media_conf.video.rtp_caps.target_bpp = p_m2svideosink->bpp;
	//DBG_MSG("bpp: %f\n", media_conf.video.rtp_caps.target_bpp);
	media_conf.video.rtp_caps.box_mode = p_m2svideosink->box_mode;
	//DBG_MSG("box_mode: %d\n", media_conf.video.rtp_caps.box_mode);
	media_conf.video.rtp_caps.box_size = p_m2svideosink->box_size;
	//DBG_MSG("box_size: %d\n", media_conf.video.rtp_caps.box_size);

	switch (media_conf.video.rtp_caps.scan)
	{
	case M2S_VIDEO_SCAN_PROGRESSIVE:
		DBG_MSG("M2S_VIDEO_SCAN_PROGRESSIVE\n");
		break;
	case M2S_VIDEO_SCAN_INTERLACE_TFF:
		DBG_MSG("M2S_VIDEO_SCAN_INTERLACE_TFF\n");
		break;
	case M2S_VIDEO_SCAN_INTERLACE_BFF:
		DBG_MSG("M2S_VIDEO_SCAN_INTERLACE_BFF\n");
		break;
	default:
		break;
	}

	p_m2svideosink->m2s_frame_rate = media_conf.video.rtp_caps.frame_rate;

	sys_conf.use_gpu_direct = p_m2svideosink->gpudirect;
	m2s_set_sys_conf(p_m2svideosink->strm_id, &sys_conf);

	m2s_set_media_conf(p_m2svideosink->strm_id, &media_conf);
	m2s_set_ip_conf(p_m2svideosink->strm_id, &ip_conf);

	g_start_time_offset_ns = calc_tr_offset(M2S_MEDIA_TYPE_VIDEO, &media_conf);

	p_m2svideosink->done_first_set_contents = false;
	p_m2svideosink->frame_offset = 0;

	return TRUE;
}

static GstFlowReturn
gst_m2svideosink_show_frame (GstVideoSink * sink, GstBuffer * buf)
{
	GstFlowReturn ret;
	int32_t ret_m2s;
	GstM2svideosink *p_m2svideosink = GST_M2SVIDEOSINK (sink);
	GstMapInfo info;
	m2s_media_t media;
	m2s_media_size_t size;
	m2s_time_info_t time_info;
	uint64_t align_time;

	GST_DEBUG_OBJECT (p_m2svideosink, "show_frame");

	if (gst_buffer_map(buf, &info, GST_MAP_READ))
	{
		size.video.frame_size = info.size;
		if ((ret_m2s = m2s_write_select(p_m2svideosink->strm_id, &size, nullptr)) != 0)
		{
			if (ret_m2s != M2S_RET_NOT_START)
			{
				//DBG_MSG("!!! gst_m2svideosink_show_frame : m2s_write_select error: ret=%#010x size=%u\n", ret_m2s, size.video.frame_size);
			}
			ret = GST_FLOW_OK;
			goto UNMAP;
		}

		if (!p_m2svideosink->done_first_set_contents)
		{
			// The first frame will be sent 500 msec after the current time.
			p_m2svideosink->start_time = m2s_get_current_tai_ns() + ((int64_t)p_m2svideosink->tx_delay_ms * 1000000);
			p_m2svideosink->done_first_set_contents = true;
		}
		align_time = m2s_calc_next_video_alignment_point(p_m2svideosink->start_time, p_m2svideosink->m2s_frame_rate, p_m2svideosink->frame_offset++);
		time_info.start_time_ns = align_time;
		time_info.start_time_ns += g_start_time_offset_ns;
		time_info.rtp_timestamp = m2s_conv_tai_to_rtptime(align_time, M2S_RTP_COUNTER_FREQ_90KHZ);

		media.video.p_frame = &info.data[0];

		if ((ret_m2s = m2s_write(p_m2svideosink->strm_id, &time_info, &media, &size)) != 0)
		{
			if (ret_m2s != M2S_RET_NOT_START)
			{
				//DBG_MSG("!!! gst_m2svideosink_show_frame : m2s_write error: ret=%d\n", ret_m2s);
			}
			ret = GST_FLOW_ERROR;
			goto UNMAP;
		}

		ret = GST_FLOW_OK;

	  UNMAP:
		gst_buffer_unmap(buf, &info);
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
	return gst_element_register (plugin, "m2svideosink", GST_RANK_NONE,
	                             GST_TYPE_M2SVIDEOSINK);
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
                   m2svideosink,
                   "FIXME plugin description",
                   plugin_init, VERSION, GST_LICENSE_UNKNOWN, PACKAGE_NAME, GST_PACKAGE_ORIGIN)
