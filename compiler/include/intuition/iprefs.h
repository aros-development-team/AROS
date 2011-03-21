#ifndef INTUITION_IPREFS_H
#define INTUITION_IPREFS_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PRIVATE/TOP SECRET!!! Communication between IPrefs program and Intuition
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef GRAPHICS_GFX_H
#   include <graphics/gfx.h>
#endif
#ifndef GRAPHICS_TEXT_H
#   include <graphics/text.h>
#endif
#ifndef INTUITION_SCREENS_H
#   include <intuition/screens.h>
#endif

/* These values may change in order to provide 
   binary compatibility with AmigaOS on m68k */

#ifdef __mc68000

#define IPREFS_TYPE_SCREENMODE_V37 1
#define IPREFS_TYPE_FONT_V37       2
#define IPREFS_TYPE_OVERSCAN_V37   3
#define IPREFS_TYPE_ICONTROL_V37   4
#define IPREFS_TYPE_POINTER_V37    5
#define IPREFS_TYPE_PALETTE_V37    6
#define IPREFS_TYPE_POINTER_V39    7
#define IPREFS_TYPE_PALETTE_V39    8
#define IPREFS_TYPE_PENS_V39       9
/* need to reserve space if OS3.5+ introduced new types? */
#define IPREFS_TYPE_POINTER_ALPHA 10

#else

#define IPREFS_TYPE_ICONTROL_V37   0
#define IPREFS_TYPE_SCREENMODE_V37 1
#define IPREFS_TYPE_POINTER_V39    2
#define IPREFS_TYPE_PALETTE_V39    3
#define IPREFS_TYPE_POINTER_ALPHA  4

/* dummy entries */
#define IPREFS_TYPE_FONT_V37       10
#define IPREFS_TYPE_OVERSCAN_V37   11
#define IPREFS_TYPE_POINTER_V37    12
#define IPREFS_TYPE_PALETTE_V37    13
#define IPREFS_TYPE_PENS_V39       14

#endif

/* backwards compatibility */
#define IPREFS_TYPE_SCREENMODE IPREFS_TYPE_SCREENMODE_V37
#define IPREFS_TYPE_ICONTROL IPREFS_TYPE_ICONTROL_V37
#define IPREFS_TYPE_POINTER IPREFS_TYPE_POINTER_V39
#define IPREFS_TYPE_OLD_PALETTE IPREFS_TYPE_PALETTE_V39

struct IScreenModePrefs
{
    ULONG smp_DisplayID;
    UWORD smp_Width;
    UWORD smp_Height;
    UWORD smp_Depth;
    UWORD smp_Control;
};

struct IIControlPrefs
{
    UWORD ic_TimeOut;
    WORD  ic_MetaDrag;
    ULONG ic_Flags;
    UBYTE ic_WBtoFront;
    UBYTE ic_FrontToBack;
    UBYTE ic_ReqTrue;
    UBYTE ic_ReqFalse;
    UWORD ic_VDragModes[2];
};

struct IPointerPrefs
{
    struct BitMap *BitMap;
    WORD  XOffset;
    WORD  YOffset;
    UWORD BytesPerRow;
    UWORD Size;
    UWORD YSize;
    UWORD Which;
    ULONG Zero;
};

struct IOldPenPrefs
{
    UWORD Count;
    UWORD Type;
    ULONG Pad;
    UWORD PenTable[NUMDRIPENS+1];
};

struct IOldOverScanPrefs
{
    ULONG DisplayID;
    Point ViewPos;
    Point Text;
    struct Rectangle Standard;
};

struct IOldFontPrefs
{
    struct TextAttr fp_TextAttr;
    UBYTE           fp_Name[32];
    ULONG           fp_NotUsed;
    WORD            fp_Type;
};

struct IFontPrefs
{
    struct TextAttr fp_TextAttr;
    UBYTE           fp_Name[32];
    ULONG           fp_xxx;
    BOOL            fp_ScrFont;
};

#endif /* INTUITION_IPREFS_H */
