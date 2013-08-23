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
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/locale.h>

#include "private.h"

static IPTR mNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
  ENTER();

  if((obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg)) != NULL)
  {
    struct InstData *data = (struct InstData *)INST_DATA(cl, obj);

    if((data->Contents = AllocContentString(40)) != NULL)
    {
      struct TagItem *tag;

      set(obj, MUIA_FillArea, FALSE);

      // muimaster V20 is MUI 3.9
      data->mui39 = LIB_VERSION_IS_AT_LEAST(MUIMasterBase, 20, 0);
      // everything beyond muimaster 20.5500 is considered to be MUI4
      data->mui4x = LIB_VERSION_IS_AT_LEAST(MUIMasterBase, 20, 5500);

      data->locale = OpenLocale(NULL);

      if((tag = FindTagItem(MUIA_Background, msg->ops_AttrList)) != NULL)
      {
        setFlag(data->Flags, FLG_OwnBackground);
        data->OwnBackground = (STRPTR)tag->ti_Data;
      }

      mSet(cl, obj, (struct opSet *)msg);

      data->BufferPos = 0;
    }
    else
    {
      CoerceMethod(cl, obj, OM_DISPOSE);
      obj = NULL;
    }
  }

  RETURN(obj);
  return((IPTR)obj);
}

static IPTR mDispose(struct IClass *cl, Object *obj, Msg msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);

  ENTER();

  if(isFlagSet(data->Flags, FLG_WindowSleepNotifyAdded))
  {
    E(DBF_INPUT, "MUIA_Window_Sleep notify still active at OM_DISPOSE!!");
  }

  FreeContentString(data->Contents);
  data->Contents = NULL;

  FreeContentString(data->Original);
  data->Original = NULL;

  FreeContentString(data->Undo);
  data->Undo = NULL;

  if(data->FNCBuffer != NULL)
  {
    SharedPoolFree(data->FNCBuffer);
    data->FNCBuffer = NULL;
  }

  if(data->locale != NULL)
  {
    CloseLocale(data->locale);
    data->locale = NULL;
  }

  LEAVE();
  return DoSuperMethodA(cl, obj, msg);
}

static IPTR mExport(struct IClass *cl, Object *obj, struct MUIP_Export *msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
  ULONG id;

  ENTER();

  if((id = (muiNotifyData(obj)->mnd_ObjectID)) != 0)
    DoMethod(msg->dataspace, MUIM_Dataspace_Add, data->Contents, strlen(data->Contents)+1, id);

  LEAVE();
  return 0;
}

static IPTR mImport(UNUSED struct IClass *cl, Object *obj, struct MUIP_Import *msg)
{
  ULONG id;

  ENTER();

  if((id = (muiNotifyData(obj)->mnd_ObjectID)) != 0)
  {
      STRPTR contents = (STRPTR)DoMethod(msg->dataspace, MUIM_Dataspace_Find, id);

//  if(contents)
      set(obj, MUIA_String_Contents, contents);
  }

  LEAVE();
  return 0;
}

