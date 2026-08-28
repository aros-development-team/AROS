/*
 * RFCOMM multiplexer, initiator and responder roles. See btcore/rfcomm.h.
 *
 * Frames are TS 07.10 basic option without the flag octets (L2CAP keeps the
 * frame boundaries): address, control, length (EA coded), information, FCS.
 * The initiator's commands carry C/R = 1 and its responses C/R = 0; the
 * responder's the other way round. A DLC to a server channel on the
 * responder has direction bit 0 (DLCI = server channel << 1) whoever looks
 * at it, so the initiator opening our channel and us opening the peer's
 * use the same DLCI formula.
 */

#include <btcore/rfcomm.h>

#include <string.h>

/* control field values (P/F bit included where we set it) */
#define RF_SABM   0x3f
#define RF_UA     0x73
#define RF_DM     0x0f
#define RF_DM_F   0x1f
#define RF_DISC   0x53
#define RF_UIH    0xef
#define RF_UIH_PF 0xff

/* multiplexer control commands (type field without EA/CR bits) */
#define RF_MCC_PN   0x20
#define RF_MCC_TEST 0x08
#define RF_MCC_FCON 0x28
#define RF_MCC_FCOFF 0x18
#define RF_MCC_MSC  0x38
#define RF_MCC_RPN  0x24
#define RF_MCC_RLS  0x14
#define RF_MCC_NSC  0x04

/* session states */
enum { SES_CLOSED, SES_WAIT_UA, SES_UP, SES_CLOSING };
/* DLC states */
enum { DLC_CLOSED, DLC_WAIT_PN, DLC_WAIT_UA, DLC_OPEN, DLC_CLOSING, DLC_WAIT_SABM };

#define MSC_SIGNALS 0x8d   /* EA | RTC | RTR | DV */

/* reversed CRC-8 of TS 07.10 (x^8 + x^2 + x + 1) */
static uint8_t crc_table[256];
static bool crc_ready;

static void crc_setup(void)
{
    unsigned i, j;
    for(i = 0; i < 256; i++)
    {
        uint8_t data = (uint8_t) i;
        for(j = 0; j < 8; j++)
            data = (data & 1) ? (uint8_t) ((data >> 1) ^ 0xe0) : (uint8_t) (data >> 1);
        crc_table[i] = data;
    }
    crc_ready = true;
}

static uint8_t rf_fcs(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xff;
    if(!crc_ready)
        crc_setup();
    while(len--)
        crc = crc_table[crc ^ *data++];
    return (uint8_t) (0xff - crc);
}

static struct bt_rfcomm_dlc *dlc_by_dlci(struct bt_rfcomm_session *s, uint8_t dlci)
{
    unsigned i;
    for(i = 0; i < BT_RFCOMM_MAX_DLCS; i++)
        if((s->dlcs[i].state != DLC_CLOSED) && (s->dlcs[i].dlci == dlci))
            return &s->dlcs[i];
    return NULL;
}

/* a non-UIH frame: address, control, length 0, FCS over the first three */
static bt_status_t send_ctrl(struct bt_rfcomm_session *s, uint8_t dlci, uint8_t control, bool cmd)
{
    uint8_t f[4];
    bool cr = s->responder ? !cmd : cmd;
    f[0] = (uint8_t) ((dlci << 2) | (cr ? 0x03 : 0x01));
    f[1] = control;
    f[2] = 0x01;
    f[3] = rf_fcs(f, 3);
    return s->send(s->context, f, 4);
}

