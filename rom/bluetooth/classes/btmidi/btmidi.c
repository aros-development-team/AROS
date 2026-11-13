/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* The BLE MIDI class: the peripheral role's GATT service, and the class
   entry points. The CAMD side is in btmidi_camd.c. */

#include "btmidi.h"
#include "blemidi.h"
#include <exec/memory.h>
#include <proto/bluetooth.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <string.h>
#include <proto/timer.h>

#define BTMIDI_TX_PACKET_MIN 20    /* fits the default ATT MTU */
#define BTMIDI_TX_PACKET_MAX 244   /* the stack's largest ATT MTU, less 3 */
#define BTMIDI_MAX_PEERS 4
#define BTMIDI_CONN_INTERVAL 12   /* 15 ms, as the BLE MIDI specification asks */

struct Library *BluetoothBase;

struct btmidi_runtime;

/* What a central has sent so far. Each connected central has its own: their
   packets interleave, and a SysEx spans packets. */
struct btmidi_peer {
    struct btmidi_runtime *runtime;
    BOOL in_use;
    APTR device;              /* the writer, or NULL from an older stack */
    ULONG last_use;
    BOOL sysex;               /* it is inside a SysEx */
    BOOL dropping;            /* which does not reach CAMD */
    struct blemidi_decoder decoder;
};

/* The peripheral role: CAMD port 0. */
struct btmidi_runtime {
    struct BTMidiBase *base;
    struct MsgPort *event_port;
    APTR event_handler;
    APTR record;
    struct BTMidiAdapter *adapter;
    LONG tx_signal;           /* the port's ring has bytes to send */
    struct btmidi_peer *peers[BTMIDI_MAX_PEERS];
    struct btmidi_peer *sysex_peer;   /* whose SysEx CAMD is receiving */
    ULONG use_counter;
    struct timerequest timer_io;  /* only opened, for its base: the clock */
    struct Device *timer;
    struct blemidi_writer writer;
    struct btmidi_rx rx;
};

/* The BLE MIDI timestamp: milliseconds, 13 bits. timer is an opened
   timer.device's base; without one every packet says 0. */
UWORD btmidi_now_ms(struct Device *timer)
{
#define TimerBase timer
    struct timeval now;

    if (!timer)
        return 0;
    GetSysTime(&now);
#undef TimerBase
    return (UWORD)(((unsigned long long)now.tv_sec * 1000ULL +
                    (unsigned long long)now.tv_usec / 1000ULL) & 0x1fff);
}

/* One MIDI byte from a central, on its way to port 0. The centrals share
   the port, and CAMD parses a single stream: whole messages may alternate,
   but only one central's SysEx can be under way. A second SysEx is left
   out, and a message that interrupts a SysEx ends it for CAMD, so the rest
   of that SysEx is left out too. */
static int peer_byte(void *context, uint16_t timestamp, uint8_t byte)
{
    struct btmidi_peer *peer = context;
    struct btmidi_runtime *runtime = peer->runtime;

    (void)timestamp;
    if (byte >= 0xf8) {
        runtime->base->stats.ms_RxMessages++;
    } else if (byte == 0xf0) {
        peer->sysex = TRUE;
        peer->dropping = runtime->sysex_peer && runtime->sysex_peer != peer;
        if (peer->dropping) {
            runtime->base->stats.ms_Errors++;
            return 0;
        }
        runtime->sysex_peer = peer;
        runtime->base->stats.ms_RxMessages++;
    } else if (peer->sysex) {
        if (byte == 0xf7) {
            peer->sysex = FALSE;
            if (runtime->sysex_peer == peer)
                runtime->sysex_peer = NULL;
        }
        if (peer->dropping) {
            if (byte == 0xf7)
                peer->dropping = FALSE;
            return 0;
        }
    } else if (byte & 0x80) {
        if (runtime->sysex_peer) {
            runtime->sysex_peer->dropping = TRUE;
            runtime->sysex_peer = NULL;
            runtime->base->stats.ms_Errors++;
        }
        runtime->base->stats.ms_RxMessages++;
    }
    btmidi_camd_rx_byte(&runtime->rx, byte);
    return 0;
}

