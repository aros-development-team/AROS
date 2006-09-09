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

#include "TextEditor_mcc.h"
#include "TextEditor_mcp.h"
#include "private.h"

extern struct keybindings keys[];

struct TextFont *GetFont(UNUSED struct InstData *data, void *obj, long attr)
{
  char *setting;
  char *fontname;
  char *size_ptr;
  struct TextFont *f;
  struct TextAttr myfont;

  ENTER();

  if(!DoMethod(obj, MUIM_GetConfigItem, attr, &setting))
  {
    RETURN(NULL);
    return NULL;
  }

  if(!setting)
  {
    RETURN(NULL);
    return NULL;
  }

  if(!(fontname = AllocVec(strlen(setting)+6, MEMF_ANY|MEMF_CLEAR)))
  {
    RETURN(NULL);
    return NULL;
  }

  f = NULL;

  myfont.ta_Name = fontname;
  myfont.ta_YSize = 8;
  myfont.ta_Style = FS_NORMAL;
  myfont.ta_Flags = 0;

  strcpy(fontname,setting);
  size_ptr = strchr(fontname,'/');
  if (size_ptr)
  {
    LONG size;

    StrToLong(size_ptr + 1, &size);
    strncpy(size_ptr, ".font", 6);
    myfont.ta_YSize = size;
  }

  f = OpenDiskFont(&myfont);
  FreeVec(fontname);

  RETURN(f);
  return f;
}

void SetCol (struct InstData *data, void *obj, long item, ULONG *storage, long bit)
{
  struct MUI_PenSpec *spec;

  ENTER();

  if(DoMethod(obj, MUIM_GetConfigItem, item, &spec))
  {
    *storage = MUI_ObtainPen(muiRenderInfo(obj), spec, 0L);
    data->allocatedpens |= 1<<bit;
  }

  LEAVE();
}

BOOL iswarned = FALSE;

