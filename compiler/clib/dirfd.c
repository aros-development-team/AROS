/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function dirfd().
*/

#define DEBUG 0
#include <aros/debug.h>

#include "__dirdesc.h"

/*****************************************************************************

    NAME */
#include <dirent.h>

	int dirfd(

/*  SYNOPSIS */
	DIR *dir)

/*  FUNCTION
	get directory stream file descriptor

    INPUTS
	dir - directory stream dir.

    RESULT
	on error -1 is returned.

    NOTES
       This descriptor is the one used internally by the directory stream.  As
       a  result,  it  is  only useful for functions which do not depend on or
       alter the file position, such as fstat(2) and fchdir(2).   It  will  be
       automatically closed when closedir(3) is called.

    EXAMPLE

    BUGS

    SEE ALSO
 	open(), readdir(), closedir(), rewinddir(), seekdir(),
	telldir()

    INTERNALS

******************************************************************************/
{
  D(bug("dirfd()=%d\n", dir->fd));
  return dir->fd;
}
