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

#include <clib/alib_protos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/locale.h>

#include "private.h"
#include "Debug.h"

struct pos_info pos;

/// FlowSpace()
LONG FlowSpace(struct InstData *data, UWORD flow, STRPTR text)
{
  LONG flowspace = 0;

  ENTER();

  if(flow != MUIV_TextEditor_Flow_Left)
  {
    flowspace  = (_mwidth(data->object)-TextLengthNew(&data->tmprp, text, LineCharsWidth(data, text)-1, data->TabSizePixels));
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
static LONG CursorOffset(struct InstData *data)
{
  struct line_node *line = data->actualline;
  STRPTR text = &line->line.Contents[data->CPos_X];
  LONG res=0;
  LONG lineCharsWidth;

  ENTER();

  // call TextFitNew() to find out how many chars would fit.
  if((lineCharsWidth = LineCharsWidth(data, text)) > 0)
  {
    struct TextExtent tExtend;
    LONG offset = data->pixel_x-FlowSpace(data, line->line.Flow, text);

    if(offset < 1)
      offset = 1;

    res = TextFitNew(&data->tmprp, text, lineCharsWidth, &tExtend, NULL, 1, offset, data->font->tf_YSize, data->TabSizePixels);

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

  pos = TextLengthNew(&data->tmprp, &data->actualline->line.Contents[bytes], x, data->TabSizePixels);

  if(data->actualline->line.Contents[data->CPos_X] == '\n')
    pos += TextLength(&data->tmprp, " ", 1)/2;
  else
    pos += TextLengthNew(&data->tmprp, &data->actualline->line.Contents[data->CPos_X], 1, data->TabSizePixels)/2;

  pos += FlowSpace(data, data->actualline->line.Flow, &data->actualline->line.Contents[bytes]);

  RETURN(pos);
  return(pos);
}

///
/// SetBookmark()
void SetBookmark(struct InstData *data, ULONG nr)
{
  ENTER();

  if(nr < ARRAY_SIZE(data->bookmarks))
  {
    data->bookmarks[nr].line = data->actualline;
    data->bookmarks[nr].x    = data->CPos_X;
  }

  LEAVE();
}

///
/// GotoBookmark()
void GotoBookmark(struct InstData *data, ULONG nr)
{
  ENTER();

  if(nr < ARRAY_SIZE(data->bookmarks))
  {
    if(data->bookmarks[nr].line)
    {
      struct line_node *actual = GetFirstLine(&data->linelist);

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
        actual = GetNextLine(actual);
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

  data->actualline = GetFirstLine(&data->linelist);
  data->CPos_X = 0;

  LEAVE();
}

///
/// GoPreviousLine()
void GoPreviousLine(struct InstData *data)
{
  ENTER();

  if(data->CPos_X == 0 && HasPrevLine(data->actualline) == TRUE)
    data->actualline = GetPrevLine(data->actualline);
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
    if(!(HasPrevLine(data->actualline) == FALSE && pos.lines == 1))
    {
      LONG lineplacement;
      LONG linemove = data->maxlines;
      struct line_node *prev;

      lineplacement = LineToVisual(data, data->actualline)+pos.lines-1;
      if(lineplacement != 1)
        linemove = lineplacement-1;
      linemove -= pos.lines-1;

      while(linemove != 0 && (prev = GetPrevLine(data->actualline)) != NULL && prev->visual <= linemove)
      {
        data->actualline = prev;
        linemove -= data->actualline->visual;
      }
      data->CPos_X = 0;
      if(linemove != 0 && HasPrevLine(data->actualline) == TRUE)
      {
        if(linemove > 0)
        {
          data->actualline = GetPrevLine(data->actualline);
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
      if(HasPrevLine(data->actualline) == TRUE)
      {
        data->actualline = GetPrevLine(data->actualline);
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

  data->actualline = GetLastLine(&data->linelist);
  data->CPos_X = data->actualline->line.Length-1;

  LEAVE();
}

///
/// GoNextLine()
void GoNextLine(struct InstData *data)
{
  ENTER();

  data->CPos_X = 0;
  if(HasNextLine(data->actualline) == TRUE)
    data->actualline = GetNextLine(data->actualline);
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
    if(!(HasNextLine(data->actualline) == FALSE&& pos.lines == data->actualline->visual))
    {
      LONG lineplacement;
      LONG linemove = data->maxlines;

      lineplacement = LineToVisual(data, data->actualline)+pos.lines-1;
      if(lineplacement != data->maxlines)
        linemove = data->maxlines - lineplacement;

      linemove += pos.lines-1;
      while(linemove != 0 && HasNextLine(data->actualline) == TRUE && data->actualline->visual <= linemove)
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
      struct line_node *next = GetNextLine(data->actualline);

      if(next != NULL)
      {
        pos.lines = 0;
        data->actualline = next;
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

  while(data->CPos_X < data->actualline->line.Length && CheckSep(data, data->actualline->line.Contents[data->CPos_X]) == FALSE)
    data->CPos_X++;

FindNextWord:

  while(data->CPos_X < data->actualline->line.Length && CheckSep(data, data->actualline->line.Contents[data->CPos_X]) == TRUE)
    data->CPos_X++;

  if(data->CPos_X == data->actualline->line.Length)
  {
    struct line_node *next = GetNextLine(data->actualline);

    if(next != NULL)
    {
      data->actualline = next;
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

  while(data->CPos_X < data->actualline->line.Length && CheckSent(data, data->actualline->line.Contents[data->CPos_X]) == FALSE)
    data->CPos_X++;
  while(data->CPos_X < data->actualline->line.Length && CheckSent(data, data->actualline->line.Contents[data->CPos_X]) == TRUE)
    data->CPos_X++;
  if(data->CPos_X >= data->actualline->line.Length-1)
    NextLine(data);
  else
  {
    while(data->CPos_X < data->actualline->line.Length && data->actualline->line.Contents[data->CPos_X] == ' ')
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

  if(data->CPos_X+1 < data->actualline->line.Length)
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
  struct line_node *prev;

  ENTER();

FindWord:

  if(data->CPos_X > 0 && CheckSep(data, data->actualline->line.Contents[data->CPos_X-1]) == TRUE)
  {
    data->CPos_X--;
    moved = TRUE;
  }
  while(data->CPos_X > 0 && CheckSep(data, data->actualline->line.Contents[data->CPos_X]) == TRUE)
  {
    data->CPos_X--;
    moved = TRUE;
  }
  prev = GetPrevLine(data->actualline);
  if(data->CPos_X == 0 && prev != NULL && (moved == FALSE || CheckSep(data, data->actualline->line.Contents[0]) == TRUE))
  {
    data->actualline = prev;
    data->CPos_X = data->actualline->line.Length-1;
    goto FindWord;
  }

  while(data->CPos_X > 0 && CheckSep(data, data->actualline->line.Contents[data->CPos_X-1]) == FALSE)
    data->CPos_X--;

  LEAVE();
}

///
/// GoPreviousSentence()
void GoPreviousSentence(struct InstData *data)
{
  struct line_node *prev;

  ENTER();

  prev = GetPrevLine(data->actualline);
  while(data->CPos_X == 0 && prev != NULL)
  {
    data->actualline = prev;
    data->CPos_X = data->actualline->line.Length-1;
    prev = GetPrevLine(prev);
  }
  if(data->CPos_X != 0)
  {
    data->CPos_X--;
    while(data->CPos_X > 0 && data->actualline->line.Contents[data->CPos_X] == ' ')
      data->CPos_X--;
    while(data->CPos_X > 0 && CheckSent(data, data->actualline->line.Contents[data->CPos_X]) == TRUE)
      data->CPos_X--;
    while(data->CPos_X > 0 && CheckSent(data, data->actualline->line.Contents[data->CPos_X]) == FALSE)
      data->CPos_X--;
    if(data->CPos_X > 0)
    {
      data->CPos_X++;
      while(data->CPos_X < data->actualline->line.Length && data->actualline->line.Contents[data->CPos_X] == ' ')
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
  else
  {
    struct line_node *prev = GetPrevLine(data->actualline);

    if(prev != NULL)
    {
      data->actualline = prev;
      data->CPos_X = data->actualline->line.Length-1;
    }
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
  struct line_node *next;

  ENTER();

  next = GetNextLine(data->actualline);
  if(next != NULL)
  {
    data->actualline = next;
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
void PosFromCursor(struct InstData *data, LONG MouseX, LONG MouseY)
{
  struct pos_info pos;
  LONG limit = data->ypos;

  ENTER();

  if(data->maxlines < data->totallines-data->visual_y+1)
    limit += (data->maxlines * data->fontheight);
  else
    limit += (data->totallines-data->visual_y+1)*data->fontheight;

  if(MouseY >= limit)
    MouseY = limit-1;

  GetLine(data, ((MouseY - data->ypos)/data->fontheight) + data->visual_y, &pos);

  data->actualline = pos.line;

  data->pixel_x = MouseX-_mleft(data->object)+1;

  if(data->pixel_x < 1)
    data->pixel_x = 1;

  data->CPos_X  = pos.x;
  data->CPos_X += CursorOffset(data);
  data->pixel_x = 0;

  LEAVE();
}

///
