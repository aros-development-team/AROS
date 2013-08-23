/***************************************************************************

 NListviews.mcp - New Listview MUI Custom Class Preferences
 Registered MUI class, Serial Number: 1d51 (0x9d510001 to 0x9d51001F
                                            and 0x9d510101 to 0x9d51013F)

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2013 by NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

#include <string.h>
#include <stdlib.h>

#include <devices/inputevent.h>
#include <libraries/asl.h>
#include <libraries/locale.h>
#include <libraries/commodities.h>
#include <mui/HotkeyString_mcc.h>

#include <clib/alib_protos.h>
#include <proto/commodities.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/console.h>
#include <proto/locale.h>
#include <proto/exec.h>

#include "private.h"
#include "version.h"

#include "locale.h"

#include "muiextra.h"

#define INTUIBASEMIN 39


#define SimpleButtonCycle(name,helpstring) \
  (void *)TextObject, \
    ButtonFrame, \
    MUIA_CycleChain, 1, \
    MUIA_Font, MUIV_Font_Button, \
    MUIA_Text_Contents, name, \
    MUIA_Text_PreParse, "\33c", \
    MUIA_InputMode    , MUIV_InputMode_RelVerify, \
    MUIA_Background   , MUII_ButtonBack, \
    MUIA_ShortHelp, helpstring, \
  End


#define ToggleButtonCycle(name,selected,disable,helpstring) \
  (void *)TextObject, \
    ButtonFrame, \
    MUIA_CycleChain, 1, \
    MUIA_Font, MUIV_Font_Button, \
    MUIA_Text_Contents, name, \
    MUIA_Text_PreParse, "\33c", \
    MUIA_Text_SetMax, TRUE, \
    MUIA_InputMode    , MUIV_InputMode_Toggle, \
    MUIA_Selected     , selected,\
    MUIA_Background   , MUII_ButtonBack, \
    MUIA_Disabled     , disable,\
    MUIA_ShortHelp, helpstring, \
  End

/*
    MUIA_ShowSelState , FALSE,\
*/
struct QualifierDef {
  char *qualstr;
  LONG qualval;
};

/*
static struct QualifierDef QualTab[] =
{
  { "SHIFT",     (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT) },
  { "LSHIFT",     IEQUALIFIER_LSHIFT },
  { "RSHIFT",     IEQUALIFIER_RSHIFT },
  { "CONTROL",    IEQUALIFIER_CONTROL },
  { "ALT",       (IEQUALIFIER_LALT|IEQUALIFIER_RALT) },
  { "LALT",       IEQUALIFIER_LALT },
  { "RALT",       IEQUALIFIER_RALT },
  { "COMMAND",   (IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND) },
  { "LCOMMAND",   IEQUALIFIER_LCOMMAND },
  { "RCOMMAND",   IEQUALIFIER_RCOMMAND },
  { "MIDBUTTON",  IEQUALIFIER_MIDBUTTON },
  { "RBUTTON",    IEQUALIFIER_RBUTTON },
  { "LEFTBUTTON", IEQUALIFIER_LEFTBUTTON },
  {  NULL,        0 }
};
*/

Object *MakeCheck(STRPTR label, STRPTR help, ULONG check)
{
  Object *obj = MUI_MakeObject(MUIO_Checkmark, label);
  if (obj)
  {
    SetAttrs(obj, MUIA_CycleChain, 1,
                  MUIA_Selected, check,
                  MUIA_ShortHelp, help,
                  TAG_DONE);
  }

  return (obj);
}

#define String2(contents,maxlen)\
  (void *)StringObject,\
    StringFrame,\
    MUIA_CycleChain, 1,\
    MUIA_String_MaxLen  , maxlen,\
    MUIA_String_Contents, contents,\
    End

#define LOAD_DATALONG(obj,attr,cfg_attr,defaultval) \
  { \
    LONG *ptrd; \
    if((ptrd = (LONG *) DoMethod(msg->configdata, MUIM_Dataspace_Find, cfg_attr))) \
      set(obj, attr, *ptrd); \
    else \
      set(obj, attr, defaultval); \
  }

#define SAVE_DATALONG(obj,attr,cfg_attr) \
  { \
    LONG ptrd=0; \
    get(obj, attr, &ptrd); \
    DoMethod(msg->configdata, MUIM_Dataspace_Add, &ptrd, sizeof(ptrd), cfg_attr); \
  }

#define LOAD_DATASPEC(obj,attr,cfg_attr,defaultval) \
  { \
    LONG ptrd; \
    if((ptrd = DoMethod(msg->configdata, MUIM_Dataspace_Find, cfg_attr))) \
      set(obj, attr, ptrd); \
    else \
      set(obj, attr, defaultval); \
  }

#define SAVE_DATASPEC(obj,attr,cfg_attr) \
  { \
    LONG ptrd=0; \
    get(obj, attr, &ptrd); \
    if (ptrd) \
      DoMethod(msg->configdata, MUIM_Dataspace_Add, ptrd, strlen((char *)ptrd)+1, cfg_attr); \
  }

#define SAVE_DATASPEC2(obj,attr,cfg_attr) \
  { \
    LONG ptrd=0; \
    get(obj, attr, &ptrd); \
    if (ptrd) \
      DoMethod(msg->configdata, MUIM_Dataspace_Add, ptrd, strlen((char *)ptrd)+1, cfg_attr); \
  }

#define LOAD_DATAFONT(obj,cfg_attr) \
  { \
    LONG ptrd; \
    if((ptrd = DoMethod(msg->configdata, MUIM_Dataspace_Find, cfg_attr))) \
      set(obj, MUIA_String_Contents, ptrd); \
    else \
      set(obj, MUIA_String_Contents, ""); \
  }

#define SAVE_DATAFONT(obj,cfg_attr) \
  { \
    IPTR ptrd=0; \
    get(obj, MUIA_String_Contents, &ptrd); \
    if (ptrd) \
    { \
      LONG dlen = strlen((char *) ptrd) + 1; \
      if (dlen > 2) \
        DoMethod(msg->configdata, MUIM_Dataspace_Add, ptrd, dlen, cfg_attr); \
      else \
        DoMethod(msg->configdata, MUIM_Dataspace_Remove, cfg_attr); \
    } \
  }

/* *********************************************************************** */

static DEFAULT_KEYS_ARRAY

static ULONG keytags[] =
{
  KEYTAG_QUALIFIER_MULTISELECT    ,
  KEYTAG_QUALIFIER_DRAG           ,
  KEYTAG_QUALIFIER_BALANCE        ,
  KEYTAG_COPY_TO_CLIPBOARD        ,
  KEYTAG_DEFAULT_WIDTH_COLUMN     ,
  KEYTAG_DEFAULT_WIDTH_ALL_COLUMNS,
  KEYTAG_DEFAULT_ORDER_COLUMN     ,
  KEYTAG_DEFAULT_ORDER_ALL_COLUMNS,
  KEYTAG_SELECT_TO_TOP            ,
  KEYTAG_SELECT_TO_BOTTOM         ,
  KEYTAG_SELECT_TO_PAGE_UP        ,
  KEYTAG_SELECT_TO_PAGE_DOWN      ,
  KEYTAG_SELECT_UP                ,
  KEYTAG_SELECT_DOWN              ,
  KEYTAG_TOGGLE_ACTIVE            ,
  KEYTAG_QUALIFIER_WHEEL_FAST     ,
  KEYTAG_QUALIFIER_WHEEL_HORIZ    ,
  KEYTAG_QUALIFIER_TITLECLICK2    ,
  0
};



static struct KeyBinding empty_key = { 0, (UWORD)-1, 0 };

/* *********************************************************************** */

/*
static const char *MainTextArray[] =
{
  "If you have problems, try to increase the stack value,",
  "in the icon infos if you launch the program from icon,",
  "stack of CLI if you start it from CLI/Shell,",
  "and if you launch it from some utility read its docs",
  "to see how to increase it.",
  "A stack of 12Kb, 16Kb or 20Kb is most of time a good idea.",
  " ",
  "If you still have problems, try to see if they happen with the Demo prog, then tell me.",
  "\033C",
  "Latest public release can be found on aminet (dev/mui).",
  "All releases are available on",
  "http://www.sourceforge.net/projects/nlist-classes/",
  "\033C",
  "\033r\0333(C) 2001-2013 by NList Open Source Team",
  "\033r\0333(C) 1996-1998 by Gilles Masson",
  "\033r\0333http://www.sourceforge.net/projects/nlist-classes/",
  "\033C",
  "\033cThis new list/listview custom class",
  "\033chandles its own configurable backgrounds,",
  "\033cpens, fonts, scrollbars, keys and qualifiers",
  "\033C",
  "\033cThe way used to handle cursor with multiselection",
  "\033cis not exactly as the listview one !",
  "\033cDrag&Drop is supported, both scrollbars",
  "\033ccan be configured to disappear automatically",
  "\033cwhen not needed because all is visible.",
  "\033cJust try it...",
  "\033C",
  "\033cYou can horizontaly scroll with cursor keys,",
  "\033cor going on the right and left of the list",
  "\033cwhile selecting with the mouse.",
  "\033cTry just clicking on the left/right borders !",
  "\033C",
  "\033cYou can change columns widths just dragging",
  "\033cthe vertical separator between columns titles",
  "\033c(try it using the balancing qualifier too).",
  "\033C",
  "\033cYou can change columns order just dragging",
  "\033cthe column titles.",
  "\033C",
  "\033cThere is builtin copy to clipboard stuff",
  "\033cand classic char selection capability.",
  "\033C",
  "\033cTry the Demo program to test all that...",
  " ",
  " ",
  "\033r\033bGive some feedback about it ! :-)",
  "\033C",
  "**************************************************************************************************",
  NULL
};
*/

// static arrays which we fill up later on
static const char *Pages[6];
static const char *RS_ColWidthDrag[4];
static const char *RS_VSB[4];
static const char *RS_HSB[5];
static const char *RS_Menu[4];
static const char *RS_MultiSelects[3];
static const char *RS_DragTypes[4];
static const char *functions_names[20];

