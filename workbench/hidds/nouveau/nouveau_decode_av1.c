/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: NVDEC AV1 decoding (profile 0, 8 and 10-bit 4:2:0) for the
          gfx.hidd decode interface. VA gives the parsed frame header and
          the tile data; the driver fills nvdec_av1_pic_s, keeps per picture
          the state later frames need (CDF tables, segment map, motion
          vectors, order hints) and derives what VA leaves out (skip-mode
          frames, sign biases, motion-field sources, scale factors).
          Programmed as NVIDIA's driver does (traced): application id 0x12,
          the tile table in the old combined format, the engine adapting the
          CDFs into the current picture's table and later frames reading the
          primary reference's table.
*/

#include "nouveau_intern.h"
#include "nouveau_decode.h"
#include "nouveau_vadec.h"
#include "nouveau_av1cdf.h"
#include "nouveau_av1grain.h"

#define DEBUG 0
#include <aros/debug.h>
#include <proto/exec.h>
#include <string.h>

#include <nvidia/video/nvdec_drv.h>

#define NVDEC_APPLICATION_ID_AV1            0x12    /* AVD */
#define NVDEC_CONTROL_CODEC_AV1             0x0A
#define NVDEC_SET_COLOC_DATA_OFFSET         0x0414
#define NVDEC_SET_INTRA_TOP_BUF_OFFSET      0x04C8
#define NVDEC_SET_TILE_SIZE_BUF_OFFSET      0x04CC
#define NVDEC_SET_FILTER_BUFFER_OFFSET      0x04D0
#define NVDEC_AV1_SET_PROB_TAB_READ         0x0640
#define NVDEC_AV1_SET_PROB_TAB_WRITE        0x0644
#define NVDEC_AV1_SET_SEGMENT_READ          0x0648
#define NVDEC_AV1_SET_SEGMENT_WRITE         0x064C
#define NVDEC_AV1_SET_COL_MV0_READ          0x0650
#define NVDEC_AV1_SET_COL_MVWRITE           0x065C
#define NVDEC_AV1_SET_GLOBAL_MODEL          0x0660
#define NVDEC_AV1_SET_FILM_GRAIN            0x0664

#define NVDEC_TILEFORMAT_KBL                1
#define NVDEC_GOB_HEIGHT_2                  0

/* in the frame's picsetup buffer (nouveau_decode.h): the setup at 0, then (NVIDIA's offsets) */
#define AV1_TILES_OFFSET                    0x0300      /* AV1TileInfo_OLD fields in 16-byte entries (traced) */
#define AV1_GLOBAL_OFFSET                   0x1300      /* 7 x 32 bytes */
#define AV1_TILE_ENTRY                      16
#define AV1_MAX_TILES_HERE                  ((AV1_GLOBAL_OFFSET - AV1_TILES_OFFSET) / AV1_TILE_ENTRY)

#define AV1_CDF_BYTES                       0x3000      /* one table (12256 bytes), 256-byte aligned */
#define AV1_GRAIN_BYTES                     0x3400      /* AV1FilmGrainMemory, 256-byte aligned */
#define AV1_RING_BYTES                      (AV1_CDF_BYTES + AV1_GRAIN_BYTES)   /* per ring entry: default CDFs, grain */
#define AV1_REFS                            7
#define AV1_PIC_CUR                         8           /* decoder picture index of the current picture */
#define AV1_PRIMARY_NONE                    7
#define AV1_KEY_FRAME                       0
#define AV1_INTRA_ONLY_FRAME                2

/* what a later frame needs of a decoded picture (per pool slot) */
struct av1_saved
{
    BOOL    valid;
    UBYTE   frame_type;
    UBYTE   order_hint;
    UBYTE   ref_order_hint[AV1_REFS];
    BOOL    segmap;                 /* its segment map was written */
    UWORD   upscaled_width, height;
    UWORD   mi_cols, mi_rows;
};

struct av1_state
{
    VADecPictureParameterBufferAV1  pp;
    BOOL                have_pp;
    VASliceParameterBufferAV1       sp[64];     /* the tile group the next data buffer belongs to */
    ULONG               nsp;
    struct { ULONG start, end; } tile[AV1_MAX_TILES_HERE];
    ULONG               ntiles;
    struct nouveau_bo   *cdf, *seg, *mv;        /* per pool slot */
    struct nouveau_bo   *gart;                  /* per ring entry: default CDF tables, film grain memory */
    uint64_t            zero;                   /* zeroed segment map / MVs (after the per-slot MVs) */
    struct nouveau_bo   *filter, *intratop, *blkbuf;
    ULONG               segsize, mvsize;
    ULONG               flt[6];                 /* filter buffer parts, 256-byte units */
    struct av1_saved    saved[NOUVEAU_DECODE_SLOTS];
};

static BOOL av1_zeroed(struct HIDDNouveauDecode *dec, ULONG flags, ULONG size, struct nouveau_bo **bo)
{
    if (nouveau_bo_new(dec->carddata->dev, flags | NOUVEAU_BO_MAP, 4096, size, NULL, bo) ||
        nouveau_bo_map(*bo, NOUVEAU_BO_WR, dec->carddata->client))
        return FALSE;
    memset((*bo)->map, 0, size);
    /* cache maintenance only on system memory: VRAM is mapped uncached through the BAR */
    if (flags & NOUVEAU_BO_GART)
        nouveau_staging_to_gpu((*bo)->map, size);
    return TRUE;
}

#define NV_ALIGN256(v)      ALIGN_UP((v), 256)

