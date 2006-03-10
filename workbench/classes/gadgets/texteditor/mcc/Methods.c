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

 $Id: Methods.c,v 1.9 2005/12/06 23:41:22 damato Exp $

***************************************************************************/

#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "TextEditor_mcc.h"
#include "private.h"

ULONG FlowSpace(UWORD, STRPTR, struct InstData *);

ULONG OM_BlockInfo(struct MUIP_TextEditor_BlockInfo *msg, struct InstData *data)
{
  ULONG result = FALSE;

  ENTER();

  if(Enabled(data))
  {
    struct marking newblock;
    NiceBlock(&data->blockinfo, &newblock);
    *(msg->startx)    = newblock.startx;
    *(msg->stopx)   = newblock.stopx;
    *(msg->starty)    = LineNr(newblock.startline, data)-1;
    *(msg->stopy)   = LineNr(newblock.stopline, data)-1;

    result = TRUE;
  }

  RETURN(result);
  return(result);
}

ULONG OM_MarkText (struct MUIP_TextEditor_MarkText *msg, struct InstData *data)
{
  ENTER();

  if(Enabled(data))
  {
    data->blockinfo.enabled = FALSE;
    MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
  }
  else
  {
    SetCursor(data->CPos_X, data->actualline, FALSE, data);
  }

  data->blockinfo.startline = LineNode(msg->start_crsr_y+1, data);
  data->blockinfo.startx = (data->blockinfo.startline->line.Length > msg->start_crsr_x) ? msg->start_crsr_x : data->blockinfo.startline->line.Length-1;
  data->blockinfo.stopline = LineNode(msg->stop_crsr_y+1, data);
  data->blockinfo.stopx = (data->blockinfo.stopline->line.Length > msg->stop_crsr_x) ? msg->stop_crsr_x : data->blockinfo.stopline->line.Length-1;
  data->blockinfo.enabled = TRUE;

  data->actualline = data->blockinfo.stopline;
  data->CPos_X = data->blockinfo.stopx;
  ScrollIntoDisplay(data);
  MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);

  RETURN(TRUE);
  return(TRUE);
}

ULONG ClearText (struct InstData *data)
{
  struct line_node *newcontents;

  ENTER();
  
  if((newcontents = AllocLine(data)))
  {
    if(Init_LineNode(newcontents, NULL, "\n", data))
    {
      if(data->undosize)
      {
        struct  marking newblock;

        newblock.startx = 0;
        newblock.startline = data->firstline;
        newblock.stopline = data->firstline;
        while(newblock.stopline->next)
        {
          newblock.stopline = newblock.stopline->next;
        }
        newblock.stopx = newblock.stopline->line.Length-1;

        data->CPos_X = 0;
        data->actualline = data->firstline;
        AddToUndoBuffer(deleteblock, (char *)&newblock, data);
      }
      FreeTextMem(data->firstline, data);
      data->firstline = newcontents;
      ResetDisplay(data);
    }
    else
    {
      FreeLine(newcontents, data);
    }
  }

  RETURN(TRUE);
  return(TRUE);
}


ULONG ToggleCursor (struct InstData *data)
{
  ENTER();

  if(data->flags & FLG_Active)
  {
    if(data->cursor_shown)
    {
      SetCursor(data->CPos_X, data->actualline, FALSE, data);
      data->cursor_shown = FALSE;
    }
    else
    {
      SetCursor(data->CPos_X, data->actualline, TRUE, data);
      data->cursor_shown = TRUE;
    }
  }

  RETURN(TRUE);
  return(TRUE);
}

