/***************************************************************************

 NList.mcc - New List MUI Custom Class
 Registered MUI class, Serial Number: 1d51 0x9d510030 to 0x9d5100A0
                                           0x9d5100C0 to 0x9d5100FF

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

#include <stdlib.h>

#include <graphics/gfxmacros.h>
#undef GetOutlinePen

#include <clib/alib_protos.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/intuition.h>

#include "private.h"

#include "NList_func.h"
#include "NListviews_mcp.h"

void NL_SetObjInfos(struct NLData *data,BOOL setall)
{
  Object *obj = data->this;
  WORD vdheight;

  if (data->do_draw_all || data->format_chge || data->do_setcols || (data->rp != _rp(obj)) ||
      (data->left != _left(obj)) || (data->top != _top(obj)) || (data->MinImageHeight > data->vinc) ||
      (data->width != _width(obj)) || (data->height != _height(obj)))
  {
    WORD vpos = data->vpos;
    WORD mtop = data->mtop;
    WORD mleft = data->mleft;
    WORD mheight = data->mheight;
    WORD mwidth = data->mwidth;
    WORD vtop = data->vtop;
    WORD vleft = data->vleft;
    WORD vheight = data->vheight;
    WORD vwidth = data->vwidth;
    WORD vinc = data->vinc;

    data->rp = _rp(obj);
    data->font = _font(obj);
    data->pens = _pens(obj);

    data->left = _left(obj);
    data->top = _top(obj);
    data->width = _width(obj);
    data->height = _height(obj);

    data->vinc = data->font->tf_YSize + data->addvinc;
    if (data->NList_MinLineHeight > data->vinc)
      data->vinc = data->NList_MinLineHeight;
    if (data->MinImageHeight > data->vinc)
    { data->vinc = data->MinImageHeight;
      mheight = -1;
    }
    if (data->vinc < 2)
      data->vinc = 2;
    if ((vinc > 1) && (vinc != data->vinc))
    { DO_NOTIFY(NTF_MinMax | NTF_LineHeight);
    }

    data->voff = data->font->tf_Baseline + (data->vinc - data->font->tf_YSize + 1)/2;

    if (setall)
    {
      struct TextExtent te;

      ReSetFont;
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

      data->tabsize = data->spacesize * data->NList_TabSize;
      if (data->tabsize < 4)
         data->tabsize = 4;
    }
    if (data->NList_PartialChar)
      data->NList_PartialChar = (data->hinc > 0) ? data->hinc : 10;

    data->mleft = _mleft(obj);
    data->mright = _mright(obj);
    data->mwidth = _mwidth(obj);
    data->mtop = _mtop(obj);
    data->mbottom = _mbottom(obj);

    if (data->VirtGroup)
    {
      Object *o = data->VirtGroup;
      struct IClass *ocl;

      data->vleft = _mleft(o);
      data->vright = _mright(o);
      data->vtop = _mtop(o);
      data->vbottom = _mbottom(o);

      data->vdx = -data->mleft;
      data->vdy = -data->mtop;

      /* (mri_Flags & 0x20000000) if in a virtgroup */
      /* (mri_Flags & 0x40000000) if it is a virtgroup */
      o = data->VirtGroup2;
      if(o == data->VirtGroup)
        o = (Object *)xget(o, MUIA_Parent);

      while(o)
      {
        ocl = OCLASS(o);

        while (ocl)
        {
          if (ocl == data->VirtClass)
          {
            if (data->VirtGroup2 == data->VirtGroup)
              data->VirtGroup2 = o;

            if (data->vleft < _mleft(o))     data->vleft = _mleft(o);
            if (data->vright > _mright(o))   data->vright = _mright(o);
            if (data->vtop < _mtop(o))       data->vtop = _mtop(o);
            if (data->vbottom > _mbottom(o)) data->vbottom = _mbottom(o);
            ocl = NULL;
          }
          else
            ocl = ocl->cl_Super;
        }

        if(o)
          o = (Object *)xget(o, MUIA_Parent);
      }

      if (data->VirtGroup2 == data->VirtGroup)
        data->VirtGroup2 = NULL;
      if (data->vleft < data->mleft)     data->vleft = data->mleft;
      if (data->vright > data->mright)   data->vright = data->mright;
      if (data->vtop < data->mtop)       data->vtop = data->mtop;
      if (data->vbottom > data->mbottom) data->vbottom = data->mbottom;
      data->vwidth = data->vright - data->vleft + 1;
      data->vheight = data->vbottom - data->vtop + 1;
    }
    else
    {
      data->vleft = _mleft(obj);
      data->vright = _mright(obj);
      data->vtop = _mtop(obj);
      data->vbottom = _mbottom(obj);
      data->vwidth = _mwidth(obj);
      data->vheight = _mheight(obj);
      data->vdx = 0;
      data->vdy = 0;
    }

    data->hpos = data->mleft;
    data->hvisible = data->mright - data->mleft;
    data->mheight = _mheight(obj);
    if (data->NList_Title)
    { data->mheight -= data->vinc;
      if (data->NList_TitleSeparator)
        data->mheight -= 2;
    }

    data->lvisible = data->mheight / data->vinc;
    vdheight = (data->mheight - data->lvisible * data->vinc);

    /* Set vertical delta top, if text should be centered vertically */
    if (data->NList_VerticalCenteredText)
      data->vdt = vdheight / 2;
    else
      data->vdt = 0;

    data->vdb = vdheight - data->vdt;
    data->vdtpos = data->mtop;
    if (data->NList_Title)
    {
      data->vdtitlepos = data->vdtpos;
      data->vdtitleheight = data->vdt + data->vinc;
    } else
    {
      data->vdtitlepos = data->vdtpos + data->vdt;
      data->vdtitleheight = data->vinc / 3;
    }
    data->vdbpos = data->mbottom + 1 - data->vdb;

    /* calculate the vertical top position of the first line of the list */
    data->vpos = data->mtop + data->vdt;
    if (data->NList_Title)
    {
      data->vpos += data->vinc;
      if (data->NList_TitleSeparator)
        data->vpos += 2;
    }

    data->badrport = FALSE;
    data->NList_Visible = data->lvisible;

    if ((mwidth != data->mwidth))
      data->do_wwrap = data->force_wwrap = TRUE;

    if ((mwidth != data->mwidth) || data->format_chge || data->do_draw_all)
    {
      data->do_draw_all = data->do_draw = data->do_draw_title = data->do_updatesb = TRUE;
      data->do_setcols = TRUE;
      data->Title_PixLen = -1;
      data->ScrollBarsPos = -2;
      if (data->NList_AdjustWidth)
        data->NList_AdjustWidth = -1;
      NOWANT_NOTIFY(NTF_MinMaxNoDraw);
    }
    else
    {
      if (WANTED_NOTIFY(NTF_MinMaxNoDraw) && (vpos == data->vpos) &&
          (mtop == data->mtop) && (mleft == data->mleft) &&
          (vtop == data->vtop) && (vleft == data->vleft) &&
          (vheight == data->vheight) && (vwidth == data->vwidth))
      {
      }
      else if (mheight != data->mheight)
      {
        data->do_draw_all = data->do_draw = data->do_draw_title = data->do_updatesb = TRUE;
        data->Title_PixLen = -1;
        data->ScrollBarsPos = -2;
        NOWANT_NOTIFY(NTF_MinMaxNoDraw);
      }
    }
    data->drawall_bits = 0;
    data->format_chge = 0;
  }
}


