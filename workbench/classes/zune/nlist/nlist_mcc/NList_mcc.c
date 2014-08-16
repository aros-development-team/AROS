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

#include <string.h>
#include <stdlib.h>

#include <dos/dosextens.h>
#include <clib/alib_protos.h>

#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif

#include <proto/muimaster.h>
#include <proto/diskfont.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include "private.h"

#include "NList_func.h"
#include "NList_grp.h"

#include "NListviews_mcp.h"

/* IO macros */
/*
 *#define IO_SIGBIT(req)  ((LONG)(((struct IORequest *)req)->io_Message.mn_ReplyPort->mp_SigBit))
 *#define IO_SIGMASK(req) ((LONG)(1L<<IO_SIGBIT(req)))
 */


DEFAULT_KEYS_ARRAY

#define INIT_PEN(attr,var_dest,test_init) \
  { \
    var_dest = -1; \
    if((tag = FindTagItem(attr, msg->ops_AttrList))) \
    { \
      test_init = tag->ti_Data; \
    } \
    else \
      test_init = FALSE; \
  }

#define INIT_BG(attr,var_dest,test_init) \
  { \
    var_dest = -1; \
    if((tag = FindTagItem(attr, msg->ops_AttrList))) \
    { \
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
      test_init = TRUE; \
    } \
    else \
      test_init = FALSE; \
  }


#define LOAD_PEN(test,var_dest,cfg_attr,defaultval) \
  { \
    if (!test) \
    { \
      IPTR ptrd; \
      if (DoMethod(obj, MUIM_GetConfigItem, cfg_attr, &ptrd)) \
        obtain_pen(obj, &(var_dest), (struct MUI_PenSpec *)ptrd); \
      else \
        obtain_pen(obj, &(var_dest), (struct MUI_PenSpec *)(defaultval)); \
    } \
    else \
      obtain_pen(obj, &(var_dest), (struct MUI_PenSpec *)(test)); \
  }

