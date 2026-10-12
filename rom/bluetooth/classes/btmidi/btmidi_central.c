/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* BLE MIDI peripherals this machine connects to as the central.

   bluetooth.library offers every registered device's services to the
   classes; this one binds to the BLE MIDI service of each device that has
   one. A binding is a task with a CAMD port of the driver btmidi, the
   first free one from btmidi.1: MIDI the device notifies arrives on the
   port, and what CAMD clients send to it is written to the device. The
   port is the binding's for as long as the binding exists, connected or
   not; the status window shows which device has which port.

   Like btbattery.class, the binding never connects the device itself: it
   follows the stack's connect and disconnect events, so the device comes
   and goes the way Bluetooth Preferences and the stack's reconnection of
   bonded devices have it. After a link comes up it waits a moment (a link
   serves one GATT request at a time, and other classes look at the device
   too), reads the characteristic once as the BLE MIDI specification asks
   of a central, and then subscribes to its notifications. */

#include "btmidi.h"

#include <exec/memory.h>
#include <proto/bluetooth.h>
#include <proto/exec.h>

#include <string.h>

#define BTMIDI_SETTLE_MS 1000   /* a new link is left alone this long */
#define BTMIDI_RETRY_MS  2000   /* and a failed request is retried after this */

/* ---- in the caller's task (the stack's class handling) ---- */

#define BluetoothBase bluetooth

static BOOL is_ble_midi(struct Library *bluetooth, struct BtService *service)
{
    static const UBYTE uuid[16] = BLEMIDI_SERVICE_UUID;
    IPTR proto = 0;
    UBYTE *service_uuid = NULL;

    btGetAttrs(BGA_SERVICE, service, BSVA_Protocol, &proto,
               BSVA_UUID, &service_uuid, TAG_END);
    return proto == BSVP_ATT && service_uuid && !memcmp(service_uuid, uuid, 16);
}

struct btmidi_binding *btmidi_force_binding(struct BTMidiBase *base,
                                            struct BtService *service)
{
    static const UBYTE io_uuid[16] = BLEMIDI_IO_UUID;
    struct Library *bluetooth;
    struct btmidi_binding *binding = NULL;
    struct BtDevice *device = NULL;
    struct BtEndpoint *endpoint;
    STRPTR device_name = NULL;
    IPTR handle = 0;

    if (!(bluetooth = OpenLibrary((CONST_STRPTR)"bluetooth.library", 45)))
        return NULL;
    btGetAttrs(BGA_SERVICE, service, BSVA_Device, &device, TAG_END);
    endpoint = btFindEndpoint(service, NULL, BEA_UUID, (IPTR)io_uuid,
                              BEA_Type, BEPT_GATT_CHAR, TAG_END);
    if (device && endpoint &&
        (binding = AllocVec(sizeof(*binding), MEMF_PUBLIC | MEMF_CLEAR))) {
        struct Task *task;
        UBYTE task_name[48];
        LONG port;

        btGetAttrs(BGA_DEVICE, device, BDA_Name, &device_name, TAG_END);
        btGetAttrs(BGA_ENDPOINT, endpoint, BEA_Handle, &handle, TAG_END);
        binding->base = base;
        binding->device = device;
        binding->service = service;
        binding->endpoint = endpoint;
        binding->handle = handle;
        binding->tx_signal = -1;
        port = btmidi_camd_reserve(base, -1, binding,
                                   device_name && device_name[0]
                                       ? (const char *)device_name
                                       : "BLE MIDI device");
        if (port < 0) {
            /* every port has a device: this one stays without CAMD */
            FreeVec(binding);
            CloseLibrary(bluetooth);
            return NULL;
        }
        binding->port = port;
        binding->rx.adapter = &base->adapters[port];

        btSafeRawDoFmt(task_name, sizeof(task_name),
                       (STRPTR)"btmidi.class<%08lx>", (IPTR)binding);
        binding->ready_signal = SIGB_SINGLE;
        binding->ready_task = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);
        if ((task = btSpawnSubTask(task_name, (APTR)btmidi_binding_task,
                                   binding)))
            btBorrowLocksWait(task, 1UL << binding->ready_signal);
        binding->ready_task = NULL;
        if (!binding->task) {
            btmidi_camd_release(base, binding->port);
            FreeVec(binding);
            binding = NULL;
        }
    }
    CloseLibrary(bluetooth);
    return binding;
}

