/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <android/log.h>
#include <stdarg.h>
#include <stdio.h>

#include <runtime.h>

void kprintf(const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_DEBUG, "AROS", fmt, args);
    va_end(args);
}
