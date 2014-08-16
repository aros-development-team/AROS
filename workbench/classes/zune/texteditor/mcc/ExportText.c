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

/// mExportText()
IPTR mExportText(struct IClass *cl, Object *obj, UNUSED struct MUIP_TextEditor_ExportText *msg)
{
  struct InstData *data = INST_DATA(cl, obj);
  struct line_node *node;
  struct Hook *exportHook = data->ExportHook;
  LONG wraplen = data->ExportWrap;
  struct ExportMessage emsg;
  void *user_data = NULL;

  ENTER();

  // clear the export message
  memset(&emsg, 0, sizeof(struct ExportMessage));

  node = GetFirstLine(&data->linelist);
  while(node != NULL)
  {
    struct line_node *next = GetNextLine(node);

    emsg.UserData = user_data;
    emsg.Contents = node->line.Contents;
    emsg.Length = node->line.Length;
    emsg.Styles = node->line.Styles;
    emsg.Colors = node->line.Colors;
    emsg.Highlight = node->line.Highlight;
    emsg.Flow = node->line.Flow;
    emsg.Separator = node->line.Separator;
    emsg.ExportWrap = wraplen;
    emsg.Last = next == NULL;

    // to make sure that for the last line we don't export the additional,
    // artificial newline '\n' we reduce the passed length value by one.
    if(next == NULL && emsg.Contents[node->line.Length-1] == '\n')
      emsg.Length--;

    // call the ExportHook and exit immediately if it returns NULL
    if((user_data = (void*)CallHookPkt(exportHook, NULL, &emsg)) == NULL)
      break;

    node = next;
  }

  RETURN((IPTR)user_data);
  return (IPTR)user_data;
}

///
