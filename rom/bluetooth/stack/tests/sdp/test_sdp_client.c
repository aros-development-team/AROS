#include "test_sdp_client.h"
#include "../support/test.h"

#include <btcore/sdp_client.h>

#include <string.h>

/* Same fake transport pattern as tests/l2cap/test_channel.c: reassembles
 * whatever gets sent via send_acl() into complete L2CAP PDUs so tests can
 * inspect exactly what went on the wire, and never answers on its own. */
struct fake_transport
{
    struct bt_hci_transport base;
    struct bt_l2cap_reassembler sent_reassembler;
    uint8_t captured_pdu[300];
    size_t captured_pdu_len;
    bool captured_complete;
    int send_count;
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

/* Takes the last captured L2CAP PDU (cid + payload), consuming the capture. */
static bool take_last_pdu(struct fake_transport *ft, uint16_t *cid, const uint8_t **payload,
                           size_t *payload_len)
{
    struct bt_buf_reader r;
    struct bt_l2cap_header hdr;

    if (!ft->captured_complete)
        return false;
    ft->captured_complete = false;

    bt_buf_reader_init(&r, ft->captured_pdu, ft->captured_pdu_len);
    if (bt_l2cap_parse_header(&r, &hdr) != BT_OK)
        return false;

    *cid = hdr.cid;
    *payload = bt_buf_reader_peek(&r, hdr.length);
    *payload_len = hdr.length;
    return *payload != NULL;
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
                       size_t data_len, uint64_t now_us)
{
    uint8_t pdu[BT_L2CAP_HEADER_LEN + 300];
    struct bt_buf_writer pw;
    uint8_t acl[BT_HCI_ACL_HEADER_LEN + BT_L2CAP_HEADER_LEN + 300];
    struct bt_buf_writer aw;
    struct bt_buf_reader ar;
    struct bt_hci_acl_header acl_hdr;

    bt_buf_writer_init(&pw, pdu, sizeof(pdu));
    bt_l2cap_encode_header(&pw, (uint16_t)data_len, cid);
    bt_buf_writer_write_bytes(&pw, data, data_len);

    bt_buf_writer_init(&aw, acl, sizeof(acl));
    bt_hci_encode_acl_header(&aw, mgr->handle, 0x00, 0x00, (uint16_t)bt_buf_writer_len(&pw));
    bt_buf_writer_write_bytes(&aw, pdu, bt_buf_writer_len(&pw));

    bt_buf_reader_init(&ar, acl, bt_buf_writer_len(&aw));
    bt_hci_parse_acl_header(&ar, &acl_hdr);
    bt_l2cap_channel_manager_on_acl(mgr, acl_hdr.pb_flag, bt_buf_reader_peek(&ar, acl_hdr.data_len),
                                     acl_hdr.data_len, now_us);
}

/* Drives a full L2CAP handshake to OPEN for whatever channel the SDP
 * client's connect() just started (assumed to be the only one open on
 * this manager), acting as a cooperative peer. */
static void drive_l2cap_open(struct bt_l2cap_channel_manager *mgr, struct fake_transport *ft,
                              uint16_t local_cid, uint16_t remote_cid)
{
    uint16_t cid;
    const uint8_t *payload;
    size_t payload_len;
    struct bt_l2cap_sig_header hdr;
    struct bt_buf_reader r;
    struct bt_l2cap_connection_request creq;
    uint8_t identifier;
    uint8_t buf[32];
    struct bt_buf_writer w;

    BT_CHECK(take_last_pdu(ft, &cid, &payload, &payload_len));
    BT_CHECK(cid == mgr->signaling_cid);
    bt_buf_reader_init(&r, payload, payload_len);
    BT_CHECK(bt_l2cap_sig_parse_header(&r, &hdr) == BT_OK);
    BT_CHECK(hdr.code == BT_L2CAP_SIG_CONNECTION_REQUEST);
    BT_CHECK(bt_l2cap_sig_parse_connection_request(bt_buf_reader_peek(&r, hdr.length), hdr.length,
                                                    &creq) == BT_OK);
    BT_CHECK(creq.psm == BT_SDP_PSM);
    BT_CHECK(creq.source_cid == local_cid);
    identifier = hdr.identifier;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_connection_response(&w, identifier, remote_cid, local_cid,
                                             BT_L2CAP_CONN_RESULT_SUCCESS, 0);
    feed_signaling_command(mgr, buf, bt_buf_writer_len(&w), 0);

    BT_CHECK(take_last_pdu(ft, &cid, &payload, &payload_len)); /* our Configure Request */
    bt_buf_reader_init(&r, payload, payload_len);
    BT_CHECK(bt_l2cap_sig_parse_header(&r, &hdr) == BT_OK);
    BT_CHECK(hdr.code == BT_L2CAP_SIG_CONFIGURE_REQUEST);
    identifier = hdr.identifier;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_configure_response(&w, identifier, remote_cid, 0,
                                            BT_L2CAP_CONFIG_RESULT_SUCCESS, 0);
    feed_signaling_command(mgr, buf, bt_buf_writer_len(&w), 0);

    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_configure_request(&w, 0x50, local_cid, 0, 0);
    feed_signaling_command(mgr, buf, bt_buf_writer_len(&w), 0);

    BT_CHECK(take_last_pdu(ft, &cid, &payload, &payload_len)); /* our Configure Response */
    (void)payload_len;
}

static void build_service_search_response(uint8_t *buf, size_t buf_cap, size_t *out_len,
                                           uint16_t transaction_id, uint16_t total, uint16_t current,
                                           const uint32_t *handles, uint8_t cont_len,
                                           const uint8_t *cont_data)
{
    struct bt_buf_writer w;
    uint8_t i;

    bt_buf_writer_init(&w, buf, buf_cap);
    bt_sdp_encode_header(&w, BT_SDP_PDU_SERVICE_SEARCH_RESPONSE, transaction_id,
                          (uint16_t)(2 + 2 + (size_t)current * 4 + 1 + cont_len));
    bt_buf_writer_write_be16(&w, total);
    bt_buf_writer_write_be16(&w, current);
    for (i = 0; i < current; i++)
        bt_buf_writer_write_be32(&w, handles[i]);
    bt_buf_writer_write_u8(&w, cont_len);
    if (cont_len > 0)
        bt_buf_writer_write_bytes(&w, cont_data, cont_len);
    *out_len = bt_buf_writer_len(&w);
}

struct connect_log
{
    int count;
    bool success;
};

static void on_connect(bool success, void *user_data)
{
    struct connect_log *log = (struct connect_log *)user_data;

    log->count++;
    log->success = success;
}

struct complete_log
{
    int count;
    enum bt_sdp_client_result result;
    uint8_t data[300];
    size_t data_len;
    uint16_t total_count;
};

static void on_complete(struct bt_sdp_client_completion *completion, void *user_data)
{
    struct complete_log *log = (struct complete_log *)user_data;

    log->count++;
    log->result = completion->result;
    log->total_count = completion->total_count;
    log->data_len = completion->data_len;
    if (completion->data != NULL && completion->data_len <= sizeof(log->data))
        memcpy(log->data, completion->data, completion->data_len);
}

static void test_connect_and_search_single_round(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct bt_sdp_client client;
    struct connect_log clog = {0, false};
    struct complete_log slog = {0, BT_SDP_CLIENT_OK, {0}, 0, 0};
    uint8_t pattern[8];
    struct bt_buf_writer pw;
    uint16_t cid;
    const uint8_t *payload;
    size_t payload_len;
    uint8_t rsp[64];
    size_t rsp_len;
    struct bt_sdp_header hdr;
    struct bt_buf_reader r;
    uint32_t handles[1] = {0x00010203};

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_CLASSIC, 200);
    bt_sdp_client_init(&client, &mgr);

