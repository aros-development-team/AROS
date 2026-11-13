/*
 * VA-API vocabulary of gfx.hidd's decode interface, private to vcgfx.
 *
 * The interface speaks VA-API (<va/va.h>): profiles are VAProfile values,
 * parameter buffers VABufferType values with the VA structures as data.
 * The driver builds without the libva port (it lives in contrib), so the
 * definitions are copied here from libva 2.22.0's va.h and va_dec_hevc.h
 * (the ABI is frozen; VA_MAJOR_VERSION 1). Keep them byte-identical to
 * libva's. Not installed: programs use <va/va.h> itself.
 *
 * From va.h:
 *
 * Copyright (c) 2007-2009 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL INTEL AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * From va_dec_hevc.h:
 *
 * Copyright (c) 2014 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL INTEL AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef VCGFX_VADEC_H
#define VCGFX_VADEC_H

#include <stdint.h>

typedef unsigned int VAGenericID;
typedef VAGenericID VASurfaceID;

#define VA_INVALID_ID       0xffffffff
#define VA_INVALID_SURFACE  VA_INVALID_ID

#define VA_SLICE_DATA_FLAG_ALL      0x00    /* whole slice is in the buffer */

/* VAProfile values the decoder may be asked about */
#define VAProfileNone                       (-1)
#define VAProfileMPEG2Simple                0
#define VAProfileMPEG2Main                  1
#define VAProfileH264Baseline               5
#define VAProfileH264Main                   6
#define VAProfileH264High                   7
#define VAProfileH264ConstrainedBaseline    13
#define VAProfileHEVCMain                   17
#define VAProfileHEVCMain10                 18
#define VAProfileVP9Profile0                19
#define VAProfileVP9Profile2                21
#define VAProfileAV1Profile0                32
#define VAProfileH264High10                 36

/* VABufferType values */
#define VAPictureParameterBufferType        0
#define VAIQMatrixBufferType                1
#define VABitPlaneBufferType                2
#define VASliceGroupMapBufferType           3
#define VASliceParameterBufferType          4
#define VASliceDataBufferType               5
#define VAProbabilityBufferType             13

#define VA_PADDING_LOW          4
#define VA_PADDING_MEDIUM       8
#define VA_PADDING_HIGH         16
#define VA_PADDING_LARGE        32
#define va_deprecated


/****************************
 * H.264/AVC data structures
 ****************************/

typedef struct _VAPictureH264 {
    VASurfaceID picture_id;
    uint32_t frame_idx;
    uint32_t flags;
    int32_t TopFieldOrderCnt;
    int32_t BottomFieldOrderCnt;

    /** \brief Reserved bytes for future use, must be zero */
    uint32_t                va_reserved[VA_PADDING_LOW];
} VAPictureH264;
/* flags in VAPictureH264 could be OR of the following */
#define VA_PICTURE_H264_INVALID         0x00000001
#define VA_PICTURE_H264_TOP_FIELD       0x00000002
#define VA_PICTURE_H264_BOTTOM_FIELD        0x00000004
#define VA_PICTURE_H264_SHORT_TERM_REFERENCE    0x00000008
#define VA_PICTURE_H264_LONG_TERM_REFERENCE 0x00000010

/** H.264 Picture Parameter Buffer */
/*
 * For each picture, and before any slice data, a single
 * picture parameter buffer must be send.
 */
