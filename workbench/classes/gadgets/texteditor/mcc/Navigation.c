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

#include <clib/alib_protos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/locale.h>

#include "private.h"
#include "Debug.h"

struct pos_info pos;

/// FlowSpace()
ULONG FlowSpace(struct InstData *data, UWORD flow, STRPTR text)
{
  ULONG flowspace = 0;

  ENTER();

  if(flow != MUIV_TextEditor_Flow_Left)
  {
    flowspace  = (data->innerwidth-TextLength(&data->tmprp, text, LineCharsWidth(data, text)-1));
    flowspace -= (data->CursorWidth == 6) ? TextLength(&data->tmprp, " ", 1) : data->CursorWidth;
    if(flow == MUIV_TextEditor_Flow_Center)
    {
      flowspace /= 2;
    }
  }

  RETURN(flowspace);
  return flowspace;
}

///
/// CursorOffset()
static ULONG CursorOffset(struct InstData *data)
{
  struct line_node *line = data->actualline;
  STRPTR text = &line->line.Contents[data->CPos_X];
  ULONG res=0;
  ULONG lineCharsWidth;

  ENTER();

  // call TextFit() to find out how many chars would fit.
  if((lineCharsWidth = LineCharsWidth(data, text)) > 0)
  {
    struct TextExtent tExtend;
    LONG offset = data->pixel_x-FlowSpace(data, line->line.Flow, text);

    if(offset < 1)
      offset = 1;

    res = TextFit(&data->tmprp, text, lineCharsWidth, &tExtend, NULL, 1, offset, data->font->tf_YSize);

    // in case of a hard-wrapped line we have to deal with
    // the possibility that the user tries to
    // select the last char in a line which should normally by a white space
    // due to soft-wrapping. So in this case we have to lower res by one so
    // that it is not possible to select that last white space. However, for
    // a hard wrapped line it still have to be possible to select that last char.
    if(lineCharsWidth-res == 0 && text[lineCharsWidth-1] <= ' ')
      res--;
  }

  RETURN(res);
  return res;
}

///
/// GetPosInPixels()
/*---------------------------------------*
 * Return the number of pixels to cursor *
 *---------------------------------------*/
static LONG GetPosInPixels(struct InstData *data, LONG bytes, LONG x)
{
  LONG pos;

  ENTER();

  pos = TextLength(&data->tmprp, &data->actualline->line.Contents[bytes], x);

  if(data->actualline->line.Contents[data->CPos_X] == '\n')
    pos += TextLength(&data->tmprp, " ", 1)/2;
  else
    pos += TextLength(&data->tmprp, &data->actualline->line.Contents[data->CPos_X], 1)/2;

  pos += FlowSpace(data, data->actualline->line.Flow, &data->actualline->line.Contents[bytes]);

  RETURN(pos);
  return(pos);
}

///
/// SetBookmark()
void SetBookmark(struct InstData *data, UWORD nr)
{
  ENTER();

  if(nr < 4)
  {
    data->bookmarks[nr].line = data->actualline;
    data->bookmarks[nr].x    = data->CPos_X;
  }

  LEAVE();
}

///
/// GotoBookmark()
void GotoBookmark(struct InstData *data, UWORD nr)
{
  ENTER();

  if(nr < 4)
  {
    if(data->bookmarks[nr].line)
    {
      struct line_node *actual = data->firstline;

      while(actual != NULL)
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
      if(actual == NULL)
      {
        DoMethod(data->object, MUIM_TextEditor_HandleError, Error_BookmarkHasBeenLost);
      }
    }
    else
    {
      DoMethod(data->object, MUIM_TextEditor_HandleError, Error_NoBookmarkInstalled);
    }
  }

  LEAVE();
}

///
/// GoTop()
/*---- CursorUp ---- */
void GoTop(struct InstData *data)
{
  ENTER();

  data->actualline = data->firstline;
  data->CPos_X = 0;

  LEAVE();
}

