#ifndef HIDDIPMI_TRANSPORT_H
#define HIDDIPMI_TRANSPORT_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Shared definitions for the IPMI system interface transports.
*/

#include <exec/ports.h>
#include <devices/timer.h>

#include "ipmi_intern.h"

/* Largest message exchanged with the BMC, including the netfn and command bytes */
#define IPMI_MAX_MSG            256

/* Per-transaction I/O context: register access and timing */
struct ipmi_io
{
    struct HIDDIPMIData *io_Data;
    struct MsgPort      *io_Port;
    struct timerequest  *io_Timer;
    struct Device       *io_TimerBase;
    ULONG                io_EClockFreq;
};

/* A request/response pair as seen by the transport */
struct ipmi_msg
{
    UBYTE  msg_NetFn;               /* request netfn (bits 7:2) | LUN (bits 1:0) */
    UBYTE  msg_Cmd;
    const UBYTE *msg_Data;          /* request data */
    ULONG  msg_DataLen;
    UBYTE *msg_Resp;                /* completion code + response data (IPMI_MAX_MSG bytes) */
    ULONG  msg_RespLen;             /* filled by the transport */
};

BOOL  ipmi_io_open(struct ipmi_io *io, struct HIDDIPMIData *data);
void  ipmi_io_close(struct ipmi_io *io);
UBYTE ipmi_in(struct ipmi_io *io, ULONG reg);
void  ipmi_out(struct ipmi_io *io, ULONG reg, UBYTE val);
void  ipmi_udelay(struct ipmi_io *io, ULONG usec);
/* Poll register 'reg' until (value & mask) == want, giving up after 'usec' */
BOOL  ipmi_wait(struct ipmi_io *io, ULONG reg, UBYTE mask, UBYTE want, ULONG usec);

BOOL ipmi_kcs_transact(struct ipmi_io *io, struct ipmi_msg *msg);
BOOL ipmi_bt_transact(struct ipmi_io *io, struct ipmi_msg *msg);

#endif /* !HIDDIPMI_TRANSPORT_H */
