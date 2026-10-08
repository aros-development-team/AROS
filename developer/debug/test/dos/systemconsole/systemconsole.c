/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dostags.h>
#include <stdio.h>

static ULONG failures;

static void check(BOOL okay, CONST_STRPTR what)
{
    if (!okay)
    {
        bug("SYSTEM CONSOLE FAIL: %s\n", what);
        failures++;
    }
}

int main(void)
{
    struct MsgPort *port = CreateMsgPort();
    APTR saved = GetConsoleTask();
    BPTR input = BNULL, output = BNULL;
    char child[512], command[600];

    input = Open("NIL:", MODE_OLDFILE);
    output = Open("NIL:", MODE_NEWFILE);
    check(port && input && output, "create console identity and child streams");
    if (!port || !input || !output)
        goto end;
    check(NameFromLock(GetProgramDir(), child, sizeof(child)) &&
          AddPart(child, "consolechild", sizeof(child)), "locate child program");
    if (failures)
        goto end;
    SetConsoleTask(port);
    snprintf(command, sizeof(command), "\"%s\" %lx", child, (unsigned long)(IPTR)port);
    check(SystemTags(command, SYS_Input, input, SYS_Output, output,
                     SYS_Error, output, SYS_Asynch, FALSE, TAG_DONE) == 0,
          "synchronous shell inherits console despite NIL input and output");
    snprintf(command, sizeof(command), "\"%s\" 0", child);
    check(SystemTags(command, SYS_Input, input, SYS_Output, output,
                     SYS_Error, output, SYS_Asynch, FALSE,
                     NP_ConsoleTask, (IPTR)NULL, TAG_DONE) == 0,
          "explicit console override remains authoritative");
end:
    SetConsoleTask(saved);
    if (input)
        Close(input);
    if (output)
        Close(output);
    if (port)
        DeleteMsgPort(port);
    bug("SYSTEM CONSOLE TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
