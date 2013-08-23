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

#include <graphics/gfxmacros.h>
#undef GetOutlinePen

#include <dos/dosextens.h>

#include <libraries/gadtools.h>
#include <clib/alib_protos.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/intuition.h>

#include "private.h"

#include "NList_func.h"
#include "NListview_mcc.h"

/*#define DO_CLIPBLIT    TRUE*/

static struct NewMenu MenuData[] =
{
  { NM_TITLE, (STRPTR)"Column", 0 ,0 ,0 ,(APTR) MUIV_NList_ContextMenu_Never },
  { NM_ITEM,  (STRPTR)"Default Width: this", 0 ,0 ,0 ,(APTR) MUIV_NList_Menu_DefWidth_This },
  { NM_ITEM,  (STRPTR)"Default Width: all",  0 ,0 ,0 ,(APTR) MUIV_NList_Menu_DefWidth_All },
  { NM_ITEM,  (STRPTR)"Default Order: this", 0 ,0 ,0 ,(APTR) MUIV_NList_Menu_DefOrder_This },
  { NM_ITEM,  (STRPTR)"Default Order: all",  0 ,0 ,0 ,(APTR) MUIV_NList_Menu_DefOrder_All },

  { NM_END,NULL,0,0,0,(APTR)0 },
};


void NL_SetCols(struct NLData *data)
{
  if (data->do_setcols)
  {
    if (data->cols)
    {
      LONG column;
      LONG weight,weight_all,weight_num,width,width_all,colx,minwidth,maxwidth,mincolwidth,maxcolwidth;
      BOOL do_colwidthmax = FALSE;

      weight_num = 0;
      weight_all = 0;
      width_all = data->mwidth;
      column = 0;
      while (column < data->numcols)
      {
        if (data->cols[column].c->userwidth >= 0)
          width_all -= data->cols[column].c->userwidth;
        else if (data->cols[column].c->width_type == CI_PIX)
          width_all -= data->cols[column].c->width;
        else if (data->cols[column].c->width_type == CI_COL)
          width_all -= data->cols[column].c->width * data->hinc +1;
        else if (data->cols[column].c->width == -1)
        { if (!do_colwidthmax)
          { if (data->cols[column].c->colwidthbiggestptr == -2)
            {
              NL_SetColsAdd(data,-2,TRUE);
              do_colwidthmax = TRUE;
            }
            else
            { data->cols[column].c->colwidthmax = data->cols[column].c->colwidthbiggest;
            }
          }
          width_all -= data->cols[column].c->colwidthmax;
        }
        else
        { weight_all += data->cols[column].c->width;
          weight_num++;
        }
        if (column < data->numcols-1)
          width_all -= data->cols[column].c->delta;
        column++;
      }
      if (weight_all <= (column*5))
        weight_all = (column*5)+1;
      colx = 0;
      column = 0;
      while (column < data->numcols)
      { weight = (LONG) data->cols[column].c->width;
        if (data->cols[column].c->userwidth >= 0)
          width = data->cols[column].c->userwidth;
        else if (data->cols[column].c->width_type == CI_PIX)
          width = weight;
        else if (data->cols[column].c->width_type == CI_COL)
          width = weight * data->hinc;
        else if (weight == -1)
          width = data->cols[column].c->colwidthmax;
        else
        {
          width = (weight * width_all) / weight_all;
          minwidth = (LONG) data->cols[column].c->minwidth;
          if (minwidth == -1)
          { if (!do_colwidthmax && (data->cols[column].c->colwidthbiggestptr == -2))
            {
              NL_SetColsAdd(data,-2,TRUE);
              do_colwidthmax = TRUE;
            }
            minwidth = data->cols[column].c->colwidthmax;
          }
          else
            minwidth = (minwidth * ((LONG) data->mwidth)) / 100L;
          maxwidth = (LONG) data->cols[column].c->maxwidth;
          maxwidth = (maxwidth * ((LONG) data->mwidth)) / 100L;

          mincolwidth = (LONG) data->cols[column].c->mincolwidth;
          mincolwidth = mincolwidth * ((LONG) data->hinc) + 1;
          maxcolwidth = (LONG) data->cols[column].c->maxcolwidth;
          maxcolwidth = maxcolwidth * ((LONG) data->hinc) + 1;

          if (mincolwidth > minwidth)
            minwidth = mincolwidth;
          if (data->cols[column].c->minpixwidth > minwidth)
            minwidth = data->cols[column].c->minpixwidth;

          if (maxcolwidth < maxwidth)
            maxwidth = maxcolwidth;
          if (data->cols[column].c->maxpixwidth < maxwidth)
            maxwidth = data->cols[column].c->maxpixwidth;

          if ((maxwidth > 10) && (width > maxwidth))
            width = maxwidth;

          if (width < minwidth)
            width = minwidth;

          if (width < 6)
            width = 6;

          width_all -= width;
          weight_all -= data->cols[column].c->width;
          if (weight_all < 10)
            weight_all = 10;
        }
        if (data->cols[column].c->dx != (WORD) width)
        { data->do_wwrap = data->force_wwrap = data->do_draw_all = TRUE;
          data->cols[column].c->dx = (WORD) width;
          if (data->NList_TypeSelect)
            UnSelectCharSel(data,FALSE);
        }
        if (data->cols[column].c->minx != (WORD) colx)
        { data->do_wwrap = data->force_wwrap = data->do_draw_all = TRUE;
          data->cols[column].c->minx = (WORD) colx;
          if (data->NList_TypeSelect)
            UnSelectCharSel(data,FALSE);
        }
        colx += width;
        if (data->cols[column].c->maxx != (WORD) colx)
        { if (column < data->numcols-1)
          { data->do_wwrap = data->force_wwrap = data->do_draw_all = TRUE;
          }
          data->cols[column].c->maxx = (WORD) colx;
        }
        if (column < data->numcols-1)
          colx += data->cols[column].c->delta;
        else if (data->cols[column].c->userwidth >= 0)
        { LONG ent = 0;
          while (data->EntriesArray && (ent < data->NList_Entries))
          { data->EntriesArray[ent]->PixLen = -1;
            ent++;
          }
        }
        if (!IS_BAR(column,data->cols[column].c))
          data->cols[column].c->maxx = MUI_MAXMAX;
        column++;
      }
    }
    data->do_setcols = FALSE;
  }
}



