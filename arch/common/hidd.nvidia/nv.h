#ifndef _NV_H
#define _NV_H
/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: private header file
    Lang: English
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/nodes.h>
#include <exec/execbase.h>
#include <exec/memory.h>

#include <dos/bptr.h>

#include <oop/oop.h>

#include <hidd/pci.h>
#include <hidd/graphics.h>

#include "nv_local.h"
#include "riva_hw.h"

#define IID_Hidd_Gfx_nVidia	"hidd.gfx.nv"
#define CLID_Hidd_Gfx_nVidia	"hidd.gfx.nv"

#define IID_Hidd_nvBitMap	"hidd.bitmap.nv"

enum {
    aoHidd_nvBitMap_Drawable,

    num_Hidd_nvBitMap_Attrs
};

#define aHidd_nvBitMap_Drawable (HiddNVidiaBitMapAttrBase + aoHidd_nvBitMap_Drawable)

#define IS_BM_ATTR(attr, idx) (((idx)=(attr)-HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
#define IS_NVBM_ATTR(attr, idx) (((idx)=(attr)-HiddNVidiaBitMapAttrBase) < num_Hidd_nvBitMap_Attrs)

extern UBYTE LIBEND;

#undef LIBBASETYPEPTR
#undef LIBBASETYPE

#define LIBBASETYPE	struct nvbase
#define LIBBASETYPEPTR	struct nvbase *

AROS_UFP3(LIBBASETYPEPTR, nv_init,
    AROS_UFHA(LIBBASETYPEPTR,	nvbase,	D0),
    AROS_UFHA(BPTR,		slist,	A0),
    AROS_UFHA(struct ExecBase*,	SysBase,A6));

typedef enum {
    NV04 = 0,	NV05,	NV05M64,    NV06,   NV10,
    NV11,       NV11M,  NV15,	    NV17,   NV17M,
    NV18,       NV18M,  NV20,	    NV25,   NV28,
    NV30,       NV31,   NV34,	    NV35,   NV36,

    CardType_Sizeof
} CardType;

typedef struct _sync {
    ULONG pixelc;
    ULONG flags;
    ULONG HDisplay, HSyncStart, HSyncEnd, HTotal;
    ULONG VDisplay, VSyncStart, VSyncEnd, VTotal;
} Sync;

typedef struct CardState {
    ULONG   bpp;
    ULONG   bitsPerPixel;
    ULONG   width;
    ULONG   height;
    ULONG   interlace;
    ULONG   repaint0;
    ULONG   repaint1;
    ULONG   screen;
    ULONG   scale;
    ULONG   dither;
    ULONG   extra;
    ULONG   pixel;
    ULONG   horiz;
    ULONG   arbitration0;
    ULONG   arbitration1;
    ULONG   vpll;
    ULONG   vpll2;
    ULONG   vpllB;
    ULONG   vpll2B;
    ULONG   pllsel;
    ULONG   general;
    ULONG   crtcOwner;
    ULONG   head;
    ULONG   head2;
    ULONG   config;
    ULONG   cursorConfig;
    ULONG   cursor0;
    ULONG   cursor1;
    ULONG   cursor2;
    ULONG   offset;
    ULONG   pitch;
    ULONG   pll;
    ULONG   pllB;
    ULONG   timingH;
    ULONG   timingV;
    ULONG   displayV;
    struct {
	UBYTE	attr[0x15];
	UBYTE	crtc[0x41];
	UBYTE	gra[0x09];
	UBYTE	seq[0x05];
	UBYTE	dac[256*3];
	UBYTE	misc;
    } Regs;
} RIVA_HW_STATE;

struct staticdata;

