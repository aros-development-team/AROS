/*
 * Copyright (C) 2005-2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * The requester the stack puts up when it cannot start: error_request().
 *
 * Moved here unchanged from kern/amiga_gui.c (Stage 7, W0): main() calls
 * it when start-up fails - also when the configuration reader could not be
 * loaded - so it must not depend on a module library; the control-panel
 * glue that stays there becomes part of the administration module. The
 * requester for an interface that does not start, iface_fail_request(),
 * is only put up while a configuration is read: it is the reader's
 * (kern/netconfig_iface.c).
 */

#include "conf.h"

#include <proto/intuition.h>

#include <devices/timer.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#if !defined(__AROS__)
#include <emul/emulinterface.h>
#endif
#include <exec/libraries.h>
#include <exec/rawfmt.h>
#include <intuition/intuition.h>
#include <libraries/miamipanel.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/miamipanel.h>
#include <kern/amiga_dhcp.h>
#include <kern/amiga_gui.h>
#include <kern/amiga_log.h>
#include <sys/synch.h>
#include <stdio.h>
#include <syslog.h>

void error_request(STRPTR Text, ...)
{
    /* Networking does not require a desktop. Open Intuition only when an
     * error requester is needed; a local base avoids mandatory autoinit. */
    struct IntuitionBase *IntuitionBase =
        (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
    struct EasyStruct es = {
        sizeof(struct EasyStruct),
        0,
        "AROSTCP error",
        NULL,
        "Ok"
    };

    es.es_TextFormat = Text;
    AROS_SLOWSTACKFORMAT_PRE(Text);
    if (IntuitionBase)
        EasyRequestArgs(NULL, &es, NULL, AROS_SLOWSTACKFORMAT_ARG(Text));
    else
    {
        RawDoFmt(Text, AROS_SLOWSTACKFORMAT_ARG(Text), RAWFMTFUNC_SERIAL, NULL);
        RawPutChar('\n');
    }
    AROS_SLOWSTACKFORMAT_POST(Text);
    if (IntuitionBase)
        CloseLibrary((struct Library *)IntuitionBase);
}
