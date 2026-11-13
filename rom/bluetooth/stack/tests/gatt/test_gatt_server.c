#include "test_gatt_server.h"
#include "../support/test.h"

#include <btcore/gatt_server.h>

#include <string.h>

/*
 * A small database with a gap in the handles, as a server whose services
 * come and go would have:
 *   0x0001 service 0x1800
 *   0x0002   characteristic 0x2A00 (read), value at 0x0003 "AROS"
 *   0x0010 service with a 128-bit UUID
 *   0x0011   characteristic 0xFFF1 (read, write, notify), value at 0x0012
 *   0x0013   its client characteristic configuration
 *   0x0014   characteristic 0xFFF2 (write without response), value at 0x0015
 */
static const uint8_t uuid128[16] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                                    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
static uint8_t value_fff1[8] = {1, 2, 3};
static uint16_t value_fff1_len = 3;
static uint8_t value_fff2[8];
static uint16_t value_fff2_len;
static uint8_t cccd[2];
static int writes;

static void set_type16(struct bt_gatt_server_attr *a, uint16_t uuid)
{
    a->type_len = 2;
    a->type[0] = (uint8_t)uuid;
    a->type[1] = (uint8_t)(uuid >> 8);
}

static void char_decl(struct bt_gatt_server_attr *a, uint8_t props, uint16_t vh, uint16_t uuid)
{
    set_type16(a, 0x2803);
    a->flags = BT_GATT_ATTR_READ;
    a->scratch[0] = props;
    a->scratch[1] = (uint8_t)vh;
    a->scratch[2] = (uint8_t)(vh >> 8);
    a->scratch[3] = (uint8_t)uuid;
    a->scratch[4] = (uint8_t)(uuid >> 8);
    a->value = a->scratch;
    a->value_len = 5;
}

static bool db_next(void *ctx, uint16_t from, struct bt_gatt_server_attr *a)
{
    static const uint16_t handles[] = {0x0001, 0x0002, 0x0003, 0x0010, 0x0011,
                                       0x0012, 0x0013, 0x0014, 0x0015};
    size_t i;

    (void)ctx;
    for (i = 0; i < sizeof(handles) / sizeof(handles[0]); i++)
        if (handles[i] >= from)
            break;
    if (i == sizeof(handles) / sizeof(handles[0]))
        return false;
    a->handle = handles[i];
    a->ref_index = (uint16_t)i;
    switch (a->handle)
    {
    case 0x0001:
        set_type16(a, 0x2800);
        a->flags = BT_GATT_ATTR_READ | BT_GATT_ATTR_SERVICE;
        a->group_end = 0x0003;
        a->scratch[0] = 0x00;
        a->scratch[1] = 0x18;
        a->value = a->scratch;
        a->value_len = 2;
        break;
    case 0x0002:
        char_decl(a, 0x02, 0x0003, 0x2a00);
        break;
    case 0x0003:
        set_type16(a, 0x2a00);
        a->flags = BT_GATT_ATTR_READ;
        a->value = (const uint8_t *)"AROS";
        a->value_len = 4;
        break;
    case 0x0010:
        set_type16(a, 0x2800);
        a->flags = BT_GATT_ATTR_READ | BT_GATT_ATTR_SERVICE;
        a->group_end = 0x0015;
        a->value = uuid128;
        a->value_len = 16;
        break;
    case 0x0011:
        char_decl(a, 0x1a, 0x0012, 0xfff1);
        break;
    case 0x0012:
        set_type16(a, 0xfff1);
        a->flags = BT_GATT_ATTR_READ | BT_GATT_ATTR_WRITE;
        a->value = value_fff1;
        a->value_len = value_fff1_len;
        break;
    case 0x0013:
        set_type16(a, 0x2902);
        a->flags = BT_GATT_ATTR_READ | BT_GATT_ATTR_WRITE;
        a->value = cccd;
        a->value_len = 2;
        break;
    case 0x0014:
        char_decl(a, 0x04, 0x0015, 0xfff2);
        break;
    case 0x0015:
        set_type16(a, 0xfff2);
        a->flags = BT_GATT_ATTR_WRITE_NR;
        a->value = value_fff2;
        a->value_len = value_fff2_len;
        break;
    }
    return true;
}

