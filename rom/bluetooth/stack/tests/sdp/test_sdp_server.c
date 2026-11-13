#include "test_sdp_server.h"
#include "../support/test.h"

#include <btcore/sdp_server.h>
#include <btcore/sdp.h>
#include <btcore/buffer.h>

#include <string.h>

/* an SPP-shaped record: handle, class list, protocol list (L2CAP, RFCOMM ch),
 * browse group, profile list, name */
static size_t build_record(uint8_t *out, size_t out_max, uint32_t handle, uint16_t cls,
                           uint8_t channel, const char *name)
{
    struct bt_buf_writer w;
    size_t nlen = strlen(name);

    bt_buf_writer_init(&w, out, out_max);
    bt_sdp_encode_uint(&w, 0x0000, 2); bt_sdp_encode_uint(&w, handle, 4);
    bt_sdp_encode_uint(&w, 0x0001, 2); bt_sdp_encode_sequence_header(&w, 3); bt_sdp_encode_uuid16(&w, cls);
    bt_sdp_encode_uint(&w, 0x0004, 2);
    bt_sdp_encode_sequence_header(&w, 12);
    bt_sdp_encode_sequence_header(&w, 3); bt_sdp_encode_uuid16(&w, 0x0100);
    bt_sdp_encode_sequence_header(&w, 5); bt_sdp_encode_uuid16(&w, 0x0003); bt_sdp_encode_uint(&w, channel, 1);
    bt_sdp_encode_uint(&w, 0x0005, 2); bt_sdp_encode_sequence_header(&w, 3); bt_sdp_encode_uuid16(&w, 0x1002);
    bt_sdp_encode_uint(&w, 0x0009, 2);
    bt_sdp_encode_sequence_header(&w, 8);
    bt_sdp_encode_sequence_header(&w, 6); bt_sdp_encode_uuid16(&w, cls); bt_sdp_encode_uint(&w, 0x0102, 2);
    bt_sdp_encode_uint(&w, 0x0100, 2);
    bt_buf_writer_write_u8(&w, 0x25); bt_buf_writer_write_u8(&w, (uint8_t)nlen);
    bt_buf_writer_write_bytes(&w, (const uint8_t *)name, nlen);
    return bt_buf_writer_len(&w);
}

static uint8_t rec1[128], rec2[128];
static struct bt_sdp_record recs[2];

static const struct bt_sdp_record *record_at(void *ctx, size_t index)
{
    (void)ctx;
    return index < 2 ? &recs[index] : NULL;
}

static size_t search_request(uint8_t *buf, size_t max, uint16_t tid, uint16_t uuid, uint16_t maxcount,
                             const uint8_t *cont, uint8_t contlen)
{
    struct bt_buf_writer w;
    bt_buf_writer_init(&w, buf, max);
    bt_sdp_encode_header(&w, BT_SDP_PDU_SERVICE_SEARCH_REQUEST, tid, (uint16_t)(5 + 2 + 1 + contlen));
    bt_sdp_encode_sequence_header(&w, 3); bt_sdp_encode_uuid16(&w, uuid);
    bt_buf_writer_write_be16(&w, maxcount);
    bt_buf_writer_write_u8(&w, contlen);
    if (contlen) bt_buf_writer_write_bytes(&w, cont, contlen);
    return bt_buf_writer_len(&w);
}

static size_t attr_request(uint8_t *buf, size_t max, uint16_t tid, uint32_t handle, uint16_t maxbytes,
                           const uint8_t *cont, uint8_t contlen)
{
    struct bt_buf_writer w;
    bt_buf_writer_init(&w, buf, max);
    bt_sdp_encode_header(&w, BT_SDP_PDU_SERVICE_ATTRIBUTE_REQUEST, tid, (uint16_t)(4 + 2 + 7 + 1 + contlen));
    bt_buf_writer_write_be32(&w, handle);
    bt_buf_writer_write_be16(&w, maxbytes);
    bt_sdp_encode_sequence_header(&w, 5); bt_sdp_encode_uint(&w, 0x0000ffffu, 4);
    bt_buf_writer_write_u8(&w, contlen);
    if (contlen) bt_buf_writer_write_bytes(&w, cont, contlen);
    return bt_buf_writer_len(&w);
}