typedef struct _VAPictureParameterBufferH264 {
    VAPictureH264 CurrPic;
    VAPictureH264 ReferenceFrames[16];  /* in DPB */
    uint16_t picture_width_in_mbs_minus1;
    uint16_t picture_height_in_mbs_minus1;
    uint8_t bit_depth_luma_minus8;
    uint8_t bit_depth_chroma_minus8;
    uint8_t num_ref_frames;
    union {
        struct {
            uint32_t chroma_format_idc          : 2;
            uint32_t residual_colour_transform_flag     : 1; /* Renamed to separate_colour_plane_flag in newer standard versions. */
            uint32_t gaps_in_frame_num_value_allowed_flag   : 1;
            uint32_t frame_mbs_only_flag            : 1;
            uint32_t mb_adaptive_frame_field_flag       : 1;
            uint32_t direct_8x8_inference_flag      : 1;
            uint32_t MinLumaBiPredSize8x8           : 1; /* see A.3.3.2 */
            uint32_t log2_max_frame_num_minus4      : 4;
            uint32_t pic_order_cnt_type         : 2;
            uint32_t log2_max_pic_order_cnt_lsb_minus4  : 4;
            uint32_t delta_pic_order_always_zero_flag   : 1;
        } bits;
        uint32_t value;
    } seq_fields;
    // FMO is not supported.
    va_deprecated uint8_t num_slice_groups_minus1;
    va_deprecated uint8_t slice_group_map_type;
    va_deprecated uint16_t slice_group_change_rate_minus1;
    int8_t pic_init_qp_minus26;
    int8_t pic_init_qs_minus26;
    int8_t chroma_qp_index_offset;
    int8_t second_chroma_qp_index_offset;
    union {
        struct {
            uint32_t entropy_coding_mode_flag   : 1;
            uint32_t weighted_pred_flag     : 1;
            uint32_t weighted_bipred_idc        : 2;
            uint32_t transform_8x8_mode_flag    : 1;
            uint32_t field_pic_flag         : 1;
            uint32_t constrained_intra_pred_flag    : 1;
            uint32_t pic_order_present_flag         : 1; /* Renamed to bottom_field_pic_order_in_frame_present_flag in newer standard versions. */
            uint32_t deblocking_filter_control_present_flag : 1;
            uint32_t redundant_pic_cnt_present_flag     : 1;
            uint32_t reference_pic_flag         : 1; /* nal_ref_idc != 0 */
        } bits;
        uint32_t value;
    } pic_fields;
    uint16_t frame_num;

    /** \brief Reserved bytes for future use, must be zero */
    uint32_t                va_reserved[VA_PADDING_MEDIUM];
} VAPictureParameterBufferH264;

/** H.264 Inverse Quantization Matrix Buffer */
typedef struct _VAIQMatrixBufferH264 {
    /** \brief 4x4 scaling list, in raster scan order. */
    uint8_t ScalingList4x4[6][16];
    /** \brief 8x8 scaling list, in raster scan order. */
    uint8_t ScalingList8x8[2][64];

    /** \brief Reserved bytes for future use, must be zero */
    uint32_t                va_reserved[VA_PADDING_LOW];
} VAIQMatrixBufferH264;

/** H.264 Slice Parameter Buffer */
typedef struct _VASliceParameterBufferH264 {
    uint32_t slice_data_size;/* number of bytes in the slice data buffer for this slice */
    /** \brief Byte offset to the NAL Header Unit for this slice. */
    uint32_t slice_data_offset;
    uint32_t slice_data_flag; /* see VA_SLICE_DATA_FLAG_XXX defintions */
    /**
     * \brief Bit offset from NAL Header Unit to the begining of slice_data().
     *
     * This bit offset is relative to and includes the NAL unit byte
     * and represents the number of bits parsed in the slice_header()
     * after the removal of any emulation prevention bytes in
     * there. However, the slice data buffer passed to the hardware is
     * the original bitstream, thus including any emulation prevention
     * bytes.
     */
    uint16_t slice_data_bit_offset;
    uint16_t first_mb_in_slice;
    uint8_t slice_type;
    uint8_t direct_spatial_mv_pred_flag;
    /**
     * H264/AVC syntax element
     *
     * if num_ref_idx_active_override_flag equals 0, host decoder should
     * set its value to num_ref_idx_l0_default_active_minus1.
     */
    uint8_t num_ref_idx_l0_active_minus1;
    /**
     * H264/AVC syntax element
     *
     * if num_ref_idx_active_override_flag equals 0, host decoder should
     * set its value to num_ref_idx_l1_default_active_minus1.
     */
    uint8_t num_ref_idx_l1_active_minus1;
    uint8_t cabac_init_idc;
    int8_t slice_qp_delta;
    uint8_t disable_deblocking_filter_idc;
    int8_t slice_alpha_c0_offset_div2;
    int8_t slice_beta_offset_div2;
    VAPictureH264 RefPicList0[32];  /* See 8.2.4.2 */
    VAPictureH264 RefPicList1[32];  /* See 8.2.4.2 */
    uint8_t luma_log2_weight_denom;
    uint8_t chroma_log2_weight_denom;
    uint8_t luma_weight_l0_flag;
    int16_t luma_weight_l0[32];
    int16_t luma_offset_l0[32];
    uint8_t chroma_weight_l0_flag;
    int16_t chroma_weight_l0[32][2];
    int16_t chroma_offset_l0[32][2];
    uint8_t luma_weight_l1_flag;
    int16_t luma_weight_l1[32];
    int16_t luma_offset_l1[32];
    uint8_t chroma_weight_l1_flag;
    int16_t chroma_weight_l1[32][2];
    int16_t chroma_offset_l1[32][2];

    /** \brief Reserved bytes for future use, must be zero */
    uint32_t                va_reserved[VA_PADDING_LOW];
} VASliceParameterBufferH264;


