/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved.
*/

#ifndef _SERIALLOGGER_INTERN_H
#define _SERIALLOGGER_INTERN_H

#include <exec/types.h>
#include <exec/semaphores.h>
#include <exec/libraries.h>
#include <exec/ports.h>

struct SerialLogResBase
{
	struct Library          selrb_Lib;
    struct Library          *selrb_DosBase;
    struct Task             *selrb_Task;
    struct MsgPort          *selrb_Port;
    APTR                    selrb_LogResBase;
    APTR                    selrb_Provider;
    ULONG                   selrb_Mask;
};

#endif /* _SERIALLOGGER_INTERN_H */
