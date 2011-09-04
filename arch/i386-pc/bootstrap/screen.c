/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: screen support functions ripped from the 32-bit native target.
*/

#include <bootconsole.h>
#include <stdarg.h>
#include <stdio.h>

static char tmpbuf[512];

void kprintf(const char *format, ...)
{
	char *out = tmpbuf;
	va_list vp;
	va_start(vp, format);

	vsnprintf(tmpbuf, 511, format, vp);

	va_end(vp);

	while (*out)
	    con_Putc(*out++);
}
