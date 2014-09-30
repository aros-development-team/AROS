/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

#include <runtime.h>

#include "bootstrap.h"

static LPTSTR StrConvert(const char *src)
{
    int len = strlen(src) + 1;
    LPTSTR res = malloc(len * 2);

    if (res)
    {
        if (!MultiByteToWideChar(CP_ACP, 0, src, -1, res, len))
        {
            free(res);
            return NULL;
        }
    }
    return res;
}

void kprintf(const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void DisplayError(char *fmt, ...)
{
    va_list args;
    LPTSTR str;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    str = StrConvert(buf);
    MessageBox(NULL, str, TEXT("AROS bootstrap error"), MB_OK|MB_ICONSTOP);
    free(str);
}
