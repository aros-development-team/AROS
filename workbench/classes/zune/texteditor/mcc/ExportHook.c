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

#include <stdio.h>
#include <string.h>

#include <exec/memory.h>
#include <proto/exec.h>

#include "private.h"
#include "Debug.h"

struct Buffer
{
  STRPTR  buffer;
  STRPTR  pointer;
  ULONG   flow;
  ULONG   size;
};

/// ExportHookFunc()
HOOKPROTONO(ExportHookFunc, STRPTR, struct ExportMessage *emsg)
{
  struct Buffer *buf = emsg->UserData;
  struct InstData *data = emsg->data;
  STRPTR result = NULL;

  ENTER();

  // create a temporary buffer if we don't have one yet
  if(buf == NULL)
  {
    if((buf = AllocVecShared(sizeof(*buf), MEMF_CLEAR)) != NULL)
    {
      if(data != NULL)
      {
        // respect our pool's threshold size
        buf->buffer = AllocVecPooled(data->mypool, 512);
        buf->size = 512;
      }
      else
      {
        buf->buffer = AllocVecShared(1024, MEMF_CLEAR);
        buf->size = 1024;
      }
    }

    if(buf != NULL)
    {
      buf->pointer = buf->buffer;
      buf->flow = 0;
    }
  }

  if(buf != NULL && buf->buffer != NULL)
  {
    LONG expand = emsg->Length * 10;

    if(buf->buffer+buf->size < buf->pointer+expand)
    {
      // remember the current buffer, size and offset
      STRPTR oldbuf = buf->buffer;
      ULONG oldsize = buf->size;
      ULONG offset = buf->pointer - oldbuf;

      // allocate an expanded buffer
      if(data != NULL)
      {
        // respect our pool's threshold size
        buf->buffer = AllocVecPooled(data->mypool, oldsize+expand+512);
        buf->size = oldsize+expand+512;
      }
      else
      {
        buf->buffer = AllocVecShared(oldsize+expand+1024, MEMF_CLEAR);
        buf->size = oldsize+expand+1024;
      }

      if(buf->buffer != NULL)
      {
        buf->pointer = buf->buffer+offset;

        memcpy(buf->buffer, oldbuf, offset);
      }
      else
      {
        W(DBF_BLOCK, "failed to allocate temporary buffer");
        emsg->failure = TRUE;
      }

      // free the old buffer
      if(data != NULL)
        FreeVecPooled(data->mypool, oldbuf);
      else
        FreeVec(oldbuf);
    }

    if(emsg->failure == FALSE)
    {
      LONG length;
      struct LineStyle *styles = emsg->Styles;
      struct LineColor *colors = emsg->Colors;
      LONG lastpos = 0;
      UWORD currentstyle = 0;
      ULONG hookType = (IPTR)hook->h_Data;
      STRPTR startx;

      // if this hook is of plain type we have consider highlighting as well.
      if(hookType == MUIV_TextEditor_ExportHook_Plain && emsg->Highlight == TRUE)
      {
        *buf->pointer++ = '\033';
        *buf->pointer++ = 'h';
      }

      if(hookType == MUIV_TextEditor_ExportHook_Plain && emsg->Flow != buf->flow)
      {
        *buf->pointer++ = '\033';

        switch(emsg->Flow)
        {
          case MUIV_TextEditor_Flow_Right:
            *buf->pointer++ = 'r';
          break;

          case MUIV_TextEditor_Flow_Center:
            *buf->pointer++ = 'c';
          break;

          case MUIV_TextEditor_Flow_Left:
          default:
            *buf->pointer++ = 'l';
          break;
        }

        buf->flow = emsg->Flow;
      }

      if(emsg->Separator != LNSF_None)
      {
        if(hookType == MUIV_TextEditor_ExportHook_Plain)
          snprintf(buf->pointer, buf->size-(buf->pointer-buf->buffer), "\033[s:%d]", emsg->Separator);
        else
          strlcpy(buf->pointer, isFlagSet(emsg->Separator, LNSF_Thick) ? "<tsb>" : "<sb>", buf->size-(buf->pointer-buf->buffer));

        buf->pointer += strlen(buf->pointer);
      }

      // define some start values.
      startx = buf->pointer;
      length = emsg->Length-emsg->SkipFront-emsg->SkipBack;
      lastpos = emsg->SkipFront;

      if((styles != NULL || colors != NULL) &&
         (hookType == MUIV_TextEditor_ExportHook_EMail || hookType == MUIV_TextEditor_ExportHook_Plain))
      {
        LONG pos;
        WORD style;
        BOOL coloured = FALSE;
        UWORD colour_state = 7;
        LONG nextStyleColumn;
        LONG nextColorColumn;

        if(styles == NULL)
          nextStyleColumn = EOS;
        else
          nextStyleColumn = styles[0].column;

        if(colors == NULL)
          nextColorColumn = EOC;
        else
          nextColorColumn = colors[0].column;

        while(length > 0 && (nextStyleColumn != EOS || nextColorColumn != EOC))
        {
          BOOL isColorChange;
          LONG len;

          SHOWVALUE(DBF_EXPORT, nextStyleColumn);
          SHOWVALUE(DBF_EXPORT, nextColorColumn);

          // check whether a style change or a color change is first to be handled
          if((coloured == TRUE  && nextStyleColumn <  nextColorColumn) ||
             (coloured == FALSE && nextStyleColumn <= nextColorColumn))
          {
            pos = styles->column - 1;
            style = styles->style;
            isColorChange = FALSE;
            // remember the next style change column
            styles++;
            nextStyleColumn = styles->column;
          }
          else
          {
            pos = colors->column - 1;
            style = colors->color;
            isColorChange = TRUE;
            // remember the next style change column
            colors++;
            nextColorColumn = colors->column;
          }

          // skip styles&colors which below lastpos
          if(pos < lastpos)
            continue;

          // depending on how much we export
          // we have to fire up the style convert routines.
          if(pos-lastpos <= length)
          {
            len = pos-lastpos;
            memcpy(buf->pointer, emsg->Contents+lastpos, len);
            buf->pointer += len;

            if(hookType == MUIV_TextEditor_ExportHook_EMail)
            {
              if(isColorChange == TRUE)
              {
                if((coloured = (style == colour_state ? TRUE : FALSE)))
                {
                  *buf->pointer++ = '#';
                  colour_state ^= 7;
                }
              }
              else
              {
                switch(style)
                {
                  case UNDERLINE:
                  case ~UNDERLINE:
                  {
                    *buf->pointer++ = '_';
                  }
                  break;

                  case BOLD:
                  case ~BOLD:
                  {
                    *buf->pointer++ = '*';
                  }
                  break;

                  case ITALIC:
                  case ~ITALIC:
                  {
                    *buf->pointer++ = '/';
                  }
                  break;
                }
              }
            }
            else if(hookType == MUIV_TextEditor_ExportHook_Plain)
            {
              if(isColorChange == TRUE)
              {
                D(DBF_EXPORT, "exporting color %ld of column %ld", style, pos+1);
                snprintf(buf->pointer, buf->size-(buf->pointer-buf->buffer), "\033p[%d]", style);
                buf->pointer += strlen(buf->pointer);
              }
              else
              {
                switch(style)
                {
                  case BOLD:
                  {
                    D(DBF_EXPORT, "exporting bold style of column %ld", pos+1);
                    *buf->pointer++ = '\033';
                    *buf->pointer++ = 'b';
                    setFlag(currentstyle, BOLD);
                  }
                  break;

                  case ITALIC:
                  {
                    D(DBF_EXPORT, "exporting italic style of column %ld", pos+1);
                    *buf->pointer++ = '\033';
                    *buf->pointer++ = 'i';
                    setFlag(currentstyle, ITALIC);
                  }
                  break;

                  case UNDERLINE:
                  {
                    D(DBF_EXPORT, "exporting underline style of column %ld", pos+1);
                    *buf->pointer++ = '\033';
                    *buf->pointer++ = 'u';
                    setFlag(currentstyle, UNDERLINE);
                  }
                  break;

                  case ~BOLD:
                  case ~ITALIC:
                  case ~UNDERLINE:
                  {
                    currentstyle &= style;

                    if(pos+1 != styles->column || styles->style < 0x8000)
                    {
                      D(DBF_EXPORT, "exporting plain style of column %ld", pos+1);
                      *buf->pointer++ = '\033';
                      *buf->pointer++ = 'n';

                      if(isFlagSet(currentstyle, UNDERLINE))
                      {
                        *buf->pointer++ = '\033';
                        *buf->pointer++ = 'u';
                      }

                      if(isFlagSet(currentstyle, BOLD))
                      {
                        *buf->pointer++ = '\033';
                        *buf->pointer++ = 'b';
                      }

                      if(isFlagSet(currentstyle, ITALIC))
                      {
                        *buf->pointer++ = '\033';
                        *buf->pointer++ = 'i';
                      }
                    }
                  }
                  break;
                }
              }
            }
          }
          else
          {
            len = length;
            memcpy(buf->pointer, emsg->Contents+lastpos, len);
            buf->pointer += len;
          }

          length -= len;

          if(length == -1)
            length = 0;

          lastpos = pos;
        }
      }

      if(length > 0)
      {
        memcpy(buf->pointer, emsg->Contents+lastpos, length);
        buf->pointer += length;
      }

      // NUL terminate our buffer string
      *buf->pointer = '\0';

      while(emsg->ExportWrap != 0 && buf->pointer-startx > emsg->ExportWrap)
      {
        LONG max = emsg->ExportWrap+1;

        if(startx[emsg->ExportWrap] != '\n')
        {
          while(--max && *(startx+max) != ' ')
            ; // empty while
        }

        if(max)
        {
          *(startx += max) = '\n';
        }
        else
        {
          while(++startx < buf->pointer && *(startx) != ' ')
            ; // empty while

          if(buf->pointer != startx)
          {
            *startx = '\n';
          }
        }
      }
    }

    if(emsg->Last == TRUE)
    {
      // free the temporary struct after we processed the last line
      result = buf->buffer;
      FreeVec(buf);
    }
    else
    {
      // return the temporary struct for the next call
      result = (STRPTR)buf;
    }
  }
  else
  {
    W(DBF_BLOCK, "failed to allocate temporary buffer");
    SHOWVALUE(DBF_BLOCK, buf);
    if(buf != NULL)
      SHOWVALUE(DBF_BLOCK, buf->buffer);
    emsg->failure = TRUE;
  }

  RETURN(result);
  return result;
}

MakeHookWithData(ExportHookPlain, ExportHookFunc, MUIV_TextEditor_ExportHook_Plain);
MakeHookWithData(ExportHookEMail, ExportHookFunc, MUIV_TextEditor_ExportHook_EMail);
MakeHookWithData(ExportHookNoStyle, ExportHookFunc, MUIV_TextEditor_ExportHook_NoStyle);

///