/* a UIH frame; credits >= 0 puts the P/F credit octet in front of the data */
static bt_status_t send_uih(struct bt_rfcomm_session *s, uint8_t dlci,
                            int credits, const uint8_t *data, size_t len)
{
    uint8_t f[BT_RFCOMM_MAX_MTU + 7];
    size_t pos = 0;

    if(len > BT_RFCOMM_MAX_MTU)
        return BT_ERR_INVALID_ARGUMENT;
    f[pos++] = (uint8_t) ((dlci << 2) | (s->responder ? 0x01 : 0x03));   /* UIH is a command */
    f[pos++] = (credits >= 0) ? RF_UIH_PF : RF_UIH;
    if(len > 127)
    {
        f[pos++] = (uint8_t) ((len & 0x7f) << 1);
        f[pos++] = (uint8_t) (len >> 7);
    } else {
        f[pos++] = (uint8_t) ((len << 1) | 1);
    }
    if(credits >= 0)
        f[pos++] = (uint8_t) credits;
    if(len)
    {
        memcpy(&f[pos], data, len);
        pos += len;
    }
    f[pos] = rf_fcs(f, 2);   /* UIH: FCS over address and control only */
    pos++;
    return s->send(s->context, f, pos);
}

/* multiplexer control message on DLCI 0 */
static bt_status_t send_mcc(struct bt_rfcomm_session *s, uint8_t type, bool cmd,
                            const uint8_t *value, size_t len)
{
    uint8_t buf[2 + 24];
    if(len > sizeof(buf) - 2)
        return BT_ERR_INVALID_ARGUMENT;
    buf[0] = (uint8_t) ((type << 2) | (cmd ? 0x02 : 0x00) | 0x01);
    buf[1] = (uint8_t) ((len << 1) | 1);
    if(len)
        memcpy(&buf[2], value, len);
    return send_uih(s, 0, -1, buf, len + 2);
}

static bt_status_t send_pn(struct bt_rfcomm_session *s, struct bt_rfcomm_dlc *d, bool cmd)
{
    uint8_t pn[8];
    pn[0] = d->dlci;
    pn[1] = cmd ? 0xf0 : 0xe0;               /* request / accept credit based flow */
    pn[2] = 0;                               /* priority */
    pn[3] = 0;                               /* ack timer (not negotiable) */
    pn[4] = (uint8_t) (d->mtu & 0xff);
    pn[5] = (uint8_t) (d->mtu >> 8);
    pn[6] = 0;                               /* max retransmissions */
    pn[7] = cmd ? 7 : d->rx_credits;         /* initial credits for the peer */
    return send_mcc(s, RF_MCC_PN, cmd, pn, 8);
}

static bt_status_t send_msc(struct bt_rfcomm_session *s, struct bt_rfcomm_dlc *d,
                            bool cmd, uint8_t signals)
{
    uint8_t msc[2];
    msc[0] = (uint8_t) ((d->dlci << 2) | 0x03);
    msc[1] = signals;
    return send_mcc(s, RF_MCC_MSC, cmd, msc, 2);
}

static void dlc_event(struct bt_rfcomm_session *s, struct bt_rfcomm_dlc *d,
                      uint8_t event, bool refused, const uint8_t *data, size_t len)
{
    struct bt_rfcomm_dlc_event ev;
    (void) s;
    if(!d->callback)
        return;
    memset(&ev, 0, sizeof(ev));
    ev.event = event;
    ev.dlci = d->dlci;
    ev.refused = refused;
    ev.data = data;
    ev.data_len = len;
    d->callback(d->user_data, &ev);
}

static void dlc_down(struct bt_rfcomm_session *s, struct bt_rfcomm_dlc *d, bool refused)
{
    if(d->state == DLC_CLOSED)
        return;
    d->state = DLC_CLOSED;
    dlc_event(s, d, BT_RFCOMM_DLC_CLOSED, refused, NULL, 0);
}

static void session_down(struct bt_rfcomm_session *s)
{
    unsigned i;
    if(s->state == SES_CLOSED)
        return;
    s->state = SES_CLOSED;
    for(i = 0; i < BT_RFCOMM_MAX_DLCS; i++)
        dlc_down(s, &s->dlcs[i], false);
    if(s->event)
        s->event(s->context, BT_RFCOMM_SESSION_DOWN);
}

