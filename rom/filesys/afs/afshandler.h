#ifndef AFS_HANDLER_H
#define AFS_HANDLER_H

/*
    Copyright © 1995-2026, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/devices.h>
#include <devices/timer.h>

struct AFSBase
{
    struct Library ab_Lib;
    BPTR   ab_SegList;		/* Pointer to SegList trampoline */
    struct Volume *volume;
    struct DosLibrary *dosbase;
    struct Library *utilitybase;
    struct MsgPort *timer_mp;
    struct MsgPort port;	/* sigtask and sigbit for changeint */
    struct List device_list;	/* list of mounted devices (struct Volume) */
    struct timerequest *timer_request;
    ULONG timer_flags;

    /* multi-user support (afs_security.c) */
    struct Library *ab_SecBase;     /* security.library, if resident              */
    BOOL   ab_SecChecked;           /* tried to open it                           */
    BOOL   ab_SecActive;            /* enforcing for the packet being processed   */
    APTR   ab_CurOwner;             /* struct secExtOwner * of the packet's sender */
    ULONG  ab_CurOwnerId;           /* uid<<16 | gid of the sender                */
    LONG   ab_CurDefProt;           /* default protection bits of the sender      */
    struct MsgPort *ab_CurPort;     /* our port (filesystem context)              */
};

#define TIMER_ACTIVE  0x00000001
#define TIMER_RESTART 0x00000002

#endif
