//==============================================================================
// Copyright (C) 2021 Macnica Inc. All Rights Reserved.
//
// This program is proprietary and confidential. By using this program
// you agree to the terms of the associated Macnica Software License Agreement.
//------------------------------------------------------------------------------
//! @file
//! @brief
//==============================================================================
#if !defined(__M2S_API_H__)
#define __M2S_API_H__
#include <m2s_types.h>

//------------------------------------------------------------------------------
// define and structures
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// prototypes
//------------------------------------------------------------------------------
#if defined(__cplusplus)
extern "C" {
#endif
void m2s_get_version(m2s_version_t *p_version);

void m2s_init_internal_cpu_affinity(int32_t cpu_affinity);
void m2s_init_ptp_clock_ip(uint32_t ptp_clock_ip);
void m2s_init_dump_mbit_info(bool enabled);

int32_t m2s_open(const m2s_open_conf_t *p_open_conf);
int32_t m2s_close(void);

int32_t m2s_create(
	m2s_strm_id_t *p_strm_id, m2s_io_type_t io_type, m2s_media_type_t media_type, m2s_memory_mode_t memory_mode,
	const m2s_cpu_affinity_t *p_cpu_affinity, const m2s_allocation_t *p_allocation, bool use_rx_hw_hitless);
int32_t m2s_delete(m2s_strm_id_t strm_id);

int32_t m2s_set_sys_conf(m2s_strm_id_t strm_id, const m2s_sys_conf_t *p_sys_conf);
int32_t m2s_set_mbit_divide_mode(m2s_strm_id_t strm_id, m2s_mbit_divide_mode_t mode);
int32_t m2s_set_audio_block_time(m2s_strm_id_t strm_id, uint32_t time_ms);
int32_t m2s_set_media_conf(m2s_strm_id_t strm_id, const m2s_media_conf_t *p_media_conf);
int32_t m2s_set_ip_conf(m2s_strm_id_t strm_id, const m2s_ip_conf_t *p_ip_conf);

int32_t m2s_get_sys_conf(m2s_strm_id_t strm_id, m2s_sys_conf_t *p_sys_conf);
int32_t m2s_get_mbit_divide_mode(m2s_strm_id_t strm_id, m2s_mbit_divide_mode_t *p_mode);
int32_t m2s_get_audio_block_time(m2s_strm_id_t strm_id, uint32_t *p_time_ms);
int32_t m2s_get_media_conf(m2s_strm_id_t strm_id, m2s_media_conf_t *p_media_conf);
int32_t m2s_get_ip_conf(m2s_strm_id_t strm_id, m2s_ip_conf_t *p_ip_conf);

int32_t m2s_start(m2s_strm_id_t strm_id);
int32_t m2s_stop(m2s_strm_id_t strm_id);

int32_t m2s_enable_select(m2s_strm_id_t strm_id, bool enabled);

int32_t m2s_write_select(
	m2s_strm_id_t strm_id,
	const m2s_media_size_t *p_write_size,
	bool *p_is_first_wakeup);

int32_t m2s_write(
	m2s_strm_id_t strm_id,
	const m2s_time_info_t *p_time,
	const m2s_media_t *p_media,
	const m2s_media_size_t *p_write_size);

int32_t m2s_read_select(
	m2s_strm_id_t strm_id,
	m2s_media_size_t *p_read_size,
	const m2s_media_size_t *p_max_read_size,
	bool *p_is_first_wakeup);

int32_t m2s_read(
	m2s_strm_id_t strm_id,
	uint32_t *p_rtp_timestamp,
	m2s_media_t *p_media,
	m2s_media_size_t *p_read_size,
	const m2s_media_size_t *p_max_read_size);

int32_t m2s_read_with_status(
	m2s_strm_id_t strm_id,
	uint32_t *p_rtp_timestamp,
	m2s_media_t *p_media,
	m2s_media_size_t *p_read_size,
	const m2s_media_size_t *p_max_read_size,
	m2s_read_status_t *p_status);

int32_t m2s_get_read_ptr(
	m2s_strm_id_t strm_id,
	uint32_t *p_rtp_timestamp,
	m2s_media_t *p_media,
	m2s_media_size_t *p_read_size);

int32_t m2s_get_read_ptr_with_status(
	m2s_strm_id_t strm_id,
	uint32_t *p_rtp_timestamp,
	m2s_media_t *p_media,
	m2s_media_size_t *p_read_size,
	m2s_read_status_t *p_status);

int32_t m2s_free_read_ptr(m2s_strm_id_t strm_id);

int32_t m2s_get_status(m2s_strm_id_t strm_id, m2s_status_t *p_status, bool clear);


int32_t m2s_mview_create(m2s_mview_id_t *p_mview_id, m2s_mview_matrix_t matrix, m2s_memory_mode_t memory_mode, uint32_t mview_fifo_size, uint32_t playout_delay_align_num);
int32_t m2s_mview_delete(m2s_mview_id_t mview_id);
int32_t m2s_mview_set_app_caps(m2s_mview_id_t mview_id, const m2s_video_app_caps_t *p_app_caps);
int32_t m2s_mview_get_app_caps(m2s_mview_id_t mview_id, m2s_video_app_caps_t *p_app_caps);
int32_t m2s_mview_start(m2s_mview_id_t mview_id);
int32_t m2s_mview_stop(m2s_mview_id_t mview_id);
int32_t m2s_mview_get_status(m2s_mview_id_t mview_id, m2s_mview_status_t *p_status, bool clear);

int32_t m2s_mview_set_sys_conf_each(
	m2s_mview_id_t mview_id, const m2s_mview_position_t *p_pos,
	const m2s_cpu_affinity_rx_t *p_cpu_affinity, const m2s_allocation_rx_t *p_allocation, bool use_rx_hw_hitless);

int32_t m2s_mview_set_rtp_caps_each(
	m2s_mview_id_t mview_id,
	const m2s_mview_position_t *p_pos,
	const m2s_video_rtp_caps_t *p_rtp_caps);

int32_t m2s_mview_set_ip_conf_each(
	m2s_mview_id_t mview_id,
	const m2s_mview_position_t *p_pos,
	const m2s_ip_conf_t *p_ip);

int32_t m2s_mview_get_rtp_caps_each(
	m2s_mview_id_t mview_id,
	const m2s_mview_position_t *p_pos,
	m2s_video_rtp_caps_t *p_rtp_caps);

int32_t m2s_mview_get_ip_conf_each(
	m2s_mview_id_t mview_id,
	const m2s_mview_position_t *p_pos,
	m2s_ip_conf_t *p_ip);

int32_t m2s_mview_start_each(m2s_mview_id_t mview_id, const m2s_mview_position_t *p_pos);
int32_t m2s_mview_stop_each(m2s_mview_id_t mview_id, const m2s_mview_position_t *p_pos);
int32_t m2s_mview_get_status_each(m2s_mview_id_t mview_id, const m2s_mview_position_t *p_pos, m2s_status_rx_t *p_rx_status, bool clear);

int32_t m2s_mview_enable_select(m2s_mview_id_t mview_id, bool enabled);

int32_t m2s_mview_read_select(
	m2s_mview_id_t mview_id,
	m2s_media_video_size_t *p_read_size,
	const m2s_media_video_size_t *p_max_read_size,
	bool *p_is_first_wakeup);

int32_t m2s_mview_read(
	m2s_mview_id_t mview_id,
	uint32_t *p_rtp_timestamp,
	m2s_media_video_t *p_media,
	m2s_media_video_size_t *p_read_size,
	const m2s_media_video_size_t *p_max_read_size);

int32_t m2s_mview_get_read_ptr(
	m2s_mview_id_t mview_id,
	uint32_t *p_rtp_timestamp,
	m2s_media_video_t *p_media,
	m2s_media_video_size_t *p_read_size);

int32_t m2s_mview_free_read_ptr(m2s_mview_id_t mview_id);


// utility
uint64_t m2s_get_current_tai_ns(void);
uint32_t m2s_conv_tai_to_rtptime(uint64_t tai_ns, m2s_rtp_counter_freq_t freq);
uint64_t m2s_conv_rtptime_to_tai(uint32_t rtptime, m2s_rtp_counter_freq_t freq);
uint64_t m2s_calc_next_video_alignment_point(uint64_t tai_ns, m2s_frame_rate_t frame_rate, int64_t frame_number_offset);
uint64_t m2s_calc_next_audio_alignment_point(uint64_t tai_ns, int64_t raw_offset);
uint32_t m2s_conv_ip_address_from_string(const char *p_addr);
int32_t  m2s_set_app_cpu_affinity(uint32_t cpu_num);

int32_t  m2s_init_anc_tx_packet(m2s_media_anc_t *p_tx_media, m2s_media_anc_size_t *p_tx_size, m2s_media_anc_size_t *p_tx_size_max, uint8_t anc_packet_max_cnt, uint16_t anc_raw_max_size);
int32_t  m2s_deinit_anc_tx_packet(m2s_media_anc_t *p_tx_media, m2s_media_anc_size_t *p_tx_size, m2s_media_anc_size_t *p_tx_size_max);
int32_t  m2s_set_anc_tx_packet(m2s_media_anc_t *p_tx_media, m2s_media_anc_size_t *p_tx_size, m2s_media_anc_size_t *p_tx_size_max, uint8_t anc_packet_cnt, m2s_media_anc_format_t *p_packet_format);
int32_t  m2s_init_anc_rx_packet(m2s_media_anc_t *p_rx_media, m2s_media_anc_size_t *p_rx_size, m2s_media_anc_size_t *p_rx_size_max, m2s_media_anc_format_t **p_packet_format,
                                uint8_t anc_packet_max_cnt, uint8_t anc_raw_max_cnt, uint16_t anc_raw_max_size, bool use_get_read_ptr);
int32_t  m2s_deinit_anc_rx_packet(m2s_media_anc_t *p_rx_media, m2s_media_anc_size_t *p_rx_size, m2s_media_anc_size_t *p_rx_size_max,
                                  m2s_media_anc_format_t **p_packet_format, bool use_get_read_ptr);
int32_t  m2s_parse_anc_rx_packet(m2s_media_anc_t *p_rx_media, m2s_media_anc_size_t *p_rx_size, m2s_media_anc_size_t *p_rx_size_max, 
                                 m2s_media_anc_format_t *p_packet_format);


// developer debugging
int32_t m2s_set_dev_conf(m2s_strm_id_t strm_id, const m2s_dev_conf_t *p_dev_conf);
int32_t m2s_get_dev_conf(m2s_strm_id_t strm_id, m2s_dev_conf_t *p_dev_conf);
int32_t m2s_set_developer_conf(const m2s_developer_conf_t *p_conf);
int32_t m2s_get_developer_conf(m2s_developer_conf_t *p_conf);
int32_t m2s_set_dbg_conf(m2s_strm_id_t strm_id, const m2s_dbg_conf_t *p_dbg_conf);

#if defined(__cplusplus)
}
#endif

#endif //__M2S_API_H__
