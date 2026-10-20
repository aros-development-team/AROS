/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <proto/dos.h>
#include "dos_intern.h"

BPTR internal_OpenBootConsole(BOOL quiet, struct BootConsole *state,
    struct DosLibrary *DOSBase)
{
    (void)quiet;
    state->quiet = FALSE;
    return Open(BOOT_CONSOLE_NAME, MODE_OLDFILE);
}

void internal_CloseBootConsole(struct BootConsole *state,
    struct DosLibrary *DOSBase)
{
    (void)state;
    (void)DOSBase;
}

BPTR internal_OpenConsoleOutput(struct DosLibrary *DOSBase)
{
    return Open("*", MODE_NEWFILE);
}
