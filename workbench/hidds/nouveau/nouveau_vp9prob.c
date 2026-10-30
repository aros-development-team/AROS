/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: VP9 header parsing and probability management for NVDEC.

          Follows the VP9 bitstream specification (uncompressed header 6.2,
          compressed header 6.3, bool decoder 9.2, adaptation 8.4) and
          libvpx's context handling (vp9_setup_past_independence, the
          refresh/adapt order of vp9_decodeframe.c). The default
          probabilities and the adaptation constants are libvpx's (BSD,
          see nouveau_vp9tables.h). The NVDEC table and counter layouts are
          nvdec_drv.h's nvdec_vp9EntropyProbs_t / nvdec_vp9EntropyCounts_t,
          checked against tables traced from NVIDIA's driver.
*/

#include <string.h>
#include "nouveau_vp9prob.h"
#include "nouveau_vp9tables.h"

/* ---- bit and bool readers ---------------------------------------------- */

struct vp9_bits
{
    const uint8_t *d;
    unsigned int size, pos;     /* in bits */
};

static int vb_f(struct vp9_bits *b, int n)
{
    int v = 0;

    while (n--)
    {
        int bit = 0;

        if (b->pos < b->size * 8)
            bit = (b->d[b->pos >> 3] >> (7 - (b->pos & 7))) & 1;
        b->pos++;
        v = (v << 1) | bit;
    }
    return v;
}

static int vb_s(struct vp9_bits *b, int n)
{
    int v = vb_f(b, n);

    return vb_f(b, 1) ? -v : v;
}

struct vp9_bool
{
    struct vp9_bits bits;
    unsigned int value, range;
    int maxbits;
};

static int vbool_read(struct vp9_bool *r, int p)
{
    unsigned int split = 1 + (((r->range - 1) * (unsigned int)p) >> 8);
    int bit;

    if (r->value < split)
    {
        r->range = split;
        bit = 0;
    }
    else
    {
        r->range -= split;
        r->value -= split;
        bit = 1;
    }
    while (r->range < 128)
    {
        int nb = 0;

        if (r->maxbits > 0)
        {
            nb = vb_f(&r->bits, 1);
            r->maxbits--;
        }
        r->range <<= 1;
        r->value = (r->value << 1) | nb;
    }
    return bit;
}

static int vbool_init(struct vp9_bool *r, const uint8_t *d, unsigned int size)
{
    r->bits.d = d;
    r->bits.size = size;
    r->bits.pos = 0;
    if (size < 1)
        return -1;
    r->value = vb_f(&r->bits, 8);
    r->range = 255;
    r->maxbits = 8 * (int)size - 8;
    return vbool_read(r, 128) ? -1 : 0;        /* marker bit */
}

static int vbool_lit(struct vp9_bool *r, int n)
{
    int v = 0;

    while (n--)
        v = (v << 1) | vbool_read(r, 128);
    return v;
}

/* ---- forward probability updates (6.3, libvpx vp9_dsubexp.c) ---------- */

static int inv_recenter_nonneg(int v, int m)
{
    if (v > 2 * m)
        return v;
    return (v & 1) ? m - ((v + 1) >> 1) : m + (v >> 1);
}

static int inv_remap_prob(int v, int m)
{
    static const uint8_t inv_map_table[255] = {
        7,   20,  33,  46,  59,  72,  85,  98,  111, 124, 137, 150, 163, 176, 189,
        202, 215, 228, 241, 254, 1,   2,   3,   4,   5,   6,   8,   9,   10,  11,
        12,  13,  14,  15,  16,  17,  18,  19,  21,  22,  23,  24,  25,  26,  27,
        28,  29,  30,  31,  32,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,
        44,  45,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  60,
        61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  73,  74,  75,  76,
        77,  78,  79,  80,  81,  82,  83,  84,  86,  87,  88,  89,  90,  91,  92,
        93,  94,  95,  96,  97,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108,
        109, 110, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 125,
        126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 138, 139, 140, 141,
        142, 143, 144, 145, 146, 147, 148, 149, 151, 152, 153, 154, 155, 156, 157,
        158, 159, 160, 161, 162, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173,
        174, 175, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 190,
        191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 203, 204, 205, 206,
        207, 208, 209, 210, 211, 212, 213, 214, 216, 217, 218, 219, 220, 221, 222,
        223, 224, 225, 226, 227, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238,
        239, 240, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 253
    };

    if (v > 254)
        v = 254;
    v = inv_map_table[v];
    m--;
    if ((m << 1) <= 255)
        return 1 + inv_recenter_nonneg(v, m);
    return 255 - inv_recenter_nonneg(v, 255 - 1 - m);
}

static int decode_uniform(struct vp9_bool *r)
{
    const int l = 8, m = (1 << l) - 191;
    int v = vbool_lit(r, l - 1);

    return (v < m) ? v : (v << 1) - m + vbool_lit(r, 1);
}

