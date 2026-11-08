/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    HEVC block: VA-API picture to command list and phase 2 values.

    The block parses slice data itself, but gets every header value from
    the command list: register writes it executes in order, with CABAC
    context loads, slice messages and markers at the ends of slice
    segments and entry-point regions (tiles, or CTB rows with WPP).
*/

#include <string.h>

#include "vcgfx_hevc.h"

/* initValue of every hardware context slot, per initType (H.265 9-5..9-37
 * in the block's order; 154 where the standard has no context) */
static const UBYTE hevc_cabac_init[3][HEVC_CABAC_STATES] =
{
    {
        153, 200, 139, 141, 157, 154, 154, 154, 154, 154, 184, 154, 154, 154,
        184,  63, 154, 154, 154, 154, 154, 154, 154, 154, 154, 154, 154, 154,
        154, 153, 138, 138, 111, 141,  94, 138, 182, 154, 154, 154, 140,  92,
        137, 138, 140, 152, 138, 139, 153,  74, 149,  92, 139, 107, 122, 152,
        140, 179, 166, 182, 140, 227, 122, 197, 110, 110, 124, 125, 140, 153,
        125, 127, 140, 109, 111, 143, 127, 111,  79, 108, 123,  63, 110, 110,
        124, 125, 140, 153, 125, 127, 140, 109, 111, 143, 127, 111,  79, 108,
        123,  63,  91, 171, 134, 141, 138, 153, 136, 167, 152, 152, 139, 139,
        111, 111, 125, 110, 110,  94, 124, 108, 124, 107, 125, 141, 179, 153,
        125, 107, 125, 141, 179, 153, 125, 107, 125, 141, 179, 153, 125, 140,
        139, 182, 182, 152, 136, 152, 136, 153, 136, 139, 111, 136, 139, 111,
    },
    {
        153, 185, 107, 139, 126, 197, 185, 201, 154, 149, 154, 139, 154, 154,
        154, 152, 110, 122,  95,  79,  63,  31,  31, 153, 153, 168, 140, 198,
         79, 124, 138,  94, 153, 111, 149, 107, 167, 154, 154, 154, 154, 196,
        196, 167, 154, 152, 167, 182, 182, 134, 149, 136, 153, 121, 136, 137,
        169, 194, 166, 167, 154, 167, 137, 182, 125, 110,  94, 110,  95,  79,
        125, 111, 110,  78, 110, 111, 111,  95,  94, 108, 123, 108, 125, 110,
         94, 110,  95,  79, 125, 111, 110,  78, 110, 111, 111,  95,  94, 108,
        123, 108, 121, 140,  61, 154, 107, 167,  91, 122, 107, 167, 139, 139,
        155, 154, 139, 153, 139, 123, 123,  63, 153, 166, 183, 140, 136, 153,
        154, 166, 183, 140, 136, 153, 154, 166, 183, 140, 136, 153, 154, 170,
        153, 123, 123, 107, 121, 107, 121, 167, 151, 183, 140, 151, 183, 140,
    },
    {
        153, 160, 107, 139, 126, 197, 185, 201, 154, 134, 154, 139, 154, 154,
        183, 152, 154, 137,  95,  79,  63,  31,  31, 153, 153, 168, 169, 198,
         79, 224, 167, 122, 153, 111, 149,  92, 167, 154, 154, 154, 154, 196,
        167, 167, 154, 152, 167, 182, 182, 134, 149, 136, 153, 121, 136, 122,
        169, 208, 166, 167, 154, 152, 167, 182, 125, 110, 124, 110,  95,  94,
        125, 111, 111,  79, 125, 126, 111, 111,  79, 108, 123,  93, 125, 110,
        124, 110,  95,  94, 125, 111, 111,  79, 125, 126, 111, 111,  79, 108,
        123,  93, 121, 140,  61, 154, 107, 167,  91, 107, 107, 167, 139, 139,
        170, 154, 139, 153, 139, 123, 123,  63, 124, 166, 183, 140, 136, 153,
        154, 166, 183, 140, 136, 153, 154, 166, 183, 140, 136, 153, 154, 170,
        153, 138, 138, 122, 121, 122, 121, 167, 151, 183, 140, 151, 183, 140,
    },
};

struct hevc_dims {
    ULONG ctblog2, ctbsize;
    ULONG w, h;                 /* picture in CTBs */
    ULONG picw, pich;           /* in luma samples */
    ULONG qpbdoffset;
    ULONG ncols, nrows;         /* tiles */
    ULONG colbd[21], rowbd[23];
    ULONG *rs2ts, *ts2rs;
    BOOL wpp;
};

