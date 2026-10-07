/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* BLE MIDI GATT service and CAMD bridge for bluetooth.library. */

#include "btmidi.h"
#include "blemidi.h"
#include "camdbridge.h"
#include <exec/memory.h>
#include <proto/bluetooth.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <string.h>
#include <proto/timer.h>

#define BTMIDI_VALUE_SIZE 512
#define BTMIDI_TX_PACKET_MIN 20    /* fits the default ATT MTU */
#define BTMIDI_TX_PACKET_MAX 244   /* the stack's largest ATT MTU, less 3 */
#define BTMIDI_MAX_PEERS 4
#define BTMIDI_CONN_INTERVAL 12   /* 15 ms, as the BLE MIDI specification asks */

struct Library *BluetoothBase;

struct btmidi_runtime;

/* What a central has sent so far. Each connected central has its own: their
   packets interleave, and running status and SysEx span packets. */
struct btmidi_peer {
    struct btmidi_runtime *runtime;
    BOOL in_use;
    APTR device;              /* the writer, or NULL from an older stack */
    ULONG last_use;
    struct blemidi_stream stream;
};

struct btmidi_runtime {
    struct BTMidiBase *base;
    char node_name[BTMIDI_NAME_SIZE];
    char in_name[BTMIDI_NAME_SIZE];
    char out_name[BTMIDI_NAME_SIZE];
    struct MsgPort *event_port;
    APTR event_handler;
    APTR record;
    struct btmidi_camd camd;
    struct btmidi_peer *peers[BTMIDI_MAX_PEERS];
    ULONG use_counter;
    ULONG tx_payload;         /* what every connected central receives whole */
    struct timerequest timer_io;  /* only opened, for its base: the clock */
    struct Device *timer;
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

static void deliver(void *context, const uint8_t *message, size_t length)
{
    struct btmidi_peer *peer = context;
    struct btmidi_runtime *runtime = peer->runtime;

    if (message[0] == 0xf0)
        btmidi_camd_deliver_sysex(&runtime->camd, message, length);
    else
        btmidi_camd_deliver(&runtime->camd, message, length);
    runtime->base->stats.ms_RxMessages++;
}

static int set_packet(struct btmidi_runtime *runtime,
                      const UBYTE *packet, ULONG length)
{
    if (btSetServiceValue(runtime->record, 0, (APTR)packet, length) != (LONG)length) {
        runtime->base->stats.ms_Errors++;
        return -1;
    }
    runtime->base->stats.ms_TxPackets++;
    return 0;
}

static int send_to_ble(void *context, const uint8_t *message, size_t length)
{
    struct btmidi_runtime *runtime = context;
    UBYTE packet[BTMIDI_TX_PACKET_MAX];
    size_t written;
    UWORD timestamp = btmidi_now_ms(runtime->timer);

    runtime->base->stats.ms_TxMessages++;

    if (length >= 2 && message[0] == 0xf0 && message[length - 1] == 0xf7) {
        size_t offset = 0;
        while (offset < length) {
            if (blemidi_encode_sysex_chunk(message, length, &offset,
                                                timestamp, packet,
                                                runtime->tx_payload, &written) ||
                set_packet(runtime, packet, written))
                return -1;
        }
        return 0;
    }
    if (blemidi_encode_message(message, length, timestamp, packet,
                                   runtime->tx_payload, &written))
        return -1;
    return set_packet(runtime, packet, written);
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
    blemidi_stream_init(&peer->stream, deliver, peer);
found:
    peer->last_use = ++runtime->use_counter;
    return peer;
}

static void forget_peer(struct btmidi_runtime *runtime, APTR device)
{
    ULONG n;

    for (n = 0; n < BTMIDI_MAX_PEERS; n++)
        if (runtime->peers[n] && runtime->peers[n]->in_use &&
            runtime->peers[n]->device == device)
            runtime->peers[n]->in_use = FALSE;
}

static void reset_peers(struct btmidi_runtime *runtime)
{
    ULONG n;

    for (n = 0; n < BTMIDI_MAX_PEERS; n++)
        if (runtime->peers[n])
            runtime->peers[n]->in_use = FALSE;
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
        if (!(peer = find_peer(runtime, device)) ||
            blemidi_stream_feed(&peer->stream, value, (size_t)length))
            runtime->base->stats.ms_Errors++;
    }
    ReplyMsg(message);
}

/* Opens the CAMD node under the configured names. Should CAMD refuse them,
   the defaults keep the service usable. */
