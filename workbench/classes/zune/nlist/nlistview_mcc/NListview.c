/***************************************************************************

 NListview.mcc - New Listview MUI Custom Class
 Registered MUI class, Serial Number: 1d51 (0x9d510020 to 0x9d51002F)

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

#include <string.h>

#include <clib/alib_protos.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/keymap.h>

#include "private.h"
#include "version.h"

#include "NListviews_mcp.h"

#include "muiextra.h"

#include "Debug.h"

#ifndef MUI_EHF_GUIMODE
#define MUI_EHF_GUIMODE     (1<<1)  /* set this if you dont want your handler to be called */
                                    /* when your object is disabled or invisible */
#endif

static LONG IMsgToChar(struct IntuiMessage *imsg, ULONG dccode, ULONG dcquali)
{
  struct InputEvent ie;
  LONG key = -1;

  ENTER();

  memset(&ie, 0, sizeof(struct InputEvent));
  ie.ie_Class = IECLASS_RAWKEY;

  if(imsg->Class == IDCMP_RAWKEY)
  {
    TEXT buf[4];

    ie.ie_Code         = imsg->Code & ~dccode;
    ie.ie_Qualifier    = imsg->Qualifier & ~dcquali;
    ie.ie_EventAddress = (APTR *)*((IPTR *)imsg->IAddress);

    #if defined(__amigaos4__)
    ie.ie_TimeStamp.Seconds = imsg->Seconds;
    ie.ie_TimeStamp.Microseconds = imsg->Micros;
    #else
    ie.ie_TimeStamp.tv_secs = imsg->Seconds;
    ie.ie_TimeStamp.tv_micro = imsg->Micros;
    #endif

    if(MapRawKey(&ie, buf , 3, 0) > 0)
      key = (LONG)buf[0];
  }

  RETURN(key);
  return key;
}

static IPTR mNLV_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
  struct NLVData *data = INST_DATA(cl, obj);
  struct IntuiMessage *imsg = msg->imsg;
  IPTR result = 0;

  ENTER();

  if(data->LI_NList != NULL)
  {
    #if !defined(__amigaos4__) && !defined(__MORPHOS__) && !defined(__AROS__)
    // with MUI 3.8 the embedded list object doesn't get this method,
    // hence we must forward it ourself.
    if(MUIMasterBase != NULL && MUIMasterBase->lib_Version <= 19 && msg->muikey != MUIKEY_NONE)
    {
      result = DoMethodA(data->LI_NList, (Msg)msg);
    }
    #endif
    if(imsg != NULL)
    {
      switch(imsg->Class)
      {
        case IDCMP_RAWKEY:
        {
          if(data->ControlChar != '\0' && IMsgToChar(imsg, 0, (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)) == data->ControlChar)
          {
            set(data->LI_NList, MUIA_NList_Active, imsg->Qualifier & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT) ? MUIV_NList_Active_Up: MUIV_NList_Active_Down);
          }
        }
        break;
      }
    }
  }

  RETURN(result);
  return result;
}

static IPTR mNLV_HandleInput(struct IClass *cl, Object *obj, struct MUIP_HandleInput *msg)
{
  struct NLVData *data = INST_DATA(cl, obj);
  IPTR result = 0;

  ENTER();

  if(data->LI_NList != NULL && msg->muikey != MUIKEY_NONE)
    result = DoMethodA(data->LI_NList, (Msg)msg);
  else
    result = DoSuperMethodA(cl, obj, (Msg)msg);

  RETURN(result);
  return result;
}

