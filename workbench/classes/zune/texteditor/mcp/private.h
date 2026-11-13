/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2014 TextEditor.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 $Id$

***************************************************************************/

#ifndef TEXTEDITOR_MCP_PRIV_H
#define TEXTEDITOR_MCP_PRIV_H

#include <mui/TextEditor_mcc.h>

#include "TextEditor_mcp.h"

#include <mcc_common.h>

#include "Debug.h"

// special flagging macros
#define setFlag(mask, flag)             (mask) |= (flag)               // set the flag "flag" in "mask"
#define clearFlag(mask, flag)           (mask) &= ~(flag)              // clear the flag "flag" in "mask"
#define maskFlag(mask, flag)            (mask) &= (flag)               // mask the variable "mask" with flags "flag" bitwise
#define isAnyFlagSet(mask, flag)        (((mask) & (flag)) != 0)       // return TRUE if at least one of the flags is set
#define isFlagSet(mask, flag)           (((mask) & (flag)) == (flag))  // return TRUE if the flag is set
#define isFlagClear(mask, flag)         (((mask) & (flag)) == 0)       // return TRUE if the flag is NOT set

#if defined(__amigaos4__)
#define AllocVecShared(size, flags)  AllocVecTags((size), AVT_Type, MEMF_SHARED, AVT_Lock, FALSE, ((flags)&MEMF_CLEAR) ? AVT_ClearWithValue : TAG_IGNORE, 0, TAG_DONE)
#else
#define AllocVecShared(size, flags)  AllocVec((size), (flags))
#endif

#define VERSION_IS_AT_LEAST(ver, rev, minver, minrev) (((ver) > (minver)) || ((ver) == (minver) && (rev) == (minrev)) || ((ver) == (minver) && (rev) > (minrev)))
#define LIB_VERSION_IS_AT_LEAST(lib, minver, minrev)  VERSION_IS_AT_LEAST(((struct Library *)(lib))->lib_Version, ((struct Library *)(lib))->lib_Revision, minver, minrev)

// if something in our configuration setup (keybindings, etc)
// has changed we can increase the config version so that TextEditor
// will popup a warning about and obsolete configuration.
#define CONFIG_VERSION 4

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
  Object *inactiveColor;
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
  Object *inactiveCursor;
  Object *selectPointer;

  const char *gTitles[5];
  const char *functions[41];
  const char *execution[3];
  const char *cycleentries[5];

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

extern const struct te_key default_keybindings[];

Object *CreatePrefsGroup(struct InstData_MCP *data);
void ImportKeys(struct InstData_MCP *data, void *);
void ExportKeys(struct InstData_MCP *data, void *);
void AddKeyBinding (STRPTR keystring, UWORD action, struct KeyAction *storage);
void ConvertKeyString (STRPTR keystring, UWORD action, struct KeyAction *storage);
void KeyToString(STRPTR buffer, ULONG buffer_len, struct KeyAction *ka);
const char *FunctionName(UWORD func);
BOOL CreateSubClasses(void);
void DeleteSubClasses(void);

// main class methods
IPTR New(REG(a0, struct IClass *cl), REG(a2, Object *obj), REG(a1, struct opSet *msg));
IPTR Dispose(REG(a0, struct IClass *cl), REG(a2, Object *obj), REG(a1, Msg msg));
IPTR GadgetsToConfig(REG(a0, struct IClass *cl), REG(a2, Object *obj), REG(a1, struct MUIP_Settingsgroup_GadgetsToConfig *msg));
IPTR ConfigToGadgets(REG(a0, struct IClass *cl), REG(a2, Object *obj), REG(a1, struct MUIP_Settingsgroup_ConfigToGadgets *msg));

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

#if !defined(__amigaos4__) && !defined(__MORPHOS__) && !defined(__AROS__)
// GetHead.c
struct Node *GetHead(struct List *);
// GetPred.c
struct Node *GetPred(struct Node *);
// GetSucc.c
struct Node *GetSucc(struct Node *);
// GetTail.c
struct Node *GetTail(struct List *);
#endif

#endif /* MUI_NLISTVIEWS_priv_MCP_H */
