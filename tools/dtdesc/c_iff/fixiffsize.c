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
 *  fixiffsize.c - fix the size when closing the iff
 */

#include "c_iff.h"

/****** c_iff/FixIFFSize ****************************************************
*
*   NAME
*       FixIFFSize -- Set the internal size of the IFF-file
*
*   SYNOPSIS
*       FixIFFSize( TheHandle )
*
*       void FixIFFSize( struct IFFHandle * )
*
*   FUNCTION
*       This internal function is called shortly before closing the IFF-file.
*       It fixes the internal size (offset 4) of the IFF-file.
*
*   INPUTS
*       TheHandle   - IFFHandle to fix
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*       This function ignores IFFHandles opened for reading.
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

void FixIFFSize(struct IFFHandle *TheHandle)
{
 long Buffer;

 if(!TheHandle)
 {
  return;
 }

 if(!TheHandle->NewIFF)
 {
  return;
 }

 if(fseek(TheHandle->TheFile, 4, SEEK_SET))
 {
  return;
 }

 Buffer=TheHandle->IFFSize;
 Buffer=Swap32IfLE(Buffer);

 fwrite((void *) &Buffer, 4, 1, TheHandle->TheFile);
}