static void AddVerticalScroller(Object *obj, struct NLVData *data)
{
  ENTER();

  if(data->Vert_Attached == FALSE)
  {
    D(DBF_STARTUP, "adding vertical scrollbar");

    DoMethod(obj, OM_ADDMEMBER, data->PR_Vert);

    // add notifications
    DoMethod(data->LI_NList, MUIM_Notify, MUIA_NList_Prop_Entries,    MUIV_EveryTime,  data->PR_Vert,  3, MUIM_NoNotifySet, MUIA_Prop_Entries, MUIV_TriggerValue);
    DoMethod(data->LI_NList, MUIM_Notify, MUIA_NList_Prop_Visible,    MUIV_EveryTime,  data->PR_Vert,  3, MUIM_NoNotifySet, MUIA_Prop_Visible, MUIV_TriggerValue);
    DoMethod(data->LI_NList, MUIM_Notify, MUIA_NList_Prop_First,      MUIV_EveryTime,  data->PR_Vert,  3, MUIM_NoNotifySet, MUIA_Prop_First, MUIV_TriggerValue);
    DoMethod(data->PR_Vert,  MUIM_Notify, MUIA_Prop_First,            MUIV_EveryTime,  data->LI_NList, 3, MUIM_NoNotifySet, MUIA_NList_Prop_First, MUIV_TriggerValue);
    DoMethod(data->LI_NList, MUIM_Notify, MUIA_NList_VertDeltaFactor, MUIV_EveryTime,  data->PR_Vert,  3, MUIM_NoNotifySet, MUIA_Prop_DeltaFactor, MUIV_TriggerValue);

    data->Vert_Attached = TRUE;

    D(DBF_STARTUP, "vertical scrollbar added");
  }

  LEAVE();
}

static void RemoveVerticalScroller(Object *obj, struct NLVData *data)
{
  ENTER();

  if(data->Vert_Attached == TRUE)
  {
    D(DBF_STARTUP, "removing vertical scrollbar");

/*
    // remove notifications
    DoMethod(data->LI_NList, MUIM_KillNotifyObj, MUIA_NList_Prop_Entries,    data->PR_Vert);
    DoMethod(data->LI_NList, MUIM_KillNotifyObj, MUIA_NList_Prop_Visible,    data->PR_Vert);
    DoMethod(data->LI_NList, MUIM_KillNotifyObj, MUIA_NList_Prop_First,      data->PR_Vert);
*/
    DoMethod(data->PR_Vert,  MUIM_KillNotifyObj, MUIA_Prop_First,            data->LI_NList);
/*
    DoMethod(data->LI_NList, MUIM_KillNotifyObj, MUIA_NList_VertDeltaFactor, data->PR_Vert);
*/

    DoMethod(obj, OM_REMMEMBER, data->PR_Vert);

    data->Vert_Attached = FALSE;

    D(DBF_STARTUP, "vertical scrollbar removed");
  }

  LEAVE();
}

static void AddHorizontalScroller(Object *obj, struct NLVData *data)
{
  ENTER();

  if(data->Horiz_Attached == FALSE)
  {
    D(DBF_STARTUP, "adding horizontal scrollbar");

    DoMethod(obj, OM_ADDMEMBER, data->PR_Horiz);

    // add notifications
    DoMethod(data->LI_NList, MUIM_Notify, MUIA_NList_Horiz_Entries,    MUIV_EveryTime, data->PR_Horiz, 3, MUIM_NoNotifySet, MUIA_Prop_Entries, MUIV_TriggerValue);
    DoMethod(data->LI_NList, MUIM_Notify, MUIA_NList_Horiz_Visible,    MUIV_EveryTime, data->PR_Horiz, 3, MUIM_NoNotifySet, MUIA_Prop_Visible, MUIV_TriggerValue);
    DoMethod(data->LI_NList, MUIM_Notify, MUIA_NList_Horiz_First,      MUIV_EveryTime, data->PR_Horiz, 3, MUIM_NoNotifySet, MUIA_Prop_First, MUIV_TriggerValue);
    DoMethod(data->PR_Horiz, MUIM_Notify, MUIA_Prop_First,             MUIV_EveryTime, data->LI_NList, 3, MUIM_NoNotifySet, MUIA_NList_Horiz_First, MUIV_TriggerValue);
    DoMethod(data->LI_NList, MUIM_Notify, MUIA_NList_HorizDeltaFactor, MUIV_EveryTime, data->PR_Horiz, 3, MUIM_NoNotifySet, MUIA_Prop_DeltaFactor, MUIV_TriggerValue);

    data->Horiz_Attached = TRUE;

    D(DBF_STARTUP, "horizontal scrollbar added");
  }

  LEAVE();
}

