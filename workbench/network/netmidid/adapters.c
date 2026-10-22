/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* The service's side of the CAMD drivers DEVS:Midi/applemidi and
 * DEVS:Midi/networkmidi2: what camd.h declares, answered through the
 * drivers' adapters. The service is no client of camd.library. A driver
 * may be loaded after the service starts, or not at all: until its
 * adapters are found, what arrives from the network for its ports is
 * dropped. Each port is one peer's: which, the caller decides. The ports
 * are numbered through: the first driver's from 0, the second's from
 * NETMIDI_CAMD_MIDI2. */

#include "camd.h"

#include <proto/exec.h>
#include <exec/memory.h>

#include <stdio.h>
#include <string.h>

#include "netmidi_camd.h"
#include "netmidi_status.h"
#include "rtpmidi/rtpmidi.h"

int netmidi_camd_open(struct netmidi_camd_runtime *runtime)
{
    if (!runtime)
        return -1;
    memset(runtime, 0, sizeof(*runtime));
    runtime->sysex_port = -1;
    runtime->signal_bit = AllocSignal(-1);
    runtime->sysex = AllocMem(RTPMIDI_SYSEX_MAX, MEMF_ANY);
    if (runtime->signal_bit < 0 || !runtime->sysex) {
        netmidi_camd_close(runtime);
        return -1;
    }
    /* what we say about ourselves, for whoever asks */
    {
        static char status_name[] = NETMIDI_STATUS_NAME;
        struct NetMidiStatus *status =
            AllocMem(sizeof(*status), MEMF_PUBLIC | MEMF_CLEAR);

        if (status) {
            status->ns_Version = NETMIDI_STATUS_VERSION;
            InitSemaphore(&status->ns_Semaphore);
            status->ns_Semaphore.ss_Link.ln_Name = status_name;
            Forbid();
            /* one service: a second's is not made public */
            if (FindSemaphore((STRPTR)NETMIDI_STATUS_NAME)) {
                Permit();
                FreeMem(status, sizeof(*status));
            } else {
                AddSemaphore(&status->ns_Semaphore);
                Permit();
                runtime->status = status;
            }
        }
    }
    runtime->opened = 1;
    return 0;
}

void netmidi_camd_publish(struct netmidi_camd_runtime *runtime,
                          const char *session, unsigned long refused,
                          const struct netmidi_port_counts *counts,
                          int count)
{
    struct NetMidiStatus *status = runtime ? runtime->status : NULL;
    int port;
    size_t i;

    if (!status)
        return;
    ObtainSemaphore(&status->ns_Semaphore);
    for (i = 0; i + 1 < sizeof(status->ns_Session) && session && session[i];
         ++i)
        status->ns_Session[i] = session[i];
    status->ns_Session[i] = 0;
    status->ns_Refused = refused;
    for (port = 0; port < NETMIDI_STATUS_PORTS; ++port) {
        struct NetMidiPortStatus *to = &status->ns_Port[port];

        memset(to, 0, sizeof(*to));
        if (port >= count || !counts[port].present)
            continue;
        to->ps_Present = 1;
        memcpy(to->ps_Name, counts[port].name, sizeof(to->ps_Name) - 1);
        to->ps_PacketsIn = counts[port].packets_in;
        to->ps_PacketsOut = counts[port].packets_out;
        to->ps_PacketsLost = counts[port].packets_lost;
        to->ps_MessagesIn = counts[port].messages_in;
        to->ps_MessagesOut = counts[port].messages_out;
        to->ps_Timed = counts[port].timed;
        to->ps_Late = counts[port].late;
        to->ps_SysExDropped = counts[port].sysex_dropped;
        to->ps_JournalsOmitted = counts[port].journals_omitted;
        to->ps_Undefined = counts[port].undefined;
    }
    ReleaseSemaphore(&status->ns_Semaphore);
}

static const char *const driver_name[NETMIDI_CAMD_DRIVERS] = {
    APPLEMIDI_ADAPTERS_NAME, NETWORKMIDI2_ADAPTERS_NAME
};

static struct NetMidiAdapter *port_of(struct netmidi_camd_runtime *runtime,
                                      int number)
{
    struct NetMidiAdapters *adapters;

    if (number < 0 || number >= NETMIDI_CAMD_PORTS * NETMIDI_CAMD_DRIVERS)
        return NULL;
    adapters = runtime->adapters[number / NETMIDI_CAMD_PORTS];
    return adapters ? &adapters->aa_Port[number % NETMIDI_CAMD_PORTS] : NULL;
}

/* Has the port's driver send what CAMD holds for it. The interrupt is the
 * driver's code, which camd.library may have unloaded: a driver that went
 * says so in aa_Version, and with task switching forbidden it cannot go
 * between the look and the call. */
static void wake_driver(struct netmidi_camd_runtime *runtime, int number)
{
    struct NetMidiAdapters *adapters =
        runtime->adapters[number / NETMIDI_CAMD_PORTS];
    struct NetMidiAdapter *port;

    if (!adapters)
        return;
    port = &adapters->aa_Port[number % NETMIDI_CAMD_PORTS];
    Forbid();
    if (adapters->aa_Version == NETMIDI_ADAPTERS_VERSION && port->ad_IsOpen)
        Cause(&port->ad_CAMDTXFunc);
    Permit();
}

