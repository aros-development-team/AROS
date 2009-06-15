/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2009 by BetterString.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 BetterString class Support Site:  http://www.sf.net/projects/bstring-mcc/

 $Id$

***************************************************************************/

#ifndef BETTERSTRING_MCC_PRIV_H
#define BETTERSTRING_MCC_PRIV_H

#include <dos/exall.h>
#include <exec/types.h>
#include <proto/intuition.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>

#include <muiextra.h>
#include <mcc_common.h>

#include "BetterString_mcc.h"
#include "Debug.h"

struct FNCData
{
  struct FNCData *next;
  struct ExAllData buffer;
};

struct InstData
{
  /* Normal stringgadget info */
  STRPTR  Contents;
  STRPTR  InactiveContents;
  STRPTR  Original;   /* Amiga-q (toggle) */
  STRPTR  Undo;       /* Amiga-z (toggle) */
  STRPTR  Accept;
  STRPTR  Reject;
  Object  *ForwardObject;
  Object  *Popup;     /* ctrl-p popup object */
  UWORD   DisplayPos;
  UWORD   BufferPos;
  UWORD   BufferLastPos;
  UWORD   MaxLength;
  WORD    Alignment;
  UWORD   UndoPos;
  UWORD   Width;

  /* Various system resources and data */
  struct  MUI_EventHandlerNode    ehnode;
  struct  RastPort            rport;
  struct  Locale            *locale;
  APTR    Pool;
  ULONG   Flags;
  Object  *PopupMenu;

  Object  *KeyUpFocus, *KeyDownFocus;

  // the selection pointer Object
  Object  *PointerObj;
  BOOL    activeSelectPointer;

  /* Filename completion */
  struct  FNCData *FNCBuffer;
  WORD    FileNumber;
  UWORD   FileEntries;
  UWORD   FileNameStart;

  /* For marking of text */
  UWORD   BlockStart;
  WORD    BlockStop;

  /* For double/triple clicking */
  ULONG   StartSecs, StartMicros;
  UBYTE   ClickCount;

  /* The hotkey which activates the stringgadget */
  UBYTE   CtrlChar;

  /* Config */
  STRPTR  InactiveBackground;
  ULONG   InactiveText;
  STRPTR  ActiveBackground;
  ULONG   ActiveText;
  ULONG   CursorColor;
  ULONG   MarkedColor;
  ULONG   MarkedTextColor;
  struct  TextFont *Font;
  BOOL    SelectOnActive;
  BOOL    SelectPointer;

  /* Edit hook */
  struct Hook *EditHook;
};

#define FLG_Secret         (1L << 0)
#define FLG_AdvanceOnCr    (1L << 1)
#define FLG_BlockEnabled   (1L << 2)
#define FLG_Active         (1L << 3)
#define FLG_Ghosted        (1L << 4)
#define FLG_Shown          (1L << 5)
#define FLG_Original       (1L << 6)
#define FLG_RedoAvailable  (1L << 7)
#define FLG_StayActive     (1L << 8)
#define FLG_SetFrame       (1L << 9)
#define FLG_OwnFont        (1L << 10)
#define FLG_OwnBackground  (1L << 11)
#define FLG_NoInput        (1L << 12)
#define FLG_DragOutside    (1L << 13)
#define FLG_NoShortcuts    (1L << 14)
#define FLG_ForceSelectOn  (1L << 15)
#define FLG_ForceSelectOff (1L << 16)
#define FLG_FreshActive    (1L << 17)

// proper RAWKEY_ defines were first introduced in OS4 and MorphOS
// and unfortunately they are also a bit different, so lets
// prepare an alternate table for it
#if defined(__amigaos4__)
#include <proto/keymap.h>

#define RAWKEY_SCRLOCK    RAWKEY_MENU
#define RAWKEY_NUMLOCK    0x79

#elif defined(__MORPHOS__)
#include <devices/rawkeycodes.h>

#define RAWKEY_CRSRUP     RAWKEY_UP
#define RAWKEY_CRSRDOWN   RAWKEY_DOWN
#define RAWKEY_CRSRRIGHT  RAWKEY_RIGHT
#define RAWKEY_CRSRLEFT   RAWKEY_LEFT
#define RAWKEY_PRINTSCR   RAWKEY_PRTSCREEN
#define RAWKEY_BREAK      RAWKEY_PAUSE
#define RAWKEY_DEL        RAWKEY_DELETE

#define RAWKEY_AUD_STOP       RAWKEY_CDTV_STOP
#define RAWKEY_AUD_PLAY_PAUSE RAWKEY_CDTV_PLAY
#define RAWKEY_AUD_PREV_TRACK RAWKEY_CDTV_PREV
#define RAWKEY_AUD_NEXT_TRACK RAWKEY_CDTV_NEXT
#define RAWKEY_AUD_SHUFFLE    RAWKEY_CDTV_REW
#define RAWKEY_AUD_REPEAT     RAWKEY_CDTV_FF