static void RemoveHorizontalScroller(Object *obj, struct NLVData *data)
{
  ENTER();

  if(data->Horiz_Attached == TRUE)
  {
    D(DBF_STARTUP, "removing horizontal scrollbar");

/*
    // remove notifications
    DoMethod(data->LI_NList, MUIM_KillNotifyObj, MUIA_NList_Horiz_Entries,    data->PR_Horiz);
    DoMethod(data->LI_NList, MUIM_KillNotifyObj, MUIA_NList_Horiz_Visible,    data->PR_Horiz);
    DoMethod(data->LI_NList, MUIM_KillNotifyObj, MUIA_NList_Horiz_First,      data->PR_Horiz);
*/
    DoMethod(data->PR_Horiz, MUIM_KillNotifyObj, MUIA_Prop_First,             data->LI_NList);
/*
    DoMethod(data->LI_NList, MUIM_KillNotifyObj, MUIA_NList_HorizDeltaFactor, data->PR_Horiz);
*/

    DoMethod(obj, OM_REMMEMBER, data->PR_Horiz);

    data->Horiz_Attached = FALSE;

    D(DBF_STARTUP, "horizontal scrollbar removed");
  }

  LEAVE();
}

static void NLV_Scrollers(Object *obj, struct NLVData *data, LONG vert, LONG horiz)
{
  LONG scrollers = 0;

  ENTER();

  if(vert & 0x0F)
  {
    data->Vert_ScrollBar = vert & 0x0F;

    if(data->Vert_ScrollBar == MUIV_NListview_VSB_Default)
    {
      LONG *scrollbar;

      if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_NListview_VSB, (IPTR)&scrollbar))
        data->VertSB = *scrollbar;
      else
        data->VertSB = MUIV_NListview_VSB_Always;
    }
    else
      data->VertSB = data->Vert_ScrollBar;

    switch(data->VertSB)
    {
      case MUIV_NListview_VSB_Always:
      case MUIV_NListview_VSB_Left:
      {
        scrollers |= MUIV_NListview_VSB_On;
      }
      break;

      case MUIV_NListview_VSB_Auto:
      case MUIV_NListview_VSB_FullAuto:
      {
        // nothing
      }
      break;

      case MUIV_NListview_VSB_None:
      {
        scrollers |= MUIV_NListview_VSB_Off;
      }
      break;
    }
  }

  if(horiz & 0x0F)
  {
    data->Horiz_ScrollBar = horiz & 0x0F;

    if(data->Horiz_ScrollBar == MUIV_NListview_HSB_Default)
    {
      LONG *scrollbar;

      if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_NListview_HSB, (IPTR)&scrollbar))
        data->HorizSB = *scrollbar;
      else
        data->HorizSB = DEFAULT_HSB;
    }
    else
      data->HorizSB = data->Horiz_ScrollBar;

    switch (data->HorizSB)
    {
      case MUIV_NListview_HSB_Always:
      {
        scrollers |= MUIV_NListview_HSB_On;
      }
      break;

      case MUIV_NListview_HSB_Auto:
      case MUIV_NListview_HSB_FullAuto:
      {
        // nothing
      }
      break;

      case MUIV_NListview_HSB_None:
      {
        scrollers |= MUIV_NListview_HSB_Off;
      }
      break;
    }
  }

  scrollers |= ((vert | horiz) & (MUIV_NListview_VSB_On | MUIV_NListview_HSB_On));

  if(((data->VertSB == MUIV_NListview_VSB_Auto || data->VertSB == MUIV_NListview_VSB_Always) && (scrollers & MUIV_NListview_VSB_On) == MUIV_NListview_VSB_Off) ||
     (data->VertSB == MUIV_NListview_VSB_None && (scrollers & MUIV_NListview_VSB_On) == MUIV_NListview_VSB_On) ||
     (data->Vert_Attached == TRUE && (scrollers & MUIV_NListview_VSB_On) == MUIV_NListview_VSB_On) ||
     (data->Vert_Attached == FALSE && (scrollers & MUIV_NListview_VSB_On) == MUIV_NListview_VSB_Off))
  {
    scrollers &= ~MUIV_NListview_VSB_On;
  }

  if(((data->HorizSB == MUIV_NListview_HSB_Auto || data->HorizSB == MUIV_NListview_HSB_Always) && (scrollers & MUIV_NListview_HSB_On) == MUIV_NListview_HSB_Off) ||
      (data->HorizSB == MUIV_NListview_HSB_None && (scrollers & MUIV_NListview_HSB_On) == MUIV_NListview_HSB_On) ||
      (data->Horiz_Attached == TRUE && (scrollers & MUIV_NListview_HSB_On) == MUIV_NListview_HSB_On) ||
      (data->Horiz_Attached == FALSE && (scrollers & MUIV_NListview_HSB_On) == MUIV_NListview_HSB_Off))
  {
    scrollers &= ~MUIV_NListview_HSB_On;
  }

  if(scrollers & MUIV_NListview_VSB_On)
  {
    if(data->SETUP == FALSE || DoMethod(obj, MUIM_Group_InitChange))
    {
      if((scrollers & MUIV_NListview_VSB_On) == MUIV_NListview_VSB_On)
      {
        AddVerticalScroller(obj, data);
        if(data->VertSB == MUIV_NListview_VSB_Left)
          DoMethod(obj, MUIM_Group_Sort, data->PR_Vert, data->Group, NULL);
      }
      else
      {
        RemoveVerticalScroller(obj, data);
      }

      if(scrollers & MUIV_NListview_HSB_On)
      {
        if(data->SETUP == FALSE || DoMethod(data->Group, MUIM_Group_InitChange))
        {
          if((scrollers & MUIV_NListview_HSB_On) == MUIV_NListview_HSB_On)
            AddHorizontalScroller(data->Group, data);
          else
            RemoveHorizontalScroller(data->Group, data);

          if(data->SETUP == TRUE)
            DoMethod(data->Group, MUIM_Group_ExitChange);
        }
      }

      if(data->SETUP == TRUE)
        DoMethod(obj, MUIM_Group_ExitChange);
    }
  }
  else if(scrollers & MUIV_NListview_HSB_On)
  {
    if(data->SETUP == FALSE || DoMethod(data->Group, MUIM_Group_InitChange))
    {
      if((scrollers & MUIV_NListview_HSB_On) == MUIV_NListview_HSB_On)
        AddHorizontalScroller(data->Group, data);
      else
        RemoveHorizontalScroller(data->Group, data);

      if(data->SETUP == TRUE)
        DoMethod(data->Group, MUIM_Group_ExitChange);
    }
  }

  LEAVE();
}

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

