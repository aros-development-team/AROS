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

#include <proto/exec.h>
#include <proto/intuition.h>

#include "private.h"
#include "Debug.h"

/// UpdateStyles()
void UpdateStyles(struct InstData *data)
{
  UWORD newStyle;

  ENTER();

  if(Enabled(data))
  {
    struct marking newblock;

    NiceBlock(&data->blockinfo, &newblock);
    if(newblock.startx == data->blockinfo.startx && newblock.startline == data->blockinfo.startline)
      newStyle = GetStyle(data->blockinfo.stopx-1, data->blockinfo.stopline);
    else
      newStyle = GetStyle(data->blockinfo.stopx, data->blockinfo.stopline);
  }
  else
  {
    newStyle = GetStyle(data->CPos_X, data->actualline);
  }

  if(newStyle != data->style)
  {
    UWORD oldStyle = data->style;

    data->style = newStyle;

    if(isFlagSet(newStyle, BOLD) && isFlagClear(oldStyle, BOLD))
      set(data->object, MUIA_TextEditor_StyleBold, TRUE);
    else if(isFlagClear(newStyle, BOLD) && isFlagSet(oldStyle, BOLD))
      set(data->object, MUIA_TextEditor_StyleBold, FALSE);

    if(isFlagSet(newStyle, ITALIC) && isFlagClear(oldStyle, ITALIC))
      set(data->object, MUIA_TextEditor_StyleItalic, TRUE);
    else if(isFlagClear(newStyle, ITALIC) && isFlagSet(oldStyle, ITALIC))
      set(data->object, MUIA_TextEditor_StyleItalic, FALSE);

    if(isFlagSet(newStyle, UNDERLINE) && isFlagClear(oldStyle, UNDERLINE))
      set(data->object, MUIA_TextEditor_StyleUnderline, TRUE);
    else if(isFlagClear(newStyle, UNDERLINE) && isFlagSet(oldStyle, UNDERLINE))
      set(data->object, MUIA_TextEditor_StyleUnderline, FALSE);
  }

  LEAVE();
}

///
/// GetStyle()
UWORD GetStyle(LONG x, struct line_node *line)
{
  UWORD style = 0;

  ENTER();

  if(line->line.Styles != NULL && x >= 0)
  {
    struct LineStyle *styles = line->line.Styles;

    while(styles->column != EOS && styles->column <= x+1)
    {
      if(styles->style > 0xff)
        style &= styles->style;
      else
        style |= styles->style;

      styles++;
    }
  }

  RETURN(style);
  return(style);
}

///
/// AddStyleToLine()
void AddStyleToLine(struct InstData *data, LONG x, struct line_node *line, LONG length, UWORD style)
{
  struct Grow styleGrow;
  struct LineStyle *styles;
  UWORD cur_style = 0;
  UWORD end_style = GetStyle(x+length, line);

  ENTER();

  x++;

  InitGrow(&styleGrow, data->mypool, sizeof(struct LineStyle));

  if((styles = line->line.Styles) != NULL)
  {
    while(styles->column != EOS && styles->column < x)
    {
      AddToGrow(&styleGrow, styles);

      if(styles->style > 0xff)
        cur_style &= styles->style;
      else
        cur_style |= styles->style;

      styles++;
    }
  }
  if(style > 0xff)
  {
    if(cur_style & ~style)
    {
      struct LineStyle newStyle;

      newStyle.column = x;
      newStyle.style = style;
      AddToGrow(&styleGrow, &newStyle);
    }
  }
  else
  {
    if(!(cur_style & style))
    {
      struct LineStyle newStyle;

      newStyle.column = x;
      newStyle.style = style;
      AddToGrow(&styleGrow, &newStyle);
    }
  }

  if(styles != NULL)
  {
    while(styles->column != EOS && styles->column <= x+length)
    {
      UWORD invstyle = ~style;

      if(styles->style != style && styles->style != invstyle)
      {
        AddToGrow(&styleGrow, styles);
      }

      styles++;
    }
  }
  if(!(((style > 0xff) && (!(end_style & ~style))) ||
      ((style < 0xff) && ((end_style & style)))))
  {
    struct LineStyle newStyle;

    newStyle.column = x+length;
    newStyle.style = ~style;
    AddToGrow(&styleGrow, &newStyle);
  }

  if(styles != NULL)
  {
    while(styles->column != EOS)
    {
      AddToGrow(&styleGrow, styles);

      styles++;
    }
  }

  // the old styles are not needed anymore
  if(line->line.Styles != NULL)
    FreeVecPooled(data->mypool, line->line.Styles);

  if(styleGrow.itemCount > 0)
  {
    struct LineStyle newStyle;

    newStyle.column = EOS;
    newStyle.style = 0;
    AddToGrow(&styleGrow, &newStyle);
  }

  line->line.Styles = (struct LineStyle *)styleGrow.array;

  LEAVE();
}

///
/// AddStyle()
void AddStyle(struct InstData *data, struct marking *realblock, UWORD style, BOOL set)
{
  struct marking newblock;
  LONG startx;
  LONG stopx;
  struct line_node *startline;
  struct line_node *stopline;

  ENTER();

  if(set == FALSE)
  {
    if((data->style & style) == 0)
    {
      LEAVE();
      return;
    }

    style = ~style;
  }
  else
  {
    if((data->style & style) != 0)
    {
      LEAVE();
      return;
    }
  }
  data->HasChanged = TRUE;

  if(realblock->enabled == TRUE && (realblock->startx != realblock->stopx || realblock->startline != realblock->stopline))
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
    AddStyleToLine(data, startx, startline, stopx-startx, style);
  }
  else
  {
    struct line_node *line = GetNextLine(startline);

    AddStyleToLine(data, startx, startline, startline->line.Length-startx-1, style);
    while(line != stopline)
    {
      AddStyleToLine(data, 0, line, line->line.Length-1, style);
      line = GetNextLine(line);
    }
    AddStyleToLine(data, 0, line, stopx, style);
  }
  RedrawArea(data, startx, startline, stopx, stopline);

  if(style > 0xff)
    data->style &= style;
  else
    data->style |= style;

  LEAVE();
}

///
