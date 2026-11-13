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
 *  checkiff.c - check if it's a valid IFF
 */

#include "c_iff.h"

/****** c_iff/CheckIFF ******************************************************
*
*   NAME
*       CheckIFF -- Check the file, if it's a valid IFF
*
*   SYNOPSIS
*       Success = CheckIFF( TheHandle )
*
*       int CheckIFF( struct IFFHandle * )
*
*   FUNCTION
*       This internal function scans a file freshly opened with OpenIFF(),
*       if it is a valid IFF-file.
*
*   INPUTS
*       TheHandle   - IFFHandle to scan
*
*   RESULT
*       Success     - TRUE when it's a valid IFF, otherwise FALSE
*
*   EXAMPLE
*       
*   NOTES
*       Sets TheHandle->IFFType to the value found in the IFF-file.
*       This function only recognises 'FORM'-IFFs.
*       The "EA IFF 85" standard  specifies several other IFFs.
*       But these are very rarly used.
*       The file-position-indicator  must point to offset 0 when entering
*       this function. It points to offset 12 after leaving the function.
*
*   BUGS
*
*   SEE ALSO
*       OpenIFF()
*
*****************************************************************************
*
*       Private notes:
*/

int CheckIFF(struct IFFHandle *TheHandle)
{
 uint32_t Buffer[3];

 if(!TheHandle)
 {
  return(FALSE);
 }

 if(!(fread((void *) Buffer, sizeof(uint32_t), 3, TheHandle->TheFile)==3))
 {
  return(FALSE);
 }

 Buffer[0]=Swap32IfLE(Buffer[0]);
 Buffer[1]=Swap32IfLE(Buffer[1]);
 Buffer[2]=Swap32IfLE(Buffer[2]);

 if(!(Buffer[0]==ID_FORM))
 {
  return(FALSE);
 }

 if(!((Buffer[1] + 2*sizeof(uint32_t))==FileSize(TheHandle->TheFile)))
 {
  return(FALSE);
 }

 TheHandle->IFFType=Buffer[2];

 return(TRUE);
}


