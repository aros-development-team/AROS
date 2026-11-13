#include "test_signaling.h"
#include "../support/test.h"

#include <btcore/l2cap.h>

static void test_header_round_trip(void)
{
    uint8_t buf[BT_L2CAP_SIG_HEADER_LEN];
    struct bt_buf_writer w;
    struct bt_buf_reader r;
    struct bt_l2cap_sig_header hdr;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_l2cap_sig_encode_header(&w, BT_L2CAP_SIG_CONNECTION_REQUEST, 0x07, 4) == BT_OK);

    bt_buf_reader_init(&r, buf, sizeof(buf));
    BT_CHECK(bt_l2cap_sig_parse_header(&r, &hdr) == BT_OK);
    BT_CHECK(hdr.code == BT_L2CAP_SIG_CONNECTION_REQUEST);
    BT_CHECK(hdr.identifier == 0x07);
    BT_CHECK(hdr.length == 4);
}

static void test_connection_request_round_trip(void)
{
    uint8_t buf[32];
    struct bt_buf_writer w;
    struct bt_l2cap_sig_header hdr;
    struct bt_buf_reader r;
    struct bt_l2cap_connection_request req;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_l2cap_sig_encode_connection_request(&w, 0x01, 0x0019, 0x0040) == BT_OK);
    BT_CHECK(bt_buf_writer_len(&w) == BT_L2CAP_SIG_HEADER_LEN + 4);

    bt_buf_reader_init(&r, buf, bt_buf_writer_len(&w));
    BT_CHECK(bt_l2cap_sig_parse_header(&r, &hdr) == BT_OK);
    BT_CHECK(hdr.code == BT_L2CAP_SIG_CONNECTION_REQUEST);
    BT_CHECK(hdr.length == 4);

    BT_CHECK(bt_l2cap_sig_parse_connection_request(buf + BT_L2CAP_SIG_HEADER_LEN, hdr.length,
                                                     &req) == BT_OK);
    BT_CHECK(req.psm == 0x0019);
    BT_CHECK(req.source_cid == 0x0040);
}

static void test_connection_response_round_trip(void)
{
    uint8_t buf[32];
    struct bt_buf_writer w;
    struct bt_l2cap_connection_response rsp;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_l2cap_sig_encode_connection_response(&w, 0x01, 0x0041, 0x0040,
                                                       BT_L2CAP_CONN_RESULT_SUCCESS, 0) == BT_OK);

    BT_CHECK(bt_l2cap_sig_parse_connection_response(buf + BT_L2CAP_SIG_HEADER_LEN, 8, &rsp) ==
              BT_OK);
    BT_CHECK(rsp.destination_cid == 0x0041);
    BT_CHECK(rsp.source_cid == 0x0040);
    BT_CHECK(rsp.result == BT_L2CAP_CONN_RESULT_SUCCESS);
    BT_CHECK(rsp.status == 0);
}

static void test_configure_request_with_mtu(void)
{
    uint8_t buf[32];
    struct bt_buf_writer w;
    struct bt_l2cap_configure_request req;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_l2cap_sig_encode_configure_request(&w, 0x02, 0x0041, 0x0000, 672) == BT_OK);
    BT_CHECK(bt_buf_writer_len(&w) == BT_L2CAP_SIG_HEADER_LEN + 8);

    BT_CHECK(bt_l2cap_sig_parse_configure_request(buf + BT_L2CAP_SIG_HEADER_LEN, 8, &req) ==
              BT_OK);
    BT_CHECK(req.destination_cid == 0x0041);
    BT_CHECK(req.flags == 0x0000);
    BT_CHECK(req.has_mtu);
    BT_CHECK(req.mtu == 672);
}

static void test_configure_request_without_mtu(void)
{
    uint8_t buf[32];
    struct bt_buf_writer w;
    struct bt_l2cap_configure_request req;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_l2cap_sig_encode_configure_request(&w, 0x02, 0x0041, 0x0000, 0) == BT_OK);
    BT_CHECK(bt_buf_writer_len(&w) == BT_L2CAP_SIG_HEADER_LEN + 4);

    BT_CHECK(bt_l2cap_sig_parse_configure_request(buf + BT_L2CAP_SIG_HEADER_LEN, 4, &req) ==
              BT_OK);
    BT_CHECK(!req.has_mtu);
}

static void test_configure_request_skips_unknown_option(void)
{
    /* dest_cid, flags, then an unknown option (type 0x99, len 3, junk),
     * then the MTU option -- parser must skip the unknown one and still
     * find MTU. */
    uint8_t params[] = {
        0x41, 0x00, /* destination_cid */
        0x00, 0x00, /* flags */
        0x99, 0x03, 0xAA, 0xBB, 0xCC, /* unknown option */
        BT_L2CAP_CONFIG_OPTION_MTU, 0x02, 0xA0, 0x02, /* MTU = 0x02A0 */
    };
    struct bt_l2cap_configure_request req;

    BT_CHECK(bt_l2cap_sig_parse_configure_request(params, sizeof(params), &req) == BT_OK);
    BT_CHECK(req.has_mtu);
    BT_CHECK(req.mtu == 0x02A0);
}

