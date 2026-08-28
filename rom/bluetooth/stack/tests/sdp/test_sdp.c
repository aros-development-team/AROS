#include "test_sdp.h"
#include "../support/test.h"

#include <btcore/sdp.h>

#include <string.h>

static void test_header_round_trip(void)
{
    uint8_t buf[BT_SDP_HEADER_LEN];
    struct bt_buf_writer w;
    struct bt_buf_reader r;
    struct bt_sdp_header hdr;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_sdp_encode_header(&w, BT_SDP_PDU_SERVICE_SEARCH_REQUEST, 0x1234, 0x0A0B) == BT_OK);

    /* Unlike HCI/L2CAP, SDP integers are big-endian. */
    BT_CHECK(buf[0] == BT_SDP_PDU_SERVICE_SEARCH_REQUEST);
    BT_CHECK(buf[1] == 0x12 && buf[2] == 0x34);
    BT_CHECK(buf[3] == 0x0A && buf[4] == 0x0B);

    bt_buf_reader_init(&r, buf, sizeof(buf));
    BT_CHECK(bt_sdp_parse_header(&r, &hdr) == BT_OK);
    BT_CHECK(hdr.pdu_id == BT_SDP_PDU_SERVICE_SEARCH_REQUEST);
    BT_CHECK(hdr.transaction_id == 0x1234);
    BT_CHECK(hdr.param_len == 0x0A0B);
}

static void test_uint_round_trip(void)
{
    uint8_t buf[8];
    struct bt_buf_writer w;
    struct bt_buf_reader r;
    struct bt_sdp_element elem;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_sdp_encode_uint(&w, 0x42, 1) == BT_OK);
    BT_CHECK(buf[0] == (uint8_t)((1u << 3) | 0)); /* type=UInt, size_desc=0 */
    BT_CHECK(buf[1] == 0x42);

    bt_buf_reader_init(&r, buf, bt_buf_writer_len(&w));
    BT_CHECK(bt_sdp_parse_element(&r, &elem) == BT_OK);
    BT_CHECK(elem.type == BT_SDP_ELEM_UINT);
    BT_CHECK(elem.width == 1);
    BT_CHECK(elem.uint == 0x42);

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_sdp_encode_uint(&w, 0x1234, 2) == BT_OK);
    BT_CHECK(buf[1] == 0x12 && buf[2] == 0x34); /* big-endian */
    bt_buf_reader_init(&r, buf, bt_buf_writer_len(&w));
    BT_CHECK(bt_sdp_parse_element(&r, &elem) == BT_OK);
    BT_CHECK(elem.width == 2);
    BT_CHECK(elem.uint == 0x1234);

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_sdp_encode_uint(&w, 0x12345678, 4) == BT_OK);
    bt_buf_reader_init(&r, buf, bt_buf_writer_len(&w));
    BT_CHECK(bt_sdp_parse_element(&r, &elem) == BT_OK);
    BT_CHECK(elem.width == 4);
    BT_CHECK(elem.uint == 0x12345678);

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_sdp_encode_uint(&w, 0, 3) == BT_ERR_INVALID_ARGUMENT);
}

static void test_uuid_round_trip(void)
{
    uint8_t buf[20];
    struct bt_buf_writer w;
    struct bt_buf_reader r;
    struct bt_sdp_element elem;
    static const uint8_t uuid128[16] = {0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00,
                                         0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_sdp_encode_uuid16(&w, 0x0100) == BT_OK); /* L2CAP service class */
    BT_CHECK(buf[0] == (uint8_t)((3u << 3) | 1));
    BT_CHECK(buf[1] == 0x01 && buf[2] == 0x00);
    bt_buf_reader_init(&r, buf, bt_buf_writer_len(&w));
    BT_CHECK(bt_sdp_parse_element(&r, &elem) == BT_OK);
    BT_CHECK(elem.type == BT_SDP_ELEM_UUID16);
    BT_CHECK(elem.uuid16 == 0x0100);

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_sdp_encode_uuid128(&w, uuid128) == BT_OK);
    bt_buf_reader_init(&r, buf, bt_buf_writer_len(&w));
    BT_CHECK(bt_sdp_parse_element(&r, &elem) == BT_OK);
    BT_CHECK(elem.type == BT_SDP_ELEM_UUID128);
    BT_CHECK(memcmp(elem.uuid128, uuid128, 16) == 0);
}

