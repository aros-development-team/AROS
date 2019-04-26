#ifndef _VMWARESVGA_HARDWARE_H
#define _VMWARESVGA_HARDWARE_H

#include <exec/libraries.h>
#include <exec/tasks.h>
#include <hidd/pci.h>
#include <hidd/gfx.h>
#include <oop/oop.h>

#include "vmwaresvga_bitmap.h"
#include "vmwaresvga_mouse.h"

#include "svga_reg.h"
#define SVGA_LEGACY_BASE_PORT	0x4560

#define VENDOR_VMWARE 0x15ad
#define DEVICE_VMWARE0710   0x0710
#define DEVICE_VMWARE0405   0x0405

/*
 *  Raster op codes (same encoding as X)
 */

#define SVGA_ROP_CLEAR          0x00     /* 0 */
#define SVGA_ROP_AND            0x01     /* src AND dst */
#define SVGA_ROP_AND_REVERSE    0x02     /* src AND NOT dst */
#define SVGA_ROP_COPY           0x03     /* src */
#define SVGA_ROP_AND_INVERTED   0x04     /* NOT src AND dst */
#define SVGA_ROP_NOOP           0x05     /* dst */
#define SVGA_ROP_XOR            0x06     /* src XOR dst */
#define SVGA_ROP_OR             0x07     /* src OR dst */
#define SVGA_ROP_NOR            0x08     /* NOT src AND NOT dst */
#define SVGA_ROP_EQUIV          0x09     /* NOT src XOR dst */
#define SVGA_ROP_INVERT         0x0a     /* NOT dst */
#define SVGA_ROP_OR_REVERSE     0x0b     /* src OR NOT dst */
#define SVGA_ROP_COPY_INVERTED  0x0c     /* NOT src */
#define SVGA_ROP_OR_INVERTED    0x0d     /* NOT src OR dst */
#define SVGA_ROP_NAND           0x0e     /* NOT src OR NOT dst */
#define SVGA_ROP_SET            0x0f     /* 1 */
#define SVGA_ROP_UNSUPPORTED    0x10

#define SVGA_NUM_SUPPORTED_ROPS   16

/*
 *  Commands in the command FIFO
 */

#define	 SVGA_CMD_INVALID_CMD		   0
	 /* FIFO layout:
            <nothing> (well, undefined) */

#define	 SVGA_CMD_UPDATE		   1
	 /* FIFO layout:
	    X, Y, Width, Height */

#define	 SVGA_CMD_RECT_FILL		   2
	 /* FIFO layout:
	    Color, X, Y, Width, Height */

#define	 SVGA_CMD_RECT_COPY		   3
	 /* FIFO layout:
	    Source X, Source Y, Dest X, Dest Y, Width, Height */

#define	 SVGA_CMD_DEFINE_BITMAP		   4
	 /* FIFO layout:
	    Pixmap ID, Width, Height, <scanlines> */

#define	 SVGA_CMD_DEFINE_BITMAP_SCANLINE   5
	 /* FIFO layout:
	    Pixmap ID, Width, Height, Line #, scanline */

#define	 SVGA_CMD_DEFINE_PIXMAP		   6
	 /* FIFO layout:
	    Pixmap ID, Width, Height, Depth, <scanlines> */

#define	 SVGA_CMD_DEFINE_PIXMAP_SCANLINE   7
	 /* FIFO layout:
	    Pixmap ID, Width, Height, Depth, Line #, scanline */

#define	 SVGA_CMD_RECT_BITMAP_FILL	   8
	 /* FIFO layout:
	    Bitmap ID, X, Y, Width, Height, Foreground, Background */

#define	 SVGA_CMD_RECT_PIXMAP_FILL	   9
	 /* FIFO layout:
	    Pixmap ID, X, Y, Width, Height */

#define	 SVGA_CMD_RECT_BITMAP_COPY	  10
	 /* FIFO layout:
	    Bitmap ID, Source X, Source Y, Dest X, Dest Y,
	    Width, Height, Foreground, Background */

#define	 SVGA_CMD_RECT_PIXMAP_COPY	  11
	 /* FIFO layout:
	    Pixmap ID, Source X, Source Y, Dest X, Dest Y, Width, Height */