struct btmidi_binding *btmidi_attempt_binding(struct BTMidiBase *base,
                                              struct BtService *service)
{
    struct Library *bluetooth;
    BOOL midi = FALSE;

    if ((bluetooth = OpenLibrary((CONST_STRPTR)"bluetooth.library", 45))) {
        midi = is_ble_midi(bluetooth, service);
        CloseLibrary(bluetooth);
    }
    return midi ? btmidi_force_binding(base, service) : NULL;
}

#undef BluetoothBase

void btmidi_release_binding(struct BTMidiBase *base,
                            struct btmidi_binding *binding)
{
    Forbid();
    binding->ready_signal = SIGB_SINGLE;
    binding->ready_task = FindTask(NULL);
    if (binding->task)
        Signal(binding->task, SIGBREAKF_CTRL_C);
    Permit();
    while (binding->task)
        Wait(1UL << binding->ready_signal);
    binding->ready_task = NULL;
    btmidi_camd_release(base, binding->port);
    FreeVec(binding);
}

/* ---- in the binding task ---- */

#define BluetoothBase binding->bt_base

static void arm_timer(struct btmidi_binding *binding, ULONG ms)
{
    if (!binding->timer_open)
        return;
    if (binding->timer_pending) {
        AbortIO((struct IORequest *)binding->timer_req);
        WaitIO((struct IORequest *)binding->timer_req);
        SetSignal(0, 1UL << binding->timer_port->mp_SigBit);
    }
    binding->timer_req->tr_node.io_Command = TR_ADDREQUEST;
    binding->timer_req->tr_time.tv_secs = ms / 1000;
    binding->timer_req->tr_time.tv_micro = (ms % 1000) * 1000;
    SendIO((struct IORequest *)binding->timer_req);
    binding->timer_pending = TRUE;
}

static int device_byte(void *context, uint16_t timestamp, uint8_t byte)
{
    struct btmidi_binding *binding = context;

    (void)timestamp;
    btmidi_camd_rx_byte(&binding->rx, byte);
    return 0;
}

static int write_packet(void *context, const uint8_t *packet, size_t length)
{
    struct btmidi_binding *binding = context;

    return btDoChannel(binding->write_ch, (APTR)packet, length) ? -1 : 0;
}

/* What CAMD clients sent to the port goes to the device, or nowhere while
   it is away: the ring is emptied either way, so no sender waits. */
static void send_to_device(struct btmidi_binding *binding)
{
    struct BTMidiAdapter *adapter = binding->rx.adapter;
    UBYTE bytes[64];
    UWORD timestamp = btmidi_now_ms(binding->timer_open ?
                                    binding->timer_req->tr_node.io_Device : NULL);
    ULONG count, n;

    if (!binding->primed) {
        while (btmidi_camd_read(adapter, bytes, sizeof(bytes)))
            ;
        blemidi_writer_reset(&binding->writer);
        return;
    }
    blemidi_writer_capacity(&binding->writer, binding->packet_limit);
    while ((count = btmidi_camd_read(adapter, bytes, sizeof(bytes))))
        for (n = 0; n < count; n++)
            blemidi_writer_put(&binding->writer, timestamp, bytes[n]);
    blemidi_writer_flush(&binding->writer);
}

/* The read first, then the notifications. Both fail while the device is
   away or its link is busy; the timer tries again. */
static void start_requests(struct btmidi_binding *binding)
{
    IPTR max_packet = 20;

    if (!binding->connected)
        return;
    if (!binding->primed) {
        if (!binding->read_busy) {
            btSendChannel(binding->read_ch, binding->read_buf,
                          sizeof(binding->read_buf));
            binding->read_busy = TRUE;
        }
        return;
    }
    btGetAttrs(BGA_ENDPOINT, binding->endpoint, BEA_MaxPktSize, &max_packet,
               TAG_END);
    binding->packet_limit = (max_packet >= 20 &&
                             max_packet <= sizeof(binding->notify_buf))
                            ? max_packet : 20;
    if (!binding->notify_posted) {
        btSendChannel(binding->notify_ch, binding->notify_buf,
                      sizeof(binding->notify_buf));
        binding->notify_posted = TRUE;
    }
}

