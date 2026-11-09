/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Hardware decoding through gfx.hidd's decode interface: HEVC
    Main on the BCM2711's HEVC block.

    A session gathers one frame's VA structures and slice data, turns them
    into a job (vcgfx_hevc_cl.c) and queues it on the engine
    (vcgfx_hevc_hw.c). Decode targets keep their buffers in the block's
    SAND128 layout; the CPU gets a linear NV12 copy through LockBuffer.
*/

#define DEBUG 0
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <hidd/gfx.h>
#include <oop/oop.h>
#include <string.h>

#include "vcgfx_hidd.h"
#include "vcgfx_hevc_hw.h"
#include "vcgfx_decode.h"

#include LC_LIBDEFS_FILE

#define MNAME_DISPLAY(x) VideoCoreGfxDisplay__Hidd_Display__ ## x

/* A decode-target bitmap: its buffers and what is known about each */
struct vcgfx_dectgt
{
    struct VideoCoreGfx_staticdata *xsd;
    struct vcgfx_hevc      *hevc;
    ULONG                   nbufs;
    ULONG                   width, height;
    BOOL                    tenbit;     /* P010 to the CPU, 10-bit columns */
    ULONG                   pitch;      /* SAND128 column pitch */
    ULONG                   chroma;     /* chroma offset in a buffer */
    ULONG                   lpitch;     /* the linear copy's bytes per row */
    struct vcgfx_scanbuf   *pic;        /* [nbufs] */
    struct vcgfx_scanbuf   *mv;         /* [nbufs], made on first use */
    volatile ULONG         *status;     /* [nbufs], HEVC_JOB_* */
    UBYTE                 **staging;    /* [nbufs], while locked */
    ULONG                  *locks;      /* [nbufs] */
};

/* A decode session: the data part of the decode interface */
struct vcgfx_decsession
{
    struct VideoCoreGfx_staticdata *xsd;
    struct vcgfx_hevc      *hevc;
    struct Hook            *resolve;
    ULONG                   lastseq;
    BOOL                    warned;

    /* The frame between BeginFrame and EndFrame */
    struct vcgfx_dectgt    *target;
    ULONG                   buffer;
    BOOL                    inframe, havepp, haveiq;
    VAPictureParameterBufferHEVC pp;
    VAIQMatrixBufferHEVC    iq;
    struct hevc_slice      *slices;     /* .data: offset into bs until EndFrame */
    ULONG                   nslices, maxslices, bound;
    UBYTE                  *bs;
    ULONG                   bslen, bsmax;
    ULONG                  *map;
    ULONG                   mapsize;
};

const ULONG vcgfx_decode_datasize = sizeof(struct vcgfx_decsession);

static struct vcgfx_dectgt *dectgt_of(struct VideoCoreGfx_staticdata *xsd, OOP_Object *bm)
{
    OOP_Class *cl = xsd->vcsd_VideoCoreGfxOffBMClass;

    if (!bm || (OOP_OCLASS(bm) != cl))
        return NULL;
    return ((struct BitmapData *)OOP_INST_DATA(cl, bm))->dectgt;
}

/* Room for 'need' elements of 'elem' bytes, contents kept */
static BOOL grow(APTR *buf, ULONG *max, ULONG need, ULONG elem)
{
    ULONG n = *max ? *max : 16;
    APTR nb;

    if (need <= *max)
        return TRUE;
    /* n ends below 2 * need, so n * elem stays in range */
    if (need > 0x7fffffff / elem)
        return FALSE;
    while (n < need)
        n *= 2;
    if (!(nb = AllocVec(n * elem, MEMF_ANY)))
        return FALSE;
    if (*buf)
    {
        CopyMem(*buf, nb, *max * elem);
        FreeVec(*buf);
    }
    *buf = nb;
    *max = n;
    return TRUE;
}

/*********** Decode-target bitmaps ***********************************/

