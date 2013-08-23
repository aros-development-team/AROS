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

#ifndef MUI_NList_MCC_H
#define MUI_NList_MCC_H

#ifndef LIBRARIES_MUI_H
#include <libraries/mui.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__AROS__) && defined(__PPC__)
  #if defined(__GNUC__)
    #pragma pack(2)
  #elif defined(__VBCC__)
    #pragma amiga-align
  #endif
#endif

/***********************************************************************/

// STACKED ensures proper alignment on AROS 64 bit systems
#if !defined(__AROS__) && !defined(STACKED)
#define STACKED
#endif
#if !defined(__AROS__) && !defined(SIPTR)
#define SIPTR LONG
#endif

/***********************************************************************/

/* MUI Prop and Scroller classes stuff which is still not in libraries/mui.h  (in MUI3.8) */
/* it gives to the prop object it's increment value */
#ifndef MUIA_Prop_DeltaFactor
#define MUIA_Prop_DeltaFactor 0x80427C5EUL
#endif

#define MUIC_NList "NList.mcc"
#if defined(__AROS__) && !defined(NO_INLINE_STDARG)
#define NListObject MUIOBJMACRO_START(MUIC_NList)
#else
#define NListObject MUI_NewObject(MUIC_NList
#endif

/* Attributes */

#define MUIA_NList_TypeSelect               0x9d510030UL /* GM  is.  LONG              */
#define MUIA_NList_Prop_DeltaFactor         0x9d510031UL /* GM  ..gn LONG              */
#define MUIA_NList_Horiz_DeltaFactor        0x9d510032UL /* GM  ..gn LONG              */

#define MUIA_NList_Horiz_First              0x9d510033UL /* GM  .sgn LONG              */
#define MUIA_NList_Horiz_Visible            0x9d510034UL /* GM  ..gn LONG              */
#define MUIA_NList_Horiz_Entries            0x9d510035UL /* GM  ..gn LONG              */

#define MUIA_NList_Prop_First               0x9d510036UL /* GM  .sgn LONG              */
#define MUIA_NList_Prop_Visible             0x9d510037UL /* GM  ..gn LONG              */
#define MUIA_NList_Prop_Entries             0x9d510038UL /* GM  ..gn LONG              */

#define MUIA_NList_TitlePen                 0x9d510039UL /* GM  isg  LONG              */
#define MUIA_NList_ListPen                  0x9d51003aUL /* GM  isg  LONG              */
#define MUIA_NList_SelectPen                0x9d51003bUL /* GM  isg  LONG              */
#define MUIA_NList_CursorPen                0x9d51003cUL /* GM  isg  LONG              */
#define MUIA_NList_UnselCurPen              0x9d51003dUL /* GM  isg  LONG              */
#define MUIA_NList_InactivePen              0x9d5100C1UL /* GM  isg  LONG              */

#define MUIA_NList_ListBackground           0x9d51003eUL /* GM  isg  LONG              */
#define MUIA_NList_TitleBackground          0x9d51003fUL /* GM  isg  LONG              */
#define MUIA_NList_SelectBackground         0x9d510040UL /* GM  isg  LONG              */
#define MUIA_NList_CursorBackground         0x9d510041UL /* GM  isg  LONG              */
#define MUIA_NList_UnselCurBackground       0x9d510042UL /* GM  isg  LONG              */
#define MUIA_NList_InactiveBackground       0x9d5100C2UL /* GM  isg  LONG              */

#define MUIA_NList_MultiClick               0x9d510043UL /* GM  ..gn LONG              */

#define MUIA_NList_DefaultObjectOnClick     0x9d510044UL /* GM  is.  BOOL              */
#define MUIA_NList_ActiveObjectOnClick      0x9d5100C3UL /* GM  is.  BOOL              */

#define MUIA_NList_ClickColumn              0x9d510045UL /* GM  ..g  LONG              */
#define MUIA_NList_DefClickColumn           0x9d510046UL /* GM  isg  LONG              */
#define MUIA_NList_DoubleClick              0x9d510047UL /* GM  ..gn LONG              */
#define MUIA_NList_DragType                 0x9d510048UL /* GM  isg  LONG              */
#define MUIA_NList_Input                    0x9d510049UL /* GM  isg  BOOL              */
#define MUIA_NList_MultiSelect              0x9d51004aUL /* GM  is.  LONG              */
#define MUIA_NList_SelectChange             0x9d51004bUL /* GM  ...n BOOL              */

