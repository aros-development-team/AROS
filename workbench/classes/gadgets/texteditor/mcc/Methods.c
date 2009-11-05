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

#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "private.h"
#include "Debug.h"

#include "TextEditor_mcp.h"

/// mBlockInfo()
IPTR mBlockInfo(struct InstData *data, struct MUIP_TextEditor_BlockInfo *msg)
{
  IPTR result = FALSE;

  ENTER();

  if(Enabled(data))
  {
    struct marking newblock;

    NiceBlock(&data->blockinfo, &newblock);
    *(msg->startx) = newblock.startx;
    *(msg->stopx)  = newblock.stopx;
    *(msg->starty) = LineNr(data, newblock.startline)-1;
    *(msg->stopy)  = LineNr(data, newblock.stopline)-1;

    result = TRUE;
  }

  RETURN(result);
  return(result);
}

///
/// mQueryKeyAction()
IPTR mQueryKeyAction(UNUSED struct IClass *cl, Object *obj, struct MUIP_TextEditor_QueryKeyAction *msg)
{
  struct te_key *userkeys;
  struct te_key *foundKey = NULL;
  ULONG setting = 0;
  int i;

  ENTER();

  // now we try to retrieve the currently active
  // key bindings, or we take the default ones
  if(!DoMethod(obj, MUIM_GetConfigItem, MUICFG_TextEditor_Keybindings, &setting) || setting == 0)
    userkeys = (struct te_key *)default_keybindings;
  else
    userkeys = (struct te_key *)setting;

  for(i=0; (WORD)userkeys[i].code != -1; i++)
  {
    struct te_key *curKey = &userkeys[i];

    if(curKey->act == msg->keyAction)
    {
      foundKey = curKey;
      break;
    }
  }

  RETURN((IPTR)foundKey);
  return (IPTR)foundKey;
}

///
/// mMarkText()
IPTR mMarkText(struct InstData *data, struct MUIP_TextEditor_MarkText *msg)
{
  ENTER();

  if(Enabled(data))
  {
    data->blockinfo.enabled = FALSE;
    MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);
  }
  else
  {
    SetCursor(data, data->CPos_X, data->actualline, FALSE);
  }

  // check if anything at all should be marked or not
  if((LONG)msg->start_crsr_y != MUIV_TextEditor_MarkText_None)
  {
    // check if only the specified area should be marked/selected
    if((LONG)msg->stop_crsr_y != MUIV_TextEditor_MarkText_All)
    {
      data->blockinfo.startline = LineNode(data, msg->start_crsr_y+1);
      data->blockinfo.startx = (data->blockinfo.startline->line.Length > msg->start_crsr_x) ? msg->start_crsr_x : data->blockinfo.startline->line.Length-1;
      data->blockinfo.stopline = LineNode(data, msg->stop_crsr_y+1);
      data->blockinfo.stopx = (data->blockinfo.stopline->line.Length > msg->stop_crsr_x) ? msg->stop_crsr_x : data->blockinfo.stopline->line.Length-1;
      data->blockinfo.enabled = TRUE;

      data->actualline = data->blockinfo.stopline;
      data->CPos_X = data->blockinfo.stopx;
      ScrollIntoDisplay(data);
    }
    else
    {
      // the user selected to mark all available text
      struct line_node *actual = data->firstline;

      data->blockinfo.startline = actual;
      data->blockinfo.startx = 0;

      while(actual->next)
        actual = actual->next;

      data->blockinfo.stopline = actual;
      data->blockinfo.stopx = data->blockinfo.stopline->line.Length-1;
      data->blockinfo.enabled = TRUE;
    }

    MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);
  }

  RETURN(TRUE);
  return(TRUE);
}

///
/// mClearText()
IPTR mClearText(struct IClass *cl, Object *obj, UNUSED Msg msg)
{
  struct InstData *data = INST_DATA(cl, obj);
  struct line_node *newcontents;

  ENTER();

  if((newcontents = AllocLine(data)) != NULL)
  {
    if(Init_LineNode(data, newcontents, NULL, "\n") == TRUE)
    {
      if(data->maxUndoSteps != 0)
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
        AddToUndoBuffer(data, ET_DELETEBLOCK, &newblock);
      }
      FreeTextMem(data, data->firstline);
      data->firstline = newcontents;
      ResetDisplay(data);
    }
    else
    {
      FreeLine(data, newcontents);
    }
  }

  RETURN(TRUE);
  return(TRUE);
}

///
/// mToggleCursor()
IPTR mToggleCursor(struct IClass *cl, Object *obj, UNUSED Msg msg)
{
  struct InstData *data = INST_DATA(cl, obj);

  ENTER();

  if(isFlagSet(data->flags, FLG_Active))
  {
    if(data->cursor_shown)
    {
      SetCursor(data, data->CPos_X, data->actualline, FALSE);
      data->cursor_shown = FALSE;
    }
    else
    {
      SetCursor(data, data->CPos_X, data->actualline, TRUE);
      data->cursor_shown = TRUE;
    }
  }

  RETURN(TRUE);
  return(TRUE);
}

