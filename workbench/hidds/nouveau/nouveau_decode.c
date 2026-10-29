/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Hardware video decoding on NVDEC behind gfx.hidd's decode interface.

    A decode session owns a channel on the NVDEC runlist with the NVxxB0
    decoder object on it, a pool of block-linear pictures (the decoder reads
    its references from and writes the new picture into block-linear memory
    only), and a ring of per-frame input buffers (bitstream, picture setup,
    slice offsets, status). The pictures the caller sees are the buffers of
    its decode-target video bitmaps (pitch-linear NV12, scanned out by the
    planes and read back by the CPU): when a decoded picture is needed there,
    the copy engine detiles it from the pool (HIDDNouveauDecodeSettle). The
    codec-specific part - VA parameters to the decoder's picture setup and
    method stream - lives in nouveau_decode_<codec>.c.
*/

#include "nouveau_intern.h"
#include "nouveau_decode.h"
#include "nouveau_vadec.h"

#define DEBUG 0
#include <aros/debug.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>

#undef HiddBitMapAttrBase
#undef HiddDisplayAttrBase
#define HiddBitMapAttrBase          (SD(cl)->bitMapAttrBase)
#define HiddDisplayAttrBase         (SD(cl)->displayAttrBase)

int nouveau_decode_enable = 1;          /* NOUVEAU_DECODE=0 turns the decoder off */

/*
 * One lock for every session's state and the decode fields of the targets.
 * Lock order: it is taken before the engine lock (copy-out, slot eviction),
 * never after it, so no caller may hold LOCK_ENGINE when it calls in here.
 */
static struct SignalSemaphore decodelock;
static BOOL decodelock_init;
static ULONG decode_sessions;           /* numbers the sessions' object handles */

VOID HIDDNouveauDecodeInit(VOID)
{
    if (!decodelock_init)
    {
        InitSemaphore(&decodelock);
        decodelock_init = TRUE;
    }
}

static inline VOID decode_lock(VOID)
{
    HIDDNouveauDecodeInit();
    ObtainSemaphore(&decodelock);
}

static inline VOID decode_unlock(VOID)
{
    ReleaseSemaphore(&decodelock);
}

/* NVDEC classes, newest first; the channel accepts the one for its chip */
static const ULONG nvdec_classes[] = { 0xcfb0, 0xcdb0, 0xc9b0, 0xc7b0, 0xc6b0, 0xc4b0, 0 };

/* Object handles: unique per client on GSP-RM parts (see nouveau_copy.c) */
#define NVDEC_HANDLE_BASE   0x0de00000

/*
 * Push buffer helpers: Fermi+ method headers, decoder on subchannel 0.
 */
static inline VOID nvdec_begin(struct nouveau_pushbuf *push, ULONG mthd, ULONG count)
{
    *push->cur++ = 0x20000000 | (count << 16) | (0 << 13) | (mthd / 4);
}

static inline VOID nvdec_data(struct nouveau_pushbuf *push, ULONG data)
{
    *push->cur++ = data;
}

VOID HIDDNouveauDecodeMethod(struct HIDDNouveauDecode *dec, ULONG mthd, ULONG data)
{
    nvdec_begin(dec->pushbuf, mthd, 1);
    nvdec_data(dec->pushbuf, data);
}

/*
 * Channel on the NVDEC runlist plus the decoder object. Shared by the probe
 * and the sessions.
 */
static LONG nvdec_channel_new(struct CardData *carddata, ULONG handle, struct nouveau_object **chan,
    struct nouveau_pushbuf **push, struct nouveau_object **obj, ULONG *oclass)
{
    struct nve0_fifo fifo;
    const ULONG *cls;
    LONG ret;

    *chan = NULL;
    *push = NULL;
    *obj = NULL;
    memset(&fifo, 0, sizeof(fifo));
    fifo.engine = NVE0_FIFO_ENGINE_NVDEC;
    ret = nouveau_object_new(&carddata->dev->object, 0, NOUVEAU_FIFO_CHANNEL_CLASS, &fifo, sizeof(fifo), chan);
    if (ret)
    {
        nvlog("[Nouveau] NVDEC: channel allocation failed (%ld)\n", (long)ret);
        return ret;
    }
    ret = nouveau_pushbuf_new(carddata->client, *chan, 4, 64 * 1024, push);
    if (ret)
    {
        nvlog("[Nouveau] NVDEC: push buffer allocation failed (%ld)\n", (long)ret);
        nouveau_object_del(chan);
        return ret;
    }
    for (cls = nvdec_classes; *cls; cls++)
    {
        if (carddata->nvdec_class && (*cls != carddata->nvdec_class))
            continue;
        ret = nouveau_object_new(*chan, handle | *cls, *cls, NULL, 0, obj);
        if (ret == 0)
        {
            *oclass = *cls;
            return 0;
        }
        D(bug("[Nouveau] NVDEC: class %04lx refused (%ld)\n", (unsigned long)*cls, (long)ret));
    }
    nouveau_pushbuf_del(push);
    nouveau_object_del(chan);
    return ret ? ret : -1;
}

static VOID nvdec_channel_del(struct nouveau_object **chan, struct nouveau_pushbuf **push, struct nouveau_object **obj)
{
    if (*obj)
        nouveau_object_del(obj);
    if (*push)
        nouveau_pushbuf_del(push);
    if (*chan)
        nouveau_object_del(chan);
}

static VOID nvdec_session_kill(struct HIDDNouveauDecode *dec, const char *why);

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
#define NVDEC_PICTURE_SLOTS                 17
#define NVDEC_EXECUTE_NOTIFY_ON_END         (1 << 0)
#define NVDEC_SEMAPHORE_D_RELEASE_FLUSH     (1 << 8)    /* RELEASE, one word, flush enabled */