static LONG DeadKeyConvert(struct NListviews_MCP_Data *data,struct KeyBinding *key)
{
  char *text = data->rawtext;
  char keystr[8];
  UWORD qual = key->kb_Qualifier & KBQUAL_MASK;
  UWORD same = key->kb_Qualifier & KBSYM_MASK;
  int posraw;

  text[0] = '\0';

  if (qual)
  { if ((qual & KBQUALIFIER_CAPS) && (same & KBSYM_CAPS))
                                          strlcat(text, (char *)"caps ", sizeof(data->rawtext));
    else if ((qual & KBQUALIFIER_SHIFT) && (same & KBSYM_SHIFT))
    {                                     strlcat(text, (char *)"shift ", sizeof(data->rawtext));
      if (qual & IEQUALIFIER_CAPSLOCK)    strlcat(text, (char *)"capslock ", sizeof(data->rawtext));
    }
    else
    { if (qual & IEQUALIFIER_LSHIFT)      strlcat(text, (char *)"lshift ", sizeof(data->rawtext));
      if (qual & IEQUALIFIER_RSHIFT)      strlcat(text, (char *)"rshift ", sizeof(data->rawtext));
      if (qual & IEQUALIFIER_CAPSLOCK)    strlcat(text, (char *)"capslock ", sizeof(data->rawtext));
    }
    if (qual & IEQUALIFIER_CONTROL)       strlcat(text, (char *)"control ", sizeof(data->rawtext));
    if ((qual & KBQUALIFIER_ALT) && (same & KBSYM_ALT))
                                          strlcat(text, (char *)"alt ", sizeof(data->rawtext));
    else
    { if (qual & IEQUALIFIER_LALT)        strlcat(text, (char *)"lalt ", sizeof(data->rawtext));
      if (qual & IEQUALIFIER_RALT)        strlcat(text, (char *)"ralt ", sizeof(data->rawtext));
    }
    if (qual & IEQUALIFIER_LCOMMAND)      strlcat(text, (char *)"lcommand ", sizeof(data->rawtext));
    if (qual & IEQUALIFIER_RCOMMAND)      strlcat(text, (char *)"rcommand ", sizeof(data->rawtext));
    if (qual & IEQUALIFIER_NUMERICPAD)    strlcat(text, (char *)"numpad ", sizeof(data->rawtext));
  }

  if (!(key->kb_KeyTag & 0x00004000) && (key->kb_Code != (UWORD)~0))
  {
    switch (key->kb_Code & 0x7F)
    {
      case 0x40 : strlcat(text,"space", sizeof(data->rawtext)); break;
      case 0x41 : strlcat(text,"backspace", sizeof(data->rawtext)); break;
      case 0x42 : strlcat(text,"tab", sizeof(data->rawtext)); break;
      case 0x43 : strlcat(text,"enter", sizeof(data->rawtext)); break;
      case 0x44 : strlcat(text,"return", sizeof(data->rawtext)); break;
      case 0x45 : strlcat(text,"esc", sizeof(data->rawtext)); break;
      case 0x46 : strlcat(text,"del", sizeof(data->rawtext)); break;
      case 0x4C : strlcat(text,"up", sizeof(data->rawtext)); break;
      case 0x4D : strlcat(text,"down", sizeof(data->rawtext)); break;
      case 0x4E : strlcat(text,"right", sizeof(data->rawtext)); break;
      case 0x4F : strlcat(text,"left", sizeof(data->rawtext)); break;
      case 0x50 : strlcat(text,"f1", sizeof(data->rawtext)); break;
      case 0x51 : strlcat(text,"f2", sizeof(data->rawtext)); break;
      case 0x52 : strlcat(text,"f3", sizeof(data->rawtext)); break;
      case 0x53 : strlcat(text,"f4", sizeof(data->rawtext)); break;
      case 0x54 : strlcat(text,"f5", sizeof(data->rawtext)); break;
      case 0x55 : strlcat(text,"f6", sizeof(data->rawtext)); break;
      case 0x56 : strlcat(text,"f7", sizeof(data->rawtext)); break;
      case 0x57 : strlcat(text,"f8", sizeof(data->rawtext)); break;
      case 0x58 : strlcat(text,"f9", sizeof(data->rawtext)); break;
      case 0x59 : strlcat(text,"f10", sizeof(data->rawtext)); break;
      case 0x5F : strlcat(text,"help", sizeof(data->rawtext)); break;
      default:
        data->ievent.ie_NextEvent = NULL;
        data->ievent.ie_Class = IECLASS_RAWKEY;
        data->ievent.ie_SubClass = 0;
        data->ievent.ie_Code = key->kb_Code;
        data->ievent.ie_Qualifier = 0;
        data->ievent.ie_position.ie_addr = (APTR) &data->ievent;
        posraw = RawKeyConvert(&data->ievent, keystr, sizeof(keystr), 0L);

        if (posraw > 0)
        {
          keystr[posraw] = '\0';
          strlcat(text, keystr, sizeof(data->rawtext));
        }
      break;
    }
  }

  return ((LONG)strlen(data->rawtext));
}


static LONG NL_SaveKeys(struct NListviews_MCP_Data *data)
{
  LONG pos,ne = 0;
  struct KeyBinding *key;

  get(data->mcp_listkeys, MUIA_NList_Entries, &ne);
  ne++;

  if((data->nlkeys = (struct KeyBinding *)AllocVecShared(ne*sizeof(struct KeyBinding),MEMF_ANY)))
  {
    pos = 0;

    while (pos < ne)
    {
      DoMethod(data->mcp_listkeys,MUIM_NList_GetEntry,pos, &key);
      if (key)
      {
        data->nlkeys[pos] = *key;
        if (data->nlkeys[pos].kb_KeyTag & 0x00004000)
          data->nlkeys[pos].kb_Code = (UWORD)~0;
      }
      else
        break;
      pos++;
    }

    data->nlkeys[pos].kb_KeyTag = 0L;
    data->nlkeys[pos].kb_Code = (UWORD)~0;
    data->nlkeys[pos].kb_Qualifier = 0;
    pos++;
    ne = ((pos * sizeof(struct KeyBinding)) + 3) & 0xFFFFFFFC;

    return (ne);
  }

  return (0);
}


static void NL_LoadKeys(Object *list,struct KeyBinding *keys)
{
  int i = 0;

  set(list, MUIA_NList_Quiet, TRUE);
  DoMethod(list,MUIM_NList_Clear);
  while (keys[i].kb_KeyTag)
  {
    DoMethod(list,MUIM_NList_InsertSingle,&keys[i], MUIV_NList_Insert_Bottom);
    i++;
  }
  set(list, MUIA_NList_Quiet, FALSE);
}


static void NL_UpdateKeys(Object *list,struct KeyBinding *keys)
{
  int i;
  LONG pos,posmax = -1;
  struct KeyBinding *key;
  set(list, MUIA_NList_Quiet, TRUE);
  get(list,MUIA_NList_Entries,&posmax);
  for (i = 0; keys[i].kb_KeyTag; i++)
  {
    for (pos = 0; pos < posmax; pos++)
    {
      DoMethod(list,MUIM_NList_GetEntry, pos, &key);
      if (key && (key->kb_KeyTag == keys[i].kb_KeyTag))
        break;
/*
{
LONG k1 = (LONG) key->kb_KeyTag;
LONG k2 = (LONG) keys[i].kb_KeyTag;
kprintf("%lx|pos=%ld  key=%lx  kt=%lx (== %lx)\n",list,pos,key,k1,k2);
}
else
{
LONG k1 = (LONG) key->kb_KeyTag;
LONG k2 = (LONG) keys[i].kb_KeyTag;
kprintf("%lx|pos=%ld  key=%lx  kt=%lx (!= %lx)\n",list,pos,key,k1,k2);
}
*/
    }
    if (pos >= posmax)
      DoMethod(list,MUIM_NList_InsertSingle,&keys[i], MUIV_NList_Insert_Bottom);
  }
  set(list, MUIA_NList_Quiet, FALSE);
}

HOOKPROTONH(StrObjFunc, LONG, Object *pop, Object *str)
{
  LONG i = 0;

  get(str,MUIA_UserData,&i);
  if (i >= 0)
    set(pop,MUIA_List_Active,i);
  else
    set(pop,MUIA_List_Active,MUIV_List_Active_Off);
  return(TRUE);
}
MakeStaticHook(StrObjHook, StrObjFunc);

HOOKPROTONH(ObjStrFunc, VOID, Object *pop, Object *str)
{
  LONG i = -1;
  get(pop,MUIA_List_Active,&i);
  if (i >= 0)
  {
    set(str,MUIA_UserData,i);
    set(str,MUIA_Text_Contents,functions_names[i]);
  }
  else
  { i = -1;
    set(str,MUIA_UserData,i);
    set(str,MUIA_Text_Contents,"");
  }
}
MakeStaticHook(ObjStrHook, ObjStrFunc);


HOOKPROTONH(WindowFunc, VOID, Object *pop, Object *win)
{
  set(win,MUIA_Window_DefaultObject,pop);
}
MakeStaticHook(WindowHook, WindowFunc);


HOOKPROTONH(TxtFctFunc, VOID, Object *list, long *val)
{
  Object *txtfct = (Object *) val[0];
  struct KeyBinding *key = NULL;
  LONG i = -1;

  get(txtfct,MUIA_UserData,&i);
  DoMethod(list,MUIM_NList_GetEntry,MUIV_NList_GetEntry_Active, &key);
  if(key && (i >= 0))
  {
    key->kb_KeyTag = keytags[i];
    DoMethod(list,MUIM_NList_Redraw,MUIV_NList_Redraw_Active);
    get(list,MUIA_NList_Active,&i);
    set(list,MUIA_NList_Active,i);
  }
}
MakeStaticHook(TxtFctHook, TxtFctFunc);

HOOKPROTONH(AckFunc, VOID, Object *list, long *val)
{
  Object *stringkey = (Object *) val[0];
  struct KeyBinding *key = NULL;
  char *ackstr = NULL;

  get(stringkey,MUIA_String_Contents, &ackstr);
  DoMethod(list,MUIM_NList_GetEntry,MUIV_NList_GetEntry_Active, &key);
  if (ackstr && key)
  {
    IX ix;

    ix.ix_Version = IX_VERSION;
    ParseIX(ackstr,&ix);
    key->kb_Qualifier = (ix.ix_Qualifier & KBQUAL_MASK) | ((ix.ix_QualSame << 12) & KBSYM_MASK);
    key->kb_Code = ix.ix_Code;
    DoMethod(list,MUIM_NList_Redraw,MUIV_NList_Redraw_Active);
  }
}
MakeStaticHook(AckHook, AckFunc);

