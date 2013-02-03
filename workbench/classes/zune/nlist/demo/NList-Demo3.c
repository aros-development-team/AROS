/***************************************************************************

 NList.mcc - New List MUI Custom Class
 Registered MUI class, Serial Number: 1d51 0x9d510030 to 0x9d5100A0
                                           0x9d5100C0 to 0x9d5100FF

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2006 by NList Open Source Team

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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dos/dos.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/io.h>
#include <libraries/dos.h>
#include <libraries/dosextens.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <workbench/workbench.h>
#include <intuition/intuition.h>
#include <intuition/classusr.h>
#include <graphics/gfxmacros.h>
#undef GetOutlinePen

#if !defined(__amigaos4__)
#include <clib/alib_protos.h>
#endif

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/asl.h>
#include <proto/intuition.h>

extern struct Library *MUIMasterBase;

#include <mui/NListview_mcc.h>
#include <mui/NFloattext_mcc.h>

#include "NList-Demo3.h"

#include <proto/muimaster.h>

#include "SDI_hook.h"

/* *********************************************** */


struct MUI_CustomClass *NLI_Class = NULL;


/* *********************************************** */


struct NLIData
{
  LONG special;
  LONG EntryCurrent;
  LONG EntryHeight;
};


/* *********************************************** */


IPTR mNLI_Draw(struct IClass *cl,Object *obj,struct MUIP_Draw *msg)
{
  register struct NLIData *data = INST_DATA(cl,obj);
  DoSuperMethodA(cl,obj,(Msg) msg);
  if ((msg->flags & MADF_DRAWOBJECT) || (msg->flags & MADF_DRAWUPDATE))
  { WORD x1,x2,x3,x4,x5,y1,y2,y3,y4,y5;
    y1 = _top(obj);
    y2 = _bottom(obj);
    x1 = _left(obj);
    x2 = _right(obj);
    if ((data->special == 0) || (data->special == 1))
    {
      y3 = (y1+y2)/2;
      x3 = (x1+x2)/2;
      SetAPen(_rp(obj),_pens(obj)[MPEN_MARK]);
      SetBPen(_rp(obj),_pens(obj)[MPEN_SHADOW]);
      SetDrMd(_rp(obj),JAM2);
      SetDrPt(_rp(obj),(UWORD) ~0);
      if      (data->special == 0)
      { Move(_rp(obj), x3-2, y1+1);
        Draw(_rp(obj), x3-2, y2-1);
        Move(_rp(obj), x3, y1+1);
        Draw(_rp(obj), x3, y2-1);
        Move(_rp(obj), x3+2, y1+1);
        Draw(_rp(obj), x3+2, y2-1);
      }
      else if (data->special == 1)
      { Move(_rp(obj), x1, y3-2);
        Draw(_rp(obj), x2, y3-2);
        Move(_rp(obj), x1, y3);
        Draw(_rp(obj), x2, y3);
        Move(_rp(obj), x1, y3+2);
        Draw(_rp(obj), x2, y3+2);
      }
      SetAPen(_rp(obj),_pens(obj)[MPEN_SHADOW]);
      Move(_rp(obj), x1, y2-1);
      Draw(_rp(obj), x1, y1+1);
      Draw(_rp(obj), x2, y1+1);
      SetAPen(_rp(obj),_pens(obj)[MPEN_SHINE]);
      Draw(_rp(obj), x2, y2-1);
      Draw(_rp(obj), x1, y2-1);
      SetDrMd(_rp(obj),JAM1);
    }
    else if (((x2 - x1) >= 10) && ((y2 - y1) >= 8))   /* and special==2 to 9 */
    {
      y3 = (y1+y2)/2;
      x3 = x1 + 1;
      x2--;
      SetAPen(_rp(obj),_pens(obj)[MPEN_SHADOW]);
      SetDrMd(_rp(obj),JAM1);

      y4 = y1;
      x4 = x3 + 2;
      y5 = y2;
      x5 = x2-6;
      if ((data->EntryHeight & 1) && (data->EntryCurrent & 1))
        y4++;
      if ((y4 & 1) != (y3 & 1))
        x4--;
      if (data->special > 5)
        x5 = x2;
      if (data->special & 1)
        y5 = y3;
      while (y4 <= y5)
      { WritePixel(_rp(obj), x3, y4);
        y4 += 2;
      }
      if (data->special <= 7)
      {
        while (x4 <= x5)
        { WritePixel(_rp(obj), x4, y3);
          x4 += 2;
        }
      }
      if (data->special <= 5)
      {
        Move(_rp(obj), x2-6, y3);
        Draw(_rp(obj), x2-6, y3-3);
        Draw(_rp(obj),   x2, y3-3);
        Draw(_rp(obj),   x2, y3+3);
        Draw(_rp(obj), x2-6, y3+3);
        Draw(_rp(obj), x2-6, y3);
        Move(_rp(obj), x2-4, y3);
        Draw(_rp(obj), x2-2, y3);
        if ((data->special == 2) || (data->special == 3))
        { Move(_rp(obj), x2-3, y3-1);
          Draw(_rp(obj), x2-3, y3+1);
        }
      }
    }
  }
  msg->flags = 0;
  return(0);
}