#define MUIA_NList_Active                   0x9d51004cUL /* GM  isgn LONG              */
#define MUIA_NList_AdjustHeight             0x9d51004dUL /* GM  i..  BOOL              */
#define MUIA_NList_AdjustWidth              0x9d51004eUL /* GM  i..  BOOL              */
#define MUIA_NList_AutoVisible              0x9d51004fUL /* GM  isg  BOOL              */
#define MUIA_NList_CompareHook              0x9d510050UL /* GM  is.  struct Hook *     */
#define MUIA_NList_ConstructHook            0x9d510051UL /* GM  is.  struct Hook *     */
#define MUIA_NList_DestructHook             0x9d510052UL /* GM  is.  struct Hook *     */
#define MUIA_NList_DisplayHook              0x9d510053UL /* GM  is.  struct Hook *     */
#define MUIA_NList_DragSortable             0x9d510054UL /* GM  isg  BOOL              */
#define MUIA_NList_DropMark                 0x9d510055UL /* GM  ..g  LONG              */
#define MUIA_NList_Entries                  0x9d510056UL /* GM  ..gn LONG              */
#define MUIA_NList_First                    0x9d510057UL /* GM  isgn LONG              */
#define MUIA_NList_Format                   0x9d510058UL /* GM  isg  STRPTR            */
#define MUIA_NList_InsertPosition           0x9d510059UL /* GM  ..gn LONG              */
#define MUIA_NList_MinLineHeight            0x9d51005aUL /* GM  is.  LONG              */
#define MUIA_NList_MultiTestHook            0x9d51005bUL /* GM  is.  struct Hook *     */
#define MUIA_NList_Pool                     0x9d51005cUL /* GM  i..  APTR              */
#define MUIA_NList_PoolPuddleSize           0x9d51005dUL /* GM  i..  ULONG             */
#define MUIA_NList_PoolThreshSize           0x9d51005eUL /* GM  i..  ULONG             */
#define MUIA_NList_Quiet                    0x9d51005fUL /* GM  .s.  BOOL              */
#define MUIA_NList_ShowDropMarks            0x9d510060UL /* GM  isg  BOOL              */
#define MUIA_NList_SourceArray              0x9d510061UL /* GM  i..  APTR *            */
#define MUIA_NList_Title                    0x9d510062UL /* GM  isg  char *            */
#define MUIA_NList_Visible                  0x9d510063UL /* GM  ..g  LONG              */
#define MUIA_NList_CopyEntryToClipHook      0x9d510064UL /* GM  is.  struct Hook *     */
#define MUIA_NList_KeepActive               0x9d510065UL /* GM  .s.  Obj *             */
#define MUIA_NList_MakeActive               0x9d510066UL /* GM  .s.  Obj *             */
#define MUIA_NList_SourceString             0x9d510067UL /* GM  i..  char *            */
#define MUIA_NList_CopyColumnToClipHook     0x9d510068UL /* GM  is.  struct Hook *     */
#define MUIA_NList_ListCompatibility        0x9d510069UL /* GM  ...  OBSOLETE          */
#define MUIA_NList_AutoCopyToClip           0x9d51006AUL /* GM  is.  BOOL              */
#define MUIA_NList_TabSize                  0x9d51006BUL /* GM  isg  ULONG             */
#define MUIA_NList_SkipChars                0x9d51006CUL /* GM  isg  char *            */
#define MUIA_NList_DisplayRecall            0x9d51006DUL /* GM  .g.  BOOL              */
#define MUIA_NList_PrivateData              0x9d51006EUL /* GM  isg  APTR              */
#define MUIA_NList_EntryValueDependent      0x9d51006FUL /* GM  isg  BOOL              */

#define MUIA_NList_IgnoreSpecialChars       0x9d510070UL /* GM  isg  const char *      */

#define MUIA_NList_StackCheck               0x9d510097UL /* GM  i..  BOOL              */
#define MUIA_NList_WordSelectChars          0x9d510098UL /* GM  isg  char *            */
#define MUIA_NList_EntryClick               0x9d510099UL /* GM  ..gn LONG              */
#define MUIA_NList_DragColOnly              0x9d51009AUL /* GM  isg  LONG              */
#define MUIA_NList_TitleClick               0x9d51009BUL /* GM  isgn LONG              */
#define MUIA_NList_DropType                 0x9d51009CUL /* GM  ..g  LONG              */
#define MUIA_NList_ForcePen                 0x9d51009DUL /* GM  isg  LONG              */
#define MUIA_NList_SourceInsert             0x9d51009EUL /* GM  i..  struct MUIP_NList_InsertWrap *   */
#define MUIA_NList_TitleSeparator           0x9d51009FUL /* GM  isg  BOOL              */

#define MUIA_NList_AutoClip                 0x9d5100C0UL /* GM  isg  BOOL              */

