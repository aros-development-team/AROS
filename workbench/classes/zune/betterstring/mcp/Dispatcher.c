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
  const APTR  DefValue;
};

static const struct PrefsExchangeData PrefsInfo[] =
{
  { ActiveBack,     MUIA_Imagedisplay_Spec,  MUICFG_BetterString_ActiveBack,     0, (APTR)CFG_BetterString_ActiveBack_Def     },
  { ActiveText,     MUIA_Pendisplay_Spec,    MUICFG_BetterString_ActiveText,     0, (APTR)CFG_BetterString_ActiveText_Def     },
  { InactiveBack,   MUIA_Imagedisplay_Spec,  MUICFG_BetterString_InactiveBack,   0, (APTR)CFG_BetterString_InactiveBack_Def   },
  { InactiveText,   MUIA_Pendisplay_Spec,    MUICFG_BetterString_InactiveText,   0, (APTR)CFG_BetterString_InactiveText_Def   },
  { Cursor,         MUIA_Pendisplay_Spec,    MUICFG_BetterString_Cursor,         0, (APTR)CFG_BetterString_Cursor_Def         },
  { MarkedBack,     MUIA_Pendisplay_Spec,    MUICFG_BetterString_MarkedBack,     0, (APTR)CFG_BetterString_MarkedBack_Def     },
  { MarkedText,     MUIA_Pendisplay_Spec,    MUICFG_BetterString_MarkedText,     0, (APTR)CFG_BetterString_MarkedText_Def     },
  { SelectOnActive, MUIA_Selected,           MUICFG_BetterString_SelectOnActive, 1, (APTR)CFG_BetterString_SelectOnActive_Def },
  { SelectPointer,  MUIA_Selected,           MUICFG_BetterString_SelectPointer,  1, (APTR)CFG_BetterString_SelectPointer_Def  }
};

DISPATCHER(_DispatcherP)
{
  struct InstData_MCP *data = (struct InstData_MCP *)INST_DATA(cl, obj);
  IPTR result = 0;

  ENTER();

  switch(msg->MethodID)
  {
    case OM_NEW:
    {
      if((obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg)))
      {
        struct InstData_MCP *data = (struct InstData_MCP *)INST_DATA(cl, obj);
        Object *prefsobject;

        // everything beyond muimaster 20.5500 is considered to be MUI4
        data->mui4x = LIB_VERSION_IS_AT_LEAST(MUIMasterBase, 20, 5500);

        if((prefsobject = CreatePrefsGroup(data)))
        {
          ULONG i;

          DoMethod(obj, OM_ADDMEMBER, prefsobject);

          // This is MUI 3.9 stuff: Each registered object will get a context-menu, like normal pref-items
          if(LIB_VERSION_IS_AT_LEAST(MUIMasterBase, 20, 0))
          {
            for(i=0; i < NumberOfObject; i++)
              DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->Objects[PrefsInfo[i].ObjIndex], PrefsInfo[i].CfgItem, 2, NULL, PrefsInfo[i].Tag);
          }

          result = (IPTR)obj;
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
          D(DBF_STARTUP, "0 MUIM_Dataspace_Find[%ld]: %08lx : %lx / %lx", i, PrefsInfo[i].CfgItem, cfg_val, cfg_val ? cfg_val : PrefsInfo[i].DefValue);

          nnset(data->Objects[PrefsInfo[i].ObjIndex], PrefsInfo[i].Tag, cfg_val ? cfg_val : PrefsInfo[i].DefValue);
        }
        else
        {
          D(DBF_STARTUP, "1 MUIM_Dataspace_Find[%ld]: %08lx : %P / %P", i, PrefsInfo[i].CfgItem, cfg_val, cfg_val ? *(IPTR *)cfg_val : (IPTR)PrefsInfo[i].DefValue);

          nnset(data->Objects[PrefsInfo[i].ObjIndex], PrefsInfo[i].Tag, cfg_val ? *(IPTR *)cfg_val : (IPTR)PrefsInfo[i].DefValue);
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
        SIPTR cfg_val;

        cfg_val = xget(data->Objects[PrefsInfo[i].ObjIndex], PrefsInfo[i].Tag);

        if(PrefsInfo[i].Type == 0)
        {
          if(stricmp((char *)cfg_val, PrefsInfo[i].DefValue) != 0)
          {
            D(DBF_STARTUP, "0 MUIM_Dataspace_Add[%ld]: %08lx : %lx", i, PrefsInfo[i].CfgItem, cfg_val);
            DoMethod(configdata, MUIM_Dataspace_Add, cfg_val, strlen((char *)cfg_val)+1, PrefsInfo[i].CfgItem);
          }
          else
          {
            D(DBF_STARTUP, "0 MUIM_Dataspace_Remove[%ld]: %08lx", i, PrefsInfo[i].CfgItem);
            DoMethod(configdata, MUIM_Dataspace_Remove, PrefsInfo[i].CfgItem);
          }
        }
        else
        {
          if(cfg_val != (SIPTR)PrefsInfo[i].DefValue)
          {
            D(DBF_STARTUP, "1 MUIM_Dataspace_Add[%ld]: %08lx : %lx / %lx", i, PrefsInfo[i].CfgItem, &cfg_val, cfg_val);
            DoMethod(configdata, MUIM_Dataspace_Add, &cfg_val, sizeof(cfg_val), PrefsInfo[i].CfgItem);
          }
          else
          {
            D(DBF_STARTUP, "1 MUIM_Dataspace_Remove[%ld]: %08lx", i, PrefsInfo[i].CfgItem);
            DoMethod(configdata, MUIM_Dataspace_Remove, PrefsInfo[i].CfgItem);
          }
        }
      }

      // erase obsolete settings
      DoMethod(configdata, MUIM_Dataspace_Remove, MUICFG_BetterString_Font);
      DoMethod(configdata, MUIM_Dataspace_Remove, MUICFG_BetterString_Frame);
    }
    break;

    default:
      result = DoSuperMethodA(cl, obj, msg);
    break;
  }

  RETURN(result);
  return result;
}