/* MPEG-2 (va.h) */
typedef struct _VAPictureParameterBufferMPEG2 {
    uint16_t horizontal_size;
    uint16_t vertical_size;
    VASurfaceID forward_reference_picture;
    VASurfaceID backward_reference_picture;
    int32_t picture_coding_type;
    int32_t f_code;                 /* fwd h, fwd v, bwd h, bwd v: 4 bits each, MSB first */
    union {
        struct {
            uint32_t intra_dc_precision         : 2;
            uint32_t picture_structure          : 2;
            uint32_t top_field_first            : 1;
            uint32_t frame_pred_frame_dct       : 1;
            uint32_t concealment_motion_vectors : 1;
            uint32_t q_scale_type               : 1;
            uint32_t intra_vlc_format           : 1;
            uint32_t alternate_scan             : 1;
            uint32_t repeat_first_field         : 1;
            uint32_t progressive_frame          : 1;
            uint32_t is_first_field             : 1;
        } bits;
        uint32_t value;
    } picture_coding_extension;
    uint32_t                va_reserved[VA_PADDING_LOW];
} VAPictureParameterBufferMPEG2;

typedef struct _VAIQMatrixBufferMPEG2 {
    int32_t load_intra_quantiser_matrix;
    int32_t load_non_intra_quantiser_matrix;
    int32_t load_chroma_intra_quantiser_matrix;
    int32_t load_chroma_non_intra_quantiser_matrix;
    uint8_t intra_quantiser_matrix[64];         /* zig-zag scan order */
    uint8_t non_intra_quantiser_matrix[64];
    uint8_t chroma_intra_quantiser_matrix[64];
    uint8_t chroma_non_intra_quantiser_matrix[64];
    uint32_t                va_reserved[VA_PADDING_LOW];
} VAIQMatrixBufferMPEG2;

typedef struct _VASliceParameterBufferMPEG2 {
    uint32_t slice_data_size;
    uint32_t slice_data_offset;
    uint32_t slice_data_flag;
    uint32_t macroblock_offset;
    uint32_t slice_horizontal_position;
    uint32_t slice_vertical_position;
    int32_t quantiser_scale_code;
    int32_t intra_slice_flag;
    uint32_t                va_reserved[VA_PADDING_LOW];
} VASliceParameterBufferMPEG2;

/* HEVC (va.h and va_dec_hevc.h) */

typedef struct _VAPictureHEVC {

    VASurfaceID             picture_id;

    int32_t                 pic_order_cnt;

    uint32_t                flags;

    uint32_t                va_reserved[VA_PADDING_LOW];
} VAPictureHEVC;

#define VA_PICTURE_HEVC_INVALID                 0x00000001
#define VA_PICTURE_HEVC_FIELD_PIC               0x00000002
#define VA_PICTURE_HEVC_BOTTOM_FIELD            0x00000004
#define VA_PICTURE_HEVC_LONG_TERM_REFERENCE     0x00000008
#define VA_PICTURE_HEVC_RPS_ST_CURR_BEFORE      0x00000010
#define VA_PICTURE_HEVC_RPS_ST_CURR_AFTER       0x00000020
#define VA_PICTURE_HEVC_RPS_LT_CURR             0x00000040

