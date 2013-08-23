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
  IPTR setting = 0;
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
      MarkAllBlock(data, &data->blockinfo);
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

  if((newcontents = AllocVecPooled(data->mypool, sizeof(struct line_node))) != NULL)
  {
    if(Init_LineNode(data, newcontents, "\n") == TRUE)
    {
      if(isFlagClear(data->flags, FLG_ReadOnly) && data->maxUndoSteps != 0)
      {
        struct marking newblock;

        MarkAllBlock(data, &newblock);

        data->CPos_X = 0;
        data->actualline = GetFirstLine(&data->linelist);

        // add the text to the undo buffer only if we really have some text
        // i.e. the block contains at least some characters or some lines
        if(newblock.startline != newblock.stopline || newblock.startx < newblock.stopx)
          AddToUndoBuffer(data, ET_DELETEBLOCK_NOMOVE, &newblock);
      }
      FreeTextMem(data, &data->linelist);
      AddLine(&data->linelist, newcontents);
      ResetDisplay(data);
    }
    else
    {
      FreeVecPooled(data->mypool, newcontents);
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
    if(data->cursor_shown == TRUE)
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
    if(data->ypos != _mtop(obj))
    {
      LONG  move;

      if(data->scr_direction)
        move = data->fontheight-(_mtop(obj)-data->ypos);
      else
        move = -(_mtop(obj)-data->ypos);

      if(move != 1 && move != -1)
        move  = (move*2)/3;

      set(obj, MUIA_TextEditor_Prop_First, (data->visual_y-1)*data->fontheight+(_mtop(obj)-data->ypos)+move);
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
    LONG MouseX = _window(obj)->MouseX;
    LONG MouseY = _window(obj)->MouseY;
    LONG oldCPos_X = data->CPos_X;
    struct line_node *oldactualline = data->actualline;
    BOOL scroll = TRUE;

    if(xget(_win(obj), MUIA_Window_Activate) == FALSE)
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
        limit += (data->maxlines * data->fontheight);
      else
        limit += (data->totallines-data->visual_y+1)*data->fontheight;

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
        if(MouseX > _mleft(obj))
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
        LONG flow;

        OffsetToLines(data, data->CPos_X, data->actualline, &pos);
        flow = FlowSpace(data, data->actualline->line.Flow, &data->actualline->line.Contents[pos.bytes]);

        if(MouseX <= _mleft(obj)+flow+TextLengthNew(&data->tmprp, &data->actualline->line.Contents[pos.bytes], pos.extra-pos.bytes-1, data->TabSizePixels))
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
  struct MinList newlines;
  struct line_node *actline = data->actualline;
  LONG x = data->CPos_X;
  LONG realx = 0;

  ENTER();

  if(ImportText(data, text, data->ImportHook, data->ImportWrap, &newlines) == TRUE)
  {
    BOOL oneline = FALSE;
    BOOL newline = FALSE;
    struct line_node *line;
    LONG tvisual_y;

    data->totallines += CountLines(data, &newlines);

    line = GetLastLine(&newlines);
    if(line->line.Contents[line->line.Length] == '\n')
      newline = TRUE;

    data->update = FALSE;
    SplitLine(data, x, actline, FALSE, NULL);
    InsertLines(&newlines, actline);
    data->CPos_X = line->line.Length-1;
    if(GetNextLine(actline) == line)
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
      line = GetNextLine(line);
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