#define LOAD_BG(test,var_dest,cfg_attr,defaultval) \
  { \
    if (!test) \
    { \
      IPTR ptrd; \
      if (DoMethod(obj, MUIM_GetConfigItem, cfg_attr, &ptrd)) \
      { \
        if(ptrd <= 0x00000100) \
        { \
          var_dest = ptrd; \
        } \
        else \
        { \
          strlcpy(var_dest##Buffer, (STRPTR)ptrd, sizeof(var_dest##Buffer)); \
          var_dest = (IPTR)var_dest##Buffer; \
        } \
      } \
      else \
        var_dest = (IPTR)(defaultval); \
    } \
  }


HOOKPROTONH(NL_ConstructFunc_String, APTR, APTR pool, char *str)
{
  char *new;
  LONG len = 0;

  while(str[len] != '\0' && str[len] != '\n' && str[len] != '\r' )
  {
    len++;
  }

  if((new = (char *)AllocVecPooled(pool, len+1)) != NULL)
  {
    memcpy(new, str, len*sizeof(char));
    new[len] = '\0'; // we have to terminate with a \0
  }

  return((APTR)new);
}
MakeHook(NL_ConstructHook_String, NL_ConstructFunc_String);

HOOKPROTONH(NL_DestructFunc_String, void, APTR pool, char *entry)
{
  FreeVecPooled(pool, entry);
}
MakeHook(NL_DestructHook_String, NL_DestructFunc_String);

HOOKPROTONHNO(NL_LayoutFuncNList, ULONG, struct MUI_LayoutMsg *lm)
{
  switch (lm->lm_Type)
  {
    case MUILM_MINMAX:
    {
      lm->lm_MinMax.MinWidth  = 1;
      lm->lm_MinMax.DefWidth  = 1;
      lm->lm_MinMax.MaxWidth  = 1;
      lm->lm_MinMax.MinHeight = 1;
      lm->lm_MinMax.DefHeight = 1;
      lm->lm_MinMax.MaxHeight = 1;

      return(0);
      //return (MUILM_UNKNOWN);
    }
    break;

    case MUILM_LAYOUT:
    {
      Object *cstate = (Object *)lm->lm_Children->mlh_Head;
      Object *child;
      LONG mw,mh;

      while((child = NextObject(&cstate)))
      {
        mw = (LONG) _defwidth(child);
        mh = (LONG) _defheight(child);

        if(!MUI_Layout(child,lm->lm_Layout.Width+MUI_MAXMAX,lm->lm_Layout.Height+MUI_MAXMAX,mw,mh,0))
        {
          /*return(FALSE);*/
        }
      }

      return(TRUE);
    }
    break;
  }

  return(MUILM_UNKNOWN);
}
MakeStaticHook(NL_LayoutHookNList, NL_LayoutFuncNList);

HOOKPROTONHNO(NL_LayoutFuncGroup, ULONG, struct MUI_LayoutMsg *lm)
{
  switch (lm->lm_Type)
  {
    case MUILM_MINMAX:
    {
      lm->lm_MinMax.MinWidth  = 2;
      lm->lm_MinMax.DefWidth  = 20;
      lm->lm_MinMax.MaxWidth  = MUI_MAXMAX+100;
      lm->lm_MinMax.MinHeight = 2;
      lm->lm_MinMax.DefHeight = 20+100;
      lm->lm_MinMax.MaxHeight = MUI_MAXMAX+100;

      return(0);
      //return (MUILM_UNKNOWN);
    }
    break;

    case MUILM_LAYOUT:
    {
      Object *cstate = (Object *)lm->lm_Children->mlh_Head;
      Object *child;
      LONG mw,mh;

      while((child = NextObject(&cstate)))
      {
        mw = (LONG) _defwidth(child);
        mh = (LONG) _defheight(child);

        if (!MUI_Layout(child,0,0,mw,mh,0))
        {
          /*return(FALSE);*/
        }
      }
      return(TRUE);
    }
    break;
  }

  return(MUILM_UNKNOWN);
}
MakeStaticHook(NL_LayoutHookGroup, NL_LayoutFuncGroup);


void release_pen(Object *obj, IPTR *pen)
{
  if((ULONG)*pen != (ULONG)-1)
  {
    MUI_ReleasePen(muiRenderInfo(obj), *pen);
    *pen = (ULONG)-1;
  }
}


void obtain_pen(Object *obj, IPTR *pen, struct MUI_PenSpec *ps)
{
  release_pen(obj, pen);
  *pen = MUI_ObtainPen(muiRenderInfo(obj), ps, 0);
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

IPTR mNL_New(struct IClass *cl,Object *obj,struct opSet *msg)
{
    struct NLData *data;
  //$$$$Sensei: msg->ops_AttrList is changed to taglist EVERYWHERE in OM_NEW!!!
  // It should speed up a bit.
    struct TagItem    *tag;
    struct TagItem    *taglist    = msg->ops_AttrList;
  LONG DragType = MUIV_NList_DragType_None;
  LONG DragSortable = FALSE; /*FALSE*/
  LONG Draggable = FALSE;    /*FALSE*/
  LONG Dropable = TRUE;      /*TRUE*/
  LONG dropable = TRUE;      /*TRUE*/
  STRPTR ShortHelp = NULL;   // RHP: Added for Special ShortHelp
  APTR img_tr,grp;
  const char *img_name = "noimage";

  if((tag = FindTagItem(MUIA_ShortHelp, taglist)))
    ShortHelp = (STRPTR)tag->ti_Data;

  if((tag = FindTagItem(MUIA_FixHeightTxt, taglist)))
    tag->ti_Tag = TAG_IGNORE;

  if((tag = FindTagItem(MUIA_FixWidthTxt, taglist)))
    tag->ti_Tag = TAG_IGNORE;

  if((tag = FindTagItem(MUIA_Draggable, taglist)))
  {
    Draggable = tag->ti_Data;
    tag->ti_Tag = TAG_IGNORE;
  }

  if((tag = FindTagItem(MUIA_Dropable, taglist)))
  {
    Dropable = dropable = tag->ti_Data;
    tag->ti_Tag = TAG_IGNORE;
  }

  if((tag = FindTagItem(MUIA_NList_DragSortable, taglist)) ||
     (tag = FindTagItem(MUIA_List_DragSortable, taglist)))
  {
    DragSortable = tag->ti_Data;
    tag->ti_Tag = TAG_IGNORE;
  }

  if((tag = FindTagItem(MUIA_NList_DragType, taglist)) ||
     (tag = FindTagItem(MUIA_Listview_DragType, taglist)))
  {
    DragType = tag->ti_Data;
    tag->ti_Tag = TAG_IGNORE;
  }

  if((DragType != MUIV_NList_DragType_None) || DragSortable)
    Draggable = TRUE;
  else if (Draggable)
    DragType = MUIV_NList_DragType_Default;
  else
  {
    DragType = MUIV_NList_DragType_None;
    DragSortable = FALSE;
  }

  if(DragSortable)
    dropable = TRUE;

  obj = (Object *) DoSuperNew(cl,obj,
    MUIA_Group_LayoutHook, &NL_LayoutHookNList,
    MUIA_FillArea, FALSE,
    MUIA_Dropable, dropable,
    NoFrame,
      Child, grp = NewObject(NGR_Class->mcc_Class,NULL,
        MUIA_Group_LayoutHook, &NL_LayoutHookGroup,
        MUIA_FillArea, FALSE,
        NoFrame,
        Child, img_tr = MUI_NewObject(MUIC_Image,
          MUIA_FillArea,FALSE,
          MUIA_Image_Spec,img_name,
/*
 *         MUIA_Image_FontMatch, TRUE,
 *         MUIA_Font, Topaz_8,
 *         MUIA_Image_State, IDS_NORMAL,
 */
        End,
        /*Child, HVSpace,*/
      End,
    TAG_MORE, taglist
  );


  if(obj == NULL || (data = INST_DATA(cl, obj)) == NULL)
    return(0);

  data->this = obj;
  data->nlistviewobj = NULL;
  data->listviewobj = NULL;
  data->scrollersobj = NULL;
  data->SETUP = FALSE;
  data->SHOW = FALSE;
  data->DRAW = 0;
  data->ncl = cl;
  data->ocl = OCLASS(obj);
  data->rp = NULL;
  data->NL_Group = grp;

  data->NList_Title = NULL;
  data->NList_TitleSeparator = TRUE;
  data->NList_TitleMark = MUIV_NList_TitleMark_None;
  data->NList_TitleMark2 = MUIV_NList_TitleMark2_None;
  data->NList_IgnoreSpecialChars = NULL;
  data->NList_LastInserted = -1;
  data->NList_Quiet = 0;
  data->NList_AffActive = MUIV_NList_Active_Off;
  data->NList_Active = MUIV_NList_Active_Off;
  data->NList_Smooth = DEFAULT_SMOOTHSCROLL;
  data->VertPropObject = NULL;
  data->NList_AffFirst = 0;
  data->NList_AffFirst_Incr = 0;
  data->NList_First = 0;
  data->NList_First_Incr = 0;
  data->NList_Visible = 50000;
  data->NList_Entries = 0;
  data->NList_Prop_First = 0;
  data->NList_Prop_First_Prec = 0;
  data->NList_Prop_First_Real = 0;
  data->NList_Prop_Add = 0;
  data->NList_Prop_Wait = 2;
  data->NList_Prop_Visible = 0;
  data->NList_Prop_Entries = 0;
  data->NList_Horiz_AffFirst = 0;
  data->NList_Horiz_First = 0;
  data->NList_Horiz_Visible = 0;
  data->NList_Horiz_Entries = 0;
  data->old_prop_visible = -1;
  data->old_prop_entries = -1;
  data->old_prop_delta = -1;
  data->old_horiz_visible = -1;
  data->old_horiz_entries = -1;
  data->old_horiz_delta = -1;
  data->NList_MultiSelect = MUIV_NList_MultiSelect_None;
  data->NList_Input = TRUE;
  data->NList_DefaultObjectOnClick = TRUE;
  data->NList_ActiveObjectOnClick = FALSE;
  data->NList_KeepActive = 0;
  data->NList_MakeActive = 0;
  data->NList_AutoVisible = FALSE;
  data->NList_Font = 0;
  data->NList_DragType = DragType;
  data->NList_Dropable = Dropable;
  data->NList_DragColOnly = -1;
  data->NList_DragSortable = DragSortable;
  data->NList_DropMark = -1;
  data->NList_SortType = MUIV_NList_SortType_None;
  data->NList_SortType2 = MUIV_NList_SortType_None;
  data->NList_ButtonClick = -1;
  data->NList_SerMouseFix = DEFAULT_SERMOUSEFIX;
  data->NList_Keys = default_keys;
  data->NList_ShortHelp = ShortHelp; // RHP: Added for Special ShortHelp
  data->Wheel_Keys = NULL;
  data->pushtrigger = 0;
  data->marktype = 0;
  data->NList_ShowDropMarks = TRUE;
  data->NImage2 = NULL;
  data->NImage.NImgObj = img_tr;
  data->NImage.ImgName = (char *)img_name;
  data->NImage.next = NULL;
  data->multiselect = MUIV_NList_MultiSelect_None;
  data->multisel_qualifier = 0;
  data->drag_qualifier = 0;
  data->InUseFont = NULL;
  data->badrport = FALSE;
  data->moves = FALSE;
  data->multiclick = 0;
  data->multiclickalone = 0;
  data->click_line = -2;
  data->click_x = 0;
  data->click_y = 0;
  data->NList_SelectChange = FALSE;
  data->EntriesArray = NULL;
  data->LastEntry = 0;
  data->vpos = 1;
  data->voff = 1;
  data->vinc = 1;
  data->addvinc = 1;
  data->hpos = 1;
  data->adding_member = 0;
  data->format_chge = 1;
  data->do_draw_all = TRUE;
  data->do_draw_title = TRUE;
  data->do_draw_active = TRUE;
  data->do_draw = TRUE;
  data->do_parse = TRUE;
  data->do_setcols = TRUE;
  data->do_updatesb = TRUE;
  data->do_wwrap = TRUE;
  data->force_wwrap = FALSE;
  data->do_images = TRUE;
  data->nodraw = 1;
  data->drawsuper = FALSE;
  data->dropping = FALSE;
  data->markdraw = FALSE;
  data->markerase = FALSE;
  data->markdrawnum = -1;
  data->markerasenum = -1;
  data->NumIntuiTick = 0;
  data->sorted = FALSE;
  data->selectmode = TE_Select_Line;
  data->first_change = LONG_MAX;
  data->last_change = LONG_MIN;
  data->lastselected = MUIV_NList_Active_Off;
  data->lastactived = MUIV_NList_Active_Off;
  data->selectskiped = FALSE;
  data->NList_ListBackGround = -1;
  data->actbackground = -1;
  data->NList_CompareHook = NULL;
  data->NList_ConstructHook = NULL;
  data->NList_DestructHook = NULL;
  data->NList_DisplayHook = NULL;
  data->NList_MultiTestHook = NULL;
  data->NList_CopyEntryToClipHook = NULL;
  data->NList_CopyColumnToClipHook = NULL;
  data->NList_CompareHook2 = FALSE;
  data->NList_ConstructHook2 = FALSE;
  data->NList_DestructHook2 = FALSE;
  data->NList_DisplayHook2 = FALSE;
  data->NList_CopyEntryToClipHook2 = FALSE;
  data->NList_CopyColumnToClipHook2 = FALSE;
/*
  data->NList_Pool = NULL;
  data->NList_PoolPuddleSize = 2008;
  data->NList_PoolThreshSize = 1024;
*/
  data->NList_MinLineHeight = 5;
  data->MinImageHeight = 5;
  data->NList_AdjustHeight = 0;
  data->NList_AdjustWidth = 0;
  data->NList_SourceArray = 0;
  data->NList_DefClickColumn = 0;
  data->NList_AutoCopyToClip = TRUE;
  data->NList_AutoClip = TRUE;
  data->NList_UseImages = NULL;
  data->NList_TabSize = 8;
  data->NList_SkipChars = NULL;
  data->NList_WordSelectChars = NULL;
  data->NList_EntryValueDependent = FALSE;
  data->NList_DragLines = DEFAULT_DRAGLINES;
  data->NList_WheelStep = DEFAULT_WHEELSTEP;
  data->NList_WheelFast = DEFAULT_WHEELFAST;
  data->NList_WheelMMB = DEFAULT_WHEELMMB;
  data->NList_PrivateData = NULL;
  data->NList_ContextMenu = data->ContextMenu = data->ContextMenuOn = MUIV_NList_ContextMenu_Default;
  data->ListCompatibility = FALSE;
  data->NList_Disabled = FALSE;
  data->MenuObj = NULL;
  data->LastImage = 0;
  data->DragRPort = NULL;
  data->cols = NULL;
  data->Title_PixLen = -1;
  data->numcols = 0;
  data->numcols2 = 0;
  data->column[0] = -1;
  data->aff_infos = NULL;
  data->numaff_infos = 0;
  data->spacesize = 6;
  data->tabsize = data->spacesize * data->NList_TabSize;
//  data->NList_Pool_Given = FALSE;
  data->NList_TypeSelect = MUIV_NList_TypeSelect_Line;
  data->min_sel = 1;
  data->max_sel = 1;
  data->sel_pt[0].ent = -1;
  data->sel_pt[1].ent = -1;
  data->minx_change_entry = -1;
  data->maxx_change_entry = -1;
  data->drag_border = FALSE;
  data->ScrollBarsPos = -4;
  data->ScrollBars = 0;
  data->ScrollBarsOld = 0;
  data->ScrollBarsTime = 0;
  data->display_ptr = NULL;
  data->Notify = 0;
  data->DoNotify = 0;
  data->Notifying = 0;
  data->TitleClick = -1;
  data->TitleClick2 = -1;
  data->NList_ForcePen = MUIV_NList_ForcePen_Default;
  data->ForcePen = DEFAULT_FORCEPEN;
  data->UpdatingScrollbars = FALSE;
  data->UpdateScrollersRedrawn = FALSE;
  data->drawall_bits = 0;
  data->drawall_dobit = 0;
  data->refreshing = FALSE;
  data->VirtGroup = NULL;
  data->VirtGroup2 = NULL;
  data->VirtClass = NULL;
  data->NList_ColWidthDrag = DEFAULT_CWD;
  data->NList_PartialCol = DEFAULT_PARTIALCOL;
  data->NList_PartialChar = DEFAULT_PARTIALCHAR;
  data->NList_List_Select = MUIV_NList_Select_List;
  data->NList_MinColSortable = 1;
  data->NList_Imports = MUIV_NList_Imports_Active | MUIV_NList_Imports_First | MUIV_NList_Imports_Cols;
  data->NList_Exports = MUIV_NList_Exports_Active | MUIV_NList_Exports_First | MUIV_NList_Exports_Cols;
  data->affover = -1; // RHP: Added for Shorthelp
  data->affbutton = -1;
  data->affbuttonline = -1;
  data->affbuttoncol = -1;
  data->affbuttonstate = 0;
  data->storebutton = TRUE;
  data->SizePointerObj = NULL;
  data->MovePointerObj = NULL;
  data->NList_SelectPointer = DEFAULT_SELECTPOINTER;
  data->SelectPointerObj = NULL;
  data->activeCustomPointer = PT_NONE;
  data->MOUSE_MOVE = FALSE;
  data->pad1 = -1;
  data->pad2 = TRUE;
  data->isActiveObject = FALSE;
  data->NList_KeyUpFocus = NULL;
  data->NList_KeyDownFocus = NULL;
  data->NList_KeyLeftFocus = NULL;
  data->NList_KeyRightFocus = NULL;

/*D(bug("%lx|NEW 1 \n",obj));*/

  //$$$Sensei: major rewrite memory handling. PuddleSize and ThreshSize takes memory, nothing else.
  /* User pool was specified passed? */
  if(( tag = FindTagItem( MUIA_NList_Pool, taglist ) ) ||
     ( tag = FindTagItem( MUIA_List_Pool, taglist ) ) )
  {
    data->Pool = (APTR)tag->ti_Data;
  }
  else
  {
    ULONG puddleSize = GetTagData(MUIA_NList_PoolPuddleSize, GetTagData(MUIA_List_PoolPuddleSize, MUIV_NList_PoolPuddleSize_Default, taglist), taglist);
    ULONG threshold = GetTagData(MUIA_NList_PoolThreshSize, GetTagData(MUIA_List_PoolThreshSize, MUIV_NList_PoolThreshSize_Default, taglist), taglist);

    /* Create internal pool using specified parameters or default one. */
    #if defined(__amigaos4__)
    data->Pool = AllocSysObjectTags(ASOT_MEMPOOL, ASOPOOL_MFlags, MEMF_SHARED,
                                                  ASOPOOL_Puddle, puddleSize,
                                                  ASOPOOL_Threshold, threshold,
                                                  ASOPOOL_Name, "NList.mcc pool",
                                                  ASOPOOL_LockMem, FALSE,
                                                  TAG_DONE);
    #else
    data->Pool = CreatePool(MEMF_ANY, puddleSize, threshold);
    #endif

    data->PoolInternal = data->Pool;
  }

  #if defined(__amigaos4__)
  // for OS4 we create an ItemPool
  data->EntryPool = AllocSysObjectTags(ASOT_ITEMPOOL, ASOITEM_MFlags, MEMF_SHARED,
                                                      ASOITEM_ItemSize, sizeof(struct TypeEntry),
                                                      ASOITEM_BatchSize, 1024,
                                                      ASOITEM_GCPolicy, ITEMGC_AFTERCOUNT,
                                                      TAG_DONE);
  #else
  // all other systems use a standard pool with puddle size and threshold set appropriately
  data->EntryPool = CreatePool(MEMF_ANY, sizeof(struct TypeEntry) * 1024, sizeof(struct TypeEntry) * 1024);
  #endif

  // are pools available?
  if(data->Pool == NULL || data->EntryPool == NULL)
  {
    CoerceMethod(cl, obj, OM_DISPOSE);
    return(0);
  }

  if ((tag = FindTagItem(MUIA_NList_ConstructHook, taglist)) ||
      (tag = FindTagItem(MUIA_List_ConstructHook, taglist)))
  {
    if (tag->ti_Data == (ULONG)MUIV_NList_ConstructHook_String)
      data->NList_ConstructHook = (struct Hook *) &NL_ConstructHook_String;
    else
      data->NList_ConstructHook = (struct Hook *) tag->ti_Data;
  }
  if ((tag = FindTagItem(MUIA_NList_DestructHook, taglist)) ||
      (tag = FindTagItem(MUIA_List_DestructHook, taglist)))
  {
    if (tag->ti_Data == (ULONG)MUIV_NList_DestructHook_String)
      data->NList_DestructHook = (struct Hook *) &NL_DestructHook_String;
    else
      data->NList_DestructHook = (struct Hook *) tag->ti_Data;
  }

  if((tag = FindTagItem(MUIA_NList_ListCompatibility, taglist)))
    data->ListCompatibility = TRUE;
  else
    data->ListCompatibility = FALSE;

  if((tag = FindTagItem(MUIA_Disabled, taglist)))
    data->NList_Disabled = 1;
  else
    data->NList_Disabled = FALSE;

  if((tag = FindTagItem(MUIA_NList_CompareHook, taglist)) ||
     (tag = FindTagItem(MUIA_List_CompareHook, taglist)))
  {
    data->NList_CompareHook = (struct Hook *) tag->ti_Data;
  }

  if((tag = FindTagItem(MUIA_NList_DisplayHook, taglist)) ||
     (tag = FindTagItem(MUIA_List_DisplayHook, taglist)))
  {
    data->NList_DisplayHook = (struct Hook *) tag->ti_Data;
  }

  if((tag = FindTagItem(MUIA_NList_MultiTestHook, taglist)) ||
     (tag = FindTagItem(MUIA_List_MultiTestHook, taglist)))
  {
    data->NList_MultiTestHook = (struct Hook *) tag->ti_Data;
  }

  if((tag = FindTagItem(MUIA_NList_CopyEntryToClipHook, taglist)))
    data->NList_CopyEntryToClipHook = (struct Hook *) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_CopyColumnToClipHook, taglist)))
    data->NList_CopyColumnToClipHook = (struct Hook *) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_ConstructHook2, taglist)))
  {
    data->NList_ConstructHook = (struct Hook *) tag->ti_Data;
    data->NList_ConstructHook2 = TRUE;
  }

  if((tag = FindTagItem(MUIA_NList_DestructHook2, taglist)))
  {
    data->NList_DestructHook = (struct Hook *) tag->ti_Data;
    data->NList_DestructHook2 = TRUE;
  }

  if((tag = FindTagItem(MUIA_NList_CompareHook2, taglist)))
  {
    data->NList_CompareHook = (struct Hook *) tag->ti_Data;
    data->NList_CompareHook2 = TRUE;
  }

  if((tag = FindTagItem(MUIA_NList_DisplayHook2, taglist)))
  {
    data->NList_DisplayHook = (struct Hook *) tag->ti_Data;
    data->NList_DisplayHook2 = TRUE;
  }

  if((tag = FindTagItem(MUIA_NList_CopyEntryToClipHook2, taglist)))
  {
    data->NList_CopyEntryToClipHook = (struct Hook *) tag->ti_Data;
    data->NList_CopyEntryToClipHook2 = TRUE;
  }

  if((tag = FindTagItem(MUIA_NList_CopyColumnToClipHook2, taglist)))
  {
    data->NList_CopyColumnToClipHook = (struct Hook *) tag->ti_Data;
    data->NList_CopyColumnToClipHook2 = TRUE;
  }

  if((tag = FindTagItem(MUICFG_NList_ForcePen, taglist)))
    data->NList_ForcePen = (LONG) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_Format, taglist)) ||
     (tag = FindTagItem(MUIA_List_Format, taglist)))
  {
    data->NList_Format = (char *) tag->ti_Data;
    if (!NL_Read_Format(data,(char *) tag->ti_Data,(tag->ti_Tag == MUIA_List_Format)))
    {
      CoerceMethod(cl, obj, OM_DISPOSE);
      return(0);
    }
  }
  else
  {
    data->NList_Format = NULL;
    if (!NL_Read_Format(data, (char *)"",FALSE))
    {
      CoerceMethod(cl, obj, OM_DISPOSE);
      return(0);
    }
  }

  if (!NeedAffInfo(data,AFFINFOS_START_MAX))
  {
    CoerceMethod(cl, obj, OM_DISPOSE);
    return(0);
  }
