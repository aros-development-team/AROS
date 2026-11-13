/* Exercises the RFCOMM multiplexer against a scripted fake peer. */

#include <btcore/rfcomm.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t lastframe[1100];
static size_t lastlen;
static int sends;

static bt_status_t t_send(void *ctx, const uint8_t *f, size_t len)
{
    (void) ctx;
    assert(len <= sizeof(lastframe));
    memcpy(lastframe, f, len);
    lastlen = len;
    sends++;
    return BT_OK;
}

static int ses_up, ses_down;
static void t_session(void *ctx, uint8_t ev)
{
    (void) ctx;
    if(ev == BT_RFCOMM_SESSION_UP) ses_up++;
    if(ev == BT_RFCOMM_SESSION_DOWN) ses_down++;
}

static int dlc_open, dlc_closed, dlc_credits, dlc_refused;
static uint8_t rxbuf[1100];
static size_t rxlen;
static void t_dlc(void *ctx, const struct bt_rfcomm_dlc_event *ev)
{
    (void) ctx;
    switch(ev->event)
    {
        case BT_RFCOMM_DLC_OPEN: dlc_open++; break;
        case BT_RFCOMM_DLC_CLOSED: dlc_closed++; if(ev->refused) dlc_refused++; break;
        case BT_RFCOMM_DLC_CREDITS: dlc_credits++; break;
        case BT_RFCOMM_DLC_DATA:
            assert(ev->data_len <= sizeof(rxbuf));
            memcpy(rxbuf, ev->data, ev->data_len);
            rxlen = ev->data_len;
            break;
    }
}

/* responder role: serve channel 1 only */
static int accepts;
static uint8_t accept_channel;
static bool t_accept(void *ctx, uint8_t channel, bt_rfcomm_dlc_fn *cb, void **ud)
{
    (void) ctx;
    accepts++;
    accept_channel = channel;
    if(channel != 1) return false;
    *cb = t_dlc;
    *ud = NULL;
    return true;
}

/* peer helpers: frames as the responder would send them */
static uint8_t fcs_calc(const uint8_t *d, size_t n)
{
    /* independent implementation for the test: bitwise reflected CRC */
    uint8_t crc = 0xff;
    size_t i; int b;
    for(i = 0; i < n; i++)
    {
        crc ^= d[i];
        for(b = 0; b < 8; b++)
            crc = (crc & 1) ? (uint8_t)((crc >> 1) ^ 0xe0) : (uint8_t)(crc >> 1);
    }
    return (uint8_t)(0xff - crc);
}

static void peer_ctrl(struct bt_rfcomm_session *s, uint8_t dlci, uint8_t control, int cmd)
{
    uint8_t f[4];
    f[0] = (uint8_t)((dlci << 2) | (cmd ? 0x01 : 0x03));  /* responder: cmd C/R=0? no: responder commands C/R=0 -> addr bit 0; responses C/R=1 */
    f[1] = control;
    f[2] = 0x01;
    f[3] = fcs_calc(f, 3);
    bt_rfcomm_on_data(s, f, 4);
}

static void peer_uih(struct bt_rfcomm_session *s, uint8_t dlci, int credits, const uint8_t *data, size_t len)
{
    uint8_t f[1100];
    size_t pos = 0;
    f[pos++] = (uint8_t)((dlci << 2) | 0x01);
    f[pos++] = (credits >= 0) ? 0xff : 0xef;
    f[pos++] = (uint8_t)((len << 1) | 1);
    if(credits >= 0) f[pos++] = (uint8_t)credits;
    if(len) { memcpy(&f[pos], data, len); pos += len; }
    f[pos] = fcs_calc(f, 2); pos++;
    bt_rfcomm_on_data(s, f, pos);
}

