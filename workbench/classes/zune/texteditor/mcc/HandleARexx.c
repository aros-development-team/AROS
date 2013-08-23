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
#include "Debug.h"

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

/// CallFunction()
static IPTR CallFunction(struct InstData *data, UWORD function, IPTR *args, const char *txtargs)
{
  struct line_node *oldactualline = data->actualline;
  LONG oldCPos_X = data->CPos_X;
  IPTR result = TRUE;
  LONG new_y = data->visual_y-1;

  ENTER();

  SHOWVALUE(DBF_REXX, function);

  if(isFlagSet(data->flags, FLG_ReadOnly))
  {
    switch(function)
    {
      case CURSOR:
      {
        if(*args++)
          new_y -= 1;
        if(*args)
          new_y += 1;
      }
      break;

      case NEXT:
      {
        if(args[3])
          new_y += data->maxlines;
      }
      break;

      case PREVIOUS:
      {
        if(args[3])
          new_y -= data->maxlines;
      }
      break;
    }
  }

  if(new_y != data->visual_y-1)
  {
    if(new_y > data->totallines-data->maxlines)
      new_y = data->totallines-data->maxlines;
    if(new_y < 0)
      new_y = 0;
    set(data->object, MUIA_TextEditor_Prop_First, new_y*data->fontheight);
  }
  else
  {
    if(function > GOTOBOOKMARK || function < GOTOLINE)
      clearFlag(data->flags, FLG_ARexxMark);

    switch(function)
    {
      case CLEAR:
      {
        DoMethod(data->object, MUIM_TextEditor_ClearText);
      }
      break;

      case CUT:
      {
        Key_Cut(data);
      }
      break;

      case COPY:
      {
        Key_Copy(data);
      }
      break;

      case PASTE:
      {
        Key_Paste(data);
      }
      break;

      case ERASE:
      {
        if(Enabled(data))
          Key_Clear(data);
      }
      break;

      case GOTOLINE:
      {
        if(*args)
        {
          STRPTR buffer;

          if((buffer = AllocVecShared(16, MEMF_ANY)) != NULL)
          {
            set(data->object, MUIA_TextEditor_CursorY, *(ULONG *)*args);

            // return the current line number, this may differ from the input value!
            snprintf(buffer, 16, "%ld", xget(data->object, MUIA_TextEditor_CursorY));
            result = (IPTR)buffer;
          }
        }
      }
      break;

      case GOTOCOLUMN:
      {
        if(*args)
        {
          STRPTR buffer;

          if((buffer = AllocVecShared(16, MEMF_ANY)) != NULL)
          {
            set(data->object, MUIA_TextEditor_CursorX, *(ULONG *)*args);
            // return the current column number, this may differ from the input value!
            snprintf(buffer, 16, "%ld", xget(data->object, MUIA_TextEditor_CursorX));
            result = (IPTR)buffer;
          }
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
      {
        if(*args)
        {
          LONG line;

          line = xget(data->object, MUIA_TextEditor_CursorY);
          line += *(LONG *)*args;
          set(data->object, MUIA_TextEditor_CursorY, (line < 0) ? 0 : line);
        }
      }
      break;

      case COLUMN:
      {
        if(*args)
        {
          LONG column;

          column = xget(data->object, MUIA_TextEditor_CursorX);
          column += *(LONG *)*args;
          set(data->object, MUIA_TextEditor_CursorX, (column < 0) ? 0 : column);
        }
      }
      break;

      case NEXT:
      {
        if(*args++)
          GoNextWord(data);
        if(*args++)
          GoNextSentence(data);
        if(*args++)
          GoNextLine(data);
        if(*args)
          GoNextPage(data);
      }
      break;

      case PREVIOUS:
      {
        if(*args++)
          GoPreviousWord(data);
        if(*args++)
          GoPreviousSentence(data);
        if(*args++)
          GoPreviousLine(data);
        if(*args)
          GoPreviousPage(data);
      }
      break;

      case POSITION:
      {
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
      }
      break;

      case InsTEXT:
      {
        if(*txtargs)
        {
          struct Hook *oldhook = data->ImportHook;

          data->ImportHook = &ImPlainHook;
          DoMethod(data->object, MUIM_TextEditor_InsertText, txtargs, MUIV_TextEditor_InsertText_Cursor);
          data->ImportHook = oldhook;
        }
      }
      break;

      case UNDO:
      {
        Undo(data);
      }
      break;

      case REDO:
      {
        Redo(data);
      }
      break;

      case GETLINE:
      {
        STRPTR buffer;

        if((buffer = AllocVecShared(data->actualline->line.Length+1, MEMF_ANY)) != NULL)
        {
          strlcpy(buffer, data->actualline->line.Contents, data->actualline->line.Length+1);
          result = (IPTR)buffer;
        }
        break;
      }

      case GETCURSOR:
      {
        STRPTR buffer;

        if((buffer = AllocVecShared(16, MEMF_ANY)) != NULL)
        {
          LONG pos = 0;

          if(*args)
            pos = xget(data->object, MUIA_TextEditor_CursorY);
          else
            pos = xget(data->object, MUIA_TextEditor_CursorX);

          snprintf(buffer, 16, "%d", (int)pos);

          result = (IPTR)buffer;
        }
        break;
      }

      case SETBOOKMARK:
      {
        if(*args)
          SetBookmark(data, *(ULONG *)*args-1);
      }
      break;

      case GOTOBOOKMARK:
      {
        if(*args)
          GotoBookmark(data, *(ULONG *)*args-1);
      }
      break;

      case MARK:
      {
        if(*++args)
        {
          data->flags &= ~FLG_ARexxMark;
          if(data->blockinfo.enabled == TRUE)
          {
            data->blockinfo.enabled = FALSE;
            MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);
          }
        }
        else
        {
          setFlag(data->flags, FLG_ARexxMark);
        }
      }
      break;

      case DELETE:
      {
        Key_Delete(data);
      }
      break;

      case BACKSPACE:
      {
        Key_Backspace(data);
      }
      break;

      case KILLLINE:
      {
        Key_DelLine(data);
      }
      break;

      case TOUPPER:
      {
        Key_ToUpper(data);
      }
      break;

      case TOLOWER:
      {
        Key_ToLower(data);
      }
      break;

      case SELECTALL:
      {
        MarkAllBlock(data, &data->blockinfo);
        MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);
      }
      break;

      case SELECTNONE:
      {
        if(data->blockinfo.enabled == TRUE)
        {
          data->blockinfo.enabled = FALSE;
          MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);
        }
        else
          W(DBF_BLOCK, "no text selected (startline=%08lx, stopline=%08lx)", data->blockinfo.startline, data->blockinfo.stopline);
      }
      break;
    }

    if(data->CPos_X != oldCPos_X || oldactualline != data->actualline)
    {
      if(isFlagSet(data->flags, FLG_Active) && function <= GOTOBOOKMARK && function >= GOTOLINE)
        SetCursor(data, oldCPos_X, oldactualline, FALSE);

      if(isFlagSet(data->flags, FLG_ARexxMark))
      {
        data->blockinfo.stopline = data->actualline;
        data->blockinfo.stopx = data->CPos_X;
        if(data->blockinfo.enabled == FALSE)
        {
          data->blockinfo.enabled = TRUE;
          data->blockinfo.startline = oldactualline;
          data->blockinfo.startx = oldCPos_X;
        }
        MarkText(data, oldCPos_X, oldactualline, data->CPos_X, data->actualline);
      }
      else
      {
        if(data->blockinfo.enabled == TRUE)
        {
          data->blockinfo.enabled = FALSE;
          MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);
        }
      }

      ScrollIntoDisplay(data);

      if(isFlagSet(data->flags, FLG_Active) && function <= GOTOBOOKMARK && function >= GOTOLINE)
        SetCursor(data, data->CPos_X, data->actualline, TRUE);

      // make sure to notify others that the cursor has changed and so on.
      data->NoNotify = TRUE;

      if(data->CPos_X != oldCPos_X)
        set(data->object, MUIA_TextEditor_CursorX, data->CPos_X);

      if(data->actualline != oldactualline)
        set(data->object, MUIA_TextEditor_CursorY, LineNr(data, data->actualline)-1);

      data->NoNotify = FALSE;
    }
  }

  RETURN(result);
  return(result);
}