/*D(bug("%lx|NEW 5 \n",obj));*/

  if ((tag = FindTagItem(MUIA_NList_Input, taglist)) ||
      (tag = FindTagItem(MUIA_Listview_Input, taglist)))
    data->NList_Input = tag->ti_Data;

  if(!FindTagItem(MUIA_Frame, taglist))
  {
    if (data->NList_Input)
    {
      nnset(obj,MUIA_Frame, MUIV_Frame_InputList);
    }
    else
    {
      nnset(obj,MUIA_Frame, MUIV_Frame_ReadList);
    }
  }

  if((tag = FindTagItem(MUIA_ContextMenu, taglist)))
  {
    data->NList_ContextMenu = data->ContextMenu = data->ContextMenuOn = tag->ti_Data;
  }
  else
    notdoset(obj,MUIA_ContextMenu,data->ContextMenu);

  if((tag = FindTagItem(MUIA_Font, taglist)))
    data->NList_Font = tag->ti_Data;
  else if (!data->ListCompatibility)
    data->NList_Font = MUIV_NList_Font;
  else
    data->NList_Font = MUIV_Font_List;

  if((tag = FindTagItem(MUIA_NList_AutoCopyToClip, taglist)))
    data->NList_AutoCopyToClip = (LONG) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_AutoClip, taglist)))
    data->NList_AutoClip = (BOOL)tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_TabSize, taglist)))
    data->NList_TabSize = (LONG) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_SkipChars, taglist)))
    data->NList_SkipChars = (char *) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_WordSelectChars, taglist)))
    data->NList_WordSelectChars = (char *) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_EntryValueDependent, taglist)))
    data->NList_EntryValueDependent = (LONG) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_PrivateData, taglist)))
    data->NList_PrivateData = (APTR) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_Title, taglist)) ||
     (tag = FindTagItem(MUIA_List_Title, taglist)))
  {
    data->NList_Title = (char *) tag->ti_Data;
  }

  if((tag = FindTagItem(MUIA_NList_TitleSeparator, taglist)))
    data->NList_TitleSeparator = (LONG) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_TitleClick, taglist)))
    WANT_NOTIFY(NTF_TitleClick);

  if((tag = FindTagItem(MUIA_NList_TitleClick2, taglist)))
    WANT_NOTIFY(NTF_TitleClick2);

  if((tag = FindTagItem(MUIA_NList_MultiSelect, taglist)) ||
     (tag = FindTagItem(MUIA_Listview_MultiSelect, taglist)))
  {
    data->multiselect = data->NList_MultiSelect = (LONG) tag->ti_Data;
  }

  if((tag = FindTagItem(MUIA_NList_DefaultObjectOnClick, taglist)))
    data->NList_DefaultObjectOnClick = (LONG) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_ActiveObjectOnClick, taglist)))
  {
    data->NList_ActiveObjectOnClick = (BOOL)tag->ti_Data;
    if(data->NList_ActiveObjectOnClick)
    {
      // disable that the object will automatically get a border when
      // the ActiveObjectOnClick option is active
      _flags(obj) |= (1<<7);
    }
  }

  if((tag = FindTagItem(MUIA_NList_MinLineHeight, taglist)))
    data->NList_MinLineHeight = (LONG) tag->ti_Data;

  if((tag = FindTagItem(MUIA_List_MinLineHeight, taglist)))
    data->NList_MinLineHeight = (LONG) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_TypeSelect, taglist)))
    data->NList_TypeSelect = (LONG) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_AutoVisible, taglist)) ||
     (tag = FindTagItem(MUIA_List_AutoVisible, taglist)))
  {
    data->NList_AutoVisible = (LONG) tag->ti_Data;
  }

  if((tag = FindTagItem(MUIA_NList_DefClickColumn, taglist)) ||
     (tag = FindTagItem(MUIA_Listview_DefClickColumn, taglist)))
  {
    data->NList_DefClickColumn = (LONG) tag->ti_Data;
  }

  if((tag = FindTagItem(MUIA_NList_ShowDropMarks, taglist)) ||
     (tag = FindTagItem(MUIA_List_ShowDropMarks, taglist)))
  {
    data->NList_ShowDropMarks = (LONG) tag->ti_Data;
  }

  if((tag = FindTagItem(MUIA_NList_DragColOnly, taglist)))
    data->NList_DragColOnly = (LONG) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_SortType, taglist)))
    data->NList_SortType = (LONG) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_SortType2, taglist)))
    data->NList_SortType2 = (LONG) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_MinColSortable, taglist)))
    data->NList_MinColSortable = (LONG) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_Imports, taglist)))
    data->NList_Imports = (LONG) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_Exports, taglist)))
    data->NList_Exports = (LONG) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_TitleMark, taglist)))
    data->NList_TitleMark = (LONG) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_TitleMark2, taglist)))
    data->NList_TitleMark2 = (LONG) tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_Columns, taglist)))
    NL_Columns(data,(BYTE *) tag->ti_Data);

  if((tag = FindTagItem(MUIA_NList_KeyUpFocus, taglist)))
    data->NList_KeyUpFocus = (Object *)tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_KeyDownFocus, taglist)))
    data->NList_KeyDownFocus = (Object *)tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_KeyLeftFocus, taglist)))
    data->NList_KeyLeftFocus = (Object *)tag->ti_Data;

  if((tag = FindTagItem(MUIA_NList_KeyRightFocus, taglist)))
    data->NList_KeyRightFocus = (Object *)tag->ti_Data;

  if (data->NList_DragSortable)
    data->NList_ShowDropMarks = TRUE;