struct hevc_state {
    struct hevc_cl *cl;
    const struct hevc_dims *d;
    ULONG tile;                 /* last entry point: tile, or WPP row */
    ULONG ex, ey;
    ULONG qp;
    ULONG bits;
};

static void emit(struct hevc_state *s, ULONG addr, ULONG value)
{
    struct hevc_cl *cl = s->cl;

    if (cl->n < cl->max)
    {
        cl->cmds[cl->n].addr = addr;
        cl->cmds[cl->n].value = value;
        cl->n++;
    }
    else
        cl->overflow = TRUE;
}

ULONG hevc_colmv_stride(ULONG width)
{
    return (width + 63) & ~63;
}

ULONG hevc_colmv_size(ULONG width, ULONG height)
{
    return hevc_colmv_stride(width) * ((height + 63) & ~63) / 16;
}

static ULONG ctblog2(const VAPictureParameterBufferHEVC *pp)
{
    return pp->log2_min_luma_coding_block_size_minus3 + 3
           + pp->log2_diff_max_min_luma_coding_block_size;
}

BOOL hevc_ctb_ok(const VAPictureParameterBufferHEVC *pp)
{
    return (ctblog2(pp) >= 4) && (ctblog2(pp) <= 6);
}

ULONG hevc_tiles_size(const VAPictureParameterBufferHEVC *pp)
{
    ULONG log2 = ctblog2(pp), size = 1 << log2;
    ULONG w = (pp->pic_width_in_luma_samples + size - 1) >> log2;
    ULONG h = (pp->pic_height_in_luma_samples + size - 1) >> log2;

    return 2 * w * h * sizeof(ULONG);
}

ULONG hevc_cl_estimate(const VAPictureParameterBufferHEVC *pp, ULONG nslices)
{
    ULONG log2 = ctblog2(pp), size = 1 << log2;
    ULONG h = (pp->pic_height_in_luma_samples + size - 1) >> log2;
    ULONG tiles = (pp->num_tile_columns_minus1 + 1) * (pp->num_tile_rows_minus1 + 1);

    return HEVC_SCALING_WORDS + 64 + nslices * (HEVC_MAX_MSGS + 80)
           + (tiles + h) * 16;
}

/* Tile boundaries and the raster <-> tile scan tables (H.265 6.5.1).
 * FALSE for a layout that does not fit the picture. */
static BOOL hevc_scan(const VAPictureParameterBufferHEVC *pp, struct hevc_dims *d, ULONG *map)
{
    ULONG i, tx, ty, x, y, ts;

    d->ncols = 1;
    d->nrows = 1;
    if (pp->pic_fields.bits.tiles_enabled_flag)
    {
        d->ncols = pp->num_tile_columns_minus1 + 1;
        d->nrows = pp->num_tile_rows_minus1 + 1;
    }
    if ((d->ncols > 20) || (d->nrows > 22) || (d->ncols > d->w) || (d->nrows > d->h))
        return FALSE;

    /* Every tile at least one CTB, the last one included */
    d->colbd[0] = 0;
    for (i = 0; i < d->ncols - 1; i++)
    {
        d->colbd[i + 1] = d->colbd[i] + pp->column_width_minus1[i] + 1;
        if (d->colbd[i + 1] >= d->w)
            return FALSE;
    }
    d->colbd[d->ncols] = d->w;

    d->rowbd[0] = 0;
    for (i = 0; i < d->nrows - 1; i++)
    {
        d->rowbd[i + 1] = d->rowbd[i] + pp->row_height_minus1[i] + 1;
        if (d->rowbd[i + 1] >= d->h)
            return FALSE;
    }
    d->rowbd[d->nrows] = d->h;

    d->rs2ts = map;
    d->ts2rs = map + d->w * d->h;

    ts = 0;
    for (ty = 0; ty < d->nrows; ty++)
        for (tx = 0; tx < d->ncols; tx++)
            for (y = d->rowbd[ty]; y < d->rowbd[ty + 1]; y++)
                for (x = d->colbd[tx]; x < d->colbd[tx + 1]; x++)
                {
                    d->rs2ts[y * d->w + x] = ts;
                    d->ts2rs[ts] = y * d->w + x;
                    ts++;
                }
    return TRUE;
}

/* The tile (raster order of tiles) a CTB lies in */
static ULONG tile_of(const struct hevc_dims *d, ULONG x, ULONG y)
{
    ULONG tx = 0, ty = 0;

    while (x >= d->colbd[tx + 1])
        tx++;
    while (y >= d->rowbd[ty + 1])
        ty++;
    return ty * d->ncols + tx;
}

