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

 $Id: ObjectCreator.c,v 1.3 2005/04/07 23:47:47 damato Exp $

***************************************************************************/

#include <string.h>
#include <stdio.h>

#include <clib/alib_protos.h>
#include <devices/inputevent.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "locale.h"
#include "private.h"

#ifndef __AROS__
#include "muiextra.h"
#else
#define MUIM_Mccprefs_RegisterGadget  0x80424828 // V20
struct MUI_ImageSpec
{
	char buf[64];
};

#endif

#include "TextEditor_mcc.h"

STRPTR FunctionName (UWORD func)
{
  STRPTR name;

  switch(func)
  {
    case mUp:
      name = GetStr(MSG_Function_Up);
      break;

    case mDown:
      name = GetStr(MSG_Function_Down);
      break;

    case mLeft:
      name = GetStr(MSG_Function_Left);
      break;

    case mRight:
      name = GetStr(MSG_Function_Right);
      break;

    case mPreviousPage:
      name = GetStr(MSG_Function_PrvPage);
      break;

    case mNextPage:
      name = GetStr(MSG_Function_NxtPage);
      break;

    case mStartOfLine:
      name = GetStr(MSG_Function_BOL);
      break;

    case mEndOfLine:
      name = GetStr(MSG_Function_EOL);
      break;

    case mTop:
      name = GetStr(MSG_Function_Top);
      break;

    case mBottom:
      name = GetStr(MSG_Function_Bottom);
      break;

    case mPreviousWord:
      name = GetStr(MSG_Function_PrvWord);
      break;

    case mNextWord:
      name = GetStr(MSG_Function_NxtWord);
      break;

    case mPreviousLine:
      name = GetStr(MSG_Function_PrvPara);
      break;

    case mNextLine:
      name = GetStr(MSG_Function_NxtPara);
      break;

    case mPreviousSentence:
      name = GetStr(MSG_Function_PrvSent);
      break;

    case mNextSentence:
      name = GetStr(MSG_Function_NxtSent);
      break;

    case kSuggestWord:
      name = GetStr(MSG_Function_SuggestSpelling);
      break;

    case kBackspace:
      name = GetStr(MSG_Function_Backspace);
      break;

    case kDelete:
      name = GetStr(MSG_Function_Delete);
      break;

    case kReturn:
      name = GetStr(MSG_Function_Return);
      break;

    case kTab:
      name = GetStr(MSG_Function_Tab);
      break;

    case kCut:
      name = GetStr(MSG_Function_Cut);
      break;

    case kCopy:
      name = GetStr(MSG_Function_Copy);
      break;

    case kPaste:
      name = GetStr(MSG_Function_Paste);
      break;

    case kUndo:
      name = GetStr(MSG_Function_Undo);
      break;
    
    case kRedo:
      name = GetStr(MSG_Function_Redo);
      break;
    
    case kDelEOL:
      name = GetStr(MSG_Function_DelEOL);
      break;
    
    case kDelBOL:
      name = GetStr(MSG_Function_DelBOL);
      break;
    
    case kDelEOW:
      name = GetStr(MSG_Function_DelEOW);
      break;
    
    case kDelBOW:
      name = GetStr(MSG_Function_DelBOW);
      break;
    
    case kDelLine:
      name = GetStr(MSG_Function_DelLine);
      break;
    
    case kNextGadget:
      name = GetStr(MSG_Function_NextGadget);
      break;
    
    case kGotoBookmark1:
      name = GetStr(MSG_Function_GotoBookmark1);
      break;
    
    case kGotoBookmark2:
      name = GetStr(MSG_Function_GotoBookmark2);
      break;
    
    case kGotoBookmark3:
      name = GetStr(MSG_Function_GotoBookmark3);
      break;
    
    case kSetBookmark1:
      name = GetStr(MSG_Function_SetBookmark1);
      break;
    
    case kSetBookmark2:
      name = GetStr(MSG_Function_SetBookmark2);
      break;
    
    case kSetBookmark3:
      name = GetStr(MSG_Function_SetBookmark3);
      break;
    
    default:
      name = "";
  }
  return(name);
}

