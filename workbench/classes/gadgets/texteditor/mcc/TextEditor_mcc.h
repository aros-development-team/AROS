/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by TextEditor.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 $Id: TextEditor_mcc.h,v 1.3 2005/06/04 15:50:30 damato Exp $

***************************************************************************/

#ifndef TEXTEDITOR_MCC_H
#define TEXTEDITOR_MCC_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
  #ifdef __PPC__
    #pragma pack(2)
  #endif
#elif defined(__VBCC__)
  #pragma amiga-align
#endif

#define MUIC_TextEditor     "TextEditor.mcc"
#define TextEditorObject    MUI_NewObject(MUIC_TextEditor

#ifdef ClassAct
#define TextEditor_Dummy   (((ULONG)(1L<<31)) + 0x5000000 + 0x26000) // ClassAct
#else
#define TextEditor_Dummy   (0xad000000) // MUI
#endif

#define MUIA_TextEditor_AreaMarked        (TextEditor_Dummy + 0x14)
#define MUIA_TextEditor_ColorMap          (TextEditor_Dummy + 0x2f)
#define MUIA_TextEditor_Contents          (TextEditor_Dummy + 0x02)
#define MUIA_TextEditor_CursorX           (TextEditor_Dummy + 0x04)
#define MUIA_TextEditor_CursorY           (TextEditor_Dummy + 0x05)
#define MUIA_TextEditor_DoubleClickHook   (TextEditor_Dummy + 0x06)
#define MUIA_TextEditor_ExportHook        (TextEditor_Dummy + 0x08)
#define MUIA_TextEditor_ExportWrap        (TextEditor_Dummy + 0x09)
#define MUIA_TextEditor_FixedFont         (TextEditor_Dummy + 0x0a)
#define MUIA_TextEditor_Flow              (TextEditor_Dummy + 0x0b)
#define MUIA_TextEditor_HasChanged        (TextEditor_Dummy + 0x0c)
#define MUIA_TextEditor_HorizontalScroll  (TextEditor_Dummy + 0x2d) /* Private and experimental! */
#define MUIA_TextEditor_ImportHook        (TextEditor_Dummy + 0x0e)
#define MUIA_TextEditor_ImportWrap        (TextEditor_Dummy + 0x10)
#define MUIA_TextEditor_InsertMode        (TextEditor_Dummy + 0x0f)
#define MUIA_TextEditor_InVirtualGroup    (TextEditor_Dummy + 0x1b)
#define MUIA_TextEditor_KeyBindings       (TextEditor_Dummy + 0x11)
#define MUIA_TextEditor_MultiColorQuoting (TextEditor_Dummy + 0x31)
#define MUIA_TextEditor_NumLock           (TextEditor_Dummy + 0x18)
#define MUIA_TextEditor_Pen               (TextEditor_Dummy + 0x2e)
#define MUIA_TextEditor_PopWindow_Open    (TextEditor_Dummy + 0x03) /* Private!!! */
#define MUIA_TextEditor_Prop_DeltaFactor  (TextEditor_Dummy + 0x0d)
#define MUIA_TextEditor_Prop_Entries      (TextEditor_Dummy + 0x15)
#define MUIA_TextEditor_Prop_First        (TextEditor_Dummy + 0x20)
#define MUIA_TextEditor_Prop_Release      (TextEditor_Dummy + 0x01) /* Private!!! */
#define MUIA_TextEditor_Prop_Visible      (TextEditor_Dummy + 0x16)
#define MUIA_TextEditor_Quiet             (TextEditor_Dummy + 0x17)
#define MUIA_TextEditor_ReadOnly          (TextEditor_Dummy + 0x19)
#define MUIA_TextEditor_RedoAvailable     (TextEditor_Dummy + 0x13)
#define MUIA_TextEditor_Separator         (TextEditor_Dummy + 0x2c)
#define MUIA_TextEditor_Slider            (TextEditor_Dummy + 0x1a)
#define MUIA_TextEditor_StyleBold         (TextEditor_Dummy + 0x1c)
#define MUIA_TextEditor_StyleItalic       (TextEditor_Dummy + 0x1d)
#define MUIA_TextEditor_StyleUnderline    (TextEditor_Dummy + 0x1e)
#define MUIA_TextEditor_TypeAndSpell      (TextEditor_Dummy + 0x07)
#define MUIA_TextEditor_UndoAvailable     (TextEditor_Dummy + 0x12)
#define MUIA_TextEditor_WrapBorder        (TextEditor_Dummy + 0x21)
#define MUIA_TextEditor_Rows              (TextEditor_Dummy + 0x32)
#define MUIA_TextEditor_Columns           (TextEditor_Dummy + 0x33)
#define MUIA_TextEditor_AutoClip          (TextEditor_Dummy + 0x34)
#define MUIA_TextEditor_CursorPosition    (TextEditor_Dummy + 0x35)
#define MUIA_TextEditor_KeyUpFocus        (TextEditor_Dummy + 0x36)