/* SAND128 <-> linear, rows of 'width' bytes */
static void sand_copy(UBYTE *sand, ULONG pitch, UBYTE *lin, ULONG lpitch,
                      ULONG width, ULONG y0, ULONG y1, BOOL tolinear)
{
    ULONG x, y;

    for (y = y0; y < y1; y++)
    {
        for (x = 0; x < width; x += 128)
        {
            UBYTE *col = sand + (x / 128) * pitch + y * 128;
            UBYTE *row = lin + y * lpitch + x;
            ULONG n = (width - x < 128) ? width - x : 128;

            if (tolinear)
                memcpy(row, col, n);
            else
                memcpy(col, row, n);
        }
    }
}

/* 10-bit columns <-> P010: three samples per word, 96 per column row. A
 * column row moves through a cached copy: word reads from uncached memory
 * are slow, a 128-byte memcpy is not. */
static void sand10_copy(UBYTE *sand, ULONG pitch, UBYTE *lin, ULONG lpitch,
                        ULONG n, ULONG y0, ULONG y1, BOOL tolinear)
{
    ULONG words[32];
    ULONG x, y, k;

    for (y = y0; y < y1; y++)
    {
        UWORD *row = (UWORD *)(lin + y * lpitch);

        for (x = 0; x < n; x += 96)
        {
            UBYTE *col = sand + (x / 96) * pitch + y * 128;
            ULONG m = (n - x < 96) ? n - x : 96;
            UWORD *out = row + x;

            if (tolinear)
            {
                memcpy(words, col, 128);
                for (k = 0; k < m; k += 3)
                {
                    ULONG w = AROS_LE2LONG(words[k / 3]);

                    out[k] = (w & 0x3ff) << 6;
                    if (k + 1 < m)
                        out[k + 1] = ((w >> 10) & 0x3ff) << 6;
                    if (k + 2 < m)
                        out[k + 2] = ((w >> 20) & 0x3ff) << 6;
                }
            }
            else
            {
                memset(words, 0, sizeof(words));
                for (k = 0; k < m; k += 3)
                {
                    ULONG w = out[k] >> 6;

                    if (k + 1 < m)
                        w |= (ULONG)(out[k + 1] >> 6) << 10;
                    if (k + 2 < m)
                        w |= (ULONG)(out[k + 2] >> 6) << 20;
                    words[k / 3] = AROS_LONG2LE(w);
                }
                memcpy(col, words, 128);
            }
        }
    }
}

static void dectgt_copy(struct vcgfx_dectgt *t, ULONG b, ULONG y0, ULONG y1, BOOL tolinear)
{
    UBYTE *sand = t->pic[b].sb_Pixels, *lin = t->staging[b];
    UBYTE *clin = lin + t->lpitch * t->height;

    /* Chroma rows hold as many samples (Cb, Cr pairs) as luma rows */
    if (t->tenbit)
    {
        sand10_copy(sand, t->pitch, lin, t->lpitch, t->width, y0, y1, tolinear);
        sand10_copy(sand + t->chroma, t->pitch, clin, t->lpitch, t->width, y0 / 2, (y1 + 1) / 2, tolinear);
    }
    else
    {
        sand_copy(sand, t->pitch, lin, t->lpitch, t->width, y0, y1, tolinear);
        sand_copy(sand + t->chroma, t->pitch, clin, t->lpitch, t->width, y0 / 2, (y1 + 1) / 2, tolinear);
    }
}

/* Buffers in SAND128, 8-bit (NV12) or 10-bit (P010); ChunkyBM is told it is
 * no scan-out bitmap */
