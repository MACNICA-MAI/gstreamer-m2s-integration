//==============================================================================
// Copyright (C) 2021 Macnica Inc. All Rights Reserved.
//
// This program is proprietary and confidential. By using this program
// you agree to the terms of the associated Macnica Software License Agreement.
//------------------------------------------------------------------------------
//! @file
//! @brief
//==============================================================================
#if !defined(__M2S_TYPES_H__)
#include <stdint.h>
#define __M2S_TYPES_H__
#if !defined(__cplusplus)
#include <stdbool.h>
#endif

//------------------------------------------------------------------------------
// define and structures
//------------------------------------------------------------------------------
#define M2S_RET_CORE_TYPE          (11 << 28)
#define M2S_RET_ERR_TYPE_INT       (M2S_RET_CORE_TYPE | (1 << 24))

#define M2S_RET_SUCCESS            (0)

#define M2S_RET_NOT_OPEN           (M2S_RET_ERR_TYPE_INT | (0x01 << 8))
#define M2S_RET_ALREADY_OPEN       (M2S_RET_ERR_TYPE_INT | (0x02 << 8))
#define M2S_RET_INVALID_STRM_ID    (M2S_RET_ERR_TYPE_INT | (0x03 << 8))
#define M2S_RET_NOT_SET_CONF       (M2S_RET_ERR_TYPE_INT | (0x04 << 8))
#define M2S_RET_NOT_START          (M2S_RET_ERR_TYPE_INT | (0x05 << 8))
#define M2S_RET_ALREADY_START      (M2S_RET_ERR_TYPE_INT | (0x06 << 8))
#define M2S_RET_INVALID_OPERATION  (M2S_RET_ERR_TYPE_INT | (0x07 << 8))
#define M2S_RET_INVALID_PARAMETER  (M2S_RET_ERR_TYPE_INT | (0x08 << 8))
#define M2S_RET_OVER_SIZE          (M2S_RET_ERR_TYPE_INT | (0x09 << 8))
#define M2S_RET_UNDER_SIZE         (M2S_RET_ERR_TYPE_INT | (0x0a << 8))
#define M2S_RET_BUFFER_IS_FULL     (M2S_RET_ERR_TYPE_INT | (0x0b << 8))
#define M2S_RET_BUFFER_IS_EMPTY    (M2S_RET_ERR_TYPE_INT | (0x0c << 8))
#define M2S_RET_INTERNAL_ERR       (M2S_RET_ERR_TYPE_INT | (0x0d << 8))
#define M2S_RET_CPU_AFFINITY       (M2S_RET_ERR_TYPE_INT | (0x0e << 8))
#define M2S_RET_NOT_SUPPORTED      (M2S_RET_ERR_TYPE_INT | (0x0f << 8))
#define M2S_RET_DISABLED           (M2S_RET_ERR_TYPE_INT | (0x10 << 8))
#define M2S_RET_MEMORY             (M2S_RET_ERR_TYPE_INT | (0x11 << 8))
#define M2S_RET_NO_RESOURCE        (M2S_RET_ERR_TYPE_INT | (0x12 << 8))
#define M2S_RET_CUDA_INIT_ERR      (M2S_RET_ERR_TYPE_INT | (0x13 << 8))
#define M2S_RET_INVALID_LICENSE    (M2S_RET_ERR_TYPE_INT | (0x14 << 8))
#define M2S_RET_SYSTEM_ERR         (M2S_RET_ERR_TYPE_INT | (0x15 << 8))
#define M2S_RET_MAX_STRM_NUM       (M2S_RET_ERR_TYPE_INT | (0x16 << 8))
#define M2S_RET_ALREADY_USED       (M2S_RET_ERR_TYPE_INT | (0x17 << 8))
#define M2S_RET_NOT_SWITCH_CONF    (M2S_RET_ERR_TYPE_INT | (0x18 << 8))
#define M2S_RET_INVALID_ADDR       (M2S_RET_ERR_TYPE_INT | (0x19 << 8))


typedef struct
{
	uint8_t major;
	uint8_t minor;
	uint8_t sub_minor;
} m2s_version_t;

typedef struct
{
	int32_t cuda_dev_num;
	const char *p_ipx_license_file;
} m2s_open_conf_t;

typedef void * m2s_strm_id_t;

typedef enum
{
	M2S_IO_TYPE_TX,
	M2S_IO_TYPE_RX,
} m2s_io_type_t;