void AddWindowSleepNotify(struct IClass *cl, Object *obj)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);

  ENTER();

  // we must check for a successful MUIM_Setup, because this function might be called during
  // OM_NEW and _win(obj) is not yet valid at that time
  if(isFlagClear(data->Flags, FLG_WindowSleepNotifyAdded) && isFlagSet(data->Flags, FLG_Setup) && _win(obj) != NULL)
  {
    if(data->SelectOnActive == TRUE || isFlagSet(data->Flags, FLG_ForceSelectOn))
    {
      // !!! CAUTION !!!
      // Ugly workaround for an ancient bug in MUI
      // MUIbase <= 2.11 adds some notifies for certain attributes which in turn
      // modify MUIA_Window_Sleep and hence trigger our own notify for this
      // attribute.
      // Removing our own notify will not remove it immediately but mark it as
      // "killed" only by MUI. The removal happens when the notifies are checked
      // for triggers. The problem arises if executing one notify triggers yet
      // another notification handling on the same object. In this case the nested
      // call will do the same removal as the first call is about to do next. This
      // will cause a double Remove() and double free of memory later in the first
      // call as here the pointer to the next notify to be handled has already been
      // obtained and will be used without further checks in the next iteration.
      // All this only happens if the removed notify directly follows the notify
      // which causes the removal. Thus we add a dummy notify to produce a "hole"
      // in the notify list and to let the nested notification check do its
      // removal work without causing a bad impact on the first check. This "hole"
      // just consists of another notify which never gets triggered. And even if
      // it would get triggered it will not cause a nested notify check. Thus the
      // first check will see this "hole" first before finally skipping the just
      // removed notify.
      // NOTE: this is neither a bug in MUIbase nor in BetterString but an ancient
      // bug in MUI itself as it does not take into account that a set() may cause
      // nested notifications which in turn may be removed inbetween!
      BOOL safeNotifies;

      #if defined(__amigaos3__) || defined(__amigaos4__)
      if(LIB_VERSION_IS_AT_LEAST(MUIMasterBase, 20, 5824))
      {
        // MUI4 for AmigaOS is safe for V20.5824+
        safeNotifies = TRUE;
      }
      else if(LIB_VERSION_IS_AT_LEAST(MUIMasterBase, 20, 2346) && LIBREV(MUIMasterBase) < 5000)
      {
        // MUI3.9 for AmigaOS is safe for V20.2346+
        safeNotifies = TRUE;
      }
      else
      {
        // MUI 3.8 and older version of MUI 3.9 or MUI4 are definitely unsafe
        safeNotifies = FALSE;
      }
      #else
      // MorphOS and AROS must be considered unsafe unless someone from the
      // MorphOS/AROS team confirms that removing notifies in nested OM_SET
      // calls is safe.
      safeNotifies = FALSE;
      #endif

      // add the dummy notify only once
      if(safeNotifies == FALSE && isFlagClear(data->Flags, FLG_DummyNotifyAdded))
      {
        // add a notify for an attribute which will *NEVER* be modified, thus the
        // trigger action will never be executed as well
        DoMethod(_win(obj), MUIM_Notify, MUIA_BetterString_Nop, MUIV_EveryTime, obj, 5, MUIM_Set, MUIA_NoNotify, TRUE, MUIA_BetterString_Nop, MUIV_TriggerValue);
        setFlag(data->Flags, FLG_DummyNotifyAdded);
        D(DBF_INPUT, "added dummy notify");
	  }

      // If the "select on active" feature is active we must be notified in case our
      // window is put to sleep to be able to deactivate the feature, because waking
      // the window up again will let ourself go active again and we will select the
      // complete content, even if it was not selected before. See YAM ticket #360
      // for details.
      // We must use a private attribute here, because the public attribute will remove
      // the notify again as soon as it is triggered.
      DoMethod(_win(obj), MUIM_Notify, MUIA_Window_Sleep, MUIV_EveryTime, obj, 3, MUIM_Set, MUIA_BetterString_InternalSelectOnActive, MUIV_NotTriggerValue);
      setFlag(data->Flags, FLG_WindowSleepNotifyAdded);
      D(DBF_INPUT, "added MUIA_Window_Sleep notify");
    }
  }

  LEAVE();
}

void RemWindowSleepNotify(struct IClass *cl, Object *obj)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);

  ENTER();

  // we must check for a successful MUIM_Setup, because this function might be called during
  // OM_NEW and _win(obj) is not yet valid at that time
  if(isFlagSet(data->Flags, FLG_WindowSleepNotifyAdded) && isFlagSet(data->Flags, FLG_Setup) && _win(obj) != NULL)
  {
    // remove the notify again
    D(DBF_INPUT, "remove MUIA_Window_Sleep notify");
    if(DoMethod(_win(obj), MUIM_KillNotifyObj, MUIA_Window_Sleep, obj) == 0)
      E(DBF_INPUT, "removing MUIA_Window_Sleep notify failed?");
    clearFlag(data->Flags, FLG_WindowSleepNotifyAdded);
  }

  LEAVE();
}

static IPTR mSetup(struct IClass *cl, Object *obj, struct MUI_RenderInfo *rinfo)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
  IPTR rc = FALSE;

  ENTER();

  InitConfig(obj, data);

  if(DoSuperMethodA(cl, obj, (Msg)rinfo))
  {
    // tell MUI we know how to indicate the active state
    _flags(obj) |= (1<<7);

    // remember that we went through MUIM_Setup
    setFlag(data->Flags, FLG_Setup);

    data->ehnode.ehn_Priority = 0;
    data->ehnode.ehn_Flags    = MUI_EHF_GUIMODE;
    data->ehnode.ehn_Object   = obj;
    data->ehnode.ehn_Class    = cl;
    data->ehnode.ehn_Events   = IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY;

    // setup the selection pointer
    if(data->SelectPointer == TRUE)
    {
      data->ehnode.ehn_Events |= IDCMP_MOUSEMOVE;
      SetupSelectPointer(data);
    }

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehnode);

    AddWindowSleepNotify(cl, obj);

    rc = TRUE;
  }
  else
  {
    FreeConfig(muiRenderInfo(obj), data);
  }

  RETURN(rc);
  return rc;
}

