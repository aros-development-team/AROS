/***************************************************************************

 NList.mcc - New List MUI Custom Class
 Registered MUI class, Serial Number: 1d51 0x9d510030 to 0x9d5100A0
                                           0x9d5100C0 to 0x9d5100FF

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2013 by NList Open Source Team

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
#include <stdlib.h>

#include <clib/alib_protos.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include "private.h"

#include "NList_func.h"
#include "version.h"
#include "NListviews_mcp.h"
#include "NListview_mcc.h"

extern const struct Hook NL_ConstructHook_String;
extern const struct Hook NL_DestructHook_String;

#define SET_PEN(var_dest,test_init) \
  { \
    tag->ti_Tag = TAG_IGNORE; \
    if(data->SETUP == TRUE) \
    { \
      test_init = tag->ti_Data; \
      obtain_pen(obj, &(var_dest), (struct MUI_PenSpec *)tag->ti_Data); \
      REDRAW_ALL; \
    } \
  }

#define SET_BG(var_dest,test_init) \
  { \
    tag->ti_Tag = TAG_IGNORE; \
    test_init = TRUE; \
    /* catch possible MUII_#? values, these must be used directly */ \
    if(tag->ti_Data <= 0x00000100) \
    { \
      var_dest = (IPTR)tag->ti_Data; \
    } \
    else \
    { \
      strlcpy(var_dest##Buffer, (STRPTR)tag->ti_Data, sizeof(var_dest##Buffer)); \
      var_dest = (IPTR)var_dest##Buffer; \
    } \
    REDRAW_ALL; \
  }




IPTR mNL_AskMinMax(struct IClass *cl,Object *obj,struct MUIP_AskMinMax *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  LONG vinc = data->vinc;
/*
 *   if (data->nodraw)
 *   {
 *     LONG minwidth = _minwidth(obj);
 *     LONG minheight = _minheight(obj);
 *     LONG maxwidth = _maxwidth(obj);
 *     LONG maxheight = _maxheight(obj);
 *     LONG defwidth = _defwidth(obj);
 *     LONG defheight = _defheight(obj);
 *
 * D(bug("%lx|mNL_AskMinMax() %ld,%ld %ld,%ld %ld,%ld \n",obj,minwidth,minheight,defwidth,defheight,maxwidth,maxheight));
 *     DoSuperMethodA(cl,obj,(Msg) msg);
 *     return (0);
 *   }
 */

  data->NList_Quiet++;
/*D(bug("%lx|mNL_AskMinMax() 1 \n",obj));*/
  DoSuperMethodA(cl,obj,(Msg) msg);
/*D(bug("%lx|mNL_AskMinMax() 2 \n",obj));*/

  data->font = _font(obj);
  data->vinc = data->font->tf_YSize + data->addvinc;
  if (data->NList_MinLineHeight > data->vinc)
    data->vinc = data->NList_MinLineHeight;
  if (data->MinImageHeight > data->vinc)
    data->vinc = data->MinImageHeight;
  if (data->vinc < 2)
    data->vinc = 2;

  if ((vinc > 1) && (vinc != data->vinc))
  {
    DO_NOTIFY(NTF_LineHeight);
  }

  if (((data->NList_SourceArray == 2) || data->VirtGroup) && (data->NList_Entries > 0))
  { struct RastPort *tmprp2 = NULL;
    if ((data->NList_AdjustWidth == -1) && !data->nodraw &&
             (tmprp2 = (struct RastPort *) AllocVecShared(sizeof(struct RastPort),0)))
    { struct RastPort *tmprp = data->rp;
      struct TextFont *tmpfont;
      WORD column,delta;
      LONG show;
      data->rp = &(_screen(obj)->RastPort);
      *tmprp2 = *(data->rp);
      data->rp = tmprp2;
      tmpfont = data->rp->Font;
      data->font = _font(obj);
      data->pens = _pens(obj);
      data->voff = data->font->tf_Baseline + (data->vinc - data->font->tf_YSize + 1)/2;
      { struct TextExtent te;
        data->hinc = 0;
        SetSoftStyle(data->rp, FSF_BOLD|FSF_ITALIC, STYLE_MASK);
        TextExtent(data->rp, "m", 1, &te);
        if (data->hinc < te.te_Width)
          data->hinc = te.te_Width;
        SetSoftStyle(data->rp, FSF_BOLD|FSF_ITALIC, STYLE_MASK);
        TextExtent(data->rp, "M", 1, &te);
        if (data->hinc < te.te_Width)
          data->hinc = te.te_Width;
        SetSoftStyle(data->rp, FSF_BOLD|FSF_ITALIC, STYLE_MASK);
        TextExtent(data->rp, "w", 1, &te);
        if (data->hinc < te.te_Width)
          data->hinc = te.te_Width;
        SetSoftStyle(data->rp, FSF_BOLD|FSF_ITALIC, STYLE_MASK);
        TextExtent(data->rp, "W", 1, &te);
        if (data->hinc < te.te_Width)
          data->hinc = te.te_Width;
        SetSoftStyle(data->rp, 0, STYLE_MASK);
        TextExtent(data->rp, " ", 1, &te);
        data->spacesize = te.te_Width;
      }
      data->tabsize = data->spacesize * data->NList_TabSize;
      if (data->tabsize < 4)
         data->tabsize = 4;

      show = data->SHOW;
      data->SHOW = 1;
      AllWidthColumns(data);
      data->SHOW = show;
      data->NList_AdjustWidth = 0;
      delta = 0;
      for (column = 0;column < data->numcols;column++)
      { data->NList_AdjustWidth += data->cols[column].c->colwidthmax;
        if (column < data->numcols-1)
          delta += data->cols[column].c->delta;
      }
      if (data->NList_AdjustWidth < 2)
        data->NList_AdjustWidth = -1;
      else
        data->NList_AdjustWidth += delta;
      if (data->rp->Font!=tmpfont)
        SetFont(data->rp,tmpfont);
      data->rp = tmprp;
      FreeVec(tmprp2);
    }
    if (data->NList_AdjustHeight)
    { if (data->NList_Title)
        data->NList_AdjustHeight = ((data->NList_Entries+1) * data->vinc) + 1;
      else
        data->NList_AdjustHeight = (data->NList_Entries * data->vinc) + 1;
      if (data->NList_AdjustHeight == 0)
        data->NList_AdjustHeight = -1;
      DONE_NOTIFY(NTF_MinMax);
    }
    if (data->NList_SourceArray == 2)
      data->NList_SourceArray = 1;
  }
/*
 *D(bug("%lx|AskMinMax1 MiW=%ld DeW=%ld MaW=%ld  MiH=%ld DeH=%ld MaH=%ld\n",obj,
 *        (LONG)msg->MinMaxInfo->MinWidth,(LONG)msg->MinMaxInfo->DefWidth,(LONG)msg->MinMaxInfo->MaxWidth,
 *        (LONG)msg->MinMaxInfo->MinHeight,(LONG)msg->MinMaxInfo->DefHeight,(LONG)msg->MinMaxInfo->MaxHeight));
 */
  if ((data->NList_AdjustWidth > 0) &&
      ((data->VirtGroup || (_screen(obj)->Width*3/4 > data->NList_AdjustWidth)) && (30 < data->NList_AdjustWidth)))
  { msg->MinMaxInfo->MinWidth  += data->NList_AdjustWidth;
    msg->MinMaxInfo->DefWidth  += data->NList_AdjustWidth;
    msg->MinMaxInfo->MaxWidth  += data->NList_AdjustWidth;
/*D(bug("%lx|1 AdjustWidth=%ld\n",obj,data->NList_AdjustWidth));*/
  }
  else if (data->NList_AdjustWidth > 0)
  { if (_screen(obj)->Width*3/4 > data->NList_AdjustWidth)
      msg->MinMaxInfo->MinWidth  += data->NList_AdjustWidth;
    else
      msg->MinMaxInfo->MinWidth  += 30;
    msg->MinMaxInfo->DefWidth  += data->NList_AdjustWidth;
    if (30 < data->NList_AdjustWidth)
      msg->MinMaxInfo->MaxWidth  += data->NList_AdjustWidth;
    else
      msg->MinMaxInfo->MaxWidth  += MUI_MAXMAX;
/*D(bug("%lx|2 AdjustWidth=%ld\n",obj,data->NList_AdjustWidth));*/
  }
  else
  { msg->MinMaxInfo->MinWidth  += 30;
    msg->MinMaxInfo->DefWidth  += 300;
    msg->MinMaxInfo->MaxWidth  += MUI_MAXMAX;
/*D(bug("%lx|3 AdjustWidth=%ld\n",obj,300));*/
  }
  if ((data->NList_AdjustHeight > 0) &&
      ((data->VirtGroup || (_screen(obj)->Height*3/4 > data->NList_AdjustHeight)) && ((data->vinc * 3) < data->NList_AdjustHeight)))
  { msg->MinMaxInfo->MinHeight += data->NList_AdjustHeight;
    msg->MinMaxInfo->DefHeight += data->NList_AdjustHeight;
    msg->MinMaxInfo->MaxHeight += data->NList_AdjustHeight;
/*D(bug("%lx|1 AdjustHeight=%ld\n",obj,data->NList_AdjustHeight));*/
  }
  else if (data->NList_AdjustHeight > 0)
  {
    if (_screen(obj)->Height*3/4 > data->NList_AdjustHeight)
      msg->MinMaxInfo->MinHeight += data->NList_AdjustHeight;
    else
      msg->MinMaxInfo->MinHeight += (data->vinc * 3);
    msg->MinMaxInfo->DefHeight += data->NList_AdjustHeight;
    if ((data->vinc * 3) < data->NList_AdjustHeight)
      msg->MinMaxInfo->MaxHeight += data->NList_AdjustHeight;
    else
      msg->MinMaxInfo->MaxHeight += MUI_MAXMAX;
/*D(bug("%lx|2 AdjustHeight=%ld\n",obj,data->NList_AdjustHeight));*/
  }
  else
  { msg->MinMaxInfo->MinHeight += (data->vinc * 3);
    msg->MinMaxInfo->DefHeight += 200;
    msg->MinMaxInfo->MaxHeight += MUI_MAXMAX;
/*D(bug("%lx|3 AdjustHeight=%ld\n",obj,200));*/
  }
  if (data->NList_TitleSeparator && data->NList_Title)
  { msg->MinMaxInfo->MinHeight += 2;
    msg->MinMaxInfo->DefHeight += 2;
    msg->MinMaxInfo->MaxHeight += 2;
  }
/*
 *D(bug("%lx|AskMinMax2 MiW=%ld DeW=%ld MaW=%ld  MiH=%ld DeH=%ld MaH=%ld\n",obj,
 *        (LONG)msg->MinMaxInfo->MinWidth,(LONG)msg->MinMaxInfo->DefWidth,(LONG)msg->MinMaxInfo->MaxWidth,
 *        (LONG)msg->MinMaxInfo->MinHeight,(LONG)msg->MinMaxInfo->DefHeight,(LONG)msg->MinMaxInfo->MaxHeight));
 */
  data->NList_Quiet--;
  if (!WANTED_NOTIFY(NTF_MinMaxNoDraw))
    data->do_draw_all = data->do_draw_title = data->do_draw = TRUE;
  return(0);
}



