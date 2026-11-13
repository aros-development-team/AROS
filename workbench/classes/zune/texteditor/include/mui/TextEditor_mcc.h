#ifndef TEXTEDITOR_MCC_H
#define TEXTEDITOR_MCC_H

/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2014 TextEditor.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 $Id$

***************************************************************************/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
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

/***********************************************************************/

#define MUIC_TextEditor     "TextEditor.mcc"

#if defined(__AROS__) && !defined(NO_INLINE_STDARG)
#define TextEditorObject    MUIOBJMACRO_START(MUIC_TextEditor)
#else
#define TextEditorObject    MUI_NewObject(MUIC_TextEditor
#endif

#define TextEditor_Dummy   (0xad000000UL)

#define MUIA_TextEditor_Contents              (TextEditor_Dummy + 0x02)
#define MUIA_TextEditor_CursorX               (TextEditor_Dummy + 0x04)
#define MUIA_TextEditor_CursorY               (TextEditor_Dummy + 0x05)
#define MUIA_TextEditor_DoubleClickHook       (TextEditor_Dummy + 0x06)
#define MUIA_TextEditor_TypeAndSpell          (TextEditor_Dummy + 0x07)
#define MUIA_TextEditor_ExportHook            (TextEditor_Dummy + 0x08)
#define MUIA_TextEditor_ExportWrap            (TextEditor_Dummy + 0x09)
#define MUIA_TextEditor_FixedFont             (TextEditor_Dummy + 0x0a)
#define MUIA_TextEditor_Flow                  (TextEditor_Dummy + 0x0b)
#define MUIA_TextEditor_HasChanged            (TextEditor_Dummy + 0x0c)
#define MUIA_TextEditor_Prop_DeltaFactor      (TextEditor_Dummy + 0x0d)
#define MUIA_TextEditor_ImportHook            (TextEditor_Dummy + 0x0e)
#define MUIA_TextEditor_InsertMode            (TextEditor_Dummy + 0x0f)
#define MUIA_TextEditor_ImportWrap            (TextEditor_Dummy + 0x10)
#define MUIA_TextEditor_KeyBindings           (TextEditor_Dummy + 0x11)
#define MUIA_TextEditor_UndoAvailable         (TextEditor_Dummy + 0x12)
#define MUIA_TextEditor_RedoAvailable         (TextEditor_Dummy + 0x13)
#define MUIA_TextEditor_AreaMarked            (TextEditor_Dummy + 0x14)
#define MUIA_TextEditor_Prop_Entries          (TextEditor_Dummy + 0x15)
#define MUIA_TextEditor_Prop_Visible          (TextEditor_Dummy + 0x16)
#define MUIA_TextEditor_Quiet                 (TextEditor_Dummy + 0x17)
#define MUIA_TextEditor_NumLock               (TextEditor_Dummy + 0x18)
#define MUIA_TextEditor_ReadOnly              (TextEditor_Dummy + 0x19)
#define MUIA_TextEditor_Slider                (TextEditor_Dummy + 0x1a)
#define MUIA_TextEditor_InVirtualGroup        (TextEditor_Dummy + 0x1b)
#define MUIA_TextEditor_StyleBold             (TextEditor_Dummy + 0x1c)
#define MUIA_TextEditor_StyleItalic           (TextEditor_Dummy + 0x1d)
#define MUIA_TextEditor_StyleUnderline        (TextEditor_Dummy + 0x1e)
#define MUIA_TextEditor_Prop_First            (TextEditor_Dummy + 0x20)
#define MUIA_TextEditor_WrapBorder            (TextEditor_Dummy + 0x21)
#define MUIA_TextEditor_Separator             (TextEditor_Dummy + 0x2c)
#define MUIA_TextEditor_Pen                   (TextEditor_Dummy + 0x2e)
#define MUIA_TextEditor_ColorMap              (TextEditor_Dummy + 0x2f)
#define MUIA_TextEditor_MultiColorQuoting     (TextEditor_Dummy + 0x31)
#define MUIA_TextEditor_Rows                  (TextEditor_Dummy + 0x32)
#define MUIA_TextEditor_Columns               (TextEditor_Dummy + 0x33)
#define MUIA_TextEditor_AutoClip              (TextEditor_Dummy + 0x34)
#define MUIA_TextEditor_CursorPosition        (TextEditor_Dummy + 0x35)
#define MUIA_TextEditor_KeyUpFocus            (TextEditor_Dummy + 0x36)
#define MUIA_TextEditor_UndoLevels            (TextEditor_Dummy + 0x38)
#define MUIA_TextEditor_WrapMode              (TextEditor_Dummy + 0x39)
#define MUIA_TextEditor_ActiveObjectOnClick   (TextEditor_Dummy + 0x3a)
#define MUIA_TextEditor_PasteStyles           (TextEditor_Dummy + 0x3b)
#define MUIA_TextEditor_PasteColors           (TextEditor_Dummy + 0x3c)
#define MUIA_TextEditor_ConvertTabs           (TextEditor_Dummy + 0x3d)
#define MUIA_TextEditor_WrapWords             (TextEditor_Dummy + 0x3e)
#define MUIA_TextEditor_TabSize               (TextEditor_Dummy + 0x3f)
#define MUIA_TextEditor_Keywords              (TextEditor_Dummy + 0x40)
#define MUIA_TextEditor_MatchedKeyword        (TextEditor_Dummy + 0x41)

