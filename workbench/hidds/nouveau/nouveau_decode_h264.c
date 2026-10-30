/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    H.264 on NVDEC: VA-API picture, matrix and slice parameters become the
    decoder's picture setup (nvdec_h264_pic_s, NVIDIA's MIT header in Mesa's
    nouveau/headers tree), the slices an Annex B bitstream with an offset
    table, and the frame one EXECUTE on the session's channel.

    NVDEC is a full VLD decoder: it parses the slice headers itself, so the
    reference picture lists, weights and QP deltas of the VA slice parameters
    are not needed; the DPB description (frame numbers, POCs, markings) is.
*/

#include "nouveau_intern.h"
#include "nouveau_decode.h"
#include "nouveau_vadec.h"

#define DEBUG 0
#include <aros/debug.h>
#include <proto/exec.h>
#include <string.h>

#include <nvidia/video/nvdec_drv.h>

/* Class methods (NVC9B0 numbering; unchanged since NVC5B0, see NVDEC-H264-REFERENCE.md) */
#define NVDEC_SET_APPLICATION_ID            0x0200
#define NVDEC_SEMAPHORE_A                   0x0240
#define NVDEC_SEMAPHORE_B                   0x0244
#define NVDEC_SEMAPHORE_C                   0x0248
#define NVDEC_EXECUTE                       0x0300
#define NVDEC_SEMAPHORE_D                   0x0304
#define NVDEC_SET_CONTROL_PARAMS            0x0400
#define NVDEC_SET_DRV_PIC_SETUP_OFFSET      0x0404
#define NVDEC_SET_IN_BUF_BASE_OFFSET        0x0408
#define NVDEC_SET_PICTURE_INDEX             0x040C
#define NVDEC_SET_SLICE_OFFSETS_BUF_OFFSET  0x0410
#define NVDEC_SET_COLOC_DATA_OFFSET         0x0414
#define NVDEC_SET_HISTORY_OFFSET            0x0418
#define NVDEC_SET_DISPLAY_BUF_SIZE          0x041C
#define NVDEC_SET_HISTOGRAM_OFFSET          0x0420
#define NVDEC_SET_NVDEC_STATUS_OFFSET       0x0424
#define NVDEC_SET_PICTURE_LUMA_OFFSET0      0x0430  /* 17 of them */
#define NVDEC_SET_PICTURE_CHROMA_OFFSET0    0x0474  /* 17 of them */
#define NVDEC_SET_PIC_SCRATCH_BUF_OFFSET    0x04B8
#define NVDEC_H264_SET_MBHIST_BUF_OFFSET    0x0500
#define NVDEC_SET_INTRA_TOP_BUF_OFFSET      0x04C8
#define NVDEC_SET_FILTER_BUFFER_OFFSET      0x04D0
#define NVDEC_H264_SET_SCALING_LIST_OFFSET  0x0680  /* Blackwell's H.264 application: the matrices in the setup */

#define NVDEC_APPLICATION_ID_H264           3
#define NVDEC_APPLICATION_ID_H264_BW        8       /* Blackwell's H.264 application: high bit depth */
#define NVDEC_CONTROL_CODEC_H264            3
#define NVDEC_CONTROL_GPTIMER_ON            (1 << 4)
#define NVDEC_CONTROL_ALL_INTRA_FRAME       (1 << 17)
#define NVDEC_EXECUTE_NOTIFY_ON_END         (1 << 0)
#define NVDEC_EXECUTE_AWAKEN                (1 << 8)
/* SEMAPHORE_D: RELEASE, one 32-bit word, flush enabled (writes visible first), awaken */
#define NVDEC_SEMAPHORE_D_RELEASE_FLUSH     (1 << 8)

#define NVDEC_H264_SLOTS                    17
#define NVDEC_H264_HISTORY_SIZE             (2 * 1024 * 1024)
#define NVDEC_H264_COLOC_PER_MB             192

/* The picture layout the pool uses: KBL block-linear, 2 GOBs (16 rows) per block */
#define NVDEC_TILEFORMAT_KBL                1
#define NVDEC_GOB_HEIGHT_2                  0

struct nvdec_h264_slice
{
    ULONG   offset;         /* of the start code in the bitstream */
    UBYTE   type;           /* slice_type % 5 */
    UBYTE   datadone;       /* its bytes are in the bitstream */
    UBYTE   l0, l1;         /* num_ref_idx_lX_active_minus1 of the slice */
    ULONG   dataoff, datasize;  /* into the VA data buffer it belongs to */
};

struct nvdec_h264
{
    VAPictureParameterBufferH264    pp;
    VAIQMatrixBufferH264            iq;
    BOOL    have_pp, have_iq;
    LONG    defrefs[2];             /* PPS num_ref_idx_lX_default_active_minus1 as last seen */
    struct nvdec_h264_slice slices[NOUVEAU_DECODE_MAX_SLICES];
    ULONG   nslices;                /* parameters received */
    ULONG   nextdata;               /* first slice whose data has not arrived */
    BYTE    dpbpos[NOUVEAU_DECODE_SLOTS];   /* pool slot -> its DPB entry while it is a reference, or -1 */
    UBYTE   slotfields[NOUVEAU_DECODE_SLOTS];   /* the slot's picture was coded as fields */
    BYTE    fieldslot;                      /* slot the last field picture went to, or -1 */
    BYTE    fieldbottom;                    /* and its parity */
    UWORD   fieldframe;                     /* and its frame_num */
    struct nouveau_bo *filter, *intratop, *blkbuf;  /* Blackwell's H.264 application */
};

