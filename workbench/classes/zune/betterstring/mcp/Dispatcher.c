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

#include <string.h>
#include <clib/alib_protos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>

#include "private.h"
#include "BetterString_mcp.h"
#include "muiextra.h"

#include "Debug.h"

struct PrefsExchangeData
{
  const ULONG ObjIndex;
  const ULONG Tag;
  const ULONG CfgItem;
  const ULONG Type;      // 0 = string, 1 = integer
  const ULONG Length;
  const APTR  DefValue;
};

static struct PrefsExchangeData PrefsInfo[] =
{
  { ActiveBack,     MUIA_Imagedisplay_Spec,  MUICFG_BetterString_ActiveBack,     0, 64,           (APTR)"2:m1"    },
  { ActiveText,     MUIA_Pendisplay_Spec,    MUICFG_BetterString_ActiveText,     0, 32,           (APTR)"m5"      },
  { InactiveBack,   MUIA_Imagedisplay_Spec,  MUICFG_BetterString_InactiveBack,   0, 64,           (APTR)"2:m2"    },
  { InactiveText,   MUIA_Pendisplay_Spec,    MUICFG_BetterString_InactiveText,   0, 32,           (APTR)"m4"      },
  { Cursor,         MUIA_Pendisplay_Spec,    MUICFG_BetterString_Cursor,         0, 32,           (APTR)"m0"      },
  { MarkedBack,     MUIA_Pendisplay_Spec,    MUICFG_BetterString_MarkedBack,     0, 32,           (APTR)"m6"      },
  { MarkedText,     MUIA_Pendisplay_Spec,    MUICFG_BetterString_MarkedText,     0, 32,           (APTR)"m5"      },
  { Font,           MUIA_String_Contents,    MUICFG_BetterString_Font,           0,  0,           (APTR)""        },
  { Frame,          MUIA_Framedisplay_Spec,  MUICFG_BetterString_Frame,          0, 32,           (APTR)"302211"  },
  { SelectOnActive, MUIA_Selected,           MUICFG_BetterString_SelectOnActive, 1, sizeof(LONG), (APTR)0         },
  { SelectPointer,  MUIA_Selected,           MUICFG_BetterString_SelectPointer,  1, sizeof(LONG), (APTR)1         }
};

DISPATCHER(_DispatcherP)
{
  struct InstData_MCP *data = (struct InstData_MCP *)INST_DATA(cl, obj);
  ULONG result = 0;

  ENTER();

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

          // This is MUI 3.9 stuff: Each registered object will get a context-menu, like normal pref-items
          if(MUIMasterBase->lib_Version >= 20)
          {
            for(i=0; i < NumberOfObject; i++)
              DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->Objects[PrefsInfo[i].ObjIndex], PrefsInfo[i].CfgItem, 0L, NULL);
          }

          result = (ULONG)obj;
        }
        else
          CoerceMethod(cl, obj, OM_DISPOSE);
      }
    }
    break;

    case MUIM_Settingsgroup_ConfigToGadgets:
    {
      ULONG i;
      Object *configdata = ((struct MUIP_Settingsgroup_ConfigToGadgets *)msg)->configdata;

      for(i=0; i < NumberOfObject; i++)
      {
        APTR cfg_val;

        cfg_val = (APTR)DoMethod(configdata, MUIM_Dataspace_Find, PrefsInfo[i].CfgItem);

        if(PrefsInfo[i].Type == 0)
        {
          W(DBF_STARTUP, "0 MUIM_Dataspace_Find[%ld]: %08lx : %lx / %lx", i, PrefsInfo[i].CfgItem, cfg_val, cfg_val ? cfg_val : PrefsInfo[i].DefValue);

          set(data->Objects[PrefsInfo[i].ObjIndex], PrefsInfo[i].Tag, cfg_val ? cfg_val : PrefsInfo[i].DefValue);
        }
        else
        {
          W(DBF_STARTUP, "1 MUIM_Dataspace_Find[%ld]: %08lx : %lx / %lx", i, PrefsInfo[i].CfgItem, cfg_val, cfg_val ? *(ULONG *)cfg_val : (ULONG)PrefsInfo[i].DefValue);

          set(data->Objects[PrefsInfo[i].ObjIndex], PrefsInfo[i].Tag, cfg_val ? *(ULONG *)cfg_val : (ULONG)PrefsInfo[i].DefValue);
        }
      }
    }
    break;

    case MUIM_Settingsgroup_GadgetsToConfig:
    {
      ULONG i;
      Object *configdata = ((struct MUIP_Settingsgroup_ConfigToGadgets *)msg)->configdata;

      for(i=0; i < NumberOfObject; i++)
      {
        LONG cfg_val;
        ULONG len;

        cfg_val = xget(data->Objects[PrefsInfo[i].ObjIndex], PrefsInfo[i].Tag);
        len = PrefsInfo[i].Length;

        if(PrefsInfo[i].Type == 0)
        {
          W(DBF_STARTUP, "0 MUIM_Dataspace_Add[%ld]: %08lx : %lx", i, PrefsInfo[i].CfgItem, cfg_val);
          DoMethod(configdata, MUIM_Dataspace_Add, cfg_val, len > 0 ? len : strlen((char *)cfg_val)+1, PrefsInfo[i].CfgItem);
        }
        else
        {
          W(DBF_STARTUP, "1 MUIM_Dataspace_Add[%ld]: %08lx : %lx / %lx", i, PrefsInfo[i].CfgItem, &cfg_val, cfg_val);
          DoMethod(configdata, MUIM_Dataspace_Add, &cfg_val, len, PrefsInfo[i].CfgItem);
        }
      }
    }
    break;

    default:
      result = DoSuperMethodA(cl, obj, msg);
    break;
  }

  RETURN(result);
  return result;
}
