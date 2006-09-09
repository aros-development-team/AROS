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

#include <stdio.h>
#include <string.h>

#include <exec/memory.h>
#include <proto/exec.h>

#include "TextEditor_mcc.h"
#include "private.h"

struct Buffer
{
  STRPTR  buffer;
  STRPTR  pointer;
  ULONG   flow;
  ULONG   size;
};

#ifdef __AROS__
AROS_HOOKPROTONO(ExportHookFunc, STRPTR, struct ExportMessage *, emsg)
#else
HOOKPROTONO(ExportHookFunc, STRPTR, struct ExportMessage *emsg)
#endif
{
  HOOK_INIT
  
  struct Buffer *buf = emsg->UserData;
  LONG length;
  UWORD *styles = emsg->Styles;
  UWORD *colors = emsg->Colors;
  UWORD lastpos = 0;
  UWORD currentstyle = 0;
  STRPTR result, startx;
  struct InstData *data = emsg->data;
  LONG expand;

  ENTER();

  if(!buf)
  {
    if(data)
    {
      buf         = MyAllocPooled(data->mypool, sizeof(struct Buffer));
      buf->buffer = MyAllocPooled(data->mypool, 512);
      buf->size   = 512;
    }
    else
    {
      buf         = AllocVec(sizeof(struct Buffer), MEMF_ANY|MEMF_CLEAR);
      buf->buffer = AllocVec(1024, MEMF_ANY|MEMF_CLEAR);
      buf->size   = 1024;
    }

    buf->pointer  = buf->buffer;
    buf->flow   = 0;
  }

  expand = 10*emsg->Length;

  if(buf->buffer+buf->size < buf->pointer+expand)
  {
    STRPTR oldbuf = buf->buffer;
    ULONG oldsize = buf->size;
    ULONG offset = buf->pointer - oldbuf;

    if(data)
    {
      buf->buffer = MyAllocPooled(data->mypool, oldsize+expand+512);
      buf->size = oldsize+expand+512;
    }
    else
    {
      buf->buffer = AllocVec(oldsize+expand+1024, MEMF_ANY|MEMF_CLEAR);
      buf->size = oldsize+expand+1024;
    }
    buf->pointer = buf->buffer+offset;

    CopyMem(oldbuf, buf->buffer, offset);

    if(data)
      MyFreePooled(data->mypool, oldbuf);
    else
      FreeVec(oldbuf);
  }

  // if h_Data is TRUE then this is an EMailHook call
  if(emsg->Highlight && !hook->h_Data)
  {
    *buf->pointer++ = '\033';
    *buf->pointer++ = 'h';
  }

  // if h_Data is TRUE then this is an EMailHook call
  if(!hook->h_Data && emsg->Flow != buf->flow)
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
    }

    buf->flow = emsg->Flow;
  }

  if(emsg->Separator)
  {
    // if h_Data is TRUE then this is an EMailHook call
    if(!hook->h_Data)
      sprintf(buf->pointer, "\033[s:%ld]", (LONG)emsg->Separator);
    else
      strcpy(buf->pointer, ((emsg->Separator & LNSF_Thick) ? "<tsb>" : "<sb>"));

    buf->pointer += strlen(buf->pointer);
  }

  startx = buf->pointer;
  length = emsg->Length;
  if(styles || colors)
  {
    UWORD pos;
    WORD style;
    BOOL coloured = FALSE;
    UWORD colour_state = 7;

    while((styles && *styles != 0xffff) || (colors && *colors != 0xffff))
    {
      BOOL color;

      if(colors == NULL || (styles && (coloured ? *styles < *colors : *styles <= *colors)))
      {
        pos = *styles++ - 1;
        style = *styles++;
        color = FALSE;
      }
      else
      {
        pos = *colors++ - 1;
        style = *colors++;
        color = TRUE;
      }

      CopyMem(emsg->Contents+lastpos, buf->pointer, pos-lastpos);
      buf->pointer += pos-lastpos;

      // if h_Data is TRUE then this is an EMailHook call
      if(hook->h_Data)
      {
        if(color)
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
      else
      {
        if(color)
        {
          sprintf(buf->pointer, "\033p[%ld]", (LONG)style);
          buf->pointer += strlen(buf->pointer);
        }
        else
        {
          switch(style)
          {
            case UNDERLINE:
            {
              *buf->pointer++ = '\033';
              *buf->pointer++ = 'u';
              currentstyle |= UNDERLINE;
            }
            break;

            case BOLD:
            {
              *buf->pointer++ = '\033';
              *buf->pointer++ = 'b';
              currentstyle |= BOLD;
            }
            break;

            case ITALIC:
            {
              *buf->pointer++ = '\033';
              *buf->pointer++ = 'i';
              currentstyle |= ITALIC;
            }
            break;

            case ~UNDERLINE:
            case ~BOLD:
            case ~ITALIC:
            {
              currentstyle &= style;

              if(pos+1 != *styles || *(styles+1) < 0x8000)
              {
                *buf->pointer++ = '\033';
                *buf->pointer++ = 'n';
                if(currentstyle & UNDERLINE)
                {
                  *buf->pointer++ = '\033';
                  *buf->pointer++ = 'u';
                }
                if(currentstyle & BOLD)
                {
                  *buf->pointer++ = '\033';
                  *buf->pointer++ = 'b';
                }
                if(currentstyle & ITALIC)
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

      length -= pos-lastpos;
      
      if(length == -1)
        length = 0;
      lastpos = pos;
    }
  }

  CopyMem(emsg->Contents+lastpos, buf->pointer, length);
  buf->pointer += length;

  while(emsg->ExportWrap && buf->pointer-startx > (LONG)emsg->ExportWrap)
  {
    ULONG max = emsg->ExportWrap+1;

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

  if(emsg->Last)
  {
    result = buf->buffer;

    if(data)
    {
      MyFreePooled(data->mypool, buf);
    }
    else
    {
      FreeVec(buf);
    }
  }
  else
  {
    result = (STRPTR)buf;
  }

  RETURN(result);
  return(result);
  
  HOOK_EXIT
}
MakeHookWithData(ExportHookPlain, ExportHookFunc, FALSE);
MakeHookWithData(ExportHookEMail, ExportHookFunc, TRUE);