void *PrefsObject(struct InstData_MCP *data)
{
  data->gTitles[0] = GetStr(MSG_Page_Settings);
  data->gTitles[1] = GetStr(MSG_Page_Keybindings);
  data->gTitles[2] = GetStr(MSG_Page_SpellChecker);
  data->gTitles[3] = GetStr(MSG_Page_Sample);
  data->gTitles[4] = NULL;

  data->functions[0] = GetStr(MSG_Function_Up);
  data->functions[1] = GetStr(MSG_Function_Down);
  data->functions[2] = GetStr(MSG_Function_Left);
  data->functions[3] = GetStr(MSG_Function_Right);
  data->functions[4] = GetStr(MSG_Function_PrvPage);
  data->functions[5] = GetStr(MSG_Function_NxtPage);
  data->functions[6] = GetStr(MSG_Function_BOL);
  data->functions[7] = GetStr(MSG_Function_EOL);
  data->functions[8] = GetStr(MSG_Function_Top);
  data->functions[9] = GetStr(MSG_Function_Bottom);
  data->functions[10] = GetStr(MSG_Function_PrvWord);
  data->functions[11] = GetStr(MSG_Function_NxtWord);
  data->functions[12] = GetStr(MSG_Function_PrvPara);
  data->functions[13] = GetStr(MSG_Function_NxtPara);
  data->functions[14] = GetStr(MSG_Function_PrvSent);
  data->functions[15] = GetStr(MSG_Function_NxtSent);

  data->functions[16] = GetStr(MSG_Function_SuggestSpelling);
  data->functions[17] = GetStr(MSG_Function_Backspace);
  data->functions[18] = GetStr(MSG_Function_Delete);
  data->functions[19] = GetStr(MSG_Function_Return);
  data->functions[20] = GetStr(MSG_Function_Tab);
  data->functions[21] = GetStr(MSG_Function_Cut);
  data->functions[22] = GetStr(MSG_Function_Copy);
  data->functions[23] = GetStr(MSG_Function_Paste);
  data->functions[24] = GetStr(MSG_Function_Undo);
  data->functions[25] = GetStr(MSG_Function_Redo);
  data->functions[26] = GetStr(MSG_Function_DelBOL);
  data->functions[27] = GetStr(MSG_Function_DelEOL);
  data->functions[28] = GetStr(MSG_Function_DelBOW);
  data->functions[29] = GetStr(MSG_Function_DelEOW);
  data->functions[30] = GetStr(MSG_Function_NextGadget);
  data->functions[31] = GetStr(MSG_Function_GotoBookmark1);
  data->functions[32] = GetStr(MSG_Function_GotoBookmark2);
  data->functions[33] = GetStr(MSG_Function_GotoBookmark3);
  data->functions[34] = GetStr(MSG_Function_SetBookmark1);
  data->functions[35] = GetStr(MSG_Function_SetBookmark2);
  data->functions[36] = GetStr(MSG_Function_SetBookmark3);
  data->functions[37] = GetStr(MSG_Function_DelLine);
  data->functions[38] = NULL;

  data->execution[0] = GetStr(MSG_Execution_CLI);
  data->execution[1] = GetStr(MSG_Execution_ARexx);
  data->execution[2] = NULL;

  data->cycleentries[0] = GetStr(MSG_CycleItem_Shift);
  data->cycleentries[1] = GetStr(MSG_CycleItem_Ctrl);
  data->cycleentries[2] = GetStr(MSG_CycleItem_Alt);
  data->cycleentries[3] = GetStr(MSG_CycleItem_Mouse);
  data->cycleentries[4] = NULL;

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
            data->separatorshadow, data->separatorshine, NULL);

  }
  return(data->obj);
}

