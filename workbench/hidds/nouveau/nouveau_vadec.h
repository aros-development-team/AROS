/*
 * VA-API decode vocabulary used by nouveau.hidd's decode interface.
 *
 * gfx.hidd's decode interface speaks VA-API (<va/va.h>): profiles are
 * VAProfile values, parameter buffers VABufferType values with the VA
 * structures as data. The hidd must build without the libva port, so the
 * definitions it needs are copied here from libva 2.22.0's va.h (the ABI
 * is frozen; VA_MAJOR_VERSION 1). Keep them byte-identical to libva's.
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
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef NOUVEAU_VADEC_H
#define NOUVEAU_VADEC_H

#include <stdint.h>

typedef unsigned int VAGenericID;
typedef VAGenericID VASurfaceID;

#define VA_INVALID_ID       0xffffffff
#define VA_INVALID_SURFACE  VA_INVALID_ID

/* VAProfile values the decoder may be asked about */
#define VAProfileNone                       (-1)
#define VAProfileMPEG2Simple                0
#define VAProfileMPEG2Main                  1
#define VAProfileH264Baseline               5
#define VAProfileVC1Simple                  8
#define VAProfileVC1Main                    9
#define VAProfileVC1Advanced                10
#define VAProfileH264Main                   6
#define VAProfileH264High                   7
#define VAProfileH264ConstrainedBaseline    13
#define VAProfileVP8Version0_3              14
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


/* ---- HEVC (va.h, va_dec_hevc.h) ---- */
typedef struct _VAPictureHEVC {
    /** \brief reconstructed picture buffer surface index
     * invalid when taking value VA_INVALID_SURFACE.
     */
    VASurfaceID             picture_id;
    /** \brief picture order count.
     * in HEVC, POCs for top and bottom fields of same picture should
     * take different values.
     */
    int32_t                 pic_order_cnt;
    /* described below */
    uint32_t                flags;

    /** \brief Reserved bytes for future use, must be zero */
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
    /** \brief buffer description of decoded current picture
     * only VA_PICTURE_HEVC_FIELD_PIC and VA_PICTURE_HEVC_BOTTOM_FIELD
     * of "flags" fields are meaningful.
     */
    VAPictureHEVC           CurrPic;
    /** \brief buffer description of reference frames in DPB */
    VAPictureHEVC           ReferenceFrames[15];
    /** \brief picture width, shall be integer multiple of minimum CB size. */
    uint16_t                pic_width_in_luma_samples;
    /** \brief picture height, shall be integer multiple of minimum CB size. */
    uint16_t                pic_height_in_luma_samples;


    union {
        struct {
            /** following flags have same syntax and semantic as those in HEVC spec */
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
            /** set based on sps_max_num_reorder_pics of current temporal layer. */
            uint32_t        NoPicReorderingFlag                         : 1;
            /** picture has no B slices */
            uint32_t        NoBiPredFlag                                : 1;

            uint32_t        ReservedBits                                : 11;
        } bits;
        uint32_t            value;
    } pic_fields;

    /** following parameters have same syntax with those in HEVC spec */
    /** \brief DPB size for current temporal layer */
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
    /**
     * when uniform_spacing_flag equals 1, application should populate
     * column_width_minus[], and row_height_minus1[] with approperiate values.
     */
    uint16_t                column_width_minus1[19];
    uint16_t                row_height_minus1[21];

    /**
     *  The Following Parameters are needed for Short Slice Format Only.
     *  Only format decoding can ignore them.
     */

    /**
     * \brief Parameters needed for parsing slice segment headers
     */
    union {
        struct {
            /** following parameters have same syntax with those in HEVC spec */
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

            /** current picture with NUT between 16 and 21 inclusive */
            uint32_t        RapPicFlag                                  : 1;
            /** current picture with NUT between 19 and 20 inclusive */
            uint32_t        IdrPicFlag                                  : 1;
            /** current picture has only intra slices */
            uint32_t        IntraPicFlag                                : 1;

            uint32_t        ReservedBits                                : 18;
        } bits;
        uint32_t            value;
    } slice_parsing_fields;

    /** following parameters have same syntax with those in HEVC spec */
    uint8_t                 log2_max_pic_order_cnt_lsb_minus4;
    uint8_t                 num_short_term_ref_pic_sets;
    uint8_t                 num_long_term_ref_pic_sps;
    uint8_t                 num_ref_idx_l0_default_active_minus1;
    uint8_t                 num_ref_idx_l1_default_active_minus1;
    int8_t                  pps_beta_offset_div2;
    int8_t                  pps_tc_offset_div2;
    uint8_t                 num_extra_slice_header_bits;

    /**
     * \brief number of bits that structure
     * short_term_ref_pic_set( num_short_term_ref_pic_sets ) takes in slice
     * segment header when short_term_ref_pic_set_sps_flag equals 0.
     * if short_term_ref_pic_set_sps_flag equals 1, the value should be 0.
     * the bit count is calculated after emulation prevention bytes are removed
     * from bit streams.
     * This variable is used for accelorater to skip parsing the
     * short_term_ref_pic_set( num_short_term_ref_pic_sets ) structure.
     */
    uint32_t                st_rps_bits;

    /** \brief Reserved bytes for future use, must be zero */
    uint32_t                va_reserved[VA_PADDING_MEDIUM];
} VAPictureParameterBufferHEVC;