/* The size of the last CTB column and row of a region ending at (x, y) */
static ULONG last_bits(const struct hevc_dims *d, ULONG x, ULONG y)
{
    ULONG lw = d->ctbsize, lh = d->ctbsize;

    if ((x == d->w - 1) && (d->picw & (d->ctbsize - 1)))
        lw = d->picw & (d->ctbsize - 1);
    if ((y == d->h - 1) && (d->pich & (d->ctbsize - 1)))
        lh = d->pich & (d->ctbsize - 1);
    return (lw << 17) | (lh << 24);
}

static BOOL slice_is(const VASliceParameterBufferHEVC *sp, ULONG type)
{
    return sp->LongSliceFlags.fields.slice_type == type;
}

#define HEVC_SLICE_B    0
#define HEVC_SLICE_P    1
#define HEVC_SLICE_I    2

static ULONG nref(const VASliceParameterBufferHEVC *sp, ULONG list)
{
    if (slice_is(sp, HEVC_SLICE_I) || (list == 1 && !slice_is(sp, HEVC_SLICE_B)))
        return 0;
    return (list ? sp->num_ref_idx_l1_active_minus1 : sp->num_ref_idx_l0_active_minus1) + 1;
}

static ULONG max_merge(const VASliceParameterBufferHEVC *sp)
{
    return slice_is(sp, HEVC_SLICE_I) ? 0 : 5 - sp->five_minus_max_num_merge_cand;
}

static LONG slice_qp(const VAPictureParameterBufferHEVC *pp, const VASliceParameterBufferHEVC *sp)
{
    return 26 + pp->init_qp_minus26 + sp->slice_qp_delta;
}

/* collocated_from_l0 as the hardware wants it */
static BOOL col_from_l0(const VASliceParameterBufferHEVC *sp)
{
    return !sp->LongSliceFlags.fields.slice_temporal_mvp_enabled_flag
           || !slice_is(sp, HEVC_SLICE_B)
           || sp->LongSliceFlags.fields.collocated_from_l0_flag;
}

static ULONG slice_bits(const VASliceParameterBufferHEVC *sp)
{
    ULONG b = max_merge(sp) | (nref(sp, 0) << 4) | (nref(sp, 1) << 8)
              | (sp->LongSliceFlags.fields.slice_type << 12)
              | (sp->LongSliceFlags.fields.slice_sao_luma_flag << 14)
              | (sp->LongSliceFlags.fields.slice_sao_chroma_flag << 15);

    if (slice_is(sp, HEVC_SLICE_B))
        b |= sp->LongSliceFlags.fields.mvd_l1_zero_flag << 16;
    return b;
}

static ULONG reg_sps0(const VAPictureParameterBufferHEVC *pp)
{
    ULONG mincb = pp->log2_min_luma_coding_block_size_minus3 + 3;
    ULONG mintb = pp->log2_min_transform_block_size_minus2 + 2;

    return mincb | (ctblog2(pp) << 4) | (mintb << 8)
           | ((mintb + pp->log2_diff_max_min_transform_block_size) << 12)
           | ((pp->bit_depth_luma_minus8 + 8) << 16)
           | ((pp->bit_depth_chroma_minus8 + 8) << 20)
           | (pp->max_transform_hierarchy_depth_intra << 24)
           | (pp->max_transform_hierarchy_depth_inter << 28);
}

static ULONG reg_sps1(const VAPictureParameterBufferHEVC *pp)
{
    ULONG minpcm = pp->log2_min_pcm_luma_coding_block_size_minus3 + 3;
    ULONG chroma = pp->pic_fields.bits.separate_colour_plane_flag
                   ? 0 : pp->pic_fields.bits.chroma_format_idc;

    return (pp->pcm_sample_bit_depth_luma_minus1 + 1)
           | ((pp->pcm_sample_bit_depth_chroma_minus1 + 1) << 4)
           | (minpcm << 8)
           | ((minpcm + pp->log2_diff_max_min_pcm_luma_coding_block_size) << 12)
           | (chroma << 16)
           | (pp->pic_fields.bits.amp_enabled_flag << 18)
           | (pp->pic_fields.bits.pcm_enabled_flag << 19)
           | (pp->pic_fields.bits.scaling_list_enabled_flag << 20)
           | (pp->pic_fields.bits.strong_intra_smoothing_enabled_flag << 21);
}

