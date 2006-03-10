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

 $Id: ExportText.c,v 1.6 2005/08/08 11:28:15 gnikl Exp $

***************************************************************************/

#include <string.h>

#include <proto/utility.h>

#include "TextEditor_mcc.h"
#include "private.h"

void *ExportText(struct line_node *node, struct Hook *exportHook, LONG wraplen)
{
	struct ExportMessage emsg;
	void *user_data = NULL;

	ENTER();

	memset(&emsg,0,sizeof(emsg));

	while (node)
	{
		struct line_node *next_node = node->next;

		emsg.UserData = user_data;
		emsg.Contents = node->line.Contents;
		emsg.Length = node->line.Length;
		emsg.Styles = node->line.Styles;
		emsg.Colors = node->line.Colors;
		emsg.Highlight = node->line.Color;
		emsg.Flow = node->line.Flow;
		emsg.Separator = node->line.Separator;
		emsg.ExportWrap = wraplen;
		emsg.Last = !next_node;

		// to make sure that for the last line we don't export the additional,
		// artificial newline '\n' we reduce the passed length value by one.
		if(next_node == NULL && emsg.Contents[node->line.Length-1] == '\n')
		  emsg.Length--;

		user_data = (void*)CallHookPkt(exportHook, NULL, &emsg);

		node = next_node;
	}

	RETURN(user_data);
	return user_data;
}
