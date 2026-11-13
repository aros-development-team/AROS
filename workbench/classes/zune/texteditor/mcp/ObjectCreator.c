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

#include <string.h>

#include <clib/alib_protos.h>
#include <devices/inputevent.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <mui/TextEditor_mcc.h>

#include "locale.h"
#include "private.h"
#include "muiextra.h"

const char *FunctionName (UWORD func)
{
  const char *name;

  switch(func)
  {
    case MUIV_TextEditor_KeyAction_Up:
      name = tr(MSG_Function_Up);
      break;

    case MUIV_TextEditor_KeyAction_Down:
      name = tr(MSG_Function_Down);
      break;

    case MUIV_TextEditor_KeyAction_Left:
      name = tr(MSG_Function_Left);
      break;

    case MUIV_TextEditor_KeyAction_Right:
      name = tr(MSG_Function_Right);
      break;

    case MUIV_TextEditor_KeyAction_PageUp:
      name = tr(MSG_Function_PrvPage);
      break;

    case MUIV_TextEditor_KeyAction_PageDown:
      name = tr(MSG_Function_NxtPage);
      break;

    case MUIV_TextEditor_KeyAction_StartOfLine:
      name = tr(MSG_Function_BOL);
      break;

    case MUIV_TextEditor_KeyAction_EndOfLine:
      name = tr(MSG_Function_EOL);
      break;

    case MUIV_TextEditor_KeyAction_Top:
      name = tr(MSG_Function_Top);
      break;

    case MUIV_TextEditor_KeyAction_Bottom:
      name = tr(MSG_Function_Bottom);
      break;

    case MUIV_TextEditor_KeyAction_PrevWord:
      name = tr(MSG_Function_PrvWord);
      break;

    case MUIV_TextEditor_KeyAction_NextWord:
      name = tr(MSG_Function_NxtWord);
      break;

    case MUIV_TextEditor_KeyAction_PrevLine:
      name = tr(MSG_Function_PrvPara);
      break;

    case MUIV_TextEditor_KeyAction_NextLine:
      name = tr(MSG_Function_NxtPara);
      break;

    case MUIV_TextEditor_KeyAction_PrevSentence:
      name = tr(MSG_Function_PrvSent);
      break;

    case MUIV_TextEditor_KeyAction_NextSentence:
      name = tr(MSG_Function_NxtSent);
      break;

    case MUIV_TextEditor_KeyAction_SuggestWord:
      name = tr(MSG_Function_SuggestSpelling);
      break;

    case MUIV_TextEditor_KeyAction_Backspace:
      name = tr(MSG_Function_Backspace);
      break;

    case MUIV_TextEditor_KeyAction_Delete:
      name = tr(MSG_Function_Delete);
      break;

    case MUIV_TextEditor_KeyAction_Return:
      name = tr(MSG_Function_Return);
      break;

    case MUIV_TextEditor_KeyAction_Tab:
      name = tr(MSG_Function_Tab);
      break;

    case MUIV_TextEditor_KeyAction_Cut:
      name = tr(MSG_Function_Cut);
      break;

    case MUIV_TextEditor_KeyAction_Copy:
      name = tr(MSG_Function_Copy);
      break;

    case MUIV_TextEditor_KeyAction_Paste:
      name = tr(MSG_Function_Paste);
      break;

    case MUIV_TextEditor_KeyAction_Undo:
      name = tr(MSG_Function_Undo);
      break;

    case MUIV_TextEditor_KeyAction_Redo:
      name = tr(MSG_Function_Redo);
      break;

    case MUIV_TextEditor_KeyAction_DelEOL:
      name = tr(MSG_Function_DelEOL);
      break;

    case MUIV_TextEditor_KeyAction_DelBOL:
      name = tr(MSG_Function_DelBOL);
      break;

    case MUIV_TextEditor_KeyAction_DelEOW:
      name = tr(MSG_Function_DelEOW);
      break;

    case MUIV_TextEditor_KeyAction_DelBOW:
      name = tr(MSG_Function_DelBOW);
      break;

    case MUIV_TextEditor_KeyAction_DelLine:
      name = tr(MSG_Function_DelLine);
      break;

    case MUIV_TextEditor_KeyAction_NextGadget:
      name = tr(MSG_Function_NextGadget);
      break;

    case MUIV_TextEditor_KeyAction_GotoBookmark1:
      name = tr(MSG_Function_GotoBookmark1);
      break;

    case MUIV_TextEditor_KeyAction_GotoBookmark2:
      name = tr(MSG_Function_GotoBookmark2);
      break;

    case MUIV_TextEditor_KeyAction_GotoBookmark3:
      name = tr(MSG_Function_GotoBookmark3);
      break;

    case MUIV_TextEditor_KeyAction_SetBookmark1:
      name = tr(MSG_Function_SetBookmark1);
      break;

    case MUIV_TextEditor_KeyAction_SetBookmark2:
      name = tr(MSG_Function_SetBookmark2);
      break;

    case MUIV_TextEditor_KeyAction_SetBookmark3:
      name = tr(MSG_Function_SetBookmark3);
      break;

    case MUIV_TextEditor_KeyAction_SelectAll:
      name = tr(MSG_Function_SelectAll);
      break;

    case MUIV_TextEditor_KeyAction_SelectNone:
      name = tr(MSG_Function_SelectNone);
      break;

    default:
      name = "";
  }

  return(name);
}

