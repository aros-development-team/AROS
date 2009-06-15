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

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <dos/rdargs.h>
#include <exec/memory.h>
#include <clib/alib_protos.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "private.h"

struct RexxCommand
{
  const char *Command;
  const char *Template;
};

static const struct RexxCommand Commands[] =
{
  { "CLEAR",        NULL },
  { "CUT",          NULL },
  { "COPY",         NULL },
  { "PASTE",        NULL },
  { "ERASE",        NULL },
  { "GOTOLINE",     "/N/A" },
  { "GOTOCOLUMN",   "/N/A" },
  { "CURSOR",       "Up/S,Down/S,Left/S,Right/S" },
  { "LINE",         "/N/A" },
  { "COLUMN",       "/N/A" },
  { "NEXT",         "Word/S,Sentence/S,Paragraph/S,Page/S" },
  { "PREVIOUS",     "Word/S,Sentence/S,Paragraph/S,Page/S" },
  { "POSITION",     "SOF/S,EOF/S,SOL/S,EOL/S,SOW/S,EOW/S,SOV/S,EOV/S" },
  { "SETBOOKMARK",  "/N/A" },
  { "GOTOBOOKMARK", "/N/A" },
  { "TEXT",         "/F" },
  { "UNDO",         NULL },
  { "REDO",         NULL },
  { "GETLINE",      NULL },
  { "GETCURSOR",    "Line/S,Column/S" },
  { "MARK",         "On/S,Off/S" },
  { "DELETE",       NULL },
  { "BACKSPACE",    NULL },
  { "KILLLINE",     NULL },
  { "TOUPPER",      NULL },
  { "TOLOWER",      NULL },
  { "SELECTALL",    NULL },
  { "SELECTNONE",   NULL },
  { NULL,           NULL }
};

enum
{
  CLEAR = 0, CUT, COPY, PASTE, ERASE, GOTOLINE, GOTOCOLUMN, CURSOR,
  LINE, COLUMN, NEXT, PREVIOUS, POSITION, SETBOOKMARK, GOTOBOOKMARK,
  InsTEXT, UNDO, REDO, GETLINE, GETCURSOR, MARK, DELETE, BACKSPACE,
  KILLLINE, TOUPPER, TOLOWER, SELECTALL, SELECTNONE
};
#define MaxArgs 8

