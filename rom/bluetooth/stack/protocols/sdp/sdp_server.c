#include <btcore/sdp_server.h>
#include <btcore/sdp.h>
#include <btcore/buffer.h>

#include <string.h>

#define PDU_SEARCH_ATTR_REQ  0x06u
#define PDU_SEARCH_ATTR_RSP  0x07u

#define ERR_INVALID_HANDLE       0x0002u
#define ERR_INVALID_SYNTAX       0x0003u
#define ERR_INVALID_CONTINUATION 0x0005u

static const uint8_t base_uuid[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
    0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb
};

static void uuid_to_128(const struct bt_sdp_element *e, uint8_t out[16])
{
    memcpy(out, base_uuid, 16);
    if (e->type == BT_SDP_ELEM_UUID16)
    {
        out[2] = (uint8_t)(e->uuid16 >> 8);
        out[3] = (uint8_t)e->uuid16;
    }
    else if (e->type == BT_SDP_ELEM_UUID32)
    {
        out[0] = (uint8_t)(e->uint >> 24);
        out[1] = (uint8_t)(e->uint >> 16);
        out[2] = (uint8_t)(e->uint >> 8);
        out[3] = (uint8_t)e->uint;
    }
    else
        memcpy(out, e->uuid128, 16);
}

static bool is_uuid(const struct bt_sdp_element *e)
{
    return e->type == BT_SDP_ELEM_UUID16 || e->type == BT_SDP_ELEM_UUID32 ||
           e->type == BT_SDP_ELEM_UUID128;
}

/* any element (nested sequences included) equal to the UUID? */
static bool contains_uuid(const uint8_t *data, size_t len, const uint8_t uuid[16], int depth)
{
    struct bt_sdp_element_iter it;
    struct bt_sdp_element e;

    if (depth > 8)
        return false;
    bt_sdp_element_iter_init(&it, data, len);
    while (bt_sdp_element_iter_next(&it, &e) == BT_OK)
    {
        if (is_uuid(&e))
        {
            uint8_t u[16];
            uuid_to_128(&e, u);
            if (memcmp(u, uuid, 16) == 0)
                return true;
        }
        else if ((e.type == BT_SDP_ELEM_SEQUENCE || e.type == BT_SDP_ELEM_ALTERNATIVE) &&
                 contains_uuid(e.seq_data, e.seq_len, uuid, depth + 1))
            return true;
    }
    return false;
}

bool bt_sdp_record_matches(const struct bt_sdp_record *rec, const uint8_t *pattern, size_t pattern_len)
{
    struct bt_sdp_element_iter it;
    struct bt_sdp_element e;
    size_t n = 0;

    bt_sdp_element_iter_init(&it, pattern, pattern_len);
    while (bt_sdp_element_iter_next(&it, &e) == BT_OK)
    {
        uint8_t u[16];
        if (!is_uuid(&e))
            return false;
        uuid_to_128(&e, u);
        if (!contains_uuid(rec->attrs, rec->attrs_len, u, 0))
            return false;
        n++;
    }
    return n > 0;
}

/* is the attribute id selected by the request's id list (ids and ranges)? */
static bool attr_selected(uint16_t id, const uint8_t *idlist, size_t idlist_len)
{
    struct bt_sdp_element_iter it;
    struct bt_sdp_element e;

    bt_sdp_element_iter_init(&it, idlist, idlist_len);
    while (bt_sdp_element_iter_next(&it, &e) == BT_OK)
    {
        if (e.type != BT_SDP_ELEM_UINT)
            continue;
        if (e.width == 2 && e.uint == id)
            return true;
        if (e.width == 4 && id >= (e.uint >> 16) && id <= (e.uint & 0xffffu))
            return true;
    }
    return false;
}