HOOKPROTONH(ActiveFunc, VOID, Object *list, long *val)
{
  struct NListviews_MCP_Data *data = (struct NListviews_MCP_Data *) (val[0]);
/*  Object *win = NULL;*/
  ULONG active = (ULONG) (val[1]);
  struct KeyBinding *key = NULL;

  if((LONG)active >= 0)
  {
    DoMethod(list,MUIM_NList_GetEntry,MUIV_NList_GetEntry_Active, &key);

    if (key)
    {
      LONG i = 0;

      while ((keytags[i] > 0) && (keytags[i] != key->kb_KeyTag))
        i++;
      if (keytags[i] == key->kb_KeyTag)
      {
        nnset(data->mcp_stringkey,MUIA_HotkeyString_Snoop, FALSE);
        nnset(data->mcp_stringkey,MUIA_Disabled, FALSE);
        nnset(data->mcp_snoopkey,MUIA_Disabled, FALSE);
        nnset(data->mcp_txtfct,MUIA_UserData,i);
        nnset(data->mcp_txtfct,MUIA_Text_Contents,functions_names[i]);

        DeadKeyConvert(data,key);
        nnset(data->mcp_stringkey,MUIA_String_Contents, data->rawtext);
/*
        get(list, MUIA_WindowObject, &win);
        if (win)
          set(win, MUIA_Window_ActiveObject, data->mcp_stringkey);
*/
      }
      else
        key = NULL;
    }
  }
  if (!key)
  {
    nnset(data->mcp_txtfct,MUIA_UserData,-1);
    nnset(data->mcp_txtfct,MUIA_Text_Contents,"");
    nnset(data->mcp_stringkey,MUIA_String_Contents, "");
    nnset(data->mcp_stringkey,MUIA_Disabled, TRUE);
    nnset(data->mcp_snoopkey,MUIA_Disabled, TRUE);
  }
}
MakeStaticHook(ActiveHook, ActiveFunc);


HOOKPROTONHNP(DefaultFunc, VOID, Object *list)
{
  if (list)
    NL_LoadKeys(list,default_keys);
}
MakeStaticHook(DefaultHook, DefaultFunc);

HOOKPROTONHNP(UpdateFunc, VOID, Object *list)
{
  if (list)
    NL_UpdateKeys(list,default_keys);
}
MakeStaticHook(UpdateHook, UpdateFunc);

HOOKPROTONHNP(InsertFunc, VOID, Object *list)
{
  if (list)
  {
    struct KeyBinding *key;
    LONG pos = 0;

    DoMethod(list,MUIM_NList_GetEntry,MUIV_NList_GetEntry_Active, &key);
    if (!key)
    {
      empty_key.kb_KeyTag = keytags[0];
      key = &empty_key;
    }
    set(list,MUIA_NList_Quiet,TRUE);
    DoMethod(list,MUIM_NList_InsertSingle,key, MUIV_NList_Insert_Active);
    get(list,MUIA_NList_InsertPosition,&pos);
    set(list,MUIA_NList_Active,pos);
    set(list,MUIA_NList_Quiet,FALSE);
  }
}
MakeStaticHook(InsertHook, InsertFunc);

HOOKPROTONH(DisplayFunc, VOID, Object *obj, struct NList_DisplayMessage *ndm)
{
  struct KeyBinding *key = (struct KeyBinding *) ndm->entry;
  struct NListviews_MCP_Data *data = NULL;

  get(obj,MUIA_UserData,&data);

  if (key && data)
  {
    LONG i;

    ndm->preparses[0]  = (STRPTR)"\033r";

    DeadKeyConvert(data,key);
    ndm->strings[0] = data->rawtext;

    ndm->strings[1] = (STRPTR)"\033c=";

    i = 0;
    while ((keytags[i] > 0) && (keytags[i] != key->kb_KeyTag))
      i++;
    ndm->strings[2] = (STRPTR)functions_names[i];
  }
  else
  {
    ndm->preparses[0] = (STRPTR)"\033r";
    ndm->strings[0] = (STRPTR)tr(MSG_HOTKEYS_KEY);
    ndm->strings[1] = (STRPTR)"";
    ndm->strings[2] = (STRPTR)tr(MSG_HOTKEYS_ACTION);
  }
}
MakeStaticHook(DisplayHook, DisplayFunc);

HOOKPROTONHNO(ConstructFunc, APTR, struct NList_ConstructMessage *ncm)
{
  struct KeyBinding *key = (struct KeyBinding *) ncm->entry;
  struct KeyBinding *key2 = (struct KeyBinding *) AllocVecShared(sizeof(struct KeyBinding),0L);
  if (key2)

    *key2 = *key;

  return ((APTR) key2);
}
MakeStaticHook(ConstructHook, ConstructFunc);

HOOKPROTONHNO(DestructFunc, VOID, struct NList_DestructMessage *ndm)
{
  struct KeyBinding *key = (struct KeyBinding *) ndm->entry;

  FreeVec((void *) key);
}
MakeStaticHook(DestructHook, DestructFunc);

