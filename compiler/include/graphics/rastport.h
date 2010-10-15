#ifndef GRAPHICS_RASTPORT_H
#define GRAPHICS_RASTPORT_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Rastport
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef GRAPHICS_GFX_H
#   include <graphics/gfx.h>
#endif

struct AreaInfo
{
    WORD * VctrTbl;
    WORD * VctrPtr;
    BYTE * FlagTbl;
    BYTE * FlagPtr;
    WORD   Count;
    WORD   MaxCount;
    WORD   FirstX;
    WORD   FirstY;
};

struct GelsInfo
{
    BYTE               sprRsrvd;
    UBYTE              Flags;
    struct VSprite   * gelHead;
    struct VSprite   * gelTail;
    WORD             * nextLine;
    WORD            ** lastColor;
    struct collTable * collHandler;
    WORD               leftmost;
    WORD               rightmost;
    WORD               topmost;
    WORD               bottommost;
    APTR               firstBlissObj;
    APTR               lastBlissObj;
};

struct TmpRas
{
    BYTE * RasPtr;
    LONG   Size;
};

struct RastPort
{
    struct Layer    * Layer;
    struct BitMap   * BitMap;
    const UWORD     * AreaPtrn;
    struct TmpRas   * TmpRas;
    struct AreaInfo * AreaInfo;
    struct GelsInfo * GelsInfo;
    UBYTE             Mask;
    BYTE              FgPen;
    BYTE              BgPen;
    BYTE              AOlPen;
    BYTE              DrawMode;
    BYTE              AreaPtSz;
    BYTE              linpatcnt;
    BYTE              dummy;
    UWORD             Flags;
    UWORD             LinePtrn;
    WORD              cp_x;
    WORD              cp_y;
    UBYTE             minterms[8];
    WORD              PenWidth;
    WORD              PenHeight;
    struct TextFont * Font;
    UBYTE             AlgoStyle;
    UBYTE             TxFlags;
    UWORD             TxHeight;
    UWORD             TxWidth;
    UWORD             TxBaseline;
    WORD              TxSpacing;
    APTR            * RP_User;
    IPTR              longreserved[2];
    UWORD             wordreserved[7];
    UBYTE             reserved[8];
};

/* Flags */
#define FRST_DOT (1<<0)
#define ONE_DOT  (1<<1)
#define DBUFFER  (1<<2)

/* Drawing Modes */
#define JAM1       0
#define JAM2       1
#define COMPLEMENT 2
#define INVERSVID  4

#define AREAOUTLINE 0x08
#define NOCROSSFILL 0x20

#endif /* GRAPHICS_RASTPORT_H */
