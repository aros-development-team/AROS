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

 $Id: CreatePrefsGroup.c,v 1.5 2005/04/11 03:04:38 tactica Exp $

***************************************************************************/

#include <stdio.h>

#include <clib/alib_protos.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include <libraries/mui.h>

#include <MUI/BetterString_mcc.h>
#include <MUI/HotkeyString_mcc.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "private.h"
#include "locale.h"
#ifndef __AROS__
#include "muiextra.h"
#endif
#include "TextEditor_mcc.h"
#include "rev.h"

#include "SDI_hook.h"

#ifdef __AROS__
AROS_HOOKPROTONH(ListDisplayFunc, void, char **, array, struct te_key *, entry)
#else
HOOKPROTONH(ListDisplayFunc, void, char **array, struct te_key *entry)
#endif
{
  HOOK_INIT
  static char buffer[118];
  struct KeyAction ka;

  if(entry)
  {
    ka.key = entry->code;
    
    if(ka.key >= 500)
    {
      ka.vanilla = TRUE;
      ka.key -= 500;
    }
    else
      ka.vanilla = FALSE;
    
    ka.qualifier = entry->qual;
    KeyToString(buffer, &ka);

    *array++ = buffer;
    *array++ = "=";
    *array = FunctionName(entry->act);
  }
  else
  {
    *array++ = GetStr(MSG_LVLabel_Key);
    *array++ = "";
    *array = GetStr(MSG_LVLabel_Action);
  }
  HOOK_EXIT
}
MakeStaticHook(ListDisplayHook, ListDisplayFunc);

#ifdef __AROS__
AROS_HOOKPROTONH(ListConstructFunc, APTR, APTR, pool, struct te_key *, entry)
#else
HOOKPROTONH(ListConstructFunc, APTR, APTR pool, struct te_key *entry)
#endif
{
  HOOK_INIT
  struct te_key *newplace = AllocPooled(pool, sizeof(struct te_key));

  if(newplace)
  {
    CopyMem(entry, newplace, sizeof(struct te_key));
  }

  return(newplace);
  HOOK_EXIT
}
MakeStaticHook(ListConstructHook, ListConstructFunc);

#ifdef __AROS__
AROS_HOOKPROTONH(ListDestructFunc, void, APTR, pool, struct te_key *, entry)
#else
HOOKPROTONH(ListDestructFunc, void, APTR pool, struct te_key *entry)
#endif
{
  HOOK_INIT
  FreePooled(pool, entry, sizeof(struct te_key));
  HOOK_EXIT
}
MakeStaticHook(ListDestructHook, ListDestructFunc);

#ifdef __AROS__
AROS_HOOKPROTONH(Popstring_WindowCode, void, Object *, pop, Object *, win)
#else
HOOKPROTONH(Popstring_WindowCode, void, Object *pop, Object *win)
#endif
{
  HOOK_INIT
  set(win, MUIA_Window_DefaultObject, pop);
  HOOK_EXIT
}
MakeStaticHook(Popstring_WindowHook, Popstring_WindowCode);

#ifdef __AROS__
AROS_HOOKPROTONH(Popstring_OpenCode, BOOL, Object *, pop, Object *, text)
#else
HOOKPROTONH(Popstring_OpenCode, BOOL, Object *pop, Object *text)
#endif
{
  HOOK_INIT
  LONG active;

  get(text, MUIA_UserData, &active);
  set(pop, MUIA_List_Active, active);
  
  return(TRUE);
  HOOK_EXIT
}
MakeStaticHook(Popstring_OpenHook, Popstring_OpenCode);

#ifdef __AROS__
AROS_HOOKPROTONH(Popstring_CloseCode, void, Object *, pop, Object *, text)
#else
HOOKPROTONH(Popstring_CloseCode, void, Object *pop, Object *text)
#endif
{
  HOOK_INIT
  LONG active;

  get(pop, MUIA_List_Active, &active);
  set(text, MUIA_UserData, active);
  HOOK_EXIT
}
MakeStaticHook(Popstring_CloseHook, Popstring_CloseCode);

#ifdef __AROS__
AROS_HOOKPROTONHNO(InsertCode, void, APTR **, array)
#else
HOOKPROTONHNO(InsertCode, void, APTR **array)
#endif
{
  HOOK_INIT
  
  static const struct te_key constentry = {76, 0, 0};
  Object *keylist = (Object *)*array++;
  ULONG entry;

  get(keylist, MUIA_List_Active, &entry);

  if((LONG)entry != MUIV_List_Active_Off)
    nnset(keylist, MUIA_List_Active, MUIV_List_Active_Off);
  else
    entry = 0;

  DoMethod(keylist, MUIM_List_InsertSingle, &constentry, entry);
  set(keylist, MUIA_List_Active, entry);
  
  HOOK_EXIT
}
MakeStaticHook(InsertHook, InsertCode);

