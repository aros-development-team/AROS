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

#include <string.h>

#include <proto/utility.h>

#include "private.h"

ULONG OM_SetBlock(struct MUIP_TextEditor_SetBlock *msg, struct InstData *data)
{
  struct marking newblock;
 
  ENTER();

  if(msg->starty <= (ULONG)data->totallines)
    newblock.startline = LineNode(msg->starty+1, data);

  if(msg->startx <= (newblock.startline)->line.Length)
    newblock.startx = msg->startx;

  if(msg->stopx < (newblock.startline)->line.Length)
    newblock.stopx = msg->stopx;
  else if(msg->stopx == (newblock.startline)->line.Length)
    newblock.stopx = msg->stopx-1;

  if(msg->starty <= (ULONG)data->totallines)
    newblock.stopline = LineNode(msg->stopy+1, data);

//D(DBF_STARTUP, "(newblock.startline)->line.Length=%ld\n", (newblock.startline)->line.Length);
//D(DBF_STARTUP, "MSG : startx=%ld, stopx=%ld, starty=%ld, stopy=%ld operation=%ld, value=%ld\n", msg->startx,msg->stopx,msg->starty,msg->stopy,msg->operation,msg->value);
//D(DBF_STARTUP, "NBK : startx=%ld, stopx=%ld, starty=%ld, stopy=%ld operation=%ld, value=%ld\n", newblock.startx,newblock.stopx,(LineNr(newblock.startline, data)-1), (LineNr(newblock.stopline, data)-1),msg->operation,msg->value);

  if(isFlagSet(msg->operation, MUIF_TextEditor_SetBlock_Color))
  {
//D(DBF_STARTUP, "SetBlock: color %ld\n", msg->value);
    newblock.enabled = TRUE;

    AddColor(&newblock, (UWORD)msg->value, data);

    newblock.enabled = FALSE;
  }

  if(isFlagSet(msg->operation, MUIF_TextEditor_SetBlock_StyleBold))
  {
//D(DBF_STARTUP, "SetBlock: StyleBold %ld\n", msg->value);
    AddStyle(&newblock, BOLD, msg->value, data);
  }

  if(isFlagSet(msg->operation, MUIF_TextEditor_SetBlock_StyleItalic))
  {
//D(DBF_STARTUP, "SetBlock: StyleItalic %ld\n", msg->value);
    AddStyle(&newblock, ITALIC, msg->value, data);
  }

  if(isFlagSet(msg->operation, MUIF_TextEditor_SetBlock_StyleUnderline))
  {
//D(DBF_STARTUP, "SetBlock: StyleUnderline %ld\n", msg->value);
    AddStyle(&newblock, UNDERLINE, msg->value, data);
  }

  if(isFlagSet(msg->operation, MUIF_TextEditor_SetBlock_Flow))
  {
    LONG start, lines = 0;
    struct marking newblock2;
    struct line_node *startline;
//D(DBF_STARTUP, "SetBlock: Flow  %ld\n", msg->value);
    data->Flow = msg->value;

    NiceBlock(&newblock, &newblock2);
    startline = newblock2.startline;
    start = LineToVisual(startline, data);

    do
    {
      lines += startline->visual;
      startline->line.Flow = msg->value;
      startline = startline->next;
    }
    while(startline != newblock2.stopline->next);

    if(start < 1)
      start = 1;
    
    if(start-1+lines > data->maxlines)
      lines = data->maxlines-(start-1);
    
    DumpText(data->visual_y+start-1, start-1, start-1+lines, TRUE, data);
  }

  RETURN(TRUE);
  return TRUE;
}
