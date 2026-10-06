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
#include "camdbridge.h"

#define BTMIDI_NAME_SIZE 32

#define BTMIDI_DEFAULT_NODE "BLE MIDI"
#define BTMIDI_DEFAULT_IN   "BLE MIDI In"
#define BTMIDI_DEFAULT_OUT  "BLE MIDI Out"

/* The class configuration, stored as chunk 'BMID' of the class's
   configuration by bluetooth.library: the names of the CAMD node and of its
   two clusters. Other programs may read it to recognise the ports. */
struct BTMidiCfg {
    ULONG mc_ChunkID;
    ULONG mc_Length;
    char mc_NodeName[BTMIDI_NAME_SIZE];
    char mc_InName[BTMIDI_NAME_SIZE];
    char mc_OutName[BTMIDI_NAME_SIZE];
};

/* Written by the service task only; the settings window reads them. */
struct BTMidiStats {
    ULONG ms_RxPackets;   /* BLE MIDI packets written by centrals */
    ULONG ms_RxMessages;  /* MIDI messages delivered to CAMD */
    ULONG ms_TxPackets;   /* notifications queued to centrals */
    ULONG ms_TxMessages;  /* MIDI messages taken from CAMD */
    ULONG ms_Errors;      /* malformed packets and failed notifications */
};

enum btmidi_camd_state {
    BTMIDI_CAMD_CLOSED,
    BTMIDI_CAMD_OPEN,
    BTMIDI_CAMD_DEFAULTS  /* the configured names failed; defaults in use */
};

struct btmidi_gui;

/* A BLE MIDI peripheral (a keyboard, a controller) this machine connects
   to as the central: bound to its BLE MIDI service, with a CAMD node of its
   own named after the device. */
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

    struct btmidi_camd camd;
    char node_name[BTMIDI_NAME_SIZE];
    char in_name[BTMIDI_NAME_SIZE];
    char out_name[BTMIDI_NAME_SIZE];
    UBYTE notify_buf[512];
    UBYTE read_buf[512];
    struct blemidi_stream stream;
};

struct BTMidiBase {
    struct Library library;
    struct Library *utility_base;
    struct Task *task;
    struct Task *ready_task;
    LONG ready_signal;

    struct BTMidiCfg cfg;           /* the configuration in effect */
    BOOL using_default_cfg;
    APTR record;                    /* the BLE MIDI service record */
    enum btmidi_camd_state camd_state;
    struct BTMidiStats stats;

    struct Task *gui_task;          /* the settings window, if open */
    struct btmidi_gui *gui;
    volatile BOOL activity_pending; /* the window has not refreshed yet */
};

void btmidi_default_cfg(struct BTMidiCfg *cfg);
void btmidi_load_cfg(struct BTMidiBase *base, struct Library *bluetooth);
void btmidi_store_cfg(struct BTMidiBase *base, struct Library *bluetooth,
                      BOOL to_disk);
void btmidi_reconfigure(struct BTMidiBase *base);
BOOL btmidi_open_cfg_window(struct BTMidiBase *base, struct Library *bluetooth);

UWORD btmidi_now_ms(struct Device *timer);
AROS_UFP0(void, btmidi_gui_task);
AROS_UFP0(void, btmidi_binding_task);

struct btmidi_binding *btmidi_attempt_binding(struct BTMidiBase *base,
                                              struct BtService *service);
struct btmidi_binding *btmidi_force_binding(struct BTMidiBase *base,
                                            struct BtService *service);
void btmidi_release_binding(struct BTMidiBase *base,
                            struct btmidi_binding *binding);

#endif