static ULONG CallFunction(UWORD function, IPTR *args, const char *txtargs, struct InstData *data)
{
  struct line_node *oldactualline = data->actualline;
  UWORD oldCPos_X = data->CPos_X;
  ULONG result = TRUE;
  LONG new_y = data->visual_y-1;

  ENTER();

  SHOWVALUE(DBF_REXX, function);

  if(data->flags & FLG_ReadOnly)
  {
    switch(function)
    {
      case CURSOR:
        if(*args++)
          new_y -= 1;
        if(*args)
          new_y += 1;
        break;

      case NEXT:
        if(args[3])
          new_y += data->maxlines;
        break;

      case PREVIOUS:
        if(args[3])
          new_y -= data->maxlines;
        break;
    }
  }

  if(new_y != data->visual_y-1)
  {
    if(new_y > data->totallines-data->maxlines)
      new_y = data->totallines-data->maxlines;
    if(new_y < 0)
      new_y = 0;
    set(data->object, MUIA_TextEditor_Prop_First, new_y*data->height);
  }
  else
  {
    if(function > GOTOBOOKMARK || function < GOTOLINE)
      data->flags &= ~FLG_ARexxMark;

    switch(function)
    {
      case CLEAR:
        DoMethod(data->object, MUIM_TextEditor_ClearText);
        break;

      case CUT:
        Key_Cut(data);
        break;

      case COPY:
        Key_Copy(data);
        break;

      case PASTE:
        Key_Paste(data);
        break;

      case ERASE:
        if(Enabled(data))
          Key_Clear(data);
        break;

      case GOTOLINE:
        if(*args)
        {
          STRPTR buffer;

          if((buffer = AllocVec(16, MEMF_SHARED)))
          {
            set(data->object, MUIA_TextEditor_CursorY, *(ULONG *)*args);

            // return the current line number, this may differ from the input value!
            snprintf(buffer, 16, "%ld", xget(data->object, MUIA_TextEditor_CursorY));
            result = (ULONG)buffer;
          }
        }
        break;

      case GOTOCOLUMN:
        if(*args)
        {
          STRPTR buffer;

          if((buffer = AllocVec(16, MEMF_SHARED)))
          {
            set(data->object, MUIA_TextEditor_CursorX, *(ULONG *)*args);
            // return the current column number, this may differ from the input value!
            snprintf(buffer, 16, "%ld", xget(data->object, MUIA_TextEditor_CursorX));
            result = (ULONG)buffer;
          }
        }
        break;

      case CURSOR:
      {
        if(*args++)
          GoUp(data);
        if(*args++)
          GoDown(data);
        if(*args++)
          GoLeft(data);
        if(*args)
          GoRight(data);
      }
      break;

      case LINE:
        if(*args)
        {
          LONG line;

          line = xget(data->object, MUIA_TextEditor_CursorY);
          line += *(LONG *)*args;
          set(data->object, MUIA_TextEditor_CursorY, (line < 0) ? 0 : line);
        }
        break;

      case COLUMN:
        if(*args)
        {
          LONG column;

          column = xget(data->object, MUIA_TextEditor_CursorX);
          column += *(LONG *)*args;
          set(data->object, MUIA_TextEditor_CursorX, (column < 0) ? 0 : column);
        }
        break;

      case NEXT:
        if(*args++)
          GoNextWord(data);
        if(*args++)
          GoNextSentence(data);
        if(*args++)
          GoNextLine(data);
        if(*args)
          GoNextPage(data);
        break;

      case PREVIOUS:
        if(*args++)
          GoPreviousWord(data);
        if(*args++)
          GoPreviousSentence(data);
        if(*args++)
          GoPreviousLine(data);
        if(*args)
          GoPreviousPage(data);
        break;

      case POSITION:
        if(*args++)
          GoTop(data);
        if(*args++)
          GoBottom(data);
        if(*args++)
          GoStartOfLine(data);
        if(*args++)
          GoEndOfLine(data);
        if(*args++)
          GoPreviousWord(data);
        if(*args++)
          GoNextWord(data);
        if(*args++)
          GoPreviousPage(data);
        if(*args)
          GoNextPage(data);
        break;

      case InsTEXT:
        if(*txtargs)
        {
            struct Hook *oldhook = data->ImportHook;

          data->ImportHook = &ImPlainHook;
          DoMethod(data->object, MUIM_TextEditor_InsertText, txtargs, MUIV_TextEditor_InsertText_Cursor);
          data->ImportHook = oldhook;
        }
        break;

      case UNDO:
        Undo(data);
        break;

      case REDO:
        Redo(data);
        break;

      case GETLINE:
      {
        STRPTR buffer;

        if((buffer = AllocVec(data->actualline->line.Length+1, MEMF_SHARED)))
        {
          memcpy(buffer, data->actualline->line.Contents, data->actualline->line.Length);

          buffer[data->actualline->line.Length] = '\0';

          result = (ULONG)buffer;
        }
        break;
      }

      case GETCURSOR:
      {
        STRPTR buffer;

        if((buffer = AllocVec(16, MEMF_SHARED)))
        {
          LONG pos = 0;

          if(*args)
            pos = xget(data->object, MUIA_TextEditor_CursorY);
          else
            pos = xget(data->object, MUIA_TextEditor_CursorX);

          snprintf(buffer, 16, "%d", (int)pos);

          result = (ULONG)buffer;
        }
        break;
      }

      case SETBOOKMARK:
        if(*args)
          SetBookmark(*(ULONG *)*args-1, data);
        break;

      case GOTOBOOKMARK:
        if(*args)
          GotoBookmark(*(ULONG *)*args-1, data);
        break;

      case MARK:
        if(*++args)
        {
          data->flags &= ~FLG_ARexxMark;
          if(data->blockinfo.enabled)
          {
            data->blockinfo.enabled = FALSE;
            MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
          }
        }
        else
        {
          data->flags |= FLG_ARexxMark;
        }
        break;

      case DELETE:
        Key_Delete(data);
        break;

      case BACKSPACE:
        Key_Backspace(data);
        break;

      case KILLLINE:
        Key_DelLine(data);
        break;

      case TOUPPER:
        Key_ToUpper(data);
        break;

      case TOLOWER:
        Key_ToLower(data);
        break;

      case SELECTALL:
      {
        struct line_node *actual = data->firstline;

        data->blockinfo.startline = actual;
        data->blockinfo.startx = 0;

        while(actual->next)
          actual = actual->next;

        data->blockinfo.stopline = actual;
        data->blockinfo.stopx = data->blockinfo.stopline->line.Length-1;
        data->blockinfo.enabled = TRUE;
        MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
      }
      break;

      case SELECTNONE:
      {
        data->blockinfo.enabled = FALSE;
        MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
      }
      break;
    }

    if(data->CPos_X != oldCPos_X || oldactualline != data->actualline)
    {
      if(data->flags & FLG_Active && function <= GOTOBOOKMARK && function >= GOTOLINE)
        SetCursor(oldCPos_X, oldactualline, FALSE, data);

      if(data->flags & FLG_ARexxMark)
      {
        data->blockinfo.stopline = data->actualline;
        data->blockinfo.stopx = data->CPos_X;
        if(!data->blockinfo.enabled)
        {
          data->blockinfo.enabled = TRUE;
          data->blockinfo.startline = oldactualline;
          data->blockinfo.startx = oldCPos_X;
        }
        MarkText(oldCPos_X, oldactualline, data->CPos_X, data->actualline, data);
      }
      else
      {
        if(data->blockinfo.enabled)
        {
          data->blockinfo.enabled = FALSE;
          MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
        }
      }

      ScrollIntoDisplay(data);

      if(data->flags & FLG_Active && function <= GOTOBOOKMARK && function >= GOTOLINE)
        SetCursor(data->CPos_X, data->actualline, TRUE, data);

      // make sure to notify others that the cursor has changed and so on.
      data->NoNotify = TRUE;

      if(data->CPos_X != oldCPos_X)
        set(data->object, MUIA_TextEditor_CursorX, data->CPos_X);

      if(data->actualline != oldactualline)
        set(data->object, MUIA_TextEditor_CursorY, LineNr(data->actualline, data)-1);

      data->NoNotify = FALSE;
    }
  }

  RETURN(result);
  return(result);
}

