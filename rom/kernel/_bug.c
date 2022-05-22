/*
    Copyright (C) 1995-2022, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <stdarg.h>
#include <stdio.h>

#include <kernel_base.h>
#include <kernel_debug.h>

/*
 * This is the internal version of KrnBug(), which is called directly by the kernel bug() macro.
 * To reimplement it, override this file using the build_archspecific metamake macros.
 * Generally, however, doing this is undesirable because __vcformat() supports
 * AROS-specific extensions, like %b, which are unlikely supported by your host OS.
 */
int krnBug(const char *format, va_list args, APTR kernelBase)
{
    return __vcformat(kernelBase, (int (*)(int, void *))krnPutC, format, args);
}