static uint8_t db_write(void *ctx, const struct bt_gatt_server_attr *a, const uint8_t *data,
                        size_t len)
{
    (void)ctx;
    writes++;
    switch (a->handle)
    {
    case 0x0012:
        if (len > sizeof(value_fff1))
            return 0x0d;
        memcpy(value_fff1, data, len);
        value_fff1_len = (uint16_t)len;
        return 0;
    case 0x0013:
        if (len != 2)
            return 0x0d;
        memcpy(cccd, data, 2);
        return 0;
    case 0x0015:
        if (len > sizeof(value_fff2))
            return 0x0d;
        memcpy(value_fff2, data, len);
        value_fff2_len = (uint16_t)len;
        return 0;
    }
    return 0x03;
}

static struct bt_gatt_server srv;
static uint8_t rsp[BT_GATT_SERVER_RX_MTU];

static size_t ask(const uint8_t *pdu, size_t len)
{
    memset(rsp, 0xee, sizeof(rsp));
    return bt_gatt_server_handle_pdu(&srv, pdu, len, rsp, sizeof(rsp));
}

static bool is_error(size_t n, uint8_t opcode, uint16_t handle, uint8_t code)
{
    return n == 5 && rsp[0] == 0x01 && rsp[1] == opcode && rsp[2] == (uint8_t)handle &&
           rsp[3] == (uint8_t)(handle >> 8) && rsp[4] == code;
}

static void test_discovery(void)
{
    static const uint8_t all_services[] = {0x10, 0x01, 0x00, 0xff, 0xff, 0x00, 0x28};
    static const uint8_t more_services[] = {0x10, 0x04, 0x00, 0xff, 0xff, 0x00, 0x28};
    static const uint8_t past_services[] = {0x10, 0x16, 0x00, 0xff, 0xff, 0x00, 0x28};
    static const uint8_t secondary[] = {0x10, 0x01, 0x00, 0xff, 0xff, 0x01, 0x28};
    static const uint8_t by_uuid16[] = {0x06, 0x01, 0x00, 0xff, 0xff, 0x00, 0x28, 0x00, 0x18};
    static const uint8_t chars[] = {0x08, 0x10, 0x00, 0x15, 0x00, 0x03, 0x28};
    static const uint8_t info[] = {0x04, 0x12, 0x00, 0x13, 0x00};
    static const uint8_t bad_range[] = {0x04, 0x05, 0x00, 0x04, 0x00};
    uint8_t by_uuid128[23] = {0x06, 0x01, 0x00, 0xff, 0xff, 0x00, 0x28};
    size_t n;

    /* services with a 16-bit UUID first: entries of one length per response */
    n = ask(all_services, sizeof(all_services));
    BT_CHECK(n == 8 && rsp[0] == 0x11 && rsp[1] == 6);
    BT_CHECK(rsp[2] == 0x01 && rsp[4] == 0x03 && rsp[6] == 0x00 && rsp[7] == 0x18);
    n = ask(more_services, sizeof(more_services));
    BT_CHECK(n == 22 && rsp[0] == 0x11 && rsp[1] == 20);
    BT_CHECK(rsp[2] == 0x10 && rsp[4] == 0x15 && memcmp(rsp + 6, uuid128, 16) == 0);
    n = ask(past_services, sizeof(past_services));
    BT_CHECK(is_error(n, 0x10, 0x0016, 0x0a));
    n = ask(secondary, sizeof(secondary));
    BT_CHECK(is_error(n, 0x10, 0x0001, 0x10));

    n = ask(by_uuid16, sizeof(by_uuid16));
    BT_CHECK(n == 5 && rsp[0] == 0x07 && rsp[1] == 0x01 && rsp[3] == 0x03);
    memcpy(by_uuid128 + 7, uuid128, 16);
    n = ask(by_uuid128, sizeof(by_uuid128));
    BT_CHECK(n == 5 && rsp[0] == 0x07 && rsp[1] == 0x10 && rsp[3] == 0x15);

    /* both characteristic declarations of the second service */
    n = ask(chars, sizeof(chars));
    BT_CHECK(n == 16 && rsp[0] == 0x09 && rsp[1] == 7);
    BT_CHECK(rsp[2] == 0x11 && rsp[4] == 0x1a && rsp[5] == 0x12 && rsp[7] == 0xf1 && rsp[8] == 0xff);
    BT_CHECK(rsp[9] == 0x14 && rsp[11] == 0x04 && rsp[12] == 0x15);

    n = ask(info, sizeof(info));
    BT_CHECK(n == 10 && rsp[0] == 0x05 && rsp[1] == 0x01);
    BT_CHECK(rsp[2] == 0x12 && rsp[4] == 0xf1 && rsp[6] == 0x13 && rsp[8] == 0x02 && rsp[9] == 0x29);
    n = ask(bad_range, sizeof(bad_range));
    BT_CHECK(is_error(n, 0x04, 0x0005, 0x01));
}

