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

#include <stdio.h>
#include <string.h>

#include <graphics/gfxmacros.h>
#undef GetOutlinePen

#include <intuition/imageclass.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "private.h"

#include "NList_func.h"

void DrawBackground(Object *obj, LONG left, LONG top, LONG width, LONG height, LONG xoff, LONG yoff)
{
  // MUI 3.9 of AmigaOS 4.1 update #3 and MUI4 treat the offsets as real offsets relative to the given
  // coordinates and not as absolute corrdinates.
  // Since all offsets are relative now these must be adapted for older versions of MUI.
  #if !defined(__MORPHOS__) && !defined(__AROS__)
  if(MUIMasterBase->lib_Version < 20 || (MUIMasterBase->lib_Version == 20 && MUIMasterBase->lib_Revision < 2326))
  #endif
  {
    xoff += left;
    yoff += top;
  }
  DoMethod(obj, MUIM_DrawBackground, left, top, width, height, xoff, yoff, 0);
}

WORD DrawTitle(struct NLData *data,LONG minx,LONG maxx,WORD hfirst)
{
  Object *obj = data->this;
  WORD linelen = 3;
#ifdef DO_CLIPPING
  APTR clippinghandle;
  clippinghandle = MUI_AddClipping(muiRenderInfo(obj),(WORD) data->mleft,(WORD) _mtop(obj),
                                          (WORD) data->mwidth,(WORD) data->vinc);
#endif

  if (minx < data->vleft)
    minx = data->vleft;
  if (maxx > data->vright + 1)
    maxx = data->vright + 1;

  if (minx < data->mleft)
    minx = data->mleft;
  if (maxx > data->mleft + data->mwidth)
    maxx = data->mleft + data->mwidth;
  if (minx >= maxx)
    return(0);

  SetBackGround(data->NList_TitleBackGround)
  DrawBackground(obj, minx, data->vdtitlepos, maxx-minx, data->vdtitleheight, hfirst + data->vdx, data->vdy);
  if (data->NList_TitleSeparator)
  {
    SetAPen(data->rp,data->pens[MPEN_SHADOW]);
    Move(data->rp, minx, data->vpos - 2);
    Draw(data->rp, maxx-1, data->vpos - 2);
    SetAPen(data->rp,data->pens[MPEN_SHINE]);
    Move(data->rp, minx, data->vpos - 1);
    Draw(data->rp, maxx-1, data->vpos - 1);
  }
  linelen = DrawText(data,-1,data->hpos - hfirst,data->vdtitlepos + data->voff,minx,maxx,MUIPEN(data->NList_TitlePen),data->hinc,FALSE);
  data->do_draw_title = FALSE;
#ifdef DO_CLIPPING
  MUI_RemoveClipping(muiRenderInfo(obj),clippinghandle);
#endif
  return (linelen);
}


void DrawOldLine(struct NLData *data,LONG ent,LONG minx,LONG maxx,WORD hfirst)
{
  Object *obj = data->this;
  ULONG mypen;
  BOOL forcepen = FALSE;
  BOOL drawtxt;
  LONG vert1,vert2,vertd;
#ifdef DO_CLIPPING
  APTR clippinghandle;
#endif

  if (minx < data->vleft)
    minx = data->vleft;
  if (maxx > data->vright + 1)
    maxx = data->vright + 1;

  if (minx < data->mleft)
    minx = data->mleft;
  if (maxx > data->mleft + data->mwidth)
    maxx = data->mleft + data->mwidth;
  if (minx >= maxx)
    return;

#ifdef DO_CLIPPING
  clippinghandle = MUI_AddClipping(muiRenderInfo(obj),(WORD) data->mleft,(WORD) data->mtop,
                                          (WORD) data->mwidth,(WORD) data->mheight);
#endif
  drawtxt = TRUE;
  vert1 = data->vpos+(data->vinc * (ent - data->NList_AffFirst));
  vert2 = data->vpos + (ent*data->vinc);
  vertd = data->vinc;
  if (data->NList_First_Incr)
  { if (ent == data->NList_First)
    { vertd -= data->NList_First_Incr;
      vert2 += data->NList_First_Incr;
      drawtxt = FALSE;
    }
    else if (ent == data->NList_First + data->NList_Visible)
    { vertd = data->NList_First_Incr;
      vert1 -= data->NList_First_Incr;
      drawtxt = FALSE;
    }
    else
    { vert1 -= data->NList_First_Incr;
    }
  }
  if ((ent < 0) || (ent >= data->NList_Entries))
  {
    SetBackGround(data->NList_ListBackGround); mypen = data->NList_ListPen;
    DrawBackground(obj, minx, vert1, maxx-minx, vertd, hfirst+data->vdx, vert2-vert1+data->vdy);
  }
  else
  {
    if (data->EntriesArray[ent]->Select == TE_Select_None)
    {
      SetBackGround(data->NList_ListBackGround); mypen = data->NList_ListPen; forcepen = FALSE;
    }
    else
    {
      if(data->NList_ActiveObjectOnClick == TRUE && (data->isActiveObject == FALSE || xget(_win(obj), MUIA_Window_Activate) == FALSE))
      {
        SetBackGround(MUII_myListInactive); mypen = data->NList_InactivePen; forcepen = data->ForcePen;
      }
      else
      {
        SetBackGround(MUII_myListSelect); mypen = data->NList_SelectPen; forcepen = data->ForcePen;
      }
    }

    DrawBackground(obj, minx, vert1, maxx-minx, vertd, hfirst+data->vdx, vert2-vert1+data->vdy);
    if (drawtxt)
      DrawText(data,ent,data->hpos-hfirst,vert1+data->voff,minx,maxx-1,MUIPEN(mypen),data->hinc,forcepen);
  }
#ifdef DO_CLIPPING
  MUI_RemoveClipping(muiRenderInfo(obj),clippinghandle);
#endif
}


