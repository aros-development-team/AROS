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

#include <string.h>

#include <clib/alib_protos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/diskfont.h>
#include <proto/dos.h>

#include "BetterString_mcp.h"
#include "private.h"

#include "Debug.h"

static ULONG GetCol(Object *obj, ULONG item, struct MUI_PenSpec *defaultcol)
{
  struct MUI_PenSpec *spec;

  if(DoMethod(obj, MUIM_GetConfigItem, item, &spec) == 0)
    spec = defaultcol;

  return MUI_ObtainPen(muiRenderInfo(obj), spec, 0L);
}

void InitConfig(Object *obj, struct InstData *data)
{
  IPTR setting;

  ENTER();

  data->InactiveText = GetCol(obj, MUICFG_BetterString_InactiveText, (struct MUI_PenSpec *)CFG_BetterString_InactiveText_Def);
  data->ActiveText = GetCol(obj, MUICFG_BetterString_ActiveText, (struct MUI_PenSpec *)CFG_BetterString_ActiveText_Def);
  data->CursorColor = GetCol(obj, MUICFG_BetterString_Cursor, (struct MUI_PenSpec *)CFG_BetterString_Cursor_Def);
  data->MarkedColor = GetCol(obj, MUICFG_BetterString_MarkedBack, (struct MUI_PenSpec *)CFG_BetterString_MarkedBack_Def);
  data->MarkedTextColor = GetCol(obj, MUICFG_BetterString_MarkedText, (struct MUI_PenSpec *)CFG_BetterString_MarkedText_Def);

  if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_BetterString_InactiveBack, &setting))
  {
    // don't remember the string from the configuration but copy it
    // with MUI4's realtime prefs the configuration might change upon
    // canceling MUI prefs and hence we would operate on invalid data.
    strlcpy(data->InactiveBackgroundBuffer, (STRPTR)setting, sizeof(data->InactiveBackgroundBuffer));
    data->InactiveBackground = data->InactiveBackgroundBuffer;
  }
  else
    data->InactiveBackground = (STRPTR)CFG_BetterString_InactiveBack_Def;

  if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_BetterString_ActiveBack, &setting))
  {
    // don't remember the string from the configuration but copy it
    // with MUI4's realtime prefs the configuration might change upon
    // canceling MUI prefs and hence we would operate on invalid data.
    strlcpy(data->ActiveBackgroundBuffer, (STRPTR)setting, sizeof(data->ActiveBackgroundBuffer));
    data->ActiveBackground = data->ActiveBackgroundBuffer;
  }
  else
    data->ActiveBackground = (STRPTR)CFG_BetterString_ActiveBack_Def;

  if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_BetterString_SelectOnActive, &setting))
    data->SelectOnActive = *(IPTR*)setting;
  else
    data->SelectOnActive = CFG_BetterString_SelectOnActive_Def;

  if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_BetterString_SelectPointer, &setting))
    data->SelectPointer = *(IPTR*)setting;
  else
    data->SelectPointer = CFG_BetterString_SelectPointer_Def;

  if(isFlagSet(data->Flags, FLG_OwnBackground))
    set(obj, MUIA_Background, data->OwnBackground);
  else if(data->mui4x == TRUE)
    set(obj, MUIA_Background, MUII_StringBack);
  else
    set(obj, MUIA_Background, data->InactiveBackground);

  LEAVE();
}

VOID FreeConfig(struct MUI_RenderInfo *mri, struct InstData *data)
{
  ENTER();

  MUI_ReleasePen(mri, data->InactiveText);
  MUI_ReleasePen(mri, data->ActiveText);
  MUI_ReleasePen(mri, data->CursorColor);
  MUI_ReleasePen(mri, data->MarkedColor);
  MUI_ReleasePen(mri, data->MarkedTextColor);

  LEAVE();
}