    BT_CHECK(bt_sdp_client_connect(&client, on_connect, &clog, 0) == BT_OK);
    drive_l2cap_open(&mgr, &ft, client.local_cid, 0x0060);

    BT_CHECK(clog.count == 1);
    BT_CHECK(clog.success);
    BT_CHECK(client.channel_ready);

    bt_buf_writer_init(&pw, pattern, sizeof(pattern));
    bt_sdp_encode_sequence_header(&pw, 3);
    bt_sdp_encode_uuid16(&pw, 0x0100);

    BT_CHECK(bt_sdp_client_search(&client, pattern, bt_buf_writer_len(&pw), 10, on_complete, &slog,
                                    10) == BT_OK);

    BT_CHECK(take_last_pdu(&ft, &cid, &payload, &payload_len));
    BT_CHECK(cid == 0x0060); /* outbound data addresses the peer's (remote) CID */
    bt_buf_reader_init(&r, payload, payload_len);
    BT_CHECK(bt_sdp_parse_header(&r, &hdr) == BT_OK);
    BT_CHECK(hdr.pdu_id == BT_SDP_PDU_SERVICE_SEARCH_REQUEST);

    build_service_search_response(rsp, sizeof(rsp), &rsp_len, hdr.transaction_id, 1, 1, handles, 0,
                                   NULL);
    feed_data(&mgr, client.local_cid, rsp, rsp_len, 20);

