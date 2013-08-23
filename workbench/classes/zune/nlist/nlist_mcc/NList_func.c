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

#include <clib/alib_protos.h>
#include <proto/intuition.h>

#include "private.h"

#include "NList_func.h"

// static functions in this file
static BOOL NL_List_Jump(struct NLData *data, LONG pos);
static BOOL NL_List_GetPos(struct NLData *data, APTR entry, LONG *pos);

/* Extent the selection between ent1 and ent2.
   Make the first_change and last_change optimal for redrawing optimiztion */
void NL_SegChanged(struct NLData *data,LONG ent1,LONG ent2)
{
// D(bug("ent1=%ld ent2=%ld\n",ent1,ent2));
// D(bug("first_change=%ld last_change=%ld\n",data->first_change,data->last_change));
  if (ent1 < data->first_change)
  { if (ent1 >= data->NList_First)
    { data->first_change = ent1;
      if (data->last_change < data->first_change)
        data->last_change = data->first_change;
    }
    else if (ent2 >= data->NList_First)
    { data->first_change = data->NList_First;
      if (data->last_change < data->first_change)
        data->last_change = data->first_change;
    }
  }
  if (ent2 > data->last_change)
  { if (ent2 < data->NList_First+data->NList_Visible)
    { data->last_change = ent2;
      if (data->first_change > data->last_change)
        data->first_change = data->last_change;
    }
    else if (ent1 < data->NList_First+data->NList_Visible)
    { data->last_change = data->NList_First+data->NList_Visible;
      if (data->first_change > data->last_change)
        data->first_change = data->last_change;
    }
  }
// D(bug("NL_SegChanged: first_change=%ld last_change=%ld\n",data->first_change,data->last_change));
}

/* Extent the selection by this entry optimal */
void NL_Changed(struct NLData *data,LONG ent)
{
// D(bug("ent=%ld\n",ent));
// D(bug("first_change=%ld last_change=%ld\n",data->first_change,data->last_change));
  if ((ent < data->first_change) && (ent >= data->NList_First))
  { data->first_change = ent;
    if (data->last_change < data->first_change)
      data->last_change = data->first_change;
  }
  if ((ent > data->last_change) && (ent < data->NList_First+data->NList_Visible))
  { data->last_change = ent;
    if (data->first_change > data->last_change)
      data->first_change = data->last_change;
  }
// D(bug("first_change=%ld last_change=%ld\n",data->first_change,data->last_change));
}


void NL_UnSelectAll(struct NLData *data,LONG untouch_ent)
{
  LONG ent;

  DONE_NOTIFY(NTF_Select | NTF_LV_Select);
  ent = 0;
  while (ent < data->NList_Entries)
  { if (ent != untouch_ent)
    { SELECT2_CHGE(ent,TE_Select_None);
      data->lastselected = MUIV_NList_Active_Off;
      data->lastactived = MUIV_NList_Active_Off;
    }
    ent++;
  }
  REDRAW;
/*  do_notifies(NTF_AllChanges|NTF_MinMax);*/
}


void UnSelectCharSel(struct NLData *data,BOOL redraw)
{
  if (data->NList_TypeSelect && !data->UpdatingScrollbars)
  {
    if ((data->sel_pt[data->min_sel].ent >= 0) && (data->sel_pt[data->max_sel].ent >= 0))
      NL_SegChanged(data,data->sel_pt[data->min_sel].ent,data->sel_pt[data->max_sel].ent);

    data->sel_pt[1].ent = -1;
    data->min_sel = 1;
    data->max_sel = 1;
    if (redraw)
      REDRAW;
  }
}


void SelectFirstPoint(struct NLData *data,WORD x,WORD y)
{
  struct MUI_NList_TestPos_Result res;
  LONG e1 = data->sel_pt[data->min_sel].ent;
  LONG e1x = data->sel_pt[data->min_sel].xoffset;
  LONG e2 = data->sel_pt[data->max_sel].ent;
  LONG e2x = data->sel_pt[data->max_sel].xoffset;
  res.char_number = 0;
  data->last_sel_click_x = x;
  data->last_sel_click_y = y;
  NL_List_TestPos(data,x,y,&res);
  if ((res.column < 0) || (res.column >= data->numcols))
    return;
  if ((e1 >= 0) && (e2 >= 0))
  { if ((e2 > e1) && (e2x == PMIN))
      NL_SegChanged(data,e1,e2-1);
    else
      NL_SegChanged(data,e1,e2);
  }
  if ((data->NList_TypeSelect == MUIV_NList_TypeSelect_CLine) && (res.entry >= 0))
  {
    data->sel_pt[2].column =  0;
    data->sel_pt[2].xoffset = PMIN;
    data->sel_pt[2].colpos =  -1;
    data->sel_pt[2].ent =     res.entry;
    data->sel_pt[3] = data->sel_pt[2];
    data->sel_pt[3].ent =     res.entry+1;
    data->sel_pt[0] = data->sel_pt[2];
    data->sel_pt[1] = data->sel_pt[3];
    data->min_sel = 0;
    data->max_sel = 1;
    NL_Changed(data,res.entry);
    data->minx_change_entry = res.entry;
    data->minx_change_offset = PMIN;
    data->maxx_change_entry = res.entry+1;
    data->maxx_change_offset = PMIN;
    REDRAW;
  }
/*
 *   else if ((data->NList_TypeSelect == MUIV_NList_TypeSelect_CWord) && (res.entry >= 0))
 *   {
 *     data->sel_pt[0].column = res.column;
 *     data->sel_pt[0].xoffset = res.xoffset;
 *     data->sel_pt[0].colpos = res.char_number;
 *     data->sel_pt[0].ent = res.entry;
 *     data->sel_pt[1] = data->sel_pt[0];
 *     data->min_sel = 0;
 *     data->max_sel = 1;
 *     if (res.char_number == -1)
 *     { if (res.char_xoffset < 0)
 *       { if (res.column == 0)
 *         { data->sel_pt[0].xoffset = PMIN;
 *           data->sel_pt[1].xoffset += data->cols[res.column].c->minx - res.char_xoffset;
 *           data->sel_pt[1].colpos = 0;
 *         }
 *         else
 *         { data->sel_pt[0].xoffset = data->cols[res.column].c->minx;
 *           data->sel_pt[1].xoffset += data->cols[res.column].c->minx - res.char_xoffset;
 *           data->sel_pt[1].colpos = 0;
 *         }
 *       }
 *       else if (res.char_xoffset >= 0)
 *       { if (res.column == data->numcols-1)
 *         { data->sel_pt[0].xoffset += data->cols[res.column].c->minx - res.char_xoffset;
 *           data->sel_pt[0].colpos = -2;
 *           data->sel_pt[1].column =  0;
 *           data->sel_pt[1].xoffset = PMIN;
 *           data->sel_pt[1].colpos = -1;
 *           data->sel_pt[1].ent++;
 *         }
 *         else
 *         { data->sel_pt[0].xoffset += data->cols[res.column].c->minx - res.char_xoffset;
 *           data->sel_pt[0].colpos = -2;
 *           data->sel_pt[1].xoffset = data->cols[res.column].c->maxx;
 *           data->sel_pt[1].colpos = -2;
 *         }
 *       }
 *     }
 *     else
 *     {
 *       data->sel_pt[0].xoffset += data->cols[res.column].c->minx - res.char_xoffset;
 *       data->sel_pt[1].xoffset = data->sel_pt[0].xoffset + data->hinc;
 *       if (res.char_xoffset < 0)
 *         data->sel_pt[0].colpos++;
 *       data->sel_pt[1].colpos = data->sel_pt[0].colpos + 1;
 *     }
 *     data->sel_pt[2] = data->sel_pt[0];
 *     data->sel_pt[3] = data->sel_pt[1];
 *
 *     NL_Changed(data,res.entry);
 *     data->minx_change_entry = data->sel_pt[0].ent;
 *     data->minx_change_offset = data->sel_pt[0].xoffset;
 *     data->maxx_change_entry = data->sel_pt[1].ent;
 *     data->maxx_change_offset = data->sel_pt[1].xoffset;
 *     REDRAW;
 *   }
 */
  else
  { if (res.char_number == -1)
    { if (res.char_xoffset < 0)
      { if (res.column == 0)
          res.xoffset = PMIN;
        else
          res.xoffset = data->cols[res.column].c->minx;
      }
      else if (res.char_xoffset >= 0)
      { if (res.column == data->numcols-1)
        { res.xoffset = PMAX;   /* when on full right, go full left of next line */
        /*
          // The last releases had these lines while the one above had been commented out.
          // The problem with these 4 lines is, that the beginning of then next line will
          // be copied as well if the line is marked until the very right position, which
          // is definitely wrong (see bugs #1190788 and #1720456). I hope this fix does
          // not have any other negative side effects.
          res.column=0;
          res.xoffset = PMIN;
          res.char_xoffset = PMIN;
          res.entry++;
        */
        }
        else
          res.xoffset = data->cols[res.column].c->maxx;
        res.char_number = -2;
      }
    }
    else
    { res.xoffset += data->cols[res.column].c->minx - res.char_xoffset;
      if (res.char_xoffset < 0)
        res.char_number++;
    }
    data->sel_pt[0].column = res.column;
    data->sel_pt[0].xoffset = res.xoffset;
    data->sel_pt[0].colpos = res.char_number;
    data->sel_pt[0].ent = res.entry;
    data->sel_pt[1].ent = -1;
    data->min_sel = 1;
    data->max_sel = 1;
    if ((e1 >= 0) && (e2 >= 0))
    { data->minx_change_entry = e1;
      data->minx_change_offset = e1x;
      data->maxx_change_entry = e2;
      data->maxx_change_offset = e2x;
    }
    REDRAW;
  }
}