static IPTR mCleanup(struct IClass *cl, Object *obj, Msg msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);

  ENTER();

  // cleanup the selection pointer
  CleanupSelectPointer(data);

  RemWindowSleepNotify(cl, obj);

  DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode);

  FreeConfig(muiRenderInfo(obj), data);

  // forget that we went through MUIM_Setup
  clearFlag(data->Flags, FLG_Setup);

  LEAVE();
  return DoSuperMethodA(cl, obj, msg);
}

static IPTR mAskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
  LONG Height;

  ENTER();

  DoSuperMethodA(cl, obj, (Msg)msg);

  Height = _font(obj)->tf_YSize;
  msg->MinMaxInfo->MinHeight += Height;
  msg->MinMaxInfo->DefHeight += Height;
  msg->MinMaxInfo->MaxHeight += Height;

  if(data->Width)
  {
    ULONG width;

    SetFont(&data->rport, _font(obj));
    width = data->Width * TextLength(&data->rport, "n", 1);

    msg->MinMaxInfo->MinWidth  += width;
    msg->MinMaxInfo->DefWidth  += width;
    msg->MinMaxInfo->MaxWidth  += width;
  }
  else
  {
    msg->MinMaxInfo->MinWidth  += 10;
    msg->MinMaxInfo->DefWidth  += 100;
    msg->MinMaxInfo->MaxWidth  += MBQ_MUI_MAXMAX;
  }

  LEAVE();
  return 0;
}

static IPTR mShow(struct IClass *cl, Object *obj, Msg msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
  struct BitMap *friendBMp = _rp(obj)->BitMap;
  WORD  width, height, depth;

  ENTER();

  DoSuperMethodA(cl, obj, msg);

  width = _mwidth(obj);
  height = _font(obj)->tf_YSize;
  depth = ((struct Library *)GfxBase)->lib_Version >= 39 ? GetBitMapAttr(friendBMp, BMA_DEPTH) : friendBMp->Depth;

  InitRastPort(&data->rport);
  data->rport.BitMap = MUIG_AllocBitMap(width+40, height, depth, 0, friendBMp);
  SetFont(&data->rport, _font(obj));
  SetDrMd(&data->rport, JAM1);

  setFlag(data->Flags, FLG_Shown);

  RETURN(TRUE);
  return TRUE;
}

static IPTR mHide(struct IClass *cl, Object *obj, Msg msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);

  ENTER();

  clearFlag(data->Flags, FLG_Shown);

  // hide the selection pointer
  HideSelectPointer(obj, data);

  MUIG_FreeBitMap(data->rport.BitMap);

  LEAVE();
  return DoSuperMethodA(cl, obj, msg);
}

static IPTR mDraw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
  ENTER();

  DoSuperMethodA(cl, obj, (Msg)msg);

  if(isFlagSet(msg->flags, MADF_DRAWUPDATE) || isFlagSet(msg->flags, MADF_DRAWOBJECT))
  {
    PrintString(cl, obj);
  }

  LEAVE();
  return 0;
}

static IPTR mHandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
  IPTR result;

  ENTER();

  if(isFlagSet(data->Flags, FLG_Ghosted) || isFlagClear(data->Flags, FLG_Shown))
  {
    result = 0;
  }
  else
  {
    ULONG display_pos = data->DisplayPos;

    result = mHandleInput(cl, obj, msg);
    if(display_pos != data->DisplayPos)
      set(obj, MUIA_String_DisplayPos, data->DisplayPos);

    if(!result && data->ForwardObject != NULL)
    {
      ULONG attr = 0;

      switch(msg->muikey)
      {
        case MUIKEY_TOP:
          attr = MUIV_List_Active_Top;
        break;

        case MUIKEY_BOTTOM:
          attr = MUIV_List_Active_Bottom;
        break;

        case MUIKEY_UP:
          attr = MUIV_List_Active_Up;
        break;

        case MUIKEY_DOWN:
          attr = MUIV_List_Active_Down;
        break;

        case MUIKEY_PAGEUP:
          attr = MUIV_List_Active_PageUp;
        break;

        case MUIKEY_PAGEDOWN:
          attr = MUIV_List_Active_PageDown;
        break;
      }

      if(attr != 0)
      {
        set(data->ForwardObject, MUIA_List_Active, attr);
        result = MUI_EventHandlerRC_Eat;
      }
    }
  }

  RETURN(result);
  return result;
}

