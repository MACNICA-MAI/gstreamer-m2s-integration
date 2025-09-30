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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <m2s_api.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>
#include <gst/video/gstvideometa.h>
#include "gstm2svideosrc.h"

#define DBG_MSG(format, args...) printf("[m2svideosrc] " format, ## args)

GST_DEBUG_CATEGORY_STATIC (m2svideosrc_debug);
#define GST_CAT_DEFAULT m2svideosrc_debug

#define DEFAULT_TIMESTAMP_OFFSET   0
#define DEFAULT_IS_LIVE            FALSE

#define DEFAULT_HW_HITLESS               (TRUE)
#define DEFAULT_GPU_NUM                  (0)
#define DEFAULT_L2_CPU_NUM               (-1)
#define DEFAULT_L1_CPU_NUM               (-1)
#define DEFAULT_P_IF_ADDRESS             "239.1.1.1"
#define DEFAULT_S_IF_ADDRESS             "0.0.0.0"
#define DEFAULT_P_DST_ADDRESS            "192.168.0.1"
#define DEFAULT_S_DST_ADDRESS            "0.0.0.0"
#define DEFAULT_P_SRC_ADDRESS            "192.168.1.1"
#define DEFAULT_S_SRC_ADDRESS            "0.0.0.0"
#define DEFAULT_P_DST_PORT               (50000)
#define DEFAULT_S_DST_PORT               (50001)
#define DEFAULT_P_SRC_PORT               (0)
#define DEFAULT_S_SRC_PORT               (0)
#define DEFAULT_PAYLOAD_TYPE             (96)
#define DEFAULT_PLAYOUT_DELAY_MS         (0)
#define DEFAULT_DEBUG_MESSAGE_INTERVAL   (10)
#define DEFAULT_SCAN                     (0)
#define DEFAULT_RTP_FORMAT               M2S_VIDEO_RTP_FORMAT_RAW_YUV422_10bit
#define DEFAULT_IPX_LICENSE              ""
#define DEFAULT_BOX_MODE                 (FALSE)
#define DEFAULT_BOX_SIZE                 (60)
#define DEFAULT_ASYNC_RTP_TIMESTAMP      (FALSE)
#define DEFAULT_FIFO_UNDER_THRESHOLD     (2)
#define DEFAULT_FIFO_MIDDLE              (4)
#define DEFAULT_FIFO_OVER_THRESHOLD      (6)
#define DEFAULT_UNDER_COUNT_MAX          (60)
#define DEFAULT_GPUDIRECT                (FALSE)

enum
{
	PROP_0,
	PROP_TIMESTAMP_OFFSET,
	PROP_IS_LIVE,
	PROP_HW_HITLESS,
	PROP_GPU_NUM,
	PROP_L2_CPU_NUM,
	PROP_L1_CPU_NUM,
	PROP_P_IF_ADDRESS,
	PROP_S_IF_ADDRESS,
	PROP_P_DST_ADDRESS,
	PROP_S_DST_ADDRESS,
	PROP_P_SRC_ADDRESS,
	PROP_S_SRC_ADDRESS,
	PROP_P_DST_PORT,
	PROP_S_DST_PORT,
	PROP_P_SRC_PORT,
	PROP_S_SRC_PORT,
	PROP_PLAYOUT_DELAY_MS,
	PROP_DEBUG_MESSAGE_INTERVAL,
	PROP_SCAN,
	PROP_PAYLOAD_TYPE,
	PROP_RTP_FORMAT,
	PROP_IPX_LICENSE,
	PROP_BOX_MODE,
	PROP_BOX_SIZE,
	PROP_ASYNC_RTP_TIMESTAMP,
	PROP_FIFO_UNDER_THRESHOLD,
	PROP_FIFO_MIDDLE,
	PROP_FIFO_OVER_THRESHOLD,
	PROP_UNDER_COUNT_MAX,
	PROP_GPUDIRECT,
	PROP_LAST
};


#define VTS_VIDEO_CAPS GST_VIDEO_CAPS_MAKE ("{ UYVP, UYVY, I420, v210, BGRx }") "," \
  "width = " GST_VIDEO_SIZE_RANGE ", "                                 \
  "height = " GST_VIDEO_SIZE_RANGE ", "                                \
  "framerate = " GST_VIDEO_FPS_RANGE


static GstStaticPadTemplate gst_m2svideosrc_template =
GST_STATIC_PAD_TEMPLATE ("src",
	GST_PAD_SRC,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS (VTS_VIDEO_CAPS)
	);

#define gst_m2svideosrc_parent_class parent_class
G_DEFINE_TYPE (GstM2svideosrc, gst_m2svideosrc, GST_TYPE_PUSH_SRC);

#define GST_TYPE_M2S_VIDEO_SRC_RTP_FORMAT (gst_m2s_video_src_rtp_format_get_type ())
static GType gst_m2s_video_src_rtp_format_get_type (void)
{
	static GType m2s_video_src_rtp_format = 0;
	if (!m2s_video_src_rtp_format) {
		static const GEnumValue rtp_formats[] = {
			{M2S_VIDEO_RTP_FORMAT_RAW_YUV422_10bit, "Raw YUV422 10bit", "raw-yuv422-10bit"},
			{M2S_VIDEO_RTP_FORMAT_JXSV_YUV422_8bit, "JPEG-XS YUV422 8bit", "jxsv-yuv422-8bit"},
			{M2S_VIDEO_RTP_FORMAT_JXSV_YUV422_10bit, "JPEG-XS YUV422 10bit", "jxsv-yuv422-10bit"},
			{M2S_VIDEO_RTP_FORMAT_JXSV_BGRA_8bit, "JPEG-XS BGRA 8bit", "jxsv-bgra-8bit"},
			{0, NULL, NULL},
		};
		m2s_video_src_rtp_format = g_enum_register_static ("GstM2sVideoSrcRtpFormat", rtp_formats);
	}
	return m2s_video_src_rtp_format;
}

static void gst_m2svideosrc_set_hw_hitless (GstM2svideosrc *m2svideosrc, bool hw_hitless);
static void gst_m2svideosrc_set_gpu_num (GstM2svideosrc *m2svideosrc, uint8_t gpu_num);
static void gst_m2svideosrc_set_l2_cpu_num (GstM2svideosrc *m2svideosrc, int32_t cpu_num);
static void gst_m2svideosrc_set_l1_cpu_num (GstM2svideosrc *m2svideosrc, int32_t cpu_num);
static void gst_m2svideosrc_set_p_if_address (GstM2svideosrc *m2svideosrc, const char *p_address);
static void gst_m2svideosrc_set_s_if_address (GstM2svideosrc *m2svideosrc, const char *p_address);
static void gst_m2svideosrc_set_p_dst_address (GstM2svideosrc *m2svideosrc, const char *p_address);
static void gst_m2svideosrc_set_s_dst_address (GstM2svideosrc *m2svideosrc, const char *p_address);
static void gst_m2svideosrc_set_p_src_address (GstM2svideosrc *m2svideosrc, const char *p_address);
static void gst_m2svideosrc_set_s_src_address (GstM2svideosrc *m2svideosrc, const char *p_address);
static void gst_m2svideosrc_set_p_dst_port (GstM2svideosrc *m2svideosrc, uint16_t port);
static void gst_m2svideosrc_set_s_dst_port (GstM2svideosrc *m2svideosrc, uint16_t port);
static void gst_m2svideosrc_set_p_src_port (GstM2svideosrc *m2svideosrc, uint16_t port);
static void gst_m2svideosrc_set_s_src_port (GstM2svideosrc *m2svideosrc, uint16_t port);
static void gst_m2svideosrc_set_playout_delay_ms (GstM2svideosrc *m2svideosrc, int32_t playout_delay_ms);
static void gst_m2svideosrc_set_debug_message_interval (GstM2svideosrc *m2svideosrc, uint16_t interval);
static void gst_m2svideosrc_set_scan (GstM2svideosrc *m2svideosrc, uint8_t scan);
static void gst_m2svideosrc_set_payload_type (GstM2svideosrc *m2svideosrc, uint8_t payload_type);
static void gst_m2svideosrc_set_rtp_format (GstM2svideosrc *m2svideosrc, m2s_video_rtp_format_t rtp_format);
static void gst_m2svideosrc_set_ipx_license (GstM2svideosrc *m2svideosrc, const char *p_file);
static void gst_m2svideosrc_set_box_mode (GstM2svideosrc *m2svideosrc, bool box_mode);
static void gst_m2svideosrc_set_box_size (GstM2svideosrc *m2svideosrc, uint8_t box_size);
static void gst_m2svideosrc_set_async_rtp_timestamp (GstM2svideosrc *m2svideosrc, bool async_rtp_timestamp);
static void gst_m2svideosrc_set_fifo_under_threshold (GstM2svideosrc *m2svideosrc, uint8_t fifo_under_threshold);
static void gst_m2svideosrc_set_fifo_middle (GstM2svideosrc *m2svideosrc, uint8_t fifo_middle);
static void gst_m2svideosrc_set_fifo_over_threshold (GstM2svideosrc *m2svideosrc, uint8_t fifo_over_threshold);
static void gst_m2svideosrc_set_under_count_max (GstM2svideosrc *m2svideosrc, uint8_t under_count_max);
static void gst_m2svideosrc_set_gpudirect (GstM2svideosrc *m2svideosrc, bool gpudirect);

