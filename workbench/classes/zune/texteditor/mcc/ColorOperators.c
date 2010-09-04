/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2010 by TextEditor.mcc Open Source Team

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
static void AddColorToLine(struct InstData *data, UWORD x, struct line_node *line, UWORD length, UWORD color)
{
  struct Grow colorGrow;
  struct LineColor *colors;

  ENTER();

  x++;

  InitGrow(&colorGrow, data->mypool, sizeof(struct LineColor));

  if((colors = line->line.Colors) != NULL)
  {
    UWORD oldcol = 0;

    while(colors->column != EOC && colors->column < x)
    {
      struct LineColor newColor;

      newColor.column = colors->column;
      newColor.color = colors->color;
      oldcol = colors->color;
      colors++;
      AddToGrow(&colorGrow, &newColor);
    }
    if(color != oldcol)
    {
      struct LineColor newColor;

      newColor.column = x;
      newColor.color = color;
      AddToGrow(&colorGrow, &newColor);
    }
    if(colors != NULL)
    {
      while(colors->column != EOC && colors->column <= x+length)
      {
        oldcol = colors->color;
        colors++;
      }
    }
    if(color != oldcol)
    {
      struct LineColor newColor;

      newColor.column = x+length;
      newColor.color = oldcol;
      AddToGrow(&colorGrow, &newColor);
    }
    if(colors != NULL)
    {
      while(colors->column != EOC)
      {
        struct LineColor newColor;

        newColor.column = colors->column;
        newColor.color = colors->color;
        AddToGrow(&colorGrow, &newColor);
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
    FreeVecPooled(data->mypool, line->line.Colors);
  }

  line->line.Colors = (struct LineColor *)colorGrow.array;

  LEAVE();
}
///
/// AddColor()
void AddColor(struct InstData *data, struct marking *realblock, UWORD color)
{
  struct marking newblock;
  struct line_node *startline, *stopline;
  UWORD startx, stopx;

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
    struct line_node *line = startline->next;

    AddColorToLine(data, startx, startline, startline->line.Length-startx-1, color);
    while(line != stopline)
    {
      AddColorToLine(data, 0, line, line->line.Length-1, color);
      line = line->next;
    }
    AddColorToLine(data, 0, line, stopx, color);
  }
  RedrawArea(data, startx, startline, stopx, stopline);

  LEAVE();
}

///