void bt_rfcomm_init(struct bt_rfcomm_session *s, uint16_t max_frame,
                    bt_rfcomm_send_fn send, bt_rfcomm_session_fn event, void *context)
{
    memset(s, 0, sizeof(*s));
    if(max_frame < 23)
        max_frame = 23;
    if(max_frame > BT_RFCOMM_MAX_MTU)
        max_frame = BT_RFCOMM_MAX_MTU;
    s->max_frame = max_frame;
    s->send = send;
    s->event = event;
    s->context = context;
}

void bt_rfcomm_init_responder(struct bt_rfcomm_session *s, uint16_t max_frame,
                              bt_rfcomm_send_fn send, bt_rfcomm_session_fn event,
                              bt_rfcomm_accept_fn accept, void *context)
{
    bt_rfcomm_init(s, max_frame, send, event, context);
    s->responder = true;
    s->accept = accept;
}

/* responder: the peer asks for a DLC to one of our server channels */
static struct bt_rfcomm_dlc *accept_dlc(struct bt_rfcomm_session *s, uint8_t dlci)
{
    struct bt_rfcomm_dlc *d = NULL;
    bt_rfcomm_dlc_fn cb = NULL;
    void *ud = NULL;
    unsigned i;

    if(!s->accept || !s->accept(s->context, (uint8_t) (dlci >> 1), &cb, &ud))
        return NULL;
    for(i = 0; i < BT_RFCOMM_MAX_DLCS; i++)
    {
        if(s->dlcs[i].state == DLC_CLOSED)
        {
            d = &s->dlcs[i];
            break;
        }
    }
    if(!d)
        return NULL;
    memset(d, 0, sizeof(*d));
    d->dlci = dlci;
    d->mtu = s->max_frame;
    d->callback = cb;
    d->user_data = ud;
    d->state = DLC_WAIT_SABM;
    return d;
}

bt_status_t bt_rfcomm_start(struct bt_rfcomm_session *s)
{
    bt_status_t st;
    if(s->state != SES_CLOSED)
        return BT_ERR_INVALID_STATE;
    st = send_ctrl(s, 0, RF_SABM, true);
    if(st == BT_OK)
        s->state = SES_WAIT_UA;
    return st;
}

void bt_rfcomm_stop(struct bt_rfcomm_session *s)
{
    if(s->state == SES_UP)
        send_ctrl(s, 0, RF_DISC, true);
    session_down(s);
}

bool bt_rfcomm_session_up(const struct bt_rfcomm_session *s)
{
    return s->state == SES_UP;
}

bt_status_t bt_rfcomm_open(struct bt_rfcomm_session *s, uint8_t server_channel,
                           bt_rfcomm_dlc_fn callback, void *user_data, uint8_t *dlci_out)
{
    struct bt_rfcomm_dlc *d = NULL;
    unsigned i;
    bt_status_t st;

    if((server_channel < 1) || (server_channel > 30))
        return BT_ERR_INVALID_ARGUMENT;
    if(s->state != SES_UP)
        return BT_ERR_INVALID_STATE;
    if(dlc_by_dlci(s, (uint8_t) ((server_channel << 1) | (s->responder ? 1 : 0))))
        return BT_ERR_ALREADY;
    for(i = 0; i < BT_RFCOMM_MAX_DLCS; i++)
    {
        if(s->dlcs[i].state == DLC_CLOSED)
        {
            d = &s->dlcs[i];
            break;
        }
    }
    if(!d)
        return BT_ERR_NO_RESOURCES;
    memset(d, 0, sizeof(*d));
    d->dlci = (uint8_t) ((server_channel << 1) | (s->responder ? 1 : 0));   /* channels on the initiator: D = 1 */
    d->mtu = s->max_frame;
    d->callback = callback;
    d->user_data = user_data;
    st = send_pn(s, d, true);
    if(st != BT_OK)
        return st;
    d->state = DLC_WAIT_PN;
    if(dlci_out)
        *dlci_out = d->dlci;
    return BT_OK;
}