    BT_CHECK(slog.count == 1);
    BT_CHECK(slog.result == BT_SDP_CLIENT_OK);
    BT_CHECK(slog.total_count == 1);
    BT_CHECK(slog.data_len == 4);
    {
        struct bt_buf_reader hr;
        uint32_t h;

        bt_buf_reader_init(&hr, slog.data, slog.data_len);
        BT_CHECK(bt_buf_reader_read_be32(&hr, &h) == BT_OK);
        BT_CHECK(h == 0x00010203);
    }
    BT_CHECK(!client.busy);
}

static void test_search_with_continuation(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct bt_sdp_client client;
    struct connect_log clog = {0, false};
    struct complete_log slog = {0, BT_SDP_CLIENT_OK, {0}, 0, 0};
    uint8_t pattern[4];
    struct bt_buf_writer pw;
    uint16_t cid;
    const uint8_t *payload;
    size_t payload_len;
    uint8_t rsp[64];
    size_t rsp_len;
    struct bt_sdp_header hdr;
    struct bt_buf_reader r;
    uint32_t handle1[1] = {0x11111111};
    uint32_t handle2[1] = {0x22222222};
    static const uint8_t cont_bytes[2] = {0xAA, 0xBB};

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_CLASSIC, 200);
    bt_sdp_client_init(&client, &mgr);

    BT_CHECK(bt_sdp_client_connect(&client, on_connect, &clog, 0) == BT_OK);
    drive_l2cap_open(&mgr, &ft, client.local_cid, 0x0060);

    bt_buf_writer_init(&pw, pattern, sizeof(pattern));
    bt_sdp_encode_sequence_header(&pw, 0);
    BT_CHECK(bt_sdp_client_search(&client, pattern, bt_buf_writer_len(&pw), 10, on_complete, &slog,
                                    0) == BT_OK);

    BT_CHECK(take_last_pdu(&ft, &cid, &payload, &payload_len));
    bt_buf_reader_init(&r, payload, payload_len);
    bt_sdp_parse_header(&r, &hdr);
    build_service_search_response(rsp, sizeof(rsp), &rsp_len, hdr.transaction_id, 2, 1, handle1,
                                   sizeof(cont_bytes), cont_bytes);
    feed_data(&mgr, client.local_cid, rsp, rsp_len, 10);

    /* Not done yet -- the client must have automatically re-issued the
     * request carrying the continuation state we just gave it. */
    BT_CHECK(slog.count == 0);
    BT_CHECK(take_last_pdu(&ft, &cid, &payload, &payload_len));
    bt_buf_reader_init(&r, payload, payload_len);
    bt_sdp_parse_header(&r, &hdr);
    BT_CHECK(hdr.pdu_id == BT_SDP_PDU_SERVICE_SEARCH_REQUEST);
    BT_CHECK(payload[payload_len - 2] == 0xAA && payload[payload_len - 1] == 0xBB);

    build_service_search_response(rsp, sizeof(rsp), &rsp_len, hdr.transaction_id, 2, 1, handle2, 0,
                                   NULL);
    feed_data(&mgr, client.local_cid, rsp, rsp_len, 20);

    BT_CHECK(slog.count == 1);
    BT_CHECK(slog.result == BT_SDP_CLIENT_OK);
    BT_CHECK(slog.data_len == 8); /* both rounds' handles concatenated */
    {
        struct bt_buf_reader hr;
        uint32_t h;

        bt_buf_reader_init(&hr, slog.data, slog.data_len);
        bt_buf_reader_read_be32(&hr, &h);
        BT_CHECK(h == 0x11111111);
        bt_buf_reader_read_be32(&hr, &h);
        BT_CHECK(h == 0x22222222);
    }
}

