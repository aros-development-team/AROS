/* **********************************************************
 * Copyright (C) 1998-2001 VMware, Inc.
 * All Rights Reserved
 * Id: svga_reg.h,v 1.9 2001/03/02 02:06:47 bhlim Exp $
 * **********************************************************/

/*
 * svga_reg.h --
 *
 * SVGA hardware definitions
 */

#ifndef _SVGA_REG_H_
#define _SVGA_REG_H_

#include "svga_limits.h"

/*
 * Memory and port addresses and fundamental constants
 */

#define SVGA_MAX_WIDTH			3840
#define SVGA_MAX_HEIGHT			2160

#define SVGA_MAX_BITS_PER_PIXEL		32
#if SVGA_MAX_WIDTH * SVGA_MAX_HEIGHT * SVGA_MAX_BITS_PER_PIXEL / 8 > \
    SVGA_FB_MAX_SIZE
#error "Bad SVGA maximum sizes"
#endif
#define SVGA_MAX_PSEUDOCOLOR_DEPTH	8
#define SVGA_MAX_PSEUDOCOLORS		(1 << SVGA_MAX_PSEUDOCOLOR_DEPTH)
#define SVGA_NUM_PALETTE_REGS       (3 * SVGA_MAX_PSEUDOCOLORS)

#define SVGA_MAGIC         0x900000
#define SVGA_MAKE_ID(ver)  (SVGA_MAGIC << 8 | (ver))

/* Version 2 let the address of the frame buffer be unsigned on Win32 */
#define SVGA_VERSION_2     2
#define SVGA_ID_2          SVGA_MAKE_ID(SVGA_VERSION_2)

/* Version 1 has new registers starting with SVGA_REG_CAPABILITIES so
   PALETTE_BASE has moved */
#define SVGA_VERSION_1     1
#define SVGA_ID_1          SVGA_MAKE_ID(SVGA_VERSION_1)

/* Version 0 is the initial version */
#define SVGA_VERSION_0     0
#define SVGA_ID_0          SVGA_MAKE_ID(SVGA_VERSION_0)

/* Invalid SVGA_ID_ */
#define SVGA_ID_INVALID    0xFFFFFFFF

/* More backwards compatibility, old location of color map: */
#define SVGA_OLD_PALETTE_BASE	 17

/* Base and Offset gets us headed the right way for PCI Base Addr Registers */
#define SVGA_LEGACY_BASE_PORT	0x4560
#define SVGA_INDEX_PORT		0x0
#define SVGA_VALUE_PORT		0x1
#define SVGA_BIOS_PORT		0x2
#define SVGA_NUM_PORTS		0x3

/* This port is deprecated, but retained because of old drivers. */
#define SVGA_LEGACY_ACCEL_PORT	0x3

/* Legal values for the SVGA_REG_CURSOR_ON register in cursor bypass mode */
#define SVGA_CURSOR_ON_HIDE               0x0    /* Must be 0 to maintain backward compatibility */
#define SVGA_CURSOR_ON_SHOW               0x1    /* Must be 1 to maintain backward compatibility */
#define SVGA_CURSOR_ON_REMOVE_FROM_FB     0x2    /* Remove the cursor from the framebuffer because we need to see what's under it */
#define SVGA_CURSOR_ON_RESTORE_TO_FB      0x3    /* Put the cursor back in the framebuffer so the user can see it */

/*
 * Registers
 */

enum {
   SVGA_REG_ID = 0,
   SVGA_REG_ENABLE = 1,
   SVGA_REG_WIDTH = 2,
   SVGA_REG_HEIGHT = 3,
   SVGA_REG_MAX_WIDTH = 4,
   SVGA_REG_MAX_HEIGHT = 5,
   SVGA_REG_DEPTH = 6,
   SVGA_REG_BITS_PER_PIXEL = 7,     /* Current bpp in the guest */
   SVGA_REG_PSEUDOCOLOR = 8,
   SVGA_REG_RED_MASK = 9,
   SVGA_REG_GREEN_MASK = 10,
   SVGA_REG_BLUE_MASK = 11,
   SVGA_REG_BYTES_PER_LINE = 12,
   SVGA_REG_FB_START = 13,
   SVGA_REG_FB_OFFSET = 14,
   SVGA_REG_VRAM_SIZE = 15,
   SVGA_REG_FB_SIZE = 16,

   /* ID 0 implementation only had the above registers, then the palette */