bt_status_t bt_rfcomm_close(struct bt_rfcomm_session *s, uint8_t dlci)
{
    struct bt_rfcomm_dlc *d = dlc_by_dlci(s, dlci);
    if(!d)
        return BT_ERR_NOT_FOUND;
    if(d->state == DLC_OPEN)
    {
        d->state = DLC_CLOSING;
        return send_ctrl(s, dlci, RF_DISC, true);
    }
    dlc_down(s, d, false);
    return BT_OK;
}

uint16_t bt_rfcomm_mtu(const struct bt_rfcomm_session *s, uint8_t dlci)
{
    const struct bt_rfcomm_dlc *d = dlc_by_dlci((struct bt_rfcomm_session *) s, dlci);
    return d ? d->mtu : 0;
}

bool bt_rfcomm_can_send(const struct bt_rfcomm_session *s, uint8_t dlci)
{
    const struct bt_rfcomm_dlc *d = dlc_by_dlci((struct bt_rfcomm_session *) s, dlci);
    if(!d || (d->state != DLC_OPEN))
        return false;
    return !d->cfc || (d->tx_credits > 0);
}

bt_status_t bt_rfcomm_send(struct bt_rfcomm_session *s, uint8_t dlci,
                           const uint8_t *data, size_t len)
{
    struct bt_rfcomm_dlc *d = dlc_by_dlci(s, dlci);
    bt_status_t st;
    int grant = -1;

    if(!d || (d->state != DLC_OPEN))
        return BT_ERR_INVALID_STATE;
    if(len > d->mtu)
        return BT_ERR_INVALID_ARGUMENT;
    if(d->cfc && (d->tx_credits <= 0))
        return BT_ERR_BUSY;
    if(d->cfc && (d->rx_credits <= BT_RFCOMM_RX_CREDIT_MAX / 2))
    {
        grant = BT_RFCOMM_RX_CREDIT_MAX - d->rx_credits;
    }
    st = send_uih(s, dlci, grant, data, len);
    if(st == BT_OK)
    {
        if(d->cfc)
            d->tx_credits--;
        if(grant >= 0)
            d->rx_credits = (uint8_t) (d->rx_credits + grant);
    }
    return st;
}

/* grant the peer fresh receive credits with an empty credit frame */
static void replenish(struct bt_rfcomm_session *s, struct bt_rfcomm_dlc *d)
{
    if(!d->cfc || (d->state != DLC_OPEN))
        return;
    if(d->rx_credits <= BT_RFCOMM_RX_CREDIT_MAX / 2)
    {
        uint8_t grant = (uint8_t) (BT_RFCOMM_RX_CREDIT_MAX - d->rx_credits);
        if(send_uih(s, d->dlci, grant, NULL, 0) == BT_OK)
            d->rx_credits = (uint8_t) (d->rx_credits + grant);
    }
}