WORD DrawLines(struct NLData *data,LONG e1,LONG e2,LONG minx,LONG maxx,WORD hfirst,WORD hmax,WORD small,BOOL do_extrems,WORD not_all)
{
  Object *obj = data->this;
  LONG ent,ent2,ent3,ent4,dent,lim1,lim2,lim3;
  WORD cursel,linelen=0;
  ULONG mypen;
  BOOL forcepen = FALSE;
  LONG vert1,vert2,vertd,vert3;
  WORD hfx = data->mleft - hfirst;
#ifdef DO_CLIPPING
  APTR clippinghandle = NULL;
  BOOL doclip = FALSE;
#endif

  //D(bug( "DRAWING LINES FROM %ld - %ld\n", e1, e2 ));

  if (small > 1)
  { lim1 = 3;
    lim2 = 2;
    lim3 = 2;
  }
  else if (small == 1)
  { lim1 = 8;
    lim2 = 4;
    lim3 = 4;
  }
  else
  { lim1 = 14;
    lim2 = 7;
    lim3 = 8;
  }

  if (minx < data->vleft)
    minx = data->vleft;
  if (maxx > data->vright + 1)
    maxx = data->vright + 1;

  if ((e2 - e1) == 1)
  { if ((e1 == data->minx_change_entry) && (minx < hfx + data->minx_change_offset))
      minx = hfx + data->minx_change_offset;
    if ((e1 == data->maxx_change_entry) && (maxx > hfx + data->maxx_change_offset))
      maxx = hfx + data->maxx_change_offset;
  }
  if (minx < data->mleft)
    minx = data->mleft;
  if (maxx > data->mleft + data->mwidth)
    maxx = data->mleft + data->mwidth;
  if (minx >= maxx)
    return (hmax);

  ent4 = ent2 = e2;
  if (ent2 > data->NList_Entries)
    ent2 = data->NList_Entries;
  ent = e1;
  if (ent < 0)
    ent = 0;

#ifdef DO_CLIPPING
  if (data->NList_First_Incr &&
      ((e1 <= data->NList_First) || (e2 >= data->NList_First + data->NList_Visible)))
  { doclip = TRUE;
    clippinghandle = MUI_AddClipping(muiRenderInfo(obj),
                                     (WORD) data->mleft,
                                     (WORD) data->vpos,
                                     (WORD) data->mwidth,
                                     (WORD) (data->NList_Visible*data->vinc));
  }
#endif
  while (ent < ent2)
  {
    forcepen = FALSE;
    if ((ent != data->NList_First) && (ent != (data->NList_First + data->NList_Visible - 1)) &&
        (((not_all == 1) && ((ent - data->NList_First) & 1L)) ||
         ((not_all == 2) && !((ent - data->NList_First) & 1L))))
    { ent++;
      continue;
    }
    if (!data->NList_TypeSelect || ((ent != data->sel_pt[data->min_sel].ent) && (ent != data->sel_pt[data->max_sel].ent)))
    {
      if (data->NList_TypeSelect)
      {
        if ((ent > data->sel_pt[data->min_sel].ent) && (ent < data->sel_pt[data->max_sel].ent))
        {
          ent3 = ent + 1;
          if (!not_all)
          {
            while ((ent3 < ent2) && (ent3 < data->sel_pt[data->max_sel].ent))
              ent3++;
            dent = ent3 - ent;
            if (dent >= 14)
              ent3 = ent + 7;
            else if (dent >= 8)
              ent3 = ent + (dent / 2);
          }

          if(data->NList_ActiveObjectOnClick == TRUE && (data->isActiveObject == FALSE || xget(_win(obj), MUIA_Window_Activate) == FALSE))
          {
            SetBackGround(MUII_myListInactive); mypen = data->NList_InactivePen; forcepen = data->ForcePen;
          }
          else
          {
            SetBackGround(MUII_myListCursor); mypen = data->NList_CursorPen; forcepen = data->ForcePen;
          }
        }
        else
        {
          ent3 = ent + 1;
          if (!not_all)
          {
            while ((ent3 < ent2) && ((ent3 > data->sel_pt[data->max_sel].ent) || (ent3 < data->sel_pt[data->min_sel].ent)))
              ent3++;
            dent = ent3 - ent;
            if (dent >= lim1)
              ent3 = ent + lim2;
            else if (dent >= lim3)
              ent3 = ent + (dent / 2);
          }

          SetBackGround(data->NList_ListBackGround); mypen = data->NList_ListPen;
        }
      }
      else
      {
        if(ent == data->NList_Active)
        {
          ent3 = ent + 1;

          if(data->NList_ActiveObjectOnClick == TRUE && (data->isActiveObject == FALSE || xget(_win(obj), MUIA_Window_Activate) == FALSE))
          {
            SetBackGround(MUII_myListInactive); mypen = data->NList_InactivePen; forcepen = data->ForcePen;
          }
          else if(data->EntriesArray[ent]->Select == TE_Select_None)
          {
            SetBackGround(MUII_myListUnselCur); mypen = data->NList_UnselCurPen; forcepen = data->ForcePen;
          }
          else
          {
            SetBackGround(MUII_myListCursor); mypen = data->NList_CursorPen; forcepen = data->ForcePen;
          }

          data->do_draw_active = FALSE;
        }
        else
        { cursel = data->EntriesArray[ent]->Select;
          ent3 = ent + 1;
          if (!not_all)
          { while ((ent3 < ent2) && (data->EntriesArray[ent3]->Select == cursel) && (ent3 != data->NList_Active))
              ent3++;
            dent = ent3 - ent;
            if (dent >= 14)
              ent3 = ent + 7;
            else if (dent >= 8)
              ent3 = ent + (dent / 2);
          }

          if(cursel == TE_Select_None)
          {
            SetBackGround(data->NList_ListBackGround); mypen = data->NList_ListPen;
          }
          else if(data->NList_ActiveObjectOnClick == TRUE && (data->isActiveObject == FALSE || xget(_win(obj), MUIA_Window_Activate) == FALSE))
          {
            SetBackGround(MUII_myListInactive); mypen = data->NList_InactivePen; forcepen = data->ForcePen;
          }
          else
          {
            SetBackGround(MUII_myListSelect); mypen = data->NList_SelectPen; forcepen = data->ForcePen;
          }
        }
      }

      vert3 = vert1 = data->vpos+(data->vinc * (ent - data->NList_First));
      vert2 = data->vpos + (ent*data->vinc);
      vertd = data->vinc * (ent3 - ent);
      if (data->NList_First_Incr)
      { vert1 -= data->NList_First_Incr;
        if (ent == data->NList_First)
        { vertd -= data->NList_First_Incr;
          vert2 += data->NList_First_Incr;
        }
        else if (ent3-1 >= data->NList_First + data->NList_Visible)
        { vertd = vertd - data->vinc + data->NList_First_Incr;
          vert3 -= data->NList_First_Incr;
        }
        else
        { vert3 -= data->NList_First_Incr;
        }
      }
      DrawBackground(obj, minx, vert3, maxx-minx, vertd, hfirst+data->vdx, vert2-vert3+data->vdy);
      while (ent < ent3)
      {
#ifndef DO_CLIPPING
        if (!data->NList_First_Incr || do_extrems || ((ent > data->NList_First) && (ent < data->NList_First + data->NList_Visible)))
#endif
        {
          linelen = DrawText(data,ent,data->hpos-hfirst,vert1+data->voff,minx,maxx-1,MUIPEN(mypen),data->hinc,forcepen);
          if(linelen > hmax)
            hmax = linelen;
        }
        vert1 += data->vinc;
        ent++;
      }
    }
    else
    {
      WORD x1,x2,x3,x4;

      x1 = minx;
      x4 = maxx;
      if ((ent == data->minx_change_entry) && (x1 < hfx + data->minx_change_offset))
        x1 = hfx + data->minx_change_offset;
      if ((ent == data->maxx_change_entry) && (x4 > hfx + data->maxx_change_offset))
        x4 = hfx + data->maxx_change_offset;
      x2 = hfx + data->sel_pt[data->min_sel].xoffset;
      x3 = hfx + data->sel_pt[data->max_sel].xoffset;
      if (x2 < x1) x2 = x1;
      if (x3 < x1) x3 = x1;
      if (x2 > x4) x2 = x4;
      if (x3 > x4) x3 = x4;
      if (ent != data->sel_pt[data->min_sel].ent)
        x2 = x1;
      if (ent != data->sel_pt[data->max_sel].ent)
        x3 = x4;

      vert3 = vert1 = data->vpos+(data->vinc * (ent - data->NList_First));
      vert2 = data->vpos + (ent*data->vinc);
      vertd = data->vinc;
      if (data->NList_First_Incr)
      { vert1 -= data->NList_First_Incr;
        if (ent == data->NList_First)
        { vertd -= data->NList_First_Incr;
          vert2 += data->NList_First_Incr;
        }
        else if (ent >= data->NList_First + data->NList_Visible)
        { vertd = vertd - data->vinc + data->NList_First_Incr;
          vert3 -= data->NList_First_Incr;
        }
        else
        { vert3 -= data->NList_First_Incr;
        }
      }

      if(x1 < x2)
      {
        SetBackGround(data->NList_ListBackGround);
        DrawBackground(obj, x1, vert3, x2-x1, vertd, hfirst+data->vdx, vert2-vert3+data->vdy);
      }
      if (x2 < x3)
      {
        if(data->NList_ActiveObjectOnClick == TRUE && (data->isActiveObject == FALSE || xget(_win(obj), MUIA_Window_Activate) == FALSE))
        {
          SetBackGround(MUII_myListInactive);
        }
        else
        {
          SetBackGround(MUII_myListCursor);
        }

        DrawBackground(obj, x2, vert3, x3-x2, vertd, hfirst+data->vdx, vert2-vert3+data->vdy);
      }
      if (x3 < x4)
      {
        SetBackGround(data->NList_ListBackGround);
        DrawBackground(obj, x3, vert3, x4-x3, vertd, hfirst+data->vdx, vert2-vert3-data->vdy);
      }

      // FIXME: This isn't perfect, but it is the fastest 'solution'
      // to fixing the distorted text output when the text is
      // italic and anti-aliased. Please note that 'spacesize' isn't entirely
      // correct as we should moved back by what the char width really is
      // at the 'x1-1' position. However, this seems to be problematic here
      // as we don't really have access to this informaton here?!?!
      if(data->NList_TypeSelect)
        x1 -= data->spacesize;

      x2 = hfx + data->sel_pt[data->min_sel].xoffset;
      x3 = hfx + data->sel_pt[data->max_sel].xoffset;
      if (x2 < x1) x2 = x1;
      if (x3 < x1) x3 = x1;
      if (x2 > x4) x2 = x4;
      if (x3 > x4) x3 = x4;
      if (ent != data->sel_pt[data->min_sel].ent)
        x2 = x1;
      if (ent != data->sel_pt[data->max_sel].ent)
        x3 = x4;
#ifndef DO_CLIPPING
      if (!data->NList_First_Incr || do_extrems || ((ent > data->NList_First) && (ent < data->NList_First + data->NList_Visible)))
#endif
      {
        if (x1 < x2)
        {
          mypen = data->NList_ListPen;
          linelen = DrawText(data,ent,data->hpos-hfirst,vert1+data->voff,x1,x2-1,MUIPEN(mypen),0,FALSE);
        }
        if (x2 < x3)
        {
          mypen = data->NList_CursorPen;
          linelen = DrawText(data,ent,data->hpos-hfirst,vert1+data->voff,x2,x3-1,MUIPEN(mypen),0,data->ForcePen);
        }
        if (x3 < x4)
        {
          mypen = data->NList_ListPen;
          linelen = DrawText(data,ent,data->hpos-hfirst,vert1+data->voff,x3,x4-1,MUIPEN(mypen),0,FALSE);
        }
        if (linelen > hmax)
          hmax = linelen;
      }
      ent++;
    }
  }

  if (ent < ent4)
  {
    SetBackGround(data->NList_ListBackGround);
    vert3 = data->vpos+(data->vinc * (ent - data->NList_First));
    vert2 = data->vpos + (ent*data->vinc);
    vertd = data->vinc * (ent4 - ent);
    if (data->NList_First_Incr)
    { if (ent == data->NList_First)
      { vertd -= data->NList_First_Incr;
        vert2 += data->NList_First_Incr;
      }
      else if (ent4-1 >= data->NList_First + data->NList_Visible)
      { vertd = vertd - data->vinc + data->NList_First_Incr;
        vert3 -= data->NList_First_Incr;
      }
      else
      { vert3 -= data->NList_First_Incr;
      }
    }
    DrawBackground(obj, minx, vert3, maxx-minx, vertd, hfirst+data->vdx, vert2-vert3+data->vdy);

    { struct colinfo *cinfo;
      WORD column,xbar,maxx2;
      xbar = data->hpos-hfirst + data->cols[0].c->minx - 8;
      for (column = 0;column < data->numcols;column++)
      { cinfo = data->cols[column].c;
        if (cinfo->maxx >= 0)
          maxx2 = data->hpos-hfirst + cinfo->maxx;
        else
          maxx2 = data->mright;
        xbar = maxx2;
        vert2 = vert3 + vertd - 1;
        if ((column < data->numcols-1) && (cinfo->delta > 0))
          xbar += ((cinfo->delta-1) / 2);
        if (IS_BAR(column,cinfo) &&
            ((cinfo->bar & 1) || ((cinfo->bar & 2) && (ent == -1))) && (cinfo->delta > 0))
        { if ((xbar+1 >= data->mleft) && (xbar <= data->mright) &&
              (xbar+1 >= minx-1) && (xbar <= maxx+1) &&
              ((column < data->numcols-1) ||
               ((data->NList_Horiz_Entries <= data->NList_Horiz_Visible + data->NList_Horiz_First) &&
                (xbar < data->mright - 1 - data->NList_Horiz_First))))
          { if ((xbar >= data->mleft) && (xbar >= minx-1))
            { SetAPen(data->rp,data->pens[MPEN_SHADOW]);
              Move(data->rp, xbar, vert3);
              Draw(data->rp, xbar, vert2);
            }
            if (!(cinfo->bar & 4) && (xbar+1 <= data->mright) && (xbar+1 <= maxx+1))
            { SetAPen(data->rp,data->pens[MPEN_SHINE]);
              Move(data->rp, xbar+1, vert3);
              Draw(data->rp, xbar+1, vert2);
            }
          }
        }
      }
    }
  }
#ifdef DO_CLIPPING
  if (doclip)
    MUI_RemoveClipping(muiRenderInfo(obj),clippinghandle);
#endif
  return (hmax);
}