static BOOL h264_open(struct HIDDNouveauDecode *dec)
{
    struct nvdec_h264 *h;
    ULONG mbs = (dec->width / 16) * (dec->height / 16);
    ULONG colocsize = (mbs * NVDEC_H264_COLOC_PER_MB + 4095) & ~4095UL;
    LONG ret;

    h = AllocMem(sizeof(*h), MEMF_PUBLIC | MEMF_CLEAR);
    if (!h)
        return FALSE;
    dec->cur.codecstate = h;
    h->defrefs[0] = h->defrefs[1] = 0;
    memset(h->dpbpos, -1, sizeof(h->dpbpos));
    h->fieldslot = -1;
    /* interlaced streams: room for two fields of whole 16-row blocks */
    dec->rowalign = 64;
    ret = nouveau_bo_new(dec->carddata->dev, NOUVEAU_BO_VRAM, 4096, (uint64_t)colocsize * NVDEC_H264_SLOTS, NULL, &dec->coloc);
    if (ret)
    {
        nvlog("[Nouveau] NVDEC: H.264 colocated buffer (%lu KB) failed (%ld)\n",
            (unsigned long)(colocsize * NVDEC_H264_SLOTS / 1024), (long)ret);
        return FALSE;
    }
    ret = nouveau_bo_new(dec->carddata->dev, NOUVEAU_BO_VRAM, 4096, NVDEC_H264_HISTORY_SIZE, NULL, &dec->history);
    if (ret)
    {
        nvlog("[Nouveau] NVDEC: H.264 history buffer failed (%ld)\n", (long)ret);
        return FALSE;
    }
    return TRUE;
}

static VOID h264_close(struct HIDDNouveauDecode *dec)
{
    struct nvdec_h264 *h = dec->cur.codecstate;

    if (h)
    {
        if (h->filter)   nouveau_bo_ref(NULL, &h->filter);
        if (h->intratop) nouveau_bo_ref(NULL, &h->intratop);
        if (h->blkbuf)   nouveau_bo_ref(NULL, &h->blkbuf);
        FreeMem(h, sizeof(struct nvdec_h264));
        dec->cur.codecstate = NULL;
    }
}

static BOOL h264_begin(struct HIDDNouveauDecode *dec)
{
    struct nvdec_h264 *h = dec->cur.codecstate;

    if (!h)
        return FALSE;
    h->have_pp = h->have_iq = FALSE;
    h->nslices = 0;
    h->nextdata = 0;
    return TRUE;
}

/*
 * Exp-Golomb reader for the slice header prefix, only to find
 * num_ref_idx_active_override_flag: VA does not carry the PPS defaults of
 * num_ref_idx_lX_active_minus1, and a slice without the override uses them.
 */
struct bitreader
{
    const UBYTE *p, *end;
    ULONG bit;
};

static ULONG br_bit(struct bitreader *br)
{
    ULONG v;

    if (br->p >= br->end)
        return 0;
    v = (br->p[0] >> (7 - br->bit)) & 1;
    if (++br->bit == 8)
    {
        br->bit = 0;
        br->p++;
    }
    return v;
}

static ULONG br_bits(struct bitreader *br, ULONG n)
{
    ULONG v = 0;

    while (n--)
        v = (v << 1) | br_bit(br);
    return v;
}

static ULONG br_ue(struct bitreader *br)
{
    ULONG zeros = 0;

    while (!br_bit(br) && (zeros < 32) && (br->p < br->end))
        zeros++;
    if (!zeros)
        return 0;
    return ((1UL << zeros) - 1) + br_bits(br, zeros);
}

static LONG br_se(struct bitreader *br)
{
    ULONG k = br_ue(br);

    return (k & 1) ? (LONG)((k + 1) / 2) : -(LONG)(k / 2);
}

/*
 * Walk a slice header up to num_ref_idx_active_override_flag; when the flag
 * is 0 the slice's num_ref_idx_lX_active_minus1 (from VA) are the PPS
 * defaults. Returns TRUE when the defaults could be taken from this slice.
 */
static BOOL h264_slice_defaults(struct nvdec_h264 *h, const UBYTE *nal, ULONG len, ULONG l0, ULONG l1)
{
    const VAPictureParameterBufferH264 *pp = &h->pp;
    struct bitreader br;
    ULONG nal_unit_type, slice_type, field_pic = 0;

    if (len < 2)
        return FALSE;
    nal_unit_type = nal[0] & 0x1f;
    if ((nal_unit_type != 1) && (nal_unit_type != 5))
        return FALSE;
    br.p = nal + 1;
    br.end = nal + len;
    br.bit = 0;
    br_ue(&br);                                     /* first_mb_in_slice */
    slice_type = br_ue(&br) % 5;
    br_ue(&br);                                     /* pic_parameter_set_id */
    if (pp->seq_fields.bits.chroma_format_idc == 3 && 0)
        br_bits(&br, 2);                            /* colour_plane_id: separate planes only */
    br_bits(&br, pp->seq_fields.bits.log2_max_frame_num_minus4 + 4);   /* frame_num */
    if (!pp->seq_fields.bits.frame_mbs_only_flag)
    {
        field_pic = br_bit(&br);
        if (field_pic)
            br_bit(&br);                            /* bottom_field_flag */
    }
    if (nal_unit_type == 5)
        br_ue(&br);                                 /* idr_pic_id */
    if (pp->seq_fields.bits.pic_order_cnt_type == 0)
    {
        br_bits(&br, pp->seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4 + 4);
        if (pp->pic_fields.bits.pic_order_present_flag && !field_pic)
            br_se(&br);                             /* delta_pic_order_cnt_bottom */
    }
    if ((pp->seq_fields.bits.pic_order_cnt_type == 1) && !pp->seq_fields.bits.delta_pic_order_always_zero_flag)
    {
        br_se(&br);
        if (pp->pic_fields.bits.pic_order_present_flag && !field_pic)
            br_se(&br);
    }
    if (pp->pic_fields.bits.redundant_pic_cnt_present_flag)
        br_ue(&br);
    if (slice_type == 1)                            /* B */
        br_bit(&br);                                /* direct_spatial_mv_pred_flag */
    if ((slice_type == 0) || (slice_type == 3) || (slice_type == 1))    /* P, SP, B */
    {
        if (br_bit(&br))                            /* num_ref_idx_active_override_flag */
            return FALSE;
        h->defrefs[0] = l0;
        if (slice_type == 1)
            h->defrefs[1] = l1;
        return TRUE;
    }
    return FALSE;
}

