/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include "__open.h"
#include <dirent.h>

void seekdir(DIR *dir, off_t offset)
{
    int pos = offset;
    fdesc *desc = __getfdesc(dir->fd);
    if (!desc)
        return;

    if (!ExamineFH(desc->fh, dir->priv))
        return;

    if (offset > 1)
       	for
	(
	    pos = 2;
	    (pos <= offset) && ExNext(desc->fh, dir->priv);
	    pos++
	);
    dir->pos = pos;
}