typedef enum
{
	M2S_MEDIA_TYPE_VIDEO,
	M2S_MEDIA_TYPE_AUDIO,
	M2S_MEDIA_TYPE_ANC,
	M2S_MEDIA_TYPE_JXSV,
} m2s_media_type_t;

typedef enum
{
	M2S_MEMORY_MODE_CPU,
	M2S_MEMORY_MODE_GPU,
	M2S_MEMORY_MODE_GPU_DIRECT,
} m2s_memory_mode_t;


///////////////////
// IP            //
///////////////////
typedef struct
{
	bool rtp_enabled[2];
	uint32_t dst_ip[2];
	uint32_t src_ip[2];
	uint16_t dst_port[2];
	uint16_t src_port[2];
	uint8_t payload_type[2];
	struct
	{
		uint32_t if_ip[2];
		int32_t playout_delay_ms;
	} rx_only;
} m2s_ip_conf_t;


///////////////////
// Video         //
///////////////////
typedef enum
{
	M2S_MBIT_DIVIDE_MODE_OFF   = 0,
	M2S_MBIT_DIVIDE_MODE_DIV1  = 1,
	M2S_MBIT_DIVIDE_MODE_DIV2  = 2,
	M2S_MBIT_DIVIDE_MODE_DIV4  = 4,
	M2S_MBIT_DIVIDE_MODE_DIV8  = 8,
} m2s_mbit_divide_mode_t;

typedef enum
{
	M2S_VIDEO_APP_FORMAT_UYVP, // YUV422_10bit
	M2S_VIDEO_APP_FORMAT_UYVY, // YUV422_8bit
	M2S_VIDEO_APP_FORMAT_V210,
	M2S_VIDEO_APP_FORMAT_I420,
	M2S_VIDEO_APP_FORMAT_BGRx,
	// M2S_VIDEO_APP_FORMAT_I422,
	// M2S_VIDEO_APP_FORMAT_I420_10LE,
	// M2S_VIDEO_APP_FORMAT_I422_10LE,
} m2s_video_app_format_t;

typedef enum
{
	M2S_VIDEO_RTP_FORMAT_RAW_YUV422_10bit,
	M2S_VIDEO_RTP_FORMAT_JXSV_YUV422_8bit,
	M2S_VIDEO_RTP_FORMAT_JXSV_YUV422_10bit,
	M2S_VIDEO_RTP_FORMAT_JXSV_BGRA_8bit,
} m2s_video_rtp_format_t;

typedef enum
{
	M2S_VIDEO_SCAN_PROGRESSIVE,
	M2S_VIDEO_SCAN_INTERLACE_TFF,
	M2S_VIDEO_SCAN_INTERLACE_BFF,
} m2s_video_scan_t;

typedef enum
{
	M2S_FRAME_RATE_60000_1001,
	M2S_FRAME_RATE_30000_1001,
	M2S_FRAME_RATE_50_1,
	M2S_FRAME_RATE_25_1,
	M2S_FRAME_RATE_60_1,
} m2s_frame_rate_t;

typedef enum
{
	M2S_VIDEO_RESOLUTION_3840x2160,
	M2S_VIDEO_RESOLUTION_1920x1080,
	// M2S_VIDEO_RESOLUTION_1280x720,
} m2s_video_resolution_t;

typedef struct
{
	m2s_video_rtp_format_t format;
	m2s_video_scan_t scan;
	m2s_frame_rate_t frame_rate;
	m2s_video_resolution_t resolution;
	// For Jpeg-XS
	bool box_mode;
	uint8_t box_size;
	float target_bpp; // TX only. About RX, will be configured by parsing headers.
} m2s_video_rtp_caps_t;

typedef struct
{
	m2s_video_app_format_t format;
	m2s_frame_rate_t frame_rate;
	m2s_video_resolution_t resolution;
} m2s_video_app_caps_t;

typedef struct
{
	m2s_video_rtp_caps_t rtp_caps;
	m2s_video_app_caps_t app_caps;
} m2s_media_conf_video_t;

typedef struct
{
	uint8_t *p_frame;
} m2s_media_video_t;

typedef struct
{
	uint32_t frame_size;
} m2s_media_video_size_t;


#define M2S_UYVP_LENGTH ((int)(1920 * 1080 * 2.5))
#define M2S_I422_Y_LENGTH  (1920 * 1080)
#define M2S_I422_CB_LENGTH ((1920 * 1080) / 2)
#define M2S_I422_CR_LENGTH ((1920 * 1080) / 2)

