#include <dos/dos.h>
#include <proto/dos.h>

#include <errno.h>

#include "__open.h"

#include <dirent.h>

struct dirent *readdir(DIR *dir)
{
    fdesc *desc;

    if (!dir)
    {
        errno = EFAULT;
	return NULL;
    }

    desc = __getfdesc(dir->fd);
    if (!desc)
    {
    	errno = EBADF;
    	return NULL;
    }

    if (ExNext(desc->fh, dir->priv))
    {
	int max = MAXFILENAMELENGTH > NAME_MAX ? NAME_MAX : MAXFILENAMELENGTH;
	strncpy
	(
	    dir->ent.d_name,
	    ((struct FileInfoBlock *)dir->priv)->fib_FileName,
	    max
        );

        return &(dir->ent);
    }

    errno = IoErr2errno(IoErr());

    return NULL;
}