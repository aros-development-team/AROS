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
 uint32_t Buffer[2];

 if(!TheHandle)
 {
  return(FALSE);
 }

 if(!(fread((void *) Buffer, sizeof(uint32_t), 2, TheHandle->TheFile)==2))
 {
  return(FALSE);
 }

 Buffer[0]=Swap32IfLE(Buffer[0]);
 Buffer[1]=Swap32IfLE(Buffer[1]);

 TheHandle->ChunkID=Buffer[0];
 TheHandle->BytesLeftInChunk=(((Buffer[1]+1)>>1)<<1);

 return(TRUE);
}


