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

#define INCLUDE_ALLOW_USERLEVEL
#define INCLUDE_ALLOW_MONITOR
//#include "includeCheck.h"

#include "svga_limits.h"

/*
 * Memory and port addresses and fundamental constants
 */

#define SVGA_MAX_WIDTH			2364
#define SVGA_MAX_HEIGHT			1773

#ifdef VMX86_SERVER
#define SVGA_DEFAULT_MAX_WIDTH		1600
#define SVGA_DEFAULT_MAX_HEIGHT		1200
#else
#define SVGA_DEFAULT_MAX_WIDTH		SVGA_MAX_WIDTH
#define SVGA_DEFAULT_MAX_HEIGHT		SVGA_MAX_HEIGHT
#endif

#define SVGA_MAX_BITS_PER_PIXEL		32
#if SVGA_MAX_WIDTH * SVGA_MAX_HEIGHT * SVGA_MAX_BITS_PER_PIXEL / 8 > \
    SVGA_FB_MAX_SIZE
#error "Bad SVGA maximum sizes"
#endif
#define SVGA_MAX_PSEUDOCOLOR_DEPTH	8
#define SVGA_MAX_PSEUDOCOLORS		(1 << SVGA_MAX_PSEUDOCOLOR_DEPTH)

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
   SVGA_REG_BITS_PER_PIXEL = 7,
   SVGA_REG_PSEUDOCOLOR = 8,
   SVGA_REG_RED_MASK = 9,
   SVGA_REG_GREEN_MASK = 10,
   SVGA_REG_BLUE_MASK = 11,
   SVGA_REG_BYTES_PER_LINE = 12,
   SVGA_REG_FB_START = 13,
   SVGA_REG_FB_OFFSET = 14,
   SVGA_REG_FB_MAX_SIZE = 15,
   SVGA_REG_FB_SIZE = 16,

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

   SVGA_REG_TOP = 28,		   /* Must be 1 greater than the last register */

   SVGA_PALETTE_BASE = 1024	   /* Base of SVGA color map */
};


/*
 *  Capabilities
 */

#define	SVGA_CAP_RECT_FILL	 0x0001
#define	SVGA_CAP_RECT_COPY	 0x0002
#define	SVGA_CAP_RECT_PAT_FILL   0x0004
#define	SVGA_CAP_OFFSCREEN       0x0008
#define	SVGA_CAP_RASTER_OP	 0x0010
#define	SVGA_CAP_CURSOR		 0x0020
#define	SVGA_CAP_CURSOR_BYPASS	 0x0040


/*
 *  Raster op codes (same encoding as X)
 */

#define	SVGA_ROP_CLEAR		0x00
#define	SVGA_ROP_AND		0x01
#define	SVGA_ROP_AND_REVERSE	0x02
#define	SVGA_ROP_COPY		0x03
#define	SVGA_ROP_AND_INVERTED	0x04
#define	SVGA_ROP_NOOP		0x05
#define	SVGA_ROP_XOR		0x06
#define	SVGA_ROP_OR		0x07
#define	SVGA_ROP_NOR		0x08
#define	SVGA_ROP_EQUIV		0x09
#define	SVGA_ROP_INVERT		0x0a
#define	SVGA_ROP_OR_REVERSE	0x0b
#define	SVGA_ROP_COPY_INVERTED	0x0c
#define	SVGA_ROP_OR_INVERTED	0x0d
#define	SVGA_ROP_NAND		0x0e
#define	SVGA_ROP_SET		0x0f


/*
 *  Memory area offsets (viewed as an array of 32-bit words)
 */

/*
 *  The distance from MIN to MAX must be at least 10K
 */

#define	 SVGA_FIFO_MIN	      0
#define	 SVGA_FIFO_MAX	      1
#define	 SVGA_FIFO_NEXT_CMD   2
#define	 SVGA_FIFO_STOP	      3

#define	 SVGA_FIFO_USER_DEFINED	    4

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

/*
 *  Increment from one scanline to the next of a bitmap or pixmap
 */
#define SVGA_BITMAP_INCREMENT(w) ((( (w)+31 ) >> 5) * sizeof (uint32))
#define SVGA_PIXMAP_INCREMENT(w,d) ((( ((w)*(d))+31 ) >> 5) * sizeof (uint32))

/*
 *  Commands in the command FIFO
 */

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

#define	SVGA_CMD_MAX			  22

#endif