static void DrawAdjustBar(struct NLData *data,WORD draw)
{
  WORD hfirst = data->NList_Horiz_AffFirst & ~1;
  WORD hfirsthpos = hfirst - data->hpos;
  WORD adjbar;
  Object *obj = data->this;

  if (draw)
  {
    if ((data->adjustbar == -2) || ((data->adjustbar == -3) && (draw == 2)))
    {
      if (data->NList_Title && (data->adjustcolumn < data->numcols) && (data->adjustcolumn >= 0))
      {
        WORD minx = (data->cols[data->adjustcolumn].c->minx - hfirsthpos);
        WORD maxx = (data->cols[data->adjustcolumn].c->maxx - hfirsthpos);
        WORD maxy = data->vdtitlepos + data->vdtitleheight;
        BOOL draw_left = TRUE;
        BOOL draw_right = TRUE;

        if (!data->NList_TitleSeparator)
          maxy--;
        if (data->adjustcolumn > 0)
          minx -= (data->cols[data->adjustcolumn-1].c->delta - (((data->cols[data->adjustcolumn-1].c->delta-1) / 2) + 1));
        maxx += ((data->cols[data->adjustcolumn].c->delta-1) / 2);
        if (data->adjustcolumn == data->numcols-1)
          maxx--;
        if (minx < data->mleft)
        {
          minx = data->mleft;
          draw_left = FALSE;
        }
        if (maxx > data->mright)
        {
          maxx = data->mright;
          if (IS_BAR(data->adjustcolumn,data->cols[data->adjustcolumn].c) || (maxx < (data->Title_PixLen - hfirsthpos)))
            draw_right = FALSE;
        }
        if ((maxx - minx) >= 1)
        {
          SetAPen(data->rp,data->pens[MPEN_SHINE]);
          Move(data->rp, minx, maxy);
          Draw(data->rp, maxx, maxy);
          if (draw_right)
            Draw(data->rp, maxx, data->vdtitlepos + 1);
          SetAPen(data->rp,data->pens[MPEN_SHADOW]);
          Move(data->rp, maxx, data->vdtitlepos);
          Draw(data->rp, minx, data->vdtitlepos);
          if (draw_left)
            Draw(data->rp, minx, maxy);
          SetAPen(data->rp,data->pens[MPEN_SHINE]);
          data->adjustbar_old = data->adjustbar;
          data->adjustbar = -3;
        }
      }
    }
    else if ((data->adjustbar >= 0) && ((data->adjustbar_old == -1) || (draw == 2)))
    {
      adjbar = data->adjustbar - hfirsthpos;
      if ((adjbar < data->mleft) || (adjbar >= data->mright))
        data->adjustbar_old = -1;
      else
      {
        if (data->NList_ColWidthDrag == MUIV_NList_ColWidthDrag_Visual)
        {
        }
        else if (data->NList_ColWidthDrag == MUIV_NList_ColWidthDrag_All)
        {
          SetAPen(data->rp,data->pens[MPEN_SHADOW]);
          Move(data->rp, adjbar+1, data->vdtitlepos);
          Draw(data->rp, adjbar+1, data->vdbpos - 1);
          SetAPen(data->rp,data->pens[MPEN_SHINE]);
          Move(data->rp, adjbar, data->vdtitlepos);
          Draw(data->rp, adjbar, data->vdbpos - 1);
        }
        else if (data->NList_Title)
        {
          SetAPen(data->rp,data->pens[MPEN_SHADOW]);
          Move(data->rp, adjbar+1, data->vdtitlepos);
          Draw(data->rp, adjbar+1, data->vdtitlepos + data->vdtitleheight - 1);
          SetAPen(data->rp,data->pens[MPEN_SHINE]);
          Move(data->rp, adjbar, data->vdtitlepos);
          Draw(data->rp, adjbar, data->vdtitlepos + data->vdtitleheight - 1);
        }
        else
        {
          SetAPen(data->rp,data->pens[MPEN_SHADOW]);
          Move(data->rp, adjbar+1, data->vpos);
          Draw(data->rp, adjbar+1, data->vpos + data->vinc - 1);
          SetAPen(data->rp,data->pens[MPEN_SHINE]);
          Move(data->rp, adjbar, data->vpos);
          Draw(data->rp, adjbar, data->vpos + data->vinc - 1);
        }
        data->adjustbar_old = data->adjustbar;
      }
    }
  }
  else
  {
    APTR clippinghandle = NULL;

    if (data->NList_Title && (data->adjustbar_old <= -2) && ((data->adjustbar == -1) || (data->adjustbar <= -4)))
    {
      if ((data->adjustcolumn < data->numcols) && (data->adjustcolumn >= 0))
      {
        WORD minx = (data->cols[data->adjustcolumn].c->minx - hfirsthpos);
        WORD maxx = (data->cols[data->adjustcolumn].c->maxx - hfirsthpos);

        if (data->adjustcolumn > 0)
          minx -= (data->cols[data->adjustcolumn-1].c->delta - (((data->cols[data->adjustcolumn-1].c->delta-1) / 2) + 1));
        maxx += ((data->cols[data->adjustcolumn].c->delta-1) / 2);
        if (minx < data->mleft)
          minx = data->mleft;
        if (maxx > data->mright)
          maxx = data->mright;
        if ((maxx - minx) > 0)
        {
          WORD height = data->vdtitleheight;

          maxx++;
          if (data->NList_TitleSeparator)
            height++;
          clippinghandle = MUI_AddClipping(muiRenderInfo(obj),(WORD) minx,data->vdtitlepos,(WORD) (maxx-minx),height);
          DrawTitle(data,minx,maxx,hfirst);
          MUI_RemoveClipping(muiRenderInfo(obj),clippinghandle);
        }
      }
      data->adjustbar_old = -1;
    }
    else if (data->adjustbar_old >= 0)
    {
      adjbar = data->adjustbar_old - hfirsthpos;
      if (adjbar < data->mleft)
      {
        adjbar = data->mleft;
        data->adjustbar_old = adjbar + hfirsthpos;
      }
      else if (adjbar >= data->mright)
      {
        adjbar = data->mright - 1;
        data->adjustbar_old = adjbar + hfirsthpos;
      }
      if (data->NList_ColWidthDrag == MUIV_NList_ColWidthDrag_Visual)
      {
      }
      else if (data->NList_ColWidthDrag == MUIV_NList_ColWidthDrag_All)
      {
        clippinghandle = MUI_AddClipping(muiRenderInfo(obj),(WORD) adjbar,data->vdtitlepos,(WORD) 2,(WORD) (data->vdbpos - data->vdtitlepos));
        DrawLines(data,data->NList_AffFirst,data->NList_AffFirst+data->NList_Visible,adjbar,adjbar+2,hfirst,0,1,TRUE,0);
        if (data->NList_Title)
          DrawTitle(data,adjbar,adjbar+2,hfirst);
        MUI_RemoveClipping(muiRenderInfo(obj),clippinghandle);
      }
      else if (data->NList_Title)
      {
        clippinghandle = MUI_AddClipping(muiRenderInfo(obj),(WORD) adjbar,data->vdtitlepos,(WORD) 2,data->vdtitleheight);
        DrawTitle(data,adjbar,adjbar+2,hfirst);
        MUI_RemoveClipping(muiRenderInfo(obj),clippinghandle);
      }
      else
      {
        clippinghandle = MUI_AddClipping(muiRenderInfo(obj),(WORD) adjbar,data->vpos,(WORD) 2,data->vinc);
        DrawLines(data,data->NList_AffFirst,data->NList_AffFirst+1,adjbar,adjbar+2,hfirst,0,1,TRUE,0);
        MUI_RemoveClipping(muiRenderInfo(obj),clippinghandle);
      }
      data->adjustbar_old = -1;
    }
  }
}


