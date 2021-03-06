/*
    Copyright (C) 2015, The AROS Development Team. All rights reserved.
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

#define D(x) x

#include "kernel_ipi.h"

void handle_ipi(uint32_t ipi, uint32_t ipi_data)
{
    int cpu = GetCPUNumber();
    uint32_t ipi_src = (ipi >> 28) & 0xF;
    uint32_t ipi_msg = ipi & ~(0xF << 28);

    D(bug("[Kernel:IPI] %s: Core #%02d IPI Msg %08x:%08x from Core #%02d\n",
        __PRETTY_FUNCTION__, cpu, ipi_msg,  ipi_data, ipi_src));
    switch (ipi_msg)
    {
        case IPI_CAUSE:
        {
            D(bug("[Kernel:IPI] IPI_CAUSE:\n"));
            break;
        }
        case IPI_DISPATCH:
        {
            D(bug("[Kernel:IPI] IPI_DISPATCH:\n"));
            break;
        }
        case IPI_SWITCH:
        {
            D(bug("[Kernel:IPI] IPI_SWITCH:\n"));
            break;
        }
        case IPI_SCHEDULE:
        {
            D(bug("[Kernel:IPI] IPI_SCHEDULE:\n"));
            break;
        }
        case IPI_CLI:
        {
            D(bug("[Kernel:IPI] IPI_CLI:\n"));
            break;
        }
        case IPI_STI:
        {
            D(bug("[Kernel:IPI] IPI_STI:\n"));
            break;
        }
        case IPI_REBOOT:
        {
            D(bug("[Kernel:IPI] IPI_REBOOT:\n"));
            break;
        }
        case IPI_ADDTASK:
        {
            D(bug("[Kernel:IPI] IPI_ADDTASK:\n"));
            break;
        }
        case IPI_REMTASK:
        {
            D(bug("[Kernel:IPI] IPI_REMTASK:\n"));
            break;
        }
    }
}
#endif