#define M2S_GET_I422_Y_PTR(p_contents) ((p_contents)->p_frame)
#define M2S_GET_I422_CB_PTR(p_contents) (&(((p_contents)->p_frame)[M2S_I422_Y_LENGTH]))
#define M2S_GET_I422_CR_PTR(p_contents) (&(((p_contents)->p_frame)[M2S_I422_Y_LENGTH + M2S_I422_CB_LENGTH]))

#define M2S_I420_Y_LENGTH  (1920 * 1080)
#define M2S_I420_CB_LENGTH ((1920 * 1080) / 4)
#define M2S_I420_CR_LENGTH ((1920 * 1080) / 4)

#define M2S_GET_I420_Y_PTR(p_contents) ((p_contents)->p_frame)
#define M2S_GET_I420_CB_PTR(p_contents) (&(((p_contents)->p_frame)[M2S_I420_Y_LENGTH]))
#define M2S_GET_I420_CR_PTR(p_contents) (&(((p_contents)->p_frame)[M2S_I420_Y_LENGTH + M2S_I420_CB_LENGTH]))

#define M2S_I420_B_LENGTH  (1920 * 1080)
#define M2S_I420_G_LENGTH  (1920 * 1080)
#define M2S_I420_R_LENGTH  (1920 * 1080)

#define M2S_GET_BGRx_B_PTR(p_contents) ((p_contents)->p_frame)
#define M2S_GET_BGRx_G_PTR(p_contents) (&(((p_contents)->p_frame)[M2S_I420_B_LENGTH]))
#define M2S_GET_BGRx_R_PTR(p_contents) (&(((p_contents)->p_frame)[M2S_I420_B_LENGTH + M2S_I420_G_LENGTH]))


///////////////////
// Audio         //
///////////////////
typedef enum
{
	M2S_AUDIO_SAMPLE_RATE_48KHz,
} m2s_audio_sample_rate_t;

typedef enum
{
	M2S_AUDIO_BIT_DEPTH_24,
} m2s_audio_bit_depth_t;

typedef enum
{
	M2S_PACKET_TIME_1ms,
	M2S_PACKET_TIME_125us,
} m2s_audio_packet_time_t;

typedef enum
{
	// M2S_AUDIO_APP_FORMAT_S24LE,
	// M2S_AUDIO_APP_FORMAT_U24LE,
	M2S_AUDIO_APP_FORMAT_S24BE,
	// M2S_AUDIO_APP_FORMAT_U24BE,
} m2s_audo_app_format_t;

typedef enum
{
	M2S_AUDIO_LAYOUT_INTERLEAVED,
	// M2S_AUDIO_LAYOUT_NON_INTERLEAVED,
} m2s_audio_layout_t;

typedef struct
{
	m2s_audio_sample_rate_t sample_rate;
	uint8_t channels;
	m2s_audio_bit_depth_t bit_depth;
	m2s_audio_packet_time_t packet_time;
} m2s_audio_rtp_caps_t;

typedef struct
{
	m2s_audio_sample_rate_t sample_rate;
	uint8_t channels;
	m2s_audo_app_format_t format;
	m2s_audio_layout_t layout;
} m2s_audio_app_caps_t;

typedef struct
{
	m2s_audio_rtp_caps_t rtp_caps;
	m2s_audio_app_caps_t app_caps;
} m2s_media_conf_audio_t;

typedef struct
{
	uint8_t *p_raw;
} m2s_media_audio_t;

typedef struct
{
	uint32_t raw_size;
} m2s_media_audio_size_t;


///////////////////
// ANC           //
///////////////////
typedef struct
{
	m2s_frame_rate_t frame_field_rate;
} m2s_media_conf_anc_t;

typedef enum
{
	M2S_ANC_F_PROGRESSIVE  = 0,
	M2S_ANC_F_NOT_VALID    = 1,
	M2S_ANC_F_FIRST_FIELD  = 2,
	M2S_ANC_F_SECOND_FIELD = 3,
} m2s_anc_f_t;

typedef struct
{
	uint8_t anc_cnt;
	m2s_anc_f_t f;
	uint8_t *p_raw;
} m2s_media_anc_packet_t;

typedef struct
{
	m2s_media_anc_packet_t *p_packet_array;
} m2s_media_anc_t;

typedef struct
{
	uint8_t packet_array_count;
	uint16_t *p_raw_length_array;
} m2s_media_anc_size_t;

