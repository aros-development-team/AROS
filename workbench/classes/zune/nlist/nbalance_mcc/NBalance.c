/***************************************************************************

 NBalance.mcc - New Balance MUI Custom Class
 Copyright (C) 2008 by NList Open Source Team

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

// ansi includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// system
#include <proto/exec.h>
#include <proto/layers.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <libraries/mui.h>
#include <mui/NBalance_mcc.h>

// local includes
#include "NBalance.h"
#include "Pointer.h"
#include "private.h"
#include "version.h"
#include "Debug.h"
#include "muiextra.h"

// functions

// methods
IPTR mNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
  BOOL result = FALSE;
  ENTER();

  if((obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg)) != NULL)
  {
    struct InstData *data;

    if((data = INST_DATA(cl, obj)) != NULL)
    {
      struct TagItem *tags = NULL;
      struct TagItem *tag = NULL;

      // set some default values
      data->pointerType = MUIV_NBalance_Pointer_Standard;

      for(tags=((struct opSet *)msg)->ops_AttrList;(tag = NextTagItem((APTR)&tags)); )
      {
        switch(tag->ti_Tag)
        {
          case MUIA_NBalance_Pointer:
            data->pointerType = tag->ti_Data;
          break;
        }
      }

      result = TRUE;
    }
  }

  if(result == FALSE)
  {
    CoerceMethod(cl, obj, OM_DISPOSE);
    obj = NULL;
  }

  RETURN((IPTR)obj);
  return (IPTR)obj;
}

IPTR mGet(struct IClass *cl, Object *obj, Msg msg)
{
  IPTR result = FALSE;
  IPTR *store = ((struct opGet *)msg)->opg_Storage;
  struct InstData *data = INST_DATA(cl, obj);

  ENTER();

  switch(((struct opGet *)msg)->opg_AttrID)
  {
    case MUIA_NBalance_Pointer:
      *store = (LONG)data->pointerType;
      result = TRUE;
    break;

    case MUIA_Version:
      *store = LIB_VERSION;
      result = TRUE;
    break;

    case MUIA_Revision:
      *store = LIB_REVISION;
      result = TRUE;
    break;
  }

  if(result == FALSE)
    result = DoSuperMethodA(cl,obj,msg);

  RETURN(result);
  return result;
}

IPTR mSet(struct IClass *cl,Object *obj, Msg msg)
{
  struct InstData *data = INST_DATA(cl, obj);
  struct TagItem *tags, *tag;
  IPTR result = FALSE;

  ENTER();

  for(tags=((struct opSet *)msg)->ops_AttrList;(tag = NextTagItem((APTR)&tags)); )
  {
    switch(tag->ti_Tag)
    {
      case MUIA_NBalance_Pointer:
      {
        data->pointerType = (LONG)tag->ti_Data;

        // hide the mouse pointer if necessary
        if(data->pointerType == MUIV_NBalance_Pointer_Off &&
           data->mouseSelectDown == TRUE)
        {
          D(DBF_INPUT, "mSet: HIDEPTR %ld", xget(obj, MUIA_ObjectID));

          data->mouseSelectDown = FALSE;
          HideCustomPointer(obj, data);
        }

        tag->ti_Tag = TAG_IGNORE;
      }
      break;
    }
  }

  result = DoSuperMethodA(cl, obj, msg);

  RETURN(result);
  return result;
}


IPTR mSetup(struct IClass *cl, Object *obj, struct MUI_RenderInfo *rinfo)
{
  IPTR result = FALSE;
  struct InstData *data = INST_DATA(cl, obj);

  ENTER();

  if(DoSuperMethodA(cl, obj, (Msg)rinfo))
  {
    Object *parent = (Object *)xget(obj, MUIA_Parent);

    // find out if the parent object is a horizontal or vertical group
    data->groupType = xget(parent, MUIA_Group_Type);

    // setup all pointers
    SetupCustomPointers(data);

    // create/add an event handler to the window the object belongs to
    data->ehnode.ehn_Priority = -2;
    data->ehnode.ehn_Flags    = MUI_EHF_GUIMODE;
    data->ehnode.ehn_Object   = obj;
    data->ehnode.ehn_Class    = cl;
    data->ehnode.ehn_Events   = IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE;
    DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehnode);

    result = TRUE;
  }

  RETURN(result);
  return result;
}

IPTR mCleanup(struct IClass *cl, Object *obj, Msg msg)
{
  IPTR result = 0;
  struct InstData *data = INST_DATA(cl, obj);

  ENTER();

  // cleanup the selection pointer
  CleanupCustomPointers(data);

  DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode);

  result = DoSuperMethodA(cl, obj, msg);

  RETURN(result);
  return result;
}

IPTR mHide(struct IClass *cl, Object *obj, Msg msg)
{
  struct InstData *data = INST_DATA(cl, obj);
  IPTR result;

  ENTER();

  if(data->mouseSelectDown == TRUE && data->mouseOverObject == FALSE)
  {
    D(DBF_INPUT, "mHide: HIDEPTR %ld", xget(obj, MUIA_ObjectID));

    data->mouseSelectDown = FALSE;
    HideCustomPointer(obj, data);
  }

  result = DoSuperMethodA(cl, obj, msg);

  RETURN(result);
  return result;
}

IPTR mHandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
  IPTR result = 0;
  struct InstData *data;

  ENTER();

  if((data = INST_DATA(cl, obj)) != NULL &&
     msg->imsg != NULL && data->pointerType != MUIV_NBalance_Pointer_Off)
  {
    struct IntuiMessage *imsg = msg->imsg;

    switch(imsg->Class)
    {
      case IDCMP_MOUSEMOVE:
      {
        BOOL isOverObject = FALSE;

        if(_isinobject(imsg->MouseX, imsg->MouseY))
        {
          struct Layer_Info *li = &(_screen(obj)->LayerInfo);
          struct Layer *layer;

          // get the layer that belongs to the current mouse coordinates
          LockLayerInfo(li);
          layer = WhichLayer(li, _window(obj)->LeftEdge + imsg->MouseX, _window(obj)->TopEdge + imsg->MouseY);
          UnlockLayerInfo(li);

          // if the mouse is currently over the object and over the object's
          // window we go and change the pointer to show the selection pointer
          if(layer != NULL && layer->Window == _window(obj))
            isOverObject = TRUE;
        }

        D(DBF_INPUT, "mouse move object %ld, %ld->%ld", xget(obj, MUIA_ObjectID), data->mouseOverObject, isOverObject);

        if(isOverObject == TRUE && data->mouseOverObject == FALSE)
        {
          D(DBF_INPUT, "SHOWPTR %ld, %ld->%ld", xget(obj, MUIA_ObjectID), data->mouseOverObject, isOverObject);

          ShowCustomPointer(obj, data);
          data->mouseOverObject = TRUE;
        }
        else if(isOverObject == FALSE && data->mouseOverObject == TRUE)
        {
          if(data->ignoreNextHidePointer == FALSE)
          {
            D(DBF_INPUT, "HIDEPTR %ld, %ld->%ld", xget(obj, MUIA_ObjectID), data->mouseOverObject, isOverObject);
            HideCustomPointer(obj, data);
            data->mouseOverObject = FALSE;
          }
          else
            data->ignoreNextHidePointer = FALSE;
        }
      }
      break;

      case IDCMP_MOUSEBUTTONS:
      {
        if(imsg->Code == SELECTDOWN)
        {
          if(_isinobject(imsg->MouseX, imsg->MouseY))
          {
            D(DBF_INPUT, "SELECTDOWN %ld, %ld", xget(obj, MUIA_ObjectID), data->mouseOverObject);
            data->mouseSelectDown = TRUE;
            data->ignoreNextHidePointer = TRUE;
            ShowCustomPointer(obj, data);
          }
        }
        else if(data->mouseSelectDown == TRUE)
        {
          D(DBF_INPUT, "SELECTUP %ld, %ld", xget(obj, MUIA_ObjectID), data->mouseOverObject);
          data->mouseSelectDown = FALSE;
          HideCustomPointer(obj, data);
        }
      }
      break;
    }
  }

  RETURN(result);
  return result;
}

