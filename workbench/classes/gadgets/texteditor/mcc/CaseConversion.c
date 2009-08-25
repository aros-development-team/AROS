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

#include <proto/utility.h>

#include "private.h"

///MangleCharacters()
VOID MangleCharacters(UBYTE (*change)(UBYTE c), struct InstData *data)
{
  LONG startx, stopx, _startx;
  struct line_node *startline, *stopline, *_startline;
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

  AddToUndoBuffer(ET_DELETEBLOCK, (char *)&newblock, data);
  AddToUndoBuffer(ET_PASTEBLOCK, (char *)&newblock, data);

  _startx = startx;
  _startline = startline;

  while(startline != stopline->next)
  {
    while(startline->line.Contents[startx] != '\n')
    {
      if(startx == stopx && startline == stopline)
        break;

      startline->line.Contents[startx] = change(startline->line.Contents[startx]);

      startx++;
    }
    startx = 0;
    startline = startline->next;
  }

  data->HasChanged = TRUE;
  RedrawArea(_startx, _startline, stopx, stopline, data);

  LEAVE();
}
///

///ChangeToUpper()
UBYTE ChangeToUpper(UBYTE c)
{
  return ToUpper(c);
}
///

///Key_ToUpper()
VOID Key_ToUpper(struct InstData *data)
{
  MangleCharacters(ChangeToUpper, data);
}
///

///ChangeToLower()
UBYTE ChangeToLower(UBYTE c)
{
  return ToLower(c);
}
///

///Key_ToLower()
VOID Key_ToLower(struct InstData *data)
{
  MangleCharacters(ChangeToLower, data);
}
///

/*
///ChangeToOtherCase()
VOID ChangeToOtherCase (UBYTE &letter)
{
  if(isupper(letter))
      letter = ToLower(letter);
  else  letter = ToUpper(letter);
}
///

///Key_ToOtherCase()
VOID Key_ToOtherCase (struct InstData *data)
{
  MangleCharacters(ChangeToOtherCase, data);
}
///
*/
