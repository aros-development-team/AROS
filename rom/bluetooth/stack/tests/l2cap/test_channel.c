#include "test_channel.h"
#include "../support/test.h"

#include <btcore/l2cap_channel.h>

#include <string.h>

/*
 * A transport that reassembles whatever gets sent via send_acl() back
 * into complete L2CAP PDUs (mirroring what a real peer would see), so
 * tests can inspect exactly what the channel manager put on the wire.
 * Never answers on its own -- tests inject responses by hand via
 * feed_signaling_command()/feed_data(), matching the same "drive it
 * precisely" approach used for the HCI command queue tests.
 */
struct fake_transport
{
    struct bt_hci_transport base;
    struct bt_l2cap_reassembler sent_reassembler;
    uint8_t captured_pdu[300];
    size_t captured_pdu_len;
    bool captured_complete;
    int send_count;
    bool fail_send;
};

static int fake_open(struct bt_hci_transport *t)
{
    (void)t;
    return 0;
}

static void fake_close(struct bt_hci_transport *t)
{
    (void)t;
}

static int fake_send_unsupported(struct bt_hci_transport *t, const uint8_t *data, size_t length)
{
    (void)t;
    (void)data;
    (void)length;
    return -1;
}

static int fake_send_acl(struct bt_hci_transport *t, const uint8_t *data, size_t length)
{
    struct fake_transport *ft = (struct fake_transport *)t->impl;
    struct bt_buf_reader r;
    struct bt_hci_acl_header hdr;
    enum bt_l2cap_reassembly_result rr;
    const uint8_t *payload;

    if (ft->fail_send)
        return -1;

    ft->send_count++;

    bt_buf_reader_init(&r, data, length);
    if (bt_hci_parse_acl_header(&r, &hdr) != BT_OK)
        return -1;

    payload = bt_buf_reader_peek(&r, hdr.data_len);
    if (payload == NULL)
        return -1;

    rr = bt_l2cap_reassembler_feed(&ft->sent_reassembler, hdr.pb_flag, payload, hdr.data_len);
    if (rr == BT_L2CAP_REASSEMBLY_COMPLETE)
    {
        const uint8_t *pdu = bt_l2cap_reassembler_take(&ft->sent_reassembler, &ft->captured_pdu_len);

        memcpy(ft->captured_pdu, pdu, ft->captured_pdu_len);
        ft->captured_complete = true;
    }

    return 0;
}

static int fake_start_receive(struct bt_hci_transport *t, bt_hci_transport_recv_fn recv,
                               void *user_data)
{
    (void)t;
    (void)recv;
    (void)user_data;
    return 0;
}

static void fake_stop_receive(struct bt_hci_transport *t)
{
    (void)t;
}

static const struct bt_hci_transport_ops fake_ops = {
    .open = fake_open,
    .close = fake_close,
    .send_command = fake_send_unsupported,
    .send_acl = fake_send_acl,
    .send_sco = fake_send_unsupported,
    .send_iso = fake_send_unsupported,
    .start_receive = fake_start_receive,
    .stop_receive = fake_stop_receive,
};

static void fake_transport_init(struct fake_transport *ft)
{
    memset(ft, 0, sizeof(*ft));
    bt_l2cap_reassembler_init(&ft->sent_reassembler);
    ft->base.ops = &fake_ops;
    ft->base.impl = ft;
}

/* Parses the last captured outbound PDU as a signaling command. Consumes
 * (clears) the capture so the next check reflects the next send. */
static bool take_last_sig_command(struct fake_transport *ft, struct bt_l2cap_sig_header *hdr,
                                   const uint8_t **cmd_data)
{
    struct bt_buf_reader r;
    struct bt_l2cap_header l2hdr;

    if (!ft->captured_complete)
        return false;
    ft->captured_complete = false;

    bt_buf_reader_init(&r, ft->captured_pdu, ft->captured_pdu_len);
    if (bt_l2cap_parse_header(&r, &l2hdr) != BT_OK)
        return false;
    if (bt_l2cap_sig_parse_header(&r, hdr) != BT_OK)
        return false;
    *cmd_data = bt_buf_reader_peek(&r, hdr->length);
    return *cmd_data != NULL;
}