#define MUIM_TextEditor_HandleError           (TextEditor_Dummy + 0x1f)
#define MUIM_TextEditor_AddKeyBindings        (TextEditor_Dummy + 0x22)
#define MUIM_TextEditor_ARexxCmd              (TextEditor_Dummy + 0x23)
#define MUIM_TextEditor_ClearText             (TextEditor_Dummy + 0x24)
#define MUIM_TextEditor_ExportText            (TextEditor_Dummy + 0x25)
#define MUIM_TextEditor_InsertText            (TextEditor_Dummy + 0x26)
#define MUIM_TextEditor_MacroBegin            (TextEditor_Dummy + 0x27)
#define MUIM_TextEditor_MacroEnd              (TextEditor_Dummy + 0x28)
#define MUIM_TextEditor_MacroExecute          (TextEditor_Dummy + 0x29)
#define MUIM_TextEditor_Replace               (TextEditor_Dummy + 0x2a)
#define MUIM_TextEditor_Search                (TextEditor_Dummy + 0x2b)
#define MUIM_TextEditor_MarkText              (TextEditor_Dummy + 0x2c)
#define MUIM_TextEditor_QueryKeyAction        (TextEditor_Dummy + 0x2d)
#define MUIM_TextEditor_SetBlock              (TextEditor_Dummy + 0x2e)
#define MUIM_TextEditor_BlockInfo             (TextEditor_Dummy + 0x30)
#define MUIM_TextEditor_ExportBlock           (TextEditor_Dummy + 0x37)

struct MUIP_TextEditor_ARexxCmd          { STACKED ULONG MethodID; STACKED STRPTR command; };
struct MUIP_TextEditor_BlockInfo         { STACKED ULONG MethodID; STACKED LONG *startx; STACKED LONG *starty; STACKED LONG *stopx; STACKED LONG *stopy; };
struct MUIP_TextEditor_ClearText         { STACKED ULONG MethodID; };
struct MUIP_TextEditor_ExportBlock       { STACKED ULONG MethodID; STACKED ULONG flags; STACKED LONG startx; STACKED LONG starty; STACKED LONG stopx; STACKED LONG stopy; };
struct MUIP_TextEditor_ExportText        { STACKED ULONG MethodID; };
struct MUIP_TextEditor_HandleError       { STACKED ULONG MethodID; STACKED ULONG errorcode; }; /* See below for error codes */
struct MUIP_TextEditor_InsertText        { STACKED ULONG MethodID; STACKED STRPTR text; STACKED LONG pos; }; /* See below for positions */
struct MUIP_TextEditor_Replace           { STACKED ULONG MethodID; STACKED STRPTR NewString; STACKED ULONG Flags; };
struct MUIP_TextEditor_Search            { STACKED ULONG MethodID; STACKED STRPTR SearchString; STACKED ULONG Flags; };
struct MUIP_TextEditor_MarkText          { STACKED ULONG MethodID; STACKED LONG start_crsr_x; STACKED LONG start_crsr_y; STACKED LONG stop_crsr_x; STACKED LONG stop_crsr_y; };
struct MUIP_TextEditor_QueryKeyAction    { STACKED ULONG MethodID; STACKED ULONG keyAction; };
struct MUIP_TextEditor_SetBlock          { STACKED ULONG MethodID; STACKED LONG startx; STACKED LONG starty; STACKED LONG stopx; STACKED LONG stopy; STACKED ULONG operation; STACKED ULONG value; };

