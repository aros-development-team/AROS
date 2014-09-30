/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/alerts.h>
#include <proto/intuition.h>

#include <stdio.h>
#include <string.h>

/*
 * An EXTREMELY ugly test quickly made of alert.hook source.
 * I just didn't have any other text data with proper layout.
 * Please rewrite this into something better.
 */

static UBYTE *const recov = "\x38\x0f" "Recoverable Alert! ";
static UBYTE *const mouse = "\x01\x50\x0f" "Press mouse button to continue.";
static UBYTE *const fmtstring = "\xa8\x2a" "Task:   12345678 - ";
static UBYTE *tname = "--task not found--";

static STRPTR mystrcpy(STRPTR dest, STRPTR src, LONG len)
{
    while(len && *src)
    {
        *dest++ = *src++;
        len--;
    }
    *dest++ = 0;
    return dest;
}

int main(int argc, char **argv)
{
    UBYTE buffer[256], *buf;
    BOOL ret;
    ULONG code;

    if ((argc > 1) && (!stricmp(argv[1], "deadend")))
	code = AT_DeadEnd;
    else
	code = 0;

    buffer[0] = 0;
    buf = &buffer[1];

    buf = mystrcpy(buf, recov, -1);
    *buf++ = 1;

    buf = mystrcpy(buf, mouse, -1);
    *buf++ = 1;

    *buf++ = 0;
    buf = mystrcpy(buf, fmtstring, -1);

    buf = mystrcpy(buf - 1, tname, 30);

    *buf = 0;

    ret = DisplayAlert(code, buffer, 0x38);

    printf("Result is: %d\n", ret);
    return 0;
}