IPTR mNL_Notify(struct IClass *cl,Object *obj,struct MUIP_Notify *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  switch(msg->TrigAttr)
  {
    case MUIA_NListview_Horiz_ScrollBar :
      WANT_NOTIFY(NTF_SB);
      data->scrollersobj = msg->DestObj;
      break;

    case MUIA_NList_Horiz_First :
      WANT_NOTIFY(NTF_HSB);
      break;

    case MUIA_NList_Horiz_Entries :
    case MUIA_NList_Horiz_Visible :
    case MUIA_NList_HorizDeltaFactor :
      break;

    case MUIA_NList_Prop_First :
      WANT_NOTIFY(NTF_VSB);
      if (msg->DestObj && !data->VertPropObject)
      {
        IPTR val;

        // check if MUIA_Prop_First exists
        if(GetAttr(MUIA_Prop_First, msg->DestObj, &val) != FALSE)
          data->VertPropObject = msg->DestObj;
      }

      if (data->VertPropObject)
      {
        set(data->VertPropObject,MUIA_Prop_DoSmooth, data->NList_Smooth);
      }
      break;

    case MUIA_NList_VertDeltaFactor :
    case MUIA_NList_Prop_Entries :
    case MUIA_NList_Prop_Visible :
      break;

    case MUIA_List_Prop_First :
      if (msg->DestObj && !data->VertPropObject)
      {
        IPTR val;

        // check if there exists a MUIA_Prop_First attribute
        if(GetAttr(MUIA_Prop_First, msg->DestObj, &val) != FALSE)
          data->VertPropObject = msg->DestObj;

        if (data->VertPropObject)
        {
          struct List *childlist;

          if((childlist = (struct List *)xget(data->VertPropObject, MUIA_Group_ChildList)))
          {
            Object *object_state = (Object *)childlist->lh_Head;
            Object *child;

            while((child = NextObject(&object_state)))
            {
              // check if there exists a MUIA_Prop_First attribute
              if(GetAttr(MUIA_Prop_First, msg->DestObj, &val) != FALSE)
              {
                data->VertPropObject = msg->DestObj;
                break;
              }
            }
          }
          DoMethod(obj, MUIM_Notify, MUIA_NList_Prop_Entries,MUIV_EveryTime,
            data->VertPropObject, 3, MUIM_NoNotifySet,MUIA_Prop_Entries,MUIV_TriggerValue);
          DoMethod(obj, MUIM_Notify, MUIA_NList_Prop_Visible,MUIV_EveryTime,
            data->VertPropObject, 3, MUIM_NoNotifySet,MUIA_Prop_Visible,MUIV_TriggerValue);
          DoMethod(obj, MUIM_Notify, MUIA_NList_Prop_First,MUIV_EveryTime,
            data->VertPropObject, 3, MUIM_NoNotifySet,MUIA_Prop_First,MUIV_TriggerValue);
          DoMethod(data->VertPropObject, MUIM_Notify, MUIA_Prop_First,MUIV_EveryTime,
            obj, 3, MUIM_NoNotifySet,MUIA_NList_Prop_First,MUIV_TriggerValue);
          DoMethod(obj, MUIM_Notify, MUIA_NList_VertDeltaFactor,MUIV_EveryTime,
            data->VertPropObject, 3, MUIM_NoNotifySet,MUIA_Prop_DeltaFactor,MUIV_TriggerValue);
          set(data->VertPropObject,MUIA_Prop_DoSmooth, data->NList_Smooth);
        }
      }
      return (0);

    case MUIA_List_Prop_Entries :
    case MUIA_List_Prop_Visible :
      return (0);

    case MUIA_NList_First :
      WANT_NOTIFY(NTF_First);
      break;

    case MUIA_NList_Entries :
      WANT_NOTIFY(NTF_Entries);
      break;

    case MUIA_NList_Active :
      WANT_NOTIFY(NTF_Active);
      break;

    case MUIA_List_Active :
      WANT_NOTIFY(NTF_L_Active);
      break;

    case MUIA_NList_SelectChange :
      WANT_NOTIFY(NTF_Select);
      break;

    case MUIA_Listview_SelectChange :
      WANT_NOTIFY(NTF_LV_Select);
      break;

    case MUIA_NList_EntryClick :
      WANT_NOTIFY(NTF_EntryClick);
      break;

    case MUIA_NList_MultiClick :
      WANT_NOTIFY(NTF_Multiclick);
      break;

    case MUIA_NList_MultiClickAlone :
      WANT_NOTIFY(NTF_MulticlickAlone);
      break;

    case MUIA_NList_DoubleClick :
      msg->TrigVal = MUIV_EveryTime;
      WANT_NOTIFY(NTF_Doubleclick);
      break;

    case MUIA_Listview_DoubleClick :
      WANT_NOTIFY(NTF_LV_Doubleclick);
      break;

    case MUIA_NList_TitleClick :
      WANT_NOTIFY(NTF_TitleClick);
      break;

    case MUIA_NList_TitleClick2 :
      WANT_NOTIFY(NTF_TitleClick2);
      break;

    case MUIA_NList_ButtonClick :
      WANT_NOTIFY(NTF_ButtonClick);
      break;

    case MUIA_NList_LineHeight :
      WANT_NOTIFY(NTF_LineHeight);
      break;

    case MUIA_NList_DragSortInsert :
      WANT_NOTIFY(NTF_DragSortInsert);
      break;

    case MUIA_NList_InsertPosition :
      WANT_NOTIFY(NTF_Insert);
      break;

    case MUIA_NList_Columns :
      WANT_NOTIFY(NTF_Columns);
      break;
/*
 *     default:
 *       D(bug("%lx|NL: Notify(0x%lx,0x%lx) ??? \n",obj,(long) msg->TrigAttr,(long) msg->TrigVal));
 */
  }
  return (DoSuperMethodA(cl,obj,(Msg) msg));
}


