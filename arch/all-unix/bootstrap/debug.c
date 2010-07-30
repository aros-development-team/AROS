#include <stdarg.h>
#include <stdio.h>

#include "debug.h"

#define D(x)

int Host_VKPrintF(const char * fmt, va_list args)
{
    return vprintf(fmt, args);
}

int Host_PutChar(int c)
{
    return putchar(c);
}

void Host_Alert(char *text)
{
    printf("[Host] Displaying alert:\n%s\n", text);
    /* TODO: what to do with it? */
}