/* static ULONG mNLV_New(struct IClass *cl,Object *obj,Msg msg) */
static IPTR mNLV_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
  struct NLVData *data;
  struct TagItem *tag;
  Object *nlist = NULL;
  Object *vgroup = NULL;
  BOOL cyclechain = FALSE;
  BOOL dropable = FALSE;

  ENTER();

  if((tag = FindTagItem(MUIA_Draggable, msg->ops_AttrList)) != NULL)
    tag->ti_Tag = TAG_IGNORE;

  if((tag = FindTagItem(MUIA_Dropable, msg->ops_AttrList)) != NULL)
  {
    tag->ti_Tag = TAG_IGNORE;
    dropable = tag->ti_Data;
  }

  if((tag = FindTagItem(MUIA_CycleChain, msg->ops_AttrList)) != NULL)
  {
    tag->ti_Tag = TAG_IGNORE;
    cyclechain = tag->ti_Data;
  }

  if((tag = FindTagItem(MUIA_NListview_NList, msg->ops_AttrList)) != NULL ||
     (tag = FindTagItem(MUIA_Listview_List, msg->ops_AttrList)) != NULL)
  {
    nlist = (Object *) tag->ti_Data;

    if(nlist != NULL)
    {
      if(dropable)
      {
        nnset(nlist,MUIA_Dropable, dropable);
      }

      if((tag = FindTagItem(MUIA_NList_DragType, msg->ops_AttrList)) != NULL ||
         (tag = FindTagItem(MUIA_Listview_DragType, msg->ops_AttrList)) != NULL)
      {
        nnset(nlist,tag->ti_Tag,tag->ti_Data);
      }

      if((tag = FindTagItem(MUIA_Listview_Input, msg->ops_AttrList)) != NULL ||
         (tag = FindTagItem(MUIA_NList_Input, msg->ops_AttrList)) != NULL)
      {
        nnset(nlist,tag->ti_Tag,tag->ti_Data);
      }

      if((tag = FindTagItem(MUIA_Listview_MultiSelect, msg->ops_AttrList)) != NULL ||
         (tag = FindTagItem(MUIA_NList_MultiSelect, msg->ops_AttrList)) != NULL)
      {
        nnset(nlist,tag->ti_Tag,tag->ti_Data);
      }

      if((tag = FindTagItem(MUIA_Listview_DoubleClick, msg->ops_AttrList)) != NULL ||
         (tag = FindTagItem(MUIA_NList_DoubleClick, msg->ops_AttrList)) != NULL)
      {
        nnset(nlist,tag->ti_Tag,tag->ti_Data);
      }

      if((tag = FindTagItem(MUIA_Listview_DefClickColumn, msg->ops_AttrList)) != NULL ||
         (tag = FindTagItem(MUIA_NList_DefClickColumn, msg->ops_AttrList)) != NULL)
      {
        nnset(nlist,tag->ti_Tag,tag->ti_Data);
      }
    }
    else
    {
      obj = NULL;
      RETURN(obj);
      return((IPTR)obj);
    }
  }
  else
  {
    nlist = MUI_NewObject(MUIC_NList, MUIA_Dropable, dropable, TAG_MORE, msg->ops_AttrList);
  }

  obj = (Object *)DoSuperNew(cl, obj,
    MUIA_Group_Horiz, TRUE,
    MUIA_Group_Spacing, 0,
    MUIA_CycleChain, cyclechain,
    NoFrame,
    Child, vgroup = VGroup,
      MUIA_Group_Spacing, 0,
      Child, nlist,
    TAG_DONE),
    TAG_MORE, msg->ops_AttrList
  );

  if(obj)
  {
    data = INST_DATA(cl,obj);
    data->sem = FALSE;
    data->SETUP = FALSE;
    data->LI_NList = nlist;
    data->Group = vgroup;
    data->Vert_Attached = FALSE;
    data->Horiz_Attached = FALSE;
    data->Vert_ScrollBar = MUIV_NListview_VSB_Default;
    data->Horiz_ScrollBar = MUIV_NListview_HSB_Default;
    if((tag = FindTagItem(MUIA_Listview_ScrollerPos, msg->ops_AttrList)) != NULL)
    {
      if(tag->ti_Data == MUIV_Listview_ScrollerPos_None)
      {
        data->Vert_ScrollBar = MUIV_NListview_VSB_None;
        data->Horiz_ScrollBar = MUIV_NListview_HSB_None;
      }
      else if(tag->ti_Data == MUIV_Listview_ScrollerPos_Left)
      {
        data->Vert_ScrollBar = MUIV_NListview_VSB_Left;
      }
      else if(tag->ti_Data == MUIV_Listview_ScrollerPos_Right)
      {
        data->Vert_ScrollBar = MUIV_NListview_VSB_Always;
      }
    }

    if((tag = FindTagItem(MUIA_NListview_Vert_ScrollBar, msg->ops_AttrList)) != NULL)
      data->Vert_ScrollBar = tag->ti_Data;

    if((tag = FindTagItem(MUIA_NListview_Horiz_ScrollBar, msg->ops_AttrList)) != NULL)
      data->Horiz_ScrollBar = tag->ti_Data;

    if((tag = FindTagItem(MUIA_ControlChar, msg->ops_AttrList)) != NULL)
      data->ControlChar = tag->ti_Data;

    data->VertSB = data->Vert_ScrollBar;
    data->HorizSB = data->Horiz_ScrollBar;

    // create the two scrollbar objects
    data->PR_Vert = ScrollbarObject,
      MUIA_Group_Horiz, FALSE,
    End;

    data->PR_Horiz = ScrollbarObject,
      MUIA_Group_Horiz, TRUE,
    End;

    DoMethod(data->LI_NList, MUIM_Notify, MUIA_NListview_Horiz_ScrollBar, MUIV_EveryTime, obj, 3, MUIM_Set, MUIA_NListview_Horiz_ScrollBar, MUIV_TriggerValue);

    set(data->LI_NList, MUIA_NList_KeepActive, (IPTR)obj);

    // derive the "active border visible" setting from the embedded list
    if(xget(data->LI_NList, MUIA_NList_ActiveObjectOnClick) == TRUE)
      _flags(obj) |= (1<<7);
  }

  RETURN(obj);
  return((IPTR)obj);
}