static void test_get_attributes_split_across_continuation(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct bt_sdp_client client;
    struct connect_log clog = {0, false};
    struct complete_log alog = {0, BT_SDP_CLIENT_OK, {0}, 0, 0};
    uint8_t attr_ids[8];
    struct bt_buf_writer aw;
    uint16_t cid;
    const uint8_t *payload;
    size_t payload_len;
    struct bt_sdp_header hdr;
    struct bt_buf_reader r;

    /* Build a full AttributeList Sequence: [AttributeID=0x0001,
     * AttributeValue=UUID16 0x1101], then split its raw bytes at an
     * arbitrary, element-boundary-crossing offset to simulate a real
     * mid-stream continuation. */
    uint8_t full_list[16];
    struct bt_buf_writer flw;
    uint8_t inner[4];
    struct bt_buf_writer iw;
    uint8_t nested[8];
    struct bt_buf_writer nw;

    bt_buf_writer_init(&iw, inner, sizeof(inner));
    bt_sdp_encode_uuid16(&iw, 0x1101);

    /* nested = AttributeID element + Value element (a Sequence wrapping
     * the UUID16 above) -- measure its real length rather than computing
     * it by hand, matching bt_sdp_parse_service_attribute_response()'s
     * own test in tests/sdp/test_sdp.c. */
    bt_buf_writer_init(&nw, nested, sizeof(nested));
    bt_sdp_encode_uint(&nw, 0x0001, 2);
    bt_sdp_encode_sequence_header(&nw, (uint16_t)bt_buf_writer_len(&iw));
    bt_buf_writer_write_bytes(&nw, inner, bt_buf_writer_len(&iw));

    bt_buf_writer_init(&flw, full_list, sizeof(full_list));
    bt_sdp_encode_sequence_header(&flw, (uint16_t)bt_buf_writer_len(&nw));
    bt_buf_writer_write_bytes(&flw, nested, bt_buf_writer_len(&nw));

    size_t full_len = bt_buf_writer_len(&flw);
    size_t split_at = 3; /* inside the sequence, not aligned to any element */

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);
    bt_sdp_client_init(&client, &mgr);

    BT_CHECK(bt_sdp_client_connect(&client, on_connect, &clog, 0) == BT_OK);
    drive_l2cap_open(&mgr, &ft, client.local_cid, 0x0060);

    bt_buf_writer_init(&aw, attr_ids, sizeof(attr_ids));
    bt_sdp_encode_sequence_header(&aw, 5);
    bt_sdp_encode_uint(&aw, 0x0000FFFFu, 4);

    BT_CHECK(bt_sdp_client_get_attributes(&client, 0x1234, 256, attr_ids, bt_buf_writer_len(&aw),
                                            on_complete, &alog, 0) == BT_OK);

    BT_CHECK(take_last_pdu(&ft, &cid, &payload, &payload_len));
    bt_buf_reader_init(&r, payload, payload_len);
    bt_sdp_parse_header(&r, &hdr);

    /* Round 1: first split_at bytes, with continuation. */
    {
        uint8_t rsp[64];
        struct bt_buf_writer w;
        static const uint8_t cont[1] = {0x01};

        bt_buf_writer_init(&w, rsp, sizeof(rsp));
        bt_sdp_encode_header(&w, BT_SDP_PDU_SERVICE_ATTRIBUTE_RESPONSE, hdr.transaction_id,
                              (uint16_t)(2 + split_at + 1 + 1));
        bt_buf_writer_write_be16(&w, (uint16_t)split_at);
        bt_buf_writer_write_bytes(&w, full_list, split_at);
        bt_buf_writer_write_u8(&w, 1);
        bt_buf_writer_write_bytes(&w, cont, 1);
        feed_data(&mgr, client.local_cid, rsp, bt_buf_writer_len(&w), 10);
    }

    BT_CHECK(alog.count == 0);
    BT_CHECK(take_last_pdu(&ft, &cid, &payload, &payload_len));
    bt_buf_reader_init(&r, payload, payload_len);
    bt_sdp_parse_header(&r, &hdr);
    BT_CHECK(payload[payload_len - 2] == 1 && payload[payload_len - 1] == 0x01);

    /* Round 2: the rest, no continuation. */
    {
        uint8_t rsp[64];
        struct bt_buf_writer w;
        size_t rest = full_len - split_at;

        bt_buf_writer_init(&w, rsp, sizeof(rsp));
        bt_sdp_encode_header(&w, BT_SDP_PDU_SERVICE_ATTRIBUTE_RESPONSE, hdr.transaction_id,
                              (uint16_t)(2 + rest + 1));
        bt_buf_writer_write_be16(&w, (uint16_t)rest);
        bt_buf_writer_write_bytes(&w, full_list + split_at, rest);
        bt_buf_writer_write_u8(&w, 0);
        feed_data(&mgr, client.local_cid, rsp, bt_buf_writer_len(&w), 20);
    }

    BT_CHECK(alog.count == 1);
    BT_CHECK(alog.result == BT_SDP_CLIENT_OK);

    /* The client hands back the parsed Sequence's *nested* bytes (not
     * the outer header), matching bt_sdp_parse_service_attribute_response(). */
    struct bt_sdp_element_iter it;
    struct bt_sdp_element id_elem, val_elem;

    bt_sdp_element_iter_init(&it, alog.data, alog.data_len);
    BT_CHECK(bt_sdp_element_iter_next(&it, &id_elem) == BT_OK);
    BT_CHECK(id_elem.type == BT_SDP_ELEM_UINT && id_elem.uint == 0x0001);
    BT_CHECK(bt_sdp_element_iter_next(&it, &val_elem) == BT_OK);
    BT_CHECK(val_elem.type == BT_SDP_ELEM_SEQUENCE);

    struct bt_sdp_element_iter vit;
    struct bt_sdp_element uuid_elem;

    bt_sdp_element_iter_init(&vit, val_elem.seq_data, val_elem.seq_len);
    BT_CHECK(bt_sdp_element_iter_next(&vit, &uuid_elem) == BT_OK);
    BT_CHECK(uuid_elem.type == BT_SDP_ELEM_UUID16 && uuid_elem.uuid16 == 0x1101);
}

