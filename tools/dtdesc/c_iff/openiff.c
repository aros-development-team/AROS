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
 *  openiff.c - open an existing IFF
 */

#include "c_iff.h"

/****** c_iff/OpenIFF *******************************************************
*
*   NAME
*       OpenIFF -- Open an existing IFF-file for reading
*
*   SYNOPSIS
*       TheHandle = NewIFF( Name )
*
*       struct IFFHandle *NewIFF( char * )
*
*   FUNCTION
*       This is your function, if you want to read an existing IFF-file.
*       It opens the existing file, allocates the IFFHandle and reads
*       and confirms the IFF-header.
*
*   INPUTS
*       Name        - name of the IFF-file to be opened
*
*   RESULT
*       TheHandle   - IFFHandle to read from
*
*   EXAMPLE
*
*   NOTES
*       IFF-files opened with OpenIFF() must be closed with CloseIFF() .
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

struct IFFHandle *OpenIFF(char *Name)
{
 struct IFFHandle *Ret;

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
 Ret->TheFile=fopen(Name, "rb");
 if(!Ret->TheFile)
 {
  free((void *) Ret);
  return(NULL);
 }

 Ret->IFFType=0;
 if(!CheckIFF(Ret))
 {
  fclose(Ret->TheFile);
  free((void *) Ret);
  return(NULL);
 }

 Ret->ChunkID=INVALID_ID;
 Ret->BytesLeftInChunk=0;
 Ret->NewIFF=FALSE;
 Ret->IFFSize=0;
 Ret->LastNode=NULL;

 return(Ret);
}