/*
#define MPEN_HALFSHINE  1
#define MPEN_BACKGROUND 2
#define MPEN_HALFSHADOW 3
*/

void NL_DrawTitleMark(struct NLData *data, LONG xf, WORD yf)
{
  APTR clippinghandle = NULL;
  Object *obj = data->this;

  if (!data->NList_PartialChar)
  {
    clippinghandle = MUI_AddClipping(muiRenderInfo(obj),(WORD) data->vleft,(WORD) data->vtop,
                                            (WORD) data->vwidth,(WORD) data->vheight);
  }

  switch (data->NList_TitleMark & MUIV_NList_TitleMark_TypeMask)
  {
    case MUIV_NList_TitleMark_Down :
      SetAPen(data->rp,data->pens[MPEN_SHADOW]);
      Move(data->rp, xf-1, yf);
      Draw(data->rp, xf-6, yf);
      Draw(data->rp, xf-6, yf+2);
      Draw(data->rp, xf-4, yf+4);
      SetAPen(data->rp,data->pens[MPEN_HALFSHADOW]);
      Move(data->rp, xf-5, yf+1);
      Draw(data->rp, xf-1, yf+1);
      Move(data->rp, xf-5, yf+2);
      Draw(data->rp, xf-1, yf+2);
      Move(data->rp, xf-4, yf+3);
      Draw(data->rp, xf-2, yf+3);
      Draw(data->rp, xf-3, yf+4);
      SetAPen(data->rp,data->pens[MPEN_SHINE]);
      Move(data->rp, xf-3, yf+5);
      Draw(data->rp, xf, yf+2);
      Draw(data->rp, xf, yf);
      break;
    case MUIV_NList_TitleMark_Up :
      SetAPen(data->rp,data->pens[MPEN_SHADOW]);
      Move(data->rp, xf-3, yf);
      Draw(data->rp, xf-6, yf+3);
      Draw(data->rp, xf-6, yf+5);
      SetAPen(data->rp,data->pens[MPEN_HALFSHADOW]);
      Move(data->rp, xf-3, yf+1);
      Draw(data->rp, xf-4, yf+2);
      Draw(data->rp, xf-2, yf+2);
      Move(data->rp, xf-5, yf+3);
      Draw(data->rp, xf-1, yf+3);
      Move(data->rp, xf-5, yf+4);
      Draw(data->rp, xf-1, yf+4);
      SetAPen(data->rp,data->pens[MPEN_SHINE]);
      Move(data->rp, xf-5, yf+5);
      Draw(data->rp, xf, yf+5);
      Draw(data->rp, xf, yf+3);
      Draw(data->rp, xf-2, yf+1);
      break;
    case MUIV_NList_TitleMark_Box :
      SetAPen(data->rp,data->pens[MPEN_SHADOW]);
      Move(data->rp, xf-2, yf);
      Draw(data->rp, xf-5, yf);
      Draw(data->rp, xf-5, yf+4);
      SetAPen(data->rp,data->pens[MPEN_HALFSHADOW]);
      Move(data->rp, xf-4, yf+1);
      Draw(data->rp, xf-2, yf+1);
      Move(data->rp, xf-4, yf+2);
      Draw(data->rp, xf-2, yf+2);
      Move(data->rp, xf-4, yf+3);
      Draw(data->rp, xf-2, yf+3);
      SetAPen(data->rp,data->pens[MPEN_SHINE]);
      Move(data->rp, xf-4, yf+4);
      Draw(data->rp, xf-1, yf+4);
      Draw(data->rp, xf-1, yf);
      break;
    case MUIV_NList_TitleMark_Circle :
      SetAPen(data->rp,data->pens[MPEN_SHADOW]);
      Move(data->rp, xf-2, yf);
      Draw(data->rp, xf-4, yf);
      Draw(data->rp, xf-5, yf+1);
      Draw(data->rp, xf-5, yf+3);
      SetAPen(data->rp,data->pens[MPEN_HALFSHADOW]);
      Move(data->rp, xf-4, yf+1);
      Draw(data->rp, xf-2, yf+1);
      Move(data->rp, xf-4, yf+2);
      Draw(data->rp, xf-2, yf+2);
      Move(data->rp, xf-4, yf+3);
      Draw(data->rp, xf-2, yf+3);
      SetAPen(data->rp,data->pens[MPEN_SHINE]);
      Move(data->rp, xf-4, yf+4);
      Draw(data->rp, xf-2, yf+4);
      Draw(data->rp, xf-1, yf+3);
      Draw(data->rp, xf-1, yf+1);
      break;
  }

  if (!data->NList_PartialChar)
  {
    MUI_RemoveClipping(muiRenderInfo(obj),clippinghandle);
  }
}