static void test_nested_sequence(void)
{
    uint8_t inner[8];
    struct bt_buf_writer iw;
    uint8_t outer[16];
    struct bt_buf_writer ow;
    struct bt_buf_reader r;
    struct bt_sdp_element seq_elem;
    struct bt_sdp_element_iter it;
    struct bt_sdp_element e1, e2;

    bt_buf_writer_init(&iw, inner, sizeof(inner));
    bt_sdp_encode_uuid16(&iw, 0x0100);
    bt_sdp_encode_uint(&iw, 0x0001, 2);

    bt_buf_writer_init(&ow, outer, sizeof(outer));
    BT_CHECK(bt_sdp_encode_sequence_header(&ow, (uint16_t)bt_buf_writer_len(&iw)) == BT_OK);
    bt_buf_writer_write_bytes(&ow, inner, bt_buf_writer_len(&iw));

    bt_buf_reader_init(&r, outer, bt_buf_writer_len(&ow));
    BT_CHECK(bt_sdp_parse_element(&r, &seq_elem) == BT_OK);
    BT_CHECK(seq_elem.type == BT_SDP_ELEM_SEQUENCE);
    BT_CHECK(seq_elem.seq_len == bt_buf_writer_len(&iw));

    bt_sdp_element_iter_init(&it, seq_elem.seq_data, seq_elem.seq_len);
    BT_CHECK(bt_sdp_element_iter_next(&it, &e1) == BT_OK);
    BT_CHECK(e1.type == BT_SDP_ELEM_UUID16 && e1.uuid16 == 0x0100);
    BT_CHECK(bt_sdp_element_iter_next(&it, &e2) == BT_OK);
    BT_CHECK(e2.type == BT_SDP_ELEM_UINT && e2.uint == 0x0001);
    BT_CHECK(bt_sdp_element_iter_next(&it, &e1) == BT_ERR_BUFFER_UNDERFLOW);
}

static void test_large_sequence_uses_2byte_length(void)
{
    uint8_t buf[303];
    struct bt_buf_writer w;
    struct bt_buf_reader r;
    struct bt_sdp_element elem;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_sdp_encode_sequence_header(&w, 300) == BT_OK);
    BT_CHECK(buf[0] == (uint8_t)((6u << 3) | 6)); /* size_desc 6 -> 2-byte length */
    BT_CHECK(buf[1] == 0x01 && buf[2] == 0x2C);   /* 300 = 0x012C, big-endian */

    /* Feed 300 bytes of padding after the header so parsing can walk past it. */
    memset(buf + 3, 0xAA, 300);
    bt_buf_reader_init(&r, buf, 3 + 300);
    BT_CHECK(bt_sdp_parse_element(&r, &elem) == BT_OK);
    BT_CHECK(elem.type == BT_SDP_ELEM_SEQUENCE);
    BT_CHECK(elem.seq_len == 300);
}

static void test_unsupported_type_rejected(void)
{
    /* Type 4 (Text string) with an invalid size descriptor (2 = 4 bytes,
       only meaningful for fixed-size types) is rejected; a reserved type
       (9) as well. */
    static const uint8_t wire[] = {(uint8_t)((4u << 3) | 2), 0x03, 'a', 'b', 'c'};
    static const uint8_t wire2[] = {(uint8_t)((9u << 3) | 5), 0x03, 'a', 'b', 'c'};
    struct bt_buf_reader r;
    struct bt_sdp_element elem;

    bt_buf_reader_init(&r, wire, sizeof(wire));
    BT_CHECK(bt_sdp_parse_element(&r, &elem) == BT_ERR_INVALID_ARGUMENT);
    bt_buf_reader_init(&r, wire2, sizeof(wire2));
    BT_CHECK(bt_sdp_parse_element(&r, &elem) == BT_ERR_INVALID_ARGUMENT);
}