LONG NL_DoNotifies(struct NLData *data,LONG which)
{
  if (data->NList_Quiet || data->NList_Disabled)
    return (TRUE);

  /* notify ButtonClick */
  if(NEED_NOTIFY(NTF_ButtonClick) & which)
  {
    DONE_NOTIFY(NTF_ButtonClick);
    notdoset(data->this,MUIA_NList_ButtonClick,data->NList_ButtonClick);
  }

  /* notify TitleClick */
  if(NEED_NOTIFY(NTF_TitleClick) & which)
  {
    DONE_NOTIFY(NTF_TitleClick);
    notdoset(data->this,MUIA_NList_TitleClick,data->TitleClick);
  }

  /* notify TitleClick2 */
  if(NEED_NOTIFY(NTF_TitleClick2) & which)
  {
    DONE_NOTIFY(NTF_TitleClick2);
    notdoset(data->this,MUIA_NList_TitleClick2,data->TitleClick2);
  }

  /* notify EntryClick */
  if(NEED_NOTIFY(NTF_EntryClick) & which)
  {
    DONE_NOTIFY(NTF_EntryClick);
    notdoset(data->this,MUIA_NList_EntryClick,data->click_line);
  }

  /* notify Doubleclick */
  if(NEED_NOTIFY(NTF_Doubleclick) & which)
  {
    DONE_NOTIFY(NTF_Doubleclick);
    notdoset(data->this,MUIA_NList_DoubleClick,data->click_line);
  }

  if(NEED_NOTIFY(NTF_LV_Doubleclick) & which)
  {
    DONE_NOTIFY(NTF_LV_Doubleclick);
    if(data->listviewobj != NULL)
      DoMethod(data->listviewobj, MUIM_Set, MUIA_Listview_DoubleClick, (LONG)TRUE);
    else
      set(data->this,MUIA_Listview_DoubleClick,(LONG) TRUE);
  }

  /* notify Multiclick */
  if(NEED_NOTIFY(NTF_Multiclick) & which)
  {
    DONE_NOTIFY(NTF_Multiclick);
    notdoset(data->this,MUIA_NList_MultiClick,data->multiclick + 1);
  }

  /* notify Multiclick */
  if(NEED_NOTIFY(NTF_MulticlickAlone) & which)
  {
    DONE_NOTIFY(NTF_MulticlickAlone);
    if (data->multiclickalone > 0)
      data->multiclickalone = -data->multiclickalone;
    notdoset(data->this,MUIA_NList_MultiClickAlone,-data->multiclickalone);
  }

  /* notify_Active */
  if(NEED_NOTIFY(NTF_Active) & which)
  {
    DONE_NOTIFY(NTF_Active);
    NOTIFY_START(NTF_Active);
    notdoset(data->this,MUIA_NList_Active,data->NList_Active);
    NOTIFY_END(NTF_Active);
  }
  if(NEED_NOTIFY(NTF_L_Active) & which)
  {
    DONE_NOTIFY(NTF_L_Active);
    NOTIFY_START(NTF_L_Active);
    notdoset(data->this,MUIA_List_Active,data->NList_Active);
    NOTIFY_END(NTF_L_Active);
  }

  /* notify_Select */
  if(NEED_NOTIFY(NTF_Select) & which)
  {
    DONE_NOTIFY(NTF_Select);
    set(data->this,MUIA_NList_SelectChange,(LONG) TRUE);
  }

  if(NEED_NOTIFY(NTF_LV_Select) & which)
  {
    DONE_NOTIFY(NTF_LV_Select);
    if(data->listviewobj != NULL)
      DoMethod(data->listviewobj, MUIM_Set, MUIA_Listview_SelectChange, (LONG)TRUE);
    else
      set(data->this,MUIA_Listview_SelectChange,(LONG) TRUE);
  }

  /* notify first */
  if(NEED_NOTIFY(NTF_First) & which)
  {
    DONE_NOTIFY(NTF_First);
    notdoset(data->this,MUIA_NList_First,data->NList_First);
  }

  /* notify entries */
  if(NEED_NOTIFY(NTF_Entries) & which)
  {
    DONE_NOTIFY(NTF_Entries);
    notdoset(data->this,MUIA_NList_Entries,data->NList_Entries);
    notdoset(data->this,MUIA_List_Entries,data->NList_Entries);
  }

  /* notify LineHeight */
  if(NEED_NOTIFY(NTF_LineHeight) & which)
  {
    DONE_NOTIFY(NTF_LineHeight);
    notdoset(data->this,MUIA_NList_LineHeight,data->vinc);
  }

  /* notify NTF_DragSortInsert */
  if(NEED_NOTIFY(NTF_DragSortInsert) & which)
  {
    DONE_NOTIFY(NTF_Insert|NTF_DragSortInsert);
    notdoset(data->this,MUIA_NList_DragSortInsert,data->vinc);
  }

  /* notify Insert */
  if(NEED_NOTIFY(NTF_Insert) & which)
  {
    DONE_NOTIFY(NTF_Insert);
    notdoset(data->this,MUIA_NList_InsertPosition,data->vinc);
  }

  /* notify minmax */
  if(!data->do_wwrap && !data->force_wwrap &&
     (ASKED_NOTIFY(NTF_MinMax) & which))
  {
    DONE_NOTIFY(NTF_MinMax);
    ForceMinMax;
  }

  /* notify columns */
  if(NEED_NOTIFY(NTF_Columns) & which)
  {
    DONE_NOTIFY(NTF_Columns);
    notdoset(data->this,MUIA_NList_Columns,(IPTR) NL_Columns(data,NULL));
  }

  return (TRUE);
}


void NL_UpdateScrollersValues(struct NLData *data)
{
  Object *obj = data->this;

  if (((data->NList_Quiet > 0) || data->NList_Disabled) && !data->do_draw_all)
    return;
  if (WANTED_NOTIFY(NTF_VSB))
  {
    LONG entries = data->NList_Prop_First + data->NList_Prop_Visible;

    if (entries < data->NList_Prop_Entries)
      entries = data->NList_Prop_Entries;
    if (entries != data->old_prop_entries)
      set(obj, MUIA_NList_Prop_Entries,entries);
    if (data->NList_Prop_Visible != data->old_prop_visible)
      set(obj, MUIA_NList_Prop_Visible,data->NList_Prop_Visible);
    if (data->NList_Prop_First != data->old_prop_first)
      set(obj, MUIA_NList_Prop_First,data->NList_Prop_First);
    if (data->vinc != data->old_prop_delta)
      set(obj, MUIA_NList_VertDeltaFactor,data->vinc);
  }
  if (WANTED_NOTIFY(NTF_HSB))
  {
    if (data->NList_Horiz_Entries != data->old_horiz_entries)
      set(obj, MUIA_NList_Horiz_Entries,data->NList_Horiz_Entries);
    if (data->NList_Horiz_Visible != data->old_horiz_visible)
      set(obj, MUIA_NList_Horiz_Visible,data->NList_Horiz_Visible);
    if (data->NList_Horiz_First != data->old_horiz_first)
      notdoset(obj, MUIA_NList_Horiz_First,data->NList_Horiz_First);
    if (data->hinc != data->old_horiz_delta)
      set(obj, MUIA_NList_HorizDeltaFactor,data->hinc);
  }
}


