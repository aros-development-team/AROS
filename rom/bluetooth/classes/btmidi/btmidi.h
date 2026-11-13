/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef BTMIDI_H
#define BTMIDI_H

#include LC_LIBDEFS_FILE

#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <exec/libraries.h>
#include <exec/ports.h>
#include <exec/tasks.h>
#include <utility/tagitem.h>

#include <devices/timer.h>
#include <libraries/bluetooth.h>
#include <libraries/btclass.h>

#include "blemidi.h"
#include "btmidi_camd.h"

#define BTMIDI_NAME_SIZE 32
#define BTMIDI_VALUE_SIZE 512     /* the largest BLE MIDI packet taken */

/* Written by the service task only; the status window reads them. */
struct BTMidiStats {
    ULONG ms_RxPackets;   /* BLE MIDI packets written by centrals */
    ULONG ms_RxMessages;  /* MIDI messages handed to CAMD */
    ULONG ms_TxPackets;   /* notifications queued to centrals */
    ULONG ms_TxMessages;  /* MIDI messages taken from CAMD */
    ULONG ms_Errors;      /* malformed packets and failed notifications */
};

enum btmidi_driver_state {
    BTMIDI_DRIVER_UNKNOWN,    /* not looked at yet: no DOS so far */
    BTMIDI_DRIVER_INSTALLED,  /* DEVS:Midi/btmidi is this class's driver */
    BTMIDI_DRIVER_FAILED      /* it could not be written */
};

/* MIDI bytes on their way to a port's CAMD side. */
struct btmidi_rx {
    struct BTMidiAdapter *adapter;
    struct BTMidiRXData data;
};

struct btmidi_gui;

/* A BLE MIDI peripheral (a keyboard, a controller) this machine connects
   to as the central: bound to its BLE MIDI service, with a CAMD port of
   its own, btmidi.1 and up. */
struct btmidi_binding {
    struct BTMidiBase *base;
    struct Library *bt_base;        /* bluetooth.library (binding task) */
    struct BtDevice *device;
    struct BtService *service;
    struct BtEndpoint *endpoint;    /* the BLE MIDI I/O characteristic */
    ULONG handle;                   /* its value handle */

    struct Task *ready_task;
    LONG ready_signal;
    struct Task *task;
    struct MsgPort *channel_port;
    struct MsgPort *event_port;
    APTR event_handler;
    struct MsgPort *timer_port;
    struct timerequest *timer_req;
    BOOL timer_open;
    BOOL timer_pending;

    APTR notify_ch;                 /* notifications from the device */
    APTR read_ch;                   /* the read a central makes first */
    APTR write_ch;                  /* writes without response to it */
    BOOL notify_posted;
    BOOL read_busy;
    BOOL connected;
    BOOL primed;                    /* read done; notifications follow */
    ULONG packet_limit;

    ULONG port;                     /* its CAMD port */
    LONG tx_signal;                 /* the port's ring has bytes to send */
    UBYTE notify_buf[BTMIDI_VALUE_SIZE];
    UBYTE read_buf[BTMIDI_VALUE_SIZE];
    struct blemidi_decoder decoder;
    struct blemidi_writer writer;
    struct btmidi_rx rx;
};

struct BTMidiBase {
    struct Library library;
    struct Library *utility_base;
    struct Task *task;
    struct Task *ready_task;
    LONG ready_signal;

    APTR record;                    /* the BLE MIDI service record */
    enum btmidi_driver_state driver_state;
    struct BTMidiStats stats;

    /* One per CAMD port. An adapter's ba_Owner and the port's name change
       under Forbid(). */
    struct BTMidiAdapter adapters[BTMIDI_PORTS];
    char port_names[BTMIDI_PORTS][BTMIDI_NAME_SIZE];

    struct Task *gui_task;          /* the status window, if open */
    struct btmidi_gui *gui;
    volatile BOOL activity_pending; /* the window has not refreshed yet */
};

BOOL btmidi_open_cfg_window(struct BTMidiBase *base, struct Library *bluetooth);
void btmidi_notify_gui(struct BTMidiBase *base);

UWORD btmidi_now_ms(struct Device *timer);
AROS_UFP0(void, btmidi_gui_task);
AROS_UFP0(void, btmidi_binding_task);

struct btmidi_binding *btmidi_attempt_binding(struct BTMidiBase *base,
                                              struct BtService *service);
struct btmidi_binding *btmidi_force_binding(struct BTMidiBase *base,
                                            struct BtService *service);
void btmidi_release_binding(struct BTMidiBase *base,
                            struct btmidi_binding *binding);

/* btmidi_camd.c: the class's side of the CAMD driver. */
void btmidi_camd_init(struct BTMidiBase *base);
void btmidi_camd_free(struct BTMidiBase *base);
void btmidi_camd_install(struct BTMidiBase *base);
/* A port belongs to one role or binding at a time. Reserving one fixes who
   it stands for; attaching names the task that does its I/O. */
LONG btmidi_camd_reserve(struct BTMidiBase *base, LONG port, APTR owner,
                         const char *name);
void btmidi_camd_attach(struct BTMidiAdapter *adapter, struct Task *task,
                        ULONG sigmask);
void btmidi_camd_release(struct BTMidiBase *base, ULONG port);
/* In the attached task only. */
ULONG btmidi_camd_read(struct BTMidiAdapter *adapter, UBYTE *buffer,
                       ULONG size);
void btmidi_camd_rx_byte(struct btmidi_rx *rx, UBYTE byte);
void btmidi_camd_rx_flush(struct btmidi_rx *rx);

#endif
