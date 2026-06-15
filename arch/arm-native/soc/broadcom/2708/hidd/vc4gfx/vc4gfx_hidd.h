#ifndef _VIDEOCOREGFX_CLASS_H
#define _VIDEOCOREGFX_CLASS_H
/*
    Copyright � 2013-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <exec/nodes.h>
#include <exec/types.h>
#include <proto/exec.h>

#include <hardware/bcm2708_dma.h>

#include "vc4gfx_hardware.h"

/* vcsd_MBoxMessage is one shared buffer for every mailbox round-trip;
 * take vcsd_GPUMemLock around each pack-write-read sequence. */
#define VC4_MBOX_LOCK(xsd)   ObtainSemaphore(&(xsd)->vcsd_GPUMemLock)
#define VC4_MBOX_UNLOCK(xsd) ReleaseSemaphore(&(xsd)->vcsd_GPUMemLock)

#define DEBUGMODEARRAY
//#define DEBUGPIXFMT
//#define DEBUGDISPLAY

#define VC_FMT_32
//#define VC_FMT_24
//#define VC_FMT_16
//#define VC_FMT_15
//#define VC_FMT_8

#define IID_Hidd_Gfx_VideoCore4  "hidd.gfx.bcmvc4"
#define CLID_Hidd_Gfx_VideoCore4 "hidd.gfx.bcmvc4"

#define MAX_TAGS        256
#define ATTRBASES_NUM   8

struct VideoCoreGfx_staticdata {
        APTR                    vcsd_MBoxBase;
        unsigned int            *vcsd_MBoxMessage;
        IPTR                    vcsd_MBoxBuff;

        struct SignalSemaphore  vcsd_GPUMemLock;
        struct MemHeaderExt     vcsd_GPUMemManage;

        OOP_Class 	    	*vcsd_basebm;            /* baseclass for CreateObject */
        OOP_Class               *vcsd_basegallium;       /* baseclass for CreateObject */
        struct Library          *vcsd_VC4GalliumLib;     /* keeps vc4gallium.hidd loaded */

        OOP_Class               *vcsd_VideoCoreGfxClass;
	OOP_Object              *vcsd_VideoCoreGfxInstance;
	OOP_Class               *vcsd_VideoCoreGfxOnBMClass;
	OOP_Class               *vcsd_VideoCoreGfxOffBMClass;

        OOP_AttrBase	        vcsd_attrBases[ATTRBASES_NUM];

	APTR                    data;

        /* HW cursor state. cur_buf == NULL means cursor unavailable. */
        ULONG                   vcsd_CurBufHandle;
        APTR                    vcsd_CurBuf;        /* CPU-physical, ARGB pixels */
        ULONG                   vcsd_CurBufBus;     /* GPU bus address */
        ULONG                   vcsd_CurWidth;
        ULONG                   vcsd_CurHeight;
        ULONG                   vcsd_CurHotX;
        ULONG                   vcsd_CurHotY;
        LONG                    vcsd_CurX;
        LONG                    vcsd_CurY;
        BOOL                    vcsd_CurVisible;

        /* Firmware-reported native HDMI resolution (set by HDMI_SyncGen),
         * used as the NominalDimensions default so a fresh boot lands on
         * the panel's native mode rather than 800x600.
         */
        ULONG                   vcsd_NativeWidth;
        ULONG                   vcsd_NativeHeight;

        /* DMA-accelerated 2D state. Channel from dma.resource
         * (DMACHF_TDMODE); -1 means no DMA (fall back to NEON). The shared
         * CB and bounce buffer are serialized by vcsd_DMALock.
         */
        LONG                    vcsd_DMAChannel;
        APTR                    vcsd_DMACBRaw;
        struct BCM2708DMACB     *vcsd_DMACB;
        ULONG                   *vcsd_DMAFillPx;    /* Fill source word */
        APTR                    vcsd_DMABounceRaw;
        UBYTE                   *vcsd_DMABounce;    /* 32-byte aligned */
        ULONG                   vcsd_DMABouncePhys;
        struct SignalSemaphore  vcsd_DMALock;

        /* SETVOFFSET page flipping. The framebuffer is allocated at twice
         * the virtual height and the two pages flipped with SETVOFFSET;
         * full-frame producers (GL, video) render to the back and flip,
         * incremental UI targets the front. vcsd_FBPages == 1 = no flipping
         * (alloc/validation failed).
         */
        OOP_Object              *vcsd_FBObj;        /* The framebuffer bitmap */
        ULONG                   vcsd_FBPage[2];     /* Page base phys addresses */
        ULONG                   vcsd_FBPageHeight;  /* Rows per page */
        UBYTE                   vcsd_FBPages;       /* 1 or 2 */
        UBYTE                   vcsd_FBFront;       /* Currently scanned-out page */
};

