/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2009 by TextEditor.mcc Open Source Team

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

#include <exec/memory.h>
#include <libraries/mui.h>
#include <clib/alib_protos.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/diskfont.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "TextEditor_mcp.h"
#include "private.h"
#include "Debug.h"

/// GetFont()
static struct TextFont *GetFont(UNUSED struct InstData *data, void *obj, long attr)
{
  struct TextFont *font = NULL;
  char *setting;

  ENTER();

  if(DoMethod(obj, MUIM_GetConfigItem, attr, &setting) && setting != NULL)
  {
    char *fontname;
    int fontnameLen;

    fontnameLen = strlen(setting)+6;
    if((fontname = AllocVec(fontnameLen, MEMF_SHARED|MEMF_CLEAR)) != NULL)
    {
      char *sizePtr;
      struct TextAttr textAttr;

      textAttr.ta_Name = fontname;
      textAttr.ta_YSize = 8;
      textAttr.ta_Style = FS_NORMAL;
      textAttr.ta_Flags = 0;

      strlcpy(fontname, setting, fontnameLen);
      if((sizePtr = strchr(fontname, '/')) != NULL)
      {
        LONG size;

        StrToLong(sizePtr + 1, &size);
        strlcpy(sizePtr, ".font", fontnameLen-(sizePtr-fontname));
        textAttr.ta_YSize = size;
      }

      font = OpenDiskFont(&textAttr);
    }

    FreeVec(fontname);
  }

  RETURN(font);
  return font;
}

///
/// SetCol()
static void SetCol(struct InstData *data, void *obj, long item, ULONG *storage, long bit)
{
  struct MUI_PenSpec *spec;

  ENTER();

  if(DoMethod(obj, MUIM_GetConfigItem, item, &spec))
  {
    *storage = MUI_ObtainPen(muiRenderInfo(obj), spec, 0L);
    data->allocatedpens |= 1<<bit;
  }
  else
    W(DBF_STARTUP, "couldn't get config item: 0x%08lx", item);

  LEAVE();
}

