#include "test_hogp_client.h"
#include "../support/test.h"

#include <btcore/hogp_client.h>

#include <string.h>

struct hogp_transport
{
    struct bt_hci_transport base;
    struct bt_l2cap_reassembler reassembler;
    uint8_t pdu[300];
    size_t pdu_len;
    bool ready;
};

static int transport_open(struct bt_hci_transport *transport)
{
    (void)transport;
    return 0;
}

static void transport_close(struct bt_hci_transport *transport)
{
    (void)transport;
}

static int unsupported_send(struct bt_hci_transport *transport,
                            const uint8_t *data, size_t len)
{
    (void)transport;
    (void)data;
    (void)len;
    return -1;
}

static int send_acl(struct bt_hci_transport *transport, const uint8_t *data, size_t len)
{
    struct hogp_transport *fake = transport->impl;
    struct bt_buf_reader r;
    struct bt_hci_acl_header header;
    const uint8_t *payload;

    bt_buf_reader_init(&r, data, len);
    if (bt_hci_parse_acl_header(&r, &header) != BT_OK)
        return -1;
    payload = bt_buf_reader_peek(&r, header.data_len);
    if (payload == NULL)
        return -1;
    if (bt_l2cap_reassembler_feed(&fake->reassembler, header.pb_flag, payload,
                                   header.data_len) == BT_L2CAP_REASSEMBLY_COMPLETE)
    {
        const uint8_t *pdu = bt_l2cap_reassembler_take(&fake->reassembler,
                                                        &fake->pdu_len);

        memcpy(fake->pdu, pdu, fake->pdu_len);
        fake->ready = true;
    }
    return 0;
}

static int start_receive(struct bt_hci_transport *transport,
                         bt_hci_transport_recv_fn receive, void *user_data)
{
    (void)transport;
    (void)receive;
    (void)user_data;
    return 0;
}

static void stop_receive(struct bt_hci_transport *transport)
{
    (void)transport;
}

static const struct bt_hci_transport_ops transport_ops = {
    .open = transport_open,
    .close = transport_close,
    .send_command = unsupported_send,
    .send_acl = send_acl,
    .send_sco = unsupported_send,
    .send_iso = unsupported_send,
    .start_receive = start_receive,
    .stop_receive = stop_receive};

static void transport_init(struct hogp_transport *fake)
{
    memset(fake, 0, sizeof(*fake));
    fake->base.ops = &transport_ops;
    fake->base.impl = fake;
    bt_l2cap_reassembler_init(&fake->reassembler);
}

static uint8_t take_opcode(struct hogp_transport *fake)
{
    struct bt_buf_reader r;
    struct bt_l2cap_header header;
    uint8_t opcode = 0;

    BT_CHECK(fake->ready);
    fake->ready = false;
    bt_buf_reader_init(&r, fake->pdu, fake->pdu_len);
    BT_CHECK(bt_l2cap_parse_header(&r, &header) == BT_OK);
    BT_CHECK(header.cid == BT_L2CAP_CID_ATT);
    BT_CHECK(bt_buf_reader_read_u8(&r, &opcode) == BT_OK);
    return opcode;
}

static void feed_att(struct bt_l2cap_channel_manager *manager,
                     const uint8_t *att, size_t att_len, uint64_t now_us)
{
    uint8_t l2cap[BT_L2CAP_HEADER_LEN + 128];
    uint8_t acl[BT_HCI_ACL_HEADER_LEN + sizeof(l2cap)];
    struct bt_buf_writer lw;
    struct bt_buf_writer aw;
    struct bt_buf_reader ar;
    struct bt_hci_acl_header header;

    bt_buf_writer_init(&lw, l2cap, sizeof(l2cap));
    bt_l2cap_encode_header(&lw, (uint16_t)att_len, BT_L2CAP_CID_ATT);
    bt_buf_writer_write_bytes(&lw, att, att_len);
    bt_buf_writer_init(&aw, acl, sizeof(acl));
    bt_hci_encode_acl_header(&aw, manager->handle, 0, 0,
                              (uint16_t)bt_buf_writer_len(&lw));
    bt_buf_writer_write_bytes(&aw, l2cap, bt_buf_writer_len(&lw));
    bt_buf_reader_init(&ar, acl, bt_buf_writer_len(&aw));
    bt_hci_parse_acl_header(&ar, &header);
    bt_l2cap_channel_manager_on_acl(manager, header.pb_flag,
                                     bt_buf_reader_peek(&ar, header.data_len),
                                     header.data_len, now_us);
}

