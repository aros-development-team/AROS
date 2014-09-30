/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdarg.h>
#include <stdio.h>

void DisplayError(char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("\n");
}