#define M2S_ANC_FIXED_LENGTH_BIT 72
#define M2S_ANC_WORD_LENGTH 32
#define M2S_ANC_CHECK_SUM_LENGTH 10
#define M2S_ANC_UDW_MAX 255
#define M2S_ANC_RAW_MAX_BYTE 440
typedef struct
{
	uint8_t  c;
	uint16_t line_number;
	uint16_t horizontal_offset;
	uint8_t  s;
	uint8_t  streamNum;
	
	// Use b0 - b7
	uint16_t did;
	uint16_t sdid_dbn;
	uint16_t data_count;
	uint16_t udw[M2S_ANC_UDW_MAX];
	// Use b0 - b8(rx only)
	uint16_t check_sum;
} m2s_anc_data_packet_format_t;

typedef struct
{
	// Ancillary Data Packet count per one packet
	uint8_t anc_cnt;

	m2s_anc_f_t f;
	// Ancillary Data Packet format
	m2s_anc_data_packet_format_t *p_format;
} m2s_media_anc_format_t;


///////////////////
// JXSV          //
///////////////////
typedef enum
{
	M2S_FRAME_FIELD_INTERLACE_INFO_PROGRESSIVE  = 0,
	M2S_FRAME_FIELD_INTERLACE_INFO_RESERVED     = 1,
	M2S_FRAME_FIELD_INTERLACE_INFO_FIRST_FIELD  = 2,
	M2S_FRAME_FIELD_INTERLACE_INFO_SECOND_FIELD = 3,
} m2s_frame_field_interlace_info_t;

typedef struct
{
	uint32_t frame_field_size;
	m2s_video_scan_t scan;
	m2s_frame_rate_t frame_rate;
	m2s_video_resolution_t resolution;
} m2s_media_conf_jxsv_t;

typedef struct
{
	uint8_t *p_frame_field;
	m2s_frame_field_interlace_info_t interlace_info;
} m2s_media_jxsv_t;

typedef struct
{
	uint32_t frame_field_size;
} m2s_media_jxsv_size_t;


///////////////////
// TX            //
///////////////////
typedef struct
{
	uint32_t reset;

	uint32_t app_fifo_enqueue;
	uint32_t app_fifo_dequeue;
	uint32_t app_fifo_stored;

	uint32_t rtp_fifo_enqueue;
	uint32_t rtp_fifo_dequeue;
	uint32_t rtp_fifo_stored;

	uint32_t packet_snd;
	uint32_t packet_zeroed_timeout;
	uint32_t packet_discontinuous;

	double cpu_load;

	//Developer Debugging
	uint32_t played;
	uint32_t skipped;
	uint32_t enc_pull_err_cnt;
	uint32_t enc_push_err_cnt;

} m2s_status_tx_t;

typedef struct
{
	int32_t num;
} m2s_cpu_affinity_tx_t;

typedef struct
{
	uint32_t app_fifo_size;
	uint32_t rtp_fifo_size;
	uint32_t packet_fifo_size;
} m2s_allocation_tx_t;


///////////////////
// RX            //
///////////////////
typedef struct
{
	bool active[2];
	uint32_t detect[2];
	uint32_t lost[2];
	uint32_t reset;

	uint32_t app_fifo_enqueue;
	uint32_t app_fifo_dequeue;
	uint32_t app_fifo_stored;

	uint32_t rtp_fifo_enqueue;
	uint32_t rtp_fifo_dequeue;
	uint32_t rtp_fifo_stored;

	uint32_t hitless_fifo_enqueue[2];
	uint32_t hitless_fifo_dequeue[2];
	uint32_t hitless_fifo_stored[2];

	uint32_t packet_7_rcv;
	uint32_t packet_7_lost;

	uint32_t packet_rcv[2];
	uint32_t packet_lost[2];
	uint32_t packet_discontinuous[2];
	uint32_t packet_underflow[2];
	uint32_t packet_overflow[2];
	uint32_t abnormal_seqnum[2];

	bool async_stream;

	uint32_t hdr_extension;
	uint32_t padding;
	uint32_t srd_is_1;
	uint32_t srd_is_2;
	uint32_t srd_is_3;
	uint32_t srd_zero_length;
	uint32_t pixel_discontinuous;
	uint32_t frame_length_err;

	double l2_cpu_load;
	double l1_cpu_load;

	//Developer Debugging
	uint32_t l2_played;
	uint32_t l2_skipped;
	uint32_t l1_played;
	uint32_t l1_skipped;
	uint32_t packet_7_discontinuous;
	uint32_t dec_pull_err_cnt;
	uint32_t dec_push_err_cnt;

} m2s_status_rx_t;

