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
#include <proto/exec.h>

#include "private.h"
#include "Debug.h"

/// ImportText()
/***********************************************************************
 Import the given 0 terminated text by invoking the given import Hook
 for every line
***********************************************************************/
BOOL ImportText(struct InstData *data, const char *contents, struct Hook *importHook, LONG wraplength, struct MinList *lines)
{
  struct line_node *line;

  ENTER();

  // make sure we start with an empty list of lines
  InitLines(lines);

  if((line = AllocVecPooled(data->mypool, sizeof(struct line_node))) != NULL)
  {
    struct ImportMessage im;

    memset(line, 0, sizeof(*line));

    im.Data = contents;
    im.ImportWrap = wraplength;
    im.PoolHandle = data->mypool;
    im.ConvertTabs = data->ConvertTabs;
    im.TabSize = data->TabSize;

    while(TRUE)
    {
      struct line_node *new_line;

      im.linenode = &line->line;

      // invoke the hook, it will return NULL in case it is finished or
      // an error occured
      im.Data = (char*)CallHookPkt(importHook, NULL, &im);

      if(im.Data == NULL)
      {
        if(line->line.Contents != NULL)
        {
          // add the last imported line to the list
          AddLine(lines, line);
        }
        else
        {
          // free the line node if it didn't contain any contents
          if(ContainsLines(lines) == FALSE)
          {
            FreeVecPooled(data->mypool, line);
          }
          else
          {
            // if the line has nor predecessor it was obviously the first line
            // so we prepare a "fake" line_node to let the textEditor clear our
            // text
            if(Init_LineNode(data, line, "\n") == TRUE)
              AddLine(lines, line);
            else
              FreeVecPooled(data->mypool, line);
          }
        }

        // bail out
        break;
      }

      // add the imported line to the list
      AddLine(lines, line);

      if((new_line = AllocVecPooled(data->mypool, sizeof(struct line_node))) == NULL)
        break;

      // inherit the flow from the current line for the next line,
      // but only if the clearFlow variable is not set
      if(line->line.clearFlow == FALSE)
        new_line->line.Flow = line->line.Flow;

      line = new_line;
    }
  }

  RETURN(ContainsLines(lines));
  return ContainsLines(lines);
}

///
/// ReimportText
// export and reimport the current text with modified TAB size or TAB conversion
BOOL ReimportText(struct IClass *cl, Object *obj)
{
  struct InstData *data = INST_DATA(cl, obj);
  BOOL result = FALSE;
  struct Hook *ExportHookCopy;
  char *buff;

  ENTER();

  // use the plain export hook
  ExportHookCopy = data->ExportHook;
  data->ExportHook = &ExportHookPlain;

  if((buff = (char *)mExportText(cl, obj, NULL)) != NULL)
  {
    struct MinList newlines;

    if(ImportText(data, buff, data->ImportHook, data->ImportWrap, &newlines) == TRUE)
    {
	  FreeTextMem(data, &data->linelist);
      MoveLines(&data->linelist, &newlines);
	  ResetDisplay(data);
      ResetUndoBuffer(data);
      result = TRUE;
    }

    FreeVec(buff);
  }

  // restore the former export hook
  data->ExportHook = ExportHookCopy;

  RETURN(result);
  return result;
}

///
