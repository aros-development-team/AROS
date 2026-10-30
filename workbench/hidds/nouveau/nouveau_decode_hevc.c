/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    HEVC (Main, Main10) on NVDEC: VA-API picture, matrix and slice parameters
    become the decoder's picture setup (nvdec_hevc_pic_s, NVIDIA's MIT header
    in Mesa's nouveau/headers tree), the slices an Annex B bitstream.

    The engine's HEVC block descends from the Hantro/VeriSilicon G2 (the
    GIP_ASIC_* sizes and the field set match Linux' hantro_g2_hevc_dec.c), so
    where nvdec_drv.h is silent this follows the G2 driver: initial reference
    lists by picture slot, POC differences, column-major scaling lists, a
    tile-size table. See NVDEC-HEVC-REFERENCE.md.
*/

#include "nouveau_intern.h"
#include "nouveau_decode.h"
#include "nouveau_vadec.h"

#define DEBUG 0
#include <aros/debug.h>
#include <proto/exec.h>
#include <string.h>
#include <stdio.h>

#include <nvidia/video/nvdec_drv.h>

#define NVDEC_APPLICATION_ID_HEVC           7
#define NVDEC_CONTROL_CODEC_HEVC            7
#define NVDEC_CONTROL_ALL_INTRA_FRAME       (1 << 17)
#define NVDEC_HEVC_SET_SCALING_LIST_OFFSET  0x0580
#define NVDEC_HEVC_SET_TILE_SIZES_OFFSET    0x0584
#define NVDEC_HEVC_SET_FILTER_BUFFER_OFFSET 0x0588
#define NVDEC_HEVC_SET_SAO_BUFFER_OFFSET    0x058C
#define NVDEC_HEVC_SET_SLICE_INFO_BUFFER_OFFSET 0x0590
#define NVDEC_HEVC_SET_SLICE_GROUP_INDEX    0x0594
#define NVDEC_SET_PIC_SCRATCH_BUF_OFFSET    0x04B8
#define NVDEC_SET_INTRA_TOP_BUF_OFFSET      0x04C8  /* c7b0+ generic line buffers */
#define NVDEC_SET_TILE_SIZE_BUF_OFFSET      0x04CC
#define NVDEC_SET_FILTER_BUFFER_OFFSET      0x04D0
#define NVDEC_SET_DISPLAY_BUF_LUMA_OFFSET   0x0428
#define NVDEC_SET_DISPLAY_BUF_CHROMA_OFFSET 0x042C
#define NVDEC_SET_EXTERNAL_MVBUFFER_OFFSET  0x04BC
#define NVDEC_H264_SET_MBHIST_BUF_OFFSET    0x0500

#define NVDEC_TILEFORMAT_KBL                1
#define NVDEC_GOB_HEIGHT_2                  0

/* picsetup buffer layout (NOUVEAU_DECODE_PICSETUP_SIZE): pic at 0, then these */
#define HEVC_SCALING_OFFSET                 0x0400  /* 1008 bytes */
#define HEVC_TILES_OFFSET                   0x0800  /* 1776 bytes */

/* filter buffer: regions at 256-byte-unit offsets given in the picture setup */
/* filter buffer regions, 256-byte units (NVIDIA's for 1920x1088; x fltscale) */
#define HEVC_FLT_SAO_LEFT                   0x0a18
#define HEVC_FLT_BSD_CTRL                   0x5ad8
#define HEVC_FLT_DBLK_ABOVE                 0x5d5e
#define HEVC_FLT_SAO_ABOVE                  0x9d62
#define HEVC_FLT_SLICE_EDGE                 0x9d62
#define HEVC_FLT_END                        0xdd66      /* SAO-above sized as the deblock-above region */
#define HEVC_SAO_SIZE                       0x40000
#define HEVC_INTRA_TOP_SIZE                 0x40000
#define HEVC_SCRATCH_SIZE                   0x200000
#define HEVC_HISTORY_SIZE                   (2 * 1024 * 1024)

struct hevc_slice
{
    ULONG   offset;             /* of the start code in the bitstream */
    ULONG   dataoff, datasize;  /* into the VA data buffer it belongs to */
    UBYTE   datadone;
    UBYTE   type;               /* LongSliceFlags slice_type: 2 = I */
};

struct hevc_state
{
    VAPictureParameterBufferHEVC    pp;
    VAPictureParameterBufferHEVCRext rext;      /* range extension (4:2:2, 4:4:4, 12-bit profiles), zero otherwise */
    VAIQMatrixBufferHEVC            iq;
    BOOL                have_pp, have_iq;
    struct hevc_slice   slices[NOUVEAU_DECODE_MAX_SLICES];
    ULONG               nslices, nextdata;
    struct nouveau_bo   *filter, *sao, *intratop, *scratch;
    struct nouveau_bo   *blkbuf;            /* Blackwell (CFB0): the 0x4DC buffer */
    ULONG               colmvsize;          /* bytes per picture slot */
    ULONG               fltscale;           /* filter buffer layout multiple */
    BOOL                firstlogged;
    BOOL                ltwarned;
};