static BOOL av1_open(struct HIDDNouveauDecode *dec)
{
    struct av1_state *v = AllocMem(sizeof(*v), MEMF_PUBLIC | MEMF_CLEAR);
    ULONG w = dec->width, h = dec->height, cols64, rows64, sizes[6], total, i;

    if (!v)
        return FALSE;
    dec->cur.codecstate = v;
    dec->rowalign = 64;
    dec->minimal = TRUE;

    /* nvdec_drv.h's sizes (AV1_SEGMENT_ID_BUF_SIZE, AV1_TEMPORAL_MV_BUF_SIZE, the AV1_*_BUF_SIZE filter parts) */
    v->segsize = ALIGN_UP(ALIGN_UP(w, 128) * ALIGN_UP(h, 128) / (64 * 64) * 128, 4096);
    v->mvsize = ALIGN_UP(ALIGN_UP(w, 128) * ALIGN_UP(h, 128) / (64 * 64) * 256, 4096);
    cols64 = ALIGN_UP(w, 64) / 64 + 2;
    rows64 = ALIGN_UP(h, 64) / 64 + 2;
    /* NVIDIA's order (traced): top, deblock vertical, CDEF, upscaler (ping-pong), restoration, film grain */
    sizes[0] = NV_ALIGN256(cols64 * 1920);
    sizes[1] = NV_ALIGN256(rows64 * 1536);
    sizes[2] = NV_ALIGN256(rows64 * 1792);
    sizes[3] = 2 * NV_ALIGN256(rows64 * 12544);
    sizes[4] = NV_ALIGN256(rows64 * 1920);
    sizes[5] = NV_ALIGN256(rows64 * 384);
    for (i = 0, total = 0; i < 6; i++)
    {
        v->flt[i] = total >> 8;
        total += sizes[i];
    }

    if (!av1_zeroed(dec, NOUVEAU_BO_VRAM, NOUVEAU_DECODE_SLOTS * AV1_CDF_BYTES, &v->cdf) ||
        !av1_zeroed(dec, NOUVEAU_BO_VRAM, NOUVEAU_DECODE_SLOTS * v->segsize, &v->seg) ||
        !av1_zeroed(dec, NOUVEAU_BO_VRAM, (NOUVEAU_DECODE_SLOTS + 1) * v->mvsize, &v->mv) ||
        !av1_zeroed(dec, NOUVEAU_BO_GART, NOUVEAU_DECODE_RING * AV1_RING_BYTES, &v->gart) ||
        !av1_zeroed(dec, NOUVEAU_BO_VRAM, total, &v->filter) ||
        !av1_zeroed(dec, NOUVEAU_BO_VRAM, 8 * 8192, &v->intratop))
        return FALSE;
    v->zero = v->mv->offset + (uint64_t)NOUVEAU_DECODE_SLOTS * v->mvsize;      /* mvsize >= segsize */
    if ((dec->oclass >= NVDEC_CLASS_BLACKWELL) &&
        !av1_zeroed(dec, NOUVEAU_BO_VRAM, NVDEC_BLK_BUF_SIZE(w), &v->blkbuf))
        return FALSE;
    return TRUE;
}

static VOID av1_close(struct HIDDNouveauDecode *dec)
{
    struct av1_state *v = dec->cur.codecstate;

    if (!v)
        return;
    if (v->cdf)      nouveau_bo_ref(NULL, &v->cdf);
    if (v->seg)      nouveau_bo_ref(NULL, &v->seg);
    if (v->mv)       nouveau_bo_ref(NULL, &v->mv);
    if (v->gart)     nouveau_bo_ref(NULL, &v->gart);
    if (v->filter)   nouveau_bo_ref(NULL, &v->filter);
    if (v->intratop) nouveau_bo_ref(NULL, &v->intratop);
    if (v->blkbuf)   nouveau_bo_ref(NULL, &v->blkbuf);
    FreeMem(v, sizeof(*v));
    dec->cur.codecstate = NULL;
}

static BOOL av1_begin(struct HIDDNouveauDecode *dec)
{
    struct av1_state *v = dec->cur.codecstate;

    if (!v)
        return FALSE;
    v->have_pp = FALSE;
    v->nsp = 0;
    v->ntiles = 0;
    return TRUE;
}

static BOOL av1_params(struct HIDDNouveauDecode *dec, ULONG type, APTR data, ULONG size, ULONG count)
{
    struct av1_state *v = dec->cur.codecstate;

    switch (type)
    {
    case VAPictureParameterBufferType:
        if (size < sizeof(VADecPictureParameterBufferAV1))
            return FALSE;
        memcpy(&v->pp, data, sizeof(v->pp));
        v->have_pp = TRUE;
        return TRUE;

    case VASliceParameterBufferType:
        /* one entry per tile of the tile group whose data follows */
        if ((size < sizeof(VASliceParameterBufferAV1)) || (count > 64))
            return FALSE;
        memcpy(v->sp, data, count * sizeof(VASliceParameterBufferAV1));
        v->nsp = count;
        return TRUE;

    default:
        return TRUE;
    }
}

/* a tile group's data: kept whole; each tile's {start, end} from its slice parameter */
static BOOL av1_bitstream(struct HIDDNouveauDecode *dec, APTR data, ULONG size)
{
    struct av1_state *v = dec->cur.codecstate;
    ULONG base = dec->cur.bitstreamlen, i;
    UBYTE *p;

    if (!v->have_pp || !v->nsp)
        return FALSE;
    p = HIDDNouveauDecodeBitstreamReserve(dec, size);
    if (!p)
        return FALSE;
    memcpy(p, data, size);
    for (i = 0; i < v->nsp; i++)
    {
        const VASliceParameterBufferAV1 *sp = &v->sp[i];
        ULONG t = sp->tile_row * v->pp.tile_cols + sp->tile_column;

        if ((t >= AV1_MAX_TILES_HERE) || (sp->slice_data_offset > size) ||
            (sp->slice_data_size > size - sp->slice_data_offset))
            return FALSE;
        v->tile[t].start = base + sp->slice_data_offset;
        v->tile[t].end = base + sp->slice_data_offset + sp->slice_data_size;
        if (t + 1 > v->ntiles)
            v->ntiles = t + 1;
    }
    v->nsp = 0;
    dec->cur.nslices = v->ntiles;
    return TRUE;
}

static ULONG av1_status(struct HIDDNouveauDecode *dec, ULONG *status)
{
    nvdec_status_s *st = (nvdec_status_s *)status;

    if (!st->error_status || (st->error_status == 0x20))
        return 0;
    if (dec->errors++ < 4)
        nvlog("[Nouveau] NVDEC: AV1 frame error %08lx\n", (unsigned long)st->error_status);
    return st->error_status;
}

/* spec get_relative_dist */
static LONG av1_rd(const VADecPictureParameterBufferAV1 *pp, LONG a, LONG b)
{
    LONG diff, m;

    if (!pp->seq_info_fields.fields.enable_order_hint)
        return 0;
    diff = a - b;
    m = 1L << pp->order_hint_bits_minus_1;
    return (diff & (m - 1)) - (diff & m);
}

