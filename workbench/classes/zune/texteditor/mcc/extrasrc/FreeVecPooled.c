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

#include <proto/exec.h>

#define DEBUG_USE_MALLOC_REDEFINE 1
#include "Debug.h"

void FreeVecPooled(APTR pool, APTR mem)
{
  ULONG *memptr, length;

  ENTER();

  UNMEMTRACK("AllocVecPooled", mem);
  memptr = &((ULONG *)mem)[-1];
  length = *memptr;

  FreePooled(pool, memptr, length);

  LEAVE();
}