static void feed_signaling_command(struct bt_l2cap_channel_manager *mgr, const uint8_t *sig_cmd,
                                    size_t sig_cmd_len, uint64_t now_us)
{
    uint8_t pdu[BT_L2CAP_HEADER_LEN + 64];
    struct bt_buf_writer pw;
    uint8_t acl[BT_HCI_ACL_HEADER_LEN + BT_L2CAP_HEADER_LEN + 64];
    struct bt_buf_writer aw;
    struct bt_buf_reader ar;
    struct bt_hci_acl_header acl_hdr;

    bt_buf_writer_init(&pw, pdu, sizeof(pdu));
    bt_l2cap_encode_header(&pw, (uint16_t)sig_cmd_len, mgr->signaling_cid);
    bt_buf_writer_write_bytes(&pw, sig_cmd, sig_cmd_len);

    bt_buf_writer_init(&aw, acl, sizeof(acl));
    bt_hci_encode_acl_header(&aw, mgr->handle, 0x00, 0x00, (uint16_t)bt_buf_writer_len(&pw));
    bt_buf_writer_write_bytes(&aw, pdu, bt_buf_writer_len(&pw));

    bt_buf_reader_init(&ar, acl, bt_buf_writer_len(&aw));
    bt_hci_parse_acl_header(&ar, &acl_hdr);
    bt_l2cap_channel_manager_on_acl(mgr, acl_hdr.pb_flag, bt_buf_reader_peek(&ar, acl_hdr.data_len),
                                     acl_hdr.data_len, now_us);
}

static void feed_data(struct bt_l2cap_channel_manager *mgr, uint16_t cid, const uint8_t *data,
                       uint16_t data_len, uint64_t now_us)
{
    uint8_t pdu[BT_L2CAP_HEADER_LEN + 64];
    struct bt_buf_writer pw;
    uint8_t acl[BT_HCI_ACL_HEADER_LEN + BT_L2CAP_HEADER_LEN + 64];
    struct bt_buf_writer aw;
    struct bt_buf_reader ar;
    struct bt_hci_acl_header acl_hdr;

    bt_buf_writer_init(&pw, pdu, sizeof(pdu));
    bt_l2cap_encode_header(&pw, data_len, cid);
    bt_buf_writer_write_bytes(&pw, data, data_len);

    bt_buf_writer_init(&aw, acl, sizeof(acl));
    bt_hci_encode_acl_header(&aw, mgr->handle, 0x00, 0x00, (uint16_t)bt_buf_writer_len(&pw));
    bt_buf_writer_write_bytes(&aw, pdu, bt_buf_writer_len(&pw));

    bt_buf_reader_init(&ar, acl, bt_buf_writer_len(&aw));
    bt_hci_parse_acl_header(&ar, &acl_hdr);
    bt_l2cap_channel_manager_on_acl(mgr, acl_hdr.pb_flag, bt_buf_reader_peek(&ar, acl_hdr.data_len),
                                     acl_hdr.data_len, now_us);
}

static const struct bt_l2cap_channel *find_chan(const struct bt_l2cap_channel_manager *mgr,
                                                 uint16_t local_cid)
{
    size_t i;

    for (i = 0; i < BT_L2CAP_CHANNEL_MANAGER_MAX_CHANNELS; i++)
    {
        if (mgr->channels[i].state != BT_L2CAP_CHAN_FREE && mgr->channels[i].local_cid == local_cid)
            return &mgr->channels[i];
    }
    return NULL;
}

struct event_log
{
    int open_count;
    int closed_count;
    int data_count;
    enum bt_l2cap_close_reason last_close_reason;
    uint8_t last_data[64];
    size_t last_data_len;
};