static void test_service_search_request_encode(void)
{
    uint8_t pattern[8];
    struct bt_buf_writer pw;
    uint8_t buf[64];
    struct bt_buf_writer w;
    struct bt_buf_reader r;
    struct bt_sdp_header hdr;
    struct bt_sdp_element elem;
    uint16_t max_count;
    uint8_t cont_len;

    bt_buf_writer_init(&pw, pattern, sizeof(pattern));
    bt_sdp_encode_sequence_header(&pw, 3); /* one UUID16 = 1 header byte + 2 data bytes */
    bt_sdp_encode_uuid16(&pw, 0x0100);

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_sdp_encode_service_search_request(&w, 0x0001, pattern, bt_buf_writer_len(&pw), 10,
                                                    NULL) == BT_OK);

    bt_buf_reader_init(&r, buf, bt_buf_writer_len(&w));
    BT_CHECK(bt_sdp_parse_header(&r, &hdr) == BT_OK);
    BT_CHECK(hdr.pdu_id == BT_SDP_PDU_SERVICE_SEARCH_REQUEST);
    BT_CHECK(hdr.param_len == bt_buf_writer_len(&pw) + 2 + 1);

    BT_CHECK(bt_sdp_parse_element(&r, &elem) == BT_OK);
    BT_CHECK(elem.type == BT_SDP_ELEM_SEQUENCE);

    BT_CHECK(bt_buf_reader_read_be16(&r, &max_count) == BT_OK);
    BT_CHECK(max_count == 10);
    BT_CHECK(bt_buf_reader_read_u8(&r, &cont_len) == BT_OK);
    BT_CHECK(cont_len == 0);
    BT_CHECK(bt_buf_reader_remaining(&r) == 0);
}

static void test_service_search_response_parse(void)
{
    /* total=2, current=2, two handles, no continuation. */
    static const uint8_t params[] = {
        0x00, 0x02, /* total_record_count */
        0x00, 0x02, /* current_record_count */
        0x00, 0x00, 0x00, 0x11, /* handle 1 */
        0x00, 0x00, 0x00, 0x22, /* handle 2 */
        0x00,                   /* continuation length 0 */
    };
    struct bt_sdp_service_search_response rsp;
    struct bt_buf_reader hr;
    uint32_t handle;

    BT_CHECK(bt_sdp_parse_service_search_response(params, sizeof(params), &rsp) == BT_OK);
    BT_CHECK(rsp.total_record_count == 2);
    BT_CHECK(rsp.current_record_count == 2);
    BT_CHECK(rsp.continuation.len == 0);

    bt_buf_reader_init(&hr, rsp.handles, (size_t)rsp.current_record_count * 4);
    BT_CHECK(bt_buf_reader_read_be32(&hr, &handle) == BT_OK && handle == 0x11);
    BT_CHECK(bt_buf_reader_read_be32(&hr, &handle) == BT_OK && handle == 0x22);
}

static void test_service_search_response_with_continuation(void)
{
    static const uint8_t params[] = {
        0x00, 0x05, /* total */
        0x00, 0x01, /* current */
        0x00, 0x00, 0x00, 0x11,
        0x02, 0xBE, 0xEF, /* continuation: len 2, data {0xBE, 0xEF} */
    };
    struct bt_sdp_service_search_response rsp;

    BT_CHECK(bt_sdp_parse_service_search_response(params, sizeof(params), &rsp) == BT_OK);
    BT_CHECK(rsp.continuation.len == 2);
    BT_CHECK(rsp.continuation.data[0] == 0xBE && rsp.continuation.data[1] == 0xEF);
}

