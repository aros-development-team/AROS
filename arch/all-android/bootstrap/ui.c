/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdarg.h>
#include <stdio.h>

#include "android.h"
#include "bootstrap.h"

void DisplayError(char *fmt, ...)
{
    va_list args;
    jstring str;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    str = (*Java_Env)->NewStringUTF(Java_Env, buf);
    (*Java_Env)->CallVoidMethod(Java_Env, Java_Object, DisplayError_mid, str);
}