static BOOL h264_params(struct HIDDNouveauDecode *dec, ULONG type, APTR data, ULONG size, ULONG count)
{
    struct nvdec_h264 *h = dec->cur.codecstate;
    ULONG i;

    if (!h)
        return FALSE;
    switch (type)
    {
    case VAPictureParameterBufferType:
        if (size < sizeof(VAPictureParameterBufferH264))
            return FALSE;
        memcpy(&h->pp, data, sizeof(h->pp));
        h->have_pp = TRUE;
        return TRUE;

    case VAIQMatrixBufferType:
        if (size < sizeof(VAIQMatrixBufferH264))
            return FALSE;
        memcpy(&h->iq, data, sizeof(h->iq));
        h->have_iq = TRUE;
        return TRUE;

    case VASliceParameterBufferType:
        if (size < sizeof(VASliceParameterBufferH264))
            return FALSE;
        for (i = 0; i < count; i++)
        {
            const VASliceParameterBufferH264 *sp = (const VASliceParameterBufferH264 *)((UBYTE *)data + i * size);
            struct nvdec_h264_slice *sl;

            if (h->nslices >= NOUVEAU_DECODE_MAX_SLICES)
            {
                nvlog("[Nouveau] NVDEC: H.264 frame with more than %lu slices\n", (unsigned long)NOUVEAU_DECODE_MAX_SLICES);
                return FALSE;
            }
            sl = &h->slices[h->nslices++];
            sl->offset = 0;
            sl->type = sp->slice_type % 5;
            sl->datadone = 0;
            sl->dataoff = sp->slice_data_offset;
            sl->datasize = sp->slice_data_size;
            sl->l0 = sp->num_ref_idx_l0_active_minus1;
            sl->l1 = sp->num_ref_idx_l1_active_minus1;
        }
        return TRUE;

    default:
        /* nothing else carries information the VLD decoder needs */
        nvlog("[Nouveau] NVDEC: H.264 parameter buffer type %lu not supported\n", (unsigned long)type);
        return FALSE;
    }
}

/*
 * Slice data: each VA data buffer carries the slices whose parameters came
 * before it (ffmpeg: one parameter buffer, then one data buffer, per slice).
 * Every slice gets a start code, as the decoder expects Annex B.
 */
static BOOL h264_bitstream(struct HIDDNouveauDecode *dec, APTR data, ULONG size)
{
    struct nvdec_h264 *h = dec->cur.codecstate;
    ULONG first = h ? h->nextdata : 0, i;

    if (!h)
        return FALSE;
    if (first >= h->nslices)
    {
        /* data without parameters: one slice, whole buffer */
        struct nvdec_h264_slice *sl;

        if (h->nslices >= NOUVEAU_DECODE_MAX_SLICES)
            return FALSE;
        sl = &h->slices[h->nslices++];
        sl->type = 0;
        sl->dataoff = 0;
        sl->datasize = size;
        sl->datadone = 0;
        sl->l0 = sl->l1 = 0;
    }
    for (i = first; i < h->nslices; i++)
    {
        struct nvdec_h264_slice *sl = &h->slices[i];
        UBYTE *p;

        if (sl->datadone)
            continue;
        if ((sl->dataoff > size) || (sl->datasize > size - sl->dataoff) || !sl->datasize)
        {
            nvlog("[Nouveau] NVDEC: H.264 slice %lu data (%lu+%lu) outside its %lu byte buffer\n",
                (unsigned long)i, (unsigned long)sl->dataoff, (unsigned long)sl->datasize, (unsigned long)size);
            return FALSE;
        }
        p = HIDDNouveauDecodeBitstreamReserve(dec, 3 + sl->datasize);
        if (!p)
            return FALSE;
        sl->offset = (ULONG)(p - (UBYTE *)dec->ring[dec->ringpos].bitstream->map);
        p[0] = 0;
        p[1] = 0;
        p[2] = 1;
        memcpy(p + 3, (UBYTE *)data + sl->dataoff, sl->datasize);
        sl->datadone = 1;
        /* the PPS reference count defaults, from a slice that does not override them */
        if (h->have_pp && (sl->type != 2) && (sl->type != 4))
            h264_slice_defaults(h, (UBYTE *)data + sl->dataoff, sl->datasize, sl->l0, sl->l1);
        dec->cur.nslices++;
    }
    h->nextdata = h->nslices;
    return TRUE;
}

/*
 * Build the picture setup and submit the frame.
 */
static VOID h264_dpb_entry(nvdec_dpb_entry_s *e, const VAPictureH264 *ref, LONG slot, BOOL fields)
{
    ULONG state;

    memset(e, 0, sizeof(*e));
    state = 0;
    if (ref->flags & VA_PICTURE_H264_TOP_FIELD)
        state |= 1;
    if (ref->flags & VA_PICTURE_H264_BOTTOM_FIELD)
        state |= 2;
    if (!state)
        state = 3;                                  /* frame reference: both fields */
    e->index = slot;
    e->col_idx = slot;
    e->state = state;
    e->is_long_term = (ref->flags & VA_PICTURE_H264_LONG_TERM_REFERENCE) ? 1 : 0;
    e->not_existing = 0;
    /* as NVIDIA's driver sets it (traced): the reference was coded as fields, not as a frame */
    e->is_field = fields ? 1 : 0;
    e->top_field_marking = (state & 1) ? (e->is_long_term ? 2 : 1) : 0;
    e->bottom_field_marking = (state & 2) ? (e->is_long_term ? 2 : 1) : 0;
    e->output_memory_layout = 0;
    /* a single field reference: both counts its own (traced) */
    e->FieldOrderCnt[0] = (state & 1) ? (unsigned int)ref->TopFieldOrderCnt : (unsigned int)ref->BottomFieldOrderCnt;
    e->FieldOrderCnt[1] = (state & 2) ? (unsigned int)ref->BottomFieldOrderCnt : (unsigned int)ref->TopFieldOrderCnt;
    e->FrameIdx = (int)ref->frame_idx;
}

