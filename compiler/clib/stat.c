#include <dos/dos.h>
#include <proto/dos.h>

#include <errno.h>

#include "__time.h"
#include "__errno.h"
#include "__stat.h"

#include <sys/stat.h>

int stat(const char *path, struct stat *sb)
{
    GETUSER;

    int res = 0;
    BPTR lock;

    lock = Lock(path, SHARED_LOCK);
    if (!lock)
    {
	if  (IoErr() == ERROR_OBJECT_IN_USE)
	{
	    /* the file is already locked exclusively, so the only way to get
	       infos about it is to find it in the parent directoy with the ExNext() function
            */

	    /* return an error for now */
	    errno = EACCES;
	    return -1;
        }

	errno = IoErr2errno(IoErr());
	return -1;
    }
    else
    res = __stat(lock, sb);

    UnLock(lock);

    return res;
}