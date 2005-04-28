/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by BetterString.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 BetterString class Support Site:  http://www.sf.net/projects/bstring-mcc/

 $Id: private.h,v 1.1 2005/04/21 20:52:04 damato Exp $

***************************************************************************/

#ifndef BETTERSTRING_MCC_PRIV_H
#define BETTERSTRING_MCC_PRIV_H

#include <dos/exall.h>
#include <exec/types.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>

#ifndef __AROS__
#include <muiextra.h>
#else
#define MUI_EHF_GUIMODE 0
#endif
#include <mcc_common.h>
#include <mcc_debug.h>

#include "BetterString_mcc.h"

struct FNCData
{
	struct FNCData *next;
	struct ExAllData buffer;
};

struct InstData
{
	/* Normal stringgadget info */
	STRPTR	Contents;
	STRPTR	Original;		/* Amiga-q (toggle) */
	STRPTR	Undo;				/* Amiga-z (toggle) */
	STRPTR	Accept;
	STRPTR	Reject;
	Object	*ForwardObject;
	Object	*Popup;			/* ctrl-p popup object */
	UWORD		DisplayPos;
	UWORD		BufferPos;
	UWORD		MaxLength;
	WORD		Alignment;
	UWORD		UndoPos;
	UWORD		Width;

	/* Various system resources and data */
	struct	MUI_EventHandlerNode		ehnode;
	struct	RastPort						rport;
	struct	Locale						*locale;
	APTR		Pool;
	ULONG		Flags;
	Object	*PopupMenu;

	Object	*KeyUpFocus, *KeyDownFocus;

	/* Filename completion */
	struct	FNCData *FNCBuffer;
	WORD		FileNumber;
	UWORD		FileEntries;
	UWORD		FileNameStart;

	/* For marking of text */
	UWORD		BlockStart;
	WORD		BlockStop;

	/* For double/triple clicking */
	ULONG		StartSecs, StartMicros;
	UBYTE		ClickCount;

	/* The hotkey which activates the stringgadget */
	UBYTE		CtrlChar;

	/* Config */
	STRPTR	InactiveBackground;
	ULONG		InactiveText;
	STRPTR	ActiveBackground;
	ULONG		ActiveText;
	ULONG		CursorColor;
	ULONG		MarkedColor;
	ULONG		MarkedTextColor;
	struct	TextFont *Font;
};

#define FLG_Secret        (1L << 0)
#define FLG_AdvanceOnCr   (1L << 1)
#define FLG_BlockEnabled  (1L << 2)
#define FLG_Active        (1L << 3)
#define FLG_Ghosted       (1L << 4)
#define FLG_Shown         (1L << 5)
#define FLG_Original      (1L << 6)
#define FLG_RedoAvailable (1L << 7)
#define FLG_StayActive    (1L << 8)
#define FLG_SetFrame      (1L << 9)
#define FLG_OwnFont       (1L << 10)
#define FLG_OwnBackground (1L << 11)
#define FLG_NoInput       (1L << 12)
#define FLG_DragOutside   (1L << 13)

// our prototypes
VOID PrintString(struct IClass *, Object *);
ULONG HandleInput(struct IClass *, Object *, struct MUIP_HandleEvent *);
ULONG ConvertKey(struct IntuiMessage *);
VOID DeleteBlock(struct InstData *);

LONG MyTextLength(struct TextFont *, STRPTR, LONG);
LONG MyTextFit(struct TextFont *, STRPTR, LONG, LONG, LONG);

ULONG Get(struct IClass *, Object *, struct opGet *);
ULONG Set(struct IClass *, Object *, struct opSet *);

APTR MyAllocPooled(APTR, ULONG);
VOID MyFreePooled(APTR, APTR);
APTR ExpandPool(APTR, APTR, ULONG);

VOID strcpyback(STRPTR, STRPTR);

BOOL Overwrite(STRPTR, UWORD, UWORD, struct InstData *);
BOOL OverwriteA(STRPTR, UWORD, UWORD, UWORD, struct InstData *);
BOOL FileNameComplete(Object *, BOOL, struct InstData *);
LONG FileNameStart(struct MUIP_BetterString_FileNameStart *msg);

WORD CmpStrings(REG(a0, STRPTR), REG(a1, STRPTR));

VOID InitConfig(Object *, struct InstData *);
VOID FreeConfig(struct MUI_RenderInfo *, struct InstData *);

struct BitMap * SAVEDS ASM MUIG_AllocBitMap(REG(d0, LONG), REG(d1, LONG), REG(d2, LONG), REG(d3, LONG flags), REG(a0, struct BitMap *));
VOID SAVEDS ASM MUIG_FreeBitMap(REG(a0, struct BitMap *));

#endif /* BETTERSTRING_MCC_PRIV_H */
