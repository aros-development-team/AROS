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

#include "private.h"
#include "Debug.h"

/// GetColor()
UWORD GetColor(UWORD x, struct line_node *line)
{
  UWORD color = 0;
  struct LineColor *colors = line->line.Colors;

  ENTER();

  if(colors != NULL)
  {
    while(colors->column <= x+1)
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
  ULONG numColors;
  struct LineColor *colors = line->line.Colors;
  struct LineColor *oldcolors = colors;
  struct LineColor *newcolors;

  ENTER();

  x++;

  if(colors != NULL)
    numColors = line->line.allocatedColors + 4;
  else
    numColors = 8;

  if((newcolors = AllocVecPooled(data->mypool, numColors * sizeof(struct LineColor))) != NULL)
  {
    ULONG usedColors = 0;
    UWORD oldcol = 0;

    line->line.Colors = newcolors;

    if(colors != NULL)
    {
      while(colors->column != EOC && colors->column < x)
      {
        newcolors->column = colors->column;
        oldcol = colors->color;
        newcolors->color = colors->color;
        colors++;
        newcolors++;
        usedColors++;
      }
    }
    if(color != oldcol)
    {
      newcolors->column = x;
      newcolors->color = color;
      newcolors++;
      usedColors++;
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
      newcolors->column = x+length;
      newcolors->color = oldcol;
      newcolors++;
      usedColors++;
    }
    if(colors != NULL)
    {
      while(colors->column != EOC)
      {
        newcolors->column = colors->column;
        newcolors->color = colors->color;
        colors++;
        newcolors++;
        usedColors++;
      }
    }
    newcolors->column = EOC;
    usedColors++;

    line->line.allocatedColors = numColors;
    line->line.usedColors = usedColors;
    if(usedColors > numColors)
    {
      E(DBF_STYLE, "used colors (%ld) > allocated colors (%ld)", usedColors, numColors);
      DumpLine(line);
    }

    if(oldcolors != NULL)
    {
      FreeVecPooled(data->mypool, oldcolors);
    }
  }

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

