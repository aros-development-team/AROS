/*
 *  c_iff - a portable IFF-parser
 *
 *  Copyright (C) 2000 Joerg Dietrich
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 *  endchunk.c - finish writing a chunk
 */

#include "c_iff.h"

/****** c_iff/EndChunk ******************************************************
*
*   NAME
*       EndChunk -- Finish writing a chunk
*
*   SYNOPSIS
*       EndChunk( TheHandle )
*
*       void EndChunk( struct IFFHandle * )
*
*   FUNCTION
*       Finishes the actual chunk startet with NewChunk() .
*       Mainly this means writing the chunksize.
*
*   INPUTS
*       TheHandle   - the IFFHandle to write to
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       NewChunk()
*
*****************************************************************************
*
*       Private notes:
*/

void EndChunk(struct IFFHandle *TheHandle)
{
 long Buffer;
 long CurPos;
 struct ChunkNode *CN, *PN;

 if(!TheHandle)
 {
  return;
 }

 CN=TheHandle->LastNode;
 if(!CN)
 {
  return;
 }

 Buffer=CN->Size;
 Buffer=Swap32IfLE(Buffer);

 CurPos=ftell(TheHandle->TheFile);
 if(CurPos<0)
 {
  return;
 }

 if(fseek(TheHandle->TheFile, TheHandle->LastNode->FilePos, SEEK_SET))
 {
  return;
 }

 if(!(fwrite((void *) &Buffer, 4, 1, TheHandle->TheFile)==1))
 {
  return;
 }

 if(fseek(TheHandle->TheFile, CurPos, SEEK_SET))
 {
  return;
 }

 if(CN->Size&0x1)
 {
  if(!(fwrite("\0", 1, 1, TheHandle->TheFile)==1))
  {
   return;
  }

  PN=CN->Previous;

  while(PN)
  {
   PN->Size++;

   PN=PN->Previous;
  }

  TheHandle->IFFSize++;
 }

 TheHandle->LastNode=CN->Previous;
 free((void *) CN);
}


