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
    ULONG   codec;          /* NOUVEAU_DECODE_CODEC_* */
    ULONG   maxw, maxh;
    ULONG   maxrefs;
};

static const struct nvdec_codec nvdec_codecs[] =
{
    { VAProfileH264ConstrainedBaseline, NOUVEAU_DECODE_CODEC_H264, 4096, 4096, 16 },
    { VAProfileH264Main,                NOUVEAU_DECODE_CODEC_H264, 4096, 4096, 16 },
    { VAProfileH264High,                NOUVEAU_DECODE_CODEC_H264, 4096, 4096, 16 },
    { VAProfileNone, 0, 0, 0, 0 }
};

static const struct nvdec_codec *nvdec_codec_for(LONG profile)
{
    const struct nvdec_codec *c;

    for (c = nvdec_codecs; c->codec; c++)
        if (c->profile == profile)
            return c;
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
        caps->VideoFmtMask  = (1UL << vHidd_VideoFmt_NV12);
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
    dec->pitch = (dec->width + 63) & ~63UL;
    dec->lumasize = dec->pitch * ((dec->height + NVDEC_POOL_BLOCK_ROWS - 1) & ~(NVDEC_POOL_BLOCK_ROWS - 1));
    dec->lumasize = (dec->lumasize + 4095) & ~4095UL;
    dec->chromaoff = dec->lumasize;
    dec->slotsize = dec->lumasize +
        dec->pitch * (((dec->height / 2) + NVDEC_POOL_BLOCK_ROWS - 1) & ~(NVDEC_POOL_BLOCK_ROWS - 1));
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
    dec->codec = c->codec;
    dec->width = (width + 15) & ~15UL;      /* macroblock aligned coded size */
    dec->height = (height + 15) & ~15UL;

    /* object handles are unique per client on GSP-RM: number the sessions (the probe used 0) */
    decode_sessions = (decode_sessions % 15) + 1;
    if (nvdec_channel_new(carddata, NVDEC_HANDLE_BASE | (decode_sessions << 16),
                          &dec->channel, &dec->pushbuf, &dec->nvdec, &dec->oclass))
        goto fail;
    if (!nvdec_pool_alloc(dec, c->maxrefs + 1))
        goto fail;
    for (i = 0; i < NOUVEAU_DECODE_RING; i++)
        if (!nvdec_frame_alloc(carddata, &dec->ring[i], 2 * 1024 * 1024))
            goto fail;
    if (!nvdec_bo_new(carddata, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 4096, 4096, NULL, &dec->sem))
        goto fail;
    memset(dec->sem->map, 0, 4096);
    nouveau_staging_to_gpu(dec->sem->map, 4096);

    switch (dec->codec)
    {
    case NOUVEAU_DECODE_CODEC_H264:
        if (!HIDDNouveauDecodeH264Open(dec))
            goto fail;
        break;
    default:
        goto fail;
    }
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
    switch (dec->codec)
    {
    case NOUVEAU_DECODE_CODEC_H264:
        HIDDNouveauDecodeH264Close(dec);
        break;
    }
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
    return HIDDNouveauDecodeStatusError(dec, status);
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
#define CE_KIND_BPP_BL_16       (2 << 16)   /* ..._BL_16: the NV12 chroma plane's 2-byte UV pairs */

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
    ok = nvdec_ce_detile(carddata, src, 0, dec->pitch, srcrows, CE_KIND_BPP_BL_8, dst, 0, bmdata->pitch, w, h);
    if (ok)
        ok = nvdec_ce_detile(carddata, src, dec->chromaoff, dec->pitch, chromarows, CE_KIND_BPP_BL_16,
                             dst, bmdata->planeoff[0], bmdata->pitch, w, h / 2);
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

ULONG HIDDNouveauDecodeSettle(struct CardData *carddata, struct HIDDNouveauBitMapData *bmdata, ULONG buffer,
    BOOL wait, ULONG timeoutms)
{
    struct HIDDNouveauDecode *dec;
    ULONG result = vHidd_DecodeStatus_Ready;

    if (buffer >= bmdata->nbufs)
        return vHidd_DecodeStatus_Error;
    decode_lock();
    dec = bmdata->buf[buffer].dec;
    switch (bmdata->buf[buffer].decstate)
    {
    case NOUVEAU_DECSTATE_NONE:
        break;

    case NOUVEAU_DECSTATE_ERROR:
        result = vHidd_DecodeStatus_Error;
        break;

    case NOUVEAU_DECSTATE_DECODING:
        if (!dec)
        {
            bmdata->buf[buffer].decstate = NOUVEAU_DECSTATE_NONE;
            break;
        }
        if (!nvdec_seq_done(dec, bmdata->buf[buffer].decseq))
        {
            if (!wait)
            {
                result = vHidd_DecodeStatus_Busy;
                break;
            }
            if (timeoutms == 0xffffffffUL)
            {
                /* the semaphore buffer carries the channel's fences */
                nouveau_bo_wait(dec->sem, NOUVEAU_BO_RDWR, carddata->client);
            }
            else
            {
                ULONG waited = 0;

                while (!nvdec_seq_done(dec, bmdata->buf[buffer].decseq))
                {
                    if (waited >= timeoutms)
                        break;
                    nvdec_pause();
                    waited += 20;
                }
            }
            if (!nvdec_seq_done(dec, bmdata->buf[buffer].decseq))
            {
                result = vHidd_DecodeStatus_Busy;
                break;
            }
        }
        /*
         * The engine's semaphore marks the end of its method processing,
         * not of its output DMA: the picture's last 16 KB per plane were
         * still being written when it fired (GB207, tail check). The host
         * fence the kernel put on the push buffer's buffers releases only
         * once the engine is idle, so require it too before reading.
         */
        {
            struct nouveau_bo *slotbo = dec->slot[bmdata->buf[buffer].decslot].bo;

            if (nouveau_bo_wait(slotbo, NOUVEAU_BO_RD | (wait ? 0 : NOUVEAU_BO_NOBLOCK), carddata->client))
            {
                result = vHidd_DecodeStatus_Busy;
                break;
            }
        }
        if (nvdec_frame_error(dec, bmdata->buf[buffer].decseq))
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
        /* fall through */

    case NOUVEAU_DECSTATE_COPYING:
        if (!wait)
        {
            result = vHidd_DecodeStatus_Busy;
            break;
        }
        nouveau_bo_wait(bmdata->buf[buffer].bo, NOUVEAU_BO_RDWR, carddata->client);
        bmdata->buf[buffer].decstate = NOUVEAU_DECSTATE_NONE;
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

    if (!dec->carddata || dec->cur.open)
        return FALSE;
    decode_lock();
    if (!nvdec_target_ok(dec, bitmap, buffer, &bmdata))
    {
        decode_unlock();
        nvlog("[Nouveau] NVDEC: BeginFrame: %p/%lu is not a usable decode target\n", bitmap, (unsigned long)buffer);
        return FALSE;
    }
    /* a buffer decoded into again: whatever it still waits for is finished first */
    if (bmdata->buf[buffer].dec)
        HIDDNouveauDecodeSettle(dec->carddata, bmdata, buffer, TRUE, 0xffffffffUL);
    /* the ring entry must be idle: its last frame completed and was judged */
    f = &dec->ring[dec->ringpos];
    if (f->seq && !nvdec_seq_done(dec, f->seq))
        nouveau_bo_wait(f->picsetup, NOUVEAU_BO_RDWR, dec->carddata->client);
    nvdec_frame_retire(dec, f);
    dec->cur.open = TRUE;
    dec->cur.failed = FALSE;
    dec->cur.bm = bitmap;
    dec->cur.buffer = buffer;
    dec->cur.slot = -1;
    dec->cur.nslices = 0;
    dec->cur.bitstreamlen = 0;
    switch (dec->codec)
    {
    case NOUVEAU_DECODE_CODEC_H264:
        ok = HIDDNouveauDecodeH264Begin(dec);
        break;
    default:
        ok = FALSE;
    }
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
    switch (dec->codec)
    {
    case NOUVEAU_DECODE_CODEC_H264:
        ok = HIDDNouveauDecodeH264Params(dec, type, data, size, count);
        break;
    }
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
    switch (dec->codec)
    {
    case NOUVEAU_DECODE_CODEC_H264:
        ok = HIDDNouveauDecodeH264Bitstream(dec, data, size);
        break;
    }
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
    switch (dec->codec)
    {
    case NOUVEAU_DECODE_CODEC_H264:
        ok = HIDDNouveauDecodeH264Submit(dec, f);
        break;
    }
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
