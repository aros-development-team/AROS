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

#include <string.h>

#include <proto/utility.h>

#include "private.h"
#include "Debug.h"

/// mSetBlock()
IPTR mSetBlock(struct InstData *data, struct MUIP_TextEditor_SetBlock *msg)
{
  BOOL result = TRUE;
  struct marking newblock;

  ENTER();

  // initialize newblock
  newblock.enabled = FALSE;

  // now we check&set the start variables to their corresponding values
  if(msg->starty == MUIV_TextEditor_SetBlock_Min)
    newblock.startline = LineNode(data, 1);
  else  if(msg->starty == MUIV_TextEditor_SetBlock_Max)
    newblock.startline = LineNode(data, data->totallines+1);
  else if(msg->starty >= 0 && msg->starty <= data->totallines)
    newblock.startline = LineNode(data, msg->starty+1);
  else
    result = FALSE;

  if((LONG)msg->startx == MUIV_TextEditor_SetBlock_Min)
    newblock.startx = 0;
  else if(newblock.startline != NULL)
  {
    if(msg->startx == MUIV_TextEditor_SetBlock_Max)
      newblock.startx = (newblock.startline)->line.Length;
    else if(msg->startx >= 0 && msg->startx <= (newblock.startline)->line.Length)
      newblock.startx = msg->startx;
    else
      result = FALSE;
  }
  else
    result = FALSE;

  // now we check&set the stop variables to their corresponding values
  if(msg->stopy == MUIV_TextEditor_SetBlock_Min)
    newblock.stopline = LineNode(data, 1);
  else  if(msg->stopy == MUIV_TextEditor_SetBlock_Max)
    newblock.stopline = LineNode(data, data->totallines+1);
  else if(msg->stopy >= 0 && msg->stopy <= data->totallines)
    newblock.stopline = LineNode(data, msg->stopy+1);
  else
    result = FALSE;

  if(msg->stopx == MUIV_TextEditor_SetBlock_Min)
    newblock.stopx = 0;
  else if(newblock.stopline != NULL)
  {
    if(msg->stopx == MUIV_TextEditor_SetBlock_Max)
      newblock.stopx = (newblock.stopline)->line.Length;
    else if(msg->stopx >= 0 && msg->stopx <= (newblock.stopline)->line.Length)
      newblock.stopx = msg->stopx;
    else
      result = FALSE;
  }
  else
    result = FALSE;

  // check if valid values had been specified for our start/stop values
  if(result == TRUE)
  {
    // enable the block during the color/style changes
    newblock.enabled = TRUE;

    if(isFlagSet(msg->operation, MUIF_TextEditor_SetBlock_Color))
    {
      AddColor(data, &newblock, (UWORD)msg->value);
    }

    if(isFlagSet(msg->operation, MUIF_TextEditor_SetBlock_StyleBold))
    {
      AddStyle(data, &newblock, BOLD, msg->value != 0);
    }

    if(isFlagSet(msg->operation, MUIF_TextEditor_SetBlock_StyleItalic))
    {
      AddStyle(data, &newblock, ITALIC, msg->value != 0);
    }

    if(isFlagSet(msg->operation, MUIF_TextEditor_SetBlock_StyleUnderline))
    {
      AddStyle(data, &newblock, UNDERLINE, msg->value != 0);
    }

    // disable the block again
    newblock.enabled = FALSE;

    if(isFlagSet(msg->operation, MUIF_TextEditor_SetBlock_Flow))
    {
      LONG start, lines = 0;
      struct marking newblock2;
      struct line_node *startline;

      data->Flow = msg->value;

      NiceBlock(&newblock, &newblock2);
      startline = newblock2.startline;
      start = LineToVisual(data, startline);

      do
      {
        lines += startline->visual;
        startline->line.Flow = msg->value;
        startline = GetNextLine(startline);
      }
      while(startline != GetNextLine(newblock2.stopline));

      if(start < 1)
        start = 1;

      if(start-1+lines > data->maxlines)
        lines = data->maxlines-(start-1);

      DumpText(data, data->visual_y+start-1, start-1, start-1+lines, TRUE);
    }
  }

  RETURN(result);
  return result;
}

///
