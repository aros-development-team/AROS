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
*       struct IFFHandle *NewIFF( char *,CARD32 )
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

struct IFFHandle *NewIFF(char *Name, CARD32 IFFType)
{
 struct IFFHandle *Ret;
 CARD32 Buffer[3];

 if(!Name)
 {
  return(NULL);
 }

 Ret=(struct IFFHandle *) malloc(sizeof(struct IFFHandle));
 if(!Ret)
 {
  return(NULL);
 }

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
 Ret->IFFSize=sizeof(CARD32);
 Ret->LastNode=NULL;

 Buffer[0]=ID_FORM;
 Buffer[1]=0;
 Buffer[2]=IFFType;

 Buffer[0]=Swap32IfLE(Buffer[0]);
 Buffer[1]=Swap32IfLE(Buffer[1]);
 Buffer[2]=Swap32IfLE(Buffer[2]);

 if(!(fwrite((void *) Buffer, sizeof(CARD32), 3, Ret->TheFile)==3))
 {
  fclose(Ret->TheFile);
  free((void *) Ret);
  return(NULL);
 }

 return(Ret);
}


