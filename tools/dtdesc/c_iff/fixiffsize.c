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
 uint32_t Buffer;

 if(!TheHandle)
 {
  return;
 }

 if(!TheHandle->NewIFF)
 {
  return;
 }

 if(fseek(TheHandle->TheFile, sizeof(uint32_t), SEEK_SET))
 {
  return;
 }

 Buffer=TheHandle->IFFSize;
 Buffer=Swap32IfLE(Buffer);

 fwrite((void *) &Buffer, sizeof(uint32_t), 1, TheHandle->TheFile);
}


