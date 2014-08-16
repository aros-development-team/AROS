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

#include <string.h>

#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/intuition.h>

#include "private.h"
#include "Debug.h"

/// FreeUndoStep()
// free the memory occupated by an undo step
static void FreeUndoStep(struct InstData *data, ULONG index)
{
  struct UserAction *step = &data->undoSteps[index];

  ENTER();

  D(DBF_UNDO, "free undo step %ld", index);

  if(step->type == ET_DELETEBLOCK || step->type == ET_DELETEBLOCK_NOMOVE || step->type == ET_PASTEBLOCK)
  {
    if(step->clip != NULL)
    {
      FreeVecPooled(data->mypool, step->clip);
      // clear the pointer
      step->clip = NULL;
    }
    // forget about the type of undo
    step->type = ET_NONE;
  }

  LEAVE();
}

///
/// Undo()
BOOL Undo(struct InstData *data)
{
  BOOL success = FALSE;

  ENTER();

  D(DBF_UNDO, "before maxUndoSteps=%ld nextUndoStep=%ld usedUndoSteps=%ld", data->maxUndoSteps, data->nextUndoStep, data->usedUndoSteps);

  // check if there is something in the undo buffer available
  if(isFlagClear(data->flags, FLG_ReadOnly) && data->nextUndoStep > 0)
  {
    struct UserAction *action;
    BOOL moveCursor = TRUE;

    if(Enabled(data))
    {
      data->blockinfo.enabled = FALSE;
      MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);
    }

    // as the redo operation automatically
    // becomes available when undo is used we just
    // check here if we didn't yet set RedoAvailable
    // as we only want to set it once
    if(data->nextUndoStep == data->usedUndoSteps)
      set(data->object, MUIA_TextEditor_RedoAvailable, TRUE);

    // go one step back
    data->nextUndoStep--;
    action = &data->undoSteps[data->nextUndoStep];

//    if(data->actualline != LineNode(data, buffer->y) || data->CPos_X != buffer->x)
    SetCursor(data, data->CPos_X, data->actualline, FALSE);

    data->CPos_X = action->x;
    data->actualline = LineNode(data, action->y);
    ScrollIntoDisplay(data);

    // perform the saved undo action
    switch(action->type)
    {
      case ET_PASTECHAR:
      {
        D(DBF_UNDO, "undo PASTECHAR");
        action->del.character = data->actualline->line.Contents[data->CPos_X];
        action->del.style = GetStyle(data->CPos_X, data->actualline);
        action->del.flow = data->actualline->line.Flow;
        action->del.separator = data->actualline->line.Separator;
        action->del.highlight = data->actualline->line.Highlight;
        RemoveChars(data, data->CPos_X, data->actualline, 1);
      }
      break;

      case ET_DELETECHAR:
      {
        D(DBF_UNDO, "undo DELETECHAR");
        PasteChars(data, data->CPos_X, data->actualline, 1, (char *)&action->del.character, action);
      }
      break;

      case ET_SPLITLINE:
      {
        D(DBF_UNDO, "undo SPLITLINE");
        MergeLines(data, data->actualline);
      }
      break;

      case ET_MERGELINES:
      {
        D(DBF_UNDO, "undo MERGELINES");
        SplitLine(data, data->CPos_X, data->actualline, FALSE, action);
      }
      break;

      case ET_BACKSPACEMERGE:
      {
        D(DBF_UNDO, "undo BACKSPACEMARGE");
        SplitLine(data, data->CPos_X, data->actualline, TRUE, action);
      }
      break;

      case ET_PASTEBLOCK:
      {
        struct marking block =
        {
          TRUE,
          LineNode(data, action->y),
          action->x,
          LineNode(data, action->blk.y),
          action->blk.x
        };
        STRPTR clip = GetBlock(data, &block);

        D(DBF_UNDO, "undo PASTEBLOCK");
        CutBlock2(data, CUTF_CUT|CUTF_UPDATE, &block);
        action->clip = clip;
      }
      break;

      case ET_DELETEBLOCK_NOMOVE:
      {
        D(DBF_UNDO, "undo DELETEBLOCK_NOMOVE");
        moveCursor = FALSE;
      }
      // fall through...

      case ET_DELETEBLOCK:
      {
        struct Hook *oldhook;

        D(DBF_UNDO, "undo DELETEBLOCK");
        oldhook = data->ImportHook;
        data->ImportHook = &ImPlainHook;
        InsertText(data, action->clip, moveCursor);
        data->ImportHook = oldhook;
        FreeVecPooled(data->mypool, action->clip);
        // clear the pointer
        action->clip = NULL;

        action->blk.x = data->CPos_X;
        action->blk.y = LineNr(data, data->actualline);

        if(moveCursor == FALSE)
        {
          data->CPos_X = action->x;
          data->actualline = LineNode(data, action->y);
        }
      }
      break;

      default:
        // nothing to do
      break;
    }

    ScrollIntoDisplay(data);

    if(isFlagSet(data->flags, FLG_Active))
      SetCursor(data, data->CPos_X, data->actualline, TRUE);

    // if there are no further undo levels we
    // have to set UndoAvailable to FALSE
    if(data->nextUndoStep == 0)
    {
      set(data->object, MUIA_TextEditor_UndoAvailable, FALSE);

      if(isFlagClear(data->flags, FLG_UndoLost))
        data->HasChanged = FALSE;
    }

    success = TRUE;
  }
  else
  {
    DoMethod(data->object, MUIM_TextEditor_HandleError, Error_NothingToUndo);
  }

  D(DBF_UNDO, "after  maxUndoSteps=%ld nextUndoStep=%ld usedUndoSteps=%ld", data->maxUndoSteps, data->nextUndoStep, data->usedUndoSteps);


  RETURN(success);
  return success;
}