static ULONG reg_pps(const VAPictureParameterBufferHEVC *pp, const VASliceParameterBufferHEVC *sp)
{
    return (ctblog2(pp) - pp->diff_cu_qp_delta_depth)
           | (pp->pic_fields.bits.cu_qp_delta_enabled_flag << 4)
           | (pp->pic_fields.bits.transquant_bypass_enabled_flag << 5)
           | (pp->pic_fields.bits.transform_skip_enabled_flag << 6)
           | (pp->pic_fields.bits.sign_data_hiding_enabled_flag << 7)
           | (((pp->pps_cb_qp_offset + sp->slice_cb_qp_offset) & 0xff) << 8)
           | (((pp->pps_cr_qp_offset + sp->slice_cr_qp_offset) & 0xff) << 16)
           | (pp->pic_fields.bits.constrained_intra_pred_flag << 24);
}

/* The slice messages (16-bit words); returns how many */
static ULONG slice_msgs(const struct hevc_picture *pic, const VASliceParameterBufferHEVC *sp,
                        ULONG *msg, const char **err)
{
    const VAPictureParameterBufferHEVC *pp = pic->pp;
    BOOL wp = (slice_is(sp, HEVC_SLICE_P) && pp->pic_fields.bits.weighted_pred_flag)
              || (slice_is(sp, HEVC_SLICE_B) && pp->pic_fields.bits.weighted_bipred_flag);
    ULONG kind = slice_is(sp, HEVC_SLICE_I) ? 1 : slice_is(sp, HEVC_SLICE_P) ? 2 : 3;
    BOOL nobackward = TRUE;
    ULONG n = 1, list, k;

    for (list = 0; list < 2; list++)
    {
        for (k = 0; k < nref(sp, list); k++)
        {
            ULONG idx = sp->RefPicList[list][k];

            if (idx >= 15)
            {
                *err = "reference index out of range";
                return 0;
            }
            if (pp->ReferenceFrames[idx].pic_order_cnt > pp->CurrPic.pic_order_cnt)
                nobackward = FALSE;
        }
    }

    msg[0] = kind | (nref(sp, 0) << 2) | (nref(sp, 1) << 6)
             | ((kind != 1 && nobackward) << 10) | (max_merge(sp) << 11)
             | (col_from_l0(sp) << 14);

    for (list = 0; list < 2; list++)
    {
        for (k = 0; k < nref(sp, list); k++)
        {
            ULONG idx = sp->RefPicList[list][k];
            const VAPictureHEVC *ref = &pp->ReferenceFrames[idx];
            BOOL lt = (ref->flags & VA_PICTURE_HEVC_LONG_TERM_REFERENCE) ? TRUE : FALSE;

            msg[n++] = idx | (lt << 4) | (wp ? (3 << 5) : 0);
            msg[n++] = ref->pic_order_cnt & 0xffff;

            if (wp)
            {
                ULONG ld = sp->luma_log2_weight_denom;
                ULONG cd = ld + sp->delta_chroma_log2_weight_denom;
                const int8_t *dlw = list ? sp->delta_luma_weight_l1 : sp->delta_luma_weight_l0;
                const int8_t *lo = list ? sp->luma_offset_l1 : sp->luma_offset_l0;
                const int8_t (*dcw)[2] = list ? sp->delta_chroma_weight_l1 : sp->delta_chroma_weight_l0;
                const int8_t (*co)[2] = list ? sp->ChromaOffsetL1 : sp->ChromaOffsetL0;
                ULONG c;

                msg[n++] = ld | ((((1 << ld) + dlw[k]) & 0x1ff) << 3);
                msg[n++] = lo[k] & 0xff;
                for (c = 0; c < 2; c++)
                {
                    msg[n++] = cd | ((((1 << cd) + dcw[k][c]) & 0x1ff) << 3);
                    msg[n++] = co[k][c] & 0xff;
                }
            }
        }
    }

    msg[n++] = (sp->slice_beta_offset_div2 & 15)
               | ((sp->slice_tc_offset_div2 & 15) << 4)
               | (sp->LongSliceFlags.fields.slice_deblocking_filter_disabled_flag << 8)
               | (sp->LongSliceFlags.fields.slice_loop_filter_across_slices_enabled_flag << 9)
               | (pp->pic_fields.bits.loop_filter_across_tiles_enabled_flag << 10);
    msg[n++] = (sp->slice_cb_qp_offset & 31) | ((sp->slice_cr_qp_offset & 31) << 5);

    return n;
}