static size_t search_attr_request(uint8_t *buf, size_t max, uint16_t tid, uint16_t uuid, uint16_t maxbytes,
                                  const uint8_t *cont, uint8_t contlen)
{
    struct bt_buf_writer w;
    bt_buf_writer_init(&w, buf, max);
    bt_sdp_encode_header(&w, 0x06, tid, (uint16_t)(5 + 2 + 8 + 1 + contlen));
    bt_sdp_encode_sequence_header(&w, 3); bt_sdp_encode_uuid16(&w, uuid);
    bt_buf_writer_write_be16(&w, maxbytes);
    bt_sdp_encode_sequence_header(&w, 6); bt_sdp_encode_uint(&w, 0x0001, 2); bt_sdp_encode_uint(&w, 0x0004, 2);
    bt_buf_writer_write_u8(&w, contlen);
    if (contlen) bt_buf_writer_write_bytes(&w, cont, contlen);
    return bt_buf_writer_len(&w);
}

static void test_search_and_attributes(void)
{
    struct bt_sdp_server srv;
    uint8_t req[64], rsp[400];
    size_t rlen;
    struct bt_buf_reader r;
    struct bt_sdp_header hdr;
    uint16_t total, current, count;
    uint32_t handle;

    recs[0].handle = 0x00010000; recs[0].attrs = rec1;
    recs[0].attrs_len = build_record(rec1, sizeof(rec1), 0x00010000, 0x1101, 1, "Serial Port");
    recs[1].handle = 0x00010001; recs[1].attrs = rec2;
    recs[1].attrs_len = build_record(rec2, sizeof(rec2), 0x00010001, 0x1115, 2, "PAN User");
    bt_sdp_server_init(&srv, record_at, NULL);

    /* the record matches its class, the L2CAP protocol UUID, and not others */
    {
        uint8_t pat[8]; struct bt_buf_writer w;
        bt_buf_writer_init(&w, pat, sizeof(pat)); bt_sdp_encode_uuid16(&w, 0x1101);
        BT_CHECK(bt_sdp_record_matches(&recs[0], pat, bt_buf_writer_len(&w)));
        BT_CHECK(!bt_sdp_record_matches(&recs[1], pat, bt_buf_writer_len(&w)));
        bt_buf_writer_init(&w, pat, sizeof(pat)); bt_sdp_encode_uuid16(&w, 0x0100);
        BT_CHECK(bt_sdp_record_matches(&recs[0], pat, bt_buf_writer_len(&w)));
        BT_CHECK(bt_sdp_record_matches(&recs[1], pat, bt_buf_writer_len(&w)));
    }

    /* ServiceSearch for SPP: one handle */
    rlen = bt_sdp_server_handle(&srv, req, search_request(req, sizeof(req), 0x1234, 0x1101, 10, NULL, 0), rsp, sizeof(rsp));
    BT_CHECK(rlen > 0);
    bt_buf_reader_init(&r, rsp, rlen);
    BT_CHECK(bt_sdp_parse_header(&r, &hdr) == BT_OK);
    BT_CHECK(hdr.pdu_id == BT_SDP_PDU_SERVICE_SEARCH_RESPONSE && hdr.transaction_id == 0x1234);
    BT_CHECK(bt_buf_reader_read_be16(&r, &total) == BT_OK && total == 1);
    BT_CHECK(bt_buf_reader_read_be16(&r, &current) == BT_OK && current == 1);
    BT_CHECK(bt_buf_reader_read_be32(&r, &handle) == BT_OK && handle == 0x00010000);
    BT_CHECK(rsp[rlen - 1] == 0);   /* no continuation */

    /* for the L2CAP UUID: both records */
    rlen = bt_sdp_server_handle(&srv, req, search_request(req, sizeof(req), 1, 0x0100, 10, NULL, 0), rsp, sizeof(rsp));
    bt_buf_reader_init(&r, rsp, rlen);
    bt_sdp_parse_header(&r, &hdr);
    bt_buf_reader_read_be16(&r, &total); bt_buf_reader_read_be16(&r, &current);
    BT_CHECK(total == 2 && current == 2);

    /* ServiceAttribute: the whole record back, wrapped in a sequence */
    rlen = bt_sdp_server_handle(&srv, req, attr_request(req, sizeof(req), 2, 0x00010000, 1000, NULL, 0), rsp, sizeof(rsp));
    bt_buf_reader_init(&r, rsp, rlen);
    BT_CHECK(bt_sdp_parse_header(&r, &hdr) == BT_OK && hdr.pdu_id == BT_SDP_PDU_SERVICE_ATTRIBUTE_RESPONSE);
    BT_CHECK(bt_buf_reader_read_be16(&r, &count) == BT_OK);
    {
        struct bt_sdp_element e;
        BT_CHECK(bt_sdp_parse_element(&r, &e) == BT_OK && e.type == BT_SDP_ELEM_SEQUENCE);
        BT_CHECK(e.seq_len == recs[0].attrs_len && memcmp(e.seq_data, rec1, e.seq_len) == 0);
        BT_CHECK(count == e.seq_len + 2);
    }

    /* an unknown handle: error 0x0002 */
    rlen = bt_sdp_server_handle(&srv, req, attr_request(req, sizeof(req), 3, 0x00020000, 1000, NULL, 0), rsp, sizeof(rsp));
    bt_buf_reader_init(&r, rsp, rlen);
    bt_sdp_parse_header(&r, &hdr);
    BT_CHECK(hdr.pdu_id == BT_SDP_PDU_ERROR_RESPONSE && rsp[5] == 0 && rsp[6] == 2);

    /* ServiceSearchAttribute, attributes 0x0001 and 0x0004 of both records */
    rlen = bt_sdp_server_handle(&srv, req, search_attr_request(req, sizeof(req), 4, 0x0100, 1000, NULL, 0), rsp, sizeof(rsp));
    bt_buf_reader_init(&r, rsp, rlen);
    BT_CHECK(bt_sdp_parse_header(&r, &hdr) == BT_OK && hdr.pdu_id == 0x07);
    BT_CHECK(bt_buf_reader_read_be16(&r, &count) == BT_OK);
    {
        struct bt_sdp_element lists, one, id;
        struct bt_sdp_element_iter it, inner;
        int n = 0;
        BT_CHECK(bt_sdp_parse_element(&r, &lists) == BT_OK && lists.type == BT_SDP_ELEM_SEQUENCE);
        bt_sdp_element_iter_init(&it, lists.seq_data, lists.seq_len);
        while (bt_sdp_element_iter_next(&it, &one) == BT_OK)
        {
            int attrs = 0;
            BT_CHECK(one.type == BT_SDP_ELEM_SEQUENCE);
            bt_sdp_element_iter_init(&inner, one.seq_data, one.seq_len);
            while (bt_sdp_element_iter_next(&inner, &id) == BT_OK)
            {
                struct bt_sdp_element val;
                BT_CHECK(id.type == BT_SDP_ELEM_UINT && (id.uint == 0x0001 || id.uint == 0x0004));
                BT_CHECK(bt_sdp_element_iter_next(&inner, &val) == BT_OK);
                attrs++;
            }
            BT_CHECK(attrs == 2);
            n++;
        }
        BT_CHECK(n == 2);
    }
}