static ULONG h264_status(struct HIDDNouveauDecode *dec, ULONG *status)
{
    nvdec_status_s *st = (nvdec_status_s *)status;
    ULONG mbs = (dec->width / 16) * (dec->height / 16);

    /* an interlaced stream's field pictures hold half the frame's macroblocks */
    if (dec->fields)
        mbs /= 2;
    /*
     * Blackwell's H.264 application (dec->minimal) counts no macroblocks:
     * its status has the cycle count and the error word only (seen: 5 for a
     * rejected setup), so a frame is good when that is clear.
     */
    if (dec->minimal)
    {
        if (!st->error_status)
            return 0;
        if (dec->errors++ < 4)
            nvlog("[Nouveau] NVDEC: frame error %08lx\n", (unsigned long)st->error_status);
        return st->error_status;
    }
    if (!st->error_status && !st->mbs_in_error && !st->slice_header_error_code &&
        (st->mbs_correctly_decoded >= mbs))
        return 0;
    /*
     * A status word the class headers do not list (the GB207 reports 0x20
     * on every frame) with every macroblock decoded and none in error is
     * taken as a warning: the picture is complete. Logged once per session.
     */
    if ((st->error_status < 0x100) && !st->mbs_in_error && !st->slice_header_error_code &&
        (st->mbs_correctly_decoded >= mbs))
    {
        if (!dec->statuswarned)
        {
            dec->statuswarned = TRUE;
            D(bug("[Nouveau] NVDEC: session %p: frames report status %08lx with all %lu macroblocks decoded: taken as a warning\n",
                dec, (unsigned long)st->error_status, (unsigned long)st->mbs_correctly_decoded));
        }
        return 0;
    }
    /* the first few per session in full; the total goes out when it closes */
    if (dec->errors++ < 4)
        nvlog("[Nouveau] NVDEC: frame error %08lx (mbs ok %lu of %lu, in error %lu, slice header error %lu)\n",
            (unsigned long)st->error_status, (unsigned long)st->mbs_correctly_decoded, (unsigned long)mbs,
            (unsigned long)st->mbs_in_error, (unsigned long)st->slice_header_error_code);
    return st->error_status ? st->error_status : 1;
}

/*
 * Blackwell's H.264 application (id 8), the only one with a bit depth:
 * NVIDIA's driver uses it for H.264 except MBAFF streams. Its 4 KB setup is
 * not in nvdec_drv.h; the layout below was traced from NVIDIA's driver with
 * the decoder reported as NVCFB0 (the old setup's fields, packed, plus the
 * initial reference lists the old application derived itself). Built from
 * the old setup of the same picture.
 */
#define H264BW_DPB              0x040   /* nvdec_dpb_entry_s[16], as the old setup's */
#define H264BW_LISTS            0x140   /* P list0, B list0, B list1, unused: 32 bytes each */
#define H264BW_PARAMS           0x1c0
#define H264BW_MATRICES         0x200   /* WeightScale, WeightScale8x8, as the old setup's */
#define H264BW_NOREF            0x1f    /* list entry: none; else the DPB index, 0x20 for a bottom field */

struct h264_listref
{
    UBYTE   i, top, bottom;     /* DPB index, the fields it has as references */
    LONG    key;                /* FrameNumWrap / POC / LongTermFrameIdx: the ordering */
};

static LONG h264_minpoc(LONG a, LONG b)
{
    return (a < b) ? a : b;
}

/* insertion sort of n references by key, descending or ascending */
static VOID h264_sort(struct h264_listref *r, LONG n, BOOL down)
{
    LONG i, k;

    for (i = 1; i < n; i++)
    {
        struct h264_listref t = r[i];

        for (k = i; (k > 0) && (down ? (r[k - 1].key < t.key) : (r[k - 1].key > t.key)); k--)
            r[k] = r[k - 1];
        r[k] = t;
    }
}

/* list entries for references in order: frames as they are, or fields by 8.2.4.2.5 (same parity first) */
static LONG h264_entries(UBYTE *out, LONG pos, const struct h264_listref *r, LONG n, BOOL field, BOOL bottom)
{
    LONG same = 0, opp = 0;

    if (!field)
    {
        for (same = 0; (same < n) && (pos < 32); same++)
            out[pos++] = r[same].i;
        return pos;
    }
    while (pos < 32)
    {
        while ((same < n) && !(bottom ? r[same].bottom : r[same].top))
            same++;
        while ((opp < n) && !(bottom ? r[opp].top : r[opp].bottom))
            opp++;
        if ((same >= n) && (opp >= n))
            break;
        if (same < n)
        {
            out[pos++] = r[same].i | (bottom ? 0x20 : 0);
            same++;
        }
        if ((opp < n) && (pos < 32))
        {
            out[pos++] = r[opp].i | (bottom ? 0 : 0x20);
            opp++;
        }
    }
    return pos;
}