void SelectSecondPoint(struct NLData *data,WORD x,WORD y)
{
  struct MUI_NList_TestPos_Result res;
  LONG e1 = data->sel_pt[1].ent;
  LONG e1x = data->sel_pt[1].xoffset;
  LONG e2,e2x,e3,e3x;
  data->last_sel_click_x = x;
  data->last_sel_click_y = y;
  res.char_number = 0;
  NL_List_TestPos(data,x,y,&res);
  if ((res.column < 0) || (res.column >= data->numcols))
    return;
  if ((data->NList_TypeSelect == MUIV_NList_TypeSelect_CLine) && (res.entry >= 0))
  { data->sel_pt[1].column =  0;
    data->sel_pt[1].xoffset = PMIN;
    data->sel_pt[1].colpos =  -1;
    data->min_sel = 0;
    data->max_sel = 1;
    if (res.entry > data->sel_pt[2].ent)
    { e3 = 2;
      data->sel_pt[1].ent =     res.entry+1;
    }
    else if (res.entry < data->sel_pt[2].ent)
    { e3 = 3;
      data->sel_pt[1].ent =     res.entry;
      data->min_sel = 1;
      data->max_sel = 0;
    }
    else
    { e3 = 2;
      data->sel_pt[1] = data->sel_pt[3];
    }
    data->sel_pt[0] = data->sel_pt[e3];
    e2 = data->sel_pt[1].ent;
    if (e1 != e2)
    { if (e1 > e2) { e3 = e1; e1 = e2; e2 = e3; }
      data->minx_change_entry = e1;
      data->minx_change_offset = PMIN;
      data->maxx_change_entry = e2;
      data->maxx_change_offset = PMIN;
      if (e2 > e1)
        e2--;
      NL_SegChanged(data,e1,e2);
      REDRAW;
    }
  }
/*
 *   else if ((data->NList_TypeSelect == MUIV_NList_TypeSelect_CWord) && (res.entry >= 0))
 *   {
 *     LONG e0 = data->sel_pt[0].ent;
 *     LONG e0x = data->sel_pt[0].xoffset;
 *     data->sel_pt[0].column = res.column;
 *     data->sel_pt[0].xoffset = res.xoffset;
 *     data->sel_pt[0].colpos = res.char_number;
 *     data->sel_pt[0].ent = res.entry;
 *     data->sel_pt[1] = data->sel_pt[0];
 *     data->min_sel = 0;
 *     data->max_sel = 1;
 *     if (res.char_number == -1)
 *     { if (res.char_xoffset < 0)
 *       { if (res.column == 0)
 *         { data->sel_pt[0].xoffset = PMIN;
 *           data->sel_pt[1].xoffset += data->cols[res.column].c->minx - res.char_xoffset;
 *           data->sel_pt[1].colpos = 0;
 *         }
 *         else
 *         { data->sel_pt[0].xoffset = data->cols[res.column].c->minx;
 *           data->sel_pt[1].xoffset += data->cols[res.column].c->minx - res.char_xoffset;
 *           data->sel_pt[1].colpos = 0;
 *         }
 *       }
 *       else if (res.char_xoffset >= 0)
 *       { if (res.column == data->numcols-1)
 *         { data->sel_pt[0].xoffset += data->cols[res.column].c->minx - res.char_xoffset;
 *           data->sel_pt[0].colpos = -2;
 *           data->sel_pt[1].column =  0;
 *           data->sel_pt[1].xoffset = PMIN;
 *           data->sel_pt[1].colpos = -1;
 *           data->sel_pt[1].ent++;
 *         }
 *         else
 *         { data->sel_pt[0].xoffset += data->cols[res.column].c->minx - res.char_xoffset;
 *           data->sel_pt[0].colpos = -2;
 *           data->sel_pt[1].xoffset = data->cols[res.column].c->maxx;
 *           data->sel_pt[1].colpos = -2;
 *         }
 *       }
 *     }
 *     else
 *     {
 *       data->sel_pt[0].xoffset += data->cols[res.column].c->minx - res.char_xoffset;
 *       data->sel_pt[1].xoffset = data->sel_pt[0].xoffset + data->hinc;
 *       if (res.char_xoffset < 0)
 *         data->sel_pt[0].colpos++;
 *       data->sel_pt[1].colpos = data->sel_pt[0].colpos + 1;
 *     }
 *
 *     if      ( (data->sel_pt[0].ent > data->sel_pt[2].ent) ||
 *               ( (data->sel_pt[0].ent == data->sel_pt[2].ent) &&
 *                 ( (data->sel_pt[0].column > data->sel_pt[2].column) ||
 *                   ( (data->sel_pt[0].column == data->sel_pt[2].column) &&
 *                     (data->sel_pt[0].xoffset > data->sel_pt[2].xoffset)
 *                   )
 *                 )
 *               )
 *             )
 *     { data->sel_pt[0] = data->sel_pt[2];
 *       if ((e1 < data->sel_pt[1].ent) || (e1x < data->sel_pt[1].xoffset))
 *       { data->minx_change_entry = e1;
 *         data->minx_change_offset = e1x;
 *         data->maxx_change_entry = data->sel_pt[1].ent;
 *         data->maxx_change_offset = data->sel_pt[1].xoffset;
 *         NL_SegChanged(data,e1,data->sel_pt[1].ent);
 *       }
 *       else
 *       { data->minx_change_entry = data->sel_pt[1].ent;
 *         data->minx_change_offset = data->sel_pt[1].xoffset;
 *         data->maxx_change_entry = e1;
 *         data->maxx_change_offset = e1x;
 *         NL_SegChanged(data,data->sel_pt[1].ent,e1);
 *       }
 *     }
 *     else if ( (data->sel_pt[1].ent < data->sel_pt[3].ent) ||
 *               ( (data->sel_pt[1].ent == data->sel_pt[3].ent) &&
 *                 ( (data->sel_pt[1].column < data->sel_pt[3].column) ||
 *                   ( (data->sel_pt[1].column == data->sel_pt[3].column) &&
 *                     (data->sel_pt[1].xoffset < data->sel_pt[3].xoffset)
 *                   )
 *                 )
 *               )
 *             )
 *     { data->sel_pt[1] = data->sel_pt[3];
 *       if ((e0 < data->sel_pt[0].ent) || (e0x < data->sel_pt[0].xoffset))
 *       { data->minx_change_entry = e0;
 *         data->minx_change_offset = e0x;
 *         data->maxx_change_entry = data->sel_pt[0].ent;
 *         data->maxx_change_offset = data->sel_pt[0].xoffset;
 *         NL_SegChanged(data,e0,data->sel_pt[0].ent);
 *       }
 *       else
 *       { data->minx_change_entry = data->sel_pt[0].ent;
 *         data->minx_change_offset = data->sel_pt[0].xoffset;
 *         data->maxx_change_entry = e0;
 *         data->maxx_change_offset = e0x;
 *         NL_SegChanged(data,data->sel_pt[0].ent,e0);
 *       }
 *     }
 *     REDRAW;
 *   }
 */
  else if (res.entry >= 0)
  {
    if (res.char_number == -1)
    { if (res.char_xoffset < 0)
      { if (res.column == 0)
          res.xoffset = PMIN;
        else
          res.xoffset = data->cols[res.column].c->minx;
      }
      else if (res.char_xoffset >= 0)
      {
        if (res.column == data->numcols-1)
        {
          res.xoffset = PMAX;   /* when on full right, go full left of next line */
          /*
          // before this part was active while the assignment above was inactive
          // see bugs #1190788 and #1720456
          res.column=0;
          res.xoffset = PMIN;
          res.char_xoffset = PMIN;
          res.entry++;
          */
        }
        else
          res.xoffset = data->cols[res.column].c->maxx;
        res.char_number = -2;
      }
    }
    else
    {
      res.xoffset += data->cols[res.column].c->minx - res.char_xoffset;
      if (res.char_xoffset < 0)
        res.char_number++;
    }
    if ((data->sel_pt[1].ent != res.entry) || (data->sel_pt[1].column != res.column) || (data->sel_pt[1].xoffset != res.xoffset))
    {
      data->sel_pt[1].column = res.column;
      data->sel_pt[1].xoffset = res.xoffset;
      data->sel_pt[1].colpos = res.char_number;
      data->sel_pt[1].ent = res.entry;
      data->min_sel = 0;
      data->max_sel = 1;
      if ( (data->sel_pt[0].ent > data->sel_pt[1].ent) ||
           ( (data->sel_pt[0].ent == data->sel_pt[1].ent) &&
             ( (data->sel_pt[0].column > data->sel_pt[1].column) ||
               ( (data->sel_pt[0].column == data->sel_pt[1].column) &&
                 (data->sel_pt[0].xoffset > data->sel_pt[1].xoffset)
               )
             )
           )
         )
      { data->min_sel = 1;
        data->max_sel = 0;
      }
      else if ((data->sel_pt[0].ent == data->sel_pt[1].ent) &&
               (data->sel_pt[0].column == data->sel_pt[1].column) &&
               (data->sel_pt[0].xoffset == data->sel_pt[1].xoffset))
      { data->min_sel = 1;
        data->max_sel = 1;
        data->sel_pt[1].ent = -1;
      }

      e2 = data->sel_pt[1].ent;
      e2x = data->sel_pt[1].xoffset;
      if (e1 < 0)
      { e1 = data->sel_pt[0].ent;
        e1x = data->sel_pt[0].xoffset;
      }
      else if (e2 < 0)
      { e2 = data->sel_pt[0].ent;
        e2x = data->sel_pt[0].xoffset;
      }
      if ((e2 < e1) || ((e2 == e1) && (e2x < e1x)))
      { e3 = e2; e2 = e1; e1 = e3;  e3x = e2x; e2x = e1x; e1x = e3x; }
      if ((e1 >= 0) && (e2 >= 0))
      { NL_SegChanged(data,e1,e2);
        data->minx_change_entry = e1;
        data->minx_change_offset = e1x;
        data->maxx_change_entry = e2;
        data->maxx_change_offset = e2x;
      }
      REDRAW;
    }
  }
}