static LONG av1_slot(struct HIDDNouveauDecode *dec, VASurfaceID surface)
{
    OOP_Object *bm;
    ULONG buffer;
    LONG slot;

    if ((surface == VA_INVALID_SURFACE) || !HIDDNouveauDecodeResolve(dec, surface, &bm, &buffer))
        return -1;
    slot = HIDDNouveauDecodeFindSlot(dec, bm, buffer);
    if (slot >= 0)
        dec->slot[slot].lastuse = dec->frames;
    return slot;
}

/* AV1 spec 7.11.3.7 Div_Lut */
static const UWORD av1_div_lut[257] =
{
    16384, 16320, 16257, 16194, 16132, 16070, 16009, 15948, 15888, 15828, 15768, 15709,
    15650, 15592, 15534, 15477, 15420, 15364, 15308, 15252, 15197, 15142, 15087, 15033,
    14980, 14926, 14873, 14821, 14769, 14717, 14665, 14614, 14564, 14513, 14463, 14413,
    14364, 14315, 14266, 14218, 14170, 14122, 14075, 14028, 13981, 13935, 13888, 13843,
    13797, 13752, 13707, 13662, 13618, 13574, 13530, 13487, 13443, 13400, 13358, 13315,
    13273, 13231, 13190, 13148, 13107, 13066, 13026, 12985, 12945, 12906, 12866, 12827,
    12788, 12749, 12710, 12672, 12633, 12596, 12558, 12520, 12483, 12446, 12409, 12373,
    12336, 12300, 12264, 12228, 12193, 12157, 12122, 12087, 12053, 12018, 11984, 11950,
    11916, 11882, 11848, 11815, 11782, 11749, 11716, 11683, 11651, 11619, 11586, 11555,
    11523, 11491, 11460, 11429, 11398, 11367, 11336, 11305, 11275, 11245, 11215, 11185,
    11155, 11125, 11096, 11067, 11038, 11009, 10980, 10951, 10923, 10894, 10866, 10838,
    10810, 10782, 10755, 10727, 10700, 10673, 10645, 10618, 10592, 10565, 10538, 10512,
    10486, 10460, 10434, 10408, 10382, 10356, 10331, 10305, 10280, 10255, 10230, 10205,
    10180, 10156, 10131, 10107, 10082, 10058, 10034, 10010, 9986, 9963, 9939, 9916,
    9892, 9869, 9846, 9823, 9800, 9777, 9754, 9732, 9709, 9687, 9664, 9642,
    9620, 9598, 9576, 9554, 9533, 9511, 9489, 9468, 9447, 9425, 9404, 9383,
    9362, 9341, 9321, 9300, 9279, 9259, 9239, 9218, 9198, 9178, 9158, 9138,
    9118, 9098, 9079, 9059, 9039, 9020, 9001, 8981, 8962, 8943, 8924, 8905,
    8886, 8867, 8849, 8830, 8812, 8793, 8775, 8756, 8738, 8720, 8702, 8684,
    8666, 8648, 8630, 8613, 8595, 8577, 8560, 8542, 8525, 8508, 8490, 8473,
    8456, 8439, 8422, 8405, 8389, 8372, 8355, 8339, 8322, 8306, 8289, 8273,
    8257, 8240, 8224, 8208, 8192,
};

static LONG av1_round2signed(int64_t x, LONG n)
{
    if (!n)
        return (LONG)x;
    return (x >= 0) ? (LONG)((x + (1LL << (n - 1))) >> n) : -(LONG)((-x + (1LL << (n - 1))) >> n);
}

static WORD av1_clip16(int64_t x)
{
    return (x < -32768) ? -32768 : (x > 32767) ? 32767 : (WORD)x;
}

/* spec 7.11.3.6 setupShear: the four shear parameters the engine wants after a global model's matrix (traced) */
static VOID av1_shear(const int32_t *m, WORD *out)
{
    ULONG d = (m[2] < 0) ? -m[2] : m[2], n = 0, e;
    LONG shift, div, k;
    int64_t v;
    WORD p[4];

    while ((d >> n) > 1)
        n++;
    e = d - (1UL << n);
    div = av1_div_lut[(n > 8) ? av1_round2signed(e, n - 8) : (LONG)(e << (8 - n))];
    shift = n + 14;
    if (m[2] < 0)
        div = -div;
    p[0] = av1_clip16((int64_t)m[2] - (1 << 16));
    p[1] = av1_clip16(m[3]);
    v = ((int64_t)m[4] << 16) * div;
    p[2] = av1_clip16(av1_round2signed(v, shift));
    v = ((int64_t)m[3] * m[4]) * div;
    p[3] = av1_clip16((int64_t)m[5] - av1_round2signed(v, shift) - (1 << 16));
    for (k = 0; k < 4; k++)
        out[k] = (WORD)(av1_round2signed(p[k], 6) * 64);
}