#define	 SVGA_CMD_FREE_OBJECT		  12
	 /* FIFO layout:
	    Object (pixmap, bitmap, ...) ID */

#define	 SVGA_CMD_RECT_ROP_FILL           13
         /* FIFO layout:
            Color, X, Y, Width, Height, ROP */

#define	 SVGA_CMD_RECT_ROP_COPY           14
         /* FIFO layout:
            Source X, Source Y, Dest X, Dest Y, Width, Height, ROP */

#define	 SVGA_CMD_RECT_ROP_BITMAP_FILL    15
         /* FIFO layout:
            ID, X, Y, Width, Height, Foreground, Background, ROP */

#define	 SVGA_CMD_RECT_ROP_PIXMAP_FILL    16
         /* FIFO layout:
            ID, X, Y, Width, Height, ROP */

#define	 SVGA_CMD_RECT_ROP_BITMAP_COPY    17
         /* FIFO layout:
            ID, Source X, Source Y,
            Dest X, Dest Y, Width, Height, Foreground, Background, ROP */

#define	 SVGA_CMD_RECT_ROP_PIXMAP_COPY    18
         /* FIFO layout:
            ID, Source X, Source Y, Dest X, Dest Y, Width, Height, ROP */

#define	SVGA_CMD_DEFINE_CURSOR		  19
	/* FIFO layout:
	   ID, Hotspot X, Hotspot Y, Width, Height,
	   Depth for AND mask, Depth for XOR mask,
	   <scanlines for AND mask>, <scanlines for XOR mask> */

#define	SVGA_CMD_DISPLAY_CURSOR		  20
	/* FIFO layout:
	   ID, On/Off (1 or 0) */

#define	SVGA_CMD_MOVE_CURSOR		  21
	/* FIFO layout:
	   X, Y */

#define SVGA_CMD_DEFINE_ALPHA_CURSOR      22
	/* FIFO layout:
	   ID, Hotspot X, Hotspot Y, Width, Height,
	   <scanlines> */

#define SVGA_CMD_DRAW_GLYPH               23
	/* FIFO layout:
	   X, Y, W, H, FGCOLOR, <stencil buffer> */

#define SVGA_CMD_DRAW_GLYPH_CLIPPED       24
	/* FIFO layout:
	   X, Y, W, H, FGCOLOR, BGCOLOR, <cliprect>, <stencil buffer>
           Transparent color expands are done by setting BGCOLOR to ~0 */

#define	SVGA_CMD_UPDATE_VERBOSE	          25
        /* FIFO layout:
	   X, Y, Width, Height, Reason */

#define SVGA_CMD_SURFACE_FILL             26
        /* FIFO layout:
	   color, dstSurfaceOffset, x, y, w, h, rop */

#define SVGA_CMD_SURFACE_COPY             27
        /* FIFO layout:
	   srcSurfaceOffset, dstSurfaceOffset, srcX, srcY,
           destX, destY, w, h, rop */

#define SVGA_CMD_SURFACE_ALPHA_BLEND      28
        /* FIFO layout:
	   srcSurfaceOffset, dstSurfaceOffset, srcX, srcY,
           destX, destY, w, h, op (SVGA_BLENDOP*), flags (SVGA_BLENDFLAGS*), 
           param1, param2 */

#define	 SVGA_CMD_FRONT_ROP_FILL          29
         /* FIFO layout:
            Color, X, Y, Width, Height, ROP */

#define	 SVGA_CMD_FENCE                   30
         /* FIFO layout:
            Fence value */

/*
 *  Capabilities
 */

#define	SVGA_CAP_RECT_FILL	    0x00001
#define	SVGA_CAP_RECT_PAT_FILL      0x00004
#define	SVGA_CAP_LEGACY_OFFSCREEN   0x00008
#define	SVGA_CAP_RASTER_OP	    0x00010
#define SVGA_CAP_GLYPH              0x00400
#define SVGA_CAP_GLYPH_CLIPPING     0x00800
#define SVGA_CAP_OFFSCREEN_1        0x01000
#define SVGA_CAP_ALPHA_BLEND        0x02000

struct HWData  {
    APTR			iobase;
    APTR			vrambase;
    APTR			mmiobase;
    ULONG			vramsize;
    ULONG			mmiosize;