/* Initial context states (H.265 9.3.2.2), one byte per slot */
static void cabac_words(const VAPictureParameterBufferHEVC *pp, const VASliceParameterBufferHEVC *sp,
                        ULONG *words)
{
    UBYTE state[HEVC_CABAC_STATES + 2];
    ULONG type, i;
    LONG qp = slice_qp(pp, sp);

    if (slice_is(sp, HEVC_SLICE_I))
        type = 0;
    else if (slice_is(sp, HEVC_SLICE_P))
        type = sp->LongSliceFlags.fields.cabac_init_flag ? 2 : 1;
    else
        type = sp->LongSliceFlags.fields.cabac_init_flag ? 1 : 2;

    if (qp < 0)
        qp = 0;
    if (qp > 51)
        qp = 51;

    for (i = 0; i < HEVC_CABAC_STATES; i++)
    {
        LONG init = hevc_cabac_init[type][i];
        LONG m = (init >> 4) * 5 - 45;
        LONG n = ((init & 15) << 3) - 16;
        LONG pre = ((m * qp) >> 4) + n;
        ULONG mps;

        if (pre < 1)
            pre = 1;
        if (pre > 126)
            pre = 126;
        mps = (pre > 63);
        state[i] = ((mps ? pre - 64 : 63 - pre) << 1) | mps;
    }
    state[HEVC_CABAC_STATES] = state[HEVC_CABAC_STATES + 1] = 0;

    for (i = 0; i < (HEVC_CABAC_STATES + 2) / 4; i++)
        words[i] = state[4 * i] | (state[4 * i + 1] << 8)
                   | (state[4 * i + 2] << 16) | ((ULONG)state[4 * i + 3] << 24);
}

/* The effective scaling lists as the block's 4064-byte table */
static void scaling_words(const VAIQMatrixBufferHEVC *iq, ULONG *words)
{
    UBYTE t[HEVC_SCALING_WORDS * 4];
    ULONG m, x, y;

    memset(t, 0, sizeof(t));
    for (m = 0; m < 6; m++)
    {
        memcpy(&t[0x000 + 16 * m], iq->ScalingList4x4[m], 16);
        memcpy(&t[0x060 + 64 * m], iq->ScalingList8x8[m], 64);
        for (y = 0; y < 16; y++)
            for (x = 0; x < 16; x++)
                t[0x1e0 + 256 * m + y * 16 + x] = iq->ScalingList16x16[m][(y / 2) * 8 + x / 2];
        t[0x1e0 + 256 * m] = iq->ScalingListDC16x16[m];
    }
    for (m = 0; m < 2; m++)
    {
        for (y = 0; y < 32; y++)
            for (x = 0; x < 32; x++)
                t[0x7e0 + 1024 * m + y * 32 + x] = iq->ScalingList32x32[m][(y / 4) * 8 + x / 4];
        t[0x7e0 + 1024 * m] = iq->ScalingListDC32x32[m];
    }

    for (x = 0; x < HEVC_SCALING_WORDS; x++)
        words[x] = t[4 * x] | (t[4 * x + 1] << 8) | (t[4 * x + 2] << 16) | ((ULONG)t[4 * x + 3] << 24);
}

static ULONG status_at(ULONG code, ULONG x, ULONG y)
{
    return code | (x << 5) | (y << 18);
}

/*
 * Entry points: the start of a region the block decodes in one go. Tile
 * mode (no WPP): a region is the rest of a tile.
 */
static void tile_entry(struct hevc_state *s, ULONG t, ULONG x, ULONG y,
                       BOOL qpreset, BOOL bte, ULONG qp, ULONG bits)
{
    const struct hevc_dims *d = s->d;
    ULONG tx = t % d->ncols, ty = t / d->ncols;
    ULONG ex = d->colbd[tx + 1] - 1, ey = d->rowbd[ty + 1] - 1;
    ULONG end = ex | (ey << 16);

    emit(s, HEVC_TILESTART, d->colbd[tx] | (d->rowbd[ty] << 16));
    emit(s, HEVC_TILEEND, end);
    if (bte)
        emit(s, HEVC_BEGINTILEEND, end);
    emit(s, HEVC_SLICE, bits | last_bits(d, ex, ey));
    if (qpreset)
        emit(s, HEVC_QP, qp + d->qpbdoffset);
    emit(s, HEVC_MODE, 0xffff | ((ex == d->w - 1) << 17) | ((ey == d->h - 1) << 18));
    emit(s, HEVC_CONTROL, x | (y << 16));

    s->tile = t;
    s->ex = x;
    s->ey = y;
    s->qp = qp;
    s->bits = bits;
}

/* Close the remembered tile and open every following one up to tile t */
static void fill_tiles(struct hevc_state *s, ULONG t)
{
    const struct hevc_dims *d = s->d;

    while (s->tile < t)
    {
        ULONG tx = s->tile % d->ncols, ty = s->tile / d->ncols;
        ULONG nt = s->tile + 1;

        emit(s, HEVC_STATUS, status_at(2, d->colbd[tx + 1] - 1, d->rowbd[ty + 1] - 1));
        emit(s, HEVC_TRANSFER, HEVC_TRANSFER_RELOAD);
        tile_entry(s, nt, d->colbd[nt % d->ncols], d->rowbd[nt / d->ncols],
                   TRUE, FALSE, s->qp, s->bits);
    }
}

