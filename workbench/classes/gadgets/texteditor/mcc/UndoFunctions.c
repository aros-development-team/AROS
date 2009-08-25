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

///LineNr()
unsigned short LineNr(struct line_node *line, struct InstData *data)
{
  unsigned short result = 1;
  struct line_node *actual = data->firstline;

  ENTER();

  while(line != actual)
  {
    result++;
    actual = actual->next;
  }

  RETURN(result);
  return(result);
}
///

///LineNode()
struct line_node *LineNode(unsigned short linenr, struct InstData *data)
{
  struct line_node *actual = data->firstline;

  ENTER();

  while(--linenr && actual->next)
  {
    actual = actual->next;
  }

  RETURN(actual);
  return(actual);
}
///

///Undo()
long Undo(struct InstData *data)
{
  ENTER();

  D(DBF_UNDO, "undolevel: %ld undocur: %ld undofill: %ld", data->undolevel, data->undocur, data->undofill);

  // check if there is something in the undo
  // buffer available
  if(data->undolevel > 0 && data->undocur > 0)
  {
    struct UserAction *buffer;
    BOOL  crsr_move = TRUE;

    if(Enabled(data))
    {
      data->blockinfo.enabled = FALSE;
      MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
    }

    // as the redo operation automatically
    // becomes available when undo is used we just
    // check here if we didn't yet set RedoAvailable
    // as we only want to set it once
    if(data->undocur == data->undofill)
      set(data->object, MUIA_TextEditor_RedoAvailable, TRUE);

    data->undopointer = (APTR)((char *)data->undopointer - sizeof(struct UserAction));
    data->undocur--;
    buffer = (struct UserAction *)data->undopointer;

//    if(data->actualline != LineNode(buffer->y, data) || data->CPos_X != buffer->x)
    SetCursor(data->CPos_X, data->actualline, FALSE, data);

    data->CPos_X = buffer->x;
    data->actualline = LineNode(buffer->y, data);
    ScrollIntoDisplay(data);

    switch(buffer->type)
    {
      case ET_PASTECHAR:
      {
        buffer->del.character = *(data->actualline->line.Contents+data->CPos_X);
        buffer->del.style = GetStyle(data->CPos_X, data->actualline);
        buffer->del.flow = data->actualline->line.Flow;
        buffer->del.separator = data->actualline->line.Separator;
        RemoveChars(data->CPos_X, data->actualline, 1, data);
      }
      break;

      case ET_BACKSPACECHAR:
      {
        PasteChars(data->CPos_X++, data->actualline, 1, (char *)&buffer->del.character, buffer, data);
      }
      break;

      case ET_DELETECHAR:
      {
        PasteChars(data->CPos_X, data->actualline, 1, (char *)&buffer->del.character, buffer, data);
      }
      break;

      case ET_SPLITLINE:
      {
        MergeLines(data->actualline, data);
      }
      break;

      case ET_MERGELINES:
      {
        SplitLine(data->CPos_X, data->actualline, FALSE, buffer, data);
      }
      break;

      case ET_BACKSPACEMERGE:
      {
        SplitLine(data->CPos_X, data->actualline, TRUE, buffer, data);
      }
      break;

      case ET_PASTEBLOCK:
      {
        struct marking block =
        {
          TRUE,
          LineNode(buffer->y, data),
          buffer->x,
          LineNode(buffer->blk.y, data),
          buffer->blk.x
        };
        char *clip = GetBlock(&block, data);

        CutBlock2(data, FALSE, FALSE, &block, TRUE);
        buffer->clip = (unsigned char *)clip;
      }
      break;

      case ET_DELETEBLOCK_NOMOVE:
        crsr_move = FALSE;
      // continue

      case ET_DELETEBLOCK:
      {
        struct Hook *oldhook = data->ImportHook;
        char *clip = (char *)buffer->clip;

        data->ImportHook = &ImPlainHook;
        InsertText(data, clip, crsr_move);
        data->ImportHook = oldhook;
        MyFreePooled(data->mypool, clip);

        buffer->blk.x = data->CPos_X;
        buffer->blk.y = LineNr(data->actualline, data);

        if(!crsr_move)
        {
          data->CPos_X = buffer->x;
          data->actualline = LineNode(buffer->y, data);
        }
      }
      break;

      default:
        // nothing to do
      break;
    }

    ScrollIntoDisplay(data);

    if(data->flags & FLG_Active)
      SetCursor(data->CPos_X, data->actualline, TRUE, data);

    // if there are no further undo levels we
    // have to set UndoAvailable to FALSE
    if(data->undocur == 0)
    {
      set(data->object, MUIA_TextEditor_UndoAvailable, FALSE);

      if(!(data->flags & FLG_UndoLost))
        data->HasChanged = FALSE;
    }

    RETURN(TRUE);
    return(TRUE);
  }
  else
  {
    DoMethod(data->object, MUIM_TextEditor_HandleError, Error_NothingToUndo);

    RETURN(FALSE);
    return(FALSE);
  }
}
///