OOP_Object *vcgfx_dectgt_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg, BOOL *handled)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);
    struct vcgfx_dectgt *t;
    struct vcgfx_hevc *h;
    struct TagItem *tags, *tag;
    OOP_Object *bm = NULL;
    ULONG width, height, nbufs, videofmt, i;

    *handled = FALSE;
    if (!GetTagData(aHidd_BitMap_DecodeTarget, FALSE, msg->attrList))
        return NULL;
    *handled = TRUE;

    width  = GetTagData(aHidd_BitMap_Width,  0, msg->attrList);
    height = GetTagData(aHidd_BitMap_Height, 0, msg->attrList);
    nbufs  = GetTagData(aHidd_BitMap_Buffers, 1, msg->attrList);
    videofmt = GetTagData(aHidd_BitMap_VideoFormat, vHidd_VideoFmt_None, msg->attrList);
    if (((videofmt != vHidd_VideoFmt_NV12) && (videofmt != vHidd_VideoFmt_P010))
        || (width < HEVC_MIN_SIZE) || (width > HEVC_MAX_SIZE) || (width & 1)
        || (height < HEVC_MIN_SIZE) || (height > HEVC_MAX_SIZE) || (height & 1)
        || !nbufs || (nbufs > 64) || !(h = vcgfx_hevc_open(xsd)))
        return NULL;

    t = AllocVec(sizeof(*t) + nbufs * (2 * sizeof(struct vcgfx_scanbuf) + sizeof(ULONG)
                                       + sizeof(UBYTE *) + sizeof(ULONG)),
                 MEMF_ANY | MEMF_CLEAR);
    if (!t)
        return NULL;
    t->xsd     = xsd;
    t->hevc    = h;
    t->nbufs   = nbufs;
    t->width   = width;
    t->height  = height;
    t->tenbit  = (videofmt == vHidd_VideoFmt_P010);
    t->pitch   = HEVC_SAND_PITCH(height);
    t->chroma  = HEVC_SAND_HEIGHT(height) * 128;
    t->lpitch  = t->tenbit ? (width * 2 + 63) & ~63 : (width + 31) & ~31;
    t->pic     = (struct vcgfx_scanbuf *)(t + 1);
    t->mv      = t->pic + nbufs;
    t->staging = (UBYTE **)(t->mv + nbufs);     /* pointers first: aligned */
    t->status  = (volatile ULONG *)(t->staging + nbufs);
    t->locks   = (ULONG *)(t->status + nbufs);

    for (i = 0; i < nbufs; i++)
    {
        if (!vcgfx_scanout_alloc(xsd, &t->pic[i], t->tenbit ? HEVC_SAND10_SIZE(width, height)
                                                             : HEVC_SAND_SIZE(width, height)))
        {
            vcgfx_dectgt_dispose(xsd, t);
            return NULL;
        }
    }

    if ((tags = CloneTagItems(msg->attrList)))
    {
        if ((tag = FindTagItem(aHidd_BitMap_Scanout, tags)))
            tag->ti_Tag = TAG_IGNORE;
        if ((tag = FindTagItem(aHidd_BitMap_VideoLayout, tags)))
            tag->ti_Tag = TAG_IGNORE;
        {
            struct TagItem extra_tags[] =
            {
                { aHidd_ChunkyBM_Buffer,    (IPTR)t->pic[0].sb_Pixels      },
                { aHidd_BitMap_BytesPerRow, t->tenbit ? (width * 2 + 15) & ~15
                                                      : HEVC_SAND_WIDTH(width) },
                { aHidd_BitMap_VideoLayout, vHidd_VideoLayout_SAND128      },
                { TAG_MORE,                 (IPTR)tags                     }
            };
            struct pRoot_New new_msg;

            new_msg.mID = msg->mID;
            new_msg.attrList = extra_tags;
            bm = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&new_msg);
        }
        FreeTagItems(tags);
    }
    if (!bm)
    {
        vcgfx_dectgt_dispose(xsd, t);
        return NULL;
    }

    {
        struct BitmapData *data = OOP_INST_DATA(cl, bm);

        /* An NV12 or P010 video bitmap to the planes, its buffers in SAND128 */
        memset(data, 0, sizeof(struct BitmapData));
        data->VideoData  = t->pic[0].sb_Pixels;
        data->width      = width;
        data->height     = height;
        data->disp       = -1;
        data->videofmt   = videofmt;
        data->colorspace = GetTagData(aHidd_BitMap_ColorSpace, vHidd_ColorSpace_BT709, msg->attrList);
        data->colorrange = GetTagData(aHidd_BitMap_ColorRange, vHidd_ColorRange_Limited, msg->attrList);
        data->dectgt     = t;
    }

    D(bug("[VideoCoreGfx] decode target %ux%u, %u buffers, column pitch %u\n",
          (unsigned)width, (unsigned)height, (unsigned)nbufs, (unsigned)t->pitch));
    return bm;
}