ULONG InputTrigger(UNUSED struct IClass *cl, struct InstData *data)
{
  ENTER();

  if(data->smooth_wait == 1 && data->scrollaction)
  {
    if(data->ypos != data->realypos)
    {
        LONG  move;

      if(data->scr_direction)
          move = data->height-(data->realypos-data->ypos);
      else  move = -(data->realypos-data->ypos);

      if(move != 1 && move != -1)
        move  = (move*2)/3;

      SetAttrs(data->object, MUIA_TextEditor_Prop_First, (data->visual_y-1)*data->height+(data->realypos-data->ypos)+move, TAG_DONE);
    }
    else
    {
      data->scrollaction = FALSE;
      RejectInput(data);

      if(data->flags & FLG_Active)
      {
        SetCursor(data->CPos_X, data->actualline, TRUE, data);
      }
    }
  }

#ifndef ClassAct
  if(data->mousemove)
  {
      LONG  MouseX = muiRenderInfo(data->object)->mri_Window->MouseX,
          MouseY = muiRenderInfo(data->object)->mri_Window->MouseY;
      LONG  oldCPos_X = data->CPos_X;
      struct line_node *oldactualline = data->actualline;
      BOOL  Scroll = TRUE;

    ULONG active;
    get(_win(data->object), MUIA_Window_Activate, &active);
    if(!active)
    {
      data->mousemove = FALSE;
      RejectInput(data);

      RETURN(TRUE);
      return(TRUE);
    }

    if(MouseY < data->ypos)
    {
        LONG diff = data->ypos - MouseY;

      if(diff > 30)
        GoUp(data);
      if(diff > 20)
        GoUp(data);
      if(diff > 10)
        GoUp(data);
      GoUp(data);
      MouseY = data->ypos;
    }
    else
    {
        LONG limit = data->ypos;

      if(data->maxlines < (data->totallines-data->visual_y+1))
          limit += (data->maxlines * data->height);
      else  limit += (data->totallines-data->visual_y+1)*data->height;

      if(MouseY >= limit)
      {
          LONG diff = MouseY - limit;

        if(diff > 30)
          GoDown(data);
        if(diff > 20)
          GoDown(data);
        if(diff > 10)
          GoDown(data);
        GoDown(data);
      }
      else
      {
        PosFromCursor(MouseX, MouseY, data);
        Scroll = FALSE;
      }
    }

    if(data->blockinfo.enabled && Scroll && data->blockinfo.stopline == data->actualline && data->blockinfo.stopx == data->CPos_X)
    {
      PosFromCursor(MouseX, MouseY, data);
    }

    if(data->selectmode != 0)
    {
      struct marking tmpblock;

      NiceBlock(&data->blockinfo, &tmpblock);
      if(data->blockinfo.startx == tmpblock.startx && data->blockinfo.startline == tmpblock.startline)
      {
        if(MouseX > data->xpos)
        {
          if(data->selectmode == 1)
          {
            while((data->CPos_X < data->actualline->line.Length-1) && !CheckSep(*(data->actualline->line.Contents+data->CPos_X), data))
//            while((data->CPos_X < data->actualline->line.Length-1) && (*(data->actualline->line.Contents+data->CPos_X) != ' '))
              data->CPos_X++;
          }
          else
          {
            GoEndOfLine(data);
          }
        }
      }
      else
      {
        struct pos_info pos;
        ULONG flow;

        OffsetToLines(data->CPos_X, data->actualline, &pos, data);
        flow = FlowSpace(data->actualline->line.Flow, data->actualline->line.Contents+pos.bytes, data);
        
        if(MouseX <= (LONG)(data->xpos+flow+TextLength(&data->tmprp, data->actualline->line.Contents+pos.bytes, pos.extra-pos.bytes-1)))
        {
          if(data->selectmode == 1)
          {
            while(data->CPos_X > 0 && !CheckSep(*(data->actualline->line.Contents+data->CPos_X-1), data))
//            while(data->CPos_X > 0 && *(data->actualline->line.Contents+data->CPos_X-1) != ' ')
              data->CPos_X--;
          }
          else
          {
            GoStartOfLine(data);
          }
        }
      }
    }

    if(data->blockinfo.enabled || data->selectmode == 0)
    {
      // if selectmode == 2, then the user has trippleclicked at the line
      // and wants to get the whole line marked
      if(data->selectmode == 2 || data->selectmode == 3)
      {
        data->selectmode = 2;

        // if the line is a hard wrapped one we have to increase CPos_X by one
        if(data->actualline->line.Contents[data->CPos_X] > ' ')
        {
          data->CPos_X++;
          MarkText(data->blockinfo.stopx, data->blockinfo.stopline, data->CPos_X, data->actualline, data);
        }
        else
          MarkText(data->blockinfo.stopx, data->blockinfo.stopline, data->CPos_X+1, data->actualline, data);

        data->selectmode = 3;
      }
      else if(data->blockinfo.stopline != data->actualline || data->blockinfo.stopx != data->CPos_X)
      {
        data->blockinfo.enabled = TRUE;
        MarkText(data->blockinfo.stopx, data->blockinfo.stopline, data->CPos_X, data->actualline, data);
      }

      data->blockinfo.stopline = data->actualline;
      data->blockinfo.stopx = data->CPos_X;
    }
    else
    {
      data->mousemove = FALSE;
      RejectInput(data);
    }

    if((oldCPos_X != data->CPos_X) || (oldactualline != data->actualline))
    {
      ScrollIntoDisplay(data);
      PosFromCursor(MouseX, MouseY, data);
    }
  }
#endif

  RETURN(TRUE);
  return(TRUE);
}


ULONG InsertText (struct InstData *data, STRPTR text, BOOL movecursor)
{
  struct line_node *line, *actline = data->actualline;
  UWORD x = data->CPos_X;
  UWORD realx = 0;

  ENTER();

  if((line = ImportText(text, data, data->ImportHook, data->ImportWrap)))
  {
    long  oneline = FALSE;
    long  newline = FALSE;
    struct line_node *startline = line;

    line->visual    = VisualHeight(line, data);
    data->totallines += line->visual;
    while(line->next)
    {
      line = line->next;
      line->visual    = VisualHeight(line, data);
      data->totallines += line->visual;
    }
    if(*(line->line.Contents+line->line.Length) == '\n')
      newline = TRUE;

    data->update = FALSE;
    SplitLine(x, actline, FALSE, NULL, data);
    line->next = actline->next;
    actline->next->previous = line;
    actline->next = startline;
    startline->previous = actline;
    data->CPos_X = line->line.Length-1;
    if(actline->next == line)
    {
      data->CPos_X += actline->line.Length-1;
      oneline = TRUE;
    }
    if(!newline)
      MergeLines(line, data);
    MergeLines(actline, data);
    if(oneline)
      line = actline;
    if(newline)
    {
      line = line->next;
      data->CPos_X = 0;
    }
    if(movecursor)
    {
      data->actualline = line;
    }
    else
    {
      realx = data->CPos_X;
      data->CPos_X = x;
    }
    data->update = TRUE;
  }

  {
      LONG tvisual_y;

    ScrollIntoDisplay(data);

    tvisual_y = LineToVisual(actline, data)-1;
    if(tvisual_y < 0)
      tvisual_y = 0;

    if(!(data->flags & FLG_Quiet))
    {
      SetCursor(data->CPos_X, line, FALSE, data);
      if(data->blockinfo.enabled)
      {
        data->blockinfo.enabled = FALSE;
        MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
      }
      DumpText(data->visual_y+tvisual_y, tvisual_y, data->maxlines, TRUE, data);
    }
    else
    {
      data->blockinfo.enabled = FALSE;
    }

    if(!movecursor)
    {
      data->CPos_X = realx;
      data->actualline = line;
    }
  }

  RETURN(TRUE);
  return(TRUE);
}
