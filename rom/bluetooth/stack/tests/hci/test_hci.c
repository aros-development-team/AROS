#include "test_hci.h"
#include "../support/test.h"

#include <btcore/hci.h>

static void test_opcode_packing(void)
{
    /* HCI_Reset is the well-known opcode 0x0C03 (OGF 0x03, OCF 0x0003). */
    BT_CHECK(BT_HCI_OPCODE_RESET == 0x0C03u);
}

static void test_encode_command(void)
{
    uint8_t buf[16];
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_hci_encode_command(&w, BT_HCI_OPCODE_RESET, NULL, 0) == BT_OK);

    /* Wire bytes for HCI Reset: opcode LE (0x03, 0x0C), param length 0. */
    BT_CHECK(bt_buf_writer_len(&w) == 3);
    BT_CHECK(buf[0] == 0x03 && buf[1] == 0x0C && buf[2] == 0x00);
}

static void test_encode_command_with_params(void)
{
    uint8_t buf[16];
    struct bt_buf_writer w;
    static const uint8_t params[] = {0xaa, 0xbb, 0xcc};

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_hci_encode_command(&w, 0x1234, params, sizeof(params)) == BT_OK);

    BT_CHECK(bt_buf_writer_len(&w) == 3 + sizeof(params));
    BT_CHECK(buf[0] == 0x34 && buf[1] == 0x12 && buf[2] == 0x03);
    BT_CHECK(buf[3] == 0xaa && buf[4] == 0xbb && buf[5] == 0xcc);
}

static void test_encode_command_too_long(void)
{
    uint8_t buf[16];
    uint8_t params[BT_HCI_MAX_PARAM_LEN + 1] = {0};
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_hci_encode_command(&w, 0x0001, params, sizeof(params)) == BT_ERR_INVALID_ARGUMENT);
}

static void test_parse_command_complete(void)
{
    /* Command Complete for a successful HCI Reset. */
    static const uint8_t wire[] = {0x0E, 0x04, 0x01, 0x03, 0x0C, 0x00};
    struct bt_buf_reader r;
    struct bt_hci_event_header hdr;
    struct bt_hci_command_complete cc;

    bt_buf_reader_init(&r, wire, sizeof(wire));

    BT_CHECK(bt_hci_parse_event_header(&r, &hdr) == BT_OK);
    BT_CHECK(hdr.event_code == BT_HCI_EVENT_COMMAND_COMPLETE);
    BT_CHECK(hdr.param_len == 4);

    BT_CHECK(bt_hci_parse_command_complete(&r, hdr.param_len, &cc) == BT_OK);
    BT_CHECK(cc.num_hci_command_packets == 1);
    BT_CHECK(cc.command_opcode == BT_HCI_OPCODE_RESET);
    BT_CHECK(cc.return_params_len == 1);
    BT_CHECK(cc.return_params[0] == 0x00);

    BT_CHECK(bt_buf_reader_remaining(&r) == 0);
}

static void test_parse_truncated_event(void)
{
    /* Header claims 4 parameter bytes but only 2 are present. */
    static const uint8_t wire[] = {0x0E, 0x04, 0x01, 0x03};
    struct bt_buf_reader r;
    struct bt_hci_event_header hdr;

    bt_buf_reader_init(&r, wire, sizeof(wire));
    BT_CHECK(bt_hci_parse_event_header(&r, &hdr) == BT_ERR_BUFFER_UNDERFLOW);
}

static void test_parse_command_complete_too_short(void)
{
    /* param_len smaller than the fixed 3-byte prefix must be rejected. */
    static const uint8_t wire[] = {0x0E, 0x02, 0x01, 0x03};
    struct bt_buf_reader r;
    struct bt_hci_event_header hdr;
    struct bt_hci_command_complete cc;

    bt_buf_reader_init(&r, wire, sizeof(wire));
    BT_CHECK(bt_hci_parse_event_header(&r, &hdr) == BT_OK);
    BT_CHECK(bt_hci_parse_command_complete(&r, hdr.param_len, &cc) == BT_ERR_BUFFER_UNDERFLOW);
}