IPTR mNL_Set(struct IClass *cl,Object *obj,Msg msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  IPTR retval;
  LONG do_things = TRUE;
  struct TagItem *tags,*tag;

  for(tags = ((struct opSet *)msg)->ops_AttrList; (tag = (struct TagItem *)NextTagItem((APTR)&tags));)
  {
    switch (tag->ti_Tag)
    {
      case MUIA_Background :
      case MUIA_Group_Forward :
        break;

      case MUIA_NList_Visible :
        tag->ti_Tag = TAG_IGNORE;
        do_things = FALSE;
        break;

      case MUIA_ShortHelp : // RHP: Added for Special Shorthelp
         if (data->affover<0)
         {
            data->NList_ShortHelp = (STRPTR)tag->ti_Data;
         }
         break;

      case MUIA_ContextMenu :
        if (do_things)
        {
          data->NList_ContextMenu = data->ContextMenu = (LONG)tag->ti_Data;

          if (data->SETUP && (data->NList_ContextMenu == (LONG)MUIV_NList_ContextMenu_Default))
          {
            LONG *ptrd;
            data->ContextMenu = MUIV_NList_ContextMenu_Always;

            if (DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_Menu, &ptrd))
            {
              switch (*ptrd)
              {
                case MUIV_NList_ContextMenu_TopOnly :
                case MUIV_NList_ContextMenu_Always :
                case MUIV_NList_ContextMenu_Never :
                  data->ContextMenu = *ptrd;
                  break;
              }
            }
          }
          if ((data->ContextMenu == (LONG)MUIV_NList_ContextMenu_TopOnly) ||
              (data->ContextMenu == (LONG)MUIV_NList_ContextMenu_Never))
            tag->ti_Data = 0;
          else if (data->ContextMenu == (LONG)MUIV_NList_ContextMenu_Always)
            tag->ti_Data = MUIV_NList_ContextMenu_Always;
          else if (((data->ContextMenu & 0x9d510030) == 0x9d510030) && (data->numcols <= 1))
            tag->ti_Data = 0;
        }
        data->ContextMenuOn = (LONG) tag->ti_Data;
        break;
      case MUIA_List_Active :
        if (!NOTIFYING(NTF_L_Active))
          NL_List_Active(data,(long) tag->ti_Data,tag,data->NList_List_Select,FALSE,0);
        NOTIFY_END(NTF_L_Active);
        DONE_NOTIFY(NTF_L_Active);
        break;
      case MUIA_NList_Active :
//      D(DBF_ALWAYS, "MUIA_NList_Active %ld was %ld %ld %ld",tag->ti_Data,data->NList_Active,NOTIFYING(NTF_Active),WANTED_NOTIFY(NTF_Active));
        if (!NOTIFYING(NTF_Active))
          NL_List_Active(data,(long) tag->ti_Data,tag,data->NList_List_Select,FALSE,0);
        NOTIFY_END(NTF_Active);
        DONE_NOTIFY(NTF_Active);
        break;
      case MUIA_List_First :
      case MUIA_NList_First :
        if (do_things)
        {
          NL_List_First(data,(long) tag->ti_Data,tag);
          data->ScrollBarsTime = SCROLLBARSTIME;
        }
        DONE_NOTIFY(NTF_First);
        break;
      case MUIA_NListview_Horiz_ScrollBar: break;
      case MUIA_List_Visible : break;
      case MUIA_List_Entries : break;
      case MUIA_NList_Entries : break;
      case MUIA_List_Quiet :
      case MUIA_NList_Quiet :
        data->display_ptr = NULL;
        data->parse_column = -1;
        if (tag->ti_Data == (ULONG)MUIV_NList_Quiet_Visual)
          VISUALQUIET;
        else if (tag->ti_Data)
          FULLQUIET;
        else
          ENDQUIET;
        tag->ti_Tag = TAG_IGNORE;
        break;
      case MUIA_List_Prop_Entries :
      case MUIA_List_Prop_Visible :
      case MUIA_List_Prop_First :
        tag->ti_Tag = TAG_IGNORE;
        break;
      case MUIA_NList_Prop_Entries :
        data->old_prop_entries = tag->ti_Data;
        break;
      case MUIA_NList_Prop_Visible :
        data->old_prop_visible = tag->ti_Data;
        break;
      case MUIA_NList_Prop_First :
        if (!WANTED_NOTIFY(NTF_VSB))
          tag->ti_Tag = TAG_IGNORE;
        else if (((tag->ti_Data / data->vinc) != (ULONG)(data->NList_Prop_First / data->vinc)) ||
                 (data->NList_Smooth && (tag->ti_Data != (ULONG)data->NList_Prop_First_Prec)))
        { LONG dobit = tag->ti_Data - data->old_prop_first;
          LONG Prop_FirstReal = -100;
          data->old_prop_first = tag->ti_Data;

          dobit = ((dobit >= 0) ? dobit : -dobit) - data->NList_Prop_Visible;
          dobit = (dobit >= 0) ? dobit : -dobit;
          if ((dobit >= 2) && !data->drawall_dobit)
            data->drawall_dobit = 2;

          NOWANT_NOTIFY(NTF_VSB);

          if(data->VertPropObject)
            Prop_FirstReal = (LONG)xget(data->VertPropObject, MUIA_Prop_TrueFirst);

          data->NList_Prop_First_Real = tag->ti_Data;
          if ((!data->NList_Smooth) || ((!data->NList_First_Incr) && (!data->NList_AffFirst_Incr) &&
                                        ((data->NList_Prop_First_Prec - (LONG)tag->ti_Data == data->vinc) ||
                                         (data->NList_Prop_First_Prec - (LONG)tag->ti_Data == -data->vinc) ||
                                         (Prop_FirstReal == data->NList_Prop_First_Real))))
          { LONG lpfirst;
            lpfirst = (data->NList_Prop_First_Real / data->vinc) * data->vinc;
            if ((lpfirst > (data->NList_Prop_Entries - data->NList_Prop_Visible)) &&
                (lpfirst > data->NList_Prop_First_Real))
              lpfirst = data->NList_Prop_Entries - data->NList_Prop_Visible;
            if (lpfirst < 0)
              lpfirst = 0;
            data->NList_Prop_First = (lpfirst / data->vinc) * data->vinc;
            data->NList_Prop_First_Real = data->NList_Prop_First;
            data->NList_Prop_Add = 0;
            lpfirst = data->NList_Prop_First / data->vinc;

            if (data->NList_First != lpfirst)
            {
              DO_NOTIFY(NTF_First);
            }

            data->NList_First = lpfirst;
            data->NList_First_Incr = 0;
            data->NList_Prop_Wait = 2;
            if ((data->NList_Prop_First_Real - data->NList_Prop_First_Prec < -20) || (data->NList_Prop_First_Real - data->NList_Prop_First_Prec > 20))
              data->ScrollBarsTime = SCROLLBARSTIME;
            if ((data->NList_First <= 0) || (data->NList_First + data->NList_Visible >= data->NList_Entries))
              data->drawall_dobit = 0;
            REDRAW;
          }
          else
          { LONG lfirst,lincr,add,add2;

            add = data->NList_Prop_First_Real - data->NList_Prop_First_Prec;
            if ((add < -5) || (add > 5))
              data->ScrollBarsTime = SCROLLBARSTIME;

            if (((data->NList_Prop_First_Real < data->NList_Prop_First_Prec) &&
                 (data->NList_Prop_First_Real < data->NList_Prop_First)) ||
                ((data->NList_Prop_First_Real > data->NList_Prop_First_Prec) &&
                 (data->NList_Prop_First_Real > data->NList_Prop_First)))
            {
              add2 = data->NList_Prop_First_Real - data->NList_Prop_First;
              if (((add2 > 0) && ((add * 2) < add2)) ||
                  ((add2 < 0) && ((add * 2) > add2)))
                data->NList_Prop_First += (add * 2);
              else
                data->NList_Prop_First = data->NList_Prop_First_Real;

              lfirst = data->NList_Prop_First / data->vinc;
              lincr = data->NList_Prop_First % data->vinc;
              if (((add < -1) || (add > 1)) &&
                  ((data->NList_First != lfirst) || (data->NList_First_Incr != lincr)))
              { data->NList_Prop_Add = add;
                data->NList_Prop_Wait = 1;

                if (data->NList_First != lfirst)
                {
                  DO_NOTIFY(NTF_First);
                }

                data->NList_First = lfirst;
                data->NList_First_Incr = lincr;
                REDRAW;
              }
              else if ((lfirst * data->vinc) == data->NList_Prop_First)
              { data->NList_Prop_Add = 0;
                data->NList_Prop_Wait = 1;

                if (data->NList_First != lfirst)
                {
                  DO_NOTIFY(NTF_First);
                }

                data->NList_First = lfirst;
                data->NList_First_Incr = lincr;
                REDRAW;
              }
              else
              { if      ((add < 0) && (data->NList_Prop_Add > -2))
                  data->NList_Prop_Add = -2;
                else if ((add > 0) && (data->NList_Prop_Add < 2))
                  data->NList_Prop_Add = 2;
              }
            }
            else
              data->NList_Prop_Add = add;
          }
          if (data->drawall_dobit == 2)
            data->drawall_dobit = 0;
          WANT_NOTIFY(NTF_VSB);
        }
        data->NList_Prop_First_Prec = tag->ti_Data;
        if ((tag->ti_Tag == MUIA_NList_Prop_First) || (tag->ti_Tag == MUIA_List_Prop_First))
          data->old_prop_first = tag->ti_Data;
        break;
      case MUIA_NList_Horiz_Entries :
        data->old_horiz_entries = tag->ti_Data;
        break;
      case MUIA_NList_Horiz_Visible :
        data->old_horiz_visible = tag->ti_Data;
        break;

      case MUIA_NList_Horiz_First:
      {
        if (do_things)
        {
          data->old_horiz_first = tag->ti_Data;
          NL_List_Horiz_First(data,(long) tag->ti_Data,tag);
          data->ScrollBarsTime = SCROLLBARSTIME;
        }
        if (tag->ti_Tag == MUIA_NList_Horiz_First)
          data->old_horiz_first = tag->ti_Data;
      }
      break;

      case MUIA_List_Title :
      case MUIA_NList_Title :
        data->display_ptr = NULL;
        if (tag->ti_Data && !data->NList_Title)
        { data->NList_Title = (char *) tag->ti_Data;
          NL_SetColsAdd(data,-1,TRUE);
          REDRAW_ALL_FORCE;
        }
        else if (!tag->ti_Data && data->NList_Title)
        { NL_SetColsRem(data,-1);
          data->NList_Title = NULL;
          REDRAW_ALL_FORCE;
        }
        else if (tag->ti_Data && data->NList_Title)
        { NL_SetColsRem(data,-1);
          data->NList_Title = (char *) tag->ti_Data;
          NL_SetColsAdd(data,-1,TRUE);
          REDRAW_FORCE;
        }
        break;
      case MUIA_NList_TitleSeparator :
        data->display_ptr = NULL;
        data->NList_TitleSeparator = (LONG) tag->ti_Data;
        REDRAW_ALL_FORCE;
        break;
      case 0x8042C53E :   /* sent by Listview, has same value as its MUIA_Listview_Input */
        break;
      case MUIA_Listview_Input :
      case MUIA_NList_Input :
        if (tag->ti_Data)
        { if (!data->NList_Input)
          { data->NList_Input = TRUE;
            nnset(obj,MUIA_Frame, MUIV_Frame_InputList);
            REDRAW_ALL_FORCE;
          }
        }
        else if (data->NList_Input)
        {
          data->NList_Input = FALSE;
          set_Active(MUIV_NList_Active_Off);
          NL_UnSelectAll(data,-1);
          nnset(data->this,MUIA_Frame, MUIV_Frame_ReadList);
          REDRAW_ALL_FORCE;
        }
        break;
      case MUIA_Listview_MultiSelect :
      case MUIA_NList_MultiSelect :
        { BOOL reactive = FALSE;
          if (data->multiselect == MUIV_NList_MultiSelect_None)
            reactive = TRUE;
          switch ((LONG) tag->ti_Data)
          { case MUIV_NList_MultiSelect_None :
              data->multiselect = data->NList_MultiSelect = MUIV_NList_MultiSelect_None;
              set_Active(MUIV_NList_Active_Off);
              NL_UnSelectAll(data,-1);
              break;
            case MUIV_NList_MultiSelect_Default :
              data->NList_MultiSelect = MUIV_NList_MultiSelect_Default;
              { LONG *multisel;

                if (data->SETUP && DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_MultiSelect, (SIPTR) (&multisel)))
                  data->multiselect = *multisel & 0x0007;
                else
                  data->multiselect = MUIV_NList_MultiSelect_Shifted;
              }
              break;
            case MUIV_NList_MultiSelect_Shifted :
            case MUIV_NList_MultiSelect_Always :
              data->multiselect = data->NList_MultiSelect = (LONG) tag->ti_Data;
              break;
          }
          if (reactive && (data->multiselect != MUIV_NList_MultiSelect_None) &&
              (data->NList_Active >= 0) && (data->NList_Active < data->NList_Entries))
          { NL_UnSelectAll(data,data->NList_Active);
            data->selectmode = MUIV_NList_Select_On;
            NL_List_Active(data,data->NList_Active,NULL,data->selectmode,TRUE,0);
          }
        }
        break;
      case MUIA_List_DragSortable :
      case MUIA_NList_DragSortable :
        data->NList_DragSortable = (LONG) tag->ti_Data;
        tag->ti_Tag = TAG_IGNORE;
        if (data->NList_DragSortable)
        { if (data->drag_type == MUIV_NList_DragType_None)
          { LONG *ptrd;
            if (data->SETUP && DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_DragType, &ptrd))
              data->drag_type = *ptrd;
            else
              data->drag_type = MUIV_NList_DragType_Immediate;
          }
        }
        else if (data->NList_DragType == MUIV_NList_DragType_None)
          data->drag_type = MUIV_NList_DragType_None;
        nnset(obj,MUIA_Dropable,data->NList_Dropable);
        break;
      case MUIA_Listview_DragType :
      case MUIA_NList_DragType :
        data->NList_DragType = (LONG) tag->ti_Data;
        tag->ti_Tag = TAG_IGNORE;
        if (data->NList_DragType == MUIV_NList_DragType_Default)
        { LONG *ptrd;
          if (data->SETUP && DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_DragType, &ptrd))
            data->drag_type = *ptrd;
          else
            data->drag_type = MUIV_NList_DragType_Immediate;
        }
        else
          data->drag_type = data->NList_DragType;
        break;
      case MUIA_NList_DragColOnly :
        data->NList_DragColOnly = (LONG) tag->ti_Data;
        break;
      case MUIA_List_ShowDropMarks :
      case MUIA_NList_ShowDropMarks :
        data->NList_ShowDropMarks = (LONG) tag->ti_Data;
        if (data->NList_DragSortable)
          data->NList_ShowDropMarks = TRUE;
        tag->ti_Tag = TAG_IGNORE;
        break;
      case MUIA_Draggable :
