/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* The CAMD driver of btmidi.class. The class embeds this object and writes
   it to DEVS:Midi/btmidi; camd.library loads it from there and opens its
   ports, btmidi.0 to btmidi.15. It holds no Bluetooth code: a port's bytes
   go to and come from the class through its adapter. Modelled on
   rom/usb/classes/camdmidi/camd/poseidonusb.c. */

#include <proto/exec.h>
#include <proto/btmidi.h>

#include <exec/types.h>
#include <exec/interrupts.h>
#include <midi/camddevices.h>

#include "btmidi_camd.h"

AROS_UFP1(BOOL, AROS_SLIB_ENTRY(Init, BTMidiCAMD, 0),
        AROS_UFPA(APTR, sysbase, A6));
AROS_UFP0(VOID, AROS_SLIB_ENTRY(Expunge, BTMidiCAMD, 0));
AROS_UFP5(struct MidiPortData *, AROS_SLIB_ENTRY(OpenPort, BTMidiCAMD, 0),
        AROS_UFPA(struct MidiDeviceData *, data, A3),
        AROS_UFPA(LONG, portnum, D0),
        AROS_UFPA(APTR, transmitfunc, A0),
        AROS_UFPA(APTR, receivefunc, A1),
        AROS_UFPA(APTR, userdata, A2));
AROS_UFP2(VOID, AROS_SLIB_ENTRY(ClosePort, BTMidiCAMD, 0),
        AROS_UFPA(struct MidiDeviceData *, data, A3),
        AROS_UFPA(LONG, portnum, D0));
AROS_UFP2(VOID, ActivateXmit,
        AROS_UFPA(APTR, userdata, A2),
        AROS_UFPA(ULONG, portnum, D0));

char name[], vers[];

/*** Identification data must follow directly *********************************/

/* Nothing here references it -- camd.library finds it by scanning the loaded
 * seglist for MDD_Magic -- so the compiler has to be told to keep it. */
static struct MidiDeviceData MidiDeviceData __attribute__((used)) =
{
  MDD_Magic,
  name,
  vers,
  1, 0,
  (APTR)AROS_SLIB_ENTRY(Init, BTMidiCAMD, 0),
  (APTR)AROS_SLIB_ENTRY(Expunge, BTMidiCAMD, 0),
  (APTR)AROS_SLIB_ENTRY(OpenPort, BTMidiCAMD, 0),
  (APTR)AROS_SLIB_ENTRY(ClosePort, BTMidiCAMD, 0),
  BTMIDI_PORTS,
  1
};

char name[] = BTMIDI_DRIVER_NAME;
char vers[] = "$VER: Bluetooth LE MIDI btmidi.class driver 1.0 (11.10.2026)";

struct ExecBase *SysBase = NULL;
struct Library *BTMidiBase = NULL;

static struct BTMidiAdapter *CAMDPortBases[BTMIDI_PORTS] = { NULL };

AROS_UFH1(BOOL, AROS_SLIB_ENTRY(Init, BTMidiCAMD, 0),
        AROS_UFHA(APTR, sysbase, A6))
{
    AROS_USERFUNC_INIT

    SysBase = sysbase;
    /* bluetooth.library has it in memory once the stack runs; before that
       it starts from its file and offers the peripheral role on its own. */
    BTMidiBase = OpenLibrary("btmidi.class", 0);
    if (!BTMidiBase)
        BTMidiBase = OpenLibrary("SYS:Classes/Bluetooth/btmidi.class", 0);

    return (BTMidiBase != NULL);

    AROS_USERFUNC_EXIT
}

AROS_UFH0(VOID, AROS_SLIB_ENTRY(Expunge, BTMidiCAMD, 0))
{
    AROS_USERFUNC_INIT

    if (BTMidiBase)
        CloseLibrary(BTMidiBase);
    BTMidiBase = NULL;

    AROS_USERFUNC_EXIT
}

/* Called by the class, in the task that owns the port, with what a BLE MIDI
   packet held. CAMD parses the bytes itself: running status, message
   lengths and SysEx. */
AROS_UFH3S(void, SendToCAMD,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct BTMidiRXData *, data, A2),
    AROS_UFHA(APTR, unused, A1))
{
    AROS_USERFUNC_INIT

    struct BTMidiAdapter *port = (struct BTMidiAdapter *)hook->h_Data;
    UBYTE *msg = data->rd_Bytes;
    ULONG len = data->rd_Length;

    while (len > 0)
    {
        AROS_UFC2NR(void, port->ba_RXFunc,
            AROS_UFCA(UWORD, *msg++, D0),
            AROS_UFCA(APTR, port->ba_UserData, A2));
        len--;
    }

    AROS_USERFUNC_EXIT
}

