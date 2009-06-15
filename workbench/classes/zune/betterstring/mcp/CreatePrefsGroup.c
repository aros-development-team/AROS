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

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <exec/libraries.h>
#include <libraries/asl.h>
#include <libraries/mui.h>

#include "BetterString_mcc.h"

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

  static const char infotext[] = "\033bBetterString.mcp " LIB_REV_STRING "\033n (" LIB_DATE ")\n"
                                 "Copyright (c) 1997-2000 Allan Odgaard\n"
                                 LIB_COPYRIGHT "\n\n"
                                 "Distributed under the terms of the LGPL2.\n\n"
                                 "For the latest version, check out:\n"
                                 "http://www.sf.net/projects/bstring-mcc/\n\n";

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

    Child, RectangleObject,
      MUIA_VertWeight, 10,
      End,

    Child, HGroup,

      Child, ColGroup(2),
        Child, RectangleObject, End,
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
            End,
          Child, objs[InactiveText] = PoppenObject,
            MUIA_CycleChain,      TRUE,
            End,
          End,

        Child, TxtLabel(GetStr(MSG_Label_Active)),
        Child, HGroup,
          Child, objs[ActiveBack] = PopimageObject,
            MUIA_Imageadjust_Type,  2,
            MUIA_CycleChain,      TRUE,
            End,
          Child, objs[ActiveText] = PoppenObject,
            MUIA_CycleChain,      TRUE,
            End,
          End,

        Child, RectangleObject, End,
        Child, RectangleObject,
          MUIA_Rectangle_HBar,  TRUE,
          MUIA_VertWeight,      5,
          End,

        Child, TxtLabel(GetStr(MSG_Label_Cursor)),
        Child, objs[Cursor] = PoppenObject,
          MUIA_CycleChain, TRUE,
          End,

        Child, TxtLabel(GetStr(MSG_Label_Marked)),
        Child, HGroup,
          Child, objs[MarkedBack] = PoppenObject,
            MUIA_CycleChain, TRUE,
            End,
          Child, objs[MarkedText] = PoppenObject,
            MUIA_CycleChain, TRUE,
            End,
          End,

        Child, RectangleObject, End,
        Child, RectangleObject,
          MUIA_Rectangle_HBar,  TRUE,
          MUIA_VertWeight,      5,
        End,

        Child, TxtLabel(GetStr(MSG_SelectOnActive)),
        Child, HGroup,
          MUIA_ShortHelp, GetStr(MSG_HELP_SelectOnActive),
          Child, objs[SelectOnActive] = MUI_MakeObject(MUIO_Checkmark, NULL),
          Child, HSpace(0),
        End,

        Child, TxtLabel(GetStr(MSG_SelectPointer)),
        Child, HGroup,
          MUIA_ShortHelp, GetStr(MSG_HELP_SelectPointer),
          Child, objs[SelectPointer] = MUI_MakeObject(MUIO_Checkmark, NULL),
          Child, HSpace(0),
        End,

      End,

      Child, RectangleObject,
        MUIA_Rectangle_VBar,  TRUE,
        MUIA_HorizWeight,    10,
        End,

      Child, ColGroup(2),
        Child, TxtLabel(GetStr(MSG_Label_Fonts)),
        Child, PopaslObject,
          MUIA_Popstring_String, objs[Font] = BetterStringObject, StringFrame,
            MUIA_CycleChain, TRUE,
            End,
          MUIA_Popstring_Button,  MUI_MakeObject(MUIO_PopButton, MUII_PopUp),
          MUIA_Popasl_Type,      ASL_FontRequest,
          End,

        Child, TxtLabel(GetStr(MSG_Label_Frame)),
        Child, objs[Frame] = PopframeObject,
          MUIA_CycleChain, TRUE,
          End,
        End,

      End,

    Child, RectangleObject,
      MUIA_VertWeight, 10,
      End,

    Child, CrawlingObject,
      TextFrame,
      MUIA_FixHeightTxt, "\n\n",
      MUIA_Background,   "m1",

      Child, TextObject,
        MUIA_Text_PreParse, "\033c",
        MUIA_Text_Contents, infotext,
      End,

      Child, TextObject,
        MUIA_Text_PreParse, "\033c",
        MUIA_Text_Contents, infotext,
      End,
    End,

  End;

  if(MUIMasterBase->lib_Version <= 19 && objs[Frame])
    set(objs[Frame], MUIA_Disabled, TRUE);

  return group;
}