///Redo()
long Redo(struct InstData *data)
{
  ENTER();

  D(DBF_UNDO, "undolevel: %ld undocur: %ld undofill: %ld", data->undolevel, data->undocur, data->undofill);

  // check if there something to redo at all
  if(data->undofill > 0 && data->undocur < data->undofill)
  {
    struct UserAction *buffer = (struct UserAction *)data->undopointer;

    if(Enabled(data))
    {
      data->blockinfo.enabled = FALSE;
      MarkText(data->blockinfo.startx, data->blockinfo.startline, data->blockinfo.stopx, data->blockinfo.stopline, data);
    }

    // in case undocur is equal zero then we have to
    // set the undoavailable attribute to true to signal
    // others that undo is available
    if(data->undocur == 0)
      set(data->object, MUIA_TextEditor_UndoAvailable, TRUE);

    data->undopointer = (APTR)((char *)data->undopointer + sizeof(struct UserAction));
    data->undocur++;

//    if(data->actualline != LineNode(buffer->y, data) || data->CPos_X != buffer->x)
    SetCursor(data->CPos_X, data->actualline, FALSE, data);
    data->CPos_X = buffer->x;
    data->actualline = LineNode(buffer->y, data);
    ScrollIntoDisplay(data);

    switch(buffer->type)
    {
      case ET_PASTECHAR:
        PasteChars(data->CPos_X++, data->actualline, 1, (char *)&buffer->del.character, buffer, data);
        break;

      case ET_BACKSPACECHAR:
      case ET_DELETECHAR:
        RemoveChars(data->CPos_X, data->actualline, 1, data);
        break;

      case ET_SPLITLINE:
        SplitLine(data->CPos_X, data->actualline, TRUE, NULL, data);
        break;

      case ET_MERGELINES:
      case ET_BACKSPACEMERGE:
        MergeLines(data->actualline, data);
        break;

      case ET_PASTEBLOCK:
      {
          struct Hook *oldhook = data->ImportHook;

          data->ImportHook = &ImPlainHook;
          InsertText(data, (char *)buffer->clip, TRUE);
          data->ImportHook = oldhook;
          MyFreePooled(data->mypool, buffer->clip);

          buffer->blk.x = data->CPos_X;
          buffer->blk.y = LineNr(data->actualline, data);
      }
      break;

      case ET_DELETEBLOCK_NOMOVE:
      case ET_DELETEBLOCK:
      {
            struct marking block =
            {
              TRUE,
              LineNode(buffer->y, data),
              buffer->x,
              LineNode(buffer->blk.y, data),
              buffer->blk.x
            };
            char  *clip = GetBlock(&block, data);

          CutBlock2(data, FALSE, FALSE, &block, TRUE);
          buffer->clip = (unsigned char *)clip;
      }
      break;

      default:
        // nothing to do
      break;
    }

    ScrollIntoDisplay(data);

    if(data->flags & FLG_Active)
      SetCursor(data->CPos_X, data->actualline, TRUE, data);

    // if undocur == undofill this signals that we
    // don't have any things to redo anymore.
    if(data->undocur == data->undofill)
      set(data->object, MUIA_TextEditor_RedoAvailable, FALSE);

    RETURN(TRUE);
    return(TRUE);
  }
  else
  {
    DoMethod(data->object, MUIM_TextEditor_HandleError, Error_NothingToRedo);

    RETURN(FALSE);
    return(FALSE);
  }
}
///

