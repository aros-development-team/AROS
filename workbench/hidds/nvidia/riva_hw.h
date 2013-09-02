 /***************************************************************************\
|*                                                                           *|
|*       Copyright 1993-1999 NVIDIA, Corporation.  All rights reserved.      *|
|*                                                                           *|
|*     NOTICE TO USER:   The source code  is copyrighted under  U.S. and     *|
|*     international laws.  Users and possessors of this source code are     *|
|*     hereby granted a nonexclusive,  royalty-free copyright license to     *|
|*     use this code in individual and commercial software.                  *|
|*                                                                           *|
|*     Any use of this source code must include,  in the user documenta-     *|
|*     tion and  internal comments to the code,  notices to the end user     *|
|*     as follows:                                                           *|
|*                                                                           *|
|*       Copyright 1993-1999 NVIDIA, Corporation.  All rights reserved.      *|
|*                                                                           *|
|*     NVIDIA, CORPORATION MAKES NO REPRESENTATION ABOUT THE SUITABILITY     *|
|*     OF  THIS SOURCE  CODE  FOR ANY PURPOSE.  IT IS  PROVIDED  "AS IS"     *|
|*     WITHOUT EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  NVIDIA, CORPOR-     *|
|*     ATION DISCLAIMS ALL WARRANTIES  WITH REGARD  TO THIS SOURCE CODE,     *|
|*     INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGE-     *|
|*     MENT,  AND FITNESS  FOR A PARTICULAR PURPOSE.   IN NO EVENT SHALL     *|
|*     NVIDIA, CORPORATION  BE LIABLE FOR ANY SPECIAL,  INDIRECT,  INCI-     *|
|*     DENTAL, OR CONSEQUENTIAL DAMAGES,  OR ANY DAMAGES  WHATSOEVER RE-     *|
|*     SULTING FROM LOSS OF USE,  DATA OR PROFITS,  WHETHER IN AN ACTION     *|
|*     OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,  ARISING OUT OF     *|
|*     OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOURCE CODE.     *|
|*                                                                           *|
|*     U.S. Government  End  Users.   This source code  is a "commercial     *|
|*     item,"  as that  term is  defined at  48 C.F.R. 2.101 (OCT 1995),     *|
|*     consisting  of "commercial  computer  software"  and  "commercial     *|
|*     computer  software  documentation,"  as such  terms  are  used in     *|
|*     48 C.F.R. 12.212 (SEPT 1995)  and is provided to the U.S. Govern-     *|
|*     ment only as  a commercial end item.   Consistent with  48 C.F.R.     *|
|*     12.212 and  48 C.F.R. 227.7202-1 through  227.7202-4 (JUNE 1995),     *|
|*     all U.S. Government End Users  acquire the source code  with only     *|
|*     those rights set forth herein.                                        *|
|*                                                                           *|
\***************************************************************************/

#ifndef __RIVA_HW_H__
#define __RIVA_HW_H__
#define RIVA_SW_VERSION 0x00010003

#include <exec/types.h>

#define mem_barrier()	/* eps */

/*
 * Define supported architectures.
 */
#define NV_ARCH_03  0x03
#define NV_ARCH_04  0x04
#define NV_ARCH_10  0x10
#define NV_ARCH_20  0x20
#define NV_ARCH_30  0x30
#define NV_ARCH_40  0x40

/***************************************************************************\
*                                                                           *
*                             FIFO registers.                               *
*                                                                           *
\***************************************************************************/

/*
 * Raster OPeration. Windows style ROP3.
 */
typedef volatile struct
{
    ULONG reserved00[4];
#if AROS_BIG_ENDIAN
    ULONG FifoFree;
#else
    UWORD FifoFree;
    UWORD Nop;
#endif
    ULONG reserved01[0x0BB];
    ULONG Rop3;
} RivaRop;
/*
 * 8X8 Monochrome pattern.
 */
typedef volatile struct
{
    ULONG reserved00[4];
#if AROS_BIG_ENDIAN
    ULONG FifoFree;
#else
    UWORD FifoFree;
    UWORD Nop;
#endif
    ULONG reserved01[0x0BD];
    ULONG Shape;
    ULONG reserved03[0x001];
    ULONG Color0;
    ULONG Color1;
    ULONG Monochrome[2];
} RivaPattern;
/*
 * Scissor clip rectangle.
 */
typedef volatile struct
{
    ULONG reserved00[4];
#if AROS_BIG_ENDIAN
    ULONG FifoFree;
#else
    UWORD FifoFree;
    UWORD Nop;
#endif
    ULONG reserved01[0x0BB];
    ULONG TopLeft;
    ULONG WidthHeight;
} RivaClip;
/*
 * 2D filled rectangle.
 */
typedef volatile struct
{
    ULONG reserved00[4];
#if AROS_BIG_ENDIAN
    ULONG FifoFree;
#else
    UWORD FifoFree;
    UWORD Nop[1];
#endif
    ULONG reserved01[0x0BC];
    ULONG Color;
    ULONG reserved03[0x03E];
    ULONG TopLeft;
    ULONG WidthHeight;
} RivaRectangle;
/*
 * 2D screen-screen BLT.
 */
