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


