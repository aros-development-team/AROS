/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <dos/var.h>
#include <proto/dos.h>

#include "cgxvideo_intern.h"

/*
 * The null backend keeps the source buffer in system RAM and displays
 * nothing. It exists to exercise the handle state machine, so it is only
 * used when the caller has set CGXVideoNullBackend; otherwise a screen
 * without overlay support keeps failing with VOERR_INVSCRMODE.
 */
BOOL cgxv_NullBackend(void)
{
    TEXT buf[2];

    return GetVar("CGXVideoNullBackend", buf, sizeof(buf), 0) >= 0;
}