/*D(bug("%lx|set(MUIA_Draggable,%lx) \n",obj,tag->ti_Data));*/
        tag->ti_Tag = TAG_IGNORE;
        if (tag->ti_Data && (data->drag_type == MUIV_NList_DragType_None))
        { LONG *ptrd;
          data->NList_DragType = MUIV_NList_DragType_Default;
          if (data->SETUP && DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_DragType, &ptrd))
            data->drag_type = *ptrd;
          else
            data->drag_type = MUIV_NList_DragType_Immediate;
        }
        else if (!tag->ti_Data)
          data->NList_DragType = data->drag_type = MUIV_NList_DragType_None;
        break;
      case MUIA_Dropable :
        data->NList_Dropable = tag->ti_Data;
        if (data->NList_Dropable || data->NList_DragSortable)
          tag->ti_Data = TRUE;
        else
          tag->ti_Data = FALSE;
        break;
      case MUIA_List_ConstructHook :
      case MUIA_NList_ConstructHook :
/*D(bug("%lx|set_ConstructHook=0x%lx \n",obj,tag->ti_Data));*/
        if (tag->ti_Data == (ULONG)MUIV_NList_ConstructHook_String)
          data->NList_ConstructHook = (struct Hook *) &NL_ConstructHook_String;
        else
          data->NList_ConstructHook = (struct Hook *) tag->ti_Data;
        data->NList_ConstructHook2 = FALSE;
        data->do_setcols = data->do_updatesb = data->do_wwrap = TRUE;
        data->display_ptr = NULL;
        break;
      case MUIA_NList_ConstructHook2 :
        data->NList_ConstructHook = (struct Hook *) tag->ti_Data;
        data->NList_ConstructHook2 = TRUE;
        data->do_setcols = data->do_updatesb = data->do_wwrap = TRUE;
        data->display_ptr = NULL;
        break;
      case MUIA_List_DestructHook :
      case MUIA_NList_DestructHook :