/* append the record's selected (id, value) pairs, raw, to w */
static bt_status_t append_attributes(struct bt_buf_writer *w, const struct bt_sdp_record *rec,
                                     const uint8_t *idlist, size_t idlist_len)
{
    struct bt_sdp_element_iter it;
    struct bt_sdp_element id, val;

    bt_sdp_element_iter_init(&it, rec->attrs, rec->attrs_len);
    for (;;)
    {
        size_t start = it.r.pos;
        if (bt_sdp_element_iter_next(&it, &id) != BT_OK)
            break;
        if (bt_sdp_element_iter_next(&it, &val) != BT_OK)
            break;
        if (id.type != BT_SDP_ELEM_UINT || !attr_selected((uint16_t)id.uint, idlist, idlist_len))
            continue;
        if (bt_buf_writer_write_bytes(w, rec->attrs + start, it.r.pos - start) != BT_OK)
            return BT_ERR_NO_RESOURCES;
    }
    return BT_OK;
}

static size_t error_response(uint16_t tid, uint16_t code, uint8_t *rsp, size_t rsp_max)
{
    struct bt_buf_writer w;
    bt_buf_writer_init(&w, rsp, rsp_max);
    if (bt_sdp_encode_header(&w, BT_SDP_PDU_ERROR_RESPONSE, tid, 2) != BT_OK ||
        bt_buf_writer_write_be16(&w, code) != BT_OK)
        return 0;
    return bt_buf_writer_len(&w);
}

/* a Data Element Sequence header for a nested length, as the DES wrapper
 * of an AttributeList / AttributeLists */
static size_t seq_header_len(size_t nested)
{
    return nested > 0xffff ? 5 : nested > 0xff ? 3 : 2;
}

/* Take the next slice of srv->body (from body_pos) that fits: writes
 * [count(2)] [slice] [continuation] after the header. For SEARCH the body
 * is the handle list and the leading fields are the two record counts. */
static size_t emit_body(struct bt_sdp_server *srv, uint16_t tid, uint16_t max_bytes,
                        uint8_t *rsp, size_t rsp_max)
{
    struct bt_buf_writer w;
    size_t fixed = (srv->body_pdu == BT_SDP_PDU_SERVICE_SEARCH_RESPONSE) ? 4 : 2;
    size_t room;
    size_t slice;
    size_t remaining = srv->body_len - srv->body_pos;
    bool more;

    if (rsp_max < BT_SDP_HEADER_LEN + fixed + 3 + 4)
        return 0;
    room = rsp_max - BT_SDP_HEADER_LEN - fixed - 3;   /* continuation up to 3 bytes */
    if (max_bytes && max_bytes < room)
        room = max_bytes;
    if (srv->body_pdu == BT_SDP_PDU_SERVICE_SEARCH_RESPONSE)
        room -= room % 4;                              /* whole handles */
    slice = remaining < room ? remaining : room;
    more = slice < remaining;

    bt_buf_writer_init(&w, rsp, rsp_max);
    if (bt_sdp_encode_header(&w, srv->body_pdu, tid,
                             (uint16_t)(fixed + slice + 1 + (more ? 2 : 0))) != BT_OK)
        return 0;
    if (srv->body_pdu == BT_SDP_PDU_SERVICE_SEARCH_RESPONSE)
    {
        bt_buf_writer_write_be16(&w, srv->body_extra);
        bt_buf_writer_write_be16(&w, (uint16_t)(slice / 4));
    }
    else
        bt_buf_writer_write_be16(&w, (uint16_t)slice);
    bt_buf_writer_write_bytes(&w, srv->body + srv->body_pos, slice);
    srv->body_pos += slice;
    if (more)
    {
        bt_buf_writer_write_u8(&w, 2);
        bt_buf_writer_write_be16(&w, (uint16_t)srv->body_pos);
    }
    else
    {
        bt_buf_writer_write_u8(&w, 0);
        srv->body_len = srv->body_pos = 0;
        srv->body_pdu = 0;
    }
    return bt_buf_writer_len(&w);
}