#else

#define RAWKEY_BACKSPACE 0x41
#define RAWKEY_TAB       0x42
#define RAWKEY_DEL       0x46
#define RAWKEY_INSERT    0x47 /* Not on classic keyboards */
#define RAWKEY_PAGEUP    0x48 /* Not on classic keyboards */
#define RAWKEY_PAGEDOWN  0x49 /* Not on classic keyboards */
#define RAWKEY_F11       0x4B /* Not on classic keyboards */
#define RAWKEY_CRSRUP    0x4C
#define RAWKEY_CRSRDOWN  0x4D
#define RAWKEY_CRSRRIGHT 0x4E
#define RAWKEY_CRSRLEFT  0x4F
#define RAWKEY_F1        0x50
#define RAWKEY_F2        0x51
#define RAWKEY_F3        0x52
#define RAWKEY_F4        0x53
#define RAWKEY_F5        0x54
#define RAWKEY_F6        0x55
#define RAWKEY_F7        0x56
#define RAWKEY_F8        0x57
#define RAWKEY_F9        0x58
#define RAWKEY_F10       0x59
#define RAWKEY_HELP      0x5F
#define RAWKEY_SCRLOCK   0x6B /* Not on classic keyboards */
#define RAWKEY_PRINTSCR  0x6D /* Not on classic keyboards */
#define RAWKEY_BREAK     0x6E /* Not on classic keyboards */
#define RAWKEY_F12       0x6F /* Not on classic keyboards */
#define RAWKEY_HOME      0x70 /* Not on classic keyboards */
#define RAWKEY_END       0x71 /* Not on classic keyboards */

#define RAWKEY_AUD_STOP       0x72
#define RAWKEY_AUD_PLAY_PAUSE 0x73
#define RAWKEY_AUD_PREV_TRACK 0x74
#define RAWKEY_AUD_NEXT_TRACK 0x75
#define RAWKEY_AUD_SHUFFLE    0x76
#define RAWKEY_AUD_REPEAT     0x77

#define RAWKEY_NUMLOCK   0x79

#endif

// our prototypes
VOID PrintString(struct IClass *, Object *);
IPTR HandleInput(struct IClass *, Object *, struct MUIP_HandleEvent *);
ULONG ConvertKey(struct IntuiMessage *);
VOID DeleteBlock(struct InstData *);

IPTR Get(struct IClass *, Object *, struct opGet *);
IPTR Set(struct IClass *, Object *, struct opSet *);
IPTR mDoAction(struct IClass *, Object *, struct MUIP_BetterString_DoAction *);

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

// Pointer.c
void SetupSelectPointer(struct InstData *data);
void CleanupSelectPointer(struct InstData *data);
void ShowSelectPointer(Object *obj, struct InstData *data);
void HideSelectPointer(Object *obj, struct InstData *data);

#define setFlag(mask, flag)             (mask) |= (flag)
#define clearFlag(mask, flag)           (mask) &= ~(flag)
#define isAnyFlagSet(mask, flag)        (((mask) & (flag)) != 0)
#define isFlagSet(mask, flag)           (((mask) & (flag)) == (flag))
#define isFlagClear(mask, flag)         (((mask) & (flag)) == 0)

#if defined(__MORPHOS__)
#include <proto/exec.h>
#define IS_MORPHOS2 (((struct Library *)SysBase)->lib_Version >= 51)
#endif

// some own usefull MUI-style macros to check mouse positions in objects
#define _between(a,x,b) 					((x)>=(a) && (x)<=(b))
#define _isinobject(o,x,y) 				(_between(_mleft(o),(x),_mright (o)) && _between(_mtop(o) ,(y),_mbottom(o)))
#define _isinwholeobject(o,x,y) 	(_between(_left(o),(x),_right (o)) && _between(_top(o) ,(y),_bottom(o)))

/// xget()
//  Gets an attribute value from a MUI object
IPTR xget(Object *obj, const IPTR attr);
#if defined(__GNUC__)
  // please note that we do not evaluate the return value of GetAttr()
  // as some attributes (e.g. MUIA_Selected) always return FALSE, even
  // when they are supported by the object. But setting b=0 right before
  // the GetAttr() should catch the case when attr doesn't exist at all
  #define xget(OBJ, ATTR) ({IPTR b=0; GetAttr(ATTR, OBJ, &b); b;})
#endif
///

#endif /* BETTERSTRING_MCC_PRIV_H */