/* DMA has a per-call setup + poll cost; below these sizes NEON wins.
 * Uncached reads are ~10x slower than writes, hence the lower thresholds
 * for read-heavy operations.
 */
#define VC4_DMA_COPY_THRESHOLD  2048    /* bytes (e.g. 32x16 px) */
#define VC4_DMA_FILL_THRESHOLD  16384   /* bytes (e.g. 64x64 px) */
#define VC4_DMA_PUT_THRESHOLD   16384   /* cached source, writes only */
#define VC4_DMA_GET_THRESHOLD   4096    /* uncached source reads */

/* Bounce buffer for DMA reads into cached memory. */
#define VC4_DMA_BOUNCE_SIZE     65536

/* Offscreen 32bpp bitmaps at least this large go in GPU memory so blits
 * can use the DMA engine without cache maintenance; smaller ones stay in
 * (cached) system RAM.
 */
#define VC4_GPUBM_THRESHOLD     65536

/* Maximum HW cursor size supported by VideoCore firmware. */
#define VC4_CURSOR_MAX_W 64
#define VC4_CURSOR_MAX_H 64
#define VC4_CURSOR_BUF_BYTES (VC4_CURSOR_MAX_W * VC4_CURSOR_MAX_H * 4)

struct VideoCoreGfxBase
{
    struct Library library;

    struct VideoCoreGfx_staticdata vsd;
};

struct DisplayMode
{
    struct Node dm_Node;
    ULONG       dm_clock;
    ULONG       dm_hdisp;
    ULONG       dm_hstart;
    ULONG       dm_hend;
    ULONG       dm_htotal;
    ULONG       dm_vdisp;
    ULONG       dm_vstart;
    ULONG       dm_vend;
    ULONG       dm_vtotal;
    STRPTR      dm_descr;
};

#define XSD(cl) (&((struct VideoCoreGfxBase *)cl->UserData)->vsd)

#undef HiddVideoCoreGfxAttrBase
#undef HiddVideoCoreGfxBitMapAttrBase
#undef HiddChunkyBMAttrBase
#undef HiddBitMapAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddGfxAttrBase
#undef HiddAttrBase

/* These must stay in the same order as interfaces[] array in vc4gfx_init.c */
#define HiddVideoCoreGfxAttrBase         XSD(cl)->vcsd_attrBases[0]
#define HiddVideoCoreGfxBitMapAttrBase   XSD(cl)->vcsd_attrBases[1]
#define HiddChunkyBMAttrBase             XSD(cl)->vcsd_attrBases[2]
#define HiddBitMapAttrBase               XSD(cl)->vcsd_attrBases[3]
#define HiddPixFmtAttrBase               XSD(cl)->vcsd_attrBases[4]
#define HiddSyncAttrBase                 XSD(cl)->vcsd_attrBases[5]
#define HiddGfxAttrBase                  XSD(cl)->vcsd_attrBases[6]
#define HiddAttrBase                     XSD(cl)->vcsd_attrBases[7]

#define FNAME_SUPPORT(x) VideoCoreGfx__Support__ ## x

int     FNAME_SUPPORT(InitMem)(void *, int, struct VideoCoreGfxBase *);
int     FNAME_SUPPORT(InitCursor)(struct VideoCoreGfx_staticdata *);
int     FNAME_SUPPORT(InitDMA)(struct VideoCoreGfx_staticdata *);
int     FNAME_SUPPORT(SDTV_SyncGen)(struct List *, OOP_Class *);
int     FNAME_SUPPORT(HDMI_SyncGen)(struct List *, OOP_Class *);
APTR    FNAME_SUPPORT(GenPixFmts)(OOP_Class *);

BOOL    vc4_dma_copy(struct VideoCoreGfx_staticdata *xsd,
                     ULONG src_phys, ULONG src_pitch,
                     ULONG dst_phys, ULONG dst_pitch,
                     ULONG width_bytes, ULONG height, BOOL bottom_up);
BOOL    vc4_dma_fill(struct VideoCoreGfx_staticdata *xsd,
                     ULONG dst_phys, ULONG dst_pitch,
                     ULONG width_bytes, ULONG height, ULONG pixel);
BOOL    vc4_dma_put(struct VideoCoreGfx_staticdata *xsd,
                    const UBYTE *src, ULONG src_modulo,
                    ULONG dst_phys, ULONG dst_pitch,
                    ULONG width_bytes, ULONG height);
BOOL    vc4_dma_get(struct VideoCoreGfx_staticdata *xsd,
                    ULONG src_phys, ULONG src_pitch,
                    UBYTE *dst, ULONG dst_modulo,
                    ULONG width_bytes, ULONG height);

ULONG   vc4_fb_backpage(struct VideoCoreGfx_staticdata *xsd);
BOOL    vc4_fb_flip(struct VideoCoreGfx_staticdata *xsd);

#endif /* _VIDEOCOREGFX_CLASS_H */
