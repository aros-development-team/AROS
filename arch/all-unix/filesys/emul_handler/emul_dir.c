/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "unix_hints.h"

#include <sys/time.h>
#include <sys/types.h>

/* This prevents redefinition of struct timeval */
#define _AROS_TYPES_TIMEVAL_S_H_

#include <aros/debug.h>

#include "emul_intern.h"
#include "emul_unix.h"

#define is_special_dir(x) (x[0] == '.' && (!x[1] || (x[1] == '.' && !x[2])))

/*
 * Retrieves next item in the directory and updates dirpos.
 * Also skips unwanted special entries (like . and ..).
 * Host call lock is already acquired so we don't need to do it.
 */
struct dirent *ReadDir(struct emulbase *emulbase, struct filehandle *fh, IPTR *dirpos)
{
    struct dirent *dir;

    do
    {
	dir = emulbase->pdata.SysIFace->readdir(fh->fd);
	AROS_HOST_BARRIER

	if (NULL == dir)
	    break;

    } while (is_special_dir(dir->d_name));

#if DEBUG
    bug("[ReadDir] Filehandle %s, ", fh->hostname);
    if (dir)
    	bug("returning entry: %s\n", dir->d_name);
    else
    	bug("end of search\n");
#endif

    *dirpos = emulbase->pdata.SysIFace->telldir(fh->fd);
    AROS_HOST_BARRIER

    return dir;
}