ULONG NL_UpdateScrollers(struct NLData *data,BOOL force)
{
  if (!data->SHOW || !data->DRAW)
    return (FALSE);

  if (data->UpdatingScrollbars)
  {
    BOOL lastact = (data->NList_Active == (data->NList_First + data->NList_Visible - 1));

    NL_SetObjInfos(data,TRUE);
    data->NList_Prop_Visible = data->lvisible * data->vinc;
    data->NList_Horiz_Visible = data->mright - data->mleft;
    data->NList_Prop_Entries = data->NList_Entries * data->vinc;
    if (lastact && (data->NList_Active == (data->NList_First + data->NList_Visible)))
      data->NList_First++;
    NL_SetCols(data);
    NL_DoWrapAll(data,FALSE,FALSE);
    NL_UpdateScrollersValues(data);
    return (FALSE);
  }

  if (data->do_draw_all || data->do_parse || data->do_images || data->do_setcols || data->do_updatesb || data->format_chge)
    force = TRUE;
  if (data->ScrollBarsTime <= 0)
  { data->do_updatesb = TRUE;
    data->ScrollBarsTime = SCROLLBARSTIME;
  }

  if (!data->do_draw_all && !data->do_updatesb && !force && (data->NList_Quiet || data->NList_Disabled))
  {
    data->ScrollBarsTime = SCROLLBARSTIME;
    return (FALSE);
  }

  NL_SetObjInfos(data,TRUE);
  if (data->do_images)
  {
    GetImages(data);
    GetNImage_Sizes(data);
  }
  if (data->do_parse)
  {
    AllParseColumns(data);
    GetNImage_Sizes(data);
  }

  NL_SetCols(data);
  NL_DoWrapAll(data,FALSE,FALSE);

  if (data->do_draw_all || force)
  {
    LONG ent,ent2,hmax,linelen,hfirst;

    data->NList_Prop_Visible = data->lvisible * data->vinc;
    data->NList_Horiz_Visible = data->mright - data->mleft;
    data->NList_Prop_Entries = data->NList_Entries * data->vinc;
    hmax = 0;
    hfirst = data->NList_Horiz_First & ~1;
    ent = data->NList_First;
    ent2 = data->NList_First + data->NList_Visible;
    if (ent2 > data->NList_Entries)
      ent2 = data->NList_Entries;
    while (data->EntriesArray && (ent < ent2))
    {
      linelen = data->EntriesArray[ent]->PixLen;
      if (((linelen < 1) || data->do_draw_all) && (data->numcols > 0) && !DontDoColumn(data,ent,data->numcols-1))
      {
        data->display_ptr = NULL;
        data->parse_column = -1;
        if (data->cols[data->numcols-1].c->userwidth > 0)
          linelen = data->cols[data->numcols-1].c->userwidth + data->cols[data->numcols-1].c->minx - 1;
        else
        {
          NL_GetDisplayArray(data,ent);
          ParseColumn(data,data->numcols-1,0);
          WidthColumn(data,data->numcols-1,0);
          linelen = data->cols[data->numcols-1].c->colwidth + data->cols[data->numcols-1].c->minx + data->cols[data->numcols-1].c->xoffset;
          if (IS_ALIGN_CENTER(data->cols[data->numcols-1].c->style) && (data->cols[data->numcols-1].c->dx > data->cols[data->numcols-1].c->colwidth))
            linelen = data->cols[data->numcols-1].c->minx + data->cols[data->numcols-1].c->dx - 1;
          data->display_ptr = NULL;
          data->parse_column = -1;
        }
        data->EntriesArray[ent]->PixLen = linelen;
      }
      if (linelen > hmax)
        hmax = linelen;
      ent++;
    }
    if (data->NList_Title)
    {
      if ((data->Title_PixLen < 0) || data->do_draw_all)
      {
        data->do_draw_title = TRUE;
        data->display_ptr = NULL;
        data->parse_column = -1;
        if (data->cols[data->numcols-1].c->userwidth > 0)
          data->Title_PixLen = data->cols[data->numcols-1].c->userwidth + data->cols[data->numcols-1].c->minx - 1;
        else
        {
          NL_GetDisplayArray(data,-1);
          ParseColumn(data,data->numcols-1,0);
          WidthColumn(data,data->numcols-1,0);
          data->Title_PixLen = data->cols[data->numcols-1].c->colwidth + data->cols[data->numcols-1].c->minx + data->cols[data->numcols-1].c->xoffset;
          if (IS_ALIGN_CENTER(data->cols[data->numcols-1].c->style) && (data->cols[data->numcols-1].c->dx > data->cols[data->numcols-1].c->colwidth))
            data->Title_PixLen = data->cols[data->numcols-1].minx + data->cols[data->numcols-1].dx - 1;
          data->display_ptr = NULL;
          data->parse_column = -1;
        }
      }
      if (data->Title_PixLen > hmax)
        hmax = data->Title_PixLen;
    }
    if (hfirst + data->hvisible > hmax)
      hmax = hfirst + data->hvisible;
    data->NList_Horiz_Entries = hmax;
    NL_UpdateScrollersValues(data);
  }

  data->ScrollBarsTime = SCROLLBARSTIME;
  if (data->do_updatesb && (data->NList_Quiet <= 0) && !data->NList_Disabled && WANTED_NOTIFY(NTF_SB))
  {
    data->do_updatesb = FALSE;
    data->ScrollBars = (data->ScrollBarsOld & ~MUIV_NListview_HSB_On) | MUIV_NListview_HSB_Off;

    if (data->ScrollBarsPos != data->NList_First)
      data->ScrollBars = MUIV_NListview_HSB_Off | MUIV_NListview_VSB_Off;

    if ((data->NList_Prop_Entries > data->NList_Prop_Visible) || (data->NList_First > 0))
      data->ScrollBars |= MUIV_NListview_VSB_On;

    if (data->NList_Horiz_Entries > data->NList_Horiz_Visible)
      data->ScrollBars |= MUIV_NListview_HSB_On;
    else
    {
      LONG ent = data->NList_First + data->NList_Visible;

      if ((ent < data->NList_Entries) && data->EntriesArray &&
          ((data->ScrollBarsOld & MUIV_NListview_HSB_On) == MUIV_NListview_HSB_On))
      {
        LONG hsb,lastent,hsbheight,vsbwidth=0;

        // check if we got a HSB_Height attribute
        if(data->scrollersobj && GetAttr(MUIA_NListview_HSB_Height, data->scrollersobj, (IPTR *)&hsbheight) != FALSE)
        {
          hsbheight += data->vdt + data->vdb;
          lastent = ent + (hsbheight / data->vinc) + 1;
        }
        else
          lastent = ent+1;

        if(data->scrollersobj)
          vsbwidth = xget(data->scrollersobj, MUIA_NListview_VSB_Width);

        hsb = 0;
        while ((ent < lastent) && (ent < data->NList_Entries))
        {
          if (((data->EntriesArray[ent]->PixLen < 1) || data->do_draw_all) && (data->numcols > 0) && !DontDoColumn(data,ent,data->numcols-1))
          {
            data->display_ptr = NULL;
            data->parse_column = -1;
            if (data->cols[data->numcols-1].c->userwidth > 0)
              data->EntriesArray[ent]->PixLen = data->cols[data->numcols-1].c->userwidth + data->cols[data->numcols-1].c->minx - 1;
            else
            {
              NL_GetDisplayArray(data,ent);
              ParseColumn(data,data->numcols-1,0);
              WidthColumn(data,data->numcols-1,0);
              data->EntriesArray[ent]->PixLen = data->cols[data->numcols-1].c->colwidth + data->cols[data->numcols-1].c->minx + data->cols[data->numcols-1].c->xoffset;
              if (IS_ALIGN_CENTER(data->cols[data->numcols-1].c->style) && (data->cols[data->numcols-1].c->dx > data->cols[data->numcols-1].c->colwidth))
                data->EntriesArray[ent]->PixLen = data->cols[data->numcols-1].c->minx + data->cols[data->numcols-1].c->dx - 1;
              data->display_ptr = NULL;
              data->parse_column = -1;
            }
          }

          if (data->EntriesArray[ent]->PixLen + vsbwidth > data->NList_Horiz_Visible)
          { hsb |= MUIV_NListview_HSB_On;
            break;
          }
          ent++;
        }
        data->ScrollBars |= hsb;
      }
    }

    if (force || (data->ScrollBars != data->ScrollBarsOld))
    {
      data->ScrollBarsOld = data->ScrollBars;
      data->ScrollBarsPos = data->NList_First;

      data->UpdatingScrollbars = TRUE;
      data->UpdateScrollersRedrawn = FALSE;
      set(data->this,MUIA_NListview_Horiz_ScrollBar,data->ScrollBars);
      data->UpdatingScrollbars = FALSE;

      NL_UpdateScrollersValues(data);

      if ((data->ScrollBars & MUIV_NListview_VSB_On) != MUIV_NListview_VSB_On)
        data->ScrollBarsPos = -2;

      return (data->UpdateScrollersRedrawn);
    }
  }
  data->do_updatesb = FALSE;
  return (FALSE);
}


