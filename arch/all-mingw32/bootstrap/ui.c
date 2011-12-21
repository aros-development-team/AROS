#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

#include "bootstrap.h"
#include "unicode.h"

void DisplayError(char *fmt, ...)
{
    va_list args;
    LPTSTR str;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    str = StrConvert(buf);
    MessageBox(NULL, str, TEXT("AROS bootstrap error"), MB_OK|MB_ICONSTOP);
    StrFree(str);
}