static int decode_term_subexp(struct vp9_bool *r)
{
    if (!vbool_lit(r, 1))
        return vbool_lit(r, 4);
    if (!vbool_lit(r, 1))
        return vbool_lit(r, 4) + 16;
    if (!vbool_lit(r, 1))
        return vbool_lit(r, 5) + 32;
    return decode_uniform(r) + 64;
}

static void diff_update(struct vp9_bool *r, uint8_t *p)
{
    if (vbool_read(r, 252))
        *p = (uint8_t)inv_remap_prob(decode_term_subexp(r), *p);
}

static void mv_update(struct vp9_bool *r, uint8_t *p)
{
    if (vbool_read(r, 252))
        *p = (uint8_t)((vbool_lit(r, 7) << 1) | 1);
}

/* ---- contexts --------------------------------------------------------- */

static void vp9_default_fc(struct vp9_fc *fc)
{
    memcpy(fc->coef[0], vp9_default_coef_4x4, sizeof(fc->coef[0]));
    memcpy(fc->coef[1], vp9_default_coef_8x8, sizeof(fc->coef[1]));
    memcpy(fc->coef[2], vp9_default_coef_16x16, sizeof(fc->coef[2]));
    memcpy(fc->coef[3], vp9_default_coef_32x32, sizeof(fc->coef[3]));
    memcpy(fc->y_mode, vp9_default_y_mode, sizeof(fc->y_mode));
    memcpy(fc->uv_mode, vp9_default_uv_mode, sizeof(fc->uv_mode));
    memcpy(fc->partition, vp9_default_partition, sizeof(fc->partition));
    memcpy(fc->skip, vp9_default_skip, sizeof(fc->skip));
    memcpy(fc->tx8, vp9_default_tx8, sizeof(fc->tx8));
    memcpy(fc->tx16, vp9_default_tx16, sizeof(fc->tx16));
    memcpy(fc->tx32, vp9_default_tx32, sizeof(fc->tx32));
    memcpy(fc->inter_mode, vp9_default_inter_mode, sizeof(fc->inter_mode));
    memcpy(fc->interp, vp9_default_interp, sizeof(fc->interp));
    memcpy(fc->intra_inter, vp9_default_intra_inter, sizeof(fc->intra_inter));
    memcpy(fc->comp_inter, vp9_default_comp_inter, sizeof(fc->comp_inter));
    memcpy(fc->single_ref, vp9_default_single_ref, sizeof(fc->single_ref));
    memcpy(fc->comp_ref, vp9_default_comp_ref, sizeof(fc->comp_ref));
    memcpy(fc->mv_joints, vp9_default_mv_joints, sizeof(fc->mv_joints));
    memcpy(fc->mv_sign, vp9_default_mv_sign, sizeof(fc->mv_sign));
    memcpy(fc->mv_classes, vp9_default_mv_classes, sizeof(fc->mv_classes));
    memcpy(fc->mv_class0, vp9_default_mv_class0, sizeof(fc->mv_class0));
    memcpy(fc->mv_bits, vp9_default_mv_bits, sizeof(fc->mv_bits));
    memcpy(fc->mv_class0_fp, vp9_default_mv_class0_fp, sizeof(fc->mv_class0_fp));
    memcpy(fc->mv_fp, vp9_default_mv_fp, sizeof(fc->mv_fp));
    memcpy(fc->mv_class0_hp, vp9_default_mv_class0_hp, sizeof(fc->mv_class0_hp));
    memcpy(fc->mv_hp, vp9_default_mv_hp, sizeof(fc->mv_hp));
}

void vp9_state_init(struct vp9_state *s)
{
    int i;

    memset(s, 0, sizeof(*s));
    for (i = 0; i < 4; i++)
        vp9_default_fc(&s->ctx[i]);
    s->lf_ref_deltas[0] = 1;
    s->lf_ref_deltas[2] = -1;
    s->lf_ref_deltas[3] = -1;
}

/* ---- uncompressed header (6.2) ---------------------------------------- */

static void color_config(struct vp9_bits *b, struct vp9_hdr *h)
{
    int cs;

    h->bit_depth = 8;
    if (h->profile >= 2)
        h->bit_depth = vb_f(b, 1) ? 12 : 10;
    cs = vb_f(b, 3);
    if (cs != 7)
    {
        vb_f(b, 1);                                 /* color_range */
        if (h->profile == 1 || h->profile == 3)
            vb_f(b, 3);                             /* subsampling x, y, reserved */
    }
    else if (h->profile == 1 || h->profile == 3)
        vb_f(b, 1);
}

