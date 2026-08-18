#include "test_gatt_client.h"
#include "../support/test.h"

#include <btcore/gatt_client.h>

#include <string.h>

/* Same fake transport pattern used throughout tests/l2cap and
 * tests/sdp: reassembles whatever gets sent via send_acl() into complete
 * L2CAP PDUs, never answers on its own. */
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

static bool take_last_att_pdu(struct fake_transport *ft, const uint8_t **payload,
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
    if (hdr.cid != BT_L2CAP_CID_ATT)
        return false;

    *payload = bt_buf_reader_peek(&r, hdr.length);
    *payload_len = hdr.length;
    return *payload != NULL;
}

static void feed_att_pdu(struct bt_l2cap_channel_manager *mgr, const uint8_t *att_pdu,
                          size_t att_pdu_len, uint64_t now_us)
{
    uint8_t pdu[BT_L2CAP_HEADER_LEN + 300];
    struct bt_buf_writer pw;
    uint8_t acl[BT_HCI_ACL_HEADER_LEN + BT_L2CAP_HEADER_LEN + 300];
    struct bt_buf_writer aw;
    struct bt_buf_reader ar;
    struct bt_hci_acl_header acl_hdr;

    bt_buf_writer_init(&pw, pdu, sizeof(pdu));
    bt_l2cap_encode_header(&pw, (uint16_t)att_pdu_len, BT_L2CAP_CID_ATT);
    bt_buf_writer_write_bytes(&pw, att_pdu, att_pdu_len);

    bt_buf_writer_init(&aw, acl, sizeof(acl));
    bt_hci_encode_acl_header(&aw, mgr->handle, 0x00, 0x00, (uint16_t)bt_buf_writer_len(&pw));
    bt_buf_writer_write_bytes(&aw, pdu, bt_buf_writer_len(&pw));

    bt_buf_reader_init(&ar, acl, bt_buf_writer_len(&aw));
    bt_hci_parse_acl_header(&ar, &acl_hdr);
    bt_l2cap_channel_manager_on_acl(mgr, acl_hdr.pb_flag, bt_buf_reader_peek(&ar, acl_hdr.data_len),
                                     acl_hdr.data_len, now_us);
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
    enum bt_gatt_client_result result;
    uint8_t att_error_code;
    struct bt_gatt_service services[BT_GATT_CLIENT_MAX_SERVICES];
    struct bt_gatt_characteristic characteristics[BT_GATT_CLIENT_MAX_CHARACTERISTICS];
    struct bt_gatt_descriptor descriptors[BT_GATT_CLIENT_MAX_DESCRIPTORS];
    size_t count_items;
    uint8_t value[BT_GATT_CLIENT_MAX_VALUE_LEN];
    size_t value_len;
};

static void on_complete(struct bt_gatt_client_completion *completion, void *user_data)
{
    struct complete_log *log = (struct complete_log *)user_data;

    log->count++;
    log->result = completion->result;
    log->att_error_code = completion->att_error_code;
    log->count_items = completion->count;
    if (completion->services != NULL)
        memcpy(log->services, completion->services, completion->count * sizeof(struct bt_gatt_service));
    if (completion->characteristics != NULL)
        memcpy(log->characteristics, completion->characteristics,
               completion->count * sizeof(struct bt_gatt_characteristic));
    if (completion->descriptors != NULL)
        memcpy(log->descriptors, completion->descriptors,
               completion->count * sizeof(struct bt_gatt_descriptor));
    if (completion->value != NULL && completion->value_len <= sizeof(log->value))
    {
        memcpy(log->value, completion->value, completion->value_len);
        log->value_len = completion->value_len;
    }
}

/* Connects and completes MTU exchange (server accepts, returns 100). */
static void connect_client(struct bt_l2cap_channel_manager *mgr, struct fake_transport *ft,
                            struct bt_gatt_client *client)
{
    struct connect_log log = {0, false};
    const uint8_t *payload;
    size_t payload_len;
    uint8_t rsp[3];
    struct bt_buf_writer w;

    BT_CHECK(bt_gatt_client_connect(client, on_connect, &log, 0) == BT_OK);
    BT_CHECK(log.count == 0); /* not done until MTU exchange completes */

