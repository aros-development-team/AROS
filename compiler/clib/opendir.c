/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX function opendir()
    Lang: english
*/

#include <dos/dos.h>
#include <proto/dos.h>

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "__open.h"
#include "__errno.h"

/*****************************************************************************

    NAME */
#include <dirent.h>

	DIR *opendir(

/*  SYNOPSIS */
	const char *name)

/*  FUNCTION
	Opens a directory

    INPUTS
	pathname - Path and filename of the directory you want to open.

    RESULT
	NULL for error or a directory stream

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
 	open(), readdir(), closedir(), rewinddir(), seekdir(),
	telldir(), scandir()

    INTERNALS

    HISTORY
	09.06.2001 falemagn created
******************************************************************************/
{
    DIR *dir;
    int fd;
    fdesc *desc;

    if (!name)
    {
    	errno = EFAULT;
	goto err1;
    }

    dir = malloc(sizeof(DIR));
    if (!dir) goto err1;

    dir->priv = malloc(sizeof(struct FileInfoBlock));
    if (!dir->priv) goto err2;

    fd = open(name, O_RDONLY);
    desc = __getfdesc(fd);
    if (!desc) goto err3;

    if (!ExamineFH(desc->fh, dir->priv))
    {
        errno = IoErr2errno(IoErr());
        goto err4;
    }

    if (((struct FileInfoBlock *)dir->priv)->fib_DirEntryType<=0)
    {
	errno = ENOTDIR;
	goto err4;
    }

    dir->fd = fd;
    dir->pos = 0;
    dir->ent.d_name[NAME_MAX] = '\0';

    return dir;

err4:
    close(fd);
err3:
    free(dir->priv);
err2:
    free(dir);
err1:
    return NULL;
}