BOOL NL_List_First(struct NLData *data,LONG lf,struct TagItem *tag)
{
  struct TagItem ltag;
  BOOL scrolled = FALSE;

  ENTER();

  if (!tag)
    tag = &ltag;

  if((data->NList_Entries > 0) && (lf != data->NList_First))
  {
    switch (lf)
    {
      case MUIV_NList_First_Top:
        lf = 0;
      break;

      case MUIV_NList_First_Bottom:
      {
        lf = data->NList_Entries - data->NList_Visible;
        if (lf < 0)
          lf = 0;
      }
      break;

      case MUIV_NList_First_Up:
      {
        lf = data->NList_First - 1;
        if (lf < 0)
          lf = 0;
      }
      break;

      case MUIV_NList_First_Down:
      {
        lf = data->NList_First + 1;
        if (lf + data->NList_Visible >= data->NList_Entries)
          lf = data->NList_Entries - data->NList_Visible;
        if (lf < 0)
          lf = 0;
      }
      break;

      case MUIV_NList_First_PageUp:
      {
        lf = data->NList_First - data->NList_Visible + 1;
        if (lf < 0)
          lf = 0;
      }
      break;

      case MUIV_NList_First_PageDown:
      {
        lf = data->NList_First + data->NList_Visible - 1;
        if (lf + data->NList_Visible >= data->NList_Entries)
          lf = data->NList_Entries - data->NList_Visible;
        if (lf < 0)
          lf = 0;
      }
      break;

      case MUIV_NList_First_Up2:
      {
        lf = data->NList_First - 2;
        if (lf < 0)
          lf = 0;
      }
      break;

      case MUIV_NList_First_Down2:
      {
        lf = data->NList_First + 2;
        if (lf + data->NList_Visible >= data->NList_Entries)
          lf = data->NList_Entries - data->NList_Visible;
        if (lf < 0)
          lf = 0;
      }
      break;

      case MUIV_NList_First_Up4:
      {
        lf = data->NList_First - 4;
        if (lf < 0)
          lf = 0;
      }
      break;

      case MUIV_NList_First_Down4 :
      {
        lf = data->NList_First + 4;
        if (lf + data->NList_Visible >= data->NList_Entries)
          lf = data->NList_Entries - data->NList_Visible;
        if (lf < 0)
          lf = 0;
      }
      break;
    }

    if ((lf >= 0) && (lf < data->NList_Entries))
    {
      if (data->NList_First != lf)
      {
        DO_NOTIFY(NTF_First);
        scrolled = TRUE;
      }

      data->NList_First = lf;
      REDRAW;
/*      do_notifies(NTF_AllChanges|NTF_MinMax);*/
      tag->ti_Data = lf;
    }
    else
      tag->ti_Tag = TAG_IGNORE;
  }
  else
    tag->ti_Tag = TAG_IGNORE;

  RETURN(scrolled);
  return scrolled;
}