#define ARRAY_SIZE(X) (sizeof(X)/sizeof(X[0]))

static Object *PrefsObject(struct InstData_MCP *data)
{
  static const void *titles[] = {
    MSG_Page_Settings,
    MSG_Page_Keybindings,
    MSG_Page_SpellChecker,
    MSG_Page_Sample
  };
  static const void *functions[] = {
    MSG_Function_Up,
    MSG_Function_Down,
    MSG_Function_Left,
    MSG_Function_Right,
    MSG_Function_PrvPage,
    MSG_Function_NxtPage,
    MSG_Function_BOL,
    MSG_Function_EOL,
    MSG_Function_Top,
    MSG_Function_Bottom,
    MSG_Function_PrvWord,
    MSG_Function_NxtWord,
    MSG_Function_PrvPara,
    MSG_Function_NxtPara,
    MSG_Function_PrvSent,
    MSG_Function_NxtSent,
    MSG_Function_SuggestSpelling,
    MSG_Function_Backspace,
    MSG_Function_Delete,
    MSG_Function_Return,
    MSG_Function_Tab,
    MSG_Function_Cut,
    MSG_Function_Copy,
    MSG_Function_Paste,
    MSG_Function_Undo,
    MSG_Function_Redo,
    MSG_Function_DelBOL,
    MSG_Function_DelEOL,
    MSG_Function_DelBOW,
    MSG_Function_DelEOW,
    MSG_Function_NextGadget,
    MSG_Function_GotoBookmark1,
    MSG_Function_GotoBookmark2,
    MSG_Function_GotoBookmark3,
    MSG_Function_SetBookmark1,
    MSG_Function_SetBookmark2,
    MSG_Function_SetBookmark3,
    MSG_Function_DelLine,
    MSG_Function_SelectAll,
    MSG_Function_SelectNone
  };
  static const void *cycleentries[] = {
    MSG_CycleItem_Shift,
    MSG_CycleItem_Ctrl,
    MSG_CycleItem_Alt,
    MSG_CycleItem_Mouse
  };
  unsigned int i;

  ASSERT( ARRAY_SIZE(data->gTitles) == (ARRAY_SIZE(titles)+1) );
  for(i=0; i<ARRAY_SIZE(titles); i++)
    data->gTitles[i] = tr(titles[i]);
  data->gTitles[ARRAY_SIZE(titles)] = NULL;

  ASSERT( ARRAY_SIZE(data->functions) == (ARRAY_SIZE(functions)+1) );
  for(i=0; i<ARRAY_SIZE(functions); i++)
    data->functions[i] = tr(functions[i]);
  data->functions[ARRAY_SIZE(functions)] = NULL;

  ASSERT( ARRAY_SIZE(data->execution) == 3 );
  data->execution[0] = tr(MSG_Execution_CLI);
  data->execution[1] = tr(MSG_Execution_ARexx);
  data->execution[2] = NULL;

  ASSERT( ARRAY_SIZE(data->cycleentries) == (ARRAY_SIZE(cycleentries)+1) );
  for(i=0; i<ARRAY_SIZE(cycleentries); i++)
    data->cycleentries[i] = tr(cycleentries[i]);
  data->cycleentries[ARRAY_SIZE(cycleentries)] = NULL;

  data->obj = CreatePrefsGroup(data);
  if(data->obj)
  {
    set(data->normalfont, MUIA_String_AdvanceOnCR, TRUE);
    set(data->fixedfont, MUIA_String_AdvanceOnCR, TRUE);

    DoMethod(data->blockqual, MUIM_MultiSet, MUIA_CycleChain, TRUE,
            data->blockqual, data->tabsize, data->smooth,
            data->normalfont, data->fixedfont, data->textcolor, data->frame,
            data->highlightcolor, data->background, data->cursorcolor,
            data->markedcolor, data->cursorwidth, data->deletekey,
            data->blinkspeed, data->suggestcmd, data->lookupcmd,
            data->typenspell, data->undosize, data->LookupExeType,
            data->SuggestExeType, data->CheckWord, data->insertkey,
            data->separatorshadow, data->separatorshine, data->inactiveCursor,
            data->selectPointer, data->inactiveColor, NULL);
  }

  return(data->obj);
}

