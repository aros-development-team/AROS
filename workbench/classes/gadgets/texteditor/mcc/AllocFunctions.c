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

#include <proto/exec.h>

#include "private.h"

#define DEBUG_USE_MALLOC_REDEFINE
#include "Debug.h"

/// AllocVecPooled()
#if !defined(__amigaos4__) && !defined(__MORPHOS__) && !defined(__AROS__)
APTR AllocVecPooled(APTR pool, ULONG length)
{
  ULONG *mem;

  ENTER();

  length += sizeof(ULONG);
  if((mem = AllocPooled(pool, length)))
    *mem++ = length;

  RETURN(mem);
  return(mem);
}
#endif // !__amigaos4__ && !__MORPHOS__

///
/// FreeVecPooled()
#if !defined(__amigaos4__) && !defined(__MORPHOS__) && !defined(__AROS__)
void FreeVecPooled(APTR pool, APTR mem)
{
  ULONG *memptr, length;

  ENTER();

  memptr = &((ULONG *)mem)[-1];
  length = *memptr;

  FreePooled(pool, memptr, length);

  LEAVE();
}
#endif // !__amigaos4__ && !__MORPHOS__

///
/// AllocLine()
struct line_node *AllocLine(struct InstData *data)
{
  struct line_node *newline;

  ENTER();

  newline = AllocVecPooled(data->mypool, sizeof(struct line_node));

  RETURN(newline);
  return newline;
}

///
/// FreeLine()
void FreeLine(struct InstData *data, struct line_node* line)
{
  ENTER();

  // we make sure the line is not referenced by other
  // structures as well such as the global blockinfo structure.
  if(data->blockinfo.startline == line)
  {
    if(line->next != NULL)
      data->blockinfo.startline = line->next;
    else
    {
      data->blockinfo.startline = NULL;
      data->blockinfo.enabled = FALSE;
    }
  }

  if(data->blockinfo.stopline == line)
  {
    if(line->previous != NULL)
      data->blockinfo.stopline = line->previous;
    else
    {
      data->blockinfo.stopline = NULL;
      data->blockinfo.enabled = FALSE;
    }
  }

  // finally free the line itself
  FreeVecPooled(data->mypool, line);

  LEAVE();
}

///