static BOOL hevc_bo(struct HIDDNouveauDecode *dec, ULONG size, struct nouveau_bo **bo)
{
    if (nouveau_bo_new(dec->carddata->dev, NOUVEAU_BO_VRAM | NOUVEAU_BO_MAP, 4096, size, NULL, bo))
    {
        nvlog("[Nouveau] NVDEC: HEVC buffer of %lu bytes failed\n", (unsigned long)size);
        *bo = NULL;
        return FALSE;
    }
    /* zeroed once: the line buffers must not start with garbage */
    if (!nouveau_bo_map(*bo, NOUVEAU_BO_WR, dec->carddata->client))
        memset((*bo)->map, 0, size);
    return TRUE;
}

static BOOL hevc_open(struct HIDDNouveauDecode *dec)
{
    struct hevc_state *h = AllocMem(sizeof(*h), MEMF_PUBLIC | MEMF_CLEAR);
    ULONG w64 = (dec->width + 63) & ~63UL, h64 = (dec->height + 63) & ~63UL;

    if (!h)
        return FALSE;
    dec->cur.codecstate = h;
    /*
     * Laid out as NVIDIA's driver does (traced on an RTX 4080): collocated
     * MVs w64*h64/16 bytes per picture slot; one filter buffer holding the
     * deblock-left, SAO-left, BSD-control, deblock-above and SAO-above
     * regions at these offsets (256-byte units; NVIDIA's 1920x1088 layout,
     * scaled up for larger pictures); an intra-top line buffer; no history.
     */
    h->colmvsize = ((w64 * h64) / 16 + 255) & ~255UL;
    h->fltscale = 1;
    while ((dec->width > 1920 * h->fltscale) || (dec->height > 1088 * h->fltscale))
        h->fltscale++;
    dec->rowalign = 64;                             /* whole CTB rows, as NVIDIA's pictures */
    dec->minimal = TRUE;
    if (!hevc_bo(dec, (h->colmvsize * NOUVEAU_DECODE_SLOTS + 0x1fffff) & ~0x1fffffUL, &dec->coloc) ||
        !hevc_bo(dec, HEVC_FLT_END * 256 * h->fltscale, &h->filter) ||
        !hevc_bo(dec, HEVC_INTRA_TOP_SIZE * h->fltscale, &h->intratop))
        return FALSE;
    if ((dec->oclass >= NVDEC_CLASS_BLACKWELL) && !hevc_bo(dec, NVDEC_BLK_BUF_SIZE(dec->width), &h->blkbuf))
        return FALSE;

    return TRUE;
}

static VOID hevc_close(struct HIDDNouveauDecode *dec)
{
    struct hevc_state *h = dec->cur.codecstate;

    if (!h)
        return;
    if (h->filter)
        nouveau_bo_ref(NULL, &h->filter);
    if (h->sao)
        nouveau_bo_ref(NULL, &h->sao);
    if (h->intratop)
        nouveau_bo_ref(NULL, &h->intratop);
    if (h->blkbuf)
        nouveau_bo_ref(NULL, &h->blkbuf);
    if (h->scratch)
        nouveau_bo_ref(NULL, &h->scratch);

    FreeMem(h, sizeof(*h));
    dec->cur.codecstate = NULL;
}

static BOOL hevc_begin(struct HIDDNouveauDecode *dec)
{
    struct hevc_state *h = dec->cur.codecstate;

    if (!h)
        return FALSE;
    h->have_pp = h->have_iq = FALSE;
    h->nslices = h->nextdata = 0;
    return TRUE;
}

static BOOL hevc_params(struct HIDDNouveauDecode *dec, ULONG type, APTR data, ULONG size, ULONG count)
{
    struct hevc_state *h = dec->cur.codecstate;
    ULONG i;

    switch (type)
    {
    case VAPictureParameterBufferType:
        if (size < sizeof(VAPictureParameterBufferHEVC))
            return FALSE;
        memcpy(&h->pp, data, sizeof(h->pp));
        /* a range-extension profile's buffer carries its parameters right after the base ones */
        if (size >= sizeof(VAPictureParameterBufferHEVC) + sizeof(VAPictureParameterBufferHEVCRext))
            memcpy(&h->rext, (UBYTE *)data + sizeof(VAPictureParameterBufferHEVC), sizeof(h->rext));
        else
            memset(&h->rext, 0, sizeof(h->rext));
        h->have_pp = TRUE;
        return TRUE;

    case VAIQMatrixBufferType:
        if (size < sizeof(VAIQMatrixBufferHEVC))
            return FALSE;
        memcpy(&h->iq, data, sizeof(h->iq));
        h->have_iq = TRUE;
        return TRUE;

    case VASliceParameterBufferType:
        if (size < sizeof(VASliceParameterBufferHEVC))
            return FALSE;
        for (i = 0; i < count; i++)
        {
            const VASliceParameterBufferHEVC *sp = (const VASliceParameterBufferHEVC *)((UBYTE *)data + i * size);
            struct hevc_slice *sl;

            if (h->nslices >= NOUVEAU_DECODE_MAX_SLICES)
                return FALSE;
            sl = &h->slices[h->nslices++];
            sl->dataoff = sp->slice_data_offset;
            sl->datasize = sp->slice_data_size;
            sl->datadone = 0;
            sl->type = sp->LongSliceFlags.fields.slice_type;
        }
        return TRUE;

    default:
        /* range-extension / SCC buffers and others: nothing for Main/Main10; refusing aborts the picture */
        return TRUE;
    }
}

