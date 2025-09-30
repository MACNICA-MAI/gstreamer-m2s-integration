#if !defined(__TR_OFFSET_H__)
#define __TR_OFFSET_H__
#if defined(__cplusplus)
extern "C" {
#endif

#define CALC_TR_OFFSET_ERR (-9999)

#define TR_OFFSET_VIDEO_2160p_59_SPECS   (637674)
#define TR_OFFSET_VIDEO_2160p_59_MARGIN  (20000)
#define TR_OFFSET_VIDEO_1080p_59_SPECS   (637674)
#define TR_OFFSET_VIDEO_1080p_59_MARGIN  (20000)
#define TR_OFFSET_VIDEO_1080i_59_SPECS   (652503)
#define TR_OFFSET_VIDEO_1080i_59_MARGIN  (20000)

#define TR_OFFSET_VIDEO_2160p_50_SPECS   (764444)
#define TR_OFFSET_VIDEO_2160p_50_MARGIN  (20000)
#define TR_OFFSET_VIDEO_1080p_50_SPECS   (764444)
#define TR_OFFSET_VIDEO_1080p_50_MARGIN  (20000)
#define TR_OFFSET_VIDEO_1080i_50_SPECS   (782222)
#define TR_OFFSET_VIDEO_1080i_50_MARGIN  (20000)

#define TR_OFFSET_VIDEO_2160p_60_SPECS   (637037)
#define TR_OFFSET_VIDEO_2160p_60_MARGIN  (20000)
#define TR_OFFSET_VIDEO_1080p_60_SPECS   (637037)
#define TR_OFFSET_VIDEO_1080p_60_MARGIN  (20000)
#define TR_OFFSET_VIDEO_1080i_60_SPECS   (651851)
#define TR_OFFSET_VIDEO_1080i_60_MARGIN  (20000)

#define TR_OFFSET_AUDIO        (100000)
#define TR_OFFSET_ANC          (40000)
#define TR_OFFSET_JXSV         (620000)


int32_t calc_tr_offset(m2s_media_type_t media_type, const m2s_media_conf_t *p_media_conf);

#if defined(__cplusplus)
}
#endif
#endif //__TR_OFFSET_H__
