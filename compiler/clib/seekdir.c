/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include "__fdesc.h"
#include "__dirdesc.h"

/*****************************************************************************

    NAME */

#include <dirent.h>

	void seekdir(

/*  SYNOPSIS */
	DIR *dir,
	off_t offset)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    int pos = offset;
    fdesc *desc = __getfdesc(dir->fd);
    if (!desc)
        return;

    if (!ExamineFH(desc->fcb->fh, dir->priv))
        return;

    if (offset > 1)
       	for
	(
	    pos = 2;
	    (pos <= offset) && ExNext(desc->fcb->fh, dir->priv);
	    pos++
	);
    dir->pos = pos;
}

