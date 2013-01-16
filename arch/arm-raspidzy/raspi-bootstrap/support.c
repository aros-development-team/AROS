/*
    Copyright (C) 2013 The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: Miscellaneous support functions.
    Lang: English
*/

#include <inttypes.h>
#include <stdio.h>

#include <raspi/raspi.h>

extern void ser_PutCMINIUART(uint32_t c);

void bprintf(const char *format, ...) {
    char tmpbuf[512];

	char *out = tmpbuf;
	va_list vp;

	va_start(vp, format);
	vsnprintf(tmpbuf, 511, format, vp);
	va_end(vp);

    while(*out) {
        /*
            Really should be to the console, but we don't have that yeat
        */
        ser_PutCMINIUART(*out++);
    }
}
