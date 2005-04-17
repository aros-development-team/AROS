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

 $Id: Navigation.c,v 1.3 2005/04/04 21:59:02 damato Exp $

***************************************************************************/

#include <clib/alib_protos.h>
#include <clib/graphics_protos.h>
#include <proto/intuition.h>
#include <proto/locale.h>

#include "TextEditor_mcc.h"
#include "private.h"

struct pos_info pos;

ULONG FlowSpace (UWORD flow, STRPTR text, struct InstData *data)
{
    ULONG flowspace = 0;

  if(flow != MUIV_TextEditor_Flow_Left)
  {
    flowspace  = (data->innerwidth-MyTextLength(data->font, text, LineCharsWidth(text, data)-1));
    flowspace -= (data->CursorWidth == 6) ? MyTextLength(data->font, "n", 1) : data->CursorWidth;
    if(flow == MUIV_TextEditor_Flow_Center)
    {
      flowspace /= 2;
    }
  }
  return(flowspace);
}

ULONG CursorOffset (struct InstData *data)
{
    struct line_node *line = data->actualline;
    STRPTR text = line->line.Contents+data->CPos_X;
    LONG  offset = data->pixel_x-FlowSpace(line->line.Flow, text, data);

  if(offset < 1)
    offset = 1;

  return(MyTextFit(data->font, text, LineCharsWidth(text, data)-1, offset, 1));
}

VOID SetBookmark (UWORD nr, struct InstData *data)
{
  if(nr < 4)
  {
    data->bookmarks[nr].line = data->actualline;
    data->bookmarks[nr].x    = data->CPos_X;
  }
}

VOID GotoBookmark (UWORD nr, struct InstData *data)
{
  if(nr < 4)
  {
    if(data->bookmarks[nr].line)
    {
        struct line_node *actual = data->firstline;

      while(actual)
      {
        if(actual == data->bookmarks[nr].line)
        {
          data->actualline = actual;
          data->CPos_X = data->bookmarks[nr].x;
          if(data->CPos_X >= actual->line.Length)
            data->CPos_X = actual->line.Length-1;
          break;
        }
        actual = actual->next;
      }
      if(!actual)
      {
        DoMethod(data->object, MUIM_TextEditor_HandleError, Error_BookmarkHasBeenLost);
      }
    }
    else
    {
      DoMethod(data->object, MUIM_TextEditor_HandleError, Error_NoBookmarkInstalled);
    }
  }
}

/*---- CursorUp ---- */

void  GoTop (struct InstData *data)
{
  data->actualline = data->firstline;
  data->CPos_X = 0;
}

void  GoPreviousLine (struct InstData *data)
{
  if(data->CPos_X == 0 && data->actualline->previous)
    data->actualline = data->actualline->previous;
  data->CPos_X = 0;
}

void  GoPreviousPage (struct InstData *data)
{
  if(data->shown)
  {
    OffsetToLines(data->CPos_X, data->actualline, &pos, data);
    if(!data->pixel_x)
      data->pixel_x = GetPosInPixels(pos.bytes, pos.x, data);
    if(!((!data->actualline->previous) && (pos.lines == 1)))
    {
        LONG   lineplacement;
        LONG   linemove = data->maxlines;

      lineplacement = LineToVisual(data->actualline, data)+pos.lines-1;
      if(lineplacement != 1)
        linemove = lineplacement-1;
      linemove -= pos.lines-1;

      while((linemove) && (data->actualline->previous) && (data->actualline->previous->visual <= linemove))
      {
        data->actualline = data->actualline->previous;
        linemove -= data->actualline->visual;
      }
      data->CPos_X = 0;
      if(linemove && data->actualline->previous)
      {
        if(linemove > 0)
        {
          data->actualline = data->actualline->previous;
          linemove = data->actualline->visual-linemove;
        }
        else
        {
          linemove = -linemove;
        }
        while(linemove--)
        {
          data->CPos_X += LineCharsWidth(data->actualline->line.Contents+data->CPos_X, data);
        }
      }
      data->CPos_X += CursorOffset(data);
    }
    else
    {
      GoTop(data);
    }
  }
}

void  GoUp  (struct InstData *data)
{
  if(data->shown)
  {
    OffsetToLines(data->CPos_X, data->actualline, &pos, data);
    if(!data->pixel_x)
    {
      data->pixel_x = GetPosInPixels(pos.bytes, pos.x, data);
    }
    if(pos.lines == 1)
    {
      if(data->actualline->previous)
      {
        data->actualline = data->actualline->previous;
        pos.lines = data->actualline->visual+1;
      }
      else
      {
//        data->CPos_X = 0;
        return;
      }
    }
    data->CPos_X = 0;
    if(--pos.lines)
    {
      while(--pos.lines)
      {
        data->CPos_X += LineCharsWidth(data->actualline->line.Contents+data->CPos_X, data);
      }
      data->CPos_X += CursorOffset(data);
    }
    else  DisplayBeep(NULL);
  }
}

/*---- CursorDown ---- */