static void log_event(struct bt_l2cap_channel_event_info *info, void *user_data)
{
    struct event_log *log = (struct event_log *)user_data;

    switch (info->event)
    {
    case BT_L2CAP_CHANNEL_EVENT_OPENED:
        log->open_count++;
        break;
    case BT_L2CAP_CHANNEL_EVENT_CLOSED:
        log->closed_count++;
        log->last_close_reason = info->close_reason;
        break;
    case BT_L2CAP_CHANNEL_EVENT_DATA:
        log->data_count++;
        log->last_data_len = info->data_len;
        if (info->data_len <= sizeof(log->last_data))
            memcpy(log->last_data, info->data, info->data_len);
        break;
    }
}

static void test_open_full_handshake(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct event_log log = {0, 0, 0, BT_L2CAP_CLOSE_LOCAL, {0}, 0};
    uint16_t local_cid;
    struct bt_l2cap_sig_header hdr;
    const uint8_t *cmd_data;
    struct bt_l2cap_connection_request creq;
    uint8_t identifier;
    uint8_t buf[32];
    struct bt_buf_writer w;

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);

    BT_CHECK(bt_l2cap_channel_manager_open(&mgr, 0x001F, 200, log_event, &log, &local_cid, 0) ==
              BT_OK);
    BT_CHECK(local_cid == BT_L2CAP_CID_DYNAMIC_START);
    BT_CHECK(ft.send_count == 1);

    BT_CHECK(take_last_sig_command(&ft, &hdr, &cmd_data));
    BT_CHECK(hdr.code == BT_L2CAP_SIG_CONNECTION_REQUEST);
    BT_CHECK(bt_l2cap_sig_parse_connection_request(cmd_data, hdr.length, &creq) == BT_OK);
    BT_CHECK(creq.psm == 0x001F);
    BT_CHECK(creq.source_cid == local_cid);
    identifier = hdr.identifier;

    /* Peer accepts the connection -- our Configure Request should follow
     * immediately, synchronously, since this manager (like the virtual
     * transport before it) reacts inline. */
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_connection_response(&w, identifier, /*destination_cid=*/0x0050, local_cid,
                                             BT_L2CAP_CONN_RESULT_SUCCESS, 0);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 10);

    BT_CHECK(ft.send_count == 2);
    BT_CHECK(take_last_sig_command(&ft, &hdr, &cmd_data));
    BT_CHECK(hdr.code == BT_L2CAP_SIG_CONFIGURE_REQUEST);
    identifier = hdr.identifier;

    /* Peer accepts our configuration. */
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_configure_response(&w, identifier, /*source_cid=*/0x0050, 0,
                                            BT_L2CAP_CONFIG_RESULT_SUCCESS, 0);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 20);

    BT_CHECK(log.open_count == 0); /* only half the handshake so far */

    /* Peer now configures its side of the channel. */
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_configure_request(&w, 0x99, local_cid, 0, 300);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 30);

    BT_CHECK(ft.send_count == 3);
    BT_CHECK(take_last_sig_command(&ft, &hdr, &cmd_data));
    BT_CHECK(hdr.code == BT_L2CAP_SIG_CONFIGURE_RESPONSE);
    BT_CHECK(hdr.identifier == 0x99);

    BT_CHECK(log.open_count == 1);
    BT_CHECK(find_chan(&mgr, local_cid)->state == BT_L2CAP_CHAN_OPEN);
    BT_CHECK(find_chan(&mgr, local_cid)->remote_mtu == 300);
}

