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

 $Id$

***************************************************************************/

#include <devices/inputevent.h>

#include "private.h"

const struct keybindings keys[] =
{
  { { 76, 0, mUp } },
  { { 76, IEQUALIFIER_ALT | IEQUALIFIER_CONTROL, mPreviousLine } },
  { { 76, IEQUALIFIER_ALT, mPreviousPage } },
  { { 76, IEQUALIFIER_CONTROL, mTop } },
  { { 77, 0, mDown } },
  { { 77, IEQUALIFIER_ALT | IEQUALIFIER_CONTROL, mNextLine } },
  { { 77, IEQUALIFIER_ALT, mNextPage } },
  { { 77, IEQUALIFIER_CONTROL, mBottom } },
  { { 78, 0, mRight } },
  { { 78, IEQUALIFIER_ALT, mNextWord } },
  { { 78, IEQUALIFIER_CONTROL, mEndOfLine } },
  { { 78, IEQUALIFIER_ALT | IEQUALIFIER_CONTROL, mNextSentence } },
  { { 79, 0, mLeft } },
  { { 79, IEQUALIFIER_ALT, mPreviousWord } },
  { { 79, IEQUALIFIER_CONTROL, mStartOfLine } },
  { { 79, IEQUALIFIER_ALT | IEQUALIFIER_CONTROL, mPreviousSentence } },
  { { 8+500, 0, kBackspace } },
  { { 0x7f+500, 0, kDelete } },
  { { 13+500, 0, kReturn } },
  { { 13+500, IEQUALIFIER_NUMERICPAD, kReturn } },
  { { 9+500, 0, kTab } },
  { { 'x'+500, IEQUALIFIER_RCOMMAND, kCut } },
  { { 'c'+500, IEQUALIFIER_RCOMMAND, kCopy } },
  { { 'v'+500, IEQUALIFIER_RCOMMAND, kPaste } },
  { { 'z'+500, IEQUALIFIER_RCOMMAND, kUndo } },
  { { 'z'+500, IEQUALIFIER_RCOMMAND | IEQUALIFIER_RSHIFT, kRedo } },
  { { 95, IEQUALIFIER_COMMAND, kSuggestWord } },
  { { 8+500, IEQUALIFIER_CONTROL, kDelBOL } },
  { { 8+500, IEQUALIFIER_SHIFT, kDelBOL } },
  { { 0x7f+500, IEQUALIFIER_CONTROL, kDelEOL } },
  { { 0x7f+500, IEQUALIFIER_SHIFT, kDelEOL } },
  { { 8+500, IEQUALIFIER_ALT, kDelBOW } },
  { { 0x7f+500, IEQUALIFIER_ALT, kDelEOW } },
  { { 'x'+500, IEQUALIFIER_CONTROL, kDelLine } },
  { { 0x9+500, IEQUALIFIER_ALT, kNextGadget } },
  { { '1'+500, IEQUALIFIER_RCOMMAND | IEQUALIFIER_RSHIFT, kSetBookmark1 } },
  { { '2'+500, IEQUALIFIER_RCOMMAND | IEQUALIFIER_RSHIFT, kSetBookmark2 } },
  { { '3'+500, IEQUALIFIER_RCOMMAND | IEQUALIFIER_RSHIFT, kSetBookmark3 } },
  { { '1'+500, IEQUALIFIER_RCOMMAND, kGotoBookmark1 } },
  { { '2'+500, IEQUALIFIER_RCOMMAND, kGotoBookmark2 } },
  { { '3'+500, IEQUALIFIER_RCOMMAND, kGotoBookmark3 } },
  { { (UWORD)-1, 0, 0 } }
};
