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


