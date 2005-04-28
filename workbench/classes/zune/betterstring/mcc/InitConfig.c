/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by BetterString.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 BetterString class Support Site:  http://www.sf.net/projects/bstring-mcc/

 $Id: InitConfig.c,v 1.1 2005/04/21 20:52:04 damato Exp $

***************************************************************************/

#include <stdio.h>
#include <string.h>

#include <clib/alib_protos.h>
#include <libraries/mui.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/diskfont.h>
#include <proto/dos.h>

#include "BetterString_mcp.h"
#include "private.h"

ULONG GetCol (Object *obj, ULONG item, struct MUI_PenSpec *defaultcol, struct InstData *data)
{
	ULONG res;
	struct MUI_PenSpec *spec;

	if(DoMethod(obj, MUIM_GetConfigItem, item, &spec))
			res = MUI_ObtainPen(muiRenderInfo(obj), spec, 0L);
	else	res = MUI_ObtainPen(muiRenderInfo(obj), defaultcol, 0L);

	return res;
}

void InitConfig (Object *obj, struct InstData *data)
{
		LONG  setting;

#ifndef __AROS__
	if((data->Flags & FLG_SetFrame) && MUIMasterBase->lib_Version >= 20)
		set(obj, MUIA_Frame, DoMethod(obj, MUIM_GetConfigItem, MUICFG_BetterString_Frame, &setting) ? (STRPTR)setting : (STRPTR)"302211");
#else
#warning "AROS: FIXME, MUIA_Frame stuff!"
#endif
	data->InactiveText = GetCol(obj, MUICFG_BetterString_InactiveText, (struct MUI_PenSpec *)"m5", data);
	data->ActiveText = GetCol(obj, MUICFG_BetterString_ActiveText, (struct MUI_PenSpec *)"m5", data);
	data->CursorColor = GetCol(obj, MUICFG_BetterString_Cursor, (struct MUI_PenSpec *)"m0", data);
	data->MarkedColor = GetCol(obj, MUICFG_BetterString_MarkedBack, (struct MUI_PenSpec *)"m6", data);
	data->MarkedTextColor = GetCol(obj, MUICFG_BetterString_MarkedText, (struct MUI_PenSpec *)"m5", data);

	if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_BetterString_InactiveBack, &setting))
			data->InactiveBackground = (STRPTR)setting;
	else	data->InactiveBackground = (STRPTR)MUII_BACKGROUND;

	if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_BetterString_ActiveBack, &setting))
			data->ActiveBackground = (STRPTR)setting;
	else	data->ActiveBackground = (STRPTR)"2:m1";

	if(!(data->Flags & FLG_OwnFont) && DoMethod(obj, MUIM_GetConfigItem, MUICFG_BetterString_Font, &setting))
	{
			STRPTR	src = (STRPTR)setting;
			UBYTE		fontname[40];
			struct	TextAttr myfont = { fontname, 8, FS_NORMAL, 0 };
			LONG		c = 0;

		while(src[c] != '/' && src[c] != '\0' && c < 32)
		{
			fontname[c-1] = src[c++];
		}
		strncpy(&fontname[c], ".font", 6);
		StrToLong(&src[c+1], &c);
		myfont.ta_YSize = c;

		data->Font = OpenDiskFont(&myfont);
	}
	else
	{
		data->Font = NULL;
	}

	if(!(data->Flags & FLG_OwnBackground))
		set(obj, MUIA_Background, data->InactiveBackground);
}

VOID FreeConfig (struct MUI_RenderInfo *mri, struct InstData *data)
{
	MUI_ReleasePen(mri, data->InactiveText);
	MUI_ReleasePen(mri, data->ActiveText);
	MUI_ReleasePen(mri, data->CursorColor);
	MUI_ReleasePen(mri, data->MarkedColor);
	MUI_ReleasePen(mri, data->MarkedTextColor);

	if(data->Font)
		CloseFont(data->Font);
}