ULONG New(REG(a0, struct IClass *cl), REG(a2, Object *obj), REG(a1, struct opSet *msg))
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
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->LookupExeType, MUICFG_TextEditor_LookupCmd, 1, GetStr(MSG_Label_LookupCmd));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->lookupcmd, MUICFG_TextEditor_LookupCmd, 1, GetStr(MSG_Label_LookupCmd));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->SuggestExeType, MUICFG_TextEditor_SuggestCmd, 1, GetStr(MSG_Label_SuggestCmd));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->suggestcmd, MUICFG_TextEditor_SuggestCmd, 1, GetStr(MSG_Label_SuggestCmd));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->keybindings, MUICFG_TextEditor_Keybindings, 1, GetStr(MSG_Page_Keybindings));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->frame, MUICFG_TextEditor_Frame, 1, GetStr(MSG_Label_Frame));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->background, MUICFG_TextEditor_Background, 1, GetStr(MSG_Label_Background));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->blinkspeed, MUICFG_TextEditor_BlinkSpeed, 1, GetStr(MSG_Label_BlinkSpeed));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->blockqual, MUICFG_TextEditor_BlockQual, 1, GetStr(MSG_Label_BlkQual));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->cursorcolor, MUICFG_TextEditor_CursorColor, 1, GetStr(MSG_Label_Cursor));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->cursorwidth, MUICFG_TextEditor_CursorWidth, 1, GetStr(MSG_Label_Width));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->fixedfont, MUICFG_TextEditor_FixedFont, 1, GetStr(MSG_Label_Fixed));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->highlightcolor, MUICFG_TextEditor_HighlightColor, 1, GetStr(MSG_Label_Highlight));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->markedcolor, MUICFG_TextEditor_MarkedColor, 1, GetStr(MSG_Label_Selected));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->normalfont, MUICFG_TextEditor_NormalFont, 1, GetStr(MSG_Label_Normal));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->smooth, MUICFG_TextEditor_Smooth, 1, GetStr(MSG_Label_Smooth));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->typenspell, MUICFG_TextEditor_TypeNSpell, 1, GetStr(MSG_ConfigMenu_TypeNSpell));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->CheckWord, MUICFG_TextEditor_CheckWord, 1, GetStr(MSG_ConfigMenu_CheckWord));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->tabsize, MUICFG_TextEditor_TabSize, 1, GetStr(MSG_Label_TabSize));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->undosize, MUICFG_TextEditor_UndoSize, 1, GetStr(MSG_Label_UndoLevel));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->textcolor, MUICFG_TextEditor_TextColor, 1, GetStr(MSG_Label_Text));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->separatorshine, MUICFG_TextEditor_SeparatorShine, 1, GetStr(MSG_Label_SeparatorShine));
        DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->separatorshadow, MUICFG_TextEditor_SeparatorShadow, 1, GetStr(MSG_Label_SeparatorShadow));
      }
      else
        set(data->frame, MUIA_Disabled, TRUE);

      return((ULONG)obj);
    }

    CoerceMethod(cl, obj, OM_DISPOSE);
  }
  return(FALSE);
}

ULONG Dispose(REG(a0, struct IClass *cl), REG(a2, Object *obj), REG(a1, Msg msg))
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

