#include "test_l2cap.h"
#include "../support/test.h"

#include <btcore/l2cap.h>

#include <string.h>

static void test_header_round_trip(void)
{
    uint8_t buf[BT_L2CAP_HEADER_LEN];
    struct bt_buf_writer w;
    struct bt_buf_reader r;
    struct bt_l2cap_header hdr;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_l2cap_encode_header(&w, 0x1234, BT_L2CAP_CID_ATT) == BT_OK);
    BT_CHECK(buf[0] == 0x34 && buf[1] == 0x12 && buf[2] == 0x04 && buf[3] == 0x00);

    bt_buf_reader_init(&r, buf, sizeof(buf));
    BT_CHECK(bt_l2cap_parse_header(&r, &hdr) == BT_OK);
    BT_CHECK(hdr.length == 0x1234);
    BT_CHECK(hdr.cid == BT_L2CAP_CID_ATT);
}

static void build_pdu(uint8_t *buf, uint16_t cid, const uint8_t *payload, uint16_t payload_len)
{
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, buf, BT_L2CAP_HEADER_LEN + payload_len);
    bt_l2cap_encode_header(&w, payload_len, cid);
    bt_buf_writer_write_bytes(&w, payload, payload_len);
}

static void test_reassembler_single_fragment(void)
{
    static const uint8_t payload[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint8_t pdu[BT_L2CAP_HEADER_LEN + 4];
    struct bt_l2cap_reassembler ra;
    const uint8_t *out;
    size_t out_len;

    build_pdu(pdu, BT_L2CAP_CID_ATT, payload, sizeof(payload));

    bt_l2cap_reassembler_init(&ra);
    BT_CHECK(bt_l2cap_reassembler_feed(&ra, 0x00, pdu, sizeof(pdu)) ==
              BT_L2CAP_REASSEMBLY_COMPLETE);

    out = bt_l2cap_reassembler_take(&ra, &out_len);
    BT_CHECK(out_len == sizeof(pdu));
    BT_CHECK(memcmp(out, pdu, sizeof(pdu)) == 0);
}

static void test_reassembler_multi_fragment(void)
{
    static const uint8_t payload[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint8_t pdu[BT_L2CAP_HEADER_LEN + 10];
    struct bt_l2cap_reassembler ra;
    const uint8_t *out;
    size_t out_len;

    build_pdu(pdu, BT_L2CAP_CID_SIGNALING_CLASSIC, payload, sizeof(payload));

    bt_l2cap_reassembler_init(&ra);
    /* Split into 3 fragments of 5 bytes each (14 bytes total: 4 header + 10 payload). */
    BT_CHECK(bt_l2cap_reassembler_feed(&ra, 0x00, pdu, 5) == BT_L2CAP_REASSEMBLY_MORE);
    BT_CHECK(bt_l2cap_reassembler_feed(&ra, 0x01, pdu + 5, 5) == BT_L2CAP_REASSEMBLY_MORE);
    BT_CHECK(bt_l2cap_reassembler_feed(&ra, 0x01, pdu + 10, 4) ==
              BT_L2CAP_REASSEMBLY_COMPLETE);

    out = bt_l2cap_reassembler_take(&ra, &out_len);
    BT_CHECK(out_len == sizeof(pdu));
    BT_CHECK(memcmp(out, pdu, sizeof(pdu)) == 0);
}

static void test_reassembler_header_split_across_fragments(void)
{
    /* Worst case for the "do we know the length yet" bookkeeping: the
     * very first fragment is smaller than the L2CAP header itself. */
    static const uint8_t payload[3] = {0x11, 0x22, 0x33};
    uint8_t pdu[BT_L2CAP_HEADER_LEN + 3];
    struct bt_l2cap_reassembler ra;
    const uint8_t *out;
    size_t out_len;

    build_pdu(pdu, BT_L2CAP_CID_ATT, payload, sizeof(payload));

    bt_l2cap_reassembler_init(&ra);
    BT_CHECK(bt_l2cap_reassembler_feed(&ra, 0x00, pdu, 2) == BT_L2CAP_REASSEMBLY_MORE);
    BT_CHECK(bt_l2cap_reassembler_feed(&ra, 0x01, pdu + 2, 2) == BT_L2CAP_REASSEMBLY_MORE);
    BT_CHECK(bt_l2cap_reassembler_feed(&ra, 0x01, pdu + 4, 3) == BT_L2CAP_REASSEMBLY_COMPLETE);

    out = bt_l2cap_reassembler_take(&ra, &out_len);
    BT_CHECK(out_len == sizeof(pdu));
    BT_CHECK(memcmp(out, pdu, sizeof(pdu)) == 0);
}

static void test_reassembler_stays_incomplete_when_truncated(void)
{
    static const uint8_t payload[10] = {0};
    uint8_t pdu[BT_L2CAP_HEADER_LEN + 10];
    struct bt_l2cap_reassembler ra;

    build_pdu(pdu, BT_L2CAP_CID_ATT, payload, sizeof(payload));

    bt_l2cap_reassembler_init(&ra);
    BT_CHECK(bt_l2cap_reassembler_feed(&ra, 0x00, pdu, 6) == BT_L2CAP_REASSEMBLY_MORE);
    /* Real link drops the rest -- nothing further arrives. The
     * reassembler must simply keep waiting, never fabricate completion. */
    BT_CHECK(bt_l2cap_reassembler_feed(&ra, 0x01, pdu + 6, 3) == BT_L2CAP_REASSEMBLY_MORE);
}

static void test_reassembler_continuation_without_start(void)
{
    static const uint8_t chunk[3] = {0x01, 0x02, 0x03};
    struct bt_l2cap_reassembler ra;

    bt_l2cap_reassembler_init(&ra);
    BT_CHECK(bt_l2cap_reassembler_feed(&ra, 0x01, chunk, sizeof(chunk)) ==
              BT_L2CAP_REASSEMBLY_ERROR);

    /* Must recover to idle, ready for a fresh start fragment. */
    static const uint8_t payload[1] = {0x99};
    uint8_t pdu[BT_L2CAP_HEADER_LEN + 1];
    build_pdu(pdu, BT_L2CAP_CID_ATT, payload, sizeof(payload));
    BT_CHECK(bt_l2cap_reassembler_feed(&ra, 0x00, pdu, sizeof(pdu)) ==
              BT_L2CAP_REASSEMBLY_COMPLETE);
}

static void test_reassembler_rejects_oversized_declared_length(void)
{
    uint8_t header[BT_L2CAP_HEADER_LEN];
    struct bt_buf_writer w;
    struct bt_l2cap_reassembler ra;

    /* Declares a payload far larger than BT_L2CAP_REASSEMBLY_MAX allows. */
    bt_buf_writer_init(&w, header, sizeof(header));
    bt_l2cap_encode_header(&w, 0xFFFF, BT_L2CAP_CID_ATT);

    bt_l2cap_reassembler_init(&ra);
    BT_CHECK(bt_l2cap_reassembler_feed(&ra, 0x00, header, sizeof(header)) ==
              BT_L2CAP_REASSEMBLY_ERROR);
}

static void test_reassembler_rejects_bytes_beyond_declared_length(void)
{
    static const uint8_t payload[4] = {1, 2, 3, 4};
    uint8_t pdu[BT_L2CAP_HEADER_LEN + 4];
    uint8_t extra[2] = {0xEE, 0xEE};
    struct bt_l2cap_reassembler ra;

    build_pdu(pdu, BT_L2CAP_CID_ATT, payload, sizeof(payload));

    bt_l2cap_reassembler_init(&ra);
    BT_CHECK(bt_l2cap_reassembler_feed(&ra, 0x00, pdu, sizeof(pdu) - 1) ==
              BT_L2CAP_REASSEMBLY_MORE);
    /* One byte short of "complete", but the extra fragment overshoots the
     * declared length instead of finishing it exactly. */
    BT_CHECK(bt_l2cap_reassembler_feed(&ra, 0x01, extra, sizeof(extra)) ==
              BT_L2CAP_REASSEMBLY_ERROR);
}

static void test_reassembler_new_start_abandons_in_progress(void)
{
    static const uint8_t payload_a[10] = {0};
    static const uint8_t payload_b[2] = {0x77, 0x88};
    uint8_t pdu_a[BT_L2CAP_HEADER_LEN + 10];
    uint8_t pdu_b[BT_L2CAP_HEADER_LEN + 2];
    struct bt_l2cap_reassembler ra;
    const uint8_t *out;
    size_t out_len;

    build_pdu(pdu_a, BT_L2CAP_CID_ATT, payload_a, sizeof(payload_a));
    build_pdu(pdu_b, BT_L2CAP_CID_SIGNALING_LE, payload_b, sizeof(payload_b));

    bt_l2cap_reassembler_init(&ra);
    BT_CHECK(bt_l2cap_reassembler_feed(&ra, 0x00, pdu_a, 6) == BT_L2CAP_REASSEMBLY_MORE);

    /* A second start fragment arrives before pdu_a finished -- abandon it
     * and reassemble pdu_b cleanly instead. */
    BT_CHECK(bt_l2cap_reassembler_feed(&ra, 0x00, pdu_b, sizeof(pdu_b)) ==
              BT_L2CAP_REASSEMBLY_COMPLETE);

    out = bt_l2cap_reassembler_take(&ra, &out_len);
    BT_CHECK(out_len == sizeof(pdu_b));
    BT_CHECK(memcmp(out, pdu_b, sizeof(pdu_b)) == 0);
}

static void test_fragmenter_and_reassembler_round_trip(void)
{
    uint8_t payload[50];
    uint8_t pdu[BT_L2CAP_HEADER_LEN + 50];
    struct bt_l2cap_fragmenter fr;
    struct bt_l2cap_reassembler ra;
    size_t i;
    int fragment_count = 0;

    for (i = 0; i < sizeof(payload); i++)
        payload[i] = (uint8_t)i;
    build_pdu(pdu, BT_L2CAP_CID_ATT, payload, sizeof(payload));

    /* Small frag_size forces several ACL fragments for one PDU. */
    bt_l2cap_fragmenter_init(&fr, 0x0041, 16, pdu, sizeof(pdu));
    bt_l2cap_reassembler_init(&ra);

    for (;;)
    {
        uint8_t acl[4 + 16];
        struct bt_buf_writer w;
        struct bt_buf_reader r;
        struct bt_hci_acl_header acl_hdr;
        bt_status_t st;
        enum bt_l2cap_reassembly_result rr;

        bt_buf_writer_init(&w, acl, sizeof(acl));
        st = bt_l2cap_fragmenter_next(&fr, &w);
        if (st == BT_ERR_BUFFER_UNDERFLOW)
            break;
        BT_CHECK(st == BT_OK);
        fragment_count++;

        bt_buf_reader_init(&r, acl, bt_buf_writer_len(&w));
        BT_CHECK(bt_hci_parse_acl_header(&r, &acl_hdr) == BT_OK);
        BT_CHECK(acl_hdr.handle == 0x0041);

        rr = bt_l2cap_reassembler_feed(&ra, acl_hdr.pb_flag, bt_buf_reader_peek(&r, acl_hdr.data_len),
                                        acl_hdr.data_len);
        BT_CHECK(rr != BT_L2CAP_REASSEMBLY_ERROR);
    }

    BT_CHECK(fragment_count == 4); /* 54 bytes / 16 per fragment, rounded up */

    {
        const uint8_t *out;
        size_t out_len;

        out = bt_l2cap_reassembler_take(&ra, &out_len);
        BT_CHECK(out_len == sizeof(pdu));
        BT_CHECK(memcmp(out, pdu, sizeof(pdu)) == 0);
    }
}

static void test_fragmenter_single_fragment_when_it_fits(void)
{
    static const uint8_t payload[4] = {1, 2, 3, 4};
    uint8_t pdu[BT_L2CAP_HEADER_LEN + 4];
    struct bt_l2cap_fragmenter fr;
    uint8_t acl[64];
    struct bt_buf_writer w;

    build_pdu(pdu, BT_L2CAP_CID_ATT, payload, sizeof(payload));

    bt_l2cap_fragmenter_init(&fr, 0x0001, 200, pdu, sizeof(pdu));

    bt_buf_writer_init(&w, acl, sizeof(acl));
    BT_CHECK(bt_l2cap_fragmenter_next(&fr, &w) == BT_OK);
    BT_CHECK(bt_buf_writer_len(&w) == BT_HCI_ACL_HEADER_LEN + sizeof(pdu));
    BT_CHECK(acl[2] == (uint8_t)sizeof(pdu)); /* ACL data_len low byte */

    bt_buf_writer_init(&w, acl, sizeof(acl));
    BT_CHECK(bt_l2cap_fragmenter_next(&fr, &w) == BT_ERR_BUFFER_UNDERFLOW);
}

void run_l2cap_tests(void)
{
    test_header_round_trip();
    test_reassembler_single_fragment();
    test_reassembler_multi_fragment();
    test_reassembler_header_split_across_fragments();
    test_reassembler_stays_incomplete_when_truncated();
    test_reassembler_continuation_without_start();
    test_reassembler_rejects_oversized_declared_length();
    test_reassembler_rejects_bytes_beyond_declared_length();
    test_reassembler_new_start_abandons_in_progress();
    test_fragmenter_and_reassembler_round_trip();
    test_fragmenter_single_fragment_when_it_fits();
}