void  GoBottom (struct InstData *data)
{
  while(data->actualline->next)
    data->actualline = data->actualline->next;
  data->CPos_X = data->actualline->line.Length-1;
}

void  GoNextLine  (struct InstData *data)
{
  data->CPos_X = 0;
  if(data->actualline->next)
      data->actualline = data->actualline->next;
  else  data->CPos_X = data->actualline->line.Length-1;
}

void  GoNextPage  (struct InstData *data)
{
  if(data->shown)
  {
    OffsetToLines(data->CPos_X, data->actualline, &pos, data);
    if(!data->pixel_x)
      data->pixel_x = GetPosInPixels(pos.bytes, pos.x, data);
    if(!((!data->actualline->next) && (pos.lines == data->actualline->visual)))
    {
        LONG   lineplacement;
        LONG   linemove = data->maxlines;

      lineplacement = LineToVisual(data->actualline, data)+pos.lines-1;
      if(lineplacement != data->maxlines)
        linemove = data->maxlines - lineplacement;

      linemove += pos.lines-1;
      while((linemove) && (data->actualline->next) && (data->actualline->visual <= linemove))
      {
        linemove -= data->actualline->visual;
        NextLine(data);
      }
      if(data->actualline->visual <= linemove)
        linemove = data->actualline->visual-1;
      data->CPos_X = 0;
      while(linemove--)
      {
        OffsetToLines(data->CPos_X, data->actualline, &pos, data);
        data->CPos_X = pos.extra;
      }
      data->CPos_X += CursorOffset(data);
    }
    else
    {
      GoBottom(data);
    }
  }
}

void  GoDown   (struct InstData *data)
{
  if(data->shown)
  {
    OffsetToLines(data->CPos_X, data->actualline, &pos, data);
    if(!data->pixel_x)
    {
      data->pixel_x = GetPosInPixels(pos.bytes, pos.x, data);
    }
    if(pos.lines == data->actualline->visual)
    {
      if(data->actualline->next)
      {
        pos.lines = 0;
        data->actualline = data->actualline->next;
        data->CPos_X  = 0;
        data->CPos_X += CursorOffset(data);
      }
      else
      {
//        data->CPos_X = data->actualline->line.Length-1;
      }
    }
    else
    {
      data->CPos_X  = pos.extra;
      data->CPos_X += CursorOffset(data);
    }
  }
}

/*---- CursorRight ---- */

void  GoEndOfLine (struct InstData *data)
{
  if(data->shown)
  {
    OffsetToLines(data->CPos_X, data->actualline, &pos, data);
    data->CPos_X = pos.bytes + LineCharsWidth(data->actualline->line.Contents+pos.bytes, data) - 1;
  }
}

void  GoNextWord  (struct InstData *data)
{
  //BOOL EOL = (data->CPos_X == data->actualline->line.Length);

  if(!CheckSep(*(data->actualline->line.Contents+data->CPos_X), data) && (data->CPos_X < data->actualline->line.Length))
  {
    while(!CheckSep(*(data->actualline->line.Contents+data->CPos_X), data) && (data->CPos_X < data->actualline->line.Length))
      data->CPos_X++;
  }

FindNextWord:

  while((CheckSep(*(data->actualline->line.Contents+data->CPos_X), data)) && (data->CPos_X < data->actualline->line.Length))
  {
    data->CPos_X++;
  }
  if(data->CPos_X == data->actualline->line.Length)
  {
    if(data->actualline->next)
    {
      data->actualline = data->actualline->next;
      data->CPos_X = 0;
      goto FindNextWord;
    }
    else  data->CPos_X--;
  }
}

void  GoNextSentence (struct InstData *data)
{
  while((!CheckSent(*(data->actualline->line.Contents+data->CPos_X), data)) && (data->CPos_X < data->actualline->line.Length))
    data->CPos_X++;
  while(CheckSent(*(data->actualline->line.Contents+data->CPos_X), data) && (data->CPos_X < data->actualline->line.Length))
    data->CPos_X++;
  if(data->CPos_X >= data->actualline->line.Length-1)
    NextLine(data);
  else
  {
    while((*(data->actualline->line.Contents+data->CPos_X) == ' ') && (data->CPos_X < data->actualline->line.Length))
      data->CPos_X++;
    if(data->CPos_X == data->actualline->line.Length-1)
      NextLine(data);
  }
}

void  GoRight  (struct InstData *data)
{
  if((LONG)data->actualline->line.Length > (data->CPos_X+1))
      data->CPos_X++;
  else  NextLine(data);
}

/*---- CursorLeft ---- */

void  GoStartOfLine  (struct InstData *data)
{
  if(data->shown)
  {
    OffsetToLines  (data->CPos_X, data->actualline, &pos, data);
    data->CPos_X = pos.bytes;
  }
}


