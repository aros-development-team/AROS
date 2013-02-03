/***************************************************************************

 NList.mcc - New List MUI Custom Class
 Registered MUI class, Serial Number: 1d51 0x9d510030 to 0x9d5100A0
                                           0x9d5100C0 to 0x9d5100FF

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2005 by NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

#include <clib/alib_protos.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>

#include "private.h"

#include "NList_grp.h"

struct MUI_CustomClass *NGR_Class = NULL;

static IPTR mNGR_New(struct IClass *cl,Object *obj,struct opSet *msg)
{
  register struct NGRData *data;

  if (!(obj = (Object *)DoSuperMethodA(cl,obj,(Msg) msg)))
    return(0);

  data = INST_DATA(cl,obj);
  data->DoDraw = FALSE;

  return((IPTR) obj);
}

/*
 * static ULONG mNGR_Draw(struct IClass *cl,Object *obj,struct MUIP_Draw *msg)
 * {
 *   register struct NGRData *data = INST_DATA(cl,obj);
 *
 * D(bug("%lx|group|draw0 %lx %lx \n",obj,msg->flags,muiAreaData(obj)->mad_Flags));
 *
 *   if (data->DoDraw)
 *     DoSuperMethodA(cl,obj,(Msg) msg);
 *
 * D(bug("%lx|group|draw1 %lx %lx \n",obj,msg->flags,muiAreaData(obj)->mad_Flags));
 *
 *   return(0);
 * }
 */

/*
static ULONG mNGR_Set(struct IClass *cl,Object *obj,Msg msg)
{
  register struct NGRData *data = INST_DATA(cl,obj);
  struct TagItem *tags,*tag;

  for(tags = ((struct opSet *)msg)->ops_AttrList;(tag = (struct TagItem *) NextTagItem(&tags));)
  {
    switch (tag->ti_Tag)
    { case MUIA_NList_Visible :
        data->DoDraw = tag->ti_Data;
    }
  }
  return (0);
}
*/

static IPTR mNGR_AskMinMax(struct IClass *cl,Object *obj,struct MUIP_AskMinMax *msg)
{
//  register struct NLData *data = INST_DATA(cl,obj);
/*D(bug("%lx|grp_AskMinMax() 1 \n",obj));*/
  DoSuperMethodA(cl,obj,(Msg) msg);
/*D(bug("%lx|grp_AskMinMax() 2 \n",obj));*/
  msg->MinMaxInfo->MinWidth  = 2;
  msg->MinMaxInfo->DefWidth  = 20;
  msg->MinMaxInfo->MaxWidth  = MUI_MAXMAX+100;
  msg->MinMaxInfo->MinHeight = 3;
  msg->MinMaxInfo->DefHeight = 20+100;
  msg->MinMaxInfo->MaxHeight = MUI_MAXMAX+100;
/*
 *   msg->MinMaxInfo->MinWidth  += MUI_MAXMAX;
 *   msg->MinMaxInfo->DefWidth  += MUI_MAXMAX;
 *   msg->MinMaxInfo->MaxWidth  += MUI_MAXMAX;
 *   msg->MinMaxInfo->MinHeight += MUI_MAXMAX;
 *   msg->MinMaxInfo->DefHeight += MUI_MAXMAX;
 *   msg->MinMaxInfo->MaxHeight += MUI_MAXMAX;
 * {
 * LONG w = (LONG) msg->MinMaxInfo->DefWidth;
 * LONG h = (LONG) msg->MinMaxInfo->DefHeight;
 * LONG w1 = (LONG) msg->MinMaxInfo->MinWidth;
 * LONG h1 = (LONG) msg->MinMaxInfo->MinHeight;
 * D(bug("%lx|group|w=%ld h=%ld (%ld,%ld) \n",obj,w,h,w1,h1));
 * }
 */
  return(0);
}

DISPATCHER(NGR_Dispatcher)
{
  switch (msg->MethodID)
  {
    case OM_NEW           : return (mNGR_New(cl,obj,(APTR)msg));
    case MUIM_AskMinMax   : return (mNGR_AskMinMax(cl,obj,(APTR)msg));
    case OM_SET           : return (0);
    case MUIM_Draw        : return (0);
    case MUIM_HandleInput : return (0);
    case MUIM_NoNotifySet : return (0);
  }
  return(DoSuperMethodA(cl,obj,msg));
}

struct MUI_CustomClass *NGR_Create(void)
{
  NGR_Class = MUI_CreateCustomClass(NULL, (STRPTR)MUIC_Group, NULL, sizeof(struct NGRData), ENTRY(NGR_Dispatcher));

  return (NGR_Class);
}


void NGR_Delete(void)
{
  if (NGR_Class)
    MUI_DeleteCustomClass(NGR_Class);
  NGR_Class = NULL;
}

