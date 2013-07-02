#include <fcntl.h>
#include <stdio.h>

#include "log.h"

/*
 * Under MinGW32CE we have problems with dup2() because STDERR_FILENO is actually fileno(stderr),
 * which is, in its turn, raw HANDLE. In order to replace it, we reassign stderr.
 * Under desktop Windows this is also going to work.
 */

char *GetAbsName(const char *filename);

int SetLog(const char *filename)
{
    FILE *res;

#ifdef UNDER_CE
    if (*filename != '\\')
    {
        char *absname = GetAbsName(filename);
        
        if (!absname)
            return -1;

        res = freopen(absname, "a", stderr);
        free(absname);
    }
    else
        /* The path is given as absolute, just use it */
#endif
    res = freopen(filename, "a", stderr);
    return res ? 0 : -1;
}
