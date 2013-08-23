/***************************************************************************

 NList.mcc - New List MUI Custom Class
 Registered MUI class, Serial Number: 1d51 0x9d510030 to 0x9d5100A0
                                           0x9d5100C0 to 0x9d5100FF

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2013 by NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

#include <exec/types.h>
#include <proto/exec.h>

/// AllocVecPooled
// allocate a vector of <memSize> bytes from the pool specified by <poolHeader>
APTR AllocVecPooled(APTR poolHeader, ULONG memSize)
{
  ULONG *memory;

  // add the number of bytes used to store the size information
  memSize += sizeof(ULONG);

  // allocate memory from the pool
  if((memory = AllocPooled(poolHeader, memSize)) != NULL)
  {
    // and finally store the size of the memory block, including the size itself
    *memory++ = memSize;
  }

  return memory;
}

///
