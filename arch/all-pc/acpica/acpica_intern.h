/*
 * Copyright (C) 2012-2023, The AROS Development Team
 * All rights reserved.
 */

#ifndef _ACPICA_INTERN_H
#define _ACPICA_INTERN_H

#include "acpi.h"
#include "accommon.h"
#include "amlcode.h"
#include "acparser.h"
#include "acdebug.h"
#include "acmacros.h"

#define ACPICAB_ENABLED         0
#define ACPICAF_ENABLED         (1 << ACPICAB_ENABLED)
#define ACPICAB_TABLEINIT       1
#define ACPICAF_TABLEINIT       (1 << ACPICAB_TABLEINIT)
#define ACPICAB_FULLINIT        2
#define ACPICAF_FULLINIT        (1 << ACPICAB_FULLINIT)
#define ACPICAB_TIMER           7
#define ACPICAF_TIMER           (1 << ACPICAB_TIMER)

struct ACPICABase {
    struct Library              ab_Lib;
    struct MsgPort              *ab_TimeMsgPort;
    struct timerequest          *ab_TimeRequest;
    struct Library              *ab_TimerBase;
    struct MemEntry             ab_MCFG;
    struct Interrupt	        ab_ResetInt;
    struct MinList              ab_IntHandlers;
    ACPI_PHYSICAL_ADDRESS       ab_RootPointer;
    UWORD                       ab_Flags;
};

#endif //_ACPICA_INTERN_H
