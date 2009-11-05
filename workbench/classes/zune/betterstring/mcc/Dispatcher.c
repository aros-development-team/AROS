/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2009 by BetterString.mcc Open Source Team

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

#include "BetterString_mcc.h"
#include "private.h"

/*
struct NewMenu PopupMenuData[] =
{
  NM_TITLE, "Stringgadget",       NULL, 0L, 0, (APTR)0,
  NM_ITEM,  "Cut",                NULL, 0L, 0, (APTR)1,
  NM_ITEM,  "Copy",               NULL, 0L, 0, (APTR)2,
  NM_ITEM,  "Paste",              NULL, 0L, 0, (APTR)3,
  NM_ITEM,  NM_BARLABEL,          NULL, 0L, 0, (APTR)0,
  NM_ITEM,  "Undo",               NULL, 0L, 0, (APTR)4,
  NM_ITEM,  "Redo",               NULL, 0L, 0, (APTR)5,
  NM_ITEM,  "Original",           NULL, 0L, 0, (APTR)6,
  NM_ITEM,  NM_BARLABEL,          NULL, 0L, 0, (APTR)0,
  NM_ITEM,  "Complete path",      NULL, 0L, 0, (APTR)7,
  NM_ITEM,  "Toggle case",        NULL, 0L, 0, (APTR)0,
  NM_SUB,   "Character",          NULL, 0L, 0, (APTR)8,
  NM_SUB,   "Word",               NULL, 0L, 0, (APTR)9,
  NM_ITEM,  "Arithmetic",         NULL, 0L, 0, (APTR)0,
  NM_SUB,   "Increase",           NULL, 0L, 0, (APTR)10,
  NM_SUB,   "Decrease",           NULL, 0L, 0, (APTR)11,
  NM_SUB,   "To decimal",         NULL, 0L, 0, (APTR)12,
  NM_SUB,   "To hexadecimal",     NULL, 0L, 0, (APTR)13,
  NM_END,   NULL,                 NULL, 0L, 0, (APTR)0
};
*/

//#ifndef MUIM_Backfill
//#define MUIA_CustomBackfill                 0x80420a63
//#define MUIM_Backfill                       0x80428d73
//struct  MUIP_Backfill { ULONG MethodID; LONG left; LONG top; LONG right; LONG bottom; LONG xoffset; LONG yoffset; };
//#endif

IPTR New(struct IClass *cl, Object *obj, struct opSet *msg)
{
  if((obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg)))
  {
    struct InstData *data = (struct InstData *)INST_DATA(cl, obj);

//    kprintf("OM_NEW by %s\n", FindTask(NULL)->tc_Node.ln_Name);

    data->locale = OpenLocale(NULL);
    data->Contents = (STRPTR)SharedPoolAlloc(40);
    *data->Contents = '\0';

//    data->PopupMenu = MUI_MakeObject(MUIO_MenustripNM, PopupMenuData, NULL);
    SetAttrs(obj,  MUIA_FillArea,    FALSE,
//              MUIA_ContextMenu,  data->PopupMenu,
//              MUIA_CustomBackfill, TRUE,
              TAG_DONE );

    if(FindTagItem(MUIA_Font, msg->ops_AttrList))
      setFlag(data->Flags, FLG_OwnFont);

    if(FindTagItem(MUIA_Background, msg->ops_AttrList))
      setFlag(data->Flags, FLG_OwnBackground);

    {
      struct TagItem *tag;

      if((tag = FindTagItem(MUIA_Frame, msg->ops_AttrList)))
      {
        if(tag->ti_Data == MUIV_Frame_String)
          setFlag(data->Flags, FLG_SetFrame);
      }
    }

    msg->MethodID = OM_SET;
    Set(cl, obj, (struct opSet *)msg);
    msg->MethodID = OM_NEW;
    data->BufferPos = 0;

    return((IPTR)obj);
  }
  CoerceMethod(cl, obj, OM_DISPOSE);

  return(FALSE);
}

IPTR Dispose(struct IClass *cl, Object *obj, Msg msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);

//  kprintf("OM_DISPOSE by %s\n", FindTask(NULL)->tc_Node.ln_Name);
/*  if(data->PopupMenu)
    MUI_DisposeObject(data->PopupMenu);
*/

  if(data->locale != NULL)
  {
    CloseLocale(data->locale);
    data->locale = NULL;
  }

  return(DoSuperMethodA(cl, obj, msg));
}

IPTR Export(struct IClass *cl, Object *obj, struct MUIP_Export *msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
  ULONG id;

  if((id = (muiNotifyData(obj)->mnd_ObjectID)) != 0)
    DoMethod(msg->dataspace, MUIM_Dataspace_Add, data->Contents, strlen(data->Contents)+1, id);

  return 0;
}

IPTR Import(UNUSED struct IClass *cl, Object *obj, struct MUIP_Import *msg)
{
  ULONG id;

  if((id = (muiNotifyData(obj)->mnd_ObjectID)) != 0)
  {
      STRPTR contents = (STRPTR)DoMethod(msg->dataspace, MUIM_Dataspace_Find, id);

//  if(contents)
      set(obj, MUIA_String_Contents, contents);
  }

  return 0;
}

