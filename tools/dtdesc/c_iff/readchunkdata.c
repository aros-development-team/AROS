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

 Ret=-1;

 if(!(TheHandle && Buffer && (BufferSize>0)))
 {
  return(-1);
 }

 BytesToRead=0;
 BytesToRead=(BufferSize>TheHandle->BytesLeftInChunk)?TheHandle->BytesLeftInChunk:BufferSize;

 Ret=fread(Buffer, 1, BytesToRead, TheHandle->TheFile);

 TheHandle->BytesLeftInChunk-=Ret;

 if(TheHandle->BytesLeftInChunk==0)
 {
  TheHandle->ChunkID=INVALID_ID;
 }

 return(Ret);
}


