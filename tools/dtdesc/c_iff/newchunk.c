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
 *  newchunk.c - open a new chunk
 */

#include "c_iff.h"

/****** c_iff/NewChunk ******************************************************
*
*   NAME
*       NewChunk -- Start a new chunk
*
*   SYNOPSIS
*       Success = NewChunk( TheHandle,ID )
*
*       int NewChunk( struct IFFHandle *,long )
*
*   FUNCTION
*       Starts a new chunk, trough writing the chunk-header.
*
*   INPUTS
*       TheHandle   - IFFHandle to write to
*       ID          - ID of the chunk
*
*   RESULT
*       Success     - TRUE when chunk-header is succesfully written,
*                     otherwise FALSE
*
*   EXAMPLE
*
*   NOTES
*       Chunks startet with NewChunk() must be finished with EndChunk()
*       to correct the internal chunk-size.
*
*   BUGS
*
*   SEE ALSO
*       EndChunk()
*
*****************************************************************************
*
*       Private notes:
*/

int NewChunk(struct IFFHandle *TheHandle, long ID)
{
 long Buffer[2];
 struct ChunkNode *CN, *PN;

 if(!TheHandle)
 {
  return(FALSE);
 }

 CN=NULL;
 CN=(struct ChunkNode *) malloc(sizeof(struct ChunkNode));
 if(!CN)
 {
  return(FALSE);
 }

 Buffer[0]=ID;
 Buffer[1]=0;

 Buffer[0]=Swap32IfLE(Buffer[0]);
 Buffer[1]=Swap32IfLE(Buffer[1]);

 if(!(fwrite((void *) Buffer, 4, 2, TheHandle->TheFile)==2))
 {
  free((void *) CN);
  return(FALSE);
 }

 CN->Size=0;
 CN->FilePos=ftell(TheHandle->TheFile);
 CN->FilePos-=4;
 CN->Previous=TheHandle->LastNode;

 PN=CN->Previous;

 while(PN)
 {
  PN->Size+=8;

  PN=PN->Previous;
 }

 TheHandle->IFFSize+=8;
 TheHandle->LastNode=CN;

 return(TRUE);
}


