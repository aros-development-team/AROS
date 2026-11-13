/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Native test: inject mouse motion and the three buttons on port 1 while
    it runs. Port 2 remains a CD32 pad. Check the public ReadJoyPort result
    against the stable hardware counters after the input sequence finishes.
*/
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/lowlevel.h>
#include <libraries/lowlevel.h>
#include <hardware/custom.h>

static ULONG failures;
struct Library *LowLevelBase;

static void check(BOOL okay, CONST_STRPTR message)
{
    if (!okay)
    {
        bug("LOWLEVEL MOUSE FAIL: %s\n", message);
        failures++;
    }
}

int main(void)
{
    volatile struct Custom *custom = (struct Custom *)0xdff000;
    ULONG initial, buttons = 0, state = 0;
    BOOL mouse_seen = FALSE;
    ULONG i;

    LowLevelBase = OpenLibrary("lowlevel.library", 40);
    if (!LowLevelBase)
    {
        check(FALSE, "open lowlevel.library");
        goto end;
    }
    initial = ReadJoyPort(0);

    bug("LOWLEVEL MOUSE READY: initial type %08lx\n", initial & JP_TYPE_MASK);
    for (i = 0; i < 400; i++)
    {
        state = ReadJoyPort(0);
        if ((state & JP_TYPE_MASK) == JP_TYPE_MOUSE)
        {
            mouse_seen = TRUE;
            buttons |= state & JP_BUTTON_MASK;
        }
        Delay(1);
    }
    check(mouse_seen, "mouse detected after movement from rest");
    check((state & JP_TYPE_MASK) == JP_TYPE_MOUSE, "mouse type persists at rest");
    check((state & JP_MOUSE_MASK) == custom->joy0dat, "absolute byte counters preserved");
    check((buttons & (JPF_BUTTON_RED | JPF_BUTTON_BLUE | JPF_BUTTON_PLAY)) ==
          (JPF_BUTTON_RED | JPF_BUTTON_BLUE | JPF_BUTTON_PLAY),
          "left, right and middle mouse buttons reported");
    check(!(state & JP_BUTTON_MASK), "released mouse buttons clear");
    check((ReadJoyPort(1) & JP_TYPE_MASK) == JP_TYPE_GAMECTLR,
          "other port retains CD32 pad classification");
end:
    if (LowLevelBase)
        CloseLibrary(LowLevelBase);
    bug("LOWLEVEL MOUSE TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