#define MUIA_NList_SortType2                0x9d5100EDUL /* GM  isgn LONG              */
#define MUIA_NList_TitleClick2              0x9d5100EEUL /* GM  isgn LONG              */
#define MUIA_NList_TitleMark2               0x9d5100EFUL /* GM  isg  LONG              */
#define MUIA_NList_MultiClickAlone          0x9d5100F0UL /* GM  ..gn LONG              */
#define MUIA_NList_TitleMark                0x9d5100F1UL /* GM  isg  LONG              */
#define MUIA_NList_DragSortInsert           0x9d5100F2UL /* GM  ..gn LONG              */
#define MUIA_NList_MinColSortable           0x9d5100F3UL /* GM  isg  LONG              */
#define MUIA_NList_Imports                  0x9d5100F4UL /* GM  isg  LONG              */
#define MUIA_NList_Exports                  0x9d5100F5UL /* GM  isg  LONG              */
#define MUIA_NList_Columns                  0x9d5100F6UL /* GM  isgn BYTE *            */
#define MUIA_NList_LineHeight               0x9d5100F7UL /* GM  ..gn LONG              */
#define MUIA_NList_ButtonClick              0x9d5100F8UL /* GM  ..gn LONG              */
#define MUIA_NList_CopyEntryToClipHook2     0x9d5100F9UL /* GM  is.  struct Hook *     */
#define MUIA_NList_CopyColumnToClipHook2    0x9d5100FAUL /* GM  is.  struct Hook *     */
#define MUIA_NList_CompareHook2             0x9d5100FBUL /* GM  is.  struct Hook *     */
#define MUIA_NList_ConstructHook2           0x9d5100FCUL /* GM  is.  struct Hook *     */
#define MUIA_NList_DestructHook2            0x9d5100FDUL /* GM  is.  struct Hook *     */
#define MUIA_NList_DisplayHook2             0x9d5100FEUL /* GM  is.  struct Hook *     */
#define MUIA_NList_SortType                 0x9d5100FFUL /* GM  isgn LONG              */

#define MUIA_NList_KeyUpFocus               0x9d5100C4UL /* GM  isg. Object *          */
#define MUIA_NList_KeyDownFocus             0x9d5100C5UL /* GM  isg. Object *          */
#define MUIA_NList_KeyLeftFocus             0x9d5100C6UL /* GM  isg. Object *          */
#define MUIA_NList_KeyRightFocus            0x9d5100C7UL /* GM  isg. Object *          */

#define MUIA_NLIMG_EntryCurrent             MUIA_NList_First   /* LONG (special for nlist custom image object) */
#define MUIA_NLIMG_EntryHeight              MUIA_NList_Visible /* LONG (special for nlist custom image object) */

#define MUIA_NList_VertDeltaFactor          MUIA_NList_Prop_DeltaFactor   /* OBSOLETE NAME */
#define MUIA_NList_HorizDeltaFactor         MUIA_NList_Horiz_DeltaFactor  /* OBSOLETE NAME */

/* Attributes special datas */
#define MUIV_NList_TypeSelect_Line        0
#define MUIV_NList_TypeSelect_Char        1

#define MUIV_NList_Font                 ((IPTR)-20)
#define MUIV_NList_Font_Little          ((IPTR)-21)
#define MUIV_NList_Font_Fixed           ((IPTR)-22)

#define MUIV_NList_ConstructHook_String  -1
#define MUIV_NList_DestructHook_String   -1

#define MUIV_NList_Active_Off            -1
#define MUIV_NList_Active_Top            -2
#define MUIV_NList_Active_Bottom         -3
#define MUIV_NList_Active_Up             -4
#define MUIV_NList_Active_Down           -5
#define MUIV_NList_Active_PageUp         -6
#define MUIV_NList_Active_PageDown       -7

#define MUIV_NList_First_Top             -2
#define MUIV_NList_First_Bottom          -3
#define MUIV_NList_First_Up              -4
#define MUIV_NList_First_Down            -5
#define MUIV_NList_First_PageUp          -6
#define MUIV_NList_First_PageDown        -7
#define MUIV_NList_First_Up2             -8
#define MUIV_NList_First_Down2           -9
#define MUIV_NList_First_Up4            -10
#define MUIV_NList_First_Down4          -11

#define MUIV_NList_Horiz_First_Start     -2
#define MUIV_NList_Horiz_First_End       -3
#define MUIV_NList_Horiz_First_Left      -4
#define MUIV_NList_Horiz_First_Right     -5
#define MUIV_NList_Horiz_First_PageLeft  -6
#define MUIV_NList_Horiz_First_PageRight -7
#define MUIV_NList_Horiz_First_Left2     -8
#define MUIV_NList_Horiz_First_Right2    -9
#define MUIV_NList_Horiz_First_Left4    -10
#define MUIV_NList_Horiz_First_Right4   -11

#define MUIV_NList_MultiSelect_None       0
#define MUIV_NList_MultiSelect_Default    1
#define MUIV_NList_MultiSelect_Shifted    2
#define MUIV_NList_MultiSelect_Always     3

#define MUIV_NList_Insert_Top             0
#define MUIV_NList_Insert_Active         -1
#define MUIV_NList_Insert_Sorted         -2
#define MUIV_NList_Insert_Bottom         -3
#define MUIV_NList_Insert_Flag_Raw       (1<<0)