static IPTR mNL_MCP_New(struct IClass *cl,Object *obj,struct opSet *msg)
{
  struct NListviews_MCP_Data *data;
  APTR group1, group2, group3, group4, group5;
  char *exampleText;
  BOOL mui39;
  BOOL safeNotifies;

  static const char infotext1[] = "\033bNListviews.mcp " LIB_REV_STRING "\033n (" LIB_DATE ")\n"
                                  "Copyright (C) 1996-2001 Gilles Masson\n"
                                  LIB_COPYRIGHT;
  static const char infotext2[] = "\n"
                                  "Distributed under the terms of the LGPL2.\n"
                                  "\n"
                                  "For the latest version, check out:\n"
                                  "http://www.sf.net/projects/nlist-classes/\n"
                                  "\n";

  if(!(obj = (Object *)DoSuperMethodA(cl, obj,(Msg) msg)))
    return(0);

  data = INST_DATA(cl,obj);

  group1 = group2 = group3 = group4 = group5 = NULL;

  data->mcp_group = NULL;
  data->mcp_list1 = NULL;
  data->mcp_list2 = NULL;
  data->mcp_PenTitle = NULL;
  data->mcp_PenList = NULL;
  data->mcp_PenSelect = NULL;
  data->mcp_PenCursor = NULL;
  data->mcp_PenUnselCur = NULL;
  data->mcp_PenInactive = NULL;
  data->mcp_BG_Title = NULL;
  data->mcp_BG_List = NULL;
  data->mcp_BG_Select = NULL;
  data->mcp_BG_Cursor = NULL;
  data->mcp_BG_UnselCur = NULL;
  data->mcp_BG_Inactive = NULL;
  data->mcp_R_Multi = NULL;
  data->mcp_B_MultiMMB = NULL;
  data->mcp_R_Drag = NULL;
  data->mcp_SL_VertInc = NULL;
  data->mcp_R_HSB = NULL;
  data->mcp_R_VSB = NULL;
  data->mcp_B_Smooth = NULL;
  data->mcp_Font = NULL;
  data->mcp_Font_Little = NULL;
  data->mcp_Font_Fixed = NULL;
  data->mcp_ForcePen = NULL;
  data->mcp_ColWidthDrag = NULL;
  data->mcp_PartialCol = NULL;
  data->mcp_List_Select = NULL;
  data->mcp_NList_Menu = NULL;
  data->mcp_PartialChar = NULL;
  data->mcp_SerMouseFix = NULL;
  data->mcp_DragLines = NULL;
  data->mcp_WheelStep = NULL;
  data->mcp_WheelFast = NULL;
  data->mcp_WheelMMB = NULL;
  data->mcp_listkeys = NULL;
  data->mcp_stringkey = NULL;
  data->mcp_snoopkey = NULL;
  data->mcp_insertkey = NULL;
  data->mcp_removekey = NULL;
  data->mcp_defaultkeys = NULL;
  data->mcp_updatekeys = NULL;
  data->mcp_txtfct = NULL;
  data->mcp_popstrfct = NULL;
  data->mcp_poplistfct = NULL;
  data->nlkeys = NULL;

  data->mcp_stringkey = HotkeyStringObject,
                          StringFrame, MUIA_CycleChain, 1,
                          MUIA_HotkeyString_Snoop, FALSE,
                          MUIA_Disabled,TRUE,
                        End;

  if(data->mcp_stringkey == NULL)
  {
    data->mcp_stringkey = StringObject,
                            StringFrame, MUIA_CycleChain, 1,
                          End;
  }

  // create a duplicate of the translated text
  if((exampleText = AllocVecShared((strlen(tr(MSG_EXAMPLE_TEXT))+1)*sizeof(char), MEMF_ANY)) != NULL)
  {
    char *p;
    LONG numLines = 0;

    // copy the text
    strcpy(exampleText, tr(MSG_EXAMPLE_TEXT));

    // count the number of lines
    p = exampleText;
    while((p = strchr(p, '\n')) != NULL)
    {
      numLines++;
      p++;
    }

    // finally split the text into separate lines
    if((data->exampleText = AllocVecShared((numLines+2)*sizeof(char *), MEMF_ANY|MEMF_CLEAR)) != NULL)
    {
      LONG line;

      p = exampleText;
      for(line = 0; line < numLines; line++)
      {
        char *q;

        q = strchr(p, '\n');
        *q++ = '\0';
        data->exampleText[line] = AllocVecShared((strlen(p)+1)*sizeof(char), MEMF_ANY);
        strcpy(data->exampleText[line], p);
        p = q;
      }
    }

    FreeVec(exampleText);
  }

  mui39 = LIB_VERSION_IS_AT_LEAST(MUIMasterBase, 20, 0);

  group1 = GroupObject,

          Child, VGroup,
            GroupFrameT(tr(MSG_GROUP_EXAMPLE)),

            Child, data->mcp_list1 = NListviewObject,
              MUIA_CycleChain, 1,
              MUIA_NList_Title,"\033cNList / NListview",
              MUIA_NList_TitleSeparator, TRUE,
              MUIA_NListview_Vert_ScrollBar, MUIV_NListview_VSB_Default,
              MUIA_NListview_Horiz_ScrollBar, MUIV_NListview_HSB_Default,
              MUIA_NList_DefaultObjectOnClick, TRUE,
              MUIA_NList_SourceArray, data->exampleText/*MainTextArray*/,
            End,
          End,

          Child, GroupObject,
            GroupFrameT(tr(MSG_FONTS)),
            GroupSpacing(2),
            MUIA_Group_Columns, 2,

            MUIA_Weight,(ULONG)30,
            Child, Label(tr(MSG_NORMAL_FONT)),
            Child, data->mcp_Font = PopaslObject,
              MUIA_Popstring_String,  String2(0,80),
              MUIA_Popstring_Button,  PopButton(mui39 == TRUE ? MUII_PopFont : MUII_PopUp),
              MUIA_Popasl_Type,       ASL_FontRequest,
              MUIA_ShortHelp,         tr(MSG_NORMAL_FONT_HELP),
              ASLFO_TitleText,        tr(MSG_NORMAL_FONT_ASL),
            End,

            Child, Label(tr(MSG_SMALL_FONT)),
            Child, data->mcp_Font_Little = PopaslObject,
              MUIA_Popstring_String,  String2(0,80),
              MUIA_Popstring_Button,  PopButton(mui39 == TRUE ? MUII_PopFont : MUII_PopUp),
              MUIA_Popasl_Type,       ASL_FontRequest,
              MUIA_ShortHelp,         tr(MSG_SMALL_FONT_HELP),
              ASLFO_TitleText,        tr(MSG_SMALL_FONT_ASL),
            End,

            Child, Label(tr(MSG_FIXED_FONT)),
            Child, data->mcp_Font_Fixed = PopaslObject,
              MUIA_Popstring_String,  String2(0,80),
              MUIA_Popstring_Button,  PopButton(mui39 == TRUE ? MUII_PopFont : MUII_PopUp),
              MUIA_Popasl_Type,       ASL_FontRequest,
              MUIA_ShortHelp,         tr(MSG_FIXED_FONT_HELP),
              ASLFO_TitleText,        tr(MSG_FIXED_FONT_ASL),
              ASLFO_FixedWidthOnly,   TRUE,
            End,

            Child, Label(tr(MSG_FONT_MARGIN)),
            Child, data->mcp_SL_VertInc = SliderObject,
              MUIA_CycleChain,    1,
              MUIA_Numeric_Min,   0,
              MUIA_Numeric_Max,   9,
              MUIA_Numeric_Value, 1,
              MUIA_ShortHelp,     tr(MSG_FONT_MARGIN_HELP),
            End,

          End,
      End;

  group2 = VGroup,
             Child, VGroup,
              GroupFrameT(tr(MSG_COLORS)),
              GroupSpacing(2),
              MUIA_VertWeight, 85,
              MUIA_Group_Columns, 3,

              Child, RectangleObject,
                MUIA_VertWeight,         0,
                MUIA_Rectangle_HBar,     TRUE,
                MUIA_Rectangle_BarTitle, tr(MSG_TEXTCOLOR),
              End,
              Child, HSpace(0),
              Child, RectangleObject,
                MUIA_VertWeight,         0,
                MUIA_Rectangle_HBar,     TRUE,
                MUIA_Rectangle_BarTitle, tr(MSG_BACKGROUNDCOLOR),
              End,

              Child, data->mcp_PenTitle = PoppenObject,
                MUIA_CycleChain,    1,
                MUIA_Window_Title,  tr(MSG_TITLE_PEN_WIN),
                MUIA_Draggable,     TRUE,
                MUIA_ShortHelp,     tr(MSG_TITLE_PEN_HELP),
              End,
              Child, VCenter(Label(tr(MSG_PBG_TITLE))),
              Child, data->mcp_BG_Title = PopimageObject,
                MUIA_CycleChain,       1,
                MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Background,
                MUIA_Window_Title,     tr(MSG_TITLE_BG_WIN),
                MUIA_Draggable,        TRUE,
                MUIA_ShortHelp,        tr(MSG_TITLE_BG_HELP),
              End,

              Child, data->mcp_PenList = PoppenObject,
                MUIA_CycleChain,   1,
                MUIA_Window_Title, tr(MSG_LIST_PEN_WIN),
                MUIA_Draggable,    TRUE,
                MUIA_ShortHelp,    tr(MSG_LIST_PEN_HELP),
              End,
              Child, VCenter(Label(tr(MSG_PBG_LIST))),
              Child, data->mcp_BG_List = PopimageObject,
                MUIA_CycleChain,        1,
                MUIA_Imageadjust_Type,  MUIV_Imageadjust_Type_Background,
                MUIA_Window_Title,      tr(MSG_LIST_BG_WIN),
                MUIA_Draggable,         TRUE,
                MUIA_ShortHelp,         tr(MSG_LIST_BG_HELP),
              End,

              Child, data->mcp_PenSelect = PoppenObject,
                MUIA_CycleChain,    1,
                MUIA_Window_Title,  tr(MSG_SELECT_PEN_WIN),
                MUIA_Draggable,     TRUE,
                MUIA_ShortHelp,     tr(MSG_SELECT_PEN_HELP),
              End,
              Child, VCenter(Label(tr(MSG_PBG_SELECT))),
              Child, data->mcp_BG_Select = PopimageObject,
                MUIA_CycleChain,        1,
                MUIA_Imageadjust_Type,  MUIV_Imageadjust_Type_Background,
                MUIA_Window_Title,      tr(MSG_SELECT_BG_WIN),
                MUIA_Draggable,         TRUE,
                MUIA_ShortHelp,         tr(MSG_SELECT_BG_HELP),
              End,

              Child, data->mcp_PenCursor = PoppenObject,
                MUIA_CycleChain,    1,
                MUIA_Window_Title,  tr(MSG_CURSOR_PEN_WIN),
                MUIA_Draggable,     TRUE,
                MUIA_ShortHelp,     tr(MSG_CURSOR_PEN_HELP),
              End,
              Child, VCenter(Label(tr(MSG_PBG_CURSOR))),
              Child, data->mcp_BG_Cursor = PopimageObject,
                MUIA_CycleChain,        1,
                MUIA_Imageadjust_Type,  MUIV_Imageadjust_Type_Background,
                MUIA_Window_Title,      tr(MSG_CURSOR_BG_WIN),
                MUIA_Draggable,         TRUE,
                MUIA_ShortHelp,         tr(MSG_CURSOR_BG_HELP),
              End,

              Child, data->mcp_PenUnselCur = PoppenObject,
                MUIA_CycleChain,    1,
                MUIA_Window_Title,  tr(MSG_UNSEL_PEN_WIN),
                MUIA_Draggable,     TRUE,
                MUIA_ShortHelp,     tr(MSG_UNSEL_PEN_HELP),
              End,
              Child, VCenter(Label(tr(MSG_PBG_UNSEL))),
              Child, data->mcp_BG_UnselCur = PopimageObject,
                MUIA_CycleChain,        1,
                MUIA_Imageadjust_Type,  MUIV_Imageadjust_Type_Background,
                MUIA_Window_Title,      tr(MSG_UNSEL_BG_WIN),
                MUIA_Draggable,         TRUE,
                MUIA_ShortHelp,         tr(MSG_UNSEL_BG_HELP),
              End,

              Child, data->mcp_PenInactive = PoppenObject,
                MUIA_CycleChain,    1,
                MUIA_Window_Title,  tr(MSG_INACT_PEN_WIN),
                MUIA_Draggable,     TRUE,
                MUIA_ShortHelp,     tr(MSG_INACT_PEN_HELP),
              End,
              Child, VCenter(Label(tr(MSG_PBG_INACT))),
              Child, data->mcp_BG_Inactive = PopimageObject,
                MUIA_CycleChain,        1,
                MUIA_Imageadjust_Type,  MUIV_Imageadjust_Type_Background,
                MUIA_Window_Title,      tr(MSG_INACT_BG_WIN),
                MUIA_Draggable,         TRUE,
                MUIA_ShortHelp,         tr(MSG_INACT_BG_HELP),
              End,

             End,

             Child, VGroup,
              GroupFrameT(tr(MSG_COLOR_OPTIONS)),
              MUIA_VertWeight, 15,

              Child, VSpace(0),
              Child, HGroup,
                Child, HSpace(0),
                Child, data->mcp_ForcePen = ImageObject,
                  ImageButtonFrame,
                  MUIA_InputMode,      MUIV_InputMode_Toggle,
                  MUIA_Image_Spec,     MUII_CheckMark,
                  MUIA_Image_FreeVert, TRUE,
                  MUIA_Background,     MUII_ButtonBack,
                  MUIA_ShowSelState,   FALSE,
               End,
               Child, Label(tr(MSG_FORCE_SELECT_PEN)),
               MUIA_ShortHelp, tr(MSG_FORCE_SELECT_PEN_HELP),
               Child, HSpace(0),
              End,
              Child, VSpace(0),
             End,
           End;

  RS_VSB[0] = tr(MSG_VSB_ALWAYS);
  RS_VSB[1] = tr(MSG_VSB_AUTO);
  RS_VSB[2] = tr(MSG_VSB_FULLAUTO);
  RS_VSB[3] = NULL;

  RS_HSB[0] = tr(MSG_HSB_ALWAYS);
  RS_HSB[1] = tr(MSG_HSB_AUTO);
  RS_HSB[2] = tr(MSG_HSB_FULLAUTO);
  RS_HSB[3] = tr(MSG_HSB_NONE);
  RS_HSB[4] = NULL;

  #if defined(__amigaos3__) || defined(__amigaos4__)
  if(LIB_VERSION_IS_AT_LEAST(MUIMasterBase, 20, 5824))
  {
    // MUI4 for AmigaOS is safe for V20.5824+
    safeNotifies = TRUE;
  }
  else if(LIB_VERSION_IS_AT_LEAST(MUIMasterBase, 20, 2346) && LIBREV(MUIMasterBase) < 5000)
  {
    // MUI3.9 for AmigaOS is safe for V20.2346+
    safeNotifies = TRUE;
  }
  else
  {
    // MUI 3.8 and older version of MUI 3.9 or MUI4 are definitely unsafe
    safeNotifies = FALSE;
  }
  #else
  // MorphOS and AROS must be considered unsafe unless someone from the
  // MorphOS/AROS team confirms that removing notifies in nested OM_SET
  // calls is safe.
  safeNotifies = FALSE;
  #endif

  group3 =  VGroup,

              Child, HGroup,
                Child, HGroup,
                  GroupFrameT(tr(MSG_SB_HORIZONTAL)),
                  Child, HSpace(0),
                  Child, VGroup,
                    Child, VSpace(0),
                    Child, data->mcp_R_HSB = RadioObject,
                      MUIA_Radio_Entries, RS_HSB,
                      MUIA_Disabled, !safeNotifies,
                    End,
                    MUIA_ShortHelp, tr(MSG_SB_HORIZONTAL_HELP),
                    Child, VSpace(0),
                  End,
                  Child, HSpace(0),
                End,

                Child, HGroup,
                  GroupFrameT(tr(MSG_SB_VERTICAL)),
                  Child, HSpace(0),
                  Child, VGroup,
                    Child, VSpace(0),
                    Child, data->mcp_R_VSB = RadioObject,
                      MUIA_Radio_Entries,RS_VSB,
                      MUIA_Disabled, !safeNotifies,
                    End,
                    MUIA_ShortHelp, tr(MSG_SB_VERTICAL_HELP),
                    Child, VSpace(0),
                  End,
                  Child, HSpace(0),
                End,
                Child, VSpace(0),
              End,

              Child, VGroup,
                GroupFrameT(tr(MSG_SCROLLCONTROL)),
                Child, VSpace(0),

                Child, ColGroup(2),
                  Child, HSpace(0),
                  Child, HGroup,
                    Child, data->mcp_B_Smooth = ImageObject,
                      ImageButtonFrame,
                      MUIA_InputMode,       MUIV_InputMode_Toggle,
                      MUIA_Image_Spec,      MUII_CheckMark,
                      MUIA_Image_FreeVert,  TRUE,
                      MUIA_Background,      MUII_ButtonBack,
                      MUIA_ShowSelState,    FALSE,
                    End,
                    Child, Label(tr(MSG_SMOOTH_SCROLLING)),
                    MUIA_ShortHelp, tr(MSG_SMOOTH_SCROLLING_HELP),
                    Child, HSpace(0),
                  End,

                  Child, HSpace(0),
                  Child, HGroup,
                    Child, data->mcp_WheelMMB = ImageObject,
                      ImageButtonFrame,
                      MUIA_InputMode,       MUIV_InputMode_Toggle,
                      MUIA_Image_Spec,      MUII_CheckMark,
                      MUIA_Image_FreeVert,  TRUE,
                      MUIA_Background,      MUII_ButtonBack,
                      MUIA_ShowSelState,    FALSE,
                    End,
                    Child, Label(tr(MSG_MMB_FASTWHEEL)),
                    MUIA_ShortHelp, tr(MSG_MMB_FASTWHEEL_HELP),
                    Child, HSpace(0),
                  End,

                  Child, Label(tr(MSG_WHEEL_STEP)),
                  Child, data->mcp_WheelStep = SliderObject,
                    MUIA_CycleChain,    1,
                    MUIA_Numeric_Min,   1,
                    MUIA_Numeric_Max,   10,
                    MUIA_Numeric_Value, DEFAULT_WHEELSTEP,
                    MUIA_ShortHelp, tr(MSG_WHEEL_STEP_HELP),
                  End,

                  Child, Label(tr(MSG_WHEEL_FAST)),
                  Child, data->mcp_WheelFast = SliderObject,
                    MUIA_CycleChain,    1,
                    MUIA_Numeric_Min,   1,
                    MUIA_Numeric_Max,   10,
                    MUIA_Numeric_Value, DEFAULT_WHEELFAST,
                    MUIA_ShortHelp, tr(MSG_WHEEL_FAST_HELP),
                  End,
                End,

                Child, VSpace(0),
              End,


            End;


  RS_MultiSelects[0] = tr(MSG_MULTISELECT_QUAL);
  RS_MultiSelects[1] = tr(MSG_MULTISELECT_ALWAYS);
  RS_MultiSelects[2] = NULL;

  RS_DragTypes[0] = tr(MSG_DRAGTYPE_IMMEDIATE);
  RS_DragTypes[1] = tr(MSG_DRAGTYPE_BORDERS);
  RS_DragTypes[2] = tr(MSG_DRAGTYPE_QUALIFIER);
  RS_DragTypes[3] = NULL;

  RS_ColWidthDrag[0] = tr(MSG_COLWIDTHDRAG_TITLE);
  RS_ColWidthDrag[1] = tr(MSG_COLWIDTHDRAG_FULLBAR);
  RS_ColWidthDrag[2] = tr(MSG_COLWIDTHDRAG_VISIBLE);
  RS_ColWidthDrag[3] = NULL;

  RS_Menu[0] = tr(MSG_CMENU_ALWAYS);
  RS_Menu[1] = tr(MSG_CMENU_TOPONLY);
  RS_Menu[2] = tr(MSG_CMENU_NEVER);
  RS_Menu[3] = NULL;

  group4 = VGroup,

              Child, HGroup,
                Child, VGroup,
                  Child, HGroup,
                    GroupFrameT(tr(MSG_MULTISELECT)),

                    Child, HSpace(0),
                    Child, VGroup,
                      Child, VSpace(0),

                      Child, HGroup,
                        Child, data->mcp_R_Multi = RadioObject,
                          MUIA_Radio_Entries, RS_MultiSelects,
                          MUIA_ShortHelp,     tr(MSG_MULTISELECT_HELP),
                        End,
                        Child, HSpace(0),
                      End,

                      Child, RectangleObject,
                        MUIA_VertWeight,     0,
                        MUIA_Rectangle_HBar, TRUE,
                      End,

                      Child, HGroup,
                        Child, data->mcp_List_Select = ImageObject,
                          ImageButtonFrame,
                          MUIA_InputMode,       MUIV_InputMode_Toggle,
                          MUIA_Image_Spec,      MUII_CheckMark,
                          MUIA_Image_FreeVert,  TRUE,
                          MUIA_Background,      MUII_ButtonBack,
                          MUIA_ShowSelState,    FALSE,
                          MUIA_Selected,        TRUE,
                        End,
                        Child, Label(tr(MSG_MULTISEL_MOVEACTIVE)),
                        MUIA_ShortHelp, tr(MSG_MULTISEL_MOVEACTIVE_HELP),
                        Child, HSpace(0),
                      End,

                      Child, HGroup,
                        Child, data->mcp_B_MultiMMB = ImageObject,
                          ImageButtonFrame,
                          MUIA_InputMode,     MUIV_InputMode_Toggle,
                          MUIA_Image_Spec,    MUII_CheckMark,
                          MUIA_Background,    MUII_ButtonBack,
                          MUIA_ShowSelState,  FALSE,
                          MUIA_Image_FreeVert,TRUE,
                        End,
                        Child, Label(tr(MSG_MMB_MULTISEL)),
                        MUIA_ShortHelp, tr(MSG_MMB_MULTISEL_HELP),
                        Child, HSpace(0),
                      End,

                      Child, VSpace(0),
                    End,
                    Child, HSpace(0),
                  End,

                  Child, HGroup,
                    GroupFrameT(tr(MSG_LAYOUT)),
                    Child, HSpace(0),
                    Child, VGroup,
                      Child, VSpace(0),
                        Child, HGroup,
                          Child, data->mcp_PartialCol = ImageObject,
                            ImageButtonFrame,
                            MUIA_InputMode,       MUIV_InputMode_Toggle,
                            MUIA_Image_Spec,      MUII_CheckMark,
                            MUIA_Image_FreeVert,  TRUE,
                            MUIA_Background,      MUII_ButtonBack,
                            MUIA_ShowSelState,    FALSE,
                            MUIA_Selected,        TRUE,
                          End,
                          Child, Label(tr(MSG_PARTIAL_COL_MARK)),
                          MUIA_ShortHelp, tr(MSG_PARTIAL_COL_MARK_HELP),
                          Child, HSpace(0),
                        End,

                        Child, HGroup,
                          Child, data->mcp_PartialChar = ImageObject,
                            ImageButtonFrame,
                            MUIA_InputMode,       MUIV_InputMode_Toggle,
                            MUIA_Image_Spec,      MUII_CheckMark,
                            MUIA_Image_FreeVert,  TRUE,
                            MUIA_Background,      MUII_ButtonBack,
                            MUIA_ShowSelState,    FALSE,
                            MUIA_Selected,        FALSE,
                          End,
                          Child, Label(tr(MSG_PARTIAL_CHARS_DRAWN)),
                          MUIA_ShortHelp, tr(MSG_PARTIAL_CHARS_DRAWN_HELP),
                          Child, HSpace(0),
                        End,

                        Child, HGroup,
                          Child, data->mcp_VerticalCenteredLines = ImageObject,
                            ImageButtonFrame,
                            MUIA_InputMode,       MUIV_InputMode_Toggle,
                            MUIA_Image_Spec,      MUII_CheckMark,
                            MUIA_Image_FreeVert,  TRUE,
                            MUIA_Background,      MUII_ButtonBack,
                            MUIA_ShowSelState,    FALSE,
                            MUIA_Selected,        FALSE,
                          End,
                          Child, Label(tr(MSG_VERT_CENTERED)),
                          MUIA_ShortHelp, tr(MSG_VERT_CENTERED_HELP),
                          Child, HSpace(0),
                        End,

                      Child, VSpace(0),
                    End,
                    Child, HSpace(0),
                  End,

                End,

                Child, VGroup,

                  Child, HGroup,
                    GroupFrameT(tr(MSG_DRAGDROP)),

                    Child, VGroup,
                      Child, VSpace(0),

                      Child, HGroup,
                        Child, HSpace(0),
                        Child, data->mcp_R_Drag = RadioObject,
                          MUIA_Radio_Entries, RS_DragTypes,
                          MUIA_ShortHelp,     tr(MSG_DRAGTYPE_HELP),
                        End,
                        Child, HSpace(0),
                      End,

                      Child, RectangleObject,
                        MUIA_VertWeight,     0,
                        MUIA_Rectangle_HBar, TRUE,
                      End,

                      Child, ColGroup(2),

                        Child, Label(tr(MSG_DRAG_LINES)),
                        Child, data->mcp_DragLines = SliderObject,
                          MUIA_ShortHelp, tr(MSG_DRAG_LINES_HELP),
                          MUIA_CycleChain,    1,
                          MUIA_Numeric_Min,   0,
                          MUIA_Numeric_Max,   20,
                          MUIA_Numeric_Value, DEFAULT_DRAGLINES,
                        End,

                        Child, HSpace(0),
                        Child, HGroup,
                          Child, data->mcp_SerMouseFix = ImageObject,
                            ImageButtonFrame,
                            MUIA_InputMode,       MUIV_InputMode_Toggle,
                            MUIA_Image_Spec,      MUII_CheckMark,
                            MUIA_Image_FreeVert,  TRUE,
                            MUIA_Background,      MUII_ButtonBack,
                            MUIA_ShowSelState,    FALSE,
                          End,
                          Child, Label(tr(MSG_SERMOUSE_FIX)),
                          MUIA_ShortHelp, tr(MSG_SERMOUSE_FIX_HELP),
                          Child, HSpace(0),
                        End,
                      End,

                      Child, VSpace(0),
                    End,

                  End,

                  Child, HGroup,
                    GroupFrameT(tr(MSG_BALANCING_COLS)),
                    Child, HSpace(0),
                    Child, VGroup,
                      Child, VSpace(0),
                      Child, data->mcp_ColWidthDrag = RadioObject,
                        MUIA_Radio_Entries, RS_ColWidthDrag,
                      End,
                      MUIA_ShortHelp, tr(MSG_BALANCING_COLS_HELP),
                      Child, VSpace(0),
                    End,
                    Child, HSpace(0),
                  End,

                End,
              End,

              Child, VGroup,
                GroupFrameT(tr(MSG_GROUP_MISC)),
                MUIA_VertWeight, 10,

                Child, VSpace(0),

                Child, HGroup,
                  Child, HSpace(0),

                  Child, VGroup,
                    Child, HGroup,
                      Child, data->mcp_SelectPointer = ImageObject,
                        ImageButtonFrame,
                        MUIA_InputMode,       MUIV_InputMode_Toggle,
                        MUIA_Image_Spec,      MUII_CheckMark,
                        MUIA_Image_FreeVert,  TRUE,
                        MUIA_Background,      MUII_ButtonBack,
                        MUIA_ShowSelState,    FALSE,
                      End,
                      Child, Label(tr(MSG_SELECT_POINTER)),
                      MUIA_ShortHelp, tr(MSG_SELECT_POINTER_HELP),
                      Child, HSpace(0),
                    End,

                  End,

                  Child, HSpace(0),

                  Child, HGroup,
                    Child, VGroup,
                      Child, VSpace(0),
                      Child, RectangleObject,
                        MUIA_VertWeight,         0,
                        MUIA_Rectangle_HBar,     TRUE,
                        MUIA_Rectangle_BarTitle, tr(MSG_BAR_CONTEXTMENU),
                      End,
                      Child, data->mcp_NList_Menu = RadioObject,
                        MUIA_Radio_Entries,RS_Menu,
                        MUIA_ShortHelp, tr(MSG_DEFAULT_CONTEXT_MENU_HELP),
                      End,
                      Child, VSpace(0),
                    End,
                  End,

                  Child, HSpace(0),
                End,

                Child, VSpace(0),
              End,

            End;

  functions_names[0] = tr(MSG_FUNC_MULTISELQUAL);
  functions_names[1] = tr(MSG_FUNC_DRAGQUAL);
  functions_names[2] = tr(MSG_FUNC_BALANCEQUAL);
  functions_names[3] = tr(MSG_FUNC_COPYCLIP);
  functions_names[4] = tr(MSG_FUNC_DEFCOLWIDTH);
  functions_names[5] = tr(MSG_FUNC_DEFALLCOLWIDTH);
  functions_names[6] = tr(MSG_FUNC_DEFORDERCOL);
  functions_names[7] = tr(MSG_FUNC_DEFALLORDERCOL);
  functions_names[8] = tr(MSG_FUNC_SELECTTOP);
  functions_names[9] = tr(MSG_FUNC_SELECTBOTTOM);
  functions_names[10]= tr(MSG_FUNC_SELECTPAGEUP);
  functions_names[11]= tr(MSG_FUNC_SELECTPAGEDOWN);
  functions_names[12]= tr(MSG_FUNC_SELECTUP);
  functions_names[13]= tr(MSG_FUNC_SELECTDOWN);
  functions_names[14]= tr(MSG_FUNC_TOGGLEACTIVE);
  functions_names[15]= tr(MSG_FUNC_FASTWHEELQUAL);
  functions_names[16]= tr(MSG_FUNC_HORIZWHEELQUAL);
  functions_names[17]= tr(MSG_FUNC_TITLECLICKQUAL);
  functions_names[18]= "";
  functions_names[19]= NULL;

  group5 = GroupObject,
          Child, NListviewObject,
            MUIA_CycleChain, 1,
            MUIA_NListview_Vert_ScrollBar, MUIV_NListview_VSB_Always,
            MUIA_NListview_Horiz_ScrollBar, MUIV_NListview_HSB_None,
            MUIA_NListview_NList, data->mcp_listkeys = NListObject,
              MUIA_NList_Title,TRUE,
              MUIA_NList_DragSortable, TRUE,
              MUIA_NList_Format, "W=100 NOBAR,NOBAR,W=100 NOBAR",
              MUIA_NList_DisplayHook2, &DisplayHook,
              MUIA_NList_ConstructHook2, &ConstructHook,
              MUIA_NList_DestructHook2, &DestructHook,
              MUIA_NList_MinColSortable, 10,
            End,
          End,

          Child, GroupObject,MUIA_Group_Horiz,TRUE,

            Child, GroupObject,MUIA_Group_Horiz,TRUE,
              Child, data->mcp_stringkey,
              Child, data->mcp_snoopkey = ToggleButtonCycle(tr(MSG_SNOOP), FALSE, TRUE, tr(MSG_SNOOP_KEY)),
            End,

            Child, BalanceObject, End,

            Child, GroupObject,MUIA_Group_Horiz,TRUE,
              Child, TextObject, MUIA_Text_Contents, "=", MUIA_Text_SetMax, TRUE, End,
              Child, data->mcp_popstrfct = PopobjectObject,
                MUIA_InputMode, MUIV_InputMode_None,
                MUIA_Popstring_String, data->mcp_txtfct = TextObject,
                  TextFrame,
                  MUIA_Background, MUII_TextBack,
                  MUIA_Text_Contents, "                     ",
                  MUIA_UserData, -1,
                End,
                MUIA_Popstring_Button, PopButton(MUII_PopUp),
                MUIA_Popobject_StrObjHook, &StrObjHook,
                MUIA_Popobject_ObjStrHook, &ObjStrHook,
                MUIA_Popobject_WindowHook, &WindowHook,
                MUIA_Popobject_Object, data->mcp_poplistfct = ListviewObject,
                  MUIA_Listview_List, ListObject,
                    InputListFrame,
                    MUIA_List_SourceArray, functions_names,
                  End,
                End,
              End,
            End,

          End,

          Child, GroupObject,MUIA_Group_Horiz,TRUE,
            Child, data->mcp_insertkey = SimpleButtonCycle(tr(MSG_BUTTON_INSERT), tr(MSG_BUTTON_INSERT_HELP)),
            Child, data->mcp_removekey = SimpleButtonCycle(tr(MSG_BUTTON_REMOVE), tr(MSG_BUTTON_REMOVE_HELP)),
            Child, data->mcp_updatekeys = SimpleButtonCycle(tr(MSG_BUTTON_UPDATEKEYS), tr(MSG_BUTTON_UPDATEKEYS_HELP)),
            Child, data->mcp_defaultkeys = SimpleButtonCycle(tr(MSG_BUTTON_DEFAULTKEYS), tr(MSG_BUTTON_DEFAULTKEYS_HELP)),
          End,

        End;

  Pages[0] = tr(MSG_PAGE_FONTS);
  Pages[1] = tr(MSG_PAGE_COLORS);
  Pages[2] = tr(MSG_PAGE_SCROLLING);
  Pages[3] = tr(MSG_PAGE_OPTIONS);
  Pages[4] = tr(MSG_PAGE_KEYBINDINGS);

  data->mcp_group = VGroup,

    Child, RegisterObject,
      MUIA_Register_Titles, Pages,
      MUIA_Register_Frame,  TRUE,
      Child, group1,
      Child, group2,
      Child, group3,
      Child, group4,
      Child, group5,
    End,

    Child, CrawlingObject,
      TextFrame,
      MUIA_FixHeightTxt, infotext1,
      MUIA_Background,   "m1",

      Child, TextObject,
        MUIA_Text_Copy, FALSE,
        MUIA_Text_PreParse, "\033c",
        MUIA_Text_Contents, infotext1,
      End,

      Child, TextObject,
        MUIA_Text_Copy, FALSE,
        MUIA_Text_PreParse, "\033c",
        MUIA_Text_Contents, infotext2,
      End,

      Child, TextObject,
        MUIA_Text_Copy, FALSE,
        MUIA_Text_PreParse, "\033c",
        MUIA_Text_Contents, infotext1,
      End,
    End,

  End;

  if(!data->mcp_group)
  {
    CoerceMethod(cl, obj, OM_DISPOSE);
    return(0);
  }

  DoMethod(obj, OM_ADDMEMBER, data->mcp_group);

  set(data->mcp_stringkey,MUIA_String_AttachedList,data->mcp_listkeys);
  set(data->mcp_listkeys,MUIA_UserData,data);

  DoMethod(data->mcp_listkeys,MUIM_Notify,MUIA_NList_Active, MUIV_EveryTime,
           data->mcp_listkeys, 3, MUIM_CallHook, &ActiveHook,data,MUIV_TriggerValue);

  DoMethod(data->mcp_listkeys,MUIM_Notify,MUIA_NList_DoubleClick, MUIV_EveryTime,
           MUIV_Notify_Window, 3, MUIM_Set, MUIA_Window_ActiveObject, data->mcp_stringkey);

  DoMethod(data->mcp_poplistfct,MUIM_Notify,MUIA_Listview_DoubleClick,TRUE,
           data->mcp_popstrfct,2,MUIM_Popstring_Close,TRUE);

  DoMethod(data->mcp_txtfct, MUIM_Notify, MUIA_Text_Contents, MUIV_EveryTime,
           data->mcp_listkeys, 3, MUIM_CallHook, &TxtFctHook,data->mcp_txtfct);

  DoMethod(data->mcp_snoopkey, MUIM_Notify, MUIA_Selected, TRUE,
           MUIV_Notify_Window, 3, MUIM_Set, MUIA_Window_ActiveObject, data->mcp_stringkey);

  DoMethod(data->mcp_snoopkey, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
           data->mcp_stringkey, 3, MUIM_Set, MUIA_HotkeyString_Snoop, MUIV_TriggerValue);

  DoMethod(data->mcp_stringkey, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
           data->mcp_snoopkey, 3, MUIM_Set, MUIA_Selected, FALSE);

  DoMethod(data->mcp_stringkey, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
           data->mcp_listkeys, 3, MUIM_CallHook, &AckHook,data->mcp_stringkey);

  DoMethod(data->mcp_insertkey,MUIM_Notify, MUIA_Pressed, FALSE,
           data->mcp_listkeys, 3, MUIM_CallHook, &InsertHook);

  DoMethod(data->mcp_removekey,MUIM_Notify, MUIA_Pressed, FALSE,
           data->mcp_listkeys, 2,MUIM_NList_Remove,MUIV_NList_Remove_Active);

  DoMethod(data->mcp_defaultkeys,MUIM_Notify, MUIA_Pressed, FALSE,
           data->mcp_listkeys, 3, MUIM_CallHook, &DefaultHook);

  DoMethod(data->mcp_updatekeys,MUIM_Notify, MUIA_Pressed, FALSE,
           data->mcp_listkeys, 3, MUIM_CallHook, &UpdateHook);

  // in case we are running for a newer MUI version we can register
  // our mcc gadgets accordingly
  if(MUIMasterBase->lib_Version >= 20)
  {
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_PenTitle,     MUICFG_NList_Pen_Title, 1, tr(MSG_TITLE_PEN_WIN));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_PenList,      MUICFG_NList_Pen_List, 1, tr(MSG_LIST_PEN_WIN));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_PenSelect,    MUICFG_NList_Pen_Select, 1, tr(MSG_SELECT_PEN_WIN));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_PenCursor,    MUICFG_NList_Pen_Cursor, 1, tr(MSG_CURSOR_PEN_WIN));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_PenUnselCur,  MUICFG_NList_Pen_UnselCur, 1, tr(MSG_UNSEL_PEN_WIN));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_PenInactive,  MUICFG_NList_Pen_Inactive, 1, tr(MSG_INACT_PEN_WIN));

    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_BG_Title,     MUICFG_NList_BG_Title, 1, tr(MSG_TITLE_BG_WIN));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_BG_List,      MUICFG_NList_BG_List, 1, tr(MSG_LIST_BG_WIN));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_BG_Select,    MUICFG_NList_BG_Select, 1, tr(MSG_SELECT_BG_WIN));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_BG_Cursor,    MUICFG_NList_BG_Cursor, 1, tr(MSG_CURSOR_BG_WIN));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_BG_UnselCur,  MUICFG_NList_BG_UnselCur, 1, tr(MSG_UNSEL_BG_WIN));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_BG_Inactive,  MUICFG_NList_BG_Inactive, 1, tr(MSG_INACT_BG_WIN));

    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_Font,         MUICFG_NList_Font, 1, tr(MSG_NORMAL_FONT));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_Font_Little,  MUICFG_NList_Font_Little, 1, tr(MSG_SMALL_FONT));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_Font_Fixed,   MUICFG_NList_Font_Fixed, 1, tr(MSG_FIXED_FONT));

    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_SL_VertInc,   MUICFG_NList_VertInc, 1, tr(MSG_FONT_MARGIN));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_R_Drag,       MUICFG_NList_DragType, 1, tr(MSG_DRAGDROP));

    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_R_Multi,      MUICFG_NList_MultiSelect, 1, tr(MSG_MULTISELECT));

    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_R_VSB,        MUICFG_NListview_VSB, 1, tr(MSG_SB_VERTICAL));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_R_HSB,        MUICFG_NListview_HSB, 1, tr(MSG_SB_HORIZONTAL));

    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_B_Smooth,     MUICFG_NList_Smooth, 1, tr(MSG_SMOOTH_SCROLLING));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_ForcePen,     MUICFG_NList_ForcePen, 1, tr(MSG_FORCE_SELECT_PEN));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_ColWidthDrag, MUICFG_NList_ColWidthDrag, 1, tr(MSG_BALANCING_COLS));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_PartialCol,   MUICFG_NList_PartialCol, 1, tr(MSG_PARTIAL_COL_MARK));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_List_Select,  MUICFG_NList_List_Select, 1, tr(MSG_MULTISEL_MOVEACTIVE));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_NList_Menu,   MUICFG_NList_Menu, 1, tr(MSG_BAR_CONTEXTMENU));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_PartialChar,  MUICFG_NList_PartialChar, 1, tr(MSG_PARTIAL_CHARS_DRAWN));

    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_SerMouseFix,  MUICFG_NList_SerMouseFix, 1, tr(MSG_SERMOUSE_FIX));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_listkeys,     MUICFG_NList_Keys, 1, tr(MSG_PAGE_KEYBINDINGS));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_DragLines,    MUICFG_NList_DragLines, 1, tr(MSG_DRAG_LINES));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_VerticalCenteredLines, MUICFG_NList_VCenteredLines, 1, tr(MSG_VERT_CENTERED));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_SelectPointer,MUICFG_NList_SelectPointer, 1, tr(MSG_SELECT_POINTER));

    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_WheelStep,    MUICFG_NList_WheelStep, 1, tr(MSG_WHEEL_STEP));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_WheelFast,    MUICFG_NList_WheelFast, 1, tr(MSG_WHEEL_FAST));
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->mcp_WheelMMB,     MUICFG_NList_WheelMMB, 1, tr(MSG_MMB_FASTWHEEL));
  }

  return ((IPTR)obj);
}

