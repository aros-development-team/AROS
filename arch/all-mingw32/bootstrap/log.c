#include <fcntl.h>
#include <stdio.h>

#include "log.h"

/*
 * Under MinGW32CE we have problems with dup2() because STDERR_FILENO is actually fileno(stderr),
 * which is, in its turn, raw HANDLE. In order to replace it, we reassign stderr.
 * Under desktop Windows this is also going to work.
 */
#ifdef UNDER_CE

char *GetAbsName(const char *filename);

int SetLog(const char *filename)
{
    FILE *res;

    if (*filename == '\\')
    {
        /* The path is given as absolute, just use it */
        res = freopen(filename, "a", stderr);
    }
    else
    {
        char *absname = GetAbsName(filename);
        
        if (!absname)
            return -1;

        res = freopen(absname, "a", stderr);
        free(absname);
    }

    return res ? 0 : -1;
}

#else

int SetLog(const char *c)
{
    return (freopen(c, "a", stderr) == NULL) ? -1 : 0;
}

#endif