static void test_connection_refused(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct event_log log = {0, 0, 0, BT_L2CAP_CLOSE_LOCAL, {0}, 0};
    uint16_t local_cid;
    struct bt_l2cap_sig_header hdr;
    const uint8_t *cmd_data;
    uint8_t identifier;
    uint8_t buf[32];
    struct bt_buf_writer w;

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_CLASSIC, 200);

    BT_CHECK(bt_l2cap_channel_manager_open(&mgr, 0x0001, 0, log_event, &log, &local_cid, 0) ==
              BT_OK);
    BT_CHECK(take_last_sig_command(&ft, &hdr, &cmd_data));
    identifier = hdr.identifier;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_connection_response(&w, identifier, 0x0000, local_cid,
                                             BT_L2CAP_CONN_RESULT_REFUSED_PSM, 0);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 10);

    BT_CHECK(log.closed_count == 1);
    BT_CHECK(log.last_close_reason == BT_L2CAP_CLOSE_REFUSED);
    BT_CHECK(find_chan(&mgr, local_cid) == NULL);
}

static void test_data_round_trip(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct event_log log = {0, 0, 0, BT_L2CAP_CLOSE_LOCAL, {0}, 0};
    uint16_t local_cid;
    struct bt_l2cap_sig_header hdr;
    const uint8_t *cmd_data;
    uint8_t identifier;
    uint8_t buf[32];
    struct bt_buf_writer w;
    static const uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);

    BT_CHECK(bt_l2cap_channel_manager_open(&mgr, 0x0019, 0, log_event, &log, &local_cid, 0) ==
              BT_OK);
    BT_CHECK(take_last_sig_command(&ft, &hdr, &cmd_data));
    identifier = hdr.identifier;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_connection_response(&w, identifier, 0x0050, local_cid,
                                             BT_L2CAP_CONN_RESULT_SUCCESS, 0);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 10);
    BT_CHECK(take_last_sig_command(&ft, &hdr, &cmd_data));
    identifier = hdr.identifier;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_configure_response(&w, identifier, 0x0050, 0,
                                            BT_L2CAP_CONFIG_RESULT_SUCCESS, 0);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 20);

    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_configure_request(&w, 0x77, local_cid, 0, 0);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 30);

    BT_CHECK(log.open_count == 1);
    ft.captured_complete = false; /* discard the Configure Response we just sent */

    /* Outbound data. */
    BT_CHECK(bt_l2cap_channel_manager_send(&mgr, local_cid, payload, sizeof(payload), 40) ==
              BT_OK);
    BT_CHECK(ft.captured_complete);
    {
        struct bt_buf_reader r;
        struct bt_l2cap_header l2hdr;

        bt_buf_reader_init(&r, ft.captured_pdu, ft.captured_pdu_len);
        BT_CHECK(bt_l2cap_parse_header(&r, &l2hdr) == BT_OK);
        BT_CHECK(l2hdr.cid == 0x0050); /* remote_cid */
        BT_CHECK(l2hdr.length == sizeof(payload));
        BT_CHECK(memcmp(bt_buf_reader_peek(&r, sizeof(payload)), payload, sizeof(payload)) == 0);
    }

    /* Inbound data. */
    feed_data(&mgr, local_cid, payload, sizeof(payload), 50);
    BT_CHECK(log.data_count == 1);
    BT_CHECK(log.last_data_len == sizeof(payload));
    BT_CHECK(memcmp(log.last_data, payload, sizeof(payload)) == 0);
}