static void test_service_search_request_with_continuation(void)
{
    uint8_t pattern[4];
    struct bt_buf_writer pw;
    uint8_t buf[64];
    struct bt_buf_writer w;
    struct bt_sdp_continuation cont;

    bt_buf_writer_init(&pw, pattern, sizeof(pattern));
    bt_sdp_encode_sequence_header(&pw, 0);

    cont.len = 3;
    cont.data[0] = 0x01;
    cont.data[1] = 0x02;
    cont.data[2] = 0x03;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_sdp_encode_service_search_request(&w, 1, pattern, bt_buf_writer_len(&pw), 1,
                                                    &cont) == BT_OK);

    /* Continuation bytes land at the very end. */
    size_t len = bt_buf_writer_len(&w);
    BT_CHECK(buf[len - 4] == 3);
    BT_CHECK(buf[len - 3] == 0x01 && buf[len - 2] == 0x02 && buf[len - 1] == 0x03);
}

static void test_service_attribute_round_trip(void)
{
    uint8_t attr_ids[8];
    struct bt_buf_writer aw;
    uint8_t req[64];
    struct bt_buf_writer w;
    struct bt_buf_reader r;
    struct bt_sdp_header hdr;
    uint32_t handle;
    uint16_t max_bytes;
    struct bt_sdp_element elem;

    /* AttributeIDList: a single range 0x0000-0xFFFF (whole-range wildcard,
     * common in real requests) encoded as one uint32. */
    bt_buf_writer_init(&aw, attr_ids, sizeof(attr_ids));
    bt_sdp_encode_sequence_header(&aw, 5); /* 1 header byte + 4 data bytes */
    bt_sdp_encode_uint(&aw, 0x0000FFFFu, 4);

    bt_buf_writer_init(&w, req, sizeof(req));
    BT_CHECK(bt_sdp_encode_service_attribute_request(&w, 7, 0x00010203, 512, attr_ids,
                                                       bt_buf_writer_len(&aw), NULL) == BT_OK);

    bt_buf_reader_init(&r, req, bt_buf_writer_len(&w));
    BT_CHECK(bt_sdp_parse_header(&r, &hdr) == BT_OK);
    BT_CHECK(hdr.pdu_id == BT_SDP_PDU_SERVICE_ATTRIBUTE_REQUEST);
    BT_CHECK(bt_buf_reader_read_be32(&r, &handle) == BT_OK && handle == 0x00010203);
    BT_CHECK(bt_buf_reader_read_be16(&r, &max_bytes) == BT_OK && max_bytes == 512);
    BT_CHECK(bt_sdp_parse_element(&r, &elem) == BT_OK);
    BT_CHECK(elem.type == BT_SDP_ELEM_SEQUENCE);
}

static void test_service_attribute_response_parse(void)
{
    /* AttributeList = Sequence of [ServiceClassIDList attribute id (0x0001,
     * uint16), value = Sequence containing one UUID16]. */
    uint8_t inner_seq[8];
    struct bt_buf_writer isw;
    uint8_t attr_list[16];
    struct bt_buf_writer alw;
    uint8_t params[32];
    struct bt_buf_writer pw;
    struct bt_sdp_service_attribute_response rsp;
    struct bt_sdp_element_iter it;
    struct bt_sdp_element id_elem;
    struct bt_sdp_element val_elem;
    struct bt_sdp_element_iter val_it;
    struct bt_sdp_element uuid_elem;

    bt_buf_writer_init(&isw, inner_seq, sizeof(inner_seq));
    bt_sdp_encode_uuid16(&isw, 0x1101); /* Serial Port service class, as an example UUID */

    bt_buf_writer_init(&alw, attr_list, sizeof(attr_list));
    bt_sdp_encode_uint(&alw, 0x0001, 2); /* AttributeID: ServiceClassIDList */
    bt_sdp_encode_sequence_header(&alw, (uint16_t)bt_buf_writer_len(&isw));
    bt_buf_writer_write_bytes(&alw, inner_seq, bt_buf_writer_len(&isw));

    bt_buf_writer_init(&pw, params, sizeof(params));
    bt_buf_writer_write_be16(&pw, (uint16_t)bt_buf_writer_len(&alw)); /* AttributeListByteCount */
    bt_sdp_encode_sequence_header(&pw, (uint16_t)bt_buf_writer_len(&alw));
    bt_buf_writer_write_bytes(&pw, attr_list, bt_buf_writer_len(&alw));
    bt_buf_writer_write_u8(&pw, 0); /* no continuation */

    BT_CHECK(bt_sdp_parse_service_attribute_response(params, bt_buf_writer_len(&pw), &rsp) ==
              BT_OK);
    BT_CHECK(rsp.continuation.len == 0);

    bt_sdp_element_iter_init(&it, rsp.attribute_list, rsp.attribute_list_len);
    BT_CHECK(bt_sdp_element_iter_next(&it, &id_elem) == BT_OK);
    BT_CHECK(id_elem.type == BT_SDP_ELEM_UINT && id_elem.uint == 0x0001);
    BT_CHECK(bt_sdp_element_iter_next(&it, &val_elem) == BT_OK);
    BT_CHECK(val_elem.type == BT_SDP_ELEM_SEQUENCE);

    bt_sdp_element_iter_init(&val_it, val_elem.seq_data, val_elem.seq_len);
    BT_CHECK(bt_sdp_element_iter_next(&val_it, &uuid_elem) == BT_OK);
    BT_CHECK(uuid_elem.type == BT_SDP_ELEM_UUID16 && uuid_elem.uuid16 == 0x1101);
}

