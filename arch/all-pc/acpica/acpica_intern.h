/*
 * Copyright (C) 2012-2018, The AROS Development Team
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

#define ACPICAB_ENABLED 0
#define ACPICAF_ENABLED (1 << ACPICAB_ENABLED)
#define ACPICAB_TIMER 1
#define ACPICAF_TIMER (1 << ACPICAB_TIMER)

struct ACPICABase {
    struct Library ab_Lib;
    UWORD ab_Flags;
    struct MsgPort *ab_TimeMsgPort;
    struct timerequest *ab_TimeRequest;
    struct Library *ab_TimerBase;

    ACPI_MCFG_ALLOCATION *ab_PCI;
    int ab_PCIs;

    ACPI_PHYSICAL_ADDRESS ab_RootPointer;
};

#endif //_ACPICA_INTERN_H
