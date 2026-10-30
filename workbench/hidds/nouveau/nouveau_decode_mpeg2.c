/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: NVDEC MPEG-2 (Simple, Main) decoding for the gfx.hidd decode
          interface. The VA picture, quantiser-matrix and slice buffers map
          onto nvdec_mpeg2_pic_s; the engine parses the slices itself. The
          picture has no reference list: decoder picture 0 is the picture
          being decoded, 1 the forward reference, 2 the backward one.
*/

#include "nouveau_intern.h"
#include "nouveau_decode.h"
#include "nouveau_vadec.h"

#define DEBUG 0
#include <aros/debug.h>
#include <proto/exec.h>
#include <string.h>

#include <nvidia/video/nvdec_drv.h>

#define NVDEC_APPLICATION_ID_MPEG12         1
#define NVDEC_CONTROL_CODEC_MPEG2           1
#define NVDEC_CONTROL_ALL_INTRA_FRAME       (1 << 17)

#define NVDEC_TILEFORMAT_KBL                1
#define NVDEC_GOB_HEIGHT_2                  0

#define MPEG2_PIC_CUR                       0
#define MPEG2_PIC_FWD                       1
#define MPEG2_PIC_BWD                       2

#define MPEG2_HISTORY_SIZE                  (2 * 1024 * 1024)
#define MPEG2_COLOC_SIZE                    (256 * 1024)

struct mpeg2_slice
{
    ULONG   offset;             /* of the slice start code in the bitstream */
    ULONG   dataoff, datasize;  /* into the VA data buffer it belongs to */
    UBYTE   datadone;
};

struct mpeg2_state
{
    VAPictureParameterBufferMPEG2   pp;
    VAIQMatrixBufferMPEG2           iq;
    BOOL                have_pp, have_iq;
    struct mpeg2_slice  slices[NOUVEAU_DECODE_MAX_SLICES];
    ULONG               nslices, nextdata;
};