BOOL NL_List_Active(struct NLData *data, LONG la, struct TagItem *tag, LONG newactsel, LONG acceptsame, ULONG flags)
{
  struct TagItem ltag;
  struct TagItem *tag2 = tag;
  LONG ent;
  BOOL changed = FALSE;

  ENTER();

  //D(DBF_STARTUP, "NL_List_Active: 0x%lx la=%ld newactsel=%ld acceptsame=%ld data->pad1=%ld",obj,la,newactsel,acceptsame,data->pad1);

  if (data->NList_TypeSelect || !data->NList_Input)
  {
    if (tag2)
      tag->ti_Data = MUIV_NList_Active_Off;

    if((la == MUIV_NList_Active_Off) || (data->NList_Entries <= 0))
    {
      data->pad1 = MUIV_NList_Active_Off;

      RETURN(TRUE);
      return TRUE;
    }
    else if (la < 0)
    {
      if(tag2 == NULL)
      {
        changed = NL_List_First(data,la,tag);

        RETURN(changed);
        return changed;
      }
      else if (data->NList_Entries > 0)
      {
        switch (la)
        {
          case MUIV_NList_Active_Top :
          case MUIV_NList_Active_UntilTop :
            la = 0;
            break;
          case MUIV_NList_Active_Bottom :
          case MUIV_NList_Active_UntilBottom :
            la = data->pad1 - 1;
            break;
          case MUIV_NList_Active_Up :
            if (data->NList_Active != MUIV_NList_Active_Off)
              la = data->pad1 - 1;
            else
              la = data->NList_First - 1 + data->NList_Visible;
            break;
          case MUIV_NList_Active_Down :
            if (data->pad1 != MUIV_NList_Active_Off)
              la = data->pad1 + 1;
            else
              la = data->NList_First;
            break;
          case MUIV_NList_Active_PageUp :
          case MUIV_NList_Active_UntilPageUp :
            if (data->pad1 == data->NList_First)
              la = data->pad1 + 1 - data->NList_Visible;
            else
              la = data->NList_First;
            break;
          case MUIV_NList_Active_PageDown :
          case MUIV_NList_Active_UntilPageDown :
            if (data->pad1 == data->NList_First - 1 + data->NList_Visible)
              la = data->pad1 - 1 + data->NList_Visible;
            else
              la = data->NList_First - 1 + data->NList_Visible;
            break;
        }
      }
    }

    if (la < 0)
      la = 0;
    else if (la >= data->NList_Entries)
      la = data->NList_Entries - 1;

    data->pad1 = la;

    // if the SetActive_Jump_Center flag is active we make sure
    // the new active entry will be centered
    if(isFlagSet(flags, MUIV_NList_SetActive_Jump_Center))
      la = MUIV_NList_Jump_Active_Center;

    // make sure the entry is visible
    if(NL_List_Jump(data, la) == TRUE)
    {
      DO_NOTIFY(NTF_First);
      REDRAW;

      changed = TRUE;
    }

    RETURN(changed);
    return changed;
  }

  if (!tag)
    tag = &ltag;

  if(data->NList_Entries > 0)
  {
    ent = -2;

    switch (la)
    {
      case MUIV_NList_Active_Top :
        la = 0;
        break;
      case MUIV_NList_Active_Bottom :
        la = data->NList_Entries - 1;
        break;
      case MUIV_NList_Active_Up :
        if (data->NList_Active != MUIV_NList_Active_Off)
          la = data->NList_Active - 1;
        else
          la = data->NList_First - 1 + data->NList_Visible;
        if (la < 0)
          la = 0;
        if (la >= data->NList_Entries)
          la = data->NList_Entries - 1;
        break;
      case MUIV_NList_Active_Down :
        if (data->NList_Active != MUIV_NList_Active_Off)
          la = data->NList_Active + 1;
        else
          la = data->NList_First;
        if (la >= data->NList_Entries)
          la = data->NList_Entries - 1;
        break;
      case MUIV_NList_Active_PageUp :
        if (data->NList_Active == data->NList_First)
          la = data->NList_Active + 1 - data->NList_Visible;
        else
          la = data->NList_First;
        if (la < 0)
          la = 0;
        break;
      case MUIV_NList_Active_PageDown :
        if (data->NList_Active == data->NList_First - 1 + data->NList_Visible)
          la = data->NList_Active - 1 + data->NList_Visible;
        else
          la = data->NList_First - 1 + data->NList_Visible;
        if (la >= data->NList_Entries)
          la = data->NList_Entries - 1;
        break;
      case MUIV_NList_Active_UntilPageUp :
        if (data->NList_Active == data->NList_First)
          la = data->NList_Active + 1 - data->NList_Visible;
        else
          la = data->NList_First;
        if (la < 0)
          la = 0;
        ent = data->NList_Active;
        break;
      case MUIV_NList_Active_UntilPageDown :
        if (data->NList_Active == data->NList_First - 1 + data->NList_Visible)
          la = data->NList_Active - 1 + data->NList_Visible;
        else
          la = data->NList_First - 1 + data->NList_Visible;
        if (la >= data->NList_Entries)
          la = data->NList_Entries - 1;
        ent = data->NList_Active;
        break;
      case MUIV_NList_Active_UntilTop :
        la = 0;
        ent = data->NList_Active;
        break;
      case MUIV_NList_Active_UntilBottom :
        la = data->NList_Entries - 1;
        ent = data->NList_Active;
        break;
      case MUIV_NList_Active_Off :
        la = MUIV_NList_Active_Off;
        break;
    }

    if ((la != data->NList_Active) || (acceptsame &&
         !((newactsel == MUIV_NList_Select_On) && (la >= 0) && (la < data->NList_Entries) &&
           (data->EntriesArray[la]->Select != TE_Select_None))))
    {
      if ((la >= 0) && (la < data->NList_Entries))
      {
        if ((data->NList_MultiSelect == MUIV_NList_MultiSelect_None)  && (la != data->NList_Active))
        {
          DO_NOTIFY(NTF_Select | NTF_LV_Select);

          if ((data->NList_Active >= 0) && (data->NList_Active < data->NList_Entries))
            SELECT2(data->NList_Active,TE_Select_None);
        }

        if ((data->NList_MultiSelect != MUIV_NList_MultiSelect_None) && (newactsel == MUIV_NList_Select_List))
        {
          DO_NOTIFY(NTF_Select | NTF_LV_Select);
          if((data->NList_Active >= 0) && (data->NList_Active < data->NList_Entries))
          {
            if (data->EntriesArray[data->NList_Active]->Select != TE_Select_None)
              SELECT2(data->NList_Active,TE_Select_None);
            else
              SELECT(data->NList_Active,TE_Select_Line);

            data->lastselected = data->NList_Active;
            data->lastactived = data->NList_Active;
          }
        }

        if((ent < -1) || (data->multiselect == MUIV_NList_MultiSelect_None))
          ent = la;

        if (ent < 0)
          ent = 0;
        else if (ent >= data->NList_Entries)
          ent = data->NList_Entries - 1;

        set_Active(la);
        changed = TRUE;

        if (ent < la)
          NL_SegChanged(data,ent,la-1);
        else if (ent > la)
          NL_SegChanged(data,la+1,ent);

        do
        {
          if (ent < la)
            ent++;
          else if (ent > la)
            ent--;
          switch (newactsel)
          { case MUIV_NList_Select_Off :
              SELECT2(ent,TE_Select_None);
              data->lastselected = ent;
              data->lastactived = ent;
              break;
            case MUIV_NList_Select_On :
              SELECT(ent,TE_Select_Line);
              data->lastselected = ent;
              data->lastactived = ent;
              break;
            case MUIV_NList_Select_Toggle :
              if (data->EntriesArray[ent]->Select != TE_Select_None)
              { SELECT2(ent,TE_Select_None);
              }
              else
              { SELECT(ent,TE_Select_Line);
              }
              data->lastselected = ent;
              data->lastactived = ent;
              break;

            case MUIV_NList_Select_List :
              if (data->NList_MultiSelect != MUIV_NList_MultiSelect_None)
              { ent = la;
                if (data->EntriesArray[ent]->Select != TE_Select_None)
                { SELECT2(ent,TE_Select_None);
                }
                else
                { SELECT(ent,TE_Select_Line);
                }
                data->lastselected = ent;
                data->lastactived = ent;
                break;
              }

            default:
              if (data->NList_MultiSelect == MUIV_NList_MultiSelect_None)
              { SELECT(ent,TE_Select_Line);
                data->lastselected = ent;
                data->lastactived = ent;
              }
              break;
          }
        } while (ent != la);

        tag->ti_Data = la;
        data->do_draw_active = TRUE;

        // if the SetActive_Jump_Center flag is active we make sure
        // the new active entry will be centered
        if(isFlagSet(flags, MUIV_NList_SetActive_Jump_Center))
          la = MUIV_NList_Jump_Active_Center;

        // make sure the entry is visible
        if(NL_List_Jump(data, la) == TRUE)
        {
          DO_NOTIFY(NTF_First);
        }

        // redraw at all means
        REDRAW;
      }
      else if (la == MUIV_NList_Active_Off)
      {
        if ((data->NList_MultiSelect == MUIV_NList_MultiSelect_None)  && (la != data->NList_Active))
        {
          DO_NOTIFY(NTF_Select | NTF_LV_Select);
          if ((data->NList_Active >= 0) && (data->NList_Active < data->NList_Entries))
            SELECT2(data->NList_Active,TE_Select_None);
        }

        set_Active(MUIV_NList_Active_Off);
        tag->ti_Data = MUIV_NList_Active_Off;
        REDRAW;

        changed = TRUE;
      }
      else
        tag->ti_Tag = TAG_IGNORE;
    }
    else
      tag->ti_Tag = TAG_IGNORE;
  }
  else
  {
    set_Active(MUIV_NList_Active_Off);
    tag->ti_Data = MUIV_NList_Active_Off;
    if (la >= 0)
      tag->ti_Tag = TAG_IGNORE;
  }
/*  do_notifies(NTF_AllChanges|NTF_MinMax);*/

  RETURN(changed);
  return changed;
}


BOOL NL_List_Horiz_First(struct NLData *data,LONG hf,struct TagItem *tag)
{
  BOOL scrolled = FALSE;

  ENTER();

  if(data->NList_Horiz_First != hf)
  {
    struct TagItem ltag;
    BOOL tagnul = FALSE;

    if(!tag)
    {
      tag = &ltag;
      tagnul = TRUE;
    }

    switch (hf)
    {
      case MUIV_NList_Horiz_First_Start :
        hf = 0;
      break;

      case MUIV_NList_Horiz_First_End :
      {
        hf = data->NList_Horiz_Entries - data->NList_Horiz_Visible;
        if(hf < 0)
          hf = 0;
      }
      break;

      case MUIV_NList_Horiz_First_Left :
      {
        hf = data->NList_Horiz_First - _font(data->this)->tf_XSize;
        if (hf < 0)
          hf = 0;
      }
      break;

      case MUIV_NList_Horiz_First_Right :
      {
        hf = data->NList_Horiz_First + _font(data->this)->tf_XSize;
        if(hf > data->NList_Horiz_Entries - data->NList_Horiz_Visible)
          hf = data->NList_Horiz_Entries - data->NList_Horiz_Visible;
      }
      break;

      case MUIV_NList_Horiz_First_PageLeft :
      {
        hf = data->NList_Horiz_First - data->NList_Horiz_Visible / 2;
        if (hf < 0)
          hf = 0;
      }
      break;

      case MUIV_NList_Horiz_First_PageRight :
      {
        hf = data->NList_Horiz_First + data->NList_Horiz_Visible / 2;
        if (hf > data->NList_Horiz_Entries - data->NList_Horiz_Visible)
          hf = data->NList_Horiz_Entries - data->NList_Horiz_Visible;
      }
      break;

      case MUIV_NList_Horiz_First_Left2 :
      {
        hf = data->NList_Horiz_First - _font(data->this)->tf_XSize*2;
        if (hf < 0)
          hf = 0;
      }
      break;

      case MUIV_NList_Horiz_First_Right2 :
      {
        hf = data->NList_Horiz_First + _font(data->this)->tf_XSize*2;
        if (hf > data->NList_Horiz_Entries - data->NList_Horiz_Visible)
          hf = data->NList_Horiz_Entries - data->NList_Horiz_Visible;
      }
      break;

      case MUIV_NList_Horiz_First_Left4 :
      {
        hf = data->NList_Horiz_First - _font(data->this)->tf_XSize*4;
        if (hf < 0)
          hf = 0;
      }
      break;

      case MUIV_NList_Horiz_First_Right4 :
      {
        hf = data->NList_Horiz_First + _font(data->this)->tf_XSize*4;
        if (hf > data->NList_Horiz_Entries - data->NList_Horiz_Visible)
          hf = data->NList_Horiz_Entries - data->NList_Horiz_Visible;
      }
      break;
    }

    if(data->NList_Horiz_First != hf)
    {
      if((hf >= 0) && (hf < data->NList_Horiz_Entries))
      {
        if(hf > data->NList_Horiz_Entries - data->NList_Horiz_Visible)
          hf = data->NList_Horiz_Entries - data->NList_Horiz_Visible;

        data->NList_Horiz_First = hf;
        REDRAW;

        tag->ti_Data = hf;
        if (tagnul /*&& data->Notify_HSB*/)
          notdoset(data->this,MUIA_NList_Horiz_First,hf);

        scrolled = TRUE;
      }
      else
        tag->ti_Tag = TAG_IGNORE;
    }
    else
      tag->ti_Tag = TAG_IGNORE;
  }

  RETURN(scrolled);
  return scrolled;
}