BOOL HIDDNouveauDecodeEmit(struct HIDDNouveauDecode *dec, struct HIDDNouveauDecodeFrame *f, LONG slot,
    const LONG *map, ULONG appid, ULONG control, struct nouveau_bo **extra, ULONG nextra,
    const ULONG *mthds, ULONG nmthds)
{
    struct nouveau_pushbuf *push = dec->pushbuf;
    struct nouveau_pushbuf_refn refs[8 + 8 + NOUVEAU_DECODE_SLOTS];
    ULONG n = 0, i;

    if (dec->dead || !push)
        return FALSE;
    if (nextra > 8)
        return FALSE;
    refs[n].bo = f->picsetup;   refs[n++].flags = NOUVEAU_BO_GART | NOUVEAU_BO_RDWR;
    refs[n].bo = f->bitstream;  refs[n++].flags = NOUVEAU_BO_GART | NOUVEAU_BO_RD;
    refs[n].bo = dec->sem;      refs[n++].flags = NOUVEAU_BO_GART | NOUVEAU_BO_RDWR;
    if (dec->coloc)   { refs[n].bo = dec->coloc;   refs[n++].flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR; }
    if (dec->history) { refs[n].bo = dec->history; refs[n++].flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR; }
    for (i = 0; i < nextra; i++)
    {
        refs[n].bo = extra[i];
        refs[n++].flags = ((extra[i]->flags & NOUVEAU_BO_GART) ? NOUVEAU_BO_GART : NOUVEAU_BO_VRAM) | NOUVEAU_BO_RDWR;
    }
    for (i = 0; i < dec->nslots; i++)
    {
        refs[n].bo = dec->slot[i].bo;
        refs[n++].flags = NOUVEAU_BO_VRAM | (((LONG)i == (map ? map[slot] : slot)) ? NOUVEAU_BO_RDWR : NOUVEAU_BO_RD);
    }
    if (nouveau_pushbuf_space(push, 128 + 2 * nmthds, 0, 0) || nouveau_pushbuf_refn(push, refs, n))
    {
        nvlog("[Nouveau] NVDEC: no push buffer space for the frame\n");
        return FALSE;
    }
    HIDDNouveauDecodeMethod(dec, NVDEC_SET_APPLICATION_ID, appid);
    HIDDNouveauDecodeMethod(dec, NVDEC_SET_CONTROL_PARAMS, control);
    HIDDNouveauDecodeMethod(dec, NVDEC_SET_DRV_PIC_SETUP_OFFSET, (ULONG)(f->picsetup->offset >> 8));
    HIDDNouveauDecodeMethod(dec, NVDEC_SET_IN_BUF_BASE_OFFSET, (ULONG)(f->bitstream->offset >> 8));
    HIDDNouveauDecodeMethod(dec, NVDEC_SET_PICTURE_INDEX, slot);
    /* codecs that want NVIDIA's minimal set (dec->minimal) get no slice offsets / history / display / histogram */
    if (!dec->minimal)
        HIDDNouveauDecodeMethod(dec, NVDEC_SET_SLICE_OFFSETS_BUF_OFFSET,
            (ULONG)((f->picsetup->offset + NOUVEAU_DECODE_SLICEOFFS_OFFSET) >> 8));
    if (!dec->minimal || dec->coloc)
        HIDDNouveauDecodeMethod(dec, NVDEC_SET_COLOC_DATA_OFFSET, dec->coloc ? (ULONG)(dec->coloc->offset >> 8) : 0);
    if (!dec->minimal || dec->history)
        HIDDNouveauDecodeMethod(dec, NVDEC_SET_HISTORY_OFFSET, dec->history ? (ULONG)(dec->history->offset >> 8) : 0);
    if (!dec->minimal)
    {
        HIDDNouveauDecodeMethod(dec, NVDEC_SET_DISPLAY_BUF_SIZE, 0);
        HIDDNouveauDecodeMethod(dec, NVDEC_SET_HISTOGRAM_OFFSET, 0);
    }
    HIDDNouveauDecodeMethod(dec, NVDEC_SET_NVDEC_STATUS_OFFSET,
        (ULONG)((f->picsetup->offset + NOUVEAU_DECODE_STATUS_OFFSET) >> 8));
    /* every index gets a valid picture: unused ones point at the current one */
    {
        struct nouveau_bo *pics[NVDEC_PICTURE_SLOTS];
        LONG cur = map ? map[slot] : slot;

        for (i = 0; i < NVDEC_PICTURE_SLOTS; i++)
        {
            LONG s = map ? map[i] : (LONG)i;

            pics[i] = ((s >= 0) && ((ULONG)s < dec->nslots)) ? dec->slot[s].bo : dec->slot[cur].bo;
        }
        for (i = 0; i < NVDEC_PICTURE_SLOTS; i++)
            HIDDNouveauDecodeMethod(dec, NVDEC_SET_PICTURE_LUMA_OFFSET0 + 4 * i, (ULONG)(pics[i]->offset >> 8));
        for (i = 0; i < NVDEC_PICTURE_SLOTS; i++)
            HIDDNouveauDecodeMethod(dec, NVDEC_SET_PICTURE_CHROMA_OFFSET0 + 4 * i,
                                    (ULONG)((pics[i]->offset + dec->chromaoff) >> 8));
    }
    if (!dec->minimal)
        HIDDNouveauDecodeMethod(dec, NVDEC_SET_PIC_SCRATCH_BUF_OFFSET, 0);
    for (i = 0; i < nmthds; i++)
        HIDDNouveauDecodeMethod(dec, mthds[2 * i], mthds[2 * i + 1]);
    /*
     * Completion: the EXECUTE notify writes word 0 when the decode ends, but
     * the picture's last writes may still be in the engine's write buffer
     * then. A SEMAPHORE_D release with flush after it writes word 1 once the
     * picture is in memory; that is the word the session waits for (and the
     * host fence on the pool picture, HIDDNouveauDecodeSettle).
     */
    HIDDNouveauDecodeMethod(dec, NVDEC_SEMAPHORE_A, (ULONG)(dec->sem->offset >> 32));
    HIDDNouveauDecodeMethod(dec, NVDEC_SEMAPHORE_B, (ULONG)dec->sem->offset);
    HIDDNouveauDecodeMethod(dec, NVDEC_SEMAPHORE_C, f->seq);
    HIDDNouveauDecodeMethod(dec, NVDEC_EXECUTE, NVDEC_EXECUTE_NOTIFY_ON_END);
    HIDDNouveauDecodeMethod(dec, NVDEC_SEMAPHORE_A, (ULONG)((dec->sem->offset + 4) >> 32));
    HIDDNouveauDecodeMethod(dec, NVDEC_SEMAPHORE_B, (ULONG)(dec->sem->offset + 4));
    HIDDNouveauDecodeMethod(dec, NVDEC_SEMAPHORE_C, f->seq);
    HIDDNouveauDecodeMethod(dec, NVDEC_SEMAPHORE_D, NVDEC_SEMAPHORE_D_RELEASE_FLUSH);
    if (nouveau_pushbuf_kick(push))
    {
        nvdec_session_kill(dec, "the channel refused the frame");
        return FALSE;
    }
    return TRUE;
}

static VOID nvdec_pause(VOID);

/*
 * The engine stopped (its channel was killed by a fault, or it stopped
 * answering): fail the session and drop its channel and push buffer at once.
 * Left in place, the push buffer still listed the session's buffers, and
 * libdrm flushes another push buffer that lists a buffer it is given: the
 * copy engine then resubmitted the dead channel's work from the compositor
 * (rejected, then a libdrm assertion halted the system). Decode lock held.
 */
static VOID nvdec_session_kill(struct HIDDNouveauDecode *dec, const char *why)
{
    if (!dec->dead)
        nvlog("[Nouveau] NVDEC: session %p: %s; session failed\n", dec, why);
    dec->dead = TRUE;
    nvdec_channel_del(&dec->channel, &dec->pushbuf, &dec->nvdec);
}


/* Find the decoder class once; the result is kept for the card */
static BOOL nvdec_probe(struct CardData *carddata)
{
    struct nouveau_object *chan, *obj;
    struct nouveau_pushbuf *push;
    ULONG oclass = 0;

    if (carddata->nvdec_probe)
        return carddata->nvdec_probe > 0;
    if (!carddata->ce_enabled || !carddata->NvCopy)
    {
        nvlog("[Nouveau] NVDEC: no copy engine for the picture copy-out: decoding disabled\n");
        carddata->nvdec_probe = -1;
        return FALSE;
    }
    if (nvdec_channel_new(carddata, NVDEC_HANDLE_BASE, &chan, &push, &obj, &oclass) == 0)
    {
        carddata->nvdec_class = oclass;
        carddata->nvdec_probe = 1;
        nvdec_channel_del(&chan, &push, &obj);
        D(bug("[Nouveau] NVDEC: decoder class %04lx available\n", (unsigned long)oclass));
        return TRUE;
    }
    carddata->nvdec_probe = -1;
    nvlog("[Nouveau] NVDEC: no decoder class accepted on a channel of its runlist: decoding disabled\n");
    return FALSE;
}