int vp9_parse_uncompressed(struct vp9_state *s, const uint8_t *data, unsigned int size, struct vp9_hdr *h,
                           int width, int height)
{
    struct vp9_bits bb = { data, size, 0 }, *b = &bb;
    int i, intra, sb64cols, minlog, maxlog;

    memset(h, 0, sizeof(*h));
    h->bit_depth = 8;
    if (vb_f(b, 2) != 2)
        return -1;
    h->profile = vb_f(b, 1);
    h->profile |= vb_f(b, 1) << 1;
    if (h->profile == 3)
        vb_f(b, 1);
    h->show_existing = vb_f(b, 1);
    if (h->show_existing)
    {
        h->existing_idx = vb_f(b, 3);
        return 0;
    }
    h->key = !vb_f(b, 1);
    h->show_frame = vb_f(b, 1);
    h->error_res = vb_f(b, 1);
    h->width = width;
    h->height = height;
    if (h->key)
    {
        vb_f(b, 24);                                /* sync code */
        color_config(b, h);
        h->width = vb_f(b, 16) + 1;
        h->height = vb_f(b, 16) + 1;
        if (vb_f(b, 1))
            vb_f(b, 32);                            /* render size */
        h->refresh_flags = 0xff;
    }
    else
    {
        h->intra_only = h->show_frame ? 0 : vb_f(b, 1);
        h->reset_ctx = h->error_res ? 0 : vb_f(b, 2);
        if (h->intra_only)
        {
            vb_f(b, 24);
            if (h->profile > 0)
                color_config(b, h);
            h->refresh_flags = vb_f(b, 8);
            h->width = vb_f(b, 16) + 1;
            h->height = vb_f(b, 16) + 1;
            if (vb_f(b, 1))
                vb_f(b, 32);
        }
        else
        {
            int found = 0;

            h->refresh_flags = vb_f(b, 8);
            for (i = 0; i < 3; i++)
            {
                h->ref_idx[i] = vb_f(b, 3);
                h->sign_bias[i] = vb_f(b, 1);
            }
            for (i = 0; i < 3 && !found; i++)
                found = vb_f(b, 1);
            if (!found)
            {
                h->width = vb_f(b, 16) + 1;
                h->height = vb_f(b, 16) + 1;
            }
            if (vb_f(b, 1))
                vb_f(b, 32);
            h->hp = vb_f(b, 1);
            if (vb_f(b, 1))
                h->interp = 4;
            else
            {
                static const int literal_to_filter[4] = { 1, 0, 2, 3 };

                h->interp = literal_to_filter[vb_f(b, 2)];
            }
        }
    }
    /* inter frames keep the depth of the last frame that coded one; intra-only frames in profile 0 are 8-bit */
    if (h->key || (h->intra_only && h->profile > 0))
        s->bit_depth = h->bit_depth;
    else if (h->intra_only)
        h->bit_depth = 8;
    else
        h->bit_depth = s->bit_depth ? s->bit_depth : 8;
    if (!h->error_res)
    {
        h->refresh_ctx = vb_f(b, 1);
        h->parallel = vb_f(b, 1);
    }
    else
        h->parallel = 1;
    h->ctx_idx = vb_f(b, 2);
    intra = h->key || h->intra_only;
    if (intra || h->error_res)
    {
        /* setup_past_independence: loop filter deltas and segmentation features */
        s->lf_ref_deltas[0] = 1;
        s->lf_ref_deltas[1] = 0;
        s->lf_ref_deltas[2] = -1;
        s->lf_ref_deltas[3] = -1;
        s->lf_mode_deltas[0] = s->lf_mode_deltas[1] = 0;
        s->seg_abs = 0;
        memset(s->seg_feature_enabled, 0, sizeof(s->seg_feature_enabled));
        memset(s->seg_feature_data, 0, sizeof(s->seg_feature_data));
    }

    /* loop filter */
    h->lf_level = vb_f(b, 6);
    h->lf_sharpness = vb_f(b, 3);
    s->lf_delta_enabled = vb_f(b, 1);
    if (s->lf_delta_enabled && vb_f(b, 1))
    {
        for (i = 0; i < 4; i++)
            if (vb_f(b, 1))
                s->lf_ref_deltas[i] = (int8_t)vb_s(b, 6);
        for (i = 0; i < 2; i++)
            if (vb_f(b, 1))
                s->lf_mode_deltas[i] = (int8_t)vb_s(b, 6);
    }

    /* quantization */
    h->base_q = vb_f(b, 8);
    h->dq_y_dc = vb_f(b, 1) ? vb_s(b, 4) : 0;
    h->dq_uv_dc = vb_f(b, 1) ? vb_s(b, 4) : 0;
    h->dq_uv_ac = vb_f(b, 1) ? vb_s(b, 4) : 0;
    h->lossless = !h->base_q && !h->dq_y_dc && !h->dq_uv_dc && !h->dq_uv_ac;

    /* segmentation */
    memset(h->seg_tree_probs, 255, sizeof(h->seg_tree_probs));
    memset(h->seg_pred_probs, 255, sizeof(h->seg_pred_probs));
    h->seg_enabled = vb_f(b, 1);
    if (h->seg_enabled)
    {
        h->seg_update_map = vb_f(b, 1);
        if (h->seg_update_map)
        {
            for (i = 0; i < 7; i++)
                h->seg_tree_probs[i] = vb_f(b, 1) ? vb_f(b, 8) : 255;
            h->seg_temporal = vb_f(b, 1);
            for (i = 0; i < 3; i++)
                h->seg_pred_probs[i] = (h->seg_temporal && vb_f(b, 1)) ? vb_f(b, 8) : 255;
        }
        h->seg_update_data = vb_f(b, 1);
        if (h->seg_update_data)
        {
            static const int fbits[4] = { 8, 6, 2, 0 }, fsigned[4] = { 1, 1, 0, 0 };
            int j;

            s->seg_abs = vb_f(b, 1);
            for (i = 0; i < 8; i++)
                for (j = 0; j < 4; j++)
                {
                    int v = 0;

                    s->seg_feature_enabled[i][j] = vb_f(b, 1);
                    if (s->seg_feature_enabled[i][j])
                    {
                        v = fbits[j] ? vb_f(b, fbits[j]) : 0;
                        if (fsigned[j] && vb_f(b, 1))
                            v = -v;
                    }
                    s->seg_feature_data[i][j] = (int16_t)v;
                }
        }
    }

    /* tiles */
    sb64cols = (((h->width + 7) >> 3) + 7) >> 3;
    minlog = 0;
    while ((64 << minlog) < sb64cols)
        minlog++;
    maxlog = 1;
    while ((sb64cols >> maxlog) >= 4)
        maxlog++;
    maxlog--;
    h->log2_tile_cols = minlog;
    while (h->log2_tile_cols < maxlog && vb_f(b, 1))
        h->log2_tile_cols++;
    h->log2_tile_rows = vb_f(b, 1);
    if (h->log2_tile_rows)
        h->log2_tile_rows += vb_f(b, 1);

    h->ch_bytes = vb_f(b, 16);
    h->uh_bytes = (b->pos + 7) >> 3;
    if ((unsigned int)(h->uh_bytes + h->ch_bytes) > size)
        return -1;
    return 0;
}

