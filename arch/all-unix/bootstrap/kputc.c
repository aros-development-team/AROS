/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdarg.h>
#include <stdio.h>

#include "hostlib.h"

/*
 * Redirect debug output to stderr. This is especially
 * needed on iOS where reading stdout is only possible with
 * remote gdb, which is tied to XCode's own build system.
 * On other unixes this won't hurt either.
 */
int KPutC(int chr)
{
    int ret;

    ret = fputc(chr, stderr);
    if (chr == '\n')
        fflush(stderr);
    
    return ret;
}