/*
 * Codec support table: what QueryDecode answers and which translation a
 * session uses. H.264 first; the decoder itself handles far more.
 */
struct nvdec_codec
{
    LONG    profile;        /* VAProfile */
    const struct HIDDNouveauDecodeOps *ops;
    ULONG   maxw, maxh;
    ULONG   maxrefs;
    ULONG   fmts;           /* vHidd_VideoFmt_* bits the decoder writes for the profile */
    BOOL    experimental;   /* not working yet: offered only with NOUVEAU_NVDEC_EXPERIMENTAL set */
};

#define FMT_8BIT    (1UL << vHidd_VideoFmt_NV12)
#define FMT_10BIT   (1UL << vHidd_VideoFmt_P010)

static const struct nvdec_codec nvdec_codecs[] =
{
    { VAProfileH264ConstrainedBaseline, &HIDDNouveauDecodeH264Ops, 4096, 4096, 16, FMT_8BIT, FALSE },
    { VAProfileH264Main,                &HIDDNouveauDecodeH264Ops, 4096, 4096, 16, FMT_8BIT, FALSE },
    { VAProfileH264High,                &HIDDNouveauDecodeH264Ops, 4096, 4096, 16, FMT_8BIT, FALSE },
    { VAProfileMPEG2Simple,             &HIDDNouveauDecodeMPEG2Ops, 4096, 4096, 3, FMT_8BIT, FALSE },
    { VAProfileMPEG2Main,               &HIDDNouveauDecodeMPEG2Ops, 4096, 4096, 3, FMT_8BIT, FALSE },
    { VAProfileVC1Simple,               &HIDDNouveauDecodeVC1Ops, 4096, 4096, 3, FMT_8BIT, FALSE },
    { VAProfileVC1Main,                 &HIDDNouveauDecodeVC1Ops, 4096, 4096, 3, FMT_8BIT, FALSE },
    { VAProfileVP8Version0_3,           &HIDDNouveauDecodeVP8Ops, 4096, 4096, 4, FMT_8BIT, FALSE },
    /* VP9: 8 reference slots */
    { VAProfileVP9Profile0,             &HIDDNouveauDecodeVP9Ops, 8192, 8192, 8, FMT_8BIT, FALSE },
    { VAProfileVP9Profile2,             &HIDDNouveauDecodeVP9Ops, 8192, 8192, 8, FMT_10BIT, FALSE },
    /* AV1 profile 0: 8 or 10-bit; 8 reference slots, and with film grain a second picture per frame */
    { VAProfileAV1Profile0,             &HIDDNouveauDecodeAV1Ops, 8192, 8192, 15, FMT_8BIT | FMT_10BIT, FALSE },
    /* HEVC: 16 picture slots at most (the long-term mask and POC differences are 16 wide) */
    { VAProfileHEVCMain,                &HIDDNouveauDecodeHEVCOps, 8192, 8192, 15, FMT_8BIT, FALSE },
    { VAProfileHEVCMain10,              &HIDDNouveauDecodeHEVCOps, 8192, 8192, 15, FMT_10BIT, FALSE },
    { VAProfileNone, NULL, 0, 0, 0, 0, FALSE }
};

static const struct nvdec_codec *nvdec_codec_for(LONG profile)
{
    const struct nvdec_codec *c;

    for (c = nvdec_codecs; c->ops; c++)
        if (c->profile == profile)
        {
            if (c->experimental)
            {
                extern char *nouveau_aros_getenv(const char *name);
                extern void kfree(const void *p);
                char *v = nouveau_aros_getenv("NOUVEAU_NVDEC_EXPERIMENTAL");

                if (!v)
                    return NULL;
                kfree(v);
            }
            return c;
        }
    return NULL;
}

ULONG HIDDNouveauDecodeQuery(OOP_Class *cl, OOP_Object *display, LONG profile, ULONG width, ULONG height,
    struct HIDD_DecodeCaps *caps)
{
    struct CardData *carddata = &(SD(cl)->carddata);
    const struct nvdec_codec *c = nvdec_codec_for(profile);

    if (caps)
        memset(caps, 0, sizeof(*caps));
    if (!c || !nouveau_decode_enable)
        return 0;
    decode_lock();
    if (!nvdec_probe(carddata))
    {
        decode_unlock();
        return 0;
    }
    decode_unlock();
    if (width || height)
    {
        if ((width < 48) || (height < 48) || (width > c->maxw) || (height > c->maxh))
            return 0;
    }
    if (caps)
    {
        caps->Flags         = DCF_DECODE | DCF_SCANOUT;
        caps->MaxWidth      = c->maxw;
        caps->MaxHeight     = c->maxh;
        caps->MinWidth      = 48;
        caps->MinHeight     = 48;
        caps->VideoFmtMask  = c->fmts;
        caps->LayoutMask    = (1UL << vHidd_VideoLayout_Linear);
        caps->MaxReferences = c->maxrefs;
    }
    return DCF_DECODE | DCF_SCANOUT;
}

/*
 * Session buffers
 */
static BOOL nvdec_bo_new(struct CardData *carddata, ULONG flags, ULONG align, ULONG size,
    union nouveau_bo_config *cfg, struct nouveau_bo **bo)
{
    LONG ret = nouveau_bo_new(carddata->dev, flags, align, size, cfg, bo);

    if (ret)
    {
        nvlog("[Nouveau] NVDEC: buffer of %lu bytes (flags %lx) failed (%ld)\n",
            (unsigned long)size, (unsigned long)flags, (long)ret);
        *bo = NULL;
        return FALSE;
    }
    if ((flags & NOUVEAU_BO_MAP) && nouveau_bo_map(*bo, NOUVEAU_BO_RDWR, carddata->client))
    {
        nvlog("[Nouveau] NVDEC: mapping a %lu byte buffer failed\n", (unsigned long)size);
        nouveau_bo_ref(NULL, bo);
        return FALSE;
    }
    return TRUE;
}

static BOOL nvdec_frame_alloc(struct CardData *carddata, struct HIDDNouveauDecodeFrame *f, ULONG bitstreamsize)
{
    if (!nvdec_bo_new(carddata, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 4096, bitstreamsize, NULL, &f->bitstream))
        return FALSE;
    f->bitstreamsize = bitstreamsize;
    if (!nvdec_bo_new(carddata, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 4096, NOUVEAU_DECODE_PICSETUP_SIZE, NULL, &f->picsetup))
        return FALSE;
    memset(f->picsetup->map, 0, NOUVEAU_DECODE_PICSETUP_SIZE);
    nouveau_staging_to_gpu(f->picsetup->map, NOUVEAU_DECODE_PICSETUP_SIZE);
    return TRUE;
}