IPTR Setup(struct IClass *cl, Object *obj, struct MUI_RenderInfo *rinfo)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);

  ENTER();

  InitConfig(obj, data);

  if(DoSuperMethodA(cl, obj, (Msg)rinfo))
  {
/*      ULONG color;
    if(DoMethod(obj, MUIM_GetConfigItem, 132, &color))
    {
        UBYTE image[6];

      strcpy(image, "2:");
      strcat(image, (STRPTR)color);
      set(obj, MUIA_Background, image);
    }
*/

    _flags(obj) |= (1<<7);

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

    RETURN(TRUE);
    return(TRUE);
  }

  RETURN(FALSE);
  return(FALSE);
}

IPTR Cleanup(struct IClass *cl, Object *obj, Msg msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);

  // cleanup the selection pointer
  CleanupSelectPointer(data);

  DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode);

  // make sure the gadget is being set to inactive state
/*
  if(isFlagSet(data->Flags, FLG_Active))
  {
    clearFlag(data->Flags, FLG_Active);
    clearFlag(data->ehnode.ehn_Events, IDCMP_MOUSEMOVE);

    if(isFlagClear(data->Flags, FLG_OwnBackground))
      set(obj, MUIA_Background, data->InactiveBackground);
  }
*/
  FreeConfig(muiRenderInfo(obj), data);

  return(DoSuperMethodA(cl, obj, msg));
}

IPTR AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
  struct TextFont *font;
  WORD Height;

  DoSuperMethodA(cl, obj, (Msg)msg);

  font = data->Font ? data->Font : muiAreaData(obj)->mad_Font;
  Height = font->tf_YSize;
  msg->MinMaxInfo->MinHeight += Height;
  msg->MinMaxInfo->DefHeight += Height;
  msg->MinMaxInfo->MaxHeight += Height;

  if(data->Width)
  {
    ULONG width;

    SetFont(&data->rport, font);
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

  return(0);
}

IPTR Show(struct IClass *cl, Object *obj, Msg msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
  struct MUI_AreaData *ad = muiAreaData(obj);
  struct BitMap *friendBMp = muiRenderInfo(obj)->mri_RastPort->BitMap;
  WORD  width, height, depth;
  struct TextFont *font = data->Font ? data->Font : ad->mad_Font;

  DoSuperMethodA(cl, obj, msg);

  width = ad->mad_Box.Width - ad->mad_subwidth;
  height = font->tf_YSize;
  depth = ((struct Library *)GfxBase)->lib_Version >= 39 ? GetBitMapAttr(friendBMp, BMA_DEPTH) : friendBMp->Depth;

  InitRastPort(&data->rport);
  data->rport.BitMap = MUIG_AllocBitMap(width+40, height, depth, (ULONG)NULL, friendBMp);
  SetFont(&data->rport, font);
  SetDrMd(&data->rport, JAM1);

  setFlag(data->Flags, FLG_Shown);

  return(TRUE);
}

IPTR Hide(struct IClass *cl, Object *obj, Msg msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);

#ifdef __AROS__
  if(isFlagSet(data->Flags, FLG_Shown))
    DeinitRastPort(&data->rport);
#endif

  clearFlag(data->Flags, FLG_Shown);

  // hide the selection pointer
  HideSelectPointer(obj, data);

  MUIG_FreeBitMap(data->rport.BitMap);

  return(DoSuperMethodA(cl, obj, msg));
}

IPTR mDraw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
  DoSuperMethodA(cl, obj, (Msg)msg);

  if(isFlagSet(msg->flags, MADF_DRAWUPDATE) || isFlagSet(msg->flags, MADF_DRAWOBJECT))
  {
    PrintString(cl, obj);
  }

  return(0);
}

IPTR HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
  IPTR result;

  if(isFlagSet(data->Flags, FLG_Ghosted) || isFlagClear(data->Flags, FLG_Shown))
  {
    result = 0;
  }
  else
  {
    ULONG display_pos = data->DisplayPos;

    result = HandleInput(cl, obj, msg);
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

  return result;
}

IPTR GoActive(struct IClass *cl, Object *obj, UNUSED Msg msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);

  D(DBF_INPUT, "GoActive: %08lx", obj);

  setFlag(data->Flags, FLG_Active);
  setFlag(data->Flags, FLG_FreshActive);
/*
  DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode);
  setFlag(data->ehnode.ehn_Events, IDCMP_RAWKEY);
  DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehnode);
*/
  if(data->Original != NULL)
    SharedPoolFree(data->Original);

  if((data->Original = (STRPTR)SharedPoolAlloc(strlen(data->Contents)+1)) != NULL)
    strlcpy(data->Original, data->Contents, strlen(data->Contents+1));

  // select everything if this is necessary or requested
  if((data->SelectOnActive == TRUE && isFlagClear(data->Flags, FLG_MouseButtonDown) && isFlagClear(data->Flags, FLG_ForceSelectOff)) ||
     isFlagSet(data->Flags, FLG_ForceSelectOn))
  {
    data->BlockStart = 0;
    data->BlockStop = strlen(data->Contents);
    setFlag(data->Flags, FLG_BlockEnabled);
  }

  if(isFlagClear(data->Flags, FLG_OwnBackground))
    set(obj, MUIA_Background, data->ActiveBackground);
  else
    MUI_Redraw(obj, MADF_DRAWUPDATE);

  return TRUE;
}