/* Looks for a driver's adapters and takes its ports; lets go of those of a
 * driver that camd.library unloaded, which may be followed by another. */
static void attach(struct netmidi_camd_runtime *runtime, int driver)
{
    struct NetMidiAdapters *adapters;
    struct NetMidiAdapter *port;
    int number;
    int other = 0;
    int went = 0;

    Forbid();
    adapters = (struct NetMidiAdapters *)
        FindSemaphore((STRPTR)driver_name[driver]);
    /* a driver of another layout is left alone */
    if (adapters && (adapters->aa_Version != NETMIDI_ADAPTERS_VERSION ||
                     adapters->aa_Ports < NETMIDI_CAMD_PORTS)) {
        adapters = NULL;
        other = 1;
    }
    if (runtime->adapters[driver] && runtime->adapters[driver] != adapters) {
        /* ours is on the list no more: its driver went */
        went = 1;
        runtime->adapters[driver] = NULL;
        for (number = 0; number < NETMIDI_CAMD_PORTS; ++number)
            memset(&runtime->stream[driver * NETMIDI_CAMD_PORTS + number], 0,
                   sizeof(runtime->stream[0]));
        if (runtime->sysex_port / NETMIDI_CAMD_PORTS == driver)
            runtime->sysex_port = -1;
    }
    if (adapters && !runtime->adapters[driver]) {
        runtime->adapters[driver] = adapters;
        for (number = 0; number < NETMIDI_CAMD_PORTS; ++number) {
            port = &adapters->aa_Port[number];
            Disable();
            port->ad_SigMask = 1UL << runtime->signal_bit;
            port->ad_Task = FindTask(NULL);
            Enable();
            /* CAMD may have had the port open all along, with bytes
               waiting */
            if (port->ad_IsOpen)
                Cause(&port->ad_CAMDTXFunc);
        }
    }
    Permit();
    if (went)
        printf("netmidid: the driver behind %s was unloaded\n",
               driver_name[driver]);
    if (other && !runtime->reported[driver]) {
        runtime->reported[driver] = 1;
        printf("netmidid: the driver behind %s is of another layout than this service; its ports are not served\n",
               driver_name[driver]);
    }
}

static void attach_all(struct netmidi_camd_runtime *runtime)
{
    int driver;

    for (driver = 0; driver < NETMIDI_CAMD_DRIVERS; ++driver)
        attach(runtime, driver);
}

void netmidi_camd_close(struct netmidi_camd_runtime *runtime)
{
    int number;

    if (!runtime)
        return;
    for (number = 0; number < NETMIDI_CAMD_PORTS * NETMIDI_CAMD_DRIVERS;
         ++number) {
        struct NetMidiAdapter *port = port_of(runtime, number);

        if (!port)
            continue;
        Disable();
        port->ad_Task = NULL;
        port->ad_SigMask = 0;
        Enable();
        /* the driver now drops what clients send */
        wake_driver(runtime, number);
    }
    /* a delivery to CAMD has finished: there is one task of ours */
    memset(runtime->adapters, 0, sizeof(runtime->adapters));
    if (runtime->status) {
        struct NetMidiStatus *status = runtime->status;

        /* off the list, so nobody new finds it; then wait for whoever
           is reading it */
        Forbid();
        RemSemaphore(&status->ns_Semaphore);
        Permit();
        ObtainSemaphore(&status->ns_Semaphore);
        ReleaseSemaphore(&status->ns_Semaphore);
        FreeMem(status, sizeof(*status));
        runtime->status = NULL;
    }
    if (runtime->signal_bit >= 0)
        FreeSignal(runtime->signal_bit);
    runtime->signal_bit = -1;
    if (runtime->sysex)
        FreeMem(runtime->sysex, RTPMIDI_SYSEX_MAX);
    runtime->sysex = NULL;
    runtime->opened = 0;
}

int netmidi_camd_is_open(const struct netmidi_camd_runtime *runtime)
{
    return runtime && runtime->opened;
}

int netmidi_camd_signal_bit(const struct netmidi_camd_runtime *runtime)
{
    if (!runtime || !runtime->opened)
        return -1;
    return runtime->signal_bit;
}

/* CAMD parses the bytes itself: running status, lengths and SysEx. */
void netmidi_camd_deliver_port(struct netmidi_camd_runtime *runtime,
                               int number, const uint8_t *message,
                               size_t length)
{
    struct NetMidiAdapter *port;

    if (!runtime || !runtime->opened)
        return;
    attach_all(runtime);
    port = port_of(runtime, number);
    if (!port)
        return;
    /* the driver closes the port under the same lock */
    ObtainSemaphore(&port->ad_Lock);
    if (port->ad_IsOpen) {
        size_t i;

        for (i = 0; i < length; ++i)
            AROS_UFC2NR(void, port->ad_RXFunc,
                AROS_UFCA(UWORD, message[i], D0),
                AROS_UFCA(APTR, port->ad_UserData, A2));
    }
    ReleaseSemaphore(&port->ad_Lock);
}