///
/// GoPreviousLine()
void GoPreviousLine(struct InstData *data)
{
  ENTER();

  if(data->CPos_X == 0 && data->actualline->previous != NULL)
    data->actualline = data->actualline->previous;
  data->CPos_X = 0;

  LEAVE();
}

///
/// GoPreviousPage()
void GoPreviousPage(struct InstData *data)
{
  ENTER();

  if(data->shown == TRUE)
  {
    OffsetToLines(data, data->CPos_X, data->actualline, &pos);
    if(data->pixel_x == 0)
      data->pixel_x = GetPosInPixels(data, pos.bytes, pos.x);
    if(!(data->actualline->previous == NULL && pos.lines == 1))
    {
      LONG lineplacement;
      LONG linemove = data->maxlines;

      lineplacement = LineToVisual(data, data->actualline)+pos.lines-1;
      if(lineplacement != 1)
        linemove = lineplacement-1;
      linemove -= pos.lines-1;

      while(linemove != 0 && data->actualline->previous != NULL && data->actualline->previous->visual <= linemove)
      {
        data->actualline = data->actualline->previous;
        linemove -= data->actualline->visual;
      }
      data->CPos_X = 0;
      if(linemove != 0 && data->actualline->previous != NULL)
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
          data->CPos_X += LineCharsWidth(data, &data->actualline->line.Contents[data->CPos_X]);
        }
      }
      data->CPos_X += CursorOffset(data);
    }
    else
    {
      GoTop(data);
    }
  }

  LEAVE();
}

///
/// GoUp()
void GoUp(struct InstData *data)
{
  ENTER();

  if(data->shown == TRUE)
  {
    OffsetToLines(data, data->CPos_X, data->actualline, &pos);
    if(data->pixel_x == 0)
    {
      data->pixel_x = GetPosInPixels(data, pos.bytes, pos.x);
    }
    if(pos.lines == 1)
    {
      if(data->actualline->previous != NULL)
      {
        data->actualline = data->actualline->previous;
        pos.lines = data->actualline->visual+1;
      }
      else
      {
//        data->CPos_X = 0;
        LEAVE();
        return;
      }
    }
    data->CPos_X = 0;
    if(--pos.lines)
    {
      while(--pos.lines)
      {
        data->CPos_X += LineCharsWidth(data, &data->actualline->line.Contents[data->CPos_X]);
      }
      data->CPos_X += CursorOffset(data);
    }
    else
      DisplayBeep(NULL);
  }

  LEAVE();
}

///
/// GoBottom()
/*---- CursorDown ---- */
void GoBottom(struct InstData *data)
{
  ENTER();

  while(data->actualline->next != NULL)
    data->actualline = data->actualline->next;
  data->CPos_X = data->actualline->line.Length-1;

  LEAVE();
}

///
/// GoNextLine()
void GoNextLine(struct InstData *data)
{
  ENTER();

  data->CPos_X = 0;
  if(data->actualline->next != NULL)
    data->actualline = data->actualline->next;
  else
    data->CPos_X = data->actualline->line.Length-1;

  LEAVE();
}

///
/// GoNextPage()
void GoNextPage(struct InstData *data)
{
  ENTER();

  if(data->shown == TRUE)
  {
    OffsetToLines(data, data->CPos_X, data->actualline, &pos);
    if(data->pixel_x == 0)
      data->pixel_x = GetPosInPixels(data, pos.bytes, pos.x);
    if(!(data->actualline->next == NULL && pos.lines == data->actualline->visual))
    {
      LONG lineplacement;
      LONG linemove = data->maxlines;

      lineplacement = LineToVisual(data, data->actualline)+pos.lines-1;
      if(lineplacement != data->maxlines)
        linemove = data->maxlines - lineplacement;

      linemove += pos.lines-1;
      while(linemove != 0 && data->actualline->next != NULL && data->actualline->visual <= linemove)
      {
        linemove -= data->actualline->visual;
        NextLine(data);
      }
      if(data->actualline->visual <= linemove)
        linemove = data->actualline->visual-1;
      data->CPos_X = 0;
      while(linemove--)
      {
        OffsetToLines(data, data->CPos_X, data->actualline, &pos);
        data->CPos_X = pos.extra;
      }
      data->CPos_X += CursorOffset(data);
    }
    else
    {
      GoBottom(data);
    }
  }

  LEAVE();
}

