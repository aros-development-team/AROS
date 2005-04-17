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

 $Id: CaseConversion.c,v 1.2 2005/03/31 17:35:28 sba Exp $

***************************************************************************/

#include <proto/utility.h>

#include "TextEditor_mcc.h"
#include "private.h"

VOID MangleCharacters(UBYTE (*change)(UBYTE c), struct InstData *data)
{
  LONG startx, stopx, _startx;
  struct line_node *startline, *stopline, *_startline;

  if(Enabled(data))
  {
    struct marking newblock;
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
  }

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
}

UBYTE ChangeToUpper(UBYTE c)
{
  return ToUpper(c);
}

VOID Key_ToUpper(struct InstData *data)
{
  MangleCharacters(ChangeToUpper, data);
}

UBYTE ChangeToLower(UBYTE c)
{
  return ToLower(c);
}

VOID Key_ToLower(struct InstData *data)
{
  MangleCharacters(ChangeToLower, data);
}

/*VOID ChangeToOtherCase (UBYTE &letter)
{
  if(isupper(letter))
      letter = ToLower(letter);
  else  letter = ToUpper(letter);
}

VOID Key_ToOtherCase (struct InstData *data)
{
  MangleCharacters(ChangeToOtherCase, data);
}
*/
