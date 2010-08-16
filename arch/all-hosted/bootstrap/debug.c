#include <stdarg.h>
#include <stdio.h>

#include "debug.h"

int Host_VKPrintF(const char * fmt, va_list args)
{
    return vprintf(fmt, args);
}