/* ---- compressed header (6.3) ------------------------------------------ */

int vp9_parse_compressed(struct vp9_state *s, const uint8_t *data, unsigned int size, struct vp9_hdr *h)
{
    struct vp9_bool rr, *r = &rr;
    struct vp9_fc *p = &s->probs;
    int intra = h->key || h->intra_only, i, j, k, l, m, tx, maxtx;
    static const int tx_mode_to_biggest[5] = { 0, 1, 2, 3, 3 };

    /* reset / load the frame context (libvpx vp9_setup_past_independence + load) */
    if (intra || h->error_res)
    {
        struct vp9_fc def;

        vp9_default_fc(&def);
        if (h->key || h->error_res || h->reset_ctx == 3)
            for (i = 0; i < 4; i++)
                s->ctx[i] = def;
        else if (h->reset_ctx == 2)
            s->ctx[h->ctx_idx] = def;
        h->ctx_idx = 0;
    }
    s->ctx_used = h->ctx_idx;
    *p = s->ctx[h->ctx_idx];

    if (vbool_init(r, data, size))
        return -1;

    /* tx mode */
    if (h->lossless)
        h->tx_mode = 0;
    else
    {
        h->tx_mode = vbool_lit(r, 2);
        if (h->tx_mode == 3)
            h->tx_mode += vbool_lit(r, 1);
    }
    if (h->tx_mode == 4)
    {
        for (i = 0; i < 2; i++)
            diff_update(r, &p->tx8[i][0]);
        for (i = 0; i < 2; i++)
            for (j = 0; j < 2; j++)
                diff_update(r, &p->tx16[i][j]);
        for (i = 0; i < 2; i++)
            for (j = 0; j < 3; j++)
                diff_update(r, &p->tx32[i][j]);
    }

    /* coefficients */
    maxtx = tx_mode_to_biggest[h->tx_mode];
    for (tx = 0; tx <= maxtx; tx++)
        if (vbool_lit(r, 1))
            for (i = 0; i < 2; i++)
                for (j = 0; j < 2; j++)
                    for (k = 0; k < 6; k++)
                        for (l = 0; l < (k ? 6 : 3); l++)
                            for (m = 0; m < 3; m++)
                                diff_update(r, &p->coef[tx][i][j][k][l][m]);

    for (i = 0; i < 3; i++)
        diff_update(r, &p->skip[i]);

    h->reference_mode = 0;
    if (!intra)
    {
        int compound_allowed = 0;

        for (i = 0; i < 7; i++)
            for (j = 0; j < 3; j++)
                diff_update(r, &p->inter_mode[i][j]);
        if (h->interp == 4)
            for (i = 0; i < 4; i++)
                for (j = 0; j < 2; j++)
                    diff_update(r, &p->interp[i][j]);
        for (i = 0; i < 4; i++)
            diff_update(r, &p->intra_inter[i]);

        /* reference mode (sign biases: [0] LAST, [1] GOLDEN, [2] ALTREF) */
        for (i = 1; i < 3; i++)
            if (h->sign_bias[i] != h->sign_bias[0])
                compound_allowed = 1;
        if (compound_allowed && vbool_lit(r, 1))
            h->reference_mode = vbool_lit(r, 1) ? 2 : 1;
        if (compound_allowed)
        {
            if (h->sign_bias[0] == h->sign_bias[1])
            {
                h->comp_fixed_ref = 3;
                h->comp_var_ref[0] = 1;
                h->comp_var_ref[1] = 2;
            }
            else if (h->sign_bias[0] == h->sign_bias[2])
            {
                h->comp_fixed_ref = 2;
                h->comp_var_ref[0] = 1;
                h->comp_var_ref[1] = 3;
            }
            else
            {
                h->comp_fixed_ref = 1;
                h->comp_var_ref[0] = 2;
                h->comp_var_ref[1] = 3;
            }
        }
        if (h->reference_mode == 2)
            for (i = 0; i < 5; i++)
                diff_update(r, &p->comp_inter[i]);
        if (h->reference_mode != 1)
            for (i = 0; i < 5; i++)
            {
                diff_update(r, &p->single_ref[i][0]);
                diff_update(r, &p->single_ref[i][1]);
            }
        if (h->reference_mode != 0)
            for (i = 0; i < 5; i++)
                diff_update(r, &p->comp_ref[i]);

        for (i = 0; i < 4; i++)
            for (j = 0; j < 9; j++)
                diff_update(r, &p->y_mode[i][j]);
        for (i = 0; i < 16; i++)
            for (j = 0; j < 3; j++)
                diff_update(r, &p->partition[i][j]);

        /* motion vectors */
        for (j = 0; j < 3; j++)
            mv_update(r, &p->mv_joints[j]);
        for (i = 0; i < 2; i++)
        {
            mv_update(r, &p->mv_sign[i]);
            for (j = 0; j < 10; j++)
                mv_update(r, &p->mv_classes[i][j]);
            mv_update(r, &p->mv_class0[i][0]);
            for (j = 0; j < 10; j++)
                mv_update(r, &p->mv_bits[i][j]);
        }
        for (i = 0; i < 2; i++)
        {
            for (j = 0; j < 2; j++)
                for (k = 0; k < 3; k++)
                    mv_update(r, &p->mv_class0_fp[i][j][k]);
            for (k = 0; k < 3; k++)
                mv_update(r, &p->mv_fp[i][k]);
        }
        if (h->hp)
            for (i = 0; i < 2; i++)
            {
                mv_update(r, &p->mv_class0_hp[i]);
                mv_update(r, &p->mv_hp[i]);
            }
    }
    return 0;
}

