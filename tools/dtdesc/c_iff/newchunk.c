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
*       int NewChunk( struct IFFHandle *,uint32_t )
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

int NewChunk(struct IFFHandle *TheHandle, uint32_t ID)
{
 uint32_t Buffer[2];
 struct ChunkNode *CN, *PN;

 if(!TheHandle)
 {
  return(FALSE);
 }

 CN=(struct ChunkNode *) malloc(sizeof(struct ChunkNode));
 if(!CN)
 {
  return(FALSE);
 }

 Buffer[0]=ID;
 Buffer[1]=0;

 Buffer[0]=Swap32IfLE(Buffer[0]);
 Buffer[1]=Swap32IfLE(Buffer[1]);

 if(!(fwrite((void *) Buffer, sizeof(uint32_t), 2, TheHandle->TheFile)==2))
 {
  free((void *) CN);
  return(FALSE);
 }

 CN->Size=0;
 CN->FilePos=ftell(TheHandle->TheFile);
 CN->FilePos-=sizeof(uint32_t);
 CN->Previous=TheHandle->LastNode;

 PN=CN->Previous;

 while(PN)
 {
  PN->Size+=2*sizeof(uint32_t);

  PN=PN->Previous;
 }

 TheHandle->IFFSize+=2*sizeof(uint32_t);
 TheHandle->LastNode=CN;

 return(TRUE);
}


