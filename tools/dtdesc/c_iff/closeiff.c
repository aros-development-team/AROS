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
 *  closeiff.c - close the IFFHandle
 */

#include "c_iff.h"

/****** c_iff/CloseIFF ******************************************************
*
*   NAME
*       CloseIFF -- Close an open IFF-file
*
*   SYNOPSIS
*       CloseIFF( TheHandle )
*
*       void CloseIFF( struct IFFHandle * )
*
*   FUNCTION
*       Closes an IFF-file previously opened by OpenIFF() or NewIFF() .
*
*   INPUTS
*       TheHandle   - IFFHandle to close
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
*       OpenIFF() NewIFF()
*
*****************************************************************************
*
*       Private notes:
*/

void CloseIFF(struct IFFHandle *TheHandle)
{
 if(TheHandle)
 {
  if(TheHandle->NewIFF)
  {
   while(TheHandle->LastNode)
   {
    EndChunk(TheHandle);
   }
  }

  FixIFFSize(TheHandle);

  if(TheHandle->TheFile)
  {
   fclose(TheHandle->TheFile);
  }

  free((void *) TheHandle);
 }
}