void NL_DrawTitleMark2(struct NLData *data, LONG xf, WORD yf)
{
  Object *obj = data->this;
  APTR clippinghandle = NULL;

  if (!data->NList_PartialChar)
  {
    clippinghandle = MUI_AddClipping(muiRenderInfo(obj),(WORD) data->vleft,(WORD) data->vtop,
                                            (WORD) data->vwidth,(WORD) data->vheight);
  }

  switch (data->NList_TitleMark2 & MUIV_NList_TitleMark2_TypeMask)
  {
    case MUIV_NList_TitleMark2_Down :
      SetAPen(data->rp,data->pens[MPEN_SHADOW]);
      Move(data->rp, xf-1, yf);
      Draw(data->rp, xf-5, yf);
      Move(data->rp, xf-5, yf+1);
      Draw(data->rp, xf-3, yf+3);
      SetAPen(data->rp,data->pens[MPEN_HALFSHADOW]);
      Move(data->rp, xf-4, yf+1);
      Draw(data->rp, xf-1, yf+1);
      Move(data->rp, xf-3, yf+2);
      Draw(data->rp, xf-2, yf+2);
      SetAPen(data->rp,data->pens[MPEN_SHINE]);
      Move(data->rp, xf-2, yf+3);
      Draw(data->rp, xf, yf+1);
      Draw(data->rp, xf, yf);
      break;
    case MUIV_NList_TitleMark2_Up :
      SetAPen(data->rp,data->pens[MPEN_SHADOW]);
      Move(data->rp, xf-3, yf);
      Draw(data->rp, xf-5, yf+2);
      Draw(data->rp, xf-5, yf+3);
      SetAPen(data->rp,data->pens[MPEN_HALFSHADOW]);
      Move(data->rp, xf-3, yf+1);
      Draw(data->rp, xf-2, yf+1);
      Move(data->rp, xf-4, yf+2);
      Draw(data->rp, xf-1, yf+2);
      SetAPen(data->rp,data->pens[MPEN_SHINE]);
      Move(data->rp, xf-4, yf+3);
      Draw(data->rp, xf, yf+3);
      Draw(data->rp, xf, yf+2);
      Draw(data->rp, xf-2, yf);
      break;
    case MUIV_NList_TitleMark2_Box :
      SetAPen(data->rp,data->pens[MPEN_SHADOW]);
      Move(data->rp, xf-2, yf);
      Draw(data->rp, xf-4, yf);
      Draw(data->rp, xf-4, yf+3);
      SetAPen(data->rp,data->pens[MPEN_HALFSHADOW]);
      Move(data->rp, xf-3, yf+1);
      Draw(data->rp, xf-2, yf+1);
      Move(data->rp, xf-3, yf+2);
      Draw(data->rp, xf-2, yf+2);
      SetAPen(data->rp,data->pens[MPEN_SHINE]);
      Move(data->rp, xf-3, yf+3);
      Draw(data->rp, xf-1, yf+3);
      Draw(data->rp, xf-1, yf);
      break;
    case MUIV_NList_TitleMark2_Circle :
      SetAPen(data->rp,data->pens[MPEN_SHADOW]);
      Move(data->rp, xf-2, yf);
      Draw(data->rp, xf-3, yf);
      Draw(data->rp, xf-4, yf+1);
      Draw(data->rp, xf-4, yf+2);
      SetAPen(data->rp,data->pens[MPEN_HALFSHADOW]);
      Move(data->rp, xf-3, yf+1);
      Draw(data->rp, xf-2, yf+1);
      Move(data->rp, xf-3, yf+2);
      Draw(data->rp, xf-2, yf+2);
      SetAPen(data->rp,data->pens[MPEN_SHINE]);
      Move(data->rp, xf-3, yf+3);
      Draw(data->rp, xf-2, yf+3);
      Draw(data->rp, xf-1, yf+2);
      Draw(data->rp, xf-1, yf+1);
      break;
  }

  if (!data->NList_PartialChar)
  {
    MUI_RemoveClipping(muiRenderInfo(obj),clippinghandle);
  }
}