static LONG DoRefresh(struct NLData *data)
{
  if (data->SHOW && data->DRAW)
  {
    Object *obj = data->this;

    if (data->DRAW > 1)
    {
      data->pushtrigger = 1;
      DoMethod(_app(obj),MUIM_Application_PushMethod,obj,1,MUIM_NList_Trigger);
      return (FALSE);
    }
    if (!(muiRenderInfo(obj)->mri_Flags & MUIMRI_REFRESHMODE) && !data->refreshing &&
        (data->rp->Layer->Flags & LAYERREFRESH))
        /*&& ((_window(obj)->Flags & WFLG_REFRESHBITS) == WFLG_SIMPLE_REFRESH) )*/
    {
      SetBackGround(data->NList_ListBackGround);
      if (MUI_BeginRefresh(muiRenderInfo(obj),0))
      {
        Object *o = (Object *)xget(_win(obj),MUIA_Window_RootObject);

        if(o)
        {
          data->refreshing = TRUE;
/*
 *         if (data->do_draw_all && data->do_draw)
 *           data->refreshing = 2;
 */
          MUI_Redraw(o,MADF_DRAWOBJECT);  /* MADF_DRAWALL */
          data->refreshing = FALSE;
          MUI_EndRefresh(muiRenderInfo(obj),0);
        }
      }
    }
    if ((data->pushtrigger == 2) ||
        (!(data->rp->Layer->Flags & LAYERREFRESH) &&
         !(muiRenderInfo(obj)->mri_Flags & MUIMRI_REFRESHMODE) && !data->refreshing))
    {
      return (TRUE);
    }
    else if (!data->pushtrigger)
    {
      data->pushtrigger = 1;
      DoMethod(_app(obj),MUIM_Application_PushMethod,obj,1,MUIM_NList_Trigger);
    }
  }
  return (FALSE);
}

/*
 *  if (!(data->rp->Layer->Flags & LAYERREFRESH) || (data->pushtrigger == 2))
 *  {
 *  }
 *  else if (!data->pushtrigger)
 *  { data->pushtrigger = 1;
 *    DoMethod(_app(obj),MUIM_Application_PushMethod,obj,1,MUIM_NList_Trigger);
 *  }
 */