/*D(bug("%lx|NEW 6 \n",obj));*/

  INIT_PEN(MUIA_NList_TitlePen,   data->NList_TitlePen,   data->Pen_Title_init);
  INIT_PEN(MUIA_NList_ListPen,    data->NList_ListPen,    data->Pen_List_init);
  INIT_PEN(MUIA_NList_SelectPen,  data->NList_SelectPen,  data->Pen_Select_init);
  INIT_PEN(MUIA_NList_CursorPen,  data->NList_CursorPen,  data->Pen_Cursor_init);
  INIT_PEN(MUIA_NList_UnselCurPen,data->NList_UnselCurPen,data->Pen_UnselCur_init);
  INIT_PEN(MUIA_NList_InactivePen,data->NList_InactivePen,data->Pen_Inactive_init);

  INIT_BG(MUIA_NList_TitleBackground,   data->NList_TitleBackGround,   data->BG_Title_init);
  INIT_BG(MUIA_Background,              data->NList_ListBackGround,    data->BG_List_init);
  if (!data->BG_List_init)
    INIT_BG(MUIA_NList_ListBackground,  data->NList_ListBackGround,    data->BG_List_init);
  INIT_BG(MUIA_NList_SelectBackground,  data->NList_SelectBackground,  data->BG_Select_init);
  INIT_BG(MUIA_NList_CursorBackground,  data->NList_CursorBackground,  data->BG_Cursor_init);
  INIT_BG(MUIA_NList_UnselCurBackground,data->NList_UnselCurBackground,data->BG_UnselCur_init);
  INIT_BG(MUIA_NList_InactiveBackground,data->NList_InactiveBackground,data->BG_Inactive_init);

