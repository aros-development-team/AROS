/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <stdarg.h>
#include <stdio.h>

#include <kernel_base.h>
#include <kernel_debug.h>

/*
 * This is internal version of KrnBug(), to be called directly by bug() macro.
 * If you want to reimplement it, override this file.
 * However, generally you won't want to do this, because __vcformat() supports
 * AROS-specific extensions, like %b, which are unlikely supported by your host OS.
 */
int krnBug(const char *format, va_list args, APTR kernelBase)
{
    return __vcformat(kernelBase, (int (*)(int, void *))krnPutC, format, args);
}