typedef struct  _VAPictureParameterBufferHEVC {

    VAPictureHEVC           CurrPic;

    VAPictureHEVC           ReferenceFrames[15];

    uint16_t                pic_width_in_luma_samples;

    uint16_t                pic_height_in_luma_samples;

    union {
        struct {

            uint32_t        chroma_format_idc                           : 2;
            uint32_t        separate_colour_plane_flag                  : 1;
            uint32_t        pcm_enabled_flag                            : 1;
            uint32_t        scaling_list_enabled_flag                   : 1;
            uint32_t        transform_skip_enabled_flag                 : 1;
            uint32_t        amp_enabled_flag                            : 1;
            uint32_t        strong_intra_smoothing_enabled_flag         : 1;
            uint32_t        sign_data_hiding_enabled_flag               : 1;
            uint32_t        constrained_intra_pred_flag                 : 1;
            uint32_t        cu_qp_delta_enabled_flag                    : 1;
            uint32_t        weighted_pred_flag                          : 1;
            uint32_t        weighted_bipred_flag                        : 1;
            uint32_t        transquant_bypass_enabled_flag              : 1;
            uint32_t        tiles_enabled_flag                          : 1;
            uint32_t        entropy_coding_sync_enabled_flag            : 1;
            uint32_t        pps_loop_filter_across_slices_enabled_flag  : 1;
            uint32_t        loop_filter_across_tiles_enabled_flag       : 1;
            uint32_t        pcm_loop_filter_disabled_flag               : 1;

            uint32_t        NoPicReorderingFlag                         : 1;

            uint32_t        NoBiPredFlag                                : 1;

            uint32_t        ReservedBits                                : 11;
        } bits;
        uint32_t            value;
    } pic_fields;

    uint8_t                 sps_max_dec_pic_buffering_minus1;
    uint8_t                 bit_depth_luma_minus8;
    uint8_t                 bit_depth_chroma_minus8;
    uint8_t                 pcm_sample_bit_depth_luma_minus1;
    uint8_t                 pcm_sample_bit_depth_chroma_minus1;
    uint8_t                 log2_min_luma_coding_block_size_minus3;
    uint8_t                 log2_diff_max_min_luma_coding_block_size;
    uint8_t                 log2_min_transform_block_size_minus2;
    uint8_t                 log2_diff_max_min_transform_block_size;
    uint8_t                 log2_min_pcm_luma_coding_block_size_minus3;
    uint8_t                 log2_diff_max_min_pcm_luma_coding_block_size;
    uint8_t                 max_transform_hierarchy_depth_intra;
    uint8_t                 max_transform_hierarchy_depth_inter;
    int8_t                  init_qp_minus26;
    uint8_t                 diff_cu_qp_delta_depth;
    int8_t                  pps_cb_qp_offset;
    int8_t                  pps_cr_qp_offset;
    uint8_t                 log2_parallel_merge_level_minus2;
    uint8_t                 num_tile_columns_minus1;
    uint8_t                 num_tile_rows_minus1;

    uint16_t                column_width_minus1[19];
    uint16_t                row_height_minus1[21];

    union {
        struct {

            uint32_t        lists_modification_present_flag             : 1;
            uint32_t        long_term_ref_pics_present_flag             : 1;
            uint32_t        sps_temporal_mvp_enabled_flag               : 1;
            uint32_t        cabac_init_present_flag                     : 1;
            uint32_t        output_flag_present_flag                    : 1;
            uint32_t        dependent_slice_segments_enabled_flag       : 1;
            uint32_t        pps_slice_chroma_qp_offsets_present_flag    : 1;
            uint32_t        sample_adaptive_offset_enabled_flag         : 1;
            uint32_t        deblocking_filter_override_enabled_flag     : 1;
            uint32_t        pps_disable_deblocking_filter_flag          : 1;
            uint32_t        slice_segment_header_extension_present_flag : 1;

            uint32_t        RapPicFlag                                  : 1;

            uint32_t        IdrPicFlag                                  : 1;

            uint32_t        IntraPicFlag                                : 1;

            uint32_t        ReservedBits                                : 18;
        } bits;
        uint32_t            value;
    } slice_parsing_fields;

    uint8_t                 log2_max_pic_order_cnt_lsb_minus4;
    uint8_t                 num_short_term_ref_pic_sets;
    uint8_t                 num_long_term_ref_pic_sps;
    uint8_t                 num_ref_idx_l0_default_active_minus1;
    uint8_t                 num_ref_idx_l1_default_active_minus1;
    int8_t                  pps_beta_offset_div2;
    int8_t                  pps_tc_offset_div2;
    uint8_t                 num_extra_slice_header_bits;

    uint32_t                st_rps_bits;

    uint32_t                va_reserved[VA_PADDING_MEDIUM];
} VAPictureParameterBufferHEVC;