static void test_local_disconnect_from_open(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct event_log log = {0, 0, 0, BT_L2CAP_CLOSE_LOCAL, {0}, 0};
    uint16_t local_cid;
    struct bt_l2cap_sig_header hdr;
    const uint8_t *cmd_data;
    uint8_t identifier;
    uint8_t buf[32];
    struct bt_buf_writer w;

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);

    BT_CHECK(bt_l2cap_channel_manager_open(&mgr, 0x0019, 0, log_event, &log, &local_cid, 0) ==
              BT_OK);
    BT_CHECK(take_last_sig_command(&ft, &hdr, &cmd_data));
    identifier = hdr.identifier;
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_connection_response(&w, identifier, 0x0050, local_cid,
                                             BT_L2CAP_CONN_RESULT_SUCCESS, 0);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 10);
    BT_CHECK(take_last_sig_command(&ft, &hdr, &cmd_data));
    identifier = hdr.identifier;
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_configure_response(&w, identifier, 0x0050, 0,
                                            BT_L2CAP_CONFIG_RESULT_SUCCESS, 0);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 20);
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_configure_request(&w, 0x77, local_cid, 0, 0);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 30);
    BT_CHECK(log.open_count == 1);
    ft.captured_complete = false;

    bt_l2cap_channel_manager_close(&mgr, local_cid, 40);
    BT_CHECK(find_chan(&mgr, local_cid)->state == BT_L2CAP_CHAN_WAIT_DISCONNECT_RSP);
    BT_CHECK(take_last_sig_command(&ft, &hdr, &cmd_data));
    BT_CHECK(hdr.code == BT_L2CAP_SIG_DISCONNECTION_REQUEST);
    identifier = hdr.identifier;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_disconnection_response(&w, identifier, 0x0050, local_cid);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 50);

    BT_CHECK(log.closed_count == 1);
    BT_CHECK(log.last_close_reason == BT_L2CAP_CLOSE_LOCAL);
    BT_CHECK(find_chan(&mgr, local_cid) == NULL);
}

static void test_peer_initiated_disconnect(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct event_log log = {0, 0, 0, BT_L2CAP_CLOSE_LOCAL, {0}, 0};
    uint16_t local_cid;
    struct bt_l2cap_sig_header hdr;
    const uint8_t *cmd_data;
    uint8_t identifier;
    uint8_t buf[32];
    struct bt_buf_writer w;

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);

    BT_CHECK(bt_l2cap_channel_manager_open(&mgr, 0x0019, 0, log_event, &log, &local_cid, 0) ==
              BT_OK);
    BT_CHECK(take_last_sig_command(&ft, &hdr, &cmd_data));
    identifier = hdr.identifier;
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_connection_response(&w, identifier, 0x0050, local_cid,
                                             BT_L2CAP_CONN_RESULT_SUCCESS, 0);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 10);
    BT_CHECK(take_last_sig_command(&ft, &hdr, &cmd_data));
    identifier = hdr.identifier;
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_configure_response(&w, identifier, 0x0050, 0,
                                            BT_L2CAP_CONFIG_RESULT_SUCCESS, 0);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 20);
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_configure_request(&w, 0x77, local_cid, 0, 0);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 30);
    BT_CHECK(log.open_count == 1);
    ft.captured_complete = false;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_disconnection_request(&w, 0x55, local_cid, 0x0050);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 40);

    BT_CHECK(take_last_sig_command(&ft, &hdr, &cmd_data));
    BT_CHECK(hdr.code == BT_L2CAP_SIG_DISCONNECTION_RESPONSE);
    BT_CHECK(hdr.identifier == 0x55);
    BT_CHECK(log.closed_count == 1);
    BT_CHECK(log.last_close_reason == BT_L2CAP_CLOSE_PEER_DISCONNECTED);
    BT_CHECK(find_chan(&mgr, local_cid) == NULL);
}