/* ---- NVDEC probability table (nvdec_vp9EntropyProbs_t) ---------------- */

/* split a 9-probability intra-mode row: 8 into one table, the 9th into the "B" table */
static void put9(uint8_t *t8, uint8_t *t1, const uint8_t *p)
{
    memcpy(t8, p, 8);
    *t1 = p[8];
}

void vp9_fill_nvdec_probs(const struct vp9_state *s, const struct vp9_hdr *h, uint8_t *t)
{
    const struct vp9_fc *p = &s->probs;
    int i, j, k, l, tx;

    memset(t, 0, VP9_NVDEC_PROBTAB_SIZE);
    for (i = 0; i < 10; i++)
        for (j = 0; j < 10; j++)
            put9(t + 0x000 + (i * 10 + j) * 8, t + 0x320 + i * 10 + j, &vp9_kf_y_mode_prob[i][j][0]);
    if (h->seg_enabled)                         /* 0 when segmentation is off, as NVIDIA's driver */
    {
        memcpy(t + 0x387, h->seg_tree_probs, 7);
        memcpy(t + 0x38e, h->seg_pred_probs, 3);
    }
    for (i = 0; i < 10; i++)
        put9(t + 0x3a0 + i * 8, t + 0x3f0 + i, &vp9_kf_uv_mode_prob[i][0]);
    /* adaptive part at 0x400 */
    for (i = 0; i < 7; i++)
        memcpy(t + 0x400 + i * 4, p->inter_mode[i], 3);
    memcpy(t + 0x41c, p->intra_inter, 4);
    for (i = 0; i < 10; i++)
        put9(t + 0x420 + i * 8, t + 0x520 + i, p->uv_mode[i]);
    t[0x470] = p->tx8[0][0];
    t[0x471] = p->tx8[1][0];
    memcpy(t + 0x472, p->tx16, 4);
    memcpy(t + 0x476, p->tx32, 6);
    for (i = 0; i < 4; i++)
        put9(t + 0x480 + i * 8, t + 0x47c + i, p->y_mode[i]);
    for (i = 0; i < 16; i++)
    {
        memcpy(t + 0x4a0 + i * 4, &vp9_kf_partition_prob[i][0], 3);
        memcpy(t + 0x4e0 + i * 4, p->partition[i], 3);
    }
    memcpy(t + 0x52a, p->interp, 8);
    memcpy(t + 0x532, p->comp_inter, 5);
    memcpy(t + 0x537, p->skip, 3);
    memcpy(t + 0x53b, p->mv_joints, 3);
    memcpy(t + 0x53e, p->mv_sign, 2);
    t[0x540] = p->mv_class0[0][0];
    t[0x541] = p->mv_class0[1][0];
    memcpy(t + 0x542, p->mv_fp, 6);
    memcpy(t + 0x548, p->mv_class0_hp, 2);
    memcpy(t + 0x54a, p->mv_hp, 2);
    memcpy(t + 0x54c, p->mv_classes, 20);
    memcpy(t + 0x560, p->mv_class0_fp, 12);
    memcpy(t + 0x56c, p->mv_bits, 20);
    memcpy(t + 0x580, p->single_ref, 10);
    memcpy(t + 0x58a, p->comp_ref, 5);
    for (tx = 0; tx < 4; tx++)
        for (i = 0; i < 2; i++)
            for (j = 0; j < 2; j++)
                for (k = 0; k < 6; k++)
                    for (l = 0; l < 6; l++)
                        memcpy(t + 0x5a0 + tx * 0x240 + ((((i * 2 + j) * 6 + k) * 6 + l) * 4),
                               p->coef[tx][i][j][k][l], 3);
}