LONG DrawText(struct NLData *data,LONG ent,LONG x,LONG y,LONG minx,LONG maxx,ULONG mypen,LONG dxpermit,BOOL forcepen)
{
  Object *obj = data->this;
  struct colinfo *cinfo;
  struct affinfo *afinfo;
  LONG linelen, next_x=0, x2, x2s, x2e, minx2, maxx2, minx3, maxx3, cmaxx;
  WORD xbar,xbar2,ybar,ybar2;
  char *ptr1;
  struct TextExtent te;
  WORD column, curclen, dcurclen, ni;

  if ((ent < -1) || (ent >= data->NList_Entries))
    return (0);

  if (((y - data->voff + data->vinc) < data->vtop) || ((y - data->voff) > data->vbottom))
  {
    if (ent > -1)
    {
      linelen = (LONG) data->EntriesArray[ent]->PixLen;
      return (linelen);
    }
  }

  SetABPenDrMd(data->rp,mypen,data->pens[MPEN_BACKGROUND],JAM1);

  NL_GetDisplayArray(data,ent);

  xbar = x + data->cols[0].c->minx - 8;

  linelen = 0;
  for (column = 0;column < data->numcols;column++)
  {
    cinfo = data->cols[column].c;

    x2 = x + cinfo->minx;
    minx2 = x + cinfo->minx;
    if (cinfo->maxx >= 0)
    {
      maxx2 = x + cinfo->maxx;
      cmaxx = maxx2;
    }
    else
    {
      maxx2 = data->mright;
      cmaxx = 100000;
    }

    xbar2 = xbar;
    xbar = maxx2;
    ybar = y - data->voff;
    ybar2 = ybar+data->vinc-1;
    if (ent == -1)
      ybar2 = ybar+data->vdtitleheight-1;
    if ((column < data->numcols-1) && (cinfo->delta > 0))
      xbar += ((cinfo->delta-1) / 2);
    if (IS_BAR(column,cinfo) &&
        ((cinfo->bar & 1) || ((cinfo->bar & 2) && (ent == -1))) && (cinfo->delta > 0))
    {
      if ((xbar+1 >= data->mleft) && (xbar <= data->mright) &&
          (xbar+1 >= minx-1) && (xbar <= maxx+1) &&
          ((column < data->numcols-1) ||
           ((data->NList_Horiz_Entries <= data->NList_Horiz_Visible + data->NList_Horiz_First) &&
            (xbar < data->mright - 1 - data->NList_Horiz_First))))
      {
        if ((xbar >= data->mleft) && (xbar >= minx-1))
        {
          SetAPen(data->rp,data->pens[MPEN_SHADOW]);
          Move(data->rp, xbar, ybar);
          Draw(data->rp, xbar, ybar2);
        }
        if (!(cinfo->bar & 4) && (xbar+1 <= data->mright) && (xbar+1 <= maxx+1))
        {
          SetAPen(data->rp,data->pens[MPEN_SHINE]);
          Move(data->rp, xbar+1, ybar);
          Draw(data->rp, xbar+1, ybar2);
        }
      }
    }
/*
    if ((ent == -1) && IS_BAR(column,cinfo) &&
        ((data->NList_TitleMark & MUIV_NList_TitleMark_ColMask) == cinfo->col))
    {
      NL_DrawTitleMark(data,xbar-1,ybar);
    }
    else if ((ent == -1) && IS_BAR(column,cinfo) &&
        ((data->NList_TitleMark2 & MUIV_NList_TitleMark2_ColMask) == cinfo->col))
    {
      NL_DrawTitleMark2(data,xbar-1,ybar);
    }
*/
    if (minx2 < data->mleft - data->NList_PartialChar)
      minx2 = data->mleft - data->NList_PartialChar;
    if (maxx2 > data->mright + data->NList_PartialChar)
      maxx2 = data->mright + data->NList_PartialChar;

    if (column+1 < data->numcols)
    {
      if (minx2 >= maxx2)
        continue;
      if (minx > maxx2)
        continue;
      if (maxx < minx2)
        continue;
    }
    minx3 = minx2;
    if (minx > minx3)
      minx3 = minx;
    maxx3 = maxx2;
    if (maxx < maxx3)
      maxx3 = maxx;
    minx3 -= (data->hinc + 2);
    maxx3 += (data->hinc + 2);

    if (DontDoColumn(data,ent,column))
      continue;

    ParseColumn(data,column,mypen);
    WidthColumn(data,column,0);

    if (IS_HLINE(cinfo->style))
    {
      WORD xb1,xb2,yb1,yb2,thick,nothick,xbe1=0,xbe2=0;

      yb1 = ybar;
      nothick = thick = 0;
      if (IS_HLINE_thick(cinfo->style) && (IS_HLINE_C(cinfo->style) || IS_HLINE_E(cinfo->style)))
        thick = 2;
      if (IS_HLINE_C(cinfo->style))
        yb1 += (data->vinc-1)/2;
      else if (IS_HLINE_E(cinfo->style))
      {
        yb1 += (data->vinc-1)/2;
        xbe1 = x2 + cinfo->xoffset - 3;
        xbe2 = xbe1 + cinfo->colwidth + 4;
      }
      else if (IS_HLINE_B(cinfo->style))
        yb1 += data->vinc-2;
      yb2 = yb1 + 1;
      if (thick)
      {
        yb1 -= 1;
        yb2 += 1;
      }
      else if (IS_HLINE_nothick(cinfo->style))
      {
        nothick = 1;
        if (IS_HLINE_B(cinfo->style))
          yb1 = yb2;
      }
      xb1 = xbar2+1;
      xb2 = xbar-1;
      if (xb1 < minx-1)
        xb1 = minx-1;
      if (xb2 > maxx+1)
        xb2 = maxx+1;
      if (xb1 < data->mleft)
        xb1 = data->mleft;
      if (xb2 > data->mright)
        xb2 = data->mright;
      if (xb1 < xb2)
      {
        if (IS_HLINE_E(cinfo->style))
        {
          if (xbe1 > xb2)
            xbe1 = xb2;
          if (xbe2 < xb1)
            xbe2 = xb1;
          if (xb1 <= xbe1)
          {
            if (!nothick)
            {
              SetAPen(data->rp,data->pens[MPEN_SHADOW]);
              Move(data->rp, xb1, yb1);
              Draw(data->rp, xbe1, yb1);
              SetAPen(data->rp,data->pens[MPEN_SHINE]);
              Move(data->rp, xb1, yb2);
              Draw(data->rp, xbe1, yb2);
              if (thick)
              {
                SetAPen(data->rp,data->HLINE_thick_pen);
                RectFill(data->rp, xb1, yb1+1, xbe1, yb1+2);
              }
            }
            else
            {
              SetAPen(data->rp,data->HLINE_thick_pen);
              Move(data->rp, xb1, yb1);
              Draw(data->rp, xbe1, yb1);
            }
          }
          if (xbe2 <= xb2)
          {
            if (!nothick)
            { SetAPen(data->rp,data->pens[MPEN_SHADOW]);
              Move(data->rp, xbe2, yb1);
              Draw(data->rp, xb2, yb1);
              SetAPen(data->rp,data->pens[MPEN_SHINE]);
              Move(data->rp, xbe2, yb2);
              Draw(data->rp, xb2, yb2);
              if (thick)
              {
                SetAPen(data->rp,data->HLINE_thick_pen);
                RectFill(data->rp, xbe2, yb1+1, xb2, yb1+2);
              }
            }
            else
            {
              SetAPen(data->rp,data->HLINE_thick_pen);
              Move(data->rp, xbe2, yb1);
              Draw(data->rp, xb2, yb1);
            }
          }
        }
        else
        {
          if (!nothick)
          {
            SetAPen(data->rp,data->pens[MPEN_SHADOW]);
            Move(data->rp, xb1, yb1);
            Draw(data->rp, xb2, yb1);
            SetAPen(data->rp,data->pens[MPEN_SHINE]);
            Move(data->rp, xb1, yb2);
            Draw(data->rp, xb2, yb2);
            if (thick)
            {
              SetAPen(data->rp,data->HLINE_thick_pen);
              RectFill(data->rp, xb1, yb1+1, xb2, yb1+2);
            }
          }
          else
          {
            SetAPen(data->rp,data->HLINE_thick_pen);
            Move(data->rp, xb1, yb1);
            Draw(data->rp, xb2, yb1);
          }
        }
      }
    }

/*    WidthColumn(data,column,0);*/

    x2 += cinfo->xoffset;

    if (column+1 == data->numcols)
    {
      if (cinfo->userwidth > 0)
        linelen = cinfo->userwidth + cinfo->minx - 1;
      else
      {
        linelen = cinfo->colwidth + cinfo->minx + cinfo->xoffset;
        if (IS_ALIGN_CENTER(cinfo->style) && (cinfo->dx > cinfo->colwidth))
          linelen = cinfo->minx + cinfo->dx - 1;
      }
      if (data->EntriesArray && (ent >= 0))
        data->EntriesArray[ent]->PixLen = linelen;
    }

    if ((x2 >= maxx2) || ((x2 + cinfo->colwidth) <= minx2))
      continue;

    x2s = x2;

/*
{
int xy, yx;

D(bug( "BEFORE ==============================>\n" ));

for( yx = 0; yx <= cinfo->ninfo; yx++ )
{
  afinfo = &data->aff_infos[yx];

  D(bug( "Drawing line len: %ld - '", afinfo->len ));

  for( xy = 0; xy < afinfo->len; xy++ )
  D(bug( "%1.1s", &afinfo->strptr[xy] ));

  D(bug( "'\n" ));
}

D(bug( "<====================================\n" ));
}
*/

    ni = 0;
    afinfo = &data->aff_infos[ni];

    while ((ni <= cinfo->ninfo) && (afinfo->len > 0) && (x2 < maxx2))
    {
      if (afinfo->style == STYLE_TAB)
        next_x = ((((x2-x2s) / data->tabsize) + 1) * data->tabsize) + x2s;
      else if ((afinfo->style == STYLE_FIXSPACE) || (afinfo->style == STYLE_SPACE))
        next_x = x2 + afinfo->len + afinfo->addinfo + afinfo->addchar;
      else if (afinfo->style == STYLE_IMAGE)
      {
        LONG dx,dx3,x3,dy,dy2;
        struct NImgList *nimg = (struct NImgList *) afinfo->strptr;

        x2 += 1;
        dx = afinfo->pen & 0x0000FFFF;
        dy = 0;
        dy2 = (afinfo->pen >> 16) & 0x0000FFFF;
        next_x = x2 + afinfo->len + afinfo->addinfo + 1;
        x3 = x2;
        dx3 = dx;
        if ((x3 + dx3) > maxx2)
          dx3 = maxx2 - x3;
        if (x3 < minx2)
        {
          dx3 -= (minx2 - x3);
          x3 = minx2;
        }

        if (nimg && nimg->NImgObj && (dx3 > 0) && (x2 <= maxx) && (next_x > minx))
        {
          struct IClass *realclass = OCLASS(obj);
          ULONG mad_Flags;
          APTR clippinghandle = NULL;
          WORD left,top,width,height,ol,ot;
          BOOL doclip = FALSE;
/*
 * if (muiRenderInfo(obj) != muiRenderInfo(nimg->NImgObj))
 * {
 * D(bug("%lx|MRI=%lx  img %lx|MRI=%lx\n",obj,muiRenderInfo(obj),nimg->NImgObj,muiRenderInfo(nimg->NImgObj)));
 * D(bug("%lx|MAD=%lx  img %lx|MAD=%lx\n",obj,muiAreaData(obj),nimg->NImgObj,muiAreaData(nimg->NImgObj)));
 * }
 */
          if (data->vinc > dy2)
            dy = (data->vinc - dy2 + 1)/2;
          left = _left(nimg->NImgObj);
          top = _top(nimg->NImgObj);
          width = _width(nimg->NImgObj);
          height = _height(nimg->NImgObj);
          ol = _left(nimg->NImgObj) = (WORD) x2;
          ot = _top(nimg->NImgObj) = (WORD) (y-data->voff+dy);
          if (dx > 0)
            _width(nimg->NImgObj) = dx;
          else
            _width(nimg->NImgObj) = 1;
          if (dy2 > 0)
            _height(nimg->NImgObj) = dy2;
          else
            _height(nimg->NImgObj) = data->vinc;
          if ((dy2 > data->vinc) || (dx3 < dx) || (_width(nimg->NImgObj) != nimg->width) || (_height(nimg->NImgObj) != nimg->height))
          {
            if (dy2 > data->vinc)
            {
              dy = 0;
              _height(nimg->NImgObj) = data->vinc;
              ot = _top(nimg->NImgObj) = (WORD) (y-data->voff+dy);
            }
            doclip = TRUE;
            clippinghandle = MUI_AddClipping(muiRenderInfo(obj),(WORD) x3,(WORD) (y-data->voff+dy),
                                                    (WORD) dx3,(WORD) data->vinc);
          }
          data->drawsuper = nimg->NImgObj;
          mad_Flags = muiAreaData(obj)->mad_Flags;
          OCLASS(obj) = data->ncl;
          if (!nimg->ImgName)
          {
            LONG vinc = (LONG) data->vinc;

            if (afinfo->tag)
            {
              SetAttrs(nimg->NImgObj, MUIA_NLIMG_EntryCurrent, ent,
                                      MUIA_NLIMG_EntryHeight, vinc,
                                      afinfo->tag, afinfo->tagval, TAG_DONE);
            }
            else
            {
              SetAttrs(nimg->NImgObj, MUIA_NLIMG_EntryCurrent, ent,
                                      MUIA_NLIMG_EntryHeight, vinc, TAG_DONE);
            }
          }
/*
 *          SetAttrs(nimg->NImgObj, MUIA_Image_State,IDS_SELECTED,
 *                                  MUIA_Selected, TRUE, TAG_DONE);
 *          SetAttrs(nimg->NImgObj, MUIA_Image_State,IDS_NORMAL,
 *                                  MUIA_Selected, FALSE, TAG_DONE);
 */
          _left(nimg->NImgObj) += nimg->dx;
          _top(nimg->NImgObj) += nimg->dy;
          if ((data->affbutton >= 0) && (data->affbutton == (LONG)afinfo->button))
          {
            if ((data->affbuttonstate == 2) || (data->affbuttonstate == -2))
            {
              muiAreaData(nimg->NImgObj)->mad_Flags &= ~MADF_VISIBLE;
              SetAttrs(nimg->NImgObj, MUIA_Image_State,IDS_SELECTED,
                                      MUIA_Selected, TRUE, TAG_DONE);
            }
            data->affbuttonpos.Left = (WORD) x3;
            data->affbuttonpos.Top = _top(nimg->NImgObj);
            data->affbuttonpos.Width = (WORD) dx3;
            data->affbuttonpos.Height = _height(nimg->NImgObj);

            muiAreaData(nimg->NImgObj)->mad_Flags |= MADF_VISIBLE;

            if(xget(nimg->NImgObj, MUIA_Disabled))
              set(nimg->NImgObj, MUIA_Disabled, FALSE);

            MUI_Redraw(nimg->NImgObj,MADF_DRAWALL);

            if ((data->affbuttonstate == 2) || (data->affbuttonstate == -2))
            {
              muiAreaData(nimg->NImgObj)->mad_Flags &= ~MADF_VISIBLE;
              SetAttrs(nimg->NImgObj, MUIA_Image_State,IDS_NORMAL,
                                      MUIA_Selected, FALSE, TAG_DONE);
            }
          }
          else
          {
            muiAreaData(nimg->NImgObj)->mad_Flags |= MADF_VISIBLE;

            if(xget(nimg->NImgObj, MUIA_Disabled))
              set(nimg->NImgObj, MUIA_Disabled, FALSE);

            MUI_Redraw(nimg->NImgObj,MADF_DRAWALL);
          }
          nimg->dx = _left(nimg->NImgObj) - ol;
          nimg->dy = _top(nimg->NImgObj) - ot;

          muiAreaData(obj)->mad_Flags = mad_Flags;
          OCLASS(obj) = realclass;
          data->drawsuper = NULL;
          if (doclip)
          {
            doclip = FALSE;
            MUI_RemoveClipping(muiRenderInfo(obj),clippinghandle);
            _width(nimg->NImgObj) = width;
            _height(nimg->NImgObj) = height;
          }
          _left(nimg->NImgObj) = left;
          _top(nimg->NImgObj) = top;
        }
        SetABPenDrMd(data->rp,afinfo->pen,data->pens[MPEN_BACKGROUND],JAM1);
        ReSetFont;
      }
      else if (afinfo->style == STYLE_IMAGE2)
      {
        LONG dx,dx2,dy,dy2;

        x2 += 1;
        dx = afinfo->pen & 0x0000FFFF;
        dx2 = 0;
        dy = 0;
        dy2 = (afinfo->pen >> 16) & 0x0000FFFF;
        next_x = x2 + afinfo->len + afinfo->addinfo + 1;
        if ((x2 + dx) > maxx2)
          dx = maxx2 - x2;
        if (x2 < minx2)
        {
          dx2 = minx2 - x2;
          x2 += dx2;
          dx -= dx2;
        }
        if ((dx > 0) && (x2 <= maxx) && (next_x > minx) && afinfo->strptr)
        {
          struct BitMapImage *bmimg = (struct BitMapImage *) afinfo->strptr;

          if (data->vinc > dy2)
            dy = (data->vinc - dy2 + 1)/2;
          BltMaskBitMapRastPort(&(bmimg->imgbmp), (WORD) dx2, (WORD) 0,
                                data->rp, (WORD) x2, (WORD) (y-data->voff+dy),
                                (WORD) dx, (WORD) dy2, (UBYTE) (ABC|ABNC|ANBC),bmimg->mask);
        }
        SetABPenDrMd(data->rp,afinfo->pen,data->pens[MPEN_BACKGROUND],JAM1);
        SetDrPt(data->rp,(UWORD)~0);
      }
      else
      {
        // we are having normal text in afinfo->strptr now, so lets
        // go and figure out if all of it fits in the current column or
        // if we have to strip it.

        // set foreground pen
        if (forcepen)
          SetAPen(data->rp,mypen);
        else
          SetAPen(data->rp,afinfo->pen);

        // set any softstyle in case the user text contains it
        SetSoftStyle(data->rp, GET_STYLE(afinfo->style), STYLE_MASK);

        if (afinfo->addchar > 0)
          data->rp->TxSpacing = afinfo->addchar;

        // figure out the pixel constraints the current text would require
        curclen = afinfo->len;
        ptr1 = afinfo->strptr;
        TextExtent(data->rp, ptr1, curclen, &te);
        if ((ni == 0) && (te.te_Extent.MinX < 0))
          x2 -= te.te_Extent.MinX;

        // save the coordinate of the next column start
        next_x = x2 + te.te_Width + afinfo->addinfo;
        D(DBF_DRAW, "next_x: %ld cmaxx: %ld");

        // save the coordinate where the current column ends in x2e
        x2e = x2 + te.te_Width;

        //////////////
        // from now on we try to figure out from where to where we are going
        // to strip the text in case it doesn't completely fit into the
        // current column

        // continue only if the start of the column (x2) does not exceed
        // the maximum field of view in x direction
        if((x2 < maxx2) && (x2 + te.te_Extent.MaxX > minx2))
        {
          int x2diff = 0;

          D(DBF_DRAW, "curclen: %ld ptr1: '%4.4s'", curclen, ptr1);
          D(DBF_DRAW, "x2: %ld x2e: %ld maxx: %ld maxx2: %ld maxx3: %ld minx: %ld minx2: %ld minx3: %ld", x2, x2e, maxx, maxx2, maxx3, minx, minx2, minx3);

          // check if the coordinate of the END of the string (x2e) does
          // not fit into the available regions (maximum x = maxx3). And
          // if so we go and calculate a new curclen
          if((curclen > 0) && (x2e > maxx3))
          {
            curclen = TextFit(data->rp, ptr1, curclen, &te, NULL, 1, maxx3 - x2, 32767);
            if(curclen > 0)
            {
              // find out the new end position of the string (x2e)
              TextExtent(data->rp, ptr1, curclen, &te);
              x2e = x2 + te.te_Width;

              D(DBF_DRAW, "RIGHT: x2: %ld x2e: %ld", x2, x2e);
            }
          }

          // check if the coordinate of the START of the string (x2) does
          // not fit into the available minimal x position (minx3). And if so,
          // we go and calculate a new curclen
          if((curclen > 0) && (x2 < minx3))
          {
            dcurclen = TextFit(data->rp, ptr1, curclen, &te, NULL, 1, minx3 - x2, 32767);
            curclen -= dcurclen;

            if(curclen > 0)
            {
              // move the start of the string by the number of characters
              // we had to reduce the amount of characters displayed
              ptr1 += dcurclen;
              x2 += te.te_Width;
              x2diff += te.te_Width;

              // find out the new end position of the string (x2e)
              TextExtent(data->rp, ptr1, curclen, &te);
              x2e = x2 + te.te_Width;

              D(DBF_DRAW, "LEFT: x2: %ld x2e: %ld", x2, x2e);
            }
          }

          if (curclen > 0)
          {
            TextExtent(data->rp, &ptr1[curclen-1], 1, &te);
            x2e -= te.te_Width;

            /* throw away chars on the right that souldn't be draw because out of list */
            while ((curclen > 0) && ((x2e + te.te_Extent.MaxX) > maxx2))
            {
              curclen--;
              if (curclen > 0)
              {
                TextExtent(data->rp, &ptr1[curclen-1], 1, &te);
                x2e -= te.te_Width;
              }
            }

            /* throw away chars on the right that i don't want to draw because out of maxx */
            if (dxpermit)
            {
              while ((curclen > 0) && ((x2e + te.te_Extent.MinX) > maxx))
              {
                curclen--;
                if (curclen > 0)
                {
                  TextExtent(data->rp, &ptr1[curclen-1], 1, &te);
                  x2e -= te.te_Width;
                }
              }
            }
            else
            {
              while ((curclen > 0) && (x2e > maxx))
              {
                curclen--;
                if (curclen > 0)
                {
                  TextExtent(data->rp, &ptr1[curclen-1], 1, &te);
                  x2e -= te.te_Width;
                }
              }
            }
          }

          if (curclen > 0)
          {
            TextExtent(data->rp, ptr1, 1, &te);

            /* throw away chars on the left that souldn't be draw because out of list */
            while ((curclen > 0) && ((x2 + te.te_Extent.MinX) < minx2))
            {
              x2 += te.te_Width;
              x2diff += te.te_Width;
              ptr1++;
              curclen--;
              TextExtent(data->rp, ptr1, 1, &te);
            }

            /* throw away chars on the left that i don't want to draw because out of minx */
            if (dxpermit)
            {
              while ((curclen > 0) && ((x2 + te.te_Extent.MaxX) < minx))
              {
                x2 += te.te_Width;
                x2diff += te.te_Width;
                ptr1++;
                curclen--;
                TextExtent(data->rp, ptr1, 1, &te);
              }
            }
            else
            {
              while ((curclen > 0) && ((x2+te.te_Width) <= minx))
              {
                x2 += te.te_Width;
                x2diff += te.te_Width;
                ptr1++;
                curclen--;
                TextExtent(data->rp, ptr1, 1, &te);
              }
            }
          }

          D(DBF_DRAW, "x2: %ld x2e: %ld cmaxx: %ld next_x: %ld cinfo->minx: %ld cinfo->maxx: %ld ", x2, x2e, cmaxx, next_x, cinfo->minx, cinfo->maxx);

          if(curclen > 0)
          {
            Move(data->rp, x2, y);

            // before we draw the text we check if it was clipped and if so
            // we add "..." at the right position in case the PartialCol
            // feature is turned on at all
            if(data->NList_PartialCol && cinfo->partcolsubst != PCS_DISABLED &&
               next_x > cmaxx && (xbar-1 >= data->mleft) && (xbar-1 <= data->mright))
            {
              int txtlen;
              int clen; // number of pixels the current text has for positioning it

              // first we get the extent of the "..." text we are going to add
              TextExtent(data->rp, "...", 3, &te);

              // if this is only a refresh of one/two chars because the user
              // is resizing the column we need to use a different clen
              clen = cmaxx - (x2 - x2diff);

              // only continue of the column fits the minimum of the three dots
              // at all
              if(clen >= te.te_Width)
              {
                char *txt;
                int cstart;

                D(DBF_DRAW, "c1: %ld, c2: %ld x2: %ld x2e: %ld cmaxx: %ld minx3: %ld x2diff: %ld | %ld'%s'", cinfo->maxx-cinfo->minx, cmaxx-(x2-x2diff), x2, x2e, cmaxx, minx3, x2diff, next_x, afinfo->strptr);

                D(DBF_DRAW, "clen: %ld afinfo->len: %ld str: %08lx ('%s')", clen, afinfo->len, afinfo->strptr, afinfo->strptr);

                // now that we add "..." at the right, left or center of the string we have to calculate the
                // TextExtent() in a kind of iterative process so that we identify at which point we need to
                // add our substitution "..." text.
                txtlen = strlen(afinfo->strptr);
                if((txt = AllocVecPooled(data->Pool, txtlen+3+1)) != NULL)
                {
                  if(cinfo->partcolsubst == PCS_LEFT)
                  {
                    snprintf(txt, txtlen+3+1, "...%s", afinfo->strptr);
                    cstart = 0;
                  }
                  else if(cinfo->partcolsubst == PCS_RIGHT)
                  {
                    strlcpy(txt, afinfo->strptr, txtlen+1);
                    strlcat(txt, "...", txtlen+3+1);
                    cstart = txtlen-1;
                  }
                  else if(cinfo->partcolsubst == PCS_CENTER)
                  {
                    strlcpy(txt, afinfo->strptr, txtlen/2);
                    strlcat(txt, "...", txtlen/2+3);
                    strlcat(txt, &afinfo->strptr[txtlen/2], txtlen+3+1);
                    cstart = txtlen/2-1;
                  }
                  else
                  {
                    // make sure we have a NUL terminated string in any case
                    txt[0] = '\0';
                    cstart = 0;
                  }

                  // get the new string length
                  txtlen = strlen(txt);

                  do
                  {
                    TextExtent(data->rp, txt, txtlen, &te);

                    D(DBF_DRAW, "te: %ld %ld: '%s'", te.te_Width, clen, txt);
                    if(te.te_Width <= clen)
                      break;

                    if(cinfo->partcolsubst == PCS_LEFT)
                    {
                      // move the text after the first "..." one to the left
                      // thus, actually moving the text start
                      memmove(&txt[3], &txt[4], txtlen+1);
                    }
                    else if(cinfo->partcolsubst == PCS_RIGHT)
                    {
                      // move the "..." text one char to the left
                      txt[txtlen-1] = '\0';
                      // make sure we don't cause a buffer underrun
                      if(txtlen > 4)
                        txt[txtlen-4] = '.';
                    }
                    else if(cinfo->partcolsubst == PCS_CENTER)
                    {
                      // we strip text from the center but alternating
                      // from the left portion and the from the right
                      // portion around the "..." text
                      if(txtlen % 2 == 0)
                      {
                        // move all text starting AT "..." one char to the left
                        // make sure we don't cause a buffer underrun
                        if(cstart > 0)
                        {
                          D(DBF_DRAW, "cstart: '%s'", &txt[cstart]);
                          memmove(&txt[cstart-1], &txt[cstart], strlen(&txt[cstart])+1);
                          cstart--;
                        }
                      }
                      else
                      {
                        // move all text starting AFTER "..." one char to the left
                        memmove(&txt[cstart+3], &txt[cstart+3+1], strlen(&txt[cstart+3+1])+1);
                      }
                    }

                    txtlen--;
                  }
                  while(TRUE);

                  D(DBF_DRAW, "curclen: %ld txtlen: %ld, %ld: '%s' '%s'", curclen, txtlen, ptr1-afinfo->strptr, ptr1, txt);

                  // calculate the len diff between afinfo->strptr and txt so that resizing
                  // a column does redraw the right thing.
                  if((ptr1-afinfo->strptr) > 0)
                  {
                    Text(data->rp, &txt[ptr1-afinfo->strptr], curclen);
                    D(DBF_DRAW, "argh: %ld %ld '%s'", x2, curclen, &txt[ptr1-afinfo->strptr]);
                  }
                  else
                    Text(data->rp, txt, txtlen);

                  FreeVecPooled(data->Pool, txt);
                }
              }
            }
            else
              Text(data->rp, ptr1, curclen);

            D(DBF_DRAW, "draw text4: %ld %ld %ld %ld %ld - %ld>%ld  '%s'", curclen, x2, next_x, maxx2, cmaxx, ni+1, cinfo->ninfo, ptr1);

/*
{
int xy;

D(bug( "Drawing line len: %ld, x: %ld, y: %ld - '", curclen, x2, y ));

for( xy = 0; xy < curclen; xy++ )
{
  D(bug( "%1.1s", &ptr1[xy] )); }
  D(bug( "'\n" ));
}

{
char txt[100];

snprintf( txt, sizeof(txt), "Drawing line len: %ld, x: %ld, y: %ld - '%%%ld.%lds'", curclen, x2, y, curclen, curclen );
D(bug( txt, ptr1 ));
}
*/
          }
        }

        if (afinfo->addchar > 0)
          data->rp->TxSpacing = 0;
      }

      x2 = next_x;
      ni++;
      afinfo = &data->aff_infos[ni];
    }

    if(data->NList_PartialCol && cinfo->partcolsubst == PCS_DISABLED &&
       (x2 > cmaxx) && (xbar-1 >= data->mleft) && (xbar-1 <= data->mright))
    {
      WORD yb;

      SetAPen(data->rp,data->pens[MPEN_SHINE]);
      for (yb = ybar+1; yb < ybar2; yb += 2)
        WritePixel(data->rp, xbar-1, yb);
      SetAPen(data->rp,data->pens[MPEN_SHADOW]);
      for (yb = ybar+2; yb < ybar2; yb += 2)
        WritePixel(data->rp, xbar-1, yb);
    }

    if ((ent == -1) && IS_BAR(column,cinfo) &&
        ((data->NList_TitleMark & MUIV_NList_TitleMark_ColMask) == cinfo->col))
    {
      NL_DrawTitleMark(data,xbar-1,ybar);
    }
    else if ((ent == -1) && IS_BAR(column,cinfo) &&
        ((data->NList_TitleMark2 & MUIV_NList_TitleMark2_ColMask) == cinfo->col))
    {
      NL_DrawTitleMark2(data,xbar-1,ybar);
    }
    if ((ent == -1) && !IS_BAR(column,cinfo) &&
        ((data->NList_TitleMark & MUIV_NList_TitleMark_ColMask) == cinfo->col))
    {
      NL_DrawTitleMark(data,next_x+6,ybar);
    }
    else if ((ent == -1) && !IS_BAR(column,cinfo) &&
        ((data->NList_TitleMark2 & MUIV_NList_TitleMark2_ColMask) == cinfo->col))
    {
      NL_DrawTitleMark2(data,next_x+6,ybar);
    }
  }
  SetSoftStyle(data->rp, 0, STYLE_MASK);

  return (linelen);
}