/* H.264 8.2.4.2: the initial reference picture lists, from the setup's DPB */
static VOID h264_initial_lists(const nvdec_h264_pic_s *pic, UBYTE *lists)
{
    struct h264_listref st[16], lt[16], a[16], b[16];
    LONG nst = 0, nlt = 0, na, nb, i, n, maxfn = 1L << (pic->log2_max_frame_num_minus4 + 4), cur;
    BOOL field = pic->field_pic_flag, bottom = pic->bottom_field_flag;
    UBYTE *p0 = lists, *b0 = lists + 32, *b1 = lists + 64;

    memset(lists, H264BW_NOREF, 4 * 32);
    cur = field ? pic->CurrFieldOrderCnt[bottom ? 1 : 0] : h264_minpoc(pic->CurrFieldOrderCnt[0], pic->CurrFieldOrderCnt[1]);
    for (i = 0; i < 16; i++)
    {
        const nvdec_dpb_entry_s *e = &pic->dpb[i];
        struct h264_listref r;

        if (!e->state || (!field && (e->state != 3)))
            continue;
        r.i = i;
        r.top = e->state & 1;
        r.bottom = (e->state >> 1) & 1;
        if (e->is_long_term)
        {
            r.key = e->FrameIdx;
            lt[nlt++] = r;
            continue;
        }
        /* FrameNumWrap; the POC (of the reference fields) goes in a second pass */
        r.key = ((LONG)e->FrameIdx > (LONG)pic->frame_num) ? (LONG)e->FrameIdx - maxfn : (LONG)e->FrameIdx;
        st[nst++] = r;
    }
    h264_sort(lt, nlt, FALSE);

    /* P: short-term by FrameNumWrap, highest first, then long-term */
    memcpy(a, st, nst * sizeof(*a));
    h264_sort(a, nst, TRUE);
    n = h264_entries(p0, 0, a, nst, field, bottom);
    h264_entries(p0, n, lt, nlt, field, bottom);

    /* B: short-term by POC, before the current picture (highest first), then after (lowest first) */
    for (i = 0; i < nst; i++)
    {
        const nvdec_dpb_entry_s *e = &pic->dpb[st[i].i];

        st[i].key = (st[i].top && st[i].bottom) ? h264_minpoc(e->FieldOrderCnt[0], e->FieldOrderCnt[1]) :
                    st[i].top ? (LONG)e->FieldOrderCnt[0] : (LONG)e->FieldOrderCnt[1];
    }
    for (i = na = nb = 0; i < nst; i++)
    {
        /* fields: "less than or equal" goes with the past (the first field of the current frame) */
        if (field ? (st[i].key <= cur) : (st[i].key < cur))
            a[na++] = st[i];
        else if (field || (st[i].key > cur))
            b[nb++] = st[i];
    }
    h264_sort(a, na, TRUE);
    h264_sort(b, nb, FALSE);
    n = h264_entries(b0, 0, a, na, field, bottom);
    n = h264_entries(b0, n, b, nb, field, bottom);
    h264_entries(b0, n, lt, nlt, field, bottom);
    n = h264_entries(b1, 0, b, nb, field, bottom);
    n = h264_entries(b1, n, a, na, field, bottom);
    n = h264_entries(b1, n, lt, nlt, field, bottom);
    /* a list1 equal to list0 with more than one entry has its first two swapped */
    if ((n > 1) && !memcmp(b0, b1, 32))
    {
        UBYTE t = b1[0];

        b1[0] = b1[1];
        b1[1] = t;
    }
}

static VOID h264_setup_bw(struct HIDDNouveauDecode *dec, const VAPictureParameterBufferH264 *pp,
    const nvdec_h264_pic_s *pic, UBYTE *page, BOOL intra)
{
    ULONG *w = (ULONG *)(page + H264BW_PARAMS);
    ULONG width = (ULONG)pic->PicWidthInMbs * 16, depth = pp->bit_depth_luma_minus8 + 8;
    ULONG small = !(pic->frame_mbs_only_flag && pic->direct_8x8_inference_flag);

    memset(page, 0, H264BW_MATRICES + sizeof(pic->WeightScale) + sizeof(pic->WeightScale8x8));
    memcpy(page + H264BW_DPB, pic->dpb, sizeof(pic->dpb));
    h264_initial_lists(pic, page + H264BW_LISTS);
    memcpy(page + H264BW_MATRICES, pic->WeightScale, sizeof(pic->WeightScale));
    memcpy(page + H264BW_MATRICES + sizeof(pic->WeightScale), pic->WeightScale8x8, sizeof(pic->WeightScale8x8));
    w[0] = 0x4d3f640;                   /* the decode timeout NVIDIA's driver sets; 0 is not a default here */
    w[1] = pic->stream_len;
    w[2] = ((dec->bpp == 2) ? 2 : 9) << 14;                         /* surface code, as Blackwell HEVC */
    w[3] = pic->CurrPicIdx | (pic->CurrColIdx << 5) | (1 << 10) | (1 << 18) | ((3 * ((depth - 8) / 2)) << 19);
    w[4] = 0xffff0000 | (pic->pitch_luma / dec->bpp / 16);          /* pitches in samples */
    w[5] = pic->pitch_chroma / dec->bpp / 16;
    w[6] = depth | ((pp->bit_depth_chroma_minus8 + 8) << 4) | (pic->PicWidthInMbs << 8) |
           (pic->FrameHeightInMbs << 18) | (pic->chroma_format_idc << 28);
    w[7] = (pic->log2_max_frame_num_minus4 << 16) | (pic->log2_max_pic_order_cnt_lsb_minus4 << 20) |
           (pic->pic_order_cnt_type << 24) | (pic->delta_pic_order_always_zero_flag << 26) |
           ((ULONG)pp->num_ref_frames << 27);
    w[8] = pic->num_ref_idx_l0_active_minus1 | (pic->num_ref_idx_l1_active_minus1 << 5) |
           ((pic->chroma_qp_index_offset & 0x1f) << 10) | ((pic->second_chroma_qp_index_offset & 0x1f) << 15) |
           ((pic->pic_init_qp_minus26 & 0x7f) << 20) | (pic->pic_order_present_flag << 27) |
           (pic->deblocking_filter_control_present_flag << 28) | (pic->transform_8x8_mode_flag << 30);
    /* units: as NVIDIA's driver, a width in 128 (32 with field or 4x4 direct motion) sample columns */
    w[9] = pic->entropy_coding_mode_flag | (pic->weighted_pred_flag << 1) | (pic->weighted_bipred_idc << 2) |
           (pic->direct_8x8_inference_flag << 14) | (pic->frame_mbs_only_flag << 15) |
           (((width + (small ? 31 : 127)) / (small ? 32 : 128)) << 16) | (small << 26);
    w[10] = pic->frame_num | (pic->field_pic_flag << 16) | (pic->bottom_field_flag << 17) |
            (pic->second_field << 18) | (pic->ref_pic_flag << 19) | ((intra ? 1 : 0) << 20);
    w[11] = pic->CurrFieldOrderCnt[0];
    w[12] = pic->CurrFieldOrderCnt[1];
    w[15] = (width + 191) / 128;
}