/*
 * struct ClipRect *cr;
 * struct RegionRectangle *rr;
 * struct Region *reg;
 * LONG MinX,MinY,MaxX,MaxY;
 * LONG mleft,mright,mtop,mbottom;
 *
 * mleft = data->mleft + ((LONG) _window(obj)->LeftEdge);
 * mright = data->mright + ((LONG) _window(obj)->LeftEdge);
 * mtop = data->mtop + ((LONG) _window(obj)->TopEdge);
 * mbottom = data->mbottom + ((LONG) _window(obj)->TopEdge);
 * D(bug("%lx|Object >(%ld,%ld / %ld,%ld)\n",obj,mleft,mtop,mright,mbottom));
 *
 * cr = _window(obj)->WLayer->ClipRect;
 * while (cr)
 * { MinX = (LONG) cr->bounds.MinX;MinY = (LONG) cr->bounds.MinY;MaxX = (LONG) cr->bounds.MaxX;MaxY = (LONG) cr->bounds.MaxY;
 *   D(bug("%lx|CR %ld %ld,%ld / %ld,%ld\n",obj,one_more,MinX,MinY,MaxX,MaxY));
 *   one_more++;cr = cr->Next;
 * }
 * one_more = 0;
 *
 * cr = _window(obj)->WLayer->_cliprects;
 * while (cr)
 * { MinX = (LONG) cr->bounds.MinX;MinY = (LONG) cr->bounds.MinY;MaxX = (LONG) cr->bounds.MaxX;MaxY = (LONG) cr->bounds.MaxY;
 *   D(bug("%lx|_cr %ld %ld,%ld / %ld,%ld\n",obj,one_more,MinX,MinY,MaxX,MaxY));
 *   one_more++;cr = cr->Next;
 * }
 * one_more = 0;
 *
 * cr = _window(obj)->WLayer->SuperClipRect;
 * while (cr)
 * { MinX = (LONG) cr->bounds.MinX;MinY = (LONG) cr->bounds.MinY;MaxX = (LONG) cr->bounds.MaxX;MaxY = (LONG) cr->bounds.MaxY;
 *   D(bug("%lx|SuperClipRect %ld>%ld,%ld / %ld,%ld\n",obj,one_more,MinX,MinY,MaxX,MaxY));
 *   one_more++;cr = cr->Next;
 * }
 * one_more = 0;
 *
 * reg = _window(obj)->WLayer->ClipRegion;
 * if (reg)
 * { MinX = (LONG) reg->bounds.MinX;MinY = (LONG) reg->bounds.MinY;MaxX = (LONG) reg->bounds.MaxX;MaxY = (LONG) reg->bounds.MaxY;
 *   D(bug("%lx|ClipRegion %ld>%ld,%ld / %ld,%ld\n",obj,one_more,MinX,MinY,MaxX,MaxY));
 *   one_more++;rr = reg->RegionRectangle;
 *   while (rr)
 *   { MinX = (LONG) rr->bounds.MinX;MinY = (LONG) rr->bounds.MinY;MaxX = (LONG) rr->bounds.MaxX;MaxY = (LONG) rr->bounds.MaxY;
 *     D(bug("%lx|ClipRegion %ld>%ld,%ld / %ld,%ld\n",obj,one_more,MinX,MinY,MaxX,MaxY));
 *     one_more++;rr = rr->Next;
 *   }
 * }
 * one_more = 0;
 */

#define LayerDamaged(l)  ((l)->DamageList && (l)->DamageList->RegionRectangle)

#define LayerCovered(l)  ((!(l)->ClipRect) || ((l)->ClipRect->bounds.MaxX != (l)->bounds.MaxX) ||\
                                              ((l)->ClipRect->bounds.MinX != (l)->bounds.MinX) ||\
                                              ((l)->ClipRect->bounds.MaxY != (l)->bounds.MaxY) ||\
                                              ((l)->ClipRect->bounds.MinY != (l)->bounds.MinY))