IPTR mNLI_New(struct IClass *cl,Object *obj,struct opSet *msg)
{
  register struct NLIData *data;
  if (!(obj = (Object *)DoSuperMethodA(cl,obj,(Msg) msg)))
    return(0);
  data = INST_DATA(cl,obj);
  data->special = 0;
  return((IPTR) obj);
}


IPTR mNLI_AskMinMax(struct IClass *cl,Object *obj,struct MUIP_AskMinMax *msg)
{
  DoSuperMethodA(cl,obj,(Msg) msg);
  msg->MinMaxInfo->MinWidth  += 8;
  msg->MinMaxInfo->DefWidth  += 18; /* the only width def value really used by NList object */
  msg->MinMaxInfo->MaxWidth  += MUI_MAXMAX;
  msg->MinMaxInfo->MinHeight += 7;  /* the only height def value really used by NList object */
  msg->MinMaxInfo->DefHeight += 12;
  msg->MinMaxInfo->MaxHeight += MUI_MAXMAX;
  return(0);
}


IPTR mNLI_Set(struct IClass *cl,Object *obj,Msg msg)
{
  register struct NLIData *data = INST_DATA(cl,obj);
  struct TagItem *tags,*tag;

  for(tags=((struct opSet *)msg)->ops_AttrList; (tag=(struct TagItem *)NextTagItem((APTR)&tags)); )
  {
    switch (tag->ti_Tag)
    {
      case MUIA_NLIMG_EntryCurrent:
        data->EntryCurrent = tag->ti_Data;
        break;
      case MUIA_NLIMG_EntryHeight:
        data->EntryHeight = tag->ti_Data;
        break;
      case MUIA_NLIMG_Spec:
        data->special = tag->ti_Data;
        break;
    }
  }
  return (0);
}

DISPATCHER(NLI_Dispatcher)
{
  switch (msg->MethodID)
  {
    case OM_NEW         : return (      mNLI_New(cl,obj,(APTR)msg));
    case OM_SET         : return (      mNLI_Set(cl,obj,(APTR)msg));
    case MUIM_AskMinMax : return (mNLI_AskMinMax(cl,obj,(APTR)msg));
    case MUIM_Draw      : return (     mNLI_Draw(cl,obj,(APTR)msg));
  }

  return(DoSuperMethodA(cl,obj,msg));
}




struct MUI_CustomClass *NLI_Create(void)
{
  NLI_Class = MUI_CreateCustomClass(NULL, (STRPTR)MUIC_Area, NULL, sizeof(struct NLIData), ENTRY(NLI_Dispatcher));

  return (NLI_Class);
}


void NLI_Delete(void)
{
  if (NLI_Class)
    MUI_DeleteCustomClass(NLI_Class);
  NLI_Class = NULL;
}


