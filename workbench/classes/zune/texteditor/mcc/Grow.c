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

#include <string.h>

#include <proto/exec.h>

#include "private.h"
#include "Debug.h"

/// InitGrow()
// Initialize a grow structure
void InitGrow(struct Grow *grow, APTR pool, int itemSize)
{
  ENTER();

  grow->array = NULL;
  grow->itemSize = itemSize;
  grow->itemCount = 0;
  grow->maxItemCount = 0;
  grow->pool = pool;

  LEAVE();
}

///
/// FreeGrow()
// Free the allocated memory of a grow structure and reinitialize it
void FreeGrow(struct Grow *grow)
{
  ENTER();

  if(grow->array != NULL)
    FreeVecPooled(grow->pool, grow->array);

  grow->array = NULL;
  grow->itemCount = 0;
  grow->maxItemCount = 0;

  LEAVE();
}

///
/// AddToGrow()
// Adds a new item to the given grow structure.
void AddToGrow(struct Grow *grow, void *newItem)
{
  ENTER();

  if(grow->itemCount+1 > grow->maxItemCount)
  {
    char *new_array;

    // we reserve 8 new items
    if((new_array = AllocVecPooled(grow->pool, (grow->maxItemCount+8) * grow->itemSize)) != NULL)
    {
      // Copy old contents into new array
      if(grow->array != NULL)
      {
        memcpy(new_array, grow->array, grow->itemCount * grow->itemSize);
        FreeVecPooled(grow->pool, grow->array);
      }

      grow->array = new_array;
      grow->maxItemCount += 8;
    }
  }

  // copy the new item to the grow structure only if there is space left
  if(grow->itemCount < grow->maxItemCount)
  {
    memcpy(&grow->array[grow->itemCount * grow->itemSize], newItem, grow->itemSize);
    grow->itemCount++;
  }

  LEAVE();
}

///
/// RemoveFromGrow()
// Remove the last item from a grow structure
void RemoveFromGrow(struct Grow *grow)
{
  ENTER();

  if(grow->itemCount > 0)
    grow->itemCount--;

  LEAVE();
}

///