/* the request's ContinuationState (after the reader's position): 0 = new
 * request, else the offset it continues from; false = malformed */
static bool read_continuation(struct bt_buf_reader *r, uint16_t *offset)
{
    uint8_t len;
    if (bt_buf_reader_read_u8(r, &len) != BT_OK)
        return false;
    *offset = 0;
    if (len == 0)
        return true;
    if (len != 2 || bt_buf_reader_read_be16(r, offset) != BT_OK)
        return false;
    return true;
}

void bt_sdp_server_init(struct bt_sdp_server *srv, bt_sdp_record_at_fn record_at, void *context)
{
    memset(srv, 0, sizeof(*srv));
    srv->record_at = record_at;
    srv->context = context;
}

size_t bt_sdp_server_handle(struct bt_sdp_server *srv, const uint8_t *req, size_t req_len,
                            uint8_t *rsp, size_t rsp_max)
{
    struct bt_buf_reader r;
    struct bt_sdp_header hdr;
    struct bt_sdp_element pattern, idlist;
    uint16_t max_bytes = 0;
    uint16_t cont = 0;
    uint32_t handle = 0;
    uint8_t rsp_pdu;
    struct bt_buf_writer body;
    size_t i;

    bt_buf_reader_init(&r, req, req_len);
    if (bt_sdp_parse_header(&r, &hdr) != BT_OK)
        return 0;
    if (hdr.pdu_id != BT_SDP_PDU_SERVICE_SEARCH_REQUEST &&
        hdr.pdu_id != BT_SDP_PDU_SERVICE_ATTRIBUTE_REQUEST &&
        hdr.pdu_id != PDU_SEARCH_ATTR_REQ)
        return error_response(hdr.transaction_id, ERR_INVALID_SYNTAX, rsp, rsp_max);

    memset(&pattern, 0, sizeof(pattern));
    memset(&idlist, 0, sizeof(idlist));
    switch (hdr.pdu_id)
    {
    case BT_SDP_PDU_SERVICE_SEARCH_REQUEST:
        rsp_pdu = BT_SDP_PDU_SERVICE_SEARCH_RESPONSE;
        if (bt_sdp_parse_element(&r, &pattern) != BT_OK || pattern.type != BT_SDP_ELEM_SEQUENCE ||
            bt_buf_reader_read_be16(&r, &max_bytes) != BT_OK)
            goto syntax;
        break;
    case BT_SDP_PDU_SERVICE_ATTRIBUTE_REQUEST:
        rsp_pdu = BT_SDP_PDU_SERVICE_ATTRIBUTE_RESPONSE;
        if (bt_buf_reader_read_be32(&r, &handle) != BT_OK ||
            bt_buf_reader_read_be16(&r, &max_bytes) != BT_OK ||
            bt_sdp_parse_element(&r, &idlist) != BT_OK || idlist.type != BT_SDP_ELEM_SEQUENCE)
            goto syntax;
        break;
    default:
        rsp_pdu = PDU_SEARCH_ATTR_RSP;
        if (bt_sdp_parse_element(&r, &pattern) != BT_OK || pattern.type != BT_SDP_ELEM_SEQUENCE ||
            bt_buf_reader_read_be16(&r, &max_bytes) != BT_OK ||
            bt_sdp_parse_element(&r, &idlist) != BT_OK || idlist.type != BT_SDP_ELEM_SEQUENCE)
            goto syntax;
        break;
    }
    if (!read_continuation(&r, &cont))
        goto syntax;

    if (cont)
    {
        /* continuing the response in progress */
        if (srv->body_pdu != rsp_pdu || cont != srv->body_pos || cont > srv->body_len)
        {
            srv->body_len = srv->body_pos = 0;
            srv->body_pdu = 0;
            return error_response(hdr.transaction_id, ERR_INVALID_CONTINUATION, rsp, rsp_max);
        }
        return emit_body(srv, hdr.transaction_id,
                         (rsp_pdu == BT_SDP_PDU_SERVICE_SEARCH_RESPONSE) ? 0 : max_bytes, rsp, rsp_max);
    }

    /* build the complete body */
    bt_buf_writer_init(&body, srv->body, sizeof(srv->body));
    srv->body_extra = 0;
    if (rsp_pdu == BT_SDP_PDU_SERVICE_SEARCH_RESPONSE)
    {
        uint16_t total = 0;
        for (i = 0; ; i++)
        {
            const struct bt_sdp_record *rec = srv->record_at(srv->context, i);
            if (!rec)
                break;
            if (!bt_sdp_record_matches(rec, pattern.seq_data, pattern.seq_len))
                continue;
            total++;
            if (total <= max_bytes /* max record count here */ &&
                bt_buf_writer_write_be32(&body, rec->handle) != BT_OK)
                break;
        }
        srv->body_extra = total < max_bytes ? total : max_bytes;
    }
    else if (rsp_pdu == BT_SDP_PDU_SERVICE_ATTRIBUTE_RESPONSE)
    {
        const struct bt_sdp_record *rec = NULL;
        uint8_t tmp[BT_SDP_SERVER_MAX_BODY];
        struct bt_buf_writer tw;
        for (i = 0; ; i++)
        {
            rec = srv->record_at(srv->context, i);
            if (!rec || rec->handle == handle)
                break;
        }
        if (!rec)
            return error_response(hdr.transaction_id, ERR_INVALID_HANDLE, rsp, rsp_max);
        bt_buf_writer_init(&tw, tmp, sizeof(tmp));
        if (append_attributes(&tw, rec, idlist.seq_data, idlist.seq_len) != BT_OK)
            goto syntax;
        bt_sdp_encode_sequence_header(&body, (uint16_t)bt_buf_writer_len(&tw));
        bt_buf_writer_write_bytes(&body, tmp, bt_buf_writer_len(&tw));
    }
    else
    {
        /* ServiceSearchAttribute: a sequence of AttributeLists */
        uint8_t tmp[BT_SDP_SERVER_MAX_BODY];
        struct bt_buf_writer tw;
        bt_buf_writer_init(&tw, tmp, sizeof(tmp));
        for (i = 0; ; i++)
        {
            const struct bt_sdp_record *rec = srv->record_at(srv->context, i);
            uint8_t one[BT_SDP_SERVER_MAX_BODY];
            struct bt_buf_writer ow;
            if (!rec)
                break;
            if (!bt_sdp_record_matches(rec, pattern.seq_data, pattern.seq_len))
                continue;
            bt_buf_writer_init(&ow, one, sizeof(one));
            if (append_attributes(&ow, rec, idlist.seq_data, idlist.seq_len) != BT_OK)
                goto syntax;
            if (bt_buf_writer_len(&ow) == 0)
                continue;
            if (bt_buf_writer_len(&tw) + seq_header_len(bt_buf_writer_len(&ow)) +
                bt_buf_writer_len(&ow) > sizeof(tmp))
                break;                                 /* what fits */
            bt_sdp_encode_sequence_header(&tw, (uint16_t)bt_buf_writer_len(&ow));
            bt_buf_writer_write_bytes(&tw, one, bt_buf_writer_len(&ow));
        }
        bt_sdp_encode_sequence_header(&body, (uint16_t)bt_buf_writer_len(&tw));
        bt_buf_writer_write_bytes(&body, tmp, bt_buf_writer_len(&tw));
    }
    srv->body_len = bt_buf_writer_len(&body);
    srv->body_pos = 0;
    srv->body_pdu = rsp_pdu;
    return emit_body(srv, hdr.transaction_id,
                     (rsp_pdu == BT_SDP_PDU_SERVICE_SEARCH_RESPONSE) ? 0 : max_bytes, rsp, rsp_max);

syntax:
    return error_response(hdr.transaction_id, ERR_INVALID_SYNTAX, rsp, rsp_max);
}
