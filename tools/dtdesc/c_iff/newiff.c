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
 *  newiff.c - open a new IFF
 */

#include "c_iff.h"

/****** c_iff/NewIFF ********************************************************
*
*   NAME
*       NewIFF -- Open a new IFF-file for writing
*
*   SYNOPSIS
*       TheHandle = NewIFF( Name,IFFType )
*
*       struct IFFHandle *NewIFF( char *,long )
*
*   FUNCTION
*       This is your function, if you want to write an IFF-file.
*       It opens a new IFF-file allocates the IFFHandle and writes
*       an IFF-header to the file.
*
*   INPUTS
*       Name        - name of the IFF-file to be created
*       IFFType     - Type of the IFF (offset 8)
*
*   RESULT
*       TheHandle   - IFFHandle to write to
*
*   EXAMPLE
*
*   NOTES
*       IFF-files created with NewIFF() must be closed with CloseIFF() .
*
*   BUGS
*
*   SEE ALSO
*       CloseIFF()
*
*****************************************************************************
*
*       Private notes:
*/

struct IFFHandle *NewIFF(char *Name, long IFFType)
{
 struct IFFHandle *Ret;
 long Buffer[3];

 Ret=NULL;

 if(!Name)
 {
  return(NULL);
 }

 Ret=(struct IFFHandle *) malloc(sizeof(struct IFFHandle));
 if(!Ret)
 {
  return(NULL);
 }

 Ret->TheFile=NULL;
 Ret->TheFile=fopen(Name, "wb");
 if(!Ret->TheFile)
 {
  free((void *) Ret);
  return(NULL);
 }

 Ret->IFFType=IFFType;
 Ret->ChunkID=INVALID_ID;
 Ret->BytesLeftInChunk=0;
 Ret->NewIFF=TRUE;
 Ret->IFFSize=4;
 Ret->LastNode=NULL;

 Buffer[0]=ID_FORM;
 Buffer[1]=0;
 Buffer[2]=IFFType;

 Buffer[0]=Swap32IfLE(Buffer[0]);
 Buffer[1]=Swap32IfLE(Buffer[1]);
 Buffer[2]=Swap32IfLE(Buffer[2]);

 if(!(fwrite((void *) Buffer, 4, 3, Ret->TheFile)==3))
 {
  fclose(Ret->TheFile);
  free((void *) Ret);
  return(NULL);
 }

 return(Ret);
}