static void test_continuation_too_long_rejected(void)
{
    uint8_t params[32] = {0};
    params[0] = 0x00;
    params[1] = 0x00; /* total = 0 */
    params[2] = 0x00;
    params[3] = 0x00; /* current = 0 */
    params[4] = 17;   /* continuation length exceeds the 16-byte cap */

    struct bt_sdp_service_search_response rsp;

    BT_CHECK(bt_sdp_parse_service_search_response(params, sizeof(params), &rsp) ==
              BT_ERR_INVALID_ARGUMENT);
}

static void test_parse_text_bool_sint(void)
{
    /* Text "HID" (0x25 0x03 'H' 'I' 'D'), Bool true (0x28 0x01),
       SInt16 -2 (0x11 0xFF 0xFE), UUID32 0x12345678 (0x1A ...) */
    static const uint8_t wire[] = {0x25, 0x03, 'H', 'I', 'D', 0x28, 0x01, 0x11, 0xFF, 0xFE,
                                   0x1A, 0x12, 0x34, 0x56, 0x78};
    struct bt_sdp_element_iter it;
    struct bt_sdp_element el;

    bt_sdp_element_iter_init(&it, wire, sizeof(wire));
    BT_CHECK(bt_sdp_element_iter_next(&it, &el) == BT_OK);
    BT_CHECK(el.type == BT_SDP_ELEM_TEXT && el.seq_len == 3 && el.seq_data[0] == 'H');
    BT_CHECK(bt_sdp_element_iter_next(&it, &el) == BT_OK);
    BT_CHECK(el.type == BT_SDP_ELEM_BOOL && el.uint == 1);
    BT_CHECK(bt_sdp_element_iter_next(&it, &el) == BT_OK);
    BT_CHECK(el.type == BT_SDP_ELEM_SINT && (int32_t)el.uint == -2);
    BT_CHECK(bt_sdp_element_iter_next(&it, &el) == BT_OK);
    BT_CHECK(el.type == BT_SDP_ELEM_UUID32 && el.uint == 0x12345678u);
    BT_CHECK(bt_sdp_element_iter_next(&it, &el) != BT_OK);
}

void run_sdp_tests(void)
{
    test_parse_text_bool_sint();
    test_header_round_trip();
    test_uint_round_trip();
    test_uuid_round_trip();
    test_nested_sequence();
    test_large_sequence_uses_2byte_length();
    test_unsupported_type_rejected();
    test_service_search_request_encode();
    test_service_search_response_parse();
    test_service_search_response_with_continuation();
    test_service_search_request_with_continuation();
    test_service_attribute_round_trip();
    test_service_attribute_response_parse();
    test_continuation_too_long_rejected();
}