static void test_connect_refused(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct bt_sdp_client client;
    struct connect_log clog = {0, true};
    uint16_t cid;
    const uint8_t *payload;
    size_t payload_len;
    struct bt_l2cap_sig_header hdr;
    struct bt_buf_reader r;
    uint8_t buf[32];
    struct bt_buf_writer w;

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_CLASSIC, 200);
    bt_sdp_client_init(&client, &mgr);

    BT_CHECK(bt_sdp_client_connect(&client, on_connect, &clog, 0) == BT_OK);
    BT_CHECK(take_last_pdu(&ft, &cid, &payload, &payload_len));
    bt_buf_reader_init(&r, payload, payload_len);
    bt_l2cap_sig_parse_header(&r, &hdr);

    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_connection_response(&w, hdr.identifier, 0, client.local_cid,
                                             BT_L2CAP_CONN_RESULT_REFUSED_PSM, 0);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 10);

    BT_CHECK(clog.count == 1);
    BT_CHECK(!clog.success);
    BT_CHECK(!client.channel_ready);
}

static void test_disconnect_while_busy(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct bt_sdp_client client;
    struct connect_log clog = {0, false};
    struct complete_log slog = {0, BT_SDP_CLIENT_OK, {0}, 0, 0};
    uint8_t pattern[4];
    struct bt_buf_writer pw;
    uint16_t cid;
    const uint8_t *payload;
    size_t payload_len;
    uint8_t buf[32];
    struct bt_buf_writer w;

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_CLASSIC, 200);
    bt_sdp_client_init(&client, &mgr);

    BT_CHECK(bt_sdp_client_connect(&client, on_connect, &clog, 0) == BT_OK);
    drive_l2cap_open(&mgr, &ft, client.local_cid, 0x0060);

    bt_buf_writer_init(&pw, pattern, sizeof(pattern));
    bt_sdp_encode_sequence_header(&pw, 0);
    BT_CHECK(bt_sdp_client_search(&client, pattern, bt_buf_writer_len(&pw), 10, on_complete, &slog,
                                    0) == BT_OK);
    ft.captured_complete = false; /* discard the request itself */
    BT_CHECK(client.busy);

    /* Peer disconnects the channel while our search is still in flight. */
    BT_CHECK(take_last_pdu(&ft, &cid, &payload, &payload_len) == false); /* nothing new sent yet */
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_disconnection_request(&w, 0x70, client.local_cid, 0x0060);
    feed_signaling_command(&mgr, buf, bt_buf_writer_len(&w), 30);

    BT_CHECK(slog.count == 1);
    BT_CHECK(slog.result == BT_SDP_CLIENT_ERROR_CLOSED);
    BT_CHECK(!client.busy);
    BT_CHECK(!client.channel_ready);
}

