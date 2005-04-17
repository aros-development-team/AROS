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

 $Id: AllocFunctions.c,v 1.1 2005/03/28 11:29:48 damato Exp $

***************************************************************************/

#include <proto/exec.h>

void *MyAllocPooled(void *pool, unsigned long length)
{
  long *mem;

  if((mem = AllocPooled(pool, length+4)))
  {
    *mem = length+4;
    mem += 1;
    
    return(mem);
  }
  else
    return(NULL);
}

void MyFreePooled(void *pool, void *mem)
{
  long *memptr = (long *)mem;
  long length;

  memptr -= 1;
  length = *(memptr);

  FreePooled(pool, memptr, length);
}