/* each slice: 00 00 01 + the NAL as VA delivers it (header included) */
static BOOL hevc_bitstream(struct HIDDNouveauDecode *dec, APTR data, ULONG size)
{
    struct hevc_state *h = dec->cur.codecstate;
    ULONG i;

    if (h->nextdata >= h->nslices)
    {
        struct hevc_slice *sl;

        if (h->nslices >= NOUVEAU_DECODE_MAX_SLICES)
            return FALSE;
        sl = &h->slices[h->nslices++];
        sl->dataoff = 0;
        sl->datasize = size;
        sl->datadone = 0;
        sl->type = 0;
    }
    for (i = h->nextdata; i < h->nslices; i++)
    {
        struct hevc_slice *sl = &h->slices[i];
        UBYTE *p;

        if (sl->datadone)
            continue;
        if ((sl->dataoff > size) || !sl->datasize || (sl->datasize > size - sl->dataoff))
            return FALSE;
        p = HIDDNouveauDecodeBitstreamReserve(dec, 3 + sl->datasize);
        if (!p)
            return FALSE;
        sl->offset = (ULONG)(p - (UBYTE *)dec->ring[dec->ringpos].bitstream->map);
        p[0] = 0;
        p[1] = 0;
        p[2] = 1;
        memcpy(p + 3, (UBYTE *)data + sl->dataoff, sl->datasize);
        sl->datadone = 1;
        dec->cur.nslices++;
    }
    h->nextdata = h->nslices;
    return TRUE;
}

/* the 32-bit ref POC difference, clipped as the spec clips td/tb */
static short hevc_pocdiff(LONG cur, LONG ref)
{
    LONG d = cur - ref;

    return (short)((d < -128) ? -128 : (d > 127) ? 127 : d);
}

/* G2 order: each matrix written column-major from VA's raster lists */
static VOID hevc_scaling(struct hevc_state *h, UBYTE *buf)
{
    const VAIQMatrixBufferHEVC *iq = &h->iq;
    ULONG m, j, k;
    UBYTE *p;

    memset(buf, 16, 1008);
    if (!h->have_iq)
        return;
    memcpy(buf + 0, iq->ScalingListDC16x16, 6);
    memcpy(buf + 6, iq->ScalingListDC32x32, 2);
    memset(buf + 8, 0, 8);
    p = buf + 0x10;
    for (m = 0; m < 6; m++)
        for (j = 0; j < 4; j++)
            for (k = 0; k < 4; k++)
                *p++ = iq->ScalingList4x4[m][4 * k + j];
    for (m = 0; m < 6; m++)
        for (j = 0; j < 8; j++)
            for (k = 0; k < 8; k++)
                *p++ = iq->ScalingList8x8[m][8 * k + j];
    for (m = 0; m < 6; m++)
        for (j = 0; j < 8; j++)
            for (k = 0; k < 8; k++)
                *p++ = iq->ScalingList16x16[m][8 * k + j];
    for (m = 0; m < 2; m++)
        for (j = 0; j < 8; j++)
            for (k = 0; k < 8; k++)
                *p++ = iq->ScalingList32x32[m][8 * k + j];
}

/* {width, height} in CTBs per tile, raster order */
static VOID hevc_tiles(struct hevc_state *h, UWORD *t)
{
    const VAPictureParameterBufferHEVC *pp = &h->pp;
    ULONG ctb = 1UL << (pp->log2_min_luma_coding_block_size_minus3 + 3 + pp->log2_diff_max_min_luma_coding_block_size);
    ULONG wc = (pp->pic_width_in_luma_samples + ctb - 1) / ctb;
    ULONG hc = (pp->pic_height_in_luma_samples + ctb - 1) / ctb;
    ULONG cols = 1, rows = 1, c, r, used, colw[20], rowh[22];

    memset(t, 0, 1776);
    if (pp->pic_fields.bits.tiles_enabled_flag)
    {
        cols = pp->num_tile_columns_minus1 + 1;
        rows = pp->num_tile_rows_minus1 + 1;
        if (cols > 20)
            cols = 20;
        if (rows > 22)
            rows = 22;
    }
    for (c = 0, used = 0; c + 1 < cols; c++)
        used += (colw[c] = pp->column_width_minus1[c] + 1);
    colw[cols - 1] = wc - used;
    for (r = 0, used = 0; r + 1 < rows; r++)
        used += (rowh[r] = pp->row_height_minus1[r] + 1);
    rowh[rows - 1] = hc - used;
    for (r = 0; r < rows; r++)
        for (c = 0; c < cols; c++)
        {
            *t++ = colw[c];
            *t++ = rowh[r];
        }
}