#ifdef __AROS__
AROS_HOOKPROTONHNO(SelectCode, void, APTR **, array)
#else
HOOKPROTONHNO(SelectCode, void, APTR **array)
#endif
{
  HOOK_INIT
  struct InstData_MCP *data = (struct InstData_MCP *)*array++;
  Object *keylist = (Object *)*array++;
  struct te_key *entry;

  DoMethod(keylist, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &entry);
  
  if(entry)
  {
    UBYTE buffer[100];
    struct KeyAction ka;
    ULONG result;

    get(data->keyfunctions, MUIA_Popstring_String, &result);
    nnset((Object *)result, MUIA_UserData, entry->act);

    ka.key = entry->code;
    
    if(ka.key >= 500)
    {
      ka.vanilla = TRUE;
      ka.key -= 500;
    } 
    else
      ka.vanilla = FALSE;
    
    ka.qualifier = entry->qual;
    KeyToString(buffer, &ka);
    nnset(data->hotkey, MUIA_String_Contents, buffer);
  }
  HOOK_EXIT
}
MakeStaticHook(SelectHook, SelectCode);

#ifdef __AROS__
AROS_HOOKPROTONHNO(UpdateCode, void, APTR **, array)
#else
HOOKPROTONHNO(UpdateCode, void, APTR **array)
#endif
{
  HOOK_INIT
  
  struct InstData_MCP *data = (struct InstData_MCP *)*array++;
  Object *keylist = (Object *)*array++;
  struct te_key entry;
  ULONG result;
  ULONG active;

  get(keylist, MUIA_List_Active, &active);
  
  if((LONG)active != MUIV_List_Active_Off)
  {
    SetAttrs(keylist, MUIA_List_Quiet,  TRUE,
                MUIA_List_Active, MUIV_List_Active_Off,
                MUIA_NoNotify,    TRUE,
                TAG_DONE);
    DoMethod(keylist, MUIM_List_Remove, active);

    get(data->keyfunctions, MUIA_Popstring_String, &result);
    get((Object *)result, MUIA_UserData, &result);

    entry.act = result;

    get(data->hotkey, MUIA_String_Contents, &result);

    if(result)
    {
        struct KeyAction keyaction;

      ConvertKeyString((STRPTR)result, entry.act, &keyaction);
      entry.code = keyaction.key;
      if(keyaction.vanilla)
      {
        entry.code += 500;
      }
      entry.qual = keyaction.qualifier;
    }

    DoMethod(keylist, MUIM_List_InsertSingle, &entry, active);
    SetAttrs(keylist, MUIA_List_Active, active,
                MUIA_List_Quiet,  FALSE,
                MUIA_NoNotify,    TRUE,
                TAG_DONE);
  }
  HOOK_EXIT
}
MakeStaticHook(UpdateHook, UpdateCode);

static APTR TxtLabel (STRPTR text, ULONG weight)
{

/*    APTR result = TextEditorObject,
        MUIA_Background, MUII_GroupBack,
        MUIA_TextEditor_ReadOnly, TRUE,
        MUIA_TextEditor_Contents, text,
        MUIA_Weight, weight,
        End;
*/
    APTR result = TextObject,
        MUIA_FramePhantomHoriz, TRUE,
        MUIA_Frame, MUIV_Frame_ImageButton,
        MUIA_Text_PreParse, "\33r",
        MUIA_Text_SetVMax, FALSE,
        MUIA_Text_Contents, text,
        MUIA_Weight, weight,
        End;

//    APTR result = MUI_MakeObject(MUIO_Label, text, 0);

  return(result);
}

