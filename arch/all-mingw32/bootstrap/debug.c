#include <stdarg.h>
#include <stdio.h>
#include <windows.h>
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
    D(printf("[Host] Displaying alert:\n%s\n", text));
    MessageBox(NULL, text, "AROS Guru meditation", MB_ICONERROR|MB_SETFOREGROUND);
}