ULONG NL_List_SelectChar(struct NLData *data,LONG pos,LONG seltype,LONG *state)
{
  LONG ent,ent2;

  if (seltype == MUIV_NList_Select_None)
    return (TRUE);

  if (data->NList_TypeSelect)
  {
    if (seltype == MUIV_NList_Select_Ask)
    { LONG first = data->sel_pt[data->min_sel].ent;
      LONG last = data->sel_pt[data->max_sel].ent;
      if ((data->sel_pt[data->max_sel].column == 0) && (data->sel_pt[data->max_sel].xoffset == PMIN))
        last--;
      if (state)
        *state = (LONG) (last - first + 1);
    }
    else if (seltype == MUIV_NList_Select_Off)
      UnSelectCharSel(data,TRUE);
    else if ((seltype == MUIV_NList_Select_On) && (data->NList_Entries > 0))
    {
      UnSelectCharSel(data,FALSE);
      if (pos == MUIV_NList_Select_All)
      { ent = 0;
        ent2 = data->NList_Entries;
      }
      else
      { if (pos == MUIV_NList_Select_Active)
          ent = data->NList_Active;
        else
          ent = pos;
        if (ent < 0)
          ent = 0;
        else if (ent >= data->NList_Entries)
          ent = data->NList_Entries - 1;
        ent2 = ent + 1;
      }
      data->sel_pt[0].column = 0;
      data->sel_pt[0].xoffset = PMIN;
      data->sel_pt[0].colpos = -1;
      data->sel_pt[0].ent = ent;
      data->sel_pt[1].column = 0;
      data->sel_pt[1].xoffset = PMIN;
      data->sel_pt[1].colpos = -1;
      data->sel_pt[1].ent = ent2;
      data->min_sel = 0;
      data->max_sel = 1;
      NL_SegChanged(data,ent,ent2);
      data->minx_change_entry = ent;
      data->minx_change_offset = PMIN;
      data->maxx_change_entry = ent2;
      data->maxx_change_offset = PMIN;
      REDRAW;
    }
    return (TRUE);
  }

  return(FALSE);
}


ULONG NL_List_Select(struct NLData *data,LONG pos,LONG pos2,LONG seltype,LONG *state)
{
  LONG ent,ent2,ent3;
/*  DONE_NOTIFY(NTF_Select | NTF_LV_Select);*/
  if ((seltype == MUIV_NList_Select_None) || data->NList_TypeSelect || !data->NList_Input)
    return (TRUE);
  if ((pos == MUIV_NList_Select_All) && data->multiselect && data->NList_Input)
  { LONG num = 0;
    ent = 0;
    switch (seltype)
    { case MUIV_NList_Select_Off :
        while (ent < data->NList_Entries)
        { if ((data->NList_List_Select == MUIV_NList_Select_List) && (ent == data->NList_Active))
          { SELECT2_CHGE(ent,TE_Select_Line);
            num++;
          }
          else
          { SELECT2_CHGE(ent,TE_Select_None);
          }
          ent++;
        }
        if (state)
          *state = num;
        REDRAW;
        break;
      case MUIV_NList_Select_On :
        while (ent < data->NList_Entries)
        { SELECT_CHGE(ent,TE_Select_Line);
          num++;
          ent++;
        }
        if (state)
          *state = num;
        REDRAW;
        break;
      case MUIV_NList_Select_Toggle :
        while (ent < data->NList_Entries)
        { if ((data->NList_List_Select == MUIV_NList_Select_List) && (ent == data->NList_Active))
          { SELECT2_CHGE(ent,TE_Select_Line);
            num++;
          }
          else if (data->EntriesArray[ent]->Select != TE_Select_None)
          { SELECT2_CHGE(ent,TE_Select_None);
          }
          else
          { SELECT_CHGE(ent,TE_Select_Line);
            num++;
          }
          ent++;
        }
        if (state)
          *state = num;
        REDRAW;
        break;
      default :   /* MUIV_NList_Select_Ask */
        while (ent < data->NList_Entries)
        { if (data->EntriesArray[ent]->Select != TE_Select_None)
            num++;
          ent++;
        }
        if (state)
          *state = num;
        break;
    }
    data->lastselected = MUIV_NList_Active_Off;
/*    do_notifies(NTF_AllChanges|NTF_MinMax);*/
    return (TRUE);
  }
  else if ((data->multiselect || (seltype == MUIV_NList_Select_Ask)) && data->NList_Input)
  { if (pos == MUIV_NList_Select_Active)
      ent = data->NList_Active;
    else
      ent = pos;
    if ((ent < 0) || (ent >= data->NList_Entries))
      return (FALSE);

    if (seltype != MUIV_NList_Select_Ask)
      data->lastselected = ent;
/*
 *    if ((data->NList_List_Select == MUIV_NList_Select_List) && (ent == data->NList_Active) && (seltype == MUIV_NList_Select_Ask))
 *      seltype = MUIV_NList_Select_On;
 */
    ent2 = pos2;
    if ((ent2 < 0) || (ent2 >= data->NList_Entries))
      ent2 = ent;
    if (ent2 < ent)
    { ent3 = ent;
      ent = ent2 + 1;
      ent2 = ent3 + 1;
    }
    else if (ent2 == ent)
      ent2++;
    while (ent < ent2)
    { switch (seltype)
      { case MUIV_NList_Select_Off :
          if (state) *state = MUIV_NList_Select_Off;
          SELECT2_CHGE(ent,TE_Select_None);
          break;
        case MUIV_NList_Select_On :
          if (state) *state = MUIV_NList_Select_On;
          SELECT_CHGE(ent,TE_Select_Line);
          break;
        case MUIV_NList_Select_Toggle :
          if (data->EntriesArray[ent]->Select == TE_Select_None)
          { if (state) *state = MUIV_NList_Select_On;
            SELECT_CHGE(ent,TE_Select_Line);
          }
          else
          { if (state) *state = MUIV_NList_Select_Off;
            SELECT2_CHGE(ent,TE_Select_None);
          }
          break;
        default :   /* MUIV_NList_Select_Ask */
          if (data->EntriesArray[ent]->Select != TE_Select_None)
          { if (state) *state = MUIV_NList_Select_On; }
          else
          { if (state) *state = MUIV_NList_Select_Off; }
          break;
      }
      ent++;
    }
    if (ASKED_NOTIFY(NTF_Select | NTF_LV_Select))
    { REDRAW;
    }
/*    do_notifies(NTF_AllChanges|NTF_MinMax);*/
    return (TRUE);
  }
  return (FALSE);
}