typedef struct Card {
    UWORD	    VendorID;
    UWORD	    ProductID;
    APTR	    FbAddress;
    UBYTE	    *FrameBuffer;
    ULONG	    FrameBufferSize;
    ULONG	    FbUsableSize;
    APTR	    Registers;
    CardType	    Type;
    BOOL	    FlatPanel;
    BOOL	    paletteEnabled;

    UWORD	    Architecture;
    UWORD	    Chipset;	    /* == ProductID */
    ULONG	    CrystalFreqKHz;
    ULONG	    RamAmountKBytes;
    ULONG	    MaxVClockFreqKHz;
    ULONG	    MinVClockFreqKHz;
    ULONG	    RamBandwidthKBytesPerSec;
    ULONG	    EnableIRQ;
    ULONG	    IO;
    ULONG	    VBlankBit;
    ULONG	    FifoFreeCount;
    ULONG	    FifoEmptyCount;
    ULONG	    CursorStart;
    ULONG	    flatPanel;
    ULONG	    CRTCnumber;
    ULONG	    Television;
    ULONG	    fpWidth;
    ULONG	    fpHeight;
    BOOL	    twoHeads;
    BOOL	    twoStagePLL;
    BOOL	    fpScaler;
    BOOL	    alphaCursor;
    ULONG	    cursorVisible;

    ULONG	    dmaPut;
    ULONG	    dmaCurrent;
    ULONG	    dmaFree;
    ULONG	    dmaMax;
    ULONG	    *dmaBase;

    ULONG	    currentROP;

    volatile ULONG	*PCRTC0;
    volatile ULONG	*PCRTC;
    volatile ULONG	*PRAMDAC0;
    volatile ULONG	*PFB;
    volatile ULONG	*PFIFO;
    volatile ULONG	*PGRAPH;
    volatile ULONG	*PEXTDEV;
    volatile ULONG	*PTIMER;
    volatile ULONG	*PMC;
    volatile ULONG	*PRAMIN;
    volatile ULONG	*FIFO;
    volatile ULONG	*CURSOR;
    volatile UBYTE	*PCIO0;
    volatile UBYTE	*PCIO;
    volatile UBYTE	*PVIO;
    volatile UBYTE	*PDIO0;
    volatile UBYTE	*PDIO;
    volatile ULONG	*PRAMDAC;

    struct CardState	*CurrentState;

    void	(*DMAKickoffCallback)(struct staticdata *sd);
} RIVA_HW_INST, *NVPtr;

struct staticdata {
    struct ExecBase	    *sysbase;
    struct Library	    *oopbase;
    struct Library	    *utilitybase;

    struct MemHeader	    *CardMem;

    struct SignalSemaphore  HWLock;	    /* Hardware exclusive semaphore */

    APTR		    memPool;

    OOP_Class		    *nvclass;
    OOP_Class		    *onbmclass;
    OOP_Class		    *offbmclass;
    OOP_Class		    *planarbmclass;

    OOP_Object		    *pci;
    OOP_Object		    *Device;
    OOP_Object		    *nvobject;
    OOP_Object		    *pcidriver;

    OOP_AttrBase	    pciAttrBase;
    OOP_AttrBase	    bitMapAttrBase;
    OOP_AttrBase	    nvBitMapAttrBase;
    OOP_AttrBase	    pixFmtAttrBase;
    OOP_AttrBase	    gfxAttrBase;
    OOP_AttrBase	    syncAttrBase;
    OOP_AttrBase	    planarAttrBase;

    HIDDT_DPMSLevel	    dpms;

    struct CardState	    *poweron_state;

    struct Card		    Card;

    UWORD		    src_pitch, dst_pitch;
    ULONG		    src_offset, dst_offset;
    ULONG		    surface_format;
    ULONG		    pattern_format;
    ULONG		    rect_format;
    ULONG		    line_format;

    OOP_MethodID	    mid_ReadLong;
    OOP_MethodID	    mid_CopyMemBox8;
    OOP_MethodID	    mid_CopyMemBox16;
    OOP_MethodID	    mid_CopyMemBox32;
    OOP_MethodID	    mid_PutMem32Image8;
    OOP_MethodID	    mid_PutMem32Image16;
    OOP_MethodID	    mid_GetMem32Image8;
    OOP_MethodID	    mid_GetMem32Image16;
    OOP_MethodID	    mid_GetImage;
    OOP_MethodID	    mid_Clear;
    
    BOOL		    gpu_busy;

    IPTR		    scratch_buffer;
};