static void gst_m2svideosrc_set_property (GObject * object, guint prop_id,
                                          const GValue * value, GParamSpec * pspec);
static void gst_m2svideosrc_get_property (GObject * object, guint prop_id,
                                          GValue * value, GParamSpec * pspec);

static GstStateChangeReturn gst_m2svideosrc_change_state (GstElement * element,
                                                          GstStateChange transition);

static gboolean gst_m2svideosrc_setcaps (GstBaseSrc * bsrc, GstCaps * caps);
static GstCaps *gst_m2svideosrc_src_fixate (GstBaseSrc * bsrc,
                                            GstCaps * caps);

static gboolean gst_m2svideosrc_is_seekable (GstBaseSrc * psrc);
static gboolean gst_m2svideosrc_do_seek (GstBaseSrc * bsrc,
                                         GstSegment * segment);
static gboolean gst_m2svideosrc_query (GstBaseSrc * bsrc, GstQuery * query);

static void gst_m2svideosrc_get_times (GstBaseSrc * basesrc,
                                       GstBuffer * buffer, GstClockTime * start, GstClockTime * end);
static gboolean gst_m2svideosrc_decide_allocation (GstBaseSrc * bsrc,
                                                   GstQuery * query);
static GstFlowReturn gst_m2svideosrc_fill (GstPushSrc * psrc,
                                           GstBuffer * buffer);
static gboolean gst_m2svideosrc_start (GstBaseSrc * basesrc);
static gboolean gst_m2svideosrc_stop (GstBaseSrc * basesrc);



static void monitoring_thread_main(GstM2svideosrc *p_m2svideosrc)
{
	m2s_status_t status;
	std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now();
	std::unique_lock<std::mutex> lock(p_m2svideosrc->mon_lock);

	while(1)
	{
		tp += std::chrono::seconds(p_m2svideosrc->debug_message_interval);
		p_m2svideosrc->mon_cond.wait_until(lock, tp);

		if (!p_m2svideosrc->mon_running)
		{
			break;
		}

		m2s_get_status(p_m2svideosrc->strm_id, &status, true);

		printf("[M2S_STATUS: RX_VIDEO(dst_ip[0]=%s)]\n"
			   " (Stream) active=%d/%d ditect=%u/%u lost=%u/%u reset=%u\n"
			   " (APP_FIFO) enqueue=%u dequeue=%u stored=%u\n"
			   " (RTP_FIFO) enqueue=%u dequeue=%u stored=%u\n"
			   " (Hitless_FIFO) enqueue=%u/%u dequeue=%u/%u stored=%u/%u\n"
			   " (Packet-7) rcv=%u lost=%u discontinuous=%u\n"
			   " (Packet) rcv=%u/%u lost=%u/%u discontinuous=%u/%u underflow=%u/%u overflow=%u/%u abnormal_seqnum=%u/%u async=%d\n"
			   " (Header) hdr_extension=%u padding=%u\n"
			   " (Video) SRD1=%u SRD2=%u SRD3=%u SRD_zero_length=%u pixel_discontinuous=%u frame_length_err=%u\n"
			   " (Debug) l2_cpu_load=%f l1_cpu_load=%f\n",
			   p_m2svideosrc->dst_ip[0],
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
			   status.rx.srd_is_1,
			   status.rx.srd_is_2,
			   status.rx.srd_is_3,
			   status.rx.srd_zero_length,
			   status.rx.pixel_discontinuous,
			   status.rx.frame_length_err,
			   status.rx.l2_cpu_load,
			   status.rx.l1_cpu_load);
		printf("\n");
	}
}

static void start_monitoring_timer(GstM2svideosrc *p_m2svideosrc)
{
	p_m2svideosrc->mon_running = true;
	p_m2svideosrc->p_mon_thread = new std::thread(&monitoring_thread_main, p_m2svideosrc);
}

static void stop_monitoring_timer(GstM2svideosrc *p_m2svideosrc)
{
	{
		std::unique_lock<std::mutex> lock(p_m2svideosrc->mon_lock);
		p_m2svideosrc->mon_running = false;
		p_m2svideosrc->mon_cond.notify_all();
	}
	p_m2svideosrc->p_mon_thread->join();
	delete p_m2svideosrc->p_mon_thread;
}

static void gst_m2svideosrc_set_hw_hitless (GstM2svideosrc *m2svideosrc, bool hw_hitless)
{
	m2svideosrc->hw_hitless = hw_hitless;
}

static void gst_m2svideosrc_set_gpu_num (GstM2svideosrc *m2svideosrc, uint8_t gpu_num)
{
	m2svideosrc->gpu_num = gpu_num;
}

static void gst_m2svideosrc_set_l2_cpu_num (GstM2svideosrc *m2svideosrc, int32_t cpu_num)
{
	m2svideosrc->l2_cpu_num = cpu_num;
}

static void gst_m2svideosrc_set_l1_cpu_num (GstM2svideosrc *m2svideosrc, int32_t cpu_num)
{
	m2svideosrc->l1_cpu_num = cpu_num;
}

static void gst_m2svideosrc_set_p_if_address (GstM2svideosrc *m2svideosrc, const char *p_address)
{
	strncpy(m2svideosrc->if_ip[0], p_address, sizeof(m2svideosrc->if_ip[0]) - 1);
}

static void gst_m2svideosrc_set_s_if_address (GstM2svideosrc *m2svideosrc, const char *p_address)
{
	strncpy(m2svideosrc->if_ip[1], p_address, sizeof(m2svideosrc->if_ip[1]) - 1);
}

static void gst_m2svideosrc_set_p_dst_address (GstM2svideosrc *m2svideosrc, const char *p_address)
{
	strncpy(m2svideosrc->dst_ip[0], p_address, sizeof(m2svideosrc->dst_ip[0]) - 1);
}

static void gst_m2svideosrc_set_s_dst_address (GstM2svideosrc *m2svideosrc, const char *p_address)
{
	strncpy(m2svideosrc->dst_ip[1], p_address, sizeof(m2svideosrc->dst_ip[1]) - 1);
}

static void gst_m2svideosrc_set_p_src_address (GstM2svideosrc *m2svideosrc, const char *p_address)
{
	strncpy(m2svideosrc->src_ip[0], p_address, sizeof(m2svideosrc->src_ip[0]) - 1);
}

static void gst_m2svideosrc_set_s_src_address (GstM2svideosrc *m2svideosrc, const char *p_address)
{
	strncpy(m2svideosrc->src_ip[1], p_address, sizeof(m2svideosrc->src_ip[1]) - 1);
}

static void gst_m2svideosrc_set_p_dst_port (GstM2svideosrc *m2svideosrc, uint16_t port)
{
	m2svideosrc->dst_port[0] = port;
}

static void gst_m2svideosrc_set_s_dst_port (GstM2svideosrc *m2svideosrc, uint16_t port)
{
	m2svideosrc->dst_port[1] = port;
}

static void gst_m2svideosrc_set_p_src_port (GstM2svideosrc *m2svideosrc, uint16_t port)
{
	m2svideosrc->src_port[0] = port;
}

static void gst_m2svideosrc_set_s_src_port (GstM2svideosrc *m2svideosrc, uint16_t port)
{
	m2svideosrc->src_port[1] = port;
}

static void gst_m2svideosrc_set_playout_delay_ms (GstM2svideosrc *m2svideosrc, int32_t playout_delay_ms)
{
	m2svideosrc->playout_delay_ms = playout_delay_ms;
}