static ULONG DrawRefresh(struct NLData *data)
{
  Object *obj = data->this;
  APTR clippinghandle = NULL;
  struct RegionRectangle *rr;
  struct Region *reg;
  WORD sX,sY,MinX,MinY,MaxX,MaxY;
  WORD mleft,mright,mtop,mbottom,mwidth,mheight;
  WORD hfirst = data->NList_Horiz_AffFirst & ~1;

/*
 *   if ((data->left != _left(obj)) || (data->top != _top(obj)) ||
 *       (data->width != _width(obj)) || (data->height != _height(obj)))
 *   {
 * LONG ml = (LONG) _mleft(obj);
 * LONG mt = (LONG) _top(obj);
 * D(bug("%lx|ml=%ld (%ld)  mt=%ld (%ld)\n",obj,ml,data->mleft,mt,data->top));
 *     NL_SetObjInfos(data,FALSE);
 *   }
 */

  LockLayer( 0, _window(obj)->WLayer );
  reg = _window(obj)->WLayer->DamageList;
  if (reg)
  { sX = reg->bounds.MinX;
    sY = reg->bounds.MinY;
    mleft =   reg->bounds.MaxX;
    mtop =    reg->bounds.MaxY;
    mright =  sX;
    mbottom = sY;
    rr = reg->RegionRectangle;
    while (rr)
    { MinX = sX + rr->bounds.MinX;
      MinY = sY + rr->bounds.MinY;
      MaxX = sX + rr->bounds.MaxX;
      MaxY = sY + rr->bounds.MaxY;
      if ((MinX < data->mright) || (MaxX > data->mleft) ||
          (MinY < data->mbottom) || (MaxY > data->mtop))
      {
        if (MinX < mleft)   mleft = MinX;
        if (MinY < mtop)    mtop = MinY;
        if (MaxX > mright)  mright = MaxX;
        if (MaxY > mbottom) mbottom = MaxY;
      }
      rr = rr->Next;
    }
    UnlockLayer( _window(obj)->WLayer );

    if ((mleft < data->mright) || (mright > data->mleft) ||
        (mtop < data->mbottom) || (mbottom > data->mtop))
    { if (mleft < data->mleft)     mleft = data->mleft;
      if (mtop < data->mtop)       mtop = data->mtop;
      if (mright > data->mright)   mright = data->mright;
      if (mbottom > data->mbottom) mbottom = data->mbottom;

      if ((mleft <= mright) && (mtop <= mbottom))
      {
        BOOL clipped = FALSE;
        WORD ly1, ly2;
        WORD lyl1=0;
        WORD lyl2=0;

        mright++;
        mbottom++;

        mwidth = mright - mleft;
        mheight = mbottom - mtop;

        ly1 = (mtop - data->vpos);
        ly2 = (mbottom - data->vpos);
        if (ly2 > 0)
        {
          lyl1 = ly1 / data->vinc;
          lyl2 = ly2 / data->vinc;
          if (data->NList_PartialChar ||
              ((data->NList_First_Incr) && ((lyl1 <= 0) || (lyl2 >= data->NList_Visible))))
          { clippinghandle = MUI_AddClipping(muiRenderInfo(obj),(WORD) data->mleft,mtop,(WORD) data->mwidth,mheight);
            clipped = TRUE;
          }
          if (lyl1 < 0) lyl1 = 0;
          if (lyl2 >= data->NList_Visible)
            lyl2 = data->NList_Visible - 1;
          if (data->NList_First_Incr)
            lyl2++;
          lyl2++;
          lyl1 += data->NList_First;
          lyl2 += data->NList_First;
        }
        ReSetFont;
        SetBackGround(data->NList_ListBackGround)
        if ((data->vdt > 0) && !data->NList_Title && (ly1 < 0))
          DrawBackground(obj, mleft, data->vdtpos, mwidth, data->vdt, data->vdx, data->vdy);
        if (data->vdb > 0)
          DrawBackground(obj, mleft, data->vdbpos, mwidth, data->vdb, data->vdx, data->vdy);
        if (data->NList_Title && (ly1 < 0))
          DrawTitle(data,mleft,mright,hfirst);


        if (ly2 > 0)
          DrawLines(data,lyl1,lyl2,mleft,mright,hfirst,0,1,TRUE,0);

        if (clipped)
          MUI_RemoveClipping(muiRenderInfo(obj),clippinghandle);
        DrawAdjustBar(data,2);
        SetBackGround(data->NList_ListBackGround);
      }
/*
 * else
 * D(bug("%lx|no usable rect\n",obj));
 */
    }
  }
  else
    UnlockLayer( _window(obj)->WLayer );

  return(0);
}