///
/// InitConfig()
void InitConfig(struct InstData *data, Object *obj)
{
  ULONG setting = 0;
  BOOL loadDefaultKeys = FALSE;

  ENTER();

  data->allocatedpens = 0;
  data->textcolor         = _pens(obj)[MPEN_TEXT];
  data->backgroundcolor   = _pens(obj)[MPEN_BACKGROUND];
  data->highlightcolor    = _pens(obj)[MPEN_SHINE];
  data->cursorcolor       = _pens(obj)[MPEN_SHINE];
  data->cursortextcolor   = _pens(obj)[MPEN_TEXT];
  data->markedcolor       = _pens(obj)[MPEN_FILL];
  data->separatorshine    = _pens(obj)[MPEN_HALFSHINE];
  data->separatorshadow   = _pens(obj)[MPEN_HALFSHADOW];
  data->inactivecolor     = _pens(obj)[MPEN_HALFSHADOW];

  SetCol(data, obj, MUICFG_TextEditor_TextColor, &data->textcolor, 0);
  SetCol(data, obj, MUICFG_TextEditor_CursorColor, &data->cursorcolor, 1);
  SetCol(data, obj, MUICFG_TextEditor_CursorTextColor, &data->cursortextcolor, 2);
  SetCol(data, obj, MUICFG_TextEditor_HighlightColor, &data->highlightcolor, 3);
  SetCol(data, obj, MUICFG_TextEditor_MarkedColor, &data->markedcolor, 4);
  SetCol(data, obj, MUICFG_TextEditor_SeparatorShine, &data->separatorshine, 5);
  SetCol(data, obj, MUICFG_TextEditor_SeparatorShadow, &data->separatorshadow, 6);
  SetCol(data, obj, MUICFG_TextEditor_InactiveColor, &data->inactivecolor, 7);

  if(isFlagClear(data->flags, FLG_OwnBackground))
  {
    LONG background = MUII_BACKGROUND;

    data->backgroundcolor = 0;
    data->fastbackground = TRUE;

    if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_Background, &setting) && setting != 0)
    {
      char *bg_setting = (char *)setting;

      if(bg_setting[0] == '2' && bg_setting[1] == ':' )
      {
        struct MUI_PenSpec *spec = (struct MUI_PenSpec *)(bg_setting+2);

        data->backgroundcolor = MUI_ObtainPen(muiRenderInfo(obj), spec, 0L);
        data->allocatedpens |= 1<<5;
      }
      else if(bg_setting[0] != '\0')
        data->fastbackground = FALSE;

      background = setting;
    }
    set(obj, MUIA_Background, background);
  }
  else
    data->fastbackground = FALSE;

  if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_TabSize, &setting))
  {
    data->TabSize = *(long *)setting;
    if(data->TabSize > 12)
      data->TabSize = 4;
  }
  else
  {
    data->TabSize = 4;
  }

  if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_CursorWidth, &setting))
  {
    data->CursorWidth = *(long *)setting;
    if(data->CursorWidth > 6)
      data->CursorWidth = 6;
  }
  else
  {
    data->CursorWidth = 6;
  }

  data->normalfont  = GetFont(data, obj, MUICFG_TextEditor_NormalFont);
  data->fixedfont   = GetFont(data, obj, MUICFG_TextEditor_FixedFont);
  data->font = (data->use_fixedfont == TRUE) ? data->fixedfont : data->normalfont;

  if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_BlockQual, &setting))
  {
    switch(*(LONG *)setting)
    {
      case 0:
        data->blockqual = IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT;
        break;
      case 1:
        data->blockqual = IEQUALIFIER_CONTROL;
        break;
      case 2:
        data->blockqual = IEQUALIFIER_LALT | IEQUALIFIER_RALT;
        break;
      case 3:
        data->blockqual = 0;
        break;
    }
  }
  else  data->blockqual = IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT;

  data->BlinkSpeed = FALSE;
  if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_BlinkSpeed, &setting))
  {
    if(*(LONG *)setting)
    {
      data->blinkhandler.ihn_Object    = obj;
      data->blinkhandler.ihn_Millis    = *(LONG *)setting*25;
      data->blinkhandler.ihn_Method    = MUIM_TextEditor_ToggleCursor;
      data->blinkhandler.ihn_Flags     = MUIIHNF_TIMER;
      data->BlinkSpeed = 1;
    }
  }

  if(isFlagClear(data->flags, FLG_OwnFrame))
  {
  #ifndef __AROS__
    if(MUIMasterBase->lib_Version >= 20)
        set(obj, MUIA_Frame, DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_Frame, &setting) ? (STRPTR)setting : (STRPTR)"302200");
    else  set(obj, MUIA_Frame, MUIV_Frame_String);
  #else
     set(obj, MUIA_Frame, MUIV_Frame_String);
     #warning "FIXME AROS/Zune: does not support things like MUIA_Frame, "302200"!"
  #endif
  }

  data->TypeAndSpell = FALSE;
  if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_TypeNSpell, &setting))
  {
    if(*(long *)setting)
      data->TypeAndSpell = TRUE;
    set(obj, MUIA_TextEditor_TypeAndSpell, data->TypeAndSpell);
  }

  if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_CheckWord, &setting))
  {
    if(*(long *)setting)
      setFlag(data->flags, FLG_CheckWords);
    else
      clearFlag(data->flags, FLG_CheckWords);
  }

  data->Flow = data->actualline->line.Flow;
  data->Pen = GetColor(data->CPos_X, data->actualline);

  if(isFlagClear(data->flags, FLG_FirstInit))
  {
    setFlag(data->flags, FLG_FirstInit);
    data->NoNotify = TRUE;
    SetAttrs(obj,
            MUIA_FillArea,              FALSE,
            MUIA_TextEditor_Flow,       data->Flow,
            MUIA_TextEditor_Pen,          data->Pen,
            MUIA_TextEditor_AreaMarked,   FALSE,
            MUIA_TextEditor_UndoAvailable,  FALSE,
            MUIA_TextEditor_RedoAvailable,  FALSE,
            MUIA_TextEditor_HasChanged,   FALSE,

            TAG_DONE);
    data->NoNotify = FALSE;
  }

  UpdateStyles(data);

  {
    long lort = TRUE;

    setting = (long)&lort;
    DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_Smooth, &setting);
    if(data->slider != NULL)
    {
      set(data->slider, MUIA_Prop_DoSmooth, *(long *)setting);
    }
  }

  data->selectPointer = TRUE;
  if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_SelectPointer, &setting))
  {
    if(*(long *)setting == 0)
      data->selectPointer = FALSE;
  }

  data->inactiveCursor = TRUE;
  if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_InactiveCursor, &setting))
  {
    if(*(long *)setting == 0)
      data->inactiveCursor = FALSE;
  }

  {
    ULONG undoSteps;

    // get the saved undo size only if it was not yet set by the application
    if(data->userUndoBufferSize == FALSE)
    {
      undoSteps = 500;

      if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_UndoSize, &setting))
      {
        undoSteps = *(long *)setting;

        // constrain the number of undo levels only if undo is enabled
        if(undoSteps != 0 && undoSteps < 20)
          undoSteps = 20;
      }
    }
    else
      undoSteps = data->maxUndoSteps;

    ResizeUndoBuffer(data, undoSteps);
  }

  data->LookupSpawn = 0;
  data->LookupCmd = "";
  if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_LookupCmd, &setting))
  {
    data->LookupSpawn = (short) *(ULONG *)setting;
    data->LookupCmd = (char *)setting+4;
  }

  data->SuggestSpawn = 1;
  data->SuggestCmd  = "\"Open('f', 'T:Matches', 'W');WriteLn('f', '%s');Close('f')\"";
  if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_SuggestCmd, &setting))
  {
    data->SuggestSpawn = (short) *(ULONG *)setting;
    data->SuggestCmd = (char *)setting+4;
  }

  if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_ConfigVersion, &setting))
  {
    if(*(ULONG *)setting != CONFIG_VERSION)
    {
      if(MUI_Request(_app(obj), NULL, 0L, "TextEditor.mcc Warning", "Ok|Abort",
                                          "Your current keybindings setup of TextEditor.mcc\n"
                                          "was found to be incompatible with this version of\n"
                                          "TextEditor.mcc.\n"
                                          "\n"
                                          "The keybindings of this object will be temporarly\n"
                                          "set to the default. Please visit the MUI preferences\n"
                                          "of TextEditor.mcc to permanently update the keybindings.") == 1)
      {
        loadDefaultKeys = TRUE;
      }
    }
  }

  {
    struct te_key *userkeys;
    ULONG count = 0;
    ULONG size;

    setting = 0;
    if(loadDefaultKeys == TRUE || !DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_Keybindings, &setting) || setting == 0)
      userkeys = (struct te_key *)default_keybindings;
    else
      userkeys = (struct te_key *)setting;

    while((WORD)userkeys[count].code != -1)
      count++;

    // now we calculate the memory size
    size = (count+1)*sizeof(struct te_key);

    if((data->RawkeyBindings = AllocVecPooled(data->mypool, size)) != NULL)
    {
      ULONG i;
      struct te_key *mykeys = data->RawkeyBindings;

      memcpy(mykeys, userkeys, size);

      for(i=0; i < count && (WORD)mykeys[i].code != -1; i++)
      {
        struct te_key *curKey = &mykeys[i];

        //D(DBF_STARTUP, "checking curKey[%d]: %08lx", i, curKey);

        if(curKey->code >= 500)
        {
          char RAW[4];
          char code = curKey->code-500;

          MapANSI(&code, 1, RAW, 1, NULL);

          curKey->code = RAW[0];
          curKey->qual |= RAW[1];

          if(RAW[0] == 67 && isFlagClear(curKey->qual, IEQUALIFIER_NUMERICPAD))
          {
            curKey->code = 68;
          }
        }
      }
    }
  }

  LEAVE();
}