    BT_CHECK(take_last_att_pdu(ft, &payload, &payload_len));
    BT_CHECK(payload[0] == BT_ATT_OPCODE_EXCHANGE_MTU_REQUEST);

    bt_buf_writer_init(&w, rsp, sizeof(rsp));
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_EXCHANGE_MTU_RESPONSE);
    bt_buf_writer_write_le16(&w, 100);
    feed_att_pdu(mgr, rsp, bt_buf_writer_len(&w), 0);

    BT_CHECK(log.count == 1);
    BT_CHECK(log.success);
    BT_CHECK(client->mtu == 100);
}

static void test_connect_mtu_exchange(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct bt_gatt_client client;

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);
    bt_gatt_client_init(&client, &mgr);

    connect_client(&mgr, &ft, &client);
    BT_CHECK(client.channel_ready);
}

static void test_connect_mtu_error_still_connects(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct bt_gatt_client client;
    struct connect_log log = {0, false};
    const uint8_t *payload;
    size_t payload_len;
    uint8_t rsp[5];
    struct bt_buf_writer w;

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);
    bt_gatt_client_init(&client, &mgr);

    BT_CHECK(bt_gatt_client_connect(&client, on_connect, &log, 0) == BT_OK);
    BT_CHECK(take_last_att_pdu(&ft, &payload, &payload_len));

    bt_buf_writer_init(&w, rsp, sizeof(rsp));
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_ERROR_RESPONSE);
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_EXCHANGE_MTU_REQUEST);
    bt_buf_writer_write_le16(&w, 0);
    bt_buf_writer_write_u8(&w, 0x06); /* Request Not Supported */
    feed_att_pdu(&mgr, rsp, bt_buf_writer_len(&w), 0);

    BT_CHECK(log.count == 1);
    BT_CHECK(log.success); /* MTU negotiation failing doesn't fail the connection */
    BT_CHECK(client.mtu == BT_ATT_DEFAULT_MTU);
}

static void test_discover_services_single_round(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct bt_gatt_client client;
    struct complete_log clog = {0};
    const uint8_t *payload;
    size_t payload_len;
    uint8_t rsp[32];
    struct bt_buf_writer w;

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);
    bt_gatt_client_init(&client, &mgr);
    connect_client(&mgr, &ft, &client);

    BT_CHECK(bt_gatt_client_discover_services(&client, on_complete, &clog, 0) == BT_OK);
    BT_CHECK(take_last_att_pdu(&ft, &payload, &payload_len));
    BT_CHECK(payload[0] == BT_ATT_OPCODE_READ_BY_GROUP_TYPE_REQUEST);

    /* Two services, ending at 0xFFFF -- discovery must stop there without
     * needing another round. */
    bt_buf_writer_init(&w, rsp, sizeof(rsp));
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_READ_BY_GROUP_TYPE_RESPONSE);
    bt_buf_writer_write_u8(&w, 6); /* entry length */
    bt_buf_writer_write_le16(&w, 0x0001);
    bt_buf_writer_write_le16(&w, 0x0005);
    bt_buf_writer_write_le16(&w, 0x1800);
    bt_buf_writer_write_le16(&w, 0x0006);
    bt_buf_writer_write_le16(&w, 0xFFFF);
    bt_buf_writer_write_le16(&w, 0x1801);
    feed_att_pdu(&mgr, rsp, bt_buf_writer_len(&w), 10);

    BT_CHECK(clog.count == 1);
    BT_CHECK(clog.result == BT_GATT_CLIENT_OK);
    BT_CHECK(clog.count_items == 2);
    BT_CHECK(clog.services[0].start_handle == 0x0001 && clog.services[0].uuid16 == 0x1800);
    BT_CHECK(clog.services[1].start_handle == 0x0006 && clog.services[1].uuid16 == 0x1801);
    BT_CHECK(!client.busy);
}

