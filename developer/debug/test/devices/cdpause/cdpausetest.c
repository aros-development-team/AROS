/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Verify CD_PAUSE compatibility with callers that only initialise the
    low word of io_Length. Requires a disc with an audio track numbered 2.
    Checks the returned previous state and the public CD_INFO pause state.
*/
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <devices/cd.h>

static ULONG checks, failures;

static void check(BOOL okay, CONST_STRPTR what)
{
    checks++;
    if (!okay)
    {
        bug("CD PAUSE FAIL: %s\n", what);
        failures++;
    }
}

static void pausecase(struct IOStdReq *io, ULONG mode, ULONG previous,
                      BOOL expected, CONST_STRPTR what)
{
    struct CDInfo info = { 0 };
    io->io_Command = CD_PAUSE;
    io->io_Length = mode;
    io->io_Data = NULL;
    io->io_Offset = 0;
    check(DoIO((struct IORequest *)io) == 0, what);
    check(io->io_Actual == previous, "return previous pause state");
    io->io_Command = CD_INFO;
    io->io_Length = sizeof(info);
    io->io_Data = &info;
    check(DoIO((struct IORequest *)io) == 0, "query pause state");
    bug("CD PAUSE mode %08lx previous %lu paused %ld expected %ld\n",
        mode, previous, (LONG)((info.Status & CDSTSF_PAUSED) != 0), (LONG)expected);
    check(((info.Status & CDSTSF_PAUSED) != 0) == expected, what);
}

int main(void)
{
    struct MsgPort *port = CreateMsgPort();
    struct IOStdReq *control = NULL, *play = NULL;
    BOOL control_open = FALSE, play_open = FALSE, playing = FALSE;

    if (!port)
        goto setupfail;
    control = (struct IOStdReq *)CreateIORequest(port, sizeof(*control));
    play = (struct IOStdReq *)CreateIORequest(port, sizeof(*play));
    if (!control || !play)
        goto setupfail;
    if (OpenDevice("cd.device", 0, (struct IORequest *)control, 0))
        goto setupfail;
    control_open = TRUE;
    if (OpenDevice("cd.device", 0, (struct IORequest *)play, 0))
        goto setupfail;
    play_open = TRUE;

    control->io_Command = CD_PAUSE;
    control->io_Length = 0;
    check(DoIO((struct IORequest *)control) == 0, "initialise unpaused state");
    play->io_Command = CD_PLAYTRACK;
    play->io_Offset = 2;
    play->io_Length = 1;
    SendIO((struct IORequest *)play);
    playing = TRUE;
    Delay(10);
    check(!CheckIO((struct IORequest *)play) && play->io_Error == 0,
          "audio play remains pending");

    pausecase(control, 1, 0, TRUE, "ordinary pause");
    pausecase(control, 0, 1, FALSE, "ordinary unpause");
    pausecase(control, 0x00010001, 0, TRUE, "pause with nonzero upper word");
    pausecase(control, 0x00020000, 1, FALSE, "unpause with nonzero upper word");
    pausecase(control, 0, 0, FALSE, "already unpaused");
    pausecase(control, 0xffff0001, 0, TRUE, "pause with all upper bits set");
    pausecase(control, 0xffff0000, 1, FALSE, "unpause with all upper bits set");
    goto end;
setupfail:
    check(FALSE, "set up CD requests");
end:
    if (playing)
    {
        AbortIO((struct IORequest *)play);
        WaitIO((struct IORequest *)play);
    }
    if (play_open) CloseDevice((struct IORequest *)play);
    if (control_open) CloseDevice((struct IORequest *)control);
    if (play) DeleteIORequest((struct IORequest *)play);
    if (control) DeleteIORequest((struct IORequest *)control);
    if (port) DeleteMsgPort(port);
    bug("CD PAUSE: %lu/%lu pass, %lu failures\n", checks - failures, checks, failures);
    return failures ? 20 : 0;
}