/* After the superclass: waits for every decode submitted, as one into
 * another target may still read these buffers as references */
void vcgfx_dectgt_dispose(struct VideoCoreGfx_staticdata *xsd, struct vcgfx_dectgt *t)
{
    ULONG i;

    vcgfx_hevc_drain(t->hevc, t->hevc->submitted);
    for (i = 0; i < t->nbufs; i++)
    {
        vcgfx_scanout_free(xsd, &t->pic[i]);
        vcgfx_scanout_free(xsd, &t->mv[i]);
        FreeVec(t->staging[i]);
    }
    FreeVec(t);
}

/* Buffer b for a plane, once the decode into it is done */
UBYTE *vcgfx_dectgt_page(struct vcgfx_dectgt *t, ULONG b)
{
    if (b >= t->nbufs)
        return NULL;
    vcgfx_hevc_wait(t->hevc, &t->status[b], ~0);
    return t->pic[b].sb_Pixels;
}

BOOL vcgfx_dectgt_sand(struct vcgfx_dectgt *t, ULONG *pitch, ULONG *chroma)
{
    *pitch  = t->pitch;
    *chroma = t->chroma;
    return t->tenbit;
}

/* A linear NV12 or P010 copy, made once the decode is done */
BOOL vcgfx_dectgt_lock(struct vcgfx_dectgt *t, struct pHidd_BitMap_LockBuffer *msg)
{
    ULONG b = msg->buffer;

    if ((b >= t->nbufs) || !msg->addressReturn || !msg->pitchReturn)
        return FALSE;

    vcgfx_hevc_wait(t->hevc, &t->status[b], ~0);
    if (!t->locks[b])
    {
        if (!t->staging[b] && !(t->staging[b] = AllocVec(t->lpitch * t->height * 3 / 2, MEMF_ANY)))
            return FALSE;
        dectgt_copy(t, b, 0, t->height, TRUE);
    }
    t->locks[b]++;

    *msg->addressReturn = t->staging[b];
    *msg->pitchReturn   = t->lpitch;
    if (msg->planeOffsets)
    {
        msg->planeOffsets[0] = t->lpitch * t->height;
        msg->planeOffsets[1] = 0;
    }
    return TRUE;
}

/* The rows written go back into the buffer */
void vcgfx_dectgt_unlock(struct vcgfx_dectgt *t, struct pHidd_BitMap_UnlockBuffer *msg)
{
    const struct Rectangle *d = msg->dirty;
    ULONG b = msg->buffer;

    if ((b >= t->nbufs) || !t->locks[b])
        return;

    if (!d)
        dectgt_copy(t, b, 0, t->height, FALSE);
    else if ((d->MaxX >= d->MinX) && (d->MaxY >= d->MinY) && (d->MinY < (WORD)t->height))
        dectgt_copy(t, b, (d->MinY < 0) ? 0 : d->MinY & ~1,
                    ((ULONG)d->MaxY + 1 > t->height) ? t->height : (ULONG)d->MaxY + 1, FALSE);

    if (!--t->locks[b])
    {
        FreeVec(t->staging[b]);
        t->staging[b] = NULL;
    }
}

/*********** The decode interface ************************************/