static IPTR mNLV_Dispose(struct IClass *cl,Object *obj,Msg msg)
{
  struct NLVData *data = INST_DATA(cl, obj);

  ENTER();

  // dispose the scrollbars if they are not currently part of the layout
  if(data->PR_Vert != NULL && data->Vert_Attached == FALSE)
  {
    MUI_DisposeObject(data->PR_Vert);
    data->PR_Vert = NULL;
  }
  if(data->PR_Horiz != NULL && data->Horiz_Attached == FALSE)
  {
    MUI_DisposeObject(data->PR_Horiz);
    data->PR_Horiz = NULL;
  }

  LEAVE();
  return DoSuperMethodA(cl,obj,msg);
}

static IPTR mNLV_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
  struct NLVData *data = INST_DATA(cl, obj);
  IPTR result = FALSE;

  ENTER();

  data->SETUP = FALSE;

  if(data->sem == FALSE)
  {
    data->sem = TRUE;
    NLV_Scrollers(obj, data, data->Vert_ScrollBar, data->Horiz_ScrollBar);
    data->sem = FALSE;
  }

  // Add the event handler for RAWKEY now
  data->eh.ehn_Class  = cl;
  data->eh.ehn_Object = obj;
  data->eh.ehn_Events = IDCMP_RAWKEY;
  data->eh.ehn_Flags  = MUI_EHF_GUIMODE;
  data->eh.ehn_Priority = -1;
  if(_win(obj) != NULL)
    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->eh);

  if(DoSuperMethodA(cl, obj, (Msg)msg))
  {
    result = TRUE;
    data->SETUP = TRUE;
  }

  RETURN(result);
  return result;
}


