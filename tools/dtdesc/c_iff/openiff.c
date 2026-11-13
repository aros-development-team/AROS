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


