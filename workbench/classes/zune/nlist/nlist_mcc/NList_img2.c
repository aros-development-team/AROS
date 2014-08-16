#if 0 /* unused? */
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

#include "private.h"

#include "NList_img.h"
#include "NList_img2.h"

struct MUI_CustomClass *NLI2_Class = NULL;

static ULONG mNLI2_Draw(struct IClass *cl,Object *obj,struct MUIP_Draw *msg)
{
  struct NLIData *data = INST_DATA(cl,obj);
  if (data->DoDraw)
    DoSuperMethodA(cl,obj,(Msg) msg);

  return(0);
}


static ULONG mNLI2_New(struct IClass *cl,Object *obj,struct opSet *msg)
{
  struct NLIData *data;

  if (!(obj = (Object *)DoSuperMethodA(cl,obj,(Msg) msg)))
    return(0);

  data = INST_DATA(cl,obj);
  data->DoDraw = FALSE;

  return((ULONG) obj);
}


static ULONG mNLI2_Set(struct IClass *cl,Object *obj,Msg msg)
{
  struct NLIData *data = INST_DATA(cl,obj);
  struct TagItem *tags,*tag;

  for (tags=((struct opSet *)msg)->ops_AttrList;(tag=(struct TagItem *) NextTagItem(&tags));)
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

DISPATCHER(NLI2_Dispatcher)
{
  switch (msg->MethodID)
  {
    case OM_NEW        : return (  mNLI2_New(cl,obj,(APTR)msg));
    case OM_SET        : return (  mNLI2_Set(cl,obj,(APTR)msg));
    case MUIM_Draw     : return ( mNLI2_Draw(cl,obj,(APTR)msg));
  }
  return(DoSuperMethodA(cl,obj,msg));
}
extern DISPATCHERPROTO(NLI_Dispatcher);
struct MUI_CustomClass *NLI2_Create(void)
{
  NLI2_Class = MUI_CreateCustomClass(NULL, MUIC_Image, NULL, sizeof(struct NLIData), ENTRY(NLI_Dispatcher));
  //NLI2_Class = MUI_CreateCustomClass(NULL, MUIC_Image, NULL, sizeof(struct NLIData), ENTRY(NLI2_Dispatcher));
  return (NLI2_Class);
}


void NLI2_Delete(void)
{
  if (NLI2_Class)
    MUI_DeleteCustomClass(NLI2_Class);
  NLI2_Class = NULL;
}
#endif /* 0 */