struct test_log
{
    int connected;
    int completed;
    int write_completed;
    enum bt_hogp_client_result result;
    int values;
    struct bt_hid_value value;
    int events;
    struct bt_hid_input_event event;
};

static void connected(bool success, void *user_data)
{
    struct test_log *log = user_data;

    log->connected = success ? 1 : -1;
}

static void completed(enum bt_hogp_client_result result, void *user_data)
{
    struct test_log *log = user_data;

    ++log->completed;
    log->result = result;
}

static void write_completed(enum bt_hogp_client_result result, void *user_data)
{
    struct test_log *log = user_data;

    ++log->write_completed;
    log->result = result;
}

static bool input_value(const struct bt_hid_value *value, void *user_data)
{
    struct test_log *log = user_data;

    ++log->values;
    log->value = *value;
    return true;
}

static bool input_event(const struct bt_hid_input_event *event, void *user_data)
{
    struct test_log *log = user_data;

    ++log->events;
    log->event = *event;
    return true;
}

static void send_error_not_found(struct bt_l2cap_channel_manager *manager,
                                 uint8_t request_opcode, uint16_t handle,
                                 uint64_t now_us)
{
    uint8_t response[5] = {
        BT_ATT_OPCODE_ERROR_RESPONSE, request_opcode,
        (uint8_t)handle, (uint8_t)(handle >> 8),
        BT_ATT_ERROR_ATTRIBUTE_NOT_FOUND};

    feed_att(manager, response, sizeof(response), now_us);
}

