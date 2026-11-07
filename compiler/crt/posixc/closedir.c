/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function closedir().
*/

#include <proto/dos.h>
#include <proto/exec.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "__fdesc.h"
#include "__dirdesc.h"

/*****************************************************************************

    NAME */
#include <dirent.h>

        int closedir(

/*  SYNOPSIS */
        DIR *dir)

/*  FUNCTION
         Closes a directory

    INPUTS
        dir - the directory stream pointing to the directory being closed

    RESULT
        The  closedir()  function  returns  0  on success or -1 on
        failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        close(), opendir(), readdir(), rewinddir(), seekdir(),
        telldir()

    INTERNALS

******************************************************************************/
{
    if (!dir)
    {
        errno = EFAULT;
        return -1;
    }

    /* close() removes the descriptor atomically with table snapshots. */
    if (close(dir->fd) < 0)
        return -1;

    FreeDosObject(DOS_FIB, dir->priv);
    free(dir);

    return 0;
}