///AddToUndoBuffer()
long AddToUndoBuffer(enum EventType eventtype, char *eventdata, struct InstData *data)
{
  ENTER();

  D(DBF_UNDO, "undolevel: %ld undocur: %ld undofill: %ld", data->undolevel, data->undocur, data->undofill);

  if(data->undolevel > 0)
  {
    struct UserAction *buffer;

    // check if there is still enough space in our undo buffer
    // and if not we clean it up a bit (10 entries)
    if(data->undofill+1 > data->undolevel)
    {
      ULONG c;
      char *t_undobuffer = (char *)data->undobuffer;

      // cleanup at most the first 10 entries in our undobuffer
      for(c=0; c < 10 && data->undofill > 0; c++)
      {
        struct UserAction *t_buffer = (struct UserAction *)t_undobuffer;

        if(t_buffer->type == ET_DELETEBLOCK)
          MyFreePooled(data->mypool, t_buffer->clip);

        t_undobuffer += sizeof(struct UserAction);

        data->undocur--;
        data->undofill--;
      }

      D(DBF_UNDO, "undobuffer was filled up, cleaned up the first %ld entries", c+1);

      if(data->undofill > 0)
      {
        // copy everything from t_undobuffer to our start of the
        // undobuffer and set the undopointer accordingly.
        memcpy(data->undobuffer, t_undobuffer, data->undosize-(t_undobuffer-(char *)data->undobuffer));
        data->undopointer = (APTR)(t_undobuffer - (char *)data->undopointer);
      }

      // signal the user that something in the
      // undo buffer was lost.
      data->flags |= FLG_UndoLost;
    }

    buffer = data->undopointer;

    // as we are about to set something new for an undo
    // operation we have to signal that redo operation
    // is cleared now.
    if(data->undocur < data->undofill)
    {
      char *t_undobuffer = (char *)data->undopointer;

      D(DBF_UNDO, "cleaning up %ld dropped undo nodes", data->undofill-data->undocur);

      // we have to first cleanup each buffer of the ET_PASTEBLOCK
      // event or otherwise we lost some memory.
      while(data->undofill > data->undocur)
      {
        struct UserAction *t_buffer = (struct UserAction *)t_undobuffer;

        if(t_buffer->type == ET_PASTEBLOCK)
        {
          D(DBF_UNDO, "cleaning uf paste buffer of user action %08lx", t_buffer);
          MyFreePooled(data->mypool, t_buffer->clip);
        }

        t_undobuffer += sizeof(struct UserAction);
        data->undofill--;
      }

      set(data->object, MUIA_TextEditor_RedoAvailable, FALSE);
    }

    // if undocur is zero we have to send the undoavailable
    // signal
    if(data->undocur == 0)
      set(data->object, MUIA_TextEditor_UndoAvailable, TRUE);

    data->undopointer = (APTR)((char *)data->undopointer + sizeof(struct UserAction));
    data->undocur++;
    data->undofill = data->undocur;

    buffer->x  = data->CPos_X;
    buffer->y  = LineNr(data->actualline, data);
    buffer->type = eventtype;

    switch(eventtype)
    {
      case ET_BACKSPACEMERGE:
      case ET_MERGELINES:
      {
        buffer->del.style = data->actualline->next->line.Color;
        buffer->del.flow = data->actualline->next->line.Flow;
        buffer->del.separator = data->actualline->next->line.Separator;
      }
      break;

      case ET_DELETECHAR:
      case ET_BACKSPACECHAR:
      {
        buffer->del.character = *eventdata;
        buffer->del.style = GetStyle(data->CPos_X, data->actualline);
        buffer->del.flow = data->actualline->line.Flow;
        buffer->del.separator = data->actualline->line.Separator;
      }
      break;

      case ET_PASTEBLOCK:
      {
        struct marking *block = (struct marking *)eventdata;

        buffer->x  = block->startx;
        buffer->y  = LineNr(block->startline, data);
        buffer->blk.x = block->stopx;
        buffer->blk.y = LineNr(block->stopline, data);
      }
      break;

      case ET_DELETEBLOCK:
      {
        char *text;
        struct marking *block = (struct marking *)eventdata;

        if((text = GetBlock((struct marking *)eventdata, data)))
        {
          buffer->x = block->startx;
          buffer->y = LineNr(block->startline, data);
          buffer->clip = (unsigned char *)text;

          if(data->flags & FLG_FreezeCrsr)
            buffer->type = ET_DELETEBLOCK_NOMOVE;
        }
        else
        {
          ResetUndoBuffer(data);
          DoMethod(data->object, MUIM_TextEditor_HandleError, Error_NotEnoughUndoMem);
        }
      }
      break;

      default:
        // nothing to do
      break;
    }
  }

  RETURN(TRUE);
  return(TRUE);
}
///

///ResetUndoBuffer()
void ResetUndoBuffer(struct InstData *data)
{
  ENTER();

  if(data->undosize != 0)
  {
    struct UserAction *buffer = (struct UserAction *)data->undobuffer;

    while(data->undocur > 0)
    {
      if(buffer->type == ET_DELETEBLOCK)
        MyFreePooled(data->mypool, buffer->clip);

      buffer++;
      data->undocur--;
      data->undofill--;
    }

    while(data->undofill > 0)
    {
      if(buffer->type == ET_PASTEBLOCK)
        MyFreePooled(data->mypool, buffer->clip);

      buffer++;
      data->undofill--;
    }

    data->undopointer = data->undobuffer;

    ASSERT(data->undocur == 0);
    ASSERT(data->undofill == 0);
  }

  LEAVE();
}
///

///ResizeUndoBuffer()
void ResizeUndoBuffer(struct InstData *data, ULONG level)
{
  ENTER();

  if(data->undolevel != level)
  {
    D(DBF_UNDO, "resizing undo buffer for %ld undo levels", level);

    if(data->undobuffer != NULL)
    {
      ResetUndoBuffer(data);
      MyFreePooled(data->mypool, data->undobuffer);
    }

    data->undobuffer = NULL;
    data->undopointer = NULL;
    data->undosize = 0;
    data->undofill = 0;
    data->undocur = 0;
    data->undolevel = level;

    if(level > 0)
    {
      ULONG newSize;

      // calculate number of bytes from number of undo levels
      newSize = (level * sizeof(struct UserAction));

      // allocate a new undo buffer
      if((data->undobuffer = MyAllocPooled(data->mypool, newSize)) != NULL)
      {
        data->undopointer = data->undobuffer;
        data->undosize = newSize;
      }
    }
  }

  LEAVE();
}
///
