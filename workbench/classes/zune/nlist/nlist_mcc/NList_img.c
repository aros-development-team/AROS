/***************************************************************************

 NList.mcc - New List MUI Custom Class
 Registered MUI class, Serial Number: 1d51 0x9d510030 to 0x9d5100A0
                                           0x9d5100C0 to 0x9d5100FF

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2014 NList Open Source Team

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
#include <proto/utility.h>
#include <proto/intuition.h>

#include "private.h"

#include "NList_img.h"

struct MUI_CustomClass *NLI_Class = NULL;

static IPTR mNLI_Draw(struct IClass *cl,Object *obj,struct MUIP_Draw *msg)
{
  struct NLIData *data = INST_DATA(cl,obj);
  if (data->DoDraw)
    DoSuperMethodA(cl,obj,(Msg) msg);

  return(0);
}


static IPTR mNLI_New(struct IClass *cl,Object *obj,struct opSet *msg)
{
  struct NLIData *data;

  if (!(obj = (Object *)DoSuperMethodA(cl,obj,(Msg) msg)))
    return(0);

  data = INST_DATA(cl,obj);
  data->DoDraw = FALSE;

  return((IPTR) obj);
}


static IPTR mNLI_Set(struct IClass *cl,Object *obj,Msg msg)
{
  struct NLIData *data = INST_DATA(cl,obj);
  struct TagItem *tags,*tag;

  for(tags=((struct opSet *)msg)->ops_AttrList;(tag=(struct TagItem *) NextTagItem((APTR)&tags));)
  {
    switch (tag->ti_Tag)
    {
      case MUIA_Image_Spec:
        tag->ti_Tag = TAG_IGNORE;
        data->DoDraw = TRUE;
        MUI_Redraw(obj,MADF_DRAWOBJECT);
        data->DoDraw = FALSE;
        break;
      case MUIA_FillArea:
      case MUIA_Font:
      case MUIA_Background:
        tag->ti_Tag = TAG_IGNORE;
        break;
    }
  }
  return (DoSuperMethodA(cl,obj,msg));
}

DISPATCHER(NLI_Dispatcher)
{
  switch (msg->MethodID)
  {
    case OM_NEW           : return (  mNLI_New(cl,obj,(APTR)msg));
    case MUIM_HandleInput : return (0);
    case OM_SET           : return (  mNLI_Set(cl,obj,(APTR)msg));
    case MUIM_Draw        : return ( mNLI_Draw(cl,obj,(APTR)msg));
  }
  return(DoSuperMethodA(cl,obj,msg));
}

struct MUI_CustomClass *NLI_Create(void)
{
  NLI_Class = MUI_CreateCustomClass(NULL, (STRPTR)MUIC_Image, NULL, sizeof(struct NLIData), ENTRY(NLI_Dispatcher));
  return (NLI_Class);
}


void NLI_Delete(void)
{
  if (NLI_Class)
    MUI_DeleteCustomClass(NLI_Class);
  NLI_Class = NULL;
}