static void channel_done(struct btmidi_binding *binding, APTR channel)
{
    LONG error = btGetChannelError(channel);
    ULONG actual = btGetChannelActual(channel);

    if (channel == binding->read_ch) {
        binding->read_busy = FALSE;
        if (error) {
            arm_timer(binding, BTMIDI_RETRY_MS);
            return;
        }
        /* a BLE MIDI read returns nothing; what it returned is ignored */
        binding->primed = TRUE;
        start_requests(binding);
    } else if (channel == binding->notify_ch) {
        binding->notify_posted = FALSE;
        if (error) {
            arm_timer(binding, BTMIDI_RETRY_MS);
            return;
        }
        if (actual) {
            if (blemidi_check(&binding->decoder, binding->notify_buf,
                              actual)) {
                /* nothing of a malformed packet reaches CAMD */
                blemidi_decoder_init(&binding->decoder);
            } else {
                blemidi_decode(&binding->decoder, binding->notify_buf,
                               actual, device_byte, binding);
                btmidi_camd_rx_flush(&binding->rx);
            }
        }
        if (!(SetSignal(0, 0) & SIGBREAKF_CTRL_C))
            start_requests(binding);
    }
}

static void device_event(struct btmidi_binding *binding, struct Message *message)
{
    IPTR event = 0;
    APTR device = NULL;

    btGetAttrs(BGA_EVENTNOTE, message, BENA_EventID, &event,
               BENA_Param1, &device, TAG_END);
    ReplyMsg(message);
    if (device != (APTR)binding->device)
        return;
    if (event == BEHMB_DEVICEDISCONNECTED) {
        binding->connected = FALSE;
        binding->primed = FALSE;
        blemidi_decoder_init(&binding->decoder);
        blemidi_writer_reset(&binding->writer);
    } else if (event == BEHMB_DEVICECONNECTED) {
        binding->connected = TRUE;
        binding->primed = FALSE;
        arm_timer(binding, BTMIDI_SETTLE_MS);
    }
}

static BOOL setup(struct btmidi_binding *binding)
{
    if (!(binding->channel_port = CreateMsgPort()) ||
        !(binding->event_port = CreateMsgPort()) ||
        !(binding->timer_port = CreateMsgPort()) ||
        !(binding->timer_req = (struct timerequest *)
              CreateIORequest(binding->timer_port, sizeof(struct timerequest))) ||
        OpenDevice((CONST_STRPTR)"timer.device", UNIT_VBLANK,
                   (struct IORequest *)binding->timer_req, 0))
        return FALSE;
    binding->timer_open = TRUE;
    if (!(binding->notify_ch = btAllocChannel(binding->device,
                                              binding->channel_port,
                                              binding->endpoint)) ||
        !(binding->read_ch = btAllocChannel(binding->device,
                                            binding->channel_port, NULL)) ||
        !(binding->write_ch = btAllocChannel(binding->device,
                                             binding->channel_port,
                                             binding->endpoint)))
        return FALSE;
    btChannelSetup(binding->notify_ch, BTPR_READ, 0, 0);
    btChannelSetup(binding->read_ch, BTPR_GATTREAD, (UWORD)binding->handle, 0);
    btChannelSetup(binding->write_ch, BTPR_GATTWRITENORSP,
                   (UWORD)binding->handle, 0);
    if ((binding->tx_signal = AllocSignal(-1)) < 0)
        return FALSE;
    blemidi_decoder_init(&binding->decoder);
    blemidi_writer_init(&binding->writer, write_packet, binding);
    binding->packet_limit = 20;
    binding->event_handler = btAddEventHandler(binding->event_port,
                                               BEHMF_DEVICECONNECTED |
                                               BEHMF_DEVICEDISCONNECTED);
    if (!binding->event_handler)
        return FALSE;
    btmidi_camd_attach(binding->rx.adapter, FindTask(NULL),
                       1UL << binding->tx_signal);
    return TRUE;
}