void NL_DrawQuietBG(struct NLData *data,LONG dowhat,LONG bg)
{
  if (data->do_draw_all)
  {
    if ((dowhat == 0) || (dowhat == 4))
      dowhat = 2;
    else if (dowhat == 3)
      dowhat = 1;
  }

  switch (dowhat)
  {
    case 0 :  /* REDRAW_IF */

      if (!data->NList_Quiet && !data->NList_Disabled && data->SHOW && data->DRAW)
      {
        if (DoRefresh(data))
        {
          if (data->do_draw_all)
          {
            ULONG mad_Flags = muiAreaData(data->this)->mad_Flags;

            data->nodraw++;
            OCLASS(data->this) = data->ncl;
            nnset(data->this,MUIA_Background,(IPTR)"0:128");
            OCLASS(data->this) = data->ocl;
            data->nodraw--;
            muiAreaData(data->this)->mad_Flags = mad_Flags;
            data->actbackground = -1;
            MUI_Redraw(data->this,MADF_DRAWUPDATE);
          }
          else if (data->do_draw)
            MUI_Redraw(data->this,MADF_DRAWUPDATE);
        }
      }
      break;

    case 1 :  /* REDRAW_ALL_FORCE */

      data->do_draw = data->do_draw_all = TRUE;
      if (data->SHOW && data->DRAW)
      {
        if (DoRefresh(data))
        {
          ULONG mad_Flags = muiAreaData(data->this)->mad_Flags;

          data->nodraw++;
          OCLASS(data->this) = data->ncl;
          nnset(data->this,MUIA_Background,(IPTR)"0:128");
          OCLASS(data->this) = data->ocl;
          data->nodraw--;
          muiAreaData(data->this)->mad_Flags = mad_Flags;
          data->actbackground = -1;
          MUI_Redraw(data->this,MADF_DRAWALL);
        }
      }
      break;

    case 2 :  /* REDRAW_ALL */

      data->do_draw = data->do_draw_all = TRUE;
      if (!data->NList_Quiet && !data->NList_Disabled && data->SHOW && data->DRAW)
      {
        if (DoRefresh(data))
        {
          ULONG mad_Flags = muiAreaData(data->this)->mad_Flags;

          data->nodraw++;
          OCLASS(data->this) = data->ncl;
          nnset(data->this,MUIA_Background,(IPTR)"0:128");
          OCLASS(data->this) = data->ocl;
          data->nodraw--;
          muiAreaData(data->this)->mad_Flags = mad_Flags;
          data->actbackground = -1;
          MUI_Redraw(data->this,MADF_DRAWOBJECT);
        }
      }
      break;

    case 3 :  /* REDRAW_FORCE */

      data->do_draw = TRUE;
      if (data->SHOW && data->DRAW)
      {
        if (DoRefresh(data))
        {
          if (data->do_draw_all)
          {
            ULONG mad_Flags = muiAreaData(data->this)->mad_Flags;

            data->nodraw++;
            OCLASS(data->this) = data->ncl;
            nnset(data->this,MUIA_Background,(IPTR)"0:128");
            OCLASS(data->this) = data->ocl;
            data->nodraw--;
            muiAreaData(data->this)->mad_Flags = mad_Flags;
            data->actbackground = -1;
            MUI_Redraw(data->this,MADF_DRAWUPDATE);
          }
          else
            MUI_Redraw(data->this,MADF_DRAWUPDATE);
        }
      }
      break;

    case 4 :  /* REDRAW */

      data->do_draw = TRUE;
      if (!data->NList_Quiet && !data->NList_Disabled && data->SHOW && data->DRAW)
      {
        if (DoRefresh(data))
        {
          if (data->do_draw_all)
          {
            ULONG mad_Flags = muiAreaData(data->this)->mad_Flags;

            data->nodraw++;
            OCLASS(data->this) = data->ncl;
            nnset(data->this,MUIA_Background,(IPTR)"0:128");
            OCLASS(data->this) = data->ocl;
            data->nodraw--;
            muiAreaData(data->this)->mad_Flags = mad_Flags;
            data->actbackground = -1;
            MUI_Redraw(data->this,MADF_DRAWUPDATE);
          }
          else
            MUI_Redraw(data->this,MADF_DRAWUPDATE);
        }
      }
      break;

    case 5 :  /* ENDQUIET */

      data->NList_Quiet = 0;
      if (data->SHOW)
      {
        if (DoRefresh(data))
        {
          if (data->do_draw_all)
          {
            ULONG mad_Flags = muiAreaData(data->this)->mad_Flags;

            data->nodraw++;
            OCLASS(data->this) = data->ncl;
            nnset(data->this,MUIA_Background,(IPTR)"0:128");
            OCLASS(data->this) = data->ocl;
            data->nodraw--;
            muiAreaData(data->this)->mad_Flags = mad_Flags;
            data->actbackground = -1;
            /*MUI_Redraw(data->this,MADF_DRAWOBJECT);*/
            MUI_Redraw(data->this,MADF_DRAWOBJECT);
          }
          else if (data->do_draw)
            MUI_Redraw(data->this,MADF_DRAWUPDATE);
        }
/*        do_notifies(NTF_All);*/
      }
      break;

    case 6 :  /* LESSQUIET */

      if (data->NList_Quiet > 0)
        data->NList_Quiet--;
      if (!data->NList_Quiet && !data->NList_Disabled && data->SHOW)
      {
        if (DoRefresh(data))
        {
          if (data->do_draw_all)
          {
            ULONG mad_Flags = muiAreaData(data->this)->mad_Flags;

            data->nodraw++;
            OCLASS(data->this) = data->ncl;
            nnset(data->this,MUIA_Background,(IPTR)"0:128");
            OCLASS(data->this) = data->ocl;
            data->nodraw--;
            muiAreaData(data->this)->mad_Flags = mad_Flags;
            data->actbackground = -1;
            /*MUI_Redraw(data->this,MADF_DRAWOBJECT);*/
            MUI_Redraw(data->this,MADF_DRAWUPDATE);
          }
          else if (data->do_draw)
            MUI_Redraw(data->this,MADF_DRAWUPDATE);
        }
/*        do_notifies(NTF_All);*/
      }
      break;

    case 7 :  /* SetBackGround */

      {
        ULONG mad_Flags = muiAreaData(data->this)->mad_Flags;

        data->actbackground = bg;
        data->nodraw++;
        OCLASS(data->this) = data->ncl;
        nnset(data->this,MUIA_Background,bg);
        OCLASS(data->this) = data->ocl;
        data->nodraw--;
        muiAreaData(data->this)->mad_Flags = mad_Flags;
      }

      break;

    case 8 :  /* Make_Active_Visible */

      if ((data->NList_AutoVisible) && (data->NList_Active >= 0) && (data->NList_Active < data->NList_Entries))
      {
        if (data->NList_Active < data->NList_First)
        {
          data->NList_First = data->NList_Active;
          DO_NOTIFY(NTF_First);
        }
        else if (data->NList_Active >= data->NList_First + data->NList_Visible)
        {
          data->NList_First = data->NList_Active - data->NList_Visible + 1;
          if (data->NList_First < 0)
            data->NList_First = 0;
          DO_NOTIFY(NTF_First);
        }
      }
      break;

    case 9 :  /* ForceMinMax */

      DONE_NOTIFY(NTF_MinMax);
      if (!WANTED_NOTIFY(NTF_MinMax) && data->SHOW && data->DRAW && data->NL_Group && data->VirtGroup && data->NList_AdjustHeight)
      {
        ULONG mad_Flags = muiAreaData(data->this)->mad_Flags;

        WANT_NOTIFY(NTF_MinMax|NTF_MinMaxNoDraw);
        if (DoMethod(data->VirtGroup,MUIM_Group_InitChange))
        {
          DoMethod(data->VirtGroup,MUIM_Group_ExitChange);
          muiAreaData(data->this)->mad_Flags = mad_Flags;
          NL_UpdateScrollersValues(data);
        }
      }
      NOWANT_NOTIFY(NTF_MinMax|NTF_MinMaxNoDraw);
      DONE_NOTIFY(NTF_MinMax);
      break;

  }
}