/* zig-zag scan position -> raster position (the normal MPEG-2 scan) */
static const UBYTE mpeg2_zigzag[64] =
{
     0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

/* ISO/IEC 13818-2 default intra matrix, raster order */
static const UBYTE mpeg2_default_intra[64] =
{
     8, 16, 19, 22, 26, 27, 29, 34,
    16, 16, 22, 24, 27, 29, 34, 37,
    19, 22, 26, 27, 29, 34, 34, 38,
    22, 22, 26, 27, 29, 34, 37, 40,
    22, 26, 27, 29, 32, 35, 40, 48,
    26, 27, 29, 32, 35, 40, 48, 58,
    26, 27, 29, 34, 38, 46, 56, 69,
    27, 29, 35, 38, 46, 56, 69, 83
};

static BOOL mpeg2_open(struct HIDDNouveauDecode *dec)
{
    struct mpeg2_state *m = AllocMem(sizeof(*m), MEMF_PUBLIC | MEMF_CLEAR);

    if (!m)
        return FALSE;
    dec->cur.codecstate = m;
    /* the engine keeps state in the history buffer: start it from zeros, not old VRAM */
    if (nouveau_bo_new(dec->carddata->dev, NOUVEAU_BO_VRAM | NOUVEAU_BO_MAP, 4096, MPEG2_HISTORY_SIZE, NULL, &dec->history) ||
        nouveau_bo_map(dec->history, NOUVEAU_BO_WR, dec->carddata->client))
        return FALSE;
    memset(dec->history->map, 0, MPEG2_HISTORY_SIZE);
    /* not known to be used by MPEG-2, but no decoder address is left at 0 */
    if (nouveau_bo_new(dec->carddata->dev, NOUVEAU_BO_VRAM | NOUVEAU_BO_MAP, 4096, MPEG2_COLOC_SIZE, NULL, &dec->coloc) ||
        nouveau_bo_map(dec->coloc, NOUVEAU_BO_WR, dec->carddata->client))
        return FALSE;
    memset(dec->coloc->map, 0, MPEG2_COLOC_SIZE);
    return TRUE;
}

static VOID mpeg2_close(struct HIDDNouveauDecode *dec)
{
    if (dec->cur.codecstate)
    {
        FreeMem(dec->cur.codecstate, sizeof(struct mpeg2_state));
        dec->cur.codecstate = NULL;
    }
}

static BOOL mpeg2_begin(struct HIDDNouveauDecode *dec)
{
    struct mpeg2_state *m = dec->cur.codecstate;

    if (!m)
        return FALSE;
    m->have_pp = m->have_iq = FALSE;
    m->nslices = m->nextdata = 0;
    return TRUE;
}

static BOOL mpeg2_params(struct HIDDNouveauDecode *dec, ULONG type, APTR data, ULONG size, ULONG count)
{
    struct mpeg2_state *m = dec->cur.codecstate;
    ULONG i;

    switch (type)
    {
    case VAPictureParameterBufferType:
        if (size < sizeof(VAPictureParameterBufferMPEG2))
            return FALSE;
        memcpy(&m->pp, data, sizeof(m->pp));
        m->have_pp = TRUE;
        return TRUE;

    case VAIQMatrixBufferType:
        if (size < sizeof(VAIQMatrixBufferMPEG2))
            return FALSE;
        memcpy(&m->iq, data, sizeof(m->iq));
        m->have_iq = TRUE;
        return TRUE;

    case VASliceParameterBufferType:
        if (size < sizeof(VASliceParameterBufferMPEG2))
            return FALSE;
        for (i = 0; i < count; i++)
        {
            const VASliceParameterBufferMPEG2 *sp = (const VASliceParameterBufferMPEG2 *)((UBYTE *)data + i * size);
            struct mpeg2_slice *sl;

            if (m->nslices >= NOUVEAU_DECODE_MAX_SLICES)
                return FALSE;
            sl = &m->slices[m->nslices++];
            sl->dataoff = sp->slice_data_offset;
            sl->datasize = sp->slice_data_size;
            sl->datadone = 0;
        }
        return TRUE;

    default:
        /* nothing else matters for MPEG-2; refusing would abort the client's picture */
        return TRUE;
    }
}

/* each VA slice already starts with its slice start code: copied as it is */
static BOOL mpeg2_bitstream(struct HIDDNouveauDecode *dec, APTR data, ULONG size)
{
    struct mpeg2_state *m = dec->cur.codecstate;
    ULONG i;

    if (m->nextdata >= m->nslices)
    {
        struct mpeg2_slice *sl;

        if (m->nslices >= NOUVEAU_DECODE_MAX_SLICES)
            return FALSE;
        sl = &m->slices[m->nslices++];
        sl->dataoff = 0;
        sl->datasize = size;
        sl->datadone = 0;
    }
    for (i = m->nextdata; i < m->nslices; i++)
    {
        struct mpeg2_slice *sl = &m->slices[i];
        UBYTE *p;

        if (sl->datadone)
            continue;
        if ((sl->dataoff > size) || !sl->datasize || (sl->datasize > size - sl->dataoff))
            return FALSE;
        p = HIDDNouveauDecodeBitstreamReserve(dec, sl->datasize);
        if (!p)
            return FALSE;
        sl->offset = (ULONG)(p - (UBYTE *)dec->ring[dec->ringpos].bitstream->map);
        memcpy(p, (UBYTE *)data + sl->dataoff, sl->datasize);
        sl->datadone = 1;
        dec->cur.nslices++;
    }
    m->nextdata = m->nslices;
    return TRUE;
}

static ULONG mpeg2_status(struct HIDDNouveauDecode *dec, ULONG *status)
{
    nvdec_status_s *st = (nvdec_status_s *)status;
    ULONG mbs = ((dec->width + 15) / 16) * ((dec->height + 15) / 16);

    /* a field picture holds half the frame's macroblocks */
    if (dec->fields)
        mbs /= 2;
    if (!st->mbs_in_error && !st->slice_header_error_code && (st->mbs_correctly_decoded >= mbs) &&
        (!st->error_status || (st->error_status == 0x20)))
        return 0;
    if (dec->errors++ < 4)
        nvlog("[Nouveau] NVDEC: MPEG-2 frame error %08lx (ok %lu of %lu, in error %lu, header %lu)\n",
            (unsigned long)st->error_status, (unsigned long)st->mbs_correctly_decoded, (unsigned long)mbs,
            (unsigned long)st->mbs_in_error, (unsigned long)st->slice_header_error_code);
    return st->error_status ? st->error_status : 1;
}

/* a VA reference surface -> the pool slot holding it, or -1 */
static LONG mpeg2_refslot(struct HIDDNouveauDecode *dec, VASurfaceID surface)
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

static BOOL mpeg2_submit(struct HIDDNouveauDecode *dec, struct HIDDNouveauDecodeFrame *f)
{
    struct mpeg2_state *m = dec->cur.codecstate;
    const VAPictureParameterBufferMPEG2 *pp;
    nvdec_mpeg2_pic_s *pic;
    ULONG *sliceoffs, avoid = 0, i;
    LONG fwd, bwd, slot, map[17];
    UBYTE *pad;

    if (!m || !m->have_pp || !dec->cur.nslices)
        return FALSE;
    pp = &m->pp;
    if ((pp->horizontal_size > dec->width) || (pp->vertical_size > dec->height))
    {
        nvlog("[Nouveau] NVDEC: MPEG-2 picture %ux%u does not fit the session\n",
            pp->horizontal_size, pp->vertical_size);
        return FALSE;
    }

    /* references: P uses the forward one for both, I none */
    fwd = bwd = -1;
    if (pp->picture_coding_type >= 2)
        fwd = mpeg2_refslot(dec, pp->forward_reference_picture);
    if (pp->picture_coding_type == 3)
        bwd = mpeg2_refslot(dec, pp->backward_reference_picture);
    if (pp->picture_coding_type == 2)
        bwd = fwd;
    if (fwd >= 0)
        avoid |= 1UL << fwd;
    if (bwd >= 0)
        avoid |= 1UL << bwd;
    slot = HIDDNouveauDecodeAllocSlot(dec, dec->cur.bm, dec->cur.buffer, avoid);
    if (slot < 0)
        return FALSE;
    dec->cur.slot = slot;
    for (i = 0; i < 17; i++)
        map[i] = -1;                /* unused decoder pictures: the current one */
    /* the engine's layout: the picture being decoded at 0, forward reference 1, backward 2 (GB207) */
    map[MPEG2_PIC_FWD] = (fwd >= 0) ? fwd : slot;
    map[MPEG2_PIC_BWD] = (bwd >= 0) ? bwd : map[MPEG2_PIC_FWD];
    map[MPEG2_PIC_CUR] = slot;

    pic = (nvdec_mpeg2_pic_s *)f->picsetup->map;
    sliceoffs = (ULONG *)((UBYTE *)f->picsetup->map + NOUVEAU_DECODE_SLICEOFFS_OFFSET);
    /* all of it up to the slice offsets: the engine reads beyond the published struct */
    memset(pic, 0, NOUVEAU_DECODE_SLICEOFFS_OFFSET);
    memset((UBYTE *)f->picsetup->map + NOUVEAU_DECODE_STATUS_OFFSET, 0, 256);

    /* the slices, then the VLD's read-ahead pad; end of stream passed separately */
    for (i = 0; i < m->nslices; i++)
        sliceoffs[i] = m->slices[i].offset;
    sliceoffs[m->nslices] = dec->cur.bitstreamlen;
    pad = HIDDNouveauDecodeBitstreamReserve(dec, 256);
    if (!pad)
        return FALSE;
    memset(pad, 0, 256);
    pic->stream_len = dec->cur.bitstreamlen - 256;
    pic->slice_count = m->nslices;
    pic->eos[0] = 0;
    pic->eos[1] = 0;
    pic->eos[2] = 1;
    pic->eos[3] = 0xb7;                     /* sequence_end_code */
    pic->explicitEOSPresentFlag = 1;

    pic->FrameWidth = pp->horizontal_size;
    pic->FrameHeight = pp->vertical_size;
    pic->picture_structure = pp->picture_coding_extension.bits.picture_structure;
    pic->picture_coding_type = pp->picture_coding_type;
    pic->intra_dc_precision = pp->picture_coding_extension.bits.intra_dc_precision;
    pic->frame_pred_frame_dct = pp->picture_coding_extension.bits.frame_pred_frame_dct;
    pic->concealment_motion_vectors = pp->picture_coding_extension.bits.concealment_motion_vectors;
    pic->intra_vlc_format = pp->picture_coding_extension.bits.intra_vlc_format;
    pic->tileFormat = NVDEC_TILEFORMAT_KBL;
    pic->gob_height = NVDEC_GOB_HEIGHT_2;
    pic->f_code[0] = (pp->f_code >> 12) & 15;
    pic->f_code[1] = (pp->f_code >> 8) & 15;
    pic->f_code[2] = (pp->f_code >> 4) & 15;
    pic->f_code[3] = pp->f_code & 15;
    pic->PicWidthInMbs = (pp->horizontal_size + 15) / 16;
    pic->FrameHeightInMbs = (pp->vertical_size + 15) / 16;
    pic->pitch_luma = dec->pitch;
    pic->pitch_chroma = dec->pitch;
    pic->HistBufferSize = MPEG2_HISTORY_SIZE >> 8;
    pic->alternate_scan = pp->picture_coding_extension.bits.alternate_scan;
    pic->q_scale_type = pp->picture_coding_extension.bits.q_scale_type;
    pic->top_field_first = pp->picture_coding_extension.bits.top_field_first;
    /*
     * Field pictures: the bottom field's offsets as NVIDIA's driver sets them
     * (traced: whole 32-row field halves); the second field of a pair goes to
     * the slot of the first, which the engine finds there itself.
     */
    {
        ULONG rows = (ULONG)pic->FrameHeightInMbs * 16;

        pic->luma_bot_offset = ((((rows / 2) + 31) & ~31UL) * dec->pitch) >> 8;
        pic->chroma_bot_offset = ((((rows / 4) + 31) & ~31UL) * dec->pitch) >> 8;
        pic->secondfield = (pic->picture_structure != 3) && !pp->picture_coding_extension.bits.is_first_field;
        if (pic->picture_structure != 3)
            dec->fields = TRUE;     /* status is checked frames later: stays set */
    }
    /* the engine takes the matrices in raster order; VA gives zig-zag order */
    for (i = 0; i < 64; i++)
    {
        pic->quant_mat_8x8intra[mpeg2_zigzag[i]] =
            (m->have_iq && m->iq.load_intra_quantiser_matrix) ? m->iq.intra_quantiser_matrix[i] :
                                                                mpeg2_default_intra[mpeg2_zigzag[i]];
        pic->quant_mat_8x8nonintra[mpeg2_zigzag[i]] =
            (m->have_iq && m->iq.load_non_intra_quantiser_matrix) ? m->iq.non_intra_quantiser_matrix[i] : 16;
    }

    nouveau_staging_to_gpu(f->picsetup->map, NOUVEAU_DECODE_PICSETUP_SIZE);
    nouveau_staging_to_gpu(f->bitstream->map, dec->cur.bitstreamlen);

    if (!HIDDNouveauDecodeEmit(dec, f, MPEG2_PIC_CUR, map, NVDEC_APPLICATION_ID_MPEG12,
                               NVDEC_CONTROL_CODEC_MPEG2 |
                               ((pp->picture_coding_type == 1) ? NVDEC_CONTROL_ALL_INTRA_FRAME : 0),
                               NULL, 0, NULL, 0))
        return FALSE;
    D(bug("[Nouveau] NVDEC: MPEG-2 frame %lu type %d slot %ld fwd %ld bwd %ld slices %lu\n",
        (unsigned long)dec->frames, (int)pp->picture_coding_type, (long)slot, (long)fwd, (long)bwd,
        (unsigned long)m->nslices));
    return TRUE;
}

const struct HIDDNouveauDecodeOps HIDDNouveauDecodeMPEG2Ops =
{
    mpeg2_open, mpeg2_close, mpeg2_begin, mpeg2_params, mpeg2_bitstream, mpeg2_submit, mpeg2_status
};
