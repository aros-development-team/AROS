/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* The class's side of its CAMD driver, DEVS:Midi/btmidi.

   camd.library loads the driver, and the driver opens this class and calls
   btmidiCAMDOpenPort() for each port a CAMD client uses. A port's adapter
   connects the two: MIDI bytes from Bluetooth go to CAMD's receive function
   through ba_CAMDRXFunc, and CAMD's transmit function fills the adapter's
   ring, from a soft interrupt in the driver, for the task that owns the
   port to send. The class never opens camd.library.

   Port 0 is the peripheral role and ports 1 and up are the bound devices.
   A port and its device come and go independently: CAMD may hold a port
   open after its device left, and what it sends is then dropped. */

#include "btmidi.h"

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/alib.h>

#include <string.h>

/* The driver, built in camd/ and linked in as a blob. */
extern const UBYTE _binary_btmididriver_start[];
extern const UBYTE _binary_btmididriver_end[];

void btmidi_camd_init(struct BTMidiBase *base)
{
    ULONG port;

    for (port = 0; port < BTMIDI_PORTS; port++) {
        base->adapters[port].ba_PortNum = port;
        InitSemaphore(&base->adapters[port].ba_Lock);
    }
}

/* At expunge: the driver has closed the class, so no port is open. */
void btmidi_camd_free(struct BTMidiBase *base)
{
    ULONG port;

    for (port = 0; port < BTMIDI_PORTS; port++) {
        FreeVec(base->adapters[port].ba_TXBuffer);
        base->adapters[port].ba_TXBuffer = NULL;
    }
}

static BOOL driver_is_current(struct DosLibrary *DOSBase, CONST_STRPTR path,
                              const UBYTE *driver, LONG size)
{
    BOOL same = FALSE;
    UBYTE *have;
    BPTR fh;

    if (!(fh = Open(path, MODE_OLDFILE)))
        return FALSE;
    if ((have = AllocVec(size + 1, MEMF_ANY))) {
        same = Read(fh, have, size + 1) == size && !memcmp(have, driver, size);
        FreeVec(have);
    }
    Close(fh);
    return same;
}

/* Writes the driver to DEVS:Midi/btmidi unless it is there already; an
   older class's driver is replaced. camd.library finds it when it starts.
   Needs DOS, so the caller has to be a process. */
void btmidi_camd_install(struct BTMidiBase *base)
{
    static const char path[] = BTMIDI_DRIVER_PATH "/" BTMIDI_DRIVER_NAME;
    struct Process *process = (struct Process *)FindTask(NULL);
    struct DosLibrary *DOSBase;
    const UBYTE *driver = _binary_btmididriver_start;
    LONG size = _binary_btmididriver_end - _binary_btmididriver_start;
    APTR window;
    BPTR fh, lock;

    if (base->driver_state == BTMIDI_DRIVER_INSTALLED ||
        process->pr_Task.tc_Node.ln_Type != NT_PROCESS ||
        !(DOSBase = (struct DosLibrary *)OpenLibrary((CONST_STRPTR)"dos.library", 37)))
        return;
    /* no requester should DEVS: be missing */
    window = process->pr_WindowPtr;
    process->pr_WindowPtr = (APTR)-1;
    if (driver_is_current(DOSBase, (CONST_STRPTR)path, driver, size)) {
        base->driver_state = BTMIDI_DRIVER_INSTALLED;
    } else {
        if (!(fh = Open((CONST_STRPTR)path, MODE_NEWFILE))) {
            if ((lock = CreateDir((CONST_STRPTR)BTMIDI_DRIVER_PATH)))
                UnLock(lock);
            fh = Open((CONST_STRPTR)path, MODE_NEWFILE);
        }
        base->driver_state = BTMIDI_DRIVER_FAILED;
        if (fh) {
            if (Write(fh, (APTR)driver, size) == size)
                base->driver_state = BTMIDI_DRIVER_INSTALLED;
            Close(fh);
            if (base->driver_state != BTMIDI_DRIVER_INSTALLED)
                DeleteFile((CONST_STRPTR)path);
        }
    }
    process->pr_WindowPtr = window;
    CloseLibrary((struct Library *)DOSBase);
}

/* Gives owner a port: the one asked for, or with port < 0 the first free
   one from 1. Returns its number, or -1 when there is none. */
LONG btmidi_camd_reserve(struct BTMidiBase *base, LONG port, APTR owner,
                         const char *name)
{
    LONG first = port < 0 ? 1 : port;
    LONG last = port < 0 ? BTMIDI_PORTS - 1 : port;
    LONG found = -1;
    ULONG n;

    Forbid();
    for (port = first; port <= last; port++)
        if (!base->adapters[port].ba_Owner) {
            base->adapters[port].ba_Owner = owner;
            for (n = 0; n < BTMIDI_NAME_SIZE - 1 && name && name[n]; n++)
                base->port_names[port][n] = name[n];
            base->port_names[port][n] = 0;
            found = port;
            break;
        }
    Permit();
    if (found >= 0)
        btmidi_notify_gui(base);
    return found;
}

void btmidi_camd_attach(struct BTMidiAdapter *adapter, struct Task *task,
                        ULONG sigmask)
{
    Disable();
    adapter->ba_SigMask = sigmask;
    adapter->ba_Task = task;
    Enable();
    /* CAMD may have had the port open all along, with bytes waiting */
    if (adapter->ba_IsOpen)
        Cause(&adapter->ba_CAMDTXFunc);
}