typedef struct  _VASliceParameterBufferHEVC {
    /** @name Codec-independent Slice Parameter Buffer base. */

    /**@{*/

    /** \brief Number of bytes in the slice data buffer for this slice
     * counting from and including NAL unit header.
     */
    uint32_t                slice_data_size;
    /** \brief The offset to the NAL unit header for this slice */
    uint32_t                slice_data_offset;
    /** \brief Slice data buffer flags. See \c VA_SLICE_DATA_FLAG_XXX. */
    uint32_t                slice_data_flag;
    /**
     * \brief Byte offset from NAL unit header to the begining of slice_data().
     *
     * This byte offset is relative to and includes the NAL unit header
     * and represents the number of bytes parsed in the slice_header()
     * after the removal of any emulation prevention bytes in
     * there. However, the slice data buffer passed to the hardware is
     * the original bitstream, thus including any emulation prevention
     * bytes.
     */
    uint32_t                slice_data_byte_offset;
    /** HEVC syntax element. */
    uint32_t                slice_segment_address;
    /** \brief index into ReferenceFrames[]
     * RefPicList[0][] corresponds to RefPicList0[] of HEVC variable.
     * RefPicList[1][] corresponds to RefPicList1[] of HEVC variable.
     * value range [0..14, 0xFF], where 0xFF indicates invalid entry.
     */
    uint8_t                 RefPicList[2][15];
    union {
        uint32_t            value;
        struct {
            /** current slice is last slice of picture. */
            uint32_t        LastSliceOfPic                              : 1;
            /** HEVC syntax element. */
            uint32_t        dependent_slice_segment_flag                : 1;
            /** HEVC syntax element. */
            uint32_t        slice_type                                  : 2;
            /** HEVC syntax element. */
            uint32_t        color_plane_id                              : 2;
            /** HEVC syntax element. */
            uint32_t        slice_sao_luma_flag                         : 1;
            /** HEVC syntax element. */
            uint32_t        slice_sao_chroma_flag                       : 1;
            /** HEVC syntax element. */
            uint32_t        mvd_l1_zero_flag                            : 1;
            /** HEVC syntax element. */
            uint32_t        cabac_init_flag                             : 1;
            /** HEVC syntax element. */
            uint32_t        slice_temporal_mvp_enabled_flag             : 1;
            /** HEVC syntax element. */
            uint32_t        slice_deblocking_filter_disabled_flag       : 1;
            /** HEVC syntax element. */
            uint32_t        collocated_from_l0_flag                     : 1;
            /** HEVC syntax element. */
            uint32_t        slice_loop_filter_across_slices_enabled_flag : 1;
            uint32_t        reserved                                    : 18;
        } fields;
    } LongSliceFlags;

    /** HEVC syntax element. Collocated Reference Picture Index.
     * index to RefPicList[0][] or RefPicList[1][].
     * when slice_temporal_mvp_enabled_flag equals 0, it should take value 0xFF.
     * value range [0..14, 0xFF].
     */
    uint8_t                 collocated_ref_idx;
    /** HEVC syntax element.
     * if num_ref_idx_active_override_flag equals 0, host decoder should
     * set its value to num_ref_idx_l0_default_active_minus1.
     */
    uint8_t                 num_ref_idx_l0_active_minus1;
    /** HEVC syntax element.
     * if num_ref_idx_active_override_flag equals 0, host decoder should
     * set its value to num_ref_idx_l1_default_active_minus1.
     */
    uint8_t                 num_ref_idx_l1_active_minus1;
    /** HEVC syntax element. */
    int8_t                  slice_qp_delta;
    /** HEVC syntax element. */
    int8_t                  slice_cb_qp_offset;
    /** HEVC syntax element. */
    int8_t                  slice_cr_qp_offset;
    /** HEVC syntax element. */
    int8_t                  slice_beta_offset_div2;
    /** HEVC syntax element. */
    int8_t                  slice_tc_offset_div2;
    /** HEVC syntax element. */
    uint8_t                 luma_log2_weight_denom;
    /** HEVC syntax element. */
    int8_t                  delta_chroma_log2_weight_denom;
    /** HEVC syntax element. */
    int8_t                  delta_luma_weight_l0[15];
    /** HEVC syntax element. */
    int8_t                  luma_offset_l0[15];
    /** HEVC syntax element. */
    int8_t                  delta_chroma_weight_l0[15][2];
    /** corresponds to HEVC spec variable of the same name. */
    int8_t                  ChromaOffsetL0[15][2];
    /** HEVC syntax element. */
    int8_t                  delta_luma_weight_l1[15];
    /** HEVC syntax element. */
    int8_t                  luma_offset_l1[15];
    /** HEVC syntax element. */
    int8_t                  delta_chroma_weight_l1[15][2];
    /** corresponds to HEVC spec variable of the same name. */
    int8_t                  ChromaOffsetL1[15][2];
    /** HEVC syntax element. */
    uint8_t                 five_minus_max_num_merge_cand;
    /** HEVC syntax element. */
    uint16_t                 num_entry_point_offsets;
    /** HEVC syntax element. */
    uint16_t                 entry_offset_to_subset_array;
    /** \brief Number of emulation prevention bytes in slice header. */
    uint16_t                slice_data_num_emu_prevn_bytes;
    /**@}*/

    /** \brief Reserved bytes for future use, must be zero */
    uint32_t                va_reserved[VA_PADDING_LOW - 2];
} VASliceParameterBufferHEVC;

