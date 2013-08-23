/***************************************************************************

 NBitmap.mcc - New Bitmap MUI Custom Class
 Copyright (C) 2006 by Daniel Allsopp
 Copyright (C) 2007-2013 by NList Open Source Team

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

/* ansi includes */
#include <string.h>

/* system includes */
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/locale.h>
#include <mui/NBitmap_mcc.h>

/* local includes */
#include "NBitmap.h"
#include "private.h"

#include "Debug.h"

/* Object *DoSuperNew() */
#if !defined(__MORPHOS__)
#ifdef __AROS__
static __attribute__ ((noinline)) Object * VARARGS68K DoSuperNew(struct IClass *cl, Object *obj, Tag tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE_AS(tag1, Object *)
    retval = (Object *)DoSuperMethod(cl, obj, OM_NEW, AROS_SLOWSTACKTAGS_ARG(tag1), NULL);
    AROS_SLOWSTACKTAGS_POST
}
#else
static Object * VARARGS68K DoSuperNew(struct IClass *cl, Object *obj, ...)
{
  Object *rc;
  VA_LIST args;

  ENTER();

  VA_START(args, obj);
  rc = (Object *)DoSuperMethod(cl, obj, OM_NEW, VA_ARG(args, ULONG), NULL);
  VA_END(args);

  RETURN(rc);
  return rc;
}
#endif
#endif // !__MORPHOS__

/* VOID setstr() */
static VOID setstr(STRPTR *dest, STRPTR str)
{
  ENTER();

  if(*dest != NULL)
  {
    FreeVec(*dest);
    *dest = NULL;
  }

  if(str != NULL)
  {
    ULONG len;

    len = strlen(str) + 1;
    if((*dest = AllocVecShared(len, MEMF_ANY)) != NULL)
      strlcpy(*dest, str, len);
  }

  LEAVE();
}

/* ULONG NBitmap_New() */
static IPTR NBitmap_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
  if((obj = (Object *)DoSuperNew(cl, obj,
    MUIA_CycleChain, TRUE,
    //MUIA_FillArea, FALSE,
    MUIA_Font, MUIV_Font_Tiny,
    TAG_MORE, msg->ops_AttrList)) != NULL)
  {
    struct InstData *data = NULL;

    if((data = INST_DATA(cl, obj)) != NULL)
    {
      // assume a valid data type for the moment
      BOOL typeOk = TRUE;
      uint32 i;
      struct TagItem *tags = NULL, *tag = NULL;

      memset(data, 0, sizeof(struct InstData));

      data->type = MUIV_NBitmap_Type_File;
      data->alpha = 0xffffffffUL;

      // passed tags
      for(tags = ((struct opSet *)msg)->ops_AttrList; (tag = NextTagItem((APTR)&tags)) != NULL; )
      {
        switch(tag->ti_Tag)
        {
          case MUIA_NBitmap_Type:
            data->type = tag->ti_Data;
          break;

          case MUIA_NBitmap_Normal:
            data->data[0] = (uint32*)tag->ti_Data;
          break;

          case MUIA_NBitmap_Ghosted:
            data->data[1] = (uint32*)tag->ti_Data;
          break;

          case MUIA_NBitmap_Selected:
            data->data[2] = (uint32*)tag->ti_Data;
          break;

          case MUIA_NBitmap_Button:
            data->button = (BOOL)tag->ti_Data;
          break;

          case MUIA_NBitmap_Label:
            setstr(&data->label, (STRPTR)tag->ti_Data);
          break;

          case MUIA_NBitmap_Width:
            data->width = tag->ti_Data;
          break;

          case MUIA_NBitmap_Height:
            data->height = tag->ti_Data;
          break;

          case MUIA_NBitmap_CLUT:
            data->clut = (const uint32 *)tag->ti_Data;
          break;

          case MUIA_NBitmap_Alpha:
            data->alpha = tag->ti_Data;
          break;
        }
      }

      // load files
      switch(data->type)
      {
        case MUIV_NBitmap_Type_File:
        {
          for(i=0; i<3; i++)
          {
            if(data->data[i] != NULL)
              NBitmap_LoadImage((STRPTR)data->data[i], i, cl, obj);
          }
        }
        break;

        case MUIV_NBitmap_Type_DTObject:
        {
          for(i=0; i<3; i++)
          {
            if(data->data[i] != NULL)
              data->dt_obj[i] = (Object *)data->data[i];
          }
        }
        break;

        case MUIV_NBitmap_Type_CLUT8:
        case MUIV_NBitmap_Type_RGB24:
        case MUIV_NBitmap_Type_ARGB32:
        {
          // nothing to do here
        }
        break;

        default:
        {
          // this is an invalid type
          typeOk = FALSE;
        }
        break;
      }

      // setup images
      if(typeOk == TRUE && NBitmap_NewImage(cl, obj) == TRUE)
        return (IPTR)obj;
    }
  }

  CoerceMethod(cl, obj, OM_DISPOSE);
  return (IPTR)NULL;;
}

