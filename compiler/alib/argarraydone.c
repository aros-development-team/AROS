/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************

    NAME */

#include <proto/icon.h>

extern struct DiskObject *__alib_dObject;
extern struct Library *IconBase;

       VOID  ArgArrayDone(

/*  SYNOPSIS */
       VOID              )

/*  FUNCTION
	Cleans up after a call to ArgArrayInit().  Make sure you don't need
	the tooltype anymore array before calling this function. 

    INPUTS

    RESULT
	Cleanup is made after an ArgArrayInit() call. This includes
	deallocation of the tooltype array returned from that function.

    NOTES
	The tooltype array got from ArgArrayInit() will no longer be valid
	after a call to this function.

    EXAMPLE

    BUGS

    SEE ALSO
	ArgArrayInit()

    INTERNALS

    HISTORY

    05.05.98  SDuvan  implemented

*****************************************************************************/
{
    if(__alib_dObject != NULL)
	FreeDiskObject(__alib_dObject);
} /* ArgArrayDone */