static ULONG hevc_status(struct HIDDNouveauDecode *dec, ULONG *status)
{
    nvdec_status_s *st = (nvdec_status_s *)status;
    struct hevc_state *hs = dec->cur.codecstate;

    if (!st->error_status && !st->mbs_in_error && !st->slice_header_error_code)
        return 0;
    /* the GB207's benign 0x20, as for H.264, when nothing is in error */
    if ((st->error_status == 0x20) && !st->mbs_in_error && !st->slice_header_error_code)
        return 0;
    if (dec->errors++ < 4)
        nvlog("[Nouveau] NVDEC: HEVC frame error %08lx (ok %lu, in error %lu, ctb %08lx, slice %08lx, header %lu)\n",
            (unsigned long)st->error_status, (unsigned long)st->mbs_correctly_decoded,
            (unsigned long)st->mbs_in_error, (unsigned long)st->hevc.error_ctb_pos,
            (unsigned long)st->hevc.error_slice_pos, (unsigned long)st->slice_header_error_code);
    return st->error_status ? st->error_status : 1;
}

static BOOL hevc_submit(struct HIDDNouveauDecode *dec, struct HIDDNouveauDecodeFrame *f)
{
    struct hevc_state *h = dec->cur.codecstate;
    const VAPictureParameterBufferHEVC *pp;
    nvdec_hevc_pic_s *pic;
    UBYTE *base;
    ULONG *sliceoffs;
    LONG refslot[15], slot, i, k;
    UBYTE before[16], after[16], lt[16], l0[48], l1[48];
    LONG nb = 0, na = 0, nl = 0, n0 = 0, n1 = 0;
    ULONG avoid = 0, allintra = 1, bitdepth;
    UBYTE *eos;
    struct nouveau_bo *extra[4];

    if (!h || !h->have_pp || !dec->cur.nslices)
        return FALSE;
    pp = &h->pp;
    bitdepth = pp->bit_depth_luma_minus8 + 8;
    /* the chroma format must be the target's: 4:2:0 half-height, 4:2:2 full, 4:4:4 full and twice as wide */
    if ((pp->pic_fields.bits.chroma_format_idc < 1) || (pp->pic_fields.bits.chroma_format_idc > 3) ||
        ((pp->pic_fields.bits.chroma_format_idc == 1) != (dec->chalf != 0)) ||
        ((pp->pic_fields.bits.chroma_format_idc == 3) != (dec->cwide == 2)) || (bitdepth > 12) ||
        ((bitdepth > 8) != (dec->bpp == 2)) ||
        (pp->pic_width_in_luma_samples > dec->width) || (pp->pic_height_in_luma_samples > dec->height))
    {
        nvlog("[Nouveau] NVDEC: HEVC stream (%ux%u, %lu-bit, chroma %u) does not fit the session\n",
            pp->pic_width_in_luma_samples, pp->pic_height_in_luma_samples, (unsigned long)bitdepth,
            pp->pic_fields.bits.chroma_format_idc);
        return FALSE;
    }
    for (i = 0; i < (LONG)h->nslices; i++)
        if (h->slices[i].type != 2)
            allintra = 0;

    /* references: every valid VA reference to the pool slot it occupies */
    for (i = 0; i < 15; i++)
    {
        const VAPictureHEVC *ref = &pp->ReferenceFrames[i];
        OOP_Object *bm;
        ULONG buffer;

        refslot[i] = -1;
        if ((ref->flags & VA_PICTURE_HEVC_INVALID) || (ref->picture_id == VA_INVALID_SURFACE))
            continue;
        if (!HIDDNouveauDecodeResolve(dec, ref->picture_id, &bm, &buffer))
            continue;
        slot = HIDDNouveauDecodeFindSlot(dec, bm, buffer);
        if (slot < 0)
            continue;
        refslot[i] = slot;
        avoid |= 1UL << slot;
        dec->slot[slot].lastuse = dec->frames;
    }
    slot = HIDDNouveauDecodeAllocSlot(dec, dec->cur.bm, dec->cur.buffer, avoid);
    if (slot < 0)
        return FALSE;
    dec->cur.slot = slot;

    base = (UBYTE *)f->picsetup->map;
    pic = (nvdec_hevc_pic_s *)base;
    sliceoffs = (ULONG *)(base + NOUVEAU_DECODE_SLICEOFFS_OFFSET);
    memset(pic, 0, NOUVEAU_DECODE_SLICEOFFS_OFFSET);  /* the whole page: the engine may read past the struct */
    memset(base + NOUVEAU_DECODE_STATUS_OFFSET, 0, 256);

    /*
     * bitstream: the slices (start code + NAL) and the read-ahead pad, as
     * NVIDIA's driver passes them. No end-of-sequence NAL: after one the
     * next picture must be an IRAP and every reference is dropped, so each
     * P/B picture failed (the engine faulted; intra pictures were fine).
     */
    for (i = 0; i < (LONG)h->nslices; i++)
        sliceoffs[i] = h->slices[i].offset;
    sliceoffs[h->nslices] = dec->cur.bitstreamlen;
    pic->stream_len = dec->cur.bitstreamlen;
    eos = HIDDNouveauDecodeBitstreamReserve(dec, 256);
    if (!eos)
        return FALSE;
    memset(eos, 0, 256);

    pic->tileformat = NVDEC_TILEFORMAT_KBL;
    pic->gob_height = NVDEC_GOB_HEIGHT_2;
    pic->sw_start_code_e = 1;
    pic->disp_output_mode = 0;                       /* native 10-bit P010 output (NVIDIA's driver: 0) */
    pic->framestride[0] = dec->pitch / dec->bpp;     /* in samples */
    pic->framestride[1] = dec->pitch * dec->cwide / dec->bpp;     /* 4:4:4: a U/V pair per pixel */
    pic->colMvBuffersize = h->colmvsize >> 8;
    pic->HevcSaoBufferOffset = HEVC_FLT_SAO_LEFT * h->fltscale;
    pic->HevcBsdCtrlOffset = HEVC_FLT_BSD_CTRL * h->fltscale;
    pic->gptimer_timeout_value = 0x4dd1e00;         /* NVIDIA's */

    pic->pic_width_in_luma_samples = pp->pic_width_in_luma_samples;
    pic->pic_height_in_luma_samples = pp->pic_height_in_luma_samples;
    pic->chroma_format_idc = pp->pic_fields.bits.chroma_format_idc;
    pic->bit_depth_luma = bitdepth;
    pic->bit_depth_chroma = pp->bit_depth_chroma_minus8 + 8;
    pic->log2_min_luma_coding_block_size = pp->log2_min_luma_coding_block_size_minus3 + 3;
    pic->log2_max_luma_coding_block_size = pic->log2_min_luma_coding_block_size + pp->log2_diff_max_min_luma_coding_block_size;
    pic->log2_min_transform_block_size = pp->log2_min_transform_block_size_minus2 + 2;
    pic->log2_max_transform_block_size = pic->log2_min_transform_block_size + pp->log2_diff_max_min_transform_block_size;
    pic->max_transform_hierarchy_depth_inter = pp->max_transform_hierarchy_depth_inter;
    pic->max_transform_hierarchy_depth_intra = pp->max_transform_hierarchy_depth_intra;
    pic->scalingListEnable = pp->pic_fields.bits.scaling_list_enabled_flag;
    pic->amp_enable_flag = pp->pic_fields.bits.amp_enabled_flag;
    pic->sample_adaptive_offset_enabled_flag = pp->slice_parsing_fields.bits.sample_adaptive_offset_enabled_flag;
    pic->pcm_enabled_flag = pp->pic_fields.bits.pcm_enabled_flag;
    if (pp->pic_fields.bits.pcm_enabled_flag)
    {
        pic->pcm_sample_bit_depth_luma = pp->pcm_sample_bit_depth_luma_minus1 + 1;
        pic->pcm_sample_bit_depth_chroma = pp->pcm_sample_bit_depth_chroma_minus1 + 1;
        pic->log2_min_pcm_luma_coding_block_size = pp->log2_min_pcm_luma_coding_block_size_minus3 + 3;
        pic->log2_max_pcm_luma_coding_block_size = pic->log2_min_pcm_luma_coding_block_size +
                                                   pp->log2_diff_max_min_pcm_luma_coding_block_size;
    }
    pic->pcm_loop_filter_disabled_flag = pp->pic_fields.bits.pcm_loop_filter_disabled_flag;
    pic->sps_temporal_mvp_enabled_flag = pp->slice_parsing_fields.bits.sps_temporal_mvp_enabled_flag;
    pic->strong_intra_smoothing_enabled_flag = pp->pic_fields.bits.strong_intra_smoothing_enabled_flag;

    pic->dependent_slice_segments_enabled_flag = pp->slice_parsing_fields.bits.dependent_slice_segments_enabled_flag;
    pic->output_flag_present_flag = pp->slice_parsing_fields.bits.output_flag_present_flag;
    pic->num_extra_slice_header_bits = pp->num_extra_slice_header_bits;
    pic->sign_data_hiding_enabled_flag = pp->pic_fields.bits.sign_data_hiding_enabled_flag;
    pic->cabac_init_present_flag = pp->slice_parsing_fields.bits.cabac_init_present_flag;
    pic->num_ref_idx_l0_default_active = pp->num_ref_idx_l0_default_active_minus1 + 1;
    pic->num_ref_idx_l1_default_active = pp->num_ref_idx_l1_default_active_minus1 + 1;
    pic->init_qp = (26 + pp->init_qp_minus26 + 6 * pp->bit_depth_luma_minus8) & 0x7f;   /* with QpBdOffsetY, as NVIDIA's driver */
    pic->constrained_intra_pred_flag = pp->pic_fields.bits.constrained_intra_pred_flag;
    pic->transform_skip_enabled_flag = pp->pic_fields.bits.transform_skip_enabled_flag;
    pic->cu_qp_delta_enabled_flag = pp->pic_fields.bits.cu_qp_delta_enabled_flag;
    pic->diff_cu_qp_delta_depth = pp->diff_cu_qp_delta_depth;
    pic->pps_cb_qp_offset = pp->pps_cb_qp_offset;
    pic->pps_cr_qp_offset = pp->pps_cr_qp_offset;
    pic->pps_beta_offset = 2 * pp->pps_beta_offset_div2;
    pic->pps_tc_offset = 2 * pp->pps_tc_offset_div2;
    pic->pps_slice_chroma_qp_offsets_present_flag = pp->slice_parsing_fields.bits.pps_slice_chroma_qp_offsets_present_flag;
    pic->weighted_pred_flag = pp->pic_fields.bits.weighted_pred_flag;
    pic->weighted_bipred_flag = pp->pic_fields.bits.weighted_bipred_flag;
    pic->transquant_bypass_enabled_flag = pp->pic_fields.bits.transquant_bypass_enabled_flag;
    pic->tiles_enabled_flag = pp->pic_fields.bits.tiles_enabled_flag;
    pic->entropy_coding_sync_enabled_flag = pp->pic_fields.bits.entropy_coding_sync_enabled_flag;
    pic->num_tile_columns = pp->pic_fields.bits.tiles_enabled_flag ? pp->num_tile_columns_minus1 + 1 : 0;
    pic->num_tile_rows = pp->pic_fields.bits.tiles_enabled_flag ? pp->num_tile_rows_minus1 + 1 : 0;
    pic->loop_filter_across_tiles_enabled_flag = pp->pic_fields.bits.loop_filter_across_tiles_enabled_flag;
    pic->loop_filter_across_slices_enabled_flag = pp->pic_fields.bits.pps_loop_filter_across_slices_enabled_flag;
    pic->deblocking_filter_control_present_flag =
        pp->slice_parsing_fields.bits.deblocking_filter_override_enabled_flag ||
        pp->slice_parsing_fields.bits.pps_disable_deblocking_filter_flag ||
        pp->pps_beta_offset_div2 || pp->pps_tc_offset_div2;
    pic->deblocking_filter_override_enabled_flag = pp->slice_parsing_fields.bits.deblocking_filter_override_enabled_flag;
    pic->pps_deblocking_filter_disabled_flag = pp->slice_parsing_fields.bits.pps_disable_deblocking_filter_flag;
    pic->lists_modification_present_flag = pp->slice_parsing_fields.bits.lists_modification_present_flag;
    pic->log2_parallel_merge_level = pp->log2_parallel_merge_level_minus2 + 2;
    pic->slice_segment_header_extension_present_flag =
        pp->slice_parsing_fields.bits.slice_segment_header_extension_present_flag;

    /* reference lists, by picture slot (the hardware applies list modifications itself) */
    for (i = 0; i < 15; i++)
    {
        const VAPictureHEVC *ref = &pp->ReferenceFrames[i];

        if (refslot[i] < 0)
            continue;
        pic->RefDiffPicOrderCnts[refslot[i]] = hevc_pocdiff(pp->CurrPic.pic_order_cnt, ref->pic_order_cnt);
        if (ref->flags & VA_PICTURE_HEVC_LONG_TERM_REFERENCE)
            pic->longtermflag |= 0x8000 >> refslot[i];
        if (ref->flags & VA_PICTURE_HEVC_RPS_ST_CURR_BEFORE)
            before[nb++] = i;
        else if (ref->flags & VA_PICTURE_HEVC_RPS_ST_CURR_AFTER)
            after[na++] = i;
        else if (ref->flags & VA_PICTURE_HEVC_RPS_LT_CURR)
            lt[nl++] = i;
    }
    /* StCurrBefore by POC descending, StCurrAfter ascending */
    for (i = 1; i < nb; i++)
        for (k = i; k > 0 && pp->ReferenceFrames[before[k]].pic_order_cnt > pp->ReferenceFrames[before[k - 1]].pic_order_cnt; k--)
        { UBYTE t = before[k]; before[k] = before[k - 1]; before[k - 1] = t; }
    for (i = 1; i < na; i++)
        for (k = i; k > 0 && pp->ReferenceFrames[after[k]].pic_order_cnt < pp->ReferenceFrames[after[k - 1]].pic_order_cnt; k--)
        { UBYTE t = after[k]; after[k] = after[k - 1]; after[k - 1] = t; }
    for (i = 0; i < nb; i++) l0[n0++] = refslot[before[i]];
    for (i = 0; i < na; i++) l0[n0++] = refslot[after[i]];
    for (i = 0; i < nl; i++) l0[n0++] = refslot[lt[i]];
    for (i = 0; i < na; i++) l1[n1++] = refslot[after[i]];
    for (i = 0; i < nb; i++) l1[n1++] = refslot[before[i]];
    for (i = 0; i < nl; i++) l1[n1++] = refslot[lt[i]];
    for (k = 0; k < 16; k++)
    {
        pic->initreflistidxl0[k] = n0 ? l0[k % n0] : slot;
        pic->initreflistidxl1[k] = n1 ? l1[k % n1] : slot;
    }
    pic->num_ref_frames = n0 ? n0 : 1;
    pic->RefDiffPicOrderCnts[slot] = 0;
    pic->IDR_picture_flag = pp->slice_parsing_fields.bits.IdrPicFlag;
    pic->RAP_picture_flag = pp->slice_parsing_fields.bits.RapPicFlag;
    pic->curr_pic_idx = slot;

    pic->separate_colour_plane_flag = pp->pic_fields.bits.separate_colour_plane_flag;
    pic->log2_max_pic_order_cnt_lsb_minus4 = pp->log2_max_pic_order_cnt_lsb_minus4;
    pic->num_short_term_ref_pic_sets = pp->num_short_term_ref_pic_sets;
    pic->num_long_term_ref_pics_sps = pp->num_long_term_ref_pic_sps;
    pic->long_term_ref_pics_present_flag = pp->slice_parsing_fields.bits.long_term_ref_pics_present_flag;
    pic->num_bits_short_term_ref_pics_in_slice = pp->st_rps_bits;
    /*
     * The slice-header bits the driver is expected to have parsed (output
     * flag, colour plane, POC LSB, the reference picture sets): the engine
     * skips them. Left at 0, every non-IDR frame failed with an unhandled
     * interrupt within ~600 cycles (GB207). Same rule as Linux' Hantro G2
     * driver (compute_header_skip_length), the engine's ancestor.
     */
    {
        ULONG skip = 0;

        if (pp->slice_parsing_fields.bits.output_flag_present_flag)
            skip += 1;
        if (pp->pic_fields.bits.separate_colour_plane_flag)
            skip += 2;
        if (!pp->slice_parsing_fields.bits.IdrPicFlag)
        {
            skip += pp->log2_max_pic_order_cnt_lsb_minus4 + 4;
            skip += 1;                                  /* short_term_ref_pic_set_sps_flag */
            if (pp->st_rps_bits)
                skip += pp->st_rps_bits;                /* the slice's own st_ref_pic_set() */
            else if (pp->num_short_term_ref_pic_sets > 1)
            {
                ULONG n = pp->num_short_term_ref_pic_sets - 1, bits = 0;

                while (n)
                {
                    bits++;
                    n >>= 1;
                }
                skip += bits;                           /* short_term_ref_pic_set_idx */
            }
            /* long-term part: VA does not carry its length (needs a slice-header parse) */
            if (pp->slice_parsing_fields.bits.long_term_ref_pics_present_flag && !h->ltwarned)
            {
                h->ltwarned = TRUE;
                nvlog("[Nouveau] NVDEC: HEVC stream signals long-term references: not handled yet\n");
            }
        }
        pic->sw_hdr_skip_length = skip;
    }
    {
        nvdec_hevc_main10_444_ext_s *x = &pic->v1.hevc_main10_444_ext;
        const VAPictureParameterBufferHEVCRext *r = &h->rext;

        /* the range extension's tools (zero for Main / Main10) */
        x->transformSkipRotationEnableFlag = r->range_extension_pic_fields.bits.transform_skip_rotation_enabled_flag;
        x->transformSkipContextEnableFlag = r->range_extension_pic_fields.bits.transform_skip_context_enabled_flag;
        x->implicitRdpcmEnableFlag = r->range_extension_pic_fields.bits.implicit_rdpcm_enabled_flag;
        x->explicitRdpcmEnableFlag = r->range_extension_pic_fields.bits.explicit_rdpcm_enabled_flag;
        x->extendedPrecisionProcessingFlag = r->range_extension_pic_fields.bits.extended_precision_processing_flag;
        x->intraSmoothingDisabledFlag = r->range_extension_pic_fields.bits.intra_smoothing_disabled_flag;
        x->highPrecisionOffsetsEnableFlag = r->range_extension_pic_fields.bits.high_precision_offsets_enabled_flag;
        x->fastRiceAdaptationEnableFlag = r->range_extension_pic_fields.bits.persistent_rice_adaptation_enabled_flag;
        x->cabacBypassAlignmentEnableFlag = r->range_extension_pic_fields.bits.cabac_bypass_alignment_enabled_flag;
        x->log2MaxTransformSkipSize = r->log2_max_transform_skip_block_size_minus2 + 2;
        x->crossComponentPredictionEnableFlag = r->range_extension_pic_fields.bits.cross_component_prediction_enabled_flag;
        x->chromaQpAdjustmentEnableFlag = r->range_extension_pic_fields.bits.chroma_qp_offset_list_enabled_flag;
        if (x->chromaQpAdjustmentEnableFlag)
        {
            x->diffCuChromaQpAdjustmentDepth = r->diff_cu_chroma_qp_offset_depth;
            x->chromaQpAdjustmentTableSize = r->chroma_qp_offset_list_len_minus1 + 1;
            memcpy(x->cb_qp_adjustment, r->cb_qp_offset_list, 6);
            memcpy(x->cr_qp_adjustment, r->cr_qp_offset_list, 6);
        }
        x->log2SaoOffsetScaleLuma = r->log2_sao_offset_scale_luma;
        x->log2SaoOffsetScaleChroma = r->log2_sao_offset_scale_chroma;
    }
    pic->v1.hevc_main10_444_ext.HevcFltAboveOffset = HEVC_FLT_DBLK_ABOVE * h->fltscale;
    pic->v1.hevc_main10_444_ext.HevcSaoAboveOffset = HEVC_FLT_SAO_ABOVE * h->fltscale;
    pic->v3.HevcSliceEdgeOffset = HEVC_FLT_SLICE_EDGE * h->fltscale;
    /* error concealment hints as NVIDIA sets them: co-located MVs, slice type P for intra pictures */
    if (h->blkbuf)
    {
        /* Blackwell's additions, as NVIDIA's driver fills them for the CFB0 class (traced) */
        pic->reserved6 = (dec->bpp == 2) ? 2 : 9;
        /* 3 x (8, 10, 12-bit: 0, 1, 2) + (4:2:0, 4:2:2, 4:4:4: 0, 1, 2), traced */
        pic->reserved7 = 3 * ((bitdepth - 8) / 2) + (pp->pic_fields.bits.chroma_format_idc - 1);
        *(ULONG *)((UBYTE *)pic + 0x11c) = NVDEC_BLK_UNITS(pp->pic_width_in_luma_samples);
        *(UWORD *)((UBYTE *)pic + 0x120) = 0xffff;
    }
    pic->v3.slice_ec_mv_type = 1;
    pic->v3.slice_ec_slice_type = allintra ? 1 : 0;

    hevc_scaling(h, base + HEVC_SCALING_OFFSET);
    hevc_tiles(h, (UWORD *)(base + HEVC_TILES_OFFSET));
    nouveau_staging_to_gpu(f->picsetup->map, NOUVEAU_DECODE_PICSETUP_SIZE);
    nouveau_staging_to_gpu(f->bitstream->map, dec->cur.bitstreamlen);

    {
        ULONG m[2 * 5];
        ULONG nm;

        /* NVIDIA's set: nothing else (no SAO / slice-info / scratch / generic tile or filter methods) */
        m[0] = NVDEC_HEVC_SET_SCALING_LIST_OFFSET;  m[1] = (ULONG)((f->picsetup->offset + HEVC_SCALING_OFFSET) >> 8);
        m[2] = NVDEC_HEVC_SET_TILE_SIZES_OFFSET;    m[3] = (ULONG)((f->picsetup->offset + HEVC_TILES_OFFSET) >> 8);
        m[4] = NVDEC_HEVC_SET_FILTER_BUFFER_OFFSET; m[5] = (ULONG)(h->filter->offset >> 8);
        m[6] = NVDEC_SET_INTRA_TOP_BUF_OFFSET;      m[7] = (ULONG)(h->intratop->offset >> 8);
        nm = 4;
        if (h->blkbuf)
        {
            m[8] = NVDEC_SET_BLK_BUF_OFFSET;        m[9] = (ULONG)(h->blkbuf->offset >> 8);
            nm = 5;
        }
        extra[0] = h->filter;
        extra[1] = h->intratop;
        extra[2] = h->blkbuf;
        /* NVIDIA's control word: gptimer, error concealment, mbtimer, event trace, PROD_RUN; no all-intra */
        if (!HIDDNouveauDecodeEmit(dec, f, slot, NULL, NVDEC_APPLICATION_ID_HEVC,
                                   NVDEC_CONTROL_CODEC_HEVC | 0x52050,
                                   extra, h->blkbuf ? 3 : 2, m, nm))
            return FALSE;
    }
    if (!h->firstlogged)
    {
        h->firstlogged = TRUE;
        D(bug("[Nouveau] NVDEC: HEVC first frame %ux%u %lu-bit, slot %ld, %ld refs\n",
            pp->pic_width_in_luma_samples, pp->pic_height_in_luma_samples, (unsigned long)bitdepth,
            (long)slot, (long)n0));
    }
    return TRUE;
}

const struct HIDDNouveauDecodeOps HIDDNouveauDecodeHEVCOps =
{
    hevc_open, hevc_close, hevc_begin, hevc_params, hevc_bitstream, hevc_submit, hevc_status
};
