/*
 *  c_iff - a portable IFF-parser
 *
 *  Copyright (C) 2000, 2001 Joerg Dietrich
 *
 *  This is the AROS-version of c_iff.
 *  It is distributed under the AROS Public License.
 *  But I reserve the right to distribute
 *  my own version under other licenses.
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


