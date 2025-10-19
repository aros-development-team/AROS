#ifndef MUISCREEN_INTERN_H
#define MUISCREEN_INTERN_H

/*
    Copyright © 2009-2025, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <libraries/muiscreen.h>

#include <aros/libcall.h>
#include LC_LIBDEFS_FILE

struct MUIScreenBase_intern
{
    struct Library          lib;
    const char              *muisb_def;
    struct Task             *muisb_closeTask;
    struct MsgPort          *muisb_taskMsgPort;
    struct SignalSemaphore  muisb_acLock;
    struct List             muisb_autocScreens;
    struct List             clients;
};

#endif