typedef volatile struct
{
    ULONG reserved00[4];
#if AROS_BIG_ENDIAN
    ULONG FifoFree;
#else
    UWORD FifoFree;
    UWORD Nop;
#endif
    ULONG reserved01[0x0BB];
    ULONG TopLeftSrc;
    ULONG TopLeftDst;
    ULONG WidthHeight;
} RivaScreenBlt;
/*
 * 2D pixel BLT.
 */
typedef volatile struct
{
    ULONG reserved00[4];
#if AROS_BIG_ENDIAN
    ULONG FifoFree;
#else
    UWORD FifoFree;
    UWORD Nop[1];
#endif
    ULONG reserved01[0x0BC];
    ULONG TopLeft;
    ULONG WidthHeight;
    ULONG WidthHeightIn;
    ULONG reserved02[0x03C];
    ULONG Pixels;
} RivaPixmap;
/*
 * Filled rectangle combined with monochrome expand.  Useful for glyphs.
 */
typedef volatile struct
{
    ULONG reserved00[4];
#if AROS_BIG_ENDIAN
    ULONG FifoFree;
#else
    UWORD FifoFree;
    UWORD Nop;
#endif
    ULONG reserved01[0x0BB];
    ULONG reserved03[(0x040)-1];
    ULONG Color1A;
    struct
    {
        ULONG TopLeft;
        ULONG WidthHeight;
    } UnclippedRectangle[64];
    ULONG reserved04[(0x080)-3];
    struct
    {
        ULONG TopLeft;
        ULONG BottomRight;
    } ClipB;
    ULONG Color1B;
    struct
    {
        ULONG TopLeft;
        ULONG BottomRight;
    } ClippedRectangle[64];
    ULONG reserved05[(0x080)-5];
    struct
    {
        ULONG TopLeft;
        ULONG BottomRight;
    } ClipC;
    ULONG Color1C;
    ULONG WidthHeightC;
    ULONG PointC;
    ULONG MonochromeData1C;
    ULONG reserved06[(0x080)+121];
    struct
    {
        ULONG TopLeft;
        ULONG BottomRight;
    } ClipD;
    ULONG Color1D;
    ULONG WidthHeightInD;
    ULONG WidthHeightOutD;
    ULONG PointD;
    ULONG MonochromeData1D;
    ULONG reserved07[(0x080)+120];
    struct
    {
        ULONG TopLeft;
        ULONG BottomRight;
    } ClipE;
    ULONG Color0E;
    ULONG Color1E;
    ULONG WidthHeightInE;
    ULONG WidthHeightOutE;
    ULONG PointE;
    ULONG MonochromeData01E;
} RivaBitmap;
/*
 * 2D line.
 */
typedef volatile struct
{
    ULONG reserved00[4];
#if AROS_BIG_ENDIAN
    ULONG FifoFree;
#else
    UWORD FifoFree;
    UWORD Nop[1];
#endif
    ULONG reserved01[0x0BC];
    ULONG Color;             /* source color               0304-0307*/
    ULONG Reserved02[0x03e];
    struct {                /* start aliased methods in array   0400-    */
        ULONG point0;        /* y_x S16_S16 in pixels            0-   3*/
        ULONG point1;        /* y_x S16_S16 in pixels            4-   7*/
    } Lin[16];              /* end of aliased methods in array      -047f*/
    struct {                /* start aliased methods in array   0480-    */
        ULONG point0X;       /* in pixels, 0 at left                0-   3*/
        ULONG point0Y;       /* in pixels, 0 at top                 4-   7*/
        ULONG point1X;       /* in pixels, 0 at left                8-   b*/
        ULONG point1Y;       /* in pixels, 0 at top                 c-   f*/
    } Lin32[8];             /* end of aliased methods in array      -04ff*/
    ULONG PolyLin[32];       /* y_x S16_S16 in pixels         0500-057f*/
    struct {                /* start aliased methods in array   0580-    */
        ULONG x;             /* in pixels, 0 at left                0-   3*/
        ULONG y;             /* in pixels, 0 at top                 4-   7*/
    } PolyLin32[16];        /* end of aliased methods in array      -05ff*/
    struct {                /* start aliased methods in array   0600-    */
        ULONG color;         /* source color                     0-   3*/
        ULONG point;         /* y_x S16_S16 in pixels            4-   7*/
    } ColorPolyLin[16];     /* end of aliased methods in array      -067f*/
} RivaLine;
/*
 * 2D/3D surfaces
 */
typedef volatile struct
{
    ULONG reserved00[4];
#if AROS_BIG_ENDIAN
    ULONG FifoFree;
#else
    UWORD FifoFree;
    UWORD Nop;
#endif
    ULONG reserved01[0x0BE];
    ULONG Offset;
} RivaSurface;

typedef volatile struct
{
    ULONG reserved00[4];
#if AROS_BIG_ENDIAN
    ULONG FifoFree;
#else
    UWORD FifoFree;
    UWORD Nop;
#endif
    ULONG reserved01[0x0BB];
    ULONG SurfaceFormat;
    ULONG SurfacePitch;
    ULONG OffsetSrc;
    ULONG OffsetDst;
} RivaSurface2;

