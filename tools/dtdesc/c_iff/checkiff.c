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
 long Buffer[3];

 if(!TheHandle)
 {
  return(FALSE);
 }

 if(!(fread((void *) Buffer, 4, 3, TheHandle->TheFile)==3))
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

 if(!((Buffer[1]+8)==FileSize(TheHandle->TheFile)))
 {
  return(FALSE);
 }

 TheHandle->IFFType=Buffer[2];

 return(TRUE);
}