static BOOL av1_submit(struct HIDDNouveauDecode *dec, struct HIDDNouveauDecodeFrame *f)
{
    struct av1_state *v = dec->cur.codecstate;
    const VADecPictureParameterBufferAV1 *pp;
    nvdec_av1_pic_s *pic;
    UBYTE *base, *pad;
    LONG slot, refslot[AV1_REFS], mapslot[8], i, k, prim = -1;
    ULONG avoid = 0, ring, upw, fw, fh, bitdepth, micols, mirows;
    BOOL intra, defaults;
    LONG mfsrc[3], nmf = 0, dslot;
    OOP_Object *rbm = NULL;
    ULONG rbuffer = 0;
    BOOL grain;
    struct av1_saved *cs;

    if (!v || !v->have_pp || !v->ntiles)
        return FALSE;
    pp = &v->pp;
    bitdepth = 8 + 2 * pp->bit_depth_idx;
    upw = pp->frame_width_minus1 + 1;
    fh = pp->frame_height_minus1 + 1;
    if ((pp->profile != 0) || (bitdepth > 10) || ((bitdepth == 10) != (dec->bpp == 2)) ||
        pp->seq_info_fields.fields.mono_chrome || (upw > dec->width) || (fh > dec->height) ||
        pp->pic_info_fields.bits.large_scale_tile || (v->ntiles > AV1_MAX_TILES_HERE))
    {
        nvlog("[Nouveau] NVDEC: AV1 stream (%lux%lu, profile %u, %lu-bit) does not fit the session\n",
            (unsigned long)upw, (unsigned long)fh, pp->profile, (unsigned long)bitdepth);
        return FALSE;
    }
    if (pp->pic_info_fields.bits.use_superres)
    {
        ULONG d = pp->superres_scale_denominator;

        fw = (upw * 8 + d / 2) / d;
        if (fw < ((upw < 16) ? upw : 16))
            fw = (upw < 16) ? upw : 16;
    }
    else
        fw = upw;
    micols = 2 * ((fw + 7) >> 3);
    mirows = 2 * ((fh + 7) >> 3);
    intra = (pp->pic_info_fields.bits.frame_type == AV1_KEY_FRAME) ||
            (pp->pic_info_fields.bits.frame_type == AV1_INTRA_ONLY_FRAME);

    /* every picture AV1 may still reference stays in the pool */
    for (i = 0; i < 8; i++)
    {
        mapslot[i] = av1_slot(dec, pp->ref_frame_map[i]);
        if (mapslot[i] >= 0)
            avoid |= 1UL << mapslot[i];
    }
    for (i = 0; i < AV1_REFS; i++)
    {
        refslot[i] = intra ? -1 : mapslot[pp->ref_frame_idx[i] & 7];
        if ((refslot[i] >= 0) && !v->saved[refslot[i]].valid)
            refslot[i] = -1;
    }
    /*
     * The target is VA's current_frame, the grain-free reference. With film
     * grain, FFmpeg shows another surface, current_display_picture: the
     * engine writes the grained picture there (the display buffer).
     */
    grain = pp->film_grain_info.film_grain_info_fields.bits.apply_grain;
    slot = HIDDNouveauDecodeAllocSlot(dec, dec->cur.bm, dec->cur.buffer, avoid);
    if (slot < 0)
        return FALSE;
    dec->cur.slot = slot;
    dslot = slot;
    if (grain)
    {
        if (!HIDDNouveauDecodeResolve(dec, pp->current_display_picture, &rbm, &rbuffer) ||
            (OOP_OCLASS(rbm) != dec->bmclass))
            return FALSE;
        dslot = HIDDNouveauDecodeAllocSlot(dec, rbm, rbuffer, avoid | (1UL << slot));
        if ((dslot < 0) || (dslot == slot))
            return FALSE;
    }
    if (!intra && (pp->primary_ref_frame < AV1_REFS))
        prim = refslot[pp->primary_ref_frame];
    defaults = (prim < 0) || pp->pic_info_fields.bits.error_resilient_mode;
    ring = (ULONG)(f - dec->ring);

    base = (UBYTE *)f->picsetup->map;
    pic = (nvdec_av1_pic_s *)base;
    memset(base, 0, NOUVEAU_DECODE_SLICEOFFS_OFFSET);
    memset(base + AV1_GLOBAL_OFFSET, 0, 0x100);
    memset(base + NOUVEAU_DECODE_STATUS_OFFSET, 0, 256);

    /* tiles: {width, height} in superblocks + {start, end} in the bitstream (AV1TileInfo_OLD) */
    {
        UBYTE *t = base + AV1_TILES_OFFSET;
        ULONG r, c, n = 0;

        for (r = 0; r < pp->tile_rows; r++)
            for (c = 0; c < pp->tile_cols; c++, n++, t += AV1_TILE_ENTRY)
            {
                t[0] = (UBYTE)(pp->width_in_sbs_minus_1[c] + 1);
                t[1] = (UBYTE)(pp->height_in_sbs_minus_1[r] + 1);
                t[2] = v->tile[n].start;       t[3] = v->tile[n].start >> 8;
                t[4] = v->tile[n].start >> 16; t[5] = v->tile[n].start >> 24;
                t[6] = v->tile[n].end;         t[7] = v->tile[n].end >> 8;
                t[8] = v->tile[n].end >> 16;   t[9] = v->tile[n].end >> 24;
            }
    }

    pic->gptimer_timeout_value = 0x4d3f640;
    pic->stream_len = dec->cur.bitstreamlen;
    pic->use_128x128_superblock = pp->seq_info_fields.fields.use_128x128_superblock;
    pic->chroma_format = 1;
    pic->bit_depth = bitdepth;
    pic->enable_filter_intra = pp->seq_info_fields.fields.enable_filter_intra;
    pic->enable_intra_edge_filter = pp->seq_info_fields.fields.enable_intra_edge_filter;
    pic->enable_interintra_compound = pp->seq_info_fields.fields.enable_interintra_compound;
    pic->enable_masked_compound = pp->seq_info_fields.fields.enable_masked_compound;
    pic->enable_dual_filter = pp->seq_info_fields.fields.enable_dual_filter;
    pic->enable_jnt_comp = pp->seq_info_fields.fields.enable_jnt_comp;
    /* as NVIDIA's driver: off for a frame whose CDEF does nothing (one strength set, all zero) */
    pic->enable_cdef = pp->seq_info_fields.fields.enable_cdef &&
        (pp->cdef_bits || pp->cdef_y_strengths[0] || pp->cdef_uv_strengths[0]);
    pic->frame_type = pp->pic_info_fields.bits.frame_type;
    pic->show_frame = pp->pic_info_fields.bits.show_frame;
    pic->disable_cdf_update = pp->pic_info_fields.bits.disable_cdf_update;
    pic->allow_screen_content_tools = pp->pic_info_fields.bits.allow_screen_content_tools;
    pic->cur_frame_force_integer_mv = pp->pic_info_fields.bits.force_integer_mv;
    pic->scale_denom_minus9 = pp->pic_info_fields.bits.use_superres ? pp->superres_scale_denominator - 9 : 0;
    pic->allow_intrabc = pp->pic_info_fields.bits.allow_intrabc;
    pic->allow_high_precision_mv = pp->pic_info_fields.bits.allow_high_precision_mv;
    pic->interp_filter = pp->interp_filter;
    pic->switchable_motion_mode = pp->pic_info_fields.bits.is_motion_mode_switchable;
    pic->use_ref_frame_mvs = pp->pic_info_fields.bits.use_ref_frame_mvs;
    pic->refresh_frame_context = !pp->pic_info_fields.bits.disable_frame_end_update_cdf;
    pic->delta_q_present_flag = pp->mode_control_fields.bits.delta_q_present_flag;
    pic->delta_q_res = pp->mode_control_fields.bits.log2_delta_q_res;
    pic->delta_lf_present_flag = pp->mode_control_fields.bits.delta_lf_present_flag;
    pic->delta_lf_res = pp->mode_control_fields.bits.log2_delta_lf_res;
    pic->delta_lf_multi = pp->mode_control_fields.bits.delta_lf_multi;
    {
        /* CodedLossless: every segment's qindex 0 and no DC/AC deltas */
        const VASegmentationStructAV1 *sg = &pp->seg_info;
        BOOL lossless = !(pp->y_dc_delta_q || pp->u_dc_delta_q || pp->u_ac_delta_q || pp->v_dc_delta_q || pp->v_ac_delta_q);

        for (i = 0; lossless && (i < 8); i++)
        {
            LONG q = pp->base_qindex;

            if (sg->segment_info_fields.bits.enabled && (sg->feature_mask[i] & 1))
                q += sg->feature_data[i][0];
            if (q < 0) q = 0;
            if (q > 255) q = 255;
            if (q)
                lossless = FALSE;
        }
        pic->coded_lossless = lossless;
    }
    pic->tile_enabled = (pp->tile_cols * pp->tile_rows) > 1;
    pic->superres_is_scaled = pp->pic_info_fields.bits.use_superres && (pp->superres_scale_denominator != 8);
    pic->tile_cols = pp->tile_cols;
    pic->tile_rows = pp->tile_rows;
    pic->context_update_tile_id = pp->context_update_tile_id;
    pic->cdef_damping_minus_3 = pp->cdef_damping_minus_3;
    pic->cdef_bits = pp->cdef_bits;
    {
        ULONG ypri = 0, uvpri = 0, ysec = 0, uvsec = 0;

        for (i = 0; i < (1L << pp->cdef_bits); i++)
        {
            ypri |= (ULONG)((pp->cdef_y_strengths[i] >> 2) & 0xf) << (4 * i);
            uvpri |= (ULONG)((pp->cdef_uv_strengths[i] >> 2) & 0xf) << (4 * i);
            ysec |= (ULONG)(pp->cdef_y_strengths[i] & 3) << (2 * i);
            uvsec |= (ULONG)(pp->cdef_uv_strengths[i] & 3) << (2 * i);
        }
        pic->cdef_y_pri_strength = ypri;
        pic->cdef_uv_pri_strength = uvpri;
        pic->cdef_y_sec_strength = ysec;
        pic->cdef_uv_sec_strength = uvsec;
    }
    {
        static const UBYTE txmode[4] = { 0, 3, 4, 4 };      /* ONLY_4X4, LARGEST, SELECT -> VP9-style numbering */

        pic->frame_tx_mode = txmode[pp->mode_control_fields.bits.tx_mode & 3];
    }
    pic->frame_reference_mode = pp->mode_control_fields.bits.reference_select ? 2 : 0;
    pic->allow_warp = pp->pic_info_fields.bits.allow_warped_motion;
    pic->reduced_tx_set_used = pp->mode_control_fields.bits.reduced_tx_set_used;
    pic->superres_upscaled_width = upw;
    if (pic->superres_is_scaled)
    {
        /* libaom av1_get_upscale_convolve_step / init subpel x */
        ULONG cup = (upw + 1) >> 1, cdown = (fw + 1) >> 1;
        LONG err, x0;

        pic->superres_luma_step = ((fw << 14) + upw / 2) / upw;
        pic->superres_chroma_step = ((cdown << 14) + cup / 2) / cup;
        err = (LONG)(upw * pic->superres_luma_step) - (LONG)(fw << 14);
        x0 = (-(((LONG)upw - (LONG)fw) << 13) + (LONG)upw / 2) / (LONG)upw + 128 - err / 2;
        pic->superres_init_luma_subpel_x = x0 & 0x3fff;
        err = (LONG)(cup * pic->superres_chroma_step) - (LONG)(cdown << 14);
        x0 = (-(((LONG)cup - (LONG)cdown) << 13) + (LONG)cup / 2) / (LONG)cup + 128 - err / 2;
        pic->superres_init_chroma_subpel_x = x0 & 0x3fff;
    }
    else
    {
        pic->superres_luma_step = 14;           /* NVIDIA's values when not scaled (traced) */
        pic->superres_chroma_step = 14;
    }
    pic->base_qindex = pp->base_qindex;
    pic->y_dc_delta_q = pp->y_dc_delta_q;
    pic->u_dc_delta_q = pp->u_dc_delta_q;
    pic->v_dc_delta_q = pp->v_dc_delta_q;
    pic->u_ac_delta_q = pp->u_ac_delta_q;
    pic->v_ac_delta_q = pp->v_ac_delta_q;
    pic->qm_y = pp->qmatrix_fields.bits.using_qmatrix ? pp->qmatrix_fields.bits.qm_y : 15;
    pic->qm_u = pp->qmatrix_fields.bits.using_qmatrix ? pp->qmatrix_fields.bits.qm_u : 15;
    pic->qm_v = pp->qmatrix_fields.bits.using_qmatrix ? pp->qmatrix_fields.bits.qm_v : 15;

    /* segmentation */
    {
        const VASegmentationStructAV1 *sg = &pp->seg_info;

        pic->segment_enabled = sg->segment_info_fields.bits.enabled;
        pic->segment_update_map = sg->segment_info_fields.bits.update_map;
        pic->segment_temporal_update = sg->segment_info_fields.bits.temporal_update;
        if (pic->segment_enabled)
            for (i = 0; i < 8; i++)
            {
                for (k = 0; k < 8; k++)
                    if (sg->feature_mask[i] & (1 << k))
                        pic->segment_feature_data[i][k] = sg->feature_data[i][k];
                if (sg->feature_mask[i])
                    pic->last_active_segid = i;
                if (sg->feature_mask[i] & 0xe0)
                    pic->segid_preskip = 1;
                if ((sg->feature_mask[i] & 1) && (sg->feature_data[i][0] < 0))
                    pic->segment_quant_sign |= 1 << i;
            }
        pic->prevsegid_flag = (prim >= 0) && v->saved[prim].segmap &&
            (v->saved[prim].mi_cols == micols) && (v->saved[prim].mi_rows == mirows);
    }

    /* loop filter, restoration */
    pic->filter_level[0] = pp->filter_level[0];
    pic->filter_level[1] = pp->filter_level[1];
    pic->filter_level_u = pp->filter_level_u;
    pic->filter_level_v = pp->filter_level_v;
    pic->lf_sharpness_level = pp->loop_filter_info_fields.bits.sharpness_level;
    if (pp->loop_filter_info_fields.bits.mode_ref_delta_enabled)
    {
        memcpy(pic->lf_ref_deltas, pp->ref_deltas, 8);
        memcpy(pic->lf_mode_deltas, pp->mode_deltas, 2);
    }
    pic->lr_type = pp->loop_restoration_fields.bits.yframe_restoration_type |
                   (pp->loop_restoration_fields.bits.cbframe_restoration_type << 2) |
                   (pp->loop_restoration_fields.bits.crframe_restoration_type << 4);
    {
        ULONG y = 1 + pp->loop_restoration_fields.bits.lr_unit_shift, uv;

        if (pp->seq_info_fields.fields.use_128x128_superblock)
            y++;                                /* the spec reads one shift bit fewer with 128x128 superblocks (untested) */
        uv = y - pp->loop_restoration_fields.bits.lr_uv_shift;
        /* as NVIDIA's driver: 3 (256) everywhere when no plane uses restoration, else the frame's sizes */
        pic->lr_unit_size = pic->lr_type ? (y | (uv << 2) | (uv << 4)) : 0x3f;
    }

    /* pictures */
    pic->current_frame.width = fw;
    pic->current_frame.height = fh;
    pic->current_frame.stride[0] = dec->pitch / 16;
    pic->current_frame.stride[1] = dec->pitch / 16;
    pic->current_frame.frame_buffer_idx = 0;        /* pictures are bound by role (decoder index 0..6 refs, 8 current); NVIDIA leaves these 0 */
    for (i = 0; i < AV1_REFS; i++)
    {
        ref_frame_struct_s *r = &pic->ref_frame[i];
        const struct av1_saved *s;

        if (refslot[i] < 0)
            continue;
        s = &v->saved[refslot[i]];
        r->info.width = s->upscaled_width;
        r->info.height = s->height;
        r->info.stride[0] = dec->pitch / 16;
        r->info.stride[1] = dec->pitch / 16;
        r->sf.x_scale_fp = (short)((((ULONG)s->upscaled_width << 14) + fw / 2) / fw);
        r->sf.y_scale_fp = (short)((((ULONG)s->height << 14) + fh / 2) / fh);
        if ((s->upscaled_width != fw) || (s->height != fh))
            pic->ref_scaling_enable = 1;
        r->sign_bias = av1_rd(pp, s->order_hint, pp->order_hint) > 0;
        r->wmtype = pp->wm[i].wmtype & 3;
        r->frame_off = av1_rd(pp, pp->order_hint, s->order_hint);     /* current minus reference (traced) */
        r->roffset = av1_rd(pp, s->order_hint, pp->order_hint);
    }

    /* skip mode frames (spec 5.9.22) */
    if (pp->mode_control_fields.bits.skip_mode_present && !intra)
    {
        LONG fwd = -1, bwd = -1, fwdhint = 0, bwdhint = 0, sec = -1, sechint = 0;

        for (i = 0; i < AV1_REFS; i++)
        {
            LONG hint;

            if (refslot[i] < 0)
                continue;
            hint = v->saved[refslot[i]].order_hint;
            if (av1_rd(pp, hint, pp->order_hint) < 0)
            {
                if ((fwd < 0) || (av1_rd(pp, hint, fwdhint) > 0))
                    fwd = i, fwdhint = hint;
            }
            else if (av1_rd(pp, hint, pp->order_hint) > 0)
            {
                if ((bwd < 0) || (av1_rd(pp, hint, bwdhint) < 0))
                    bwd = i, bwdhint = hint;
            }
        }
        if ((fwd >= 0) && (bwd < 0))
            for (i = 0; i < AV1_REFS; i++)
            {
                LONG hint;

                if (refslot[i] < 0)
                    continue;
                hint = v->saved[refslot[i]].order_hint;
                if ((av1_rd(pp, hint, fwdhint) < 0) && ((sec < 0) || (av1_rd(pp, hint, sechint) > 0)))
                    sec = i, sechint = hint;
            }
        if ((fwd >= 0) && ((bwd >= 0) || (sec >= 0)))
        {
            LONG other = (bwd >= 0) ? bwd : sec;

            pic->skip_mode_flag = 1;
            pic->skip_ref0 = 1 + ((fwd < other) ? fwd : other);
            pic->skip_ref1 = 1 + ((fwd < other) ? other : fwd);
        }
    }

    /* motion field sources (libaom av1_setup_motion_field) */
    mfsrc[0] = mfsrc[1] = mfsrc[2] = -1;
    if (pp->pic_info_fields.bits.use_ref_frame_mvs && pp->seq_info_fields.fields.enable_order_hint && !intra)
    {
        LONG stamp = 2;
        /* ref names: LAST 1, LAST2 2, LAST3 3, GOLDEN 4, BWDREF 5, ALTREF2 6, ALTREF 7 */
#define MF_VALID(R) ((refslot[(R) - 1] >= 0) && \
    (v->saved[refslot[(R) - 1]].frame_type != AV1_KEY_FRAME) && \
    (v->saved[refslot[(R) - 1]].frame_type != AV1_INTRA_ONLY_FRAME) && \
    (v->saved[refslot[(R) - 1]].mi_cols == micols) && (v->saved[refslot[(R) - 1]].mi_rows == mirows))
#define MF_ADD(R)   (mfsrc[nmf++] = (R))

        if (refslot[0] >= 0)
        {
            BOOL overlay = (refslot[3] >= 0) &&
                (v->saved[refslot[0]].ref_order_hint[6] == v->saved[refslot[3]].order_hint);

            if (!overlay && MF_VALID(1) && (nmf < 3))
                MF_ADD(1);
            stamp--;
        }
        for (k = 5; k <= 7; k++)
            if ((refslot[k - 1] >= 0) && (av1_rd(pp, v->saved[refslot[k - 1]].order_hint, pp->order_hint) > 0) &&
                ((k < 7) || (stamp >= 0)) && MF_VALID(k) && (nmf < 3))
            {
                MF_ADD(k);
                stamp--;
            }
        if ((stamp >= 0) && MF_VALID(2) && (nmf < 3))
            MF_ADD(2);
#undef MF_VALID
#undef MF_ADD
        for (i = 0; i < nmf; i++)
        {
            const struct av1_saved *s = &v->saved[refslot[mfsrc[i] - 1]];

            for (k = 0; k < AV1_REFS; k++)
            {
                LONG off = av1_rd(pp, s->order_hint, s->ref_order_hint[k]);

                pic->mfmv_offset[i][k] = off;
                pic->mfmv_side[i][k] = (off < 0) ? 1 : 0;      /* traced: set for negative offsets */
            }
        }
        pic->use_temporal0_mvs = nmf > 0;
        pic->use_temporal1_mvs = nmf > 1;
        pic->use_temporal2_mvs = nmf > 2;
        pic->mf1_type = (nmf > 0) ? mfsrc[0] : 0;
        pic->mf2_type = (nmf > 1) ? mfsrc[1] : 0;
        pic->mf3_type = (nmf > 2) ? mfsrc[2] : 0;
    }

    pic->tileformat = NVDEC_TILEFORMAT_KBL;
    pic->gob_height = NVDEC_GOB_HEIGHT_2;
    pic->Av1FltTopOffset = v->flt[0];
    pic->Av1FltVertOffset = v->flt[1];
    pic->Av1CdefVertOffset = v->flt[2];
    pic->Av1HusVertOffset = v->flt[3];
    pic->Av1LrVertOffset = v->flt[4];
    pic->Av1FgsVertOffset = v->flt[5];
    if (grain)
    {
        /* the driver generates the grain templates and scaling tables; the engine adds them (traced) */
        const VAFilmGrainStructAV1 *fg = &pp->film_grain_info;
        struct av1_grain_params gp;

        memset(&gp, 0, sizeof(gp));
        gp.seed = fg->grain_seed;
        gp.bit_depth = bitdepth;
        gp.num_y = fg->num_y_points;
        gp.num_cb = fg->num_cb_points;
        gp.num_cr = fg->num_cr_points;
        memcpy(gp.y_value, fg->point_y_value, 14);
        memcpy(gp.y_scaling, fg->point_y_scaling, 14);
        memcpy(gp.cb_value, fg->point_cb_value, 10);
        memcpy(gp.cb_scaling, fg->point_cb_scaling, 10);
        memcpy(gp.cr_value, fg->point_cr_value, 10);
        memcpy(gp.cr_scaling, fg->point_cr_scaling, 10);
        gp.chroma_scaling_from_luma = fg->film_grain_info_fields.bits.chroma_scaling_from_luma;
        gp.ar_coeff_lag = fg->film_grain_info_fields.bits.ar_coeff_lag;
        gp.ar_coeff_shift = fg->film_grain_info_fields.bits.ar_coeff_shift_minus_6 + 6;
        gp.grain_scale_shift = fg->film_grain_info_fields.bits.grain_scale_shift;
        memcpy(gp.ar_y, fg->ar_coeffs_y, 24);
        memcpy(gp.ar_cb, fg->ar_coeffs_cb, 25);
        memcpy(gp.ar_cr, fg->ar_coeffs_cr, 25);
        av1_grain_generate(&gp, (UBYTE *)v->gart->map + ring * AV1_RING_BYTES + AV1_CDF_BYTES);
        nouveau_staging_to_gpu((UBYTE *)v->gart->map + ring * AV1_RING_BYTES + AV1_CDF_BYTES, AV1_GRAIN_BYTES);

        pic->enable_fgs = 1;
        pic->fgs_cfg.apply_grain = 1;
        pic->fgs_cfg.overlap_flag = fg->film_grain_info_fields.bits.overlap_flag;
        pic->fgs_cfg.clip_to_restricted_range = fg->film_grain_info_fields.bits.clip_to_restricted_range;
        pic->fgs_cfg.chroma_scaling_from_luma = gp.chroma_scaling_from_luma;
        pic->fgs_cfg.num_y_points_b = gp.num_y > 0;
        pic->fgs_cfg.num_cb_points_b = gp.num_cb > 0;
        pic->fgs_cfg.num_cr_points_b = gp.num_cr > 0;
        pic->fgs_cfg.scaling_shift = fg->film_grain_info_fields.bits.grain_scaling_minus_8 + 8;
        pic->fgs_cfg.sw_random_seed = fg->grain_seed;
        /* the spec's de-biased values (traced) */
        pic->fgs_cfg.cb_offset = (short)(fg->cb_offset - 256);
        pic->fgs_cfg.cr_offset = (short)(fg->cr_offset - 256);
        pic->fgs_cfg.cb_mult = (char)(fg->cb_mult - 128);
        pic->fgs_cfg.cb_luma_mult = (char)(fg->cb_luma_mult - 128);
        pic->fgs_cfg.cr_mult = (char)(fg->cr_mult - 128);
        pic->fgs_cfg.cr_luma_mult = (char)(fg->cr_luma_mult - 128);
    }

    if (v->blkbuf)
    {
        /* Blackwell's additions, as NVIDIA's driver fills them for the CFB0 class (traced) */
        *(UWORD *)(base + 0x54) = 0xffff;
        pic->reserved_stat = (dec->bpp == 2) ? (2 | (3 << 4)) : 9;
        *(ULONG *)(base + 0x244) = NVDEC_BLK_UNITS(upw);
    }

    /* global motion per reference (traced): int32 wmmat[0..5], then int16 alpha, beta, gamma, delta */
    for (i = 0; i < AV1_REFS; i++)
    {
        UBYTE *g = base + AV1_GLOBAL_OFFSET + 32 * i;

        memcpy(g, pp->wm[i].wmmat, 24);
        av1_shear(pp->wm[i].wmmat, (WORD *)(g + 24));
    }

    /* the CDFs the frame starts from: the defaults for its q context, or the primary reference's adapted ones */
    if (defaults)
    {
        UWORD *t = (UWORD *)((UBYTE *)v->gart->map + ring * AV1_RING_BYTES);
        ULONG q = pp->base_qindex, qctx = (q <= 20) ? 0 : (q <= 60) ? 1 : (q <= 120) ? 2 : 3;

        memcpy(t, av1_default_cdfs, sizeof(av1_default_cdfs));
        memcpy(t + AV1_CDF_COEF_U16, av1_default_coef_cdfs[qctx], sizeof(av1_default_coef_cdfs[0]));
        nouveau_staging_to_gpu(t, AV1_CDF_BYTES);
    }

    nouveau_staging_to_gpu(base, NOUVEAU_DECODE_PICSETUP_SIZE);
    pad = HIDDNouveauDecodeBitstreamReserve(dec, 256);
    if (!pad)
        return FALSE;
    memset(pad, 0, 256);
    nouveau_staging_to_gpu(f->bitstream->map, dec->cur.bitstreamlen);

    {
        ULONG m[2 * 20], nm = 0;
        struct nouveau_bo *extra[8];
        ULONG nextra;
        uint64_t zero = v->zero;
        uint64_t mvw = v->mv->offset + (uint64_t)slot * v->mvsize;
#define M(mthd, addr)   do { m[2 * nm] = (mthd); m[2 * nm + 1] = (ULONG)((addr) >> 8); nm++; } while (0)

        /* NVIDIA's set and order (traced) */
        M(NVDEC_SET_COLOC_DATA_OFFSET, zero);
        M(NVDEC_SET_TILE_SIZE_BUF_OFFSET, f->picsetup->offset + AV1_TILES_OFFSET);
        M(NVDEC_AV1_SET_GLOBAL_MODEL, f->picsetup->offset + AV1_GLOBAL_OFFSET);
        M(NVDEC_AV1_SET_FILM_GRAIN, v->gart->offset + ring * AV1_RING_BYTES + AV1_CDF_BYTES);
        M(NVDEC_AV1_SET_SEGMENT_READ, pic->prevsegid_flag ? v->seg->offset + (uint64_t)prim * v->segsize : zero);
        M(NVDEC_AV1_SET_SEGMENT_WRITE, v->seg->offset + (uint64_t)slot * v->segsize);
        for (i = 0; i < 3; i++)
            M(NVDEC_AV1_SET_COL_MV0_READ + 4 * i,
              (i < nmf) ? v->mv->offset + (uint64_t)refslot[mfsrc[i] - 1] * v->mvsize : zero);
        M(NVDEC_AV1_SET_COL_MVWRITE, mvw);
        M(NVDEC_SET_FILTER_BUFFER_OFFSET, v->filter->offset);
        M(NVDEC_SET_INTRA_TOP_BUF_OFFSET, v->intratop->offset);
        M(NVDEC_AV1_SET_PROB_TAB_READ, defaults ? v->gart->offset + ring * AV1_RING_BYTES :
                                                  v->cdf->offset + (uint64_t)prim * AV1_CDF_BYTES);
        M(NVDEC_AV1_SET_PROB_TAB_WRITE, v->cdf->offset + (uint64_t)slot * AV1_CDF_BYTES);
        if (v->blkbuf)
            M(NVDEC_SET_BLK_BUF_OFFSET, v->blkbuf->offset);
        if (grain)
        {
            /* the grained picture goes to the display buffer: the target's own picture (traced) */
            M(0x0428, dec->slot[dslot].bo->offset);                         /* SET_DISPLAY_BUF_LUMA_OFFSET */
            M(0x042C, dec->slot[dslot].bo->offset + dec->chromaoff);        /* SET_DISPLAY_BUF_CHROMA_OFFSET */
        }
#undef M
        nextra = 0;
        extra[nextra++] = v->cdf;
        extra[nextra++] = v->seg;
        extra[nextra++] = v->mv;
        extra[nextra++] = v->gart;
        extra[nextra++] = v->filter;
        extra[nextra++] = v->intratop;
        if (v->blkbuf)
            extra[nextra++] = v->blkbuf;
        if (grain)
            extra[nextra++] = dec->slot[dslot].bo;
        LONG map[17];

        /* as NVIDIA's driver: decoder pictures 0..6 = LAST..ALTREF, 8 = the current picture */
        for (i = 0; i < 17; i++)
            map[i] = -1;
        for (i = 0; i < AV1_REFS; i++)
            map[i] = (refslot[i] >= 0) ? refslot[i] : slot;
        map[AV1_PIC_CUR] = slot;
        if (!HIDDNouveauDecodeEmit(dec, f, AV1_PIC_CUR, map, NVDEC_APPLICATION_ID_AV1,
                                   NVDEC_CONTROL_CODEC_AV1 | 0x52050, extra, nextra, m, nm))
            return FALSE;
    }
    if (dslot != slot)
    {
        /* the grained picture: registered for the display surface, as EndFrame does for the target */
        struct HIDDNouveauBitMapData *dbm = OOP_INST_DATA(dec->bmclass, rbm);

        dec->slot[dslot].valid = TRUE;
        dec->slot[dslot].bm = rbm;
        dec->slot[dslot].buffer = rbuffer;
        dec->slot[dslot].lastuse = dec->frames;
        if (rbuffer < dbm->nbufs)
        {
            dbm->buf[rbuffer].dec = dec;
            dbm->buf[rbuffer].decseq = f->seq;
            dbm->buf[rbuffer].decslot = dslot;
            dbm->buf[rbuffer].decstate = NOUVEAU_DECSTATE_DECODING;
        }
    }

    /* what later frames need of this one */
    cs = &v->saved[slot];
    cs->valid = TRUE;
    cs->frame_type = pp->pic_info_fields.bits.frame_type;
    cs->order_hint = pp->order_hint;
    for (i = 0; i < AV1_REFS; i++)
        cs->ref_order_hint[i] = (refslot[i] >= 0) ? v->saved[refslot[i]].order_hint : 0;
    cs->segmap = pp->seg_info.segment_info_fields.bits.enabled;
    cs->upscaled_width = upw;
    cs->height = fh;
    cs->mi_cols = micols;
    cs->mi_rows = mirows;
    D(bug("[Nouveau] NVDEC: AV1 frame %lu type %u slot %ld primary %ld mf %ld\n", (unsigned long)dec->frames,
        pp->pic_info_fields.bits.frame_type, (long)slot, (long)prim, (long)nmf));
    return TRUE;
}

const struct HIDDNouveauDecodeOps HIDDNouveauDecodeAV1Ops =
{
    av1_open, av1_close, av1_begin, av1_params, av1_bitstream, av1_submit, av1_status
};
