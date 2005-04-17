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

 $Id: CA_InitConfig.c,v 1.1 2005/03/28 11:29:49 damato Exp $

***************************************************************************/

#include <StdIO.h>
#include <clib/alib_protos.h>
#include <Libraries/MUI.h>
#include <Proto/DOS.h>
#include <Proto/DiskFont.h>
#include <Proto/Exec.h>
#include <Proto/Graphics.h>
#include <Proto/Intuition.h>
#include <Proto/KeyMap.h>
#include <Proto/MUIMaster.h>
#include <String.h>

#include <Editor.h>
#include <TextEditor_MCC.h>

#include <stormamiga.h>
#define sprintf SPRINTF

  extern struct keybindings keys[];
/*
struct TextFont *GetFont (struct mydata *data, void *obj, long attr)
{
    long  setting;

  if(DoMethod(obj, MUIM_GetConfigItem, attr, &setting))
  {
      char  *src = (char *)setting;
      char  fontname[40];
      struct TextAttr myfont =
      {
        fontname,
        8,
        FS_NORMAL,
        0
      };
      long  c = 0;

    while(src[c] != '/' && src[c] != '\0' && c < 32)
    {
      fontname[c-1] = src[c++];
    }
    strncpy(&fontname[c], ".font", 6);
    StrToLong(&src[c+1], &c);
    myfont.ta_YSize = c;

    return(OpenDiskFont(&myfont));
  }
  else
  {
    return(NULL);
  }
}

void SetCol (struct mydata *data, void *obj, long item, ULONG *storage, long bit)
{
    struct MUI_PenSpec *spec;

  if(DoMethod(obj, MUIM_GetConfigItem, item, &spec))
  {
    *storage = MUI_ObtainPen(muiRenderInfo(obj), spec, 0L);
    data->allocatedpens |= 1<<bit;
  }
}
*/
void InitConfig (void *obj, struct mydata *data)
{
  data->allocatedpens   = 0;

  data->textcolor         = 1;
  data->backgroundcolor   = 0;
  data->highlightcolor    = 2;
  data->cursorcolor       = 2;
  data->cursortextcolor   = 1;
  data->markedcolor       = 3;
  data->separatorshine    = 2;
  data->separatorshadow = 1;

  data->fastbackground = TRUE;

  data->TabSize = 4;
  data->CursorWidth = 6;

//  data->normalfont  = GetFont(data, obj, MUICFG_TextEditor_NormalFont);
//  data->fixedfont   = GetFont(data, obj, MUICFG_TextEditor_FixedFont);

  data->blockqual = IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT;

  data->BlinkSpeed = FALSE;
  data->TypeAndSpell = FALSE;
  data->Flow = data->actualline->flow;

  {
      long undolevel = 500;

    undolevel += 5;

    if(data->undosize != (undolevel*sizeof(UserAction))+1)
    {
      if(data->undobuffer = MyAllocPooled(data->mypool, (undolevel*sizeof(UserAction))+1))
      {
        data->undopointer = data->undobuffer;
        *(short *)data->undopointer = 0xff;
        data->undosize = (undolevel*sizeof(UserAction))+1;
      }
      else
      {
        data->undosize = 0;
      }
    }
  }

  data->LookupSpawn = 0;
  data->LookupCmd = "";

  data->SuggestSpawn = 1;
  data->SuggestCmd  = "\"Open('f', 'T:Matches', 'W');WriteLn('f', '%s');Close('f')";

  {
      struct keybindings *userkeys;
      ULONG  c = 0;
      long  setting;

    setting = (long)keys;
    userkeys = (struct keybindings *)setting;

    while(userkeys->keydata.code != (UWORD)-1)
    {
      userkeys++;
      c++;
    }

    if(data->RawkeyBindings = MyAllocPooled(data->mypool, (c+2)*sizeof(te_key)))
    {
        LONG count;
        struct keybindings *mykeys = data->RawkeyBindings;

      CopyMem((APTR)setting, data->RawkeyBindings, c*sizeof(te_key));
      (mykeys+c)->keydata.code = (UWORD)-1;

      for(count = 0;count != c;count++)
      {
        if((mykeys+count)->keydata.code >= 500)
        {
            UBYTE  RAW[4];
            UBYTE  code = (mykeys+count)->keydata.code-500;

          MapANSI(&code, 1, RAW, 1, NULL);
          (mykeys+count)->keydata.code = RAW[0];
          (mykeys+count)->keydata.qual |= RAW[1];
          if(RAW[0] == 67 && !((mykeys+count)->keydata.qual & IEQUALIFIER_NUMERICPAD))
          {
            (mykeys+count)->keydata.code = 68;
          }
        }
      }
    }
  }
}

void  FreeConfig  (struct mydata *data, struct MUI_RenderInfo *mri)
{
  if(data->RawkeyBindings)
    MyFreePooled(data->mypool, data->RawkeyBindings);

/*  if(data->allocatedpens & 1)
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
  }*/
}

