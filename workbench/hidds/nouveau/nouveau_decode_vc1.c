/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: NVDEC VC-1 (Simple, Main: WMV3) decoding for the gfx.hidd decode
          interface. The VA picture buffer maps onto nvdec_vc1_pic_s; the
          engine parses the picture header (bitplanes included) from the
          frame data itself, so VA's bitplane buffers are not needed. As for
          MPEG-2, decoder picture 0 is the picture being decoded, 1 the
          forward reference and 2 the backward one.
*/

#include "nouveau_intern.h"
#include "nouveau_decode.h"
#include "nouveau_vadec.h"

#define DEBUG 0
#include <aros/debug.h>
#include <proto/exec.h>
#include <string.h>

#include <nvidia/video/nvdec_drv.h>

#define NVDEC_APPLICATION_ID_VC1            2
#define NVDEC_CONTROL_CODEC_VC1             2
#define NVDEC_CONTROL_ALL_INTRA_FRAME       (1 << 17)

#define NVDEC_TILEFORMAT_KBL                1
#define NVDEC_GOB_HEIGHT_2                  0

#define VC1_PIC_CUR                         0
#define VC1_PIC_FWD                         1
#define VC1_PIC_BWD                         2

#define VC1_HISTORY_SIZE                    (2 * 1024 * 1024)
#define VC1_SCRATCH_BYTES                   768             /* NVIDIA's (720x480) */

struct vc1_slice
{
    ULONG   offset;             /* in the bitstream */
    ULONG   dataoff, datasize;  /* into the VA data buffer it belongs to */
    UBYTE   datadone;
};

struct vc1_state
{
    VAPictureParameterBufferVC1     pp;
    BOOL                have_pp;
    struct vc1_slice    slices[NOUVEAU_DECODE_MAX_SLICES];
    ULONG               nslices, nextdata;
    struct nouveau_bo   *scratch;
    ULONG               scratchsize;
    LONG                fieldfwd, fieldbwd; /* the first field's references, for the second */
};

/* a zeroed VRAM buffer: the engine keeps state in these */
static BOOL vc1_zeroed(struct HIDDNouveauDecode *dec, ULONG size, struct nouveau_bo **bo)
{
    if (nouveau_bo_new(dec->carddata->dev, NOUVEAU_BO_VRAM | NOUVEAU_BO_MAP, 4096, size, NULL, bo) ||
        nouveau_bo_map(*bo, NOUVEAU_BO_WR, dec->carddata->client))
        return FALSE;
    memset((*bo)->map, 0, size);
    return TRUE;
}

static BOOL vc1_open(struct HIDDNouveauDecode *dec)
{
    struct vc1_state *v = AllocMem(sizeof(*v), MEMF_PUBLIC | MEMF_CLEAR);
    ULONG mbs = ((dec->width + 15) / 16) * ((dec->height + 15) / 16);

    if (!v)
        return FALSE;
    dec->cur.codecstate = v;
    /*
     * Scratch pictures (intensity compensation and range reduction work on
     * modified copies of the references): two NV12 pictures of the session
     * size, generously aligned.
     */
    dec->minimal = TRUE;
    dec->rowalign = 128;            /* the field offsets assume 128-row aligned planes (as NVIDIA's pictures) */
    v->scratchsize = 2 * (((dec->width + 63) & ~63UL) * (((dec->height + 15) & ~15UL) * 3 / 2));
    v->scratchsize = (v->scratchsize + 0xffff) & ~0xffffUL;
    if (!vc1_zeroed(dec, VC1_HISTORY_SIZE, &dec->history) ||
        !vc1_zeroed(dec, (mbs * 256 + 0xffff) & ~0xffffUL, &dec->coloc) ||
        !vc1_zeroed(dec, v->scratchsize, &v->scratch))
        return FALSE;
    return TRUE;
}

static VOID vc1_close(struct HIDDNouveauDecode *dec)
{
    struct vc1_state *v = dec->cur.codecstate;

    if (!v)
        return;
    if (v->scratch)
        nouveau_bo_ref(NULL, &v->scratch);
    FreeMem(v, sizeof(*v));
    dec->cur.codecstate = NULL;
}

static BOOL vc1_begin(struct HIDDNouveauDecode *dec)
{
    struct vc1_state *v = dec->cur.codecstate;

    if (!v)
        return FALSE;
    v->have_pp = FALSE;
    v->nslices = v->nextdata = 0;
    return TRUE;
}