static void gst_m2svideosrc_set_debug_message_interval (GstM2svideosrc *m2svideosrc, uint16_t interval)
{
	std::unique_lock<std::mutex> lock(m2svideosrc->mon_lock);
	m2svideosrc->debug_message_interval = interval;
}

static void gst_m2svideosrc_set_scan (GstM2svideosrc *m2svideosrc, uint8_t scan)
{
	m2svideosrc->scan = scan;
}

static void gst_m2svideosrc_set_payload_type (GstM2svideosrc *m2svideosrc, uint8_t payload_type)
{
	m2svideosrc->payload_type = payload_type;
}

static void gst_m2svideosrc_set_rtp_format (GstM2svideosrc *m2svideosrc, m2s_video_rtp_format_t rtp_format)
{
	m2svideosrc->rtp_format = rtp_format;
}

static void gst_m2svideosrc_set_ipx_license (GstM2svideosrc *m2svideosrc, const char *p_file)
{
	strncpy(m2svideosrc->ipx_license, p_file, sizeof(m2svideosrc->ipx_license) - 1);
}

static void gst_m2svideosrc_set_box_mode (GstM2svideosrc *m2svideosrc, bool box_mode)
{
	m2svideosrc->box_mode = box_mode;
}

static void gst_m2svideosrc_set_box_size (GstM2svideosrc *m2svideosrc, uint8_t box_size)
{
	m2svideosrc->box_size = box_size;
}

static void gst_m2svideosrc_set_async_rtp_timestamp (GstM2svideosrc *m2svideosrc, bool async_rtp_timestamp)
{
	m2svideosrc->async_rtp_timestamp = async_rtp_timestamp;
}

static void gst_m2svideosrc_set_fifo_under_threshold (GstM2svideosrc *m2svideosrc, uint8_t fifo_under_threshold)
{
	m2svideosrc->fifo_under_threshold = fifo_under_threshold;
}

static void gst_m2svideosrc_set_fifo_middle (GstM2svideosrc *m2svideosrc, uint8_t fifo_middle)
{
	m2svideosrc->fifo_middle = fifo_middle;
}

static void gst_m2svideosrc_set_fifo_over_threshold (GstM2svideosrc *m2svideosrc, uint8_t fifo_over_threshold)
{
	m2svideosrc->fifo_over_threshold = fifo_over_threshold;
}

static void gst_m2svideosrc_set_under_count_max (GstM2svideosrc *m2svideosrc, uint8_t under_count_max)
{
	m2svideosrc->under_count_max = under_count_max;
}

static void gst_m2svideosrc_set_gpudirect (GstM2svideosrc *m2svideosrc, bool gpudirect)
{
	m2svideosrc->gpudirect = gpudirect;
}