///
/// mHandleARexx()
IPTR mHandleARexx(struct IClass *cl, Object *obj, struct MUIP_TextEditor_ARexxCmd *msg)
{
  struct InstData *data = INST_DATA(cl, obj);
  IPTR result = 0;
  STRPTR command = msg->command;

  ENTER();

  if(data->shown == TRUE && command != NULL && command[0] != '\0')
  {
    const char *txtargs = "";
    const char *cmd;
    int function;

    SHOWSTRING(DBF_REXX, command);

    for(function=0; (cmd = Commands[function].Command) != NULL; function++)
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

    if(Commands[function].Command != NULL && (*txtargs == '\0' || Commands[function].Template != NULL))
    {
      IPTR Args[MaxArgs];

      memset(Args, 0, sizeof(Args));

      // skip leading spaces
      while(isspace(*txtargs))
        txtargs++;

      if(*txtargs != '\0' && function != InsTEXT)
      {
        struct RDArgs *myrdargs = NULL;

        if((myrdargs = AllocDosObject(DOS_RDARGS, NULL)) != NULL)
        {
          ULONG length = strlen(txtargs);
          char *buffer;

          if((buffer = AllocVecPooled(data->mypool, length+2)) != NULL)
          {
            struct RDArgs *ra_result = NULL;

            snprintf(buffer, length+2, "%s\n", txtargs);

            myrdargs->RDA_Source.CS_Buffer = buffer;
            myrdargs->RDA_Source.CS_Length = length+1;
            myrdargs->RDA_Source.CS_CurChr = 0;
            myrdargs->RDA_Flags |= RDAF_NOPROMPT;

            if((ra_result = ReadArgs(Commands[function].Template, (APTR)Args, myrdargs)) != NULL)
            {
              result = CallFunction(data, function, Args, NULL);

              FreeArgs(ra_result);
            }
            FreeVecPooled(data->mypool, buffer);
          }
          FreeDosObject(DOS_RDARGS, myrdargs);
        }
      }
      else
        result = CallFunction(data, function, Args, (char *)txtargs);
    }
  }

  RETURN(result);
  return result;
}

///
