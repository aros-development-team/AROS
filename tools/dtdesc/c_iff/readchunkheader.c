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
 *  readchunkheader.c - read the header of the actual chunk
 */

#include "c_iff.h"

/****** c_iff/ReadChunkHeader ***********************************************
*
*   NAME
*       ReadChunkHeader -- Read the header of the current chunk
*
*   SYNOPSIS
*       Success = ReadChunkData( TheHandle )
*
*       int ReadChunkData( struct IFFHandle * )
*
*   FUNCTION
*       Reads the header of the current chunk. Fills out TheHandle->ChunkID.
*       TheHandle->BytesLeftInChunk contains the size of the entire chunk.
*
*   INPUTS
*       TheHandle   - IFFHandle to read from
*
*   RESULT
*       Success     - TRUE when the header was successfully read
*                     otherwise FALSE
*
*   EXAMPLE
*
*   NOTES
*       Attention!! Chunks are stored WORD-aligned. This means, when
*       chunksize is odd one padding-byte is appended.
*       TheHandle->BytesLeftInChunk shows the even size with the pad.
*       The previous chunk must have read completely!
*
*   BUGS
*
*   SEE ALSO
*
*****************************************************************************
*
*       Private notes:
*/

int ReadChunkHeader(struct IFFHandle *TheHandle)
{
 long Buffer[2];

 if(!TheHandle)
 {
  return(FALSE);
 }

 if(!(fread((void *) Buffer, 4, 2, TheHandle->TheFile)==2))
 {
  return(FALSE);
 }

 Buffer[0]=Swap32IfLE(Buffer[0]);
 Buffer[1]=Swap32IfLE(Buffer[1]);

 TheHandle->ChunkID=Buffer[0];
 TheHandle->BytesLeftInChunk=(((Buffer[1]+1)>>1)<<1);

 return(TRUE);
}


