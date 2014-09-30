/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/* These two are needed for some definitions in emul_host.h */
#include <sys/select.h>
#include <sys/statfs.h>

/* This prevents redefinition of struct timeval */
#define _AROS_TYPES_TIMEVAL_S_H_

#include <aros/debug.h>

#include "emul_intern.h"
#include "emul_unix.h"

#define is_special_dir(x) (x[0] == '.' && (!x[1] || (x[1] == '.' && !x[2])))

/*
 * Bionic lacks seekdir() and telldir(), so we use the same approach as
 * in Windows version. See comments there for explaination.
 */
struct dirent *ReadDir(struct emulbase *emulbase, struct filehandle *fh, IPTR *dirpos)
{
    struct dirent *dir;

    D(bug("[emul] Current dirpos %lu, requested %lu\n", fh->ph.dirpos, *dirpos));
    if (fh->ph.dirpos > *dirpos)
    {
	D(bug("[emul] Resetting search handle\n"));

	/* The same as DoRewindDir(), just do not torture a semaphore */
        emulbase->pdata.SysIFace->rewinddir(fh->fd);
	fh->ph.dirpos = 0;
    }

    do
    {
	do
	{
	    dir = emulbase->pdata.SysIFace->readdir(fh->fd);
            if (!dir)
		return NULL;

	    fh->ph.dirpos++;
	    D(bug("[emul] Found %s, position %lu\n", dir->d_name, fh->ph.dirpos));
        } while (fh->ph.dirpos <= *dirpos);

	(*dirpos)++;
	D(bug("[emul] New dirpos: %lu\n", *dirpos));

    } while (is_special_dir(dir->d_name));

    return dir;
}
