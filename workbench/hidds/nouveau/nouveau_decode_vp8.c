/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: NVDEC VP8 decoding for the gfx.hidd decode interface. The engine
          parses the bool-coded frame header and keeps the entropy state
          itself; the driver passes the frame from partition 1 on (as
          NVIDIA's driver does, traced) and the frame tag fields.
          Decoder pictures: golden 0, altref 1, last 2, current 3
          (VP8_FRAME_SFC_ID).
*/

#include "nouveau_intern.h"
#include "nouveau_decode.h"
#include "nouveau_vadec.h"

#define DEBUG 0
#include <aros/debug.h>
#include <proto/exec.h>
#include <string.h>

#include <nvidia/video/nvdec_drv.h>

#define NVDEC_APPLICATION_ID_VP8            5
#define NVDEC_CONTROL_CODEC_VP8             5
#define NVDEC_CONTROL_ALL_INTRA_FRAME       (1 << 17)
#define NVDEC_VP8_SET_PROB_DATA_OFFSET      0x0540

#define NVDEC_TILEFORMAT_KBL                1
#define NVDEC_GOB_HEIGHT_2                  0

#define VP8_HISTORY_SIZE                    (2 * 1024 * 1024)
#define VP8_PROB_SIZE                       (40 * 1024)     /* NVIDIA's size */

/* VP8 default probabilities (RFC 6386 13.5, 16.2, 17.2) in the NVDEC probability buffer layout */
static const UBYTE vp8_default_coef_probs[4][8][3][11] =
{
    {
        { { 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128 }, { 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128 }, { 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128 } },
        { { 253, 136, 254, 255, 228, 219, 128, 128, 128, 128, 128 }, { 189, 129, 242, 255, 227, 213, 255, 219, 128, 128, 128 }, { 106, 126, 227, 252, 214, 209, 255, 255, 128, 128, 128 } },
        { { 1, 98, 248, 255, 236, 226, 255, 255, 128, 128, 128 }, { 181, 133, 238, 254, 221, 234, 255, 154, 128, 128, 128 }, { 78, 134, 202, 247, 198, 180, 255, 219, 128, 128, 128 } },
        { { 1, 185, 249, 255, 243, 255, 128, 128, 128, 128, 128 }, { 184, 150, 247, 255, 236, 224, 128, 128, 128, 128, 128 }, { 77, 110, 216, 255, 236, 230, 128, 128, 128, 128, 128 } },
        { { 1, 101, 251, 255, 241, 255, 128, 128, 128, 128, 128 }, { 170, 139, 241, 252, 236, 209, 255, 255, 128, 128, 128 }, { 37, 116, 196, 243, 228, 255, 255, 255, 128, 128, 128 } },
        { { 1, 204, 254, 255, 245, 255, 128, 128, 128, 128, 128 }, { 207, 160, 250, 255, 238, 128, 128, 128, 128, 128, 128 }, { 102, 103, 231, 255, 211, 171, 128, 128, 128, 128, 128 } },
        { { 1, 152, 252, 255, 240, 255, 128, 128, 128, 128, 128 }, { 177, 135, 243, 255, 234, 225, 128, 128, 128, 128, 128 }, { 80, 129, 211, 255, 194, 224, 128, 128, 128, 128, 128 } },
        { { 1, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128 }, { 246, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128 }, { 255, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128 } },
    },
    {
        { { 198, 35, 237, 223, 193, 187, 162, 160, 145, 155, 62 }, { 131, 45, 198, 221, 172, 176, 220, 157, 252, 221, 1 }, { 68, 47, 146, 208, 149, 167, 221, 162, 255, 223, 128 } },
        { { 1, 149, 241, 255, 221, 224, 255, 255, 128, 128, 128 }, { 184, 141, 234, 253, 222, 220, 255, 199, 128, 128, 128 }, { 81, 99, 181, 242, 176, 190, 249, 202, 255, 255, 128 } },
        { { 1, 129, 232, 253, 214, 197, 242, 196, 255, 255, 128 }, { 99, 121, 210, 250, 201, 198, 255, 202, 128, 128, 128 }, { 23, 91, 163, 242, 170, 187, 247, 210, 255, 255, 128 } },
        { { 1, 200, 246, 255, 234, 255, 128, 128, 128, 128, 128 }, { 109, 178, 241, 255, 231, 245, 255, 255, 128, 128, 128 }, { 44, 130, 201, 253, 205, 192, 255, 255, 128, 128, 128 } },
        { { 1, 132, 239, 251, 219, 209, 255, 165, 128, 128, 128 }, { 94, 136, 225, 251, 218, 190, 255, 255, 128, 128, 128 }, { 22, 100, 174, 245, 186, 161, 255, 199, 128, 128, 128 } },
        { { 1, 182, 249, 255, 232, 235, 128, 128, 128, 128, 128 }, { 124, 143, 241, 255, 227, 234, 128, 128, 128, 128, 128 }, { 35, 77, 181, 251, 193, 211, 255, 205, 128, 128, 128 } },
        { { 1, 157, 247, 255, 236, 231, 255, 255, 128, 128, 128 }, { 121, 141, 235, 255, 225, 227, 255, 255, 128, 128, 128 }, { 45, 99, 188, 251, 195, 217, 255, 224, 128, 128, 128 } },
        { { 1, 1, 251, 255, 213, 255, 128, 128, 128, 128, 128 }, { 203, 1, 248, 255, 255, 128, 128, 128, 128, 128, 128 }, { 137, 1, 177, 255, 224, 255, 128, 128, 128, 128, 128 } },
    },
    {
        { { 253, 9, 248, 251, 207, 208, 255, 192, 128, 128, 128 }, { 175, 13, 224, 243, 193, 185, 249, 198, 255, 255, 128 }, { 73, 17, 171, 221, 161, 179, 236, 167, 255, 234, 128 } },
        { { 1, 95, 247, 253, 212, 183, 255, 255, 128, 128, 128 }, { 239, 90, 244, 250, 211, 209, 255, 255, 128, 128, 128 }, { 155, 77, 195, 248, 188, 195, 255, 255, 128, 128, 128 } },
        { { 1, 24, 239, 251, 218, 219, 255, 205, 128, 128, 128 }, { 201, 51, 219, 255, 196, 186, 128, 128, 128, 128, 128 }, { 69, 46, 190, 239, 201, 218, 255, 228, 128, 128, 128 } },
        { { 1, 191, 251, 255, 255, 128, 128, 128, 128, 128, 128 }, { 223, 165, 249, 255, 213, 255, 128, 128, 128, 128, 128 }, { 141, 124, 248, 255, 255, 128, 128, 128, 128, 128, 128 } },
        { { 1, 16, 248, 255, 255, 128, 128, 128, 128, 128, 128 }, { 190, 36, 230, 255, 236, 255, 128, 128, 128, 128, 128 }, { 149, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128 } },
        { { 1, 226, 255, 128, 128, 128, 128, 128, 128, 128, 128 }, { 247, 192, 255, 128, 128, 128, 128, 128, 128, 128, 128 }, { 240, 128, 255, 128, 128, 128, 128, 128, 128, 128, 128 } },
        { { 1, 134, 252, 255, 255, 128, 128, 128, 128, 128, 128 }, { 213, 62, 250, 255, 255, 128, 128, 128, 128, 128, 128 }, { 55, 93, 255, 128, 128, 128, 128, 128, 128, 128, 128 } },
        { { 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128 }, { 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128 }, { 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128 } },
    },
    {
        { { 202, 24, 213, 235, 186, 191, 220, 160, 240, 175, 255 }, { 126, 38, 182, 232, 169, 184, 228, 174, 255, 187, 128 }, { 61, 46, 138, 219, 151, 178, 240, 170, 255, 216, 128 } },
        { { 1, 112, 230, 250, 199, 191, 247, 159, 255, 255, 128 }, { 166, 109, 228, 252, 211, 215, 255, 174, 128, 128, 128 }, { 39, 77, 162, 232, 172, 180, 245, 178, 255, 255, 128 } },
        { { 1, 52, 220, 246, 198, 199, 249, 220, 255, 255, 128 }, { 124, 74, 191, 243, 183, 193, 250, 221, 255, 255, 128 }, { 24, 71, 130, 219, 154, 170, 243, 182, 255, 255, 128 } },
        { { 1, 182, 225, 249, 219, 240, 255, 224, 128, 128, 128 }, { 149, 150, 226, 252, 216, 205, 255, 171, 128, 128, 128 }, { 28, 108, 170, 242, 183, 194, 254, 223, 255, 255, 128 } },
        { { 1, 81, 230, 252, 204, 203, 255, 192, 128, 128, 128 }, { 123, 102, 209, 247, 188, 196, 255, 233, 128, 128, 128 }, { 20, 95, 153, 243, 164, 173, 255, 203, 128, 128, 128 } },
        { { 1, 222, 248, 255, 216, 213, 128, 128, 128, 128, 128 }, { 168, 175, 246, 252, 235, 205, 255, 255, 128, 128, 128 }, { 47, 116, 215, 255, 211, 212, 255, 255, 128, 128, 128 } },
        { { 1, 121, 236, 253, 212, 214, 255, 255, 128, 128, 128 }, { 141, 84, 213, 252, 201, 202, 255, 219, 128, 128, 128 }, { 42, 80, 160, 240, 162, 185, 255, 205, 128, 128, 128 } },
        { { 1, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128 }, { 244, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128 }, { 238, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128 } },
    },
};
static const UBYTE vp8_default_ymode_probs[4] = { 112, 86, 140, 37 };
static const UBYTE vp8_default_uvmode_probs[3] = { 162, 101, 204 };
static const UBYTE vp8_default_mv_probs[2][19] =
{
    { 162, 128, 225, 146, 172, 147, 214, 39, 156, 128, 129, 132, 75, 145, 178, 206, 239, 254, 254 },
    { 164, 128, 204, 170, 119, 235, 140, 230, 228, 128, 130, 130, 74, 148, 180, 203, 236, 254, 254 }
};

/*
 * The probability buffer as NVIDIA's driver fills it once, at decoder
 * creation (traced): coefficient probabilities [4][8][3] as 11 bytes + 1
 * pad, Y-mode (4), UV-mode (3) + 1 pad, MV contexts [2] as 19 + 1 pad. The
 * engine keeps it up to date from then on.
 */
static VOID vp8_prob_init(UBYTE *p)
{
    ULONG i, j, k;

    memset(p, 0, VP8_PROB_SIZE);
    for (i = 0; i < 4; i++)
        for (j = 0; j < 8; j++)
            for (k = 0; k < 3; k++)
                memcpy(p + ((i * 8 + j) * 3 + k) * 12, vp8_default_coef_probs[i][j][k], 11);
    memcpy(p + 0x480, vp8_default_ymode_probs, 4);
    memcpy(p + 0x484, vp8_default_uvmode_probs, 3);
    memcpy(p + 0x488, vp8_default_mv_probs[0], 19);
    memcpy(p + 0x49c, vp8_default_mv_probs[1], 19);
}

struct vp8_state
{
    VAPictureParameterBufferVP8     pp;
    VASliceParameterBufferVP8       sp;
    BOOL                have_pp, have_sp, have_data;
    struct nouveau_bo   *prob;
};

static BOOL vp8_zeroed(struct HIDDNouveauDecode *dec, ULONG size, struct nouveau_bo **bo)
{
    if (nouveau_bo_new(dec->carddata->dev, NOUVEAU_BO_VRAM | NOUVEAU_BO_MAP, 4096, size, NULL, bo) ||
        nouveau_bo_map(*bo, NOUVEAU_BO_WR, dec->carddata->client))
        return FALSE;
    memset((*bo)->map, 0, size);
    return TRUE;
}

static BOOL vp8_open(struct HIDDNouveauDecode *dec)
{
    struct vp8_state *v = AllocMem(sizeof(*v), MEMF_PUBLIC | MEMF_CLEAR);

    if (!v)
        return FALSE;
    dec->cur.codecstate = v;
    /* the entropy state lives in the probability buffer from frame to frame */
    if (!vp8_zeroed(dec, VP8_HISTORY_SIZE, &dec->history) ||
        !vp8_zeroed(dec, VP8_PROB_SIZE, &v->prob))
        return FALSE;
    vp8_prob_init(v->prob->map);
    return TRUE;
}

static VOID vp8_close(struct HIDDNouveauDecode *dec)
{
    struct vp8_state *v = dec->cur.codecstate;

    if (!v)
        return;
    if (v->prob)
        nouveau_bo_ref(NULL, &v->prob);
    FreeMem(v, sizeof(*v));
    dec->cur.codecstate = NULL;
}

static BOOL vp8_begin(struct HIDDNouveauDecode *dec)
{
    struct vp8_state *v = dec->cur.codecstate;

    if (!v)
        return FALSE;
    v->have_pp = v->have_sp = v->have_data = FALSE;
    return TRUE;
}

static BOOL vp8_params(struct HIDDNouveauDecode *dec, ULONG type, APTR data, ULONG size, ULONG count)
{
    struct vp8_state *v = dec->cur.codecstate;

    switch (type)
    {
    case VAPictureParameterBufferType:
        if (size < sizeof(VAPictureParameterBufferVP8))
            return FALSE;
        memcpy(&v->pp, data, sizeof(v->pp));
        v->have_pp = TRUE;
        return TRUE;

    case VASliceParameterBufferType:
        if (size < sizeof(VASliceParameterBufferVP8))
            return FALSE;
        memcpy(&v->sp, data, sizeof(v->sp));
        v->have_sp = TRUE;
        return TRUE;

    default:
        /* probabilities and quantiser indices: the engine reads them from the frame header */
        return TRUE;
    }
}

/*
 * The frame from partition 1 on, as VA delivers it and as NVIDIA's driver
 * passes it (traced): the uncompressed chunk is not in the bitstream; its
 * fields are in the picture setup.
 */
static BOOL vp8_bitstream(struct HIDDNouveauDecode *dec, APTR data, ULONG size)
{
    struct vp8_state *v = dec->cur.codecstate;
    const VASliceParameterBufferVP8 *sp = &v->sp;
    ULONG off = 0, len = size;
    UBYTE *p;

    if (!v->have_pp || !v->have_sp || v->have_data)
        return FALSE;
    if (sp->slice_data_size && (sp->slice_data_offset <= size) && (sp->slice_data_size <= size - sp->slice_data_offset))
    {
        off = sp->slice_data_offset;
        len = sp->slice_data_size;
    }
    p = HIDDNouveauDecodeBitstreamReserve(dec, len);
    if (!p)
        return FALSE;
    memcpy(p, (UBYTE *)data + off, len);
    v->have_data = TRUE;
    dec->cur.nslices = 1;
    return TRUE;
}

static ULONG vp8_status(struct HIDDNouveauDecode *dec, ULONG *status)
{
    nvdec_status_s *st = (nvdec_status_s *)status;
    ULONG mbs = ((dec->width + 15) / 16) * ((dec->height + 15) / 16);
    struct vp8_state *v = dec->cur.codecstate;

    if (!st->mbs_in_error && !st->slice_header_error_code && (st->mbs_correctly_decoded >= mbs) &&
        (!st->error_status || (st->error_status == 0x20)))
        return 0;
    if (dec->errors++ < 4)
        nvlog("[Nouveau] NVDEC: VP8 frame error %08lx (ok %lu of %lu, in error %lu, header %lu)\n",
            (unsigned long)st->error_status, (unsigned long)st->mbs_correctly_decoded, (unsigned long)mbs,
            (unsigned long)st->mbs_in_error, (unsigned long)st->slice_header_error_code);
    return st->error_status ? st->error_status : 1;
}

static LONG vp8_refslot(struct HIDDNouveauDecode *dec, VASurfaceID surface)
{
    OOP_Object *bm;
    ULONG buffer;
    LONG slot;

    if (!HIDDNouveauDecodeResolve(dec, surface, &bm, &buffer))
        return -1;
    slot = HIDDNouveauDecodeFindSlot(dec, bm, buffer);
    if (slot >= 0)
        dec->slot[slot].lastuse = dec->frames;
    return slot;
}

static BOOL vp8_submit(struct HIDDNouveauDecode *dec, struct HIDDNouveauDecodeFrame *f)
{
    struct vp8_state *v = dec->cur.codecstate;
    const VAPictureParameterBufferVP8 *pp;
    nvdec_vp8_pic_s *pic;
    ULONG avoid = 0, i, datalen;
    LONG golden, altref, last, slot, map[17], cur = VP8_CURR_FRAME_SFC;
    UBYTE *pad;
    BOOL key;

    if (!v || !v->have_pp || !v->have_data)
        return FALSE;
    pp = &v->pp;
    key = !pp->pic_fields.bits.key_frame;
    if ((pp->frame_width > dec->width) || (pp->frame_height > dec->height))
    {
        nvlog("[Nouveau] NVDEC: VP8 frame %lux%lu does not fit the session\n",
            (unsigned long)pp->frame_width, (unsigned long)pp->frame_height);
        return FALSE;
    }
    golden = altref = last = -1;
    if (!key)
    {
        golden = vp8_refslot(dec, pp->golden_ref_frame);
        altref = vp8_refslot(dec, pp->alt_ref_frame);
        last = vp8_refslot(dec, pp->last_ref_frame);
    }
    if (golden >= 0) avoid |= 1UL << golden;
    if (altref >= 0) avoid |= 1UL << altref;
    if (last >= 0)   avoid |= 1UL << last;
    slot = HIDDNouveauDecodeAllocSlot(dec, dec->cur.bm, dec->cur.buffer, avoid);
    if (slot < 0)
        return FALSE;
    dec->cur.slot = slot;
    for (i = 0; i < 17; i++)
        map[i] = -1;
    map[VP8_GOLDEN_FRAME_SFC] = (golden >= 0) ? golden : slot;
    map[VP8_ALTREF_FRAME_SFC] = (altref >= 0) ? altref : slot;
    map[VP8_LAST_FRAME_SFC] = (last >= 0) ? last : slot;
    map[VP8_CURR_FRAME_SFC] = slot;

    pic = (nvdec_vp8_pic_s *)f->picsetup->map;
    memset(pic, 0, NOUVEAU_DECODE_SLICEOFFS_OFFSET);
    memset((UBYTE *)f->picsetup->map + NOUVEAU_DECODE_STATUS_OFFSET, 0, 256);
    ((ULONG *)((UBYTE *)f->picsetup->map + NOUVEAU_DECODE_SLICEOFFS_OFFSET))[0] = 0;
    ((ULONG *)((UBYTE *)f->picsetup->map + NOUVEAU_DECODE_SLICEOFFS_OFFSET))[1] = dec->cur.bitstreamlen;
    datalen = dec->cur.bitstreamlen;
    pad = HIDDNouveauDecodeBitstreamReserve(dec, 256);
    if (!pad)
        return FALSE;
    memset(pad, 0, 256);

    /* whole macroblocks, as NVIDIA's driver: the engine clamps references to these (360 rows mispredicted the last MB row) */
    pic->FrameWidth = (pp->frame_width + 15) & ~15UL;
    pic->FrameHeight = (pp->frame_height + 15) & ~15UL;
    pic->keyFrame = key ? 1 : 0;
    pic->version = pp->pic_fields.bits.version;
    pic->tileFormat = NVDEC_TILEFORMAT_KBL;
    pic->gob_height = NVDEC_GOB_HEIGHT_2;
    pic->errorConcealOn = 1;
    pic->firstPartSize = v->sp.partition_size[0] + (v->sp.macroblock_offset + 7) / 8;
    pic->HistBufferSize = VP8_HISTORY_SIZE >> 8;
    pic->VLDBufferSize = datalen + (key ? 10 : 3);      /* the whole frame, as NVIDIA's driver counts it */
    pic->FrameStride[0] = dec->pitch >> 4;              /* in 16-byte units */
    pic->FrameStride[1] = dec->pitch >> 4;
    pic->luma_bot_offset = (dec->pitch * ((dec->height + 31) & ~31UL) / 2) >> 8;
    pic->chroma_bot_offset = (dec->pitch * (((dec->height + 31) & ~31UL) / 2) / 2) >> 8;
    pic->segmentation_feature_data_update =
        pp->pic_fields.bits.segmentation_enabled ? pp->pic_fields.bits.update_segment_feature_data : 0;

    nouveau_staging_to_gpu(f->picsetup->map, NOUVEAU_DECODE_PICSETUP_SIZE);
    nouveau_staging_to_gpu(f->bitstream->map, dec->cur.bitstreamlen);
    {
        ULONG mt[2];

        mt[0] = NVDEC_VP8_SET_PROB_DATA_OFFSET;
        mt[1] = (ULONG)(v->prob->offset >> 8);
        /* NVIDIA's control word (gptimer, error concealment, mbtimer, event trace, PROD_RUN) */
        if (!HIDDNouveauDecodeEmit(dec, f, cur, map, NVDEC_APPLICATION_ID_VP8,
                                   NVDEC_CONTROL_CODEC_VP8 | 0x52050,
                                   &v->prob, 1, mt, 1))
            return FALSE;
    }
    D(bug("[Nouveau] NVDEC: VP8 frame %lu key %d slot %ld g %ld a %ld l %ld\n",
        (unsigned long)dec->frames, key, (long)slot, (long)golden, (long)altref, (long)last));
    return TRUE;
}

const struct HIDDNouveauDecodeOps HIDDNouveauDecodeVP8Ops =
{
    vp8_open, vp8_close, vp8_begin, vp8_params, vp8_bitstream, vp8_submit, vp8_status
};