#ifdef ClassAct
#undef TextEditor_Dummy
#define TextEditor_Dummy   (0x45000)
#endif

#define MUIM_TextEditor_AddKeyBindings    (TextEditor_Dummy + 0x22)
#define MUIM_TextEditor_ARexxCmd          (TextEditor_Dummy + 0x23)
#define MUIM_TextEditor_BlockInfo         (TextEditor_Dummy + 0x30)
#define MUIM_TextEditor_ClearText         (TextEditor_Dummy + 0x24)
#define MUIM_TextEditor_ExportText        (TextEditor_Dummy + 0x25)
#define MUIM_TextEditor_HandleError       (TextEditor_Dummy + 0x1f)
#define MUIM_TextEditor_InsertText        (TextEditor_Dummy + 0x26)
#define MUIM_TextEditor_MacroBegin        (TextEditor_Dummy + 0x27)
#define MUIM_TextEditor_MacroEnd          (TextEditor_Dummy + 0x28)
#define MUIM_TextEditor_MacroExecute      (TextEditor_Dummy + 0x29)
#define MUIM_TextEditor_MarkText          (TextEditor_Dummy + 0x2c)
#define MUIM_TextEditor_Replace           (TextEditor_Dummy + 0x2a)
#define MUIM_TextEditor_Search            (TextEditor_Dummy + 0x2b)

#ifdef ClassAct
struct MUIP_TextEditor_ARexxCmd          { ULONG MethodID; struct GadgetInfo *GInfo; STRPTR command; };
struct MUIP_TextEditor_BlockInfo         { ULONG MethodID; struct GadgetInfo *GInfo; ULONG *startx; ULONG *starty; ULONG *stopx; ULONG *stopy; };
struct MUIP_TextEditor_ClearText         { ULONG MethodID; struct GadgetInfo *GInfo; };
struct MUIP_TextEditor_ExportText        { ULONG MethodID; struct GadgetInfo *GInfo; };
struct MUIP_TextEditor_HandleError       { ULONG MethodID; struct GadgetInfo *GInfo; ULONG errorcode; }; /* See below for error codes */
struct MUIP_TextEditor_InsertText        { ULONG MethodID; struct GadgetInfo *GInfo; STRPTR text; LONG pos; }; /* See below for positions */
struct MUIP_TextEditor_MarkText          { ULONG MethodID; struct GadgetInfo *GInfo; ULONG start_crsr_x; ULONG start_crsr_y; ULONG stop_crsr_x; ULONG stop_crsr_y; };
struct MUIP_TextEditor_Search            { ULONG MethodID; struct GadgetInfo *GInfo; STRPTR SearchString; ULONG Flags; };
struct MUIP_TextEditor_Replace           { ULONG MethodID; struct GadgetInfo *GInfo; STRPTR NewString; ULONG Flags; };
#else
struct MUIP_TextEditor_ARexxCmd          { ULONG MethodID; STRPTR command; };
struct MUIP_TextEditor_BlockInfo         { ULONG MethodID; ULONG *startx; ULONG *starty; ULONG *stopx; ULONG *stopy; };
struct MUIP_TextEditor_ClearText         { ULONG MethodID; };
struct MUIP_TextEditor_ExportText        { ULONG MethodID; };
struct MUIP_TextEditor_HandleError       { ULONG MethodID; ULONG errorcode; }; /* See below for error codes */
struct MUIP_TextEditor_InsertText        { ULONG MethodID; STRPTR text; LONG pos; }; /* See below for positions */
struct MUIP_TextEditor_MarkText          { ULONG MethodID; ULONG start_crsr_x; ULONG start_crsr_y; ULONG stop_crsr_x; ULONG stop_crsr_y; };
struct MUIP_TextEditor_Search            { ULONG MethodID; STRPTR SearchString; ULONG Flags; };
struct MUIP_TextEditor_Replace           { ULONG MethodID; STRPTR NewString; ULONG Flags; };
#endif

#define MUIF_TextEditor_Search_FromTop       (1 << 0)
#define MUIF_TextEditor_Search_Next          (1 << 1)
#define MUIF_TextEditor_Search_CaseSensitive (1 << 2)
#define MUIF_TextEditor_Search_DOSPattern    (1 << 3)
#define MUIF_TextEditor_Search_Backwards     (1 << 4)

#define MUIV_TextEditor_ExportHook_Plain       0x00000000
#define MUIV_TextEditor_ExportHook_EMail       0x00000001