typedef struct _VAIQMatrixBufferHEVC {
    /**
     * \brief scaling lists,
     * corresponds to same HEVC spec syntax element
     * ScalingList[ i ][ MatrixID ][ j ].
     *
     * \brief 4x4 scaling,
     * correspongs i = 0, MatrixID is in the range of 0 to 5,
     * inclusive. And j is in the range of 0 to 15, inclusive.
     */
    uint8_t                 ScalingList4x4[6][16];
    /**
     * \brief 8x8 scaling,
     * correspongs i = 1, MatrixID is in the range of 0 to 5,
     * inclusive. And j is in the range of 0 to 63, inclusive.
     */
    uint8_t                 ScalingList8x8[6][64];
    /**
     * \brief 16x16 scaling,
     * correspongs i = 2, MatrixID is in the range of 0 to 5,
     * inclusive. And j is in the range of 0 to 63, inclusive.
     */
    uint8_t                 ScalingList16x16[6][64];
    /**
     * \brief 32x32 scaling,
     * correspongs i = 3, MatrixID is in the range of 0 to 1,
     * inclusive. And j is in the range of 0 to 63, inclusive.
     */
    uint8_t                 ScalingList32x32[2][64];
    /**
     * \brief DC values of the 16x16 scaling lists,
     * corresponds to HEVC spec syntax
     * scaling_list_dc_coef_minus8[ sizeID - 2 ][ matrixID ] + 8
     * with sizeID = 2 and matrixID in the range of 0 to 5, inclusive.
     */
    uint8_t                 ScalingListDC16x16[6];
    /**
     * \brief DC values of the 32x32 scaling lists,
     * corresponds to HEVC spec syntax
     * scaling_list_dc_coef_minus8[ sizeID - 2 ][ matrixID ] + 8
     * with sizeID = 3 and matrixID in the range of 0 to 1, inclusive.
     */
    uint8_t                 ScalingListDC32x32[2];

    /** \brief Reserved bytes for future use, must be zero */
    uint32_t                va_reserved[VA_PADDING_LOW];
} VAIQMatrixBufferHEVC;


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