/*
 *   for (tags=((struct opSet *)msg)->ops_AttrList;tag=NextTagItem(&tags);)
 *   { switch (tag->ti_Tag)
 *     { case MYATTR_PEN:
 *         if (tag->ti_Data)
 *           data->penspec = *((struct MUI_PenSpec *)tag->ti_Data);
 *         break;
 *     }
 *   }
 */

/*D(bug("%lx|NEW 7 \n",obj));*/
  if((tag = FindTagItem(MUIA_NList_AdjustHeight, taglist)))
  {
    if (tag->ti_Data)
      data->NList_AdjustHeight = -1;
  }

  if((tag = FindTagItem(MUIA_NList_AdjustWidth, taglist)))
  {
    if (tag->ti_Data)
      data->NList_AdjustWidth = -1;
  }

  if((tag = FindTagItem(MUIA_NList_SourceInsert, taglist)) && tag->ti_Data)
  {
    struct MUIP_NList_InsertWrap *ins = (struct MUIP_NList_InsertWrap *) tag->ti_Data;

    NL_List_Insert(data,ins->entries,ins->count,ins->pos,ins->wrapcol,ins->align & ALIGN_MASK,0);

    if(data->NList_Entries > 0)
      data->NList_SourceArray = 1;

    if((tag = FindTagItem(MUIA_NList_First, taglist)))
      NL_List_First(data,(long) tag->ti_Data,tag);

    if((tag = FindTagItem(MUIA_NList_Active, taglist)))
      NL_List_Active(data,(long) tag->ti_Data,tag,MUIV_NList_Select_None,FALSE,0);
  }
  else if((tag = FindTagItem(MUIA_NList_SourceString, taglist)) && tag->ti_Data)
  {
    NL_List_Insert(data,(APTR *) tag->ti_Data,-2,MUIV_NList_Insert_Bottom,0,0,0);

    if(data->NList_Entries > 0)
      data->NList_SourceArray = 1;

    if((tag = FindTagItem(MUIA_NList_First, taglist)))
      NL_List_First(data,(long) tag->ti_Data,tag);

    if((tag = FindTagItem(MUIA_NList_Active, taglist)))
      NL_List_Active(data,(long) tag->ti_Data,tag,MUIV_NList_Select_None,FALSE,0);
  }
  else if((tag = FindTagItem(MUIA_NList_SourceArray, taglist)) ||
          (tag = FindTagItem(MUIA_List_SourceArray, taglist)))
  {
    NL_List_Insert(data,(APTR *) tag->ti_Data,-1,MUIV_NList_Insert_Bottom,0,0,0);

    if(data->NList_Entries > 0)
      data->NList_SourceArray = 1;

    if((tag = FindTagItem(MUIA_NList_First, taglist)))
      NL_List_First(data,(long) tag->ti_Data,tag);

    if((tag = FindTagItem(MUIA_NList_Active, taglist)))
      NL_List_Active(data,(long) tag->ti_Data,tag,MUIV_NList_Select_None,FALSE,0);
  }

  if((tag = FindTagItem(MUIA_NList_IgnoreSpecialChars, taglist)))
    data->NList_IgnoreSpecialChars = (const char *)tag->ti_Data;

  data->ihnode.ihn_Object  = obj;
  data->ihnode.ihn_Millis  = 30;
  data->ihnode.ihn_Method  = MUIM_NList_Trigger;
  data->ihnode.ihn_Flags   = MUIIHNF_TIMER;

  set(obj,MUIA_FillArea,(LONG) FALSE);

