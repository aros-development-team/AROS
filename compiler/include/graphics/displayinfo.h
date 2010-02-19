#ifndef GRAPHICS_DISPLAYINFO_H
#define GRAPHICS_DISPLAYINFO_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DisplayInfo definitions and structures.
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef GRAPHICS_GFX_H
#   include <graphics/gfx.h>
#endif
#ifndef GRAPHICS_MODEID_H
#   include <graphics/modeid.h>
#endif
#ifndef GRAPHICS_MONITOR_H
#   include <graphics/monitor.h>
#endif
#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

typedef APTR DisplayInfoHandle;

struct QueryHeader
{
    ULONG StructID;
    ULONG DisplayID;
    ULONG SkipID;
    ULONG Length;
};

struct DisplayInfo
{
    struct QueryHeader Header;

    UWORD NotAvailable;     /* ==0 means DisplayInfo is available
                               !=0 means not available (see below) */
    ULONG PropertyFlags;    /* see below */
    Point Resolution;
    UWORD PixelSpeed;
    UWORD NumStdSprites;
    UWORD PaletteRange;
    Point SpriteResolution;
    UBYTE pad[4];
    UBYTE RedBits;
    UBYTE GreenBits;
    UBYTE BlueBits;
    UBYTE pad2[5];
    ULONG reserved[2];
};

/* NotAvailable */
#define DI_AVAIL_NOCHIPS        (1<<0)
#define DI_AVAIL_NOMONITOR      (1<<1)
#define DI_AVAIL_NOTWITHGENLOCK (1<<2)

/* PropertyFlags */
#define DIPF_IS_LACE              (1L<<0)
#define DIPF_IS_DUALPF            (1L<<1)
#define DIPF_IS_PF2PRI            (1L<<2)
#define DIPF_IS_HAM               (1L<<3)
#define DIPF_IS_ECS               (1L<<4)
#define DIPF_IS_PAL               (1L<<5)
#define DIPF_IS_SPRITES           (1L<<6)
#define DIPF_IS_GENLOCK           (1L<<7)
#define DIPF_IS_WB                (1L<<8)
#define DIPF_IS_DRAGGABLE         (1L<<9)
#define DIPF_IS_PANELLED          (1L<<10)
#define DIPF_IS_BEAMSYNC          (1L<<11)
#define DIPF_IS_EXTRAHALFBRITE    (1L<<12)
#define DIPF_IS_SPRITES_ATT       (1L<<13)
#define DIPF_IS_SPRITES_CHNG_RES  (1L<<14)
#define DIPF_IS_SPRITES_BORDER    (1L<<15)
#define DIPF_IS_AA                (1L<<16)
#define DIPF_IS_SCANDBL           (1L<<17)
#define DIPF_IS_SPRITES_CHNG_BASE (1L<<18)
#define DIPF_IS_SPRITES_CHNG_PRI  (1L<<19)
#define DIPF_IS_DBUFFER           (1L<<20)
#define DIPF_IS_PROGBEAM          (1L<<21)
#define DIPF_IS_FOREIGN           (1L<<31)

struct DimensionInfo
{
    struct QueryHeader Header;

    UWORD MaxDepth;
    UWORD MinRasterWidth;
    UWORD MinRasterHeight;
    UWORD MaxRasterWidth;
    UWORD MaxRasterHeight;

    struct Rectangle Nominal;
    struct Rectangle MaxOScan;
    struct Rectangle VideoOScan;
    struct Rectangle TxtOScan;
    struct Rectangle StdOScan;

    UBYTE pad[14];
    ULONG reserved[2];
};

struct MonitorInfo
{
    struct QueryHeader Header;

    struct MonitorSpec * Mspc;
    Point                ViewPosition;
    Point                ViewResolution;
    struct Rectangle     ViewPositionRange;
    UWORD                TotalRows;
    UWORD                TotalColorClocks;
    UWORD                MinRow;
    WORD                 Compatibility;       /* see below */
    UBYTE                pad[32];
    Point                MouseTicks;
    Point                DefaultViewPosition;
    ULONG                PreferredModeID;
    ULONG                reserved[2];
};

/* Compability */
#define MCOMPAT_NOBODY -1
#define MCOMPAT_MIXED   0
#define MCOMPAT_SELF    1

#define DISPLAYNAMELEN 32
struct NameInfo
{
    struct QueryHeader Header;

    UBYTE Name[DISPLAYNAMELEN];
    ULONG reserved[2];
};

/* Tags */
#define DTAG_DISP (TAG_USER)
#define DTAG_DIMS (TAG_USER + 0x1000)
#define DTAG_MNTR (TAG_USER + 0x2000)
#define DTAG_NAME (TAG_USER + 0x3000)
#define DTAG_VEC  (TAG_USER + 0x4000)

/* PRIVATE */
struct VecInfo
{
    struct QueryHeader Header;

    APTR  Vec;
    APTR  Data;
    UWORD Type;
    UWORD pad[3];
    IPTR reserved[2];
};

#endif /* GRAPHICS_DISPLAYINFO_H */