/* VC-1 (va.h) */
typedef struct _VAPictureParameterBufferVC1 {
    VASurfaceID forward_reference_picture;
    VASurfaceID backward_reference_picture;
    VASurfaceID inloop_decoded_picture;
    union {
        struct {
            uint32_t pulldown       : 1;
            uint32_t interlace      : 1;
            uint32_t tfcntrflag     : 1;
            uint32_t finterpflag    : 1;
            uint32_t psf            : 1;
            uint32_t multires       : 1;
            uint32_t overlap        : 1;
            uint32_t syncmarker     : 1;
            uint32_t rangered       : 1;
            uint32_t max_b_frames   : 3;
            uint32_t profile        : 2;
        } bits;
        uint32_t value;
    } sequence_fields;
    uint16_t coded_width;
    uint16_t coded_height;
    union {
        struct {
            uint32_t broken_link    : 1;
            uint32_t closed_entry   : 1;
            uint32_t panscan_flag   : 1;
            uint32_t loopfilter     : 1;
        } bits;
        uint32_t value;
    } entrypoint_fields;
    uint8_t conditional_overlap_flag;
    uint8_t fast_uvmc_flag;
    union {
        struct {
            uint32_t luma_flag      : 1;
            uint32_t luma           : 3;
            uint32_t chroma_flag    : 1;
            uint32_t chroma         : 3;
        } bits;
        uint32_t value;
    } range_mapping_fields;
    uint8_t b_picture_fraction;
    uint8_t cbp_table;
    uint8_t mb_mode_table;
    uint8_t range_reduction_frame;
    uint8_t rounding_control;
    uint8_t post_processing;
    uint8_t picture_resolution_index;
    uint8_t luma_scale;
    uint8_t luma_shift;
    union {
        struct {
            uint32_t picture_type           : 3;
            uint32_t frame_coding_mode      : 3;
            uint32_t top_field_first        : 1;
            uint32_t is_first_field         : 1;
            uint32_t intensity_compensation : 1;
        } bits;
        uint32_t value;
    } picture_fields;
    union {
        struct {
            uint32_t mv_type_mb : 1;
            uint32_t direct_mb  : 1;
            uint32_t skip_mb    : 1;
            uint32_t field_tx   : 1;
            uint32_t forward_mb : 1;
            uint32_t ac_pred    : 1;
            uint32_t overflags  : 1;
        } flags;
        uint32_t value;
    } raw_coding;
    union {
        struct {
            uint32_t bp_mv_type_mb  : 1;
            uint32_t bp_direct_mb   : 1;
            uint32_t bp_skip_mb     : 1;
            uint32_t bp_field_tx    : 1;
            uint32_t bp_forward_mb  : 1;
            uint32_t bp_ac_pred     : 1;
            uint32_t bp_overflags   : 1;
        } flags;
        uint32_t value;
    } bitplane_present;
    union {
        struct {
            uint32_t reference_distance_flag        : 1;
            uint32_t reference_distance             : 5;
            uint32_t num_reference_pictures         : 1;
            uint32_t reference_field_pic_indicator  : 1;
        } bits;
        uint32_t value;
    } reference_fields;
    union {
        struct {
            uint32_t mv_mode                        : 3;
            uint32_t mv_mode2                       : 3;
            uint32_t mv_table                       : 3;
            uint32_t two_mv_block_pattern_table     : 2;
            uint32_t four_mv_switch                 : 1;
            uint32_t four_mv_block_pattern_table    : 2;
            uint32_t extended_mv_flag               : 1;
            uint32_t extended_mv_range              : 2;
            uint32_t extended_dmv_flag              : 1;
            uint32_t extended_dmv_range             : 2;
        } bits;
        uint32_t value;
    } mv_fields;
    union {
        struct {
            uint32_t dquant                 : 2;
            uint32_t quantizer              : 2;
            uint32_t half_qp                : 1;
            uint32_t pic_quantizer_scale    : 5;
            uint32_t pic_quantizer_type     : 1;
            uint32_t dq_frame               : 1;
            uint32_t dq_profile             : 2;
            uint32_t dq_sb_edge             : 2;
            uint32_t dq_db_edge             : 2;
            uint32_t dq_binary_level        : 1;
            uint32_t alt_pic_quantizer      : 5;
        } bits;
        uint32_t value;
    } pic_quantizer_fields;
    union {
        struct {
            uint32_t variable_sized_transform_flag  : 1;
            uint32_t mb_level_transform_type_flag   : 1;
            uint32_t frame_level_transform_type     : 2;
            uint32_t transform_ac_codingset_idx1    : 2;
            uint32_t transform_ac_codingset_idx2    : 2;
            uint32_t intra_transform_dc_table       : 1;
        } bits;
        uint32_t value;
    } transform_fields;
    uint8_t luma_scale2;
    uint8_t luma_shift2;
    uint8_t intensity_compensation_field;
    uint32_t                va_reserved[VA_PADDING_MEDIUM - 1];
} VAPictureParameterBufferVC1;