static void open_camd(struct btmidi_runtime *runtime)
{
    struct BTMidiBase *base = runtime->base;
    enum btmidi_camd_state state = BTMIDI_CAMD_OPEN;

    Forbid();
    CopyMem(base->cfg.mc_NodeName, runtime->node_name, BTMIDI_NAME_SIZE);
    CopyMem(base->cfg.mc_InName, runtime->in_name, BTMIDI_NAME_SIZE);
    CopyMem(base->cfg.mc_OutName, runtime->out_name, BTMIDI_NAME_SIZE);
    Permit();
    if (btmidi_camd_open_named(&runtime->camd, runtime->node_name,
                                  runtime->in_name, runtime->out_name)) {
        strcpy(runtime->node_name, BTMIDI_DEFAULT_NODE);
        strcpy(runtime->in_name, BTMIDI_DEFAULT_IN);
        strcpy(runtime->out_name, BTMIDI_DEFAULT_OUT);
        state = btmidi_camd_open_named(&runtime->camd, runtime->node_name,
                                          runtime->in_name, runtime->out_name)
                    ? BTMIDI_CAMD_CLOSED : BTMIDI_CAMD_DEFAULTS;
    }
    base->camd_state = state;
}

static BOOL camd_names_changed(struct btmidi_runtime *runtime)
{
    BOOL changed;

    Forbid();
    changed = memcmp(runtime->node_name, runtime->base->cfg.mc_NodeName,
                     BTMIDI_NAME_SIZE) ||
              memcmp(runtime->in_name, runtime->base->cfg.mc_InName,
                     BTMIDI_NAME_SIZE) ||
              memcmp(runtime->out_name, runtime->base->cfg.mc_OutName,
                     BTMIDI_NAME_SIZE);
    Permit();
    return changed;
}

/* At most one wake-up per refresh of the settings window. */
static void notify_gui(struct BTMidiBase *base)
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
    runtime.camd.signal_bit = -1;
    if (!OpenDevice((CONST_STRPTR)"timer.device", UNIT_VBLANK,
                    (struct IORequest *)&runtime.timer_io, 0))
        runtime.timer = runtime.timer_io.tr_node.io_Device;
    BluetoothBase = OpenLibrary((CONST_STRPTR)"bluetooth.library", 45);
    if (BluetoothBase && (BluetoothBase->lib_Version == 45) &&
        (BluetoothBase->lib_Revision < 19)) {
        CloseLibrary(BluetoothBase);
        BluetoothBase = NULL;
    }
    if (BluetoothBase && (runtime.event_port = CreateMsgPort()) &&
        (runtime.record = add_service())) {
        /* without CAMD the service still runs; the window tells */
        open_camd(&runtime);
        runtime.event_handler = btAddEventHandler(runtime.event_port,
                                                   BEHMF_SERVICEWRITE |
                                                   BEHMF_DEVICEDISCONNECTED);
        if (runtime.event_handler) {
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
            ULONG mask = (1UL << runtime.event_port->mp_SigBit) |
                         SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E;

            if (runtime.camd.signal_bit >= 0)
                mask |= 1UL << runtime.camd.signal_bit;
            signals = Wait(mask);
            while ((message = GetMsg(runtime.event_port)))
                handle_event(&runtime, message);
            if ((runtime.camd.signal_bit >= 0) &&
                (signals & (1UL << runtime.camd.signal_bit))) {
                /* larger packets once every central has negotiated a
                   larger ATT MTU: SysEx needs fewer of them */
                runtime.tx_payload = BTMIDI_TX_PACKET_MIN;
                btGetAttrs(BGA_STACK, NULL, BSA_LENotifyPayload,
                           &runtime.tx_payload, TAG_END);
                if (runtime.tx_payload < BTMIDI_TX_PACKET_MIN)
                    runtime.tx_payload = BTMIDI_TX_PACKET_MIN;
                if (runtime.tx_payload > BTMIDI_TX_PACKET_MAX)
                    runtime.tx_payload = BTMIDI_TX_PACKET_MAX;
                btmidi_camd_poll(&runtime.camd, send_to_ble, &runtime);
            }
            if ((signals & SIGBREAKF_CTRL_E) && camd_names_changed(&runtime)) {
                /* new port names: CAMD clients reconnect by name */
                btmidi_camd_close(&runtime.camd);
                reset_peers(&runtime);
                open_camd(&runtime);
            }
            notify_gui(runtime.base);
        } while (!(signals & SIGBREAKF_CTRL_C));
    }
    runtime.base->record = NULL;
    runtime.base->camd_state = BTMIDI_CAMD_CLOSED;
    if (runtime.event_handler)
        btRemEventHandler(runtime.event_handler);
    while (runtime.event_port && (message = GetMsg(runtime.event_port)))
        ReplyMsg(message);
    if (runtime.record)
        btRemServiceRecord(runtime.record);
    btmidi_camd_close(&runtime.camd);
    for (index = 0; index < BTMIDI_MAX_PEERS; index++)
        FreeVec(runtime.peers[index]);
    if (runtime.event_port)
        DeleteMsgPort(runtime.event_port);
    if (BluetoothBase)
        CloseLibrary(BluetoothBase);
    if (runtime.timer)
        CloseDevice((struct IORequest *)&runtime.timer_io);
    runtime.base->task = NULL;
    Forbid();
    if (runtime.base->ready_task)
        Signal(runtime.base->ready_task, 1UL << runtime.base->ready_signal);
    Permit();
    AROS_USERFUNC_EXIT
}