#define SELECTABLE(ent) \
  ((!data->NList_MultiTestHook) ||\
   (MyCallHookPkt(data->this,FALSE,data->NList_MultiTestHook,data->this,data->EntriesArray[ent]->Entry)))


void NL_Select(struct NLData *data,LONG dowhat,LONG ent,BYTE sel)
{
/* D(bug("%lx dowhat=%ld ent=%ld sel=%ld\n",obj,dowhat,ent,sel));*/
  switch (dowhat)
  {
    case 0 :  /* SELECT(ent,sel) */
      if (!data->NList_TypeSelect && SELECTABLE(ent))
      { if (data->EntriesArray[ent]->Select != sel)
        { data->EntriesArray[ent]->Select = sel;
          DO_NOTIFY(NTF_Select | NTF_LV_Select);

          DoMethod( data->this, MUIM_NList_SelectChange, ent, MUIV_NList_Select_On, 0 );
        }
      }
      break;

    case 1 :  /* SELECT2(ent,sel) */
      if (!data->NList_TypeSelect && (data->EntriesArray[ent]->Select != sel))
      { data->EntriesArray[ent]->Select = sel;
        DO_NOTIFY(NTF_Select | NTF_LV_Select);

        DoMethod( data->this, MUIM_NList_SelectChange, ent, MUIV_NList_Select_Off, 0 );
      }
      break;

    case 2 :  /* SELECT_CHGE(ent,sel) */
      if (!data->NList_TypeSelect && SELECTABLE(ent))
      { if (data->EntriesArray[ent]->Select != sel)
        { data->EntriesArray[ent]->Select = sel;
          NL_Changed(data,ent);
          DO_NOTIFY(NTF_Select | NTF_LV_Select);

          DoMethod( data->this, MUIM_NList_SelectChange, ent, MUIV_NList_Select_On, MUIV_NList_SelectChange_Flag_Multi );
        }
      }
      break;

    case 3 :  /* SELECT2_CHGE(ent,sel) */
      if (!data->NList_TypeSelect && (data->EntriesArray[ent]->Select != sel))
      { data->EntriesArray[ent]->Select = sel;
        NL_Changed(data,ent);
        DO_NOTIFY(NTF_Select | NTF_LV_Select);

        DoMethod( data->this, MUIM_NList_SelectChange, ent, MUIV_NList_Select_Off, MUIV_NList_SelectChange_Flag_Multi );
      }
      break;

    case 4 :  /* set_Active(ent) */
      { if (data->NList_Active != (ent))
        {
          data->NList_Active = (ent);
          DO_NOTIFY(NTF_Active | NTF_L_Active);

          DoMethod( data->this, MUIM_NList_SelectChange, ent, MUIV_NList_Select_Active, 0 );
        }
      }
      break;

  }
}





/*
 *     if ((w->Flags & WFLG_REFRESHBITS) == WFLG_SIMPLE_REFRESH)
 *     {
 *       struct Layer *lay, *l = w->WLayer;
 *       WORD mleft,mright,mtop,mbottom;
 *       mleft = w->LeftEdge + data->mleft;
 *       mright = w->LeftEdge + data->mright;
 *       mtop = w->TopEdge + data->mtop;
 *       mbottom = w->TopEdge + data->mbottom;
 *       lay = w->WLayer->front;
 *       while (lay)
 *       { if ((lay->bounds.MaxX > mleft) &&
 *             (lay->bounds.MinX < mright) &&
 *             (lay->bounds.MaxY > mtop) &&
 *             (lay->bounds.MinY < mbottom))
 *         { data->do_draw_all = TRUE;
 *           data->NList_First_Incr = 0;
 *           break;
 *         }
 *         lay = lay->front;
 *       }
 *     }
 */

#define LayerCovered(l)  ((!(l)->ClipRect) || ((l)->ClipRect->bounds.MaxX != (l)->bounds.MaxX) ||\
                                              ((l)->ClipRect->bounds.MinX != (l)->bounds.MinX) ||\
                                              ((l)->ClipRect->bounds.MaxY != (l)->bounds.MaxY) ||\
                                              ((l)->ClipRect->bounds.MinY != (l)->bounds.MinY))

#define LayerDamaged(l)  ((l)->DamageList && (l)->DamageList->RegionRectangle)

#define ABS(x)  (((x) >= 0) ? (x) : -(x))

void ScrollVert(struct NLData *data,WORD dy,LONG LPVisible)
{
  WORD y1 = data->vpos;
  WORD y2 = data->vpos + LPVisible - 1;

  if (data->vwidth <= 0)
    return;
  if (y1 < data->vtop) y1 = data->vtop;
  if (y2 > data->vbottom) y2 = data->vbottom;
  if ((y2 - y1) <= ABS(dy))
    return;

#ifndef DO_CLIPBLIT
  if (LIBVER(GfxBase) >= 39)
  {
    struct Hook *oldbackfilhook;

    oldbackfilhook = InstallLayerHook(data->rp->Layer, LAYERS_NOBACKFILL);
    ScrollRasterBF(data->rp,0,dy,data->vleft,y1,data->vright,y2);
    InstallLayerHook(data->rp->Layer, oldbackfilhook);
  }
  else
#endif
  {
    struct Window *w = _window(data->this);
    struct Layer *l = w->WLayer;

    if (dy > 0)
      ClipBlit(data->rp,data->vleft,y1+dy,data->rp,data->vleft,y1,data->vwidth,y2-y1+1-dy,0xC0);
    else
      ClipBlit(data->rp,data->vleft,y1,data->rp,data->vleft,y1-dy,data->vwidth,y2-y1+1+dy,0xC0);
    if (((w->Flags & WFLG_REFRESHBITS) == WFLG_SIMPLE_REFRESH) &&
        (LayerCovered(l) || LayerDamaged(l)))
    {
      UBYTE oldmask = data->rp->Mask;

      SetWrMsk(data->rp,0);
      ScrollRaster(data->rp,0,dy,data->vleft,y1,data->vright,y2);
      SetWrMsk(data->rp,oldmask);
    }
  }
}


