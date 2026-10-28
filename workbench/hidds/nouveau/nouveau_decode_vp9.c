/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: NVDEC VP9 decoding (profiles 0 and 2) for the gfx.hidd decode
          interface. The engine decodes the tiles; the driver parses both
          frame headers, keeps the four probability contexts, gives the
          engine each frame's probabilities and adapts them afterwards from
          the symbol counters the engine writes (nouveau_vp9prob.c).
          Programmed as NVIDIA's driver does (traced): the tile data as
          the bitstream (both headers are the driver's), decoder pictures LAST 0, GOLDEN 1, ALTREF 2,
          current 3 (VP9_FRAME_SFC_ID).
*/

#include "nouveau_intern.h"
#include "nouveau_decode.h"
#include "nouveau_vadec.h"
#include "nouveau_vp9prob.h"

#define DEBUG 0
#include <aros/debug.h>
#include <proto/exec.h>
#include <string.h>

#include <nvidia/video/nvdec_drv.h>

#define NVDEC_APPLICATION_ID_VP9            9
#define NVDEC_CONTROL_CODEC_VP9             9
#define NVDEC_SET_INTRA_TOP_BUF_OFFSET      0x04C8
#define NVDEC_VP9_SET_PROB_TAB_BUF_OFFSET   0x05C0
#define NVDEC_VP9_SET_CTX_COUNTER_BUF_OFFSET 0x05C4
#define NVDEC_VP9_SET_SEGMENT_READ_BUF_OFFSET 0x05C8
#define NVDEC_VP9_SET_SEGMENT_WRITE_BUF_OFFSET 0x05CC
#define NVDEC_VP9_SET_TILE_SIZE_BUF_OFFSET  0x05D0
#define NVDEC_VP9_SET_COL_MVWRITE_BUF_OFFSET 0x05D4
#define NVDEC_VP9_SET_COL_MVREAD_BUF_OFFSET 0x05D8
#define NVDEC_VP9_SET_FILTER_BUFFER_OFFSET  0x05DC

#define NVDEC_TILEFORMAT_KBL                1
#define NVDEC_GOB_HEIGHT_2                  0

/* in the frame's picsetup buffer (nouveau_decode.h): the setup at 0, then */
#define VP9_TILES_OFFSET                    0x0200      /* u16 {columns, rows} in superblocks per tile */
#define VP9_PROBS_OFFSET                    0x2100      /* after the status area */
#define VP9_COUNTS_STRIDE                   0x3400      /* per ring entry in the counter buffer */

struct vp9_dec
{
    VADecPictureParameterBufferVP9  pp;
    VASliceParameterBufferVP9       sp;
    BOOL                have_pp, have_sp, have_data;
    ULONG               framelen;           /* bytes of the frame in the bitstream buffer */
    struct vp9_state    ps;                 /* contexts and persistent header state */
    struct vp9_hdr      pendhdr;            /* the frame whose counters are still to be read */
    ULONG               pendseq, pendring;
    BOOL                pending;
    struct nouveau_bo   *segmap, *mv, *filter, *intratop, *counts;
    struct nouveau_bo   *blkbuf;            /* Blackwell (CFB0): the 0x4DC buffer */
    ULONG               segsize, mvsize;    /* one of the two areas */
    ULONG               segidx, mvidx;      /* the areas the next frame reads */
    BOOL                segused;            /* a frame had segmentation on: the maps hold data */
    ULONG               filtercols;         /* most tile columns the filter buffer is sized for */
    UWORD               slotw[NOUVEAU_DECODE_SLOTS], sloth[NOUVEAU_DECODE_SLOTS];
};

static BOOL vp9_zeroed(struct HIDDNouveauDecode *dec, ULONG flags, ULONG size, struct nouveau_bo **bo)
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

static BOOL vp9_open(struct HIDDNouveauDecode *dec)
{
    struct vp9_dec *v = AllocMem(sizeof(*v), MEMF_PUBLIC | MEMF_CLEAR);
    ULONG w64 = (dec->width + 63) / 64, h64 = (dec->height + 63) / 64, f, maxlog;

    if (!v)
        return FALSE;
    dec->cur.codecstate = v;
    dec->rowalign = 64;                             /* whole superblock rows, as NVIDIA's pictures */
    dec->minimal = TRUE;
    vp9_state_init(&v->ps);

    /* the most tile columns the width allows (at least 4 superblocks each) */
    for (maxlog = 1; (w64 >> maxlog) >= 4; maxlog++)
        ;
    v->filtercols = 1UL << (maxlog - 1);
    f = (v->filtercols > 1) ? v->filtercols - 1 : 1;

    v->segsize = (w64 * h64 * 32 + 255) & ~255UL;   /* 32 bytes per superblock */
    v->mvsize = (w64 * h64 * 2048 + 255) & ~255UL;
    /* filter buffer, traced: left / tile edges 3072 B and BSD control 256 B per superblock row and edge, then the above rows */
    if (!vp9_zeroed(dec, NOUVEAU_BO_VRAM, 2 * v->segsize, &v->segmap) ||
        !vp9_zeroed(dec, NOUVEAU_BO_VRAM, 2 * v->mvsize, &v->mv) ||
        !vp9_zeroed(dec, NOUVEAU_BO_VRAM, (3072 + 256) * h64 * f + 8192 * (w64 + 2), &v->filter) ||
        !vp9_zeroed(dec, NOUVEAU_BO_VRAM, (w64 * 2048 > 65536) ? w64 * 2048 : 65536, &v->intratop) ||
        !vp9_zeroed(dec, NOUVEAU_BO_GART, NOUVEAU_DECODE_RING * VP9_COUNTS_STRIDE, &v->counts))
        return FALSE;
    if ((dec->oclass >= NVDEC_CLASS_BLACKWELL) &&
        !vp9_zeroed(dec, NOUVEAU_BO_VRAM, NVDEC_BLK_BUF_SIZE(dec->width), &v->blkbuf))
        return FALSE;
    return TRUE;
}

static VOID vp9_close(struct HIDDNouveauDecode *dec)
{
    struct vp9_dec *v = dec->cur.codecstate;

    if (!v)
        return;
    if (v->segmap)   nouveau_bo_ref(NULL, &v->segmap);
    if (v->mv)       nouveau_bo_ref(NULL, &v->mv);
    if (v->filter)   nouveau_bo_ref(NULL, &v->filter);
    if (v->intratop) nouveau_bo_ref(NULL, &v->intratop);
    if (v->counts)   nouveau_bo_ref(NULL, &v->counts);
    if (v->blkbuf)   nouveau_bo_ref(NULL, &v->blkbuf);
    FreeMem(v, sizeof(*v));
    dec->cur.codecstate = NULL;
}

static BOOL vp9_begin(struct HIDDNouveauDecode *dec)
{
    struct vp9_dec *v = dec->cur.codecstate;

    if (!v)
        return FALSE;
    v->have_pp = v->have_sp = v->have_data = FALSE;
    return TRUE;
}

static BOOL vp9_params(struct HIDDNouveauDecode *dec, ULONG type, APTR data, ULONG size, ULONG count)
{
    struct vp9_dec *v = dec->cur.codecstate;

    switch (type)
    {
    case VAPictureParameterBufferType:
        if (size < sizeof(VADecPictureParameterBufferVP9))
            return FALSE;
        memcpy(&v->pp, data, sizeof(v->pp));
        v->have_pp = TRUE;
        return TRUE;

    case VASliceParameterBufferType:
        if (size < sizeof(VASliceParameterBufferVP9))
            return FALSE;
        memcpy(&v->sp, data, sizeof(v->sp));
        v->have_sp = TRUE;
        return TRUE;

    default:
        return TRUE;
    }
}

/* the whole frame, headers included, as NVIDIA's driver passes it */
static BOOL vp9_bitstream(struct HIDDNouveauDecode *dec, APTR data, ULONG size)
{
    struct vp9_dec *v = dec->cur.codecstate;
    ULONG off = 0, len = size;
    UBYTE *p;

    if (!v->have_pp || v->have_data)
        return FALSE;
    if (v->have_sp && v->sp.slice_data_size && (v->sp.slice_data_offset <= size) &&
        (v->sp.slice_data_size <= size - v->sp.slice_data_offset))
    {
        off = v->sp.slice_data_offset;
        len = v->sp.slice_data_size;
    }
    p = HIDDNouveauDecodeBitstreamReserve(dec, len);
    if (!p)
        return FALSE;
    memcpy(p, (UBYTE *)data + off, len);
    v->framelen = len;
    v->have_data = TRUE;
    dec->cur.nslices = 1;
    return TRUE;
}

static ULONG vp9_status(struct HIDDNouveauDecode *dec, ULONG *status)
{
    nvdec_status_s *st = (nvdec_status_s *)status;

    if (!st->mbs_in_error && !st->slice_header_error_code && (!st->error_status || (st->error_status == 0x20)))
        return 0;
    if (dec->errors++ < 4)
        nvlog("[Nouveau] NVDEC: VP9 frame error %08lx (ok %lu, in error %lu, header %lu)\n",
            (unsigned long)st->error_status, (unsigned long)st->mbs_correctly_decoded,
            (unsigned long)st->mbs_in_error, (unsigned long)st->slice_header_error_code);
    return st->error_status ? st->error_status : 1;
}

/* the previous frame's backward adaptation: its counters, once it is done */
static BOOL vp9_settle_pending(struct HIDDNouveauDecode *dec, struct vp9_dec *v)
{
    const uint32_t *counts;

    if (!v->pending)
        return TRUE;
    v->pending = FALSE;
    if (!HIDDNouveauDecodeWait(dec, v->pendseq))
        return FALSE;
    counts = (const uint32_t *)((UBYTE *)v->counts->map + v->pendring * VP9_COUNTS_STRIDE);
    nouveau_staging_from_gpu((APTR)counts, VP9_NVDEC_COUNTS_SIZE);
    vp9_finish_frame(&v->ps, &v->pendhdr, counts);
    return TRUE;
}

/* tile sizes in superblocks (VP9 spec get_tile_offset), rows outer */
static VOID vp9_tiles(const struct vp9_hdr *h, ULONG w64, ULONG h64, UWORD *t)
{
    ULONG r, c, cols = 1UL << h->log2_tile_cols, rows = 1UL << h->log2_tile_rows;

    for (r = 0; r < rows; r++)
        for (c = 0; c < cols; c++)
        {
            *t++ = (UWORD)((((c + 1) * w64) >> h->log2_tile_cols) - ((c * w64) >> h->log2_tile_cols));
            *t++ = (UWORD)((((r + 1) * h64) >> h->log2_tile_rows) - ((r * h64) >> h->log2_tile_rows));
        }
}

static LONG vp9_refslot(struct HIDDNouveauDecode *dec, VASurfaceID surface)
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

static BOOL vp9_submit(struct HIDDNouveauDecode *dec, struct HIDDNouveauDecodeFrame *f)
{
    struct vp9_dec *v = dec->cur.codecstate;
    const VADecPictureParameterBufferVP9 *pp;
    struct vp9_hdr hd, *h = &hd;
    nvdec_vp9_pic_s *pic;
    UBYTE *base, *frame, *pad;
    ULONG avoid = 0, i, w64, h64, f1, ring;
    LONG refs[3], all[8], slot, map[17];
    BOOL intra;

    if (!v || !v->have_pp || !v->have_data)
        return FALSE;
    pp = &v->pp;
    if (!vp9_settle_pending(dec, v))
        return FALSE;

    /* the headers, from the frame as copied */
    frame = (UBYTE *)f->bitstream->map;
    if (vp9_parse_uncompressed(&v->ps, frame, v->framelen, h, pp->frame_width, pp->frame_height) ||
        h->show_existing ||
        vp9_parse_compressed(&v->ps, frame + h->uh_bytes, h->ch_bytes, h))
    {
        nvlog("[Nouveau] NVDEC: VP9 frame %lu: bad frame header\n", (unsigned long)dec->frames);
        return FALSE;
    }
    if ((h->width > (int)dec->width) || (h->height > (int)dec->height) || ((h->bit_depth > 8) != (dec->bpp == 2)) ||
        (h->bit_depth > 10) || (h->profile & 1))
    {
        nvlog("[Nouveau] NVDEC: VP9 stream (%dx%d, profile %d, %d-bit) does not fit the session\n",
            h->width, h->height, h->profile, h->bit_depth);
        return FALSE;
    }
    intra = h->key || h->intra_only;
    /* the engine gets the tile data only (traced); stream_len stays the whole frame's size, as NVIDIA's driver sets it */
    memmove(frame, frame + h->uh_bytes + h->ch_bytes, v->framelen - h->uh_bytes - h->ch_bytes);
    dec->cur.bitstreamlen -= h->uh_bytes + h->ch_bytes;

    /* every picture VP9 may still reference stays in the pool */
    for (i = 0; i < 8; i++)
    {
        all[i] = vp9_refslot(dec, pp->reference_frames[i]);
        if (all[i] >= 0)
            avoid |= 1UL << all[i];
    }
    refs[0] = refs[1] = refs[2] = -1;
    if (!intra)
    {
        refs[0] = all[pp->pic_fields.bits.last_ref_frame];
        refs[1] = all[pp->pic_fields.bits.golden_ref_frame];
        refs[2] = all[pp->pic_fields.bits.alt_ref_frame];
    }
    slot = HIDDNouveauDecodeAllocSlot(dec, dec->cur.bm, dec->cur.buffer, avoid);
    if (slot < 0)
        return FALSE;
    dec->cur.slot = slot;
    v->slotw[slot] = (UWORD)h->width;
    v->sloth[slot] = (UWORD)h->height;
    for (i = 0; i < 17; i++)
        map[i] = -1;
    for (i = 0; i < 3; i++)
        map[VP9_LAST_FRAME_SFC + i] = (refs[i] >= 0) ? refs[i] : slot;
    map[VP9_CURR_FRAME_SFC] = slot;

    /* segmentation maps start over with a key frame, intra-only or error-resilient frame, or a new size */
    if (v->segused && (intra || h->error_res || (v->ps.have_last &&
        ((h->width != v->ps.last_width) || (h->height != v->ps.last_height)))))
    {
        if ((f->seq > 1) && !HIDDNouveauDecodeWait(dec, f->seq - 1))
            return FALSE;
        memset(v->segmap->map, 0, 2 * v->segsize);
        v->segused = FALSE;
    }
    if (h->seg_enabled)
        v->segused = TRUE;

    w64 = (h->width + 63) / 64;
    h64 = (h->height + 63) / 64;
    f1 = (h->log2_tile_cols > 1) ? (1UL << h->log2_tile_cols) - 1 : 1;
    if (f1 > ((v->filtercols > 1) ? v->filtercols - 1 : 1))
        return FALSE;

    base = (UBYTE *)f->picsetup->map;
    pic = (nvdec_vp9_pic_s *)base;
    memset(base, 0, NOUVEAU_DECODE_SLICEOFFS_OFFSET);
    memset(base + NOUVEAU_DECODE_STATUS_OFFSET, 0, 256);

    pic->stream_len = v->framelen;
    pic->gptimer_timeout_value = 0x4d3f640;         /* NVIDIA's value, at every size */
    pic->tileformat = NVDEC_TILEFORMAT_KBL;
    pic->gob_height = NVDEC_GOB_HEIGHT_2;
    pic->Vp9BsdCtrlOffset = 12 * h64 * f1;          /* 256-byte units, after the left / tile-edge area */
    pic->v1.Vp9FltAboveOffset = 13 * h64 * f1;
    pic->v1.bit_depth = h->bit_depth;
    if (!intra)
    {
        pic->ref0_width = v->slotw[map[0]];  pic->ref0_height = v->sloth[map[0]];
        pic->ref1_width = v->slotw[map[1]];  pic->ref1_height = v->sloth[map[1]];
        pic->ref2_width = v->slotw[map[2]];  pic->ref2_height = v->sloth[map[2]];
        pic->ref0_stride[0] = pic->ref0_stride[1] = dec->pitch / dec->bpp;
        pic->ref1_stride[0] = pic->ref1_stride[1] = dec->pitch / dec->bpp;
        pic->ref2_stride[0] = pic->ref2_stride[1] = dec->pitch / dec->bpp;
        pic->refFrameSignBias[1] = h->sign_bias[0];
        pic->refFrameSignBias[2] = h->sign_bias[1];
        pic->refFrameSignBias[3] = h->sign_bias[2];
    }
    pic->width = h->width;
    pic->height = h->height;
    pic->framestride[0] = dec->pitch / dec->bpp;    /* in samples */
    pic->framestride[1] = dec->pitch / dec->bpp;
    pic->keyFrame = h->key;
    pic->prevIsKeyFrame = v->ps.have_last ? v->ps.last_key : 0;
    pic->resolutionChange = v->ps.have_last && ((h->width != v->ps.last_width) || (h->height != v->ps.last_height));
    pic->errorResilient = h->error_res;
    pic->prevShowFrame = v->ps.have_last ? v->ps.last_show : 0;
    pic->intraOnly = h->intra_only;
    pic->loopFilterLevel = h->lf_level;
    pic->loopFilterSharpness = h->lf_sharpness;
    pic->qpYAc = h->base_q;
    pic->qpYDc = h->dq_y_dc;
    pic->qpChAc = h->dq_uv_ac;
    pic->qpChDc = h->dq_uv_dc;
    pic->lossless = h->lossless;
    pic->transform_mode = h->tx_mode;
    pic->allow_high_precision_mv = h->hp;
    {
        /* the bitstream's order (0 smooth, 1 regular, 2 sharp, 3 bilinear, 4 switchable), regular on key frames */
        static const UBYTE to_literal[5] = { 1, 0, 2, 3, 4 };

        pic->mcomp_filter_type = intra ? 1 : to_literal[h->interp];
    }
    pic->comp_pred_mode = h->reference_mode;
    pic->comp_fixed_ref = h->comp_fixed_ref;
    pic->comp_var_ref[0] = h->comp_var_ref[0];
    pic->comp_var_ref[1] = h->comp_var_ref[1];
    pic->log2_tile_columns = h->log2_tile_cols;
    pic->log2_tile_rows = h->log2_tile_rows;
    pic->segmentEnabled = h->seg_enabled;
    pic->segmentMapUpdate = h->seg_update_map;
    pic->segmentMapTemporalUpdate = h->seg_temporal;
    pic->segmentFeatureMode = v->ps.seg_abs;
    memcpy(pic->segmentFeatureEnable, v->ps.seg_feature_enabled, sizeof(pic->segmentFeatureEnable));
    memcpy(pic->segmentFeatureData, v->ps.seg_feature_data, sizeof(pic->segmentFeatureData));
    pic->modeRefLfEnabled = v->ps.lf_delta_enabled;
    memcpy(pic->mbRefLfDelta, v->ps.lf_ref_deltas, 4);
    memcpy(pic->mbModeLfDelta, v->ps.lf_mode_deltas, 2);
    if (v->blkbuf)
    {
        /* Blackwell's additions, as NVIDIA's driver fills them for the CFB0 class (traced) */
        pic->reserved5 = (dec->bpp == 2) ? 2 : 9;
        pic->reserved1[2] = (dec->bpp == 2) ? 3 : 0;
        *(ULONG *)(base + 0x108) = NVDEC_BLK_UNITS(h->width);
    }

    vp9_tiles(h, w64, h64, (UWORD *)(base + VP9_TILES_OFFSET));
    vp9_fill_nvdec_probs(&v->ps, h, base + VP9_PROBS_OFFSET);
    nouveau_staging_to_gpu(base, NOUVEAU_DECODE_PICSETUP_SIZE);

    /* this frame's counters start from zero (the ring entry is idle) */
    ring = (ULONG)(f - dec->ring);
    memset((UBYTE *)v->counts->map + ring * VP9_COUNTS_STRIDE, 0, VP9_COUNTS_STRIDE);
    nouveau_staging_to_gpu((UBYTE *)v->counts->map + ring * VP9_COUNTS_STRIDE, VP9_COUNTS_STRIDE);

    pad = HIDDNouveauDecodeBitstreamReserve(dec, 256);
    if (!pad)
        return FALSE;
    memset(pad, 0, 256);
    nouveau_staging_to_gpu(f->bitstream->map, dec->cur.bitstreamlen);

    {
        ULONG m[2 * 10], nm = 9;
        struct nouveau_bo *extra[6];

        m[0] = NVDEC_VP9_SET_TILE_SIZE_BUF_OFFSET;      m[1] = (ULONG)((f->picsetup->offset + VP9_TILES_OFFSET) >> 8);
        m[2] = NVDEC_VP9_SET_PROB_TAB_BUF_OFFSET;       m[3] = (ULONG)((f->picsetup->offset + VP9_PROBS_OFFSET) >> 8);
        m[4] = NVDEC_VP9_SET_CTX_COUNTER_BUF_OFFSET;    m[5] = (ULONG)((v->counts->offset + ring * VP9_COUNTS_STRIDE) >> 8);
        m[6] = NVDEC_VP9_SET_SEGMENT_READ_BUF_OFFSET;   m[7] = (ULONG)((v->segmap->offset + v->segidx * v->segsize) >> 8);
        m[8] = NVDEC_VP9_SET_SEGMENT_WRITE_BUF_OFFSET;  m[9] = (ULONG)((v->segmap->offset + (1 - v->segidx) * v->segsize) >> 8);
        m[10] = NVDEC_VP9_SET_COL_MVREAD_BUF_OFFSET;    m[11] = (ULONG)((v->mv->offset + v->mvidx * v->mvsize) >> 8);
        m[12] = NVDEC_VP9_SET_COL_MVWRITE_BUF_OFFSET;   m[13] = (ULONG)((v->mv->offset + (1 - v->mvidx) * v->mvsize) >> 8);
        m[14] = NVDEC_VP9_SET_FILTER_BUFFER_OFFSET;     m[15] = (ULONG)(v->filter->offset >> 8);
        m[16] = NVDEC_SET_INTRA_TOP_BUF_OFFSET;         m[17] = (ULONG)(v->intratop->offset >> 8);
        if (v->blkbuf)
        {
            m[2 * nm] = NVDEC_SET_BLK_BUF_OFFSET;
            m[2 * nm + 1] = (ULONG)(v->blkbuf->offset >> 8);
            nm++;
        }
        extra[0] = v->segmap;
        extra[1] = v->mv;
        extra[2] = v->filter;
        extra[3] = v->intratop;
        extra[4] = v->counts;
        extra[5] = v->blkbuf;
        /* NVIDIA's control word (gptimer, error concealment, mbtimer, event trace, PROD_RUN) */
        if (!HIDDNouveauDecodeEmit(dec, f, VP9_CURR_FRAME_SFC, map, NVDEC_APPLICATION_ID_VP9,
                                   NVDEC_CONTROL_CODEC_VP9 | 0x52050, extra, v->blkbuf ? 6 : 5, m, nm))
            return FALSE;
    }

    /* motion vectors alternate every frame; the segment map when the frame wrote a new one */
    v->mvidx ^= 1;
    if (h->seg_enabled && h->seg_update_map)
        v->segidx ^= 1;
    if (vp9_needs_counts(h))
    {
        v->pending = TRUE;
        v->pendseq = f->seq;
        v->pendring = ring;
        v->pendhdr = *h;
    }
    else
        vp9_finish_frame(&v->ps, h, NULL);
    D(bug("[Nouveau] NVDEC: VP9 frame %lu %s%s slot %ld refs %ld %ld %ld ctx %d\n", (unsigned long)dec->frames,
        h->key ? "key" : intra ? "intra" : "inter", h->show_frame ? "" : " hidden", (long)slot,
        (long)refs[0], (long)refs[1], (long)refs[2], h->ctx_idx));
    return TRUE;
}

const struct HIDDNouveauDecodeOps HIDDNouveauDecodeVP9Ops =
{
    vp9_open, vp9_close, vp9_begin, vp9_params, vp9_bitstream, vp9_submit, vp9_status
};