IPTR mNL_Draw(struct IClass *cl,Object *obj,struct MUIP_Draw *msg)
{
  register struct NLData *data = INST_DATA(cl,obj);

  if (data->drawsuper)
  {
    struct MUIP_Draw mymsg;
    mymsg.MethodID = MUIM_Draw;
    mymsg.flags = 0;
/*D(bug("%lx|drawsuper1 %lx %lx %lx\n",obj,msg->flags,muiAreaData(obj)->mad_Flags,muiAreaData(data->drawsuper)->mad_Flags));*/
    DoMethodA((Object *)data->drawsuper, (Msg)(void*)&mymsg);
/*D(bug("%lx|drawsuper2 %lx %lx %lx\n",obj,msg->flags,muiAreaData(obj)->mad_Flags,muiAreaData(data->drawsuper)->mad_Flags));*/
    msg->flags = 0;
    return(0);
  }

  if (data->nodraw || !data->SHOW)
  {
/*D(bug("%lx|nodraw \n",obj));*/
    msg->flags = 0;
    return(0);
  }

  if (WANTED_NOTIFY(NTF_MinMaxNoDraw) && !data->do_draw_all && !data->do_draw)
  {
    NL_SetObjInfos(data,FALSE);
    if (WANTED_NOTIFY(NTF_MinMaxNoDraw))
    {
      msg->flags = 0;
      return(0);
    }
  }

/*D(bug("%lx|draw1=%ld\n",obj,data->DRAW));*/
  if (data->actbackground == -1)
  { SetBackGround(data->NList_ListBackGround);
  }

  if (data->NList_Disabled)
  { data->DRAW = 1;
    data->do_draw = FALSE;
    data->do_draw_all = FALSE;
    data->do_draw_active = FALSE;
    data->do_draw_title = FALSE;
    data->first_change = LONG_MAX;
    data->last_change = LONG_MIN;
    data->minx_change_entry = -1;
    data->maxx_change_entry = -1;
    if ((muiAreaData(obj)->mad_Flags & MADF_DRAWOBJECT) ||
        (data->NList_Disabled < 2))
    {
      NL_SetObjInfos(data,FALSE);
      data->NList_Disabled = 2;
      DoSuperMethodA(cl,obj,(Msg)msg);
    }

    return (0);
  }

  if (data->DragRPort && (data->DragText || (data->DragEntry >= 0)))
  {
    DrawDragText(data,TRUE);
    msg->flags = 0;
    return (0);
  }

  data->DRAW++;

  if(/*(data->refreshing != 2) && (data->DRAW > 1) &&*/
      ((data->refreshing) ||
       (muiRenderInfo(obj)->mri_Flags & MUIMRI_REFRESHMODE)))
//     || (data->rp->Layer->Flags & LAYERREFRESH)))
//       ((data->rp->Layer->Flags & LAYERREFRESH) && !(data->do_draw_all && data->do_draw))))
  {
/*D(bug("%lx|Refresh %ld %ld %ld %ld %lx %lx\n",obj,data->DRAW,data->do_draw_all,data->do_draw,(LONG)data->refreshing,(LONG)(muiRenderInfo(obj)->mri_Flags & MUIMRI_REFRESHMODE),(LONG)(data->rp->Layer->Flags & LAYERREFRESH)));*/
    /* Avoid Superclass to draw anything *in* the object */
    if (muiRenderInfo(obj)->mri_Flags & MUIMRI_REFRESHMODE)
    {
#ifndef __AROS__
      muiAreaData(obj)->mad_Flags &= ~0x00000001;
#else
      /* "AROS: FIXME: No frame drawn if doing: muiAreaData(obj)->mad_Flags &= ~0x00000001;" This is still valid 16.01.2012 */
#endif
      DoSuperMethodA(cl,obj,(Msg) msg);
    }
    DrawRefresh(data);
/*
 *     data->do_draw = FALSE;
 *     data->do_draw_all = FALSE;
 *     data->do_draw_active = FALSE;
 *     data->do_draw_title = FALSE;
 *     data->first_change = LONG_MAX;
 *     data->last_change = LONG_MIN;
 *     data->minx_change_entry = -1;
 *     data->maxx_change_entry = -1;
 */
    if (data->DRAW > 1)
      data->DRAW--;
    return (0);
  }
/*
 *else if (data->refreshing)
 *D(bug("%lx|Refresh rejected %ld %ld %ld %ld %lx %lx\n",obj,data->DRAW,data->do_draw_all,data->do_draw,(LONG)data->refreshing,(LONG)(muiRenderInfo(obj)->mri_Flags & MUIMRI_REFRESHMODE),(LONG)(data->rp->Layer->Flags & LAYERREFRESH)));
 */

  if (muiAreaData(obj)->mad_Flags & MADF_DRAWOBJECT)
    data->do_draw_all = TRUE;

  if (NL_UpdateScrollers(data,FALSE))
  { msg->flags = 0;
    if (data->DRAW > 1)
      data->DRAW--;
    return (0);
  }
  data->UpdateScrollersRedrawn = TRUE;


  /* Avoid Superclass to draw anything *in* the object */
#ifndef __AROS__
  muiAreaData(obj)->mad_Flags &= ~0x00000001;
#else
  /* "AROS: FIXME: No frame drawn if doing: muiAreaData(obj)->mad_Flags &= ~0x00000001;" This is still valid 16.01.2012 */
#endif
  DoSuperMethodA(cl,obj,(Msg) msg);

/*D(bug("%lx|Draw %lx %lx\n",obj,msg->flags,muiAreaData(obj)->mad_Flags));*/

  {
    LONG ent, ent2;
    APTR fullclippinghandle = NULL;
    APTR clippinghandle = NULL;
    WORD hmax,linelen,hfirst;
    LONG LPFirst,LPVisible,LPEntries;
    LONG one_more = 0;
/*
 *  LONG vfyl,vlyl;
 */
    BOOL need_refresh = FALSE;
    BOOL draw_all_force;
    BOOL fullclipped = FALSE;

    if (data->do_draw_all)
      draw_all_force = TRUE;
    else
      draw_all_force = FALSE;

    if (!(msg->flags & MADF_DRAWOBJECT) && !(msg->flags & MADF_DRAWUPDATE) && !data->do_draw_all)
    { if (data->DRAW > 1)
        data->DRAW--;
      return (0);
    }

/*
 *     if (muiRenderInfo(obj)->mri_Flags & MUIMRI_REFRESHMODE)
 *       D(bug("MUIMRI_REFRESHMODE (do_draw_all=%ld)\n",data->do_draw_all));
 */

    ReSetFont;

    if (data->do_draw_all)
    {
      /*data->do_draw_all = TRUE;*/
      data->do_draw_title = TRUE;
      data->drawall_bits = 0;
      data->NList_First_Incr = 0;
      data->minx_change_entry = -1;
      data->maxx_change_entry = -1;
    }
    else if ((data->rp->Layer->Flags & LAYERREFRESH) || (msg->flags & MADF_DRAWOBJECT))
    {
      data->do_draw_all = TRUE;
      data->do_draw_title = TRUE;
      data->drawall_bits = 0;
      data->minx_change_entry = -1;
      data->maxx_change_entry = -1;
    }

    if ((data->NList_First_Incr < 0) || (data->NList_First_Incr >= data->vinc))
      data->NList_First_Incr = 0;

    LPFirst = data->NList_First * data->vinc;
    LPVisible = data->lvisible * data->vinc;
    LPEntries = data->NList_Entries * data->vinc;
    if (LPFirst + LPVisible > LPEntries)
      LPEntries = LPFirst + LPVisible;

    if (LPEntries != data->NList_Prop_Entries)
      data->NList_Prop_Entries = LPEntries;
    if (LPVisible != data->NList_Prop_Visible)
      data->NList_Prop_Visible = LPVisible;
    if (data->NList_First != data->NList_Prop_First / data->vinc)
    { data->NList_Prop_First_Real = LPFirst;
      data->NList_Prop_First_Prec = LPFirst;
      data->NList_Prop_First = LPFirst;
    }

    if (data->NList_Horiz_First >= data->NList_Horiz_Entries)
      data->NList_Horiz_First = data->NList_Horiz_Entries;
    else if (data->NList_Horiz_First < 0)
      data->NList_Horiz_First = 0;
    hfirst = data->NList_Horiz_First & ~1;

    if (!data->do_draw_all)
    { if (data->markerase && (data->markerasenum >= 0))
      { if (data->markerasenum < data->first_change)
          data->first_change = data->markerasenum;
        if (data->markerasenum > data->last_change)
          data->last_change = data->markerasenum;
      }
      data->last_change += 1;
      if (data->first_change < data->last_change)
      { if ((data->NList_First != data->NList_AffFirst) ||
            (data->NList_First_Incr != data->NList_AffFirst_Incr) ||
            (hfirst != data->NList_Horiz_AffFirst))
        { data->do_draw_all = TRUE;
          data->NList_First_Incr = 0;
        }
      }
      else if (((data->NList_First != data->NList_AffFirst) ||
                (data->NList_First_Incr != data->NList_AffFirst_Incr)) &&
               (hfirst != data->NList_Horiz_AffFirst))
      { data->do_draw_all = TRUE;
        data->NList_First_Incr = 0;
        data->do_draw_title = TRUE;
      }
      else if ((data->NList_First != data->NList_AffFirst) || (data->NList_First_Incr != data->NList_AffFirst_Incr))
      { if (data->NList_Prop_Visible < 180)
          hmax = (data->NList_Visible * 7 / 8);
        else if (data->NList_Prop_Visible < 360)
          hmax = (data->NList_Visible * 6 / 8);
        else
          hmax = (data->NList_Visible * 5 / 8);
        if (abs(data->NList_First - data->NList_AffFirst) > hmax)
        { data->do_draw_all = TRUE;
          data->NList_First_Incr = 0;
        }
      }
      else if (hfirst != data->NList_Horiz_AffFirst)
      { if (data->NList_Horiz_Visible < 180)
          hmax = (data->NList_Horiz_Visible * 7 / 8);
        else if (data->NList_Horiz_Visible < 360)
          hmax = (data->NList_Horiz_Visible * 6 / 8);
        else
          hmax = (data->NList_Horiz_Visible * 5 / 8);
        if (abs(hfirst - data->NList_Horiz_AffFirst) > hmax)
        { data->do_draw_all = TRUE;
          data->NList_First_Incr = 0;
          data->do_draw_title = TRUE;
        }
      }
    }

    if (data->do_draw_all)
    { data->NList_First_Incr = 0;
      data->first_change = data->NList_First;
      data->last_change = data->NList_First + data->NList_Visible;
    }

    if (data->NList_First_Incr)
      one_more = 1;

    NL_UpdateScrollersValues(data);

/*
 *     if (data->first_change < data->last_change)
 *       D(bug("NL: mNL_Draw(%ld to %ld) \n",data->first_change,data->last_change));
 *     if (data->do_draw_all)
 *       D(bug("NL: mNL_Draw(do_draw_all) \n"));
 *     else if (data->do_draw_active)
 *       D(bug("NL: mNL_Draw(do_draw_active) \n"));
 *     else if (data->do_draw_title)
 *       D(bug("NL: mNL_Draw(do_draw_title) \n"));
 *     else if (data->NList_First != data->NList_AffFirst)
 *       D(bug("NL: mNL_Draw(NList_First) \n"));
 *     else if (hfirst != data->NList_Horiz_AffFirst)
 *       D(bug("NL: mNL_Draw(NList_Horiz_First) \n"));
 *     else
 *       D(bug("NL: mNL_Draw(redraw???) \n"));
 */

    if (data->NList_PartialChar)
    {
      fullclipped = TRUE;
      fullclippinghandle = MUI_AddClipping(muiRenderInfo(obj),(WORD) data->vleft,(WORD) data->vtop,
                                              (WORD) data->vwidth,(WORD) data->vheight);
    }

    if ((data->NList_Active != data->NList_AffActive) && !data->NList_TypeSelect)
    {
      data->do_draw_active = TRUE;

      if (!(msg->flags & MADF_DRAWOBJECT))
      {
        LONG old_one_more = 0;
        BOOL clipped = FALSE;

        if(data->NList_AffFirst_Incr)
          old_one_more = 1;

        ent = data->NList_AffActive;
        if((ent >= data->NList_AffFirst) && (ent < data->NList_AffFirst + data->NList_Visible + old_one_more))
        {
          if ((data->NList_AffFirst_Incr) && ((ent <= data->NList_AffFirst) || (ent >= data->NList_AffFirst + data->NList_Visible)))
          {
            clipped = TRUE;
            clippinghandle = MUI_AddClipping(muiRenderInfo(obj),
                                             (WORD) data->mleft,
                                             (WORD) data->vpos,
                                             (WORD) data->mwidth,
                                             (WORD) (data->NList_Visible*data->vinc));
          }
          DrawOldLine(data,ent,PMIN,PMAX,hfirst);

          if (clipped)
            MUI_RemoveClipping(muiRenderInfo(obj),clippinghandle);
        }
      }
    }

    if (!data->do_draw_all && data->drawall_bits)
    {
      LONG tmp_hfirst = hfirst;
      LONG tmp_NList_First = data->NList_First;
      LONG tmp_NList_First_Incr = data->NList_First_Incr;

      hfirst = data->NList_Horiz_AffFirst & ~1;
      data->NList_First = data->NList_AffFirst;
      data->NList_First_Incr = data->NList_AffFirst_Incr;
      hmax = DrawLines(data,data->NList_First,data->NList_First + data->NList_Visible,PMIN,PMAX,hfirst,0,0,FALSE,data->drawall_bits);
      data->drawall_bits = 0;
      data->drawall_dobit = 0;
      hfirst = tmp_hfirst;
      data->NList_First = tmp_NList_First;
      data->NList_First_Incr = tmp_NList_First_Incr;

      data->ScrollBarsTime = SCROLLBARSTIME;
      if (hmax > data->NList_Horiz_Entries)
        data->NList_Horiz_Entries = hmax;
    }

/*
 *  vfyl = (data->vtop - data->vpos) / data->vinc;
 *  vlyl = ((data->vbottom - data->vpos) / data->vinc) + 1;
 */

    if (data->do_draw_all && data->do_draw_title && data->NList_Title/* && (vfyl <= 0)*/)
    {
      data->Title_PixLen = DrawTitle(data,PMIN,PMAX,hfirst);

      if(data->Title_PixLen > data->NList_Horiz_Entries)
        data->NList_Horiz_Entries = data->Title_PixLen;

      data->do_draw_title = FALSE;
    }
/*
 *     if (data->first_change < data->last_change)
 *     { if (vfyl > data->first_change)
 *         data->first_change = vfyl;
 *       if (vlyl < data->last_change)
 *         data->last_change = vlyl;
 *     }
 */
    if (data->first_change < data->last_change)
    {
      WORD not_all = 0;
      BOOL clipped = FALSE;

      data->drawall_bits = 0;
      if(data->do_draw_all)
      {
        SetBackGround(data->NList_ListBackGround)

        if((data->vdt > 0) && !data->NList_Title)
          DrawBackground(obj, data->mleft, data->vdtpos, data->mwidth, data->vdt, data->vdx, data->vdy);
        if(data->vdb > 0)
          DrawBackground(obj, data->mleft, data->vdbpos, data->mwidth, data->vdb, data->vdx, data->vdy);

        if(!data->dropping && !draw_all_force &&
           ((abs(data->NList_First - data->NList_AffFirst) > (120 / data->vinc)) ||
            (abs(hfirst - data->NList_Horiz_AffFirst) > 120)))
        {
          if (data->drawall_dobit == 1)
          {
            not_all = 2;
            data->drawall_dobit = 0;
            data->drawall_bits = 1;
          }
          else if (data->drawall_dobit == 2)
          {
            not_all = data->drawall_dobit = 1;
            data->drawall_bits = 2;
          }
        }
      }
      else
      {
        if (data->first_change < data->NList_First)
          data->first_change = data->NList_First;

        if (data->last_change >= data->NList_First + data->NList_Visible + one_more)
          data->last_change = data->NList_First + data->NList_Visible + one_more;
      }

      if ((data->NList_First_Incr) && ((data->first_change <= data->NList_First) || (data->last_change >= data->NList_First + data->NList_Visible)))
      {
        clipped = TRUE;
        clippinghandle = MUI_AddClipping(muiRenderInfo(obj),
                                         (WORD) data->mleft,
                                         (WORD) data->vpos,
                                         (WORD) data->mwidth,
                                         (WORD) (data->NList_Visible*data->vinc));
      }

      hmax = DrawLines(data,data->first_change,data->last_change,PMIN,PMAX,hfirst,0,0,FALSE,not_all);

      if (clipped)
        MUI_RemoveClipping(muiRenderInfo(obj),clippinghandle);

      if (data->NList_Title && (data->Title_PixLen > hmax))
        hmax = data->Title_PixLen;
      if (hfirst + data->hvisible > hmax)
        hmax = hfirst + data->hvisible;
      if (data->do_draw_all)
      {
        if ((data->drawall_bits) && (hmax != data->NList_Horiz_Entries))
        { data->NList_Horiz_Entries = hmax;
          data->ScrollBarsTime = SCROLLBARSTIME;
        }
        else if (hmax != data->NList_Horiz_Entries)
          data->NList_Horiz_Entries = hmax;

        if (data->hvisible != data->NList_Horiz_Visible)
          data->NList_Horiz_Visible = data->hvisible;
      }
      else
      {
        if (hmax > data->NList_Horiz_Entries)
          data->NList_Horiz_Entries = hmax;
      }

      data->NList_Horiz_AffFirst = hfirst;
      data->NList_AffFirst = data->NList_First;
      data->NList_AffFirst_Incr = data->NList_First_Incr;
    }
    else if ((data->NList_First != data->NList_AffFirst) || (data->NList_First_Incr != data->NList_AffFirst_Incr))
    {
      /*WORD dyl,fyl,lyl,dy,yl1,yl2,vpos;*/
      LONG dyl,fyl,lyl,dy,y1,y2,vpos,vbot;

      dyl = data->NList_First - data->NList_AffFirst;
      dy = dyl*data->vinc + (data->NList_First_Incr - data->NList_AffFirst_Incr);
      vpos = data->vpos;
      vbot = data->vpos + (data->NList_Visible * data->vinc) - 1;
      if (data->vtop > vpos)
      {
        vpos = data->vtop;
        y1 = (vpos + data->NList_First_Incr - data->vpos);
        fyl = y1 / data->vinc;
      }
      else
      {
        y1 = data->NList_First_Incr;
        fyl = 0;
      }

      if (data->vbottom < vbot)
      {
        vbot = data->vbottom;
        y2 = (vbot + data->NList_First_Incr - data->vpos);
        lyl = y2 / data->vinc;
      }
      else
      {
        y2 = (vbot + data->NList_First_Incr - data->vpos);
        lyl = y2 / data->vinc;
      }

      if (dy < 0)
      {
        lyl = (y1 - dy) / data->vinc;
      }
      else
      {
        fyl = (y2 - dy) / data->vinc;
      }
      lyl++;
      fyl += data->NList_First;
      lyl += data->NList_First;

      if (fyl <= lyl)
      {
/*
 *       if ((_window(obj)->Flags & WFLG_REFRESHBITS) == WFLG_SIMPLE_REFRESH)
 *         WaitTOF();
 */
        ScrollVert(data,dy,LPVisible);
        need_refresh = TRUE;

        if (data->NList_First_Incr || data->NList_AffFirst_Incr)
        {
          WORD dy2;

          if (dy < 0)
          {
            dy2 = -dy;
            clippinghandle = MUI_AddClipping(muiRenderInfo(obj),
                                             (WORD) data->mleft,
                                             (WORD) vpos,
                                             (WORD) data->mwidth,
                                             (WORD) dy2);
          }
          else
          {
            dy2 = dy;
            clippinghandle = MUI_AddClipping(muiRenderInfo(obj),
                                             (WORD) data->mleft,
                                             (WORD) vbot + 1 - dy2,
                                             (WORD) data->mwidth,
                                             (WORD) dy2);
          }
          DrawLines(data,fyl,lyl,PMIN,PMAX,hfirst,0,2,TRUE,0);
          MUI_RemoveClipping(muiRenderInfo(obj),clippinghandle);
        }
        else
          DrawLines(data,fyl,lyl,PMIN,PMAX,hfirst,0,2,FALSE,0);

        hmax = 0;
        ent2 = data->NList_First + data->NList_Visible;
        if (ent2 > data->NList_Entries)
          ent2 = data->NList_Entries;
        ent = data->NList_First;
        while (ent < ent2)
        {
          linelen = data->EntriesArray[ent]->PixLen;
          if (linelen > hmax)
            hmax = linelen;
          ent++;
        }
        if (data->NList_Title && (data->Title_PixLen > hmax))
          hmax = data->Title_PixLen;
        if (hfirst + data->hvisible > hmax)
          hmax = hfirst + data->hvisible;
        if (hmax != data->NList_Horiz_Entries)
          data->NList_Horiz_Entries = hmax;
        if (data->hvisible != data->NList_Horiz_Visible)
          data->NList_Horiz_Visible = data->hvisible;
        data->NList_AffFirst = data->NList_First;
        data->NList_AffFirst_Incr = data->NList_First_Incr;
      }
    }
    else if (hfirst != data->NList_Horiz_AffFirst)
    {
      LONG fyl,lyl;
      WORD dx,fx,lx;
      BOOL clipped = FALSE;

      dx = hfirst - data->NList_Horiz_AffFirst;
      if (dx < 0)
      {
        fx = data->vleft;
        lx = fx - dx;
        if (lx > data->vright + 1) lx = data->vright + 1;
      }
      else
      {
        lx = data->vright + 1;
        fx = lx - dx;
        if (fx  < data->vleft) fx = data->vleft;
      }

      fyl = data->NList_First;
      lyl = data->NList_First+data->NList_Visible+one_more;
/*
 *       if (vfyl > fyl)
 *         fyl = vfyl;
 *       if (vlyl < lyl)
 *         lyl = vlyl;
 *       if ((fyl < lyl) || ((data->vdtitlepos < data->vbottom) && ((data->vdtitlepos + data->vdtitleheight) > data->vtop)))
 */
      {
/*
 *       if ((_window(obj)->Flags & WFLG_REFRESHBITS) == WFLG_SIMPLE_REFRESH)
 *         WaitTOF();
 */
        ScrollHoriz(data,dx,LPVisible);
        need_refresh = TRUE;

        if (data->NList_Title)
          data->Title_PixLen = DrawTitle(data,fx,lx,hfirst);

/*        if (fyl < lyl)*/
        {
          if (data->NList_First_Incr)
          {
            clipped = TRUE;
            clippinghandle = MUI_AddClipping(muiRenderInfo(obj),
                                             (WORD) data->mleft,
                                             (WORD) data->vpos,
                                             (WORD) data->mwidth,
                                             (WORD) (data->NList_Visible*data->vinc));
          }

          hmax = DrawLines(data,fyl,lyl,fx,lx,hfirst,0,1,FALSE,0);

          if (clipped)
            MUI_RemoveClipping(muiRenderInfo(obj),clippinghandle);

          if (data->NList_Title && (data->Title_PixLen > hmax))
            hmax = data->Title_PixLen;
          if (hfirst + data->hvisible > hmax)
            hmax = hfirst + data->hvisible;
          if (hmax != data->NList_Horiz_Entries)
            data->NList_Horiz_Entries = hmax;
          if (data->hvisible != data->NList_Horiz_Visible)
            data->NList_Horiz_Visible = data->hvisible;
          data->NList_Horiz_AffFirst = hfirst;
        }
      }
    }

    if (!data->NList_TypeSelect && data->do_draw_active)
    {
      BOOL clipped = FALSE;

      ent = data->NList_Active;
      if ((ent >= data->NList_First) && (ent < data->NList_First + data->NList_Visible + one_more)/* && (vfyl <= ent) && (ent <= vlyl)*/)
      {
        if ((data->NList_First_Incr) && ((ent <= data->NList_First) || (ent >= data->NList_First + data->NList_Visible)))
        {
          clipped = TRUE;
          clippinghandle = MUI_AddClipping(muiRenderInfo(obj),
                                           (WORD) data->mleft,
                                           (WORD) data->vpos,
                                           (WORD) data->mwidth,
                                           (WORD) (data->NList_Visible*data->vinc));
        }

        DrawLines(data,ent,ent+1,PMIN,PMAX,hfirst,0,0,FALSE,0);

        if (clipped)
          MUI_RemoveClipping(muiRenderInfo(obj),clippinghandle);
      }
    }
    if (data->do_draw_title && data->NList_Title/* && (vfyl <= 0)*/)
    {
      data->Title_PixLen = DrawTitle(data,PMIN,PMAX,hfirst);
      if (data->Title_PixLen > data->NList_Horiz_Entries)
        data->NList_Horiz_Entries = data->Title_PixLen;
    }

    if (fullclipped)
    {
      MUI_RemoveClipping(muiRenderInfo(obj),fullclippinghandle);
      fullclipped = FALSE;
    }

    if (data->markdraw)
    {
      LONG mdy;

      data->markerasenum = -1;
      if ((data->markdrawnum >= data->NList_First) && (data->markdrawnum < data->NList_First + data->NList_Visible))
      {
        mdy = data->vpos + (data->vinc * (data->markdrawnum - data->NList_First));
        DoMethod(obj,MUIM_NList_DropDraw, data->markdrawnum,data->marktype,
                                          data->mleft,data->mright,mdy,mdy+data->vinc-1);
        data->markerasenum = data->markdrawnum;
      }
    }
    data->markdraw = FALSE;
    data->markerase = FALSE;

    data->NList_AffFirst = data->NList_First;
    data->NList_AffFirst_Incr = data->NList_First_Incr;
    data->NList_AffActive = data->NList_Active;

    if (!data->refreshing && need_refresh && (data->rp->Layer->Flags & LAYERREFRESH))
    {
      SetBackGround(data->NList_ListBackGround);
      if (MUI_BeginRefresh(muiRenderInfo(obj),0))
      {
        Object *o = (Object *)xget(_win(obj), MUIA_Window_RootObject);

        if(o)
        {
          data->refreshing = TRUE;
          MUI_Redraw(o,MADF_DRAWOBJECT);  /* MADF_DRAWALL */
          data->refreshing = FALSE;
          MUI_EndRefresh(muiRenderInfo(obj),0);
        }
      }
    }
    DrawAdjustBar(data,0);
    DrawAdjustBar(data,1);

    SetBackGround(data->NList_ListBackGround);

    data->do_draw = FALSE;
    data->do_draw_all = FALSE;
    data->do_draw_active = FALSE;
    data->do_draw_title = FALSE;
    data->first_change = LONG_MAX;
    data->last_change = LONG_MIN;
    data->minx_change_entry = -1;
    data->maxx_change_entry = -1;

    NL_UpdateScrollersValues(data);
    /*do_notifies(NTF_First|NTF_Entries|NTF_MinMax);*/

    if (data->drawall_dobit != 1)
      data->drawall_dobit = 0;

    if (data->DRAW > 1)
      data->DRAW--;
    else
      data->DRAW = 1;

    /*if (msg->flags & MADF_DRAWOBJECT) D(bug("NL: mNL_Draw(MADF_DRAWOBJECT) /after \n"));*/
  }

  return(0);
}