/* A soft interrupt: moves what CAMD has to send into the port's ring and
   wakes the task that turns it into BLE MIDI packets. */
AROS_INTH1(GetFromCAMD, struct BTMidiAdapter *, port)
{
    AROS_INTFUNC_INIT

    struct Task *task;
    ULONG val, pos, next;

    if (!port->ba_IsOpen)
        return 0;

    task = port->ba_Task;
    if (!task)
    {
        /* The device's binding is gone. Drop what CAMD has queued, or a
           client sending to the port would wait for room forever. */
        do
        {
            val = AROS_UFC1(ULONG, port->ba_TXFunc,
                      AROS_UFCA(APTR, port->ba_UserData, A2));
        } while (val <= 0xff);
        return 0;
    }

    for (;;)
    {
        pos = port->ba_TXWritePos;
        next = (pos + 1) & (port->ba_TXBufSize - 1);
        if (next == port->ba_TXReadPos)
        {
            /* The task asks again when it has made room. */
            port->ba_TXMore = TRUE;
            break;
        }
        val = AROS_UFC1(ULONG, port->ba_TXFunc,
                  AROS_UFCA(APTR, port->ba_UserData, A2));
        if (val > 0xff)
            break;
        port->ba_TXBuffer[pos] = val;
        port->ba_TXWritePos = next;
    }

    Signal(task, port->ba_SigMask);

    return 0;

    AROS_INTFUNC_EXIT
}

AROS_UFH5(struct MidiPortData *, AROS_SLIB_ENTRY(OpenPort, BTMidiCAMD, 0),
        AROS_UFHA(struct MidiDeviceData *, data, A3),
        AROS_UFHA(LONG, portnum, D0),
        AROS_UFHA(APTR, transmitfunc, A0),
        AROS_UFHA(APTR, receivefunc, A1),
        AROS_UFHA(APTR, userdata, A2))
{
    AROS_USERFUNC_INIT

    struct BTMidiAdapter *port;

    if (portnum < 0 || portnum >= BTMIDI_PORTS)
        return NULL;

    /* Fails for a port no device is bound to. */
    port = btmidiCAMDOpenPort(transmitfunc, receivefunc, userdata, name, portnum);
    if (!port)
        return NULL;
    /* a class newer or older than this driver: nothing in the adapter
       beyond its version can be relied on */
    if (port->ba_Version != BTMIDI_ADAPTER_VERSION)
        return NULL;

    port->ba_ActivateFunc = ActivateXmit;
    port->ba_CAMDRXFunc.h_Entry = (HOOKFUNC)SendToCAMD;
    port->ba_CAMDRXFunc.h_Data = port;
    port->ba_CAMDTXFunc.is_Node.ln_Type = NT_INTERRUPT;
    port->ba_CAMDTXFunc.is_Node.ln_Name = name;
    port->ba_CAMDTXFunc.is_Code = (VOID_FUNC)GetFromCAMD;
    port->ba_CAMDTXFunc.is_Data = port;
    CAMDPortBases[portnum] = port;
    port->ba_IsOpen = TRUE;

    return (struct MidiPortData *)&port->ba_ActivateFunc;

    AROS_USERFUNC_EXIT
}

AROS_UFH2(VOID, AROS_SLIB_ENTRY(ClosePort, BTMidiCAMD, 0),
        AROS_UFHA(struct MidiDeviceData *, data, A3),
        AROS_UFHA(LONG, portnum, D0))
{
    AROS_USERFUNC_INIT

    if (portnum < 0 || portnum >= BTMIDI_PORTS)
        return;

    if (CAMDPortBases[portnum])
    {
        CAMDPortBases[portnum] = NULL;
        /* The class marks the port closed, clear of a delivery in progress. */
        btmidiCAMDClosePort(portnum, name);
    }

    AROS_USERFUNC_EXIT
}

/* CAMD has something to send on this port. */
AROS_UFH2(VOID, ActivateXmit,
        AROS_UFHA(APTR, userdata, A2),
        AROS_UFHA(ULONG, portnum, D0))
{
    AROS_USERFUNC_INIT

    if (portnum < BTMIDI_PORTS && CAMDPortBases[portnum])
        Cause(&CAMDPortBases[portnum]->ba_CAMDTXFunc);

    AROS_USERFUNC_EXIT
}