static void handle_mcc(struct bt_rfcomm_session *s, const uint8_t *p, size_t len)
{
    uint8_t typebyte, type;
    bool cmd;
    size_t vlen;
    const uint8_t *value;

    if(len < 2)
        return;
    typebyte = p[0];
    type = (uint8_t) (typebyte >> 2);
    cmd = (typebyte & 0x02) != 0;   /* MCC: commands carry C/R 1, responses C/R 0 */
    vlen = p[1] >> 1;
    value = &p[2];
    if(!(p[1] & 1) || (vlen > len - 2))
        return;

    switch(type)
    {
        case RF_MCC_PN:
            if(vlen < 8)
                break;
            if(!cmd)
            {
                /* our PN answered: SABM next */
                struct bt_rfcomm_dlc *d = dlc_by_dlci(s, value[0]);
                if(d && (d->state == DLC_WAIT_PN))
                {
                    uint16_t mtu = (uint16_t) (value[4] | (value[5] << 8));
                    if(mtu && (mtu < d->mtu))
                        d->mtu = mtu;
                    d->cfc = ((value[1] & 0xf0) == 0xe0);
                    if(d->cfc)
                        d->tx_credits = value[7];
                    if(send_ctrl(s, d->dlci, RF_SABM, true) == BT_OK)
                        d->state = DLC_WAIT_UA;
                    else
                        dlc_down(s, d, false);
                }
            } else if(s->responder && (s->state == SES_UP)) {
                /* the peer negotiating a DLC to one of our channels: answer
                   with what we can do, then wait for its SABM */
                struct bt_rfcomm_dlc *d = dlc_by_dlci(s, value[0]);
                uint16_t mtu = (uint16_t) (value[4] | (value[5] << 8));
                uint8_t pn[8];
                if(!d)
                    d = accept_dlc(s, value[0]);
                if(!d)
                    break;                     /* DM follows its SABM */
                if(mtu && (mtu < d->mtu))
                    d->mtu = mtu;
                d->cfc = ((value[1] & 0xf0) == 0xf0);
                if(d->cfc)
                {
                    d->tx_credits = value[7];
                    d->rx_credits = 7;
                }
                memcpy(pn, value, 8);
                pn[1] = d->cfc ? 0xe0 : 0x00;
                pn[4] = (uint8_t) (d->mtu & 0xff);
                pn[5] = (uint8_t) (d->mtu >> 8);
                pn[7] = d->cfc ? 7 : 0;
                send_mcc(s, RF_MCC_PN, false, pn, 8);
            } else {
                /* an initiator being negotiated at: accept the peer's terms */
                uint8_t pn[8];
                memcpy(pn, value, 8);
                pn[1] = ((value[1] & 0xf0) == 0xf0) ? 0xe0 : 0x00;
                pn[7] = 7;
                send_mcc(s, RF_MCC_PN, false, pn, 8);
            }
            break;

        case RF_MCC_MSC:
            if(vlen < 2)
                break;
            if(cmd)
            {
                struct bt_rfcomm_dlc *d = dlc_by_dlci(s, (uint8_t) (value[0] >> 2));
                send_mcc(s, RF_MCC_MSC, false, value, vlen);   /* echo as response */
                if(d)
                {
                    d->peer_signals = value[1];
                    d->msc_seen = true;
                    if(!d->msc_sent)
                    {
                        d->msc_sent = true;
                        send_msc(s, d, true, MSC_SIGNALS);
                    }
                }
            }
            break;

        case RF_MCC_TEST:
            if(cmd)
                send_mcc(s, RF_MCC_TEST, false, value, vlen);
            break;

        case RF_MCC_FCON:
        case RF_MCC_FCOFF:
        case RF_MCC_RLS:
            if(cmd)
                send_mcc(s, type, false, value, vlen);
            break;

        case RF_MCC_RPN:
            if(cmd && (vlen >= 1))
            {
                /* answer with the defaults: 9600 8N1, no flow control */
                uint8_t rpn[8];
                memset(rpn, 0, sizeof(rpn));
                rpn[0] = value[0];
                rpn[1] = 0x03;        /* 9600 */
                rpn[2] = 0x03;        /* 8 bits, 1 stop, no parity */
                send_mcc(s, RF_MCC_RPN, false, rpn, 8);
            }
            break;

        default:
            if(cmd)
            {
                uint8_t nsc = typebyte;
                send_mcc(s, RF_MCC_NSC, false, &nsc, 1);
            }
            break;
    }
}