static void peer_reset(struct btmidi_peer *peer)
{
    if (peer->runtime->sysex_peer == peer)
        peer->runtime->sysex_peer = NULL;   /* the next status ends it */
    peer->sysex = peer->dropping = FALSE;
    blemidi_decoder_init(&peer->decoder);
}

static int set_packet(void *context, const uint8_t *packet, size_t length)
{
    struct btmidi_runtime *runtime = context;

    if (btSetServiceValue(runtime->record, 0, (APTR)packet, length) != (LONG)length) {
        runtime->base->stats.ms_Errors++;
        return -1;
    }
    runtime->base->stats.ms_TxPackets++;
    return 0;
}

/* What CAMD clients sent to port 0 goes to every subscribed central. */
static void send_to_ble(struct btmidi_runtime *runtime)
{
    UBYTE bytes[64];
    IPTR payload = BTMIDI_TX_PACKET_MIN;
    unsigned long before = runtime->writer.messages;
    UWORD timestamp = btmidi_now_ms(runtime->timer);
    ULONG count, n;

    /* larger packets once every central has negotiated a larger ATT MTU:
       SysEx needs fewer of them */
    btGetAttrs(BGA_STACK, NULL, BSA_LENotifyPayload, &payload, TAG_END);
    if (payload > BTMIDI_TX_PACKET_MAX)
        payload = BTMIDI_TX_PACKET_MAX;
    blemidi_writer_capacity(&runtime->writer, payload);
    while ((count = btmidi_camd_read(runtime->adapter, bytes, sizeof(bytes))))
        for (n = 0; n < count; n++)
            blemidi_writer_put(&runtime->writer, timestamp, bytes[n]);
    blemidi_writer_flush(&runtime->writer);
    runtime->base->stats.ms_TxMessages += runtime->writer.messages - before;
}

static APTR add_service(void)
{
    static const UBYTE service_uuid[16] = BLEMIDI_SERVICE_UUID;
    static const UBYTE io_uuid[16] = BLEMIDI_IO_UUID;
    struct BtGattCharDef characteristic;

    memset(&characteristic, 0, sizeof(characteristic));
    characteristic.bgd_UUID128 = io_uuid;
    /* the specification wants a read answered with no payload */
    characteristic.bgd_Properties = BGDP_READ | BGDP_WRITENR |
                                    BGDP_WRITE | BGDP_NOTIFY | BGDP_STREAM;
    characteristic.bgd_MaxLen = BTMIDI_VALUE_SIZE;
    return btAddServiceRecord(BSRA_Protocol, BSVP_ATT,
                              BSRA_UUID128, (IPTR)service_uuid,
                              BSRA_Name, (IPTR)"BLE MIDI",
                              BSRA_Owner, (IPTR)"btmidi.class",
                              BSRA_LEConnInterval, BTMIDI_CONN_INTERVAL,
                              BSRA_Characteristics, (IPTR)&characteristic,
                              BSRA_NumCharacteristics, 1, TAG_END);
}

/* The state of the central that wrote. A new central takes a free slot, or
   the one unused for longest when more centrals write than there are slots. */
static struct btmidi_peer *find_peer(struct btmidi_runtime *runtime, APTR device)
{
    struct btmidi_peer *peer, *oldest = NULL;
    ULONG n;

    for (n = 0; n < BTMIDI_MAX_PEERS; n++) {
        peer = runtime->peers[n];
        if (peer && peer->in_use && peer->device == device)
            goto found;
    }
    for (n = 0; n < BTMIDI_MAX_PEERS; n++) {
        peer = runtime->peers[n];
        if (!peer) {
            peer = runtime->peers[n] = AllocVec(sizeof(*peer), MEMF_ANY);
            if (!peer)
                continue;
            peer->in_use = FALSE;
        }
        if (!peer->in_use) {
            oldest = peer;
            break;
        }
        if (!oldest || (LONG)(peer->last_use - oldest->last_use) < 0)
            oldest = peer;
    }
    if (!(peer = oldest))
        return NULL;
    peer->runtime = runtime;
    peer->in_use = TRUE;
    peer->device = device;
    peer_reset(peer);
found:
    peer->last_use = ++runtime->use_counter;
    return peer;
}

