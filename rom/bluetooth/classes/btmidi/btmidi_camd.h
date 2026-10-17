/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* What btmidi.class and its CAMD driver, DEVS:Midi/btmidi, share: one
   adapter per CAMD port. The driver is the embedded object in camd/. */

#ifndef BTMIDI_CAMD_H
#define BTMIDI_CAMD_H

#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <exec/tasks.h>
#include <exec/types.h>
#include <utility/hooks.h>

#define BTMIDI_DRIVER_NAME  "btmidi"
#define BTMIDI_DRIVER_PATH  "DEVS:Midi"

/* Port 0 is this machine as the BLE MIDI peripheral: whatever centrals
   connect to it. Ports 1 and up are the BLE MIDI devices it is bound to. */
#define BTMIDI_PORTS        16
#define BTMIDI_TXBUF_SIZE   4096    /* a power of two */

/* camd.library never unloads a driver, so the one it has in memory may come
   from an older class than the one that answers it. The driver opens a port
   only when the class's adapter has the layout it was built with. Count this
   up with any change to struct BTMidiAdapter; ba_ActivateFunc and ba_Version
   keep their places whatever else changes. */
#define BTMIDI_ADAPTER_VERSION  1

struct BTMidiAdapter
{
    APTR                    ba_ActivateFunc;  /* struct MidiPortData -- filled by the CAMD driver */
    ULONG                   ba_Version;       /* BTMIDI_ADAPTER_VERSION of the class */
    BOOL                    ba_IsOpen;        /* TRUE while CAMD has the port open */
    struct Hook             ba_CAMDRXFunc;    /* RX function in the CAMD driver */
    struct Interrupt        ba_CAMDTXFunc;    /* TX function in the CAMD driver (soft interrupt) */
    ULONG                   ba_PortNum;       /* Port number -- filled by OpenPort() */
    APTR                    ba_TXFunc;        /* CAMD's transmit function -- filled by OpenPort() */
    APTR                    ba_RXFunc;        /* CAMD's receive function -- filled by OpenPort() */
    APTR                    ba_UserData;      /* CAMD's user data -- filled by OpenPort() */
    UBYTE                  *ba_TXBuffer;      /* Ring of MIDI bytes to send over Bluetooth */
    ULONG                   ba_TXBufSize;     /* Its size */
    volatile ULONG          ba_TXWritePos;    /* Written by the TX function only */
    volatile ULONG          ba_TXReadPos;     /* Written by the owning task only */
    volatile BOOL           ba_TXMore;        /* The ring was full and CAMD has more */
    struct Task            *ba_Task;          /* The owning task, signalled when the ring has data */
    ULONG                   ba_SigMask;       /* with this */
    APTR                    ba_Owner;         /* The role or binding the port stands for, or NULL */
    struct SignalSemaphore  ba_Lock;          /* Opening, closing and delivery to CAMD */
};

/* What the RX function is handed: MIDI bytes, timestamps removed. */
struct BTMidiRXData
{
    ULONG rd_Length;
    UBYTE rd_Bytes[252];
};

#endif
