#ifndef DEVICES_CONUNIT_H
#define DEVICES_CONUNIT_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_PORTS_H
#   include <exec/ports.h>
#endif
#ifndef DEVICES_CONSOLE_H
#   include <devices/console.h>
#endif
#ifndef DEVICES_KEYMAP_H
#   include <devices/keymap.h>
#endif
#ifndef DEVICES_INPUTEVENT_H
#   include <devices/inputevent.h>
#endif

/* Console units */
#define CONU_LIBRARY	-1
#define CONU_STANDARD	0
#define CONU_CHARMAP	1
#define CONU_SNIPMAP	3

#define CONFLAG_DEFAULT			0
#define CONFLAG_NODRAW_ON_NEWSIZE	1

#define PMB_ASM		(M_LNM + 1)
#define PMB_AWM		(PMB_ASM + 1)
#define MAXTABS		80

struct ConUnit
{
    struct MsgPort cu_MP;
    
    struct Window *cu_Window;    
    WORD	cu_XCP;
    WORD	cu_YCP;
    WORD	cu_XMax;
    WORD	cu_YMax;
    WORD	cu_XRSize;
    WORD	cu_YRSize;
    WORD	cu_XROrigin;
    WORD	cu_YROrigin;
    WORD	cu_XRExtant;
    WORD	cu_YRExtant;
    WORD	cu_XMinShrink;
    WORD	cu_YMinShrink;
    WORD	cu_XCCP;
    WORD	cu_YCCP;
    
    struct KeyMap cu_KeyMapStruct;
    UWORD	cu_TabStops[MAXTABS];
    
    BYTE	cu_Mask;
    BYTE	cu_FgPen;
    BYTE	cu_BgPen;
    BYTE	cu_AOLPen;
    BYTE	cu_DrawMode;
    BYTE	cu_Obsolete1;
    APTR	cu_Obsolete2;
    UBYTE	cu_Minterms[8];
    struct TextFont *cu_Font;
    UBYTE	cu_AlgoStyle;
    UBYTE	cu_TxFlags;
    UWORD	cu_TxHeight;
    UWORD	cu_TxWidth;
    UWORD	cu_TxBaseline;
    WORD	cu_TxSpacing;
    
    UBYTE	cu_Modes[ (PMB_AWM + 7) / 8];
    UBYTE	cu_RawEvents[(IECLASS_MAX + 8) / 8];
};

#endif /* DEVICES_CONUNIT_H */
