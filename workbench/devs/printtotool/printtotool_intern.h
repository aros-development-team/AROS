/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef PRINTTOTOOL_INTERN_H
#define PRINTTOTOOL_INTERN_H

#include <exec/devices.h>
#include <libraries/asl.h>

#define PRINTTOTOOL_UNITS       10      /* Same as the max # of printers */

struct PrintToToolBase {
    struct Device pt_Device;

    struct PrintToToolUnit {
        struct SignalSemaphore pu_Lock;
        struct FileRequester *pu_FileReq;
        BPTR pu_File;
    } pt_Unit[PRINTTOTOOL_UNITS];

    struct Library *pt_AslBase;
    struct Library *pt_DOSBase;
};

#define AslBase PrintToToolBase->pt_AslBase
#define DOSBase PrintToToolBase->pt_DOSBase

#endif /* PRINTTOTOOL_INTERN_H */