ULONG NL_List_TestPos(struct NLData *data,LONG x,LONG y,struct MUI_NList_TestPos_Result *res)
{
  if ((x == MUI_MAXMAX) && (y == MUI_MAXMAX))
  {
    x = data->click_x;
    y = data->click_y;
  }
  else if ((x == MUI_MAXMAX) && (y == 0))
  {
    x = data->mouse_x;
    y = data->mouse_y;
  }
  if (res)
  {
    WORD ly = (y - data->vpos);
    WORD lyl = ly / data->vinc;
    WORD lx = (x - data->hpos);
    res->entry = -1;
    res->column = -1;
    res->flags = 0;
    res->xoffset = 0;
    res->yoffset = 0;
    res->preparse = 0;
    if (res->char_number != -2)
      res->char_number = -1;
    res->char_xoffset = 0;
    if ((lx >= 0) && (lx < data->NList_Horiz_Visible) &&
        (ly >= 0) && (lyl < data->NList_Visible))
    { LONG line = lyl + data->NList_First;
      if (!data->NList_Title && (ly < data->vinc/2))
        res->flags |= MUI_NLPR_ONTOP;
      lx +=  data->NList_Horiz_First;
      if ((line >= 0) && ((line < data->NList_First + data->NList_Visible) || (line < data->NList_Entries)))
      { if (data->cols)
        { res->column = 1;
          while (res->column < data->numcols)
          { if (data->cols[res->column].c->minx > lx)
              break;
            res->column++;
          }
          res->column--;
          if ((res->column == data->numcols-1) && (lx > data->cols[res->column].c->maxx+data->cols[res->column].c->delta))
            res->column = -1;
          else
          {
            if ((lx > data->cols[res->column].c->maxx-2) && (res->column < data->numcols-1))
              res->flags |= MUI_NLPR_BAR;
            else if ((res->column == data->numcols-1) && IS_BAR(res->column,data->cols[res->column].c) &&
                     (lx > data->cols[res->column].c->maxx-4) && (lx <= data->cols[res->column].c->maxx+data->cols[res->column].c->delta))
              res->flags |= MUI_NLPR_BAR;
            else if ((res->column > 0) && (res->column == data->numcols-1) &&
                     (data->NList_Horiz_First + data->NList_Horiz_Visible >= data->NList_Horiz_Entries) &&
                     (x >= data->mright - 2) && (res->flags & MUI_NLPR_ONTOP))
              res->flags |= MUI_NLPR_BAR;
            lx -= data->cols[res->column].c->minx;
          }
        }
        else
          res->column = -1;
      }
      if ((res->column >= 0) && (line >= 0) && (line < data->NList_Entries))
      { res->entry = line;
        res->xoffset = lx;
        res->yoffset = ly - ((lyl * data->vinc) + (data->vinc / 2));
        if (res->char_number != -2)
          FindCharInColumn(data,line,res->column,lx,&res->char_xoffset,&res->char_number);
        if (res->char_number < -PREPARSE_OFFSET_ENTRY)
        { res->char_number += PREPARSE_OFFSET_COL;
          res->preparse = 2;
        }
        else if (res->char_number < -1)
        { res->char_number += PREPARSE_OFFSET_ENTRY;
          res->preparse = 1;
        }
      }
      else
      { if (line < 0)
          res->flags |= MUI_NLPR_ABOVE;
        if (line >= data->NList_Entries)
          res->flags |= MUI_NLPR_BELOW;
      }
    }
    else if (data->NList_Title && (lx >= 0) && (lx < data->NList_Horiz_Visible) &&
             (ly < 0) && (y >= data->vdtitlepos))
    { res->flags = MUI_NLPR_ABOVE | MUI_NLPR_TITLE | MUI_NLPR_ONTOP;
      lx +=  data->NList_Horiz_First;
      if (data->cols)
      { res->column = 1;
        while (res->column < data->numcols)
        { if (data->cols[res->column].c->minx > lx)
            break;
          res->column++;
        }
        res->column--;
        if ((res->column == data->numcols-1) && (lx > data->cols[res->column].c->maxx+data->cols[res->column].c->delta))
          res->column = -1;
        else
        {
          if ((lx > data->cols[res->column].c->maxx-2) && (res->column < data->numcols-1))
            res->flags |= MUI_NLPR_BAR;
          else if ((res->column == data->numcols-1) && IS_BAR(res->column,data->cols[res->column].c) &&
                   (lx > data->cols[res->column].c->maxx-4) && (lx <= data->cols[res->column].c->maxx+data->cols[res->column].c->delta))
            res->flags |= MUI_NLPR_BAR;
          else if ((res->column > 0) && (res->column == data->numcols-1) &&
                   (data->NList_Horiz_First + data->NList_Horiz_Visible >= data->NList_Horiz_Entries) &&
                   (x >= data->mright - 2))
            res->flags |= MUI_NLPR_BAR;
          lx -= data->cols[res->column].c->minx;
        }
      }
      else
        res->column = -1;
      res->xoffset = lx;
      res->yoffset = y - ((data->vdtitlepos + data->vpos) / 2);
    }
    else
    { if (lx < 0)
        res->flags |= MUI_NLPR_LEFT;
      else if (lx >= data->NList_Horiz_Visible)
        res->flags |= MUI_NLPR_RIGHT;
      if (ly < 0)
        res->flags |= MUI_NLPR_ABOVE;
      else if (lyl >= data->NList_Visible)
        res->flags |= MUI_NLPR_BELOW;
    }
    return (TRUE);
  }
  return (FALSE);
}


ULONG NL_List_TestPosOld(struct NLData *data,LONG x,LONG y,struct MUI_List_TestPos_Result *res)
{
  if (res)
  {
    WORD ly = (y - data->vpos);
    WORD lyl = ly / data->vinc;
    WORD lx = (x - data->hpos);
    res->entry = -1;
    res->column = -1;
    res->flags = 0;
    res->xoffset = 0;
    res->yoffset = 0;
    if ((lx >= 0) && (lx < data->NList_Horiz_Visible) &&
        (ly >= 0) && (lyl < data->NList_Visible))
    { LONG line = lyl + data->NList_First;
      if ((line >= 0) && (line < data->NList_Entries))
      { res->entry = line;
        lx +=  data->NList_Horiz_First;
        if (data->cols)
        { res->column = 1;
          while (res->column < data->numcols)
          { if (data->cols[res->column].c->minx > lx)
              break;
            res->column++;
          }
          res->column--;
          lx -= data->cols[res->column].c->minx;
        }
        else
          res->column = 0;
        res->xoffset = lx;
        res->yoffset = ly - ((lyl * data->vinc) + (data->vinc / 2));
      }
      else
      { if (line < 0)
          res->flags |= MUI_NLPR_ABOVE;
        if (line >= data->NList_Entries)
          res->flags |= MUI_NLPR_BELOW;
      }
    }
    else
    { if (lx < 0)
        res->flags |= MUI_NLPR_LEFT;
      else if (lx >= data->NList_Horiz_Visible)
        res->flags |= MUI_NLPR_RIGHT;
      if (ly < 0)
        res->flags |= MUI_NLPR_ABOVE;
      else if (lyl >= data->NList_Visible)
        res->flags |= MUI_NLPR_BELOW;
    }
    return (TRUE);
  }
  return (FALSE);
}


IPTR mNL_List_GetEntry(struct IClass *cl,Object *obj,struct  MUIP_NList_GetEntry *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  LONG ent = msg->pos;
  APTR entry = NULL;

  if(ent == MUIV_NList_GetEntry_Active)
    ent = data->NList_Active;

  if(ent >= 0 && ent < data->NList_Entries && isFlagSet(data->EntriesArray[ent]->Wrap, TE_Wrap_TmpLine))
    ent -= data->EntriesArray[ent]->dnum;

  if(ent >= 0 && ent < data->NList_Entries)
    entry = data->EntriesArray[ent]->Entry;

  // return the entry in the message if we got a valid pointer to store it
  if(msg->entry != NULL)
    *msg->entry = entry;

  // return the entry as a normal return value in any case
  return (IPTR)entry;
}


IPTR mNL_List_GetEntryInfo(struct IClass *cl,Object *obj,struct  MUIP_NList_GetEntryInfo *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  LONG line,ent2,ent = msg->res->pos;
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/

  if (ent == MUIV_NList_GetEntryInfo_Line)
  { ent = line = 0;
    ent2 = msg->res->line;
    while (ent < data->NList_Entries)
    { if (!(data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine))
      { if (line == ent2)
          break;
        line++;
      }
      ent++;
    }
    if ((ent >= 0) && (ent < data->NList_Entries))
    { msg->res->entry_pos = ent;
      msg->res->entry = data->EntriesArray[ent]->Entry;
      msg->res->wrapcol = (LONG) data->EntriesArray[ent]->Wrap;
      msg->res->charpos = (LONG) data->EntriesArray[ent]->pos;
      msg->res->charlen = (LONG) data->EntriesArray[ent]->len;
    }
    else
    { msg->res->line = -1;
      msg->res->entry_pos = -1;
      msg->res->entry = NULL;
      msg->res->wrapcol = (LONG) 0;
      msg->res->charpos = (LONG) 0;
      msg->res->charlen = (LONG) 0;
    }
  }
  else
  { if (ent == MUIV_NList_GetEntry_Active)
      ent = data->NList_Active;

    if (ent == -3 ) /* Magic number to get the last inserted entry <aphaso> */
    {
      if ( data->NList_LastInserted == -1 )
        ent = (data->NList_Entries - 1);
      else
        ent = data->NList_LastInserted;
    }

    ent2 = ent;
    if ((ent >= 0) && (ent < data->NList_Entries) && (data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine))
      ent -= data->EntriesArray[ent]->dnum;
    if ((ent >= 0) && (ent < data->NList_Entries))
    {
      if (msg->res->line == -2) /* Magic number to receive the NList entry pointer, not the supplied! <aphaso> */
      {
        msg->res->entry_pos = ent;
        msg->res->entry = data->EntriesArray[ent];
      }
      else
      {
        msg->res->entry_pos = ent;
        msg->res->entry = data->EntriesArray[ent]->Entry;
        msg->res->wrapcol = (LONG) data->EntriesArray[ent2]->Wrap;
        msg->res->charpos = (LONG) data->EntriesArray[ent2]->pos;
        msg->res->charlen = (LONG) data->EntriesArray[ent2]->len;
        ent2 = line = 0;
        while (ent2 < ent)
        { if (!(data->EntriesArray[ent2]->Wrap & TE_Wrap_TmpLine))
            line++;
          ent2++;
        }
        msg->res->line = line;
      }
    }
    else
    { msg->res->line = -1;
      msg->res->entry_pos = -1;
      msg->res->entry = NULL;
      msg->res->wrapcol = (LONG) 0;
      msg->res->charpos = (LONG) 0;
      msg->res->charlen = (LONG) 0;
    }
  }
  return (TRUE);
}

