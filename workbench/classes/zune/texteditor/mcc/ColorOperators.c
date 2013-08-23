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

#include "private.h"
#include "Debug.h"

/// GetColor()
UWORD GetColor(LONG x, struct line_node *line)
{
  UWORD color = 0;
  struct LineColor *colors = line->line.Colors;

  ENTER();

  if(colors != NULL && x >= 0)
  {
    while(colors->column != EOC && colors->column <= x+1)
    {
      color = colors->color;
      colors++;
    }
  }

  RETURN(color);
  return(color);
}

///
/// AddColorToLine()
static void AddColorToLine(struct InstData *data, LONG x, struct line_node *line, LONG length, UWORD color)
{
  struct Grow colorGrow;
  struct LineColor *colors;
  UWORD oldcol = 0;

  ENTER();

  x++;

  InitGrow(&colorGrow, data->mypool, sizeof(struct LineColor));

  if((colors = line->line.Colors) != NULL)
  {
    // keep all color changes ahead of the new color
    while(colors->column != EOC && colors->column < x)
    {
      oldcol = colors->color;
      AddToGrow(&colorGrow, colors);
      colors++;
    }
  }
  // add the new color if it is different from the last one
  if(color != oldcol)
  {
    struct LineColor newColor;

    newColor.column = x;
    newColor.color = color;
    AddToGrow(&colorGrow, &newColor);
  }
  // skip and forget all color changes in the new range
  if(colors != NULL)
  {
    while(colors->column != EOC && colors->column <= x+length)
    {
      oldcol = colors->color;
      colors++;
    }
  }
  // add another color change if the new color is different from the last skipped one within the range
  if(color != oldcol)
  {
    struct LineColor newColor;

    newColor.column = x+length;
    newColor.color = oldcol;
    AddToGrow(&colorGrow, &newColor);
  }
  // keep all color changes until the end of the line
  if(colors != NULL)
  {
    while(colors->column != EOC)
    {
      AddToGrow(&colorGrow, colors);
      colors++;
    }
  }

  // terminate the color array if we have any colors at all
  if(colorGrow.itemCount > 0)
  {
    struct LineColor newColor;

    newColor.column = EOC;
    newColor.color = 0;
    AddToGrow(&colorGrow, &newColor);
  }

  // the old colors are not needed anymore
  if(line->line.Colors != NULL)
    FreeVecPooled(data->mypool, line->line.Colors);

  line->line.Colors = (struct LineColor *)colorGrow.array;

  LEAVE();
}
///
/// AddColor()
void AddColor(struct InstData *data, struct marking *realblock, UWORD color)
{
  struct marking newblock;
  struct line_node *startline;
  struct line_node *stopline;
  LONG startx;
  LONG stopx;

  ENTER();

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
    AddColorToLine(data, startx, startline, stopx-startx, color);
  }
  else
  {
    struct line_node *line = GetNextLine(startline);

    AddColorToLine(data, startx, startline, startline->line.Length-startx-1, color);
    while(line != stopline)
    {
      AddColorToLine(data, 0, line, line->line.Length-1, color);
      line = GetNextLine(line);
    }
    AddColorToLine(data, 0, line, stopx, color);
  }
  RedrawArea(data, startx, startline, stopx, stopline);

  LEAVE();
}

///