static BOOL h264_zeroed(struct HIDDNouveauDecode *dec, ULONG size, struct nouveau_bo **bo)
{
    if (nouveau_bo_new(dec->carddata->dev, NOUVEAU_BO_VRAM | NOUVEAU_BO_MAP, 4096, size, NULL, bo) ||
        nouveau_bo_map(*bo, NOUVEAU_BO_WR, dec->carddata->client))
        return FALSE;
    memset((*bo)->map, 0, size);
    return TRUE;
}

static BOOL h264_submit(struct HIDDNouveauDecode *dec, struct HIDDNouveauDecodeFrame *f)
{
    struct nvdec_h264 *h = dec->cur.codecstate;
    const VAPictureParameterBufferH264 *pp;
    nvdec_h264_pic_s *pic;
    ULONG *sliceoffs, *status;
    struct nouveau_pushbuf *push = dec->pushbuf;
    struct nouveau_pushbuf_refn refs[6 + NVDEC_H264_SLOTS];
    LONG refslot[16], slot, i, n, nrefs = 0;
    ULONG avoid = 0, control, allintra = 1, colocsize;
    BOOL bw;
    UBYTE *pad;

    if (!h || !h->have_pp || !dec->cur.nslices)
        return FALSE;
    pp = &h->pp;
    if (((ULONG)(pp->picture_width_in_mbs_minus1 + 1) * 16 > dec->width) ||
        ((ULONG)(pp->picture_height_in_mbs_minus1 + 1) * 16 > dec->height))
    {
        nvlog("[Nouveau] NVDEC: H.264 picture %lux%lu MBs exceeds the session's %lux%lu\n",
            (unsigned long)pp->picture_width_in_mbs_minus1 + 1, (unsigned long)pp->picture_height_in_mbs_minus1 + 1,
            (unsigned long)dec->width / 16, (unsigned long)dec->height / 16);
        return FALSE;
    }
    for (i = 0; i < (LONG)h->nslices; i++)
        if ((h->slices[i].type != 2) && (h->slices[i].type != 4))
            allintra = 0;
    /*
     * High bit depth takes Blackwell's H.264 application: its own buffers
     * (sizes as NVIDIA's driver's, with room to spare) and no history buffer.
     */
    bw = (dec->oclass >= NVDEC_CLASS_BLACKWELL) && (dec->bpp == 2);
    if (bw && pp->seq_fields.bits.mb_adaptive_frame_field_flag)
    {
        /* NVIDIA's driver submits nothing for these either (traced): MBAFF is the old application's alone */
        if (!dec->errors++)
            nvlog("[Nouveau] NVDEC: high bit depth H.264 with MBAFF is not decodable\n");
        return FALSE;
    }
    if (bw && !h->filter)
    {
        if (!h264_zeroed(dec, ((dec->width / 16) + 1) * 1024, &h->filter) ||
            !h264_zeroed(dec, ((dec->width + 63) / 64) * 1024, &h->intratop) ||
            !h264_zeroed(dec, NVDEC_BLK_BUF_SIZE(dec->width), &h->blkbuf))
            return FALSE;
        if (dec->history)
            nouveau_bo_ref(NULL, &dec->history);
        dec->minimal = TRUE;
    }

    /* References: resolve each VA surface to a pool slot it still occupies */
    for (i = 0; i < 16; i++)
    {
        const VAPictureH264 *ref = &pp->ReferenceFrames[i];
        OOP_Object *bm;
        ULONG buffer;

        refslot[i] = -1;
        if ((ref->flags & VA_PICTURE_H264_INVALID) || (ref->picture_id == VA_INVALID_SURFACE))
            continue;
        if (!HIDDNouveauDecodeResolve(dec, ref->picture_id, &bm, &buffer))
        {
            D(bug("[Nouveau] NVDEC: H.264 reference surface %lu unknown\n", (unsigned long)ref->picture_id));
            continue;
        }
        slot = HIDDNouveauDecodeFindSlot(dec, bm, buffer);
        if (slot < 0)
        {
            D(bug("[Nouveau] NVDEC: H.264 reference %lu (%p/%lu) is not in the pool any more\n",
                (unsigned long)ref->picture_id, bm, (unsigned long)buffer));
            continue;
        }
        refslot[i] = slot;
        avoid |= 1UL << slot;
        dec->slot[slot].lastuse = dec->frames;
        nrefs++;
    }
    slot = HIDDNouveauDecodeAllocSlot(dec, dec->cur.bm, dec->cur.buffer, avoid);
    if (slot < 0)
        return FALSE;
    dec->cur.slot = slot;

    /* Picture setup */
    pic = (nvdec_h264_pic_s *)f->picsetup->map;
    sliceoffs = (ULONG *)((UBYTE *)f->picsetup->map + NOUVEAU_DECODE_SLICEOFFS_OFFSET);
    status = (ULONG *)((UBYTE *)f->picsetup->map + NOUVEAU_DECODE_STATUS_OFFSET);
    memset(pic, 0, NOUVEAU_DECODE_SLICEOFFS_OFFSET);  /* the whole page: the engine may read past the struct */
    memset(status, 0, 256);

    /* the bitstream: slice offsets, then a zero pad the VLD may read ahead into */
    for (i = 0; i < (LONG)h->nslices; i++)
        sliceoffs[i] = h->slices[i].offset;
    sliceoffs[h->nslices] = dec->cur.bitstreamlen;
    /*
     * End of stream: the decoder's output stage holds the last ~62
     * macroblocks of the picture until it sees one. GB207, verified
     * bit-exact against FFmpeg's software decoder over 1022 frames: with
     * the end-of-stream NAL passed through eos[] / explicitEOSPresentFlag
     * ("to pass end of stream data separately if not present in bitstream
     * surface", nvdec_drv.h) the picture is complete; without it the final
     * 16 KB of luma and 3.5 KB of chroma are never written. The pad after
     * the slices is for the VLD's read-ahead.
     */
    pad = HIDDNouveauDecodeBitstreamReserve(dec, 256);
    if (!pad)
        return FALSE;
    memset(pad, 0, 256);
    pic->stream_len = dec->cur.bitstreamlen - 256;
    pic->eos[0] = 0;
    pic->eos[1] = 0;
    pic->eos[2] = 1;
    pic->eos[3] = 0x0b;                     /* NAL unit type 11: end of stream */
    pic->explicitEOSPresentFlag = 1;
    pic->slice_count = h->nslices;

    pic->log2_max_pic_order_cnt_lsb_minus4 = pp->seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4;
    pic->delta_pic_order_always_zero_flag = pp->seq_fields.bits.delta_pic_order_always_zero_flag;
    pic->frame_mbs_only_flag = pp->seq_fields.bits.frame_mbs_only_flag;
    pic->PicWidthInMbs = pp->picture_width_in_mbs_minus1 + 1;
    pic->FrameHeightInMbs = pp->picture_height_in_mbs_minus1 + 1;
    pic->tileFormat = NVDEC_TILEFORMAT_KBL;
    pic->gob_height = NVDEC_GOB_HEIGHT_2;
    pic->entropy_coding_mode_flag = pp->pic_fields.bits.entropy_coding_mode_flag;
    pic->pic_order_present_flag = pp->pic_fields.bits.pic_order_present_flag;
    pic->num_ref_idx_l0_active_minus1 = h->defrefs[0];
    pic->num_ref_idx_l1_active_minus1 = h->defrefs[1];
    pic->deblocking_filter_control_present_flag = pp->pic_fields.bits.deblocking_filter_control_present_flag;
    pic->redundant_pic_cnt_present_flag = pp->pic_fields.bits.redundant_pic_cnt_present_flag;
    pic->transform_8x8_mode_flag = pp->pic_fields.bits.transform_8x8_mode_flag;
    pic->pitch_luma = dec->pitch;
    pic->pitch_chroma = dec->pitch;
    pic->HistBufferSize = NVDEC_H264_HISTORY_SIZE >> 8;
    pic->MbaffFrameFlag = (pp->seq_fields.bits.mb_adaptive_frame_field_flag && !pp->pic_fields.bits.field_pic_flag) ? 1 : 0;
    pic->direct_8x8_inference_flag = pp->seq_fields.bits.direct_8x8_inference_flag;
    pic->weighted_pred_flag = pp->pic_fields.bits.weighted_pred_flag;
    pic->constrained_intra_pred_flag = pp->pic_fields.bits.constrained_intra_pred_flag;
    pic->ref_pic_flag = pp->pic_fields.bits.reference_pic_flag;
    pic->field_pic_flag = pp->pic_fields.bits.field_pic_flag;
    pic->bottom_field_flag = (pp->CurrPic.flags & VA_PICTURE_H264_BOTTOM_FIELD) ? 1 : 0;
    /* the second field of a pair goes to the slot of the first, with the other parity */
    pic->second_field = (pic->field_pic_flag && (h->fieldslot == slot) && (h->fieldbottom != pic->bottom_field_flag) &&
                         (h->fieldframe == pp->frame_num)) ? 1 : 0;
    h->slotfields[slot] = pic->field_pic_flag;
    if (pic->field_pic_flag && !pic->second_field)
    {
        h->fieldslot = slot;
        h->fieldbottom = pic->bottom_field_flag;
        h->fieldframe = pp->frame_num;
    }
    else
        h->fieldslot = -1;
    pic->log2_max_frame_num_minus4 = pp->seq_fields.bits.log2_max_frame_num_minus4;
    pic->chroma_format_idc = pp->seq_fields.bits.chroma_format_idc;
    pic->pic_order_cnt_type = pp->seq_fields.bits.pic_order_cnt_type;
    /* FFmpeg's value includes QpBdOffsetY (6 per bit above 8); the engine wants the PPS field */
    pic->pic_init_qp_minus26 = pp->pic_init_qp_minus26 - 6 * pp->bit_depth_luma_minus8;
    pic->chroma_qp_index_offset = pp->chroma_qp_index_offset;
    pic->second_chroma_qp_index_offset = pp->second_chroma_qp_index_offset;
    pic->weighted_bipred_idc = pp->pic_fields.bits.weighted_bipred_idc;
    pic->CurrPicIdx = slot;
    pic->CurrColIdx = slot;
    pic->frame_num = pp->frame_num;
    pic->output_memory_layout = 0;
    if (!pp->seq_fields.bits.frame_mbs_only_flag)
    {
        /*
         * Interlaced: the surface fields as NVIDIA's driver sets them
         * (traced): bottom offsets one field's worth of whole 16-row blocks
         * in. The engine still leaves the picture in frame order.
         */
        ULONG rows = (ULONG)pic->FrameHeightInMbs * 16;

        dec->fields = TRUE;
        pic->frame_surfaces = 0;
        pic->luma_bot_offset = ((((rows / 2) + 15) & ~15UL) * dec->pitch) >> 8;
        pic->chroma_bot_offset = ((((rows / 4) + 15) & ~15UL) * dec->pitch) >> 8;
    }
    else
    {
        dec->fields = FALSE;
        pic->frame_surfaces = 1;
    }
    if (pic->field_pic_flag)
        pic->CurrFieldOrderCnt[0] = pic->CurrFieldOrderCnt[1] = pic->bottom_field_flag ?
            pp->CurrPic.BottomFieldOrderCnt : pp->CurrPic.TopFieldOrderCnt;
    else
    {
        pic->CurrFieldOrderCnt[0] = pp->CurrPic.TopFieldOrderCnt;
        pic->CurrFieldOrderCnt[1] = pp->CurrPic.BottomFieldOrderCnt;
    }
    /*
     * Each reference keeps its DPB entry for as long as it is one (as a
     * Vulkan DPB slot does in NVK); new references take the lowest free
     * entry. Packing the list afresh each frame moved pictures between
     * entries as older ones left, and a stream with a 16-frame DPB (Big Buck
     * Bunny, x264) then mispredicted from its first B frames on.
     */
    {
        BYTE keep[NOUVEAU_DECODE_SLOTS];
        UWORD used = 0;

        memset(keep, -1, sizeof(keep));
        for (i = 0; i < 16; i++)
            if ((refslot[i] >= 0) && (h->dpbpos[refslot[i]] >= 0))
            {
                keep[refslot[i]] = h->dpbpos[refslot[i]];
                used |= 1 << h->dpbpos[refslot[i]];
            }
        for (i = 0; i < 16; i++)
        {
            if ((refslot[i] < 0) || (keep[refslot[i]] >= 0))
                continue;
            for (n = 0; n < 16 && (used & (1 << n)); n++)
                ;
            if (n >= 16)
                continue;
            keep[refslot[i]] = n;
            used |= 1 << n;
        }
        memcpy(h->dpbpos, keep, sizeof(keep));
        for (i = 0; i < 16; i++)
            if (refslot[i] >= 0)
                h264_dpb_entry(&pic->dpb[(LONG)keep[refslot[i]]], &pp->ReferenceFrames[i], refslot[i],
                               h->slotfields[refslot[i]]);
    }
    if (h->have_iq)
    {
        memcpy(pic->WeightScale, h->iq.ScalingList4x4, sizeof(pic->WeightScale));
        memcpy(pic->WeightScale8x8[0], h->iq.ScalingList8x8[0], 64);
        memcpy(pic->WeightScale8x8[1], h->iq.ScalingList8x8[1], 64);
    }
    else
    {
        memset(pic->WeightScale, 16, sizeof(pic->WeightScale));
        memset(pic->WeightScale8x8, 16, sizeof(pic->WeightScale8x8));
    }
    if (bw)
    {
        nvdec_h264_pic_s old = *pic;

        h264_setup_bw(dec, pp, &old, (UBYTE *)pic, allintra);
    }
    nouveau_staging_to_gpu(f->picsetup->map, NOUVEAU_DECODE_PICSETUP_SIZE);
    nouveau_staging_to_gpu(f->bitstream->map, dec->cur.bitstreamlen);

    if (bw)
    {
        struct nouveau_bo *extra[3] = { h->filter, h->intratop, h->blkbuf };
        ULONG m[2 * 4] =
        {
            NVDEC_H264_SET_SCALING_LIST_OFFSET, (ULONG)((f->picsetup->offset + H264BW_MATRICES) >> 8),
            NVDEC_SET_FILTER_BUFFER_OFFSET,     (ULONG)(h->filter->offset >> 8),
            NVDEC_SET_INTRA_TOP_BUF_OFFSET,     (ULONG)(h->intratop->offset >> 8),
            NVDEC_SET_BLK_BUF_OFFSET,           (ULONG)(h->blkbuf->offset >> 8),
        };

        /* NVIDIA's control word (traced): the old application takes the codec alone, this one does not */
        if (!HIDDNouveauDecodeEmit(dec, f, slot, NULL, NVDEC_APPLICATION_ID_H264_BW, NVDEC_CONTROL_CODEC_H264 | 0x52050,
                                   extra, 3, m, 4))
            return FALSE;
    }
    else
    {
        static const ULONG none[] = { NVDEC_H264_SET_MBHIST_BUF_OFFSET, 0 };

        /*
         * Never the all-intra bit (NVIDIA's driver does not set it): with it
         * an intra field leaves its colocated motion data unwritten, and B
         * fields that take it as their direct-mode reference read stale data.
         */
        control = NVDEC_CONTROL_CODEC_H264;
        if (!HIDDNouveauDecodeEmit(dec, f, slot, NULL, NVDEC_APPLICATION_ID_H264, control, NULL, 0, none, 1))
            return FALSE;
    }
    D(bug("[Nouveau] NVDEC: H.264 frame %lu: slot %ld, %lu refs, %lu slices, %lu bytes%s\n",
        (unsigned long)dec->frames, (long)slot, (unsigned long)nrefs, (unsigned long)h->nslices,
        (unsigned long)pic->stream_len, allintra ? " (intra)" : ""));
    return TRUE;
}

const struct HIDDNouveauDecodeOps HIDDNouveauDecodeH264Ops =
{
    h264_open, h264_close, h264_begin, h264_params, h264_bitstream, h264_submit, h264_status
};
