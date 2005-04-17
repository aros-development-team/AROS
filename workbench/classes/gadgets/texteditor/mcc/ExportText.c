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

 $Id: ExportText.c,v 1.2 2005/04/04 12:27:04 sba Exp $

***************************************************************************/

#include <string.h>

#include <proto/exec.h>
#include <proto/utility.h>

#include "TextEditor_mcc.h"
#include "private.h"

void *ExportText(struct line_node *node, struct Hook *exportHook, LONG wraplen)
{
	struct ExportMessage emsg;
	void *user_data = NULL;

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

		user_data = (void*)CallHookPkt(exportHook, NULL, &emsg);

		node = next_node;
	}
  return user_data;
}