/* ULONG NBitmap_Get() */
static ULONG NBitmap_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
  ULONG result = FALSE;
  IPTR *store = msg->opg_Storage;
  struct InstData *data = INST_DATA(cl, obj);

  ENTER();

  switch (((struct opGet *)msg)->opg_AttrID)
  {
    case MUIA_NBitmap_Width:
      *store = (LONG) data->width;
      result = TRUE;
    break;

    case MUIA_NBitmap_Height:
      *store = (LONG) data->height;
      result = TRUE;
    break;

    case MUIA_NBitmap_MaxWidth:
      *store = (LONG) data->maxwidth;
      result = TRUE;
    break;

    case MUIA_NBitmap_MaxHeight:
      *store = (LONG) data->maxheight;
      result = TRUE;
    break;
  }

  if(result == FALSE)
    result = DoSuperMethodA(cl, obj, (Msg)msg);

  RETURN(result);
  return result;
}

/* ULONG NBitmap_Set() */
static IPTR NBitmap_Set(struct IClass *cl,Object *obj, struct opSet *msg)
{
  struct InstData *data = INST_DATA(cl, obj);
  struct TagItem *tags, *tag;
  IPTR result;

  ENTER();

  for(tags = msg->ops_AttrList; (tag = NextTagItem((APTR)&tags)) != NULL; )
  {
    switch(tag->ti_Tag)
    {
      case MUIA_NBitmap_Normal:
      {
        if(data->type == MUIV_NBitmap_Type_File)
        {
          data->data[0] = (uint32*)tag->ti_Data;
          NBitmap_UpdateImage(0, (STRPTR)data->data[0], cl, obj);
          MUI_Redraw(obj, MADF_DRAWOBJECT);
        }

        tag->ti_Tag = TAG_IGNORE;
      }
      break;

      case MUIA_NBitmap_Ghosted:
      {
        if(data->type == MUIV_NBitmap_Type_File)
        {
          data->data[1] = (uint32*)tag->ti_Data;
          NBitmap_UpdateImage(1, (STRPTR)data->data[1], cl, obj);
          MUI_Redraw(obj, MADF_DRAWOBJECT);
        }

        tag->ti_Tag = TAG_IGNORE;
      }
      break;

      case MUIA_NBitmap_Selected:
      {
        if(data->type == MUIV_NBitmap_Type_File)
        {
          data->data[2] = (uint32*)tag->ti_Data;
          NBitmap_UpdateImage(2, (STRPTR)data->data[2], cl, obj);
          MUI_Redraw(obj, MADF_DRAWOBJECT);
        }

        tag->ti_Tag = TAG_IGNORE;
      }
      break;

      case MUIA_NBitmap_MaxWidth:
        data->maxwidth = (uint32)tag->ti_Data;
        tag->ti_Tag = TAG_IGNORE;
      break;

      case MUIA_NBitmap_MaxHeight:
        data->maxheight = (uint32)tag->ti_Data;
        tag->ti_Tag = TAG_IGNORE;
      break;
    }
  }

  result = DoSuperMethodA(cl, obj, (Msg)msg);

  RETURN(result);
  return result;
}

/* ULONG NBitmap_Dispose() */
static IPTR NBitmap_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
  IPTR result;

  ENTER();

  NBitmap_DisposeImage(cl, obj);

  result = DoSuperMethodA(cl, obj, msg);

  RETURN(result);
  return result;
}

/* ULONG NBitmap_Setup() */
static IPTR NBitmap_Setup(struct IClass *cl, Object *obj, Msg msg)
{
  IPTR result;

  ENTER();

  if((result = DoSuperMethodA(cl, obj, msg)) != 0)
  {
    result = NBitmap_SetupImage(cl, obj);
  }

  RETURN(result);
  return result;
}

/* ULONG NBitmap_Cleanup() */
static IPTR NBitmap_Cleanup(struct IClass *cl, Object *obj, Msg msg)
{
  IPTR result;

  ENTER();

  NBitmap_CleanupImage(cl, obj);

  result = DoSuperMethodA(cl, obj, msg);

  RETURN(result);
  return(result);
}