/* The owner is leaving. CAMD may keep the port open; the driver then
   drops what is sent to it, and the next owner takes it over as it is. */
void btmidi_camd_release(struct BTMidiBase *base, ULONG port)
{
    struct BTMidiAdapter *adapter = &base->adapters[port];

    Disable();
    adapter->ba_Task = NULL;
    adapter->ba_SigMask = 0;
    Enable();
    if (adapter->ba_IsOpen)
        Cause(&adapter->ba_CAMDTXFunc);
    /* a delivery to CAMD has to finish first */
    ObtainSemaphore(&adapter->ba_Lock);
    Forbid();
    adapter->ba_Owner = NULL;
    base->port_names[port][0] = 0;
    Permit();
    ReleaseSemaphore(&adapter->ba_Lock);
    btmidi_notify_gui(base);
}

/* Takes up to size bytes CAMD sent out of the port's ring. */
ULONG btmidi_camd_read(struct BTMidiAdapter *adapter, UBYTE *buffer,
                       ULONG size)
{
    ULONG pos = adapter->ba_TXReadPos, count = 0;

    if (!adapter->ba_TXBuffer)
        return 0;
    while (count < size && pos != adapter->ba_TXWritePos) {
        buffer[count++] = adapter->ba_TXBuffer[pos];
        pos = (pos + 1) & (adapter->ba_TXBufSize - 1);
    }
    adapter->ba_TXReadPos = pos;
    if (count && adapter->ba_TXMore) {
        /* the ring was full: let the driver fetch the rest */
        adapter->ba_TXMore = FALSE;
        if (adapter->ba_IsOpen)
            Cause(&adapter->ba_CAMDTXFunc);
    }
    return count;
}

void btmidi_camd_rx_flush(struct btmidi_rx *rx)
{
    struct BTMidiAdapter *adapter = rx->adapter;

    if (!rx->data.rd_Length)
        return;
    /* CAMD's receive function only queues the bytes and signals, so it is
       safe to hold the lock that keeps ClosePort() out meanwhile. */
    ObtainSemaphore(&adapter->ba_Lock);
    if (adapter->ba_IsOpen)
        CallHookA(&adapter->ba_CAMDRXFunc, (Object *)&rx->data, NULL);
    ReleaseSemaphore(&adapter->ba_Lock);
    rx->data.rd_Length = 0;
}

void btmidi_camd_rx_byte(struct btmidi_rx *rx, UBYTE byte)
{
    if (rx->data.rd_Length == sizeof(rx->data.rd_Bytes))
        btmidi_camd_rx_flush(rx);
    rx->data.rd_Bytes[rx->data.rd_Length++] = byte;
}

AROS_LH5(struct BTMidiAdapter *, btmidiCAMDOpenPort,
         AROS_LHA(APTR, xmitfct, A0),
         AROS_LHA(APTR, recvfct, A1),
         AROS_LHA(APTR, userdata, A2),
         AROS_LHA(STRPTR, idstr, A3),
         AROS_LHA(ULONG, port, D0),
         LIBBASETYPEPTR, base, 15, btmidi)
{
    AROS_LIBFUNC_INIT

    struct BTMidiAdapter *adapter, *result = NULL;

    (void)idstr;
    if (port >= BTMIDI_PORTS)
        return NULL;
    adapter = &base->adapters[port];
    ObtainSemaphore(&adapter->ba_Lock);
    /* a port stands for a role or a bound device, or it does not exist */
    if (adapter->ba_Owner && !adapter->ba_IsOpen) {
        if (!adapter->ba_TXBuffer) {
            /* kept until the class is expunged: the driver's soft
               interrupt may still look at it after ClosePort() */
            adapter->ba_TXBufSize = BTMIDI_TXBUF_SIZE;
            adapter->ba_TXBuffer = AllocVec(BTMIDI_TXBUF_SIZE,
                                            MEMF_PUBLIC | MEMF_CLEAR);
        }
        if (adapter->ba_TXBuffer) {
            Disable();
            adapter->ba_TXFunc = xmitfct;
            adapter->ba_RXFunc = recvfct;
            adapter->ba_UserData = userdata;
            adapter->ba_TXReadPos = adapter->ba_TXWritePos = 0;
            adapter->ba_TXMore = FALSE;
            Enable();
            result = adapter;
        }
    }
    ReleaseSemaphore(&adapter->ba_Lock);
    if (result)
        btmidi_notify_gui(base);
    return result;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, btmidiCAMDClosePort,
         AROS_LHA(ULONG, port, D0),
         AROS_LHA(STRPTR, idstr, A1),
         LIBBASETYPEPTR, base, 16, btmidi)
{
    AROS_LIBFUNC_INIT

    struct BTMidiAdapter *adapter;

    (void)idstr;
    if (port >= BTMIDI_PORTS)
        return;
    adapter = &base->adapters[port];
    /* waits for a delivery to CAMD in progress; the soft interrupt cannot
       run while interrupts are off, and finds the port closed afterwards */
    ObtainSemaphore(&adapter->ba_Lock);
    Disable();
    adapter->ba_IsOpen = FALSE;
    Enable();
    ReleaseSemaphore(&adapter->ba_Lock);
    btmidi_notify_gui(base);

    AROS_LIBFUNC_EXIT
}