int main(void)
{
    struct bt_rfcomm_session ses;
    uint8_t dlci = 0;

    /* known FCS vector: SABM on DLCI 0 from the initiator */
    {
        uint8_t hdr[3] = { 0x03, 0x3f, 0x01 };
        printf("FCS(03 3f 01) = 0x%02x\n", fcs_calc(hdr, 3));
    }

    bt_rfcomm_init(&ses, 666, t_send, t_session, NULL);
    assert(bt_rfcomm_start(&ses) == BT_OK);
    assert(lastlen == 4 && lastframe[0] == 0x03 && lastframe[1] == 0x3f);
    /* peer answers UA on DLCI 0 (response: C/R=1 from responder) */
    {
        uint8_t f[4] = { 0x03, 0x73, 0x01, 0 };
        f[3] = fcs_calc(f, 3);
        bt_rfcomm_on_data(&ses, f, 4);
    }
    assert(ses_up == 1);

    /* open server channel 1 -> PN command goes out */
    assert(bt_rfcomm_open(&ses, 1, t_dlc, NULL, &dlci) == BT_OK);
    assert(dlci == 2);
    assert(lastframe[1] == 0xef);            /* UIH on DLCI 0 */
    assert(lastframe[3] == 0x83);            /* PN command */
    /* peer PN response: cfc granted, mtu 127, 5 initial credits */
    {
        uint8_t pn[10] = { 0x81, (8 << 1) | 1, dlci, 0xe0, 0, 0, 127, 0, 0, 5 };
        peer_uih(&ses, 0, -1, pn, 10);
    }
    /* SABM for the DLC went out; peer answers UA (responder response C/R=1... addr has our C/R=1 for cmd) */
    assert(lastframe[0] == ((dlci << 2) | 0x03) && lastframe[1] == 0x3f);
    {
        uint8_t f[4] = { (uint8_t)((dlci << 2) | 0x03), 0x73, 0x01, 0 };
        f[3] = fcs_calc(f, 3);
        bt_rfcomm_on_data(&ses, f, 4);
    }
    assert(dlc_open == 1);
    assert(bt_rfcomm_mtu(&ses, dlci) == 127);
    assert(bt_rfcomm_can_send(&ses, dlci));

    /* send data until the credits run out */
    {
        uint8_t msg[5] = { 'h', 'e', 'l', 'l', 'o' };
        int n = 0;
        while(bt_rfcomm_send(&ses, dlci, msg, 5) == BT_OK) n++;
        printf("sent %d frames on 5 credits\n", n);
        assert(n == 5);
        assert(bt_rfcomm_send(&ses, dlci, msg, 5) == BT_ERR_BUSY);
    }
    /* peer grants 3 credits with an empty UIH */
    peer_uih(&ses, dlci, 3, NULL, 0);
    assert(dlc_credits == 1);
    assert(bt_rfcomm_can_send(&ses, dlci));

    /* peer sends data */
    {
        uint8_t msg[3] = { 'a', 'b', 'c' };
        peer_uih(&ses, dlci, -1, msg, 3);
        assert(rxlen == 3 && !memcmp(rxbuf, "abc", 3));
    }

    /* peer disconnects the DLC */
    peer_ctrl(&ses, dlci, 0x53, 1);
    assert(dlc_closed == 1 && dlc_refused == 0);

    /* refused open: DM response */
    dlci = 0;
    assert(bt_rfcomm_open(&ses, 3, t_dlc, NULL, &dlci) == BT_OK);
    {
        uint8_t pn[10] = { 0x81, (8 << 1) | 1, dlci, 0xe0, 0, 0, 127, 0, 0, 0 };
        peer_uih(&ses, 0, -1, pn, 10);
    }
    {
        uint8_t f[4] = { (uint8_t)((dlci << 2) | 0x03), 0x1f, 0x01, 0 };
        f[3] = fcs_calc(f, 3);
        bt_rfcomm_on_data(&ses, f, 4);
    }
    assert(dlc_closed == 2 && dlc_refused == 1);

    bt_rfcomm_stop(&ses);
    assert(ses_down == 1);

    /* ---- responder role: the peer starts the multiplexer and opens our channel 1 */
    {
        struct bt_rfcomm_session rs;
        uint8_t f[4];
        int opened_before = dlc_open;
        int closed_before = dlc_closed;
        rxlen = 0;
        bt_rfcomm_init_responder(&rs, 666, t_send, t_session, t_accept, NULL);
        /* initiator's SABM on DLCI 0 (command: C/R 1) */
        f[0] = 0x03; f[1] = 0x3f; f[2] = 0x01; f[3] = fcs_calc(f, 3);
        bt_rfcomm_on_data(&rs, f, 4);
        assert(ses_up == 2 && bt_rfcomm_session_up(&rs));
        assert(lastlen == 4 && lastframe[0] == 0x03 && lastframe[1] == 0x73);   /* our UA: responder response C/R 1 */
        /* PN command for DLCI 2 (channel 1), credit flow requested, mtu 100, 4 credits */
        {
            uint8_t pn[10] = { 0x83, (8 << 1) | 1, 2, 0xf0, 0, 0, 100, 0, 0, 4 };
            uint8_t u[16];
            size_t pos = 0;
            u[pos++] = 0x03; u[pos++] = 0xef; u[pos++] = (uint8_t)((10 << 1) | 1);
            memcpy(&u[pos], pn, 10); pos += 10;
            u[pos] = fcs_calc(u, 2); pos++;
            bt_rfcomm_on_data(&rs, u, pos);
        }
        assert(accepts == 1 && accept_channel == 1);
        assert(lastframe[0] == 0x01 && lastframe[3] == 0x81);      /* PN response on DLCI 0, our UIH C/R 0 */
        assert(lastframe[6] == 0xe0 && lastframe[9] == 100);        /* credits accepted, mtu 100 */
        /* SABM on DLCI 2 */
        f[0] = (2 << 2) | 0x03; f[1] = 0x3f; f[2] = 0x01; f[3] = fcs_calc(f, 3);
        bt_rfcomm_on_data(&rs, f, 4);
        assert(dlc_open == opened_before + 1);
        assert(bt_rfcomm_mtu(&rs, 2) == 100);
        assert(bt_rfcomm_can_send(&rs, 2));
        /* our data frame carries the responder's C/R (address 0x09) */
        {
            uint8_t msg[3] = { 'x', 'y', 'z' };
            assert(bt_rfcomm_send(&rs, 2, msg, 3) == BT_OK);
            assert(lastframe[0] == ((2 << 2) | 0x01));
        }
        /* the initiator's data (its UIH: C/R 1) */
        {
            uint8_t u[8]; size_t pos = 0;
            u[pos++] = (2 << 2) | 0x03; u[pos++] = 0xef; u[pos++] = (uint8_t)((3 << 1) | 1);
            u[pos++] = 'a'; u[pos++] = 'b'; u[pos++] = 'c';
            u[pos] = fcs_calc(u, 2); pos++;
            bt_rfcomm_on_data(&rs, u, pos);
            assert(rxlen == 3 && !memcmp(rxbuf, "abc", 3));
        }
        /* initiator disconnects the DLC, then the session */
        f[0] = (2 << 2) | 0x03; f[1] = 0x53; f[2] = 0x01; f[3] = fcs_calc(f, 3);
        bt_rfcomm_on_data(&rs, f, 4);
        assert(dlc_closed == closed_before + 1);
        assert(lastframe[0] == ((2 << 2) | 0x03) && lastframe[1] == 0x73);
        /* a channel nobody serves: DM */
        f[0] = (4 << 2) | 0x03; f[1] = 0x3f; f[2] = 0x01; f[3] = fcs_calc(f, 3);
        bt_rfcomm_on_data(&rs, f, 4);
        assert(accepts == 2 && accept_channel == 2);
        assert(lastframe[0] == ((4 << 2) | 0x03) && lastframe[1] == 0x1f);
        f[0] = 0x03; f[1] = 0x53; f[2] = 0x01; f[3] = fcs_calc(f, 3);
        bt_rfcomm_on_data(&rs, f, 4);
        assert(ses_down == 2);
    }

    puts("rfcomm tests passed");
    return 0;
}