static IPTR mNL_MCP_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
  struct NListviews_MCP_Data *data = INST_DATA(cl, obj);

  if(data->exampleText != NULL)
  {
    LONG i = 0;

    while(data->exampleText[i] != NULL)
    {
      FreeVec(data->exampleText[i]);
      i++;
    }

    FreeVec(data->exampleText);
  }

  return DoSuperMethodA(cl, obj, msg);
}

IPTR mNL_MCP_ConfigToGadgets(struct IClass *cl,Object *obj,struct MUIP_Settingsgroup_ConfigToGadgets *msg)
{
  struct NListviews_MCP_Data *data = INST_DATA(cl, obj);

  D(DBF_STARTUP, "configToGadgets");

  LOAD_DATASPEC(data->mcp_PenTitle,   MUIA_Pendisplay_Spec,  MUICFG_NList_Pen_Title,   DEFAULT_PEN_TITLE);
  LOAD_DATASPEC(data->mcp_PenList,    MUIA_Pendisplay_Spec,  MUICFG_NList_Pen_List,    DEFAULT_PEN_LIST);
  LOAD_DATASPEC(data->mcp_PenSelect,  MUIA_Pendisplay_Spec,  MUICFG_NList_Pen_Select,  DEFAULT_PEN_SELECT);
  LOAD_DATASPEC(data->mcp_PenCursor,  MUIA_Pendisplay_Spec,  MUICFG_NList_Pen_Cursor,  DEFAULT_PEN_CURSOR);
  LOAD_DATASPEC(data->mcp_PenUnselCur,MUIA_Pendisplay_Spec,  MUICFG_NList_Pen_UnselCur,DEFAULT_PEN_UNSELCUR);
  LOAD_DATASPEC(data->mcp_PenInactive,MUIA_Pendisplay_Spec,  MUICFG_NList_Pen_Inactive,DEFAULT_PEN_INACTIVE);

  LOAD_DATASPEC(data->mcp_BG_Title,   MUIA_Imagedisplay_Spec,MUICFG_NList_BG_Title,    DEFAULT_BG_TITLE);
  LOAD_DATASPEC(data->mcp_BG_List,    MUIA_Imagedisplay_Spec,MUICFG_NList_BG_List,     DEFAULT_BG_LIST);
  LOAD_DATASPEC(data->mcp_BG_Select,  MUIA_Imagedisplay_Spec,MUICFG_NList_BG_Select,   DEFAULT_BG_SELECT);
  LOAD_DATASPEC(data->mcp_BG_Cursor,  MUIA_Imagedisplay_Spec,MUICFG_NList_BG_Cursor,   DEFAULT_BG_CURSOR);
  LOAD_DATASPEC(data->mcp_BG_UnselCur,MUIA_Imagedisplay_Spec,MUICFG_NList_BG_UnselCur, DEFAULT_BG_UNSELCUR);
  LOAD_DATASPEC(data->mcp_BG_Inactive,MUIA_Imagedisplay_Spec,MUICFG_NList_BG_Inactive, DEFAULT_BG_INACTIVE);

  LOAD_DATALONG(data->mcp_SL_VertInc, MUIA_Numeric_Value,    MUICFG_NList_VertInc,     DEFAULT_VERT_INC);

  LOAD_DATALONG(data->mcp_B_Smooth,   MUIA_Selected,         MUICFG_NList_Smooth,      DEFAULT_SMOOTHSCROLL);

  LOAD_DATAFONT(data->mcp_Font,       MUICFG_NList_Font);
  LOAD_DATAFONT(data->mcp_Font_Little,MUICFG_NList_Font_Little);
  LOAD_DATAFONT(data->mcp_Font_Fixed, MUICFG_NList_Font_Fixed);

  {
    LONG *ptrd;
    LONG num = DEFAULT_DRAGTYPE;
    if((ptrd = (LONG *) DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NList_DragType)))
    {
      if      (*ptrd == MUIV_NList_DragType_Qualifier)
        num = 2;
      else if (*ptrd == MUIV_NList_DragType_Borders)
        num = 1;
      else
        num = 0;
    }
    set(data->mcp_R_Drag,MUIA_Radio_Active, num);

  }

  {
    LONG *ptrd;
    LONG num = DEFAULT_CWD;
    if((ptrd = (LONG *) DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NList_ColWidthDrag)))
    {
      if      (*ptrd <= 0)
        num = 0;
      else if (*ptrd == 2)
        num = 2;
      else
        num = 1;
    }
    set(data->mcp_ColWidthDrag,MUIA_Radio_Active, num);
  }

  {
    LONG *ptrd;
    LONG num = DEFAULT_MULTISELECT;

    if((ptrd = (LONG *) DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NList_MultiSelect)))
      num = *ptrd;

    if ((num & MUIV_NList_MultiSelect_MMB_On) == MUIV_NList_MultiSelect_MMB_On)
    {
      set(data->mcp_B_MultiMMB, MUIA_Selected, TRUE);
    }
    else
    {
      set(data->mcp_B_MultiMMB, MUIA_Selected, FALSE);
    }
    num &= 0x0007;
    if (num == MUIV_NList_MultiSelect_Always)
      num = 1;
    else
      num = 0;
    set(data->mcp_R_Multi,MUIA_Radio_Active, num);
  }

  {
    LONG *ptrd;
    LONG num = DEFAULT_HSB;

    if((ptrd = (LONG *) DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NListview_HSB)))
      num = *ptrd;

    if ((num < 1) || (num > 4))
      num = DEFAULT_HSB;
    num--;

    set(data->mcp_R_HSB,MUIA_Radio_Active, num);
  }

  {
    LONG *ptrd;
    LONG num = DEFAULT_VSB;

    if((ptrd = (LONG *) DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NListview_VSB)))
      num = *ptrd;

    if ((num < 1) || (num > 3))
      num = 1;
    num--;
    set(data->mcp_R_VSB,MUIA_Radio_Active, num);
  }

  {
    LONG *ptrd;
    LONG num = DEFAULT_FORCEPEN;
    if((ptrd = (LONG *) DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NList_ForcePen)))
    {
      if (*ptrd)
        num = TRUE;
      else
        num = FALSE;
    }
    set(data->mcp_ForcePen, MUIA_Selected, num);
  }

  LOAD_DATALONG(data->mcp_DragLines,    MUIA_Numeric_Value,    MUICFG_NList_DragLines, DEFAULT_DRAGLINES);
  LOAD_DATALONG(data->mcp_VerticalCenteredLines, MUIA_Selected, MUICFG_NList_VCenteredLines, DEFAULT_VCENTERED);
  LOAD_DATALONG(data->mcp_SelectPointer, MUIA_Selected, MUICFG_NList_SelectPointer, DEFAULT_SELECTPOINTER);
  LOAD_DATALONG(data->mcp_WheelStep,    MUIA_Numeric_Value,    MUICFG_NList_WheelStep, DEFAULT_WHEELSTEP);
  LOAD_DATALONG(data->mcp_WheelFast,    MUIA_Numeric_Value,    MUICFG_NList_WheelFast, DEFAULT_WHEELFAST);
  LOAD_DATALONG(data->mcp_WheelMMB,     MUIA_Selected,         MUICFG_NList_WheelMMB,  DEFAULT_WHEELMMB);

  {
    LONG *ptrd;
    LONG num = DEFAULT_SERMOUSEFIX;
    if((ptrd = (LONG *) DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NList_SerMouseFix)))
    {
      if (*ptrd)
        num = TRUE;
      else
        num = FALSE;
    }
    set(data->mcp_SerMouseFix, MUIA_Selected, num);
  }

  {
    LONG *ptrd;
    LONG num = DEFAULT_LIST_SELECT;
    if((ptrd = (LONG *) DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NList_List_Select)))
    {
      if (*ptrd)
        num = TRUE;
      else
        num = FALSE;
    }
    set(data->mcp_List_Select, MUIA_Selected, num);
  }

  {
    LONG *ptrd;
    LONG num = DEFAULT_PARTIALCOL;
    if((ptrd = (LONG *) DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NList_PartialCol)))
    {
      if (*ptrd)
        num = TRUE;
      else
        num = FALSE;
    }
    set(data->mcp_PartialCol, MUIA_Selected, num);
  }

  {
    LONG *ptrd;
    LONG num = DEFAULT_PARTIALCHAR;
    if((ptrd = (LONG *) DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NList_PartialChar)))
    {
      if (*ptrd)
        num = TRUE;
      else
        num = FALSE;
    }
    set(data->mcp_PartialChar, MUIA_Selected, num);
  }

  {
    LONG *ptrd;
    LONG num = DEFAULT_CMENU;
    if((ptrd = (LONG *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NList_Menu)) != NULL)
    {
      if(*ptrd == (LONG)MUIV_NList_ContextMenu_TopOnly)
        num = 1;
      else if(*ptrd == (LONG)MUIV_NList_ContextMenu_Never)
        num = 2;
    }
    set(data->mcp_NList_Menu, MUIA_Radio_Active, num);
  }

  {
    LONG *ptrd;
    struct KeyBinding *keys = default_keys;

    if((ptrd = (LONG *) DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NList_Keys)))
      keys = (struct KeyBinding *) ptrd;

    NL_LoadKeys(data->mcp_listkeys,keys);
  }

  return(0);
}