/*D(bug("%lx|set_DestructHook=0x%lx \n",obj,tag->ti_Data));*/
        if (tag->ti_Data == (ULONG)MUIV_NList_DestructHook_String)
          data->NList_DestructHook = (struct Hook *) &NL_DestructHook_String;
        else
          data->NList_DestructHook = (struct Hook *) tag->ti_Data;
        data->NList_DestructHook2 = FALSE;
        data->do_setcols = data->do_updatesb = data->do_wwrap = TRUE;
        data->display_ptr = NULL;
        break;
      case MUIA_NList_DestructHook2 :
        data->NList_DestructHook = (struct Hook *) tag->ti_Data;
        data->NList_DestructHook2 = TRUE;
        data->do_setcols = data->do_updatesb = data->do_wwrap = TRUE;
        data->display_ptr = NULL;
        break;
      case MUIA_List_CompareHook :
      case MUIA_NList_CompareHook :
/*D(bug("%lx|set_CompareHook=0x%lx \n",obj,tag->ti_Data));*/
        data->NList_CompareHook = (struct Hook *) tag->ti_Data;
        data->NList_CompareHook2 = FALSE;
        break;
      case MUIA_NList_CompareHook2 :
        data->NList_CompareHook = (struct Hook *) tag->ti_Data;
        data->NList_CompareHook2 = TRUE;
        break;
      case MUIA_List_DisplayHook :
      case MUIA_NList_DisplayHook :
/*D(bug("%lx|set_DisplayHook=0x%lx \n",obj,tag->ti_Data));*/
        data->NList_DisplayHook = (struct Hook *) tag->ti_Data;
        data->NList_DisplayHook2 = FALSE;
        data->do_setcols = data->do_updatesb = data->do_wwrap = TRUE;
        data->display_ptr = NULL;
        break;
      case MUIA_NList_DisplayHook2 :
        data->NList_DisplayHook = (struct Hook *) tag->ti_Data;
        data->NList_DisplayHook2 = TRUE;
        data->do_setcols = data->do_updatesb = data->do_wwrap = TRUE;
        data->display_ptr = NULL;
        break;
      case MUIA_List_MultiTestHook :
      case MUIA_NList_MultiTestHook :
/*D(bug("%lx|set_MultiTestHook=0x%lx \n",obj,tag->ti_Data));*/
        data->NList_MultiTestHook = (struct Hook *) tag->ti_Data;
        break;
      case MUIA_NList_CopyEntryToClipHook :
        data->NList_CopyEntryToClipHook = (struct Hook *) tag->ti_Data;
        data->NList_CopyEntryToClipHook2 = FALSE;
        break;
      case MUIA_NList_CopyEntryToClipHook2 :
        data->NList_CopyEntryToClipHook = (struct Hook *) tag->ti_Data;
        data->NList_CopyEntryToClipHook2 = TRUE;
        break;
      case MUIA_NList_CopyColumnToClipHook :
        data->NList_CopyColumnToClipHook = (struct Hook *) tag->ti_Data;
        data->NList_CopyColumnToClipHook2 = FALSE;
        break;
      case MUIA_NList_CopyColumnToClipHook2 :
        data->NList_CopyColumnToClipHook = (struct Hook *) tag->ti_Data;
        data->NList_CopyColumnToClipHook2 = TRUE;
        break;
      case MUIA_Listview_DoubleClick:
        WANT_NOTIFY(NTF_LV_Doubleclick);
        break;
      case MUIA_NList_EntryClick :
        WANT_NOTIFY(NTF_EntryClick);
        break;
      case MUIA_NList_MultiClick :
        WANT_NOTIFY(NTF_Multiclick);
        break;
      case MUIA_NList_MultiClickAlone :
        WANT_NOTIFY(NTF_MulticlickAlone);
        break;
      case MUIA_NList_DoubleClick :
        WANT_NOTIFY(NTF_Doubleclick);
        break;
      case MUIA_NList_TitleClick:
/*D(bug("set(%lx,NList_TitleClick,%lx)\n",obj,tag->ti_Data));*/
        WANT_NOTIFY(NTF_TitleClick);
        break;
      case MUIA_NList_TitleClick2:
/*D(bug("set(%lx,NList_TitleClick2,%lx)\n",obj,tag->ti_Data));*/
        WANT_NOTIFY(NTF_TitleClick2);
        break;
      case MUIA_Listview_SelectChange:
        WANT_NOTIFY(NTF_LV_Select);
        break;
      case MUIA_NList_SelectChange:
        WANT_NOTIFY(NTF_Select);
        break;
      case MUIA_NList_TitlePen :
        SET_PEN(data->NList_TitlePen,data->Pen_Title_init);
        break;
      case MUIA_NList_ListPen :
        SET_PEN(data->NList_ListPen,data->Pen_List_init);
        break;
      case MUIA_NList_SelectPen :
        SET_PEN(data->NList_SelectPen,data->Pen_Select_init);
        break;
      case MUIA_NList_CursorPen :
        SET_PEN(data->NList_CursorPen,data->Pen_Cursor_init);
        break;
      case MUIA_NList_UnselCurPen :
        SET_PEN(data->NList_UnselCurPen,data->Pen_UnselCur_init);
        break;
      case MUIA_NList_InactivePen :
        SET_PEN(data->NList_InactivePen,data->Pen_Inactive_init);
        break;
      case MUIA_NList_TitleBackground :
        SET_BG(data->NList_TitleBackGround,data->BG_Title_init);
        break;
      case MUIA_NList_ListBackground :
        SET_BG(data->NList_ListBackGround,data->BG_List_init);
        break;
      case MUIA_NList_SelectBackground :
        SET_BG(data->NList_SelectBackground,data->BG_Select_init);
        break;
      case MUIA_NList_CursorBackground :
        SET_BG(data->NList_CursorBackground,data->BG_Cursor_init);
        break;
      case MUIA_NList_UnselCurBackground :
        SET_BG(data->NList_UnselCurBackground,data->BG_UnselCur_init);
        break;
      case MUIA_NList_InactiveBackground :
        SET_BG(data->NList_InactiveBackground,data->BG_Inactive_init);
        break;

      case MUIA_NList_DefaultObjectOnClick :
        data->NList_DefaultObjectOnClick = (LONG) tag->ti_Data;
      break;

      case MUIA_NList_ActiveObjectOnClick :
      {
        data->NList_ActiveObjectOnClick = (BOOL)tag->ti_Data;
        if(data->NList_ActiveObjectOnClick)
        {
          // disable that the object will automatically get a border when
          // the ActiveObjectOnClick option is active
          _flags(obj) |= (1<<7);
          if(data->nlistviewobj != NULL)
            _flags(data->nlistviewobj) |= (1<<7);
        }
        else
        {
          // enable that the object will automatically get a border when
          // the ActiveObjectOnClick option is active
          _flags(obj) &= ~(1<<7);
          if(data->nlistviewobj != NULL)
            _flags(data->nlistviewobj) &= ~(1<<7);
        }
      }
      break;

      case MUIA_List_MinLineHeight :
      case MUIA_NList_MinLineHeight :
        data->NList_MinLineHeight = (LONG) tag->ti_Data;
        if (data->NList_MinLineHeight <= 0)
        { data->addvinc = -data->NList_MinLineHeight;
          REDRAW_ALL;
        }
        else if ((data->NList_MinLineHeight != data->vinc) &&
                 (data->NList_MinLineHeight > data->font->tf_YSize) &&
                 (data->NList_MinLineHeight > data->MinImageHeight))
        { data->addvinc = DEFAULT_VERT_INC;
          if (data->SETUP)
          { LONG *ptrd;
            if (DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_VertInc, &ptrd))
              data->addvinc = *ptrd;
          }
          REDRAW_ALL;
        }
        break;
      case MUIA_NList_ForcePen :
        data->NList_ForcePen = (LONG) tag->ti_Data;
        tag->ti_Tag = TAG_IGNORE;
        if (data->NList_ForcePen == MUIV_NList_ForcePen_Default)
        { LONG *ptrd;
          if (data->SETUP && DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_ForcePen, &ptrd))
            data->ForcePen = *ptrd;
          else
            data->ForcePen = MUIV_NList_ForcePen_Off;
        }
        else
          data->ForcePen = data->NList_ForcePen;
        REDRAW_ALL;
        break;
      case MUIA_List_Format :
      case MUIA_NList_Format :
        data->NList_Format = (char *) tag->ti_Data;
        if (data->NList_Format)
          NL_Read_Format(data,data->NList_Format,(tag->ti_Tag == MUIA_List_Format));
        else
          NL_Read_Format(data, (char *)"",(tag->ti_Tag == MUIA_List_Format));
        Make_Active_Visible;
        REDRAW;
        break;
      case MUIA_NList_MakeActive :
        data->NList_MakeActive = tag->ti_Data;
        break;
      case MUIA_NList_KeepActive :
        data->NList_KeepActive = tag->ti_Data;
        break;
      case MUIA_NList_TypeSelect :
        MOREQUIET;
        NL_UnSelectAll(data,FALSE);
        UnSelectCharSel(data,FALSE);
        data->NList_TypeSelect = (LONG) tag->ti_Data;
        NL_UnSelectAll(data,FALSE);
        UnSelectCharSel(data,FALSE);
        set_Active(MUIV_NList_Active_Off);
        REDRAW_ALL;
        LESSQUIET;
        break;
      case MUIA_Listview_DefClickColumn :
      case MUIA_NList_DefClickColumn :
        data->NList_DefClickColumn = (LONG) tag->ti_Data;
        break;
      case MUIA_NList_AutoCopyToClip :
        data->NList_AutoCopyToClip = (LONG) tag->ti_Data;
        break;
      case MUIA_NList_AutoClip :
        data->NList_AutoClip = (BOOL)tag->ti_Data;
        break;
      case MUIA_NList_TabSize :
        data->NList_TabSize = (LONG) tag->ti_Data;
        REDRAW_ALL;
        break;
      case MUIA_NList_SkipChars :
        data->NList_SkipChars = (char *) tag->ti_Data;
        REDRAW_ALL;
        break;
      case MUIA_NList_WordSelectChars :
        data->NList_WordSelectChars = (char *) tag->ti_Data;
        break;
      case MUIA_NList_DisplayRecall :
        data->display_ptr = NULL;
        break;
      case MUIA_List_AutoVisible :
      case MUIA_NList_AutoVisible :
        data->NList_AutoVisible = (LONG) tag->ti_Data;
        break;
      case MUIA_NList_EntryValueDependent :
        data->NList_EntryValueDependent = (LONG) tag->ti_Data;
        break;
      case MUIA_NList_SortType :
        data->NList_SortType = (LONG) tag->ti_Data;
/*D(bug("set(%lx,NList_SortType,%lx)\n",obj,tag->ti_Data));*/
        break;
      case MUIA_NList_SortType2 :
        data->NList_SortType2 = (LONG) tag->ti_Data;
/*D(bug("set(%lx,NList_SortType2,%lx)\n",obj,tag->ti_Data));*/
        break;
      case MUIA_NList_PrivateData :
        data->NList_PrivateData = (APTR) tag->ti_Data;
        break;
      case MUIA_NList_VertDeltaFactor :
        data->old_prop_delta = tag->ti_Data;
        break;
      case MUIA_NList_HorizDeltaFactor :
        data->old_horiz_delta = tag->ti_Data;
        break;
      case MUIA_NList_MinColSortable :
        data->NList_MinColSortable = tag->ti_Data;
        break;
      case MUIA_NList_Imports :
        data->NList_Imports = tag->ti_Data;
        break;
      case MUIA_NList_Exports :
        data->NList_Exports = tag->ti_Data;
        break;
      case MUIA_NList_TitleMark :
        if (data->NList_TitleMark != (LONG)tag->ti_Data)
        { if (((data->NList_TitleMark & MUIV_NList_TitleMark_TypeMask) == MUIV_NList_TitleMark_None) &&
              ((data->NList_TitleMark2 & MUIV_NList_TitleMark2_TypeMask) == MUIV_NList_TitleMark2_None) &&
              ((tag->ti_Data & MUIV_NList_TitleMark_TypeMask) != MUIV_NList_TitleMark_None))
          {
            LONG column;
            for (column = 0;column < data->numcols;column++)
            { if (data->cols[column].c->userwidth < 0)
                data->cols[column].c->colwidthbiggestptr = -2;
            }
            data->do_setcols = TRUE;
/*            NL_SetColsAdd(data,-1,TRUE);*/
/*            data->do_draw = data->do_draw_all = data->do_parse = data->do_setcols = data->do_updatesb = data->do_wwrap = TRUE;*/
          }
          data->NList_TitleMark = tag->ti_Data;
/*D(bug("set(%lx,NList_TitleMark,%lx)\n",obj,tag->ti_Data));*/
          data->display_ptr = NULL;
          data->do_draw_title = TRUE;
          REDRAW_FORCE;
        }
        break;
      case MUIA_NList_TitleMark2 :
        if (data->NList_TitleMark2 != (LONG)tag->ti_Data)
        { if (((data->NList_TitleMark & MUIV_NList_TitleMark_TypeMask) == MUIV_NList_TitleMark_None) &&
              ((data->NList_TitleMark2 & MUIV_NList_TitleMark2_TypeMask) == MUIV_NList_TitleMark2_None) &&
              ((tag->ti_Data & MUIV_NList_TitleMark2_TypeMask) != MUIV_NList_TitleMark2_None))
          {
            LONG column;
            for (column = 0;column < data->numcols;column++)
            { if (data->cols[column].c->userwidth < 0)
                data->cols[column].c->colwidthbiggestptr = -2;
            }
            data->do_setcols = TRUE;
/*            NL_SetColsAdd(data,-1,TRUE);*/
/*            data->do_draw = data->do_draw_all = data->do_parse = data->do_setcols = data->do_updatesb = data->do_wwrap = TRUE;*/
          }
          data->NList_TitleMark2 = tag->ti_Data;
/*D(bug("set(%lx,NList_TitleMark2,%lx)\n",obj,tag->ti_Data));*/
          data->display_ptr = NULL;
          data->do_draw_title = TRUE;
          REDRAW_FORCE;
        }
        break;
      case MUIA_NList_Columns :