static IPTR mNLV_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
  struct NLVData *data = INST_DATA(cl, obj);

  ENTER();

  if(_win(obj) != NULL)
    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->eh);

  data->SETUP = FALSE;

  LEAVE();
  return DoSuperMethodA(cl, obj, (Msg)msg);
}


static IPTR mNLV_Notify(struct IClass *cl, Object *obj, struct MUIP_Notify *msg)
{
  struct NLVData *data = INST_DATA(cl, obj);

  switch(msg->TrigAttr)
  {
    case MUIA_List_Prop_Entries:
    case MUIA_List_Prop_Visible:
    case MUIA_List_Prop_First:
    case MUIA_NList_Horiz_Entries:
    case MUIA_NList_Horiz_Visible:
    case MUIA_NList_Horiz_First:
    case MUIA_NList_HorizDeltaFactor:
    case MUIA_NList_Prop_Entries:
    case MUIA_NList_Prop_Visible:
    case MUIA_NList_Prop_First:
    case MUIA_NList_VertDeltaFactor:
    case MUIA_NList_SelectChange:
    case MUIA_NList_MultiClick:
    case MUIA_NList_DoubleClick:
    case MUIA_NList_EntryClick:
    case MUIA_NList_Active:
    case MUIA_NList_First:
    case MUIA_NList_Entries:
    case MUIA_NList_TitleClick:
    case MUIA_List_Active:
    case MUIA_Listview_SelectChange:
    case MUIA_Listview_DoubleClick:
    {
      return DoMethodA(data->LI_NList, (Msg)msg);
    }

    default:
      return DoSuperMethodA(cl, obj, (Msg)msg);
  }
}



