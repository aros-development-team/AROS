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

#include <devices/inputevent.h>

#include "private.h"

// our default keybindings (every rawkey value with +500 is a vanilla key specification)
const struct te_key default_keybindings[] =
{
  { RAWKEY_CRSRUP,    0, MUIV_TextEditor_KeyAction_Up },
  { RAWKEY_CRSRUP,    IEQUALIFIER_ALT | IEQUALIFIER_CONTROL, MUIV_TextEditor_KeyAction_PrevLine },
  { RAWKEY_CRSRUP,    IEQUALIFIER_ALT, MUIV_TextEditor_KeyAction_PageUp },
  { RAWKEY_CRSRUP,    IEQUALIFIER_CONTROL, MUIV_TextEditor_KeyAction_Top },
  { RAWKEY_CRSRDOWN,  0, MUIV_TextEditor_KeyAction_Down },
  { RAWKEY_CRSRDOWN,  IEQUALIFIER_ALT | IEQUALIFIER_CONTROL, MUIV_TextEditor_KeyAction_NextLine },
  { RAWKEY_CRSRDOWN,  IEQUALIFIER_ALT, MUIV_TextEditor_KeyAction_PageDown },
  { RAWKEY_CRSRDOWN,  IEQUALIFIER_CONTROL, MUIV_TextEditor_KeyAction_Bottom },
  { RAWKEY_CRSRRIGHT, 0, MUIV_TextEditor_KeyAction_Right },
  { RAWKEY_CRSRRIGHT, IEQUALIFIER_ALT, MUIV_TextEditor_KeyAction_NextWord },
  { RAWKEY_CRSRRIGHT, IEQUALIFIER_CONTROL, MUIV_TextEditor_KeyAction_EndOfLine },
  { RAWKEY_CRSRRIGHT, IEQUALIFIER_ALT | IEQUALIFIER_CONTROL, MUIV_TextEditor_KeyAction_NextSentence },
  { RAWKEY_CRSRLEFT,  0, MUIV_TextEditor_KeyAction_Left },
  { RAWKEY_CRSRLEFT,  IEQUALIFIER_ALT, MUIV_TextEditor_KeyAction_PrevWord },
  { RAWKEY_CRSRLEFT,  IEQUALIFIER_CONTROL, MUIV_TextEditor_KeyAction_StartOfLine },
  { RAWKEY_CRSRLEFT,  IEQUALIFIER_ALT | IEQUALIFIER_CONTROL, MUIV_TextEditor_KeyAction_PrevSentence },

  { 8+500, 0, MUIV_TextEditor_KeyAction_Backspace },
  { 0x7f+500, 0, MUIV_TextEditor_KeyAction_Delete },
  { 13+500, 0, MUIV_TextEditor_KeyAction_Return },
  { 13+500, IEQUALIFIER_NUMERICPAD, MUIV_TextEditor_KeyAction_Return },
  { 9+500, 0, MUIV_TextEditor_KeyAction_Tab },
  { 'x'+500, IEQUALIFIER_RCOMMAND, MUIV_TextEditor_KeyAction_Cut },
  { 'c'+500, IEQUALIFIER_RCOMMAND, MUIV_TextEditor_KeyAction_Copy },
  { 'v'+500, IEQUALIFIER_RCOMMAND, MUIV_TextEditor_KeyAction_Paste },
  { 'z'+500, IEQUALIFIER_RCOMMAND, MUIV_TextEditor_KeyAction_Undo },
  { 'z'+500, IEQUALIFIER_RCOMMAND | IEQUALIFIER_SHIFT, MUIV_TextEditor_KeyAction_Redo },
  { RAWKEY_HELP, IEQUALIFIER_COMMAND, MUIV_TextEditor_KeyAction_SuggestWord },

  { 8+500, IEQUALIFIER_CONTROL, MUIV_TextEditor_KeyAction_DelBOL },
  { 8+500, IEQUALIFIER_SHIFT, MUIV_TextEditor_KeyAction_DelBOL },
  { 0x7f+500, IEQUALIFIER_CONTROL, MUIV_TextEditor_KeyAction_DelEOL },
  { 0x7f+500, IEQUALIFIER_SHIFT, MUIV_TextEditor_KeyAction_DelEOL },
  { 8+500, IEQUALIFIER_ALT, MUIV_TextEditor_KeyAction_DelBOW },
  { 0x7f+500, IEQUALIFIER_ALT, MUIV_TextEditor_KeyAction_DelEOW },
  { 'x'+500, IEQUALIFIER_CONTROL, MUIV_TextEditor_KeyAction_DelLine },
  { 0x9+500, IEQUALIFIER_RCOMMAND, MUIV_TextEditor_KeyAction_NextGadget },

  { '1'+500, IEQUALIFIER_RCOMMAND | IEQUALIFIER_SHIFT, MUIV_TextEditor_KeyAction_SetBookmark1 },
  { '2'+500, IEQUALIFIER_RCOMMAND | IEQUALIFIER_SHIFT, MUIV_TextEditor_KeyAction_SetBookmark2 },
  { '3'+500, IEQUALIFIER_RCOMMAND | IEQUALIFIER_SHIFT, MUIV_TextEditor_KeyAction_SetBookmark3 },

  { '1'+500, IEQUALIFIER_RCOMMAND, MUIV_TextEditor_KeyAction_GotoBookmark1 },
  { '2'+500, IEQUALIFIER_RCOMMAND, MUIV_TextEditor_KeyAction_GotoBookmark2 },
  { '3'+500, IEQUALIFIER_RCOMMAND, MUIV_TextEditor_KeyAction_GotoBookmark3 },

  { RAWKEY_PAGEUP,   0, MUIV_TextEditor_KeyAction_PageUp },
  { RAWKEY_PAGEDOWN, 0, MUIV_TextEditor_KeyAction_PageDown },
  { RAWKEY_HOME,     0, MUIV_TextEditor_KeyAction_Top },
  { RAWKEY_END,      0, MUIV_TextEditor_KeyAction_Bottom },

  { 'a'+500, IEQUALIFIER_RCOMMAND, MUIV_TextEditor_KeyAction_SelectAll },
  { 'a'+500, IEQUALIFIER_RCOMMAND | IEQUALIFIER_SHIFT, MUIV_TextEditor_KeyAction_SelectNone },
  { -1, 0, 0 }
};