ULONG HandleARexx(struct InstData *data, STRPTR command)
{
  ULONG result = 0;

  ENTER();

  if(data->shown && command && command[0] != '\0')
  {
    const char *txtargs = "";
    const char *cmd;
    int function;

    SHOWSTRING(DBF_REXX, command);

    for(function=0; (cmd = Commands[function].Command); function++)
    {
      int cmdlen = strlen(cmd);

      if(strnicmp(command, cmd, cmdlen) == 0 &&
         (command[cmdlen] == ' ' || command[cmdlen] == '\0'))
      {
        txtargs = &command[cmdlen];
        break;
      }
    }

    SHOWVALUE(DBF_REXX, function);
    SHOWSTRING(DBF_REXX, txtargs);

    if(Commands[function].Command && (*txtargs == '\0' || Commands[function].Template))
    {
      IPTR Args[MaxArgs];

      memset(Args, 0, sizeof(Args));

      // skip leading spaces
      while(isspace(*txtargs))
        txtargs++;

      if(*txtargs != '\0' && function != InsTEXT)
      {
        struct RDArgs *myrdargs = NULL;

        if((myrdargs = AllocDosObject(DOS_RDARGS, NULL)))
        {
          ULONG length = strlen(txtargs);
          char *buffer;

          if((buffer = MyAllocPooled(data->mypool, length+2)))
          {
            struct RDArgs *ra_result = NULL;

            memcpy(buffer, txtargs, length);
            buffer[length] = '\n';
            buffer[length+1] = '\0';

            myrdargs->RDA_Source.CS_Buffer = buffer;
            myrdargs->RDA_Source.CS_Length = length+1;
            myrdargs->RDA_Source.CS_CurChr = 0;
            myrdargs->RDA_Flags |= RDAF_NOPROMPT;

            if((ra_result = ReadArgs(Commands[function].Template, (LONG *)Args, myrdargs)))
            {
              result = CallFunction(function, Args, NULL, data);

              FreeArgs(ra_result);
            }
            MyFreePooled(data->mypool, buffer);
          }
          FreeDosObject(DOS_RDARGS, myrdargs);
        }
      }
      else
        result = CallFunction(function, Args, (char *)txtargs, data);
    }
  }

  RETURN(result);
  return result;
}