static BOOL NL_List_Jump(struct NLData *data, LONG pos)
{
  BOOL result = FALSE;
  ENTER();

  switch(pos)
  {
    case MUIV_NList_Jump_Top:
    {
      pos = 0;
    }
    break;

    case MUIV_NList_Jump_Bottom:
    {
      pos = data->NList_Entries - 1;
    }
    break;

    case MUIV_NList_Jump_Active:
    {
      pos = data->NList_Active;
    }
    break;

    case MUIV_NList_Jump_Up:
    {
      pos = data->NList_First - 1;
    }
    break;

    case MUIV_NList_Jump_Down:
    {
      pos = data->NList_First + data->NList_Visible;
    }
    break;

    case MUIV_NList_Jump_Active_Center:
    {
      // center the item in the visible area
      LONG first = data->NList_Active - data->NList_Visible/2;

      // make sure that the last item is displayed in the last line
      while(first + data->NList_Visible > data->NList_Entries)
        first--;

      if(first < 0)
        first = 0;

      data->NList_First = first;

      result = TRUE;
      pos = -1;
    }
    break;
  }

  if(pos >= 0 && pos < data->NList_Entries)
  {
    // old style jump, just make the requested item visible
    if(pos < data->NList_First)
    {
      data->NList_First = pos;

      result = TRUE;
    }
    else if(pos >= data->NList_First + data->NList_Visible)
    {
      data->NList_First = pos - data->NList_Visible + 1;

      // make sure that the last item is displayed in the last line
      while(data->NList_First + data->NList_Visible > data->NList_Entries)
        data->NList_First--;

      if(data->NList_First < 0)
        data->NList_First = 0;

      result = TRUE;
    }
  }

  RETURN(result);
  return result;
}

IPTR mNL_List_Jump(struct IClass *cl, Object *obj, struct MUIP_NList_Jump *msg)
{
  struct NLData *data = INST_DATA(cl,obj);

  ENTER();

  if(NL_List_Jump(data, msg->pos) == TRUE)
  {
    DO_NOTIFY(NTF_First);
    REDRAW;
  }

/*  do_notifies(NTF_AllChanges|NTF_MinMax);*/

  RETURN(TRUE);
  return TRUE;
}

IPTR mNL_List_SetActive(struct IClass *cl, Object *obj, struct MUIP_NList_SetActive *msg)
{
  BOOL result = FALSE;
  struct NLData *data = INST_DATA(cl,obj);
  LONG pos = (LONG)(SIPTR)msg->pos;

  ENTER();

  // check if the user used msg->pos for specifying the entry position (integer)
  // or by using the entry address
  if(isFlagSet(msg->flags, MUIV_NList_SetActive_Entry))
  {
    pos = MUIV_NList_GetPos_Start;
    NL_List_GetPos(data, (APTR)msg->pos, &pos);
  }

  result = NL_List_Active(data, pos, NULL, data->NList_List_Select, FALSE, msg->flags);
  if(result == TRUE)
  {
    DO_NOTIFY(NTF_Active | NTF_L_Active);
  }

  RETURN(result);
  return result;
}

IPTR mNL_List_Select(struct IClass *cl,Object *obj,struct MUIP_NList_Select *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/
  if (data->NList_TypeSelect)
    return (NL_List_SelectChar(data,msg->pos,msg->seltype,msg->state));
  else
    return (NL_List_Select(data,msg->pos,msg->pos,msg->seltype,msg->state));
  return (0);
}


IPTR mNL_List_TestPos(struct IClass *cl,Object *obj,struct MUIP_NList_TestPos *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/
  return (NL_List_TestPos(data,msg->x,msg->y,msg->res));
}


IPTR mNL_List_TestPosOld(struct IClass *cl,Object *obj,struct MUIP_List_TestPos *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/
  return (NL_List_TestPosOld(data,msg->x,msg->y,msg->res));
}


IPTR mNL_List_Redraw(struct IClass *cl,Object *obj,struct MUIP_NList_Redraw *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  long ent;

  ENTER();

  /*DoSuperMethodA(cl,obj,(Msg) msg);*/
  ent = msg->pos;
  data->display_ptr = NULL;
  data->parse_column = -1;

  /*D(bug("%lx|List_Redraw1 pos=%ld Q=%ld D=%ld S=%ld\n",obj,msg->pos,data->NList_Quiet,data->DRAW,data->SETUP));*/

  if(data->DRAW > 1)
  {
    // defer the redraw process
    /*D(bug("%lx|List_Redraw no! %ld %ld (push it)\n",obj,msg->pos,data->DRAW));*/
    DoMethod(_app(obj), MUIM_Application_PushMethod, obj, 2, msg->MethodID, msg->pos);
  }
  else
  {
    if(msg->MethodID == MUIM_List_Redraw && msg->pos == MUIV_NList_Redraw_All)
      msg->pos = -6;

    switch(msg->pos)
    {
      case -6:
      {
        if(data->DRAW)
        {
          NL_SetColsAdd(data,-2,TRUE);
          DoMethod(_app(obj), MUIM_Application_PushMethod, obj, 2, MUIM_NList_Redraw, MUIV_NList_Redraw_All);
        }
      }
      break;

      case MUIV_NList_Redraw_All:
      {
        // redraw all entries
        if(data->DRAW)
        {
          NL_SetColsAdd(data,-2,TRUE);
          for(ent = 0; ent < data->NList_Entries; ent++)
            data->EntriesArray[ent]->PixLen = -1;
        }
        data->Title_PixLen = -1;
        data->do_draw_title = TRUE;
        data->do_parse = TRUE;
        data->do_setcols = TRUE;
        data->do_updatesb = TRUE;
        data->force_wwrap = TRUE;
        REDRAW_ALL;
      }
      break;

      case MUIV_NList_Redraw_VisibleCols:
      {
        // redraw visible columns only
        if(data->DRAW)
          NL_SetColsAdd(data, -3, TRUE);
      }
      break;

      case MUIV_NList_Redraw_Selected:
      {
        // redraw selected entries only
        if(data->DRAW)
        {
          BOOL doDraw = FALSE;

          if(data->NList_TypeSelect == MUIV_NList_TypeSelect_Line)
          {
            for(ent=0; ent < data->NList_Entries; ent++)
            {
              // mark the selected entries as "to be redrawn"
              if(data->EntriesArray[ent]->Select != TE_Select_None)
              {
                NL_SetColsAdd(data, ent, TRUE);
                data->EntriesArray[ent]->PixLen = -1;
                NL_Changed(data, ent);
                doDraw = TRUE;
              }
            }
          }
          else
          {
            NL_SegChanged(data,data->sel_pt[data->min_sel].ent,data->sel_pt[data->max_sel].ent);
            doDraw = TRUE;
          }

          if(doDraw == TRUE)
          {
            // at least one entry must be redrawn
            data->display_ptr = NULL;
            data->do_draw = TRUE;
            REDRAW;
          }
        }
      }
      break;

      case MUIV_NList_Redraw_Title:
      {
        // redraw title only
        if(data->DRAW)
        {
          NL_SetColsAdd(data, -1, TRUE);
          data->Title_PixLen = -1;
        }
        data->do_draw_title = TRUE;
        data->do_setcols = TRUE;
        REDRAW;
      }
      break;

      case MUIV_NList_Redraw_Active:
      {
        // redraw the active entry only
        ent = data->NList_Active;
      }
      // fall through to the default redraw

      default:
      {
        // redraw a specific entry
        if(ent >= 0 && ent < data->NList_Entries)
        {
          if(data->DRAW)
          {
            NL_SetColsAdd(data, ent, TRUE);
            data->EntriesArray[ent]->PixLen = -1;
          }
          NL_Changed(data, ent);
          REDRAW;
        }
      }
      break;
    }

    /*D(bug("%lx|List_Redraw2 pos=%ld Q=%ld D=%ld S=%ld\n",obj,msg->pos,data->NList_Quiet,data->DRAW,data->SETUP));*/
  }

  RETURN(TRUE);
  return TRUE;
}


IPTR mNL_List_RedrawEntry(struct IClass *cl,Object *obj,struct MUIP_NList_RedrawEntry *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  LONG ent = 0;
  BOOL dodraw = FALSE;
  if (!msg->entry)
    return (FALSE);
  while (ent < data->NList_Entries)
  { if (data->EntriesArray[ent]->Entry == msg->entry)
    { data->EntriesArray[ent]->PixLen = -1;
      NL_Changed(data,ent);
      dodraw = TRUE;
    }
    ent++;
  }
  if (dodraw)
  {
    /* sba: This enforces redrawing the entry completly */
    data->display_ptr = NULL;

    data->do_draw = TRUE;
    REDRAW;

    return (TRUE);
  }
  return (FALSE);
}