ULONG GadgetsToConfig(REG(a0, struct IClass *cl), REG(a2, Object *obj), REG(a1, struct MUIP_Settingsgroup_GadgetsToConfig *msg))
{
  struct InstData_MCP *data = INST_DATA(cl, obj);
  LONG cfg_data = 2;

  // first save the config version
  DoMethod(msg->configdata, MUIM_Dataspace_Add, &cfg_data, sizeof(LONG), MUICFG_TextEditor_ConfigVersion);

  ExportKeys(msg->configdata, data);

  get(data->frame, MUIA_Framedisplay_Spec, &cfg_data);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, sizeof(struct MUI_FrameSpec), MUICFG_TextEditor_Frame);

  get(data->background, MUIA_Imagedisplay_Spec, &cfg_data);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, sizeof(struct MUI_ImageSpec), MUICFG_TextEditor_Background);

  get(data->blinkspeed, MUIA_Numeric_Value, &cfg_data);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, &cfg_data, sizeof(LONG), MUICFG_TextEditor_BlinkSpeed);

  get(data->blockqual, MUIA_Cycle_Active, &cfg_data);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, &cfg_data, sizeof(LONG), MUICFG_TextEditor_BlockQual);

  get(data->cursorcolor, MUIA_Pendisplay_Spec, &cfg_data);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, sizeof(struct MUI_PenSpec), MUICFG_TextEditor_CursorColor);

  get(data->cursorwidth, MUIA_Numeric_Value, &cfg_data);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, &cfg_data, sizeof(LONG), MUICFG_TextEditor_CursorWidth);

  get(data->fixedfont, MUIA_String_Contents, &cfg_data);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, strlen((char *)cfg_data)+1, MUICFG_TextEditor_FixedFont);

  get(data->frame, MUIA_Framedisplay_Spec, &cfg_data);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, sizeof(struct MUI_FrameSpec), MUICFG_TextEditor_Frame);

  get(data->highlightcolor, MUIA_Pendisplay_Spec, &cfg_data);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, sizeof(struct MUI_PenSpec), MUICFG_TextEditor_HighlightColor);

  get(data->markedcolor, MUIA_Pendisplay_Spec, &cfg_data);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, sizeof(struct MUI_PenSpec), MUICFG_TextEditor_MarkedColor);

  get(data->normalfont, MUIA_String_Contents, &cfg_data);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, strlen((char *)cfg_data)+1, MUICFG_TextEditor_NormalFont);

  {
    ULONG buffer[260/4];
    ULONG active;
    
    get(data->LookupExeType, MUIA_Cycle_Active, &active);
    buffer[0] = active;
    
    get(data->lookupcmd, MUIA_String_Contents, &cfg_data);
    CopyMem((void *)cfg_data, buffer+1, 256);
    DoMethod(msg->configdata, MUIM_Dataspace_Add, buffer, strlen((char *)cfg_data)+5, MUICFG_TextEditor_LookupCmd);

    get(data->SuggestExeType, MUIA_Cycle_Active, &active);
    buffer[0] = active;
    
    get(data->suggestcmd, MUIA_String_Contents, &cfg_data);
    CopyMem((void *)cfg_data, buffer+1, 256);
    DoMethod(msg->configdata, MUIM_Dataspace_Add, buffer, strlen((char *)cfg_data)+5, MUICFG_TextEditor_SuggestCmd);
  }

  get(data->smooth, MUIA_Selected, &cfg_data);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, &cfg_data, sizeof(LONG), MUICFG_TextEditor_Smooth);

  get(data->typenspell, MUIA_Selected, &cfg_data);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, &cfg_data, sizeof(LONG), MUICFG_TextEditor_TypeNSpell);

  get(data->CheckWord, MUIA_Selected, &cfg_data);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, &cfg_data, sizeof(LONG), MUICFG_TextEditor_CheckWord);

  get(data->tabsize, MUIA_Numeric_Value, &cfg_data);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, &cfg_data, sizeof(LONG), MUICFG_TextEditor_TabSize);

  get(data->undosize, MUIA_Numeric_Value, &cfg_data);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, &cfg_data, sizeof(LONG), MUICFG_TextEditor_UndoSize);

  get(data->textcolor, MUIA_Pendisplay_Spec, &cfg_data);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, sizeof(struct MUI_PenSpec), MUICFG_TextEditor_TextColor);

  get(data->separatorshine, MUIA_Pendisplay_Spec, &cfg_data);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, sizeof(struct MUI_PenSpec), MUICFG_TextEditor_SeparatorShine);

  get(data->separatorshadow, MUIA_Pendisplay_Spec, &cfg_data);
  DoMethod(msg->configdata, MUIM_Dataspace_Add, cfg_data, sizeof(struct MUI_PenSpec), MUICFG_TextEditor_SeparatorShadow);

  return(0);
}