typedef struct  _VASliceParameterBufferHEVC {

    uint32_t                slice_data_size;

    uint32_t                slice_data_offset;

    uint32_t                slice_data_flag;

    uint32_t                slice_data_byte_offset;

    uint32_t                slice_segment_address;

    uint8_t                 RefPicList[2][15];
    union {
        uint32_t            value;
        struct {

            uint32_t        LastSliceOfPic                              : 1;

            uint32_t        dependent_slice_segment_flag                : 1;

            uint32_t        slice_type                                  : 2;

            uint32_t        color_plane_id                              : 2;

            uint32_t        slice_sao_luma_flag                         : 1;

            uint32_t        slice_sao_chroma_flag                       : 1;

            uint32_t        mvd_l1_zero_flag                            : 1;

            uint32_t        cabac_init_flag                             : 1;

            uint32_t        slice_temporal_mvp_enabled_flag             : 1;

            uint32_t        slice_deblocking_filter_disabled_flag       : 1;

            uint32_t        collocated_from_l0_flag                     : 1;

            uint32_t        slice_loop_filter_across_slices_enabled_flag : 1;
            uint32_t        reserved                                    : 18;
        } fields;
    } LongSliceFlags;

    uint8_t                 collocated_ref_idx;

    uint8_t                 num_ref_idx_l0_active_minus1;

    uint8_t                 num_ref_idx_l1_active_minus1;

    int8_t                  slice_qp_delta;

    int8_t                  slice_cb_qp_offset;

    int8_t                  slice_cr_qp_offset;

    int8_t                  slice_beta_offset_div2;

    int8_t                  slice_tc_offset_div2;

    uint8_t                 luma_log2_weight_denom;

    int8_t                  delta_chroma_log2_weight_denom;

    int8_t                  delta_luma_weight_l0[15];

    int8_t                  luma_offset_l0[15];

    int8_t                  delta_chroma_weight_l0[15][2];

    int8_t                  ChromaOffsetL0[15][2];

    int8_t                  delta_luma_weight_l1[15];

    int8_t                  luma_offset_l1[15];

    int8_t                  delta_chroma_weight_l1[15][2];

    int8_t                  ChromaOffsetL1[15][2];

    uint8_t                 five_minus_max_num_merge_cand;

    uint16_t                 num_entry_point_offsets;

    uint16_t                 entry_offset_to_subset_array;

    uint16_t                slice_data_num_emu_prevn_bytes;

    uint32_t                va_reserved[VA_PADDING_LOW - 2];
} VASliceParameterBufferHEVC;

typedef struct _VAIQMatrixBufferHEVC {

    uint8_t                 ScalingList4x4[6][16];

    uint8_t                 ScalingList8x8[6][64];

    uint8_t                 ScalingList16x16[6][64];

    uint8_t                 ScalingList32x32[2][64];

    uint8_t                 ScalingListDC16x16[6];

    uint8_t                 ScalingListDC32x32[2];

    uint32_t                va_reserved[VA_PADDING_LOW];
} VAIQMatrixBufferHEVC;

#endif /* VCGFX_VADEC_H */