/* ---- adaptation (8.4, libvpx vp9_adapt_*) ----------------------------- */

static uint8_t get_prob(unsigned int num, unsigned int den)
{
    int p = (int)(((uint64_t)num * 256 + (den >> 1)) / den);

    return (uint8_t)(p > 255 ? 255 : p < 1 ? 1 : p);
}

static uint8_t weighted_prob(int p1, int p2, int factor)
{
    return (uint8_t)((p1 * (256 - factor) + p2 * factor + 128) >> 8);
}

static uint8_t merge_probs(uint8_t pre, unsigned int c0, unsigned int c1, unsigned int sat, unsigned int maxf)
{
    unsigned int den = c0 + c1, count = den < sat ? den : sat;
    uint8_t prob = den ? get_prob(c0, den) : 128;

    return weighted_prob(pre, prob, maxf * count / sat);
}

static uint8_t mode_mv_merge(uint8_t pre, unsigned int c0, unsigned int c1)
{
    static const int factor[21] = { 0, 6, 12, 19, 25, 32, 38, 44, 51, 57, 64,
                                    70, 76, 83, 89, 96, 102, 108, 115, 121, 128 };
    unsigned int den = c0 + c1;

    if (!den)
        return pre;
    return weighted_prob(pre, get_prob(c0, den), factor[den < 20 ? den : 20]);
}

/* libvpx vpx_tree_merge_probs */
static unsigned int tree_merge(int i, const int8_t *tree, const uint8_t *pre, const unsigned int *counts, uint8_t *probs)
{
    int l = tree[i], rr = tree[i + 1];
    unsigned int lc = (l <= 0) ? counts[-l] : tree_merge(l, tree, pre, counts, probs);
    unsigned int rc = (rr <= 0) ? counts[-rr] : tree_merge(rr, tree, pre, counts, probs);

    probs[i >> 1] = mode_mv_merge(pre[i >> 1], lc, rc);
    return lc + rc;
}

/* trees (libvpx enum values: intra DC 0 V 1 H 2 D45 3 D135 4 D117 5 D153 6 D207 7 D63 8 TM 9) */
static const int8_t intra_mode_tree[18] = { -0, 2, -9, 4, -1, 6, 8, 12, -2, 10, -4, -5, -3, 14, -8, 16, -6, -7 };
static const int8_t inter_mode_tree[6] = { -2, 2, -0, 4, -1, -3 };     /* INTER_OFFSET: NEAREST 0 NEAR 1 ZERO 2 NEW 3 */
static const int8_t partition_tree[6] = { -0, 2, -1, 4, -2, -3 };
static const int8_t interp_tree[4] = { -0, 2, -1, -2 };
static const int8_t mv_joint_tree[6] = { -0, 2, -1, 4, -2, -3 };
static const int8_t mv_class_tree[20] = { -0, 2, -1, 4, 6, 8, -2, -3, 10, 12, -4, -5, -6, 14, 16, 18, -7, -8, -9, -10 };
static const int8_t mv_class0_tree[2] = { -0, -1 };
static const int8_t mv_fp_tree[6] = { -0, 2, -1, 4, -2, -3 };

