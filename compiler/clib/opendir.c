#include <dos/dos.h>
#include <proto/dos.h>

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include "__open.h"
#include "__errno.h"

#include <dirent.h>

DIR *opendir(const char *name)
{
    DIR *dir;
    int fd;
    fdesc *desc;
    char *dupname;

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