static void test_parse_command_status(void)
{
    static const uint8_t wire[] = {0x0F, 0x04, 0x00, 0x01, 0x03, 0x0C};
    struct bt_buf_reader r;
    struct bt_hci_event_header hdr;
    struct bt_hci_command_status cs;

    bt_buf_reader_init(&r, wire, sizeof(wire));
    BT_CHECK(bt_hci_parse_event_header(&r, &hdr) == BT_OK);
    BT_CHECK(hdr.event_code == BT_HCI_EVENT_COMMAND_STATUS);

    BT_CHECK(bt_hci_parse_command_status(&r, hdr.param_len, &cs) == BT_OK);
    BT_CHECK(cs.status == 0x00);
    BT_CHECK(cs.num_hci_command_packets == 1);
    BT_CHECK(cs.command_opcode == BT_HCI_OPCODE_RESET);
}

static void test_parse_command_status_wrong_length(void)
{
    static const uint8_t wire[] = {0x0F, 0x03, 0x00, 0x01, 0x03};
    struct bt_buf_reader r;
    struct bt_hci_event_header hdr;
    struct bt_hci_command_status cs;

    bt_buf_reader_init(&r, wire, sizeof(wire));
    BT_CHECK(bt_hci_parse_event_header(&r, &hdr) == BT_OK);
    BT_CHECK(bt_hci_parse_command_status(&r, hdr.param_len, &cs) == BT_ERR_INVALID_ARGUMENT);
}

static void test_acl_header_round_trip(void)
{
    uint8_t buf[BT_HCI_ACL_HEADER_LEN];
    struct bt_buf_writer w;
    struct bt_buf_reader r;
    struct bt_hci_acl_header hdr;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_hci_encode_acl_header(&w, 0x0041, 0x02, 0x00, 23) == BT_OK);

    /* Handle 0x041 (12 bits) | PB=2 (bits 12-13) | BC=0 (bits 14-15) -> 0x2041. */
    BT_CHECK(buf[0] == 0x41 && buf[1] == 0x20);
    BT_CHECK(buf[2] == 23 && buf[3] == 0x00);

    bt_buf_reader_init(&r, buf, sizeof(buf));
    BT_CHECK(bt_hci_parse_acl_header(&r, &hdr) == BT_OK);
    BT_CHECK(hdr.handle == 0x0041);
    BT_CHECK(hdr.pb_flag == 0x02);
    BT_CHECK(hdr.bc_flag == 0x00);
    BT_CHECK(hdr.data_len == 23);
}

static void test_acl_header_rejects_out_of_range(void)
{
    uint8_t buf[BT_HCI_ACL_HEADER_LEN];
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_hci_encode_acl_header(&w, 0x1000, 0, 0, 0) == BT_ERR_INVALID_ARGUMENT);
    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_hci_encode_acl_header(&w, 0, 4, 0, 0) == BT_ERR_INVALID_ARGUMENT);
}

static void test_parse_local_version(void)
{
    /* status=0, hci_version=0x0b, hci_revision=0x1234, lmp_pal_version=0x0b,
     * manufacturer=0x000f, lmp_pal_subversion=0x5678 */
    static const uint8_t rp[] = {0x00, 0x0b, 0x34, 0x12, 0x0b, 0x0f, 0x00, 0x78, 0x56};
    struct bt_hci_local_version v;

    BT_CHECK(bt_hci_parse_local_version(rp, sizeof(rp), &v) == BT_OK);
    BT_CHECK(v.status == 0x00);
    BT_CHECK(v.hci_version == 0x0b);
    BT_CHECK(v.hci_revision == 0x1234);
    BT_CHECK(v.lmp_pal_version == 0x0b);
    BT_CHECK(v.manufacturer_name == 0x000f);
    BT_CHECK(v.lmp_pal_subversion == 0x5678);

    BT_CHECK(bt_hci_parse_local_version(rp, sizeof(rp) - 1, &v) == BT_ERR_INVALID_ARGUMENT);
}