static VOID nvdec_frame_free(struct HIDDNouveauDecodeFrame *f)
{
    if (f->bitstream)
        nouveau_bo_ref(NULL, &f->bitstream);
    if (f->picsetup)
        nouveau_bo_ref(NULL, &f->picsetup);
    f->bitstreamsize = 0;
}

/* Grow a frame's bitstream buffer; the frame is idle (BeginFrame waited for it) */
UBYTE *HIDDNouveauDecodeBitstreamReserve(struct HIDDNouveauDecode *dec, ULONG bytes)
{
    struct HIDDNouveauDecodeFrame *f = &dec->ring[dec->ringpos];
    ULONG need = dec->cur.bitstreamlen + bytes;
    UBYTE *p;

    if (need > f->bitstreamsize)
    {
        struct nouveau_bo *bigger = NULL;
        ULONG size = (need + 1024 * 1024 + 4095) & ~4095UL;

        if (!nvdec_bo_new(dec->carddata, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 4096, size, NULL, &bigger))
            return NULL;
        if (dec->cur.bitstreamlen)
            memcpy(bigger->map, f->bitstream->map, dec->cur.bitstreamlen);
        nouveau_bo_ref(NULL, &f->bitstream);
        f->bitstream = bigger;
        f->bitstreamsize = size;
    }
    p = (UBYTE *)f->bitstream->map + dec->cur.bitstreamlen;
    dec->cur.bitstreamlen = need;
    return p;
}

/*
 * The picture pool: block-linear NV12 pictures, 64-byte GOB rows in blocks
 * of 2 GOBs (tile_mode 0x010 = 16 rows): the layout the decoder is told
 * about in its picture setup (gob height 2, "KBL" block-linear).
 */
#define NVDEC_POOL_TILE_MODE    0x010
#define NVDEC_POOL_BLOCK_ROWS   16

static BOOL nvdec_pool_alloc(struct HIDDNouveauDecode *dec, ULONG nslots)
{
    union nouveau_bo_config cfg;
    ULONG i;

    memset(&cfg, 0, sizeof(cfg));
    /*
     * Generic block-linear page kind: 0xfe on Fermi to Pascal, 0x06 from
     * Turing on (the newer kind table has 16 entries; 0xfe is refused).
     */
    cfg.nvc0.memtype = (dec->carddata->dev->chipset >= 0x160) ? 0x06 : 0xfe;
    cfg.nvc0.tile_mode = NVDEC_POOL_TILE_MODE;
    dec->pitch = (dec->width * dec->bpp + 63) & ~63UL;
    {
        /* rows: block aligned, or the codec's coding-block alignment (HEVC: 64, as NVIDIA lays its pictures out) */
        ULONG align = dec->rowalign ? dec->rowalign : NVDEC_POOL_BLOCK_ROWS;

        dec->lumasize = dec->pitch * ((dec->height + align - 1) & ~(align - 1));
    }
    dec->lumasize = (dec->lumasize + 4095) & ~4095UL;
    dec->chromaoff = dec->lumasize;
    {
        ULONG calign = dec->rowalign ? dec->rowalign / 2 : NVDEC_POOL_BLOCK_ROWS;

        dec->slotsize = dec->lumasize + dec->pitch * (((dec->height / 2) + calign - 1) & ~(calign - 1));
    }
    dec->slotsize = (dec->slotsize + 4095) & ~4095UL;
    for (i = 0; i < nslots; i++)
    {
        if (!nvdec_bo_new(dec->carddata, NOUVEAU_BO_VRAM, 4096, dec->slotsize, &cfg, &dec->slot[i].bo))
            return FALSE;
        dec->slot[i].bm = NULL;
        dec->slot[i].valid = FALSE;
        dec->slot[i].lastuse = 0;
        dec->nslots = i + 1;
    }
    return TRUE;
}

LONG HIDDNouveauDecodeFindSlot(struct HIDDNouveauDecode *dec, OOP_Object *bm, ULONG buffer)
{
    ULONG i;

    for (i = 0; i < dec->nslots; i++)
        if (dec->slot[i].valid && (dec->slot[i].bm == bm) && (dec->slot[i].buffer == buffer))
            return i;
    return -1;
}

/*
 * A slot for the picture about to be decoded into (bm, buffer): the one it
 * already has, else the least recently used slot outside 'avoid' (the
 * frame's references). The evicted picture is copied out first if its
 * target still waits for it.
 */
LONG HIDDNouveauDecodeAllocSlot(struct HIDDNouveauDecode *dec, OOP_Object *bm, ULONG buffer, ULONG avoid)
{
    LONG best = -1, i;

    i = HIDDNouveauDecodeFindSlot(dec, bm, buffer);
    if (i >= 0)
        return i;
    for (i = 0; i < (LONG)dec->nslots; i++)
    {
        if (avoid & (1UL << i))
            continue;
        if (!dec->slot[i].valid)
        {
            best = i;
            break;
        }
        if ((best < 0) || (dec->slot[i].lastuse < dec->slot[best].lastuse))
            best = i;
    }
    if (best < 0)
        return -1;
    if (dec->slot[best].valid && dec->slot[best].bm)
    {
        struct HIDDNouveauBitMapData *obmdata = OOP_INST_DATA(dec->bmclass, dec->slot[best].bm);
        ULONG ob = dec->slot[best].buffer;

        if ((ob < obmdata->nbufs) && (obmdata->buf[ob].dec == dec) && (obmdata->buf[ob].decslot == best))
            HIDDNouveauDecodeSettle(dec->carddata, obmdata, ob, TRUE, 0xffffffffUL);
    }
    dec->slot[best].valid = FALSE;
    dec->slot[best].bm = bm;
    dec->slot[best].buffer = buffer;
    return best;
}

BOOL HIDDNouveauDecodeResolve(struct HIDDNouveauDecode *dec, ULONG surface, OOP_Object **bm, ULONG *buffer)
{
    struct HIDD_DecodeSurfaceMsg msg;

    *bm = NULL;
    *buffer = 0;
    if ((surface == VA_INVALID_SURFACE) || !dec->resolve)
        return FALSE;
    msg.SurfaceID = surface;
    msg.BitMap = NULL;
    msg.Buffer = 0;
    if (!CallHookPkt(dec->resolve, dec, &msg) || !msg.BitMap)
        return FALSE;
    *bm = msg.BitMap;
    *buffer = msg.Buffer;
    return TRUE;
}

/*
 * Session open / close
 */
