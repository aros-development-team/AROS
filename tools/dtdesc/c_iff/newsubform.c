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

/****** c_iff/NewSubFORM ****************************************************
*
*   NAME
*       NewSubFORM -- Start a new sub-FORM
*
*   SYNOPSIS
*       Success = NewSubFORM( TheHandle,Type )
*
*       int NewSubFORM( struct IFFHandle *,uint32_t )
*
*   FUNCTION
*       Some IFF's, e.g. ANIM, have cascading FORM's, this means one or
*       more FORM's inside the main-FORM.
*       With NewSubFORM() you can start such a child-FORM.
*
*   INPUTS
*       TheHandle   - IFFHandle to write to
*       Type        - type of the sub-FORM
*
*   RESULT
*       Success     - TRUE when the FORM-header is succesfully written,
*                     otherwise FALSE
*
*   EXAMPLE
*
*   NOTES
*       Sub-FORM's startet with NewSubFORM() must be finished
*       with EndChunk() to correct the internal size.
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

int NewSubFORM(struct IFFHandle *TheHandle, uint32_t Type)
{
 uint32_t Buffer[3];
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

 Buffer[0]=ID_FORM;
 Buffer[1]=0;
 Buffer[2]=Type;

 Buffer[0]=Swap32IfLE(Buffer[0]);
 Buffer[1]=Swap32IfLE(Buffer[1]);
 Buffer[2]=Swap32IfLE(Buffer[2]);

 if(!(fwrite((void *) Buffer, sizeof(uint32_t), 3, TheHandle->TheFile)==3))
 {
  free((void *) CN);
  return(FALSE);
 }

 CN->Size=sizeof(uint32_t);
 CN->FilePos=ftell(TheHandle->TheFile);
 CN->FilePos-=8;
 CN->Previous=TheHandle->LastNode;

 PN=CN->Previous;

 while(PN)
 {
  PN->Size+=3*sizeof(uint32_t);

  PN=PN->Previous;
 }

 TheHandle->IFFSize+=3*sizeof(uint32_t);
 TheHandle->LastNode=CN;

 return(TRUE);
}


