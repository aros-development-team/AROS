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

 $Id: Dispatcher.c,v 1.1 2005/04/21 20:52:04 damato Exp $

***************************************************************************/

#include <string.h>
#include <clib/alib_protos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>

#include "private.h"
#include "BetterString_mcp.h"

#ifndef __AROS__
#include "muiextra.h"
#else
#define MUIM_Mccprefs_RegisterGadget  0x80424828 // V20
#endif

struct PrefsExchangeData { ULONG ObjIndex, Tag, CfgItem, Length; STRPTR DefValue; };

static struct PrefsExchangeData PrefsInfo[] =
{
	{ ActiveBack,		MUIA_Imagedisplay_Spec,	MUICFG_BetterString_ActiveBack,		64, "2:m1"		},
	{ ActiveText,		MUIA_Pendisplay_Spec,	  MUICFG_BetterString_ActiveText,		32, "m5"			},
	{ InactiveBack,	MUIA_Imagedisplay_Spec,	MUICFG_BetterString_InactiveBack,	64, "2:m2"		},
	{ InactiveText,	MUIA_Pendisplay_Spec,	  MUICFG_BetterString_InactiveText,	32, "m5"			},
	{ Cursor,			  MUIA_Pendisplay_Spec,	  MUICFG_BetterString_Cursor,			  32, "m0"			},
	{ MarkedBack,		MUIA_Pendisplay_Spec,	  MUICFG_BetterString_MarkedBack,		32, "m6"			},
	{ MarkedText,		MUIA_Pendisplay_Spec,	  MUICFG_BetterString_MarkedText,		32, "m5"			},
	{ Font,				  MUIA_String_Contents,	  MUICFG_BetterString_Font,				  0,  ""			  },
	{ Frame,				MUIA_Framedisplay_Spec, MUICFG_BetterString_Frame,				32, "302211"	}
};

DISPATCHERPROTO(_DispatcherP)
{
    	DISPATCHER_INIT
	
	struct InstData_MCP *data = (struct InstData_MCP *)INST_DATA(cl, obj);
	ULONG result = 0;
	switch(msg->MethodID)
	{
		case OM_NEW:
		{
			if((obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg)))
			{
				struct InstData_MCP *data = (struct InstData_MCP *)INST_DATA(cl, obj);

				Object *prefsobject;
				if((prefsobject = CreatePrefsGroup(data)))
				{
					ULONG i;

          DoMethod(obj, OM_ADDMEMBER, prefsobject);

					/* This is MUI 3.9 stuff: Each registered object will get a context-menu, like normal pref-items */
					for(i=0; i < NumberOfObject; i++)
						DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->Objects[PrefsInfo[i].ObjIndex], PrefsInfo[i].CfgItem, 0L, NULL);

					result = (ULONG)obj;
				}
				else
				{
					CoerceMethod(cl, obj, OM_DISPOSE);
				}
			}
		}
		break;

		case MUIM_Settingsgroup_ConfigToGadgets:
		{
			ULONG i;
      Object *configdata = ((struct MUIP_Settingsgroup_ConfigToGadgets *)msg)->configdata;
			
      for(i=0; i < NumberOfObject; i++)
			{
				STRPTR cfg_val = (STRPTR)DoMethod(configdata, MUIM_Dataspace_Find, PrefsInfo[i].CfgItem);
				set(data->Objects[PrefsInfo[i].ObjIndex], PrefsInfo[i].Tag, cfg_val ? cfg_val : PrefsInfo[i].DefValue);
			}
		}
		break;

		case MUIM_Settingsgroup_GadgetsToConfig:
		{
			ULONG i;
      Object *configdata = ((struct MUIP_Settingsgroup_ConfigToGadgets *)msg)->configdata;

			for(i=0; i < NumberOfObject; i++)
			{
				STRPTR cfg_val;
				ULONG len;

#ifdef __AROS__
        get(data->Objects[PrefsInfo[i].ObjIndex], PrefsInfo[i].Tag, &cfg_val);
#else
        get(data->Objects[PrefsInfo[i].ObjIndex], PrefsInfo[i].Tag, (ULONG)&cfg_val);
#endif
				len = PrefsInfo[i].Length;
				
        DoMethod(configdata, MUIM_Dataspace_Add, cfg_val, len ? len : strlen(cfg_val)+1, PrefsInfo[i].CfgItem);
			}
		}
		break;

		default:
			result = DoSuperMethodA(cl, obj, msg);
		break;
	}
	return result;
	
	DISPATCHER_EXIT
}