typedef struct
{
	int32_t l2_num;
	int32_t l1_num;
} m2s_cpu_affinity_rx_t;

typedef struct
{
	uint32_t app_fifo_size;
	uint32_t rtp_fifo_size;
	uint32_t hitless_fifo_size;
	uint32_t packet_fifo_size;
} m2s_allocation_rx_t;

typedef struct
{
	uint32_t packet_rcv;
	uint32_t packet_lost;
} m2s_read_status_t;


///////////////////
// General       //
///////////////////
typedef struct
{
	bool async_rx_rtp_timestamp; // use rx only
	bool use_gpu_direct;
} m2s_sys_conf_t;

typedef union
{
	m2s_media_conf_video_t video;
	m2s_media_conf_audio_t audio;
	m2s_media_conf_anc_t anc;
	m2s_media_conf_jxsv_t jxsv;
} m2s_media_conf_t;

typedef union
{
	m2s_media_video_t video;
	m2s_media_audio_t audio;
	m2s_media_anc_t anc;
	m2s_media_jxsv_t jxsv;
} m2s_media_t;

typedef union
{
	m2s_media_video_size_t video;
	m2s_media_audio_size_t audio;
	m2s_media_anc_size_t anc;
	m2s_media_jxsv_size_t jxsv;
} m2s_media_size_t;

typedef union
{
	m2s_status_tx_t tx;
	m2s_status_rx_t rx;
} m2s_status_t;

typedef union
{
	m2s_cpu_affinity_tx_t tx;
	m2s_cpu_affinity_rx_t rx;
} m2s_cpu_affinity_t;

typedef union
{
	m2s_allocation_tx_t tx;
	m2s_allocation_rx_t rx;
} m2s_allocation_t;


///////////////////
// Multi-view    //
///////////////////
typedef void *m2s_mview_id_t;

typedef enum
{
	M2S_MVIEW_MATRIX_2x1,
	M2S_MVIEW_MATRIX_2x2,
	M2S_MVIEW_MATRIX_3x3,
	M2S_MVIEW_MATRIX_4x4,
} m2s_mview_matrix_t;

typedef struct
{
	uint8_t x;
	uint8_t y;
} m2s_mview_position_t;

typedef struct
{
	uint32_t mview_fifo_enqueue;
	uint32_t mview_fifo_dequeue;
	uint32_t mview_fifo_stored;
} m2s_mview_status_t;

///////////////////
// Utility       //
///////////////////
typedef struct
{
	uint64_t start_time_ns;
	uint32_t rtp_timestamp;
} m2s_time_info_t;

typedef enum
{
	M2S_RTP_COUNTER_FREQ_90KHZ = 90000,
	M2S_RTP_COUNTER_FREQ_48KHZ = 48000,
} m2s_rtp_counter_freq_t;


/////////////////////////
// Developer Debugging //
/////////////////////////
typedef enum
{
	M2S_CAPSULATION_MODE_SW,
	M2S_CAPSULATION_MODE_AUTO,
} m2s_capsulation_mode_t;

typedef struct
{
	bool async_rx_rtp_timestamp; // use rx only

	bool use_gpu_direct;
	bool add_extra_queue;
	bool nocuda;
	m2s_capsulation_mode_t capsulation_mode;

	m2s_mbit_divide_mode_t mbit_divide_mode;
	uint32_t audio_block_time;
} m2s_dev_conf_t;

typedef struct
{
	int32_t ptp_calibration_offset;
	uint64_t hitless_pd_ns;
} m2s_developer_conf_t;

typedef struct
{
	// common
	bool separate_rtp_mem;
	bool allocate_memory;
	bool register_memory;
	bool assert_mc_addr;

	// tx
	bool do_debug_print;
	bool do_completion_journaling;
	uint32_t mem_block_len;
	uint32_t chunk_size_in_stride;
	uint32_t chunks_num_per_frame_field;
	uint32_t frames_fields_per_mem_block;

	// rx
	int32_t dhds_type;
	uint32_t min;
	uint32_t max;
} m2s_dbg_conf_t;

//------------------------------------------------------------------------------
// prototypes
//------------------------------------------------------------------------------


#endif //__M2S_TYPES_H__
