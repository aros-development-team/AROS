#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include <exec/lists.h>
#include <proto/exec.h>

#include "__stdio.h"
#include "__open.h"

FILE *fdopen(int filedes, const char *mode)
{
    int sflags, oflags;
    fdesc *fdesc;
    FILENODE *fn;

    if (!(fdesc = __getfdesc(filedes)))
    {
	errno = EBADF;
	return NULL;
    }


    oflags = fdesc->flags;

    if (mode)
    {
	int tmp;

	oflags = __smode2oflags(mode);
	tmp = oflags & O_ACCMODE;

	/* check if oflags are a subset of the flags that the already open file has */
	if (tmp != O_RDWR && (tmp != (fdesc->flags & O_ACCMODE)))
	{
	    errno = EINVAL;
	    return NULL;
	}
    }

    sflags = __oflags2sflags(oflags);

    fn = malloc(sizeof(FILENODE));
    if (!fn) return NULL;

    fn->File.flags = sflags;
    fn->File.fd = filedes;

    AddTail ((struct List *)&__stdio_files, (struct Node *)fn);

    return FILENODE2FILE(fn);
}
