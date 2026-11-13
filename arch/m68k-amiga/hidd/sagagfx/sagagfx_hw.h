#ifndef SAGAGFX_HW_H
#define SAGAGFX_HW_H

/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SAGAGfx hardware header.
    Lang: English.
*/

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>
#include <exec/types.h>

BOOL SAGA_Init();
void SAGA_SetPLL(ULONG clock);
void SAGA_LoadCLUT(ULONG *palette, UWORD startIndex, UWORD count);

static inline __attribute__((always_inline)) UWORD READ16(IPTR a) { return (*(volatile UWORD*)a); }
static inline __attribute__((always_inline)) ULONG READ32(IPTR a) { return (*(volatile ULONG*)a); }
static inline __attribute__((always_inline)) void WRITE16(IPTR a, UWORD b) { D(bug("WRITE16(%p,%04x)\n", a, b)); *(volatile UWORD *)(a) = (b); }
static inline __attribute__((always_inline)) void WRITE32(IPTR a, ULONG b) { D(bug("WRITE32(%p,%08x)\n", a, b)); *(volatile ULONG *)(a) = (b); }

/* SAGA Registers **********************************************************/

#define VREG_BOARD                  0xDFF3FC

/* SAGA VIDEO definitions***************************************************/

#define SAGA_PIXELCLOCK             (28375000)
#define SAGA_MEMORYSIZE             (4*1024*1024)

#define SAGA_MOUSE_DELTAX           16
#define SAGA_MOUSE_DELTAY           8

#define SAGA_VIDEO_MAXHV            16384 // AROS=16384, AOS=4096
#define SAGA_VIDEO_MAXHR            16384 // AROS=16384, AOS=4096
#define SAGA_VIDEO_MAXVV            16384 // AROS=16384, AOS=2048
#define SAGA_VIDEO_MAXVR            16384 // AROS=16384, AOS=2048

#define SAGA_VIDEO_SPRITEX          0xDFF1d0
#define SAGA_VIDEO_SPRITEY          0xDFF1d2
#define SAGA_VIDEO_SPRITECLUT       0xDFF3A2
#define SAGA_VIDEO_SPRITEBPL        0xDFF800

#define SAGA_VIDEO_BPLHMOD          0xDFF1E6
#define SAGA_VIDEO_BPLPTR           0xDFF1EC
#define SAGA_VIDEO_MODE             0xDFF1F4
#define SAGA_VIDEO_HPIXEL           0xDFF300
#define SAGA_VIDEO_HSSTRT           0xDFF302
#define SAGA_VIDEO_HSSTOP           0xDFF304
#define SAGA_VIDEO_HTOTAL           0xDFF306
#define SAGA_VIDEO_VPIXEL           0xDFF308
#define SAGA_VIDEO_VSSTRT           0xDFF30A
#define SAGA_VIDEO_VSSTOP           0xDFF30C
#define SAGA_VIDEO_VTOTAL           0xDFF30E
#define SAGA_VIDEO_HVSYNC           0xDFF310
#define SAGA_VIDEO_PLLV4            0xDFF3F8
#define SAGA_VIDEO_CLUT(x)          (0xDFF400 + (((x) & 0xFF) << 2))

#define SAGA_VIDEO_FORMAT_AMIGA     0x0000
#define SAGA_VIDEO_FORMAT_CLUT8     0x0001
#define SAGA_VIDEO_FORMAT_RGB16     0x0002
#define SAGA_VIDEO_FORMAT_RGB15     0x0003
#define SAGA_VIDEO_FORMAT_RGB24     0x0004
#define SAGA_VIDEO_FORMAT_RGB32     0x0005
#define SAGA_VIDEO_FORMAT_YUV422    0x0006
#define SAGA_VIDEO_FORMAT_OFF       0x8000

#define SAGA_VIDEO_DBLSCAN_OFF      0
#define SAGA_VIDEO_DBLSCAN_X        1
#define SAGA_VIDEO_DBLSCAN_Y        2
#define SAGA_VIDEO_DBLSCAN_XY       (SAGA_VIDEO_DBLSCAN_X|SAGA_VIDEO_DBLSCAN_Y)

#define SAGA_VIDEO_FORMAT_BE        0
#define SAGA_VIDEO_FORMAT_LE        1
#define SAGA_VIDEO_FORMAT_ENDIAN    7

#define SAGA_BYTESWAP (SAGA_VIDEO_FORMAT_LE << SAGA_VIDEO_FORMAT_ENDIAN)

#define SAGA_VIDEO_MODE_FORMAT(x)   (((x) & 0xff) << 0)
#define SAGA_VIDEO_MODE_DBLSCN(x)   (((x) & 0xff) << 8)

#define SAGA_CLUT_ENTRY_VALID       (1UL << 31)

#define IS_DOUBLEX(w) ((w) <= 500)
#define IS_DOUBLEY(h) ((h) <= 300)

#endif /* SAGAGFX_HW_H */