IPTR mNL_List_NextSelected(struct IClass *cl,Object *obj,struct MUIP_NList_NextSelected *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  long ent = *msg->pos;
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/
  if ((ent == MUIV_NList_NextSelected_Start) || (ent < 0))
    ent = 0;
  else
    ent++;
  if (data->NList_TypeSelect)
  {
    if (ent < data->sel_pt[data->min_sel].ent)
      ent = data->sel_pt[data->min_sel].ent;
    else if (ent > data->sel_pt[data->max_sel].ent)
      ent = MUIV_NList_NextSelected_End;
    else if ((ent == data->sel_pt[data->max_sel].ent) &&
             (data->sel_pt[data->max_sel].column == 0) &&
             (data->sel_pt[data->max_sel].xoffset == PMIN))
      ent = MUIV_NList_NextSelected_End;
  }
  else
  { while ((ent < data->NList_Entries) && (data->EntriesArray[ent]->Select == TE_Select_None))
      ent++;
    if (ent >= data->NList_Entries)
      ent = MUIV_NList_NextSelected_End;
  }
  *msg->pos = ent;
  return (TRUE);
}


IPTR mNL_List_PrevSelected(struct IClass *cl,Object *obj,struct MUIP_NList_PrevSelected *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  long ent = *msg->pos;
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/
  if ((ent == MUIV_NList_PrevSelected_Start) || (ent < 0) || (ent >= data->NList_Entries))
    ent = data->NList_Entries - 1;
  else
    ent--;
  if (data->NList_TypeSelect)
  {
    if (ent < data->sel_pt[data->min_sel].ent)
      ent = MUIV_NList_PrevSelected_End;
    else if ((ent == data->sel_pt[data->max_sel].ent) &&
             (data->sel_pt[data->max_sel].column == 0) &&
             (data->sel_pt[data->max_sel].xoffset == PMIN))
      ent = data->sel_pt[data->max_sel].ent - 1;
    else if (ent > data->sel_pt[data->max_sel].ent)
      ent = data->sel_pt[data->max_sel].ent;
  }
  else
  { while ((ent > 0) && (data->EntriesArray[ent]->Select == TE_Select_None))
      ent--;
    if (ent < 0)
      ent = MUIV_NList_PrevSelected_End;
  }
  *msg->pos = ent;
  return (TRUE);
}


IPTR mNL_List_GetSelectInfo(struct IClass *cl,Object *obj,struct MUIP_NList_GetSelectInfo *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  LONG ent;

  msg->res->start = 1;
  msg->res->vstart = -1;
  msg->res->end = 1;
  msg->res->vend = -1;
  msg->res->num = -1;
  msg->res->vnum = 0;
  msg->res->start_column = -1;
  msg->res->end_column = -1;
  msg->res->start_pos= -1;
  msg->res->end_pos = -1;

  if(data->NList_TypeSelect == MUIV_NList_TypeSelect_Line)
  {
    ent = 0;

    while(ent < data->NList_Entries)
    {
      if(data->EntriesArray[ent] != NULL && data->EntriesArray[ent]->Select != TE_Select_None)
      {
        if(msg->res->start == -1)
          msg->res->start = ent;

        msg->res->end = ent;
        msg->res->vnum++;
      }
      ent++;
    }
    msg->res->vstart = msg->res->start;
    msg->res->vend = msg->res->end;
  }
  else
  {
    msg->res->start_column = data->sel_pt[data->min_sel].column;
    msg->res->start_pos = data->sel_pt[data->min_sel].colpos;
    msg->res->end_column = data->sel_pt[data->max_sel].column;
    msg->res->end_pos = data->sel_pt[data->max_sel].colpos;
    msg->res->start = data->sel_pt[data->min_sel].ent;
    msg->res->end = data->sel_pt[data->max_sel].ent;
    msg->res->vstart = msg->res->start;
    msg->res->vend = msg->res->end;

    if((msg->res->vstart >= 0) && (msg->res->vend >= msg->res->vstart))
      msg->res->vnum = msg->res->vend - msg->res->vstart + 1;
  }

  if (msg->res->start >= 0 && msg->res->start < data->NList_Entries && data->EntriesArray[msg->res->start] != NULL && data->EntriesArray[msg->res->start]->Wrap)
  {
    if (data->EntriesArray[msg->res->start]->Wrap & TE_Wrap_TmpLine)
      msg->res->start -= data->EntriesArray[msg->res->start]->dnum;
  }

  if (msg->res->end >= 0 && msg->res->end < data->NList_Entries && data->EntriesArray[msg->res->end] != NULL && data->EntriesArray[msg->res->end]->Wrap)
  {
    if (data->EntriesArray[msg->res->end]->Wrap & TE_Wrap_TmpLine)
      msg->res->end -= data->EntriesArray[msg->res->end]->dnum;
  }

  return (TRUE);
}


IPTR mNL_List_DoMethod(struct IClass *cl,Object *obj,struct MUIP_NList_DoMethod *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  LONG ent = msg->pos;
  APTR dest = msg->DestObj;

  if((msg->FollowParams >= 1) && (msg->FollowParams < 40) &&
     ((msg->pos == MUIV_NList_DoMethod_Active) ||
      (msg->pos == MUIV_NList_DoMethod_Selected) ||
      (msg->pos == MUIV_NList_DoMethod_All) ||
      ((msg->pos >= 0) && (msg->pos < data->NList_Entries))))
  {
    LONG *table1 = (LONG *)&msg->FollowParams;
    struct
    {
	  STACKED ULONG MethodID;
	  SIPTR params[64]; /* MAXIMUM 40 (see docs) <aphaso> */
    } newMsg;

    if(msg->FollowParams > 63) /* MAXIMUM 40 (see docs) <aphaso> */
    {
      msg->FollowParams = 63;
      newMsg.params[63] = 0;
    }

    if((IPTR)msg->DestObj == (IPTR)MUIV_NList_DoMethod_Self)
      dest = (APTR) obj;
    else if((IPTR)msg->DestObj == (IPTR)MUIV_NList_DoMethod_App)
    {
      if(data->SETUP)
        dest = (APTR) _app(obj);
      else
        dest = NULL;
    }
    if(msg->pos == MUIV_NList_DoMethod_Active)
      ent = MUIV_NList_DoMethod_Active;
    else if(msg->pos == MUIV_NList_DoMethod_Selected)
    {
      if(data->NList_TypeSelect)
        ent = data->sel_pt[data->min_sel].ent;
      else
      {
        ent = 0;
        while((ent < data->NList_Entries) && (data->EntriesArray[ent]->Select == TE_Select_None))
          ent++;
      }
    }
    else if(msg->pos == MUIV_NList_DoMethod_All)
      ent = 0;
    while((ent >= 0) && (ent < data->NList_Entries))
    {
      if((IPTR) msg->DestObj == (IPTR)MUIV_NList_DoMethod_Entry)
        dest = data->EntriesArray[ent]->Entry;
      if(dest)
      {
        ULONG num;

        newMsg.MethodID = table1[1];

        for(num = 1;num < msg->FollowParams;num++)
        {
          switch(table1[num+1])
          {
            case MUIV_NList_EntryValue:
              newMsg.params[num] = (SIPTR)data->EntriesArray[ent]->Entry;
              break;
            case MUIV_NList_EntryPosValue:
              newMsg.params[num] = (LONG)ent;
              break;
            case MUIV_NList_SelfValue:
              newMsg.params[num] = (SIPTR)obj;
              break;
            case MUIV_NList_AppValue:
              if (data->SETUP)
                newMsg.params[num] = (SIPTR)_app(obj);
              else
                newMsg.params[num] = 0;
              break;
            default:
              newMsg.params[num] = table1[num+1];
              break;
          }
        }

        DoMethodA(dest, (Msg)((APTR)&newMsg));
      }
      ent++;
      if(msg->pos == MUIV_NList_DoMethod_Selected)
      {
        if(data->NList_TypeSelect)
        {
          if((ent > data->sel_pt[data->max_sel].ent) ||
             ((ent == data->sel_pt[data->max_sel].ent) &&
              (data->sel_pt[data->max_sel].column == 0) &&
              (data->sel_pt[data->max_sel].xoffset == PMIN)))
            break;
        }
        else
        {
          while((ent < data->NList_Entries) && (data->EntriesArray[ent]->Select == TE_Select_None))
            ent++;
        }
      }
      else if(msg->pos != MUIV_NList_DoMethod_All)
        break;
    }
  }
  return (TRUE);
}

static BOOL NL_List_GetPos(struct NLData *data, APTR entry, LONG *pos)
{
  BOOL result = FALSE;

  ENTER();

  if(entry != NULL)
  {
    if((IPTR)entry == (IPTR)-2)
    {
      if(data->NList_LastInserted == -1)
        *pos = (data->NList_Entries - 1);
      else
        *pos = data->NList_LastInserted;

      result = TRUE;
    }
    else
    {
      LONG ent = *pos + 1;

      while(ent < data->NList_Entries)
      {
        if(data->EntriesArray[ent]->Entry == entry)
        {
          *pos = ent;

          result = TRUE;
          break;
        }
        ent++;
      }
    }
  }

  if(result == FALSE)
  {
    *pos = MUIV_NList_GetPos_End;
  }

  RETURN(result);
  return result;
}

IPTR mNL_List_GetPos(struct IClass *cl,Object *obj,struct MUIP_NList_GetPos *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  BOOL result;

  result = NL_List_GetPos(data, msg->entry, msg->pos);

  return result;
}