static LONG DrawEntryTextOnly(struct NLData *data,struct RastPort *rp,LONG ent,LONG col,UNUSED LONG x,LONG y,LONG minx,LONG maxx,ULONG mypen,BOOL draw)
{
  struct colinfo *cinfo;
  struct affinfo *afinfo;
  LONG next_x, x2, x2e,column;
  STRPTR ptr1;
  struct TextExtent te;
  WORD curclen, dcurclen, ni;

  NL_GetDisplayArray(data,ent);

  column = NL_ColToColumn(data,col);
  if (column < 0)
    return (0);

  cinfo = data->cols[column].c;

  if (!DontDoColumn(data,ent,column))
  {
    ParseColumn(data,column,mypen);
    WidthColumn(data,column,0);
    x2 = 1;

    ni = 0;
    afinfo = &data->aff_infos[ni];

    while ((ni <= cinfo->ninfo) && (afinfo->len > 0) && (x2 < maxx))
    {
      if ((afinfo->style == STYLE_TAB) || (afinfo->style == STYLE_FIXSPACE) || (afinfo->style == STYLE_SPACE))
        next_x = x2 + data->spacesize;
      else if ((afinfo->style == STYLE_IMAGE) || (afinfo->style == STYLE_IMAGE2))
        next_x = x2 + 2;
      else
      {
        SetAPen(rp,afinfo->pen);
        SetSoftStyle(rp, GET_STYLE(afinfo->style), STYLE_MASK);

        rp->TxSpacing = 0;

        curclen = afinfo->len;
        ptr1 = afinfo->strptr;

        TextExtent(rp, ptr1, curclen, &te);
        if ((ni == 0) && (te.te_Extent.MinX < 0))
          x2 -= te.te_Extent.MinX;
        next_x = x2 + te.te_Width;
        x2e = x2 + te.te_Width;

        if ((x2 < maxx) && (x2 + te.te_Extent.MaxX > minx))
        { /* skip most of unwanted chars on the right */
          if ((curclen > 0) && (x2e > maxx))
          { dcurclen = te.te_Extent.MaxX - te.te_Width - te.te_Extent.MinX  +2;
            curclen = TextFit(rp, ptr1, curclen, &te, NULL,1,maxx - x2 + dcurclen,32767);
            if (curclen > 0)
            { TextExtent(rp, ptr1, curclen, &te);
              x2e = x2 + te.te_Width;
            }
          }
          if (curclen > 0)
          { TextExtent(rp, &ptr1[curclen-1], 1, &te);
            x2e -= te.te_Width;
            /* throw away chars on the right that souldn't be draw because out of list */
            while ((curclen > 0) && ((x2e + te.te_Extent.MaxX) > maxx))
            { curclen--;
              if (curclen > 0)
              { TextExtent(rp, &ptr1[curclen-1], 1, &te);
                x2e -= te.te_Width;
              }
            }
          }
          if((curclen > 0) && draw)
          {
            Move(rp, x2, y);
            Text(rp, ptr1, curclen);
          }
        }
      }
      x2 = next_x;
      ni++;
      afinfo = &data->aff_infos[ni];
    }
    SetSoftStyle(rp, 0, STYLE_MASK);
    return (x2);
  }
  return (0);
}


