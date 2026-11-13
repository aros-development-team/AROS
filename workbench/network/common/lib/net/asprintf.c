#include <clib/netlib_protos.h>
#include <stdio.h>

int asprintf(char **ret, const char *format, ...)
{
    va_list ap;
    int r;
    
    va_start(ap, format);
    r = vasprintf(ret, format, ap);
    va_end(ap);
    return r;
}