typedef struct _VASliceParameterBufferVC1 {
    uint32_t slice_data_size;
    uint32_t slice_data_offset;
    uint32_t slice_data_flag;
    uint32_t macroblock_offset;
    uint32_t slice_vertical_position;
    uint32_t                va_reserved[VA_PADDING_LOW];
} VASliceParameterBufferVC1;

/* VP8 (va_dec_vp8.h) */
typedef struct _VABoolCoderContextVPX {
    uint8_t range;
    uint8_t value;
    uint8_t count;
} VABoolCoderContextVPX;

typedef struct _VAPictureParameterBufferVP8 {
    uint32_t frame_width;
    uint32_t frame_height;
    VASurfaceID last_ref_frame;
    VASurfaceID golden_ref_frame;
    VASurfaceID alt_ref_frame;
    VASurfaceID out_of_loop_frame;
    union {
        struct {
            uint32_t key_frame                      : 1;    /* 0 = key frame */
            uint32_t version                        : 3;
            uint32_t segmentation_enabled           : 1;
            uint32_t update_mb_segmentation_map     : 1;
            uint32_t update_segment_feature_data    : 1;
            uint32_t filter_type                    : 1;
            uint32_t sharpness_level                : 3;
            uint32_t loop_filter_adj_enable         : 1;
            uint32_t mode_ref_lf_delta_update       : 1;
            uint32_t sign_bias_golden               : 1;
            uint32_t sign_bias_alternate            : 1;
            uint32_t mb_no_coeff_skip               : 1;
            uint32_t loop_filter_disable            : 1;
        } bits;
        uint32_t value;
    } pic_fields;
    uint8_t mb_segment_tree_probs[3];
    uint8_t loop_filter_level[4];
    int8_t loop_filter_deltas_ref_frame[4];
    int8_t loop_filter_deltas_mode[4];
    uint8_t prob_skip_false;
    uint8_t prob_intra;
    uint8_t prob_last;
    uint8_t prob_gf;
    uint8_t y_mode_probs[4];
    uint8_t uv_mode_probs[3];
    uint8_t mv_probs[2][19];
    VABoolCoderContextVPX bool_coder_ctx;
    uint32_t                va_reserved[VA_PADDING_LOW];
} VAPictureParameterBufferVP8;

typedef struct _VASliceParameterBufferVP8 {
    uint32_t slice_data_size;
    uint32_t slice_data_offset;
    uint32_t slice_data_flag;
    uint32_t macroblock_offset;         /* bits from the start of partition 1 */
    uint8_t num_of_partitions;
    uint32_t partition_size[9];
    uint32_t                va_reserved[VA_PADDING_LOW];
} VASliceParameterBufferVP8;

