#include "test_att.h"
#include "../support/test.h"

#include <btcore/att.h>

#include <string.h>

static void test_error_response(void)
{
    static const uint8_t params[] = {BT_ATT_OPCODE_READ_REQUEST, 0x03, 0x00, 0x0A};
    struct bt_att_error_response err;

    BT_CHECK(bt_att_parse_error_response(params, sizeof(params), &err) == BT_OK);
    BT_CHECK(err.request_opcode == BT_ATT_OPCODE_READ_REQUEST);
    BT_CHECK(err.handle_in_error == 0x0003);
    BT_CHECK(err.error_code == 0x0A); /* Attribute Not Found */

    BT_CHECK(bt_att_parse_error_response(params, sizeof(params) - 1, &err) ==
              BT_ERR_INVALID_ARGUMENT);
}

static void test_exchange_mtu(void)
{
    uint8_t buf[3];
    struct bt_buf_writer w;
    uint16_t server_mtu;
    static const uint8_t rsp[] = {0xF0, 0x00}; /* 240, little-endian (ATT is LE, unlike SDP) */

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_att_encode_exchange_mtu_request(&w, 185) == BT_OK);
    BT_CHECK(buf[0] == BT_ATT_OPCODE_EXCHANGE_MTU_REQUEST);
    BT_CHECK(buf[1] == 0xB9 && buf[2] == 0x00); /* 185 = 0x00B9, little-endian */

    BT_CHECK(bt_att_parse_exchange_mtu_response(rsp, sizeof(rsp), &server_mtu) == BT_OK);
    BT_CHECK(server_mtu == 240);
}

static void test_read_by_group_type_service_discovery(void)
{
    uint8_t buf[8];
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_att_encode_read_by_group_type_request(&w, 0x0001, 0xFFFF,
                                                        BT_GATT_UUID_PRIMARY_SERVICE) == BT_OK);
    BT_CHECK(buf[0] == BT_ATT_OPCODE_READ_BY_GROUP_TYPE_REQUEST);
    BT_CHECK(buf[1] == 0x01 && buf[2] == 0x00); /* starting handle LE */
    BT_CHECK(buf[3] == 0xFF && buf[4] == 0xFF); /* ending handle LE */
    BT_CHECK(buf[5] == 0x00 && buf[6] == 0x28); /* 0x2800 LE */

    /* Two services, each with a 16-bit service UUID value (entry_len =
     * 4 + 2 = 6): [0x0001-0x0005]=UUID 0x1800, [0x0006-0x0009]=UUID 0x1801. */
    static const uint8_t params[] = {
        6,                          /* entry length */
        0x01, 0x00, 0x05, 0x00, 0x00, 0x18, /* handle, end_group, uuid16 0x1800 */
        0x06, 0x00, 0x09, 0x00, 0x01, 0x18, /* handle, end_group, uuid16 0x1801 */
    };
    struct bt_att_group_type_iter it;
    struct bt_att_group_entry entry;

    BT_CHECK(bt_att_read_by_group_type_response_iter_init(&it, params, sizeof(params)) == BT_OK);

    BT_CHECK(bt_att_read_by_group_type_response_iter_next(&it, &entry) == BT_OK);
    BT_CHECK(entry.handle == 0x0001 && entry.end_group_handle == 0x0005);
    BT_CHECK(entry.value_len == 2 && entry.value[0] == 0x00 && entry.value[1] == 0x18);

    BT_CHECK(bt_att_read_by_group_type_response_iter_next(&it, &entry) == BT_OK);
    BT_CHECK(entry.handle == 0x0006 && entry.end_group_handle == 0x0009);
    BT_CHECK(entry.value[0] == 0x01 && entry.value[1] == 0x18);

    BT_CHECK(bt_att_read_by_group_type_response_iter_next(&it, &entry) == BT_ERR_BUFFER_UNDERFLOW);
}

static void test_find_information(void)
{
    uint8_t request[5];
    const uint8_t response[] = {0x01, 0x03, 0x00, 0x02, 0x29,
                               0x04, 0x00, 0x08, 0x29};
    struct bt_buf_writer w;
    struct bt_att_find_information_iter it;
    struct bt_att_information_entry entry;

    bt_buf_writer_init(&w, request, sizeof(request));
    BT_CHECK(bt_att_encode_find_information_request(&w, 3, 9) == BT_OK);
    BT_CHECK(request[0] == BT_ATT_OPCODE_FIND_INFORMATION_REQUEST);
    BT_CHECK(request[1] == 3 && request[2] == 0);
    BT_CHECK(request[3] == 9 && request[4] == 0);
    BT_CHECK(bt_att_find_information_response_iter_init(&it, response,
                                                        sizeof(response)) == BT_OK);
    BT_CHECK(bt_att_find_information_response_iter_next(&it, &entry) == BT_OK);
    BT_CHECK(entry.handle == 3 && entry.uuid_len == 2);
    BT_CHECK(entry.uuid[0] == 0x02 && entry.uuid[1] == 0x29);
    BT_CHECK(bt_att_find_information_response_iter_next(&it, &entry) == BT_OK);
    BT_CHECK(entry.handle == 4 && entry.uuid[0] == 0x08 && entry.uuid[1] == 0x29);
    BT_CHECK(bt_att_find_information_response_iter_next(&it, &entry) ==
              BT_ERR_BUFFER_UNDERFLOW);
}

