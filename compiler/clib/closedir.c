#include <errno.h>
#include <dirent.h>

int closedir(DIR *dir)
{
    if (!dir)
    {
        errno = EFAULT;
	return -1;
    }

    if (close(dir->fd) == -1)
    	return -1;

    free(dir->priv);
    free(dir);

    return 0;
}