/*
LONG StringCompare (STRPTR str1, STRPTR str2, LONG *match)
{
  while(*str1 && *str2)
  {
    if(ToUpper(*str1++) != *str2++)
      return(FALSE);
  }
  if(*str1 || *str2)
  {
    return(FALSE);
  }
  else
  {
    *match = TRUE;
    return(TRUE);
  }
}

VOID Navigate (STRPTR command, struct InstData *data, LONG *match)
{
    struct   line_node   *oldactualline = data->actualline;
    int                  oldCPos_X = data->CPos_X;

  if(StringCompare(command, "EOL", match))
    GoEndOfLine(data);
  if(StringCompare(command, "BOL", match))
    GoStartOfLine(data);

  if(StringCompare(command, "TOP", match))
    GoTop(data);
  if(StringCompare(command, "BOTTOM", match))
    GoBottom(data);

  if(StringCompare(command, "NEXTWORD", match))
    GoNextWord(data);
  if(StringCompare(command, "PREVWORD", match))
    GoPreviousWord(data);

  if(StringCompare(command, "NEXTSENTENCE", match))
    GoNextSentence(data);
  if(StringCompare(command, "PREVSENTENCE", match))
    GoPreviousSentence(data);

  if(StringCompare(command, "NEXTPARAGRAPH", match))
    GoNextLine(data);
  if(StringCompare(command, "PREVPARAGRAPH", match))
    GoPreviousLine(data);

  if(StringCompare(command, "NEXTPAGE", match))
    GoNextPage(data);
  if(StringCompare(command, "PREVPAGE", match))
    GoPreviousPage(data);

  if(StringCompare(command, "MOVELEFT", match))
    GoLeft(data);
  if(StringCompare(command, "MOVERIGHT", match))
    GoRight(data);

  if(StringCompare(command, "MOVEUP", match))
    GoUp(data);
  if(StringCompare(command, "MOVEDOWN", match))
    GoDown(data);

  if((*match) && (data->CPos_X != oldCPos_X || oldactualline != data->actualline))
  {
    if(data->flags & FLG_Active)
      SetCursor(oldCPos_X, oldactualline, FALSE, data);

    ScrollIntoDisplay(data);

    if(tst == (ULONG)data->object)
      SetCursor(data->CPos_X, data->actualline, TRUE, data);
  }
}

ULONG HandleARexx (struct InstData *data, STRPTR command)
{
    LONG  match = FALSE;

  if(StringCompare(command, "COPY", &match))
    Key_Copy(data);

  if(!(data->flags & FLG_ReadOnly))
  {
      UBYTE buffer[6];

    strncpy(buffer, command, 5);
    buffer[5] = '\0';
    if(StringCompare(buffer, "TEXT ", &match))
    {
        struct Hook *oldhook = data->ImportHook;

      data->ImportHook = &ImPlainHook;
      DoMethod(data->object, MUIM_TextEditor_InsertText, command+5, MUIV_TextEditor_InsertText_Cursor);
      data->ImportHook = oldhook;
    }

    if(StringCompare(command, "CLEAR", &match))
      DoMethod(data->object, MUIM_TextEditor_ClearText);
    if(StringCompare(command, "PASTE", &match))
      Key_Paste(data);
    if(StringCompare(command, "UNDO", &match))
      Undo(data);
    if(StringCompare(command, "REDO", &match))
      Redo(data);

    if(StringCompare(command, "CUT", &match))
      Key_Cut(data);

    if(Enabled(data))
    {
      if(StringCompare(command, "ERASE", &match))
        Key_Clear(data);
    }
    else
    {
      if(StringCompare(command, "BACKSPACE", &match))
        Key_Backspace(data);
      if(StringCompare(command, "DELETE", &match))
        Key_Delete(data);
    }

    if(!match)
      Navigate(command, data, &match);

    if(match && (data->flags & FLG_Active))
    {
      SetCursor(data->CPos_X, data->actualline, FALSE, data);
    }
  }
  else
  {
      int new_y = data->visual_y-1;

    if(StringCompare(command, "TOP", &match))
      new_y = 0;
    if(StringCompare(command, "PREVPAGE", &match))
      new_y -= data->maxlines;
    if(StringCompare(command, "MOVEUP", &match))
      new_y -= 1;
    if(StringCompare(command, "BOTTOM", &match))
      new_y = data->totallines-data->maxlines;
    if(StringCompare(command, "NEXTPAGE", &match))
      new_y += data->maxlines;
    if(StringCompare(command, "MOVEDOWN", &match))
      new_y += 1;

    if(new_y != data->visual_y-1)
    {
      if(new_y > data->totallines-data->maxlines)
        new_y = data->totallines-data->maxlines;
      if(new_y < 0)
        new_y = 0;
      SetAttrs(data->object, MUIA_TextEditor_Prop_First, new_y*data->height, TAG_DONE);
    }
  }
  return(match);
}*/
/*  GetBlock
  GetLine
  GetContents
  GetCursorX
  GetCursorY    */
