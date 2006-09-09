/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by BetterString.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 BetterString class Support Site:  http://www.sf.net/projects/bstring-mcc/

 $Id$

***************************************************************************/

#include <graphics/text.h>

#include "SDI_compiler.h"

long MyTextLength(struct TextFont *font, char *text, long length)
{
  register UWORD    stringlength = 0;
  register short    *spacing;
  register short    *kerning;
  long res = 0;

  if(length > 0)
  {
    if(font->tf_Flags & FPF_PROPORTIONAL)
    {
      register unsigned char  current;

      spacing = (short *)font->tf_CharSpace - font->tf_LoChar;
      kerning = (short *)font->tf_CharKern - font->tf_LoChar;
      length++;

      while(--length > 0)
      {
        current = *text++;
        stringlength += *(spacing+current) + *(kerning+current);
      }

      res = stringlength;
    }
    else
      res = length*font->tf_XSize;
  }

  return res;
}

long MyTextFit(struct TextFont *font, char *text, long length, long width, UNUSED long direction)
{
  register int  stringlength = 0;
  register UWORD charsthatfit = length;
  register short *spacing;
  register short *kerning;
  long res = 0;

  if(length > 0)
  {
    if(font->tf_Flags & FPF_PROPORTIONAL)
    {
      register unsigned char current;

      spacing = (short *)font->tf_CharSpace - font->tf_LoChar;
      kerning = (short *)font->tf_CharKern - font->tf_LoChar;
      length++;

      while((stringlength < width) && (--length) > 0)
      {
        current = *text++;
        stringlength += *(spacing+current) + *(kerning+current);
      }

      res = charsthatfit-length;
    }
    else
    {
      if((width / font->tf_XSize) < length)
        res = width / font->tf_XSize;
      else
        res = length;
    }
  }

  return res;
}