static void test_read_by_type_characteristic_discovery(void)
{
    uint8_t buf[8];
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_att_encode_read_by_type_request(&w, 0x0001, 0x0005,
                                                  BT_GATT_UUID_CHARACTERISTIC) == BT_OK);
    BT_CHECK(buf[0] == BT_ATT_OPCODE_READ_BY_TYPE_REQUEST);

    /* One characteristic declaration: Properties(1) + Value Handle(2) +
     * UUID16(2) = 5 bytes value, entry_len = 2 + 5 = 7. */
    static const uint8_t params[] = {
        7,
        0x02, 0x00,                   /* handle of the declaration itself */
        0x0A, 0x03, 0x00, 0x00, 0x2A, /* properties=0x0A, value handle=0x0003, uuid=0x2A00 */
    };
    struct bt_att_read_by_type_iter it;
    struct bt_att_type_entry entry;

    BT_CHECK(bt_att_read_by_type_response_iter_init(&it, params, sizeof(params)) == BT_OK);
    BT_CHECK(bt_att_read_by_type_response_iter_next(&it, &entry) == BT_OK);
    BT_CHECK(entry.handle == 0x0002);
    BT_CHECK(entry.value_len == 5);
    BT_CHECK(entry.value[0] == 0x0A);
    BT_CHECK(bt_att_read_by_type_response_iter_next(&it, &entry) == BT_ERR_BUFFER_UNDERFLOW);
}

static void test_read_and_write_requests(void)
{
    uint8_t buf[16];
    struct bt_buf_writer w;
    static const uint8_t value[] = {0x01, 0x02, 0x03};

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_att_encode_read_request(&w, 0x0003) == BT_OK);
    BT_CHECK(bt_buf_writer_len(&w) == 3);
    BT_CHECK(buf[0] == BT_ATT_OPCODE_READ_REQUEST && buf[1] == 0x03 && buf[2] == 0x00);

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_att_encode_read_blob_request(&w, 0x1234, 0x5678) == BT_OK);
    BT_CHECK(bt_buf_writer_len(&w) == 5);
    BT_CHECK(buf[0] == BT_ATT_OPCODE_READ_BLOB_REQUEST);
    BT_CHECK(buf[1] == 0x34 && buf[2] == 0x12);
    BT_CHECK(buf[3] == 0x78 && buf[4] == 0x56);

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_att_encode_write_request(&w, 0x0003, value, sizeof(value)) == BT_OK);
    BT_CHECK(bt_buf_writer_len(&w) == 3 + sizeof(value));
    BT_CHECK(buf[0] == BT_ATT_OPCODE_WRITE_REQUEST);
    BT_CHECK(memcmp(buf + 3, value, sizeof(value)) == 0);
}

static void test_handle_value_notification(void)
{
    static const uint8_t params[] = {0x05, 0x00, 0xDE, 0xAD, 0xBE, 0xEF};
    struct bt_att_handle_value hv;

    BT_CHECK(bt_att_parse_handle_value(params, sizeof(params), &hv) == BT_OK);
    BT_CHECK(hv.handle == 0x0005);
    BT_CHECK(hv.value_len == 4);
    BT_CHECK(memcmp(hv.value, "\xDE\xAD\xBE\xEF", 4) == 0);

    BT_CHECK(bt_att_parse_handle_value(params, 1, &hv) == BT_ERR_INVALID_ARGUMENT);

    uint8_t buf[1];
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_att_encode_handle_value_confirmation(&w) == BT_OK);
    BT_CHECK(buf[0] == BT_ATT_OPCODE_HANDLE_VALUE_CONFIRMATION);
}

static void test_truncated_group_type_entry_rejected(void)
{
    /* entry_len says 6 but only one full entry plus 3 stray bytes follow. */
    static const uint8_t params[] = {
        6, 0x01, 0x00, 0x05, 0x00, 0x00, 0x18, /* one full entry */
        0x06, 0x00, 0x09,                      /* truncated second entry */
    };
    struct bt_att_group_type_iter it;
    struct bt_att_group_entry entry;

    BT_CHECK(bt_att_read_by_group_type_response_iter_init(&it, params, sizeof(params)) == BT_OK);
    BT_CHECK(bt_att_read_by_group_type_response_iter_next(&it, &entry) == BT_OK);
    BT_CHECK(bt_att_read_by_group_type_response_iter_next(&it, &entry) ==
              BT_ERR_BUFFER_UNDERFLOW);
}

void run_att_tests(void)
{
    test_error_response();
    test_exchange_mtu();
    test_read_by_group_type_service_discovery();
    test_find_information();
    test_read_by_type_characteristic_discovery();
    test_read_and_write_requests();
    test_handle_value_notification();
    test_truncated_group_type_entry_rejected();
}
