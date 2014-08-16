/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2014 TextEditor.mcc Open Source Team

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

#include <proto/utility.h>

#include "private.h"
#include "Debug.h"

/// MangleCharacters()
static void MangleCharacters(struct InstData *data, char (*change)(char c))
{
  LONG startx;
  LONG stopx;
  LONG _startx;
  struct line_node *startline;
  struct line_node *stopline;
  struct line_node *_startline;
  struct marking newblock;

  ENTER();

  if(Enabled(data))
  {
    NiceBlock(&data->blockinfo, &newblock);
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

    newblock.enabled   = FALSE;
    newblock.startline = startline;
    newblock.stopline  = stopline;
    newblock.startx    = startx;
    newblock.stopx     = stopx;
  }

  AddToUndoBuffer(data, ET_DELETEBLOCK, &newblock);
  AddToUndoBuffer(data, ET_PASTEBLOCK, &newblock);

  _startx = startx;
  _startline = startline;

  while(startline != GetNextLine(stopline))
  {
    while(startline->line.Contents[startx] != '\n')
    {
      if(startx == stopx && startline == stopline)
        break;

      startline->line.Contents[startx] = change(startline->line.Contents[startx]);

      startx++;
    }
    startx = 0;
    startline = GetNextLine(startline);
  }

  data->HasChanged = TRUE;
  RedrawArea(data, _startx, _startline, stopx, stopline);

  LEAVE();
}

///
/// ChangeToUpper()
static char ChangeToUpper(char c)
{
  return ToUpper(c);
}

///
/// Key_ToUpper()
void Key_ToUpper(struct InstData *data)
{
  MangleCharacters(data, ChangeToUpper);
}

///
/// ChangeToLower()
static char ChangeToLower(char c)
{
  return ToLower(c);
}

///
/// Key_ToLower()
void Key_ToLower(struct InstData *data)
{
  MangleCharacters(data, ChangeToLower);
}

///