#define MUIV_NList_Remove_First           0
#define MUIV_NList_Remove_Active         -1
#define MUIV_NList_Remove_Last           -2
#define MUIV_NList_Remove_Selected       -3

#define MUIV_NList_Select_Off             0
#define MUIV_NList_Select_On              1
#define MUIV_NList_Select_Toggle          2
#define MUIV_NList_Select_Ask             3

#define MUIV_NList_GetEntry_Active       -1
#define MUIV_NList_GetEntryInfo_Line     -2

#define MUIV_NList_Select_Active         -1
#define MUIV_NList_Select_All            -2

#define MUIV_NList_Redraw_Active         -1
#define MUIV_NList_Redraw_All            -2
#define MUIV_NList_Redraw_Title          -3
#define MUIV_NList_Redraw_Selected       -4
#define MUIV_NList_Redraw_VisibleCols    -5

#define MUIV_NList_Move_Top               0
#define MUIV_NList_Move_Active           -1
#define MUIV_NList_Move_Bottom           -2
#define MUIV_NList_Move_Next             -3 /* only valid for second parameter (and not with Move_Selected) */
#define MUIV_NList_Move_Previous         -4 /* only valid for second parameter (and not with Move_Selected) */
#define MUIV_NList_Move_Selected         -5 /* only valid for first parameter */

#define MUIV_NList_Exchange_Top           0
#define MUIV_NList_Exchange_Active       -1
#define MUIV_NList_Exchange_Bottom       -2
#define MUIV_NList_Exchange_Next         -3 /* only valid for second parameter */
#define MUIV_NList_Exchange_Previous     -4 /* only valid for second parameter */

#define MUIV_NList_Jump_Top               0
#define MUIV_NList_Jump_Active           -1
#define MUIV_NList_Jump_Bottom           -2
#define MUIV_NList_Jump_Down             -3
#define MUIV_NList_Jump_Up               -4
#define MUIV_NList_Jump_Active_Center    -5

#define MUIV_NList_NextSelected_Start    -1
#define MUIV_NList_NextSelected_End      -1

#define MUIV_NList_PrevSelected_Start    -1
#define MUIV_NList_PrevSelected_End      -1

#define MUIV_NList_DragType_None          0
#define MUIV_NList_DragType_Default       1
#define MUIV_NList_DragType_Immediate     2
#define MUIV_NList_DragType_Borders       3
#define MUIV_NList_DragType_Qualifier     4

#define MUIV_NList_CopyToClip_Active     -1
#define MUIV_NList_CopyToClip_Selected   -2
#define MUIV_NList_CopyToClip_All        -3
#define MUIV_NList_CopyToClip_Entries    -4
#define MUIV_NList_CopyToClip_Entry      -5
#define MUIV_NList_CopyToClip_Strings    -6
#define MUIV_NList_CopyToClip_String     -7

#define MUIV_NList_CopyTo_Active         -1
#define MUIV_NList_CopyTo_Selected       -2
#define MUIV_NList_CopyTo_All            -3
#define MUIV_NList_CopyTo_Entries        -4
#define MUIV_NList_CopyTo_Entry          -5

#define MUIV_NLCT_Success                 0
#define MUIV_NLCT_OpenErr                 1
#define MUIV_NLCT_WriteErr                2
#define MUIV_NLCT_Failed                  3

#define MUIV_NList_ForcePen_On            1
#define MUIV_NList_ForcePen_Off           0
#define MUIV_NList_ForcePen_Default      -1

#define MUIV_NList_DropType_Mask          0x00FF
#define MUIV_NList_DropType_None          0
#define MUIV_NList_DropType_Above         1
#define MUIV_NList_DropType_Below         2
#define MUIV_NList_DropType_Onto          3

#define MUIV_NList_DoMethod_Active       -1
#define MUIV_NList_DoMethod_Selected     -2
#define MUIV_NList_DoMethod_All          -3

#define MUIV_NList_DoMethod_Entry        -1
#define MUIV_NList_DoMethod_Self         -2
#define MUIV_NList_DoMethod_App          -3

#define MUIV_NList_EntryValue             (MUIV_TriggerValue+0x100)
#define MUIV_NList_EntryPosValue          (MUIV_TriggerValue+0x102)
#define MUIV_NList_SelfValue              (MUIV_TriggerValue+0x104)
#define MUIV_NList_AppValue               (MUIV_TriggerValue+0x106)

#define MUIV_NList_ColWidth_All          -1
#define MUIV_NList_ColWidth_Default      -1
#define MUIV_NList_ColWidth_Get          -2

#define MUIV_NList_ContextMenu_Default    0x9d510031
#define MUIV_NList_ContextMenu_TopOnly    0x9d510033
#define MUIV_NList_ContextMenu_BarOnly    0x9d510035
#define MUIV_NList_ContextMenu_Bar_Top    0x9d510037
#define MUIV_NList_ContextMenu_Always     0x9d510039
#define MUIV_NList_ContextMenu_Never      0x9d51003b

