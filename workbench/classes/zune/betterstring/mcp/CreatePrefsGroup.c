/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2013 by BetterString.mcc Open Source Team

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

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <exec/libraries.h>
#include <libraries/asl.h>
#include <libraries/mui.h>

#include <mui/BetterString_mcc.h>

#include "locale.h"
#include "private.h"
#include "muiextra.h"
#include "version.h"

const char *Key4[]  = { "Amiga + z", "Undo ralast deletion" };
const char *Key5[]  = { "Amiga + Z", "Redo last deletion" };
const char *Key6[]  = { "Amiga + q", "Toggle between original and modified buffer" };
const char *Key7[]  = { "Amiga + g", "Toggle case on char" };
const char *Key8[]  = { "Amiga + G", "Toggle case on word" };
const char *Key9[]  = { "Amiga + i", "Increase number" };
const char *Key10[] = { "Amiga + d", "Decrease number" };
const char *Key11[] = { "Amiga + #", "Hex to decimal" };
const char *Key12[] = { "Amiga + $", "Decimal to hex" };
const char *Key13[] = { "Amiga + Tab", "Filenamecompletion (use shift to cycle back)" };

const char *Key14[] = { "Ctrl + crsr",    "Mark" };
const char *Key15[] = { "Ctrl + bs/del",  "Delete to start/end of line" };
const char *Key16[] = { "Shift + crsr",   "Go to start/end of line" };
const char *Key17[] = { "Shift + bs/del", "Delete to start/end of line" };
const char *Key18[] = { "Alt + crsr",     "Go to prev/next word" };
const char *Key19[] = { "Alt + bs/del",   "Delete prev/next word" };

Object *TxtLabel(const char *text)
{
  return TextObject, ImageButtonFrame,
    MUIA_FramePhantomHoriz,    TRUE,
    MUIA_Text_PreParse,      "\33r",
    MUIA_Text_SetVMax,      FALSE,
    MUIA_Text_Contents,      text,
    MUIA_Weight,          0,
    End;
}

Object *TxtLLabel(const char *text)
{
  return TextObject, ImageButtonFrame,
    MUIA_FramePhantomHoriz,    TRUE,
    MUIA_Text_SetVMax,      FALSE,
    MUIA_Text_Contents,      text,
    MUIA_Weight,          0,
    End;
}

HOOKPROTONH(DisplayCode, VOID, STRPTR* place, STRPTR *item)
{
  *place++ = *item++;
  *place++ = (STRPTR)"=";
  *place   = *item;
}
MakeStaticHook(DisplayHook, DisplayCode);