   SVGA_REG_CAPABILITIES = 17,
   SVGA_REG_MEM_START = 18,	   /* Memory for command FIFO and bitmaps */
   SVGA_REG_MEM_SIZE = 19,
   SVGA_REG_CONFIG_DONE = 20,      /* Set when memory area configured */
   SVGA_REG_SYNC = 21,             /* Write to force synchronization */
   SVGA_REG_BUSY = 22,             /* Read to check if sync is done */
   SVGA_REG_GUEST_ID = 23,	   /* Set guest OS identifier */
   SVGA_REG_CURSOR_ID = 24,	   /* ID of cursor */
   SVGA_REG_CURSOR_X = 25,	   /* Set cursor X position */
   SVGA_REG_CURSOR_Y = 26,	   /* Set cursor Y position */
   SVGA_REG_CURSOR_ON = 27,	   /* Turn cursor on/off */
   SVGA_REG_HOST_BITS_PER_PIXEL = 28, /* Current bpp in the host */
   SVGA_REG_SCRATCH_SIZE = 29,     /* Number of scratch registers */
   SVGA_REG_MEM_REGS = 30,         /* Number of FIFO registers */
   SVGA_REG_NUM_DISPLAYS = 31,     /* Number of guest displays */
   SVGA_REG_PITCHLOCK = 32,        /* Fixed pitch for all modes */
   SVGA_REG_TOP = 33,		   /* Must be 1 more than the last register */

   SVGA_PALETTE_BASE = 1024,	   /* Base of SVGA color map */
   /* Next 768 (== 256*3) registers exist for colormap */
   SVGA_SCRATCH_BASE = SVGA_PALETTE_BASE + SVGA_NUM_PALETTE_REGS
                                   /* Base of scratch registers */
   /* Next reg[SVGA_REG_SCRATCH_SIZE] registers exist for scratch usage:
      First 4 are reserved for VESA BIOS Extension; any remaining are for
      the use of the current SVGA driver. */
};


/*
 *  Capabilities
 */

#define	SVGA_CAP_NONE               0x00000
#define	SVGA_CAP_RECT_FILL	    0x00001
#define	SVGA_CAP_RECT_COPY	    0x00002
#define	SVGA_CAP_RECT_PAT_FILL      0x00004
#define	SVGA_CAP_LEGACY_OFFSCREEN   0x00008
#define	SVGA_CAP_RASTER_OP	    0x00010
#define	SVGA_CAP_CURSOR		    0x00020
#define	SVGA_CAP_CURSOR_BYPASS	    0x00040
#define	SVGA_CAP_CURSOR_BYPASS_2    0x00080
#define	SVGA_CAP_8BIT_EMULATION     0x00100
#define SVGA_CAP_ALPHA_CURSOR       0x00200
#define SVGA_CAP_GLYPH              0x00400
#define SVGA_CAP_GLYPH_CLIPPING     0x00800
#define SVGA_CAP_OFFSCREEN_1        0x01000
#define SVGA_CAP_ALPHA_BLEND        0x02000
#define SVGA_CAP_3D                 0x04000
#define SVGA_CAP_EXTENDED_FIFO      0x08000
#define SVGA_CAP_MULTIMON           0x10000
#define SVGA_CAP_PITCHLOCK          0x20000


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
 *  Memory area offsets (viewed as an array of 32-bit words)
 */

/*
 *  The distance from MIN to MAX must be at least 10K
 */

enum VMWareSVGA_FIFO_Register
{
	// Block 1 - Basic registers
	SVGA_FIFO_MIN					= 0,
	SVGA_FIFO_MAX,
	SVGA_FIFO_NEXT_CMD,
	SVGA_FIFO_STOP,

	// Block 2 - Extended FIFO Registers (SVGA_CAP_EXTENDED_FIFO)
	SVGA_FIFO_CAPABILITIES			= 4,
	SVGA_FIFO_FLAGS,
	SVGA_FIFO_FENCE,						// 													Requires SVGA_FIFO_CAP_FENCE

	// Block 3a - Optional Extended FIFO Registers Banks (Only exist if enough distance is reserved)
	SVGA_FIFO_3D_HWVERSION,
	SVGA_FIFO_PITCHLOCK,					// 													Requires SVGA_FIFO_CAP_PITCHLOCK