#define MUIV_NList_Menu_DefWidth_This     0x9d51003d
#define MUIV_NList_Menu_DefWidth_All      0x9d51003f
#define MUIV_NList_Menu_DefOrder_This     0x9d510041
#define MUIV_NList_Menu_DefOrder_All      0x9d510043
#define MUIV_NList_Menu_Default_This      MUIV_NList_Menu_DefWidth_This
#define MUIV_NList_Menu_Default_All       MUIV_NList_Menu_DefWidth_All

#define MUIV_NList_SortType_None          0xF0000000
#define MUIV_NList_SortTypeAdd_None       0x00000000
#define MUIV_NList_SortTypeAdd_2Values    0x80000000
#define MUIV_NList_SortTypeAdd_4Values    0x40000000
#define MUIV_NList_SortTypeAdd_Mask       0xC0000000
#define MUIV_NList_SortTypeValue_Mask     0x3FFFFFFF

#define MUIV_NList_Sort3_SortType_Both    0x00000000
#define MUIV_NList_Sort3_SortType_1       0x00000001
#define MUIV_NList_Sort3_SortType_2       0x00000002

#define MUIV_NList_Quiet_None             0
#define MUIV_NList_Quiet_Full            -1
#define MUIV_NList_Quiet_Visual          -2

#define MUIV_NList_Imports_Active         (1 << 0)
#define MUIV_NList_Imports_Selected       (1 << 1)
#define MUIV_NList_Imports_First          (1 << 2)
#define MUIV_NList_Imports_ColWidth       (1 << 3)
#define MUIV_NList_Imports_ColOrder       (1 << 4)
#define MUIV_NList_Imports_TitleMark      (1 << 7)
#define MUIV_NList_Imports_Cols           0x000000F8
#define MUIV_NList_Imports_All            0x0000FFFF

#define MUIV_NList_Exports_Active         (1 << 0)
#define MUIV_NList_Exports_Selected       (1 << 1)
#define MUIV_NList_Exports_First          (1 << 2)
#define MUIV_NList_Exports_ColWidth       (1 << 3)
#define MUIV_NList_Exports_ColOrder       (1 << 4)
#define MUIV_NList_Exports_TitleMark      (1 << 7)
#define MUIV_NList_Exports_Cols           0x000000F8
#define MUIV_NList_Exports_All            0x0000FFFF

#define MUIV_NList_TitleMark_ColMask      0x000000FF
#define MUIV_NList_TitleMark_TypeMask     0xF0000000
#define MUIV_NList_TitleMark_None         0xF0000000
#define MUIV_NList_TitleMark_Down         0x00000000
#define MUIV_NList_TitleMark_Up           0x80000000
#define MUIV_NList_TitleMark_Box          0x40000000
#define MUIV_NList_TitleMark_Circle       0xC0000000

#define MUIV_NList_TitleMark2_ColMask     0x000000FF
#define MUIV_NList_TitleMark2_TypeMask    0xF0000000
#define MUIV_NList_TitleMark2_None        0xF0000000
#define MUIV_NList_TitleMark2_Down        0x00000000
#define MUIV_NList_TitleMark2_Up          0x80000000
#define MUIV_NList_TitleMark2_Box         0x40000000
#define MUIV_NList_TitleMark2_Circle      0xC0000000

#define MUIV_NList_SetColumnCol_Default   (-1)

#define MUIV_NList_GetPos_Start           (-1)
#define MUIV_NList_GetPos_End             (-1)

#define MUIV_NList_SelectChange_Flag_Multi (1 << 0)

#define MUIV_NList_UseImage_All           (-1)

#define MUIV_NList_SetActive_Entry         (1 << 0)
#define MUIV_NList_SetActive_Jump_Center   (1 << 1)

/* Structs */

struct BitMapImage
{
  ULONG    control;   /* should be == to MUIM_NList_CreateImage for a valid BitMapImage struct */
  WORD     width;     /* if control == MUIA_Image_Spec then obtainpens is a pointer to an Object */
  WORD     height;
  WORD    *obtainpens;
  PLANEPTR mask;
  struct BitMap imgbmp;
  LONG     flags;
};


struct MUI_NList_TestPos_Result
{
  LONG  entry;   /* number of entry, -1 if mouse not over valid entry */
  WORD  column;  /* numer of column, -1 if no valid column */
  UWORD flags;   /* not in the list, see below */
  WORD  xoffset; /* x offset in column */
  WORD  yoffset; /* y offset of mouse click from center of line */
  WORD  preparse;     /* 2 if in column preparse string, 1 if in entry preparse string, else 0 */
  WORD  char_number;  /* the number of the clicked char in column, -1 if no valid */
  WORD  char_xoffset; /* x offset of mouse clicked from left of char if positive */
};                    /* and left of next char if negative. If there is no char there */
                      /* negative if from left of first char else from right of last one */