static void test_discover_services_multi_round_terminated_by_error(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct bt_gatt_client client;
    struct complete_log clog = {0};
    const uint8_t *payload;
    size_t payload_len;
    uint8_t rsp[32];
    struct bt_buf_writer w;

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);
    bt_gatt_client_init(&client, &mgr);
    connect_client(&mgr, &ft, &client);

    BT_CHECK(bt_gatt_client_discover_services(&client, on_complete, &clog, 0) == BT_OK);
    BT_CHECK(take_last_att_pdu(&ft, &payload, &payload_len));

    /* First round: one service ending well before 0xFFFF -- must trigger
     * a second request automatically. */
    bt_buf_writer_init(&w, rsp, sizeof(rsp));
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_READ_BY_GROUP_TYPE_RESPONSE);
    bt_buf_writer_write_u8(&w, 6);
    bt_buf_writer_write_le16(&w, 0x0001);
    bt_buf_writer_write_le16(&w, 0x0005);
    bt_buf_writer_write_le16(&w, 0x1800);
    feed_att_pdu(&mgr, rsp, bt_buf_writer_len(&w), 10);

    BT_CHECK(clog.count == 0);
    BT_CHECK(take_last_att_pdu(&ft, &payload, &payload_len));
    BT_CHECK(payload[0] == BT_ATT_OPCODE_READ_BY_GROUP_TYPE_REQUEST);
    BT_CHECK(payload[1] == 0x06 && payload[2] == 0x00); /* starting handle = 0x0006 */

    /* Second round: server signals no more results. */
    bt_buf_writer_init(&w, rsp, sizeof(rsp));
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_ERROR_RESPONSE);
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_READ_BY_GROUP_TYPE_REQUEST);
    bt_buf_writer_write_le16(&w, 0x0006);
    bt_buf_writer_write_u8(&w, BT_ATT_ERROR_ATTRIBUTE_NOT_FOUND);
    feed_att_pdu(&mgr, rsp, bt_buf_writer_len(&w), 20);

    BT_CHECK(clog.count == 1);
    BT_CHECK(clog.result == BT_GATT_CLIENT_OK); /* not an error */
    BT_CHECK(clog.count_items == 1);
    BT_CHECK(clog.services[0].uuid16 == 0x1800);
}

static void test_discover_characteristics(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct bt_gatt_client client;
    struct complete_log clog = {0};
    const uint8_t *payload;
    size_t payload_len;
    uint8_t rsp[32];
    struct bt_buf_writer w;

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);
    bt_gatt_client_init(&client, &mgr);
    connect_client(&mgr, &ft, &client);

    /* Range end (0x0002) matches the single characteristic declaration's
     * own handle below, so this one response is enough: the discovery
     * loop only asks again while the last handle seen is still short of
     * the requested range end. */
    BT_CHECK(bt_gatt_client_discover_characteristics(&client, 0x0001, 0x0002, on_complete, &clog, 0) ==
              BT_OK);
    BT_CHECK(take_last_att_pdu(&ft, &payload, &payload_len));
    BT_CHECK(payload[0] == BT_ATT_OPCODE_READ_BY_TYPE_REQUEST);

    bt_buf_writer_init(&w, rsp, sizeof(rsp));
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_READ_BY_TYPE_RESPONSE);
    bt_buf_writer_write_u8(&w, 7); /* handle(2) + properties(1) + value_handle(2) + uuid16(2) */
    bt_buf_writer_write_le16(&w, 0x0002);
    bt_buf_writer_write_u8(&w, 0x0A); /* properties: read+write */
    bt_buf_writer_write_le16(&w, 0x0003);
    bt_buf_writer_write_le16(&w, 0x2A00); /* Device Name */
    feed_att_pdu(&mgr, rsp, bt_buf_writer_len(&w), 10);

    BT_CHECK(clog.count == 1);
    BT_CHECK(clog.result == BT_GATT_CLIENT_OK);
    BT_CHECK(clog.count_items == 1);
    BT_CHECK(clog.characteristics[0].value_handle == 0x0003);
    BT_CHECK(clog.characteristics[0].uuid16 == 0x2A00);
    BT_CHECK(clog.characteristics[0].properties == 0x0A);
}

