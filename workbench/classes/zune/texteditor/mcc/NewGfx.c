/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2013 by TextEditor.mcc Open Source Team

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

#include <proto/graphics.h>

#include "private.h"

#include "Debug.h"

/// TextLengthNew
LONG TextLengthNew(struct RastPort *rp, const char *string, ULONG count, LONG tabSizePixels)
{
  LONG result = 0;
  const char *tptr = string;
  const char *tptr0 = string;
  LONG count0 = 0;

  ENTER();

  do
  {
    char c;

    if(count == 0)
    {
      // we parsed the string until the end
      // add the remaining characters' width
      if(count0 != 0)
        result += TextLength(rp, tptr0, count0);
      break;
    }

    // check the next character
    c = *tptr++;
    if(c == '\t')
    {
      // we found a TAB, calculate the characters' width so far
      if(count0 != 0)
        result += TextLength(rp, tptr0, count0);

      tptr0 = tptr;
      count0 = 0;

      // add the size of a TAB
      result += tabSizePixels;
    }
    else
      count0++;

    count--;
  }
  while(TRUE);

  RETURN(result);
  return(result);
}

///
/// TextFitNew
ULONG TextFitNew(struct RastPort *rp, const char *string, ULONG strLen, struct TextExtent *textExtent, const struct TextExtent *constrainingExtent, LONG strDirection, LONG constrainingBitWidth, LONG constrainingBitHeight, LONG tabSizePixels)
{
  ULONG result = 0;
  ULONG strLen0 = 0;
  const char *tptr = string;
  const char *tptr0 = string;

  ENTER();

  do
  {
    char c;

    if(strLen == 0)
    {
      // we parsed the string until the end
      // add the number of fitting remaining characters
      result += TextFit(rp, tptr0, strLen0, textExtent, (struct TextExtent *)constrainingExtent, strDirection, constrainingBitWidth, constrainingBitHeight);
      break;
    }

    // check the next character
    c = *tptr++;
    if(c == '\t')
    {
      // we found a TAB, calculate the number of fitting characters so far
      if(strLen0 != 0)
        result += TextFit(rp, tptr0, strLen0, textExtent, (struct TextExtent *)constrainingExtent, strDirection, constrainingBitWidth, constrainingBitHeight);

      // substract the width of the maximum width
      constrainingBitWidth -= TextLengthNew(rp, tptr0, strLen0+1, tabSizePixels);
      // bail out if no space is left
      if(constrainingBitWidth <= 0)
        break;

      result++;
      tptr0 = tptr;
      strLen0 = 0;
    }
    else
      strLen0++;

    strLen--;
  }
  while(TRUE);

  RETURN(result);
  return(result);
}

///
/// TextNew
void TextNew(struct RastPort *rp, const char *string, ULONG count, LONG tabSizePixels)
{
  const char *tptr = string;
  const char *tptr0 = string;
  LONG count0 = 0;

  ENTER();

  do
  {
    char c;

    if(count == 0)
    {
      // we parsed the string until the end
      // print out the remaining characters
      Text(rp, tptr0, count0);
      break;
    }

    // check the next character
    c = *tptr++;
    if(c == '\t')
    {
      // we found a TAB, print out the characters so far
      if(count0 != 0)
        Text(rp, tptr0, count0);

      tptr0 = tptr;
      count0 = 0;

      // advance the rastport's cursor position
      rp->cp_x += tabSizePixels;
    }
    else
      count0++;

    count--;
  }
  while(TRUE);

  LEAVE();
}

///