static BOOL vc1_params(struct HIDDNouveauDecode *dec, ULONG type, APTR data, ULONG size, ULONG count)
{
    struct vc1_state *v = dec->cur.codecstate;
    ULONG i;

    switch (type)
    {
    case VAPictureParameterBufferType:
        if (size < sizeof(VAPictureParameterBufferVC1))
            return FALSE;
        memcpy(&v->pp, data, sizeof(v->pp));
        v->have_pp = TRUE;
        return TRUE;

    case VASliceParameterBufferType:
        if (size < sizeof(VASliceParameterBufferVC1))
            return FALSE;
        for (i = 0; i < count; i++)
        {
            const VASliceParameterBufferVC1 *sp = (const VASliceParameterBufferVC1 *)((UBYTE *)data + i * size);
            struct vc1_slice *sl;

            if (v->nslices >= NOUVEAU_DECODE_MAX_SLICES)
                return FALSE;
            sl = &v->slices[v->nslices++];
            sl->dataoff = sp->slice_data_offset;
            sl->datasize = sp->slice_data_size;
            sl->datadone = 0;
        }
        return TRUE;

    default:
        /* bitplanes and others: the engine reads the picture header itself */
        return TRUE;
    }
}

/* Simple / Main: the frame data as VA delivers it (no start codes) */
static BOOL vc1_bitstream(struct HIDDNouveauDecode *dec, APTR data, ULONG size)
{
    struct vc1_state *v = dec->cur.codecstate;
    ULONG i;

    if (v->nextdata >= v->nslices)
    {
        struct vc1_slice *sl;

        if (v->nslices >= NOUVEAU_DECODE_MAX_SLICES)
            return FALSE;
        sl = &v->slices[v->nslices++];
        sl->dataoff = 0;
        sl->datasize = size;
        sl->datadone = 0;
    }
    for (i = v->nextdata; i < v->nslices; i++)
    {
        struct vc1_slice *sl = &v->slices[i];
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
    v->nextdata = v->nslices;
    return TRUE;
}

static ULONG vc1_status(struct HIDDNouveauDecode *dec, ULONG *status)
{
    nvdec_status_s *st = (nvdec_status_s *)status;
    ULONG mbs = ((dec->width + 15) / 16) * ((dec->height + 15) / 16);

    if (dec->fields)
        mbs /= 2;
    if (!st->mbs_in_error && !st->slice_header_error_code && (st->mbs_correctly_decoded >= mbs) &&
        (!st->error_status || (st->error_status == 0x20)))
        return 0;
    if (dec->errors++ < 4)
        nvlog("[Nouveau] NVDEC: VC-1 frame error %08lx (ok %lu of %lu, in error %lu, header %lu)\n",
            (unsigned long)st->error_status, (unsigned long)st->mbs_correctly_decoded, (unsigned long)mbs,
            (unsigned long)st->mbs_in_error, (unsigned long)st->slice_header_error_code);
    return st->error_status ? st->error_status : 1;
}

static LONG vc1_refslot(struct HIDDNouveauDecode *dec, VASurfaceID surface)
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

static BOOL vc1_submit(struct HIDDNouveauDecode *dec, struct HIDDNouveauDecodeFrame *f)
{
    struct vc1_state *v = dec->cur.codecstate;
    const VAPictureParameterBufferVC1 *pp;
    nvdec_vc1_pic_s *pic;
    ULONG *sliceoffs, avoid = 0, i;
    LONG fwd, bwd, slot, map[17];
    int ptype;
    UBYTE *pad;

    if (!v || !v->have_pp || !dec->cur.nslices)
        return FALSE;
    pp = &v->pp;
    if ((pp->coded_width > dec->width) || (pp->coded_height > dec->height))
    {
        nvlog("[Nouveau] NVDEC: VC-1 picture %ux%u does not fit the session\n", pp->coded_width, pp->coded_height);
        return FALSE;
    }
    ptype = pp->picture_fields.bits.picture_type;   /* I 0, P 1, B 2, BI 3, skipped 4: NVDEC's numbering */
    if (pp->picture_fields.bits.frame_coding_mode == 2)
    {
        /*
         * A field picture: the type is the pair's (FPTYPE: I/I, I/P, P/I,
         * P/P, B/B, B/BI, BI/B, BI/BI); this field's own one picks the
         * references.
         */
        static const UBYTE first[8] = { 0, 0, 1, 1, 2, 2, 3, 3 }, second[8] = { 0, 1, 0, 1, 2, 3, 2, 3 };

        ptype = pp->picture_fields.bits.is_first_field ? first[ptype & 7] : second[ptype & 7];
    }

    /* references */
    fwd = bwd = -1;
    if ((ptype == 1) || (ptype == 2) || (ptype == 4))
        fwd = vc1_refslot(dec, pp->forward_reference_picture);
    if (ptype == 2)
        bwd = vc1_refslot(dec, pp->backward_reference_picture);
    if (bwd < 0)
        bwd = fwd;
    /* a second field takes the references of the first, as NVIDIA's driver does (traced) */
    if ((pp->picture_fields.bits.frame_coding_mode == 2) && !pp->picture_fields.bits.is_first_field)
    {
        fwd = v->fieldfwd;
        bwd = v->fieldbwd;
    }
    else
    {
        v->fieldfwd = fwd;
        v->fieldbwd = bwd;
    }
    if (fwd >= 0)
        avoid |= 1UL << fwd;
    if (bwd >= 0)
        avoid |= 1UL << bwd;
    slot = HIDDNouveauDecodeAllocSlot(dec, dec->cur.bm, dec->cur.buffer, avoid);
    if (slot < 0)
        return FALSE;
    dec->cur.slot = slot;
    for (i = 0; i < 17; i++)
        map[i] = -1;
    map[VC1_PIC_CUR] = slot;
    map[VC1_PIC_FWD] = (fwd >= 0) ? fwd : slot;
    map[VC1_PIC_BWD] = (bwd >= 0) ? bwd : map[VC1_PIC_FWD];

    pic = (nvdec_vc1_pic_s *)f->picsetup->map;
    sliceoffs = (ULONG *)((UBYTE *)f->picsetup->map + NOUVEAU_DECODE_SLICEOFFS_OFFSET);
    memset(pic, 0, NOUVEAU_DECODE_SLICEOFFS_OFFSET);
    memset((UBYTE *)f->picsetup->map + NOUVEAU_DECODE_STATUS_OFFSET, 0, 256);

    for (i = 0; i < v->nslices; i++)
        sliceoffs[i] = v->slices[i].offset;
    sliceoffs[v->nslices] = 0;                  /* no end marker: NVIDIA's table is just the starts (traced) */
    pad = HIDDNouveauDecodeBitstreamReserve(dec, 256);
    if (!pad)
        return FALSE;
    memset(pad, 0, 256);
    pic->stream_len = dec->cur.bitstreamlen - 256;
    pic->slice_count = v->nslices;
    /*
     * As NVIDIA's driver fills it (traced, WMV3): the sequence-level fields,
     * the frame start code as prefix and end of sequence as EOS data, the
     * sizes and strides; every picture-layer field stays 0 (the engine reads
     * the picture header, bitplanes included, from the frame data).
     */
    /* FFmpeg passes the data after the start code: put it back (a second field has its own) */
    pic->prefixStartCode[2] = 1;
    pic->prefixStartCode[3] = ((pp->picture_fields.bits.frame_coding_mode == 2) &&
                               !pp->picture_fields.bits.is_first_field) ? 0x0c : 0x0d;
    pic->eos[2] = 1;
    pic->eos[3] = 0x0a;                                 /* end of sequence */
    pic->eos[10] = 1;
    pic->eos[11] = 0x0a;                                /* NVIDIA's driver writes it twice (traced) */
    pic->explicitEOSPresentFlag = 1;
    pic->scratch_pic_buffer_size = VC1_SCRATCH_BYTES;
    pic->gptimer_timeout_value = 0x4d3f640;
    pic->FrameWidth = pp->coded_width;
    pic->FrameHeight = pp->coded_height;
    pic->CodedWidth = pp->coded_width;
    pic->CodedHeight = pp->coded_height;
    pic->profile = (pp->sequence_fields.bits.profile == 3) ? 2 : 1;    /* advanced, else simple or main */
    pic->refdist_flag = pp->reference_fields.bits.reference_distance_flag;
    pic->postprocflag = pp->post_processing ? 1 : 0;
    pic->pulldown = pp->sequence_fields.bits.pulldown;
    pic->interlace = pp->sequence_fields.bits.interlace;
    pic->tfcntrflag = pp->sequence_fields.bits.tfcntrflag;
    pic->finterpflag = pp->sequence_fields.bits.finterpflag;
    pic->psf = pp->sequence_fields.bits.psf;
    pic->tileFormat = NVDEC_TILEFORMAT_KBL;
    pic->gob_height = NVDEC_GOB_HEIGHT_2;
    pic->multires = pp->sequence_fields.bits.multires;
    pic->syncmarker = pp->sequence_fields.bits.syncmarker;
    pic->rangered = pp->sequence_fields.bits.rangered;
    pic->maxbframes = (pic->profile == 2) ? 0 : pp->sequence_fields.bits.max_b_frames;   /* simple/main only (traced) */
    pic->dquant = pp->pic_quantizer_fields.bits.dquant;
    pic->panscan_flag = pp->entrypoint_fields.bits.panscan_flag;
    pic->quantizer = pp->pic_quantizer_fields.bits.quantizer;
    pic->extended_mv = pp->mv_fields.bits.extended_mv_flag;
    pic->extended_dmv = pp->mv_fields.bits.extended_dmv_flag;
    pic->overlap = pp->sequence_fields.bits.overlap;
    pic->vstransform = pp->transform_fields.bits.variable_sized_transform_flag;
    pic->loopfilter = pp->entrypoint_fields.bits.loopfilter;
    pic->fastuvmc = pp->fast_uvmc_flag;
    pic->HistBufferSize = ((pp->coded_width + 15) / 16) * 3;    /* 768 bytes per macroblock column, as NVIDIA's driver (traced) */
    pic->FrameStride[0] = dec->pitch;
    pic->FrameStride[1] = dec->pitch;
    if (pic->profile == 2)
    {
        /* as NVIDIA's driver sets them for the advanced profile (traced): a 32-row aligned luma field, chroma half */
        ULONG rows = ((pp->coded_height / 2) + 31) & ~31UL;

        pic->luma_bot_offset = (dec->pitch * rows) >> 8;
        pic->chroma_bot_offset = (dec->pitch * (rows / 2)) >> 8;
    }
    else
    {
        pic->luma_bot_offset = (dec->pitch * (((dec->height + 127) & ~127UL) / 2)) >> 8;
        pic->chroma_bot_offset = (dec->pitch * (((dec->height + 127) & ~127UL) / 4)) >> 8;
    }
    if (pp->picture_fields.bits.frame_coding_mode == 2)
        dec->fields = TRUE;                             /* field pictures: half the macroblocks (status) */
    /*
     * Layout 1 keeps the two fields in separate halves of the picture, 0
     * stores it in frame order. Progressive streams use 0. Interlaced ones
     * need 1, as NVIDIA's driver sets it (traced): field-coded motion
     * compensation from frame-order references smeared every moving area.
     * The copy-out weaves the fields together again.
     */
    if (pp->sequence_fields.bits.interlace)
    {
        pic->output_memory_layout = 1;
        pic->ref_memory_layout[0] = 1;
        pic->ref_memory_layout[1] = 1;
        dec->weaverows = (pic->luma_bot_offset << 8) / dec->pitch;
        dec->cweaverows = (pic->chroma_bot_offset << 8) / dec->pitch;
    }
    else
    {
        pic->output_memory_layout = 0;
        pic->ref_memory_layout[0] = 0;
        pic->ref_memory_layout[1] = 0;
        dec->weaverows = dec->cweaverows = 0;
    }

    nouveau_staging_to_gpu(f->picsetup->map, NOUVEAU_DECODE_PICSETUP_SIZE);
    nouveau_staging_to_gpu(f->bitstream->map, dec->cur.bitstreamlen);
    {
        ULONG mt[10], nmt = 3;
        struct nouveau_bo *cur = dec->slot[slot].bo;

        mt[0] = 0x04B8;     /* SET_PIC_SCRATCH_BUF_OFFSET */
        mt[1] = (ULONG)(v->scratch->offset >> 8);
        mt[2] = 0x0428;     /* SET_DISPLAY_BUF_LUMA_OFFSET: the picture itself, as NVIDIA's driver */
        mt[3] = (ULONG)(cur->offset >> 8);
        mt[4] = 0x042C;     /* SET_DISPLAY_BUF_CHROMA_OFFSET */
        mt[5] = (ULONG)((cur->offset + dec->chromaoff) >> 8);
        /* NVIDIA's method set and nothing else: on Blackwell an extra one (display size, histogram or an early 0x4B8) faulted */
        mt[6] = 0x0410;         /* SET_SLICE_OFFSETS_BUF_OFFSET */
        mt[7] = (ULONG)((f->picsetup->offset + NOUVEAU_DECODE_SLICEOFFS_OFFSET) >> 8);
        nmt = 4;
        /* each picture its own index, as NVIDIA's driver (its surface number): the engine keeps per-picture state by it */
        mt[8] = 0x040C;         /* SET_PICTURE_INDEX */
        mt[9] = dec->frames;    /* a running count, every field its own (traced) */
        nmt = 5;
        if (!HIDDNouveauDecodeEmit(dec, f, VC1_PIC_CUR, map, NVDEC_APPLICATION_ID_VC1,
                                   NVDEC_CONTROL_CODEC_VC1 | 0x52050, &v->scratch, 1, mt, nmt))
            return FALSE;
    }
    D(bug("[Nouveau] NVDEC: VC-1 frame %lu type %d slot %ld fwd %ld bwd %ld\n",
        (unsigned long)dec->frames, ptype, (long)slot, (long)fwd, (long)bwd));
    return TRUE;
}

const struct HIDDNouveauDecodeOps HIDDNouveauDecodeVC1Ops =
{
    vc1_open, vc1_close, vc1_begin, vc1_params, vc1_bitstream, vc1_submit, vc1_status
};