/* a response that does not fit the channel comes in continued slices */
static void test_continuation(void)
{
    struct bt_sdp_server srv;
    uint8_t req[64], rsp[64], whole[400];
    size_t rlen, got = 0;
    struct bt_buf_reader r;
    struct bt_sdp_header hdr;
    uint16_t count;
    uint8_t cont[3];
    uint8_t contlen = 0;
    int rounds = 0;

    bt_sdp_server_init(&srv, record_at, NULL);
    do
    {
        rlen = bt_sdp_server_handle(&srv, req, attr_request(req, sizeof(req), 9, 0x00010000, 1000, cont, contlen),
                                    rsp, 48);
        BT_CHECK(rlen > 0 && rlen <= 48);
        bt_buf_reader_init(&r, rsp, rlen);
        BT_CHECK(bt_sdp_parse_header(&r, &hdr) == BT_OK && hdr.pdu_id == BT_SDP_PDU_SERVICE_ATTRIBUTE_RESPONSE);
        BT_CHECK(bt_buf_reader_read_be16(&r, &count) == BT_OK);
        memcpy(whole + got, bt_buf_reader_peek(&r, count), count);
        got += count;
        bt_buf_reader_skip(&r, count);
        BT_CHECK(bt_buf_reader_read_u8(&r, &contlen) == BT_OK);
        if (contlen)
        {
            BT_CHECK(contlen == 2);
            memcpy(cont, bt_buf_reader_peek(&r, 2), 2);
        }
        rounds++;
    } while (contlen && rounds < 20);
    BT_CHECK(rounds > 1);
    {
        struct bt_sdp_element e;
        bt_buf_reader_init(&r, whole, got);
        BT_CHECK(bt_sdp_parse_element(&r, &e) == BT_OK && e.type == BT_SDP_ELEM_SEQUENCE);
        BT_CHECK(e.seq_len == recs[0].attrs_len && memcmp(e.seq_data, rec1, e.seq_len) == 0);
    }
    /* a stale continuation is refused */
    cont[0] = 0; cont[1] = 5;
    rlen = bt_sdp_server_handle(&srv, req, attr_request(req, sizeof(req), 10, 0x00010000, 1000, cont, 2), rsp, sizeof(rsp));
    bt_buf_reader_init(&r, rsp, rlen);
    bt_sdp_parse_header(&r, &hdr);
    BT_CHECK(hdr.pdu_id == BT_SDP_PDU_ERROR_RESPONSE && rsp[6] == 5);
}

void run_sdp_server_tests(void)
{
    test_search_and_attributes();
    test_continuation();
}