#define MUIV_TextEditor_Flow_Left              0x00000000
#define MUIV_TextEditor_Flow_Center            0x00000001
#define MUIV_TextEditor_Flow_Right             0x00000002
#define MUIV_TextEditor_Flow_Justified         0x00000003

#define MUIV_TextEditor_ImportHook_Plain       0x00000000
#define MUIV_TextEditor_ImportHook_EMail       0x00000002
#define MUIV_TextEditor_ImportHook_MIME        0x00000003
#define MUIV_TextEditor_ImportHook_MIMEQuoted  0x00000004

#define MUIV_TextEditor_InsertText_Cursor      0x00000000
#define MUIV_TextEditor_InsertText_Top         0x00000001
#define MUIV_TextEditor_InsertText_Bottom      0x00000002

#define MUIV_TextEditor_LengthHook_Plain       0x00000000
#define MUIV_TextEditor_LengthHook_ANSI        0x00000001
#define MUIV_TextEditor_LengthHook_HTML        0x00000002
#define MUIV_TextEditor_LengthHook_MAIL        0x00000003

/* Error codes given as argument to MUIM_TextEditor_HandleError */
#define Error_ClipboardIsEmpty         0x01
#define Error_ClipboardIsNotFTXT       0x02
#define Error_MacroBufferIsFull        0x03
#define Error_MemoryAllocationFailed   0x04
#define Error_NoAreaMarked             0x05
#define Error_NoMacroDefined           0x06
#define Error_NothingToRedo            0x07
#define Error_NothingToUndo            0x08
#define Error_NotEnoughUndoMem         0x09 /* This will cause all the stored undos to be freed */
#define Error_StringNotFound           0x0a
#define Error_NoBookmarkInstalled      0x0b
#define Error_BookmarkHasBeenLost      0x0c

struct ClickMessage
{
  STRPTR  LineContents;  /* This field is ReadOnly!!! */
  ULONG   ClickPosition;
};

struct ExportMessage
{
  APTR     UserData;     /* This is set to what your hook returns (NULL the first time) */
  STRPTR   Contents;     /* Pointer to the current line */
  ULONG    Length;       /* Length of Contents, including the '\n' character */
  UWORD    *Styles;      /* Pointer to array of words */
  UWORD    *Colors;
  BOOL   Highlight;
  UWORD    Flow;         /* Current lines flow */
  UWORD    Separator;    /* Se definitions bellow */
  ULONG    ExportWrap;   /* For your use only (reflects MUIA_TextEditor_ExportWrap) */
  BOOL     Last;         /* Set to TRUE if this is the last line */
  APTR   data;           /* Private! */
};

struct ImportMessage
{
  STRPTR  Data;               /* The first time the hook is called, then this will be either the value of MUIA_TextEditor_Contents, or the argument given to MUIM_TextEditor_Insert. */
  struct  LineNode *linenode; /* Pointer to a linenode, which you should fill out */
  APTR    PoolHandle;         /* A poolhandle, all allocations done for styles or contents must be made from this pool, and the size of the allocation must be stored in the first LONG */
  ULONG   ImportWrap;         /* For your use only (reflects MUIA_TextEditor_ImportWrap) */
};

struct LineNode
{
  STRPTR   Contents;      /* Set this to the linecontents (allocated via the poolhandle) */
  ULONG    Length;        /* The length of the line (including the '\n') */
  UWORD    *Styles;       /* Set this to the styles used for this line (allocated via the poolhandle) the format is: pos,style,pos,style,...,-1,0*/
  UWORD    *Colors;       /* The colors to use (allocated via the poolhandle) the format is: pos,color,pos,color,...,-1,-0 */
  BOOL     Color;         /* Set this to TRUE if you want the line to be highlighted */
  UWORD    Flow;          /* Use the MUIV_TextEditor_Flow_xxx values... */
  UWORD    Separator;     /* See definitions below */
  BOOL     clearFlow;     /* if the flow definition should be cleared on the next line */
};

/* Definitions for Separator type */

#define LNSB_Top             0 /* Mutual exclude: */
#define LNSB_Middle          1 /* Placement of    */
#define LNSB_Bottom          2 /*  the separator  */
#define LNSB_StrikeThru      3 /* Let separator go thru the textfont */
#define LNSB_Thick           4 /* Extra thick separator */

#define LNSF_Top             (1<<LNSB_Top)
#define LNSF_Middle          (1<<LNSB_Middle)
#define LNSF_Bottom          (1<<LNSB_Bottom)
#define LNSF_StrikeThru      (1<<LNSB_StrikeThru)
#define LNSF_Thick           (1<<LNSB_Thick)

#ifdef __GNUC__
  #ifdef __PPC__
    #pragma pack()
  #endif
#elif defined(__VBCC__)
  #pragma default-align
#endif

#ifdef __cplusplus
}
#endif

#endif /* TEXTEDITOR_MCC_H */