static IPTR mNLV_Set(struct IClass *cl,Object *obj,Msg msg)
{
  struct NLVData *data = INST_DATA(cl, obj);
  struct TagItem *tags, *tag;

  ENTER();

  tags = ((struct opSet *)msg)->ops_AttrList;
  while((tag = NextTagItem((APTR)&tags)) != NULL)
  {
    switch (tag->ti_Tag)
    {
      case MUIA_Listview_ScrollerPos:
      {
        if(tag->ti_Data == MUIV_Listview_ScrollerPos_None)
          tag->ti_Data = MUIV_NListview_VSB_None;
        else if(tag->ti_Data == MUIV_Listview_ScrollerPos_Left)
          tag->ti_Data = MUIV_NListview_VSB_Left;
        else if(tag->ti_Data == MUIV_Listview_ScrollerPos_Right)
          tag->ti_Data = MUIV_NListview_VSB_Always;
        else
          tag->ti_Data = MUIV_NListview_VSB_Default;
      }
      // fall through...

      case MUIA_NListview_Vert_ScrollBar:
      {
        tag->ti_Tag = TAG_IGNORE;
        if(data->sem == FALSE)
        {
          data->sem = TRUE;
          NLV_Scrollers(obj, data, tag->ti_Data, 0);
          data->sem = FALSE;
        }
      }
      break;

      case MUIA_NListview_Horiz_ScrollBar:
      {
        tag->ti_Tag = TAG_IGNORE;
        if(data->sem == FALSE)
        {
          data->sem = TRUE;
          NLV_Scrollers(obj, data, 0, tag->ti_Data);
          data->sem = FALSE;
        }
      }
      break;

      case MUIA_NList_KeyUpFocus:
      case MUIA_NList_KeyDownFocus:
      case MUIA_NList_KeyLeftFocus:
      case MUIA_NList_KeyRightFocus:
      {
        // forward these to the embedded list object
        if(data->LI_NList != NULL)
          set(data->LI_NList, tag->ti_Tag, tag->ti_Data);
      }
      break;
    }
  }

  LEAVE();
  return DoSuperMethodA(cl, obj, msg);
}


static IPTR mNLV_Get(struct IClass *cl,Object *obj,Msg msg)
{
  struct NLVData *data = INST_DATA(cl, obj);
  IPTR *store = ((struct opGet *)msg)->opg_Storage;
  IPTR result = FALSE;

  ENTER();

  switch(((struct opGet *)msg)->opg_AttrID)
  {
    case MUIA_Listview_List:
    case MUIA_NListview_NList:
    {
      *store = (IPTR)data->LI_NList;
      result = TRUE;
    }
    break;

    case MUIA_NListview_Vert_ScrollBar:
    {
      *store = (IPTR)data->VertSB;
      result = TRUE;
    }
    break;

    case MUIA_NListview_Horiz_ScrollBar:
    {
      *store = (IPTR)data->HorizSB;
      result = TRUE;
    }
    break;

    case MUIA_NListview_VSB_Width:
    {
      if(data->Vert_Attached == TRUE)
        *store = (IPTR)_width(data->PR_Vert);
      else
        *store = (IPTR)0;

      result = TRUE;
    }
    break;

    case MUIA_NListview_HSB_Height:
    {
      if(data->Horiz_Attached == TRUE)
        *store = (IPTR)_height(data->PR_Horiz);
      else
        *store = (IPTR)0;

      result = TRUE;
    }
    break;

    case MUIA_Version:
    {
      *store = LIB_VERSION;
      result = TRUE;
    }
    break;

    case MUIA_Revision:
    {
      *store = LIB_REVISION;
      result = TRUE;
    }
    break;

    default:
    {
      result = DoSuperMethodA(cl, obj, msg);
    }
    break;
  }

  RETURN(result);
  return result;
}

static IPTR mNLV_GoActive(struct IClass *cl, Object *obj, Msg msg)
{
  struct NLVData *data = INST_DATA(cl, obj);

  // forward the method to the NList object
  D(DBF_ALWAYS, "go active %08lx", obj);
  if(data->LI_NList != NULL)
    DoMethod(data->LI_NList, MUIM_NList_GoActive);

  return DoSuperMethodA(cl, obj, msg);
}

