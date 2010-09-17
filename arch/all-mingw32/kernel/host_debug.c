#include <windows.h>

#include "host_intern.h"

HANDLE conin, conout;

int __declspec(dllexport) core_putc(char c)
{
    DWORD cnt;

    WriteConsole(conout, &c, 1, &cnt, NULL);
    return cnt;
}

int __declspec(dllexport) core_getc(void)
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
    } while ((input.EventType != KEY_EVENT) || (!input.Event.KeyEvent.bKeyDown));

    return input.Event.KeyEvent.uChar.AsciiChar;
}
