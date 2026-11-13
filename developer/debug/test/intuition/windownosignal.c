/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Windows of a task that has no free signal bit.

    Build with make test-intuition, also included by make test.
    Run windownosignal from a native Shell. It opens two small windows
    without IDCMP on the default public screen and closes them again.

    OpenWindow() and CloseWindow() hand their work to Intuition's input
    handler and wait for it. They did so through a message port from
    CreateMsgPort(), which needs a free signal bit of the calling task. A
    task that had allocated all of its signal bits got no port: the work
    was never handed over, and the caller was not told. CloseWindow()
    returned and the window stayed open, and OpenWindow() failed.

    The program opens a window while it still has free signal bits. Then
    it allocates every signal bit that is left, closes that window, opens
    a second one and closes it too, and gives the bits back. A window is
    looked for in the window list of the screen under LockIBase(), by its
    address alone: a closed window is never read. The screen stays locked
    by the program for the whole run.

    The first line names the version of the intuition.library that the
    program has opened. Nothing is printed while the task has no free
    signal bit. A window that is still open at the end is closed again
    once the bits are free, so that no window is left behind on a library
    with the fault.

    Success prints:
      windownosignal: 8 checks, 0 failed
    and returns zero. A regression returns 10.
*/

#include <exec/tasks.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include <stdio.h>

struct IntuitionBase *IntuitionBase;

static unsigned checks, failed;

static void check(BOOL ok, const char *what)
{
    checks++;
    if (!ok)
        failed++;
    printf("%s: %s\n", ok ? "ok" : "FAILED", what);
}

/* Whether window is in the window list of screen. Only addresses are
   compared, so window may be one that has been closed. */
static BOOL listed(struct Screen *screen, struct Window *window)
{
    struct Window *w;
    BOOL found = FALSE;
    ULONG lock = LockIBase(0);

    for (w = screen->FirstWindow; w; w = w->NextWindow)
    {
        if (w == window)
        {
            found = TRUE;
            break;
        }
    }
    UnlockIBase(lock);
    return found;
}

static struct Window *open_window(struct Screen *screen, WORD left)
{
    return OpenWindowTags(NULL,
                          WA_PubScreen, (IPTR)screen,
                          WA_Left, left, WA_Top, 40, WA_Width, 120, WA_Height, 40,
                          WA_IDCMP, 0,
                          WA_Activate, FALSE,
                          TAG_DONE);
}

int main(void)
{
    struct Task *me = FindTask(NULL);
    struct Screen *screen = NULL;
    struct Window *first = NULL, *second = NULL;
    BYTE taken[32];
    ULONG before, during, after;
    BOOL first_listed, first_closed, second_opened = FALSE, second_closed = FALSE;
    int count = 0, i;

    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39);
    if (IntuitionBase)
        screen = LockPubScreen(NULL);
    if (!screen)
    {
        printf("windownosignal: no intuition.library or no public screen\n");
        if (IntuitionBase)
            CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }

    printf("intuition.library %u.%u\n", (unsigned)IntuitionBase->LibNode.lib_Version,
           (unsigned)IntuitionBase->LibNode.lib_Revision);

    first = open_window(screen, 40);
    first_listed = first && listed(screen, first);
    check(first_listed, "a window opens and is in its screen's list");

    if (first_listed)
    {
        /* every signal bit that this task has left, and only those */
        before = me->tc_SigAlloc;
        while (count < 32 && (taken[count] = AllocSignal(-1)) != -1)
            count++;
        during = me->tc_SigAlloc;

        CloseWindow(first);
        first_closed = !listed(screen, first);

        second = open_window(screen, 200);
        second_opened = second && listed(screen, second);
        if (second_opened)
        {
            CloseWindow(second);
            second_closed = !listed(screen, second);
        }

        for (i = 0; i < count; i++)
            FreeSignal(taken[i]);
        after = me->tc_SigAlloc;

        check(count > 0, "the task had free signal bits to take");
        check(during == 0xFFFFFFFF, "no signal bit of the task was free");
        check(first_closed, "CloseWindow() without a free signal bit closes the window");
        check(second_opened, "OpenWindow() without a free signal bit opens a window");
        check(second_closed, "and CloseWindow() closes that one too");
        check(after == before, "the task has its signal bits back");
        printf("signal bits: %08lx before, %d taken, %08lx with none free, %08lx after\n",
               (unsigned long)before, count, (unsigned long)during, (unsigned long)after);

        /* With its bits back the task can close what was left open. */
        if (!first_closed)
            CloseWindow(first);
        if (second_opened && !second_closed)
            CloseWindow(second);
        check(!listed(screen, first) && !(second && listed(screen, second)),
              "no window of the program is left on the screen");
    }
    else if (first)
    {
        CloseWindow(first);
    }

    UnlockPubScreen(NULL, screen);
    CloseLibrary((struct Library *)IntuitionBase);
    printf("windownosignal: %u checks, %u failed\n", checks, failed);
    return failed ? 10 : 0;
}