static IPTR mNLV_GoInactive(struct IClass *cl, Object *obj, Msg msg)
{
  struct NLVData *data = INST_DATA(cl, obj);

  D(DBF_ALWAYS, "go inactive %08lx", obj);
  // forward the method to the NList object
  if(data->LI_NList != NULL)
    DoMethod(data->LI_NList, MUIM_NList_GoInactive);

  return DoSuperMethodA(cl, obj, msg);
}

DISPATCHER(_Dispatcher)
{
  switch(msg->MethodID)
  {
    case OM_NEW                     : return(mNLV_New(cl,obj,(APTR)msg));
    case OM_DISPOSE                 : return(mNLV_Dispose(cl,obj,(APTR)msg));
    case OM_GET                     : return(mNLV_Get(cl,obj,(APTR)msg));
    case OM_SET                     : return(mNLV_Set(cl,obj,(APTR)msg));
    case MUIM_Setup                 : return(mNLV_Setup(cl,obj,(APTR)msg));
    case MUIM_Cleanup               : return(mNLV_Cleanup(cl,obj,(APTR)msg));
    case MUIM_HandleInput           : return(mNLV_HandleInput(cl,obj,(APTR)msg));
    case MUIM_HandleEvent           : return(mNLV_HandleEvent(cl,obj,(APTR)msg));
    case MUIM_NList_QueryBeginning  : return(0);
    case MUIM_DragQuery             : return(MUIV_DragQuery_Refuse);

    // we catch all notify relevant method
    // calls in one function
    case MUIM_KillNotify            :
    case MUIM_KillNotifyObj         :
    case MUIM_Notify                : return (mNLV_Notify(cl,obj,(APTR)msg));

    case MUIM_GoActive              : return mNLV_GoActive(cl, obj, msg);
    case MUIM_GoInactive            : return mNLV_GoInactive(cl, obj, msg);

    // the following method calls are all forwarded
    // to the corresponding NList object
    case MUIM_List_Sort             :
    case MUIM_List_Insert           :
    case MUIM_List_InsertSingle     :
    case MUIM_List_GetEntry         :
    case MUIM_List_Clear            :
    case MUIM_List_Jump             :
    case MUIM_List_Select           :
    case MUIM_List_TestPos          :
    case MUIM_List_Redraw           :
    case MUIM_List_Exchange         :
    case MUIM_List_Move             :
    case MUIM_List_NextSelected     :
    case MUIM_List_Remove           :
    case MUIM_List_CreateImage      :
    case MUIM_List_DeleteImage      :
    case MUIM_NList_Sort            :
    case MUIM_NList_Sort2           :
    case MUIM_NList_Insert          :
    case MUIM_NList_InsertSingle    :
    case MUIM_NList_GetEntry        :
    case MUIM_NList_Clear           :
    case MUIM_NList_Jump            :
    case MUIM_NList_Select          :
    case MUIM_NList_TestPos         :
    case MUIM_NList_Redraw          :
    case MUIM_NList_RedrawEntry     :
    case MUIM_NList_Exchange        :
    case MUIM_NList_Move            :
    case MUIM_NList_NextSelected    :
    case MUIM_NList_Remove          :
    case MUIM_NList_CreateImage     :
    case MUIM_NList_DeleteImage     :
    case MUIM_NList_CopyToClip      :
    case MUIM_NList_UseImage        :
    case MUIM_NList_ReplaceSingle   :
    case MUIM_NList_InsertWrap      :
    case MUIM_NList_InsertSingleWrap:
    case MUIM_NList_GetEntryInfo    :
    case MUIM_NList_GetSelectInfo   :
    case MUIM_NList_CopyTo          :
    case MUIM_NList_DropType        :
    case MUIM_NList_DropDraw        :
    case MUIM_NList_DoMethod        :
    case MUIM_NList_ColWidth        :
    case MUIM_NList_ColToColumn     :
    case MUIM_NList_ColumnToCol     :
    {
      struct NLVData *data = INST_DATA(cl, obj);

      if(data->LI_NList != NULL)
        return DoMethodA(data->LI_NList, msg);
      else
        return 0;
    }

    default: return DoSuperMethodA(cl, obj, msg);
  }
}