/* VP9 (va_dec_vp9.h) */
typedef struct _VADecPictureParameterBufferVP9 {
    uint16_t frame_width;
    uint16_t frame_height;
    VASurfaceID reference_frames[8];
    union {
        struct {
            uint32_t subsampling_x                  : 1;
            uint32_t subsampling_y                  : 1;
            uint32_t frame_type                     : 1;    /* 0 = key frame */
            uint32_t show_frame                     : 1;
            uint32_t error_resilient_mode           : 1;
            uint32_t intra_only                     : 1;
            uint32_t allow_high_precision_mv        : 1;
            uint32_t mcomp_filter_type              : 3;    /* libvpx order, 4 = switchable */
            uint32_t frame_parallel_decoding_mode   : 1;
            uint32_t reset_frame_context            : 2;
            uint32_t refresh_frame_context          : 1;
            uint32_t frame_context_idx              : 2;
            uint32_t segmentation_enabled           : 1;
            uint32_t segmentation_temporal_update   : 1;
            uint32_t segmentation_update_map        : 1;
            uint32_t last_ref_frame                 : 3;
            uint32_t last_ref_frame_sign_bias       : 1;
            uint32_t golden_ref_frame               : 3;
            uint32_t golden_ref_frame_sign_bias     : 1;
            uint32_t alt_ref_frame                  : 3;
            uint32_t alt_ref_frame_sign_bias        : 1;
            uint32_t lossless_flag                  : 1;
        } bits;
        uint32_t value;
    } pic_fields;
    uint8_t filter_level;
    uint8_t sharpness_level;
    uint8_t log2_tile_rows;
    uint8_t log2_tile_columns;
    uint8_t frame_header_length_in_bytes;   /* uncompressed header */
    uint16_t first_partition_size;          /* compressed header */
    uint8_t mb_segment_tree_probs[7];
    uint8_t segment_pred_probs[3];
    uint8_t profile;
    uint8_t bit_depth;
    uint32_t                va_reserved[VA_PADDING_MEDIUM];
} VADecPictureParameterBufferVP9;

typedef struct _VASegmentParameterVP9 {
    union {
        struct {
            uint16_t segment_reference_enabled      : 1;
            uint16_t segment_reference              : 2;
            uint16_t segment_reference_skipped      : 1;
        } fields;
        uint16_t value;
    } segment_flags;
    uint8_t filter_level[4][2];
    int16_t luma_ac_quant_scale;
    int16_t luma_dc_quant_scale;
    int16_t chroma_ac_quant_scale;
    int16_t chroma_dc_quant_scale;
    uint32_t                va_reserved[VA_PADDING_LOW];
} VASegmentParameterVP9;

typedef struct _VASliceParameterBufferVP9 {
    uint32_t slice_data_size;
    uint32_t slice_data_offset;
    uint32_t slice_data_flag;
    VASegmentParameterVP9 seg_param[8];
    uint32_t                va_reserved[VA_PADDING_LOW];
} VASliceParameterBufferVP9;

/* AV1 (va_dec_av1.h) */
typedef int32_t VAAV1TransformationType;    /* enum: 0 identity, 1 translation, 2 rotzoom, 3 affine */

typedef struct _VASegmentationStructAV1 {
    union {
        struct {
            uint32_t         enabled                                     : 1;
            uint32_t         update_map                                  : 1;
            uint32_t         temporal_update                             : 1;
            uint32_t         update_data                                 : 1;
            uint32_t         reserved                                    : 28;
        } bits;
        uint32_t             value;
    } segment_info_fields;
    int16_t                 feature_data[8][8];
    uint8_t                 feature_mask[8];
    uint32_t                va_reserved[VA_PADDING_LOW];
} VASegmentationStructAV1;

