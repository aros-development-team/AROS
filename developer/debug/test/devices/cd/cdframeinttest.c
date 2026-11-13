/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CD_ADDFRAMEINT/CD_REMFRAMEINT test. Requires an audio track numbered 2.
    Tests pending-request ownership, playback, pause, removal and abort.
*/
#include <aros/debug.h>
#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/interrupts.h>
#include <devices/cd.h>

static ULONG failures;
static volatile ULONG counters[2];

static AROS_INTH1(countframe, volatile ULONG *, counter)
{
    AROS_INTFUNC_INIT
    (*counter)++;
    return FALSE;
    AROS_INTFUNC_EXIT
}

static void check(BOOL okay, CONST_STRPTR what)
{
    if (!okay)
    {
        bug("CD FRAME FAIL: %s\n", what);
        failures++;
    }
}

int main(void)
{
    struct MsgPort *port = CreateMsgPort();
    struct IOStdReq *io[4] = { NULL, NULL, NULL, NULL };
    struct Interrupt handlers[2] = { { 0 }, { 0 } };
    ULONG opened = 0, registered = 0, i, before[2];
    BOOL playing = FALSE;

    if (!port)
    {
        check(FALSE, "create reply port");
        goto end;
    }
    for (i = 0; i < 4; i++)
    {
        io[i] = (struct IOStdReq *)CreateIORequest(port, sizeof(*io[i]));
        if (!io[i] || OpenDevice("cd.device", 0, (struct IORequest *)io[i], 0))
        {
            check(FALSE, "open CD request");
            goto end;
        }
        opened++;
    }
    for (i = 0; i < 2; i++)
    {
        handlers[i].is_Node.ln_Type = NT_INTERRUPT;
        handlers[i].is_Data = (APTR)&counters[i];
        handlers[i].is_Code = (VOID_FUNC)countframe;
        io[i]->io_Command = CD_ADDFRAMEINT;
        io[i]->io_Data = &handlers[i];
        io[i]->io_Length = sizeof(handlers[i]);
        SendIO((struct IORequest *)io[i]);
        Delay(2);
        check(!CheckIO((struct IORequest *)io[i]) && io[i]->io_Error == 0,
              "registration remains pending");
        if (CheckIO((struct IORequest *)io[i]))
        {
            WaitIO((struct IORequest *)io[i]);
            goto end;
        }
        registered |= 1UL << i;
    }
    Delay(10);
    check(counters[0] == 0 && counters[1] == 0, "no callbacks before audio");

    io[3]->io_Command = CD_SEARCH;
    io[3]->io_Length = CDMODE_NORMAL;
    check(DoIO((struct IORequest *)io[3]) == 0 &&
          io[3]->io_Actual == CDMODE_NORMAL, "select normal playback mode");

    io[2]->io_Command = CD_PLAYTRACK;
    io[2]->io_Offset = 2;
    io[2]->io_Length = 1;
    SendIO((struct IORequest *)io[2]);
    playing = TRUE;
    Delay(25);
    check(io[2]->io_Error == 0 && !CheckIO((struct IORequest *)io[2]),
          "audio playback remains pending");
    check(counters[0] >= 20 && counters[0] <= 60 && counters[1] == counters[0],
          "both listeners receive CD frames");
    io[3]->io_Command = CD_PAUSE;
    io[3]->io_Length = 1;
    check(DoIO((struct IORequest *)io[3]) == 0, "pause audio");
    before[0] = counters[0];
    before[1] = counters[1];
    Delay(10);
    check(counters[0] == before[0] && counters[1] == before[1], "no callbacks while paused");
    io[3]->io_Length = 0;
    check(DoIO((struct IORequest *)io[3]) == 0, "resume audio");
    Delay(10);
    check(counters[0] > before[0] && counters[1] > before[1], "callbacks resume");

    io[0]->io_Command = CD_REMFRAMEINT;
    check(DoIO((struct IORequest *)io[0]) == 0, "remove with original request");
    registered &= ~1UL;
    before[0] = counters[0];
    before[1] = counters[1];
    Delay(10);
    check(counters[0] == before[0] && counters[1] > before[1], "only removed listener stops");
    AbortIO((struct IORequest *)io[1]);
    check(WaitIO((struct IORequest *)io[1]) == CDERR_ABORTED, "abort frame registration");
    registered &= ~2UL;
    before[1] = counters[1];
    Delay(10);
    check(counters[1] == before[1], "aborted listener stops");

    io[0]->io_Command = CD_ADDFRAMEINT;
    io[0]->io_Length = sizeof(handlers[0]) - 1;
    check(DoIO((struct IORequest *)io[0]) == CDERR_BADLENGTH, "reject short interrupt structure");
    io[0]->io_Length = sizeof(handlers[0]);
    io[0]->io_Data = NULL;
    check(DoIO((struct IORequest *)io[0]) == CDERR_BADADDRESS, "reject null interrupt");
end:
    for (i = 0; i < 2; i++)
        if (registered & (1UL << i))
        {
            AbortIO((struct IORequest *)io[i]);
            WaitIO((struct IORequest *)io[i]);
        }
    if (playing)
    {
        AbortIO((struct IORequest *)io[2]);
        WaitIO((struct IORequest *)io[2]);
    }
    for (i = 0; i < 4; i++)
    {
        if (i < opened) CloseDevice((struct IORequest *)io[i]);
        if (io[i]) DeleteIORequest((struct IORequest *)io[i]);
    }
    if (port) DeleteMsgPort(port);
    bug("CD FRAME TEST: %lu failures; counters %lu/%lu\n",
        (unsigned long)failures, (unsigned long)counters[0], (unsigned long)counters[1]);
    return failures ? 20 : 0;
}