IPTR New(REG(a0, struct IClass *cl), REG(a2, Object *obj), REG(a1, struct opSet *msg))
{
  if((obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg)))
  {
    struct InstData_MCP *data = INST_DATA(cl, obj);

    // create the main prefs object
    Object *prefsobject = PrefsObject(data);

    if((data->CfgObj = prefsobject))
    {
      DoMethod(obj, OM_ADDMEMBER, prefsobject);

      if(MUIMasterBase->lib_Version >= 20)
      {
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->LookupExeType,   MUICFG_TextEditor_LookupCmd,       2, tr(MSG_Label_LookupCmd),       MUIA_Cycle_Active);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->lookupcmd,       MUICFG_TextEditor_LookupCmd,       2, tr(MSG_Label_LookupCmd),       MUIA_String_Contents);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->SuggestExeType,  MUICFG_TextEditor_SuggestCmd,      2, tr(MSG_Label_SuggestCmd),      MUIA_Cycle_Active);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->suggestcmd,      MUICFG_TextEditor_SuggestCmd,      2, tr(MSG_Label_SuggestCmd),      MUIA_String_Contents);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->keybindings,     MUICFG_TextEditor_Keybindings,     1, tr(MSG_Page_Keybindings)       );
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->frame,           MUICFG_TextEditor_Frame,           2, tr(MSG_Label_Frame),           MUIA_Framedisplay_Spec);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->background,      MUICFG_TextEditor_Background,      2, tr(MSG_Label_Background),      MUIA_Imagedisplay_Spec);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->blinkspeed,      MUICFG_TextEditor_BlinkSpeed,      2, tr(MSG_Label_BlinkSpeed),      MUIA_Numeric_Value);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->blockqual,       MUICFG_TextEditor_BlockQual,       2, tr(MSG_Label_BlkQual),         MUIA_Cycle_Active);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->cursorcolor,     MUICFG_TextEditor_CursorColor,     2, tr(MSG_Label_Cursor),          MUIA_Pendisplay_Spec);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->cursorwidth,     MUICFG_TextEditor_CursorWidth,     2, tr(MSG_Label_Width),           MUIA_Numeric_Value);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->fixedfont,       MUICFG_TextEditor_FixedFont,       2, tr(MSG_Label_Fixed),           MUIA_String_Contents);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->highlightcolor,  MUICFG_TextEditor_HighlightColor,  2, tr(MSG_Label_Highlight),       MUIA_Pendisplay_Spec);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->markedcolor,     MUICFG_TextEditor_MarkedColor,     2, tr(MSG_Label_Selected),        MUIA_Pendisplay_Spec);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->inactiveColor,   MUICFG_TextEditor_InactiveColor,   2, tr(MSG_Label_InactiveColor),   MUIA_Pendisplay_Spec);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->normalfont,      MUICFG_TextEditor_NormalFont,      2, tr(MSG_Label_Normal),          MUIA_String_Contents);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->smooth,          MUICFG_TextEditor_Smooth,          2, tr(MSG_Label_Smooth),          MUIA_Selected);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->selectPointer,   MUICFG_TextEditor_SelectPointer,   2, tr(MSG_Label_SelectPointer),   MUIA_Selected);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->typenspell,      MUICFG_TextEditor_TypeNSpell,      2, tr(MSG_ConfigMenu_TypeNSpell), MUIA_Selected);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->CheckWord,       MUICFG_TextEditor_CheckWord,       2, tr(MSG_ConfigMenu_CheckWord),  MUIA_Selected);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->tabsize,         MUICFG_TextEditor_TabSize,         2, tr(MSG_Label_TabSize),         MUIA_Numeric_Value);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->textcolor,       MUICFG_TextEditor_TextColor,       2, tr(MSG_Label_Text),            MUIA_Pendisplay_Spec);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->separatorshine,  MUICFG_TextEditor_SeparatorShine,  2, tr(MSG_Label_SeparatorShine),  MUIA_Pendisplay_Spec);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->separatorshadow, MUICFG_TextEditor_SeparatorShadow, 2, tr(MSG_Label_SeparatorShadow), MUIA_Pendisplay_Spec);
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->inactiveCursor,  MUICFG_TextEditor_InactiveCursor,  2, tr(MSG_Label_InactiveCursor),  MUIA_Pendisplay_Spec);
      }
      else
        set(data->frame, MUIA_Disabled, TRUE);

      return((IPTR)obj);
    }

    CoerceMethod(cl, obj, OM_DISPOSE);
  }
  return(FALSE);
}

