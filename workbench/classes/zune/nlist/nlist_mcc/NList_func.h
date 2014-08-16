/***************************************************************************

 NList.mcc - New List MUI Custom Class
 Registered MUI class, Serial Number: 1d51 0x9d510030 to 0x9d5100A0
                                           0x9d5100C0 to 0x9d5100FF

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2014 NList Open Source Team

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

#define PMIN -30000
#define PMAX 30000

#define NTF_SB              ((LONG) (1<<0))
#define NTF_VSB             ((LONG) (1<<1))
#define NTF_HSB             ((LONG) (1<<2))
#define NTF_First           ((LONG) (1<<3))
#define NTF_Entries         ((LONG) (1<<4))
#define NTF_Active          ((LONG) (1<<5))
#define NTF_L_Active        ((LONG) (1<<6))
#define NTF_Select          ((LONG) (1<<7))
#define NTF_LV_Select       ((LONG) (1<<8))
#define NTF_Multiclick      ((LONG) (1<<9))
#define NTF_Doubleclick     ((LONG) (1<<10))
#define NTF_LV_Doubleclick  ((LONG) (1<<11))
#define NTF_TitleClick      ((LONG) (1<<12))
#define NTF_EntryClick      ((LONG) (1<<13))
#define NTF_MinMax          ((LONG) (1<<14))
#define NTF_MinMaxNoDraw    ((LONG) (1<<15))
#define NTF_ButtonClick     ((LONG) (1<<16))
#define NTF_LineHeight      ((LONG) (1<<17))
#define NTF_Insert          ((LONG) (1<<18))
#define NTF_DragSortInsert  ((LONG) (1<<19))
#define NTF_Columns         ((LONG) (1<<20))
#define NTF_MulticlickAlone ((LONG) (1<<21))
#define NTF_TitleClick2     ((LONG) (1<<22))

#define NTF_DragWait        ((LONG) (1<<31))

#define NTF_AllChanges      (NTF_First|NTF_Entries|NTF_Active|NTF_L_Active|NTF_Select|NTF_LV_Select|NTF_LineHeight|NTF_Insert|NTF_DragSortInsert|NTF_Columns)
#define NTF_AllClick        (NTF_Multiclick|NTF_MulticlickAlone|NTF_Doubleclick|NTF_LV_Doubleclick|NTF_TitleClick|NTF_TitleClick2|NTF_EntryClick|NTF_ButtonClick)
#define NTF_All             (NTF_AllChanges|NTF_AllClick)

#define NTF_AlwaysDoNotify  (NTF_SB|NTF_VSB|NTF_HSB|NTF_MinMax|NTF_MinMaxNoDraw|NTF_LineHeight|NTF_DragSortInsert)

#define WANT_NOTIFY(ntf)    data->Notify |= (ntf)
#define NOWANT_NOTIFY(ntf)  data->Notify &= ~(ntf)
#define WANTED_NOTIFY(ntf)  (data->Notify & (ntf))
#define DO_NOTIFY(ntf)      if(data->NList_Quiet < 900) data->DoNotify |= (ntf); else data->DoNotify |= ((ntf) & NTF_AlwaysDoNotify)
#define ASKED_NOTIFY(ntf)   (data->DoNotify & (ntf))
#define NEED_NOTIFY(ntf)    (data->Notify & data->DoNotify & (ntf))
#define DONE_NOTIFY(ntf)    data->DoNotify &= ~(ntf)
#define NOTIFY_START(ntf)   data->Notifying |= (ntf)
#define NOTIFYING(ntf)      (data->Notifying & (ntf))
#define NOTIFY_END(ntf)     data->Notifying &= ~(ntf)

/* mad_Flags |= 0x00004000 to make it think it's visible even if in a virtgroup */

#define REDRAW_IF                 NL_DrawQuietBG(data,0,0)
#define REDRAW_ALL_FORCE          NL_DrawQuietBG(data,1,0)
#define REDRAW_ALL                NL_DrawQuietBG(data,2,0)
#define REDRAW_FORCE              NL_DrawQuietBG(data,3,0)
#define REDRAW                    NL_DrawQuietBG(data,4,0)

#define MOREQUIET                 data->NList_Quiet++
#define FULLQUIET                 data->NList_Quiet |= 0x1000
#define FULLQUIET_END             data->NList_Quiet &= 0x0FFF
#define VISUALQUIET               data->NList_Quiet = (data->NList_Quiet & 0x0FFF) + 1
#define ENDQUIET                  NL_DrawQuietBG(data,5,0)
#define LESSQUIET                 NL_DrawQuietBG(data,6,0)

#define SetBackGround(bg)         if((bg) != (ULONG)data->actbackground) NL_DrawQuietBG(data, 7, (bg));

#define Make_Active_Visible       NL_DrawQuietBG(data,8,0)

#define ForceMinMax               NL_DrawQuietBG(data,9,0)

#define do_notifies(which)        NL_DoNotifies(data,(which))


#define SELECT(ent,sel)           NL_Select(data,0,(ent),(sel))
#define SELECT2(ent,sel)          NL_Select(data,1,(ent),(sel))
#define SELECT_CHGE(ent,sel)      NL_Select(data,2,(ent),(sel))
#define SELECT2_CHGE(ent,sel)     NL_Select(data,3,(ent),(sel))
#define set_Active(new_act)       NL_Select(data,4,(new_act),0)




#define ReSetFont                 if(data->rp->Font!=data->font) SetFont(data->rp,data->font);

#define notdoset(obj,attr,value)  SetAttrs(obj,MUIA_Group_Forward,FALSE,MUIA_NList_Visible,0,attr,value,TAG_DONE)


