/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <windows.h>

#include "host_intern.h"

HANDLE conin, conout;

int __declspec(dllexport) __aros core_putc(char c)
{
    DWORD cnt;

    WriteConsole(conout, &c, 1, &cnt, NULL);
    return cnt;
}

int __declspec(dllexport) __aros core_getc(void)
{
    DWORD cnt;
    INPUT_RECORD input;

    do
    {
	if (!PeekConsoleInput(conin, &input, 1, &cnt))
	    return -1;
	if (cnt < 1)
	    return -1;

	if (!ReadConsoleInput(conin, &input, 1, &cnt))
	    return -1;
	/* Control keys also generate events with zero character, so we ignore them */
    } while ((input.EventType != KEY_EVENT) || (!input.Event.KeyEvent.bKeyDown) ||
	     (!input.Event.KeyEvent.uChar.AsciiChar));

    return input.Event.KeyEvent.uChar.AsciiChar;
}

void __declspec(dllexport) __aros core_alert(const char *text)
{
    MessageBox(NULL, text, "AROS guru meditation", MB_ICONERROR|MB_SETFOREGROUND);
}