static void test_parse_local_features(void)
{
    static const uint8_t rp[] = {0x00, 0xff, 0xfe, 0x8d, 0xfe, 0x9b, 0xf9, 0x00, 0x80};
    struct bt_hci_local_features f;

    BT_CHECK(bt_hci_parse_local_features(rp, sizeof(rp), &f) == BT_OK);
    BT_CHECK(f.status == 0x00);
    BT_CHECK(f.features[0] == 0xff && f.features[6] == 0x00 && f.features[7] == 0x80);

    BT_CHECK(bt_hci_parse_local_features(rp, sizeof(rp) - 1, &f) == BT_ERR_INVALID_ARGUMENT);
}

static void test_parse_buffer_size(void)
{
    /* status=0, acl_len=0x00fb (251), sco_len=0, total_acl=0x000a (10), total_sco=0 */
    static const uint8_t rp[] = {0x00, 0xfb, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00};
    struct bt_hci_buffer_size bs;

    BT_CHECK(bt_hci_parse_buffer_size(rp, sizeof(rp), &bs) == BT_OK);
    BT_CHECK(bs.status == 0x00);
    BT_CHECK(bs.acl_data_packet_length == 251);
    BT_CHECK(bs.sco_data_packet_length == 0);
    BT_CHECK(bs.total_num_acl_data_packets == 10);
    BT_CHECK(bs.total_num_sco_data_packets == 0);

    BT_CHECK(bt_hci_parse_buffer_size(rp, sizeof(rp) - 1, &bs) == BT_ERR_INVALID_ARGUMENT);
}

static void test_encode_inquiry(void)
{
    uint8_t buf[16];
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_hci_encode_inquiry(&w, BT_HCI_GIAC_LAP, 0x08, 0x00) == BT_OK);

    /* opcode LE (0x01,0x04 -> OGF 1, OCF 1), length 5, LAP LE (0x33,0x8B,0x9E),
     * inquiry_length, num_responses. */
    BT_CHECK(bt_buf_writer_len(&w) == 3 + 5);
    BT_CHECK(buf[0] == 0x01 && buf[1] == 0x04 && buf[2] == 0x05);
    BT_CHECK(buf[3] == 0x33 && buf[4] == 0x8B && buf[5] == 0x9E);
    BT_CHECK(buf[6] == 0x08 && buf[7] == 0x00);
}

static void test_inquiry_result_iter(void)
{
    /* num_responses=2, then each parameter as an array of two (column-major,
     * as the event is specified): BD_ADDR[2], Page_Scan_Repetition_Mode[2],
     * Reserved[2] (2 bytes each), Class_Of_Device[2], Clock_Offset[2]. */
    static const uint8_t wire[] = {
        0x02,
        /* BD_ADDR: AA:BB:CC:DD:EE:01, AA:BB:CC:DD:EE:02 */
        0x01, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x02, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA,
        /* Page_Scan_Repetition_Mode: 0x01, 0x00 */
        0x01, 0x00,
        /* Reserved */
        0x00, 0x00, 0x00, 0x00,
        /* Class_Of_Device LE: 0x123456, 0x000000 */
        0x56, 0x34, 0x12, 0x00, 0x00, 0x00,
        /* Clock_Offset LE: 0x1122, 0x0000 */
        0x22, 0x11, 0x00, 0x00,
    };
    struct bt_hci_inquiry_result_iter it;
    struct bt_hci_inquiry_result_entry entry;

    BT_CHECK(bt_hci_inquiry_result_iter_init(&it, wire, sizeof(wire)) == BT_OK);

    BT_CHECK(bt_hci_inquiry_result_iter_next(&it, &entry) == BT_OK);
    BT_CHECK(entry.bd_addr.b[0] == 0x01 && entry.bd_addr.b[5] == 0xAA);
    BT_CHECK(entry.page_scan_repetition_mode == 0x01);
    BT_CHECK(entry.class_of_device == 0x123456);
    BT_CHECK(entry.clock_offset == 0x1122);

    BT_CHECK(bt_hci_inquiry_result_iter_next(&it, &entry) == BT_OK);
    BT_CHECK(entry.bd_addr.b[0] == 0x02);
    BT_CHECK(entry.class_of_device == 0x000000);

    BT_CHECK(bt_hci_inquiry_result_iter_next(&it, &entry) == BT_ERR_BUFFER_UNDERFLOW);
}

