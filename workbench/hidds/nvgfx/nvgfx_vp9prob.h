/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: VP9 header parsing and probability management for NVDEC (the
          engine needs the driver to parse both frame headers, keep the four
          frame contexts, apply the forward updates and adapt from its symbol
          counters). Plain C so that it also builds on a host for testing
          against tables traced from NVIDIA's driver.
*/
#ifndef NOUVEAU_VP9PROB_H
#define NOUVEAU_VP9PROB_H

#include <stdint.h>

#define VP9_NVDEC_PROBTAB_SIZE      0xea0       /* nvdec_vp9EntropyProbs_t */
#define VP9_NVDEC_COUNTS_SIZE       0x33d0      /* nvdec_vp9EntropyCounts_t */

/* one frame context, libvpx order */
struct vp9_fc
{
    uint8_t coef[4][2][2][6][6][3];
    uint8_t y_mode[4][9];
    uint8_t uv_mode[10][9];
    uint8_t partition[16][3];
    uint8_t skip[3];
    uint8_t tx8[2][1], tx16[2][2], tx32[2][3];
    uint8_t inter_mode[7][3];
    uint8_t interp[4][2];
    uint8_t intra_inter[4];
    uint8_t comp_inter[5];
    uint8_t single_ref[5][2];
    uint8_t comp_ref[5];
    uint8_t mv_joints[3];
    uint8_t mv_sign[2], mv_classes[2][10], mv_class0[2][1], mv_bits[2][10];
    uint8_t mv_class0_fp[2][2][3], mv_fp[2][3], mv_class0_hp[2], mv_hp[2];
};

/* one frame's headers */
struct vp9_hdr
{
    int profile, bit_depth;
    int show_existing, existing_idx;
    int key, show_frame, error_res, intra_only, reset_ctx;
    int refresh_flags, ref_idx[3], sign_bias[3];
    int width, height;
    int hp, interp;                 /* interp: libvpx order, 4 = switchable */
    int refresh_ctx, parallel, ctx_idx;
    int lf_level, lf_sharpness;
    int base_q, dq_y_dc, dq_uv_dc, dq_uv_ac, lossless;
    int seg_enabled, seg_update_map, seg_temporal, seg_update_data;
    uint8_t seg_tree_probs[7], seg_pred_probs[3];
    int log2_tile_cols, log2_tile_rows;
    int uh_bytes, ch_bytes;         /* uncompressed header bytes, compressed header bytes */
    /* compressed header */
    int tx_mode, reference_mode, comp_fixed_ref, comp_var_ref[2];
};

/* session state */
struct vp9_state
{
    struct vp9_fc ctx[4];
    int bit_depth;                  /* from the last key / intra-only frame */
    /* persistent across frames */
    int lf_delta_enabled;
    int8_t lf_ref_deltas[4], lf_mode_deltas[2];
    int seg_abs;
    uint8_t seg_feature_enabled[8][4];
    int16_t seg_feature_data[8][4];
    int last_width, last_height, last_key, last_show, last_intra_only, have_last;
    /* this frame */
    struct vp9_fc probs;            /* after the forward updates */
    int ctx_used;                   /* the context index the frame loaded */
};

void vp9_state_init(struct vp9_state *s);
/* parse the uncompressed header; width/height known from the container/VA when the frame takes its size from a reference */
int vp9_parse_uncompressed(struct vp9_state *s, const uint8_t *data, unsigned int size, struct vp9_hdr *h,
                           int width, int height);
/* contexts reset/load, then the compressed header's forward updates into s->probs */
int vp9_parse_compressed(struct vp9_state *s, const uint8_t *data, unsigned int size, struct vp9_hdr *h);
/* the NVDEC probability table for this frame */
void vp9_fill_nvdec_probs(const struct vp9_state *s, const struct vp9_hdr *h, uint8_t *table);
/* after the frame: save or adapt the context (counts: nvdec_vp9EntropyCounts_t, or NULL when not adapting) */
void vp9_finish_frame(struct vp9_state *s, const struct vp9_hdr *h, const uint32_t *counts);
/* does the frame need its counters read back (backward adaptation)? */
int vp9_needs_counts(const struct vp9_hdr *h);

#endif