typedef struct _VAFilmGrainStructAV1 {
    union {
        struct {
            uint32_t        apply_grain                                 : 1;
            uint32_t        chroma_scaling_from_luma                    : 1;
            uint32_t        grain_scaling_minus_8                       : 2;
            uint32_t        ar_coeff_lag                                : 2;
            uint32_t        ar_coeff_shift_minus_6                      : 2;
            uint32_t        grain_scale_shift                           : 2;
            uint32_t        overlap_flag                                : 1;
            uint32_t        clip_to_restricted_range                    : 1;
            uint32_t        reserved                                    : 20;
        } bits;
        uint32_t            value;
    } film_grain_info_fields;
    uint16_t                grain_seed;
    uint8_t                 num_y_points;
    uint8_t                 point_y_value[14];
    uint8_t                 point_y_scaling[14];
    uint8_t                 num_cb_points;
    uint8_t                 point_cb_value[10];
    uint8_t                 point_cb_scaling[10];
    uint8_t                 num_cr_points;
    uint8_t                 point_cr_value[10];
    uint8_t                 point_cr_scaling[10];
    int8_t                  ar_coeffs_y[24];
    int8_t                  ar_coeffs_cb[25];
    int8_t                  ar_coeffs_cr[25];
    uint8_t                 cb_mult;
    uint8_t                 cb_luma_mult;
    uint16_t                cb_offset;
    uint8_t                 cr_mult;
    uint8_t                 cr_luma_mult;
    uint16_t                cr_offset;
    uint32_t                va_reserved[VA_PADDING_LOW];
} VAFilmGrainStructAV1;

typedef struct _VAWarpedMotionParamsAV1 {
    VAAV1TransformationType  wmtype;
    int32_t                 wmmat[8];
    uint8_t  invalid;
    uint32_t                va_reserved[VA_PADDING_LOW];
} VAWarpedMotionParamsAV1;

