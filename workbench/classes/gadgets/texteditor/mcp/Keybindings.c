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

 $Id: Keybindings.c,v 1.1 2005/03/28 11:29:49 damato Exp $

***************************************************************************/

#include <devices/inputevent.h>

#include "private.h"

static struct te_key k01 = { 76, 0, mUp };
static struct te_key k02 = { 76, IEQUALIFIER_ALT | IEQUALIFIER_CONTROL, mPreviousLine };
static struct te_key k03 = { 76, IEQUALIFIER_ALT, mPreviousPage };
static struct te_key k04 = { 76, IEQUALIFIER_CONTROL, mTop };
static struct te_key k05 = { 77, 0, mDown };
static struct te_key k06 = { 77, IEQUALIFIER_ALT | IEQUALIFIER_CONTROL, mNextLine };
static struct te_key k07 = { 77, IEQUALIFIER_ALT, mNextPage };
static struct te_key k08 = { 77, IEQUALIFIER_CONTROL, mBottom };
static struct te_key k09 = { 78, 0, mRight };
static struct te_key k10 = { 78, IEQUALIFIER_ALT, mNextWord };
static struct te_key k11 = { 78, IEQUALIFIER_CONTROL, mEndOfLine };
static struct te_key k12 = { 78, IEQUALIFIER_ALT | IEQUALIFIER_CONTROL, mNextSentence };
static struct te_key k13 = { 79, 0, mLeft };
static struct te_key k14 = { 79, IEQUALIFIER_ALT, mPreviousWord };
static struct te_key k15 = { 79, IEQUALIFIER_CONTROL, mStartOfLine };
static struct te_key k16 = { 79, IEQUALIFIER_ALT | IEQUALIFIER_CONTROL, mPreviousSentence };

static struct te_key k17 = { 8+500, 0, kBackspace };
static struct te_key k18 = { 0x7f+500, 0, kDelete };
static struct te_key k19 = { 13+500, 0, kReturn };
static struct te_key k20 = { 13+500, IEQUALIFIER_NUMERICPAD, kReturn };
static struct te_key k21 = { 9+500, 0, kTab };
static struct te_key k22 = { 'x'+500, IEQUALIFIER_RCOMMAND, kCut };
static struct te_key k23 = { 'c'+500, IEQUALIFIER_RCOMMAND, kCopy };
static struct te_key k24 = { 'v'+500, IEQUALIFIER_RCOMMAND, kPaste };
static struct te_key k25 = { 'z'+500, IEQUALIFIER_RCOMMAND, kUndo };
static struct te_key k26 = { 'z'+500, IEQUALIFIER_RCOMMAND | IEQUALIFIER_RSHIFT, kRedo };
static struct te_key k27 = { 95, IEQUALIFIER_COMMAND, kSuggestWord };

static struct te_key k28 = { 8+500, IEQUALIFIER_CONTROL, kDelBOL };
static struct te_key k29 = { 8+500, IEQUALIFIER_SHIFT, kDelBOL };
static struct te_key k30 = { 0x7f+500, IEQUALIFIER_CONTROL, kDelEOL };
static struct te_key k31 = { 0x7f+500, IEQUALIFIER_SHIFT, kDelEOL };
static struct te_key k32 = { 8+500, IEQUALIFIER_ALT, kDelBOW };
static struct te_key k33 = { 0x7f+500, IEQUALIFIER_ALT, kDelEOW };
static struct te_key k34 = { 'x'+500, IEQUALIFIER_CONTROL, kDelLine };
static struct te_key k35 = { 0x9+500, IEQUALIFIER_ALT, kNextGadget };

static struct te_key k36 = { '1'+500, IEQUALIFIER_RCOMMAND | IEQUALIFIER_RSHIFT, kSetBookmark1 };
static struct te_key k37 = { '2'+500, IEQUALIFIER_RCOMMAND | IEQUALIFIER_RSHIFT, kSetBookmark2 };
static struct te_key k38 = { '3'+500, IEQUALIFIER_RCOMMAND | IEQUALIFIER_RSHIFT, kSetBookmark3 };

static struct te_key k39 = { '1'+500, IEQUALIFIER_RCOMMAND, kGotoBookmark1 };
static struct te_key k40 = { '2'+500, IEQUALIFIER_RCOMMAND, kGotoBookmark2 };
static struct te_key k41 = { '3'+500, IEQUALIFIER_RCOMMAND, kGotoBookmark3 };

const struct te_key *keybindings[] =
{
  &k01, &k02, &k03, &k04, &k05, &k06, &k07, &k08, &k09, &k10,
  &k11, &k12, &k13, &k14, &k15, &k16, &k17, &k18, &k19, &k20,
  &k21, &k22, &k23, &k24, &k25, &k26, &k27, &k28, &k29, &k30,
  &k31, &k32, &k33, &k34, &k35, &k36, &k37, &k38, &k39, &k40,
  &k41,
  NULL
};
