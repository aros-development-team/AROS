#ifndef BTCORE_L2CAP_CHANNEL_H
#define BTCORE_L2CAP_CHANNEL_H

#include <btcore/l2cap.h>
#include <btcore/timer.h>
#include <btcore/transport.h>
#include <btcore/types.h>

/*
 * Connection-oriented L2CAP channel lifecycle (project.md, Fase 5), over
 * one ACL handle. One bt_l2cap_channel_manager per handle; it owns a
 * fixed pool of channels and the reassembler for that handle's inbound
 * ACL stream.
 *
 * Scope reductions from the full spec state diagram, documented rather
 * than silently assumed:
 *   - Only the initiator role is implemented: bt_l2cap_channel_manager_open()
 *     sends Connection Request. Responding to an incoming Connection
 *     Request (acceptor role) isn't implemented yet.
 *   - Configuration is single-round-trip: each side sends one Configure
 *     Request and accepts whatever the peer proposes (MTU only, per
 *     btcore/l2cap.h's own scope reduction). The full spec allows
 *     multi-round negotiation loops; not modeled here.
 *   - Two independent booleans (outbound/inbound config done) stand in
 *     for the spec's ~8-state configuration sub-state diagram.
 *   - bt_l2cap_channel_manager_send() caps payload at BT_L2CAP_MAX_SEND_LEN
 *     (a fixed stack buffer, no dynamic allocation).
 */

#ifndef BT_L2CAP_CHANNEL_MANAGER_MAX_CHANNELS
#define BT_L2CAP_CHANNEL_MANAGER_MAX_CHANNELS 8
#endif

#ifndef BT_L2CAP_RTX_TIMEOUT_US
#define BT_L2CAP_RTX_TIMEOUT_US 60000000ull /* 60s: spec's default RTX timeout */
#endif

#ifndef BT_L2CAP_DEFAULT_MTU
#define BT_L2CAP_DEFAULT_MTU 672 /* spec default when a peer doesn't negotiate one */
#endif

#ifndef BT_L2CAP_MAX_ACL_FRAGMENT
#define BT_L2CAP_MAX_ACL_FRAGMENT 512
#endif

#ifndef BT_L2CAP_MAX_SEND_LEN
#define BT_L2CAP_MAX_SEND_LEN 1700 /* big enough for a BNEP-wrapped ethernet frame */
#endif

enum bt_l2cap_channel_state
{
    BT_L2CAP_CHAN_FREE, /* slot unused */
    BT_L2CAP_CHAN_WAIT_CONNECT_RSP,
    BT_L2CAP_CHAN_CONFIG,
    BT_L2CAP_CHAN_OPEN,
    BT_L2CAP_CHAN_WAIT_DISCONNECT_RSP
};

enum bt_l2cap_close_reason
{
    BT_L2CAP_CLOSE_LOCAL,             /* bt_l2cap_channel_manager_close() was called */
    BT_L2CAP_CLOSE_PEER_DISCONNECTED,
    BT_L2CAP_CLOSE_REFUSED,           /* peer rejected the Connection Request */
    BT_L2CAP_CLOSE_CONFIG_FAILED,
    BT_L2CAP_CLOSE_TIMEOUT
};

enum bt_l2cap_channel_event
{
    BT_L2CAP_CHANNEL_EVENT_OPENED,
    BT_L2CAP_CHANNEL_EVENT_CLOSED,
    BT_L2CAP_CHANNEL_EVENT_DATA
};

struct bt_l2cap_channel_event_info
{
    enum bt_l2cap_channel_event event;
    uint16_t local_cid;
    enum bt_l2cap_close_reason close_reason; /* meaningful only for CLOSED */
    const uint8_t *data;                     /* meaningful only for DATA */
    size_t data_len;
    uint64_t now_us; /* meaningful only for DATA; supplied by on_acl() */
};

typedef void (*bt_l2cap_channel_event_fn)(struct bt_l2cap_channel_event_info *info,
                                           void *user_data);

struct bt_l2cap_channel_manager;

struct bt_l2cap_channel
{
    struct bt_l2cap_channel_manager *owner;
    enum bt_l2cap_channel_state state;
    bool is_fixed; /* registered via open_fixed(): no handshake, no on-wire teardown */
    uint16_t psm;
    uint16_t local_cid;
    uint16_t remote_cid;
    uint16_t local_mtu;
    uint16_t remote_mtu;
    bool outbound_config_done; /* our Configure Request was accepted */
    bool inbound_config_done;  /* we accepted the peer's Configure Request */
    uint8_t pending_identifier; /* nonzero while we have a signaling request outstanding */
    struct bt_timer rtx_timer;
    bt_l2cap_channel_event_fn on_event;
    void *user_data;
};

#ifndef BT_L2CAP_CHANNEL_MANAGER_MAX_LISTENERS
#define BT_L2CAP_CHANNEL_MANAGER_MAX_LISTENERS 8
#endif

/* Acceptor role: a PSM registered with bt_l2cap_channel_manager_listen()
 * accepts incoming Connection Requests; the resulting channel reports
 * OPENED/DATA/CLOSED to the listener's callback like a locally opened one. */