/* nvdec_vp9EntropyCounts_t offsets, in 32-bit words */
#define CNT_INTER_MODE      (0x000 / 4)     /* [7][3][2] */
#define CNT_Y_MODE          (0x0a8 / 4)     /* [4][10] */
#define CNT_UV_MODE         (0x148 / 4)     /* [10][10] */
#define CNT_PARTITION       (0x2d8 / 4)     /* [16][4] */
#define CNT_INTERP          (0x3d8 / 4)     /* [4][3] */
#define CNT_INTRA_INTER     (0x408 / 4)     /* [4][2] */
#define CNT_COMP_INTER      (0x428 / 4)     /* [5][2] */
#define CNT_SINGLE_REF      (0x450 / 4)     /* [5][2][2] */
#define CNT_COMP_REF        (0x4a0 / 4)     /* [5][2] */
#define CNT_TX32            (0x4c8 / 4)     /* [2][4] */
#define CNT_TX16            (0x4e8 / 4)     /* [2][3] */
#define CNT_TX8             (0x500 / 4)     /* [2][2] */
#define CNT_SKIP            (0x510 / 4)     /* [3][2] */
#define CNT_MV_JOINTS       (0x528 / 4)     /* [4] */
#define CNT_MV_SIGN         (0x538 / 4)     /* [2][2] */
#define CNT_MV_CLASSES      (0x548 / 4)     /* [2][11] */
#define CNT_MV_CLASS0       (0x5a0 / 4)     /* [2][2] */
#define CNT_MV_BITS         (0x5b0 / 4)     /* [2][10][2] */
#define CNT_MV_CLASS0_FP    (0x650 / 4)     /* [2][2][4] */
#define CNT_MV_FP           (0x690 / 4)     /* [2][4] */
#define CNT_MV_CLASS0_HP    (0x6b0 / 4)     /* [2][2] */
#define CNT_MV_HP           (0x6c0 / 4)     /* [2][2] */
#define CNT_COEF(tx)        ((0x6d0 + (tx) * 0x900) / 4)     /* [2][2][6][6][4] */
#define CNT_EOB             (0x2ad0 / 4)    /* [4][2][2][6][6] */

int vp9_needs_counts(const struct vp9_hdr *h)
{
    return h->refresh_ctx && !h->error_res && !h->parallel;
}

