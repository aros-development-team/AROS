/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* What the CAMD drivers DEVS:Midi/applemidi and DEVS:Midi/networkmidi2
   and the service that runs the sessions share: one adapter per CAMD port.

   The driver holds no network code and the service never opens
   camd.library. camd.library loads the driver once and never unloads it,
   so a driver is the one that lasts: it owns the adapters and makes them
   public under a name. The service comes and goes with the network; while
   it runs it attaches to the adapters, takes what CAMD clients send from a
   port's ring, and hands CAMD what arrived for the port. */

#ifndef NETMIDI_CAMD_H
#define NETMIDI_CAMD_H

#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <exec/tasks.h>
#include <exec/types.h>

/* Two drivers are built from one source, one to a transport, so that a
   port's name says what kind of peer is behind it: AppleMIDI (RTP-MIDI)
   sessions, and Network MIDI 2.0 sessions. Each makes its adapters public
   as a semaphore of the name given here. */
#define APPLEMIDI_DRIVER_NAME       "applemidi"
#define APPLEMIDI_ADAPTERS_NAME     "applemidi.driver"
#define NETWORKMIDI2_DRIVER_NAME    "networkmidi2"
#define NETWORKMIDI2_ADAPTERS_NAME  "networkmidi2.driver"

#define NETMIDI_DRIVER_PORTS         16
/* What a CAMD client sent and the service has not taken yet. The driver
   asks CAMD for more when the service has made room, so a small ring only
   means more turns for a long SysEx message; sixteen ports of each of two
   drivers have one each, whether used or not. */
#define NETMIDI_TXBUF_SIZE    1024    /* a power of two */

/* The driver camd.library has in memory may be older or newer than the
   service that finds it. The service attaches only to adapters of the
   layout it was built with. Count this up with any change to the two
   structures below; aa_Semaphore and aa_Version keep their places. */
#ifndef NETMIDI_ADAPTERS_VERSION
#define NETMIDI_ADAPTERS_VERSION  1
#endif
/* What a driver that camd.library has let go of leaves in aa_Version: its
   adapters are still there, for a service that may be attached, and are
   nobody's any more. */
#define NETMIDI_ADAPTERS_GONE     0

struct NetMidiAdapter
{
    APTR                    ad_ActivateFunc;  /* struct MidiPortData */
    BOOL                    ad_IsOpen;        /* TRUE while CAMD has the port open */
    struct Interrupt        ad_CAMDTXFunc;    /* soft interrupt in the driver: fills the ring */
    ULONG                   ad_PortNum;
    APTR                    ad_TXFunc;        /* CAMD's transmit function -- filled by OpenPort() */
    APTR                    ad_RXFunc;        /* CAMD's receive function -- filled by OpenPort() */
    APTR                    ad_UserData;      /* CAMD's user data -- filled by OpenPort() */
    UBYTE                  *ad_TXBuffer;      /* Ring of MIDI bytes CAMD clients sent */
    ULONG                   ad_TXBufSize;
    volatile ULONG          ad_TXWritePos;    /* Written by the TX function only */
    volatile ULONG          ad_TXReadPos;     /* Written by the service only */
    volatile BOOL           ad_TXMore;        /* The ring was full and CAMD has more */
    struct Task            *ad_Task;          /* The service, signalled when the ring has data */
    ULONG                   ad_SigMask;       /* with this */
    struct SignalSemaphore  ad_Lock;          /* Closing, and delivery to CAMD */
};

struct NetMidiAdapters
{
    struct SignalSemaphore  aa_Semaphore;     /* public, named APPLEMIDI_ADAPTERS_NAME */
    ULONG                   aa_Version;       /* NETMIDI_ADAPTERS_VERSION of the driver */
    ULONG                   aa_Ports;
    struct NetMidiAdapter aa_Port[NETMIDI_DRIVER_PORTS];
};

#endif