IPTR Dispose(REG(a0, struct IClass *cl), REG(a2, Object *obj), REG(a1, Msg msg))
{
  struct InstData_MCP *data = INST_DATA(cl, obj);
  Object *editpopup = data->editpopup;

  if(data->CfgObj)
  {
    DoMethod(obj, OM_REMMEMBER, data->CfgObj);
    MUI_DisposeObject(data->CfgObj);
  }

  if(editpopup)
    MUI_DisposeObject(editpopup);

  return(DoSuperMethodA(cl, obj, msg));
}

IPTR GadgetsToConfig(REG(a0, struct IClass *cl), REG(a2, Object *obj), REG(a1, struct MUIP_Settingsgroup_GadgetsToConfig *msg))
{
  struct InstData_MCP *data = INST_DATA(cl, obj);
  IPTR cfg_data;

  ENTER();

  // first save the config version
  cfg_data = CONFIG_VERSION;
  DoMethod(msg->configdata, MUIM_Dataspace_Add, &cfg_data, sizeof(cfg_data), MUICFG_TextEditor_ConfigVersion);

  ExportKeys(data, msg->configdata);

  cfg_data = xget(data->frame, MUIA_Framedisplay_Spec);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, strlen((char *)cfg_data)+1, MUICFG_TextEditor_Frame);

  cfg_data = xget(data->background, MUIA_Imagedisplay_Spec);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, strlen((char *)cfg_data)+1, MUICFG_TextEditor_Background);

  cfg_data = xget(data->blinkspeed, MUIA_Numeric_Value);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, &cfg_data, sizeof(cfg_data), MUICFG_TextEditor_BlinkSpeed);

  cfg_data = xget(data->blockqual, MUIA_Cycle_Active);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, &cfg_data, sizeof(cfg_data), MUICFG_TextEditor_BlockQual);

  cfg_data = xget(data->cursorcolor, MUIA_Pendisplay_Spec);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, strlen((char *)cfg_data)+1, MUICFG_TextEditor_CursorColor);

  cfg_data = xget(data->cursorwidth, MUIA_Numeric_Value);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, &cfg_data, sizeof(cfg_data), MUICFG_TextEditor_CursorWidth);

  cfg_data = xget(data->fixedfont, MUIA_String_Contents);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, strlen((char *)cfg_data)+1, MUICFG_TextEditor_FixedFont);

  cfg_data = xget(data->frame, MUIA_Framedisplay_Spec);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, strlen((char *)cfg_data)+1, MUICFG_TextEditor_Frame);

  cfg_data = xget(data->highlightcolor, MUIA_Pendisplay_Spec);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, strlen((char *)cfg_data)+1, MUICFG_TextEditor_HighlightColor);

  cfg_data = xget(data->markedcolor, MUIA_Pendisplay_Spec);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, strlen((char *)cfg_data)+1, MUICFG_TextEditor_MarkedColor);

  cfg_data = xget(data->inactiveColor, MUIA_Pendisplay_Spec);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, strlen((char *)cfg_data)+1, MUICFG_TextEditor_InactiveColor);

  cfg_data = xget(data->normalfont, MUIA_String_Contents);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, strlen((char *)cfg_data)+1, MUICFG_TextEditor_NormalFont);

  {
    ULONG buffer[260/4];

    get(data->LookupExeType, MUIA_Cycle_Active, buffer);
    cfg_data = xget(data->lookupcmd, MUIA_String_Contents);
    CopyMem((APTR)cfg_data, &buffer[1], 256);
    DoMethod(msg->configdata, MUIM_Dataspace_Add, buffer, strlen((char *)cfg_data)+5, MUICFG_TextEditor_LookupCmd);

    get(data->SuggestExeType, MUIA_Cycle_Active, buffer);
    cfg_data = xget(data->suggestcmd, MUIA_String_Contents);
    CopyMem((APTR)cfg_data, &buffer[1], 256);
    DoMethod(msg->configdata, MUIM_Dataspace_Add, buffer, strlen((char *)cfg_data)+5, MUICFG_TextEditor_SuggestCmd);
  }

  cfg_data = xget(data->smooth, MUIA_Selected);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, &cfg_data, sizeof(cfg_data), MUICFG_TextEditor_Smooth);

  cfg_data = xget(data->selectPointer, MUIA_Selected);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, &cfg_data, sizeof(cfg_data), MUICFG_TextEditor_SelectPointer);

  cfg_data = xget(data->typenspell, MUIA_Selected);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, &cfg_data, sizeof(cfg_data), MUICFG_TextEditor_TypeNSpell);

  cfg_data = xget(data->CheckWord, MUIA_Selected);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, &cfg_data, sizeof(cfg_data), MUICFG_TextEditor_CheckWord);

  cfg_data = xget(data->tabsize, MUIA_Numeric_Value);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, &cfg_data, sizeof(cfg_data), MUICFG_TextEditor_TabSize);

  cfg_data = xget(data->undosize, MUIA_Numeric_Value);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, &cfg_data, sizeof(cfg_data), MUICFG_TextEditor_UndoSize);

  cfg_data = xget(data->textcolor, MUIA_Pendisplay_Spec);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, strlen((char *)cfg_data)+1, MUICFG_TextEditor_TextColor);

  cfg_data = xget(data->separatorshine, MUIA_Pendisplay_Spec);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, strlen((char *)cfg_data)+1, MUICFG_TextEditor_SeparatorShine);

  cfg_data = xget(data->separatorshadow, MUIA_Pendisplay_Spec);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, strlen((char *)cfg_data)+1, MUICFG_TextEditor_SeparatorShadow);

  cfg_data = xget(data->inactiveCursor, MUIA_Selected);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, &cfg_data, sizeof(cfg_data), MUICFG_TextEditor_InactiveCursor);

  RETURN(0);
  return(0);
}

