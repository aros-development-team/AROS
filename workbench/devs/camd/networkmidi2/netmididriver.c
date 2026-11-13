/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: CAMD driver for network MIDI sessions.
*/

/* Built twice: as DEVS:Midi/applemidi, for AppleMIDI (RTP-MIDI) sessions,
   and with NETMIDI_DRIVER_NETWORKMIDI2 as DEVS:Midi/networkmidi2, for
   Network MIDI 2.0 sessions, which reach CAMD translated to MIDI 1.0.
   camd.library loads each and opens its ports, <name>.in.0 to
   <name>.in.15 and <name>.out.0 to <name>.out.15. A driver holds no
   network code: the sessions are run by a service of the network stack,
   which finds the adapters made here by their public name and attaches to
   them. A port is there whether or not a peer is: what is sent to a port
   nobody serves is dropped. Modelled on
   rom/usb/classes/camdmidi/camd/poseidonusb.c. */

#include <proto/exec.h>

#include <exec/memory.h>
#include <exec/types.h>
#include <exec/interrupts.h>
#include <midi/camddevices.h>

#include "netmidi_camd.h"

int main(void)
{
    /* a CAMD driver is not run */
    return -1;
}

AROS_UFP1(BOOL, AROS_SLIB_ENTRY(Init, NetMidiCAMD, 0),
        AROS_UFPA(APTR, sysbase, A6));
AROS_UFP0(VOID, AROS_SLIB_ENTRY(Expunge, NetMidiCAMD, 0));
AROS_UFP5(struct MidiPortData *, AROS_SLIB_ENTRY(OpenPort, NetMidiCAMD, 0),
        AROS_UFPA(struct MidiDeviceData *, data, A3),
        AROS_UFPA(LONG, portnum, D0),
        AROS_UFPA(APTR, transmitfunc, A0),
        AROS_UFPA(APTR, receivefunc, A1),
        AROS_UFPA(APTR, userdata, A2));
AROS_UFP2(VOID, AROS_SLIB_ENTRY(ClosePort, NetMidiCAMD, 0),
        AROS_UFPA(struct MidiDeviceData *, data, A3),
        AROS_UFPA(LONG, portnum, D0));
AROS_UFP2(VOID, ActivateXmit,
        AROS_UFPA(APTR, userdata, A2),
        AROS_UFPA(ULONG, portnum, D0));

char name[], vers[];

/* camd.library finds this by scanning the loaded seglist for MDD_Magic;
   nothing here references it, so the compiler is told to keep it. */
static struct MidiDeviceData MidiDeviceData __attribute__((used)) =
{
  MDD_Magic,
  name,
  vers,
  1, 0,
  (APTR)AROS_SLIB_ENTRY(Init, NetMidiCAMD, 0),
  (APTR)AROS_SLIB_ENTRY(Expunge, NetMidiCAMD, 0),
  (APTR)AROS_SLIB_ENTRY(OpenPort, NetMidiCAMD, 0),
  (APTR)AROS_SLIB_ENTRY(ClosePort, NetMidiCAMD, 0),
  NETMIDI_DRIVER_PORTS,
  1
};

#ifdef NETMIDI_DRIVER_NETWORKMIDI2
char name[] = NETWORKMIDI2_DRIVER_NAME;
char vers[] = "$VER: Network MIDI 2.0 CAMD driver 1.0 (12.10.2026)";
static char adapters_name[] = NETWORKMIDI2_ADAPTERS_NAME;
#else
char name[] = APPLEMIDI_DRIVER_NAME;
char vers[] = "$VER: AppleMIDI CAMD driver 1.0 (12.10.2026)";
static char adapters_name[] = APPLEMIDI_ADAPTERS_NAME;
#endif

struct ExecBase *SysBase = NULL;

static struct NetMidiAdapters *Adapters = NULL;

/* A soft interrupt: moves what CAMD has to send into the port's ring and
   wakes the service. */
AROS_INTH1(GetFromCAMD, struct NetMidiAdapter *, port)
{
    AROS_INTFUNC_INIT

    struct Task *task;
    ULONG val, pos, next;

    if (!port->ad_IsOpen)
        return 0;

    task = port->ad_Task;
    if (!task)
    {
        /* No service. Drop what CAMD has queued, or a client sending to
           the port would wait for room forever. */
        do
        {
            val = AROS_UFC1(ULONG, port->ad_TXFunc,
                      AROS_UFCA(APTR, port->ad_UserData, A2));
        } while (val <= 0xff);
        return 0;
    }

    for (;;)
    {
        pos = port->ad_TXWritePos;
        next = (pos + 1) & (port->ad_TXBufSize - 1);
        if (next == port->ad_TXReadPos)
        {
            /* The service asks again when it has made room. */
            port->ad_TXMore = TRUE;
            break;
        }
        val = AROS_UFC1(ULONG, port->ad_TXFunc,
                  AROS_UFCA(APTR, port->ad_UserData, A2));
        if (val > 0xff)
            break;
        port->ad_TXBuffer[pos] = val;
        port->ad_TXWritePos = next;
    }

    Signal(task, port->ad_SigMask);

    return 0;

    AROS_INTFUNC_EXIT
}

