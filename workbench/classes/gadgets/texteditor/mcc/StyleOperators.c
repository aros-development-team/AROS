/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2009 by TextEditor.mcc Open Source Team

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

  if(line->line.Styles != NULL)
  {
    struct LineStyle *styles = line->line.Styles;

    while(styles->column <= x+1)
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
  ULONG numStyles;
  struct LineStyle *styles = line->line.Styles;
  struct LineStyle *oldStyles = styles;
  struct LineStyle *newstyles;
  UWORD cur_style = 0;
  UWORD end_style = GetStyle(x+length, line);

  ENTER();

  x++;

  if(styles != NULL)
    numStyles = line->line.allocatedStyles + 4;
  else
    numStyles = 8;

  if((newstyles = AllocVecPooled(data->mypool, numStyles * sizeof(struct LineStyle))) != NULL)
  {
    ULONG usedStyles = 0;

    line->line.Styles = newstyles;

    if(styles != NULL)
    {
      while(styles->column != EOS && styles->column < x)
      {
        newstyles->column = styles->column;
        newstyles->style = styles->style;

        if(styles->style > 0xff)
          cur_style &= styles->style;
        else
          cur_style |= styles->style;

        styles++;
        newstyles++;
        usedStyles++;
      }
    }
    if(style > 0xff)
    {
      if(cur_style & ~style)
      {
        newstyles->column = x;
        newstyles->style = style;
        newstyles++;
        usedStyles++;
      }
    }
    else
    {
      if(!(cur_style & style))
      {
        newstyles->column = x;
        newstyles->style = style;
        newstyles++;
        usedStyles++;
      }
    }
    if(styles != NULL)
    {
      while(styles->column != EOS && styles->column <= x+length)
      {
        if(styles->style != style && styles->style != (UWORD)~style)
        {
          newstyles->column = styles->column;
          newstyles->style = styles->style;
          newstyles++;
          usedStyles++;
        }
        styles++;
      }
    }
    if(!(((style > 0xff) && (!(end_style & ~style))) ||
        ((style < 0xff) && ((end_style & style)))))
    {
      newstyles->column = x+length;
      newstyles->style = ~style;
      newstyles++;
      usedStyles++;
    }

    if(styles != NULL)
    {
      while(styles->column != EOS)
      {
        newstyles->column = styles->column;
        newstyles->style = styles->style;
        styles++;
        newstyles++;
        usedStyles++;
      }
    }
    newstyles->column = EOS;
    usedStyles++;

    line->line.allocatedStyles = numStyles;
    line->line.usedStyles = usedStyles;
    if(usedStyles > numStyles)
    {
      E(DBF_STYLE, "used styles (%ld) > allocated styles (%ld)", usedStyles, numStyles);
      DumpLine(line);
    }

    if(oldStyles != NULL)
    {
      FreeVecPooled(data->mypool, oldStyles);
    }
  }

  LEAVE();
}

///
/// AddStyle()
void AddStyle(struct InstData *data, struct marking *realblock, UWORD style, BOOL set)
{
  struct marking newblock;
  LONG startx, stopx;
  struct line_node *startline, *stopline;

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
    struct line_node *line = startline->next;

    AddStyleToLine(data, startx, startline, startline->line.Length-startx-1, style);
    while(line != stopline)
    {
      AddStyleToLine(data, 0, line, line->line.Length-1, style);
      line = line->next;
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