static void test_read_write(void)
{
    static const uint8_t read_name[] = {0x0a, 0x03, 0x00};
    static const uint8_t read_blob[] = {0x0c, 0x03, 0x00, 0x02, 0x00};
    static const uint8_t read_blob_end[] = {0x0c, 0x03, 0x00, 0x04, 0x00};
    static const uint8_t read_blob_past[] = {0x0c, 0x03, 0x00, 0x05, 0x00};
    static const uint8_t read_gap[] = {0x0a, 0x08, 0x00};
    static const uint8_t read_wo[] = {0x0a, 0x15, 0x00};
    static const uint8_t read_by_uuid[] = {0x08, 0x01, 0x00, 0xff, 0xff, 0x00, 0x2a};
    static const uint8_t write_ro[] = {0x12, 0x03, 0x00, 0x41};
    static const uint8_t write_val[] = {0x12, 0x12, 0x00, 9, 8, 7, 6};
    static const uint8_t write_long[] = {0x12, 0x12, 0x00, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    static const uint8_t write_cccd[] = {0x12, 0x13, 0x00, 0x01, 0x00};
    static const uint8_t write_cmd[] = {0x52, 0x15, 0x00, 0x55};
    static const uint8_t write_cmd_ro[] = {0x52, 0x03, 0x00, 0x55};
    static const uint8_t read_val[] = {0x0a, 0x12, 0x00};
    size_t n;

    n = ask(read_name, sizeof(read_name));
    BT_CHECK(n == 5 && rsp[0] == 0x0b && memcmp(rsp + 1, "AROS", 4) == 0);
    n = ask(read_blob, sizeof(read_blob));
    BT_CHECK(n == 3 && rsp[0] == 0x0d && rsp[1] == 'O' && rsp[2] == 'S');
    n = ask(read_blob_end, sizeof(read_blob_end));
    BT_CHECK(n == 1 && rsp[0] == 0x0d);
    n = ask(read_blob_past, sizeof(read_blob_past));
    BT_CHECK(is_error(n, 0x0c, 0x0003, 0x07));
    n = ask(read_gap, sizeof(read_gap));
    BT_CHECK(is_error(n, 0x0a, 0x0008, 0x01));
    n = ask(read_wo, sizeof(read_wo));
    BT_CHECK(is_error(n, 0x0a, 0x0015, 0x02));
    n = ask(read_by_uuid, sizeof(read_by_uuid));
    BT_CHECK(n == 8 && rsp[0] == 0x09 && rsp[1] == 6 && rsp[2] == 0x03 && rsp[4] == 'A');

    writes = 0;
    n = ask(write_ro, sizeof(write_ro));
    BT_CHECK(is_error(n, 0x12, 0x0003, 0x03) && writes == 0);
    n = ask(write_val, sizeof(write_val));
    BT_CHECK(n == 1 && rsp[0] == 0x13 && writes == 1 && value_fff1_len == 4 && value_fff1[0] == 9);
    n = ask(read_val, sizeof(read_val));
    BT_CHECK(n == 5 && rsp[0] == 0x0b && rsp[4] == 6);
    n = ask(write_long, sizeof(write_long));
    BT_CHECK(is_error(n, 0x12, 0x0012, 0x0d) && value_fff1_len == 4);
    n = ask(write_cccd, sizeof(write_cccd));
    BT_CHECK(n == 1 && rsp[0] == 0x13 && cccd[0] == 0x01);

    /* commands never get an answer, not even a refusal */
    writes = 0;
    n = ask(write_cmd, sizeof(write_cmd));
    BT_CHECK(n == 0 && writes == 1 && value_fff2_len == 1 && value_fff2[0] == 0x55);
    n = ask(write_cmd_ro, sizeof(write_cmd_ro));
    BT_CHECK(n == 0 && writes == 1);
}

static void test_mtu_and_misc(void)
{
    static const uint8_t mtu_small[] = {0x02, 0x10, 0x00};
    static const uint8_t mtu_big[] = {0x02, 0x00, 0x02};
    static const uint8_t read_multiple[] = {0x0e, 0x03, 0x00, 0x12, 0x00};
    static const uint8_t prepare[] = {0x16, 0x12, 0x00, 0x00, 0x00, 0x01};
    static const uint8_t signed_write[] = {0xd2, 0x12, 0x00, 0x01};
    static const uint8_t confirmation[] = {0x1e};
    uint8_t out[32];
    uint8_t big[40];
    size_t n;

    n = ask(mtu_small, sizeof(mtu_small));
    BT_CHECK(n == 3 && rsp[0] == 0x03 && rsp[1] == BT_GATT_SERVER_RX_MTU && srv.mtu == 23);
    n = ask(read_multiple, sizeof(read_multiple));
    BT_CHECK(is_error(n, 0x0e, 0x0000, 0x06));
    n = ask(prepare, sizeof(prepare));
    BT_CHECK(is_error(n, 0x16, 0x0000, 0x06));
    n = ask(signed_write, sizeof(signed_write));
    BT_CHECK(n == 0);

    /* a notification is cut to the MTU, an indication waits for its confirmation */
    memset(big, 0x5a, sizeof(big));
    n = bt_gatt_server_encode_handle_value(&srv, 0x0012, big, sizeof(big), false, out, sizeof(out));
    BT_CHECK(n == 23 && out[0] == 0x1b && out[1] == 0x12 && out[3] == 0x5a);
    n = bt_gatt_server_encode_handle_value(&srv, 0x0012, big, 2, true, out, sizeof(out));
    BT_CHECK(n == 5 && out[0] == 0x1d && srv.indication_pending);
    n = bt_gatt_server_encode_handle_value(&srv, 0x0012, big, 2, true, out, sizeof(out));
    BT_CHECK(n == 0);
    n = ask(confirmation, sizeof(confirmation));
    BT_CHECK(n == 0 && !srv.indication_pending);
    n = bt_gatt_server_encode_handle_value(&srv, 0x0012, big, 2, true, out, sizeof(out));
    BT_CHECK(n == 5);

    n = ask(mtu_big, sizeof(mtu_big));
    BT_CHECK(n == 3 && srv.mtu == BT_GATT_SERVER_RX_MTU);
}

void run_gatt_server_tests(void)
{
    bt_gatt_server_init(&srv, db_next, db_write, NULL);
    test_discovery();
    test_read_write();
    test_mtu_and_misc();
}
