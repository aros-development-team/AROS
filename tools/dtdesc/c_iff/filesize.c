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

 if(!TheFile)
 {
  return(0);
 }

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


