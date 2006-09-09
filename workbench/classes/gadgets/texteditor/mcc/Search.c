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

 $Id$

***************************************************************************/

#include <string.h>

#include <clib/alib_protos.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "TextEditor_mcc.h"
#include "private.h"

VOID SimpleMarkText (UWORD startx, struct line_node *startline, UWORD stopx, struct line_node *stopline, struct InstData *data)
{
  ENTER();

  if(Enabled(data))
  {
    data->blockinfo.enabled = FALSE;
    MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
  }
//  else
  {
    SetCursor(data->CPos_X, data->actualline, FALSE, data);
  }

  data->blockinfo.startline = startline;
  data->blockinfo.startx = startx;
  data->blockinfo.stopline = data->actualline = stopline;
  data->blockinfo.stopx = data->CPos_X = stopx;
  data->blockinfo.enabled = TRUE;

  ScrollIntoDisplay(data);
  MarkText(startx, startline, stopx, stopline, data);

  LEAVE();
}

static LONG Native_strncmp (STRPTR str1, STRPTR str2, LONG len) { return strncmp(str1, str2, len); }
static LONG Utility_strnicmp (STRPTR str1, STRPTR str2, LONG len) { return Strnicmp(str1, str2, len); }

ULONG OM_Search (struct MUIP_TextEditor_Search *msg, struct InstData *data)
{
  STRPTR str = msg->SearchString;
  LONG len = strlen(str), step = 0;

  ENTER();

  if(len && len <= 120)
  {
    BYTE map[256];
    LONG (*StrCmp)(STRPTR, STRPTR, LONG);
    UWORD cursor;
    struct line_node *line;

    // if the FromTop flag is set we start the search right from the top
    if(msg->Flags & MUIF_TextEditor_Search_FromTop)
    {
      cursor = 0;
      line = data->firstline;
    }
    else
    {
      cursor = data->CPos_X;
      line = data->actualline;
    }

    memset(map, len, 256);

    // if a casesensitive search is requested we use a different
    // compare function.
    if(msg->Flags & MUIF_TextEditor_Search_CaseSensitive)
    {
      StrCmp = Native_strncmp;

      while(*str)
        map[(int)*str++] = step--;
    }
    else
    {
      StrCmp = Utility_strnicmp;
      while(*str)
      {
        map[ToLower(*str)] = step;
        map[ToUpper(*str++)] = step--;
      }
    }

    while(line)
    {
      LONG skip;
      STRPTR contents = line->line.Contents + cursor + len-1;
      STRPTR upper = line->line.Contents + line->line.Length;

      while(contents < upper)
      {
        skip = map[(int)(*contents)];
        contents += skip;

        if(skip <= 0)
        {
          if(!StrCmp(contents, msg->SearchString, len))
          {
            UWORD startx = contents - line->line.Contents;

            SimpleMarkText(startx, line, startx+len, line, data);

            RETURN(TRUE);
            return TRUE;
          }
          contents += len;
        }
      }

      cursor = 0;
      line = line->next;
    }
  }

  RETURN(FALSE);
  return FALSE;
}

ULONG OM_Replace (Object *obj, struct MUIP_TextEditor_Replace *msg, struct InstData *data)
{
  ULONG res = FALSE;

  ENTER();

  if(Enabled(data))
  {
    Key_Clear(data);
#ifdef ClassAct
    DoMethod(obj, MUIM_TextEditor_InsertText, msg->GInfo, msg->NewString, MUIV_TextEditor_InsertText_Cursor);
#else
    DoMethod(obj, MUIM_TextEditor_InsertText, msg->NewString, MUIV_TextEditor_InsertText_Cursor);
#endif
    res = TRUE;
  }

  RETURN(res);
  return res;
}