static void test_hogp_discovery_and_input_notification(void)
{
    static const uint8_t report_map[] = {
        0x05, 0x0C, 0x09, 0x01, 0xA1, 0x01,
        0x85, 0x01, 0x09, 0xE9, 0x15, 0x00,
        0x25, 0x01, 0x75, 0x01, 0x95, 0x01,
        0x81, 0x02, 0x75, 0x07, 0x95, 0x01,
        0x81, 0x01, 0xC0};
    struct hogp_transport transport;
    struct bt_l2cap_channel_manager manager;
    struct bt_gatt_client gatt;
    struct bt_hogp_client hogp;
    struct test_log log = {0};
    uint8_t response[64];
    struct bt_buf_writer w;

    transport_init(&transport);
    bt_l2cap_channel_manager_init(&manager, &transport.base, 0x0041,
                                   BT_L2CAP_CID_SIGNALING_LE, 200);
    bt_gatt_client_init(&gatt, &manager);
    BT_CHECK(bt_gatt_client_connect(&gatt, connected, &log, 0) == BT_OK);
    BT_CHECK(take_opcode(&transport) == BT_ATT_OPCODE_EXCHANGE_MTU_REQUEST);
    {
        const uint8_t mtu[] = {BT_ATT_OPCODE_EXCHANGE_MTU_RESPONSE, 100, 0};

        feed_att(&manager, mtu, sizeof(mtu), 1);
    }
    BT_CHECK(log.connected == 1);

    bt_hogp_client_init(&hogp, &gatt);
    bt_hogp_client_set_event_handler(&hogp, input_event, &log);
    BT_CHECK(bt_hogp_client_discover(&hogp, completed, input_value, &log, 2) == BT_OK);
    BT_CHECK(take_opcode(&transport) == BT_ATT_OPCODE_READ_BY_GROUP_TYPE_REQUEST);
    bt_buf_writer_init(&w, response, sizeof(response));
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_READ_BY_GROUP_TYPE_RESPONSE);
    bt_buf_writer_write_u8(&w, 6);
    bt_buf_writer_write_le16(&w, 1);
    bt_buf_writer_write_le16(&w, 20);
    bt_buf_writer_write_le16(&w, BT_HOGP_UUID_SERVICE);
    feed_att(&manager, response, bt_buf_writer_len(&w), 3);
    BT_CHECK(take_opcode(&transport) == BT_ATT_OPCODE_READ_BY_GROUP_TYPE_REQUEST);
    send_error_not_found(&manager, BT_ATT_OPCODE_READ_BY_GROUP_TYPE_REQUEST, 21, 4);

    BT_CHECK(take_opcode(&transport) == BT_ATT_OPCODE_READ_BY_TYPE_REQUEST);
    bt_buf_writer_init(&w, response, sizeof(response));
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_READ_BY_TYPE_RESPONSE);
    bt_buf_writer_write_u8(&w, 7);
    bt_buf_writer_write_le16(&w, 2);
    bt_buf_writer_write_u8(&w, 0x02);
    bt_buf_writer_write_le16(&w, 3);
    bt_buf_writer_write_le16(&w, BT_HOGP_UUID_REPORT_MAP);
    bt_buf_writer_write_le16(&w, 4);
    bt_buf_writer_write_u8(&w, 0x10);
    bt_buf_writer_write_le16(&w, 5);
    bt_buf_writer_write_le16(&w, BT_HOGP_UUID_REPORT);
    bt_buf_writer_write_le16(&w, 8);
    bt_buf_writer_write_u8(&w, 0x08);
    bt_buf_writer_write_le16(&w, 9);
    bt_buf_writer_write_le16(&w, BT_HOGP_UUID_REPORT);
    bt_buf_writer_write_le16(&w, 11);
    bt_buf_writer_write_u8(&w, 0x08);
    bt_buf_writer_write_le16(&w, 12);
    bt_buf_writer_write_le16(&w, BT_HOGP_UUID_PROTOCOL_MODE);
    bt_buf_writer_write_le16(&w, 13);
    bt_buf_writer_write_u8(&w, 0x10);
    bt_buf_writer_write_le16(&w, 14);
    bt_buf_writer_write_le16(&w, BT_HOGP_UUID_BOOT_KEYBOARD_INPUT);
    bt_buf_writer_write_le16(&w, 16);
    bt_buf_writer_write_u8(&w, 0x08);
    bt_buf_writer_write_le16(&w, 17);
    bt_buf_writer_write_le16(&w, BT_HOGP_UUID_BOOT_KEYBOARD_OUTPUT);
    feed_att(&manager, response, bt_buf_writer_len(&w), 5);
    BT_CHECK(take_opcode(&transport) == BT_ATT_OPCODE_READ_BY_TYPE_REQUEST);
    send_error_not_found(&manager, BT_ATT_OPCODE_READ_BY_TYPE_REQUEST, 5, 6);

    BT_CHECK(take_opcode(&transport) == BT_ATT_OPCODE_FIND_INFORMATION_REQUEST);
    bt_buf_writer_init(&w, response, sizeof(response));
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_FIND_INFORMATION_RESPONSE);
    bt_buf_writer_write_u8(&w, 1);
    bt_buf_writer_write_le16(&w, 6);
    bt_buf_writer_write_le16(&w, BT_HOGP_UUID_CCCD);
    bt_buf_writer_write_le16(&w, 7);
    bt_buf_writer_write_le16(&w, BT_HOGP_UUID_REPORT_REFERENCE);
    bt_buf_writer_write_le16(&w, 10);
    bt_buf_writer_write_le16(&w, BT_HOGP_UUID_REPORT_REFERENCE);
    bt_buf_writer_write_le16(&w, 15);
    bt_buf_writer_write_le16(&w, BT_HOGP_UUID_CCCD);
    feed_att(&manager, response, bt_buf_writer_len(&w), 7);
    BT_CHECK(take_opcode(&transport) == BT_ATT_OPCODE_FIND_INFORMATION_REQUEST);
    send_error_not_found(&manager, BT_ATT_OPCODE_FIND_INFORMATION_REQUEST, 8, 8);

    BT_CHECK(take_opcode(&transport) == BT_ATT_OPCODE_READ_REQUEST);
    bt_buf_writer_init(&w, response, sizeof(response));
    bt_buf_writer_write_u8(&w, BT_ATT_OPCODE_READ_RESPONSE);
    bt_buf_writer_write_bytes(&w, report_map, sizeof(report_map));
    feed_att(&manager, response, bt_buf_writer_len(&w), 9);

    BT_CHECK(take_opcode(&transport) == BT_ATT_OPCODE_READ_REQUEST);
    {
        const uint8_t reference[] = {BT_ATT_OPCODE_READ_RESPONSE, 1,
                                     BT_HOGP_REPORT_TYPE_INPUT};

        feed_att(&manager, reference, sizeof(reference), 10);
    }
    BT_CHECK(take_opcode(&transport) == BT_ATT_OPCODE_READ_REQUEST);
    {
        const uint8_t reference[] = {BT_ATT_OPCODE_READ_RESPONSE, 2,
                                     BT_HOGP_REPORT_TYPE_OUTPUT};

        feed_att(&manager, reference, sizeof(reference), 10);
    }
    BT_CHECK(take_opcode(&transport) == BT_ATT_OPCODE_WRITE_REQUEST);
    {
        const uint8_t written[] = {BT_ATT_OPCODE_WRITE_RESPONSE};

        feed_att(&manager, written, sizeof(written), 11);
    }
    BT_CHECK(log.completed == 1 && log.result == BT_HOGP_CLIENT_OK);
    BT_CHECK(hogp.reports[0].report_id == 1);
    BT_CHECK(hogp.reports[0].cccd_handle == 6);

    {
        const uint8_t notification[] = {
            BT_ATT_OPCODE_HANDLE_VALUE_NOTIFICATION, 5, 0, 1};

        feed_att(&manager, notification, sizeof(notification), 12);
    }
    BT_CHECK(log.values == 1);
    BT_CHECK(log.value.report_id == 1);
    BT_CHECK(log.value.usage_page == 0x0C && log.value.usage == 0xE9);
    BT_CHECK(log.value.value == 1);
    BT_CHECK(log.events == 1);
    BT_CHECK(log.event.kind == BT_HID_INPUT_EVENT_CONSUMER);
    BT_CHECK(log.event.usage == 0xE9 && log.event.value == 1);

    {
        const uint8_t leds = 0x03;

        BT_CHECK(bt_hogp_client_write_report(
                     &hogp, 2, BT_HOGP_REPORT_TYPE_OUTPUT, &leds, 1,
                     write_completed, &log, 13) == BT_OK);
    }
    BT_CHECK(take_opcode(&transport) == BT_ATT_OPCODE_WRITE_REQUEST);
    {
        const uint8_t written[] = {BT_ATT_OPCODE_WRITE_RESPONSE};

        feed_att(&manager, written, sizeof(written), 14);
    }
    BT_CHECK(log.write_completed == 1 && log.result == BT_HOGP_CLIENT_OK);

    BT_CHECK(bt_hogp_client_set_protocol_mode(
                 &hogp, BT_HOGP_PROTOCOL_MODE_BOOT, write_completed, &log, 15) == BT_OK);
    BT_CHECK(take_opcode(&transport) == BT_ATT_OPCODE_WRITE_REQUEST);
    {
        const uint8_t written[] = {BT_ATT_OPCODE_WRITE_RESPONSE};

        feed_att(&manager, written, sizeof(written), 16);
    }
    BT_CHECK(take_opcode(&transport) == BT_ATT_OPCODE_WRITE_REQUEST);
    {
        const uint8_t written[] = {BT_ATT_OPCODE_WRITE_RESPONSE};

        feed_att(&manager, written, sizeof(written), 17);
    }
    BT_CHECK(log.write_completed == 2 && log.result == BT_HOGP_CLIENT_OK);

    {
        const uint8_t notification[] = {
            BT_ATT_OPCODE_HANDLE_VALUE_NOTIFICATION, 14, 0,
            0x02, 0x00, 0x04, 0, 0, 0, 0, 0};

        feed_att(&manager, notification, sizeof(notification), 18);
    }
    BT_CHECK(log.events == 3);
    BT_CHECK(log.event.kind == BT_HID_INPUT_EVENT_KEY);
    BT_CHECK(log.event.usage == 4 && log.event.value == 1);

    BT_CHECK(bt_hogp_client_write_boot_keyboard_output(
                 &hogp, 0x03, write_completed, &log, 19) == BT_OK);
    BT_CHECK(take_opcode(&transport) == BT_ATT_OPCODE_WRITE_REQUEST);
    {
        const uint8_t written[] = {BT_ATT_OPCODE_WRITE_RESPONSE};

        feed_att(&manager, written, sizeof(written), 20);
    }
    BT_CHECK(log.write_completed == 3 && log.result == BT_HOGP_CLIENT_OK);
}

void run_hogp_client_tests(void)
{
    test_hogp_discovery_and_input_notification();
}
