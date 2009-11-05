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
#include <proto/exec.h>

#include "private.h"
#include "Debug.h"

/// ImportText()
/***********************************************************************
 Import the given 0 terminated text by invoking the gicen import Hook
 for every line
***********************************************************************/
struct line_node *ImportText(struct InstData *data, char *contents, struct Hook *importHook, LONG wraplength)
{
  struct line_node *first_line;

  ENTER();

  if((first_line = AllocLine(data)) != NULL)
  {
    struct line_node *line;
    struct ImportMessage im;

    im.Data = contents;
    im.ImportWrap = wraplength;
    im.PoolHandle = data->mypool;

    memset(first_line, 0, sizeof(*first_line));
    line = first_line;

    while(TRUE)
    {
      struct line_node *new_line;

      im.linenode = &line->line;

      /* invoke the hook, it will return NULL in case it is finished or
       * an error occured */
      im.Data = (char*)CallHookPkt(importHook, NULL, &im);

      if (!im.Data)
      {
        if (!line->line.Contents)
        {
          /* Free the line node if it didn't contain any contents */
          if (line->previous)
          {
            line->previous->next = NULL;

            FreeLine(data, line);
          }
          else
          {
            char *ctext;

            // if the line has nor predecessor it was obviously the first line
            // so we prepare a "fake" line_node to let the textEditor clear our
            // text
            if((ctext = AllocVecPooled(data->mypool, 2)) != NULL)
            {
              ctext[0] = '\n';
              ctext[1] = '\0';
              line->line.Contents = ctext;
              line->line.Length = 1;
              line->line.allocatedContents = 2;
            }
            else
            {
              FreeLine(data, first_line);
              first_line = NULL;
            }
          }
        }
        break;
      }

      if((new_line = AllocLine(data)) == NULL)
        break;

      memset(new_line, 0, sizeof(*new_line));

      // Inherit the flow from the previous line, but only if
      // the clearFlow variable is not set
      if(line->line.clearFlow == FALSE)
        new_line->line.Flow = line->line.Flow;

      new_line->previous = line;
      line->next = new_line;
      line = new_line;
    }
  }

  RETURN(first_line);
  return first_line;
}
///