static void test_removal_during_negotiation(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct event_log log = {0, 0, 0, BT_L2CAP_CLOSE_LOCAL, {0}, 0};
    uint16_t local_cid;
    struct bt_l2cap_sig_header hdr;
    const uint8_t *cmd_data;
    uint8_t identifier;
    uint8_t buf[32];
    struct bt_buf_writer w;

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);

    BT_CHECK(bt_l2cap_channel_manager_open(&mgr, 0x0019, 0, log_event, &log, &local_cid, 0) ==
              BT_OK);
    BT_CHECK(take_last_sig_command(&ft, &hdr, &cmd_data));
    identifier = hdr.identifier;

    /* Still WAIT_CONNECT_RSP -- close now, before any response arrives.
     * project.md's "remoção durante negociação": must clean up locally,
     * with no wire traffic for a channel that never finished connecting. */
    bt_l2cap_channel_manager_close(&mgr, local_cid, 5);

    BT_CHECK(log.closed_count == 1);
    BT_CHECK(log.last_close_reason == BT_L2CAP_CLOSE_LOCAL);
    BT_CHECK(find_chan(&mgr, local_cid) == NULL);
    BT_CHECK(ft.send_count == 1); /* only the original Connection Request; no Disconnect sent */

    /* A stale Connection Response for the old identifier arrives late.
     * Must be silently ignored, not resurrect or corrupt anything. */
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_connection_response(&w, identifier, 0x0050, local_cid,
                                             BT_L2CAP_CONN_RESULT_SUCCESS, 0);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 10);

    BT_CHECK(log.open_count == 0);
    BT_CHECK(log.closed_count == 1); /* unchanged */
    BT_CHECK(find_chan(&mgr, local_cid) == NULL);

    /* Same check, but abandoning mid-CONFIG instead of mid-connect. */
    struct event_log log2 = {0, 0, 0, BT_L2CAP_CLOSE_LOCAL, {0}, 0};
    uint16_t cid2;

    BT_CHECK(bt_l2cap_channel_manager_open(&mgr, 0x0019, 0, log_event, &log2, &cid2, 20) == BT_OK);
    BT_CHECK(take_last_sig_command(&ft, &hdr, &cmd_data));
    identifier = hdr.identifier;
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_connection_response(&w, identifier, 0x0051, cid2,
                                             BT_L2CAP_CONN_RESULT_SUCCESS, 0);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 30);
    BT_CHECK(find_chan(&mgr, cid2)->state == BT_L2CAP_CHAN_CONFIG);

    bt_l2cap_channel_manager_close(&mgr, cid2, 40);
    BT_CHECK(log2.closed_count == 1);
    BT_CHECK(log2.last_close_reason == BT_L2CAP_CLOSE_LOCAL);
    BT_CHECK(find_chan(&mgr, cid2) == NULL);
}

static void test_timeout_waiting_for_connect_rsp(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct event_log log = {0, 0, 0, BT_L2CAP_CLOSE_LOCAL, {0}, 0};
    uint16_t local_cid;

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);

    BT_CHECK(bt_l2cap_channel_manager_open(&mgr, 0x0019, 0, log_event, &log, &local_cid, 0) ==
              BT_OK);

    bt_l2cap_channel_manager_tick(&mgr, BT_L2CAP_RTX_TIMEOUT_US - 1);
    BT_CHECK(log.closed_count == 0);

    bt_l2cap_channel_manager_tick(&mgr, BT_L2CAP_RTX_TIMEOUT_US + 1);
    BT_CHECK(log.closed_count == 1);
    BT_CHECK(log.last_close_reason == BT_L2CAP_CLOSE_TIMEOUT);
    BT_CHECK(find_chan(&mgr, local_cid) == NULL);
}

static void test_pool_exhaustion(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct event_log log = {0, 0, 0, BT_L2CAP_CLOSE_LOCAL, {0}, 0};
    uint16_t cid;
    int i;

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);

    for (i = 0; i < BT_L2CAP_CHANNEL_MANAGER_MAX_CHANNELS; i++)
        BT_CHECK(bt_l2cap_channel_manager_open(&mgr, 0x0019, 0, log_event, &log, &cid, 0) ==
                  BT_OK);

    BT_CHECK(bt_l2cap_channel_manager_open(&mgr, 0x0019, 0, log_event, &log, &cid, 0) ==
              BT_ERR_NO_RESOURCES);
}

