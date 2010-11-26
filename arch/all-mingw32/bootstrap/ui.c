#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

static char buf[1024];

void DisplayError(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    MessageBox(NULL, buf, "AROS bootstrap error", MB_OK|MB_ICONSTOP);
}