	SVGA_FIFO_CURSOR_ON,					// 													Requires SVGA_FIFO_CAP_CURSOR_BYPASS_3
	SVGA_FIFO_CURSOR_X,						// 													Requires SVGA_FIFO_CAP_CURSOR_BYPASS_3
	SVGA_FIFO_CURSOR_Y,						// 													Requires SVGA_FIFO_CAP_CURSOR_BYPASS_3
	SVGA_FIFO_CURSOR_COUNT,					// Incremented when any of the above change		- 	Requires SVGA_FIFO_CAP_CURSOR_BYPASS_3
	SVGA_FIFO_CURSOR_LAST_UPDATED,			// Timestamp of last update						-	Requires SVGA_FIFO_CAP_CURSOR_BYPASS_3

	SVGA_FIFO_RESERVED,						// Bytes past NEXT_CMD with real contents		-	Requires SVGA_FIFO_CAP_RESERVE

	SVGA_FIFO_CURSOR_SCREEN_ID,				// Either SVGA_ID_INVALID or Offset Screen id	-	Requires SVGA_FIFO_CAP_SCREEN_OBJECT or SVGA_FIFO_CAP_SCREEN_OBJECT_2
	SVGA_FIFO_DEAD,							// Arbitrary value written by the host			-	Requires SVGA_FIFO_CAP_DEAD
	SVGA_FIFO_3D_HWVERSION_REVISED,			// 3D Hardware version							-	Requires SVGA_FIFO_CAP_3D_HWVERSION_REVISED

	// 3D Caps Block
	SVGA_FIFO_3D_CAPS				= 32,
	SVGA_FIFO_3D_CAPS_LAST			= (32 + 255),

	// Block 3b - Optional Extended FIFO Register Entries
	SVGA_FIFO_GUEST_3D_HWVERSION,
	SVGA_FIFO_FENCE_GOAL,					// Target for SVGA_IRQFLAG_FENCE_GOAL
	SVGA_FIFO_BUSY,
	SVGA_FIFO_NUM_REGS						// To be read from SVGA_REG_MEM_REGS
};

#define SVGA_FIFO_CAP_NONE                  0
#define SVGA_FIFO_CAP_FENCE             (1<<0)					// Fence register and command are supported
#define SVGA_FIFO_CAP_ACCELFRONT        (1<<1)					// Front buffer only commands are supported
#define SVGA_FIFO_CAP_PITCHLOCK         (1<<2)					// Pitch lock register is supported
#define SVGA_FIFO_CAP_VIDEO             (1<<3)					// SVGA Video overlay units are supported
#define SVGA_FIFO_CAP_CURSOR_BYPASS_3   (1<<4)
#define SVGA_FIFO_CAP_ESCAPE            (1<<5)					// Escape command is supported
#define SVGA_FIFO_CAP_RESERVE           (1<<6)
#define SVGA_FIFO_CAP_SCREEN_OBJECT     (1<<7)
#define SVGA_FIFO_CAP_GMR2              (1<<8)
#define SVGA_FIFO_CAP_3D_HWVERSION_REVISED  SVGA_FIFO_CAP_GMR2
#define SVGA_FIFO_CAP_SCREEN_OBJECT_2   (1<<9)
#define SVGA_FIFO_CAP_DEAD              (1<<10)

/*
 *  Drawing object ID's, in the range 0 to SVGA_MAX_ID
 */

#define	 SVGA_MAX_ID	      499

/*
 *  Macros to compute variable length items (sizes in 32-bit words)
 */

#define SVGA_BITMAP_SIZE(w,h) ((((w)+31) >> 5) * (h))
#define SVGA_BITMAP_SCANLINE_SIZE(w) (( (w)+31 ) >> 5)
#define SVGA_PIXMAP_SIZE(w,h,d) ((( ((w)*(d))+31 ) >> 5) * (h))
#define SVGA_PIXMAP_SCANLINE_SIZE(w,d) (( ((w)*(d))+31 ) >> 5)
#define SVGA_GLYPH_SIZE(w,h) ((((((w) + 7) >> 3) * (h)) + 3) >> 2)
#define SVGA_GLYPH_SCANLINE_SIZE(w) (((w) + 7) >> 3)

/*
 *  Increment from one scanline to the next of a bitmap or pixmap
 */
#define SVGA_BITMAP_INCREMENT(w) ((( (w)+31 ) >> 5) * sizeof (uint32))
#define SVGA_PIXMAP_INCREMENT(w,d) ((( ((w)*(d))+31 ) >> 5) * sizeof (uint32))

#define SVGA_COLOR_TRANSPARENT (~0)

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

#define	SVGA_CMD_MAX			  31

#define SVGA_CMD_MAX_ARGS                 64

#endif
