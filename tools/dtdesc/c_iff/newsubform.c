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
*       int NewSubFORM( struct IFFHandle *,long )
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

int NewSubFORM(struct IFFHandle *TheHandle, long Type)
{
 long Buffer[3];
 struct ChunkNode *CN, *PN;

 if(!TheHandle)
 {
  return(FALSE);
 }

 CN=NULL;
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

 if(!(fwrite((void *) Buffer, 4, 3, TheHandle->TheFile)==3))
 {
  free((void *) CN);
  return(FALSE);
 }

 CN->Size=4;
 CN->FilePos=ftell(TheHandle->TheFile);
 CN->FilePos-=8;
 CN->Previous=TheHandle->LastNode;

 PN=CN->Previous;

 while(PN)
 {
  PN->Size+=12;

  PN=PN->Previous;
 }

 TheHandle->IFFSize+=12;
 TheHandle->LastNode=CN;

 return(TRUE);
}