static void test_configure_response_round_trip(void)
{
    uint8_t buf[32];
    struct bt_buf_writer w;
    struct bt_l2cap_configure_response rsp;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_l2cap_sig_encode_configure_response(&w, 0x02, 0x0040, 0x0000,
                                                      BT_L2CAP_CONFIG_RESULT_SUCCESS, 512) ==
              BT_OK);
    BT_CHECK(bt_buf_writer_len(&w) == BT_L2CAP_SIG_HEADER_LEN + 10);

    BT_CHECK(bt_l2cap_sig_parse_configure_response(buf + BT_L2CAP_SIG_HEADER_LEN, 10, &rsp) ==
              BT_OK);
    BT_CHECK(rsp.source_cid == 0x0040);
    BT_CHECK(rsp.result == BT_L2CAP_CONFIG_RESULT_SUCCESS);
    BT_CHECK(rsp.has_mtu);
    BT_CHECK(rsp.mtu == 512);
}

static void test_disconnection_round_trip(void)
{
    uint8_t req_buf[32];
    uint8_t rsp_buf[32];
    struct bt_buf_writer w;
    struct bt_l2cap_disconnection req;
    struct bt_l2cap_disconnection rsp;

    bt_buf_writer_init(&w, req_buf, sizeof(req_buf));
    BT_CHECK(bt_l2cap_sig_encode_disconnection_request(&w, 0x03, 0x0041, 0x0040) == BT_OK);
    BT_CHECK(bt_l2cap_sig_parse_disconnection(req_buf + BT_L2CAP_SIG_HEADER_LEN, 4, &req) ==
              BT_OK);
    BT_CHECK(req.destination_cid == 0x0041);
    BT_CHECK(req.source_cid == 0x0040);

    bt_buf_writer_init(&w, rsp_buf, sizeof(rsp_buf));
    BT_CHECK(bt_l2cap_sig_encode_disconnection_response(&w, 0x03, 0x0041, 0x0040) == BT_OK);
    BT_CHECK(bt_l2cap_sig_parse_disconnection(rsp_buf + BT_L2CAP_SIG_HEADER_LEN, 4, &rsp) ==
              BT_OK);
    BT_CHECK(rsp.destination_cid == 0x0041);
    BT_CHECK(rsp.source_cid == 0x0040);
}

static void test_command_reject_round_trip(void)
{
    uint8_t buf[32];
    struct bt_buf_writer w;
    struct bt_l2cap_command_reject rej;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_l2cap_sig_encode_command_reject(&w, 0x09, 0x0000) == BT_OK);

    BT_CHECK(bt_l2cap_sig_parse_command_reject(buf + BT_L2CAP_SIG_HEADER_LEN, 2, &rej) == BT_OK);
    BT_CHECK(rej.reason == 0x0000);
    BT_CHECK(rej.data_len == 0);
}

static void test_invalid_lengths_rejected(void)
{
    struct bt_l2cap_connection_request creq;
    struct bt_l2cap_connection_response crsp;
    struct bt_l2cap_configure_request cfgreq;
    struct bt_l2cap_configure_response cfgrsp;
    struct bt_l2cap_disconnection disc;
    struct bt_l2cap_command_reject rej;
    static const uint8_t junk[8] = {0};

    BT_CHECK(bt_l2cap_sig_parse_connection_request(junk, 3, &creq) == BT_ERR_INVALID_ARGUMENT);
    BT_CHECK(bt_l2cap_sig_parse_connection_response(junk, 7, &crsp) == BT_ERR_INVALID_ARGUMENT);
    BT_CHECK(bt_l2cap_sig_parse_configure_request(junk, 3, &cfgreq) == BT_ERR_INVALID_ARGUMENT);
    BT_CHECK(bt_l2cap_sig_parse_configure_response(junk, 5, &cfgrsp) == BT_ERR_INVALID_ARGUMENT);
    BT_CHECK(bt_l2cap_sig_parse_disconnection(junk, 3, &disc) == BT_ERR_INVALID_ARGUMENT);
    BT_CHECK(bt_l2cap_sig_parse_command_reject(junk, 1, &rej) == BT_ERR_INVALID_ARGUMENT);
}

void run_l2cap_signaling_tests(void)
{
    test_header_round_trip();
    test_connection_request_round_trip();
    test_connection_response_round_trip();
    test_configure_request_with_mtu();
    test_configure_request_without_mtu();
    test_configure_request_skips_unknown_option();
    test_configure_response_round_trip();
    test_disconnection_round_trip();
    test_command_reject_round_trip();
    test_invalid_lengths_rejected();
}