///
/// Redo()
BOOL Redo(struct InstData *data)
{
  BOOL success = FALSE;

  ENTER();

  D(DBF_UNDO, "before maxUndoSteps=%ld nextUndoStep=%ld usedUndoSteps=%ld", data->maxUndoSteps, data->nextUndoStep, data->usedUndoSteps);

  // check if there something to redo at all
  if(isFlagClear(data->flags, FLG_ReadOnly) && data->nextUndoStep < data->usedUndoSteps)
  {
    struct UserAction *action;

    if(Enabled(data))
    {
      data->blockinfo.enabled = FALSE;
      MarkText(data, data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline);
    }

    // in case nextUndoStep is equal zero then we have to
    // set the undoavailable attribute to true to signal
    // others that undo is available
    if(data->nextUndoStep == 0)
      set(data->object, MUIA_TextEditor_UndoAvailable, TRUE);

    action = &data->undoSteps[data->nextUndoStep];
    data->nextUndoStep++;

//    if(data->actualline != LineNode(data, buffer->y) || data->CPos_X != buffer->x)
    SetCursor(data, data->CPos_X, data->actualline, FALSE);
    data->CPos_X = action->x;
    data->actualline = LineNode(data, action->y);
    ScrollIntoDisplay(data);

    switch(action->type)
    {
      case ET_PASTECHAR:
      {
        D(DBF_UNDO, "redo PASTECHAR");
        PasteChars(data, data->CPos_X, data->actualline, 1, (char *)&action->del.character, action);
        data->CPos_X++;
      }
      break;

      case ET_DELETECHAR:
      {
        D(DBF_UNDO, "redo DELETECHAR");
        RemoveChars(data, data->CPos_X, data->actualline, 1);
      }
      break;

      case ET_SPLITLINE:
      {
        D(DBF_UNDO, "redo SPLITLINE");
        SplitLine(data, data->CPos_X, data->actualline, TRUE, NULL);
      }
      break;

      case ET_MERGELINES:
      case ET_BACKSPACEMERGE:
      {
        D(DBF_UNDO, "redo MERGELINES/BACKSPACEMERGE");
        MergeLines(data, data->actualline);
      }
      break;

      case ET_PASTEBLOCK:
      {
        struct Hook *oldhook = data->ImportHook;

        D(DBF_UNDO, "redo PASTEBLOCK");
        data->ImportHook = &ImPlainHook;
        InsertText(data, action->clip, TRUE);
        data->ImportHook = oldhook;
        FreeVecPooled(data->mypool, action->clip);
        // clear the pointer
        action->clip = NULL;

        action->blk.x = data->CPos_X;
        action->blk.y = LineNr(data, data->actualline);
      }
      break;

      case ET_DELETEBLOCK_NOMOVE:
      case ET_DELETEBLOCK:
      {
        struct marking block =
        {
          TRUE,
          LineNode(data, action->y),
          action->x,
          LineNode(data, action->blk.y),
          action->blk.x
        };
        STRPTR clip = GetBlock(data, &block);

        D(DBF_UNDO, "redo DELETEBLOCK/DELETEBLOCK_NOMOVE");
        CutBlock2(data, CUTF_CUT|CUTF_UPDATE, &block);
        action->clip = clip;
      }
      break;

      default:
        // nothing to do
      break;
    }

    ScrollIntoDisplay(data);

    if(isFlagSet(data->flags, FLG_Active))
      SetCursor(data, data->CPos_X, data->actualline, TRUE);

    // if nextUndoStep == usedUndoSteps this signals that we
    // don't have any things to redo anymore.
    if(data->nextUndoStep == data->usedUndoSteps)
      set(data->object, MUIA_TextEditor_RedoAvailable, FALSE);

    success = TRUE;
  }
  else
  {
    DoMethod(data->object, MUIM_TextEditor_HandleError, Error_NothingToRedo);
  }

  D(DBF_UNDO, "after  maxUndoSteps=%ld nextUndoStep=%ld usedUndoSteps=%ld", data->maxUndoSteps, data->nextUndoStep, data->usedUndoSteps);

  RETURN(success);
  return success;
}