void ScrollHoriz(struct NLData *data,WORD dx,LONG LPVisible)
{
  WORD y1 = data->vpos;
  WORD y2 = data->vpos + LPVisible - 1;

  if (data->vwidth <= ABS(dx))
    return;
  if (data->NList_Title)
    y1 = data->vdtitlepos;
  if (y1 < data->vtop) y1 = data->vtop;
  if (y2 > data->vbottom) y2 = data->vbottom;
  if (y1 > y2)
    return;

#ifndef DO_CLIPBLIT
  if (LIBVER(GfxBase) >= 39)
  {
    struct Hook *oldbackfilhook;

    oldbackfilhook = InstallLayerHook(data->rp->Layer, LAYERS_NOBACKFILL);
    ScrollRasterBF(data->rp,dx,0,data->vleft,y1,data->vright,y2);
    InstallLayerHook(data->rp->Layer, oldbackfilhook);
  }
  else
#endif
  {
    struct Window *w = _window(data->this);
    struct Layer *l = w->WLayer;

    if (dx > 0)
      ClipBlit(data->rp,data->vleft+dx,y1,data->rp,data->vleft,y1,data->vwidth-dx,y2-y1+1,0xC0);
    else
      ClipBlit(data->rp,data->vleft,y1,data->rp,data->vleft-dx,y1,data->vwidth+dx,y2-y1+1,0xC0);
    if (((w->Flags & WFLG_REFRESHBITS) == WFLG_SIMPLE_REFRESH) &&
        (LayerCovered(l) || LayerDamaged(l)))
    {
      UBYTE oldmask = data->rp->Mask;

      SetWrMsk(data->rp,0);
      ScrollRaster(data->rp,dx,0,data->vleft,y1,data->vright,y2);
      SetWrMsk(data->rp,oldmask);
    }
  }
}


LONG  NL_ColToColumn(struct NLData *data,LONG col)
{
  LONG column;

  if ((col >= 0) && (col < DISPLAY_ARRAY_MAX))
  {
    for (column = 0; column < data->numcols; column++)
    {
      if (data->cols[column].c->col == col)
        return (column);
    }
  }
  return (-1);
}


LONG  NL_ColumnToCol(struct NLData *data,LONG column)
{
  if ((column >= 0) && (column < data->numcols))
    return ((LONG) data->cols[column].c->col);
  return (-1);
}


LONG  NL_SetCol(struct NLData *data,LONG column,LONG col)
{ LONG column2d = -1;
  if ((column == MUIV_NList_SetColumnCol_Default) && (col == MUIV_NList_SetColumnCol_Default))
  { for (column = 0; column < data->numcols; column++)
    { if (data->cols[column].c != &data->cols[column])
        data->do_setcols = TRUE;
      data->cols[column].c = &data->cols[column];
    }
    if (data->do_setcols)
    { DO_NOTIFY(NTF_Columns);
      REDRAW_FORCE;
    }
    return (0);
  }
  else if ((col == MUIV_NList_SetColumnCol_Default) &&
           (column >= 0) && (column < data->numcols))
  { for (column2d = 0; column2d < data->numcols; column2d++)
    { if (data->cols[column].c == &data->cols[column2d])
      { column = column2d;
        break;
      }
    }
  }
  else if ((col >= 0) && (col < DISPLAY_ARRAY_MAX))
  { for (column2d = 0; column2d < data->numcols; column2d++)
    { if (data->cols[column2d].col == col)
      { if (column == MUIV_NList_SetColumnCol_Default)
          column = column2d;
        break;
      }
    }
  }

  if ((column >= 0) && (column < data->numcols) &&
      (column2d >= 0) && (column2d < data->numcols) &&
      (data->cols[column].c != &data->cols[column2d]))
  { LONG column2;
    for (column2 = 0; column2 < data->numcols; column2++)
    { if (data->cols[column2].c == &data->cols[column2d])
        break;
    }
    if ((column2 >= 0) && (column2 < data->numcols))
    { data->cols[column2].c = data->cols[column].c;
      data->cols[column].c = &data->cols[column2d];
/*
 *    data->do_draw_all = data->do_draw_title = data->do_draw = TRUE;
 *    data->do_parse = data->do_setcols = data->do_updatesb = data->do_wwrap = TRUE;
 *    do_draw = TRUE;
 */
      data->do_setcols = TRUE;
      DO_NOTIFY(NTF_Columns);
      REDRAW_FORCE;
    }
  }
  return (0);
}


LONG NL_ColWidth(struct NLData *data,LONG col,LONG width)
{
  LONG column = NL_ColToColumn(data,col);
  WORD userwidth = (WORD) width;

  if (userwidth < 4)  /* < 4 is considered as MUIV_NList_ColWidth_Default */
    userwidth = -1;
  if (userwidth > 2000)
    userwidth = 2000;
  if (width == MUIV_NList_ColWidth_Get)
  {
    if ((column >= 0) && (column < data->numcols))
      return ((LONG) data->cols[column].c->userwidth);
  }
  else if (col == MUIV_NList_ColWidth_All)
  {
    for (column = 0;column < data->numcols;column++)
    {
      if (data->cols[column].c->userwidth != userwidth)
      {
        if (userwidth < 0)
          data->cols[column].c->colwidthbiggestptr = -2;
        data->cols[column].c->userwidth = userwidth;
        data->do_setcols = TRUE;
      }
    }
    if (data->do_setcols)
    {
      REDRAW_FORCE;
    }
  }
  else if ((column >= 0) && (column < data->numcols) && (data->cols[column].c->userwidth != userwidth))
  {
    if (userwidth < 0)
      data->cols[column].c->colwidthbiggestptr = -2;
    data->cols[column].c->userwidth = userwidth;
    data->do_setcols = TRUE;
    REDRAW_FORCE;
    return (userwidth);
  }
  return (0);
}


BYTE *NL_Columns(struct NLData *data,BYTE *columns)
{ LONG column;
  if (columns)
  { column = 0;
    while ((column < data->numcols) && (columns[column] != -1))
    {
      NL_SetCol(data,column,columns[column]);
      column++;
    }
  }
  for (column = 0; column < data->numcols; column++)
  { data->column[column] = data->cols[column].c->col;
  }
  data->column[column] = -1;
  return (data->column);
}


IPTR mNL_ColToColumn(struct IClass *cl,Object *obj,struct MUIP_NList_ColToColumn *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  return ((IPTR) NL_ColToColumn(data,msg->col));
}