/*D(bug("set(%lx,MUIA_NList_Columns,%lx)\n",obj,tag->ti_Data));*/
        if (do_things)
        { NL_Columns(data,(BYTE *) tag->ti_Data);
          DONE_NOTIFY(NTF_Columns);
        }
        break;
      case MUIA_Disabled :
        if (tag->ti_Data)
          data->NList_Disabled = 1;
        else
        { if (data->NList_Disabled)
          { data->do_draw_all = data->do_draw_title = data->do_draw = TRUE;
          }
          data->NList_Disabled = FALSE;
        }
/*D(bug("%lx|Disabled=%ld\n",obj,data->NList_Disabled));*/
        break;
      case MUIA_Font :
        if (do_things)
          data->NList_Font = tag->ti_Data;
        break;

      case MUIA_NList_KeyUpFocus:
        data->NList_KeyUpFocus = (Object *)tag->ti_Data;
      break;

      case MUIA_NList_KeyDownFocus:
        data->NList_KeyDownFocus = (Object *)tag->ti_Data;
      break;

      case MUIA_NList_KeyLeftFocus:
        data->NList_KeyLeftFocus = (Object *)tag->ti_Data;
      break;

      case MUIA_NList_KeyRightFocus:
        data->NList_KeyRightFocus = (Object *)tag->ti_Data;
      break;

      case MUIA_NList_IgnoreSpecialChars:
        data->NList_IgnoreSpecialChars = (char *)tag->ti_Data;
      break;

/*
 *       case 0x8042AC64 :
 *       case 0x8042BE50 :
 *       case 0x8042B704 :
 *       case 0x804237F9 :
 *       case 0x8042C53E :
 *       case MUIA_FillArea :
 *         break;
 *
 *       case TAG_IGNORE :
 *         D(bug("NL: Set(TAG_IGNORE,0x%lx) \n",(long) tag->ti_Data));
 *         break;
 *       default:
 *         D(bug("%lx|NL: Set(0x%lx,0x%lx) ??? \n",obj,(long) tag->ti_Tag,(long) tag->ti_Data));
 */
    }
  }
  retval = DoSuperMethodA(cl,obj,msg);

/*  do_notifies(NTF_AllChanges|NTF_MinMax);*/
  return (retval);
}

