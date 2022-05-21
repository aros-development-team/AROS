/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.

    Desc: Formats a message and makes sure the user will see it.
*/

#include <aros/config.h>
#include <aros/arossupportbase.h>
#include <stdarg.h>

#include <aros/system.h>
#include <proto/exec.h>
#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>
#include <proto/arossupport.h>
#undef vkprintf
#include <exec/execbase.h>

#include <proto/arossupport.h>

extern int _vkprintf(const char * format, va_list args);

/******************************************************************************/
int vkprintf (const char * format, va_list args)
{
  return _vkprintf (format, args);
} /* vkprintf */