#define MUIV_TextEditor_ExportHook_Plain       0x00000000UL
#define MUIV_TextEditor_ExportHook_EMail       0x00000001UL
#define MUIV_TextEditor_ExportHook_NoStyle     0x00000002UL

#define MUIV_TextEditor_Flow_Left              0x00000000UL
#define MUIV_TextEditor_Flow_Center            0x00000001UL
#define MUIV_TextEditor_Flow_Right             0x00000002UL
#define MUIV_TextEditor_Flow_Justified         0x00000003UL

#define MUIV_TextEditor_ImportHook_Plain       0x00000000UL
#define MUIV_TextEditor_ImportHook_EMail       0x00000002UL
#define MUIV_TextEditor_ImportHook_MIME        0x00000003UL
#define MUIV_TextEditor_ImportHook_MIMEQuoted  0x00000004UL

#define MUIV_TextEditor_InsertText_Cursor      0x00000000UL
#define MUIV_TextEditor_InsertText_Top         0x00000001UL
#define MUIV_TextEditor_InsertText_Bottom      0x00000002UL

/* Values for MUIA_TextEditor_WrapMode */
#define MUIV_TextEditor_WrapMode_NoWrap        0x00000000UL
#define MUIV_TextEditor_WrapMode_SoftWrap      0x00000001UL
#define MUIV_TextEditor_WrapMode_HardWrap      0x00000002UL

/* Values for MUIA_TextEditor_TabSize */
#define MUIV_TextEditor_TabSize_Default        0

/* Values for MUIM_TextEditor_MarkText */
#define MUIV_TextEditor_MarkText_All           (-1)
#define MUIV_TextEditor_MarkText_None          (-1)

/* Values for MUIM_TextEditor_SetBlock */
#define MUIV_TextEditor_SetBlock_Min           (-1)
#define MUIV_TextEditor_SetBlock_Max           (-2)

/* Flags for MUIM_TextEditor_Search */
#define MUIF_TextEditor_Search_FromTop         (1 << 0)
#define MUIF_TextEditor_Search_Next            (1 << 1)
#define MUIF_TextEditor_Search_CaseSensitive   (1 << 2)
#define MUIF_TextEditor_Search_DOSPattern      (1 << 3)
#define MUIF_TextEditor_Search_Backwards       (1 << 4)

/* Flags for MUIM_TextEditor_ExportBlock */
#define MUIF_TextEditor_ExportBlock_FullLines  (1 << 0)
#define MUIF_TextEditor_ExportBlock_TakeBlock  (1 << 1)

/* Flags for MUIM_TextEditor_SetBlock */
#define MUIF_TextEditor_SetBlock_Color           (1 << 0)
#define MUIF_TextEditor_SetBlock_StyleBold       (1 << 1)
#define MUIF_TextEditor_SetBlock_StyleItalic     (1 << 2)
#define MUIF_TextEditor_SetBlock_StyleUnderline  (1 << 3)
#define MUIF_TextEditor_SetBlock_Flow            (1 << 4)

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
  STACKED STRPTR  LineContents;  /* This field is ReadOnly!!! */
  STACKED ULONG   ClickPosition;
  STACKED ULONG   Qualifier;     /* V15.26+, a possible qualifier that was pressed during the double click */
};

