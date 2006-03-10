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

 $Id: ColorOperators.c,v 1.3 2005/05/14 00:27:14 damato Exp $

***************************************************************************/

#include "private.h"

UWORD GetColor (UWORD x, struct line_node *line)
{
  UWORD color = 0;
  UWORD *colors = line->line.Colors;

  ENTER();

  if(colors)
  {
    while(*colors <= x+1)
    {
      color = *(colors+1);
      colors += 2;
    }
  }

  RETURN(color);
  return(color);
}

void  AddColorToLine (UWORD x, struct line_node *line, UWORD length, UWORD color, struct InstData *data)
{
  UWORD *colors   = line->line.Colors;
  UWORD *oldcolors  = colors;
  UWORD *newcolors;

  ENTER();

  x++;

  if(colors)
      newcolors = MyAllocPooled(data->mypool, (*((long *)colors-1))+4);
  else  newcolors = MyAllocPooled(data->mypool, 32);

  if(newcolors)
  {
    UWORD oldcol = 0;

    line->line.Colors = newcolors;
    if(colors)
    {
      while(*colors != 0xffff && *colors < x)
      {
        *newcolors++ = *colors++;
        oldcol = *colors;
        *newcolors++ = *colors++;
      }
    }
    if(color != oldcol)
    {
      *newcolors++ = x;
      *newcolors++ = color;
    }
    if(colors)
    {
      while(*colors != 0xffff && *colors <= x+length)
      {
        oldcol = *(colors+1);
        colors += 2;
      }
    }
    if(color != oldcol)
    {
      *newcolors++ = x+length;
      *newcolors++ = oldcol;
    }
    if(colors)
    {
      while(*colors != 0xffff)
      {
        *newcolors++ = *colors++;
        *newcolors++ = *colors++;
      }
    }
    *newcolors = 0xffff;

    if(oldcolors)
    {
      MyFreePooled(data->mypool, oldcolors);
    }
  }

  LEAVE();
}

VOID AddColor (struct marking *realblock, UWORD color, struct InstData *data)
{
  struct marking    newblock;
  struct line_node  *startline, *stopline;
  UWORD         startx, stopx;

  ENTER();

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
    AddColorToLine(startx, startline, stopx-startx, color, data);
  }
  else
  {
      struct  line_node *line = startline->next;

    AddColorToLine(startx, startline, startline->line.Length-startx-1, color, data);
    while(line != stopline)
    {
      AddColorToLine(0, line, line->line.Length-1, color, data);
      line = line->next;
    }
    AddColorToLine(0, line, stopx, color, data);
  }
  RedrawArea(startx, startline, stopx, stopline, data);

  LEAVE();
}
