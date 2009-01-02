#include <stdio.h>
#include <stdlib.h>

int vasprintf(char **ret, const char *format, va_list ap)
{    
    /* This can be done better, but i hope this is enough */
    *ret = malloc(1024);
    if (!*ret)
	return -1;
    return vsnprintf(*ret, 1023, format, ap);
}
