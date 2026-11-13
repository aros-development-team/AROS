/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef PRINTTOFILE_INTERN_H
#define PRINTTOFILE_INTERN_H

#include <exec/devices.h>
#include <libraries/asl.h>

#define PRINTTOFILE_UNITS       10      /* Same as the max # of printers */

struct PrintToFileBase {
    struct Device pf_Device;

    struct PrintToFileUnit {
        struct SignalSemaphore pu_Lock;
        struct FileRequester *pu_FileReq;
        BPTR pu_File;
    } pf_Unit[PRINTTOFILE_UNITS];

    struct Library *pf_AslBase;
    struct Library *pf_DOSBase;
};

#define AslBase PrintToFileBase->pf_AslBase
#define DOSBase PrintToFileBase->pf_DOSBase

#endif /* PRINTTOFILE_INTERN_H */
