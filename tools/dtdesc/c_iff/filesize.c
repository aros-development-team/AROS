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
 *  filesize.c - get the size of a file
 */

#include "c_iff.h"

/****** c_iff/FileSize ******************************************************
*
*   NAME
*       FileSize -- Get the Size of a file
*
*   SYNOPSIS
*       Size = FileSize( TheFile )
*
*       size_t FileSize( FILE * )
*
*   FUNCTION
*       Returns the size of the given file.
*
*   INPUTS
*       TheFile     - the file to count
*
*   RESULT
*       Size        - size of the file
*                     or 0 when an error occurs
*
*   EXAMPLE
*
*   NOTES
*       This is a support function. It has very few to do with c_iff.
*
*   BUGS
*
*   SEE ALSO
*
*****************************************************************************
*
*       Private notes:
*/

size_t FileSize(FILE *TheFile)
{
 long Ret;
 long CurPos;

 Ret=0;

 if(!TheFile)
 {
  return(0);
 }

 CurPos=0;
 CurPos=ftell(TheFile);
 if(CurPos<0)
 {
  return(0);
 }

 if(fseek(TheFile, 0, SEEK_END))
 {
  return(0);
 }

 Ret=ftell(TheFile);
 if(Ret<0)
 {
  return(0);
 }

 if(fseek(TheFile, CurPos, SEEK_SET))
 {
  return(0);
 }

 return((size_t) Ret);
}


