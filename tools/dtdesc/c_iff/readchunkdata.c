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
 *  readchunkdata.c - read the data of the actual chunk
 */

#include "c_iff.h"

/****** c_iff/ReadChunkData *************************************************
*
*   NAME
*       ReadChunkData -- Read some data from the current chunk
*
*   SYNOPSIS
*       Size = ReadChunkData( TheHandle,Buffer,BufferSize )
*
*       long ReadChunkData( struct IFFHandle *,char *,size_t )
*
*   FUNCTION
*       Reads some data from the current chunk into Buffer.
*       If BufferSize is greater than the chunksize, the whole chunk
*       is copied into the Buffer otherwise BufferSize bytes are copied.
*
*   INPUTS
*       TheHandle   - IFFHandle to read from
*       Buffer      - the buffer to write to
*       BufferSize  - size of the provided buffer
*
*   RESULT
*       Size        - number of bytes copied to the buffer
*                     Can be 0 when no bytes are copied,
*                     or -1 when an error occured.
*
*   EXAMPLE
*
*   NOTES
*       To read data from a chunk you have to start this chunk with
*       ReadChunkHeader() .
*       Make sure to read or skip the entire chunk!
*
*   BUGS
*
*   SEE ALSO
*       ReadChunkHeader() SkipChunkData()
*
*****************************************************************************
*
*       Private notes:
*/

long ReadChunkData(struct IFFHandle *TheHandle,
		   char *Buffer,
		   size_t BufferSize)
{
 long Ret;
 size_t BytesToRead;

 if(!(TheHandle && Buffer && (BufferSize>0)))
 {
  return(-1);
 }

 BytesToRead=(BufferSize>TheHandle->BytesLeftInChunk)?TheHandle->BytesLeftInChunk:BufferSize;

 Ret=fread(Buffer, 1, BytesToRead, TheHandle->TheFile);

 TheHandle->BytesLeftInChunk-=Ret;

 if(TheHandle->BytesLeftInChunk==0)
 {
  TheHandle->ChunkID=INVALID_ID;
 }

 return(Ret);
}


