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

 $Id: StyleOperators.c,v 1.4 2005/06/24 14:38:44 gnikl Exp $

***************************************************************************/

#include <proto/intuition.h>

#include "TextEditor_mcc.h"
#include "private.h"

#ifdef ClassAct
VOID NotifySet (Object *, struct GadgetInfo *, ULONG, ULONG);

void  CA_UpdateStyles (struct GadgetInfo *GInfo, struct InstData *data)
{
  UWORD style;

  if(Enabled(data))
  {
      struct marking newblock;

    NiceBlock(&data->blockinfo, &newblock);
    style = GetStyle(data->blockinfo.stopx - ((data->blockinfo.stopx && newblock.startx == data->blockinfo.startx && newblock.startline == data->blockinfo.startline) ? 1 : 0), data->blockinfo.stopline);
  }
  else
  {
    style = GetStyle(data->CPos_X, data->actualline);
  }

  if(style != data->style)
  {
      UWORD oldstyle = data->style;

    data->style = style;

    if(style & BOLD)
    {
      if(!(oldstyle & BOLD))
        NotifySet(data->object, GInfo, MUIA_TextEditor_StyleBold, TRUE);
    }
    else
    {
      if(oldstyle & BOLD)
        NotifySet(data->object, GInfo, MUIA_TextEditor_StyleBold, FALSE);
    }
    if(style & ITALIC)
    {
      if(!(oldstyle & ITALIC))
        NotifySet(data->object, GInfo, MUIA_TextEditor_StyleItalic, TRUE);
    }
    else
    {
      if(oldstyle & ITALIC)
        NotifySet(data->object, GInfo, MUIA_TextEditor_StyleItalic, FALSE);
    }
    if(style & UNDERLINE)
    {
      if(!(oldstyle & UNDERLINE))
        NotifySet(data->object, GInfo, MUIA_TextEditor_StyleUnderline, TRUE);
    }
    else
    {
      if(oldstyle & UNDERLINE)
        NotifySet(data->object, GInfo, MUIA_TextEditor_StyleUnderline, FALSE);
    }
  }
}

#else

void  UpdateStyles (struct InstData *data)
{
  UWORD style;

  ENTER();

  if(Enabled(data))
  {
      struct marking newblock;

    NiceBlock(&data->blockinfo, &newblock);
    style = GetStyle(data->blockinfo.stopx - ((newblock.startx == data->blockinfo.startx && newblock.startline == data->blockinfo.startline) ? 1 : 0), data->blockinfo.stopline);
  }
  else
  {
    style = GetStyle(data->CPos_X, data->actualline);
  }

  if(style != data->style)
  {
      UWORD oldstyle = data->style;

    data->style = style;

    if(style & BOLD)
    {
      if(!(oldstyle & BOLD))
        set(data->object, MUIA_TextEditor_StyleBold, TRUE);
    }
    else
    {
      if(oldstyle & BOLD)
        set(data->object, MUIA_TextEditor_StyleBold, FALSE);
    }
    if(style & ITALIC)
    {
      if(!(oldstyle & ITALIC))
        set(data->object, MUIA_TextEditor_StyleItalic, TRUE);
    }
    else
    {
      if(oldstyle & ITALIC)
        set(data->object, MUIA_TextEditor_StyleItalic, FALSE);
    }
    if(style & UNDERLINE)
    {
      if(!(oldstyle & UNDERLINE))
        set(data->object, MUIA_TextEditor_StyleUnderline, TRUE);
    }
    else
    {
      if(oldstyle & UNDERLINE)
        set(data->object, MUIA_TextEditor_StyleUnderline, FALSE);
    }
  }

  LEAVE();
}

#endif

LONG  GetStyle (LONG x, struct line_node *line)
{
  LONG  style = 0;

  ENTER();

  if(line->line.Styles)
  {
      unsigned short *styles = line->line.Styles;

    while(*styles <= x+1)
    {
      if(*++styles > 0xff)
          style &= *styles++;
      else  style |= *styles++;
    }
  }

  RETURN(style);
  return(style);
}

void  AddStyleToLine  (LONG x, struct line_node *line, LONG length, unsigned short style, struct InstData *data)
{
  unsigned short *styles    = line->line.Styles;
  unsigned short *oldstyles = styles;
  unsigned short *newstyles;
  unsigned short cur_style = 0;
  unsigned short end_style = GetStyle(x+length, line);

  ENTER();

  x++;

  if(styles)
      newstyles = MyAllocPooled(data->mypool, (*((long *)styles-1))+8);
  else  newstyles = MyAllocPooled(data->mypool, 32);

  if(newstyles)
  {
    line->line.Styles = newstyles;
    if(styles)
    {
      while(*styles != EOS && *styles < x)
      {
        *newstyles++ = *styles++;
        if(*styles > 0xff)
            cur_style &= *styles;
        else  cur_style |= *styles;
        *newstyles++ = *styles++;
      }
    }
    if(style > 0xff)
    {
      if(cur_style & ~style)
      {
        *newstyles++ = x;
        *newstyles++ = style;
      }
    }
    else
    {
      if(!(cur_style & style))
      {
        *newstyles++ = x;
        *newstyles++ = style;
      }
    }
    if(styles)
    {
      while(*styles != EOS && *styles <= x+length)
      {
        if((*(styles+1) != style) && (*(styles+1) != (unsigned short)~style))
        {
          *newstyles++ = *styles++;
          *newstyles++ = *styles++;
        }
        else  styles += 2;
      }
    }
    if(!(((style > 0xff) && (!(end_style & ~style))) ||
        ((style < 0xff) && ((end_style & style)))))
    {
      *newstyles++ = x+length;
      *newstyles++ = ~style;
    }

    if(styles)
    {
      while(*styles != EOS)
      {
        *newstyles++ = *styles++;
        *newstyles++ = *styles++;
      }
    }
    *newstyles = EOS;
    if(oldstyles)
    {
      MyFreePooled(data->mypool, oldstyles);
    }
  }

  LEAVE();
}

void  AddStyle (struct marking *realblock, unsigned short style, long Set, struct InstData *data)
{
  struct  marking newblock;
  LONG  startx, stopx;
  struct  line_node *startline, *stopline;

  ENTER();

  if(!Set)
  {
    if(!(data->style & style))
    {
      LEAVE();
      return;
    }

    style = ~style;
  }
  else
  {
    if(data->style & style)
    {
      LEAVE();
      return;
    }
  }
  data->HasChanged = TRUE;

  if(realblock->enabled && (realblock->startx != realblock->stopx || realblock->startline != realblock->stopline))
  {
    NiceBlock(realblock, &newblock);
    startx    = newblock.startx;
    stopx     = newblock.stopx;
    startline = newblock.startline;
    stopline  = newblock.stopline;
  }
  else
  {
    startx    = data->CPos_X;
    stopx     = startx+1;
    startline = data->actualline;
    stopline  = startline;
  }

  if(startline == stopline)
  {
    AddStyleToLine(startx, startline, stopx-startx, style, data);
  }
  else
  {
    struct line_node *line = startline->next;

    AddStyleToLine(startx, startline, startline->line.Length-startx-1, style, data);
    while(line != stopline)
    {
      AddStyleToLine(0, line, line->line.Length-1, style, data);
      line = line->next;
    }
    AddStyleToLine(0, line, stopx, style, data);
  }
  RedrawArea(startx, startline, stopx, stopline, data);

  if(style > 0xff)
      data->style &= style;
  else  data->style |= style;

  LEAVE();
}
