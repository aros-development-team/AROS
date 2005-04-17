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

 $Id: TextFunctions.c,v 1.2 2005/04/04 21:59:02 damato Exp $

***************************************************************************/

#include <proto/diskfont.h>
#include <proto/graphics.h>
#include <graphics/text.h>
#include <graphics/rastport.h>

#include "SDI_compiler.h"

long MyTextLength(struct TextFont *font, char *text, long length)
{
  register UWORD    stringlength = 0;
  register short    *spacing;
  register short    *kerning;

  if(font->tf_Flags & FPF_PROPORTIONAL)
  {
    register unsigned char  current;

    spacing = (short *)font->tf_CharSpace - font->tf_LoChar;
    kerning = (short *)font->tf_CharKern - font->tf_LoChar;
    length++;

    while (--length)
    {
      current = *text++;
      stringlength += *(spacing+current) + *(kerning+current);
    }
    return(stringlength);
  }
  else  return (length * font->tf_XSize);
}

long MyTextFit(struct TextFont *font, char *text, long length, long width, UNUSED long direction)
{
  register int    stringlength = 0;
  register UWORD    charsthatfit = length;
  register short    *spacing;
  register short    *kerning;

  if(font->tf_Flags & FPF_PROPORTIONAL)
  {
        register unsigned char  current;

    spacing = (short *)font->tf_CharSpace - font->tf_LoChar;
    kerning = (short *)font->tf_CharKern - font->tf_LoChar;
    length++;

    while ((stringlength < width) && (--length))
    {
      current = *text++;
      stringlength += *(spacing+current) + *(kerning+current);
    }
    return(charsthatfit-length);
  }
  else
  {
    if ((width / font->tf_XSize) < length)
        return (width / font->tf_XSize);
    else  return (length);
  }
}
