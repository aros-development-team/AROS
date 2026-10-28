#ifndef NOUVEAU_DECODE_H
#define NOUVEAU_DECODE_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Hardware video decoding (NVDEC) behind gfx.hidd's decode interface.
*/

#include "nouveau_intern.h"

/*
 * One decode session (moHidd_Display_GetDecodeInterface): its own channel
 * on the NVDEC runlist, the decoder class object, a pool of block-linear
 * reference pictures the decoder reads and writes, and a ring of per-frame
 * input buffers. Decoded pictures are copied (detiled) by the copy engine
 * into the pitch-linear target buffers the display scans out and the CPU
 * reads back. The base class allocates this structure after the vector
 * table (aHidd_Display_DecodeDataSize = sizeof(struct HIDDNouveauDecode)).
 */
#define NOUVEAU_DECODE_SLOTS        17      /* H.264 DPB (16) + the picture being decoded */
#define NOUVEAU_DECODE_RING         4       /* frames in flight per session */
#define NOUVEAU_DECODE_MAX_SLICES   512     /* slice offsets per frame */

struct HIDDNouveauDecodeSlot
{
    struct nouveau_bo   *bo;                /* luma at 0, chroma at chromaoff; block-linear */
    OOP_Object          *bm;                /* the (bitmap, buffer) this picture was decoded for */
    ULONG               buffer;
    ULONG               lastuse;            /* frame counter when last decoded into or referenced */
    BOOL                valid;              /* holds a decoded picture */
};

struct HIDDNouveauDecodeFrame
{
    struct nouveau_bo   *bitstream;         /* GART: the slices of one frame */
    struct nouveau_bo   *picsetup;          /* GART: picture setup + slice offsets + status */
    ULONG               bitstreamsize;
    ULONG               seq;                /* semaphore value written when the decode is done */
    OOP_Object          *bm;                /* the frame's target, for a status check at reuse */
    ULONG               buffer;
    BOOL                checked;            /* its status was read */
};

struct HIDDNouveauDecode
{
    struct CardData     *carddata;
    OOP_Object          *display;
    OOP_Class           *bmclass;           /* the nouveau bitmap class, for the targets */
    struct Hook         *resolve;           /* surface id -> (bitmap, buffer) */
    LONG                profile;            /* VAProfile */
    ULONG               codec;              /* NOUVEAU_DECODE_CODEC_* */
    ULONG               width, height;      /* coded picture size (macroblock aligned) */
    ULONG               pitch;              /* pool picture pitch (bytes, 64-aligned) */
    ULONG               lumasize;           /* bytes of one pool luma plane (block aligned) */
    ULONG               chromaoff;          /* offset of the chroma plane in a pool picture */
    ULONG               slotsize;           /* bytes of one pool picture */

    struct nouveau_object   *channel;       /* NVDEC runlist channel */
    struct nouveau_pushbuf  *pushbuf;
    struct nouveau_object   *nvdec;         /* the NVxxB0 decoder object */
    ULONG               oclass;             /* its class */

    struct HIDDNouveauDecodeSlot slot[NOUVEAU_DECODE_SLOTS];
    ULONG               nslots;
    struct HIDDNouveauDecodeFrame ring[NOUVEAU_DECODE_RING];
    ULONG               ringpos;
    struct nouveau_bo   *coloc;             /* VRAM: colocated motion data (per slot) */
    struct nouveau_bo   *history;           /* VRAM: decoder history / scratch */
    struct nouveau_bo   *sem;               /* GART: semaphore (seq) + status words */
    ULONG               frames;             /* frames submitted */
    BOOL                statuswarned;       /* the unlisted status word was logged */
    ULONG               errors;             /* frames the decoder reported as failed */
    ULONG               seq;                /* last semaphore value handed out */

    /* the frame being assembled between BeginFrame and EndFrame */
    struct
    {
        BOOL            open;
        BOOL            failed;
        OOP_Object      *bm;
        ULONG           buffer;
        LONG            slot;               /* pool slot decoded into */
        ULONG           nslices;
        ULONG           bitstreamlen;
        APTR            codecstate;         /* per-codec picture state (nouveau_decode_h264.c) */
    } cur;
};