/* Definitions for Separator type */
#define LNSB_Top             0 /* Mutual exclude: */
#define LNSB_Middle          1 /* Placement of    */
#define LNSB_Bottom          2 /*  the separator  */
#define LNSB_StrikeThru      3 /* Let separator go thru the textfont */
#define LNSB_Thick           4 /* Extra thick separator */

#define LNSF_None            0
#define LNSF_Top             (1<<LNSB_Top)
#define LNSF_Middle          (1<<LNSB_Middle)
#define LNSF_Bottom          (1<<LNSB_Bottom)
#define LNSF_StrikeThru      (1<<LNSB_StrikeThru)
#define LNSF_Thick           (1<<LNSB_Thick)

/* Keyaction definitions */
#define MUIV_TextEditor_KeyAction_Up              0x00
#define MUIV_TextEditor_KeyAction_Down            0x01
#define MUIV_TextEditor_KeyAction_Left            0x02
#define MUIV_TextEditor_KeyAction_Right           0x03
#define MUIV_TextEditor_KeyAction_PageUp          0x04
#define MUIV_TextEditor_KeyAction_PageDown        0x05
#define MUIV_TextEditor_KeyAction_StartOfLine     0x06
#define MUIV_TextEditor_KeyAction_EndOfLine       0x07
#define MUIV_TextEditor_KeyAction_Top             0x08
#define MUIV_TextEditor_KeyAction_Bottom          0x09
#define MUIV_TextEditor_KeyAction_PrevWord        0x0a
#define MUIV_TextEditor_KeyAction_NextWord        0x0b
#define MUIV_TextEditor_KeyAction_PrevLine        0x0c
#define MUIV_TextEditor_KeyAction_NextLine        0x0d
#define MUIV_TextEditor_KeyAction_PrevSentence    0x0e
#define MUIV_TextEditor_KeyAction_NextSentence    0x0f
#define MUIV_TextEditor_KeyAction_SuggestWord     0x10
#define MUIV_TextEditor_KeyAction_Backspace       0x11
#define MUIV_TextEditor_KeyAction_Delete          0x12
#define MUIV_TextEditor_KeyAction_Return          0x13
#define MUIV_TextEditor_KeyAction_Tab             0x14
#define MUIV_TextEditor_KeyAction_Cut             0x15
#define MUIV_TextEditor_KeyAction_Copy            0x16
#define MUIV_TextEditor_KeyAction_Paste           0x17
#define MUIV_TextEditor_KeyAction_Undo            0x18
#define MUIV_TextEditor_KeyAction_Redo            0x19
#define MUIV_TextEditor_KeyAction_DelBOL          0x1a
#define MUIV_TextEditor_KeyAction_DelEOL          0x1b
#define MUIV_TextEditor_KeyAction_DelBOW          0x1c
#define MUIV_TextEditor_KeyAction_DelEOW          0x1d
#define MUIV_TextEditor_KeyAction_NextGadget      0x1e
#define MUIV_TextEditor_KeyAction_GotoBookmark1   0x1f
#define MUIV_TextEditor_KeyAction_GotoBookmark2   0x20
#define MUIV_TextEditor_KeyAction_GotoBookmark3   0x21
#define MUIV_TextEditor_KeyAction_SetBookmark1    0x22
#define MUIV_TextEditor_KeyAction_SetBookmark2    0x23
#define MUIV_TextEditor_KeyAction_SetBookmark3    0x24
#define MUIV_TextEditor_KeyAction_DelLine         0x25
#define MUIV_TextEditor_KeyAction_SelectAll       0x26
#define MUIV_TextEditor_KeyAction_SelectNone      0x27

/* result structure for MUIM_TextEditor_QueryKeyAction */
struct MUIP_TextEditor_Keybinding
{
  STACKED const UWORD code;       // the RAWKEY code      read only
  STACKED const ULONG qualifier;  // the Qualifier flags  read only
  STACKED const UWORD action;     // the keyaction        read only
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

#endif /* TEXTEDITOR_MCC_H */

