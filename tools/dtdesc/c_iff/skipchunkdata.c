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
 *  skipchunkdata.c - skip the data of the actual chunk
 */

#include "c_iff.h"

/****** c_iff/SkipChunkData *************************************************
*
*   NAME
*       SkipChunkData -- Skip the entire current chunk
*
*   SYNOPSIS
*       Success = SkipChunkData( TheHandle )
*
*       int SkipChunkData( struct IFFHandle * )
*
*   FUNCTION
*       Skips the entire current chunk.
*       You can continue to ReadChunkHeader() the next chunk.
*
*   INPUTS
*       TheHandle   - IFFHandle to skip
*
*   RESULT
*       Success     - TRUE when successfully skiped to the end of the chunk
*                     otherwise FALSE
*
*   EXAMPLE
*
*   NOTES
*       To skip a chunk you have to start this chunk with
*       ReadChunkHeader() .
*
*   BUGS
*
*   SEE ALSO
*       ReadChunkHeader() ReadChunkData()
*
*****************************************************************************
*
*       Private notes:
*/

int SkipChunkData(struct IFFHandle *TheHandle)
{
 if(!TheHandle)
 {
  return(FALSE);
 }

 if(!TheHandle->BytesLeftInChunk)
 {
  return(TRUE);
 }

 if(fseek(TheHandle->TheFile, TheHandle->BytesLeftInChunk, SEEK_CUR))
 {
  return(FALSE);
 }

 TheHandle->ChunkID=INVALID_ID;
 TheHandle->BytesLeftInChunk=0;

 return(TRUE);
}