static void test_read_and_write(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct bt_gatt_client client;
    struct complete_log rlog = {0};
    struct complete_log wlog = {0};
    const uint8_t *payload;
    size_t payload_len;
    uint8_t rsp[8];
    struct bt_buf_writer w;
    static const uint8_t write_value[] = {0x01};

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);
    bt_gatt_client_init(&client, &mgr);
    connect_client(&mgr, &ft, &client);

    BT_CHECK(bt_gatt_client_read(&client, 0x0003, on_complete, &rlog, 0) == BT_OK);
    BT_CHECK(take_last_att_pdu(&ft, &payload, &payload_len));
    BT_CHECK(payload[0] == BT_ATT_OPCODE_READ_REQUEST);

    bt_buf_writer_init(&w, rsp, sizeof(rsp));
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_READ_RESPONSE);
    bt_buf_writer_write_bytes(&w, (const uint8_t *)"hi", 2);
    feed_att_pdu(&mgr, rsp, bt_buf_writer_len(&w), 10);

    BT_CHECK(rlog.count == 1);
    BT_CHECK(rlog.result == BT_GATT_CLIENT_OK);
    BT_CHECK(rlog.value_len == 2);
    BT_CHECK(memcmp(rlog.value, "hi", 2) == 0);

    BT_CHECK(bt_gatt_client_write(&client, 0x0005, write_value, sizeof(write_value), on_complete,
                                    &wlog, 10) == BT_OK);
    BT_CHECK(take_last_att_pdu(&ft, &payload, &payload_len));
    BT_CHECK(payload[0] == BT_ATT_OPCODE_WRITE_REQUEST);
    BT_CHECK(payload[3] == 0x01);

    bt_buf_writer_init(&w, rsp, sizeof(rsp));
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_WRITE_RESPONSE);
    feed_att_pdu(&mgr, rsp, bt_buf_writer_len(&w), 20);

    BT_CHECK(wlog.count == 1);
    BT_CHECK(wlog.result == BT_GATT_CLIENT_OK);
}

static void test_discover_descriptors(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct bt_gatt_client client;
    struct complete_log log = {0};
    const uint8_t *payload;
    size_t payload_len;
    uint8_t rsp[16];
    struct bt_buf_writer w;

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);
    bt_gatt_client_init(&client, &mgr);
    connect_client(&mgr, &ft, &client);

    BT_CHECK(bt_gatt_client_discover_descriptors(&client, 0x0003, 0x0004,
                                                  on_complete, &log, 0) == BT_OK);
    BT_CHECK(take_last_att_pdu(&ft, &payload, &payload_len));
    BT_CHECK(payload[0] == BT_ATT_OPCODE_FIND_INFORMATION_REQUEST);
    BT_CHECK(payload[1] == 3 && payload[3] == 4);

    bt_buf_writer_init(&w, rsp, sizeof(rsp));
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_FIND_INFORMATION_RESPONSE);
    bt_buf_writer_write_u8(&w, 1);
    bt_buf_writer_write_le16(&w, 3);
    bt_buf_writer_write_le16(&w, 0x2902);
    bt_buf_writer_write_le16(&w, 4);
    bt_buf_writer_write_le16(&w, 0x2908);
    feed_att_pdu(&mgr, rsp, bt_buf_writer_len(&w), 10);

    BT_CHECK(log.count == 1 && log.result == BT_GATT_CLIENT_OK);
    BT_CHECK(log.count_items == 2);
    BT_CHECK(log.descriptors[0].handle == 3 &&
             log.descriptors[0].uuid16 == 0x2902);
    BT_CHECK(log.descriptors[1].handle == 4 &&
             log.descriptors[1].uuid16 == 0x2908);
}

static void test_read_att_error(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct bt_gatt_client client;
    struct complete_log rlog = {0};
    const uint8_t *payload;
    size_t payload_len;
    uint8_t rsp[8];
    struct bt_buf_writer w;

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);
    bt_gatt_client_init(&client, &mgr);
    connect_client(&mgr, &ft, &client);

    BT_CHECK(bt_gatt_client_read(&client, 0x0099, on_complete, &rlog, 0) == BT_OK);
    BT_CHECK(take_last_att_pdu(&ft, &payload, &payload_len));

    bt_buf_writer_init(&w, rsp, sizeof(rsp));
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_ERROR_RESPONSE);
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_READ_REQUEST);
    bt_buf_writer_write_le16(&w, 0x0099);
    bt_buf_writer_write_u8(&w, 0x02); /* Read Not Permitted */
    feed_att_pdu(&mgr, rsp, bt_buf_writer_len(&w), 10);

    BT_CHECK(rlog.count == 1);
    BT_CHECK(rlog.result == BT_GATT_CLIENT_ERROR_ATT);
    BT_CHECK(rlog.att_error_code == 0x02);
}