void netmidi_camd_deliver(struct netmidi_camd_runtime *runtime,
                          const uint8_t *message, size_t length)
{
    netmidi_camd_deliver_port(runtime, 0, message, length);
}

void netmidi_camd_deliver_sysex(struct netmidi_camd_runtime *runtime,
                                const uint8_t *message, size_t length)
{
    netmidi_camd_deliver_port(runtime, 0, message, length);
}

int netmidi_camd_ports(const struct netmidi_camd_runtime *runtime)
{
    (void)runtime;
    return NETMIDI_CAMD_PORTS;
}

unsigned long netmidi_camd_sysex_dropped(
    const struct netmidi_camd_runtime *runtime, int number)
{
    if (!runtime || number < 0 ||
        number >= NETMIDI_CAMD_PORTS * NETMIDI_CAMD_DRIVERS)
        return 0;
    return runtime->stream[number].sysex_dropped;
}

static unsigned data_bytes(uint8_t status)
{
    if (status < 0xf0)
        return (status & 0xf0) == 0xc0 || (status & 0xf0) == 0xd0 ? 1 : 2;
    if (status == 0xf1 || status == 0xf3)
        return 1;
    return status == 0xf2 ? 2 : 0;
}

/* One byte of the stream a CAMD client sent, by the rules of a MIDI 1.0
 * byte stream (MIDI 1.0 Detailed Specification 4.2.1, pages 5, 6, 29 and
 * 30): whole messages go to output. */
static void take_byte(struct netmidi_camd_runtime *all, int number,
                      uint8_t byte, aros_camd_output output, void *context)
{
#define runtime (&all->stream[number])
    if (byte >= 0xf8) {
        /* System Real Time: anywhere, and no new status */
        if (byte != 0xf9 && byte != 0xfd)
            output(context, &byte, 1);
        return;
    }
    if (runtime->in_sysex) {
        if (!(byte & 0x80)) {
            /* one buffer for all ports: a second port's SysEx message
               while the first is unfinished is left out, as one that is
               too long is */
            if (runtime->in_sysex == 1 && all->sysex_port == number &&
                runtime->sysex_length < RTPMIDI_SYSEX_MAX - 1) {
                all->sysex[runtime->sysex_length++] = byte;
            } else if (runtime->in_sysex == 1) {
                runtime->in_sysex = 2;
                ++runtime->sysex_dropped;
            }
            return;
        }
        /* EOX, or any other status, ends it */
        if (all->sysex_port == number) {
            if (runtime->in_sysex == 1) {
                all->sysex[runtime->sysex_length++] = 0xf7;
                output(context, all->sysex, runtime->sysex_length);
            }
            all->sysex_port = -1;
        }
        runtime->in_sysex = 0;
        if (byte == 0xf7)
            return;
    }
    if (byte == 0xf0) {
        if (all->sysex_port < 0) {
            all->sysex_port = number;
            runtime->in_sysex = 1;
            all->sysex[0] = 0xf0;
        } else {
            runtime->in_sysex = 2;
            ++runtime->sysex_dropped;
        }
        runtime->sysex_length = 1;
        runtime->status = 0;
        return;
    }
    if (byte & 0x80) {
        runtime->have = 0;
        runtime->status = 0;
        if (byte == 0xf7 || byte == 0xf4 || byte == 0xf5)
            return;
        runtime->need = (uint8_t)data_bytes(byte);
        if (!runtime->need) {
            output(context, &byte, 1);
            return;
        }
        runtime->status = byte;
        return;
    }
    if (!runtime->status)
        return;                 /* data with no status before it */
    runtime->message[0] = runtime->status;
    runtime->message[1 + runtime->have++] = byte;
    if (runtime->have < runtime->need)
        return;
    output(context, runtime->message, (size_t)runtime->need + 1);
    runtime->have = 0;
    if (runtime->status >= 0xf0)
        runtime->status = 0;    /* running status is for channel messages */
#undef runtime
}

void netmidi_camd_poll(struct netmidi_camd_runtime *runtime,
                       aros_camd_output output, void *context)
{
    struct NetMidiAdapter *port;
    ULONG pos;
    int number;

    if (!runtime || !runtime->opened)
        return;
    attach_all(runtime);
    for (number = 0; number < NETMIDI_CAMD_PORTS * NETMIDI_CAMD_DRIVERS;
         ++number) {
        port = port_of(runtime, number);
        if (!port)
            continue;
        runtime->port_now = number;
        pos = port->ad_TXReadPos;
        while (pos != port->ad_TXWritePos) {
            take_byte(runtime, number, port->ad_TXBuffer[pos], output,
                      context);
            pos = (pos + 1) & (port->ad_TXBufSize - 1);
        }
        port->ad_TXReadPos = pos;
        if (port->ad_TXMore) {
            /* the ring was full: there is room now */
            port->ad_TXMore = FALSE;
            wake_driver(runtime, number);
        }
    }
    runtime->port_now = 0;
}
