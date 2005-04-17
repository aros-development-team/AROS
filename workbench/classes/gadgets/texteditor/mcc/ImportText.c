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

 $Id: ImportText.c,v 1.5 2005/04/05 15:35:03 sba Exp $

***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <exec/types.h>
#include <clib/dos_protos.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include "TextEditor_mcc.h"
#include "private.h"

/***********************************************************************
 Import the given 0 terminated text by invoking the gicen import Hook
 for every line
***********************************************************************/
struct line_node *ImportText(char *contents, void *mempool, struct Hook *importHook, LONG wraplength)
{
	struct line_node *first_line, *line;
	struct ImportMessage im;

	im.Data = contents;
	im.ImportWrap = wraplength;
	im.PoolHandle = mempool;

	line = AllocPooled(mempool,sizeof(struct line_node));
	if (!line) return NULL;

	memset(line,0,sizeof(*line));
	first_line = line;

	while (1)
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

  				FreePooled(mempool,line,sizeof(struct line_node));
				}
				else
				{
          char *ctext;

					// if the line has nor predecessor it was obviously the first line
          // so we prepare a "fake" line_node to let the textEditor clear our
          // text
          if((ctext = MyAllocPooled(mempool, 2)))
          {
            ctext[0] = '\n';
            ctext[1] = '\0';
            line->line.Contents = ctext;
            line->line.Length   = 1;
          } else
          {
	  				FreePooled(mempool,first_line,sizeof(struct line_node));
          	first_line = NULL;
          }
				}
			}
			break;
		}

		if (!(new_line = AllocPooled(mempool,sizeof(struct line_node))))
			break;
		memset(new_line,0,sizeof(*new_line));
		/* Inherit the flow from the previous line */
		new_line->line.Flow = line->line.Flow;
		new_line->previous = line;
		line->next = new_line;
		line = new_line;
	}

	return first_line;
}
