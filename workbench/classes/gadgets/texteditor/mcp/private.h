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

 $Id: private.h,v 1.4 2005/04/07 23:47:47 damato Exp $

***************************************************************************/

#ifndef TEXTEDITOR_MCP_PRIV_H
#define TEXTEDITOR_MCP_PRIV_H

#include "TextEditor_mcp.h"

#include <mcc_common.h>
#include <mcc_debug.h>

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
  BOOL  vanilla;
  UWORD key;
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
STRPTR FunctionName(UWORD func);
BOOL CreateSubClasses(void);
void DeleteSubClasses(void);

// main class methods
ULONG New(REG(a0, struct IClass *cl), REG(a2, Object *obj), REG(a1, struct opSet *msg));
ULONG Dispose(REG(a0, struct IClass *cl), REG(a2, Object *obj), REG(a1, Msg msg));
ULONG GadgetsToConfig(REG(a0, struct IClass *cl), REG(a2, Object *obj), REG(a1, struct MUIP_Settingsgroup_GadgetsToConfig *msg));
ULONG ConfigToGadgets(REG(a0, struct IClass *cl), REG(a2, Object *obj), REG(a1, struct MUIP_Settingsgroup_ConfigToGadgets *msg));

#endif /* MUI_NLISTVIEWS_priv_MCP_H */
