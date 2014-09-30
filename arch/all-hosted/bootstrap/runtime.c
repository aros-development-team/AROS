/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <runtime.h>
#include <stdarg.h>
#include <stdio.h>

void kprintf(const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}