typedef struct  _VADecPictureParameterBufferAV1 {
    uint8_t                 profile;
    uint8_t                 order_hint_bits_minus_1;
    uint8_t                 bit_depth_idx;
    uint8_t                 matrix_coefficients;
    union {
        struct {
            uint32_t        still_picture                               : 1;
            uint32_t        use_128x128_superblock                      : 1;
            uint32_t        enable_filter_intra                         : 1;
            uint32_t        enable_intra_edge_filter                    : 1;
            uint32_t        enable_interintra_compound                  : 1;
            uint32_t        enable_masked_compound                      : 1;
            uint32_t        enable_dual_filter                          : 1;
            uint32_t        enable_order_hint                           : 1;
            uint32_t        enable_jnt_comp                             : 1;
            uint32_t        enable_cdef                                 : 1;
            uint32_t        mono_chrome                                 : 1;
            uint32_t        color_range                                 : 1;
            uint32_t        subsampling_x                               : 1;
            uint32_t        subsampling_y                               : 1;
            va_deprecated uint32_t        chroma_sample_position        : 1;
            uint32_t        film_grain_params_present                   : 1;
            uint32_t        reserved                                    : 16;
        } fields;
        uint32_t value;
    } seq_info_fields;
    VASurfaceID             current_frame;
    VASurfaceID             current_display_picture;
    uint8_t                anchor_frames_num;
    VASurfaceID             *anchor_frames_list;
    uint16_t                frame_width_minus1;
    uint16_t                frame_height_minus1;
    uint16_t                output_frame_width_in_tiles_minus_1;
    uint16_t                output_frame_height_in_tiles_minus_1;
    VASurfaceID             ref_frame_map[8];
    uint8_t                 ref_frame_idx[7];
    uint8_t                 primary_ref_frame;
    uint8_t                 order_hint;
    VASegmentationStructAV1 seg_info;
    VAFilmGrainStructAV1    film_grain_info;
    uint8_t                 tile_cols;
    uint8_t                 tile_rows;
    uint16_t                width_in_sbs_minus_1[63];
    uint16_t                height_in_sbs_minus_1[63];
    uint16_t                tile_count_minus_1;
    uint16_t                context_update_tile_id;
    union {
        struct {
            uint32_t        frame_type                                  : 2;
            uint32_t        show_frame                                  : 1;
            uint32_t        showable_frame                              : 1;
            uint32_t        error_resilient_mode                        : 1;
            uint32_t        disable_cdf_update                          : 1;
            uint32_t        allow_screen_content_tools                  : 1;
            uint32_t        force_integer_mv                            : 1;
            uint32_t        allow_intrabc                               : 1;
            uint32_t        use_superres                                : 1;
            uint32_t        allow_high_precision_mv                     : 1;
            uint32_t        is_motion_mode_switchable                   : 1;
            uint32_t        use_ref_frame_mvs                           : 1;
            uint32_t        disable_frame_end_update_cdf                : 1;
            uint32_t        uniform_tile_spacing_flag                   : 1;
            uint32_t        allow_warped_motion                         : 1;
            uint32_t        large_scale_tile                            : 1;
            uint32_t        reserved                                    : 15;
        } bits;
        uint32_t            value;
    } pic_info_fields;
    uint8_t                 superres_scale_denominator;
    uint8_t                 interp_filter;
    uint8_t                 filter_level[2];
    uint8_t                 filter_level_u;
    uint8_t                 filter_level_v;
    union {
        struct {
            uint8_t         sharpness_level                             : 3;
            uint8_t         mode_ref_delta_enabled                      : 1;
            uint8_t         mode_ref_delta_update                       : 1;
            uint8_t         reserved                                    : 3;
        } bits;
        uint8_t             value;
    } loop_filter_info_fields;
    int8_t                  ref_deltas[8];
    int8_t                  mode_deltas[2];
    uint8_t                base_qindex;
    int8_t                  y_dc_delta_q;
    int8_t                  u_dc_delta_q;
    int8_t                  u_ac_delta_q;
    int8_t                  v_dc_delta_q;
    int8_t                  v_ac_delta_q;
    union {
        struct {
            uint16_t        using_qmatrix                               : 1;
            uint16_t        qm_y                                        : 4;
            uint16_t        qm_u                                        : 4;
            uint16_t        qm_v                                        : 4;
            uint16_t        reserved                                    : 3;
        } bits;
        uint16_t            value;
    } qmatrix_fields;
    union {
        struct {
            uint32_t        delta_q_present_flag                        : 1;
            uint32_t        log2_delta_q_res                            : 2;
            uint32_t        delta_lf_present_flag                       : 1;
            uint32_t        log2_delta_lf_res                           : 2;
            uint32_t        delta_lf_multi                              : 1;
            uint32_t        tx_mode                                     : 2;
            uint32_t        reference_select                            : 1;
            uint32_t        reduced_tx_set_used                         : 1;
            uint32_t        skip_mode_present                           : 1;
            uint32_t        reserved                                    : 20;
        } bits;
        uint32_t            value;
    } mode_control_fields;
    uint8_t                 cdef_damping_minus_3;
    uint8_t                 cdef_bits;
    uint8_t                 cdef_y_strengths[8];
    uint8_t                 cdef_uv_strengths[8];
    union {
        struct {
            uint16_t        yframe_restoration_type                     : 2;
            uint16_t        cbframe_restoration_type                    : 2;
            uint16_t        crframe_restoration_type                    : 2;
            uint16_t        lr_unit_shift                               : 2;
            uint16_t        lr_uv_shift                                 : 1;
            uint16_t        reserved                                    : 7;
        } bits;
        uint16_t            value;
    } loop_restoration_fields;
    VAWarpedMotionParamsAV1 wm[7];
    uint32_t                va_reserved[VA_PADDING_MEDIUM];
} VADecPictureParameterBufferAV1;

typedef struct _VASliceParameterBufferAV1 {
    uint32_t                slice_data_size;
    uint32_t                slice_data_offset;
    uint32_t                slice_data_flag;
    uint16_t                tile_row;
    uint16_t                tile_column;
    va_deprecated uint16_t  tg_start;
    va_deprecated uint16_t  tg_end;
    uint8_t                 anchor_frame_idx;
    uint16_t                tile_idx_in_tile_list;
    uint32_t                va_reserved[VA_PADDING_LOW];
} VASliceParameterBufferAV1;


#endif /* NOUVEAU_VADEC_H */