Object *CreatePrefsGroup(struct InstData_MCP *data)
{
  Object *slider, *slider2, *readview, *button, *group,
         *editor, *keylist, *defaultkeys, *functionname,
         *plist, *popbutton;

  struct NewMenu editpopupdata[] =
  {
    { NM_TITLE, GetStr(MSG_MenuTitle_Edit),    0, 0, 0, (APTR)0 },
    { NM_ITEM,  GetStr(MSG_MenuItem_Cut),       NULL, NM_COMMANDSTRING, 0, (APTR)1 },
    { NM_ITEM,  GetStr(MSG_MenuItem_Copy),     NULL, NM_COMMANDSTRING, 0, (APTR)2 },
    { NM_ITEM,  GetStr(MSG_MenuItem_Paste),   NULL, NM_COMMANDSTRING, 0, (APTR)3 },
    { NM_ITEM,  GetStr(MSG_MenuItem_Delete), NULL, NM_COMMANDSTRING, 0, (APTR)4 },
    { NM_ITEM,  NM_BARLABEL, 0, 0, 0, (APTR)0 },
    { NM_ITEM,  GetStr(MSG_MenuItem_Undo), NULL, NM_COMMANDSTRING, 0, (APTR)5 },
    { NM_ITEM,  GetStr(MSG_MenuItem_Redo), NULL, NM_COMMANDSTRING, 0, (APTR)6 },
    { NM_ITEM,  NM_BARLABEL, 0, 0, 0, (APTR)0 },
    { NM_ITEM,  GetStr(MSG_MenuItem_Bold), NULL, NM_COMMANDSTRING | CHECKIT|MENUTOGGLE, 0, (APTR)7 },
    { NM_ITEM,  GetStr(MSG_MenuItem_Italic), NULL, NM_COMMANDSTRING | CHECKIT|MENUTOGGLE, 0, (APTR)8 },
    { NM_ITEM,  GetStr(MSG_MenuItem_Underline), NULL, NM_COMMANDSTRING | CHECKIT|MENUTOGGLE, 0, (APTR)9 },
    { NM_ITEM,  NM_BARLABEL, 0, 0, 0, (APTR)0 },
    { NM_ITEM,  GetStr(MSG_MenuSubTitle_Alignment), 0, 0, 0, (APTR)0 },
    { NM_SUB,   GetStr(MSG_MenuItem_Left), NULL, NM_COMMANDSTRING | CHECKIT|CHECKED, ~1, (APTR)10 },
    { NM_SUB,   GetStr(MSG_MenuItem_Center), NULL, NM_COMMANDSTRING | CHECKIT, ~2, (APTR)11 },
    { NM_SUB,   GetStr(MSG_MenuItem_Right), NULL, NM_COMMANDSTRING | CHECKIT, ~4, (APTR)12 },

    { NM_ITEM,  GetStr(MSG_MenuSubTitle_Color), 0, 0, 0, (APTR)0 },
    { NM_SUB,   GetStr(MSG_MenuItem_Normal), NULL, NM_COMMANDSTRING | CHECKIT|CHECKED, ~1, (APTR)13 },
    { NM_SUB,   GetStr(MSG_MenuItem_Shine), NULL, NM_COMMANDSTRING | CHECKIT, ~2, (APTR)14 },
    { NM_SUB,   GetStr(MSG_MenuItem_Halfshine), NULL, NM_COMMANDSTRING | CHECKIT, ~4, (APTR)15 },
    { NM_SUB,   GetStr(MSG_MenuItem_Background), NULL, NM_COMMANDSTRING | CHECKIT, ~8, (APTR)16 },
    { NM_SUB,   GetStr(MSG_MenuItem_Halfshadow), NULL, NM_COMMANDSTRING | CHECKIT, ~16, (APTR)17 },
    { NM_SUB,   GetStr(MSG_MenuItem_Shadow), NULL, NM_COMMANDSTRING | CHECKIT, ~32, (APTR)18 },
    { NM_SUB,   GetStr(MSG_MenuItem_Text), NULL, NM_COMMANDSTRING | CHECKIT, ~64, (APTR)19 },
    { NM_SUB,   GetStr(MSG_MenuItem_Fill), NULL, NM_COMMANDSTRING | CHECKIT, ~128, (APTR)20 },
    { NM_SUB,   GetStr(MSG_MenuItem_Mark), NULL, NM_COMMANDSTRING | CHECKIT, ~256, (APTR)21 },

    { NM_END,   NULL, 0, 0, 0, (APTR)0 }
  };

  data->editpopup = MUI_MakeObject(MUIO_MenustripNM, editpopupdata, NULL);

  group = VGroup,
    Child, RegisterGroup(data->gTitles),
      MUIA_CycleChain, TRUE,
      MUIA_Background, MUII_RegisterBack,
      Child, HGroup,
        Child, VGroup,
          Child, VGroup,
            MUIA_VertWeight, 0,
            MUIA_Background, MUII_GroupBack,
            MUIA_Frame, MUIV_Frame_Group,
            MUIA_FrameTitle, GetStr(MSG_GroupTitle_Control),
            Child, RectangleObject,
            End,
            Child, ColGroup(2),
              Child, TxtLabel(GetStr(MSG_Label_UndoLevel), 0),
              Child, data->undosize = SliderObject,
                MUIA_ShortHelp, GetStr(HelpBubble_UndoLevel),
                MUIA_Numeric_Min, 20,
                MUIA_Numeric_Max, 2000,
                MUIA_Numeric_Format, "%ld (± 5)",
              End,
              Child, TxtLabel(GetStr(MSG_Label_TabSize), 0),
              Child, data->tabsize = SliderObject,
                MUIA_Numeric_Min, 2,
                MUIA_Numeric_Max, 12,
                MUIA_Numeric_Format, GetStr(MSG_SliderText_TabSize),
              End,
              Child, TxtLabel(GetStr(MSG_Label_Smooth), 0),
              Child, HGroup,
                MUIA_ShortHelp, GetStr(HelpBubble_Smooth),
                Child, data->smooth = MUI_MakeObject(MUIO_Checkmark, NULL),
                Child, RectangleObject,
                End,
              End,
            End,
            Child, RectangleObject,
            End,
          End,

          Child, ColGroup(4),
//            MUIA_VertWeight, 0,
            MUIA_Group_SameHeight, FALSE,
            MUIA_Background, MUII_GroupBack,
            MUIA_Frame, MUIV_Frame_Group,
            MUIA_FrameTitle, GetStr(MSG_GroupTitle_Design),

            Child, TxtLabel(GetStr(MSG_Label_Frame), 0),
            Child, data->frame = MUI_NewObject("Popframe.mui",
              MUIA_Window_Title, GetStr(MSG_PopWinTitle_Frame),
            End,
            Child, TxtLabel(GetStr(MSG_Label_Background), 0),
            Child, data->background = MUI_NewObject("Popimage.mui",
              MUIA_Window_Title, GetStr(MSG_PopWinTitle_Background),
              MUIA_Imageadjust_Type, 2,
            End,
            Child, TxtLabel(GetStr(MSG_Label_Text), 0),
            Child, data->textcolor = PoppenObject,
              MUIA_Window_Title, GetStr(MSG_PopWinTitle_Text),
            End,
            Child, TxtLabel(GetStr(MSG_Label_Highlight), 0),
            Child, data->highlightcolor = PoppenObject,
              MUIA_Window_Title, GetStr(MSG_PopWinTitle_Highlight),
            End,
          End,

          Child, HGroup,
            MUIA_VertWeight, 60,
            MUIA_Background, MUII_GroupBack,
            MUIA_Frame, MUIV_Frame_Group,
            MUIA_FrameTitle, GetStr(MSG_GroupTitle_Separator),
            Child, TxtLabel(GetStr(MSG_Label_SeparatorShine), 0),
            Child, data->separatorshine = PoppenObject,
              MUIA_Window_Title, GetStr(MSG_PopWinTitle_SeparatorShine),
            End,
            Child, TxtLabel(GetStr(MSG_Label_SeparatorShadow), 0),
            Child, data->separatorshadow = PoppenObject,
              MUIA_Window_Title, GetStr(MSG_PopWinTitle_SeparatorShadow),
            End,
          End,
        End,

        Child, VGroup,
          Child, VGroup,
//            MUIA_VertWeight, 0,
            MUIA_Background, MUII_GroupBack,
            MUIA_Frame, MUIV_Frame_Group,
            MUIA_FrameTitle, GetStr(MSG_GroupTitle_Fonts),
            Child, RectangleObject,
            End,
            Child, ColGroup(2),
              Child, TxtLabel(GetStr(MSG_Label_Normal), 0),
              Child, PopaslObject,
                MUIA_Popstring_String,  data->normalfont = BetterStringObject, StringFrame, End,
                MUIA_Popstring_Button,  MUI_MakeObject(MUIO_PopButton, MUII_PopUp),
                MUIA_Popasl_Type,     ASL_FontRequest,
              End,
              Child, TxtLabel(GetStr(MSG_Label_Fixed), 0),
              Child, PopaslObject,
                MUIA_Popstring_String,  data->fixedfont = BetterStringObject, StringFrame, End,
                MUIA_Popstring_Button,  MUI_MakeObject(MUIO_PopButton, MUII_PopUp),
                MUIA_Popasl_Type,     ASL_FontRequest,
                ASLFO_Flags,        FOF_FIXEDWIDTHONLY,
              End,
            End,
            Child, RectangleObject,
            End,
          End,

          Child, VGroup,
//            MUIA_VertWeight, 0,
            MUIA_Background, MUII_GroupBack,
            MUIA_Frame, MUIV_Frame_Group,
            MUIA_FrameTitle, GetStr(MSG_GroupTitle_Cursor),
            MUIA_Group_Columns, 2,
            Child, TxtLabel(GetStr(MSG_Label_Cursor), 0),
            Child, data->cursorcolor = PoppenObject,
              MUIA_Window_Title, GetStr(MSG_PopWinTitle_Cursor),
            End,
            Child, TxtLabel(GetStr(MSG_Label_Selected), 0),
            Child, data->markedcolor = PoppenObject,
              MUIA_Window_Title, GetStr(MSG_PopWinTitle_Selected),
            End,
            Child, TxtLabel(GetStr(MSG_Label_Width), 0),
            Child, data->cursorwidth = NewObject(widthslider_mcc->mcc_Class, NULL,
              MUIA_Numeric_Min, 1,
              MUIA_Numeric_Max, 6,
              MUIA_Numeric_Format, GetStr(MSG_SliderText_StdWidth),
            TAG_DONE),
            Child, TxtLabel(GetStr(MSG_Label_BlinkSpeed), 0),
            Child, data->blinkspeed = NewObject(speedslider_mcc->mcc_Class, NULL,
              MUIA_Numeric_Min, 0,
              MUIA_Numeric_Max, 20,
              MUIA_Numeric_Format, GetStr(MSG_SliderText_StdSpeed),
            TAG_DONE),
          End,

        End,

      End,

      Child, VGroup,
        Child, HGroup,
          Child, defaultkeys = SimpleButton(GetStr(MSG_Button_DefaultKeys)),
          Child, TxtLabel(GetStr(MSG_Label_BlkQual), 1000),
          Child, data->blockqual = MUI_MakeObject(MUIO_Cycle, NULL, data->cycleentries),
        End,
        Child, data->keybindings = ListviewObject,
          MUIA_CycleChain, TRUE,
          MUIA_Listview_DragType, MUIV_Listview_DragType_Immediate,
          MUIA_Listview_List, keylist = ListObject,
            MUIA_Frame,               MUIV_Frame_InputList,
            MUIA_Background,          MUII_ListBack,
            MUIA_List_Title,          TRUE,
            MUIA_List_ConstructHook,  &ListConstructHook,
            MUIA_List_DestructHook,   &ListDestructHook,
            MUIA_List_DisplayHook,    &ListDisplayHook,
            MUIA_List_DragSortable,   TRUE,
            MUIA_List_Format,         "P=\33r W=100 0, P=\33c 1, 2",
//            MUIA_List_Format, "P=\33r W=100 0, P=\33c NoTB NoBar 1, 2",
//            MUIA_List_SourceArray, keybindings,
//            MUIA_List_TitleClick, TRUE,
//            MUIA_List_DragType, MUIV_List_DragType_Default,
          End,
        End,
        Child, VGroup,
          Child, HGroup,
            Child, data->hotkey = HotkeyStringObject,
              MUIA_CycleChain, TRUE,
              MUIA_Frame, MUIV_Frame_String,
              MUIA_HotkeyString_Snoop, FALSE,
              MUIA_Weight, 500,
            End,
            Child, button = TextObject, ButtonFrame,
              MUIA_CycleChain, TRUE,
              MUIA_Background, MUII_ButtonBack,
              MUIA_Text_Contents, GetStr(MSG_Button_Snoop),
              MUIA_Text_SetMax, TRUE,
              MUIA_InputMode, MUIV_InputMode_Toggle,
            End,

            Child, BalanceObject, End,

            Child, MUI_MakeObject(MUIO_Label, "=", MUIO_Label_Centered),
//            Child, MUI_MakeObject(MUIO_Cycle, NULL, data->functions),
            Child, data->keyfunctions = PopobjectObject,
              MUIA_Weight, 400,
              MUIA_Popstring_String,    functionname = NewObject(text_mcc->mcc_Class, NULL, TextFrame, MUIA_Background, MUII_TextBack, TAG_DONE),
              MUIA_Popstring_Button,    popbutton = MUI_MakeObject(MUIO_PopButton, MUII_PopUp),
              MUIA_Popobject_StrObjHook,  &Popstring_OpenHook,
              MUIA_Popobject_ObjStrHook,  &Popstring_CloseHook,
              MUIA_Popobject_WindowHook,  &Popstring_WindowHook,

              MUIA_Popobject_Object, plist = ListviewObject,
                MUIA_CycleChain, TRUE,
                MUIA_Listview_List, ListObject,
                  InputListFrame,
                  MUIA_List_AutoVisible, TRUE,
                  MUIA_List_SourceArray, data->functions,
                End,
              End,

//              MUIA_Poplist_Array,     data->functions,
            End,

          End,
          Child, HGroup,
            Child, data->insertkey = SimpleButton(GetStr(MSG_Button_Insert)),
            Child, data->deletekey = SimpleButton(GetStr(MSG_Button_Delete)),
          End,
        End,
      End,

      Child, VGroup,

        Child, HGroup,
          GroupSpacing(0),
          Child, readview = TextEditorObject,
            ReadListFrame,
            MUIA_CycleChain, TRUE,
            MUIA_TextEditor_ReadOnly, TRUE,
            MUIA_TextEditor_Contents, GetStr(MSG_HelpTxt_SpellChecker),
          End,
          Child, slider2 = ScrollbarObject,
          End,
        End,

        Child, BalanceObject, End,

        Child, ColGroup(3),
          Child, TxtLabel(GetStr(MSG_Label_LookupCmd), 0),
          Child, data->LookupExeType = CycleObject,
            MUIA_Cycle_Entries, data->execution,
            MUIA_Weight, 0,
          End,
          Child, data->lookupcmd = BetterStringObject, StringFrame, End,
          Child, TxtLabel(GetStr(MSG_Label_SuggestCmd), 0),
          Child, data->SuggestExeType = CycleObject,
            MUIA_Cycle_Entries, data->execution,
            MUIA_Weight, 0,
          End,
          Child, data->suggestcmd = BetterStringObject, StringFrame, MUIA_String_AdvanceOnCR, TRUE, End,
        End,
        Child, ColGroup(2),
          Child, TxtLabel(GetStr(MSG_Label_SpellNType), 1000),
          Child, data->typenspell = MUI_MakeObject(MUIO_Checkmark, NULL),
          Child, TxtLabel(GetStr(MSG_Label_LookupWords), 1000),
          Child, data->CheckWord = MUI_MakeObject(MUIO_Checkmark, NULL),
          MUIA_Weight, 0,
        End,

        Child, RectangleObject,
          MUIA_Weight, 25,
        End,

      End,

      Child, HGroup,
        GroupSpacing(0),
        Child, editor = TextEditorObject,
          MUIA_CycleChain, TRUE,
          MUIA_ContextMenu, data->editpopup,
          MUIA_TextEditor_Contents,
            "\033r\033b" LIB_DATE "\033n\n"
            "\n\33cTextEditor.mcp " LIB_REV_STRING CPU " (" LIB_DATE ")\n"
            "Copyright (C) 1997-2000 Allan Odgaard\n"
            LIB_COPYRIGHT
            "\n\033[s:9]\n"
            "For the latest version, try: \33u\33p[7]http://www.sourceforge.net/projects/texteditor-mcc/\33p[0]\33n\n"
            "\n"
            "This gadget is \33ifree software\33n. You can redistribute it and/or modify it under the terms of "
            "the GNU Lesser General Public License as published by the Free Software Foundation; either version 2.1 "
            "of the License, or (at your option) any later version.\n"
            "\n"
            "\33[s:2]\33c\33u\33b Usage: \33n\n"
            "\33l\n"
            "Besides the many keys you can configure on the \33iKeybindings\33n page, there is also the following you should know:\n"
            "\n"
            "- You can double-click a word to select it. If you hold LMB after a double-click, then it will only mark \33bcomplete\33n words. Triple-clicking has the same effect, but for entire lines.\n"
            "\n"
            "- In the default configuration you can extend your block by holding down any <Shift> key while you press LMB and drag the mouse until you reach the point where you want the block to end.\n"
            "\n"
            "- While you drag to scroll, the farther away from the gadget your mouse pointer is, the faster the contents will scroll.\n"
            "\n"
            "\33[s:2]\33c\33u\33b ARexx: \33n\n"
            "\33l\n"
            "The gadget offers the application using it the chance to extend its ARexx port to also cover this editor gadget. If this is done, you will have the following additional commands available:\n"
            "\n"
            "CLEAR, CUT, COPY, PASTE, ERASE, GOTOLINE (\"/N/A\"), GOTOCOLUMN (\"/N/A\"), CURSOR (\"Up/S,Down/S,Left/S,Right/S\"), LINE (\"/N/A\"), COLUMN (\"/N/A\"), NEXT (\"Word/S,Sentence/S,Paragraph/S,Page/S\"), PREVIOUS (\"Word/S,Sentence/S,Paragraph/S,Page/S\"), POSITION (\"SOF/S,EOF/S,SOL/S,EOL/S,SOW/S,EOW/S,SOV/S,EOV/S\"), SETBOOKMARK (\"/N/A\"), GOTOBOOKMARK (\"/N/A\"), TEXT (\"/F\"), UNDO, REDO, GETLINE, GETCURSOR (\"Line/S,Column/S\"), MARK(\"On/S,Off/S\"), DELETE, BACKSPACE, KILLLINE, TOUPPER, TOLOWER.\n"
            "\n"
            "Please refer to the TextEditor.mcc documentation for an explanation on how to use these commands.\n"
            "\n\33[s:10]\n\33cThis instance has a context sensitive menu, from where you can play with text styles, colors, alignments etc.",
        End,
        Child, slider = ScrollbarObject,
        End,
      End,
    End,
  End;

  if(group)
  {
    set(readview, MUIA_TextEditor_Slider, slider2);
    set(editor, MUIA_TextEditor_Slider, slider);

    set(data->blockqual, MUIA_ShortHelp, GetStr(HelpBubble_BlockQual));
    set(data->typenspell, MUIA_ShortHelp, GetStr(HelpBubble_TypeNSpell));
    set(data->CheckWord, MUIA_ShortHelp, GetStr(HelpBubble_CheckWord));

    set(data->hotkey, MUIA_String_AttachedList, data->keybindings);
    set(popbutton, MUIA_CycleChain, TRUE);

    DoMethod(data->lookupcmd, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, MUIV_Notify_Window, 3, MUIM_Set, MUIA_Window_ActiveObject, data->suggestcmd);

    DoMethod(button, MUIM_Notify, MUIA_Selected, TRUE, MUIV_Notify_Window, 3, MUIM_Set, MUIA_Window_ActiveObject, data->hotkey);
//    DoMethod(button, MUIM_Notify, MUIA_Selected, FALSE, data->hotkey, 3, MUIM_Set, MUIA_String_Acknowledge, TRUE);
    DoMethod(button, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, data->hotkey, 3, MUIM_Set, MUIA_HotkeyString_Snoop, MUIV_TriggerValue);
    DoMethod(data->hotkey, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, button, 3, MUIM_Set, MUIA_Selected, FALSE);

    DoMethod(plist, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, data->keyfunctions, 2, MUIM_Popstring_Close, TRUE);

    DoMethod(defaultkeys, MUIM_Notify,
        MUIA_Pressed, FALSE,
        keylist, 1, MUIM_List_Clear);
    DoMethod(defaultkeys, MUIM_Notify,
        MUIA_Pressed, FALSE,
        keylist, 4, MUIM_List_Insert, keybindings, -1, MUIV_List_Insert_Bottom);

    DoMethod(data->insertkey, MUIM_Notify,
        MUIA_Pressed, FALSE,
        MUIV_Notify_Self, 4, MUIM_CallHook, &InsertHook, data, keylist);

    DoMethod(keylist, MUIM_Notify,
        MUIA_List_Active, MUIV_EveryTime,
        MUIV_Notify_Self, 8, MUIM_MultiSet, MUIA_Disabled, FALSE,
        data->hotkey, button, data->keyfunctions, data->deletekey, NULL);
    DoMethod(keylist, MUIM_Notify,
        MUIA_List_Active, MUIV_List_Active_Off,
        MUIV_Notify_Self, 8, MUIM_MultiSet, MUIA_Disabled, TRUE,
        data->hotkey, button, data->keyfunctions, data->deletekey, NULL);

    DoMethod(keylist, MUIM_Notify,
        MUIA_List_Active, MUIV_EveryTime,
        MUIV_Notify_Self, 4, MUIM_CallHook, &SelectHook, data, keylist);

    DoMethod(functionname, MUIM_Notify,
        MUIA_UserData, MUIV_EveryTime,
        MUIV_Notify_Self, 4, MUIM_CallHook, &UpdateHook, data, keylist);

    DoMethod(data->hotkey, MUIM_Notify,
        MUIA_String_Acknowledge, MUIV_EveryTime,
        MUIV_Notify_Self, 4, MUIM_CallHook, &UpdateHook, data, keylist);

    DoMethod(data->deletekey, MUIM_Notify,
        MUIA_Pressed, FALSE,
        keylist, 2, MUIM_List_Remove, MUIV_List_Remove_Active);


    DoMethod(data->hotkey, MUIM_MultiSet,
        MUIA_Disabled, TRUE,
        data->hotkey, button, data->keyfunctions, data->deletekey, NULL);

    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,1), MUIM_Notify,
        MUIA_Menuitem_Trigger, MUIV_EveryTime,
        editor, 2, MUIM_TextEditor_ARexxCmd, "Cut");
    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,2), MUIM_Notify,
        MUIA_Menuitem_Trigger, MUIV_EveryTime,
        editor, 2, MUIM_TextEditor_ARexxCmd, "Copy");
    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,3), MUIM_Notify,
        MUIA_Menuitem_Trigger, MUIV_EveryTime,
        editor, 2, MUIM_TextEditor_ARexxCmd, "Paste");
    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,4), MUIM_Notify,
        MUIA_Menuitem_Trigger, MUIV_EveryTime,
        editor, 2, MUIM_TextEditor_ARexxCmd, "Erase");
    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,5), MUIM_Notify,
        MUIA_Menuitem_Trigger, MUIV_EveryTime,
        editor, 2, MUIM_TextEditor_ARexxCmd, "Undo");
    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,6), MUIM_Notify,
        MUIA_Menuitem_Trigger, MUIV_EveryTime,
        editor, 2, MUIM_TextEditor_ARexxCmd, "Redo");

    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,7), MUIM_Notify,
        MUIA_Menuitem_Checked, MUIV_EveryTime,
        editor, 3, MUIM_Set, MUIA_TextEditor_StyleBold, MUIV_TriggerValue);
    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,8), MUIM_Notify,
        MUIA_Menuitem_Checked, MUIV_EveryTime,
        editor, 3, MUIM_Set, MUIA_TextEditor_StyleItalic, MUIV_TriggerValue);
    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,9), MUIM_Notify,
        MUIA_Menuitem_Checked, MUIV_EveryTime,
        editor, 3, MUIM_Set, MUIA_TextEditor_StyleUnderline, MUIV_TriggerValue);

    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,10), MUIM_Notify,
        MUIA_Menuitem_Checked, TRUE,
        editor, 3, MUIM_Set, MUIA_TextEditor_Flow, MUIV_TextEditor_Flow_Left);
    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,11), MUIM_Notify,
        MUIA_Menuitem_Checked, TRUE,
        editor, 3, MUIM_Set, MUIA_TextEditor_Flow, MUIV_TextEditor_Flow_Center);
    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,12), MUIM_Notify,
        MUIA_Menuitem_Checked, TRUE,
        editor, 3, MUIM_Set, MUIA_TextEditor_Flow, MUIV_TextEditor_Flow_Right);

    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,13), MUIM_Notify, MUIA_Menuitem_Checked, TRUE, editor, 3, MUIM_Set, MUIA_TextEditor_Pen, 0);
    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,14), MUIM_Notify, MUIA_Menuitem_Checked, TRUE, editor, 3, MUIM_Set, MUIA_TextEditor_Pen, 1);
    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,15), MUIM_Notify, MUIA_Menuitem_Checked, TRUE, editor, 3, MUIM_Set, MUIA_TextEditor_Pen, 2);
    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,16), MUIM_Notify, MUIA_Menuitem_Checked, TRUE, editor, 3, MUIM_Set, MUIA_TextEditor_Pen, 3);
    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,17), MUIM_Notify, MUIA_Menuitem_Checked, TRUE, editor, 3, MUIM_Set, MUIA_TextEditor_Pen, 4);
    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,18), MUIM_Notify, MUIA_Menuitem_Checked, TRUE, editor, 3, MUIM_Set, MUIA_TextEditor_Pen, 5);
    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,19), MUIM_Notify, MUIA_Menuitem_Checked, TRUE, editor, 3, MUIM_Set, MUIA_TextEditor_Pen, 6);
    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,20), MUIM_Notify, MUIA_Menuitem_Checked, TRUE, editor, 3, MUIM_Set, MUIA_TextEditor_Pen, 7);
    DoMethod((Object *)DoMethod(data->editpopup,MUIM_FindUData,21), MUIM_Notify, MUIA_Menuitem_Checked, TRUE, editor, 3, MUIM_Set, MUIA_TextEditor_Pen, 8);

    DoMethod(editor, MUIM_Notify,
        MUIA_TextEditor_StyleBold, MUIV_EveryTime,
        (Object *)DoMethod(data->editpopup,MUIM_FindUData,7), 3, MUIM_Set, MUIA_Menuitem_Checked, MUIV_TriggerValue);
    DoMethod(editor, MUIM_Notify,
        MUIA_TextEditor_StyleItalic, MUIV_EveryTime,
        (Object *)DoMethod(data->editpopup,MUIM_FindUData,8), 3, MUIM_Set, MUIA_Menuitem_Checked, MUIV_TriggerValue);
    DoMethod(editor, MUIM_Notify,
        MUIA_TextEditor_StyleUnderline, MUIV_EveryTime,
        (Object *)DoMethod(data->editpopup,MUIM_FindUData,9), 3, MUIM_Set, MUIA_Menuitem_Checked, MUIV_TriggerValue);

    DoMethod(editor, MUIM_Notify,
        MUIA_TextEditor_Flow, MUIV_TextEditor_Flow_Left,
        (Object *)DoMethod(data->editpopup,MUIM_FindUData,10), 3, MUIM_Set, MUIA_Menuitem_Checked, MUIV_TriggerValue);
    DoMethod(editor, MUIM_Notify,
        MUIA_TextEditor_Flow, MUIV_TextEditor_Flow_Center,
        (Object *)DoMethod(data->editpopup,MUIM_FindUData,11), 3, MUIM_Set, MUIA_Menuitem_Checked, MUIV_TriggerValue);
    DoMethod(editor, MUIM_Notify,
        MUIA_TextEditor_Flow, MUIV_TextEditor_Flow_Right,
        (Object *)DoMethod(data->editpopup,MUIM_FindUData,12), 3, MUIM_Set, MUIA_Menuitem_Checked, MUIV_TriggerValue);

    DoMethod(editor, MUIM_Notify, MUIA_TextEditor_Pen, 0, (Object *)DoMethod(data->editpopup,MUIM_FindUData,13), 3, MUIM_Set, MUIA_Menuitem_Checked, MUIV_TriggerValue);
    DoMethod(editor, MUIM_Notify, MUIA_TextEditor_Pen, 1, (Object *)DoMethod(data->editpopup,MUIM_FindUData,14), 3, MUIM_Set, MUIA_Menuitem_Checked, MUIV_TriggerValue);
    DoMethod(editor, MUIM_Notify, MUIA_TextEditor_Pen, 2, (Object *)DoMethod(data->editpopup,MUIM_FindUData,15), 3, MUIM_Set, MUIA_Menuitem_Checked, MUIV_TriggerValue);
    DoMethod(editor, MUIM_Notify, MUIA_TextEditor_Pen, 3, (Object *)DoMethod(data->editpopup,MUIM_FindUData,16), 3, MUIM_Set, MUIA_Menuitem_Checked, MUIV_TriggerValue);
    DoMethod(editor, MUIM_Notify, MUIA_TextEditor_Pen, 4, (Object *)DoMethod(data->editpopup,MUIM_FindUData,17), 3, MUIM_Set, MUIA_Menuitem_Checked, MUIV_TriggerValue);
    DoMethod(editor, MUIM_Notify, MUIA_TextEditor_Pen, 5, (Object *)DoMethod(data->editpopup,MUIM_FindUData,18), 3, MUIM_Set, MUIA_Menuitem_Checked, MUIV_TriggerValue);
    DoMethod(editor, MUIM_Notify, MUIA_TextEditor_Pen, 6, (Object *)DoMethod(data->editpopup,MUIM_FindUData,19), 3, MUIM_Set, MUIA_Menuitem_Checked, MUIV_TriggerValue);
    DoMethod(editor, MUIM_Notify, MUIA_TextEditor_Pen, 7, (Object *)DoMethod(data->editpopup,MUIM_FindUData,20), 3, MUIM_Set, MUIA_Menuitem_Checked, MUIV_TriggerValue);
    DoMethod(editor, MUIM_Notify, MUIA_TextEditor_Pen, 8, (Object *)DoMethod(data->editpopup,MUIM_FindUData,21), 3, MUIM_Set, MUIA_Menuitem_Checked, MUIV_TriggerValue);

    DoMethod(editor, MUIM_Notify, MUIA_TextEditor_AreaMarked, MUIV_EveryTime, MUIV_Notify_Self, 7, MUIM_MultiSet, MUIA_Menuitem_Enabled, MUIV_TriggerValue, (Object *)DoMethod(data->editpopup,MUIM_FindUData,1), (Object *)DoMethod(data->editpopup,MUIM_FindUData,2), (Object *)DoMethod(data->editpopup,MUIM_FindUData,4), NULL);

    DoMethod(editor, MUIM_Notify, MUIA_TextEditor_UndoAvailable, MUIV_EveryTime, (Object *)DoMethod(data->editpopup,MUIM_FindUData, 5), 3, MUIM_Set, MUIA_Menuitem_Enabled, MUIV_TriggerValue);
    DoMethod(editor, MUIM_Notify, MUIA_TextEditor_RedoAvailable, MUIV_EveryTime, (Object *)DoMethod(data->editpopup,MUIM_FindUData, 6), 3, MUIM_Set, MUIA_Menuitem_Enabled, MUIV_TriggerValue);
  }

  return(group);
}