IPTR mNL_ColumnToCol(struct IClass *cl,Object *obj,struct MUIP_NList_ColumnToCol *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  return ((IPTR) NL_ColumnToCol(data,msg->column));
}


IPTR mNL_SetColumnCol(struct IClass *cl,Object *obj,struct MUIP_NList_SetColumnCol *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  LONG retval;
  retval = NL_SetCol(data,msg->column,msg->col);
  DONE_NOTIFY(NTF_Columns);
  return ((IPTR) retval);
}


IPTR mNL_List_ColWidth(struct IClass *cl,Object *obj,struct MUIP_NList_ColWidth *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  return ((IPTR) NL_ColWidth(data,msg->col,msg->width));
}



IPTR mNL_ContextMenuBuild(struct IClass *cl,Object *obj,struct MUIP_ContextMenuBuild *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  Object *MenuObj = NULL;
  LONG column;
  LONG mo = 0;
  BOOL do_it = FALSE;
  BOOL order_it = FALSE;

  if (data->NList_Disabled)
    return (0);

  if((mo = xget(obj, MUIA_ContextMenu)))
  {
    if ((mo & 0x9d510030) != 0x9d510030)
    {
      if (data->MenuObj)
      { MUI_DisposeObject(data->MenuObj);
        data->MenuObj = NULL;
      }
      return(DoSuperMethodA(cl,obj,(Msg) msg));
    }
    else if (mo == (LONG)MUIV_NList_ContextMenu_Never)
    {
      data->ContextMenuOn = FALSE;
      data->NList_ContextMenu = MUIV_NList_ContextMenu_Never;
      nnset(obj,MUIA_ContextMenu,NULL);
    }
    for (column = 0;column < data->numcols;column++)
    {
      if (data->cols[column].c->userwidth >= 0)
        do_it = TRUE;
      if (data->cols[column].c != &data->cols[column])
        order_it = TRUE;
    }

    /* sba: Contextmenu problem: Disabled */
/*    if (data->numcols > 1) */
    if (data->numcols > 0)
    { Object *mithis = NULL;
      struct MUI_NList_TestPos_Result res;
      LONG flags,ontop;

      data->click_x = msg->mx;
      data->click_y = msg->my;
      res.char_number = -2;
      NL_List_TestPos(data,MUI_MAXMAX,0,&res);
      column = (LONG) res.column;
      flags = (LONG) res.flags;
      if (res.flags & MUI_NLPR_TITLE)
        ontop = TRUE;
      else
        ontop = FALSE;

      MenuObj = (Object *) DoMethod(obj,MUIM_NList_ContextMenuBuild,msg->mx,msg->my,res.entry,column,flags,ontop);

      if ((IPTR)MenuObj == (IPTR)-1)
        return (0);

      if (!MenuObj)
      { if (!data->MenuObj)
          data->MenuObj = MUI_MakeObject(MUIO_MenustripNM,MenuData,0);
        MenuObj = data->MenuObj;
      }

      if (MenuObj && (mithis = (Object *) DoMethod(MenuObj,MUIM_FindUData,MUIV_NList_Menu_DefWidth_This)))
      {
        if ((res.column >= 0) && (res.column < data->numcols) &&
            !(res.flags & MUI_NLPR_BAR) && (data->cols[res.column].c->userwidth > 0))
        { nnset(mithis,MUIA_Menuitem_Enabled,TRUE);
        }
        else
        { nnset(mithis,MUIA_Menuitem_Enabled,FALSE);
        }
      }
      if (MenuObj && (mithis = (Object *) DoMethod(MenuObj,MUIM_FindUData,MUIV_NList_Menu_DefWidth_All)))
      { if (do_it)
        { nnset(mithis,MUIA_Menuitem_Enabled,TRUE);
        }
        else
        { nnset(mithis,MUIA_Menuitem_Enabled,FALSE);
        }
      }
      if (MenuObj && (mithis = (Object *) DoMethod(MenuObj,MUIM_FindUData,MUIV_NList_Menu_DefOrder_This)))
      {
        if ((res.column >= 0) && (res.column < data->numcols) &&
            !(res.flags & MUI_NLPR_BAR) && (data->cols[res.column].c != &data->cols[res.column]))
        { nnset(mithis,MUIA_Menuitem_Enabled,TRUE);
        }
        else
        { nnset(mithis,MUIA_Menuitem_Enabled,FALSE);
        }
      }
      if (MenuObj && (mithis = (Object *) DoMethod(MenuObj,MUIM_FindUData,MUIV_NList_Menu_DefOrder_All)))
      { if (order_it)
        { nnset(mithis,MUIA_Menuitem_Enabled,TRUE);
        }
        else
        { nnset(mithis,MUIA_Menuitem_Enabled,FALSE);
        }
      }
      return ((IPTR) MenuObj);
    }
  }

  return (0);
}



IPTR mNL_ContextMenuChoice(struct IClass *cl,Object *obj,struct MUIP_ContextMenuChoice *msg)
{
  struct NLData *data = INST_DATA(cl,obj);

  if (data->NList_Disabled)
    return (0);

  if (msg->item)
  {
    if (muiUserData(msg->item) == MUIV_NList_Menu_DefWidth_This)
    {
      struct MUI_NList_TestPos_Result res;

      res.char_number = -2;
      NL_List_TestPos(data,MUI_MAXMAX,0,&res);
      if ((res.column >= 0) && (res.column < data->numcols) && !(res.flags & MUI_NLPR_BAR))
        NL_ColWidth(data,NL_ColumnToCol(data,res.column),MUIV_NList_ColWidth_Default);
    }
    else if (muiUserData(msg->item) == MUIV_NList_Menu_DefWidth_All)
      NL_ColWidth(data,MUIV_NList_ColWidth_All,MUIV_NList_ColWidth_Default);
    else if (muiUserData(msg->item) == MUIV_NList_Menu_DefOrder_This)
    {
      struct MUI_NList_TestPos_Result res;

      res.char_number = -2;
      NL_List_TestPos(data,MUI_MAXMAX,0,&res);
      if ((res.column >= 0) && (res.column < data->numcols) && !(res.flags & MUI_NLPR_BAR))
        NL_SetCol(data,res.column,MUIV_NList_SetColumnCol_Default);
    }
    else if (muiUserData(msg->item) == MUIV_NList_Menu_DefOrder_All)
      NL_SetCol(data,MUIV_NList_SetColumnCol_Default,MUIV_NList_SetColumnCol_Default);
    else
      return(DoSuperMethodA(cl,obj,(Msg) msg));
  }

  return (0);
}