static void
gst_m2svideosrc_class_init (GstM2svideosrcClass * klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;
	GstBaseSrcClass *gstbasesrc_class;
	GstPushSrcClass *gstpushsrc_class;

	gobject_class = (GObjectClass *) klass;
	gstelement_class = (GstElementClass *) klass;
	gstbasesrc_class = (GstBaseSrcClass *) klass;
	gstpushsrc_class = (GstPushSrcClass *) klass;

	gobject_class->set_property = gst_m2svideosrc_set_property;
	gobject_class->get_property = gst_m2svideosrc_get_property;

	g_object_class_install_property (gobject_class, PROP_TIMESTAMP_OFFSET,
	                                 g_param_spec_int64 ("timestamp-offset", "Timestamp offset",
	                                                     "An offset added to timestamps set on buffers (in ns)", 0,
	                                                     (G_MAXLONG == G_MAXINT64) ? G_MAXINT64 : (G_MAXLONG * GST_SECOND - 1),
	                                                     0, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
	g_object_class_install_property (gobject_class, PROP_IS_LIVE,
	                                 g_param_spec_boolean ("is-live", "Is Live",
	                                                       "Whether to act as a live source", DEFAULT_IS_LIVE,
	                                                       (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_HW_HITLESS,
	                                 g_param_spec_boolean ("hw-hitless", "HW Hitless",
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

	g_object_class_install_property (gobject_class, PROP_P_IF_ADDRESS,
	                                 g_param_spec_string ("p-if-address", "Primary Interface Address",
	                                                      "Interface Address", DEFAULT_P_IF_ADDRESS,
	                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_S_IF_ADDRESS,
	                                 g_param_spec_string ("s-if-address", "Secondary Interface Address",
	                                                      "Interface Address", DEFAULT_S_IF_ADDRESS,
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

	g_object_class_install_property (gobject_class, PROP_PLAYOUT_DELAY_MS,
	                                 g_param_spec_int  ("playout-delay-ms", "Playout delay milliseconds",
	                                                    "Playout delay ms", 0x80000000, 0x7fffffff, DEFAULT_PLAYOUT_DELAY_MS,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_DEBUG_MESSAGE_INTERVAL,
	                                 g_param_spec_uint ("debug-message-interval", "Debug message interval",
	                                                    "Debug message interval", 0, 65535, DEFAULT_DEBUG_MESSAGE_INTERVAL,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_SCAN,
	                                 g_param_spec_uint ("scan", "SCAN",
	                                                    "0:PROGRESSIVE, 1:INTERLACE_TFF, 2:INTERLACE_BFF", 0, 2, DEFAULT_SCAN,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_PAYLOAD_TYPE,
	                                 g_param_spec_uint ("payload-type", "Payload Type",
	                                                    "Payload Type", 0, 127, DEFAULT_PAYLOAD_TYPE,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_RTP_FORMAT,
	                                 g_param_spec_enum ("rtp-format", "RTP Format",
	                                                    "RTP Format.", GST_TYPE_M2S_VIDEO_SRC_RTP_FORMAT, DEFAULT_RTP_FORMAT,
	                                                    (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_IPX_LICENSE,
	                                 g_param_spec_string ("ipx-license", "IPX LICENSE",
	                                                      "Path to IntoPIX licence file", DEFAULT_IPX_LICENSE,
	                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_BOX_MODE,
	                                 g_param_spec_boolean ("box-mode", "Box Mode",
	                                                       "Box Mode", DEFAULT_BOX_MODE,
	                                                       (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_BOX_SIZE,
	                               g_param_spec_uint ("box-size", "Box Size",
	                                                  "Box Size", 0, 255, DEFAULT_BOX_SIZE,
	                                                  (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_ASYNC_RTP_TIMESTAMP,
	                                 g_param_spec_boolean ("async-rtp-timestamp", "Async RTP Timestamp",
	                                                       "Async RTP Timestamp", DEFAULT_ASYNC_RTP_TIMESTAMP,
	                                                       (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_FIFO_UNDER_THRESHOLD,
	                               g_param_spec_uint ("fifo-under-threshold", "FIFO Under Threshold",
	                                                  "FIFO Under Threshold", 0, 255, DEFAULT_FIFO_UNDER_THRESHOLD,
	                                                  (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_FIFO_MIDDLE,
	                               g_param_spec_uint ("fifo-middle", "FIFO Middle",
	                                                  "FIFO Middle", 0, 255, DEFAULT_FIFO_MIDDLE,
	                                                  (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_FIFO_OVER_THRESHOLD,
	                               g_param_spec_uint ("fifo-over-threshold", "FIFO Over Threshold",
	                                                  "FIFO Over Threshold", 0, 255, DEFAULT_FIFO_OVER_THRESHOLD,
	                                                  (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_UNDER_COUNT_MAX,
	                               g_param_spec_uint ("under-count-max", "Under Count Max",
	                                                  "Under Count Max", 0, 255, DEFAULT_UNDER_COUNT_MAX,
	                                                  (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	g_object_class_install_property (gobject_class, PROP_GPUDIRECT,
	                                 g_param_spec_boolean ("gpudirect", "gpu direct",
	                                                       "GPUDirect", DEFAULT_GPUDIRECT,
	                                                       (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	gstelement_class->change_state = gst_m2svideosrc_change_state;

	gst_element_class_set_static_metadata (gstelement_class,
	                                       "FIXME Long name", "Generic",
	                                       "FIXME Description", "FIXME <fixme@example.com>");

	gst_element_class_add_static_pad_template (gstelement_class,
	                                           &gst_m2svideosrc_template);

	gstbasesrc_class->set_caps = gst_m2svideosrc_setcaps;
	gstbasesrc_class->fixate = gst_m2svideosrc_src_fixate;
	gstbasesrc_class->is_seekable = gst_m2svideosrc_is_seekable;
	gstbasesrc_class->do_seek = gst_m2svideosrc_do_seek;
	gstbasesrc_class->query = gst_m2svideosrc_query;
	gstbasesrc_class->get_times = gst_m2svideosrc_get_times;
	gstbasesrc_class->start = gst_m2svideosrc_start;
	gstbasesrc_class->stop = gst_m2svideosrc_stop;
	gstbasesrc_class->decide_allocation = gst_m2svideosrc_decide_allocation;

	gstpushsrc_class->fill = gst_m2svideosrc_fill;
}

static void
gst_m2svideosrc_init (GstM2svideosrc * p_m2svideosrc)
{
	p_m2svideosrc->timestamp_offset = DEFAULT_TIMESTAMP_OFFSET;

	/* we operate in time */
	gst_base_src_set_format (GST_BASE_SRC (p_m2svideosrc), GST_FORMAT_TIME);
	gst_base_src_set_live (GST_BASE_SRC (p_m2svideosrc), DEFAULT_IS_LIVE);

	p_m2svideosrc->fifo_is_almost_empty = true;
	gst_m2svideosrc_set_hw_hitless(p_m2svideosrc, DEFAULT_HW_HITLESS);
	gst_m2svideosrc_set_gpu_num(p_m2svideosrc, DEFAULT_GPU_NUM);
	gst_m2svideosrc_set_l2_cpu_num(p_m2svideosrc, DEFAULT_L2_CPU_NUM);
	gst_m2svideosrc_set_l1_cpu_num(p_m2svideosrc, DEFAULT_L1_CPU_NUM);
	gst_m2svideosrc_set_p_if_address(p_m2svideosrc, DEFAULT_P_IF_ADDRESS);
	gst_m2svideosrc_set_s_if_address(p_m2svideosrc, DEFAULT_S_IF_ADDRESS);
	gst_m2svideosrc_set_p_dst_address(p_m2svideosrc, DEFAULT_P_DST_ADDRESS);
	gst_m2svideosrc_set_s_dst_address(p_m2svideosrc, DEFAULT_S_DST_ADDRESS);
	gst_m2svideosrc_set_p_src_address(p_m2svideosrc, DEFAULT_P_SRC_ADDRESS);
	gst_m2svideosrc_set_s_src_address(p_m2svideosrc, DEFAULT_S_SRC_ADDRESS);
	gst_m2svideosrc_set_p_dst_port(p_m2svideosrc, DEFAULT_P_DST_PORT);
	gst_m2svideosrc_set_s_dst_port(p_m2svideosrc, DEFAULT_S_DST_PORT);
	gst_m2svideosrc_set_p_src_port(p_m2svideosrc, DEFAULT_P_SRC_PORT);
	gst_m2svideosrc_set_s_src_port(p_m2svideosrc, DEFAULT_S_SRC_PORT);
	gst_m2svideosrc_set_playout_delay_ms(p_m2svideosrc, DEFAULT_PLAYOUT_DELAY_MS);
	gst_m2svideosrc_set_debug_message_interval(p_m2svideosrc, DEFAULT_DEBUG_MESSAGE_INTERVAL);
	gst_m2svideosrc_set_scan(p_m2svideosrc, DEFAULT_SCAN);
	gst_m2svideosrc_set_payload_type(p_m2svideosrc, DEFAULT_PAYLOAD_TYPE);
	gst_m2svideosrc_set_rtp_format(p_m2svideosrc, DEFAULT_RTP_FORMAT);
	gst_m2svideosrc_set_ipx_license(p_m2svideosrc, DEFAULT_IPX_LICENSE);
	gst_m2svideosrc_set_box_mode(p_m2svideosrc, DEFAULT_BOX_MODE);
	gst_m2svideosrc_set_box_size(p_m2svideosrc, DEFAULT_BOX_SIZE);
	gst_m2svideosrc_set_async_rtp_timestamp(p_m2svideosrc, DEFAULT_ASYNC_RTP_TIMESTAMP);
	gst_m2svideosrc_set_fifo_under_threshold(p_m2svideosrc, DEFAULT_FIFO_UNDER_THRESHOLD);
	gst_m2svideosrc_set_fifo_middle(p_m2svideosrc, DEFAULT_FIFO_MIDDLE);
	gst_m2svideosrc_set_fifo_over_threshold(p_m2svideosrc, DEFAULT_FIFO_OVER_THRESHOLD);
	gst_m2svideosrc_set_under_count_max(p_m2svideosrc, DEFAULT_UNDER_COUNT_MAX);
	gst_m2svideosrc_set_gpudirect(p_m2svideosrc, DEFAULT_GPUDIRECT);
}

static GstCaps *
gst_m2svideosrc_src_fixate (GstBaseSrc * bsrc, GstCaps * caps)
{
	GstStructure *structure;

	caps = gst_caps_make_writable (caps);
	structure = gst_caps_get_structure (caps, 0);

	gst_structure_fixate_field_nearest_int (structure, "width", 320);
	gst_structure_fixate_field_nearest_int (structure, "height", 240);

	if (gst_structure_has_field (structure, "framerate"))
		gst_structure_fixate_field_nearest_fraction (structure, "framerate", 30, 1);
	else
		gst_structure_set (structure, "framerate", GST_TYPE_FRACTION, 30, 1, NULL);

	if (gst_structure_has_field (structure, "pixel-aspect-ratio"))
		gst_structure_fixate_field_nearest_fraction (structure,
		                                             "pixel-aspect-ratio", 1, 1);
	else
		gst_structure_set (structure, "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
		                   NULL);

	if (gst_structure_has_field (structure, "colorimetry"))
		gst_structure_fixate_field_string (structure, "colorimetry", "bt601");
	if (gst_structure_has_field (structure, "chroma-site"))
		gst_structure_fixate_field_string (structure, "chroma-site", "mpeg2");

	if (gst_structure_has_field (structure, "interlace-mode"))
		gst_structure_fixate_field_string (structure, "interlace-mode",
		                                   "progressive");
	else
		gst_structure_set (structure, "interlace-mode", G_TYPE_STRING,
		                   "progressive", NULL);

	if (gst_structure_has_field (structure, "multiview-mode"))
		gst_structure_fixate_field_string (structure, "multiview-mode",
		                                   gst_video_multiview_mode_to_caps_string
		                                   (GST_VIDEO_MULTIVIEW_MODE_MONO));
	else
		gst_structure_set (structure, "multiview-mode", G_TYPE_STRING,
		                   gst_video_multiview_mode_to_caps_string (GST_VIDEO_MULTIVIEW_MODE_MONO),
		                   NULL);

	caps = GST_BASE_SRC_CLASS (parent_class)->fixate (bsrc, caps);

	return caps;
}

static void
gst_m2svideosrc_set_property (GObject * object, guint prop_id,
                              const GValue * value, GParamSpec * pspec)
{
	GstM2svideosrc *p_m2svideosrc = GST_M2SVIDEOSRC (object);

	switch (prop_id) {
	case PROP_TIMESTAMP_OFFSET:
		p_m2svideosrc->timestamp_offset = g_value_get_int64 (value);
		break;
	case PROP_IS_LIVE:
		gst_base_src_set_live (GST_BASE_SRC (p_m2svideosrc), g_value_get_boolean (value));
		break;

	case PROP_HW_HITLESS:
		gst_m2svideosrc_set_hw_hitless (p_m2svideosrc, g_value_get_boolean (value));
		break;
	case PROP_GPU_NUM:
		gst_m2svideosrc_set_gpu_num (p_m2svideosrc, g_value_get_uint (value));
		break;
	case PROP_L2_CPU_NUM:
		gst_m2svideosrc_set_l2_cpu_num (p_m2svideosrc, g_value_get_int (value));
		break;
	case PROP_L1_CPU_NUM:
		gst_m2svideosrc_set_l1_cpu_num (p_m2svideosrc, g_value_get_int (value));
		break;
	case PROP_P_IF_ADDRESS:
		gst_m2svideosrc_set_p_if_address (p_m2svideosrc, g_value_get_string (value));
		break;
	case PROP_S_IF_ADDRESS:
		gst_m2svideosrc_set_s_if_address (p_m2svideosrc, g_value_get_string (value));
		break;
	case PROP_P_DST_ADDRESS:
		gst_m2svideosrc_set_p_dst_address (p_m2svideosrc, g_value_get_string (value));
		break;
	case PROP_S_DST_ADDRESS:
		gst_m2svideosrc_set_s_dst_address (p_m2svideosrc, g_value_get_string (value));
		break;
	case PROP_P_SRC_ADDRESS:
		gst_m2svideosrc_set_p_src_address (p_m2svideosrc, g_value_get_string (value));
		break;
	case PROP_S_SRC_ADDRESS:
		gst_m2svideosrc_set_s_src_address (p_m2svideosrc, g_value_get_string (value));
		break;
	case PROP_P_DST_PORT:
		gst_m2svideosrc_set_p_dst_port (p_m2svideosrc, g_value_get_uint (value));
		break;
	case PROP_S_DST_PORT:
		gst_m2svideosrc_set_s_dst_port (p_m2svideosrc, g_value_get_uint (value));
		break;
	case PROP_P_SRC_PORT:
		gst_m2svideosrc_set_p_src_port (p_m2svideosrc, g_value_get_uint (value));
		break;
	case PROP_S_SRC_PORT:
		gst_m2svideosrc_set_s_src_port (p_m2svideosrc, g_value_get_uint (value));
		break;
	case PROP_PLAYOUT_DELAY_MS:
		gst_m2svideosrc_set_playout_delay_ms (p_m2svideosrc, g_value_get_int (value));
		break;
	case PROP_DEBUG_MESSAGE_INTERVAL:
		gst_m2svideosrc_set_debug_message_interval (p_m2svideosrc, g_value_get_uint (value));
		break;
	case PROP_SCAN:
		gst_m2svideosrc_set_scan (p_m2svideosrc, g_value_get_uint (value));
		break;
	case PROP_PAYLOAD_TYPE:
		gst_m2svideosrc_set_payload_type (p_m2svideosrc, g_value_get_uint (value));
		break;
	case PROP_RTP_FORMAT:
		gst_m2svideosrc_set_rtp_format (p_m2svideosrc, (m2s_video_rtp_format_t)g_value_get_enum (value));
		break;
	case PROP_IPX_LICENSE:
		gst_m2svideosrc_set_ipx_license (p_m2svideosrc, g_value_get_string (value));
		break;
	case PROP_BOX_MODE:
		gst_m2svideosrc_set_box_mode (p_m2svideosrc, g_value_get_boolean (value));
		break;
	case PROP_BOX_SIZE:
		gst_m2svideosrc_set_box_size (p_m2svideosrc, g_value_get_uint (value));
		break;
	case PROP_ASYNC_RTP_TIMESTAMP:
		gst_m2svideosrc_set_async_rtp_timestamp (p_m2svideosrc, g_value_get_boolean (value));
		break;
	case PROP_FIFO_UNDER_THRESHOLD:
		gst_m2svideosrc_set_fifo_under_threshold (p_m2svideosrc, g_value_get_uint (value));
		break;
	case PROP_FIFO_MIDDLE:
		gst_m2svideosrc_set_fifo_middle (p_m2svideosrc, g_value_get_uint (value));
		break;
	case PROP_FIFO_OVER_THRESHOLD:
		gst_m2svideosrc_set_fifo_over_threshold (p_m2svideosrc, g_value_get_uint (value));
		break;
	case PROP_UNDER_COUNT_MAX:
		gst_m2svideosrc_set_under_count_max (p_m2svideosrc, g_value_get_uint (value));
		break;
	case PROP_GPUDIRECT:
		gst_m2svideosrc_set_gpudirect (p_m2svideosrc, g_value_get_boolean (value));
		break;

	default:
		break;
	}
}

static void
gst_m2svideosrc_get_property (GObject * object, guint prop_id,
                              GValue * value, GParamSpec * pspec)
{
	GstM2svideosrc *p_m2svideosrc = GST_M2SVIDEOSRC (object);

	switch (prop_id) {
	case PROP_TIMESTAMP_OFFSET:
		g_value_set_int64 (value, p_m2svideosrc->timestamp_offset);
		break;
	case PROP_IS_LIVE:
		g_value_set_boolean (value, gst_base_src_is_live (GST_BASE_SRC (p_m2svideosrc)));
		break;

	case PROP_HW_HITLESS:
		g_value_set_boolean (value, p_m2svideosrc->hw_hitless);
		break;
	case PROP_GPU_NUM:
		g_value_set_uint (value, p_m2svideosrc->gpu_num);
		break;
	case PROP_L2_CPU_NUM:
		g_value_set_int (value, p_m2svideosrc->l2_cpu_num);
		break;
	case PROP_L1_CPU_NUM:
		g_value_set_int (value, p_m2svideosrc->l1_cpu_num);
		break;
	case PROP_P_IF_ADDRESS:
		g_value_set_string (value, p_m2svideosrc->if_ip[0]);
		break;
	case PROP_S_IF_ADDRESS:
		g_value_set_string (value, p_m2svideosrc->if_ip[1]);
		break;
	case PROP_P_DST_ADDRESS:
		g_value_set_string (value, p_m2svideosrc->dst_ip[0]);
		break;
	case PROP_S_DST_ADDRESS:
		g_value_set_string (value, p_m2svideosrc->dst_ip[1]);
		break;
	case PROP_P_SRC_ADDRESS:
		g_value_set_string (value, p_m2svideosrc->src_ip[0]);
		break;
	case PROP_S_SRC_ADDRESS:
		g_value_set_string (value, p_m2svideosrc->src_ip[1]);
		break;
	case PROP_P_DST_PORT:
		g_value_set_uint (value, p_m2svideosrc->dst_port[0]);
		break;
	case PROP_S_DST_PORT:
		g_value_set_uint (value, p_m2svideosrc->dst_port[1]);
		break;
	case PROP_P_SRC_PORT:
		g_value_set_uint (value, p_m2svideosrc->src_port[0]);
		break;
	case PROP_S_SRC_PORT:
		g_value_set_uint (value, p_m2svideosrc->src_port[1]);
		break;
	case PROP_PLAYOUT_DELAY_MS:
		g_value_set_int (value, p_m2svideosrc->playout_delay_ms);
		break;
	case PROP_DEBUG_MESSAGE_INTERVAL:
		g_value_set_uint (value, p_m2svideosrc->debug_message_interval);
		break;
	case PROP_SCAN:
		g_value_set_uint (value, p_m2svideosrc->scan);
		break;
	case PROP_PAYLOAD_TYPE:
		g_value_set_uint (value, p_m2svideosrc->payload_type);
		break;
	case PROP_RTP_FORMAT:
		g_value_set_enum (value, p_m2svideosrc->rtp_format);
		break;
	case PROP_IPX_LICENSE:
		g_value_set_string (value, p_m2svideosrc->ipx_license);
		break;
	case PROP_BOX_MODE:
		g_value_set_boolean (value, p_m2svideosrc->box_mode);
		break;
	case PROP_BOX_SIZE:
		g_value_set_uint (value, p_m2svideosrc->box_size);
		break;
	case PROP_ASYNC_RTP_TIMESTAMP:
		g_value_set_boolean (value, p_m2svideosrc->async_rtp_timestamp);
		break;
	case PROP_FIFO_UNDER_THRESHOLD:
		g_value_set_uint (value, p_m2svideosrc->fifo_under_threshold);
		break;
	case PROP_FIFO_MIDDLE:
		g_value_set_uint (value, p_m2svideosrc->fifo_middle);
		break;
	case PROP_FIFO_OVER_THRESHOLD:
		g_value_set_uint (value, p_m2svideosrc->fifo_over_threshold);
		break;
	case PROP_UNDER_COUNT_MAX:
		g_value_set_uint (value, p_m2svideosrc->under_count_max);
		break;
	case PROP_GPUDIRECT:
		g_value_set_boolean (value, p_m2svideosrc->gpudirect);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static GstStateChangeReturn
gst_m2svideosrc_change_state (GstElement * element, GstStateChange transition)
{
	GstM2svideosrc *p_m2svideosrc = GST_M2SVIDEOSRC (element);
	GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

	switch (transition)
	{
	case GST_STATE_CHANGE_NULL_TO_READY:

		p_m2svideosrc->p_frame_from_m2s = nullptr;
		p_m2svideosrc->under_count = 0;

		m2s_open_conf_t open_conf;
		open_conf.cuda_dev_num = p_m2svideosrc->gpu_num;
		open_conf.p_ipx_license_file = p_m2svideosrc->ipx_license;
		m2s_open(&open_conf);

		m2s_cpu_affinity_t cpu_affinity;
		cpu_affinity.rx.l2_num = p_m2svideosrc->l2_cpu_num;
		cpu_affinity.rx.l1_num = p_m2svideosrc->l1_cpu_num;
		m2s_create(&p_m2svideosrc->strm_id, M2S_IO_TYPE_RX, M2S_MEDIA_TYPE_VIDEO, M2S_MEMORY_MODE_CPU, &cpu_affinity, NULL, p_m2svideosrc->hw_hitless);

		break;

	case GST_STATE_CHANGE_READY_TO_PAUSED:
		break;

	case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
		m2s_start(p_m2svideosrc->strm_id);
		start_monitoring_timer(p_m2svideosrc);
		break;

	case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
		stop_monitoring_timer(p_m2svideosrc);
		m2s_stop(p_m2svideosrc->strm_id);
		break;

	case GST_STATE_CHANGE_PAUSED_TO_READY:
		break;

	case GST_STATE_CHANGE_READY_TO_NULL:
		if (p_m2svideosrc->p_frame_from_m2s)
		{
			m2s_free_read_ptr(p_m2svideosrc->strm_id);
			p_m2svideosrc->p_frame_from_m2s = nullptr;
		}

		m2s_delete(p_m2svideosrc->strm_id);
		//m2s_close();
		break;

	default:
		break;
	}

	ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
	return ret;
}

static gboolean
gst_m2svideosrc_decide_allocation (GstBaseSrc * bsrc, GstQuery * query)
{
	GstM2svideosrc *m2svideosrc;
	GstBufferPool *pool;
	gboolean update;
	guint size, min, max;
	GstStructure *config;
	GstCaps *caps = NULL;

	m2svideosrc = GST_M2SVIDEOSRC (bsrc);

	if (gst_query_get_n_allocation_pools (query) > 0) {
		gst_query_parse_nth_allocation_pool (query, 0, &pool, &size, &min, &max);

		/* adjust size */
		size = MAX (size, m2svideosrc->info.size);
		update = TRUE;
	} else {
		pool = NULL;
		size = m2svideosrc->info.size;
		min = max = 0;
		update = FALSE;
	}

	/* no downstream pool, make our own */
	if (pool == NULL) {
		pool = gst_video_buffer_pool_new ();
	}

	config = gst_buffer_pool_get_config (pool);

	gst_query_parse_allocation (query, &caps, NULL);
	if (caps)
		gst_buffer_pool_config_set_params (config, caps, size, min, max);

	if (gst_query_find_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL)) {
		gst_buffer_pool_config_add_option (config,
		                                   GST_BUFFER_POOL_OPTION_VIDEO_META);
	}
	gst_buffer_pool_set_config (pool, config);

	if (update)
		gst_query_set_nth_allocation_pool (query, 0, pool, size, min, max);
	else
		gst_query_add_allocation_pool (query, pool, size, min, max);

	if (pool)
		gst_object_unref (pool);

	return GST_BASE_SRC_CLASS (parent_class)->decide_allocation (bsrc, query);
}

static inline void set_m2s_conf(GstM2svideosrc *p_m2svideosrc)
{
	m2s_sys_conf_t sys_conf;
	m2s_media_conf_t media_conf;
	m2s_ip_conf_t ip_conf;
	memset(&sys_conf, 0, sizeof(sys_conf));
	memset(&media_conf, 0, sizeof(media_conf));
	memset(&ip_conf, 0, sizeof(ip_conf));

	for (int i = 0; i < 2; i++)
	{
		ip_conf.rx_only.if_ip[i] = m2s_conv_ip_address_from_string(p_m2svideosrc->if_ip[i]);
		DBG_MSG("if_ip[%u]=%s\n", i, p_m2svideosrc->if_ip[i]);
		ip_conf.dst_ip[i] = m2s_conv_ip_address_from_string(p_m2svideosrc->dst_ip[i]);
		DBG_MSG("dst_ip[%u]=%s\n", i, p_m2svideosrc->dst_ip[i]);
		ip_conf.src_ip[i] = m2s_conv_ip_address_from_string(p_m2svideosrc->src_ip[i]);
		DBG_MSG("src_ip[%u]=%s\n", i, p_m2svideosrc->src_ip[i]);
		ip_conf.dst_port[i] = p_m2svideosrc->dst_port[i];
		DBG_MSG("dst_port[%u]=%u\n", i, ip_conf.dst_port[i]);
		ip_conf.src_port[i] = p_m2svideosrc->src_port[i];
		DBG_MSG("src_port[%u]=%u\n", i, ip_conf.src_port[i]);
		ip_conf.payload_type[i] = p_m2svideosrc->payload_type;
		DBG_MSG("payload_type[%u]=%u\n", i, ip_conf.payload_type[i]);
		ip_conf.rtp_enabled[i] = (ip_conf.rx_only.if_ip[i] == 0) ? false : true;
	}
	ip_conf.rx_only.playout_delay_ms = p_m2svideosrc->playout_delay_ms;

	if ((GST_VIDEO_INFO_WIDTH(&p_m2svideosrc->info) == 3840) &&
		(GST_VIDEO_INFO_HEIGHT(&p_m2svideosrc->info) == 2160))
	{
		media_conf.video.app_caps.resolution = M2S_VIDEO_RESOLUTION_3840x2160;
	}
	else if ((GST_VIDEO_INFO_WIDTH(&p_m2svideosrc->info) == 1920) &&
			 (GST_VIDEO_INFO_HEIGHT(&p_m2svideosrc->info) == 1080))
	{
		media_conf.video.app_caps.resolution = M2S_VIDEO_RESOLUTION_1920x1080;
	}
	else
	{
		DBG_MSG("!!! unsupported video resolution !!!\n");
	}

	if (GST_VIDEO_INFO_FORMAT(&p_m2svideosrc->info) == GST_VIDEO_FORMAT_I420)
	{
		DBG_MSG("GST_VIDEO_FORMAT_I420\n");
		media_conf.video.app_caps.format = M2S_VIDEO_APP_FORMAT_I420;
	}
	else if (GST_VIDEO_INFO_FORMAT(&p_m2svideosrc->info) == GST_VIDEO_FORMAT_UYVP)
	{
		DBG_MSG("GST_VIDEO_FORMAT_UYVP\n");
		media_conf.video.app_caps.format = M2S_VIDEO_APP_FORMAT_UYVP;
	}
	else if (GST_VIDEO_INFO_FORMAT(&p_m2svideosrc->info) == GST_VIDEO_FORMAT_UYVY)
	{
		DBG_MSG("GST_VIDEO_FORMAT_UYVY\n");
		media_conf.video.app_caps.format = M2S_VIDEO_APP_FORMAT_UYVY;
	}
	else if (GST_VIDEO_INFO_FORMAT(&p_m2svideosrc->info) == GST_VIDEO_FORMAT_v210)
	{
		DBG_MSG("GST_VIDEO_FORMAT_v210\n");
		media_conf.video.app_caps.format = M2S_VIDEO_APP_FORMAT_V210;
	}
	else if (GST_VIDEO_INFO_FORMAT(&p_m2svideosrc->info) == GST_VIDEO_FORMAT_BGRx)
	{
		DBG_MSG("GST_VIDEO_FORMAT_BGRx\n");
		media_conf.video.app_caps.format = M2S_VIDEO_APP_FORMAT_BGRx;
	}
	else
	{
		DBG_MSG("!!! unknown video format !!!\n");
	}

	if ((GST_VIDEO_INFO_FPS_N(&p_m2svideosrc->info) == 60000) &&
		(GST_VIDEO_INFO_FPS_D(&p_m2svideosrc->info) == 1001))
	{
		DBG_MSG("M2S_FRAME_RATE_60000_1001\n");
		media_conf.video.app_caps.frame_rate = M2S_FRAME_RATE_60000_1001;
	}
	else if ((GST_VIDEO_INFO_FPS_N(&p_m2svideosrc->info) == 30000) &&
			 (GST_VIDEO_INFO_FPS_D(&p_m2svideosrc->info) == 1001))
	{
		DBG_MSG("M2S_FRAME_RATE_30000_1001\n");
		media_conf.video.app_caps.frame_rate = M2S_FRAME_RATE_30000_1001;
	}
	else if ((GST_VIDEO_INFO_FPS_N(&p_m2svideosrc->info) == 50) &&
			 (GST_VIDEO_INFO_FPS_D(&p_m2svideosrc->info) == 1))
	{
		DBG_MSG("M2S_FRAME_RATE_50_1\n");
		media_conf.video.app_caps.frame_rate = M2S_FRAME_RATE_50_1;
	}
	else if ((GST_VIDEO_INFO_FPS_N(&p_m2svideosrc->info) == 25) &&
			 (GST_VIDEO_INFO_FPS_D(&p_m2svideosrc->info) == 1))
	{
		DBG_MSG("M2S_FRAME_RATE_25_1\n");
		media_conf.video.app_caps.frame_rate = M2S_FRAME_RATE_25_1;
	}
	else
	{
		DBG_MSG("!!! unsupported framerate !!!\n");
	}

	media_conf.video.rtp_caps.format = p_m2svideosrc->rtp_format;
	media_conf.video.rtp_caps.scan = (m2s_video_scan_t)p_m2svideosrc->scan;
	media_conf.video.rtp_caps.frame_rate = media_conf.video.app_caps.frame_rate;
	media_conf.video.rtp_caps.resolution = media_conf.video.app_caps.resolution;
	media_conf.video.rtp_caps.box_mode = p_m2svideosrc->box_mode;
	//DBG_MSG("box_mode: %d\n", media_conf.video.rtp_caps.box_mode);
	media_conf.video.rtp_caps.box_size = p_m2svideosrc->box_size;
	//DBG_MSG("box_size: %d\n", media_conf.video.rtp_caps.box_size);
	media_conf.video.rtp_caps.target_bpp = 0; // TX only

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

	sys_conf.async_rx_rtp_timestamp = p_m2svideosrc->async_rtp_timestamp;
	sys_conf.use_gpu_direct = p_m2svideosrc->gpudirect;
	m2s_set_sys_conf(p_m2svideosrc->strm_id, &sys_conf);

	m2s_set_media_conf(p_m2svideosrc->strm_id, &media_conf);
	m2s_set_ip_conf(p_m2svideosrc->strm_id, &ip_conf);
}

static gboolean
gst_m2svideosrc_setcaps (GstBaseSrc * bsrc, GstCaps * caps)
{
	const GstStructure *structure;
	GstM2svideosrc *p_m2svideosrc;
	GstVideoInfo info;
	guint i;
	guint n_lines;
	gint offset;

	p_m2svideosrc = GST_M2SVIDEOSRC (bsrc);

	structure = gst_caps_get_structure (caps, 0);

	GST_OBJECT_LOCK (p_m2svideosrc);

	if (gst_structure_has_name (structure, "video/x-raw")) {
		/* we can use the parsing code */
		if (!gst_video_info_from_caps (&info, caps))
			goto parse_failed;

	} else {
		goto unsupported_caps;
	}

	/* create chroma subsampler */
	if (p_m2svideosrc->subsample)
		gst_video_chroma_resample_free (p_m2svideosrc->subsample);
	p_m2svideosrc->subsample = gst_video_chroma_resample_new ((GstVideoChromaMethod)0,
	                                                          info.chroma_site, (GstVideoChromaFlags)0, info.finfo->unpack_format, -info.finfo->w_sub[2],
	                                                          -info.finfo->h_sub[2]);

	for (i = 0; i < p_m2svideosrc->n_lines; i++)
		g_free (p_m2svideosrc->lines[i]);
	g_free (p_m2svideosrc->lines);

	if (p_m2svideosrc->subsample != NULL) {
		gst_video_chroma_resample_get_info (p_m2svideosrc->subsample,
		                                    &n_lines, &offset);
	} else {
		n_lines = 1;
		offset = 0;
	}

	p_m2svideosrc->lines = (void**)g_malloc (sizeof (gpointer) * n_lines);
	for (i = 0; i < n_lines; i++)
		p_m2svideosrc->lines[i] = g_malloc ((info.width + 16) * 8);
	p_m2svideosrc->n_lines = n_lines;
	p_m2svideosrc->offset = offset;

	/* looks ok here */
	p_m2svideosrc->info = info;

	GST_DEBUG_OBJECT (p_m2svideosrc, "size %dx%d, %d/%d fps",
	                  info.width, info.height, info.fps_n, info.fps_d);

	p_m2svideosrc->accum_rtime += p_m2svideosrc->running_time;
	p_m2svideosrc->accum_frames += p_m2svideosrc->n_frames;

	p_m2svideosrc->running_time = 0;
	p_m2svideosrc->n_frames = 0;

	set_m2s_conf(p_m2svideosrc);

	GST_OBJECT_UNLOCK (p_m2svideosrc);

	return TRUE;

	/* ERRORS */
 parse_failed:
	{
		GST_OBJECT_UNLOCK (p_m2svideosrc);
		GST_DEBUG_OBJECT (bsrc, "failed to parse caps");
		return FALSE;
	}
 unsupported_caps:
	{
		GST_OBJECT_UNLOCK (p_m2svideosrc);
		GST_DEBUG_OBJECT (bsrc, "unsupported caps: %" GST_PTR_FORMAT, caps);
		return FALSE;
	}
}

static gboolean
gst_m2svideosrc_query (GstBaseSrc * bsrc, GstQuery * query)
{
	gboolean res = FALSE;
	GstM2svideosrc *src;

	src = GST_M2SVIDEOSRC (bsrc);

	switch (GST_QUERY_TYPE (query)) {
	case GST_QUERY_CONVERT:
	{
		GstFormat src_fmt, dest_fmt;
		gint64 src_val, dest_val;

		GST_OBJECT_LOCK (src);
		gst_query_parse_convert (query, &src_fmt, &src_val, &dest_fmt, &dest_val);
		res =
			gst_video_info_convert (&src->info, src_fmt, src_val, dest_fmt,
			                        &dest_val);
		gst_query_set_convert (query, src_fmt, src_val, dest_fmt, dest_val);
		GST_OBJECT_UNLOCK (src);
		break;
	}
	case GST_QUERY_LATENCY:
	{
		GST_OBJECT_LOCK (src);
		if (src->info.fps_n > 0) {
			GstClockTime latency;

			latency =
				gst_util_uint64_scale (GST_SECOND, src->info.fps_d,
				                       src->info.fps_n);
			GST_OBJECT_UNLOCK (src);
			gst_query_set_latency (query,
			                       gst_base_src_is_live (GST_BASE_SRC_CAST (src)), latency,
			                       GST_CLOCK_TIME_NONE);
			GST_DEBUG_OBJECT (src, "Reporting latency of %" GST_TIME_FORMAT,
			                  GST_TIME_ARGS (latency));
			res = TRUE;
		} else {
			GST_OBJECT_UNLOCK (src);
		}
		break;
	}
	case GST_QUERY_DURATION:{
		if (bsrc->num_buffers != -1) {
			GstFormat format;

			gst_query_parse_duration (query, &format, NULL);
			switch (format) {
			case GST_FORMAT_TIME:{
				gint64 dur;

				GST_OBJECT_LOCK (src);
				if (src->info.fps_n) {
					dur = gst_util_uint64_scale_int_round (bsrc->num_buffers
					                                       * GST_SECOND, src->info.fps_d, src->info.fps_n);
					res = TRUE;
					gst_query_set_duration (query, GST_FORMAT_TIME, dur);
				}
				GST_OBJECT_UNLOCK (src);
				goto done;
			}
			case GST_FORMAT_BYTES:
				GST_OBJECT_LOCK (src);
				res = TRUE;
				gst_query_set_duration (query, GST_FORMAT_BYTES,
										bsrc->num_buffers * src->info.size);
				GST_OBJECT_UNLOCK (src);
				goto done;
			default:
				break;
			}
		}
		/* fall through */
	}
	default:
		res = GST_BASE_SRC_CLASS (parent_class)->query (bsrc, query);
		break;
	}
 done:
	return res;
}

static void
gst_m2svideosrc_get_times (GstBaseSrc * basesrc, GstBuffer * buffer,
                           GstClockTime * start, GstClockTime * end)
{
	/* for live sources, sync on the timestamp of the buffer */
	if (gst_base_src_is_live (basesrc)) {
		GstClockTime timestamp = GST_BUFFER_PTS (buffer);

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
gst_m2svideosrc_do_seek (GstBaseSrc * bsrc, GstSegment * segment)
{
	GstClockTime position;
	GstM2svideosrc *src;

	src = GST_M2SVIDEOSRC (bsrc);

	segment->time = segment->start;
	position = segment->position;
	src->reverse = segment->rate < 0;

	/* now move to the position indicated */
	if (src->info.fps_n) {
		src->n_frames = gst_util_uint64_scale (position,
		                                       src->info.fps_n, src->info.fps_d * GST_SECOND);
	} else {
		src->n_frames = 0;
	}
	src->accum_frames = 0;
	src->accum_rtime = 0;
	if (src->info.fps_n) {
		src->running_time = gst_util_uint64_scale (src->n_frames,
		                                           src->info.fps_d * GST_SECOND, src->info.fps_n);
	} else {
		/* FIXME : Not sure what to set here */
		src->running_time = 0;
	}

	g_assert (src->running_time <= position);

	return TRUE;
}

static gboolean
gst_m2svideosrc_is_seekable (GstBaseSrc * psrc)
{
	/* we're seekable... */
	return FALSE;
}

static inline void get_ptr_m2s(GstM2svideosrc *p_m2svideosrc)
{
	m2s_media_t media;
	m2s_media_size_t size;
	m2s_get_read_ptr(p_m2svideosrc->strm_id, nullptr, &media, &size);
	p_m2svideosrc->p_frame_from_m2s = media.video.p_frame;
	p_m2svideosrc->frame_size_from_m2s = size.video.frame_size;
}

static inline void free_and_get_ptr_m2s(GstM2svideosrc *p_m2svideosrc)
{
	m2s_free_read_ptr(p_m2svideosrc->strm_id);
	get_ptr_m2s(p_m2svideosrc);
}

static inline void read_from_m2s(GstM2svideosrc *p_m2svideosrc, GstVideoFrame *p_frame)
{
	uint8_t *p_gst_dst = (uint8_t *)GST_VIDEO_FRAME_PLANE_DATA (p_frame, 0);
	uint32_t gst_size = (uint32_t)GST_VIDEO_FRAME_SIZE(p_frame);
	m2s_status_t status;
	bool write_m2s = false;
	bool inc_under = false;

	if (m2s_get_status(p_m2svideosrc->strm_id, &status, false) == M2S_RET_SUCCESS)
	{
		if (p_m2svideosrc->p_frame_from_m2s == nullptr)
		{
			if (status.rx.app_fifo_stored >= p_m2svideosrc->fifo_middle)
			{
				get_ptr_m2s(p_m2svideosrc);
				write_m2s = true;
			}
		}
		else
		{
			if (status.rx.app_fifo_stored > p_m2svideosrc->fifo_over_threshold)
			{
				free_and_get_ptr_m2s(p_m2svideosrc);
				free_and_get_ptr_m2s(p_m2svideosrc);
			}
			else if (status.rx.app_fifo_stored >= p_m2svideosrc->fifo_under_threshold)
			{
				free_and_get_ptr_m2s(p_m2svideosrc);
			}
			else
			{
				inc_under = true;
			}
			write_m2s = true;
		}

		if (inc_under)
		{
			p_m2svideosrc->under_count++;
		}
		else
		{
			p_m2svideosrc->under_count = 0;
		}

		if ((p_m2svideosrc->p_frame_from_m2s != nullptr) &&
			((gst_size != p_m2svideosrc->frame_size_from_m2s) || (p_m2svideosrc->under_count >= p_m2svideosrc->under_count_max)))
		{
			m2s_free_read_ptr(p_m2svideosrc->strm_id);
			p_m2svideosrc->p_frame_from_m2s = nullptr;
			p_m2svideosrc->under_count = 0;
			write_m2s = false;
		}
	}

	if (write_m2s)
	{
		memcpy(p_gst_dst, p_m2svideosrc->p_frame_from_m2s, gst_size);
	}
	else
	{
		memset(p_gst_dst, 0, gst_size);
	}
}

static GstFlowReturn
gst_m2svideosrc_fill (GstPushSrc * psrc, GstBuffer * buffer)
{
	GstM2svideosrc *src;
	GstClockTime next_time;
	GstVideoFrame frame;

	src = GST_M2SVIDEOSRC (psrc);

	if (G_UNLIKELY (GST_VIDEO_INFO_FORMAT (&src->info) ==
	                GST_VIDEO_FORMAT_UNKNOWN))
		goto not_negotiated;

	/* 0 framerate and we are at the second frame, eos */
	if (G_UNLIKELY (src->info.fps_n == 0 && src->n_frames == 1))
		goto eos;

	if (G_UNLIKELY (src->n_frames == -1)) {
		/* EOS for reverse playback */
		goto eos;
	}

	GST_LOG_OBJECT (src,
	                "creating buffer from pool for frame %" G_GINT64_FORMAT, src->n_frames);

	if (!gst_video_frame_map (&frame, &src->info, buffer, GST_MAP_WRITE))
		goto invalid_frame;

	GST_BUFFER_PTS (buffer) =
		src->accum_rtime + src->timestamp_offset + src->running_time;
	GST_BUFFER_DTS (buffer) = GST_CLOCK_TIME_NONE;

	gst_object_sync_values (GST_OBJECT (psrc), GST_BUFFER_PTS (buffer));

	read_from_m2s(src, &frame);

	gst_video_frame_unmap (&frame);

	GST_DEBUG_OBJECT (src, "Timestamp: %" GST_TIME_FORMAT " = accumulated %"
	                  GST_TIME_FORMAT " + offset: %"
	                  GST_TIME_FORMAT " + running time: %" GST_TIME_FORMAT,
	                  GST_TIME_ARGS (GST_BUFFER_PTS (buffer)), GST_TIME_ARGS (src->accum_rtime),
	                  GST_TIME_ARGS (src->timestamp_offset), GST_TIME_ARGS (src->running_time));

	GST_BUFFER_OFFSET (buffer) = src->accum_frames + src->n_frames;
	if (src->reverse) {
		src->n_frames--;
	} else {
		src->n_frames++;
	}
	GST_BUFFER_OFFSET_END (buffer) = GST_BUFFER_OFFSET (buffer) + 1;
	if (src->info.fps_n) {
		next_time = gst_util_uint64_scale (src->n_frames,
		                                   src->info.fps_d * GST_SECOND, src->info.fps_n);
		if (src->reverse) {
			/* We already decremented to next frame */
			GstClockTime prev_pts = gst_util_uint64_scale (src->n_frames + 2,
			                                               src->info.fps_d * GST_SECOND, src->info.fps_n);

			GST_BUFFER_DURATION (buffer) = prev_pts - GST_BUFFER_PTS (buffer);
		} else {
			GST_BUFFER_DURATION (buffer) = next_time - src->running_time;
		}
	} else {
		next_time = src->timestamp_offset;
		/* NONE means forever */
		GST_BUFFER_DURATION (buffer) = GST_CLOCK_TIME_NONE;
	}

	src->running_time = next_time;

	return GST_FLOW_OK;

 not_negotiated:
	{
		return GST_FLOW_NOT_NEGOTIATED;
	}
 eos:
	{
		GST_DEBUG_OBJECT (src, "eos: 0 framerate, frame %d", (gint) src->n_frames);
		return GST_FLOW_EOS;
	}
 invalid_frame:
	{
		GST_DEBUG_OBJECT (src, "invalid frame");
		return GST_FLOW_OK;
	}
}

static gboolean
gst_m2svideosrc_start (GstBaseSrc * basesrc)
{
	GstM2svideosrc *src = GST_M2SVIDEOSRC (basesrc);

	GST_OBJECT_LOCK (src);
	src->running_time = 0;
	src->n_frames = 0;
	src->accum_frames = 0;
	src->accum_rtime = 0;

	gst_video_info_init (&src->info);
	GST_OBJECT_UNLOCK (src);

	return TRUE;
}

static gboolean
gst_m2svideosrc_stop (GstBaseSrc * basesrc)
{
	GstM2svideosrc *src = GST_M2SVIDEOSRC (basesrc);
	guint i;

	if (src->subsample)
		gst_video_chroma_resample_free (src->subsample);
	src->subsample = NULL;

	for (i = 0; i < src->n_lines; i++)
		g_free (src->lines[i]);
	g_free (src->lines);
	src->n_lines = 0;
	src->lines = NULL;

	return TRUE;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
	GST_DEBUG_CATEGORY_INIT (m2svideosrc_debug, "m2svideosrc", 0,
	                         "Video Test Source");

	return gst_element_register (plugin, "m2svideosrc",
	                             GST_RANK_NONE, GST_TYPE_M2SVIDEOSRC);
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
                   m2svideosrc,
                   "FIXME plugin description",
                   plugin_init, VERSION, GST_LICENSE_UNKNOWN, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