IPTR mNL_MCP_GadgetsToConfig(struct IClass *cl,Object *obj,struct MUIP_Settingsgroup_GadgetsToConfig *msg)
{
  struct NListviews_MCP_Data *data = INST_DATA(cl, obj);

  D(DBF_STARTUP, "GadgetsToConfig");

  SAVE_DATASPEC(data->mcp_PenTitle,   MUIA_Pendisplay_Spec,   MUICFG_NList_Pen_Title);
  SAVE_DATASPEC(data->mcp_PenList,    MUIA_Pendisplay_Spec,   MUICFG_NList_Pen_List);
  SAVE_DATASPEC(data->mcp_PenSelect,  MUIA_Pendisplay_Spec,   MUICFG_NList_Pen_Select);
  SAVE_DATASPEC(data->mcp_PenCursor,  MUIA_Pendisplay_Spec,   MUICFG_NList_Pen_Cursor);
  SAVE_DATASPEC(data->mcp_PenUnselCur,MUIA_Pendisplay_Spec,   MUICFG_NList_Pen_UnselCur);
  SAVE_DATASPEC(data->mcp_PenInactive,MUIA_Pendisplay_Spec,   MUICFG_NList_Pen_Inactive);

  SAVE_DATASPEC2(data->mcp_BG_Title,   MUIA_Imagedisplay_Spec,MUICFG_NList_BG_Title);
  SAVE_DATASPEC2(data->mcp_BG_List,    MUIA_Imagedisplay_Spec,MUICFG_NList_BG_List);
  SAVE_DATASPEC2(data->mcp_BG_Select,  MUIA_Imagedisplay_Spec,MUICFG_NList_BG_Select);
  SAVE_DATASPEC2(data->mcp_BG_Cursor,  MUIA_Imagedisplay_Spec,MUICFG_NList_BG_Cursor);
  SAVE_DATASPEC2(data->mcp_BG_UnselCur,MUIA_Imagedisplay_Spec,MUICFG_NList_BG_UnselCur);
  SAVE_DATASPEC2(data->mcp_BG_Inactive,MUIA_Imagedisplay_Spec,MUICFG_NList_BG_Inactive);

  SAVE_DATALONG(data->mcp_SL_VertInc, MUIA_Numeric_Value,     MUICFG_NList_VertInc);

  SAVE_DATALONG(data->mcp_B_Smooth,   MUIA_Selected,          MUICFG_NList_Smooth);

  SAVE_DATAFONT(data->mcp_Font,       MUICFG_NList_Font);
  SAVE_DATAFONT(data->mcp_Font_Little,MUICFG_NList_Font_Little);
  SAVE_DATAFONT(data->mcp_Font_Fixed, MUICFG_NList_Font_Fixed);

  {
    LONG ptrd=0,num;
    get(data->mcp_R_Drag, MUIA_Radio_Active, &ptrd);
    if      (ptrd == 2)
      num = MUIV_NList_DragType_Qualifier;
    else if (ptrd == 1)
      num = MUIV_NList_DragType_Borders;
    else
      num = MUIV_NList_DragType_Immediate;
    DoMethod(msg->configdata, MUIM_Dataspace_Add, &num, sizeof(num), MUICFG_NList_DragType);
  }

  {
    LONG ptrd=0,num;
    get(data->mcp_ColWidthDrag, MUIA_Radio_Active, &ptrd);
    num = ptrd;
    DoMethod(msg->configdata, MUIM_Dataspace_Add, &num, sizeof(num), MUICFG_NList_ColWidthDrag);
  }

  {
    LONG ptrd=0,num;
    get(data->mcp_R_Multi, MUIA_Radio_Active, &ptrd);
    if (ptrd == 1)
      num = MUIV_NList_MultiSelect_Always;
    else
      num = MUIV_NList_MultiSelect_Shifted;
    get(data->mcp_B_MultiMMB, MUIA_Selected, &ptrd);
    if (ptrd)
      num |= MUIV_NList_MultiSelect_MMB_On;
    else
      num |= MUIV_NList_MultiSelect_MMB_Off;
    DoMethod(msg->configdata, MUIM_Dataspace_Add, &num, sizeof(num), MUICFG_NList_MultiSelect);
  }

  {
    LONG ptrd=0,num;
    get(data->mcp_R_HSB, MUIA_Radio_Active, &ptrd);
    num = ptrd+1;
    DoMethod(msg->configdata, MUIM_Dataspace_Add, &num, sizeof(num), MUICFG_NListview_HSB);
  }
  {
    LONG ptrd=0,num;
    get(data->mcp_R_VSB, MUIA_Radio_Active, &ptrd);
    num = ptrd+1;
    DoMethod(msg->configdata, MUIM_Dataspace_Add, &num, sizeof(num), MUICFG_NListview_VSB);
  }

  {
    LONG ptrd=0,num;
    get(data->mcp_ForcePen, MUIA_Selected, &ptrd);
    if (ptrd)
      num = TRUE;
    else
      num = FALSE;
    DoMethod(msg->configdata, MUIM_Dataspace_Add, &num, sizeof(num), MUICFG_NList_ForcePen);
  }

  SAVE_DATALONG(data->mcp_DragLines,    MUIA_Numeric_Value,     MUICFG_NList_DragLines);
  SAVE_DATALONG(data->mcp_VerticalCenteredLines,    MUIA_Selected,     MUICFG_NList_VCenteredLines);
  SAVE_DATALONG(data->mcp_SelectPointer,MUIA_Selected,          MUICFG_NList_SelectPointer);
  SAVE_DATALONG(data->mcp_WheelStep,    MUIA_Numeric_Value,     MUICFG_NList_WheelStep);
  SAVE_DATALONG(data->mcp_WheelFast,    MUIA_Numeric_Value,     MUICFG_NList_WheelFast);
  SAVE_DATALONG(data->mcp_WheelMMB,     MUIA_Selected,          MUICFG_NList_WheelMMB);

  {
    LONG ptrd=0,num;
    get(data->mcp_SerMouseFix, MUIA_Selected, &ptrd);
    if (ptrd)
      num = TRUE;
    else
      num = FALSE;
    DoMethod(msg->configdata, MUIM_Dataspace_Add, &num, sizeof(num), MUICFG_NList_SerMouseFix);
  }

  {
    LONG ptrd=0,num;
    get(data->mcp_List_Select, MUIA_Selected, &ptrd);
    if (ptrd)
      num = TRUE;
    else
      num = FALSE;
    DoMethod(msg->configdata, MUIM_Dataspace_Add, &num, sizeof(num), MUICFG_NList_List_Select);
  }

  {
    LONG ptrd=0,num;
    get(data->mcp_PartialCol, MUIA_Selected, &ptrd);
    if (ptrd)
      num = TRUE;
    else
      num = FALSE;
    DoMethod(msg->configdata, MUIM_Dataspace_Add, &num, sizeof(num), MUICFG_NList_PartialCol);
  }

  {
    LONG ptrd=0,num;
    get(data->mcp_PartialChar, MUIA_Selected, &ptrd);
    if (ptrd)
      num = TRUE;
    else
      num = FALSE;
    DoMethod(msg->configdata, MUIM_Dataspace_Add, &num, sizeof(num), MUICFG_NList_PartialChar);
  }

  {
    LONG ptrd=0,num = MUIV_NList_ContextMenu_Always;
    get(data->mcp_NList_Menu, MUIA_Radio_Active, &ptrd);
    if      (ptrd == 1)
      num = MUIV_NList_ContextMenu_TopOnly;
    else if (ptrd == 2)
      num = MUIV_NList_ContextMenu_Never;
    DoMethod(msg->configdata, MUIM_Dataspace_Add, &num, sizeof(num), MUICFG_NList_Menu);
  }

  {
    LONG sk = NL_SaveKeys(data);
    if (sk > 0)
    {
      DoMethod(msg->configdata, MUIM_Dataspace_Add, data->nlkeys, sk, MUICFG_NList_Keys);
      FreeVec((void *) data->nlkeys);
      data->nlkeys = NULL;
    }
  }

  return(0);
}


static IPTR mNL_MCP_Get(struct IClass *cl,Object *obj,Msg msg)
{
  IPTR *store = ((struct opGet *)msg)->opg_Storage;

  switch (((struct opGet *)msg)->opg_AttrID)
  {
    case MUIA_Version:
      *store = LIB_VERSION;
      return(TRUE);
      break;
    case MUIA_Revision:
      *store = LIB_REVISION;
      return(TRUE);
      break;
  }
  return (DoSuperMethodA(cl,obj,msg));
}

DISPATCHER(_DispatcherP)
{
  switch (msg->MethodID)
  {
    case OM_NEW:                             return(mNL_MCP_New(cl,obj,(APTR)msg));
    case OM_DISPOSE:                         return(mNL_MCP_Dispose(cl,obj,(APTR)msg));
    case OM_GET:                             return(mNL_MCP_Get(cl,obj,(APTR)msg));
    case MUIM_Settingsgroup_ConfigToGadgets: return(mNL_MCP_ConfigToGadgets(cl,obj,(APTR)msg));
    case MUIM_Settingsgroup_GadgetsToConfig: return(mNL_MCP_GadgetsToConfig(cl,obj,(APTR)msg));
  }

  return(DoSuperMethodA(cl,obj,msg));
}