static void test_fragmented_data(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct event_log log = {0, 0, 0, BT_L2CAP_CLOSE_LOCAL, {0}, 0};
    uint16_t local_cid;
    struct bt_l2cap_sig_header hdr;
    const uint8_t *cmd_data;
    uint8_t identifier;
    uint8_t buf[32];
    struct bt_buf_writer w;
    uint8_t payload[60];
    size_t i;

    for (i = 0; i < sizeof(payload); i++)
        payload[i] = (uint8_t)i;

    fake_transport_init(&ft);
    /* Small fragment size forces several ACL fragments for one L2CAP PDU. */
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 16);

    BT_CHECK(bt_l2cap_channel_manager_open(&mgr, 0x0019, 0, log_event, &log, &local_cid, 0) ==
              BT_OK);
    BT_CHECK(take_last_sig_command(&ft, &hdr, &cmd_data));
    identifier = hdr.identifier;
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_connection_response(&w, identifier, 0x0050, local_cid,
                                             BT_L2CAP_CONN_RESULT_SUCCESS, 0);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 10);
    BT_CHECK(take_last_sig_command(&ft, &hdr, &cmd_data));
    identifier = hdr.identifier;
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_configure_response(&w, identifier, 0x0050, 0,
                                            BT_L2CAP_CONFIG_RESULT_SUCCESS, 0);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 20);
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_configure_request(&w, 0x77, local_cid, 0, 0);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 30);
    BT_CHECK(log.open_count == 1);
    ft.captured_complete = false;

    BT_CHECK(bt_l2cap_channel_manager_send(&mgr, local_cid, payload, sizeof(payload), 40) ==
              BT_OK);
    BT_CHECK(ft.send_count > 1); /* had to fragment */
    BT_CHECK(ft.captured_complete);
    BT_CHECK(ft.captured_pdu_len == BT_L2CAP_HEADER_LEN + sizeof(payload));
}

static void test_fixed_channel_open_and_data(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct event_log log = {0, 0, 0, BT_L2CAP_CLOSE_LOCAL, {0}, 0};
    static const uint8_t payload[] = {0xAA, 0xBB, 0xCC};

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);

    /* No handshake at all -- OPEN fires synchronously, right here. */
    BT_CHECK(bt_l2cap_channel_manager_open_fixed(&mgr, BT_L2CAP_CID_ATT, log_event, &log) == BT_OK);
    BT_CHECK(log.open_count == 1);
    BT_CHECK(ft.send_count == 0); /* nothing goes on the wire to establish it */

    const struct bt_l2cap_channel *chan = find_chan(&mgr, BT_L2CAP_CID_ATT);
    BT_CHECK(chan != NULL);
    BT_CHECK(chan->state == BT_L2CAP_CHAN_OPEN);
    BT_CHECK(chan->remote_cid == BT_L2CAP_CID_ATT); /* fixed CIDs match on both ends */

    /* Re-registering the same fixed CID must fail. */
    BT_CHECK(bt_l2cap_channel_manager_open_fixed(&mgr, BT_L2CAP_CID_ATT, log_event, &log) ==
              BT_ERR_INVALID_ARGUMENT);

    BT_CHECK(bt_l2cap_channel_manager_send(&mgr, BT_L2CAP_CID_ATT, payload, sizeof(payload), 0) ==
              BT_OK);
    BT_CHECK(ft.captured_complete);

    feed_data(&mgr, BT_L2CAP_CID_ATT, payload, sizeof(payload), 10);
    BT_CHECK(log.data_count == 1);
    BT_CHECK(log.last_data_len == sizeof(payload));

    /* Closing a fixed channel is purely local -- no Disconnection
     * Request, since there's no on-wire teardown for a fixed channel. */
    ft.captured_complete = false;
    int send_count_before = ft.send_count;
    bt_l2cap_channel_manager_close(&mgr, BT_L2CAP_CID_ATT, 20);
    BT_CHECK(log.closed_count == 1);
    BT_CHECK(log.last_close_reason == BT_L2CAP_CLOSE_LOCAL);
    BT_CHECK(ft.send_count == send_count_before);
    BT_CHECK(find_chan(&mgr, BT_L2CAP_CID_ATT) == NULL);
}