static BOOL dec_BeginFrame(APTR obj, OOP_Object *bitmap, ULONG buffer)
{
    struct vcgfx_decsession *s = obj;
    struct vcgfx_dectgt *t = dectgt_of(s->xsd, bitmap);

    if (!t || (buffer >= t->nbufs) || t->locks[buffer])
        return FALSE;

    /* One decode per buffer at a time: its status belongs to the last */
    vcgfx_hevc_wait(s->hevc, &t->status[buffer], ~0);

    s->target  = t;
    s->buffer  = buffer;
    s->inframe = TRUE;
    s->havepp  = FALSE;
    s->haveiq  = FALSE;
    s->nslices = 0;
    s->bound   = 0;
    s->bslen   = 0;
    return TRUE;
}

/* The base structures lead the extended ones, so 'size' may be larger */
static BOOL dec_SubmitParams(APTR obj, ULONG type, APTR data, ULONG size, ULONG count)
{
    struct vcgfx_decsession *s = obj;
    ULONG i;

    if (!s->inframe || !count)
        return FALSE;

    switch (type)
    {
    case VAPictureParameterBufferType:
        if (size < sizeof(s->pp))
            return FALSE;
        CopyMem(data, &s->pp, sizeof(s->pp));
        s->havepp = TRUE;
        return TRUE;

    case VAIQMatrixBufferType:
        if (size < sizeof(s->iq))
            return FALSE;
        CopyMem(data, &s->iq, sizeof(s->iq));
        s->haveiq = TRUE;
        return TRUE;

    case VASliceParameterBufferType:
        if ((size < sizeof(VASliceParameterBufferHEVC))
            || !grow((APTR *)&s->slices, &s->maxslices, s->nslices + count, sizeof(struct hevc_slice)))
            return FALSE;
        for (i = 0; i < count; i++)
        {
            CopyMem((UBYTE *)data + i * size, &s->slices[s->nslices].sp, sizeof(VASliceParameterBufferHEVC));
            s->slices[s->nslices++].data = 0;
        }
        return TRUE;
    }
    return FALSE;
}

/* Slice data for the slices submitted since the last */
static BOOL dec_SubmitBitstream(APTR obj, APTR data, ULONG size)
{
    struct vcgfx_decsession *s = obj;
    ULONG at = (s->bslen + 63) & ~63, i;

    if (!s->inframe || ((UQUAD)at + size > 0x7fffffff) || !grow((APTR *)&s->bs, &s->bsmax, at + size, 1))
        return FALSE;

    for (i = s->bound; i < s->nslices; i++)
    {
        s->slices[i].data = at;
        s->slices[i].datasize = size;
    }
    s->bound = s->nslices;
    CopyMem(data, s->bs + at, size);
    s->bslen = at + size;
    return TRUE;
}

/* Every slice whole, and inside the buffer it came in */
static BOOL dec_slices_fit(struct vcgfx_decsession *s)
{
    ULONG i;

    for (i = 0; i < s->nslices; i++)
    {
        const VASliceParameterBufferHEVC *sp = &s->slices[i].sp;

        if ((sp->slice_data_flag != VA_SLICE_DATA_FLAG_ALL)
            || ((UQUAD)sp->slice_data_offset + sp->slice_data_size > s->slices[i].datasize))
            return FALSE;
    }
    return TRUE;
}

static void dec_refs(struct vcgfx_decsession *s, struct hevc_picture *pic)
{
    struct HIDD_DecodeSurfaceMsg m;
    ULONG i;

    for (i = 0; i < 15; i++)
    {
        const VAPictureHEVC *r = &s->pp.ReferenceFrames[i];
        struct vcgfx_dectgt *rt;

        if ((r->picture_id == VA_INVALID_SURFACE) || (r->flags & VA_PICTURE_HEVC_INVALID))
            continue;
        m.SurfaceID = r->picture_id;
        m.BitMap = NULL;
        m.Buffer = 0;
        if (!CallHookPkt(s->resolve, s, &m))
            continue;
        /* Every picture of the stream in one geometry: the block reads them so */
        rt = dectgt_of(s->xsd, m.BitMap);
        if (!rt || (m.Buffer >= rt->nbufs) || (rt->pitch != s->target->pitch)
            || (rt->width != s->target->width) || (rt->height != s->target->height)
            || (rt->tenbit != s->target->tenbit))
            continue;
        pic->refs[i].luma   = vcgfx_hevc_bus(rt->pic[m.Buffer].sb_Pixels);
        pic->refs[i].chroma = pic->refs[i].luma + rt->chroma;
        pic->refs[i].colmv  = rt->mv[m.Buffer].sb_Raw ? vcgfx_hevc_bus(rt->mv[m.Buffer].sb_Pixels) : 0;
    }
}