/*D(bug("%lx|NEW 9 \n",obj));*/

  return((IPTR)obj);
}


IPTR mNL_Dispose(struct IClass *cl,Object *obj,Msg msg)
{
  struct NLData *data;
  data = INST_DATA(cl,obj);

/*D(bug("%lx|mNL_Dispose() 1 \n",obj));*/

  data->NList_Quiet = 1;
  data->SETUP = 3;

  NL_List_Clear(data);

  data->format_chge = data->do_draw_all = data->do_draw_title = data->do_draw_active = data->do_draw = FALSE;
  data->do_parse = data->do_images = data->do_setcols = data->do_updatesb = data->do_wwrap = data->force_wwrap = FALSE;
  data->Notify = data->DoNotify = data->Notifying = 0;

  if (data->MenuObj)
  {
    MUI_DisposeObject(data->MenuObj);
    data->MenuObj = NULL;
  }

  DeleteNImages2(data);

  DeleteNImages(data);

  if (data->NList_UseImages)
    FreeVecPooled(data->Pool, data->NList_UseImages);

  data->NList_UseImages = NULL;
  data->LastImage = 0;

  FreeAffInfo(data);

  NL_Free_Format(data);

  if(data->EntryPool != NULL)
  {
    #if defined(__amigaos4__)
    FreeSysObject(ASOT_ITEMPOOL, data->EntryPool);
    #else
    DeletePool(data->EntryPool);
    #endif
  }

  if(data->PoolInternal != NULL)
  {
    #if defined(__amigaos4__)
    FreeSysObject(ASOT_MEMPOOL, data->PoolInternal);
    #else
    DeletePool(data->PoolInternal);
    #endif
  }

  return(DoSuperMethodA(cl,obj,msg));

}



IPTR mNL_Setup(struct IClass *cl,Object *obj,struct MUIP_Setup *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  LONG ent;
  Object *o;

/*D(bug("%lx|mNL_Setup() 1 \n",obj));*/

  data->DRAW = 0;
  data->SETUP = FALSE;

  if (data->NList_Disabled)
    data->NList_Disabled = 1;

  if (data->NList_AdjustWidth)
    data->NList_AdjustWidth = -1;

  GetImages(data);

  data->SETUP = TRUE;

/*  data->MinImageHeight = 5;*/
  data->display_ptr = NULL;
  if (data->NList_Font && !data->InUseFont)
  {
    char *fontname = NULL;
    IPTR fonttmp = data->NList_Font;

    if (data->NList_Font == MUIV_NList_Font)
    {
      fonttmp = MUIV_Font_List;
      DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_Font, &fontname);
    }
    else if (data->NList_Font == MUIV_NList_Font_Little)
    {
      fonttmp = MUIV_Font_Tiny;
      DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_Font_Little, &fontname);
    }
    else if (data->NList_Font == MUIV_NList_Font_Fixed)
    {
      fonttmp = MUIV_Font_Fixed;
      DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_Font_Fixed, &fontname);
    }
    if (fontname && *fontname)
    {
      struct TextAttr myta;
      LONG fsize = 8;
      char fname[64];
      char *p;

      strlcpy(fname, fontname, sizeof(fname));
      // strip the font size from the name and extract the number
      if((p = strchr(fname, '/')) != NULL)
      {
        *p++ = '\0';
        fsize = atol(p);
      }
      // append the ".font" suffix
      strlcat(fname, ".font", sizeof(fname));

      myta.ta_Name = fname;
      myta.ta_YSize = fsize;
      myta.ta_Style = 0;
      myta.ta_Flags = 0;
      data->InUseFont = OpenDiskFont(&myta);
    }
    if (data->InUseFont)
    {
      notdoset(obj,MUIA_Font,data->InUseFont);
      /*_font(obj) = data->InUseFont;*/
    }
    else
    {
      notdoset(obj,MUIA_Font,fonttmp);
    }
    NL_SetColsRem(data,-2);
  }

  if (!(DoSuperMethodA(cl,obj,(Msg) msg)))
  {
    if (data->InUseFont)
    {
      notdoset(obj,MUIA_Font,0L);
      CloseFont(data->InUseFont);
      data->InUseFont = NULL;
    }
    return(FALSE);
  }

  data->rp = NULL;

  data->nodraw = 0;

  if (data->NList_MinLineHeight <= 0)
    data->addvinc = -data->NList_MinLineHeight;
  else
  {
    LONG *ptrd;
    if (DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_VertInc, &ptrd))
      data->addvinc = *ptrd;
    else
      data->addvinc = DEFAULT_VERT_INC;
  }

  {
    LONG *ptrd;
    if (DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_ColWidthDrag, &ptrd))
      data->NList_ColWidthDrag = *ptrd;
    else
      data->NList_ColWidthDrag = DEFAULT_CWD;
  }

  {
    LONG *ptrd;
    if (DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_PartialCol, &ptrd))
      data->NList_PartialCol = *ptrd;
    else
      data->NList_PartialCol = DEFAULT_PARTIALCOL;
  }

  {
    LONG *ptrd;
    if (DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_PartialChar, &ptrd))
      data->NList_PartialChar = *ptrd;
    else
      data->NList_PartialChar = DEFAULT_PARTIALCHAR;
  }

  {
    LONG *ptrd;
    data->NList_List_Select = MUIV_NList_Select_List;
    if (DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_List_Select, &ptrd))
    { if (!*ptrd)
        data->NList_List_Select = MUIV_NList_Select_None;
    }
  }

  {
    LONG *ptrd;
    if (DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_SerMouseFix, &ptrd))
      data->NList_SerMouseFix = *ptrd;
    else
      data->NList_SerMouseFix = DEFAULT_SERMOUSEFIX;
  }

  {
    LONG *ptrd;
    if (DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_DragLines, &ptrd))
      data->NList_DragLines = *ptrd + 1;
    else
      data->NList_DragLines = DEFAULT_DRAGLINES;
  }

  {
    LONG *ptrd;
    if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_WheelStep, &ptrd))
      data->NList_WheelStep = *ptrd;
    else
      data->NList_WheelStep = DEFAULT_WHEELSTEP;
  }

  {
    LONG *ptrd;

    if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_WheelFast, &ptrd))
      data->NList_WheelFast = *ptrd;
    else
      data->NList_WheelFast = DEFAULT_WHEELFAST;
  }

  {
    LONG *ptrd;
    data->NList_WheelMMB = DEFAULT_WHEELMMB;

    if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_WheelMMB, &ptrd))
    {
      if(*ptrd != 0)
        data->NList_WheelMMB = TRUE;
      else
        data->NList_WheelMMB = FALSE;
    }
  }

  {
    LONG *ptrd;
    data->NList_SelectPointer = DEFAULT_SELECTPOINTER;
    if(DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_SelectPointer, &ptrd))
    {
      if(*ptrd != 0)
        data->NList_SelectPointer = TRUE;
      else
        data->NList_SelectPointer = FALSE;
    }
  }

  // determine our parent NListview object
  data->nlistviewobj = NULL;
  o = obj;
  while((o = (Object *)xget(o, MUIA_Parent)) != NULL)
  {
    // check if the parent object return ourself as its NList object
    // this one will be our parent NListview object
    if((Object *)xget(o, MUIA_NListview_NList) == obj)
    {
      data->nlistviewobj = o;
      D(DBF_STARTUP, "found parent NListview object %08lx", data->nlistviewobj);
      break;
    }
  }

  // now we try to see if the parent listview object is
  // an NListview or a plain Listview.mui object so that we
  // can set the listviewobj pointer accordingly
  data->listviewobj = NULL;
  o = obj;
  while((o = (Object *)xget(o, MUIA_Parent)))
  {
    Object *tagobj;

    if((tagobj = (Object *)xget(o, MUIA_Listview_List)) != NULL)
    {
      if(tagobj == obj && (Object *)xget(o, MUIA_NListview_NList) == NULL)
      {
        SIPTR tagval;

        data->listviewobj = o;
        WANT_NOTIFY(NTF_LV_Select);
        WANT_NOTIFY(NTF_LV_Doubleclick);
        WANT_NOTIFY(NTF_L_Active);
        WANT_NOTIFY(NTF_Entries);

        // check if we have a DragType attribute or not
        if(GetAttr(MUIA_Listview_DragType, data->listviewobj, (IPTR *)&tagval) != FALSE)
          data->NList_DragType = tagval;

        // in case this is MUI 3.8 we can query more detailed information
        // by directly accessing the raw instance data of the Listview.mui
        // object. Puh, what a hack!
        if(MUIMasterBase->lib_Version <= 19 && data->pad2)
        {
          struct IClass *lcl,*lcl1,*lcl2,*lcl3,*lcl4;
          UBYTE *ldata = ((UBYTE *) o) + 178;
          lcl = lcl1 = lcl2 = lcl3 = lcl4 = OCLASS(o);

          while (lcl->cl_Super)     /* when loop is finished : */
          { lcl4 = lcl3;            /*  Listview  */
            lcl3 = lcl2;            /*  Group     */
            lcl2 = lcl1;            /*  Area      */
            lcl1 = lcl;             /*  Notify    */
            lcl = lcl->cl_Super;    /*  rootclass */
          }

          if (lcl4->cl_InstSize == 68)  /* data size of Listview.mui class in 3.8 */
          {
            data->multiselect = data->NList_MultiSelect = (LONG) ldata[43];
            data->NList_Input = (LONG) ldata[65];
            ldata[40] = ldata[41] = ldata[42] = ldata[43] = 0;
            ldata[64] = ldata[65] = 0;
          }
        }
      }
      break;
    }
  }

  if (data->NList_DragType == MUIV_NList_DragType_Default)
  { LONG *ptrd;
    if (DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_DragType, &ptrd))
      data->drag_type = *ptrd;
    else
      data->drag_type = MUIV_NList_DragType_Immediate;
  }
  else
    data->drag_type = data->NList_DragType;

  if ((data->NList_DragSortable) && (data->drag_type == MUIV_NList_DragType_None))
  { LONG *ptrd;
    if (DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_DragType, &ptrd))
      data->drag_type = *ptrd;
    else
      data->drag_type = MUIV_NList_DragType_Immediate;
  }