/* WPP mode: a region is one CTB row */
static void wpp_entry(struct hevc_state *s, ULONG x, ULONG y, BOOL qpreset, BOOL bte,
                      ULONG qp, ULONG bits)
{
    const struct hevc_dims *d = s->d;
    ULONG end = (d->w - 1) | (y << 16);

    emit(s, HEVC_TILESTART, 0);
    emit(s, HEVC_TILEEND, end);
    if (bte)
        emit(s, HEVC_BEGINTILEEND, end);
    emit(s, HEVC_SLICE, bits | last_bits(d, d->w - 1, y));
    if (qpreset)
        emit(s, HEVC_QP, qp + d->qpbdoffset);
    emit(s, HEVC_MODE, 1 | (1 << 17) | ((y == d->h - 1) << 18));
    emit(s, HEVC_CONTROL, x | (y << 16));

    s->ex = x;
    s->ey = y;
    s->qp = qp;
    s->bits = bits;
}

/* Save the contexts after CTB column 1 of row r (the WPP storage point) */
static void wpp_pause(struct hevc_state *s, ULONG r)
{
    const struct hevc_dims *d = s->d;

    emit(s, HEVC_STATUS, status_at(5, 1, r));
    emit(s, HEVC_TRANSFER, HEVC_TRANSFER_BACKUP);
    emit(s, HEVC_MODE, (r == d->h - 1) ? 0x70000 : 0x30000);
    emit(s, HEVC_CONTROL, 2 | (r << 16));
}

static void fill_rows(struct hevc_state *s, ULONG last)
{
    const struct hevc_dims *d = s->d;

    while (s->ey < last)
    {
        if (d->w > 2)
            wpp_pause(s, s->ey);
        emit(s, HEVC_STATUS, status_at(2, d->w - 1, s->ey));
        emit(s, HEVC_TRANSFER, (d->w == 2) ? HEVC_TRANSFER_BACKUP : HEVC_TRANSFER_RELOAD);
        wpp_entry(s, 0, s->ey + 1, TRUE, FALSE, s->qp, s->bits);
    }
}

static void emit_cabac(struct hevc_state *s, const VAPictureParameterBufferHEVC *pp,
                       const VASliceParameterBufferHEVC *sp)
{
    ULONG words[(HEVC_CABAC_STATES + 2) / 4], i;

    cabac_words(pp, sp, words);
    for (i = 0; i < (HEVC_CABAC_STATES + 2) / 4; i++)
        emit(s, HEVC_CABAC_TABLE + 4 * i, words[i]);
    emit(s, HEVC_TRANSFER, HEVC_TRANSFER_BACKUP);
}

const char *hevc_build(const struct hevc_picture *pic, struct hevc_cl *cl,
                       struct hevc_phase2 *p2, ULONG *map)
{
    const VAPictureParameterBufferHEVC *pp = pic->pp;
    struct hevc_dims d;
    struct hevc_state s;
    ULONG msgs[HEVC_MAX_MSGS];
    ULONG sliceaddr = 0, i, k;
    const VASliceParameterBufferHEVC *colsp = NULL;
    const char *err = NULL;

    /* What the block can do: Main and Main10, 4:2:0 */
    if (pp->pic_fields.bits.chroma_format_idc != 1 || pp->pic_fields.bits.separate_colour_plane_flag)
        return "not 4:2:0";
    if ((pp->bit_depth_luma_minus8 != 0 && pp->bit_depth_luma_minus8 != 2)
        || (pp->bit_depth_chroma_minus8 != pp->bit_depth_luma_minus8))
        return "not 8 or 10-bit";
    if (pp->pic_width_in_luma_samples < HEVC_MIN_SIZE || pp->pic_width_in_luma_samples > HEVC_MAX_SIZE
        || pp->pic_height_in_luma_samples < HEVC_MIN_SIZE || pp->pic_height_in_luma_samples > HEVC_MAX_SIZE)
        return "picture size";
    if (!hevc_ctb_ok(pp)
        || pp->log2_min_transform_block_size_minus2 + 2 + pp->log2_diff_max_min_transform_block_size > 5)
        return "block sizes";
    if (!pic->nslices || pic->slices[0].sp.slice_segment_address)
        return "first slice not at 0";
    if (pp->pic_fields.bits.entropy_coding_sync_enabled_flag && pp->pic_fields.bits.tiles_enabled_flag
        && (pp->num_tile_columns_minus1 || pp->num_tile_rows_minus1))
        return "WPP with tiles";

