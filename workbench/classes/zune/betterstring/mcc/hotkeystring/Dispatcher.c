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

#include <clib/alib_protos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <libraries/mui.h>

#include "private.h"
#include "version.h"

IPTR Setup(struct IClass *cl, Object *obj, struct MUI_RenderInfo *rinfo)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
  IPTR result;

  if((result = DoSuperMethodA(cl, obj, (Msg)rinfo)))
  {
    data->EventNode.ehn_Priority  = 1;
    data->EventNode.ehn_Flags    = 0;
    data->EventNode.ehn_Object    = obj;
    data->EventNode.ehn_Class    = cl;
    data->EventNode.ehn_Events    = IDCMP_RAWKEY;
    DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->EventNode);
  }
  return result;
}

IPTR Cleanup (struct IClass *cl, Object *obj, Msg msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);

  DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->EventNode);
  return DoSuperMethodA(cl, obj, msg);
}

VOID Set (struct IClass *cl, Object *obj, struct opSet *msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
  struct TagItem *tag;

  if((tag = FindTagItem(MUIA_HotkeyString_Snoop, msg->ops_AttrList)))
  {
    if(tag->ti_Data)
      setFlag(data->Flags, FLG_Snoop);
    else
      clearFlag(data->Flags, FLG_Snoop);

    if(isFlagSet(data->Flags, FLG_Active))
      set(_win(obj), MUIA_Window_DisableKeys, isFlagSet(data->Flags, FLG_Snoop) ? 0xffffff : 0);
  }

  if((tag = FindTagItem(MUIA_HotkeyString_IX, msg->ops_AttrList)))
  {
    struct InputXpression  *ix  = (struct InputXpression *)tag->ti_Data;
    struct IntuiMessage    imsg;

    if (ix && ix->ix_Class == IECLASS_RAWKEY)
    {
      struct MUIP_HandleEvent msg = { 0, &imsg, MUIKEY_NONE };

      imsg.Class    = IDCMP_RAWKEY;
      imsg.Code    = ix->ix_Code;
      imsg.Qualifier  = ix->ix_Qualifier;

      HandleInput(cl, obj, &msg);
    }
  }
}

IPTR Get (struct IClass *cl, Object *obj, struct opGet *msg)
{
  IPTR ti_Data;

  switch(msg->opg_AttrID)
  {
    case MUIA_Version:
      ti_Data = LIB_VERSION;
    break;

    case MUIA_Revision:
      ti_Data = LIB_REVISION;
    break;

    default:
      return DoSuperMethodA(cl, obj, (Msg)msg);
    break;
  }
  *msg->opg_Storage = ti_Data;
  return TRUE;
}

DISPATCHER(_Dispatcher)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
  IPTR  result = 0;

  switch(msg->MethodID)
  {
    case OM_NEW:
    {
      if((obj = (Object *)(result = DoSuperMethodA(cl, obj, msg))))
      {
        struct InstData *data = (struct InstData *)INST_DATA(cl, obj);

        setFlag(data->Flags, FLG_Snoop);
        Set(cl, obj, (struct opSet *)msg);
      }
    }
    break;

    case MUIM_Setup:
      result = Setup(cl, obj, (struct MUI_RenderInfo *)msg);
    break;

    case OM_SET:
      Set(cl, obj, (struct opSet *)msg);
      result = DoSuperMethodA(cl, obj, msg);
    break;

    case OM_GET:
      result = Get(cl, obj, (struct opGet *)msg);
    break;

    case MUIM_HandleEvent:
    {
      struct MUIP_HandleEvent *hmsg = (struct MUIP_HandleEvent *)msg;

      if(isFlagSet(data->Flags, FLG_Active) && isFlagSet(data->Flags, FLG_Snoop) && hmsg->imsg)
        result = HandleInput(cl, obj, hmsg);
    }
    break;

    case MUIM_GoActive:
    {
      setFlag(data->Flags, FLG_Active);
      if(isFlagSet(data->Flags, FLG_Snoop))
        set(_win(obj), MUIA_Window_DisableKeys, 0xffffffff);
      result = DoSuperMethodA(cl, obj, msg);
    }
    break;

    case MUIM_GoInactive:
    {
      clearFlag(data->Flags, FLG_Active);
      if(isFlagSet(data->Flags, FLG_Snoop))
        set(_win(obj), MUIA_Window_DisableKeys, 0);
      result = DoSuperMethodA(cl, obj, msg);
    }
    break;

    case MUIM_Cleanup:
      result = Cleanup(cl, obj, msg);
    break;

    default:
      result = DoSuperMethodA(cl, obj, msg);
    break;
  }
  return result;
}