ULONG ConfigToGadgets(REG(a0, struct IClass *cl), REG(a2, Object *obj), REG(a1, struct MUIP_Settingsgroup_ConfigToGadgets *msg))
{
  struct InstData_MCP *data = INST_DATA(cl, obj);
  APTR  cfg_data;

/*  if(cfg_data = (APTR)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_ConfigVersion))
      kprintf("Config version: %ld\n", *((ULONG *)cfg_data));
  else  kprintf("Obsolete config\n");
*/
  ImportKeys(msg->configdata, data);

  if((cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_LookupCmd)))
  {
    set(data->LookupExeType, MUIA_Cycle_Active, *(ULONG *)cfg_data);
    set(data->lookupcmd, MUIA_String_Contents, (ULONG *)cfg_data+1);
  }
  else
  {
    set(data->LookupExeType, MUIA_Cycle_Active, 0);
    set(data->lookupcmd, MUIA_String_Contents, "");
  }

  if((cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_SuggestCmd)))
  {
    set(data->SuggestExeType, MUIA_Cycle_Active, *(ULONG *)cfg_data);
    set(data->suggestcmd, MUIA_String_Contents, (ULONG *)cfg_data+1);
  }
  else
  {
    set(data->SuggestExeType, MUIA_Cycle_Active, 1);
    set(data->suggestcmd, MUIA_String_Contents, "\"Open('f', 'T:Matches', 'W');WriteLn('f', '%s');Close('f')");
  }

  set(data->frame, MUIA_Framedisplay_Spec, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_Frame)) ?  cfg_data : "302200");
  set(data->background, MUIA_Imagedisplay_Spec, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_Background)) ? cfg_data : "2:m2");
  set(data->blinkspeed, MUIA_Numeric_Value, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_BlinkSpeed)) ? *(ULONG *)cfg_data : 0);
  set(data->blockqual, MUIA_Cycle_Active, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_BlockQual)) ? *(ULONG *)cfg_data : 0);
  set(data->cursorcolor, MUIA_Pendisplay_Spec, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_CursorColor)) ? cfg_data : "m0");
  set(data->cursorwidth, MUIA_Numeric_Value, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_CursorWidth)) ? *(ULONG *)cfg_data : 6);
  set(data->fixedfont, MUIA_String_Contents, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_FixedFont)) ? cfg_data : "");
  set(data->highlightcolor, MUIA_Pendisplay_Spec, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_HighlightColor)) ? cfg_data : "m0");
  set(data->markedcolor, MUIA_Pendisplay_Spec, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_MarkedColor)) ? cfg_data : "m6");
  set(data->normalfont, MUIA_String_Contents, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_NormalFont)) ? cfg_data : "");
  set(data->smooth, MUIA_Selected, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_Smooth)) ? *(ULONG *)cfg_data : TRUE);
  set(data->typenspell, MUIA_Selected, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_TypeNSpell)) ? *(ULONG *)cfg_data : FALSE);
  set(data->CheckWord, MUIA_Selected, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_CheckWord)) ? *(ULONG *)cfg_data : FALSE);
  set(data->tabsize, MUIA_Numeric_Value, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_TabSize)) ? *(ULONG *)cfg_data : 4);
  set(data->undosize, MUIA_Numeric_Value, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_UndoSize)) ? *(ULONG *)cfg_data : 500);
  set(data->textcolor, MUIA_Pendisplay_Spec, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_TextColor)) ? cfg_data : "m5");
  set(data->separatorshine, MUIA_Pendisplay_Spec, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_SeparatorShine)) ? cfg_data : "m1");
  set(data->separatorshadow, MUIA_Pendisplay_Spec, (cfg_data = (void *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_TextEditor_SeparatorShadow)) ? cfg_data : "m3");

  return(0);
}
