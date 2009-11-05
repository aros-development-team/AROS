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

#include <string.h>

#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/intuition.h>

#include "private.h"
#include "Debug.h"

/// FreeUndoStep()
// free the memory occupated an undo step
static void FreeUndoStep(struct InstData *data, struct UserAction *step)
{
  ENTER();

  if(step->type == ET_DELETEBLOCK || step->type == ET_PASTEBLOCK)
  {
    if(step->clip != NULL)
    {
      FreeVecPooled(data->mypool, step->clip);
      step->clip = NULL;
    }
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
  if(data->nextUndoStep > 0)
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
        #warning is buffer->del.highlight missing here?
        RemoveChars(data, data->CPos_X, data->actualline, 1);
      }
      break;

      case ET_BACKSPACECHAR:
      {
        D(DBF_UNDO, "undo BACKSPACECHAR");
        PasteChars(data, data->CPos_X, data->actualline, 1, (char *)&action->del.character, action);
        data->CPos_X++;
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
        CutBlock2(data, FALSE, FALSE, TRUE, &block);
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
  if(data->nextUndoStep < data->usedUndoSteps)
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

      case ET_BACKSPACECHAR:
      case ET_DELETECHAR:
      {
        D(DBF_UNDO, "redo BACKSPACECHAR/DELETECHAR");
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
        CutBlock2(data, FALSE, FALSE, TRUE, &block);
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

  if(data->maxUndoSteps > 0)
  {
    struct UserAction *action;

    // check if there is still enough space in our undo buffer
    // and if not we clean it up one entry
    if(data->nextUndoStep == data->maxUndoSteps)
    {
      ULONG i;

      // free the oldest stored action and forget about it
      FreeUndoStep(data, &data->undoSteps[0]);
      data->nextUndoStep--;
      data->usedUndoSteps--;

      // shift all remaining actions one step to the front
      for(i = 1; i < data->maxUndoSteps; i++)
        memcpy(&data->undoSteps[i-1], &data->undoSteps[i], sizeof(data->undoSteps[i]));

      // signal the user that something in the undo buffer was lost
      setFlag(data->flags, FLG_UndoLost);
      D(DBF_UNDO, "one undo step was lost");
    }

    action = &data->undoSteps[data->nextUndoStep];

    // clear any previous data
    memset(action, 0, sizeof(*action));

    // advance the index for the next undo step
    data->nextUndoStep++;
    // and count this new step
    data->usedUndoSteps++;

    // as we are about to set something new for an undo
    // operation we have to signal that redo operation
    // is cleared now.
    if(data->nextUndoStep == data->usedUndoSteps)
      set(data->object, MUIA_TextEditor_RedoAvailable, FALSE);

    // and we definitely have something to undo now
    set(data->object, MUIA_TextEditor_UndoAvailable, TRUE);

    action->x = data->CPos_X;
    action->y = LineNr(data, data->actualline);
    action->type = eventtype;

    switch(eventtype)
    {
      case ET_BACKSPACEMERGE:
      case ET_MERGELINES:
      {
        D(DBF_UNDO, "add undo MERGELINES/BACKSPACEMERGE");
        action->del.highlight = data->actualline->next->line.Highlight;
        action->del.flow = data->actualline->next->line.Flow;
        action->del.separator = data->actualline->next->line.Separator;
      }
      break;

      case ET_DELETECHAR:
      case ET_BACKSPACECHAR:
      {
        STRPTR str = (STRPTR)eventData;

        D(DBF_UNDO, "add undo DELETECHAR/BACKSPACECHAR");
        action->del.character = str[0];
        action->del.style = GetStyle(data->CPos_X, data->actualline);
        action->del.flow = data->actualline->line.Flow;
        action->del.separator = data->actualline->line.Separator;
        #warning is buffer->del.highlight missing here?
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
      {
        STRPTR text;
        struct marking *block = (struct marking *)eventData;

        D(DBF_UNDO, "add undo DELETEBLOCK");
        if((text = GetBlock(data, block)) != NULL)
        {
          action->x = block->startx;
          action->y = LineNr(data, block->startline);
          action->clip = text;

          if(isFlagSet(data->flags, FLG_FreezeCrsr))
            action->type = ET_DELETEBLOCK_NOMOVE;
        }
        else
        {
          ResetUndoBuffer(data);
          DoMethod(data->object, MUIM_TextEditor_HandleError, Error_NotEnoughUndoMem);
        }
      }
      break;

      default:
      {
        D(DBF_UNDO, "add undo PASTECHARS");
        // nothing to do
      }
      break;
    }
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

  if(data->maxUndoSteps != 0)
  {
    ULONG i;

    for(i = 0; i < data->usedUndoSteps; i++)
      FreeUndoStep(data, &data->undoSteps[i]);

    data->usedUndoSteps = 0;
    data->nextUndoStep = 0;
  }

  LEAVE();
}

///
/// ResizeUndoBuffer()
void ResizeUndoBuffer(struct InstData *data, ULONG maxSteps)
{
  ENTER();

  if(data->maxUndoSteps != maxSteps)
  {
    D(DBF_UNDO, "resizing undo buffer for %ld undo steps", maxSteps);

    if(data->undoSteps != NULL)
    {
      ResetUndoBuffer(data);
      FreeVecPooled(data->mypool, data->undoSteps);
    }

    // reset everything to zero
    data->undoSteps = NULL;
    data->maxUndoSteps = 0;
    data->usedUndoSteps = 0;
    data->nextUndoStep = 0;

    if(maxSteps > 0)
    {
      ULONG newSize;

      // calculate number of bytes from number of undo levels
      newSize = (maxSteps * sizeof(struct UserAction));

      // allocate a new undo buffer
      if((data->undoSteps = AllocVecPooled(data->mypool, newSize)) != NULL)
        data->maxUndoSteps = maxSteps;
    }
  }

  LEAVE();
}

///