static void forget_peer(struct btmidi_runtime *runtime, APTR device)
{
    ULONG n;

    for (n = 0; n < BTMIDI_MAX_PEERS; n++)
        if (runtime->peers[n] && runtime->peers[n]->in_use &&
            runtime->peers[n]->device == device) {
            peer_reset(runtime->peers[n]);
            runtime->peers[n]->in_use = FALSE;
        }
}

static void handle_event(struct btmidi_runtime *runtime, struct Message *message)
{
    IPTR event = 0, index = -1, length = 0;
    APTR record = NULL, device = NULL;
    UBYTE *value = NULL;
    struct btmidi_peer *peer;

    btGetAttrs(BGA_EVENTNOTE, message, BENA_EventID, &event,
               BENA_Param1, &record, BENA_Param2, &index,
               BENA_Data, &value, BENA_DataLength, &length,
               BENA_Device, &device, TAG_END);
    if (event == BEHMB_DEVICEDISCONNECTED) {
        forget_peer(runtime, record);   /* Param1 is the device */
    } else if (event == BEHMB_SERVICEWRITE && record == runtime->record &&
               index == 0 && value && length > 0 && length <= BTMIDI_VALUE_SIZE) {
        runtime->base->stats.ms_RxPackets++;
        if (!(peer = find_peer(runtime, device))) {
            runtime->base->stats.ms_Errors++;
        } else if (blemidi_check(&peer->decoder, value, (size_t)length)) {
            /* nothing of a malformed packet reaches CAMD */
            runtime->base->stats.ms_Errors++;
            peer_reset(peer);
        } else {
            blemidi_decode(&peer->decoder, value, (size_t)length,
                           peer_byte, peer);
            btmidi_camd_rx_flush(&runtime->rx);
        }
    }
    ReplyMsg(message);
}

/* At most one wake-up per refresh of the settings window. */
void btmidi_notify_gui(struct BTMidiBase *base)
{
    Forbid();
    if (base->gui_task && !base->activity_pending) {
        base->activity_pending = TRUE;
        Signal(base->gui_task, SIGBREAKF_CTRL_F);
    }
    Permit();
}