static void adapt(struct vp9_state *s, const struct vp9_hdr *h, const uint32_t *c)
{
    const struct vp9_fc *pre = &s->ctx[s->ctx_used];
    struct vp9_fc *fc = &s->probs;
    int intra = h->key || h->intra_only, tx, i, j, k, l;
    unsigned int sat = 24, maxf = 112;

    if (!intra && s->have_last && s->last_key)
        maxf = 128;
    for (tx = 0; tx < 4; tx++)
        for (i = 0; i < 2; i++)
            for (j = 0; j < 2; j++)
                for (k = 0; k < 6; k++)
                    for (l = 0; l < (k ? 6 : 3); l++)
                    {
                        const uint32_t *cc = c + CNT_COEF(tx) + (((i * 2 + j) * 6 + k) * 6 + l) * 4;
                        unsigned int n0 = cc[0], n1 = cc[1], n2 = cc[2], neob = cc[3];
                        unsigned int eobb = c[CNT_EOB + (((tx * 2 + i) * 2 + j) * 6 + k) * 6 + l];
                        const uint8_t *pp = pre->coef[tx][i][j][k][l];
                        uint8_t *fp = fc->coef[tx][i][j][k][l];

                        fp[0] = merge_probs(pp[0], neob, eobb - neob, sat, maxf);
                        fp[1] = merge_probs(pp[1], n0, n1 + n2, sat, maxf);
                        fp[2] = merge_probs(pp[2], n1, n2, sat, maxf);
                    }
    if (intra)
        return;

    for (i = 0; i < 4; i++)
        fc->intra_inter[i] = mode_mv_merge(pre->intra_inter[i], c[CNT_INTRA_INTER + i * 2], c[CNT_INTRA_INTER + i * 2 + 1]);
    for (i = 0; i < 5; i++)
        fc->comp_inter[i] = mode_mv_merge(pre->comp_inter[i], c[CNT_COMP_INTER + i * 2], c[CNT_COMP_INTER + i * 2 + 1]);
    for (i = 0; i < 5; i++)
        fc->comp_ref[i] = mode_mv_merge(pre->comp_ref[i], c[CNT_COMP_REF + i * 2], c[CNT_COMP_REF + i * 2 + 1]);
    for (i = 0; i < 5; i++)
        for (j = 0; j < 2; j++)
            fc->single_ref[i][j] = mode_mv_merge(pre->single_ref[i][j], c[CNT_SINGLE_REF + (i * 2 + j) * 2],
                                                 c[CNT_SINGLE_REF + (i * 2 + j) * 2 + 1]);
    for (i = 0; i < 7; i++)
    {
        /* NVDEC [3][2] -> libvpx INTER_OFFSET order { NEAREST, NEAR, ZERO, NEW } */
        const uint32_t *ci = c + CNT_INTER_MODE + i * 6;
        unsigned int cnt[4] = { ci[2], ci[4], ci[0], ci[5] };

        tree_merge(0, inter_mode_tree, pre->inter_mode[i], cnt, fc->inter_mode[i]);
    }
    for (i = 0; i < 4; i++)
        tree_merge(0, intra_mode_tree, pre->y_mode[i], (const unsigned int *)(c + CNT_Y_MODE + i * 10), fc->y_mode[i]);
    for (i = 0; i < 10; i++)
        tree_merge(0, intra_mode_tree, pre->uv_mode[i], (const unsigned int *)(c + CNT_UV_MODE + i * 10), fc->uv_mode[i]);
    for (i = 0; i < 16; i++)
        tree_merge(0, partition_tree, pre->partition[i], (const unsigned int *)(c + CNT_PARTITION + i * 4), fc->partition[i]);
    if (h->interp == 4)
        for (i = 0; i < 4; i++)
            tree_merge(0, interp_tree, pre->interp[i], (const unsigned int *)(c + CNT_INTERP + i * 3), fc->interp[i]);
    if (h->tx_mode == 4)
        for (i = 0; i < 2; i++)
        {
            const uint32_t *t8 = c + CNT_TX8 + i * 2, *t16 = c + CNT_TX16 + i * 3, *t32 = c + CNT_TX32 + i * 4;

            fc->tx8[i][0] = mode_mv_merge(pre->tx8[i][0], t8[0], t8[1]);
            fc->tx16[i][0] = mode_mv_merge(pre->tx16[i][0], t16[0], t16[1] + t16[2]);
            fc->tx16[i][1] = mode_mv_merge(pre->tx16[i][1], t16[1], t16[2]);
            fc->tx32[i][0] = mode_mv_merge(pre->tx32[i][0], t32[0], t32[1] + t32[2] + t32[3]);
            fc->tx32[i][1] = mode_mv_merge(pre->tx32[i][1], t32[1], t32[2] + t32[3]);
            fc->tx32[i][2] = mode_mv_merge(pre->tx32[i][2], t32[2], t32[3]);
        }
    for (i = 0; i < 3; i++)
        fc->skip[i] = mode_mv_merge(pre->skip[i], c[CNT_SKIP + i * 2], c[CNT_SKIP + i * 2 + 1]);

    /* motion vectors */
    tree_merge(0, mv_joint_tree, pre->mv_joints, (const unsigned int *)(c + CNT_MV_JOINTS), fc->mv_joints);
    for (i = 0; i < 2; i++)
    {
        fc->mv_sign[i] = mode_mv_merge(pre->mv_sign[i], c[CNT_MV_SIGN + i * 2], c[CNT_MV_SIGN + i * 2 + 1]);
        tree_merge(0, mv_class_tree, pre->mv_classes[i], (const unsigned int *)(c + CNT_MV_CLASSES + i * 11), fc->mv_classes[i]);
        tree_merge(0, mv_class0_tree, pre->mv_class0[i], (const unsigned int *)(c + CNT_MV_CLASS0 + i * 2), fc->mv_class0[i]);
        for (j = 0; j < 10; j++)
            fc->mv_bits[i][j] = mode_mv_merge(pre->mv_bits[i][j], c[CNT_MV_BITS + (i * 10 + j) * 2],
                                              c[CNT_MV_BITS + (i * 10 + j) * 2 + 1]);
        for (j = 0; j < 2; j++)
            tree_merge(0, mv_fp_tree, pre->mv_class0_fp[i][j], (const unsigned int *)(c + CNT_MV_CLASS0_FP + (i * 2 + j) * 4),
                       fc->mv_class0_fp[i][j]);
        tree_merge(0, mv_fp_tree, pre->mv_fp[i], (const unsigned int *)(c + CNT_MV_FP + i * 4), fc->mv_fp[i]);
        if (h->hp)
        {
            fc->mv_class0_hp[i] = mode_mv_merge(pre->mv_class0_hp[i], c[CNT_MV_CLASS0_HP + i * 2], c[CNT_MV_CLASS0_HP + i * 2 + 1]);
            fc->mv_hp[i] = mode_mv_merge(pre->mv_hp[i], c[CNT_MV_HP + i * 2], c[CNT_MV_HP + i * 2 + 1]);
        }
    }
}

void vp9_finish_frame(struct vp9_state *s, const struct vp9_hdr *h, const uint32_t *counts)
{
    if (h->show_existing)
        return;
    if (counts && vp9_needs_counts(h))
        adapt(s, h, counts);
    if (h->refresh_ctx)
        s->ctx[s->ctx_used] = s->probs;
    s->last_width = h->width;
    s->last_height = h->height;
    s->last_key = h->key;
    s->last_show = h->show_frame;
    s->last_intra_only = h->intra_only;
    s->have_last = 1;
}