///
/// AddToUndoBuffer()
BOOL AddToUndoBuffer(struct InstData *data, enum EventType eventtype, void *eventData)
{
  ENTER();

  D(DBF_UNDO, "before maxUndoSteps=%ld nextUndoStep=%ld usedUndoSteps=%ld", data->maxUndoSteps, data->nextUndoStep, data->usedUndoSteps);

  if(isFlagClear(data->flags, FLG_ReadOnly) && data->maxUndoSteps > 0)
  {
    struct UserAction *action;
    BOOL success = TRUE;

    // check if there is still enough space in our undo buffer
    // and if not we clean it up one entry
    if(data->nextUndoStep == data->maxUndoSteps)
    {
      // free the oldest stored action and forget about it
      D(DBF_UNDO, "undo buffer is full, loose the oldest step");
      FreeUndoStep(data, 0);
      data->nextUndoStep--;
      data->usedUndoSteps--;

      // shift all remaining actions one step to the front
      memmove(&data->undoSteps[0], &data->undoSteps[1], sizeof(data->undoSteps[0]) * data->maxUndoSteps);

      // signal the user that something in the undo buffer was lost
      setFlag(data->flags, FLG_UndoLost);
    }
    else
    {
      ULONG i;

      // adding something new to the undo buffer will erase all previously
      // performed redo's
      for(i = data->nextUndoStep; i < data->usedUndoSteps; i++)
      {
        D(DBF_UNDO, "free not yet redone step %ld", i);
        FreeUndoStep(data, i);
      }
      data->usedUndoSteps = data->nextUndoStep;
    }

    action = &data->undoSteps[data->nextUndoStep];

    // clear any previous data
    memset(action, 0, sizeof(*action));

    action->x = data->CPos_X;
    action->y = LineNr(data, data->actualline);
    action->type = eventtype;

    switch(eventtype)
    {
      case ET_BACKSPACEMERGE:
      case ET_MERGELINES:
      {
        struct line_node *next = GetNextLine(data->actualline);

        D(DBF_UNDO, "add undo MERGELINES/BACKSPACEMERGE");
        action->del.highlight = next->line.Highlight;
        action->del.flow = next->line.Flow;
        action->del.separator = next->line.Separator;
      }
      break;

      case ET_DELETECHAR:
      {
        STRPTR str = (STRPTR)eventData;

        D(DBF_UNDO, "add undo DELETECHAR");
        action->del.character = str[0];
        action->del.style = GetStyle(data->CPos_X, data->actualline);
        action->del.flow = data->actualline->line.Flow;
        action->del.separator = data->actualline->line.Separator;
        action->del.highlight = data->actualline->line.Highlight;
      }
      break;

      case ET_PASTEBLOCK:
      {
        struct marking *block = (struct marking *)eventData;

        D(DBF_UNDO, "add undo PASTEBLOCK");
        action->x = block->startx;
        action->y = LineNr(data, block->startline);
        action->blk.x = block->stopx;
        action->blk.y = LineNr(data, block->stopline);
      }
      break;

      case ET_DELETEBLOCK:
      case ET_DELETEBLOCK_NOMOVE:
      {
        STRPTR text;
        struct marking *block = (struct marking *)eventData;

        D(DBF_UNDO, "add undo DELETEBLOCK");
        if((text = GetBlock(data, block)) != NULL)
        {
          action->x = block->startx;
          action->y = LineNr(data, block->startline);
          action->clip = text;

          if(eventtype == ET_DELETEBLOCK && isFlagSet(data->flags, FLG_FreezeCrsr))
            action->type = ET_DELETEBLOCK_NOMOVE;
        }
        else
        {
          success = FALSE;
        }
      }
      break;

      default:
      {
        // nothing to do
      }
      break;
    }

    if(success == TRUE)
    {
      // adding the undo step was successful, update the variables
      // advance the index for the next undo step
      data->nextUndoStep++;
      // and count this new step
      data->usedUndoSteps++;
    }
    else
    {
      // something went wrong, invoke the error method
      DoMethod(data->object, MUIM_TextEditor_HandleError, Error_NotEnoughUndoMem);
    }

    // trigger possible notifications, no matter if the action succeeded or not,
    // because the undo/redo situation might have changed
    SetAttrs(data->object, MUIA_TextEditor_RedoAvailable, data->nextUndoStep < data->usedUndoSteps,
                           MUIA_TextEditor_UndoAvailable, data->usedUndoSteps != 0,
                           TAG_DONE);
  }

  D(DBF_UNDO, "after  maxUndoSteps=%ld nextUndoStep=%ld usedUndoSteps=%ld", data->maxUndoSteps, data->nextUndoStep, data->usedUndoSteps);

  RETURN(TRUE);
  return(TRUE);
}