///
/// mInputTrigger()
IPTR mInputTrigger(struct IClass *cl, Object *obj, UNUSED Msg msg)
{
  struct InstData *data = INST_DATA(cl, obj);

  ENTER();

  if(data->smooth_wait == 1 && data->scrollaction == TRUE)
  {
    if(data->ypos != data->realypos)
    {
      LONG  move;

      if(data->scr_direction)
        move = data->height-(data->realypos-data->ypos);
      else
        move = -(data->realypos-data->ypos);

      if(move != 1 && move != -1)
        move  = (move*2)/3;

      set(data->object, MUIA_TextEditor_Prop_First, (data->visual_y-1)*data->height+(data->realypos-data->ypos)+move);
    }
    else
    {
      data->scrollaction = FALSE;
      RejectInput(data);

      if(isFlagSet(data->flags, FLG_Active))
      {
        SetCursor(data, data->CPos_X, data->actualline, TRUE);
      }
    }
  }

  if(data->mousemove == TRUE)
  {
    LONG MouseX = muiRenderInfo(data->object)->mri_Window->MouseX;
    LONG MouseY = muiRenderInfo(data->object)->mri_Window->MouseY;
    LONG oldCPos_X = data->CPos_X;
    struct line_node *oldactualline = data->actualline;
    BOOL scroll = TRUE;

    if(xget(_win(data->object), MUIA_Window_Activate) == FALSE)
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
      else
        limit += (data->totallines-data->visual_y+1)*data->height;

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
        PosFromCursor(data, MouseX, MouseY);
        scroll = FALSE;
      }
    }

    if(data->blockinfo.enabled == TRUE && scroll == TRUE && data->blockinfo.stopline == data->actualline && data->blockinfo.stopx == data->CPos_X)
    {
      PosFromCursor(data, MouseX, MouseY);
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
            while(data->CPos_X < data->actualline->line.Length-1 && CheckSep(data, data->actualline->line.Contents[data->CPos_X]) == FALSE)
//            while((data->CPos_X < data->actualline->line.Length-1) && (data->actualline->line.Contents[data->CPos_X] != ' '))
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

        OffsetToLines(data, data->CPos_X, data->actualline, &pos);
        flow = FlowSpace(data, data->actualline->line.Flow, &data->actualline->line.Contents[pos.bytes]);

        if(MouseX <= (LONG)(data->xpos+flow+TextLength(&data->tmprp, &data->actualline->line.Contents[pos.bytes], pos.extra-pos.bytes-1)))
        {
          if(data->selectmode == 1)
          {
            while(data->CPos_X > 0 && CheckSep(data, data->actualline->line.Contents[data->CPos_X-1]) == FALSE)
//            while(data->CPos_X > 0 && data->actualline->line.Contents[data->CPos_X-1] != ' ')
              data->CPos_X--;
          }
          else
          {
            GoStartOfLine(data);
          }
        }
      }
    }

    if(data->blockinfo.enabled == TRUE || data->selectmode == 0)
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
          MarkText(data, data->blockinfo.stopx, data->blockinfo.stopline, data->CPos_X, data->actualline);
        }
        else
          MarkText(data, data->blockinfo.stopx, data->blockinfo.stopline, data->CPos_X+1, data->actualline);

        data->selectmode = 3;
      }
      else if(data->blockinfo.stopline != data->actualline || data->blockinfo.stopx != data->CPos_X)
      {
        data->blockinfo.enabled = TRUE;
        MarkText(data, data->blockinfo.stopx, data->blockinfo.stopline, data->CPos_X, data->actualline);
      }

      data->blockinfo.stopline = data->actualline;
      data->blockinfo.stopx = data->CPos_X;
    }
    else
    {
      data->mousemove = FALSE;
      RejectInput(data);
    }

    if(oldCPos_X != data->CPos_X || oldactualline != data->actualline)
    {
      ScrollIntoDisplay(data);
      PosFromCursor(data, MouseX, MouseY);

      // make sure to notify others that the cursor has changed and so on.
      data->NoNotify = TRUE;

      if(data->CPos_X != oldCPos_X)
        set(obj, MUIA_TextEditor_CursorX, data->CPos_X);

      if(data->actualline != oldactualline)
        set(obj, MUIA_TextEditor_CursorY, LineNr(data, data->actualline)-1);

      data->NoNotify = FALSE;
    }
  }

  RETURN(TRUE);
  return(TRUE);
}

///
/// InsertText()
ULONG InsertText(struct InstData *data, STRPTR text, BOOL moveCursor)
{
  struct line_node *line, *actline = data->actualline;
  UWORD x = data->CPos_X;
  UWORD realx = 0;

  ENTER();

  if((line = ImportText(data, text, data->ImportHook, data->ImportWrap)) != NULL)
  {
    BOOL oneline = FALSE;
    BOOL newline = FALSE;
    struct line_node *startline = line;

    line->visual = VisualHeight(data, line);
    data->totallines += line->visual;

    while(line->next != NULL)
    {
      line = line->next;
      line->visual = VisualHeight(data, line);
      data->totallines += line->visual;
    }

    if(line->line.Contents[line->line.Length] == '\n')
      newline = TRUE;

    data->update = FALSE;
    SplitLine(data, x, actline, FALSE, NULL);
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
    if(newline == FALSE)
      MergeLines(data, line);
    MergeLines(data, actline);
    if(oneline == TRUE)
      line = actline;
    if(newline == TRUE)
    {
      line = line->next;
      data->CPos_X = 0;
    }
    if(moveCursor == TRUE)
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

    tvisual_y = LineToVisual(data, actline)-1;
    if(tvisual_y < 0)
      tvisual_y = 0;

    if(isFlagClear(data->flags, FLG_Quiet))
    {
      SetCursor(data, data->CPos_X, line, FALSE);
      if(data->blockinfo.enabled == TRUE)
      {
        data->blockinfo.enabled = FALSE;
        MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);
      }
      DumpText(data, data->visual_y+tvisual_y, tvisual_y, data->maxlines, TRUE);
    }
    else
    {
      data->blockinfo.enabled = FALSE;
    }

    if(moveCursor == FALSE)
    {
      data->CPos_X = realx;
      data->actualline = line;
    }
  }

  RETURN(TRUE);
  return(TRUE);
}

///