///
/// FreeConfig()
void FreeConfig(struct InstData *data, struct MUI_RenderInfo *mri)
{
  ENTER();

  if(data->RawkeyBindings != NULL)
    FreeVecPooled(data->mypool, data->RawkeyBindings);

  if(data->allocatedpens & 1)
    MUI_ReleasePen(mri, data->textcolor);
  if(data->allocatedpens & 2)
    MUI_ReleasePen(mri, data->cursorcolor);
  if(data->allocatedpens & 4)
    MUI_ReleasePen(mri, data->cursortextcolor);
  if(data->allocatedpens & 8)
    MUI_ReleasePen(mri, data->highlightcolor);
  if(data->allocatedpens & 16)
    MUI_ReleasePen(mri, data->markedcolor);
  if(data->allocatedpens & 32)
    MUI_ReleasePen(mri, data->backgroundcolor);
  if(data->allocatedpens & 64)
    MUI_ReleasePen(mri, data->separatorshine);
  if(data->allocatedpens & 128)
    MUI_ReleasePen(mri, data->separatorshadow);
  if(data->allocatedpens & 256)
    MUI_ReleasePen(mri, data->inactivecolor);

  if(data->normalfont != NULL)
    CloseFont(data->normalfont);
  if(data->fixedfont != NULL)
    CloseFont(data->fixedfont);

  if(data->BlinkSpeed == 2)
  {
    DoMethod(_app(data->object), MUIM_Application_RemInputHandler, &data->blinkhandler);
    data->BlinkSpeed = 1;
  }

  LEAVE();
}

///