///
/// GoDown()
void GoDown (struct InstData *data)
{
  ENTER();

  if(data->shown == TRUE)
  {
    OffsetToLines(data, data->CPos_X, data->actualline, &pos);
    if(data->pixel_x == 0)
    {
      data->pixel_x = GetPosInPixels(data, pos.bytes, pos.x);
    }

    if(pos.lines == data->actualline->visual)
    {
      if(data->actualline->next != NULL)
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

  LEAVE();
}

///
/// GoEndOfLine()
/*---- CursorRight ---- */
void GoEndOfLine(struct InstData *data)
{
  ENTER();

  if(data->shown == TRUE)
  {
    LONG c;

    OffsetToLines(data, data->CPos_X, data->actualline, &pos);

    if((c = LineCharsWidth(data, &data->actualline->line.Contents[pos.bytes])) > 0)
      data->CPos_X = pos.bytes + c - 1;
    else
      data->CPos_X = pos.bytes;
  }

  LEAVE();
}

///
/// GoNextWord()
void GoNextWord(struct InstData *data)
{
  ENTER();

  while(CheckSep(data, data->actualline->line.Contents[data->CPos_X]) == FALSE && data->CPos_X < data->actualline->line.Length)
    data->CPos_X++;

FindNextWord:

  while(CheckSep(data, data->actualline->line.Contents[data->CPos_X]) == TRUE && data->CPos_X < data->actualline->line.Length)
    data->CPos_X++;

  if(data->CPos_X == data->actualline->line.Length)
  {
    if(data->actualline->next != NULL)
    {
      data->actualline = data->actualline->next;
      data->CPos_X = 0;
      goto FindNextWord;
    }
    else
      data->CPos_X--;
  }

  LEAVE();
}

///
/// GoNextSentence()
void GoNextSentence(struct InstData *data)
{
  ENTER();

  while(CheckSent(data, data->actualline->line.Contents[data->CPos_X]) == FALSE && data->CPos_X < data->actualline->line.Length)
    data->CPos_X++;
  while(CheckSent(data, data->actualline->line.Contents[data->CPos_X]) == TRUE && data->CPos_X < data->actualline->line.Length)
    data->CPos_X++;
  if(data->CPos_X >= data->actualline->line.Length-1)
    NextLine(data);
  else
  {
    while(data->actualline->line.Contents[data->CPos_X] == ' ' && data->CPos_X < data->actualline->line.Length)
      data->CPos_X++;
    if(data->CPos_X == data->actualline->line.Length-1)
      NextLine(data);
  }

  LEAVE();
}

///
/// GoRight()
void GoRight(struct InstData *data)
{
  ENTER();

  if((LONG)data->actualline->line.Length > data->CPos_X+1)
    data->CPos_X++;
  else
    NextLine(data);

  LEAVE();
}

///
/// GoStartOfLine()
void GoStartOfLine(struct InstData *data)
{
  ENTER();

  if(data->shown == TRUE)
  {
    OffsetToLines(data, data->CPos_X, data->actualline, &pos);
    data->CPos_X = pos.bytes;
  }

  LEAVE();
}

///
/// GoPreviousWord()
void GoPreviousWord(struct InstData *data)
{
  BOOL moved = FALSE;

  ENTER();

FindWord:

  if(CheckSep(data, data->actualline->line.Contents[data->CPos_X-1]) == TRUE && data->CPos_X > 0)
  {
    data->CPos_X--;
    moved = TRUE;
  }
  while(CheckSep(data, data->actualline->line.Contents[data->CPos_X]) == TRUE && data->CPos_X > 0)
  {
    data->CPos_X--;
    moved = TRUE;
  }
  if(data->CPos_X == 0 && data->actualline->previous != NULL && (moved == FALSE || CheckSep(data, data->actualline->line.Contents[0]) == TRUE))
  {
    data->actualline = data->actualline->previous;
    data->CPos_X = data->actualline->line.Length-1;
    goto FindWord;
  }

  while(CheckSep(data, data->actualline->line.Contents[data->CPos_X-1]) == FALSE && data->CPos_X > 0)
    data->CPos_X--;

  LEAVE();
}

///
/// GoPreviousSentence()
void GoPreviousSentence(struct InstData *data)
{
  ENTER();

  while(data->CPos_X == 0 && data->actualline->previous != NULL)
  {
    data->actualline = data->actualline->previous;
    data->CPos_X = data->actualline->line.Length-1;
  }
  if(data->CPos_X != 0)
  {
    data->CPos_X--;
    while(data->actualline->line.Contents[data->CPos_X] == ' ' && data->CPos_X > 0)
      data->CPos_X--;
    while(CheckSent(data, data->actualline->line.Contents[data->CPos_X]) == TRUE && data->CPos_X > 0)
      data->CPos_X--;
    while(CheckSent(data, data->actualline->line.Contents[data->CPos_X]) == FALSE && data->CPos_X > 0)
      data->CPos_X--;
    if(data->CPos_X > 0)
    {
      data->CPos_X++;
      while(data->actualline->line.Contents[data->CPos_X] == ' ' && data->CPos_X < data->actualline->line.Length)
        data->CPos_X++;
    }
  }

  LEAVE();
}

///
/// GoLeft()
void GoLeft(struct InstData *data)
{
  ENTER();

  if(data->CPos_X > 0)
    data->CPos_X--;
  else if(data->actualline->previous != NULL)
  {
    data->actualline = data->actualline->previous;
    data->CPos_X = data->actualline->line.Length-1;
  }

  LEAVE();
}
///

/// CheckSep()
/*-----------------------------------------*
 * Check if given char is a word-seperator *
 *-----------------------------------------*/
BOOL CheckSep(struct InstData *data, char character)
{
  BOOL isSep = FALSE;

  ENTER();

  if(!IsAlNum(data->mylocale, (long)character))
  	isSep = TRUE;

  RETURN(isSep);
  return isSep;
}

///
/// CheckSent()
/*-----------------------------------------*
 * Check if given char is a sentence ender *
 *-----------------------------------------*/
BOOL CheckSent(UNUSED struct InstData *data, char character)
{
  BOOL isSent = FALSE;

  ENTER();

  if(character == '.' || character == '!' || character == '?')
    isSent = TRUE;

  RETURN(isSent);
  return isSent;
}

///
/// NextLine()
/*-----------------------------------*
 * Move cursor to start of next line *
 *-----------------------------------*/
void NextLine(struct InstData *data)
{
  ENTER();

  if(data->actualline->next != NULL)
  {
    data->actualline = data->actualline->next;
    data->CPos_X = 0;
  }
  else
  {
    data->CPos_X = data->actualline->line.Length-1;
  }

  LEAVE();
}

///
/// PosFromCursor()
/*-----------------------------------------*
 * Place the cursor, based on an X Y coord *
 *-----------------------------------------*/
void PosFromCursor(struct InstData *data, WORD MouseX, WORD MouseY)
{
  struct pos_info pos;
  LONG limit = data->ypos;

  ENTER();

  if(data->maxlines < data->totallines-data->visual_y+1)
    limit += (data->maxlines * data->height);
  else
    limit += (data->totallines-data->visual_y+1)*data->height;

  if(MouseY >= limit)
    MouseY = limit-1;

  GetLine(data, ((MouseY - data->ypos)/data->height) + data->visual_y, &pos);

  data->actualline = pos.line;

  data->pixel_x = MouseX-data->xpos+1;

  if(data->pixel_x < 1)
    data->pixel_x = 1;

  data->CPos_X  = pos.x;
  data->CPos_X += CursorOffset(data);
  data->pixel_x = 0;

  LEAVE();
}

///
