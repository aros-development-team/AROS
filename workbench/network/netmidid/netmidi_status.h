/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* What the network MIDI service says about itself to whoever asks: the
   peer behind each port of its two drivers, and what has passed. The
   service makes it public as a semaphore of the name below while it runs,
   and writes it about once a second. A reader finds the semaphore with
   task switching forbidden, obtains it shared, copies what it wants and
   lets go: the service obtains it exclusively to write, and before it
   takes it away. */

#ifndef NETMIDI_STATUS_H
#define NETMIDI_STATUS_H

#include <exec/semaphores.h>
#include <exec/types.h>

#define NETMIDI_STATUS_NAME     "netmidi.status"
#define NETMIDI_STATUS_VERSION  1
#define NETMIDI_STATUS_PORTS    32      /* sixteen of each driver */

/* The first sixteen are the ports of applemidi, the rest of networkmidi2. */
struct NetMidiPortStatus
{
    UBYTE   ps_Present;         /* a peer has the port */
    UBYTE   ps_Pad[3];
    char    ps_Name[64];        /* as the peer calls its session or endpoint */
    ULONG   ps_PacketsIn;       /* RTP packets, or UMP Data commands */
    ULONG   ps_PacketsOut;
    ULONG   ps_PacketsLost;     /* gaps in what came */
    ULONG   ps_MessagesIn;      /* handed to CAMD */
    ULONG   ps_MessagesOut;     /* taken from CAMD and sent */
    ULONG   ps_Timed;           /* messages that came with a time to keep */
    ULONG   ps_Late;            /* of those, the ones already past it */
    ULONG   ps_SysExDropped;    /* too long, or broken */
    ULONG   ps_JournalsOmitted; /* packets sent without their journal */
    ULONG   ps_Undefined;       /* undefined statuses stepped over */
};

struct NetMidiStatus
{
    struct SignalSemaphore   ns_Semaphore;   /* public, named NETMIDI_STATUS_NAME */
    ULONG                    ns_Version;
    char                     ns_Session[64]; /* the name we go by */
    ULONG                    ns_Refused;     /* invitations turned away */
    struct NetMidiPortStatus ns_Port[NETMIDI_STATUS_PORTS];
};

#endif