static BOOL dec_EndFrame(APTR obj)
{
    struct vcgfx_decsession *s = obj;
    struct vcgfx_dectgt *t = s->target;
    struct VideoCoreGfx_staticdata *xsd = s->xsd;
    struct vcgfx_hevc_job *job = NULL;
    struct hevc_picture pic;
    const char *err = NULL;
    ULONG b = s->buffer, max, i;
    UQUAD bs;

    if (!s->inframe)
        return FALSE;
    s->inframe = FALSE;

    if (!s->havepp || !s->nslices || (s->bound != s->nslices))
        err = "incomplete frame";
    else if (!hevc_ctb_ok(&s->pp))
        err = "CTB size not 16, 32 or 64";
    else if ((s->pp.bit_depth_luma_minus8 != 0) != t->tenbit)
        err = "bit depth not the target's";
    else if ((s->pp.pic_width_in_luma_samples > t->width)
             || (s->pp.pic_height_in_luma_samples > t->height))
        err = "picture larger than the target";
    else if (!dec_slices_fit(s))
        err = "slice data outside its buffer";
    else if ((s->pp.slice_parsing_fields.bits.sps_temporal_mvp_enabled_flag) && !t->mv[b].sb_Raw
             && !vcgfx_scanout_alloc(xsd, &t->mv[b], hevc_colmv_size(t->width, t->height)))
        err = "no memory for the MV buffer";
    else if (!grow((APTR *)&s->map, &s->mapsize, hevc_tiles_size(&s->pp), 1))
        err = "no memory";
    else if (!(job = AllocVec(sizeof(*job), MEMF_ANY | MEMF_CLEAR)))
        err = "no memory";

    if (!err)
    {
        max = hevc_cl_estimate(&s->pp, s->nslices);
        if (!vcgfx_scanout_alloc(xsd, &job->clmem, max * sizeof(struct hevc_cmd))
            || !vcgfx_scanout_alloc(xsd, &job->bsmem, s->bslen + 64))
            err = "no memory for the job";
    }

    if (!err)
    {
        job->cl.cmds = (struct hevc_cmd *)job->clmem.sb_Pixels;
        job->cl.max  = max;
        CopyMem(s->bs, job->bsmem.sb_Pixels, s->bslen);
        bs = vcgfx_hevc_bus(job->bsmem.sb_Pixels);
        for (i = 0; i < s->nslices; i++)
            s->slices[i].data += bs;

        memset(&pic, 0, sizeof(pic));
        pic.pp        = &s->pp;
        pic.iq        = s->haveiq ? &s->iq : NULL;
        pic.slices    = s->slices;
        pic.nslices   = s->nslices;
        pic.out.luma   = vcgfx_hevc_bus(t->pic[b].sb_Pixels);
        pic.out.chroma = pic.out.luma + t->chroma;
        pic.out.colmv  = t->mv[b].sb_Raw ? vcgfx_hevc_bus(t->mv[b].sb_Pixels) : 0;
        pic.pitch     = t->pitch;
        dec_refs(s, &pic);

        err = hevc_build(&pic, &job->cl, &job->p2, s->map);
    }

    if (err)
    {
        if (!s->warned)
            bug("[VideoCoreGfx] HEVC: frame not decoded: %s\n", err);
        s->warned = TRUE;
        if (job)
            vcgfx_hevc_job_free(s->hevc, job);
        t->status[b] = HEVC_JOB_ERROR;
        return FALSE;
    }

    job->status = &t->status[b];
    s->lastseq = vcgfx_hevc_submit(s->hevc, job);
    return TRUE;
}