    UWORD			indexReg;
    UWORD			valueReg;

    ULONG			capabilities;
    ULONG			fifocapabilities;

    struct HIDD_ViewPortData    *shown;

    ULONG			depth;
    ULONG			redmask;
    ULONG			greenmask;
    ULONG			bluemask;
    ULONG			redshift;
    ULONG			greenshift;
    ULONG			blueshift;
    ULONG			bytesperpixel;
    ULONG			bitsperpixel;
    ULONG			bytesperline;

    ULONG			maxwidth;
    ULONG			maxheight;
    ULONG			displaycount;

    ULONG			fboffset;		/* last byte in framebuffer of current screen mode */
    ULONG			pseudocolor;

    UWORD			display_width;
    UWORD			display_height;
    ULONG			bytes_per_line;

    APTR			maskPool;
    APTR  			irq;
    ULONG			hwint;
    ULONG			fifomin;
    ULONG			fence;

    struct Box			delta_damage;
    struct Task 		*render_task;
    struct SignalSemaphore	damage_control;
};

#define clearCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_CLEAR)
#define andCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_AND)
#define andReverseCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_AND_REVERSE)
#define copyCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_COPY)
#define andInvertedCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_AND_INVERTED)
#define noOpCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_NOOP)
#define xorCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_XOR)
#define orCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_OR)
#define norCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_NOR)
#define equivCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_EQUIV)
#define invertCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_INVERT)
#define orReverseCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_OR_REVERSE)
#define copyInvertedCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_COPY_INVERTED)
#define orInvertedCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_OR_INVERTED)
#define nandCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_NAND)
#define setCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_SET)

#define clearFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_CLEAR)
#define andFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_AND)
#define andReverseFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_AND_REVERSE)
#define copyFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_COPY)
#define andInvertedFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_AND_INVERTED)
#define noOpFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_NOOP)
#define xorFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_XOR)
#define orFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_OR)
#define norFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_NOR)
#define equivFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_EQUIV)
#define invertFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_INVERT)
#define orReverseFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_OR_REVERSE)
#define copyInvertedFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_COPY_INVERTED)
#define orInvertedFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_OR_INVERTED)
#define nandFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_NAND)
#define setFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_SET)

ULONG vmwareReadReg(struct HWData *, ULONG);
void vmwareWriteReg(struct HWData *, ULONG, ULONG);

VOID enableVMWareSVGA(struct HWData *);
VOID disableVMWareSVGA(struct HWData *);

BOOL initVMWareSVGAHW(struct HWData *, OOP_Object *);
VOID initDisplayVMWareSVGA(struct HWData *);
VOID getModeCfgVMWareSVGA(struct HWData *);
VOID setModeVMWareSVGA(struct HWData *, ULONG, ULONG);
VOID refreshAreaVMWareSVGA(struct HWData *, struct Box *);

VOID rectFillVMWareSVGA(struct HWData *, ULONG, LONG, LONG, LONG, LONG);
VOID ropFillVMWareSVGA(struct HWData *, ULONG, LONG, LONG, LONG, LONG, ULONG);
VOID ropCopyVMWareSVGA(struct HWData *, LONG, LONG, LONG, LONG, ULONG, ULONG, ULONG);

VOID defineCursorVMWareSVGA(struct HWData *, struct MouseData *);
VOID displayCursorVMWareSVGA(struct HWData *, LONG);
VOID moveCursorVMWareSVGA(struct HWData *, LONG, LONG);

VOID writeVMWareSVGAFIFO(struct HWData *, ULONG);
VOID syncVMWareSVGAFIFO(struct HWData *);

ULONG fenceVMWareSVGAFIFO(struct HWData *);
VOID syncfenceVMWareSVGAFIFO(struct HWData *, ULONG);

VOID vmwareHandlerIRQ(struct HWData *, void *);

VOID VMWareSVGA_Damage_Reset(struct HWData *);
VOID VMWareSVGA_Damage_DeltaAdd(struct HWData *, struct Box *);
VOID VMWareSVGA_RestartRenderTask(struct HWData *);

#endif /* _VMWARESVGA_HARDWARE_H */
