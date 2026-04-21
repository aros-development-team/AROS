/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder

    RunM68K — run an m68k binary via m68kemu.library
    Usage: RunM68K <filename> [args]
*/
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct Library *emubase;

    if (argc < 2) {
        printf("Usage: RunM68K <m68k-binary> [args]\n");
        return 20;
    }

    emubase = OpenLibrary("m68kemu.library", 0);
    if (!emubase) {
        printf("Cannot open m68kemu.library\n");
        return 20;
    }

    /* Build argument string */
    char argbuf[512] = "";
    for (int i = 2; i < argc; i++) {
        if (i > 2) strcat(argbuf, " ");
        strcat(argbuf, argv[i]);
    }
    strcat(argbuf, "\n");

    LONG (*RunFile)(CONST_STRPTR, ULONG, CONST_STRPTR, ULONG) =
        (LONG (*)(CONST_STRPTR, ULONG, CONST_STRPTR, ULONG))__AROS_GETVECADDR(emubase, 6);

    LONG result = RunFile(argv[1], 0, argbuf, strlen(argbuf));

    CloseLibrary(emubase);
    return result;
}