static ULONG dec_Sync(APTR obj, OOP_Object *bitmap, ULONG buffer, ULONG timeoutms)
{
    struct vcgfx_decsession *s = obj;
    struct vcgfx_dectgt *t = dectgt_of(s->xsd, bitmap);

    if (!t || (buffer >= t->nbufs))
        return vHidd_DecodeStatus_Error;
    return vcgfx_hevc_wait(s->hevc, &t->status[buffer], timeoutms);
}

static ULONG dec_Status(APTR obj, OOP_Object *bitmap, ULONG buffer)
{
    struct vcgfx_decsession *s = obj;
    struct vcgfx_dectgt *t = dectgt_of(s->xsd, bitmap);

    if (!t || (buffer >= t->nbufs))
        return vHidd_DecodeStatus_Error;
    return t->status[buffer];
}

const struct HIDD_DecodeInterface vcgfx_decode_vectors =
{
    sizeof(struct HIDD_DecodeInterface),
    dec_BeginFrame,
    dec_SubmitParams,
    dec_SubmitBitstream,
    dec_EndFrame,
    dec_Sync,
    dec_Status
};

/*********** Display methods *****************************************/

ULONG MNAME_DISPLAY(QueryDecode)(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_QueryDecode *msg)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);
    ULONG flags;

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    /* The block brought up, and memory it can write */
    if (((msg->profile != VAProfileHEVCMain) && (msg->profile != VAProfileHEVCMain10))
        || !vcgfx_hevc_open(xsd) || !vcgfx_scanout_ok(xsd))
        return 0;
    if ((msg->width || msg->height)
        && ((msg->width < HEVC_MIN_SIZE) || (msg->width > HEVC_MAX_SIZE)
            || (msg->height < HEVC_MIN_SIZE) || (msg->height > HEVC_MAX_SIZE)))
        return 0;

    /* Planes that scan SAND128 out; elsewhere LockBuffer reads it back */
    flags = DCF_DECODE | (vcgfx_sand_scanout(xsd) ? DCF_SCANOUT : 0);
    if (msg->caps)
    {
        msg->caps->Flags         = flags;
        msg->caps->MaxWidth      = HEVC_MAX_SIZE;
        msg->caps->MaxHeight     = HEVC_MAX_SIZE;
        msg->caps->MinWidth      = HEVC_MIN_SIZE;
        msg->caps->MinHeight     = HEVC_MIN_SIZE;
        msg->caps->VideoFmtMask  = 1 << ((msg->profile == VAProfileHEVCMain10) ? vHidd_VideoFmt_P010
                                                                               : vHidd_VideoFmt_NV12);
        msg->caps->LayoutMask    = 1 << vHidd_VideoLayout_SAND128;
        msg->caps->MaxReferences = 15;
    }
    return flags;
}

APTR MNAME_DISPLAY(GetDecodeInterface)(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_GetDecodeInterface *msg)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);
    struct vcgfx_decsession *s;
    struct vcgfx_hevc *h;

    if (((msg->profile != VAProfileHEVCMain) && (msg->profile != VAProfileHEVCMain10))
        || !(h = vcgfx_hevc_open(xsd)))
        return NULL;

    s = (struct vcgfx_decsession *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (s)
    {
        s->xsd     = xsd;
        s->hevc    = h;
        s->resolve = msg->resolve;
    }
    return s;
}

VOID MNAME_DISPLAY(FreeDecodeInterface)(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_FreeDecodeInterface *msg)
{
    struct vcgfx_decsession *s = msg->iface;

    if (s)
    {
        vcgfx_hevc_drain(s->hevc, s->lastseq);
        FreeVec(s->slices);
        FreeVec(s->bs);
        FreeVec(s->map);
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}
