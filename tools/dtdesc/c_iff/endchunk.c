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
 *  endchunk.c - finish writing a chunk
 */

#include "c_iff.h"

/****** c_iff/EndChunk ******************************************************
*
*   NAME
*       EndChunk -- Finish writing a chunk
*
*   SYNOPSIS
*       EndChunk( TheHandle )
*
*       void EndChunk( struct IFFHandle * )
*
*   FUNCTION
*       Finishes the actual chunk startet with NewChunk() .
*       Mainly this means writing the chunksize.
*
*   INPUTS
*       TheHandle   - the IFFHandle to write to
*
*   RESULT
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

void EndChunk(struct IFFHandle *TheHandle)
{
 uint32_t Buffer;
 long CurPos;
 struct ChunkNode *CN, *PN;

 if(!TheHandle)
 {
  return;
 }

 CN=TheHandle->LastNode;
 if(!CN)
 {
  return;
 }

 Buffer=CN->Size;
 Buffer=Swap32IfLE(Buffer);

 CurPos=ftell(TheHandle->TheFile);
 if(CurPos<0)
 {
  return;
 }

 if(fseek(TheHandle->TheFile, TheHandle->LastNode->FilePos, SEEK_SET))
 {
  return;
 }

 if(!(fwrite((void *) &Buffer, sizeof(uint32_t), 1, TheHandle->TheFile)==1))
 {
  return;
 }

 if(fseek(TheHandle->TheFile, CurPos, SEEK_SET))
 {
  return;
 }

 if(CN->Size&0x1)
 {
  if(!(fwrite("\0", 1, 1, TheHandle->TheFile)==1))
  {
   return;
  }

  PN=CN->Previous;

  while(PN)
  {
   PN->Size++;

   PN=PN->Previous;
  }

  TheHandle->IFFSize++;
 }

 TheHandle->LastNode=CN->Previous;
 free((void *) CN);
}