static void test_encode_le_scan(void)
{
    uint8_t buf[16];
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_hci_encode_le_set_scan_parameters(&w, 0x01, 0x0010, 0x0010, 0x00, 0x00) == BT_OK);
    BT_CHECK(bt_buf_writer_len(&w) == 3 + 7);
    BT_CHECK(buf[0] == 0x0B && buf[1] == 0x20); /* opcode LE for OGF 0x08, OCF 0x000B */

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_hci_encode_le_set_scan_enable(&w, 0x01, 0x00) == BT_OK);
    BT_CHECK(bt_buf_writer_len(&w) == 3 + 2);
    BT_CHECK(buf[0] == 0x0C && buf[1] == 0x20);
    BT_CHECK(buf[3] == 0x01 && buf[4] == 0x00);
}

static void test_le_adv_report_iter(void)
{
    static const uint8_t wire[] = {
        BT_HCI_LE_META_SUBEVENT_ADVERTISING_REPORT,
        0x02, /* num_reports */
        /* report 1: event_type=0x00, addr_type=0x00, addr ..01, data_len=2, data={0xAA,0xBB}, rssi=-40 */
        0x00, 0x00, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x01, 0x02, 0xAA, 0xBB, (uint8_t)-40,
        /* report 2: event_type=0x04, addr_type=0x01, addr ..02, data_len=0, rssi=-70 */
        0x04, 0x01, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x02, 0x00, (uint8_t)-70,
    };
    struct bt_hci_le_adv_report_iter it;
    struct bt_hci_le_adv_report report;

    BT_CHECK(bt_hci_le_adv_report_iter_init(&it, wire, sizeof(wire)) == BT_OK);

    BT_CHECK(bt_hci_le_adv_report_iter_next(&it, &report) == BT_OK);
    BT_CHECK(report.event_type == 0x00);
    BT_CHECK(report.address_type == 0x00);
    BT_CHECK(report.address.b[0] == 0xEE && report.address.b[5] == 0x01);
    BT_CHECK(report.data_len == 2);
    BT_CHECK(report.data[0] == 0xAA && report.data[1] == 0xBB);
    BT_CHECK(report.rssi == -40);

    BT_CHECK(bt_hci_le_adv_report_iter_next(&it, &report) == BT_OK);
    BT_CHECK(report.event_type == 0x04);
    BT_CHECK(report.data_len == 0);
    BT_CHECK(report.rssi == -70);

    BT_CHECK(bt_hci_le_adv_report_iter_next(&it, &report) == BT_ERR_BUFFER_UNDERFLOW);
}

static void test_le_adv_report_iter_rejects_wrong_subevent(void)
{
    static const uint8_t wire[] = {0x01, 0x00}; /* not an advertising report */
    struct bt_hci_le_adv_report_iter it;

    BT_CHECK(bt_hci_le_adv_report_iter_init(&it, wire, sizeof(wire)) == BT_ERR_INVALID_ARGUMENT);
}