/* The configuration lives with the other Bluetooth class settings. The
   functions below take the caller's bluetooth.library base: the global one
   belongs to the service task. */
#define BluetoothBase bluetooth

static void terminate_name(char *name, const char *fallback)
{
    name[BTMIDI_NAME_SIZE - 1] = 0;
    if (!name[0])
        strcpy(name, fallback);
}

void btmidi_default_cfg(struct BTMidiCfg *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->mc_ChunkID = AROS_LONG2BE(MAKE_ID('B','M','I','D'));
    cfg->mc_Length = AROS_LONG2BE(sizeof(*cfg) - 8);
    strcpy(cfg->mc_NodeName, BTMIDI_DEFAULT_NODE);
    strcpy(cfg->mc_InName, BTMIDI_DEFAULT_IN);
    strcpy(cfg->mc_OutName, BTMIDI_DEFAULT_OUT);
}

void btmidi_load_cfg(struct BTMidiBase *base, struct Library *bluetooth)
{
    struct BTMidiCfg *cfg = &base->cfg;
    struct BTMidiCfg *chunk;
    APTR pic;

    Forbid();
    btmidi_default_cfg(cfg);
    base->using_default_cfg = TRUE;
    if ((pic = btGetClsCfg((STRPTR)MOD_NAME_STRING)) &&
        (chunk = btGetCfgChunk(pic, AROS_LONG2BE(cfg->mc_ChunkID)))) {
        ULONG len = AROS_LONG2BE(chunk->mc_Length);

        if (len > sizeof(*cfg) - 8)
            len = sizeof(*cfg) - 8;
        CopyMem(((UBYTE *)chunk) + 8, ((UBYTE *)cfg) + 8, len);
        btFreeVec(chunk);
        terminate_name(cfg->mc_NodeName, BTMIDI_DEFAULT_NODE);
        terminate_name(cfg->mc_InName, BTMIDI_DEFAULT_IN);
        terminate_name(cfg->mc_OutName, BTMIDI_DEFAULT_OUT);
        base->using_default_cfg = FALSE;
    }
    Permit();
}

void btmidi_store_cfg(struct BTMidiBase *base, struct Library *bluetooth,
                      BOOL to_disk)
{
    APTR pic;

    if (!(pic = btGetClsCfg((STRPTR)MOD_NAME_STRING))) {
        btSetClsCfg((STRPTR)MOD_NAME_STRING, NULL);
        pic = btGetClsCfg((STRPTR)MOD_NAME_STRING);
    }
    if (pic && btAddCfgEntry(pic, &base->cfg)) {
        base->using_default_cfg = FALSE;
        if (to_disk)
            btSaveCfgToDisk(NULL, FALSE);
    }
}

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

void btmidi_reconfigure(struct BTMidiBase *base)
{
    Forbid();
    if (base->task)
        Signal(base->task, SIGBREAKF_CTRL_E);
    Permit();
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
    btmidi_load_cfg(base, bluetooth);
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
            *((STRPTR *)tag->ti_Data) = (STRPTR)"BLE MIDI: devices and phones as CAMD ports"; count++;
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
            *((IPTR *)tag->ti_Data) = base->using_default_cfg; count++;
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
    case BCM_ConfigChangedEvent:
        if (!(bluetooth = OpenLibrary((CONST_STRPTR)"bluetooth.library", 45)))
            break;
        if (method == BCM_OpenCfgWindow) {
            result = btmidi_open_cfg_window(base, bluetooth);
        } else if (!base->gui_task) {
            /* not while the window edits it; its Use and Save apply it */
            btmidi_load_cfg(base, bluetooth);
            btmidi_reconfigure(base);
            result = TRUE;
        }
        CloseLibrary(bluetooth);
        break;
    }
    return result;
    AROS_LIBFUNC_EXIT
}
