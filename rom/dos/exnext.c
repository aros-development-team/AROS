/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(LONG, ExNext,

/*  SYNOPSIS */
	AROS_LHA(BPTR                  , lock, D1),
	AROS_LHA(struct FileInfoBlock *, fileInfoBlock, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 18, Dos)

/*  FUNCTION

    Examine the next entry in a directory.
        
    INPUTS

    lock  --  lock on the direcory the contents of which to examine
    fib   --  a FileInfoBlock previously initialized by Examine()
              (or used before with ExNext())

    RESULT

    success  --  a boolean telling whether the operation was successful
                 or not. A failure occurs also if there is no "next" entry in
		 the directory. Then IoErr() equals ERROR_NO_MORE_ENTRIES.

    NOTES

    If scanning a filesystem tree recursively, you'll need to allocate a
    new FileInfoBlock for each directory level.

    EXAMPLE

    To examine a directory, do the following:

    1.  Pass a lock on the directory and a FileInfoBlock (allocated by
        AllocDosObject()) to Examine().
    2.  Pass the same parameters to ExNext().
    3.  Do something with the FileInfoBlock returned.
    4.  Call ExNext() repeatedly until it returns FALSE and use the
        information you are provided. When ExNext returns FALSE, check IoErr()
	to make sure that there was no real failure (ERROR_NO_MORE_ENTRIES).

    BUGS

    SEE ALSO

    Examine(), IoErr(), AllocDosObject(), ExAll()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Get pointer to filehandle */
    struct FileLock *fl = BADDR(lock);
    LONG ret;

    ASSERT_VALID_PTR_OR_NULL(BADDR(lock));
    ASSERT_VALID_FILELOCK(lock);

    D(bug("[ExNext] lock=%x fib=%x\n", fl, fileInfoBlock));
    ret = dopacket2(DOSBase, NULL,  fl->fl_Task, ACTION_EXAMINE_NEXT, lock, MKBADDR(fileInfoBlock));
    if (ret) {
    	fixfib(fileInfoBlock);
    	D(bug("[ExNext] '%s'\n", fileInfoBlock->fib_FileName));
    } else {
    	D(bug("[ExNext] ret=%d err=%d\n", ret, IoErr()));
    }
    return ret;

    AROS_LIBFUNC_EXIT
} /* ExNext */
