/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by TextEditor.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 $Id: private.h,v 1.9 2005/12/09 13:03:47 gnikl Exp $

***************************************************************************/

#ifndef TEXTEDITOR_MCP_PRIV_H
#define TEXTEDITOR_MCP_PRIV_H

#include "TextEditor_mcp.h"

#include <mcc_common.h>

#include "Debug.h"

#define PREFSIMAGEOBJECT \
  BitmapObject,\
    MUIA_Bitmap_Bitmap,       (UBYTE *)&image_bitmap,\
    MUIA_Bitmap_Height,       IMAGE_HEIGHT,\
    MUIA_Bitmap_Precision,    0,\
    MUIA_Bitmap_SourceColors, (ULONG *)image_palette,\
    MUIA_Bitmap_Transparent,  0,\
    MUIA_Bitmap_Width,        IMAGE_WIDTH,\
    MUIA_FixHeight,           IMAGE_HEIGHT,\
    MUIA_FixWidth,            IMAGE_WIDTH,\
  End

#define MCPMAXRAWBUF 64

#define IEQUALIFIER_SHIFT   0x0200
#define IEQUALIFIER_ALT     0x0400
#define IEQUALIFIER_COMMAND 0x0800

// proper RAWKEY_ defines were first introduced in OS4 and MorphOS
// and unfortunately they are also a bit different, so lets
// prepare an alternate table for it
#if defined(__amigaos4__)
#include <proto/keymap.h>

#define RAWKEY_NUMLOCK    0x79

#elif defined(__MORPHOS__)
#include <devices/rawkeycodes.h>

#define RAWKEY_CRSRUP     RAWKEY_UP
#define RAWKEY_CRSRDOWN   RAWKEY_DOWN
#define RAWKEY_CRSRRIGHT  RAWKEY_RIGHT
#define RAWKEY_CRSRLEFT   RAWKEY_LEFT
#define RAWKEY_PRINTSCR   RAWKEY_PRTSCREEN
#define RAWKEY_BREAK      RAWKEY_PAUSE

#define RAWKEY_AUD_STOP       RAWKEY_CDTV_STOP
#define RAWKEY_AUD_PLAY_PAUSE RAWKEY_CDTV_PLAY
#define RAWKEY_AUD_PREV_TRACK RAWKEY_CDTV_PREV
#define RAWKEY_AUD_NEXT_TRACK RAWKEY_CDTV_NEXT
#define RAWKEY_AUD_SHUFFLE    RAWKEY_CDTV_REW
#define RAWKEY_AUD_REPEAT     RAWKEY_CDTV_FF

#else

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

enum
{
  mUp, mDown, mLeft, mRight, mPreviousPage, mNextPage,
  mStartOfLine, mEndOfLine, mTop, mBottom, mPreviousWord,
  mNextWord, mPreviousLine, mNextLine, mPreviousSentence,
  mNextSentence, kSuggestWord, kBackspace, kDelete, kReturn,
  kTab, kCut, kCopy, kPaste, kUndo, kRedo,
  kDelBOL, kDelEOL, kDelBOW, kDelEOW,
  kNextGadget, kGotoBookmark1, kGotoBookmark2, kGotoBookmark3,
  kSetBookmark1, kSetBookmark2, kSetBookmark3, kDelLine,
  mKey_LAST
};

struct InstData_MCP
{
  Object *editpopup;
  Object *obj;
  Object *hotkey;
  Object *keyfunctions;
  Object *keyfunctions_txt;
  Object *insertkey;
  Object *deletekey;
  Object *cursorwidth;
  Object *blinkspeed;
  Object *background;
  Object *frame;
  Object *textcolor;
  Object *highlightcolor;
  Object *cursorcolor;
  Object *markedcolor;
  Object *blockqual;
  Object *smooth;
  Object *tabsize;
  Object *normalfont;
  Object *fixedfont;
  Object *undosize;
  Object *typenspell;
  Object *lookupcmd;
  Object *suggestcmd;
  Object *keybindings;
  Object *SuggestExeType;
  Object *LookupExeType;
  Object *CheckWord;
  Object *separatorshine;
  Object *separatorshadow;
  Object *CfgObj;

  char *gTitles[5];
  char *functions[39];
  char *execution[3];
  char *cycleentries[5];

  struct Catalog *catalog;
};

struct KeyAction
{
  BOOL vanilla;
  unsigned int key;
  ULONG qualifier;
  UWORD action;
};

#include "amiga-align.h"

struct te_key
{
  UWORD code;
  ULONG qual;
  UWORD act;
};

#include "default-align.h"

extern struct MUI_CustomClass *widthslider_mcc;
extern struct MUI_CustomClass *speedslider_mcc;
extern struct MUI_CustomClass *text_mcc;

extern const struct te_key *keybindings[];

Object *CreatePrefsGroup(struct InstData_MCP *data);
void ImportKeys(void *, struct InstData_MCP *data);
void ExportKeys(void *, struct InstData_MCP *);
void AddKeyBinding (STRPTR keystring, UWORD action, struct KeyAction *storage);
void ConvertKeyString (STRPTR keystring, UWORD action, struct KeyAction *storage);
void KeyToString(STRPTR buffer, struct KeyAction *ka);
char *FunctionName(UWORD func);
#ifndef __AROS__
BOOL CreateSubClasses(void);
void DeleteSubClasses(void);
#endif

// main class methods
ULONG New(REG(a0, struct IClass *cl), REG(a2, Object *obj), REG(a1, struct opSet *msg));
ULONG Dispose(REG(a0, struct IClass *cl), REG(a2, Object *obj), REG(a1, Msg msg));
ULONG GadgetsToConfig(REG(a0, struct IClass *cl), REG(a2, Object *obj), REG(a1, struct MUIP_Settingsgroup_GadgetsToConfig *msg));
ULONG ConfigToGadgets(REG(a0, struct IClass *cl), REG(a2, Object *obj), REG(a1, struct MUIP_Settingsgroup_ConfigToGadgets *msg));

#endif /* MUI_NLISTVIEWS_priv_MCP_H */
