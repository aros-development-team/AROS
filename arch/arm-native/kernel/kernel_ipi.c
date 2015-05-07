/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <inttypes.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <aros/types/spinlock_s.h>
#include <aros/arm/cpucontext.h>

#include "kernel_base.h"

#include "etask.h"

#include "kernel_cpu.h"
#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_scheduler.h"

#if defined(__AROSEXEC_SMP__)

#define D(x)

#include "kernel_ipi.h"

void handle_ipi(uint32_t ipi, uint32_t ipi_data)
{
    D(bug("[KRN:IPI] %s: IPI Msg %08x Param  %08x\n", __PRETTY_FUNCTION__, ipi,  ipi_data));
    switch (ipi)
    {
        case IPI_CAUSE:
        {
            D(bug("[KRN:IPI] IPI_CAUSE:\n"));
            break;
        }
        case IPI_DISPATCH:
        {
            D(bug("[KRN:IPI] IPI_DISPATCH:\n"));
            break;
        }
        case IPI_SWITCH:
        {
            D(bug("[KRN:IPI] IPI_SWITCH:\n"));
            break;
        }
        case IPI_SCHEDULE:
        {
            D(bug("[KRN:IPI] IPI_SCHEDULE:\n"));
            break;
        }
        case IPI_CLI:
        {
            D(bug("[KRN:IPI] IPI_CLI:\n"));
            break;
        }
        case IPI_STI:
        {
            D(bug("[KRN:IPI] IPI_STI:\n"));
            break;
        }
        case IPI_REBOOT:
        {
            D(bug("[KRN:IPI] IPI_REBOOT:\n"));
            break;
        }
        case IPI_ADDTASK:
        {
            D(bug("[KRN:IPI] IPI_ADDTASK:\n"));
            break;
        }
        case IPI_REMTASK:
        {
            D(bug("[KRN:IPI] IPI_REMTASK:\n"));
            break;
        }
    }
}
#endif