static void test_long_read_uses_read_blob(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct bt_gatt_client client;
    struct complete_log rlog = {0};
    const uint8_t *payload;
    size_t payload_len;
    uint8_t rsp[100];
    struct bt_buf_writer w;
    size_t i;

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);
    bt_gatt_client_init(&client, &mgr);
    connect_client(&mgr, &ft, &client); /* negotiated MTU = 100 */

    BT_CHECK(bt_gatt_client_read(&client, 0x0042, on_complete, &rlog, 0) == BT_OK);
    BT_CHECK(take_last_att_pdu(&ft, &payload, &payload_len));
    BT_CHECK(payload[0] == BT_ATT_OPCODE_READ_REQUEST);

    bt_buf_writer_init(&w, rsp, sizeof(rsp));
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_READ_RESPONSE);
    for (i = 0; i < 99; ++i)
        bt_buf_writer_write_u8(&w, (uint8_t)i);
    feed_att_pdu(&mgr, rsp, bt_buf_writer_len(&w), 10);

    BT_CHECK(rlog.count == 0);
    BT_CHECK(take_last_att_pdu(&ft, &payload, &payload_len));
    BT_CHECK(payload_len == 5 && payload[0] == BT_ATT_OPCODE_READ_BLOB_REQUEST);
    BT_CHECK(payload[1] == 0x42 && payload[2] == 0x00);
    BT_CHECK(payload[3] == 99 && payload[4] == 0);

    bt_buf_writer_init(&w, rsp, sizeof(rsp));
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_READ_BLOB_RESPONSE);
    bt_buf_writer_write_u8(&w, 0xAA);
    bt_buf_writer_write_u8(&w, 0xBB);
    feed_att_pdu(&mgr, rsp, bt_buf_writer_len(&w), 20);

    BT_CHECK(rlog.count == 1 && rlog.result == BT_GATT_CLIENT_OK);
    BT_CHECK(rlog.value_len == 101);
    BT_CHECK(rlog.value[0] == 0 && rlog.value[98] == 98);
    BT_CHECK(rlog.value[99] == 0xAA && rlog.value[100] == 0xBB);
}

struct notify_log
{
    int count;
    uint16_t handle;
    uint8_t value[16];
    size_t value_len;
    bool is_indication;
};

static void on_notify(uint16_t handle, const uint8_t *value, size_t value_len, bool is_indication,
                       void *user_data)
{
    struct notify_log *log = (struct notify_log *)user_data;

    log->count++;
    log->handle = handle;
    log->is_indication = is_indication;
    log->value_len = value_len;
    if (value_len <= sizeof(log->value))
        memcpy(log->value, value, value_len);
}

static void test_notification_and_indication(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct bt_gatt_client client;
    struct notify_log nlog = {0, 0, {0}, 0, false};
    const uint8_t *payload;
    size_t payload_len;
    uint8_t pdu[8];
    struct bt_buf_writer w;

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);
    bt_gatt_client_init(&client, &mgr);
    connect_client(&mgr, &ft, &client);
    bt_gatt_client_set_notify_handler(&client, on_notify, &nlog);

    /* Unsolicited, and delivered even though client.busy is false. */
    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_HANDLE_VALUE_NOTIFICATION);
    bt_buf_writer_write_le16(&w, 0x0010);
    bt_buf_writer_write_bytes(&w, (const uint8_t *)"AB", 2);
    feed_att_pdu(&mgr, pdu, bt_buf_writer_len(&w), 10);

    BT_CHECK(nlog.count == 1);
    BT_CHECK(nlog.handle == 0x0010);
    BT_CHECK(!nlog.is_indication);
    BT_CHECK(nlog.value_len == 2 && memcmp(nlog.value, "AB", 2) == 0);
    BT_CHECK(!ft.captured_complete); /* no confirmation for a notification */

    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_HANDLE_VALUE_INDICATION);
    bt_buf_writer_write_le16(&w, 0x0011);
    bt_buf_writer_write_bytes(&w, (const uint8_t *)"C", 1);
    feed_att_pdu(&mgr, pdu, bt_buf_writer_len(&w), 20);

    BT_CHECK(nlog.count == 2);
    BT_CHECK(nlog.is_indication);
    BT_CHECK(take_last_att_pdu(&ft, &payload, &payload_len));
    BT_CHECK(payload_len == 1 && payload[0] == BT_ATT_OPCODE_HANDLE_VALUE_CONFIRMATION);
}

