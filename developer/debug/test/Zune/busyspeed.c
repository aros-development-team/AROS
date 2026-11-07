/*
    Copyright (C) 2026, The AROS Development Team.
    All rights reserved.

    Checks MUIA_Busy_Speed of Busy.mcc without user interaction:

    - the value given at creation is used (MUIV_Busy_Speed_Off keeps the
      bar still, MUIM_Busy_Move moves it);
    - OM_GET returns the value that was set, MUIV_Busy_Speed_User included;
    - a bar with MUIV_Busy_Speed_User moves by itself.

    It also prints the time of 500 MUIM_Busy_Move calls on a wide bar.

    Usage: busyspeed [<path of a Busy.mcc to test>]
    With a path, that file is opened first and is then the class used, so a
    build can be tested without installing it. Busy.mcc must not be in use
    (run "Avail FLUSH" before).
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/timer.h>
#include <clib/alib_protos.h>
#include <libraries/mui.h>
#include <mui/Busy_mcc.h>

#include <stdio.h>
#include <string.h>

struct Library *MUIMasterBase;
struct Device *TimerBase;

static Object *app;
static ULONG sigmask;
static int failures;

static void check(BOOL ok, const char *what)
{
    printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        failures++;
}

/* Let the application run its input handlers for about ticks/50 s. */
static void run_app(int ticks)
{
    while (ticks-- > 0)
    {
        ULONG got;

        Delay(1);
        got = SetSignal(0, sigmask) & sigmask;
        DoMethod(app, MUIM_Application_NewInput, (IPTR)&got);
        if (got)
            sigmask = got;
    }
}

/* The pens of the bar's first 32 pixels, as one number per pixel summed
   with its position: changes when the stripes move. */
static ULONG signature(Object *obj)
{
    ULONG sum = 0;
    WORD x;

    for (x = 0; x < 32 && x < _mwidth(obj); x++)
        sum += (ULONG)(ReadPixel(_rp(obj), _mleft(obj) + x, _mtop(obj)) + 1) * (x + 1);
    return sum;
}

/* Whether the bar changes within about ticks/50 s. Looked at after every
   tick: the stripes repeat every 16 moves, so two samples far apart can
   match although the bar moved. */
static BOOL moves_within(Object *obj, int ticks)
{
    ULONG start = signature(obj);

    while (ticks-- > 0)
    {
        run_app(1);
        if (signature(obj) != start)
            return TRUE;
    }
    return FALSE;
}

static LONG speed_of(Object *obj)
{
    IPTR v = 0;

    get(obj, MUIA_Busy_Speed, &v);
    return (LONG)v;
}

int main(int argc, char **argv)
{
    struct Library *own = NULL;
    struct MsgPort *tport = NULL;
    struct timerequest *treq = NULL;
    Object *wnd, *b_off, *b_user, *b_ms, *b_wide;
    ULONG before, after;

    if (argc > 1)
    {
        own = OpenLibrary((CONST_STRPTR)argv[1], 0);
        if (!own)
        {
            printf("cannot open %s\n", argv[1]);
            return RETURN_FAIL;
        }
        printf("using %s\n", argv[1]);
    }

    MUIMasterBase = OpenLibrary((CONST_STRPTR)"muimaster.library", 0);
    tport = CreateMsgPort();
    treq = (struct timerequest *)CreateIORequest(tport, sizeof(*treq));
    if (!MUIMasterBase || !treq || OpenDevice((CONST_STRPTR)"timer.device", UNIT_MICROHZ, &treq->tr_node, 0))
    {
        printf("cannot open muimaster.library or timer.device\n");
        return RETURN_FAIL;
    }
    TimerBase = treq->tr_node.io_Device;

    app = ApplicationObject,
        SubWindow, wnd = WindowObject,
            MUIA_Window_Title, "busyspeed",
            WindowContents, VGroup,
                Child, b_off = BusyObject,
                    MUIA_Busy_Speed, MUIV_Busy_Speed_Off,
                    MUIA_FixWidth, 64, MUIA_FixHeight, 8,
                End,
                Child, b_user = BusyObject,
                    MUIA_Busy_Speed, MUIV_Busy_Speed_User,
                    MUIA_FixWidth, 64, MUIA_FixHeight, 8,
                End,
                Child, b_ms = BusyObject,
                    MUIA_Busy_Speed, 20,
                    MUIA_FixWidth, 64, MUIA_FixHeight, 8,
                End,
                Child, b_wide = BusyObject,
                    MUIA_Busy_Speed, MUIV_Busy_Speed_Off,
                    MUIA_FixWidth, 600, MUIA_FixHeight, 16,
                End,
            End,
        End,
    End;
    if (!app)
    {
        printf("cannot create the application (Busy.mcc missing?)\n");
        return RETURN_FAIL;
    }

    if (own)
    {
        struct Library *l;

        Forbid();
        l = (struct Library *)FindName(&SysBase->LibList, (CONST_STRPTR)"Busy.mcc");
        Permit();
        check(l == own, "the class in use is the file given");
    }

    check(speed_of(b_off) == MUIV_Busy_Speed_Off, "get after Speed_Off at creation returns 0");
    check(speed_of(b_user) == MUIV_Busy_Speed_User, "get after Speed_User at creation returns -1");
    check(speed_of(b_ms) == 20, "get after 20 ms at creation returns 20");

    DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigmask);
    set(wnd, MUIA_Window_Open, TRUE);
    run_app(10);

    check(!moves_within(b_off, 25), "a bar created with Speed_Off stays still");
    check(moves_within(b_user, 25), "a bar with Speed_User moves by itself");

    before = signature(b_off);
    DoMethod(b_off, MUIM_Busy_Move);
    after = signature(b_off);
    check(before != after, "MUIM_Busy_Move moves a bar with Speed_Off");

    set(b_off, MUIA_Busy_Speed, MUIV_Busy_Speed_User);
    check(speed_of(b_off) == MUIV_Busy_Speed_User, "get after set Speed_User returns -1");
    set(b_off, MUIA_Busy_Speed, MUIV_Busy_Speed_Off);
    run_app(5);
    check(!moves_within(b_off, 25), "a bar set back to Speed_Off stays still");

    {
        struct timeval t0, t1;
        int i;

        GetSysTime(&t0);
        for (i = 0; i < 500; i++)
            DoMethod(b_wide, MUIM_Busy_Move);
        GetSysTime(&t1);
        SubTime(&t1, &t0);
        printf("INFO: 500 moves of a %d pixel bar: %lu ms\n", (int)_mwidth(b_wide),
               (unsigned long)(t1.tv_secs * 1000 + t1.tv_micro / 1000));
    }

    MUI_DisposeObject(app);
    CloseDevice(&treq->tr_node);
    DeleteIORequest(&treq->tr_node);
    DeleteMsgPort(tport);
    CloseLibrary(MUIMasterBase);
    if (own)
        CloseLibrary(own);

    printf("%s: %d check(s) failed\n", failures ? "FAIL" : "PASS", failures);
    return failures ? RETURN_FAIL : RETURN_OK;
}