/* Acceptor role: the peer opens a channel to a PSM we listen on. */
static void test_incoming_connection_accepted(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct event_log log = {0, 0, 0, BT_L2CAP_CLOSE_LOCAL, {0}, 0};
    struct bt_l2cap_sig_header hdr;
    const uint8_t *cmd_data;
    struct bt_l2cap_connection_response crsp;
    uint8_t buf[32];
    struct bt_buf_writer w;
    uint16_t local_cid;
    uint8_t identifier;
    static const uint8_t payload[] = {0xA1, 0x01, 0x02};

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0042, BT_L2CAP_CID_SIGNALING_CLASSIC, 200);
    BT_CHECK(bt_l2cap_channel_manager_listen(&mgr, 0x0013, 200, log_event, &log) == BT_OK);

    /* a request for a PSM nobody listens on is refused */
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_connection_request(&w, 0x10, 0x0011, 0x0060);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 5);
    BT_CHECK(take_last_sig_command(&ft, &hdr, &cmd_data));
    BT_CHECK(hdr.code == BT_L2CAP_SIG_CONNECTION_RESPONSE);
    BT_CHECK(bt_l2cap_sig_parse_connection_response(cmd_data, hdr.length, &crsp) == BT_OK);
    BT_CHECK(crsp.result == BT_L2CAP_CONN_RESULT_REFUSED_PSM);
    BT_CHECK(log.open_count == 0);

    /* the listened PSM: response success, then our Configure Request */
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_connection_request(&w, 0x11, 0x0013, 0x0061);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 10);
    /* two PDUs went out; the capture keeps the last (Configure Request) */
    BT_CHECK(ft.send_count == 3);
    BT_CHECK(take_last_sig_command(&ft, &hdr, &cmd_data));
    BT_CHECK(hdr.code == BT_L2CAP_SIG_CONFIGURE_REQUEST);
    identifier = hdr.identifier;
    local_cid = BT_L2CAP_CID_DYNAMIC_START;
    BT_CHECK(find_chan(&mgr, local_cid) != NULL);
    BT_CHECK(find_chan(&mgr, local_cid)->remote_cid == 0x0061);
    BT_CHECK(find_chan(&mgr, local_cid)->psm == 0x0013);

    /* peer accepts our config and configures its side */
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_configure_response(&w, identifier, 0x0061, 0, BT_L2CAP_CONFIG_RESULT_SUCCESS, 0);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 20);
    BT_CHECK(log.open_count == 0);
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_configure_request(&w, 0x12, local_cid, 0, 48);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 30);
    BT_CHECK(log.open_count == 1);
    BT_CHECK(find_chan(&mgr, local_cid)->state == BT_L2CAP_CHAN_OPEN);

    /* data flows to the listener's callback */
    feed_data(&mgr, local_cid, payload, sizeof(payload), 40);
    BT_CHECK(log.data_count == 1);
    BT_CHECK(log.last_data_len == 3 && log.last_data[0] == 0xA1);

    /* unlisten refuses the next request */
    bt_l2cap_channel_manager_unlisten(&mgr, 0x0013);
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_connection_request(&w, 0x13, 0x0013, 0x0062);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 50);
    BT_CHECK(take_last_sig_command(&ft, &hdr, &cmd_data));
    BT_CHECK(bt_l2cap_sig_parse_connection_response(cmd_data, hdr.length, &crsp) == BT_OK);
    BT_CHECK(crsp.result == BT_L2CAP_CONN_RESULT_REFUSED_PSM);
}

void run_l2cap_channel_tests(void)
{
    test_incoming_connection_accepted();
    test_open_full_handshake();
    test_connection_refused();
    test_data_round_trip();
    test_local_disconnect_from_open();
    test_peer_initiated_disconnect();
    test_removal_during_negotiation();
    test_timeout_waiting_for_connect_rsp();
    test_pool_exhaustion();
    test_fragmented_data();
    test_fixed_channel_open_and_data();
}