BOOL HIDDNouveauDecodeOpen(OOP_Class *cl, OOP_Object *display, struct HIDDNouveauDecode *dec, LONG profile,
    ULONG width, ULONG height, struct Hook *resolve)
{
    struct CardData *carddata = &(SD(cl)->carddata);
    const struct nvdec_codec *c = nvdec_codec_for(profile);
    ULONG i;

    if (!c || !resolve || !width || !height)
        return FALSE;
    decode_lock();
    if (!nvdec_probe(carddata))
    {
        decode_unlock();
        return FALSE;
    }
    memset(dec, 0, sizeof(*dec));
    dec->carddata = carddata;
    dec->display = display;
    dec->bmclass = SD(cl)->bmclass;
    dec->resolve = resolve;
    dec->profile = profile;
    dec->ops = c->ops;
    dec->width = (width + 15) & ~15UL;      /* macroblock aligned coded size */
    dec->height = (height + 15) & ~15UL;

    /* object handles are unique per client on GSP-RM: number the sessions (the probe used 0) */
    decode_sessions = (decode_sessions % 15) + 1;
    if (nvdec_channel_new(carddata, NVDEC_HANDLE_BASE | (decode_sessions << 16),
                          &dec->channel, &dec->pushbuf, &dec->nvdec, &dec->oclass))
        goto fail;
    /* the pool's sample size follows the targets (AV1 profile 0 is 8 or 10-bit): first frame */
    dec->poolslots = c->maxrefs + 1;
    for (i = 0; i < NOUVEAU_DECODE_RING; i++)
        if (!nvdec_frame_alloc(carddata, &dec->ring[i], 2 * 1024 * 1024))
            goto fail;
    if (!nvdec_bo_new(carddata, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 4096, 4096, NULL, &dec->sem))
        goto fail;
    memset(dec->sem->map, 0, 4096);
    nouveau_staging_to_gpu(dec->sem->map, 4096);

    if (!dec->ops->open(dec))
        goto fail;
    D(bug("[Nouveau] NVDEC: session %p: profile %ld %lux%lu, class %04lx, %lu picture slots of %lu KB\n",
        dec, (long)profile, (unsigned long)dec->width, (unsigned long)dec->height,
        (unsigned long)dec->oclass, (unsigned long)dec->nslots, (unsigned long)(dec->slotsize / 1024)));
    decode_unlock();
    return TRUE;

fail:
    HIDDNouveauDecodeClose(dec);
    decode_unlock();
    return FALSE;
}

/* Wait for everything the session submitted (its semaphore fence) */
static VOID nvdec_session_idle(struct HIDDNouveauDecode *dec)
{
    if (dec->sem && dec->frames)
        nouveau_bo_wait(dec->sem, NOUVEAU_BO_RDWR, dec->carddata->client);
}

VOID HIDDNouveauDecodeClose(struct HIDDNouveauDecode *dec)
{
    ULONG i;

    if (!dec->carddata)
        return;
    decode_lock();
    /* settles waiting on this session without the lock finish first */
    while (dec->waiters)
    {
        decode_unlock();
        nvdec_pause();
        decode_lock();
    }
    if (!dec->dead)
        nvdec_session_idle(dec);
    /* targets still waiting for a copy-out get it; then they forget the session */
    for (i = 0; i < dec->nslots; i++)
    {
        if (dec->slot[i].valid && dec->slot[i].bm)
        {
            struct HIDDNouveauBitMapData *bmdata = OOP_INST_DATA(dec->bmclass, dec->slot[i].bm);
            ULONG b = dec->slot[i].buffer;

            if ((b < bmdata->nbufs) && (bmdata->buf[b].dec == dec))
            {
                HIDDNouveauDecodeSettle(dec->carddata, bmdata, b, TRUE, 0xffffffffUL);
                bmdata->buf[b].dec = NULL;
                bmdata->buf[b].decstate = NOUVEAU_DECSTATE_NONE;
            }
        }
        dec->slot[i].valid = FALSE;
    }
    if (dec->ops)
        dec->ops->close(dec);
    for (i = 0; i < dec->nslots; i++)
        if (dec->slot[i].bo)
            nouveau_bo_ref(NULL, &dec->slot[i].bo);
    dec->nslots = 0;
    for (i = 0; i < NOUVEAU_DECODE_RING; i++)
        nvdec_frame_free(&dec->ring[i]);
    if (dec->coloc)
        nouveau_bo_ref(NULL, &dec->coloc);
    if (dec->history)
        nouveau_bo_ref(NULL, &dec->history);
    if (dec->sem)
        nouveau_bo_ref(NULL, &dec->sem);
    nvdec_channel_del(&dec->channel, &dec->pushbuf, &dec->nvdec);
    if (dec->errors)
        nvlog("[Nouveau] NVDEC: session %p closed after %lu frames, %lu failed\n", dec,
            (unsigned long)dec->frames, (unsigned long)dec->errors);
    dec->carddata = NULL;
    decode_unlock();
}

/*
 * Decode targets (bitmap side)
 */
BOOL HIDDNouveauDecodeTargetInit(struct CardData *carddata, struct HIDDNouveauBitMapData *bmdata)
{
    ULONG i;

    if (!nouveau_decode_enable)
        return FALSE;
    for (i = 0; i < NOUVEAU_MAX_BUFFERS; i++)
    {
        bmdata->buf[i].dec = NULL;
        bmdata->buf[i].decstate = NOUVEAU_DECSTATE_NONE;
    }
    return TRUE;
}

VOID HIDDNouveauDecodeTargetFree(struct CardData *carddata, struct HIDDNouveauBitMapData *bmdata)
{
    ULONG i, s;

    decode_lock();
    for (i = 0; i < bmdata->nbufs; i++)
    {
        struct HIDDNouveauDecode *dec = bmdata->buf[i].dec;

        if (!dec)
            continue;
        HIDDNouveauDecodeSettle(carddata, bmdata, i, TRUE, 0xffffffffUL);
        /* the session's pool must not point at a bitmap that is going away */
        for (s = 0; s < dec->nslots; s++)
        {
            if (dec->slot[s].bm && (OOP_INST_DATA(dec->bmclass, dec->slot[s].bm) == bmdata))
            {
                dec->slot[s].bm = NULL;
                dec->slot[s].valid = FALSE;
            }
        }
        bmdata->buf[i].dec = NULL;
        bmdata->buf[i].decstate = NOUVEAU_DECSTATE_NONE;
        for (s = 0; s < NOUVEAU_DECODE_RING; s++)
            if (dec->ring[s].bm && (OOP_INST_DATA(dec->bmclass, dec->ring[s].bm) == bmdata))
                dec->ring[s].bm = NULL;
    }
    decode_unlock();
}

static inline BOOL nvdec_seq_done(struct HIDDNouveauDecode *dec, ULONG seq)
{
    volatile ULONG *sem = (volatile ULONG *)dec->sem->map;
    ULONG v;

    /* word 1: the flushed release after the decode (word 0 is the bare notify) */
    nouveau_staging_from_gpu((APTR)sem, 2 * sizeof(ULONG));
    v = sem[1];
    return (LONG)(v - seq) >= 0;
}

/* The decoder's status word for a frame, 0 = decoded without error */
static ULONG nvdec_frame_status(struct HIDDNouveauDecode *dec, struct HIDDNouveauDecodeFrame *f)
{
    ULONG *status = (ULONG *)((UBYTE *)f->picsetup->map + NOUVEAU_DECODE_STATUS_OFFSET);

    nouveau_staging_from_gpu(status, 64);
    f->checked = TRUE;
    return dec->ops->status(dec, status);
}

static ULONG nvdec_frame_error(struct HIDDNouveauDecode *dec, ULONG seq)
{
    ULONG i;

    for (i = 0; i < NOUVEAU_DECODE_RING; i++)
        if (dec->ring[i].seq == seq)
            return nvdec_frame_status(dec, &dec->ring[i]);
    return 0;       /* already judged when its ring entry was reused */
}

/*
 * A ring entry about to be reused: if nobody read its frame's status, do
 * it now and give the verdict to the target buffer, which may still be
 * waiting to be settled.
 */
