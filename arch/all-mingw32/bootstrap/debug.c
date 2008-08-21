#include <stdio.h>
#include "debug.h"

void Host_VKPrintF(const char * fmt, va_list args)
{
    return vprintf(fmt, args);
}