#define MUI_NLPR_ABOVE  (1<<0)
#define MUI_NLPR_BELOW  (1<<1)
#define MUI_NLPR_LEFT   (1<<2)
#define MUI_NLPR_RIGHT  (1<<3)
#define MUI_NLPR_BAR    (1<<4)  /* if between two columns you'll get the left
                                   column number of both, and that flag */
#define MUI_NLPR_TITLE  (1<<5)  /* if clicked on title, only column, xoffset and yoffset (and MUI_NLPR_BAR)
                                    are valid (you'll get MUI_NLPR_ABOVE too) */
#define MUI_NLPR_ONTOP  (1<<6)  /* it is on title/half of first visible entry */


struct MUI_NList_GetEntryInfo
{
  LONG pos;             /* num of entry you want info about */
  LONG line;            /* real line number */
  LONG entry_pos;       /* entry num of returned entry ptr */
  APTR entry;           /* entry pointer */
  LONG wrapcol;         /* NOWRAP, WRAPCOLx, or WRAPPED|WRAPCOLx */
  LONG charpos;         /* start char number in string (unused if NOWRAP) */
  LONG charlen;         /* string lenght (unused if NOWRAP) */
};

#define NOWRAP          0x00
#define WRAPCOL0        0x01
#define WRAPCOL1        0x02
#define WRAPCOL2        0x04
#define WRAPCOL3        0x08
#define WRAPCOL4        0x10
#define WRAPCOL5        0x20
#define WRAPCOL6        0x40
#define WRAPPED         0x80


struct MUI_NList_GetSelectInfo
{
  LONG start;        /* num of first selected *REAL* entry/line (first of wrapped from which start is issued) */
  LONG end;          /* num of last selected *REAL* entry/line (first of wrapped from which start is issued) */
  LONG num;          /* not used */
  LONG start_column; /* column of start of selection in 'start' entry */
  LONG end_column;   /* column of end of selection in 'end' entry */
  LONG start_pos;    /* char pos of start of selection in 'start_column' entry */
  LONG end_pos;      /* char pos of end of selection in 'end_column' entry */
  LONG vstart;       /* num of first visually selected entry */
  LONG vend;         /* num of last visually selected entry */
  LONG vnum;         /* number of visually selected entries */
};
/* NOTE that vstart==start, vend==end in all cases if no wrapping is used */

/* Methods */

#define MUIM_NList_Clear              0x9d510070UL /* GM */
#define MUIM_NList_CreateImage        0x9d510071UL /* GM */
#define MUIM_NList_DeleteImage        0x9d510072UL /* GM */
#define MUIM_NList_Exchange           0x9d510073UL /* GM */
#define MUIM_NList_GetEntry           0x9d510074UL /* GM */
#define MUIM_NList_Insert             0x9d510075UL /* GM */
#define MUIM_NList_InsertSingle       0x9d510076UL /* GM */
#define MUIM_NList_Jump               0x9d510077UL /* GM */
#define MUIM_NList_Move               0x9d510078UL /* GM */
#define MUIM_NList_NextSelected       0x9d510079UL /* GM */
#define MUIM_NList_Redraw             0x9d51007aUL /* GM */
#define MUIM_NList_Remove             0x9d51007bUL /* GM */
#define MUIM_NList_Select             0x9d51007cUL /* GM */
#define MUIM_NList_Sort               0x9d51007dUL /* GM */
#define MUIM_NList_TestPos            0x9d51007eUL /* GM */
#define MUIM_NList_CopyToClip         0x9d51007fUL /* GM */
#define MUIM_NList_UseImage           0x9d510080UL /* GM */
#define MUIM_NList_ReplaceSingle      0x9d510081UL /* GM */
#define MUIM_NList_InsertWrap         0x9d510082UL /* GM */
#define MUIM_NList_InsertSingleWrap   0x9d510083UL /* GM */
#define MUIM_NList_GetEntryInfo       0x9d510084UL /* GM */
#define MUIM_NList_QueryBeginning     0x9d510085UL /* Obsolete */
#define MUIM_NList_GetSelectInfo      0x9d510086UL /* GM */
#define MUIM_NList_CopyTo             0x9d510087UL /* GM */
#define MUIM_NList_DropType           0x9d510088UL /* GM */
#define MUIM_NList_DropDraw           0x9d510089UL /* GM */
#define MUIM_NList_RedrawEntry        0x9d51008aUL /* GM */
#define MUIM_NList_DoMethod           0x9d51008bUL /* GM */
#define MUIM_NList_ColWidth           0x9d51008cUL /* GM */
#define MUIM_NList_ContextMenuBuild   0x9d51008dUL /* GM */
#define MUIM_NList_DropEntryDrawErase 0x9d51008eUL /* GM */
#define MUIM_NList_ColToColumn        0x9d51008fUL /* GM */
#define MUIM_NList_ColumnToCol        0x9d510091UL /* GM */
#define MUIM_NList_Sort2              0x9d510092UL /* GM */
#define MUIM_NList_PrevSelected       0x9d510093UL /* GM */
#define MUIM_NList_SetColumnCol       0x9d510094UL /* GM */
#define MUIM_NList_Sort3              0x9d510095UL /* GM */
#define MUIM_NList_GetPos             0x9d510096UL /* GM */
#define MUIM_NList_SelectChange       0x9d5100A0UL /* GM */
#define MUIM_NList_Construct          0x9d5100A1UL /* GM */
#define MUIM_NList_Destruct           0x9d5100A2UL /* GM */
#define MUIM_NList_Compare            0x9d5100A3UL /* GM */
#define MUIM_NList_Display            0x9d5100A4UL /* GM */
#define MUIM_NList_GoActive           0x9d5100A5UL /* GM */
#define MUIM_NList_GoInactive         0x9d5100A6UL /* GM */
#define MUIM_NList_SetActive          0x9d5100A7UL /* GM */

/*
for future extensions, skip 0x9d5100AF as method ID, this one is already used by NFloattext
*/

struct MUIP_NList_Clear              { STACKED ULONG MethodID; };
struct MUIP_NList_CreateImage        { STACKED ULONG MethodID; STACKED Object *obj; STACKED ULONG flags; };
struct MUIP_NList_DeleteImage        { STACKED ULONG MethodID; STACKED APTR listimg; };
struct MUIP_NList_Exchange           { STACKED ULONG MethodID; STACKED LONG pos1; STACKED LONG pos2; };
struct MUIP_NList_GetEntry           { STACKED ULONG MethodID; STACKED LONG pos; STACKED APTR *entry; };
struct MUIP_NList_Insert             { STACKED ULONG MethodID; STACKED APTR *entries; STACKED LONG count; STACKED LONG pos; STACKED ULONG flags; };
struct MUIP_NList_InsertSingle       { STACKED ULONG MethodID; STACKED APTR entry; STACKED LONG pos; };
struct MUIP_NList_Jump               { STACKED ULONG MethodID; STACKED LONG pos; };
struct MUIP_NList_Move               { STACKED ULONG MethodID; STACKED LONG from; STACKED LONG to; };
struct MUIP_NList_NextSelected       { STACKED ULONG MethodID; STACKED LONG *pos; };
struct MUIP_NList_Redraw             { STACKED ULONG MethodID; STACKED LONG pos; };
struct MUIP_NList_Remove             { STACKED ULONG MethodID; STACKED LONG pos; };
struct MUIP_NList_Select             { STACKED ULONG MethodID; STACKED LONG pos; STACKED LONG seltype; STACKED LONG *state; };
struct MUIP_NList_Sort               { STACKED ULONG MethodID; };
struct MUIP_NList_TestPos            { STACKED ULONG MethodID; STACKED LONG x; STACKED LONG y; STACKED struct MUI_NList_TestPos_Result *res; };
struct MUIP_NList_CopyToClip         { STACKED ULONG MethodID; STACKED LONG pos; STACKED ULONG clipnum; STACKED APTR *entries; STACKED struct Hook *hook; };
struct MUIP_NList_UseImage           { STACKED ULONG MethodID; STACKED Object *obj; STACKED LONG imgnum; STACKED ULONG flags; };
struct MUIP_NList_ReplaceSingle      { STACKED ULONG MethodID; STACKED APTR entry; STACKED LONG pos; STACKED LONG wrapcol; STACKED LONG align; };
struct MUIP_NList_InsertWrap         { STACKED ULONG MethodID; STACKED APTR *entries; STACKED LONG count; STACKED LONG pos; STACKED LONG wrapcol; STACKED LONG align; STACKED ULONG flags; };
struct MUIP_NList_InsertSingleWrap   { STACKED ULONG MethodID; STACKED APTR entry; STACKED LONG pos; STACKED LONG wrapcol; STACKED LONG align; };
struct MUIP_NList_GetEntryInfo       { STACKED ULONG MethodID; STACKED struct MUI_NList_GetEntryInfo *res; };
struct MUIP_NList_QueryBeginning     { STACKED ULONG MethodID; };
struct MUIP_NList_GetSelectInfo      { STACKED ULONG MethodID; STACKED struct MUI_NList_GetSelectInfo *res; };
struct MUIP_NList_CopyTo             { STACKED ULONG MethodID; STACKED LONG pos; STACKED char *filename; STACKED APTR *result; STACKED APTR *entries; };
struct MUIP_NList_DropType           { STACKED ULONG MethodID; STACKED LONG *pos; STACKED LONG *type; STACKED LONG minx, maxx, miny, maxy; STACKED LONG mousex, mousey; };
struct MUIP_NList_DropDraw           { STACKED ULONG MethodID; STACKED LONG pos; STACKED LONG type; STACKED LONG minx, maxx, miny, maxy; };
struct MUIP_NList_RedrawEntry        { STACKED ULONG MethodID; STACKED APTR entry; };
struct MUIP_NList_DoMethod           { STACKED ULONG MethodID; STACKED LONG pos; STACKED APTR DestObj; STACKED ULONG FollowParams; /* ... */  };
struct MUIP_NList_ColWidth           { STACKED ULONG MethodID; STACKED LONG col; STACKED LONG width; };
struct MUIP_NList_ContextMenuBuild   { STACKED ULONG MethodID; STACKED LONG mx; STACKED LONG my; STACKED LONG pos; STACKED LONG column; STACKED LONG flags; STACKED LONG ontop; };
struct MUIP_NList_DropEntryDrawErase { STACKED ULONG MethodID; STACKED LONG type; STACKED LONG drawpos; STACKED LONG erasepos; };
struct MUIP_NList_ColToColumn        { STACKED ULONG MethodID; STACKED LONG col; };
struct MUIP_NList_ColumnToCol        { STACKED ULONG MethodID; STACKED LONG column; };
struct MUIP_NList_Sort2              { STACKED ULONG MethodID; STACKED LONG sort_type; STACKED LONG sort_type_add; };
struct MUIP_NList_PrevSelected       { STACKED ULONG MethodID; STACKED LONG *pos; };
struct MUIP_NList_SetColumnCol       { STACKED ULONG MethodID; STACKED LONG column; STACKED LONG col; };
struct MUIP_NList_Sort3              { STACKED ULONG MethodID; STACKED LONG sort_type; STACKED LONG sort_type_add; STACKED LONG which; };
struct MUIP_NList_GetPos             { STACKED ULONG MethodID; STACKED APTR entry; STACKED LONG *pos; };
struct MUIP_NList_SelectChange       { STACKED ULONG MethodID; STACKED LONG pos; STACKED LONG state; STACKED ULONG flags; };
struct MUIP_NList_Construct          { STACKED ULONG MethodID; STACKED APTR entry; STACKED APTR pool; };
struct MUIP_NList_Destruct           { STACKED ULONG MethodID; STACKED APTR entry; STACKED APTR pool; };
struct MUIP_NList_Compare            { STACKED ULONG MethodID; STACKED APTR entry1; STACKED APTR entry2; STACKED LONG sort_type1; STACKED LONG sort_type2; };
struct MUIP_NList_Display            { STACKED ULONG MethodID; STACKED APTR entry; STACKED LONG entry_pos; STACKED STRPTR *strings; STACKED STRPTR *preparses; };
struct MUIP_NList_GoActive           { STACKED ULONG MethodID; };
struct MUIP_NList_GoInactive         { STACKED ULONG MethodID; };
struct MUIP_NList_SetActive          { STACKED ULONG MethodID; STACKED SIPTR pos; STACKED ULONG flags; };

#define DISPLAY_ARRAY_MAX 64

#define ALIGN_LEFT      0x0000
#define ALIGN_CENTER    0x0100
#define ALIGN_RIGHT     0x0200
#define ALIGN_JUSTIFY   0x0400


/*  Be carrefull ! the 'sort_type2' member don't exist in releases before 19.96
 *  where MUIM_NList_Sort3, MUIA_NList_SortType2, MUIA_NList_TitleClick2 and
 *  MUIA_NList_TitleMark2 have appeared !
 *  You can safely use get(obj,MUIA_NList_SortType2,&st2) instead if you are not
 *  sure of the NList.mcc release which is used.
 */
struct NList_CompareMessage
{
  STACKED APTR entry1;
  STACKED APTR entry2;
  STACKED LONG sort_type;
  STACKED LONG sort_type2;
};

struct NList_ConstructMessage
{
  STACKED APTR entry;
  STACKED APTR pool;
};

struct NList_DestructMessage
{
  STACKED APTR entry;
  STACKED APTR pool;
};

struct NList_DisplayMessage
{
  STACKED APTR entry;
  STACKED LONG entry_pos;
  STACKED char *strings[DISPLAY_ARRAY_MAX];
  STACKED char *preparses[DISPLAY_ARRAY_MAX];
};

struct NList_CopyEntryToClipMessage
{
  STACKED APTR entry;
  STACKED LONG entry_pos;
  STACKED char *str_result;
  STACKED LONG column1;
  STACKED LONG column1_pos;
  STACKED LONG column2;
  STACKED LONG column2_pos;
  STACKED LONG column1_pos_type;
  STACKED LONG column2_pos_type;
};

struct NList_CopyColumnToClipMessage
{
  STACKED char *string;
  STACKED LONG entry_pos;
  STACKED char *str_result;
  STACKED LONG str_pos1;
  STACKED LONG str_pos2;
};

#if !defined(__AROS__) && defined(__PPC__)
  #if defined(__GNUC__)
    #pragma pack()
  #elif defined(__VBCC__)
    #pragma default-align
  #endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* MUI_NList_MCC_H */