IPTR ConfigToGadgets(REG(a0, struct IClass *cl), REG(a2, Object *obj), REG(a1, struct MUIP_Settingsgroup_ConfigToGadgets *msg))
{
  struct InstData_MCP *data = INST_DATA(cl, obj);
  APTR cfg_data;
  BOOL importKeys = TRUE;

  if((cfg_data = (APTR)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_ConfigVersion)))
  {
    if(*((ULONG *)cfg_data) != CONFIG_VERSION)
    {
      if(MUI_Request(NULL, NULL, 0L, tr(MSG_WarnConfigVersion_Title), tr(MSG_ResetAbort), tr(MSG_WarnConfigVersion)) == 1)
      {
        // reset the keybindings to their default values.
        ImportKeys(data, NULL);
        importKeys = FALSE;
      }
    }
  }

  if(importKeys == TRUE)
    ImportKeys(data, msg->configdata);

  if((cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_LookupCmd)))
  {
    nnset(data->LookupExeType, MUIA_Cycle_Active, *(ULONG *)cfg_data);
    nnset(data->lookupcmd, MUIA_String_Contents, (ULONG *)cfg_data+1);
  }
  else
  {
    nnset(data->LookupExeType, MUIA_Cycle_Active, 0);
    nnset(data->lookupcmd, MUIA_String_Contents, "");
  }

  if((cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_SuggestCmd)))
  {
    nnset(data->SuggestExeType, MUIA_Cycle_Active, *(ULONG *)cfg_data);
    nnset(data->suggestcmd, MUIA_String_Contents, (ULONG *)cfg_data+1);
  }
  else
  {
    nnset(data->SuggestExeType, MUIA_Cycle_Active, 1);
    nnset(data->suggestcmd, MUIA_String_Contents, "\"Open('f', 'T:Matches', 'W');WriteLn('f', '%s');Close('f')");
  }

  nnset(data->frame, MUIA_Framedisplay_Spec, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_Frame)) ?  cfg_data : "302200");
  nnset(data->background, MUIA_Imagedisplay_Spec, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_Background)) ? cfg_data : "2:m2");
  nnset(data->blinkspeed, MUIA_Numeric_Value, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_BlinkSpeed)) ? *(ULONG *)cfg_data : 0);
  nnset(data->blockqual, MUIA_Cycle_Active, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_BlockQual)) ? *(ULONG *)cfg_data : 0);
  nnset(data->cursorcolor, MUIA_Pendisplay_Spec, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_CursorColor)) ? cfg_data : "m0");
  nnset(data->cursorwidth, MUIA_Numeric_Value, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_CursorWidth)) ? *(ULONG *)cfg_data : 6);
  nnset(data->fixedfont, MUIA_String_Contents, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_FixedFont)) ? cfg_data : "");
  nnset(data->highlightcolor, MUIA_Pendisplay_Spec, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_HighlightColor)) ? cfg_data : "m0");
  nnset(data->markedcolor, MUIA_Pendisplay_Spec, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_MarkedColor)) ? cfg_data : "m6");
  nnset(data->inactiveColor, MUIA_Pendisplay_Spec, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_InactiveColor)) ? cfg_data : "m3");
  nnset(data->normalfont, MUIA_String_Contents, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_NormalFont)) ? cfg_data : "");
  nnset(data->smooth, MUIA_Selected, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_Smooth)) ? *(ULONG *)cfg_data : TRUE);
  nnset(data->selectPointer, MUIA_Selected, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_SelectPointer)) ? *(ULONG *)cfg_data : TRUE);
  nnset(data->typenspell, MUIA_Selected, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_TypeNSpell)) ? *(ULONG *)cfg_data : FALSE);
  nnset(data->CheckWord, MUIA_Selected, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_CheckWord)) ? *(ULONG *)cfg_data : FALSE);
  nnset(data->tabsize, MUIA_Numeric_Value, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_TabSize)) ? *(ULONG *)cfg_data : 4);
  nnset(data->undosize, MUIA_Numeric_Value, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_UndoSize)) ? *(ULONG *)cfg_data : 500);
  nnset(data->textcolor, MUIA_Pendisplay_Spec, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_TextColor)) ? cfg_data : "m5");
  nnset(data->separatorshine, MUIA_Pendisplay_Spec, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_SeparatorShine)) ? cfg_data : "m1");
  nnset(data->separatorshadow, MUIA_Pendisplay_Spec, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_SeparatorShadow)) ? cfg_data : "m3");
  nnset(data->inactiveCursor, MUIA_Selected, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_InactiveCursor)) ? *(ULONG *)cfg_data : TRUE);

  return(0);
}