static VOID nvdec_frame_retire(struct HIDDNouveauDecode *dec, struct HIDDNouveauDecodeFrame *f)
{
    if (!f->seq || f->checked || !f->bm)
        return;
    if (nvdec_frame_status(dec, f) && (OOP_OCLASS(f->bm) == dec->bmclass))
    {
        struct HIDDNouveauBitMapData *bmdata = OOP_INST_DATA(dec->bmclass, f->bm);

        if ((f->buffer < bmdata->nbufs) && (bmdata->buf[f->buffer].dec == dec) &&
            (bmdata->buf[f->buffer].decseq == f->seq) &&
            (bmdata->buf[f->buffer].decstate == NOUVEAU_DECSTATE_DECODING))
            bmdata->buf[f->buffer].decstate = NOUVEAU_DECSTATE_ERROR;
    }
}

/*
 * Copy engine detile of one plane: block-linear source (the pool) to the
 * pitch-linear target. The same method sequence as the hidd's copy helper
 * (nouveau_copya0b5.c), plus the GOB kind on Blackwell's class: from
 * Blackwell on, the block-linear byte order depends on the element size
 * (NIL's GOBType::choose; the copy class' SET_SRC_BLOCK_SIZE_KIND_BPP), and
 * the decoder writes its 8-bit samples in the 8-bit order. Without the
 * field the engine reads the 32-bit order and scrambles every GOB.
 */
#define CE_SUBC                 4
#define CE_SET_SRC_BLOCK_SIZE   0x0728
#define CE_SET_DST_BLOCK_SIZE   0x070c
#define CE_OFFSET_IN_UPPER      0x0400
#define CE_LAUNCH_DMA           0x0300
#define CE_KIND_BPP_BL_8        (1 << 16)   /* NVCAB5_SET_SRC_BLOCK_SIZE_KIND_BPP_BL_8 */
#define CE_KIND_BPP_BL_16       (2 << 16)   /* ..._BL_16: NV12 chroma's 2-byte UV pairs, P010 luma */
#define CE_KIND_BPP_BL_32       (0 << 16)   /* ..._BL_32 (Turing 2D order): P010 chroma's 4-byte UV pairs */

static inline VOID ce_begin(struct nouveau_pushbuf *push, ULONG mthd, ULONG count)
{
    *push->cur++ = 0x20000000 | (count << 16) | (CE_SUBC << 13) | (mthd / 4);
}

static BOOL nvdec_ce_detile(struct CardData *carddata, struct nouveau_bo *src, ULONG srcoff, ULONG srcpitch,
    ULONG srcrows, ULONG kind, struct nouveau_bo *dst, ULONG dstoff, ULONG dstpitch, ULONG w, ULONG h)
{
    struct nouveau_pushbuf *push = carddata->ce_pushbuf;
    struct nouveau_pushbuf_refn refs[] = {
        { src, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD },
        { dst, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR },
    };
    ULONG blocksize = 0x00001000 | src->config.nvc0.tile_mode;   /* 1 GOB wide, FERMI_8 GOB height */
    uint64_t saddr = src->offset + srcoff, daddr = dst->offset + dstoff;

    if (!carddata->NvCopy || !src->config.nvc0.memtype)
        return FALSE;
    if (carddata->NvCopy->oclass >= 0xcab5)
        blocksize |= kind;
    if (nouveau_pushbuf_space(push, 64, 0, 0) || nouveau_pushbuf_refn(push, refs, 2))
        return FALSE;
    ce_begin(push, CE_SET_SRC_BLOCK_SIZE, 6);
    *push->cur++ = blocksize;
    *push->cur++ = srcpitch;            /* width of the block-linear surface, bytes */
    *push->cur++ = srcrows;
    *push->cur++ = 1;
    *push->cur++ = 0;
    *push->cur++ = 0;                   /* origin (0,0) */
    ce_begin(push, CE_SET_DST_BLOCK_SIZE, 6);
    *push->cur++ = 0x00001000;
    *push->cur++ = dstpitch;
    *push->cur++ = h;
    *push->cur++ = 1;
    *push->cur++ = 0;
    *push->cur++ = 0;
    ce_begin(push, CE_OFFSET_IN_UPPER, 8);
    *push->cur++ = (ULONG)(saddr >> 32);
    *push->cur++ = (ULONG)saddr;
    *push->cur++ = (ULONG)(daddr >> 32);
    *push->cur++ = (ULONG)daddr;
    *push->cur++ = srcpitch;
    *push->cur++ = dstpitch;
    *push->cur++ = w;                   /* line length, bytes */
    *push->cur++ = h;
    ce_begin(push, CE_LAUNCH_DMA, 1);
    *push->cur++ = 0x00000206 | 0x00000100;     /* non-pipelined, flush, multi-line; dst pitch, src block-linear */
    return TRUE;
}

/* Copy engine: pool slot (block-linear) -> target buffer (pitch-linear NV12) */
static BOOL nvdec_copy_out(struct HIDDNouveauDecode *dec, struct HIDDNouveauBitMapData *bmdata, ULONG buffer, LONG slot)
{
    struct CardData *carddata = dec->carddata;
    struct nouveau_bo *src = dec->slot[slot].bo;
    struct nouveau_bo *dst = bmdata->buf[buffer].bo;
    ULONG w = bmdata->drawable.width, h = bmdata->drawable.height;
    /* the block-linear planes' heights: whole blocks (the slot is 4 KB padded beyond) */
    ULONG srcrows = (dec->height + NVDEC_POOL_BLOCK_ROWS - 1) & ~(NVDEC_POOL_BLOCK_ROWS - 1);
    ULONG chromarows = ((dec->height / 2) + NVDEC_POOL_BLOCK_ROWS - 1) & ~(NVDEC_POOL_BLOCK_ROWS - 1);
    BOOL ok;

    if (!src || !dst || (w > dec->width) || (h > dec->height))
        return FALSE;
    LOCK_ENGINE
    /* luma: 1-byte samples; chroma: interleaved 2-byte UV pairs, the 16-bit order on Blackwell */
    /*
     * Blackwell orders a GOB by element size, and the decoder's codec blocks
     * differ: H.264 writes the element-size order (luma samples, chroma UV
     * pairs of twice the size); HEVC (a different core) writes the 32-bit
     * order for its pictures (GB207, matched on a dumped frame).
     */
    {
        ULONG lk = dec->gob32 ? CE_KIND_BPP_BL_32 : (dec->bpp == 2) ? CE_KIND_BPP_BL_16 : CE_KIND_BPP_BL_8;
        ULONG ck = (dec->gob32 || (dec->bpp == 2)) ? CE_KIND_BPP_BL_32 : CE_KIND_BPP_BL_16;

        ok = nvdec_ce_detile(carddata, src, 0, dec->pitch, srcrows, lk, dst, 0, bmdata->pitch, w * dec->bpp, h);
        if (ok)
            ok = nvdec_ce_detile(carddata, src, dec->chromaoff, dec->pitch, chromarows, ck,
                                 dst, bmdata->planeoff[0], bmdata->pitch, w * dec->bpp, h / 2);
    }
    if (ok)
        nouveau_pushbuf_kick(carddata->ce_pushbuf);
    UNLOCK_ENGINE
    if (!ok)
        nvlog("[Nouveau] NVDEC: copy-out of slot %ld to buffer %lu failed\n", (long)slot, (unsigned long)buffer);
    return ok;
}