AROS_UFH1(BOOL, AROS_SLIB_ENTRY(Init, NetMidiCAMD, 0),
        AROS_UFHA(APTR, sysbase, A6))
{
    AROS_USERFUNC_INIT

    struct NetMidiAdapters *adapters;
    ULONG n;

    SysBase = sysbase;
    if (Adapters)
        return TRUE;
    adapters = AllocMem(sizeof(*adapters), MEMF_PUBLIC | MEMF_CLEAR);
    if (!adapters)
        return FALSE;
    for (n = 0; n < NETMIDI_DRIVER_PORTS; n++)
    {
        struct NetMidiAdapter *port = &adapters->aa_Port[n];

        port->ad_TXBuffer = AllocMem(NETMIDI_TXBUF_SIZE, MEMF_PUBLIC);
        if (!port->ad_TXBuffer)
        {
            while (n--)
                FreeMem(adapters->aa_Port[n].ad_TXBuffer, NETMIDI_TXBUF_SIZE);
            FreeMem(adapters, sizeof(*adapters));
            return FALSE;
        }
        port->ad_TXBufSize = NETMIDI_TXBUF_SIZE;
        port->ad_PortNum = n;
        port->ad_ActivateFunc = ActivateXmit;
        port->ad_CAMDTXFunc.is_Node.ln_Type = NT_INTERRUPT;
        port->ad_CAMDTXFunc.is_Node.ln_Name = name;
        port->ad_CAMDTXFunc.is_Code = (VOID_FUNC)(APTR)GetFromCAMD;
        port->ad_CAMDTXFunc.is_Data = port;
        InitSemaphore(&port->ad_Lock);
    }
    adapters->aa_Version = NETMIDI_ADAPTERS_VERSION;
    adapters->aa_Ports = NETMIDI_DRIVER_PORTS;
    adapters->aa_Semaphore.ss_Link.ln_Name = adapters_name;
    adapters->aa_Semaphore.ss_Link.ln_Pri = 0;
    Adapters = adapters;
    AddSemaphore(&adapters->aa_Semaphore);

    return TRUE;

    AROS_USERFUNC_EXIT
}

/* camd.library is going, and this code with it. A service may still be
   attached, so the adapters are taken from the public list, marked as
   nobody's, and left where they are: nothing finds them again, no port of
   theirs is open, and their interrupt, which is code of ours, must not be
   caused again. The service looks at aa_Version, with task switching
   forbidden, before it does. */
AROS_UFH0(VOID, AROS_SLIB_ENTRY(Expunge, NetMidiCAMD, 0))
{
    AROS_USERFUNC_INIT

    ULONG n;

    if (Adapters)
    {
        Forbid();
        RemSemaphore(&Adapters->aa_Semaphore);
        Adapters->aa_Version = NETMIDI_ADAPTERS_GONE;
        for (n = 0; n < NETMIDI_DRIVER_PORTS; n++)
        {
            struct NetMidiAdapter *port = &Adapters->aa_Port[n];

            Disable();
            port->ad_IsOpen = FALSE;
            port->ad_TXMore = FALSE;
            port->ad_Task = NULL;
            Enable();
        }
        Permit();
    }
    Adapters = NULL;

    AROS_USERFUNC_EXIT
}

AROS_UFH5(struct MidiPortData *, AROS_SLIB_ENTRY(OpenPort, NetMidiCAMD, 0),
        AROS_UFHA(struct MidiDeviceData *, data, A3),
        AROS_UFHA(LONG, portnum, D0),
        AROS_UFHA(APTR, transmitfunc, A0),
        AROS_UFHA(APTR, receivefunc, A1),
        AROS_UFHA(APTR, userdata, A2))
{
    AROS_USERFUNC_INIT

    struct NetMidiAdapter *port;

    (void)data;
    if (!Adapters || portnum < 0 || portnum >= NETMIDI_DRIVER_PORTS)
        return NULL;
    port = &Adapters->aa_Port[portnum];

    ObtainSemaphore(&port->ad_Lock);
    port->ad_TXFunc = transmitfunc;
    port->ad_RXFunc = receivefunc;
    port->ad_UserData = userdata;
    port->ad_IsOpen = TRUE;
    ReleaseSemaphore(&port->ad_Lock);

    return (struct MidiPortData *)&port->ad_ActivateFunc;

    AROS_USERFUNC_EXIT
}

AROS_UFH2(VOID, AROS_SLIB_ENTRY(ClosePort, NetMidiCAMD, 0),
        AROS_UFHA(struct MidiDeviceData *, data, A3),
        AROS_UFHA(LONG, portnum, D0))
{
    AROS_USERFUNC_INIT

    struct NetMidiAdapter *port;

    (void)data;
    if (!Adapters || portnum < 0 || portnum >= NETMIDI_DRIVER_PORTS)
        return;
    port = &Adapters->aa_Port[portnum];

    /* clear of a delivery the service has in progress */
    ObtainSemaphore(&port->ad_Lock);
    port->ad_IsOpen = FALSE;
    ReleaseSemaphore(&port->ad_Lock);

    AROS_USERFUNC_EXIT
}

/* CAMD has something to send on this port. */
AROS_UFH2(VOID, ActivateXmit,
        AROS_UFHA(APTR, userdata, A2),
        AROS_UFHA(ULONG, portnum, D0))
{
    AROS_USERFUNC_INIT

    (void)userdata;
    if (Adapters && portnum < NETMIDI_DRIVER_PORTS)
        Cause(&Adapters->aa_Port[portnum].ad_CAMDTXFunc);

    AROS_USERFUNC_EXIT
}