Object *CreatePrefsGroup(struct InstData_MCP *data)
{
  Object **objs = data->Objects;
  Object *group;
  static const char *key01[2];
  static const char *key02[2];
  static const char *key03[2];
  static const char *key04[2];
  static const char *key05[2];
  static const char *key06[2];
  static const char *key07[2];
  static const char *key08[2];
  static const char *key09[2];
  static const char *key10[2];
  static const char *key11[2];
  static const char *key12[2];
  static const char *key13[2];
  static const char *key14[2];
  static const char *key15[2];
  static const char *key16[2];
  static const char *key17[2];
  static const char *key18[2];

  static const char infotext1[] = "\033bBetterString.mcp " LIB_REV_STRING "\033n (" LIB_DATE ")\n"
                                  "Copyright (C) 1997-2000 Allan Odgaard\n"
                                  LIB_COPYRIGHT;
  static const char infotext2[] = "\n"
                                  "Distributed under the terms of the LGPL2.\n"
                                  "\n"
                                  "For the latest version, check out:\n"
                                  "http://www.sf.net/projects/bstring-mcc/\n"
                                  "\n";

  static const char **keyinfo[] =
  {
    key01, key02, key03, key04, key05, key06, key07, key08, key09, key10,
    key11, key12, key13, key14, key15, key16, key17, key18,
    NULL
  };

  key01[0] = GetStr(MSG_Help_Copy_Shortcut);                   key01[1] = GetStr(MSG_Help_Copy);
  key02[0] = GetStr(MSG_Help_Cut_Shortcut);                    key02[1] = GetStr(MSG_Help_Cut);
  key03[0] = GetStr(MSG_Help_Paste_Shortcut);                  key03[1] = GetStr(MSG_Help_Paste);
  key04[0] = GetStr(MSG_Help_Undo_Shortcut);                   key04[1] = GetStr(MSG_Help_Undo);
  key05[0] = GetStr(MSG_Help_Redo_Shortcut);                   key05[1] = GetStr(MSG_Help_Redo);
  key06[0] = GetStr(MSG_Help_ToggleBuffer_Shortcut);           key06[1] = GetStr(MSG_Help_ToggleBuffer);
  key07[0] = GetStr(MSG_Help_ToggleCaseChar_Shortcut);         key07[1] = GetStr(MSG_Help_ToggleCaseChar);
  key08[0] = GetStr(MSG_Help_ToggleCaseWord_Shortcut);         key08[1] = GetStr(MSG_Help_ToggleCaseWord);
  key09[0] = GetStr(MSG_Help_IncreaseNumber_Shortcut);         key09[1] = GetStr(MSG_Help_IncreaseNumber);
  key10[0] = GetStr(MSG_Help_DecreaseNumber_Shortcut);         key10[1] = GetStr(MSG_Help_DecreaseNumber);
  key11[0] = GetStr(MSG_Help_HexToDecimal_Shortcut);           key11[1] = GetStr(MSG_Help_HexToDecimal);
  key12[0] = GetStr(MSG_Help_DecimalToHex_Shortcut);           key12[1] = GetStr(MSG_Help_DecimalToHex);
  key13[0] = GetStr(MSG_Help_FilenameCompletition_Shortcut);   key13[1] = GetStr(MSG_Help_FilenameCompletition);
  key14[0] = GetStr(MSG_Help_Mark_Shortcut);                   key14[1] = GetStr(MSG_Help_Mark);
  key15[0] = GetStr(MSG_Help_GotoToStartEndOfLine_Shortcut);   key15[1] = GetStr(MSG_Help_GotoToStartEndOfLine);
  key16[0] = GetStr(MSG_Help_DeleteToStartEndOfLine_Shortcut); key16[1] = GetStr(MSG_Help_DeleteToStartEndOfLine);
  key17[0] = GetStr(MSG_Help_GotoToPrevNextWord_Shortcut);     key17[1] = GetStr(MSG_Help_GotoToPrevNextWord);
  key18[0] = GetStr(MSG_Help_DeleteToPrevNextWord_Shortcut);   key18[1] = GetStr(MSG_Help_DeleteToPrevNextWord);

  group = VGroup,

    Child, PopobjectObject,
      MUIA_Popstring_String,  BetterStringObject, StringFrame,
        MUIA_String_Contents,  GetStr(MSG_String_TestString),
        MUIA_String_Format,    MUIV_String_Format_Center,
        MUIA_CycleChain,      TRUE,
        End,
      MUIA_Popstring_Button, PopButton(MUII_PopUp),
      MUIA_Popobject_Object, ListviewObject,
        MUIA_Listview_Input,    FALSE,
        MUIA_Listview_List,    ListObject, ReadListFrame,
          MUIA_List_DisplayHook,    &DisplayHook,
          MUIA_List_Format,        ",,",
          MUIA_List_SourceArray,    keyinfo,
          End,
        End,
      End,

    Child, VSpace(4),

    Child, ColGroup(2),
      Child, HSpace(-1),
      Child, HGroup,
        MUIA_Group_SameWidth, TRUE,
        Child, TextObject,
          MUIA_Font,          MUIV_Font_Tiny,
          MUIA_Text_Contents,  GetStr(MSG_Label_Background),
          MUIA_Text_PreParse, "\33c",
          End,
        Child, TextObject,
          MUIA_Font,          MUIV_Font_Tiny,
          MUIA_Text_Contents,  GetStr(MSG_Label_Text),
          MUIA_Text_PreParse, "\33c",
          End,
        End,

      Child, TxtLabel(GetStr(MSG_Label_Inactive)),
      Child, HGroup,
        Child, objs[InactiveBack] = PopimageObject,
          MUIA_Imageadjust_Type,  2,
          MUIA_CycleChain,      TRUE,
          MUIA_FixHeight,       0,
          End,
        Child, objs[InactiveText] = PoppenObject,
          MUIA_CycleChain,      TRUE,
          MUIA_FixHeight,       0,
          End,
        End,

      Child, TxtLabel(GetStr(MSG_Label_Active)),
      Child, HGroup,
        Child, objs[ActiveBack] = PopimageObject,
          MUIA_Imageadjust_Type,  2,
          MUIA_CycleChain,      TRUE,
          MUIA_FixHeight,       0,
          End,
        Child, objs[ActiveText] = PoppenObject,
          MUIA_CycleChain,      TRUE,
          MUIA_FixHeight,       0,
          End,
        End,

      Child, HSpace(-1),
      Child, RectangleObject,
        MUIA_Rectangle_HBar,  TRUE,
        MUIA_VertWeight,      0,
        End,

      Child, TxtLabel(GetStr(MSG_Label_Cursor)),
      Child, objs[Cursor] = PoppenObject,
        MUIA_CycleChain, TRUE,
        MUIA_FixHeight,  0,
        End,

      Child, TxtLabel(GetStr(MSG_Label_Marked)),
      Child, HGroup,
        Child, objs[MarkedBack] = PoppenObject,
          MUIA_CycleChain, TRUE,
          MUIA_FixHeight,  0,
          End,
        Child, objs[MarkedText] = PoppenObject,
          MUIA_CycleChain, TRUE,
          MUIA_FixHeight,  0,
          End,
        End,

      Child, HSpace(-1),
      Child, RectangleObject,
        MUIA_Rectangle_HBar,  TRUE,
        MUIA_VertWeight,      0,
      End,

      Child, HSpace(-1),
      Child, HGroup,
	    MUIA_Weight, 0,
        Child, objs[SelectOnActive] = MUI_MakeObject(MUIO_Checkmark, NULL),
        Child, TxtLLabel(GetStr(MSG_SelectOnActive)),
        Child, HSpace(-1),
      End,

      Child, HSpace(-1),
      Child, HGroup,
	    MUIA_Weight, 0,
        Child, objs[SelectPointer] = MUI_MakeObject(MUIO_Checkmark, NULL),
        Child, TxtLLabel(GetStr(MSG_SelectPointer)),
        Child, HSpace(-1),
      End,

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

  // the inactive background will be the same as for String.mui on MUI4
  if(objs[InactiveBack] != NULL)
    set(objs[InactiveBack], MUIA_Disabled, data->mui4x);

  // the active background will be the same as for String.mui on MUI4
  if(objs[ActiveBack] != NULL)
    set(objs[ActiveBack], MUIA_Disabled, data->mui4x);

  if(objs[SelectOnActive] != NULL)
    set(objs[SelectOnActive], MUIA_ShortHelp, GetStr(MSG_HELP_SelectOnActive));

  if(objs[SelectPointer] != NULL)
    set(objs[SelectPointer], MUIA_ShortHelp, GetStr(MSG_HELP_SelectPointer));

  return group;
}