/*
  { LONG *ptrd;
    if (DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_DragQualifier, &ptrd))
      data->drag_qualifier = *ptrd;
    else
      data->drag_qualifier = 0;
  }
*/

  { LONG *ptrd;

    if (DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_Smooth, &ptrd))
      data->NList_Smooth = *ptrd;
    else
      data->NList_Smooth = DEFAULT_SMOOTHSCROLL;

    if (data->VertPropObject)
    {
      set(data->VertPropObject,MUIA_Prop_DoSmooth, data->NList_Smooth);
    }
  }

  {
    LONG *ptrd;

    if (data->NList_Keys && (data->NList_Keys != default_keys))
    {
      FreeVecPooled(data->Pool, data->NList_Keys);
      data->NList_Keys = default_keys;
    }

    if (DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_Keys, &ptrd))
    {
      struct KeyBinding *keys = (struct KeyBinding *) ptrd;
      LONG nk = 0;

      while (keys[nk].kb_KeyTag)
        nk++;

      if((data->NList_Keys = AllocVecPooled(data->Pool, sizeof(struct KeyBinding) * (nk + 1))) != NULL)
      {
        while (nk >= 0)
        {
          data->NList_Keys[nk] = keys[nk];
          nk--;
        }
      }
      else
        data->NList_Keys = default_keys;
    }
    else
      data->NList_Keys = default_keys;
  }

  LOAD_PEN(data->Pen_Title_init,   data->NList_TitlePen,   MUICFG_NList_Pen_Title,   DEFAULT_PEN_TITLE);
  LOAD_PEN(data->Pen_List_init,    data->NList_ListPen,    MUICFG_NList_Pen_List,    DEFAULT_PEN_LIST);
  LOAD_PEN(data->Pen_Select_init,  data->NList_SelectPen,  MUICFG_NList_Pen_Select,  DEFAULT_PEN_SELECT);
  LOAD_PEN(data->Pen_Cursor_init,  data->NList_CursorPen,  MUICFG_NList_Pen_Cursor,  DEFAULT_PEN_CURSOR);
  LOAD_PEN(data->Pen_UnselCur_init,data->NList_UnselCurPen,MUICFG_NList_Pen_UnselCur,DEFAULT_PEN_UNSELCUR);
  LOAD_PEN(data->Pen_Inactive_init,data->NList_InactivePen,MUICFG_NList_Pen_Inactive,DEFAULT_PEN_INACTIVE);

  LOAD_BG(data->BG_Title_init,   data->NList_TitleBackGround,   MUICFG_NList_BG_Title,   DEFAULT_BG_TITLE);
  LOAD_BG(data->BG_List_init,    data->NList_ListBackGround,    MUICFG_NList_BG_List,    DEFAULT_BG_LIST);
  LOAD_BG(data->BG_Select_init,  data->NList_SelectBackground,  MUICFG_NList_BG_Select,  DEFAULT_BG_SELECT);
  LOAD_BG(data->BG_Cursor_init,  data->NList_CursorBackground,  MUICFG_NList_BG_Cursor,  DEFAULT_BG_CURSOR);
  LOAD_BG(data->BG_UnselCur_init,data->NList_UnselCurBackground,MUICFG_NList_BG_UnselCur,DEFAULT_BG_UNSELCUR);
  LOAD_BG(data->BG_Inactive_init,data->NList_InactiveBackground,MUICFG_NList_BG_Inactive,DEFAULT_BG_INACTIVE);

  if (data->NList_ForcePen == MUIV_NList_ForcePen_Default)
  {
    LONG *ptrd, fpen = MUIV_NList_ForcePen_Off;
    if (DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_ForcePen, &ptrd))
      fpen = *ptrd;
    data->ForcePen = (LONG) fpen;
  }
  else
    data->ForcePen = data->NList_ForcePen;

  for(ent = 0;ent < data->NList_Entries;ent++)
    data->EntriesArray[ent]->PixLen = -1;

  data->actbackground = -1;

  if (data->NList_SourceArray)
    data->NList_SourceArray = 2;

  data->multiselect = data->NList_MultiSelect;
  data->multisel_qualifier = 0;
  { LONG *multisel;
    LONG mult = MUIV_NList_MultiSelect_Shifted;
    if (DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_MultiSelect, &multisel))
      mult = *multisel;
    if (data->NList_MultiSelect == MUIV_NList_MultiSelect_Default)
      data->multiselect = mult & 0x0007;
    if ((mult & MUIV_NList_MultiSelect_MMB_On) == MUIV_NList_MultiSelect_MMB_On)
      data->multisel_qualifier = IEQUALIFIER_MIDBUTTON;
  }

  if (data->NList_ContextMenu == (LONG)MUIV_NList_ContextMenu_Default)
  {
    LONG *ptrd;
    data->ContextMenu = MUIV_NList_ContextMenu_Always;
    if (DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_Menu, &ptrd))
    {
      switch (*ptrd)
      { case MUIV_NList_ContextMenu_TopOnly :
        case MUIV_NList_ContextMenu_Always :
        case MUIV_NList_ContextMenu_Never :
          data->ContextMenu = *ptrd;
          break;
      }
    }
  }

  /* Use centered text lines? */
  {
    LONG *vert;

    data->NList_VerticalCenteredText = DEFAULT_VCENTERED;
    if (DoMethod(obj, MUIM_GetConfigItem, MUICFG_NList_VCenteredLines, &vert))
      data->NList_VerticalCenteredText = *vert;
  }

  if (data->ContextMenu != data->ContextMenuOn)
  {
    if (/*(((data->ContextMenu & 0x9d510030) == 0x9d510030) && (data->numcols <= 1)) ||*/ /* sba: Contextmenu problem: Disabled */
        (data->ContextMenu == (LONG)MUIV_NList_ContextMenu_Never))
      notdoset(obj,MUIA_ContextMenu,NULL);
    else
      notdoset(obj,MUIA_ContextMenu,data->ContextMenu);
  }

  data->ScrollBarsPos = -2;
  data->ScrollBars = 0;
  data->ScrollBarsTime = -1;

  NL_CreateImages(data);

  data->drawsuper = NULL;
  data->format_chge = 1;
  data->do_draw_all = data->do_draw_title = data->do_draw = TRUE;
  data->do_parse = data->do_setcols = data->do_updatesb = data->do_wwrap = TRUE;
  /*data->force_wwrap = TRUE;*/
  data->badrport = FALSE;
  data->refreshing = FALSE;

  data->first_change = LONG_MAX;
  data->last_change = LONG_MIN;

  data->adjustbar = -1;
  data->adjustbar_old = -1;

  Make_Active_Visible;

  {
    Object *o = obj;

    data->VirtGroup = NULL;
    data->VirtGroup2 = NULL;

    while((o = (Object *)xget(o, MUIA_Parent)))
    {
      IPTR val;

      // check if the class "knows" the Virtgroup_Left and
      // Virtgroup_Top attributes
      if(GetAttr(MUIA_Virtgroup_Left, o, &val) != FALSE &&
         GetAttr(MUIA_Virtgroup_Top, o, &val) != FALSE)
      {
        data->VirtGroup = o;

        if (!data->VirtClass)
        {
          o = MUI_NewObject(MUIC_Virtgroup,Child, MUI_NewObject(MUIC_Rectangle, TAG_DONE), TAG_DONE);

          if (o)
          {
            data->VirtClass = OCLASS(o);
            MUI_DisposeObject(o);
          }
        }
        break;
      }
    }
    data->VirtGroup2 = data->VirtGroup;
  }

  // setup our custom selection pointer
  SetupCustomPointers(data);

