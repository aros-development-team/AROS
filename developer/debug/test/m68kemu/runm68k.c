/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

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
    size_t len = 0;
    for (int i = 2; i < argc; i++) {
        if (i > 2 && len + 1 < sizeof(argbuf)) {
            argbuf[len++] = ' ';
            argbuf[len] = '\0';
        }
        size_t arglen = strnlen(argv[i], sizeof(argbuf));
        if (len + arglen < sizeof(argbuf) - 2) {
            memcpy(argbuf + len, argv[i], arglen);
            len += arglen;
            argbuf[len] = '\0';
        }
    }
    if (len + 1 < sizeof(argbuf)) {
        argbuf[len++] = '\n';
        argbuf[len] = '\0';
    }

    LONG (*RunFile)(CONST_STRPTR, ULONG, CONST_STRPTR, ULONG) =
        (LONG (*)(CONST_STRPTR, ULONG, CONST_STRPTR, ULONG))__AROS_GETVECADDR(emubase, 6);

    LONG result = RunFile(argv[1], 0, argbuf, (ULONG)len);

    CloseLibrary(emubase);
    return result;
}