void bt_rfcomm_on_data(struct bt_rfcomm_session *s, const uint8_t *data, size_t len)
{
    while(len >= 4)
    {
        uint8_t addr = data[0];
        uint8_t control = data[1];
        uint8_t dlci = (uint8_t) (addr >> 2);
        size_t hlen = 3;
        size_t ilen;
        int credits = -1;
        const uint8_t *info;
        struct bt_rfcomm_dlc *d;

        ilen = data[2] >> 1;
        if(!(data[2] & 1))
        {
            if(len < 5)
                return;
            ilen |= ((size_t) data[3]) << 7;
            hlen = 4;
        }
        if(((control & 0xef) == RF_UIH) && (control & 0x10))
        {
            if(len < hlen + 1)
                return;
            credits = data[hlen];
            hlen++;
        }
        if(len < hlen + ilen + 1)
            return;                       /* truncated frame */
        if(rf_fcs(data, ((control & 0xef) == RF_UIH) ? 2 : 3) != data[hlen + ilen])
            return;                       /* bad checksum */
        info = &data[hlen];

        switch(control & 0xef)
        {
            case RF_SABM & 0xef:
                if(s->responder && (dlci == 0) && (s->state == SES_CLOSED))
                {
                    /* the peer starts the multiplexer */
                    if(send_ctrl(s, 0, RF_UA, false) == BT_OK)
                    {
                        s->state = SES_UP;
                        if(s->event)
                            s->event(s->context, BT_RFCOMM_SESSION_UP);
                    }
                }
                else if(s->responder && dlci && (s->state == SES_UP))
                {
                    /* a DLC to one of our server channels (PN may have
                       come first, or not at all) */
                    d = dlc_by_dlci(s, dlci);
                    if(!d)
                        d = accept_dlc(s, dlci);
                    if(d && (d->state == DLC_WAIT_SABM))
                    {
                        if(send_ctrl(s, dlci, RF_UA, false) == BT_OK)
                        {
                            d->state = DLC_OPEN;
                            d->msc_sent = true;
                            send_msc(s, d, true, MSC_SIGNALS);
                            replenish(s, d);
                            dlc_event(s, d, BT_RFCOMM_DLC_OPEN, false, NULL, 0);
                        }
                        else
                            dlc_down(s, d, false);
                    }
                    else
                        send_ctrl(s, dlci, RF_DM_F, false);
                }
                else
                    send_ctrl(s, dlci, RF_DM_F, false);
                break;

            case RF_UA & 0xef:
                if(dlci == 0)
                {
                    if(s->state == SES_WAIT_UA)
                    {
                        s->state = SES_UP;
                        if(s->event)
                            s->event(s->context, BT_RFCOMM_SESSION_UP);
                    }
                    else if(s->state == SES_CLOSING)
                        session_down(s);
                }
                else if((d = dlc_by_dlci(s, dlci)))
                {
                    if(d->state == DLC_WAIT_UA)
                    {
                        d->state = DLC_OPEN;
                        d->msc_sent = true;
                        send_msc(s, d, true, MSC_SIGNALS);
                        replenish(s, d);
                        dlc_event(s, d, BT_RFCOMM_DLC_OPEN, false, NULL, 0);
                    }
                    else if(d->state == DLC_CLOSING)
                        dlc_down(s, d, false);
                }
                break;

            case RF_DM & 0xef:
                if(dlci == 0)
                    session_down(s);
                else if((d = dlc_by_dlci(s, dlci)))
                    dlc_down(s, d, d->state != DLC_CLOSING);
                break;

            case RF_DISC & 0xef:
                send_ctrl(s, dlci, RF_UA, false);
                if(dlci == 0)
                    session_down(s);
                else if((d = dlc_by_dlci(s, dlci)))
                    dlc_down(s, d, false);
                break;

            case RF_UIH & 0xef:
                if(dlci == 0)
                {
                    handle_mcc(s, info, ilen);
                }
                else if((d = dlc_by_dlci(s, dlci)))
                {
                    if(credits >= 0)
                    {
                        bool was_stuck = d->cfc && (d->tx_credits <= 0);
                        d->tx_credits = (int16_t) (d->tx_credits + credits);
                        if(was_stuck && (d->tx_credits > 0))
                            dlc_event(s, d, BT_RFCOMM_DLC_CREDITS, false, NULL, 0);
                    }
                    if(ilen)
                    {
                        if(d->cfc && d->rx_credits)
                            d->rx_credits--;
                        dlc_event(s, d, BT_RFCOMM_DLC_DATA, false, info, ilen);
                        replenish(s, d);
                    }
                }
                break;
        }
        data += hlen + ilen + 1;
        len -= hlen + ilen + 1;
    }
}