AROS_UFH0(void, btmidi_task)
{
    AROS_USERFUNC_INIT
    struct Task *task = FindTask(NULL);
    struct btmidi_runtime runtime;
    struct Message *message;
    ULONG signals, index;

    memset(&runtime, 0, sizeof(runtime));
    runtime.base = task->tc_UserData;
    runtime.adapter = &runtime.base->adapters[0];
    runtime.rx.adapter = runtime.adapter;
    blemidi_writer_init(&runtime.writer, set_packet, &runtime);
    runtime.tx_signal = AllocSignal(-1);
    if (!OpenDevice((CONST_STRPTR)"timer.device", UNIT_VBLANK,
                    (struct IORequest *)&runtime.timer_io, 0))
        runtime.timer = runtime.timer_io.tr_node.io_Device;
    BluetoothBase = OpenLibrary((CONST_STRPTR)"bluetooth.library", 45);
    if (BluetoothBase && (BluetoothBase->lib_Version == 45) &&
        (BluetoothBase->lib_Revision < 19)) {
        CloseLibrary(BluetoothBase);
        BluetoothBase = NULL;
    }
    if (BluetoothBase && runtime.tx_signal >= 0 &&
        (runtime.event_port = CreateMsgPort()) &&
        (runtime.record = add_service())) {
        runtime.event_handler = btAddEventHandler(runtime.event_port,
                                                   BEHMF_SERVICEWRITE |
                                                   BEHMF_DEVICEDISCONNECTED);
        if (runtime.event_handler) {
            /* CAMD finds the driver the next time it starts; until then,
               and without CAMD, the service runs and the window tells */
            btmidi_camd_install(runtime.base);
            btmidi_camd_reserve(runtime.base, 0, &runtime, "This computer");
            btmidi_camd_attach(runtime.adapter, task,
                               1UL << runtime.tx_signal);
            runtime.base->record = runtime.record;
            runtime.base->task = task;
        }
    }
    /* A failed start must remain private to this task until cleanup is done:
       libInit frees the base after the readiness signal. */
    if (runtime.base->task) {
        Forbid();
        if (runtime.base->ready_task)
            Signal(runtime.base->ready_task, 1UL << runtime.base->ready_signal);
        Permit();
    }
    if (runtime.base->task) {
        do {
            signals = Wait((1UL << runtime.event_port->mp_SigBit) |
                           (1UL << runtime.tx_signal) | SIGBREAKF_CTRL_C);
            while ((message = GetMsg(runtime.event_port)))
                handle_event(&runtime, message);
            if (signals & (1UL << runtime.tx_signal))
                send_to_ble(&runtime);
            btmidi_notify_gui(runtime.base);
        } while (!(signals & SIGBREAKF_CTRL_C));
    }
    if (runtime.base->task)
        btmidi_camd_release(runtime.base, 0);
    runtime.base->record = NULL;
    if (runtime.event_handler)
        btRemEventHandler(runtime.event_handler);
    while (runtime.event_port && (message = GetMsg(runtime.event_port)))
        ReplyMsg(message);
    if (runtime.record)
        btRemServiceRecord(runtime.record);
    for (index = 0; index < BTMIDI_MAX_PEERS; index++)
        FreeVec(runtime.peers[index]);
    if (runtime.event_port)
        DeleteMsgPort(runtime.event_port);
    if (BluetoothBase)
        CloseLibrary(BluetoothBase);
    if (runtime.tx_signal >= 0)
        FreeSignal(runtime.tx_signal);
    if (runtime.timer)
        CloseDevice((struct IORequest *)&runtime.timer_io);
    runtime.base->task = NULL;
    Forbid();
    if (runtime.base->ready_task)
        Signal(runtime.base->ready_task, 1UL << runtime.base->ready_signal);
    Permit();
    AROS_USERFUNC_EXIT
}

/* The functions below take the caller's bluetooth.library base: the global
   one belongs to the service task. */
#define BluetoothBase bluetooth

BOOL btmidi_open_cfg_window(struct BTMidiBase *base, struct Library *bluetooth)
{
    BOOL ok = FALSE;

    Forbid();
    if (base->gui_task)
        ok = TRUE;
    else if (base->task &&
             (base->gui_task = btSpawnSubTask((STRPTR)"btmidi.class GUI",
                                              (APTR)btmidi_gui_task, base)))
        ok = TRUE;
    Permit();
    return ok;
}

static int GM_UNIQUENAME(libInit)(LIBBASETYPEPTR base)
{
    struct Library *bluetooth;
    struct Task *task;
    base->utility_base = OpenLibrary((CONST_STRPTR)"utility.library", 39);
    if (!base->utility_base)
        return FALSE;
    bluetooth = OpenLibrary((CONST_STRPTR)"bluetooth.library", 45);
    if (!bluetooth || ((bluetooth->lib_Version == 45) &&
                       (bluetooth->lib_Revision < 19))) {
        if (bluetooth) CloseLibrary(bluetooth);
        CloseLibrary(base->utility_base);
        return FALSE;
    }
    btmidi_camd_init(base);
    base->ready_signal = SIGB_SINGLE;
    base->ready_task = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);
    task = btSpawnSubTask((STRPTR)"btmidi.class", (APTR)btmidi_task, base);
    if (task)
        btBorrowLocksWait(task, SIGF_SINGLE);
    base->ready_task = NULL;
    CloseLibrary(bluetooth);
    if (!base->task) {
        CloseLibrary(base->utility_base);
        return FALSE;
    }
    return TRUE;
}

