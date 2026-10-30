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
#define NOUVEAU_DECODE_MAX_SLICES   8191    /* slice offsets per frame: one per macroblock at 1080p fits */

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
    const struct HIDDNouveauDecodeOps *ops; /* the codec's translation */
    ULONG               width, height;      /* coded picture size (macroblock aligned) */
    ULONG               bpp;                /* bytes per sample: 1 = 8-bit, 2 = 10/12-bit */
    ULONG               chalf;              /* chroma plane: half the rows (4:2:0), else all (4:2:2, 4:4:4) */
    ULONG               cwide;              /* chroma rows this many luma rows long (4:4:4: 2) */
    BOOL                gob32;              /* the codec's block writes the 32-bit GOB order (HEVC) */
    ULONG               poolslots;          /* pictures the pool gets (allocated on the first frame) */
    ULONG               rowalign;           /* picture row alignment the codec needs (0 = block rows) */
    BOOL                minimal;            /* emit only the methods NVIDIA's driver uses for this codec */
    ULONG               pitch;              /* pool picture pitch (bytes, 64-aligned) */
    ULONG               lumasize;           /* bytes of one pool luma plane (block aligned) */
    ULONG               chromaoff;          /* offset of the chroma plane in a pool picture */
    ULONG               slotsize;           /* bytes of one pool picture */
    BOOL                fields;             /* interlaced stream: a picture may be a single field */
    ULONG               weaverows;          /* pool pictures keep the fields apart: rows of one luma field (0 = frame order) */
    ULONG               cweaverows;         /* and of one chroma field; the bottom field starts that far in */

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
    BOOL                dead;               /* the engine stopped answering (channel killed) */
    ULONG               waiters;            /* settles waiting on it without the decode lock */
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



/*
 * Blackwell (NVCFB0): NVIDIA's driver gives the HEVC / VP9 / new-H.264 paths
 * one more buffer (method 0x4DC, after the intra-top buffer) and a field of
 * ceil(width / 128) + 1 in the setup (traced with the class reported as CFB0).
 */
#define NVDEC_CLASS_BLACKWELL               0xcfb0
#define NVDEC_SET_BLK_BUF_OFFSET            0x04DC
#define NVDEC_BLK_UNITS(w)                  ((((w) + 127) / 128) + 1)
#define NVDEC_BLK_BUF_SIZE(w)               (NVDEC_BLK_UNITS(w) * 16384)

/* Layout of a frame's picsetup buffer (GART, CPU mapped) */
#define NOUVEAU_DECODE_PICSETUP_SIZE        0xA000
#define NOUVEAU_DECODE_SLICEOFFS_OFFSET     0x1000  /* uint32 per slice + terminator (MAX_SLICES + 1) */
#define NOUVEAU_DECODE_STATUS_OFFSET        0x9000  /* nvdec_status_s, zeroed per frame */

/* nouveau_decode.c helpers for the codec files */
LONG HIDDNouveauDecodeFindSlot(struct HIDDNouveauDecode *dec, OOP_Object *bm, ULONG buffer);
LONG HIDDNouveauDecodeAllocSlot(struct HIDDNouveauDecode *dec, OOP_Object *bm, ULONG buffer, ULONG avoid);
BOOL HIDDNouveauDecodeResolve(struct HIDDNouveauDecode *dec, ULONG surface, OOP_Object **bm, ULONG *buffer);
UBYTE *HIDDNouveauDecodeBitstreamReserve(struct HIDDNouveauDecode *dec, ULONG bytes);
BOOL HIDDNouveauDecodeWait(struct HIDDNouveauDecode *dec, ULONG seq);   /* frame seq finished (lock held) */
VOID HIDDNouveauDecodeMethod(struct HIDDNouveauDecode *dec, ULONG mthd, ULONG data);
/*
 * Submit a frame whose picture setup, bitstream, slice offsets and status
 * area are in f: the common methods (application, control, buffers, the 17
 * picture slots, the codec's own methods as {method, value} pairs) and the
 * completion (EXECUTE notify, flushed semaphore release), then kick.
 * extra: the codec's own buffers the engine touches. map: for codecs whose
 * picture indices are fixed roles (VP8/VP9: LAST, GOLDEN, ...; MPEG-2/VC-1:
 * forward, backward), map[i] is the pool slot behind decoder index i (-1 =
 * the current picture); NULL = decoder index i is pool slot i (H.264, HEVC).
 * slot is then the decoder index of the current picture.
 */
BOOL HIDDNouveauDecodeEmit(struct HIDDNouveauDecode *dec, struct HIDDNouveauDecodeFrame *f, LONG slot,
    const LONG *map, ULONG appid, ULONG control, struct nouveau_bo **extra, ULONG nextra,
    const ULONG *mthds, ULONG nmthds);

/*
 * A codec's translation (nouveau_decode_<codec>.c): VA parameters and slice
 * data in, the decoder's picture setup and method stream out. The core
 * (nouveau_decode.c) owns sessions, the picture pool, fences and copy-out.
 */
struct HIDDNouveauDecodeOps
{
    BOOL  (*open)     (struct HIDDNouveauDecode *dec);          /* codec buffers, state */
    VOID  (*close)    (struct HIDDNouveauDecode *dec);
    BOOL  (*begin)    (struct HIDDNouveauDecode *dec);          /* a frame starts */
    BOOL  (*params)   (struct HIDDNouveauDecode *dec, ULONG type, APTR data, ULONG size, ULONG count);
    BOOL  (*bitstream)(struct HIDDNouveauDecode *dec, APTR data, ULONG size);
    BOOL  (*submit)   (struct HIDDNouveauDecode *dec, struct HIDDNouveauDecodeFrame *f);
    ULONG (*status)   (struct HIDDNouveauDecode *dec, ULONG *status);  /* error code of a finished frame, 0 = good */
};

extern const struct HIDDNouveauDecodeOps HIDDNouveauDecodeH264Ops;
extern const struct HIDDNouveauDecodeOps HIDDNouveauDecodeHEVCOps;
extern const struct HIDDNouveauDecodeOps HIDDNouveauDecodeMPEG2Ops;
extern const struct HIDDNouveauDecodeOps HIDDNouveauDecodeVC1Ops;
extern const struct HIDDNouveauDecodeOps HIDDNouveauDecodeVP8Ops;
extern const struct HIDDNouveauDecodeOps HIDDNouveauDecodeVP9Ops;
extern const struct HIDDNouveauDecodeOps HIDDNouveauDecodeAV1Ops;

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
/* CPU access to a decode target buffer through a GART copy: the address, or NULL to map
   the buffer itself; the unlock copies a written copy back (dirty NULL = all of it) */
UBYTE *HIDDNouveauDecodeLockCPU(struct CardData *carddata, struct HIDDNouveauBitMapData *bmdata, ULONG buffer);
VOID HIDDNouveauDecodeUnlockCPU(struct CardData *carddata, struct HIDDNouveauBitMapData *bmdata, ULONG buffer,
    struct Rectangle *dirty);
ULONG HIDDNouveauDecodeSettle(struct CardData *carddata, struct HIDDNouveauBitMapData *bmdata, ULONG buffer,
    BOOL wait, ULONG timeoutms);

#endif /* NOUVEAU_DECODE_H */
