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
 *  writechunkdata.c - write data to the actual chunk
 */

#include "c_iff.h"

/****** c_iff/WriteChunkData ************************************************
*
*   NAME
*       WriteChunkData -- Write some data to the current chunk
*
*   SYNOPSIS
*       Size = WriteChunkData( TheHandle,Buffer,Size )
*
*       long WriteChunkData( struct IFFHandle *,char *,size_t )
*
*   FUNCTION
*       Writes Buffer into the current chunk.
*
*   INPUTS
*       TheHandle   - IFFHandle to write to
*       Buffer      - the buffer containing the data
*       Size        - number of bytes to be written
*
*   RESULT
*       Size        - number of bytes written to the IFF-file
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

long WriteChunkData(struct IFFHandle *TheHandle,
		    char *Buffer,
		    size_t Size)
{
 long Ret;
 struct ChunkNode *CN, *PN;

 Ret=-1;

 if(!(TheHandle && Buffer))
 {
  return(-1);
 }

 Ret=fwrite(Buffer, 1, Size, TheHandle->TheFile);

 if(Ret>0)
 {
  CN=TheHandle->LastNode;

  CN->Size+=Ret;

  PN=CN->Previous;

  while(PN)
  {
   PN->Size+=Ret;

   PN=PN->Previous;
  }

  TheHandle->IFFSize+=Ret;
 }

 return(Ret);
}