static void free_channel(struct btmidi_binding *binding, APTR *channel)
{
    if (*channel) {
        btAbortChannel(*channel);
        btWaitChannel(*channel);
        btFreeChannel(*channel);
        *channel = NULL;
    }
}

static void cleanup(struct btmidi_binding *binding)
{
    if (binding->event_handler)
        btRemEventHandler(binding->event_handler);
    free_channel(binding, &binding->notify_ch);
    free_channel(binding, &binding->read_ch);
    free_channel(binding, &binding->write_ch);
    /* the port stays reserved until the binding is released, but nothing
       signals this task any more */
    btmidi_camd_attach(binding->rx.adapter, NULL, 0);
    if (binding->tx_signal >= 0)
        FreeSignal(binding->tx_signal);
    binding->tx_signal = -1;
    if (binding->timer_open) {
        if (binding->timer_pending) {
            AbortIO((struct IORequest *)binding->timer_req);
            WaitIO((struct IORequest *)binding->timer_req);
        }
        CloseDevice((struct IORequest *)binding->timer_req);
    }
    if (binding->timer_req)
        DeleteIORequest((struct IORequest *)binding->timer_req);
    if (binding->event_port) {
        struct Message *message;

        while ((message = GetMsg(binding->event_port)))
            ReplyMsg(message);
        DeleteMsgPort(binding->event_port);
    }
    if (binding->timer_port)
        DeleteMsgPort(binding->timer_port);
    if (binding->channel_port)
        DeleteMsgPort(binding->channel_port);
}

AROS_UFH0(void, btmidi_binding_task)
{
    AROS_USERFUNC_INIT
    struct btmidi_binding *binding = FindTask(NULL)->tc_UserData;

    binding->bt_base = OpenLibrary((CONST_STRPTR)"bluetooth.library", 45);
    if (binding->bt_base && setup(binding))
        binding->task = FindTask(NULL);

    /* On failure the parent owns and frees binding as soon as it is woken.
       Finish every access to it before publishing the failed start. */
    if (!binding->task) {
        cleanup(binding);
        if (binding->bt_base)
            CloseLibrary(binding->bt_base);
        binding->bt_base = NULL;
        Forbid();
        if (binding->ready_task)
            Signal(binding->ready_task, 1UL << binding->ready_signal);
        Permit();
        return;
    }

    Forbid();
    if (binding->ready_task)
        Signal(binding->ready_task, 1UL << binding->ready_signal);
    Permit();

    {
        IPTR connected = FALSE;
        ULONG signals = 0;

        btGetAttrs(BGA_DEVICE, binding->device, BDA_IsConnected, &connected,
                   TAG_END);
        binding->connected = connected ? TRUE : FALSE;
        if (binding->connected)
            arm_timer(binding, BTMIDI_SETTLE_MS);
        while (!(signals & SIGBREAKF_CTRL_C)) {
            struct Message *message;
            APTR channel;

            signals = Wait((1UL << binding->channel_port->mp_SigBit) |
                           (1UL << binding->event_port->mp_SigBit) |
                           (1UL << binding->timer_port->mp_SigBit) |
                           (1UL << binding->tx_signal) |
                           SIGBREAKF_CTRL_C);
            while ((message = GetMsg(binding->event_port)))
                device_event(binding, message);
            while ((channel = (APTR)GetMsg(binding->channel_port)))
                channel_done(binding, channel);
            if (signals & (1UL << binding->timer_port->mp_SigBit)) {
                while (GetMsg(binding->timer_port))
                    binding->timer_pending = FALSE;
                if (!binding->timer_pending)
                    start_requests(binding);
            }
            if (signals & (1UL << binding->tx_signal))
                send_to_device(binding);
        }
    }

    cleanup(binding);
    if (binding->bt_base)
        CloseLibrary(binding->bt_base);
    binding->bt_base = NULL;
    Forbid();
    binding->task = NULL;
    if (binding->ready_task)
        Signal(binding->ready_task, 1UL << binding->ready_signal);
    AROS_USERFUNC_EXIT
}