static IPTR mGoActive(struct IClass *cl, Object *obj, UNUSED Msg msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);

  ENTER();

  D(DBF_INPUT, "GoActive: %08lx %08lx", obj, data->Flags);

  FreeContentString(data->Original);
  if((data->Original = AllocContentString(strlen(data->Contents)+1)) != NULL)
    strlcpy(data->Original, data->Contents, strlen(data->Contents+1));

  // select everything if this is necessary or requested
  if((data->SelectOnActive == TRUE && isFlagClear(data->Flags, FLG_ForceSelectOff)) ||
     isFlagSet(data->Flags, FLG_ForceSelectOn))
  {
    // If the active flag is still clear we have been activated by keyboard or by
    // the application. Otherwise this method is called due to activation by mouse
    // and we must skip the "select on active" stuff as this has been done already.
    if(isFlagClear(data->Flags, FLG_Active))
    {
      DoMethod(obj, MUIM_BetterString_DoAction, MUIV_BetterString_DoAction_SelectAll);
    }
  }

  //  now declare ourself as active
  setFlag(data->Flags, FLG_Active);
  setFlag(data->Flags, FLG_FreshActive);

  if(isFlagClear(data->Flags, FLG_OwnBackground) && data->mui4x == FALSE)
    set(obj, MUIA_Background, data->ActiveBackground);
  else if(data->mui4x == TRUE)
    set(obj, MUIA_Background, MUII_StringActiveBack);
  else
    MUI_Redraw(obj, MADF_DRAWUPDATE);

  RETURN(TRUE);
  return TRUE;
}

static IPTR mGoInactive(struct IClass *cl, Object *obj, UNUSED Msg msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);

  ENTER();

  D(DBF_INPUT, "GoInActive: %08lx", obj);

  // clean an eventually marked block and the
  // active state flag of the gadget
  clearFlag(data->Flags, FLG_BlockEnabled);
  clearFlag(data->Flags, FLG_Active);
  clearFlag(data->Flags, FLG_FreshActive);

  if(isFlagSet(data->Flags, FLG_OwnBackground))
  {
    set(obj, MUIA_Background, data->OwnBackground);
    // MUI 3.8 needs an explicit refresh
    if(data->mui39 == FALSE && data->mui4x == FALSE)
      MUI_Redraw(obj, MADF_DRAWUPDATE);
  }
  else if(data->mui4x == TRUE)
    set(obj, MUIA_Background, MUII_StringBack);
  else
    set(obj, MUIA_Background, data->InactiveBackground);

  RETURN(TRUE);
  return TRUE;
}

DISPATCHER(_Dispatcher)
{
  IPTR result;

  ENTER();

  switch(msg->MethodID)
  {
    case OM_NEW:
      result = mNew(cl, obj, (struct opSet *)msg);
    break;

    case MUIM_Setup:
      result = mSetup(cl, obj, (struct MUI_RenderInfo *)msg);
    break;

    case MUIM_Show:
      result = mShow(cl, obj, msg);
    break;

    case MUIM_AskMinMax:
      result = mAskMinMax(cl, obj, (struct MUIP_AskMinMax *)msg);
    break;

    case MUIM_Draw:
      result = mDraw(cl, obj, (struct MUIP_Draw *)msg);
    break;

    case OM_GET:
      result = mGet(cl, obj, (struct opGet *)msg);
    break;

    case OM_SET:
      mSet(cl, obj, (struct opSet *)msg);
      result = DoSuperMethodA(cl, obj, msg);
    break;

    case MUIM_HandleEvent:
      result = mHandleEvent(cl, obj, (struct MUIP_HandleEvent *)msg);
    break;

    case MUIM_GoActive:
      result = mGoActive(cl, obj, msg);
    break;

    case MUIM_GoInactive:
      result = mGoInactive(cl, obj, msg);
    break;

    case MUIM_Hide:
      result = mHide(cl, obj, msg);
    break;

    case MUIM_Cleanup:
      result = mCleanup(cl, obj, msg);
    break;

    case OM_DISPOSE:
      result = mDispose(cl, obj, msg);
    break;

    case MUIM_Export:
      result = mExport(cl, obj, (struct MUIP_Export *)msg);
    break;

    case MUIM_Import:
      result = mImport(cl, obj, (struct MUIP_Import *)msg);
    break;

    case MUIM_BetterString_ClearSelected:
    {
      // forward the clear request to our new DoAction method
      // which in fact will do the very same, but a bit more clever
      DoMethod(obj, MUIM_BetterString_DoAction, MUIV_BetterString_DoAction_Delete);
      result = TRUE;
    }
    break;

    case MUIM_BetterString_Insert:
      result = mInsert(cl, obj, (struct MUIP_BetterString_Insert *)msg);
    break;

    case MUIM_BetterString_DoAction:
      result = mDoAction(cl, obj, (struct MUIP_BetterString_DoAction *)msg);
    break;

    case MUIM_BetterString_FileNameStart:
      result = mFileNameStart((struct MUIP_BetterString_FileNameStart *)msg);
    break;

    default:
      result = DoSuperMethodA(cl, obj, msg);
    break;
  }

  RETURN(result);
  return(result);
}
