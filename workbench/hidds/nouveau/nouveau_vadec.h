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

#endif /* NOUVEAU_VADEC_H */