/* Sleep a little while polling a timed wait */
static VOID nvdec_pause(VOID)
{
    struct Library *DOSBase = OpenLibrary("dos.library", 0);

    if (DOSBase)
    {
        Delay(1);
        CloseLibrary(DOSBase);
    }
}

/*
 * Wait for the engine without holding the decode lock: a stalled engine must
 * not freeze every task that touches a decode target (the compositor commits
 * the video plane from input.device; holding the lock through a dead
 * channel's fence waits froze the whole UI). The session cannot go away
 * meanwhile: it counts its waiters and Close waits for them.
 */
/*
 * The host fence of the session's last submission, polled. It is taken on the
 * semaphore buffer: every frame lists that one for writing. A read-only wait
 * on a pool picture returns at once once later frames list the picture as a
 * reference (libdrm only waits when the buffer's last listed use was a
 * write), and the copy-out then read rows the engine was still writing.
 */
static BOOL nvdec_fence_done(struct HIDDNouveauDecode *dec)
{
    return !nouveau_bo_wait(dec->sem, NOUVEAU_BO_RDWR | NOUVEAU_BO_NOBLOCK, dec->carddata->client);
}

#define NVDEC_STALL_MS      3000    /* a frame takes under a millisecond: no answer by then = stopped */

static BOOL nvdec_wait_unlocked(struct HIDDNouveauDecode *dec, ULONG seq, struct nouveau_bo *slotbo,
    ULONG timeoutms)
{
    ULONG waited = 0, limit = (timeoutms == 0xffffffffUL) ? NVDEC_STALL_MS : timeoutms;
    BOOL done = FALSE;

    /*
     * Only the semaphore word is read unlocked. libdrm is not thread safe: a
     * buffer wait may kick the session's push buffer, and doing that from
     * the compositor while the decoding task builds its next frame corrupted
     * the push records (rejected as "push 0 buffer not in list").
     */
    dec->waiters++;
    for (;;)
    {
        if (nvdec_seq_done(dec, seq) && nvdec_fence_done(dec))
        {
            done = TRUE;
            break;
        }
        if ((waited >= limit) || dec->dead)
            break;
        decode_unlock();
        nvdec_pause();
        decode_lock();
        waited += 20;
    }
    dec->waiters--;
    return done;
}

/* For a codec that needs a finished frame's results (VP9's symbol counters); called with the lock held */
BOOL HIDDNouveauDecodeWait(struct HIDDNouveauDecode *dec, ULONG seq)
{
    if (nvdec_seq_done(dec, seq))
        return TRUE;
    if (nvdec_wait_unlocked(dec, seq, NULL, 0xffffffffUL))
        return TRUE;
    nvdec_session_kill(dec, "the decoder stopped responding");
    return FALSE;
}

ULONG HIDDNouveauDecodeSettle(struct CardData *carddata, struct HIDDNouveauBitMapData *bmdata, ULONG buffer,
    BOOL wait, ULONG timeoutms)
{
    struct HIDDNouveauDecode *dec;
    ULONG result = vHidd_DecodeStatus_Ready;

    if (buffer >= bmdata->nbufs)
        return vHidd_DecodeStatus_Error;
    decode_lock();
retry:
    dec = bmdata->buf[buffer].dec;
    switch (bmdata->buf[buffer].decstate)
    {
    case NOUVEAU_DECSTATE_NONE:
        break;

    case NOUVEAU_DECSTATE_ERROR:
        result = vHidd_DecodeStatus_Error;
        break;

    case NOUVEAU_DECSTATE_DECODING:
    {
        ULONG seq = bmdata->buf[buffer].decseq;
        struct nouveau_bo *slotbo;

        if (!dec)
        {
            bmdata->buf[buffer].decstate = NOUVEAU_DECSTATE_NONE;
            break;
        }
        slotbo = dec->slot[bmdata->buf[buffer].decslot].bo;
        /*
         * Finished means: the engine's semaphore (end of its method
         * processing) and the host fence on the pool picture (its output
         * DMA idle: on the GB207 the last 16 KB per plane were still being
         * written when the semaphore fired).
         */
        if (!nvdec_seq_done(dec, seq) || !nvdec_fence_done(dec))
        {
            BOOL done;

            if (!wait)
            {
                result = vHidd_DecodeStatus_Busy;
                break;
            }
            if (dec->dead)
            {
                bmdata->buf[buffer].decstate = NOUVEAU_DECSTATE_ERROR;
                result = vHidd_DecodeStatus_Error;
                break;
            }
            done = nvdec_wait_unlocked(dec, seq, slotbo, timeoutms);
            /* the buffer may have moved on while unlocked */
            if ((bmdata->buf[buffer].dec != dec) || (bmdata->buf[buffer].decseq != seq) ||
                (bmdata->buf[buffer].decstate != NOUVEAU_DECSTATE_DECODING))
                goto retry;
            if (!done)
            {
                if (timeoutms == 0xffffffffUL)
                {
                    nvdec_session_kill(dec, "the decoder stopped responding");
                    bmdata->buf[buffer].decstate = NOUVEAU_DECSTATE_ERROR;
                    result = vHidd_DecodeStatus_Error;
                }
                else
                    result = vHidd_DecodeStatus_Busy;
                break;
            }
        }
        if (nvdec_frame_error(dec, seq))
        {
            bmdata->buf[buffer].decstate = NOUVEAU_DECSTATE_ERROR;
            result = vHidd_DecodeStatus_Error;
            break;
        }
        if (!nvdec_copy_out(dec, bmdata, buffer, bmdata->buf[buffer].decslot))
        {
            bmdata->buf[buffer].decstate = NOUVEAU_DECSTATE_ERROR;
            result = vHidd_DecodeStatus_Error;
            break;
        }
        bmdata->buf[buffer].decstate = NOUVEAU_DECSTATE_COPYING;
    }
        /* fall through */

    case NOUVEAU_DECSTATE_COPYING:
        if (!wait)
        {
            result = vHidd_DecodeStatus_Busy;
            break;
        }
        {
            /* the copy engine's work: short; a libdrm call, so under the lock */
            nouveau_bo_wait(bmdata->buf[buffer].bo, NOUVEAU_BO_RDWR, carddata->client);
            bmdata->buf[buffer].decstate = NOUVEAU_DECSTATE_NONE;
        }
        break;
    }
    decode_unlock();
    return result;
}

/*
 * The decode interface vectors (gfx.hidd calls them with iface = dec)
 */
static BOOL nvdec_target_ok(struct HIDDNouveauDecode *dec, OOP_Object *bm, ULONG buffer,
    struct HIDDNouveauBitMapData **bmdata)
{
    if (!bm || (OOP_OCLASS(bm) != dec->bmclass))
        return FALSE;
    *bmdata = OOP_INST_DATA(dec->bmclass, bm);
    if (!(*bmdata)->decodetarget || (buffer >= (*bmdata)->nbufs) || !(*bmdata)->buf[buffer].bo)
        return FALSE;
    if (((*bmdata)->drawable.width > dec->width) || ((*bmdata)->drawable.height > dec->height))
        return FALSE;
    return TRUE;
}

