#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <m2s_types.h>
#include "tr_offset.h"

int32_t calc_tr_offset(m2s_media_type_t media_type, const m2s_media_conf_t *p_media_conf)
{
	int32_t tr_offset = CALC_TR_OFFSET_ERR;
	int32_t tr_offset_specs = 0;
	int32_t tr_offset_margin = 0;

	switch (media_type)
	{
	case M2S_MEDIA_TYPE_VIDEO:
		switch (p_media_conf->video.rtp_caps.frame_rate)
		{
		case M2S_FRAME_RATE_60000_1001: // progressive
			switch (p_media_conf->video.rtp_caps.resolution)
			{
			case M2S_VIDEO_RESOLUTION_3840x2160:
				tr_offset_specs = TR_OFFSET_VIDEO_2160p_59_SPECS;
				tr_offset_margin = TR_OFFSET_VIDEO_2160p_59_MARGIN;
				break;
			case M2S_VIDEO_RESOLUTION_1920x1080:
				tr_offset_specs = TR_OFFSET_VIDEO_1080p_59_SPECS;
				tr_offset_margin = TR_OFFSET_VIDEO_1080p_59_MARGIN;
				break;
			default:
				break;
			}
			break;
		case M2S_FRAME_RATE_30000_1001: // interlace
			switch (p_media_conf->video.rtp_caps.resolution)
			{
			case M2S_VIDEO_RESOLUTION_1920x1080:
				tr_offset_specs = TR_OFFSET_VIDEO_1080i_59_SPECS;
				tr_offset_margin = TR_OFFSET_VIDEO_1080i_59_MARGIN;
				break;
			default:
				break;
			}
			break;
		case M2S_FRAME_RATE_50_1: // progressive
			switch (p_media_conf->video.rtp_caps.resolution)
			{
			case M2S_VIDEO_RESOLUTION_3840x2160:
				tr_offset_specs = TR_OFFSET_VIDEO_2160p_50_SPECS;
				tr_offset_margin = TR_OFFSET_VIDEO_2160p_50_MARGIN;
				break;
			case M2S_VIDEO_RESOLUTION_1920x1080:
				tr_offset_specs = TR_OFFSET_VIDEO_1080p_50_SPECS;
				tr_offset_margin = TR_OFFSET_VIDEO_1080p_50_MARGIN;
				break;
			default:
				break;
			}
			break;
		case M2S_FRAME_RATE_25_1: // interlace
			switch (p_media_conf->video.rtp_caps.resolution)
			{
			case M2S_VIDEO_RESOLUTION_1920x1080:
				tr_offset_specs = TR_OFFSET_VIDEO_1080i_50_SPECS;
				tr_offset_margin = TR_OFFSET_VIDEO_1080i_50_MARGIN;
				break;
			default:
				break;
			}
			break;
		case M2S_FRAME_RATE_60_1: // progressive
			switch (p_media_conf->video.rtp_caps.resolution)
			{
			case M2S_VIDEO_RESOLUTION_3840x2160:
				tr_offset_specs = TR_OFFSET_VIDEO_2160p_60_SPECS;
				tr_offset_margin = TR_OFFSET_VIDEO_2160p_60_MARGIN;
				break;
			case M2S_VIDEO_RESOLUTION_1920x1080:
				tr_offset_specs = TR_OFFSET_VIDEO_1080p_60_SPECS;
				tr_offset_margin = TR_OFFSET_VIDEO_1080p_60_MARGIN;
				break;
			default:
				break;
			}
			break;
		}
		if (tr_offset_specs || tr_offset_margin)
		{
			tr_offset = tr_offset_specs - tr_offset_margin;
		}
		break;
	case M2S_MEDIA_TYPE_AUDIO:
		tr_offset = TR_OFFSET_AUDIO;
		break;
	case M2S_MEDIA_TYPE_ANC:
		tr_offset = TR_OFFSET_ANC;
		break;
	case M2S_MEDIA_TYPE_JXSV:
		tr_offset = TR_OFFSET_JXSV;
		break;
	default:
		break;
	}

	return tr_offset;
}
