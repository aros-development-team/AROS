/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Register access and timing helpers for the IPMI transports.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/timer.h>

#if defined(__i386__) || defined(__x86_64__)
#include <asm/io.h>
#define IPMI_HAVE_PORT_IO 1
#endif

#include "ipmi_transport.h"

#define TimerBase (io->io_TimerBase)

/* Poll by spinning for this long before switching to timed sleeps */
#define IPMI_SPIN_USEC      200
/* Sleep granularity once spinning has stopped */
#define IPMI_SLEEP_USEC     100

BOOL ipmi_io_open(struct ipmi_io *io, struct HIDDIPMIData *data)
{
    io->io_Data = data;
    io->io_Port = NULL;
    io->io_Timer = NULL;
    io->io_TimerBase = NULL;
    io->io_EClockFreq = 0;

    io->io_Port = CreateMsgPort();
    if (io->io_Port)
    {
        io->io_Timer = (struct timerequest *)CreateIORequest(io->io_Port, sizeof(struct timerequest));
        if (io->io_Timer)
        {
            if (OpenDevice("timer.device", UNIT_MICROHZ, &io->io_Timer->tr_node, 0) == 0)
            {
                struct EClockVal ev;

                io->io_TimerBase = io->io_Timer->tr_node.io_Device;
                io->io_EClockFreq = ReadEClock(&ev);
                return TRUE;
            }
            DeleteIORequest(&io->io_Timer->tr_node);
            io->io_Timer = NULL;
        }
        DeleteMsgPort(io->io_Port);
        io->io_Port = NULL;
    }

    /* Without timer.device waits degrade to bounded spinning */
    D(bug("[IPMI] %s: timer.device unavailable, using spin waits\n", __func__));
    return TRUE;
}

void ipmi_io_close(struct ipmi_io *io)
{
    if (io->io_Timer)
    {
        CloseDevice(&io->io_Timer->tr_node);
        DeleteIORequest(&io->io_Timer->tr_node);
        io->io_Timer = NULL;
        io->io_TimerBase = NULL;
    }
    if (io->io_Port)
    {
        DeleteMsgPort(io->io_Port);
        io->io_Port = NULL;
    }
}

UBYTE ipmi_in(struct ipmi_io *io, ULONG reg)
{
    IPTR addr = io->io_Data->ipmi_BaseAddress + reg * io->io_Data->ipmi_RegStride;

    if (io->io_Data->ipmi_AddressSpace == vHidd_IPMI_AddressSpace_IO)
    {
#if IPMI_HAVE_PORT_IO
        return inb(addr);
#else
        return 0xFF;
#endif
    }
    return *(volatile UBYTE *)addr;
}

void ipmi_out(struct ipmi_io *io, ULONG reg, UBYTE val)
{
    IPTR addr = io->io_Data->ipmi_BaseAddress + reg * io->io_Data->ipmi_RegStride;

    if (io->io_Data->ipmi_AddressSpace == vHidd_IPMI_AddressSpace_IO)
    {
#if IPMI_HAVE_PORT_IO
        outb(val, addr);
#endif
        return;
    }
    *(volatile UBYTE *)addr = val;
}

static inline UQUAD ipmi_eclock(struct ipmi_io *io)
{
    struct EClockVal ev;

    ReadEClock(&ev);
    return ((UQUAD)ev.ev_hi << 32) | ev.ev_lo;
}

void ipmi_udelay(struct ipmi_io *io, ULONG usec)
{
    if (io->io_Timer)
    {
        io->io_Timer->tr_node.io_Command = TR_ADDREQUEST;
        io->io_Timer->tr_time.tv_secs = usec / 1000000;
        io->io_Timer->tr_time.tv_micro = usec % 1000000;
        DoIO(&io->io_Timer->tr_node);
    }
    else
    {
        volatile ULONG n = usec * 50;
        while (n--)
            ;
    }
}

BOOL ipmi_wait(struct ipmi_io *io, ULONG reg, UBYTE mask, UBYTE want, ULONG usec)
{
    if (io->io_TimerBase)
    {
        UQUAD start = ipmi_eclock(io);
        UQUAD limit = ((UQUAD)usec * io->io_EClockFreq) / 1000000;
        UQUAD spin = ((UQUAD)IPMI_SPIN_USEC * io->io_EClockFreq) / 1000000;

        for (;;)
        {
            UQUAD elapsed;

            if ((ipmi_in(io, reg) & mask) == want)
                return TRUE;
            elapsed = ipmi_eclock(io) - start;
            if (elapsed >= limit)
                return FALSE;
            if (elapsed >= spin)
                ipmi_udelay(io, IPMI_SLEEP_USEC);
        }
    }
    else
    {
        ULONG n = usec / IPMI_SLEEP_USEC + 1;

        while (n--)
        {
            if ((ipmi_in(io, reg) & mask) == want)
                return TRUE;
            ipmi_udelay(io, IPMI_SLEEP_USEC);
        }
    }

    return (ipmi_in(io, reg) & mask) == want;
}
