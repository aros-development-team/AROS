/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Exercise CD_PLAYTRACK callers that initialise only word-sized track
    arguments. Requires an audio track 2 longer than 0.1 seconds.
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
        bug("CD PLAYTRACK FAIL: %s\n", what);
        failures++;
    }
}

static void playcase(struct IOStdReq *play, struct IOStdReq *control,
                     ULONG track, ULONG count, BOOL valid)
{
    struct CDInfo info = { 0 };
    BOOL completed;

    play->io_Command = CD_PLAYTRACK;
    play->io_Offset = track;
    play->io_Length = count;
    play->io_Data = NULL;
    SendIO((struct IORequest *)play);
    Delay(5);
    completed = CheckIO((struct IORequest *)play) != NULL;
    bug("CD PLAYTRACK track %08lx count %08lx completed %ld error %ld\n",
        track, count, (LONG)completed, (LONG)play->io_Error);
    if (valid)
    {
        check(!completed && play->io_Error == 0,
              "valid low-word track arguments start asynchronous playback");
        control->io_Command = CD_INFO;
        control->io_Data = &info;
        control->io_Length = sizeof(info);
        check(DoIO((struct IORequest *)control) == 0, "read playback state");
        check((info.Status & CDSTSF_PLAYING) != 0, "audio playback is active");
    }
    else
        check(completed && play->io_Error == CDERR_BADLENGTH,
              "zero low-word track count is rejected");

    if (!completed)
        AbortIO((struct IORequest *)play);
    WaitIO((struct IORequest *)play);
}

int main(void)
{
    struct MsgPort *port = CreateMsgPort();
    struct IOStdReq *play = NULL, *control = NULL;
    BOOL play_open = FALSE, control_open = FALSE;

    if (!port)
        goto setupfail;
    play = (struct IOStdReq *)CreateIORequest(port, sizeof(*play));
    control = (struct IOStdReq *)CreateIORequest(port, sizeof(*control));
    if (!play || !control)
        goto setupfail;
    if (OpenDevice("cd.device", 0, (struct IORequest *)play, 0))
        goto setupfail;
    play_open = TRUE;
    if (OpenDevice("cd.device", 0, (struct IORequest *)control, 0))
        goto setupfail;
    control_open = TRUE;
    control->io_Command = CD_PAUSE;
    control->io_Length = 0;
    check(DoIO((struct IORequest *)control) == 0, "initialise unpaused state");

    playcase(play, control, 2, 1, TRUE);
    playcase(play, control, 0x00010002, 1, TRUE);
    playcase(play, control, 0xffff0002, 0x00020001, TRUE);
    playcase(play, control, 2, 0, FALSE);
    playcase(play, control, 2, 0x00010000, FALSE);
    goto end;
setupfail:
    check(FALSE, "set up CD requests");
end:
    if (control_open) CloseDevice((struct IORequest *)control);
    if (play_open) CloseDevice((struct IORequest *)play);
    if (control) DeleteIORequest((struct IORequest *)control);
    if (play) DeleteIORequest((struct IORequest *)play);
    if (port) DeleteMsgPort(port);
    bug("CD PLAYTRACK: %lu/%lu pass, %lu failures\n", checks - failures, checks, failures);
    return failures ? 20 : 0;
}