typedef volatile struct
{
    ULONG reserved00[4];
#if AROS_BIG_ENDIAN
    ULONG FifoFree;
#else
    UWORD FifoFree;
    UWORD Nop;
#endif
    ULONG reserved01[0x0BD];
    ULONG Pitch;
    ULONG RenderBufferOffset;
    ULONG ZBufferOffset;
} RivaSurface3D;
    
/***************************************************************************\
*                                                                           *
*                        Virtualized RIVA H/W interface.                    *
*                                                                           *
\***************************************************************************/

#define FP_ENABLE  1
#define FP_DITHER  2

#if 0
struct _riva_hw_inst;
struct _riva_hw_state;
/*
 * Virtialized chip interface. Makes RIVA 128 and TNT look alike.
 */
typedef struct _riva_hw_inst
{
    /*
     * Chip specific settings.
     */
    ULONG Architecture;
    ULONG Version;
    ULONG Chipset;
    ULONG CrystalFreqKHz;
    ULONG RamAmountKBytes;
    ULONG MaxVClockFreqKHz;
    ULONG RamBandwidthKBytesPerSec;
    ULONG EnableIRQ;
    ULONG IO;
    ULONG VBlankBit;
    ULONG FifoFreeCount;
    ULONG FifoEmptyCount;
    ULONG CursorStart;
    ULONG flatPanel;
    Bool twoHeads;
    /*
     * Non-FIFO registers.
     */
    volatile ULONG *PCRTC0;
    volatile ULONG *PCRTC;
    volatile ULONG *PRAMDAC0;
    volatile ULONG *PFB;
    volatile ULONG *PFIFO;
    volatile ULONG *PGRAPH;
    volatile ULONG *PEXTDEV;
    volatile ULONG *PTIMER;
    volatile ULONG *PMC;
    volatile ULONG *PRAMIN;
    volatile ULONG *FIFO;
    volatile ULONG *CURSOR;
    volatile U008 *PCIO0;
    volatile U008 *PCIO;
    volatile U008 *PVIO;
    volatile U008 *PDIO0;
    volatile U008 *PDIO;
    volatile ULONG *PRAMDAC;
    /*
     * Common chip functions.
     */
    int  (*Busy)(struct _riva_hw_inst *);
    void (*CalcStateExt)(struct _riva_hw_inst *,struct _riva_hw_state *,int,int,int,int,int,int);
    void (*LoadStateExt)(struct _riva_hw_inst *,struct _riva_hw_state *);
    void (*UnloadStateExt)(struct _riva_hw_inst *,struct _riva_hw_state *);
    void (*SetStartAddress)(struct _riva_hw_inst *,ULONG);
    int  (*ShowHideCursor)(struct _riva_hw_inst *,int);
    void (*LockUnlock)(struct _riva_hw_inst *, int);
    /*
     * Current extended mode settings.
     */
    struct _riva_hw_state *CurrentState;
    /*
     * FIFO registers.
     */
    RivaRop                 *Rop;
    RivaPattern             *Patt;
    RivaClip                *Clip;
    RivaPixmap              *Pixmap;
    RivaScreenBlt           *Blt;
    RivaBitmap              *Bitmap;
    RivaLine                *Line;
    RivaSurface2	    *Surface;
} RIVA_HW_INST;
/*
 * Extended mode state information.
 */
typedef struct _riva_hw_state
{
    ULONG bpp;
    ULONG width;
    ULONG height;
    ULONG interlace;
    ULONG repaint0;
    ULONG repaint1;
    ULONG screen;
    ULONG scale;
    ULONG dither;
    ULONG extra;
    ULONG pixel;
    ULONG horiz;
    ULONG arbitration0;
    ULONG arbitration1;
    ULONG vpll;
    ULONG vpll2;
    ULONG vpllB;
    ULONG vpll2B;
    ULONG pllsel;
    ULONG general;
    ULONG crtcOwner;
    ULONG head; 
    ULONG head2; 
    ULONG config;
    ULONG cursorConfig;
    ULONG cursor0;
    ULONG cursor1;
    ULONG cursor2;
    ULONG offset;
    ULONG pitch;
} RIVA_HW_STATE;
#endif
/*
 * FIFO Free Count. Should attempt to yield processor if RIVA is busy.
 */

#define RIVA_FIFO_FREE(hwinst,hwptr,cnt)                           \
{                                                                  \
   while ((hwinst).FifoFreeCount < (cnt)) {                          \
        mem_barrier(); \
        mem_barrier(); \
	(hwinst).FifoFreeCount = (hwinst).hwptr->FifoFree >> 2;        \
   } \
   (hwinst).FifoFreeCount -= (cnt);                                \
}
#define RIVA_BUSY(hwinst) \
{ \
   mem_barrier(); \
   while ((hwinst).Busy(&(hwinst))); \
}
#endif /* __RIVA_HW_H__ */

