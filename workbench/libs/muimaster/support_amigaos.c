#include <stdarg.h>
#include <stdio.h>

#include "support_amigaos.h"

char *StrDup(char *x)
{
    char *dup;
    if (!x) return NULL;
    dup = AllocVec(strlen(x) + 1, MEMF_PUBLIC);
    if (dup) CopyMem((x), dup, strlen(x) + 1);
    return dup;
}

int snprintf(char *buf, int size, const char *fmt, ...)
{
    int ret;
    va_list argptr;
    va_start(argptr,fmt);
    ret = vsprintf(buf,fmt,argptr);
    va_end(argptr);
    return ret;
}

int strlcat(char *buf, char *src, int len)
{
    int l = strlen(buf);
    buf += l;
    len -= l;

    if (len>0)
    {
	int i;
	for (i=0; i < len - 1 && *src; i++)
	    *buf++ = *src++;
	*buf = 0;
    }
}