//$$$Sensei: major cleanup: return value variable wasn't needed. The same as store. It just takes stack space.
// Some NList attributes (namely MUIA_NList_DragColOnly) wasn't returning TRUE after
// getting so DoSuperMethodA() was called on them, result probably was undefined!
// (It depends on what superclass do in OM_GET, if it clear storage data for undefined
// attributes, it could cause problems...
// Fixed msg type to correct one.
IPTR mNL_Get(struct IClass *cl,Object *obj,struct opGet *msg)
{
  struct NLData *data = INST_DATA( cl, obj );

  switch (((struct opGet *)msg)->opg_AttrID)
  {
    case MUIA_Listview_SelectChange:
    case MUIA_NList_SelectChange:
    case MUIA_Listview_DoubleClick:         *msg->opg_Storage = (ULONG) TRUE;                           return( TRUE );
    case MUIA_NList_DoubleClick:            *msg->opg_Storage = (ULONG) data->click_line;               return( TRUE );
    case MUIA_NList_MultiClick:             *msg->opg_Storage    = (ULONG) data->multiclick + 1;            return( TRUE );
    case MUIA_NList_MultiClickAlone:        *msg->opg_Storage    = (ULONG) abs( data->multiclickalone );    return( TRUE );
    case MUIA_NList_EntryClick:             *msg->opg_Storage    = (ULONG) data->click_line;                return( TRUE );
    case MUIA_List_InsertPosition:
    case MUIA_NList_InsertPosition:
    case MUIA_NList_DragSortInsert:         *msg->opg_Storage    = (ULONG) data->NList_LastInserted;        return( TRUE );
    case MUIA_List_First:
    case MUIA_NList_First:                  *msg->opg_Storage    = (ULONG) data->NList_First;                return( TRUE );
    case MUIA_List_Visible:
    case MUIA_NList_Visible:                *msg->opg_Storage    = (ULONG) ( data->SHOW ? data->NList_Visible : -1 );    return( TRUE );
    case MUIA_List_Entries:
    case MUIA_NList_Entries:                *msg->opg_Storage    = (ULONG) data->NList_Entries;            return( TRUE );
    case MUIA_List_Active:
    case MUIA_NList_Active:                 *msg->opg_Storage   = (ULONG) data->NList_Active;                return( TRUE );
    case MUIA_NList_Horiz_First:            *msg->opg_Storage    = (ULONG) data->NList_Horiz_First;        return( TRUE );
    case MUIA_NList_Horiz_Visible:          *msg->opg_Storage    = (ULONG) data->NList_Horiz_Visible;    return( TRUE );
    case MUIA_NList_Horiz_Entries:          *msg->opg_Storage    = (ULONG) data->NList_Horiz_Entries;    return( TRUE );
    case MUIA_NList_Prop_First:
    case MUIA_List_Prop_First:              *msg->opg_Storage    = (ULONG) data->NList_Prop_First;        return( TRUE );
    case MUIA_NList_Prop_Visible:
    case MUIA_List_Prop_Visible:            *msg->opg_Storage    = (ULONG) data->NList_Prop_Visible;        return( TRUE );
    case MUIA_NList_Prop_Entries:
    case MUIA_List_Prop_Entries:            *msg->opg_Storage    = (ULONG) data->NList_Prop_Entries;        return( TRUE );
    case MUIA_NList_VertDeltaFactor:        *msg->opg_Storage    = (ULONG) data->vinc;                        return( TRUE );
    case MUIA_NList_HorizDeltaFactor:       *msg->opg_Storage    = (ULONG) data->hinc;                        return( TRUE );
    case MUIA_NListview_Horiz_ScrollBar:    *msg->opg_Storage    = (ULONG) 0;                                    return( TRUE );
    case MUIA_NListview_Vert_ScrollBar:     *msg->opg_Storage    = (ULONG) 0;                                    return( TRUE );
    case MUIA_NList_TitlePen:               *msg->opg_Storage    = (ULONG) data->NList_TitlePen;            return( TRUE );
    case MUIA_NList_ListPen:                *msg->opg_Storage = (ULONG) data->NList_ListPen;            return( TRUE );
    case MUIA_NList_SelectPen:              *msg->opg_Storage = (ULONG) data->NList_SelectPen;            return( TRUE );
    case MUIA_NList_CursorPen:              *msg->opg_Storage = (ULONG) data->NList_CursorPen;            return( TRUE );
    case MUIA_NList_UnselCurPen:            *msg->opg_Storage = (ULONG) data->NList_UnselCurPen;        return( TRUE );
    case MUIA_NList_InactivePen:            *msg->opg_Storage = (ULONG) data->NList_InactivePen;        return( TRUE );
    case MUIA_NList_TitleBackground:        *msg->opg_Storage = (ULONG) data->NList_TitleBackGround;    return( TRUE );
    case MUIA_NList_ListBackground:         *msg->opg_Storage = (ULONG) data->NList_ListBackGround;    return( TRUE );
    case MUIA_NList_SelectBackground:       *msg->opg_Storage = (ULONG) data->NList_SelectBackground;    return( TRUE );
    case MUIA_NList_CursorBackground:       *msg->opg_Storage = (ULONG) data->NList_CursorBackground;    return( TRUE );
    case MUIA_NList_UnselCurBackground:     *msg->opg_Storage = (ULONG) data->NList_UnselCurBackground;    return( TRUE );
    case MUIA_NList_InactiveBackground:     *msg->opg_Storage = (ULONG) data->NList_InactiveBackground;    return( TRUE );
    case MUIA_Listview_Input:
    case MUIA_NList_Input:                  *msg->opg_Storage = (ULONG) data->NList_Input;                return( TRUE );
    case MUIA_List_Format:
    case MUIA_NList_Format:                 *msg->opg_Storage = (IPTR) data->NList_Format;                return( TRUE );
    case MUIA_List_Title:
    case MUIA_NList_Title:                  *msg->opg_Storage = (IPTR) data->NList_Title;                return( TRUE );
    case MUIA_NList_TitleSeparator:         *msg->opg_Storage = (ULONG) data->NList_TitleSeparator;    return( TRUE );
    case MUIA_List_DragSortable:
    case MUIA_NList_DragSortable:           *msg->opg_Storage = (ULONG) data->NList_DragSortable;        return( TRUE );
    case MUIA_Listview_DragType:
    case MUIA_NList_DragType:               *msg->opg_Storage = (ULONG) data->NList_DragType;            return( TRUE );
    case MUIA_NList_DragColOnly:            *msg->opg_Storage = (ULONG) data->NList_DragColOnly;        return( TRUE );
    case MUIA_List_ShowDropMarks:
    case MUIA_NList_ShowDropMarks:          *msg->opg_Storage = (ULONG) data->NList_ShowDropMarks;    return( TRUE );
    case MUIA_List_DropMark:
    case MUIA_NList_DropMark:
      if ((data->marktype & MUIV_NList_DropType_Mask) == MUIV_NList_DropType_Below)
        *msg->opg_Storage = (LONG) data->NList_DropMark + 1;
      else
        *msg->opg_Storage = (LONG) data->NList_DropMark;
      return( TRUE );
    case MUIA_NList_DropType:
      if ((data->marktype & MUIV_NList_DropType_Mask) == MUIV_NList_DropType_Below)
        *msg->opg_Storage = (LONG) (data->marktype & ~MUIV_NList_DropType_Mask) | MUIV_NList_DropType_Above;
      else
        *msg->opg_Storage = (LONG) data->marktype;
      return( TRUE );
    case MUIA_Listview_DefClickColumn:
    case MUIA_NList_DefClickColumn:            *msg->opg_Storage = (ULONG) data->NList_DefClickColumn;    return( TRUE );
    case MUIA_Listview_ClickColumn:
    case MUIA_NList_ClickColumn:
      { struct MUI_NList_TestPos_Result res;
        res.char_number = -2;
        NL_List_TestPos(data,MUI_MAXMAX,MUI_MAXMAX,&res);
        *msg->opg_Storage = (LONG) res.column;
      }
      return( TRUE );
    case MUIA_NList_TitleClick:                *msg->opg_Storage = (ULONG) data->TitleClick;                return( TRUE );
    case MUIA_NList_TitleClick2:                *msg->opg_Storage = (ULONG) data->TitleClick2;                return( TRUE );
    case MUIA_NList_ForcePen:                    *msg->opg_Storage = (ULONG) data->ForcePen;                    return( TRUE );
    case MUIA_List_AutoVisible:
    case MUIA_NList_AutoVisible:                *msg->opg_Storage = (ULONG) data->NList_AutoVisible;        return( TRUE );
    case MUIA_NList_TabSize:                    *msg->opg_Storage = (ULONG) data->NList_TabSize;            return( TRUE );
    case MUIA_NList_SkipChars:                *msg->opg_Storage = (IPTR) data->NList_SkipChars;            return( TRUE );
    case MUIA_NList_WordSelectChars:        *msg->opg_Storage = (IPTR) data->NList_WordSelectChars;    return( TRUE );
    case MUIA_NList_EntryValueDependent:    *msg->opg_Storage = (ULONG) data->NList_EntryValueDependent;    return( TRUE );
    case MUIA_NList_SortType:                    *msg->opg_Storage = (ULONG) data->NList_SortType;            return( TRUE );
    case MUIA_NList_SortType2:                *msg->opg_Storage = (ULONG) data->NList_SortType2;            return( TRUE );
    case MUIA_NList_ButtonClick:                *msg->opg_Storage = (ULONG) data->NList_ButtonClick;        return( TRUE );
    case MUIA_NList_MinColSortable:            *msg->opg_Storage = (ULONG) data->NList_MinColSortable;    return( TRUE );
    case MUIA_NList_Columns:                    *msg->opg_Storage = (IPTR) NL_Columns(data,NULL);    return( TRUE );
    case MUIA_NList_Imports:                    *msg->opg_Storage = (ULONG) data->NList_Imports;            return( TRUE );
    case MUIA_NList_Exports:                    *msg->opg_Storage = (ULONG) data->NList_Exports;            return( TRUE );
    case MUIA_NList_TitleMark:                *msg->opg_Storage = (ULONG) data->NList_TitleMark;            return( TRUE );
    case MUIA_NList_TitleMark2:                *msg->opg_Storage = (ULONG) data->NList_TitleMark2;        return( TRUE );
    case MUIA_NList_AutoClip:                *msg->opg_Storage = (ULONG) data->NList_AutoClip;     return(TRUE);
    case MUIA_NList_PrivateData:        *msg->opg_Storage = (IPTR) data->NList_PrivateData;    return(TRUE);

    case MUIA_NList_IgnoreSpecialChars: *msg->opg_Storage = (IPTR)data->NList_IgnoreSpecialChars; return(TRUE);

    case MUIA_NList_DefaultObjectOnClick: *msg->opg_Storage = (IPTR)data->NList_DefaultObjectOnClick; return(TRUE);
    case MUIA_NList_ActiveObjectOnClick:  *msg->opg_Storage = (IPTR)data->NList_ActiveObjectOnClick; return(TRUE);

    case MUIA_NList_KeyUpFocus:     *msg->opg_Storage = (IPTR)data->NList_KeyUpFocus;    return(TRUE);
    case MUIA_NList_KeyDownFocus:   *msg->opg_Storage = (IPTR)data->NList_KeyDownFocus;  return(TRUE);
    case MUIA_NList_KeyLeftFocus:   *msg->opg_Storage = (IPTR)data->NList_KeyLeftFocus;  return(TRUE);
    case MUIA_NList_KeyRightFocus:  *msg->opg_Storage = (IPTR)data->NList_KeyRightFocus; return(TRUE);

    case MUIA_Version:                            *msg->opg_Storage = (ULONG) LIB_VERSION;                          return(TRUE);
    case MUIA_Revision:                            *msg->opg_Storage = (ULONG) LIB_REVISION;                          return(TRUE);

    // Asking the superclass always yields MUIA_Disabled==FALSE even for really disabled lists.
    // Hence we return our own internal value, because NList intercepts MUIA_Disabled during
    // set() already.
    case MUIA_Disabled:                         *msg->opg_Storage = data->NList_Disabled ? TRUE : FALSE;          return(TRUE);

    default:
      return(DoSuperMethodA(cl, obj, (Msg)msg));

  }

/* Cut-n-pasted from bottom of function.
    LONG *store = ((struct opGet *)msg)->opg_Storage;
  ULONG retval;
  LONG affit = TRUE;
    case MUIA_NList_MultiClickAlone:
      if (data->multiclickalone < 0)
        *store = -data->multiclickalone;
      else
        *store = data->multiclickalone;
      return( TRUE );
  retval = DoSuperMethodA(cl,obj,msg);

 *
 *   if (affit)
 *   {
 *     if (retval)
 *       D(bug("NL: Get(0x%lx)=0x%lx \n",(long) ((struct opGet *)msg)->opg_AttrID, (long) *store));
 *     else
 *       D(bug("NL: Get(0x%lx)=???? \n",(long) ((struct opGet *)msg)->opg_AttrID));
 *   }
 *
  return(retval);
 */

 return(0);
}