    memset(&d, 0, sizeof(d));
    d.ctblog2 = ctblog2(pp);
    d.ctbsize = 1 << d.ctblog2;
    d.picw = pp->pic_width_in_luma_samples;
    d.pich = pp->pic_height_in_luma_samples;
    d.w = (d.picw + d.ctbsize - 1) >> d.ctblog2;
    d.h = (d.pich + d.ctbsize - 1) >> d.ctblog2;
    d.qpbdoffset = 6 * pp->bit_depth_luma_minus8;
    d.wpp = pp->pic_fields.bits.entropy_coding_sync_enabled_flag;
    if (!hevc_scan(pp, &d, map))
        return "tile layout";

    memset(&s, 0, sizeof(s));
    s.cl = cl;
    s.d = &d;
    cl->n = 0;
    cl->overflow = FALSE;

    for (i = 0; i < pic->nslices; i++)
    {
        const VASliceParameterBufferHEVC *sp = &pic->slices[i].sp;
        ULONG rs = sp->slice_segment_address, ts, prs, x0, y0, xp = 0, yp = 0, nmsg;
        BOOL dep = (i > 0) && sp->LongSliceFlags.fields.dependent_slice_segment_flag;
        BOOL last = (i == pic->nslices - 1);
        BOOL qpreset;
        UQUAD start;
        ULONG len;
        LONG qp = slice_qp(pp, sp);

        if (rs >= d.w * d.h)
            return "slice address";
        if (sp->slice_data_byte_offset > sp->slice_data_size)
            return "slice data offset";
        /* At most 15 active references a list; VA's counts go to 256 */
        if ((nref(sp, 0) > 15) || (nref(sp, 1) > 15))
            return "reference count";
        ts = d.rs2ts[rs];
        x0 = rs % d.w;
        y0 = rs / d.w;
        if (i > 0)
        {
            if (!ts)
                return "slice address";
            prs = d.ts2rs[ts - 1];
            xp = prs % d.w;
            yp = prs / d.w;
        }

        if (!colsp && !slice_is(sp, HEVC_SLICE_I) && sp->LongSliceFlags.fields.slice_temporal_mvp_enabled_flag)
            colsp = sp;

        /* Close the previous segment */
        if (i > 0)
        {
            if (d.wpp)
            {
                fill_rows(&s, yp);
                if (s.ex < 2 && (s.ey < y0 || x0 > 2) && d.w > 2)
                    wpp_pause(&s, yp);
                emit(&s, HEVC_STATUS, status_at(1, xp, yp));
                if (x0 == 2 || (d.w == 2 && s.ey < y0))
                    emit(&s, HEVC_TRANSFER, HEVC_TRANSFER_BACKUP);
            }
            else
            {
                fill_tiles(&s, tile_of(&d, xp, yp));
                emit(&s, HEVC_STATUS, status_at(1, xp, yp));
            }
        }

        /* Its slice data: the block drops emulation-prevention bytes itself */
        start = pic->slices[i].data + sp->slice_data_offset + sp->slice_data_byte_offset;
        len = sp->slice_data_size - sp->slice_data_byte_offset;
        emit(&s, HEVC_BFBASE, HEVC_ADDR(start));
        emit(&s, HEVC_BFNUM, len);
        emit(&s, HEVC_BFCONTROL, (start & 63) | 0x80);
        emit(&s, HEVC_BFCONTROL, (start & 63) | 0x40);

        /* Fresh contexts, WPP-synchronised ones, or carry on */
        if (d.wpp)
        {
            if (i == 0 || !dep || d.w == 1)
            {
                emit_cabac(&s, pp, sp);
                qpreset = TRUE;
            }
            else if (x0 == 0)
            {
                emit(&s, HEVC_TRANSFER, HEVC_TRANSFER_RELOAD);
                qpreset = TRUE;
            }
            else
                qpreset = FALSE;
        }
        else
        {
            qpreset = (i == 0) || !dep || (tile_of(&d, x0, y0) != tile_of(&d, xp, yp));
            if (qpreset)
                emit_cabac(&s, pp, sp);
        }

        nmsg = slice_msgs(pic, sp, msgs, &err);
        if (err)
            return err;
        emit(&s, HEVC_SLICECMDS, nmsg | (i << 8));
        for (k = 0; k < nmsg; k++)
            emit(&s, HEVC_SLICE_MSGS + 4 * k, msgs[k]);
        emit(&s, HEVC_SPS0, reg_sps0(pp));
        emit(&s, HEVC_SPS1, reg_sps1(pp));
        emit(&s, HEVC_PPS, reg_pps(pp, sp));

        /* The block always scales by the table: flat 16 without lists */
        if (i == 0)
        {
            ULONG words[HEVC_SCALING_WORDS];

            if (!pp->pic_fields.bits.scaling_list_enabled_flag)
            {
                for (k = 0; k < HEVC_SCALING_WORDS; k++)
                    words[k] = 0x10101010;
            }
            else if (!pic->iq)
                return "no scaling lists";
            else
                scaling_words(pic->iq, words);
            for (k = 0; k < HEVC_SCALING_WORDS; k++)
                emit(&s, HEVC_SCALING_TABLE + 4 * k, words[k]);
        }

        if (!dep)
            sliceaddr = x0 | (y0 << 16);
        emit(&s, HEVC_SLICESTART, sliceaddr);

        if (d.wpp)
            wpp_entry(&s, x0, y0, qpreset, !dep, qp, slice_bits(sp));
        else
            tile_entry(&s, tile_of(&d, x0, y0), x0, y0, qpreset, !dep, qp, slice_bits(sp));

        /* The picture's end */
        if (last)
        {
            if (d.wpp)
            {
                fill_rows(&s, d.h - 1);
                if (s.ex < 2 && d.w > 2)
                    wpp_pause(&s, d.h - 1);
            }
            else
                fill_tiles(&s, d.ncols * d.nrows - 1);
            emit(&s, HEVC_STATUS, status_at(1, d.w - 1, d.h - 1));
        }
    }