void  GoPreviousWord (struct InstData *data)
{
  //BOOL SOL = (!data->CPos_X);
  BOOL moved = FALSE;

FindWord:

  if(CheckSep(*(data->actualline->line.Contents+data->CPos_X-1), data) && (data->CPos_X > 0))
  {
    data->CPos_X--;
    moved = TRUE;
  }
  if(CheckSep(*(data->actualline->line.Contents+data->CPos_X), data) && (data->CPos_X > 0))
  {
    while(CheckSep(*(data->actualline->line.Contents+data->CPos_X), data) && (data->CPos_X > 0))
    {
      data->CPos_X--;
      moved = TRUE;
    }
  }
  if((data->CPos_X == 0) && (data->actualline->previous) && ((!moved) || (CheckSep(*data->actualline->line.Contents, data))))
  {
    data->actualline = data->actualline->previous;
    data->CPos_X = data->actualline->line.Length-1;
    goto FindWord;
  }

  while((!CheckSep(*(data->actualline->line.Contents+data->CPos_X-1), data)) && (data->CPos_X > 0))
  {
    data->CPos_X--;
  }
}


void  GoPreviousSentence   (struct InstData *data)
{
  while(data->CPos_X == 0 && data->actualline->previous)
  {
    data->actualline = data->actualline->previous;
    data->CPos_X = data->actualline->line.Length-1;
  }
  if(data->CPos_X != 0)
  {
    data->CPos_X--;
    while((*(data->actualline->line.Contents+data->CPos_X) == ' ') && (data->CPos_X > 0))
      data->CPos_X--;
    while(CheckSent(*(data->actualline->line.Contents+data->CPos_X), data) && (data->CPos_X > 0))
      data->CPos_X--;
    while((!CheckSent(*(data->actualline->line.Contents+data->CPos_X), data)) && (data->CPos_X > 0))
      data->CPos_X--;
    if(data->CPos_X > 0)
    {
      data->CPos_X++;
      while((*(data->actualline->line.Contents+data->CPos_X) == ' ') && (data->CPos_X < data->actualline->line.Length))
        data->CPos_X++;
    }
  }
}

void  GoLeft   (struct InstData *data)
{
  if(data->CPos_X > 0)
    data->CPos_X--;
  else
  {
    if(data->actualline->previous)
    {
      data->actualline = data->actualline->previous;
      data->CPos_X = data->actualline->line.Length-1;
    }
  }
}

/*-----------------------------------------*
 * Check if given char is a word-seperator *
 *-----------------------------------------*/
long  CheckSep (unsigned char character, struct InstData *data)
{
  return(!IsAlNum(data->mylocale, (long)character));

/* if((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z')
    || (character >= '0' && character <= '9') || (character >= 192))
      return(FALSE);
  else  return(TRUE);  */
}
/*-----------------------------------------*
 * Check if given char is a sentence ender *
 *-----------------------------------------*/
long CheckSent(unsigned char character, UNUSED struct InstData *data)
{
  if((character == '.') || (character == '!') || (character == '?'))
      return(TRUE);
  else  return(FALSE);
}

/*-----------------------------------*
 * Move cursor to start of next line *
 *-----------------------------------*/
void  NextLine (struct InstData *data)
{
  if(data->actualline->next)
  {
    data->actualline = data->actualline->next;
    data->CPos_X = 0;
  }
  else
  {
    data->CPos_X = data->actualline->line.Length-1;
  }
}
/*---------------------------------------*
 * Return the number of pixels to cursor *
 *---------------------------------------*/
LONG   GetPosInPixels (LONG bytes, LONG x, struct InstData *data)
{
    LONG  pos = MyTextLength(data->font, data->actualline->line.Contents+bytes, x);

  if(*(data->actualline->line.Contents+data->CPos_X) == '\n')
      pos += MyTextLength(data->font, " ", 1)/2;
  else  pos += MyTextLength(data->font, data->actualline->line.Contents+data->CPos_X, 1)/2;
  pos += FlowSpace(data->actualline->line.Flow, data->actualline->line.Contents+bytes, data);
  return(pos);
}

/*-----------------------------------------*
 * Place the cursor, based on an X Y coord *
 *-----------------------------------------*/
void  PosFromCursor(WORD MouseX, WORD MouseY, struct InstData *data)
{
    struct pos_info pos;
#ifdef ClassAct
    LONG limit = 0;
#else
    LONG limit = data->ypos;
#endif

  if(data->maxlines < (data->totallines-data->visual_y+1))
      limit += (data->maxlines * data->height);
  else  limit += (data->totallines-data->visual_y+1)*data->height;

  if(MouseY >= limit)
    MouseY = limit-1;
#ifndef ClassAct
  GetLine(((MouseY - data->ypos)/data->height) + data->visual_y, &pos, data);
#else
  GetLine((MouseY/data->height) + data->visual_y, &pos, data);
#endif
  data->actualline = pos.line;
#ifdef ClassAct
  data->pixel_x = MouseX-data->BevelHoriz-2;
#else
  data->pixel_x = MouseX-data->xpos+1;
#endif
  if(data->pixel_x < 1)
    data->pixel_x = 1;
  data->CPos_X  = pos.x;
  data->CPos_X += CursorOffset(data);
  data->pixel_x = 0;
}