IPTR mNL_DropDraw(struct IClass *cl,Object *obj,struct MUIP_NList_DropDraw *msg)
{
  struct NLData *data = INST_DATA(cl,obj);

  SetABPenDrMd(data->rp,data->pens[MPEN_SHINE],data->pens[MPEN_SHADOW],JAM2);
  SetDrPt(data->rp,0xF0F0);
  if ((msg->type & MUIV_NList_DropType_Mask) == MUIV_NList_DropType_Above)
  { Move(data->rp, msg->minx, msg->miny);
    Draw(data->rp, msg->maxx, msg->miny);
  }
  else if ((msg->type & MUIV_NList_DropType_Mask) == MUIV_NList_DropType_Below)
  { Move(data->rp, msg->minx, msg->maxy);
    Draw(data->rp, msg->maxx, msg->maxy);
  }
  else if ((msg->type & MUIV_NList_DropType_Mask) == MUIV_NList_DropType_Onto)
  { Move(data->rp, msg->minx, msg->miny);
    Draw(data->rp, msg->maxx, msg->miny);
    Move(data->rp, msg->minx, msg->maxy);
    Draw(data->rp, msg->maxx, msg->maxy);
    Move(data->rp, msg->minx, msg->miny);
    Draw(data->rp, msg->minx, msg->maxy);
    Move(data->rp, msg->maxx, msg->miny);
    Draw(data->rp, msg->maxx, msg->maxy);
  }
  SetDrPt(data->rp,(UWORD)~0);
  SetABPenDrMd(data->rp,data->pens[MPEN_TEXT],data->pens[MPEN_BACKGROUND],JAM1);

  return(0);
}

