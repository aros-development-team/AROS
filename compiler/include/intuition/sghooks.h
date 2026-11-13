#ifndef  INTUITION_SGHOOKS_H
#define INTUITION_SGHOOKS_H 

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include file for GTYP_STRGADGET gadgets.
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

struct StringExtend
{
    struct TextFont *Font;
    UBYTE	Pens[2];
    UBYTE	ActivePens[2];
    ULONG	InitialModes;
    struct Hook *EditHook;
    STRPTR	WorkBuffer;
    ULONG	Reserved[4];
};

struct SGWork
{
    struct Gadget	*Gadget;
    struct StringInfo	*StringInfo;
    STRPTR		WorkBuffer;
    STRPTR		PrevBuffer;
    ULONG		Modes;
    struct InputEvent	*IEvent;
    UWORD		Code;
    WORD		BufferPos;
    WORD		NumChars;
    ULONG		Actions;
    LONG		LongInt;
    struct GadgetInfo	*GadgetInfo;
    UWORD		EditOp;
};

#define EO_NOOP		(0x0001)
#define EO_DELBACKWARD	(0x0002)
#define EO_DELFORWARD	(0x0003)
#define EO_MOVECURSOR	(0x0004)
#define EO_ENTER	(0x0005)
#define EO_RESET	(0x0006)
#define EO_REPLACECHAR	(0x0007)
#define EO_INSERTCHAR	(0x0008)
#define EO_BADFORMAT	(0x0009)
#define EO_BIGCHANGE	(0x000A)
#define EO_UNDO		(0x000B)
#define EO_CLEAR	(0x000C)
#define EO_SPECIAL	(0x000D)

#define SGM_REPLACE	(1L << 0)
#define SGM_FIXEDFIELD	(1L << 1)
#define SGM_NOFILTER	(1L << 2)
#define SGM_EXITHELP	(1L << 7)

/* For internal use */
#define SGM_NOCHANGE	(1L << 3)
#define SGM_NOWORKB	(1L << 4)
#define SGM_CONTROL	(1L << 5)
#define SGM_LONGINT	(1L << 6)

#define SGA_USE		(0x1L)
#define SGA_END		(0x2L)
#define SGA_BEEP	(0x4L)
#define SGA_REUSE	(0x8L)
#define SGA_REDISPLAY	(0x10L)
#define SGA_NEXTACTIVE	(0x20L)
#define SGA_PREVACTIVE	(0x40L)

#define SGH_KEY		(1L)
#define SGH_CLICK	(2L)

#endif /* INTUITION_SGHOOKS_H */
