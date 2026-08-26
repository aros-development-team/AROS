/*
 * RFCOMM multiplexer (client / initiator role) over one L2CAP channel
 * (PSM 0x0003). Transport agnostic: the owner passes outgoing frames to
 * the send callback and feeds every received L2CAP SDU to
 * bt_rfcomm_on_data(). One session per ACL link, multiplexing up to
 * BT_RFCOMM_MAX_DLCS data link connections (DLCs), with TS 07.10 parameter
 * negotiation, modem status exchange and credit based flow control.
 */

#ifndef BTCORE_RFCOMM_H
#define BTCORE_RFCOMM_H

#include <btcore/types.h>
#include <btcore/status.h>

#define BT_RFCOMM_MAX_DLCS       8
#define BT_RFCOMM_DEFAULT_MTU    127
#define BT_RFCOMM_MAX_MTU        1013
#define BT_RFCOMM_RX_CREDIT_MAX  16   /* receive credits we keep granted to the peer */

/* session events */
enum
{
    BT_RFCOMM_SESSION_UP = 1,     /* control channel established, DLCs may open */
    BT_RFCOMM_SESSION_DOWN        /* control channel refused or closed; all DLCs are gone */
};

/* DLC events */
enum
{
    BT_RFCOMM_DLC_OPEN = 1,       /* SABM accepted, data may flow */
    BT_RFCOMM_DLC_DATA,           /* data received (info/info_len of the event) */
    BT_RFCOMM_DLC_CLOSED,         /* closed by the peer or refused */
    BT_RFCOMM_DLC_CREDITS         /* transmit credits arrived; sending may resume */
};

struct bt_rfcomm_dlc_event
{
    uint8_t event;                /* BT_RFCOMM_DLC_xxx */
    uint8_t dlci;
    bool refused;                 /* CLOSED: the peer answered DM (never opened) */
    const uint8_t *data;          /* DATA */
    size_t data_len;
};

typedef bt_status_t (*bt_rfcomm_send_fn)(void *context, const uint8_t *frame, size_t len);
typedef void (*bt_rfcomm_session_fn)(void *context, uint8_t event);
typedef void (*bt_rfcomm_dlc_fn)(void *context, const struct bt_rfcomm_dlc_event *event);

struct bt_rfcomm_dlc
{
    uint8_t state;                /* internal */
    uint8_t dlci;
    uint8_t rx_credits;           /* granted to the peer, still unused */
    uint8_t peer_signals;         /* last modem status from the peer */
    bool cfc;                     /* credit based flow control on this DLC */
    bool msc_sent;
    bool msc_seen;
    uint16_t mtu;                 /* negotiated maximum frame size */
    int16_t tx_credits;           /* frames we may still send */
    bt_rfcomm_dlc_fn callback;
    void *user_data;
};

struct bt_rfcomm_session
{
    uint8_t state;                /* internal */
    uint16_t max_frame;           /* largest frame the transport can carry */
    bt_rfcomm_send_fn send;
    bt_rfcomm_session_fn event;
    void *context;
    struct bt_rfcomm_dlc dlcs[BT_RFCOMM_MAX_DLCS];
};

/* set up the session state; max_frame = largest RFCOMM frame that fits the
 * L2CAP channel (SDU size - 6), clamped to BT_RFCOMM_MAX_MTU */
void bt_rfcomm_init(struct bt_rfcomm_session *s, uint16_t max_frame,
                    bt_rfcomm_send_fn send, bt_rfcomm_session_fn event, void *context);

/* start the multiplexer (SABM on DLCI 0); BT_RFCOMM_SESSION_UP follows */
bt_status_t bt_rfcomm_start(struct bt_rfcomm_session *s);

/* tear everything down (DISC on DLCI 0) */
void bt_rfcomm_stop(struct bt_rfcomm_session *s);

bool bt_rfcomm_session_up(const struct bt_rfcomm_session *s);

/* open a DLC to the peer's server channel (1-30); the DLCI (channel << 1)
 * identifies it from here on. Session must be up. */
bt_status_t bt_rfcomm_open(struct bt_rfcomm_session *s, uint8_t server_channel,
                           bt_rfcomm_dlc_fn callback, void *user_data, uint8_t *dlci_out);

bt_status_t bt_rfcomm_close(struct bt_rfcomm_session *s, uint8_t dlci);

/* send one frame of data on an open DLC. BT_ERR_BUSY = out of transmit
 * credits (wait for BT_RFCOMM_DLC_CREDITS); BT_ERR_INVALID_ARGUMENT = frame
 * larger than bt_rfcomm_mtu(). */
bt_status_t bt_rfcomm_send(struct bt_rfcomm_session *s, uint8_t dlci,
                           const uint8_t *data, size_t len);

uint16_t bt_rfcomm_mtu(const struct bt_rfcomm_session *s, uint8_t dlci);
bool bt_rfcomm_can_send(const struct bt_rfcomm_session *s, uint8_t dlci);

/* feed one received L2CAP SDU from the RFCOMM channel */
void bt_rfcomm_on_data(struct bt_rfcomm_session *s, const uint8_t *data, size_t len);

#endif /* BTCORE_RFCOMM_H */
