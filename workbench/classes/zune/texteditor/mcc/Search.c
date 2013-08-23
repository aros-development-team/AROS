/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2013 by TextEditor.mcc Open Source Team

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
#include <proto/utility.h>

#include "private.h"
#include "Debug.h"

/// SimpleMarkText()
static void SimpleMarkText(struct InstData *data, LONG startx, struct line_node *startline, LONG stopx, struct line_node *stopline)
{
  ENTER();

  if(Enabled(data))
  {
    data->blockinfo.enabled = FALSE;
    MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);
  }
//  else
  {
    SetCursor(data, data->CPos_X, data->actualline, FALSE);
  }

  data->blockinfo.startline = startline;
  data->blockinfo.startx = startx;
  data->blockinfo.stopline = data->actualline = stopline;
  data->blockinfo.stopx = data->CPos_X = stopx;
  data->blockinfo.enabled = TRUE;

  ScrollIntoDisplay(data);
  MarkText(data, startx, startline, stopx, stopline);

  LEAVE();
}

///

static LONG Native_strncmp (STRPTR str1, STRPTR str2, LONG len) { return strncmp(str1, str2, len); }
static LONG Utility_strnicmp (STRPTR str1, STRPTR str2, LONG len) { return Strnicmp(str1, str2, len); }

/// mSearch()
IPTR mSearch(UNUSED struct IClass *cl, Object *obj, struct MUIP_TextEditor_Search *msg)
{
  struct InstData *data = INST_DATA(cl, obj);
  STRPTR str = msg->SearchString;
  LONG len = strlen(str), step = 0;

  ENTER();

  if(len > 0 && len <= 120)
  {
    BYTE map[256];
    LONG (*StrCmp)(STRPTR, STRPTR, LONG);
    LONG cursor;
    struct line_node *line;

    // if the FromTop flag is set we start the search right from the top
    if(isFlagSet(msg->Flags, MUIF_TextEditor_Search_FromTop))
    {
      cursor = 0;
      line = GetFirstLine(&data->linelist);
    }
    else
    {
      cursor = data->CPos_X;
      line = data->actualline;
    }

    memset(map, len, 256);

    // if a casesensitive search is requested we use a different
    // compare function.
    if(isFlagSet(msg->Flags, MUIF_TextEditor_Search_CaseSensitive))
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

    if(isFlagSet(msg->Flags, MUIF_TextEditor_Search_Backwards))
    {
//D(DBF_STARTUP, "MUIF_TextEditor_Search_Backwards  search=%s\n", msg->SearchString);
      if(Enabled(data))
        cursor -= len;

      while(line != NULL)
      {
        LONG lenTmp  = len;
        STRPTR contents = line->line.Contents + cursor - lenTmp+1;
        STRPTR lower = line->line.Contents;

        while(contents >= lower)
        {
//D(DBF_STARTUP, "MUIF_TextEditor_Search_Backwards  previous=%ld, contents=%s\n",line, contents);
          if(!StrCmp(contents, msg->SearchString, len))
          {
            LONG startx = contents - line->line.Contents;

//D(DBF_STARTUP, "MUIF_TextEditor_Search_Backwards found\n");

            SimpleMarkText(data, startx, line, startx+len, line);

            RETURN(TRUE);
            return TRUE;
          }
          contents -= 1;
          lenTmp += 1;
        }

        line = GetPrevLine(line);

        if(line != NULL)
          cursor = line->line.Length;
      }
    }
    else
    {
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
              LONG startx = contents - line->line.Contents;

              SimpleMarkText(data, startx, line, startx+len, line);

              RETURN(TRUE);
              return TRUE;
            }
            contents += len;
          }
        }

        cursor = 0;

        line = GetNextLine(line);
      }
    }

  }

  RETURN(FALSE);
  return FALSE;
}

///
/// mReplace()
IPTR mReplace(UNUSED struct IClass *cl, Object *obj, struct MUIP_TextEditor_Replace *msg)
{
  struct InstData *data = INST_DATA(cl, obj);
  IPTR res = FALSE;

  ENTER();

  if(Enabled(data))
  {
    Key_Clear(data);
    DoMethod(obj, MUIM_TextEditor_InsertText, msg->NewString, MUIV_TextEditor_InsertText_Cursor);
    res = TRUE;
  }

  RETURN(res);
  return res;
}

///
