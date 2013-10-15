/*
    Copyright � 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Function to get a dos.library file handle associated with given file 
    descriptor.
*/

#include "__fdesc.h"

/* FIXME: Do we need this function ? */

/*****************************************************************************

    NAME */

	int __get_default_file (

/*  SYNOPSIS */
	int file_descriptor,
	long * file_handle)

/*  FUNCTION
	Gets dos.library file handle associated with a given file descriptor.

    INPUTS
	file_descriptor - the File Descriptor you wish to obtain the associated 
		file handle for.
	file_handle - Pointer to store the associated file handle.

    RESULT
	!=0 on error, 0 on success.

    NOTES
        This function is not a part of the ISO C standard, it comes from clib2
        project and was implemented to make porting of abc-shell easier.
        Function should not be used in new code.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    fdesc *fd;

    if(
       file_descriptor < 0 || 
       (fd = __getfdesc(file_descriptor)) == NULL
    )
        return -1;
    else
        *(BPTR*)file_handle = fd->fcb->handle;

    return 0;
}