static int GM_UNIQUENAME(libExpunge)(LIBBASETYPEPTR base)
{
    if (base->gui_task)
        return FALSE;
    if (base->task) {
        base->ready_signal = SIGB_SINGLE;
        base->ready_task = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);
        Signal(base->task, SIGBREAKF_CTRL_C);
        while (base->task) Wait(SIGF_SINGLE);
        base->ready_task = NULL;
    }
    btmidi_camd_free(base);
    CloseLibrary(base->utility_base);
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(libInit), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(libExpunge), 0)
#undef BluetoothBase

#define UtilityBase base->utility_base
AROS_LH3(LONG, btcGetAttrsA,
         AROS_LHA(ULONG, type, D0), AROS_LHA(APTR, object, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, base, 5, btmidi)
{
    AROS_LIBFUNC_INIT
    struct TagItem *tag;
    LONG count = 0;
    if (type == BCGA_CLASS) {
        if ((tag = FindTagItem(BCCA_Priority, tags))) {
            *((SIPTR *)tag->ti_Data) = 0; count++;
        }
        if ((tag = FindTagItem(BCCA_Description, tags))) {
            *((STRPTR *)tag->ti_Data) = (STRPTR)"BLE MIDI: devices and phones as ports of the CAMD driver btmidi"; count++;
        }
        if ((tag = FindTagItem(BCCA_HasClassCfgGUI, tags))) {
            *((IPTR *)tag->ti_Data) = TRUE; count++;
        }
        if ((tag = FindTagItem(BCCA_HasBindingCfgGUI, tags))) {
            *((IPTR *)tag->ti_Data) = FALSE; count++;
        }
        if ((tag = FindTagItem(BCCA_AfterDOSRestart, tags))) {
            *((IPTR *)tag->ti_Data) = TRUE; count++;
        }
        if ((tag = FindTagItem(BCCA_UsingDefaultCfg, tags))) {
            *((IPTR *)tag->ti_Data) = TRUE; count++;
        }
    } else if (type == BCGA_BINDING) {
        struct btmidi_binding *binding = object;
        if ((tag = FindTagItem(BCBA_UsingDefaultCfg, tags))) {
            *((IPTR *)tag->ti_Data) = TRUE; count++;
        }
        if ((tag = FindTagItem(BCBA_Device, tags))) {
            *((struct BtDevice **)tag->ti_Data) = binding->device; count++;
        }
        if ((tag = FindTagItem(BCBA_Service, tags))) {
            *((struct BtService **)tag->ti_Data) = binding->service; count++;
        }
        if ((tag = FindTagItem(BCBA_Task, tags))) {
            *((struct Task **)tag->ti_Data) = binding->task; count++;
        }
    }
    return count;
    AROS_LIBFUNC_EXIT
}

AROS_LH3(LONG, btcSetAttrsA,
         AROS_LHA(ULONG, type, D0), AROS_LHA(APTR, object, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, base, 6, btmidi)
{
    AROS_LIBFUNC_INIT
    (void)type; (void)object; (void)tags;
    return 0;
    AROS_LIBFUNC_EXIT
}
#undef UtilityBase

AROS_LH2(SIPTR, btcDoMethodA,
         AROS_LHA(ULONG, method, D0), AROS_LHA(IPTR *, data, A1),
         LIBBASETYPEPTR, base, 7, btmidi)
{
    AROS_LIBFUNC_INIT
    struct Library *bluetooth;
    SIPTR result = 0;

    switch (method) {
    case BCM_AttemptServiceBinding:
        return (SIPTR)btmidi_attempt_binding(base, (struct BtService *)data[0]);
    case BCM_ForceServiceBinding:
        return (SIPTR)btmidi_force_binding(base, (struct BtService *)data[0]);
    case BCM_ReleaseServiceBinding:
        btmidi_release_binding(base, (struct btmidi_binding *)data[0]);
        return TRUE;
    case BCM_OpenCfgWindow:
        if (!(bluetooth = OpenLibrary((CONST_STRPTR)"bluetooth.library", 45)))
            break;
        result = btmidi_open_cfg_window(base, bluetooth);
        CloseLibrary(bluetooth);
        break;
    }
    return result;
    AROS_LIBFUNC_EXIT
}