IPTR GoInactive(struct IClass *cl, Object *obj, UNUSED Msg msg)
{
  struct InstData *data = (struct InstData *)INST_DATA(cl, obj);

  D(DBF_INPUT, "GoInActive: %08lx", obj);

  // clean an eventually marked block and the
  // active state flag of the gadget
  clearFlag(data->Flags, FLG_BlockEnabled);
  clearFlag(data->Flags, FLG_Active);
  clearFlag(data->Flags, FLG_FreshActive);
/*
  DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode);
  clearFlag(data->ehnode.ehn_Events, IDCMP_MOUSEMOVE);
  DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehnode);
*/

  if(isFlagClear(data->Flags, FLG_OwnBackground))
    set(obj, MUIA_Background, data->InactiveBackground);
  else
    MUI_Redraw(obj, MADF_DRAWUPDATE);

  return TRUE;
}

DISPATCHER(_Dispatcher)
{
  IPTR result = TRUE;

  ENTER();

  switch(msg->MethodID)
  {
    case OM_NEW:
      result = New(cl, obj, (struct opSet *)msg);
    break;

    case MUIM_Setup:
      result = Setup(cl, obj, (struct MUI_RenderInfo *)msg);
    break;

    case MUIM_Show:
      result = Show(cl, obj, msg);
    break;

    case MUIM_AskMinMax:
      result = AskMinMax(cl, obj, (struct MUIP_AskMinMax *)msg);
    break;

    case MUIM_Draw:
      result = mDraw(cl, obj, (struct MUIP_Draw *)msg);
    break;

    case OM_GET:
      result = Get(cl, obj, (struct opGet *)msg);
    break;

    case OM_SET:
      Set(cl, obj, (struct opSet *)msg);
      result = DoSuperMethodA(cl, obj, msg);
    break;

    case MUIM_HandleEvent:
      result = HandleEvent(cl, obj, (struct MUIP_HandleEvent *)msg);
    break;

    case MUIM_GoActive:
      result = GoActive(cl, obj, msg);
    break;

    case MUIM_GoInactive:
      result = GoInactive(cl, obj, msg);
    break;

    case MUIM_Hide:
      result = Hide(cl, obj, msg);
    break;

    case MUIM_Cleanup:
      result = Cleanup(cl, obj, msg);
    break;

    case OM_DISPOSE:
      result = Dispose(cl, obj, msg);
    break;

    case MUIM_Export:
      result = Export(cl, obj, (struct MUIP_Export *)msg);
    break;

    case MUIM_Import:
      result = Import(cl, obj, (struct MUIP_Import *)msg);
    break;

    case MUIM_BetterString_ClearSelected:
    {
      // forward the clear request to our new DoAction method
      // which in fact will do the very same, but a bit more clever
      DoMethod(obj, MUIM_BetterString_DoAction, MUIV_BetterString_DoAction_Delete);
    }
    break;

    case MUIM_BetterString_Insert:
    {
      struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
      struct MUIP_BetterString_Insert *ins_msg = (struct MUIP_BetterString_Insert *)msg;
      UWORD pos;

      switch(ins_msg->pos)
      {
/*        case MUIV_BetterString_Insert_StartOfString:
          pos = 0;
          break;
*/
        case MUIV_BetterString_Insert_EndOfString:
          pos = strlen(data->Contents);
          break;

        case MUIV_BetterString_Insert_BufferPos:
          pos = data->BufferPos;
          break;

        default:
          pos = ins_msg->pos;
          break;
      }
      Overwrite(ins_msg->text, pos, 0, data);
      clearFlag(data->Flags, FLG_BlockEnabled);
      MUI_Redraw(obj, MADF_DRAWUPDATE);
    }
    break;

    case MUIM_BetterString_DoAction:
      result = mDoAction(cl, obj, (struct MUIP_BetterString_DoAction *)msg);
    break;


/*
    case MUIM_Backfill:
    {
      struct MUIP_Backfill *fill_msg = (struct MUIP_Backfill *)msg;

      if(isFlagClear(data->Flags, FLG_Active))
        DoMethod(obj, MUIM_DrawBackground, fill_msg->left, fill_msg->top, fill_msg->right-fill_msg->left+1, fill_msg->bottom-fill_msg->top+1, fill_msg->xoffset, fill_msg->yoffset);
//      printf("%ld, %ld, %ld, %ld\n%ld, %ld\n", fill_msg->left, fill_msg->top, fill_msg->right, fill_msg->bottom, fill_msg->xoffset, fill_msg->yoffset);
    }
*/
    case MUIM_BetterString_FileNameStart:
      result = FileNameStart((struct MUIP_BetterString_FileNameStart *)msg);
    break;

    default:
      result = DoSuperMethodA(cl, obj, msg);
    break;
  }

  RETURN(result);
  return(result);
}
