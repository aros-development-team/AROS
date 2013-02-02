/*
 * Copyright (C) 2013, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef CD_INTERN_H
#define CD_INTERN_H

#include <exec/devices.h>
#include <exec/io.h>
#include <exec/semaphores.h>

#include <devices/timer.h>

#include <dos/filehandler.h>

#include LC_LIBDEFS_FILE

struct cdBase {
    struct Device cb_Device;
    struct List cb_Units;
    struct SignalSemaphore cb_UnitsLock;
    ULONG       cb_MaxUnit;

    struct MsgPort  cb_TimerPort;
    struct timerequest cb_TimerRequest;
};

struct cdUnitOps {
    CONST_STRPTR  uo_Name;
    LONG        (*uo_DoIO)(struct IOStdReq *io, APTR priv);
    VOID        (*uo_Expunge)(APTR priv);
};

LONG cdAddUnit(LIBBASETYPE *cb, const struct cdUnitOps *ops, APTR priv, const struct DosEnvec *de);
VOID cdDelayMS(LIBBASETYPE *cb, ULONG timeout_ms);

#define IOF_ABORT       (1 << 7)

#endif /* CD_INTERN_H */