/* codecs */
#define NOUVEAU_DECODE_CODEC_H264   1


/* Layout of a frame's picsetup buffer (GART, CPU mapped) */
#define NOUVEAU_DECODE_PICSETUP_SIZE        0x3000
#define NOUVEAU_DECODE_SLICEOFFS_OFFSET     0x1000  /* uint32 per slice + terminator */
#define NOUVEAU_DECODE_STATUS_OFFSET        0x2000  /* nvdec_status_s, zeroed per frame */

/* nouveau_decode.c helpers for the codec files */
LONG HIDDNouveauDecodeFindSlot(struct HIDDNouveauDecode *dec, OOP_Object *bm, ULONG buffer);
LONG HIDDNouveauDecodeAllocSlot(struct HIDDNouveauDecode *dec, OOP_Object *bm, ULONG buffer, ULONG avoid);
BOOL HIDDNouveauDecodeResolve(struct HIDDNouveauDecode *dec, ULONG surface, OOP_Object **bm, ULONG *buffer);
UBYTE *HIDDNouveauDecodeBitstreamReserve(struct HIDDNouveauDecode *dec, ULONG bytes);
VOID HIDDNouveauDecodeMethod(struct HIDDNouveauDecode *dec, ULONG mthd, ULONG data);

/* nouveau_decode_h264.c */
BOOL HIDDNouveauDecodeH264Open(struct HIDDNouveauDecode *dec);
VOID HIDDNouveauDecodeH264Close(struct HIDDNouveauDecode *dec);
BOOL HIDDNouveauDecodeH264Begin(struct HIDDNouveauDecode *dec);
BOOL HIDDNouveauDecodeH264Params(struct HIDDNouveauDecode *dec, ULONG type, APTR data, ULONG size, ULONG count);
BOOL HIDDNouveauDecodeH264Bitstream(struct HIDDNouveauDecode *dec, APTR data, ULONG size);
BOOL HIDDNouveauDecodeH264Submit(struct HIDDNouveauDecode *dec, struct HIDDNouveauDecodeFrame *f);
/* the error code in a frame's status words, 0 = none (any codec) */
ULONG HIDDNouveauDecodeStatusError(struct HIDDNouveauDecode *dec, ULONG *status);

/* nouveau_decode.c: implementation behind the display class methods */
VOID HIDDNouveauDecodeInit(VOID);               /* once, from the hidd's New */
ULONG HIDDNouveauDecodeQuery(OOP_Class *cl, OOP_Object *display, LONG profile, ULONG width, ULONG height,
    struct HIDD_DecodeCaps *caps);
BOOL HIDDNouveauDecodeOpen(OOP_Class *cl, OOP_Object *display, struct HIDDNouveauDecode *dec, LONG profile,
    ULONG width, ULONG height, struct Hook *resolve);
VOID HIDDNouveauDecodeClose(struct HIDDNouveauDecode *dec);
extern const struct HIDD_DecodeInterface HIDDNouveauDecodeVectors;

/* Per decode-target buffer state, kept in the bitmap (nouveau_bitmapclass.c) */
BOOL HIDDNouveauDecodeTargetInit(struct CardData *carddata, struct HIDDNouveauBitMapData *bmdata);
VOID HIDDNouveauDecodeTargetFree(struct CardData *carddata, struct HIDDNouveauBitMapData *bmdata);
/* Make buffer 'buffer' of a decode target hold its last decode: issue the
   copy-out if it is still pending and, with wait, wait for it. Returns a
   vHidd_DecodeStatus_* value. timeoutms ~0 = for ever (wait only) */
ULONG HIDDNouveauDecodeSettle(struct CardData *carddata, struct HIDDNouveauBitMapData *bmdata, ULONG buffer,
    BOOL wait, ULONG timeoutms);

#endif /* NOUVEAU_DECODE_H */