LONG DrawDragText(struct NLData *data,BOOL draw)
{
  Object *obj = data->this;
  struct TextExtent te;
  struct RastPort *rp;
  LONG curclen,x,w;

  if (draw)
    rp = data->DragRPort;
  else
    rp = data->rp;

  if (rp && data->DragText)
  {
    char *text = data->DragText;

    x = 0;
    if (draw)
    {
      data->DragText = NULL;
      data->DragEntry = -1;
      SetBackGround(MUII_myListCursor);
      _rp(obj) = data->DragRPort;
      _left(obj) = 0;
      _top(obj) = 0;
      _width(obj) = data->DragWidth;
      _height(obj) = data->DragHeight;
      DrawBackground(obj, 0, 0, data->DragWidth, data->DragHeight, 0, 0);
      _rp(obj) = data->rp;
      _left(obj) = data->left;
      _top(obj) = data->top;
      _width(obj) = data->width;
      _height(obj) = data->height;
      SetABPenDrMd(data->DragRPort,MUIPEN(data->NList_CursorPen),data->pens[MPEN_BACKGROUND],JAM1);
    }
    SetSoftStyle(rp, 0, STYLE_MASK);
    curclen = strlen(text);
    curclen = TextFit(rp, text, curclen, &te, NULL,1,data->DragWidth,32767);
    if (te.te_Extent.MinX < 0)
      x -= te.te_Extent.MinX;
    w = te.te_Extent.MaxX - te.te_Extent.MinX;
    if (draw)
    {
      if (w < data->DragWidth)
        x += ((data->DragWidth - w) / 2);
      if (curclen > 0)
      {
        Move(rp, x, data->voff);
        Text(rp, text, curclen);
      }
    }
    return (w);
  }
  else if (rp && (data->DragEntry >= 0) && (data->NList_DragColOnly >= 0))
  {
    LONG ent = data->DragEntry;

    if (draw)
    {
      data->DragText = NULL;
      data->DragEntry = -1;
      SetBackGround(MUII_myListCursor);
      _rp(obj) = data->DragRPort;
      _left(obj) = 0;
      _top(obj) = 0;
      _width(obj) = data->DragWidth;
      _height(obj) = data->DragHeight;
      DrawBackground(obj, 0, 0, data->DragWidth, data->DragHeight, 0, 0);
      _rp(obj) = data->rp;
      _left(obj) = data->left;
      _top(obj) = data->top;
      _width(obj) = data->width;
      _height(obj) = data->height;
    }
    w = DrawEntryTextOnly(data,rp,ent,data->NList_DragColOnly,0,data->voff,0,data->DragWidth,MUIPEN(data->NList_CursorPen),draw);
    if (w > data->DragWidth)
      w = data->DragWidth;
    return (w);
  }
  else
  {
    data->DragText = NULL;
    data->DragEntry = -1;
  }
  return (4);
}