///
/// ResetUndoBuffer()
void ResetUndoBuffer(struct InstData *data)
{
  ENTER();

  if(data->maxUndoSteps != 0 && data->undoSteps != NULL)
  {
    ULONG i;

    for(i = 0; i < data->usedUndoSteps; i++)
      FreeUndoStep(data, i);

    data->usedUndoSteps = 0;
    data->nextUndoStep = 0;

    // trigger possible notifications
    SetAttrs(data->object, MUIA_TextEditor_RedoAvailable, FALSE,
                           MUIA_TextEditor_UndoAvailable, FALSE,
                           TAG_DONE);
  }

  LEAVE();
}

///
/// ResizeUndoBuffer()
void ResizeUndoBuffer(struct InstData *data, ULONG newMaxUndoSteps)
{
  ENTER();

  if(data->maxUndoSteps != newMaxUndoSteps)
  {
    struct UserAction *newUndoSteps = NULL;

    D(DBF_UNDO, "resizing undo buffer from %ld to %ld steps", data->maxUndoSteps, newMaxUndoSteps);

    if(newMaxUndoSteps != 0)
    {
      ULONG oldSize;
      ULONG newSize;

      // calculate number of bytes from number of undo levels
      oldSize = (data->maxUndoSteps * sizeof(struct UserAction));
      newSize = (newMaxUndoSteps * sizeof(struct UserAction));

      // allocate a new undo buffer
      if((newUndoSteps = AllocVecPooled(data->mypool, newSize)) != NULL)
      {
        if(data->undoSteps != NULL)
        {
          // copy over the old undo steps
          memcpy(newUndoSteps, data->undoSteps, MIN(oldSize, newSize));
        }
      }
    }

    if(data->undoSteps != NULL)
    {
      ULONG i;

      // free the steps which don't fit into the new buffer anymore
      for(i = newMaxUndoSteps; i < data->maxUndoSteps; i++)
        FreeUndoStep(data, i);

      // free the old buffer
      FreeVecPooled(data->mypool, data->undoSteps);
    }

    // reset everything to the new values
    data->undoSteps = newUndoSteps;
    data->maxUndoSteps = newMaxUndoSteps;
    data->usedUndoSteps = MIN(data->usedUndoSteps, newMaxUndoSteps);
    data->nextUndoStep = MIN(data->nextUndoStep, newMaxUndoSteps);

    // trigger possible notifications
    SetAttrs(data->object, MUIA_TextEditor_RedoAvailable, data->nextUndoStep < data->usedUndoSteps,
                           MUIA_TextEditor_UndoAvailable, data->usedUndoSteps != 0,
                           TAG_DONE);
  }

  LEAVE();
}

///
/// FreeUndoBuffer()
void FreeUndoBuffer(struct InstData *data)
{
  ENTER();

  if(data->undoSteps != NULL)
  {
    ULONG i;

    for(i = 0; i < data->usedUndoSteps; i++)
      FreeUndoStep(data, i);

    FreeVecPooled(data->mypool, data->undoSteps);
    data->undoSteps = NULL;
  }

  LEAVE();
}

///
