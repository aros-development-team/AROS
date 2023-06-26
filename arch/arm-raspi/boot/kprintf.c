/*
    Copyright (C) 2013, The AROS Development Team. All rights reserved.
*/

#include <stdio.h>

#include "serialdebug.h"
#include "vc_fb.h"
//#include "bootconsole.h"

void putBytes(const char *str)
{
    const char *s;
    
    s = str;
    while(*s)
    {
        putByte(*s++);
    }
    /*
    if (fb_is_initialized)
    {
        s = str;
        while (*s)
        {
            fb_Putc(*s++);
        }
    }
    */
}

static char tmpbuf[512];

void kprintf(const char *format, ...)
{
    char *out = tmpbuf;
    va_list vp;

    va_start(vp, format);
    vsnprintf(tmpbuf, 511, format, vp);
    va_end(vp);

    putBytes(out);
}