#define STYLE_MASK             (FSF_UNDERLINED|FSF_BOLD|FSF_ITALIC)
#define SET_STYLE_NORMAL(s)    ((s) = (s) & ~STYLE_MASK)
#define SET_STYLE_UNDERLINE(s) ((s) = (s) | FSF_UNDERLINED)
#define SET_STYLE_BOLD(s)      ((s) = (s) | FSF_BOLD)
#define SET_STYLE_ITALIC(s)    ((s) = (s) | FSF_ITALIC)
#define IS_STYLE_NORMAL(s)     (((s) & STYLE_MASK) == FS_NORMAL)
#define IS_STYLE_UNDERLINE(s)  ((s) & FSF_UNDERLINED)
#define IS_STYLE_BOLD(s)       ((s) & FSF_BOLD)
#define IS_STYLE_ITALIC(s)     ((s) & FSF_ITALIC)
#define GET_STYLE(s)           ((s) & STYLE_MASK)

#define STYLE_STRMASK   (0x00F0)
#define STYLE_IMAGE     (0x0010)
#define STYLE_IMAGE2    (0x0020)
#define STYLE_TAB       (0x0040)
#define STYLE_SPACE     (0x0050)
#define STYLE_FIXSPACE  (0x0060)

#define ALIGN_MASK      (0x0700)
/* already defined in NList_mcc.h
#define ALIGN_LEFT      (0x0000)
#define ALIGN_CENTER    (0x0100)
#define ALIGN_RIGHT     (0x0200)
#define ALIGN_JUSTIFY   (0x0400)
*/
#define SET_ALIGN_LEFT(a)      ((a) = ((a) & ~ALIGN_MASK) | ALIGN_LEFT)
#define SET_ALIGN_CENTER(a)    ((a) = ((a) & ~ALIGN_MASK) | ALIGN_CENTER)
#define SET_ALIGN_RIGHT(a)     ((a) = ((a) & ~ALIGN_MASK) | ALIGN_RIGHT)
#define SET_ALIGN_JUSTIFY(a)   ((a) = ((a) & ~ALIGN_MASK) | ALIGN_JUSTIFY)
#define IS_ALIGN_LEFT(a)       (((a) & ALIGN_MASK) == ALIGN_LEFT)
#define IS_ALIGN_CENTER(a)     (((a) & ALIGN_MASK) == ALIGN_CENTER)
#define IS_ALIGN_RIGHT(a)      (((a) & ALIGN_MASK) == ALIGN_RIGHT)
#define IS_ALIGN_JUSTIFY(a)    (((a) & ALIGN_MASK) == ALIGN_JUSTIFY)

#define HLINE_MASK      (0x7000)
#define HLINE_T         (0x1000)
#define HLINE_C         (0x2000)
#define HLINE_B         (0x3000)
#define HLINE_E         (0x4000)
#define SET_HLINE_T(a)  ((a) = ((a) & ~HLINE_MASK) | HLINE_T)
#define SET_HLINE_C(a)  ((a) = ((a) & ~HLINE_MASK) | HLINE_C)
#define SET_HLINE_B(a)  ((a) = ((a) & ~HLINE_MASK) | HLINE_B)
#define SET_HLINE_E(a)  ((a) = ((a) & ~HLINE_MASK) | HLINE_E)
#define IS_HLINE(a)     ((a) & HLINE_MASK)
#define IS_HLINE_T(a)   (((a) & HLINE_MASK) == HLINE_T)
#define IS_HLINE_C(a)   (((a) & HLINE_MASK) == HLINE_C)
#define IS_HLINE_B(a)   (((a) & HLINE_MASK) == HLINE_B)
#define IS_HLINE_E(a)   (((a) & HLINE_MASK) == HLINE_E)

#define HLINE_thick_MASK      (0x8800)
#define HLINE_thick           (0x8000)
#define HLINE_nothick         (0x0800)
#define SET_HLINE_thick(a)    ((a) = ((a) & ~HLINE_thick_MASK) | HLINE_thick)
#define SET_HLINE_nothick(a)  ((a) = ((a) & ~HLINE_thick_MASK) | HLINE_nothick)
#define IS_HLINE_thick(a)     (((a) & HLINE_thick_MASK) == HLINE_thick)
#define IS_HLINE_nothick(a)   (((a) & HLINE_thick_MASK) == HLINE_nothick)

#define FIXPEN_MASK     (0x0008)
#define SET_FIXPEN(a)   ((a) |= FIXPEN_MASK)
#define SET_NOFIXPEN(a) ((a) &= ~FIXPEN_MASK)
#define IS_FIXPEN(a)    ((a) & FIXPEN_MASK)
/*data->cols[column].style*/


#if !defined(__amigaos4__) && !defined(__MORPHOS__) && !defined(__AROS__)
// AllocVecPooled.c
APTR AllocVecPooled(APTR, ULONG);
// FreeVecPooled.c
void FreeVecPooled(APTR, APTR);
#endif

#if defined(__amigaos4__)
#define AllocTypeEntry()      ItemPoolAlloc(data->EntryPool)
#define FreeTypeEntry(entry)  ItemPoolFree(data->EntryPool, entry)
#else
#define AllocTypeEntry()      AllocPooled(data->EntryPool, sizeof(struct TypeEntry))
#define FreeTypeEntry(entry)  FreePooled(data->EntryPool, entry, sizeof(struct TypeEntry))
#endif

#define IS_BAR(c,ci) \
    (((c) < data->numcols-1) ||\
     (((c) > 0) &&\
      (((ci)->width < 0) || ((ci)->width_type != CI_PERCENT) || ((ci)->userwidth > 0))))