void DisposeDragRPort(struct NLData *data)
{
  if(data->DragRPort != NULL)
  {
    struct BitMap *bm = data->DragRPort->BitMap;

    FreeVecPooled(data->Pool, data->DragRPort);
    data->DragRPort = NULL;

    if(bm != NULL)
    {
      WaitBlit();
      FreeBitMap(bm);
    }
  }
}


struct RastPort *CreateDragRPort(struct NLData *data,LONG numlines,LONG first,LONG last)
{
  Object *obj = data->this;

  if(last >= first)
  {
    data->DragWidth = data->mwidth;
    data->DragHeight = data->vinc * numlines;

    return NULL;
  }
  else
  {
    static char text[40];

    data->DragHeight = data->vinc;
    data->DragWidth = data->mwidth;

    if(data->NList_DragColOnly < 0)
    {
      if(numlines == 1)
        strlcpy(text, "Dragging one item...", sizeof(text));
      else
        snprintf(text, sizeof(text), "Dragging %ld items...", (long)numlines);

      data->DragText = text;
      data->DragEntry = -1;
    }
    else
    {
      if(numlines != 1)
      {
        snprintf(text, sizeof(text), "%ld items.", (long)numlines);

        data->DragText = text;
        data->DragEntry = -1;
      }
      else if(data->NList_DragLines == 1)
      {
        strlcpy(text, "1 item.", sizeof(text));

        data->DragText = text;
        data->DragEntry = -1;
      }
      else
      {
        data->DragText = NULL;
        data->DragEntry = first;
      }

      data->DragWidth = DrawDragText(data, FALSE);
    }
  }

  if(data->DragRPort == NULL &&
     ((data->DragRPort = (struct RastPort *)AllocVecPooled(data->Pool, sizeof(struct RastPort)))) != NULL)
  {
    struct BitMap *fbm = _window(obj)->RPort->BitMap;

    InitRastPort(data->DragRPort);

    if((data->DragRPort->BitMap = AllocBitMap((ULONG)data->DragWidth, (ULONG)data->DragHeight, GetBitMapAttr(fbm, BMA_DEPTH), BMF_MINPLANES, fbm)) != NULL)
    {
      struct IClass *realclass = OCLASS(obj);

      SetFont(data->DragRPort,data->font);
      OCLASS(obj) = data->ncl;
      REDRAW_FORCE;
      OCLASS(obj) = realclass;

      return data->DragRPort;
    }
    else
    {
      FreeVecPooled(data->Pool, data->DragRPort);
      data->DragRPort = NULL;
    }
  }

  return NULL;
}