static void test_le_security_commands(void)
{
    static const uint8_t key[16] = {
        0xBF, 0x01, 0xFB, 0x9D, 0x4E, 0xF3, 0xBC, 0x36,
        0xD8, 0x74, 0xF5, 0x39, 0x41, 0x38, 0x68, 0x4C};
    static const uint8_t plaintext[16] = {
        0x13, 0x02, 0xF1, 0xE0, 0xDF, 0xCE, 0xBD, 0xAC,
        0x79, 0x68, 0x57, 0x46, 0x35, 0x24, 0x13, 0x02};
    uint8_t buf[80];
    uint8_t random[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    uint8_t x[32] = {0};
    uint8_t y[32] = {0};
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_hci_encode_le_encrypt(&w, key, plaintext) == BT_OK);
    BT_CHECK(bt_buf_writer_len(&w) == 35);
    BT_CHECK(buf[0] == 0x17 && buf[1] == 0x20 && buf[2] == 32);
    BT_CHECK(buf[3] == 0xBF && buf[18] == 0x4C && buf[19] == 0x13 && buf[34] == 0x02);

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_hci_encode_le_rand(&w) == BT_OK);
    BT_CHECK(bt_buf_writer_len(&w) == 3 && buf[0] == 0x18 && buf[1] == 0x20);

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_hci_encode_le_enable_encryption(&w, 0x0041, random, 0x1234, key) == BT_OK);
    BT_CHECK(bt_buf_writer_len(&w) == 31);
    BT_CHECK(buf[0] == 0x19 && buf[1] == 0x20 && buf[2] == 28);
    BT_CHECK(buf[3] == 0x41 && buf[4] == 0x00);
    BT_CHECK(buf[13] == 0x34 && buf[14] == 0x12);

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_hci_encode_le_read_local_p256_public_key(&w) == BT_OK);
    BT_CHECK(bt_buf_writer_len(&w) == 3 && buf[0] == 0x25 && buf[1] == 0x20);

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_hci_encode_le_generate_dhkey(&w, x, y) == BT_OK);
    BT_CHECK(bt_buf_writer_len(&w) == 67 && buf[0] == 0x26 && buf[1] == 0x20 && buf[2] == 64);
}

static void test_le_security_returns_and_p256_events(void)
{
    uint8_t encrypt_return[17] = {0};
    uint8_t rand_return[9] = {0};
    uint8_t public_event[66] = {0};
    uint8_t dhkey_event[34] = {0};
    uint8_t status;
    uint8_t value16[16];
    uint8_t value8[8];
    const uint8_t *dhkey;
    struct bt_hci_le_p256_public_key_complete public_key;

    encrypt_return[0] = 0;
    encrypt_return[16] = 0xAA;
    BT_CHECK(bt_hci_parse_le_encrypt_return(encrypt_return, sizeof(encrypt_return), &status,
                                             value16) == BT_OK);
    BT_CHECK(status == 0 && value16[15] == 0xAA);
    rand_return[0] = 0;
    rand_return[8] = 0xBB;
    BT_CHECK(bt_hci_parse_le_rand_return(rand_return, sizeof(rand_return), &status, value8) == BT_OK);
    BT_CHECK(value8[7] == 0xBB);

    public_event[0] = BT_HCI_LE_META_SUBEVENT_READ_LOCAL_P256_PUBLIC_KEY_COMPLETE;
    public_event[1] = 0;
    public_event[2] = 0x11;
    public_event[34] = 0x22;
    BT_CHECK(bt_hci_parse_le_p256_public_key_complete(public_event, sizeof(public_event),
                                                       &public_key) == BT_OK);
    BT_CHECK(public_key.status == 0 && public_key.x[0] == 0x11 && public_key.y[0] == 0x22);

    dhkey_event[0] = BT_HCI_LE_META_SUBEVENT_GENERATE_DHKEY_COMPLETE;
    dhkey_event[1] = 0;
    dhkey_event[2] = 0xCC;
    BT_CHECK(bt_hci_parse_le_generate_dhkey_complete(dhkey_event, sizeof(dhkey_event),
                                                      &status, &dhkey) == BT_OK);
    BT_CHECK(status == 0 && dhkey[0] == 0xCC);
    BT_CHECK(bt_hci_parse_le_generate_dhkey_complete(dhkey_event, sizeof(dhkey_event) - 1,
                                                      &status, &dhkey) ==
              BT_ERR_INVALID_ARGUMENT);
}

void run_hci_tests(void)
{
    test_opcode_packing();
    test_encode_command();
    test_encode_command_with_params();
    test_encode_command_too_long();
    test_parse_command_complete();
    test_parse_truncated_event();
    test_parse_command_complete_too_short();
    test_parse_command_status();
    test_parse_command_status_wrong_length();
    test_acl_header_round_trip();
    test_acl_header_rejects_out_of_range();
    test_parse_local_version();
    test_parse_local_features();
    test_parse_buffer_size();
    test_encode_inquiry();
    test_inquiry_result_iter();
    test_encode_le_scan();
    test_le_adv_report_iter();
    test_le_adv_report_iter_rejects_wrong_subevent();
    test_le_security_commands();
    test_le_security_returns_and_p256_events();
}
