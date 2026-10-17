#ifndef FIDOSIM_INTERN_H
#define FIDOSIM_INTERN_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: fidosim.device - a simulated FIDO U2F security key with the
          interface of fido.device, for testing fido.secplugin and the
          login's challenge-token path without hardware.
*/

#include <exec/devices.h>
#include <exec/semaphores.h>
#include <exec/lists.h>

#define SIM_REPORTSIZE      64
#define SIM_MAXMSG          (57 + 128 * 59)

struct SimPacket
{
    struct MinNode      Node;
    UBYTE               Data[SIM_REPORTSIZE];
};

struct FidoSimBase
{
    struct Device           dev;
    struct SignalSemaphore  Sem;
    struct MinList          Reads;          /* CMD_READs waiting for a report   */
    struct MinList          Out;            /* reports waiting for a CMD_READ   */
    ULONG                   NextCID;
    ULONG                   Counter;        /* the key's signature counter      */
    ULONG                   RegCount;       /* registrations made               */
    BOOL                    Touched;        /* the one "touch" given for an op  */
    ULONG                   Rng;            /* a toy RNG for mbedTLS            */
    UBYTE                   Seed[32];
    /* the message being received */
    ULONG                   CID;
    UBYTE                   Cmd;
    UBYTE                   Seq;
    ULONG                   Expect;
    ULONG                   Got;
    UBYTE                   Msg[SIM_MAXMSG];
    UBYTE                   Reply[SIM_MAXMSG];
};

#endif /* FIDOSIM_INTERN_H */
