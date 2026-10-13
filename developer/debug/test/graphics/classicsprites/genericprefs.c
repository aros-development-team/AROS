/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include "intuition_preferences.h"

/* Check the platform-neutral defaults even when compiled with an m68k
 * compiler. Native beam origins must come from the target override. */
int main(void)
{
    ULONG failures = 0;

    if (IntuitionDefaultPreferences.ViewInitX != 0)
    {
        bug("GENERIC PREFS FAIL: default horizontal origin is platform-specific\n");
        failures++;
    }
    if (IntuitionDefaultPreferences.ViewInitY != 0)
    {
        bug("GENERIC PREFS FAIL: default vertical origin is platform-specific\n");
        failures++;
    }
    bug("GENERIC PREFS TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