static BOOL nvdec_BeginFrame(APTR obj, OOP_Object *bitmap, ULONG buffer)
{
    struct HIDDNouveauDecode *dec = obj;
    struct HIDDNouveauBitMapData *bmdata;
    struct HIDDNouveauDecodeFrame *f;
    BOOL ok;

    if (!dec->carddata || dec->cur.open || dec->dead)
        return FALSE;
    decode_lock();
    if (!nvdec_target_ok(dec, bitmap, buffer, &bmdata))
    {
        decode_unlock();
        nvlog("[Nouveau] NVDEC: BeginFrame: %p/%lu is not a usable decode target\n", bitmap, (unsigned long)buffer);
        return FALSE;
    }
    if (!dec->nslots)
    {
        dec->bpp = (bmdata->videoformat == vHidd_VideoFmt_P010) ? 2 : 1;
        if (!nvdec_pool_alloc(dec, dec->poolslots))
        {
            decode_unlock();
            return FALSE;
        }
    }
    else if (((bmdata->videoformat == vHidd_VideoFmt_P010) ? 2 : 1) != dec->bpp)
    {
        decode_unlock();
        nvlog("[Nouveau] NVDEC: BeginFrame: target sample size differs from the session's pictures\n");
        return FALSE;
    }
    /* a buffer decoded into again: whatever it still waits for is finished first */
    if (bmdata->buf[buffer].dec)
        HIDDNouveauDecodeSettle(dec->carddata, bmdata, buffer, TRUE, 0xffffffffUL);
    /* the ring entry must be idle: its last frame completed and was judged */
    f = &dec->ring[dec->ringpos];
    if (f->seq && !nvdec_seq_done(dec, f->seq))
    {
        ULONG waited = 0;

        /* bounded and unlocked, as in HIDDNouveauDecodeSettle */
        dec->waiters++;
        decode_unlock();
        while (!nvdec_seq_done(dec, f->seq) && (waited < NVDEC_STALL_MS))
        {
            nvdec_pause();
            waited += 20;
        }
        decode_lock();
        dec->waiters--;
        if (!nvdec_seq_done(dec, f->seq))
        {
            nvdec_session_kill(dec, "the decoder stopped responding");
            decode_unlock();
            return FALSE;
        }
    }
    nvdec_frame_retire(dec, f);
    dec->cur.open = TRUE;
    dec->cur.failed = FALSE;
    dec->cur.bm = bitmap;
    dec->cur.buffer = buffer;
    dec->cur.slot = -1;
    dec->cur.nslices = 0;
    dec->cur.bitstreamlen = 0;
    ok = dec->ops->begin(dec);
    if (!ok)
        dec->cur.failed = TRUE;
    decode_unlock();
    return ok;
}

static BOOL nvdec_SubmitParams(APTR obj, ULONG type, APTR data, ULONG size, ULONG count)
{
    struct HIDDNouveauDecode *dec = obj;
    BOOL ok = FALSE;

    if (!dec->carddata || !dec->cur.open || dec->cur.failed || !data || !size || !count)
        return FALSE;
    decode_lock();
    ok = dec->ops->params(dec, type, data, size, count);
    if (!ok)
        dec->cur.failed = TRUE;
    decode_unlock();
    return ok;
}

static BOOL nvdec_SubmitBitstream(APTR obj, APTR data, ULONG size)
{
    struct HIDDNouveauDecode *dec = obj;
    BOOL ok = FALSE;

    if (!dec->carddata || !dec->cur.open || dec->cur.failed || !data || !size)
        return FALSE;
    decode_lock();
    ok = dec->ops->bitstream(dec, data, size);
    if (!ok)
        dec->cur.failed = TRUE;
    decode_unlock();
    return ok;
}

static BOOL nvdec_EndFrame(APTR obj)
{
    struct HIDDNouveauDecode *dec = obj;
    struct HIDDNouveauBitMapData *bmdata;
    struct HIDDNouveauDecodeFrame *f;
    BOOL ok = FALSE;

    if (!dec->carddata || !dec->cur.open)
        return FALSE;
    decode_lock();
    dec->cur.open = FALSE;
    if (dec->cur.failed || !dec->cur.nslices || !dec->cur.bitstreamlen)
    {
        decode_unlock();
        return FALSE;
    }
    if (!nvdec_target_ok(dec, dec->cur.bm, dec->cur.buffer, &bmdata))
    {
        decode_unlock();
        return FALSE;
    }
    f = &dec->ring[dec->ringpos];
    f->seq = ++dec->seq;
    f->bm = dec->cur.bm;
    f->buffer = dec->cur.buffer;
    f->checked = FALSE;
    dec->frames++;
    ok = dec->ops->submit(dec, f);
    if (ok && (dec->cur.slot >= 0))
    {
        LONG slot = dec->cur.slot;

        dec->slot[slot].valid = TRUE;
        dec->slot[slot].bm = dec->cur.bm;
        dec->slot[slot].buffer = dec->cur.buffer;
        dec->slot[slot].lastuse = dec->frames;
        bmdata->buf[dec->cur.buffer].dec = dec;
        bmdata->buf[dec->cur.buffer].decseq = f->seq;
        bmdata->buf[dec->cur.buffer].decslot = slot;
        bmdata->buf[dec->cur.buffer].decstate = NOUVEAU_DECSTATE_DECODING;
        dec->ringpos = (dec->ringpos + 1) % NOUVEAU_DECODE_RING;
    }
    else
    {
        f->seq = 0;
        dec->seq--;
        dec->frames--;
        ok = FALSE;
    }
    decode_unlock();
    return ok;
}

static ULONG nvdec_Sync(APTR obj, OOP_Object *bitmap, ULONG buffer, ULONG timeoutms)
{
    struct HIDDNouveauDecode *dec = obj;
    struct HIDDNouveauBitMapData *bmdata;

    if (!dec->carddata || !bitmap || (OOP_OCLASS(bitmap) != dec->bmclass))
        return vHidd_DecodeStatus_Error;
    bmdata = OOP_INST_DATA(dec->bmclass, bitmap);
    return HIDDNouveauDecodeSettle(dec->carddata, bmdata, buffer, TRUE, timeoutms);
}

static ULONG nvdec_Status(APTR obj, OOP_Object *bitmap, ULONG buffer)
{
    struct HIDDNouveauDecode *dec = obj;
    struct HIDDNouveauBitMapData *bmdata;

    if (!dec->carddata || !bitmap || (OOP_OCLASS(bitmap) != dec->bmclass))
        return vHidd_DecodeStatus_Error;
    bmdata = OOP_INST_DATA(dec->bmclass, bitmap);
    return HIDDNouveauDecodeSettle(dec->carddata, bmdata, buffer, FALSE, 0);
}

const struct HIDD_DecodeInterface HIDDNouveauDecodeVectors =
{
    sizeof(struct HIDD_DecodeInterface),
    nvdec_BeginFrame,
    nvdec_SubmitParams,
    nvdec_SubmitBitstream,
    nvdec_EndFrame,
    nvdec_Sync,
    nvdec_Status
};