/* An SDP server that never answers must not leave the caller hanging: the
 * tick fails the operation once its deadline passes, and the client is
 * usable again afterwards. */
static void test_search_times_out(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct bt_sdp_client client;
    struct connect_log clog = {0, false};
    struct complete_log slog = {0, BT_SDP_CLIENT_OK, {0}, 0, 0};
    uint8_t pattern[8];
    struct bt_buf_writer pw;
    uint64_t t0 = 1000000ULL;

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_CLASSIC, 200);
    bt_sdp_client_init(&client, &mgr);
    BT_CHECK(bt_sdp_client_connect(&client, on_connect, &clog, 0) == BT_OK);
    drive_l2cap_open(&mgr, &ft, client.local_cid, 0x0060);
    BT_CHECK(clog.success);

    bt_buf_writer_init(&pw, pattern, sizeof(pattern));
    bt_sdp_encode_sequence_header(&pw, 3);
    bt_sdp_encode_uuid16(&pw, 0x0100);
    BT_CHECK(bt_sdp_client_search(&client, pattern, bt_buf_writer_len(&pw), 10, on_complete, &slog,
                                    t0) == BT_OK);
    ft.captured_complete = false;
    BT_CHECK(client.busy);

    /* well within the deadline: nothing happens */
    bt_sdp_client_tick(&client, t0 + BT_SDP_CLIENT_REQUEST_TIMEOUT_US / 2);
    BT_CHECK(client.busy);
    BT_CHECK(slog.count == 0);

    /* past it: the operation fails with TIMEOUT and the client is idle */
    bt_sdp_client_tick(&client, t0 + BT_SDP_CLIENT_REQUEST_TIMEOUT_US);
    BT_CHECK(slog.count == 1);
    BT_CHECK(slog.result == BT_SDP_CLIENT_ERROR_TIMEOUT);
    BT_CHECK(!client.busy);
    BT_CHECK(client.channel_ready);

    /* idle: ticking again is a no-op, and a new search is accepted */
    bt_sdp_client_tick(&client, t0 + 2 * BT_SDP_CLIENT_REQUEST_TIMEOUT_US);
    BT_CHECK(slog.count == 1);
    BT_CHECK(bt_sdp_client_search(&client, pattern, bt_buf_writer_len(&pw), 10, on_complete, &slog,
                                    t0 + 3 * BT_SDP_CLIENT_REQUEST_TIMEOUT_US) == BT_OK);
    BT_CHECK(client.busy);
}

void run_sdp_client_tests(void)
{
    test_connect_and_search_single_round();
    test_search_times_out();
    test_search_with_continuation();
    test_get_attributes_split_across_continuation();
    test_connect_refused();
    test_disconnect_while_busy();
}