typedef struct __bm {
    struct SignalSemaphore bmLock;

    OOP_Object	*BitMap;	// BitMap OOP Object
    IPTR	framebuffer;	// Points to pixel data
    ULONG	width;		// Bitmap width
    ULONG	height;		// Bitmap height   
    ULONG	pitch;		// BytesPerRow aligned
    UBYTE	depth;		// Bitmap depth
    UBYTE	bpp;		// BytesPerPixel
    UBYTE	onbm;		// is onbitmap?
    UBYTE	fbgfx;		// is framebuffer in gfx memory
    ULONG	usecount;	// counts BitMap accesses

    ULONG	surface_format;
    ULONG	pattern_format;
    ULONG	rect_format;
    ULONG	line_format;

    struct CardState *state;
} nvBitMap;

struct planarbm_data
{
    UBYTE   **planes;
    ULONG   planebuf_size;
    ULONG   bytesperrow;
    ULONG   rows;
    UBYTE   depth;
    BOOL    planes_alloced;
};

#define LOCK_HW		{ ObtainSemaphore(&sd->HWLock); }
#define UNLOCK_HW	{ ReleaseSemaphore(&sd->HWLock); }

#define LOCK_BITMAP	{ ObtainSemaphore(&bm->bmLock); }
#define UNLOCK_BITMAP	{ ReleaseSemaphore(&bm->bmLock); }

LIBBASETYPE {
    struct Library	LibNode;
    struct ExecBase	*sysbase;
    BPTR		segList;
    APTR		memPool;    
    struct staticdata	*sd;
    struct MemHeader	mh;
};

#define V_DBLSCAN   0x01
#define V_LACE	    0x02

OOP_Class *init_nvclass(struct staticdata*);
OOP_Class *init_onbitmapclass(struct staticdata *);
OOP_Class *init_offbitmapclass(struct staticdata *);
OOP_Class *init_nvplanarbmclass(struct staticdata *sd);

void LoadState(struct staticdata *, struct CardState *);
void SaveState(struct staticdata *, struct CardState *);
void DPMS(struct staticdata *, HIDDT_DPMSLevel);
void InitMode(struct staticdata *sd, struct CardState *,
    ULONG width, ULONG height, UBYTE bpp, ULONG pixelc, ULONG base,
    ULONG HDisplay, ULONG VDisplay, 
    ULONG HSyncStart, ULONG HSyncEnd, ULONG HTotal,
    ULONG VSyncStart, ULONG VSyncEnd, ULONG VTotal);
void acc_test(struct staticdata *);


void NVLockUnlock(struct staticdata *, UBYTE);
int NVShowHideCursor (struct staticdata *, UBYTE);
void NVDmaKickoff(struct Card *);
void NVDmaWait(struct Card *, int);
void NVSync(struct staticdata *);
void NVDMAKickoffCallback(struct staticdata *);
void NVSetPattern(struct staticdata *, ULONG, ULONG, ULONG, ULONG);
void NVSetRopSolid(struct staticdata *, ULONG, ULONG);
void NVSelectHead(struct staticdata *sd, UBYTE head);
BOOL NVIsConnected (struct staticdata *sd, UBYTE output);

void nv4GetConfig(struct staticdata *);
void nv10GetConfig(struct staticdata *);
IPTR AllocBitmapArea(struct staticdata *, ULONG, ULONG, ULONG, BOOL);
VOID FreeBitmapArea(struct staticdata *, IPTR, ULONG, ULONG, ULONG);

#define NVDmaNext(pNv, data) \
     (pNv)->dmaBase[(pNv)->dmaCurrent++] = (data)

#define NVDmaStart(pNv, tag, size) {          \
     if((pNv)->dmaFree <= (size))             \
        NVDmaWait(pNv, size);                 \
     NVDmaNext(pNv, ((size) << 18) | (tag));  \
     (pNv)->dmaFree -= ((size) + 1);          \
}

//#if defined(__i386__)
//#define _NV_FENCE() asm volatile ("outb %0,%w1"::"a"(0),"Nd"(0x3d0));
//#else
#define _NV_FENCE() /* eps */
//#endif

#define WRITE_PUT(pNv, data) {       \
  volatile UBYTE scratch;            \
  _NV_FENCE()                        \
  scratch = (pNv)->FrameBuffer[0];       \
  (pNv)->FIFO[0x0010] = (data) << 2; \
  mem_barrier();                     \
}

#define READ_GET(pNv) ((pNv)->FIFO[0x0011] >> 2)

#endif // _NV_H

