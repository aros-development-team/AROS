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

/// AllocLine()
struct line_node *AllocLine(struct InstData *data)
{
  struct line_node *newline;

  ENTER();

  newline = AllocVecPooled(data->mypool, sizeof(struct line_node));
  MEMTRACK("AllocLine", newline, sizeof(struct line_node));

  RETURN(newline);
  return newline;
}

///
/// FreeLine()
void FreeLine(struct InstData *data, struct line_node *line)
{
  ENTER();

  // we make sure the line is not referenced by other
  // structures as well such as the global blockinfo structure.
  if(data->blockinfo.startline == line)
  {
    if(line->next != NULL)
      data->blockinfo.startline = line->next;
    else
    {
      data->blockinfo.startline = NULL;
      data->blockinfo.enabled = FALSE;
    }
  }

  if(data->blockinfo.stopline == line)
  {
    if(line->previous != NULL)
      data->blockinfo.stopline = line->previous;
    else
    {
      data->blockinfo.stopline = NULL;
      data->blockinfo.enabled = FALSE;
    }
  }

  // finally free the line itself
  FreeVecPooled(data->mypool, line);
  UNMEMTRACK("AllocLine", line);

  LEAVE();
}

///