static void test_channel_closed_while_busy(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct bt_gatt_client client;
    struct complete_log rlog = {0};

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);
    bt_gatt_client_init(&client, &mgr);
    connect_client(&mgr, &ft, &client);

    BT_CHECK(bt_gatt_client_read(&client, 0x0003, on_complete, &rlog, 0) == BT_OK);
    BT_CHECK(client.busy);

    bt_l2cap_channel_manager_close(&mgr, BT_L2CAP_CID_ATT, 30); /* local close (fixed channel) */

    BT_CHECK(rlog.count == 1);
    BT_CHECK(rlog.result == BT_GATT_CLIENT_ERROR_CLOSED);
    BT_CHECK(!client.channel_ready);
}

static void test_busy_guard(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct bt_gatt_client client;
    struct complete_log rlog = {0};

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);
    bt_gatt_client_init(&client, &mgr);
    connect_client(&mgr, &ft, &client);

    BT_CHECK(bt_gatt_client_read(&client, 0x0003, on_complete, &rlog, 0) == BT_OK);
    BT_CHECK(bt_gatt_client_discover_services(&client, on_complete, &rlog, 0) ==
              BT_ERR_INVALID_ARGUMENT);
}

static void test_request_timeout_and_late_response(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct bt_gatt_client client;
    struct complete_log rlog = {0};
    uint8_t rsp[] = {BT_ATT_OPCODE_READ_RESPONSE, 0x42};

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);
    bt_gatt_client_init(&client, &mgr);
    connect_client(&mgr, &ft, &client);

    BT_CHECK(bt_gatt_client_read(&client, 0x0003, on_complete, &rlog, 100) == BT_OK);
    bt_gatt_client_tick(&client, 100 + BT_GATT_CLIENT_REQUEST_TIMEOUT_US - 1);
    BT_CHECK(rlog.count == 0);
    BT_CHECK(client.busy);

    bt_gatt_client_tick(&client, 100 + BT_GATT_CLIENT_REQUEST_TIMEOUT_US);
    BT_CHECK(rlog.count == 1);
    BT_CHECK(rlog.result == BT_GATT_CLIENT_ERROR_TIMEOUT);
    BT_CHECK(!client.busy);

    feed_att_pdu(&mgr, rsp, sizeof(rsp), 100 + BT_GATT_CLIENT_REQUEST_TIMEOUT_US + 1);
    BT_CHECK(rlog.count == 1); /* late response must not complete a newer operation */
}

static void test_mtu_timeout_falls_back_to_default(void)
{
    struct fake_transport ft;
    struct bt_l2cap_channel_manager mgr;
    struct bt_gatt_client client;
    struct connect_log log = {0, false};

    fake_transport_init(&ft);
    bt_l2cap_channel_manager_init(&mgr, &ft.base, 0x0041, BT_L2CAP_CID_SIGNALING_LE, 200);
    bt_gatt_client_init(&client, &mgr);

    BT_CHECK(bt_gatt_client_connect(&client, on_connect, &log, 50) == BT_OK);
    BT_CHECK(log.count == 0);
    bt_gatt_client_tick(&client, 50 + BT_GATT_CLIENT_REQUEST_TIMEOUT_US);

    BT_CHECK(log.count == 1);
    BT_CHECK(log.success);
    BT_CHECK(client.channel_ready);
    BT_CHECK(client.mtu == BT_ATT_DEFAULT_MTU);
}

void run_gatt_client_tests(void)
{
    test_connect_mtu_exchange();
    test_connect_mtu_error_still_connects();
    test_discover_services_single_round();
    test_discover_services_multi_round_terminated_by_error();
    test_discover_characteristics();
    test_read_and_write();
    test_discover_descriptors();
    test_read_att_error();
    test_long_read_uses_read_blob();
    test_notification_and_indication();
    test_channel_closed_while_busy();
    test_busy_guard();
    test_request_timeout_and_late_response();
    test_mtu_timeout_falls_back_to_default();
}