/*  MUI_RequestIDCMP(obj,IDCMP_MOUSEBUTTONS|IDCMP_RAWKEY|IDCMP_INTUITICKS|IDCMP_ACTIVEWINDOW|IDCMP_INACTIVEWINDOW);*/
/*  MUI_RequestIDCMP(obj,IDCMP_MOUSEBUTTONS|IDCMP_RAWKEY|IDCMP_INTUITICKS);*/

  data->MOUSE_MOVE = FALSE;
/*  data->ehnode.ehn_Events = (IDCMP_MOUSEBUTTONS|IDCMP_RAWKEY|IDCMP_INTUITICKS|IDCMP_MOUSEMOVE);*/
  data->ehnode.ehn_Events = (IDCMP_MOUSEBUTTONS|IDCMP_RAWKEY|IDCMP_INTUITICKS|IDCMP_ACTIVEWINDOW|IDCMP_INACTIVEWINDOW);
  data->ehnode.ehn_Priority = 1;
  data->ehnode.ehn_Flags = MUI_EHF_GUIMODE;
  data->ehnode.ehn_Object = obj;
  data->ehnode.ehn_Class  = cl;

  // add IDCMP_EXTENDEDMOUSE for OS4 wheelmouse support
  #if defined(__amigaos4__)
  data->ehnode.ehn_Events |= IDCMP_EXTENDEDMOUSE;
  #endif

  DoMethod(_win(obj),MUIM_Window_AddEventHandler, &data->ehnode);

  DoMethod(_app(obj),MUIM_Application_AddInputHandler,&data->ihnode);

/*  GetNImage_Sizes(data);*/

  data->pad2 = FALSE;

/*D(bug("%lx|mNL_Setup() 2 \n",obj));*/

  return(TRUE);
}

IPTR mNL_Cleanup(struct IClass *cl,Object *obj,struct MUIP_Cleanup *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  IPTR retval;

/*D(bug("%lx|mNL_Cleanup() 1 \n",obj));*/

  data = INST_DATA(cl,obj);

  data->nodraw = 1;
  data->DRAW = 0;

  // cleanup our custom mouse pointers
  CleanupCustomPointers(data);

  DoMethod(_app(obj),MUIM_Application_RemInputHandler,&data->ihnode);

  DoMethod(_win(obj),MUIM_Window_RemEventHandler, &data->ehnode);

/*  MUI_RejectIDCMP(obj,IDCMP_MOUSEBUTTONS|IDCMP_RAWKEY|IDCMP_INTUITICKS|IDCMP_ACTIVEWINDOW|IDCMP_INACTIVEWINDOW);*/
/*  MUI_RejectIDCMP(obj,IDCMP_MOUSEBUTTONS|IDCMP_RAWKEY|IDCMP_INTUITICKS);*/

  data->NList_Quiet++;
  data->VirtGroup = NULL;

  // forget that this object has received a MUIM_Setup
  // this must be done before calling NL_DeleteImages(), because that function
  // will try to relayout the group data->NL_Group, which is not a very wise
  // thing to do while we a cleaning up ourselves.
  data->SETUP = FALSE;

  NL_DeleteImages(data);

  if (data->NList_Keys && (data->NList_Keys != default_keys))
  {
    FreeVecPooled(data->Pool, data->NList_Keys);
    data->NList_Keys = default_keys;
  }

  release_pen(obj, &data->NList_TitlePen);
  release_pen(obj, &data->NList_ListPen);
  release_pen(obj, &data->NList_SelectPen);
  release_pen(obj, &data->NList_CursorPen);
  release_pen(obj, &data->NList_UnselCurPen);
  release_pen(obj, &data->NList_InactivePen);

  retval = DoSuperMethodA(cl,obj,(Msg) msg);

  if (data->InUseFont)
  {
    notdoset(obj,MUIA_Font,0L);
    CloseFont(data->InUseFont);
    data->InUseFont = NULL;
  }

  data->voff = 1;

  data->rp = NULL;
  data->badrport = FALSE;
  data->UpdateScrollersRedrawn = FALSE;

  data->NList_Quiet--;

/*D(bug("%lx|mNL_Cleanup() 2 \n",obj));*/

  return (retval);
}