void InitConfig(Object *obj, struct InstData *data)
{
  long  setting;
  UWORD *muipens = _pens(obj);

  ENTER();

  data->allocatedpens = 0;
  data->textcolor         = *(muipens+MPEN_TEXT);
  data->backgroundcolor   = *(muipens+MPEN_BACKGROUND);
  data->highlightcolor    = *(muipens+MPEN_SHINE);
  data->cursorcolor       = *(muipens+MPEN_SHINE);
  data->cursortextcolor   = *(muipens+MPEN_TEXT);
  data->markedcolor       = *(muipens+MPEN_FILL);
  data->separatorshine    = *(muipens+MPEN_HALFSHINE);
  data->separatorshadow = *(muipens+MPEN_HALFSHADOW);

  SetCol(data, obj, MUICFG_TextEditor_TextColor, &data->textcolor, 0);
  SetCol(data, obj, MUICFG_TextEditor_CursorColor, &data->cursorcolor, 1);
  SetCol(data, obj, MUICFG_TextEditor_CursorTextColor, &data->cursortextcolor, 2);
  SetCol(data, obj, MUICFG_TextEditor_HighlightColor, &data->highlightcolor, 3);
  SetCol(data, obj, MUICFG_TextEditor_MarkedColor, &data->markedcolor, 4);
  SetCol(data, obj, MUICFG_TextEditor_SeparatorShine, &data->separatorshine, 5);
  SetCol(data, obj, MUICFG_TextEditor_SeparatorShadow, &data->separatorshadow, 6);

  if(!(data->flags & FLG_OwnBkgn))
  {
      LONG background = MUII_BACKGROUND;

    data->backgroundcolor = 0;
    data->fastbackground = TRUE;
    if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_Background, &setting))
    {
      if(*(char *)setting == '2' && *((char *)setting+1) == ':' )
      {
          struct MUI_PenSpec *spec = (struct MUI_PenSpec *)(char *)(setting+2);

        data->backgroundcolor = MUI_ObtainPen(muiRenderInfo(obj), spec, 0L);
        data->allocatedpens |= 1<<5;
      }
      else
      {
        if(*(char *)setting != '\0')
        {
          data->fastbackground = FALSE;
        }
      }
      background = setting;
    }
    set(obj, MUIA_Background, background);
  }
  else  data->fastbackground = FALSE;

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
  data->font = data->use_fixedfont ? data->fixedfont : data->normalfont;

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

  if(!(data->flags & FLG_OwnFrame))
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
    data->TypeAndSpell = *(long *)setting;
    set(obj, MUIA_TextEditor_TypeAndSpell, data->TypeAndSpell);
  }

  if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_CheckWord, &setting))
  {
    if(*(long *)setting)
    {
      data->flags |= FLG_CheckWords;
    }
    else
    {
      data->flags &= ~FLG_CheckWords;
    }
  }

  data->Flow = data->actualline->line.Flow;
  data->Pen = GetColor(data->CPos_X, data->actualline);

  if(~data->flags & FLG_FirstInit)
  {
    data->flags |= FLG_FirstInit;
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
      long  lort = TRUE;

    setting = (long)&lort;
    DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_Smooth, &setting);
    if(data->slider)
    {
      set(data->slider, MUIA_Prop_DoSmooth, *(long *)setting);
    }
  }

  {
      long undolevel = 500;

    if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_UndoSize, &setting))
    {
      undolevel = *(long *)setting;
      if(undolevel < 20)
        undolevel = 20;
    }
    undolevel += 5;

    if(data->undosize != (undolevel*sizeof(struct UserAction))+1)
    {
      if((data->undobuffer = MyAllocPooled(data->mypool, (undolevel*sizeof(struct UserAction))+1)))
      {
        data->undopointer = data->undobuffer;
        *(short *)data->undopointer = 0xff;
        data->undosize = (undolevel*sizeof(struct UserAction))+1;
      }
      else
      {
        data->undosize = 0;
      }
    }
  }

  data->LookupSpawn = 0;
  data->LookupCmd = "";
  if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_LookupCmd, &setting))
  {
    data->LookupSpawn = (short) *(ULONG *)setting;
    data->LookupCmd = (char *)setting+4;
  }

  data->SuggestSpawn = 1;
  data->SuggestCmd  = "\"Open('f', 'T:Matches', 'W');WriteLn('f', '%s');Close('f')";
  if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_SuggestCmd, &setting))
  {
    data->SuggestSpawn = (short) *(ULONG *)setting;
    data->SuggestCmd = (char *)setting+4;
  }

  if(!iswarned && DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_ConfigVersion, &setting))
  {
    iswarned = TRUE;
    if(*(ULONG *)setting < 2)
      MUI_Request(_app(obj), NULL, 0L, "TextEditor.mcc", "Continue", "\33cThe keybindings page has been updated\nsince your last visit to MUIPrefs.", 0);
  }

  {
      struct keybindings *userkeys;
      ULONG  c = 0;

    if(!DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_Keybindings, &setting))
    {
      setting = (long)keys;
    }
    userkeys = (struct keybindings *)setting;

    while(userkeys->keydata.code != (UWORD)-1)
    {
      userkeys++;
      c++;
    }

    if((data->RawkeyBindings = MyAllocPooled(data->mypool, (c+2)*sizeof(struct te_key))))
    {
      unsigned long count;
      struct keybindings *mykeys = data->RawkeyBindings;

      CopyMem((APTR)setting, data->RawkeyBindings, c*sizeof(struct te_key));
      (mykeys+c)->keydata.code = (UWORD)-1;

      for(count = 0;count != c;count++)
      {
        if((mykeys+count)->keydata.code >= 500)
        {
          char RAW[4];
          char code = (mykeys+count)->keydata.code-500;

          MapANSI(&code, 1, RAW, 1, NULL);

          (mykeys+count)->keydata.code = RAW[0];
          (mykeys+count)->keydata.qual |= RAW[1];

/*          if((mykeys+count)->keydata.qual & IEQUALIFIER_NUMERICPAD)
          {
            printf("0x%lx, %d (*)\n", (mykeys+count)->keydata.qual, (mykeys+count)->keydata.code);
          }
*/
          if(RAW[0] == 67 && !((mykeys+count)->keydata.qual & IEQUALIFIER_NUMERICPAD))
          {
            (mykeys+count)->keydata.code = 68;
          }
        }
      }
    }
  }

  LEAVE();
}

void  FreeConfig  (struct InstData *data, struct MUI_RenderInfo *mri)
{
  ENTER();

  if(data->RawkeyBindings)
    MyFreePooled(data->mypool, data->RawkeyBindings);

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

  if(data->normalfont)
    CloseFont(data->normalfont);
  if(data->fixedfont)
    CloseFont(data->fixedfont);

  if(data->BlinkSpeed == 2)
  {
    DoMethod(_app(data->object), MUIM_Application_RemInputHandler, &data->blinkhandler);
    data->BlinkSpeed = 1;
  }

  LEAVE();
}