/* ULONG NBitmap_HandleEvent() */
static IPTR NBitmap_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
  struct InstData *data;
  IPTR result;

  //ENTER();

  result = 0;

  if((data = INST_DATA(cl, obj)) != NULL)
  {
    if(msg->imsg != NULL)
    {
      switch(msg->imsg->Class)
      {
        case IDCMP_MOUSEBUTTONS:
        {
          if(_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
          {
            if(msg->imsg->Code == SELECTDOWN)
            {
              if(data->pressed == FALSE)
              {
                data->pressed = TRUE;

                MUI_Redraw(obj, MADF_DRAWUPDATE);
                SetAttrs(obj, MUIA_Pressed, TRUE, TAG_DONE);
              }
            }
            else if(msg->imsg->Code == SELECTUP)
            {
              if(data->overlay && data->pressed)
              {
                data->pressed = FALSE;
                data->overlay = FALSE;

                MUI_Redraw(obj, MADF_DRAWUPDATE);
                SetAttrs(obj, MUIA_Pressed, FALSE, TAG_DONE);
              }
            }

            result = MUI_EventHandlerRC_Eat;
          }
          else
          {
            if(msg->imsg->Code==SELECTUP)
            {
              data->pressed = FALSE;
            }
          }
        }
        break;

        case IDCMP_MOUSEMOVE:
        {
          if(_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
          {
            if(data->overlay == FALSE)
            {
              data->overlay = TRUE;

              MUI_Redraw(obj, MADF_DRAWUPDATE);
            }

            //result = MUI_EventHandlerRC_Eat;
          }
          else
          {
            if(data->overlay)
            {
              data->overlay = FALSE;

              MUI_Redraw(obj, MADF_DRAWUPDATE);
            }
          }
        }
        break;
      }
    }
  }

  if(result == 0)
    result = DoSuperMethodA(cl, obj, (Msg)msg);

  //RETURN(result);
  return result;
}

/* ULONG  NBitmap_AskMinMax() */
static IPTR NBitmap_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
  struct InstData *data;

  ENTER();

  DoSuperMethodA(cl, obj, (Msg)msg);

  if((data = INST_DATA(cl, obj)) != NULL)
  {
    uint32 oldBorderHoriz = data->border_horiz;
    uint32 oldBorderVert = data->border_vert;

    // spacing
    data->border_horiz = 0;
    data->border_vert = 0;

    if(data->button)
    {
      data->border_horiz += 4;
      data->border_horiz += data->prefs.spacing_horiz * 2;

      data->border_vert += 4;
      data->border_vert += data->prefs.spacing_vert * 2;
    }

    /* label */
    data->label_horiz = 0;
    data->label_vert = 0;

    if(data->label != NULL && data->button != FALSE)
    {
      struct RastPort rp;

      memcpy(&rp, &_screen(obj)->RastPort, sizeof(rp));

      SetFont(&rp, _font(obj));
      TextExtent(&rp, (STRPTR)data->label, strlen(data->label), &data->labelte);

      if(data->width < (uint32)data->labelte.te_Width)
        data->border_horiz += (data->labelte.te_Width - data->width);

      data->label_vert = data->labelte.te_Height;
      data->border_vert += data->labelte.te_Height;
      data->border_vert += 2;
    }

    // standard width & height
    msg->MinMaxInfo->MinWidth = data->width + data->border_horiz;
    msg->MinMaxInfo->DefWidth = data->width + data->border_horiz;
    msg->MinMaxInfo->MaxWidth = data->width + data->border_horiz;

    msg->MinMaxInfo->MinHeight = data->height + data->border_vert;
    msg->MinMaxInfo->DefHeight = data->height + data->border_vert;
    msg->MinMaxInfo->MaxHeight = data->height + data->border_vert;

    if(data->border_horiz != oldBorderHoriz || data->border_vert != oldBorderVert)
    {
      if(data->depth > 8) {
        // the border size has changed and we must recalculate the shade arrays
        NBitmap_CleanupShades(data);
        NBitmap_SetupShades(data);
      }
    }
  }

  RETURN(0);
  return 0;
}

/* ULONG NBitmap_Draw() */
static IPTR NBitmap_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
  IPTR result;

  ENTER();

  result = DoSuperMethodA(cl, obj, (Msg)msg);

  if((msg->flags & MADF_DRAWUPDATE) || (msg->flags & MADF_DRAWOBJECT))
  {
    NBitmap_DrawImage(cl, obj);
  }

  RETURN(result);
  return result;
}

/* DISPATCHER() */
DISPATCHER(_Dispatcher)
{
  IPTR result;

  //ENTER();

  switch(msg->MethodID)
  {
    case OM_NEW:
      result = NBitmap_New(cl, obj, (struct opSet *)msg);
    break;

    case OM_SET:
      result = NBitmap_Set(cl, obj, (struct opSet *)msg);
    break;

    case OM_GET:
      result = NBitmap_Get(cl, obj, (struct opGet *)msg);
    break;

    case OM_DISPOSE:
      result = NBitmap_Dispose(cl, obj, msg);
    break;

    case MUIM_Setup:
      result = NBitmap_Setup(cl, obj, msg);
    break;

    case MUIM_HandleEvent:
      result = NBitmap_HandleEvent(cl, obj, (struct MUIP_HandleEvent *)msg);
    break;

    case MUIM_Cleanup:
      result = NBitmap_Cleanup(cl, obj, msg);
    break;

    case MUIM_AskMinMax:
      result = NBitmap_AskMinMax(cl, obj, (struct MUIP_AskMinMax *)msg);
    break;

    case MUIM_Draw:
      result = NBitmap_Draw(cl, obj, (struct MUIP_Draw *)msg);
    break;

    default:
      result = DoSuperMethodA(cl, obj, msg);
    break;
  }

  //RETURN(result);
  return result;
}
