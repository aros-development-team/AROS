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
    GETUSER;

    DIR *dir;
    int fd;
    fdesc *desc;
#if 0
    char *dupname;
#endif

    if (!name)
    {
    	errno = EFAULT;
	goto err1;
    }
/*
    dupname = strdup(name);
    if (!dupname) goto err1;

    if (name[0] == '.')
    {
    	if (name[1] == '\0')
	{
	    name = getcwd(NULL, 0);
	    if (!name) goto err1;
	}
	else
	if (name[1] == '/')
	{
	    name +=
*/
    dir = malloc(sizeof(DIR));
    if (!dir) goto err2;

    dir->priv = malloc(sizeof(struct FileInfoBlock));
    if (!dir->priv) goto err3;

    fd = open(name, O_RDONLY);
    desc = __getfdesc(fd);
    if (!desc) goto err4;

    if (!ExamineFH(desc->fh, dir->priv))
    {
        errno = IoErr2errno(IoErr());
        goto err5;
    }

    if (((struct FileInfoBlock *)dir->priv)->fib_DirEntryType<=0)
    {
	errno = ENOTDIR;
	goto err5;
    }

    dir->fd = fd;
    dir->pos = 0;
    dir->ent.d_name[NAME_MAX] = '\0';

    return dir;

err5:
    close(fd);
err4:
    free(dir->priv);
err3:
    free(dir);
err2:
//    free(dupname);
err1:
    return NULL;
}