struct bt_l2cap_listener
{
    uint16_t psm;               /* 0 = slot unused */
    uint16_t local_mtu;
    bt_l2cap_channel_event_fn on_event;
    void *user_data;
};

struct bt_l2cap_channel_manager
{
    struct bt_hci_transport *transport;
    struct bt_timer_list timers; /* private to this manager -- see bt_controller for why */
    uint16_t handle;
    uint16_t signaling_cid;
    size_t acl_frag_size;
    struct bt_l2cap_reassembler reassembler;
    struct bt_l2cap_channel channels[BT_L2CAP_CHANNEL_MANAGER_MAX_CHANNELS];
    struct bt_l2cap_listener listeners[BT_L2CAP_CHANNEL_MANAGER_MAX_LISTENERS];
    uint16_t next_local_cid;
    uint8_t next_identifier;
};

/* signaling_cid is BT_L2CAP_CID_SIGNALING_CLASSIC or BT_L2CAP_CID_SIGNALING_LE
 * depending on the link type this handle belongs to. acl_frag_size is the
 * controller's negotiated ACL data length (from HCI Read Buffer Size),
 * clamped to BT_L2CAP_MAX_ACL_FRAGMENT. */
void bt_l2cap_channel_manager_init(struct bt_l2cap_channel_manager *mgr,
                                    struct bt_hci_transport *transport, uint16_t handle,
                                    uint16_t signaling_cid, size_t acl_frag_size);

/* Initiates a connection-oriented channel to the given PSM: sends
 * Connection Request immediately. *out_local_cid identifies the channel
 * for every later call. Fails only if the channel pool is full. */
bt_status_t bt_l2cap_channel_manager_open(struct bt_l2cap_channel_manager *mgr, uint16_t psm,
                                           uint16_t local_mtu, bt_l2cap_channel_event_fn on_event,
                                           void *user_data, uint16_t *out_local_cid,
                                           uint64_t now_us);

/* Accepts incoming Connection Requests for psm (acceptor role). The channel
 * created for such a request fires OPENED (once configured), DATA and
 * CLOSED on on_event/user_data. Fails when the listener table is full. A
 * Connection Request for a PSM nobody listens on is refused
 * (BT_L2CAP_CONN_RESULT_REFUSED_PSM). */
bt_status_t bt_l2cap_channel_manager_listen(struct bt_l2cap_channel_manager *mgr, uint16_t psm,
                                             uint16_t local_mtu, bt_l2cap_channel_event_fn on_event,
                                             void *user_data);
void bt_l2cap_channel_manager_unlisten(struct bt_l2cap_channel_manager *mgr, uint16_t psm);

/* Registers a fixed channel (e.g. BT_L2CAP_CID_ATT) as immediately OPEN
 * -- fixed channels exist for as long as the ACL link does, with no
 * Connection Request/Response or configuration handshake (per spec, not
 * a shortcut taken here). local_cid == remote_cid == cid always, since
 * fixed CIDs are the same value on both ends by definition. Fails if the
 * pool is full or cid is already registered on this manager. Fires
 * BT_L2CAP_CHANNEL_EVENT_OPENED synchronously before returning, so
 * callers can treat it uniformly with the async dynamic-channel path. */
bt_status_t bt_l2cap_channel_manager_open_fixed(struct bt_l2cap_channel_manager *mgr, uint16_t cid,
                                                 bt_l2cap_channel_event_fn on_event,
                                                 void *user_data);

/* If OPEN, sends Disconnection Request and waits for the response. If
 * still connecting/configuring, aborts locally right away (project.md's
 * "remoção durante negociação") -- there is no on-the-wire teardown
 * defined for a channel that never finished opening, so none is sent.
 * Either way the channel slot is freed and a CLOSED event fires (with
 * BT_L2CAP_CLOSE_LOCAL) unless it was already FREE. */
void bt_l2cap_channel_manager_close(struct bt_l2cap_channel_manager *mgr, uint16_t local_cid,
                                     uint64_t now_us);

/* Sends data on an OPEN channel, fragmenting over ACL as needed. len must
 * be <= BT_L2CAP_MAX_SEND_LEN. */
bt_status_t bt_l2cap_channel_manager_send(struct bt_l2cap_channel_manager *mgr, uint16_t local_cid,
                                           const uint8_t *data, size_t len, uint64_t now_us);

/* Feed one HCI ACL Data fragment's payload in (pb_flag from the ACL
 * header, plus the bytes after it). Reassembles internally; once a full
 * L2CAP PDU is available, routes it to signaling processing or to the
 * matching channel's DATA event. */
void bt_l2cap_channel_manager_on_acl(struct bt_l2cap_channel_manager *mgr, uint8_t pb_flag,
                                      const uint8_t *data, size_t len, uint64_t now_us);

/* Fires RTX timeouts for anything overdue. Call whenever the shared timer
 * list has something due. */
void bt_l2cap_channel_manager_tick(struct bt_l2cap_channel_manager *mgr, uint64_t now_us);

#endif /* BTCORE_L2CAP_CHANNEL_H */