    if (cl->overflow)
        return "command list full";

    /* Phase 2 */
    memset(p2, 0, sizeof(*p2));
    p2->numrows = d.h;
    p2->framesize = d.picw | (d.pich << 16);
    p2->currpoc = pp->CurrPic.pic_order_cnt;
    p2->outy = pic->out.luma;
    p2->outc = pic->out.chroma;
    p2->outstride = pic->pitch;
    p2->config2 = (pp->bit_depth_luma_minus8 + 8) | ((pp->bit_depth_chroma_minus8 + 8) << 4)
                  | ((pp->bit_depth_luma_minus8 != 0) << 8) | ((pp->bit_depth_chroma_minus8 != 0) << 9)
                  | (d.ctblog2 << 10)
                  | (pp->pic_fields.bits.constrained_intra_pred_flag << 13)
                  | (pp->pic_fields.bits.strong_intra_smoothing_enabled_flag << 14)
                  | ((pp->log2_parallel_merge_level_minus2 + 2) << 16)
                  | (pp->slice_parsing_fields.bits.sps_temporal_mvp_enabled_flag
                     ? (pic->slices[0].sp.LongSliceFlags.fields.slice_temporal_mvp_enabled_flag << 19) : 0)
                  | (pp->pic_fields.bits.pcm_loop_filter_disabled_flag << 20)
                  | ((pp->pps_cb_qp_offset & 31) << 21)
                  | ((ULONG)(pp->pps_cr_qp_offset & 31) << 26);

    /* Every picture may become a collocated one, so its MVs are kept */
    if (pp->slice_parsing_fields.bits.sps_temporal_mvp_enabled_flag)
    {
        if (!pic->out.colmv)
            return "no MV buffer for the picture";
        p2->config2 |= 1 << 15;
        p2->mvbase = pic->out.colmv;
        p2->mvstride = hevc_colmv_stride(d.picw);
        if (colsp)
        {
            ULONG list = col_from_l0(colsp) ? 0 : 1;
            ULONG idx;

            if (colsp->collocated_ref_idx >= nref(colsp, list))
                return "collocated reference";
            idx = colsp->RefPicList[list][colsp->collocated_ref_idx];
            if (idx >= 15 || !pic->refs[idx].colmv)
                return "no collocated MVs";
            p2->colbase = pic->refs[idx].colmv;
        }
    }

    /* Unused slots point at the picture itself: never a stray read */
    for (i = 0; i < HEVC_REF_SLOTS; i++)
    {
        const struct hevc_surface *ref = (i < 15) ? &pic->refs[i] : NULL;

        if (ref && ref->luma && !(pp->ReferenceFrames[i].flags & VA_PICTURE_HEVC_INVALID)
            && pp->ReferenceFrames[i].picture_id != VA_INVALID_SURFACE)
        {
            p2->refy[i] = ref->luma;
            p2->refc[i] = ref->chroma;
        }
        else
        {
            p2->refy[i] = pic->out.luma;
            p2->refc[i] = pic->out.chroma;
        }
    }

    return NULL;
}
