#include <btcore/sdp.h>

#define SDP_TYPE_NIL 0u
#define SDP_TYPE_UINT 1u
#define SDP_TYPE_UUID 3u
#define SDP_TYPE_SINT 2u
#define SDP_TYPE_TEXT 4u
#define SDP_TYPE_BOOL 5u
#define SDP_TYPE_SEQUENCE 6u
#define SDP_TYPE_ALTERNATIVE 7u
#define SDP_TYPE_URL 8u

bt_status_t bt_sdp_encode_header(struct bt_buf_writer *w, uint8_t pdu_id, uint16_t transaction_id,
                                  uint16_t param_len)
{
    bt_status_t st;

    st = bt_buf_writer_write_u8(w, pdu_id);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_be16(w, transaction_id);
    if (st != BT_OK)
        return st;

    return bt_buf_writer_write_be16(w, param_len);
}

bt_status_t bt_sdp_parse_header(struct bt_buf_reader *r, struct bt_sdp_header *out)
{
    bt_status_t st;

    st = bt_buf_reader_read_u8(r, &out->pdu_id);
    if (st != BT_OK)
        return st;
    st = bt_buf_reader_read_be16(r, &out->transaction_id);
    if (st != BT_OK)
        return st;

    return bt_buf_reader_read_be16(r, &out->param_len);
}

bt_status_t bt_sdp_encode_uint(struct bt_buf_writer *w, uint32_t value, uint8_t width)
{
    uint8_t size_desc;
    uint8_t header;
    bt_status_t st;

    switch (width)
    {
    case 1:
        size_desc = 0;
        break;
    case 2:
        size_desc = 1;
        break;
    case 4:
        size_desc = 2;
        break;
    default:
        return BT_ERR_INVALID_ARGUMENT;
    }

    header = (uint8_t)((SDP_TYPE_UINT << 3) | size_desc);
    st = bt_buf_writer_write_u8(w, header);
    if (st != BT_OK)
        return st;

    if (width == 1)
        return bt_buf_writer_write_u8(w, (uint8_t)value);
    if (width == 2)
        return bt_buf_writer_write_be16(w, (uint16_t)value);
    return bt_buf_writer_write_be32(w, value);
}

bt_status_t bt_sdp_encode_uuid16(struct bt_buf_writer *w, uint16_t uuid)
{
    uint8_t header = (uint8_t)((SDP_TYPE_UUID << 3) | 1); /* size_desc 1 -> 2 bytes */
    bt_status_t st;

    st = bt_buf_writer_write_u8(w, header);
    if (st != BT_OK)
        return st;

    return bt_buf_writer_write_be16(w, uuid);
}

bt_status_t bt_sdp_encode_uuid128(struct bt_buf_writer *w, const uint8_t uuid[16])
{
    uint8_t header = (uint8_t)((SDP_TYPE_UUID << 3) | 4); /* size_desc 4 -> 16 bytes */
    bt_status_t st;

    st = bt_buf_writer_write_u8(w, header);
    if (st != BT_OK)
        return st;

    return bt_buf_writer_write_bytes(w, uuid, 16);
}

bt_status_t bt_sdp_encode_sequence_header(struct bt_buf_writer *w, uint16_t nested_len)
{
    bt_status_t st;

    if (nested_len <= 0xFFu)
    {
        st = bt_buf_writer_write_u8(w, (uint8_t)((SDP_TYPE_SEQUENCE << 3) | 5));
        if (st != BT_OK)
            return st;
        return bt_buf_writer_write_u8(w, (uint8_t)nested_len);
    }

    st = bt_buf_writer_write_u8(w, (uint8_t)((SDP_TYPE_SEQUENCE << 3) | 6));
    if (st != BT_OK)
        return st;
    return bt_buf_writer_write_be16(w, nested_len);
}

bt_status_t bt_sdp_parse_element(struct bt_buf_reader *r, struct bt_sdp_element *out)
{
    uint8_t header;
    uint8_t type;
    uint8_t size_desc;
    bt_status_t st;

    st = bt_buf_reader_read_u8(r, &header);
    if (st != BT_OK)
        return st;

    type = (uint8_t)(header >> 3);
    size_desc = (uint8_t)(header & 0x07u);

    switch (type)
    {
    case SDP_TYPE_NIL:
        if (size_desc != 0)
            return BT_ERR_INVALID_ARGUMENT;
        out->type = BT_SDP_ELEM_NIL;
        return BT_OK;

    case SDP_TYPE_UINT:
    {
        uint8_t width;

        switch (size_desc)
        {
        case 0:
            width = 1;
            break;
        case 1:
            width = 2;
            break;
        case 2:
            width = 4;
            break;
        default:
            {
                size_t n = (size_desc == 3) ? 8u : (size_desc == 4) ? 16u : 0u;
                const uint8_t *p;
                if (n == 0)
                    return BT_ERR_INVALID_ARGUMENT;
                p = bt_buf_reader_peek(r, n);
                if (p == NULL)
                    return BT_ERR_BUFFER_UNDERFLOW;
                bt_buf_reader_skip(r, n);
                out->type = BT_SDP_ELEM_OTHER;
                out->seq_data = p;
                out->seq_len = n;
                return BT_OK;
            }
        }

        out->type = BT_SDP_ELEM_UINT;
        out->width = width;

        if (width == 1)
        {
            uint8_t v;

            st = bt_buf_reader_read_u8(r, &v);
            if (st != BT_OK)
                return st;
            out->uint = v;
        }
        else if (width == 2)
        {
            uint16_t v;

            st = bt_buf_reader_read_be16(r, &v);
            if (st != BT_OK)
                return st;
            out->uint = v;
        }
        else
        {
            uint32_t v;

            st = bt_buf_reader_read_be32(r, &v);
            if (st != BT_OK)
                return st;
            out->uint = v;
        }
        return BT_OK;
    }

    case SDP_TYPE_UUID:
        if (size_desc == 1)
        {
            st = bt_buf_reader_read_be16(r, &out->uuid16);
            if (st != BT_OK)
                return st;
            out->type = BT_SDP_ELEM_UUID16;
            return BT_OK;
        }
        if (size_desc == 4)
        {
            st = bt_buf_reader_read_bytes(r, out->uuid128, 16);
            if (st != BT_OK)
                return st;
            out->type = BT_SDP_ELEM_UUID128;
            return BT_OK;
        }
        if (size_desc == 2)
        {
            uint32_t v;
            st = bt_buf_reader_read_be32(r, &v);
            if (st != BT_OK)
                return st;
            out->type = BT_SDP_ELEM_UUID32;
            out->uint = v;
            return BT_OK;
        }
        return BT_ERR_INVALID_ARGUMENT;

    case SDP_TYPE_SEQUENCE:
    case SDP_TYPE_ALTERNATIVE:
    case SDP_TYPE_TEXT:
    case SDP_TYPE_URL:
    {
        size_t nested_len;
        const uint8_t *p;

        if (size_desc == 5)
        {
            uint8_t l;

            st = bt_buf_reader_read_u8(r, &l);
            if (st != BT_OK)
                return st;
            nested_len = l;
        }
        else if (size_desc == 6)
        {
            uint16_t l;

            st = bt_buf_reader_read_be16(r, &l);
            if (st != BT_OK)
                return st;
            nested_len = l;
        }
        else if (size_desc == 7)
        {
            uint32_t l;

            st = bt_buf_reader_read_be32(r, &l);
            if (st != BT_OK)
                return st;
            nested_len = l;
        }
        else
        {
            return BT_ERR_INVALID_ARGUMENT;
        }

        p = bt_buf_reader_peek(r, nested_len);
        if (p == NULL)
            return BT_ERR_BUFFER_UNDERFLOW;
        st = bt_buf_reader_skip(r, nested_len);
        if (st != BT_OK)
            return st;

        out->type = (type == SDP_TYPE_SEQUENCE) ? BT_SDP_ELEM_SEQUENCE :
                    (type == SDP_TYPE_ALTERNATIVE) ? BT_SDP_ELEM_ALTERNATIVE :
                    (type == SDP_TYPE_TEXT) ? BT_SDP_ELEM_TEXT : BT_SDP_ELEM_URL;
        out->seq_data = p;
        out->seq_len = nested_len;
        return BT_OK;
    }

    case SDP_TYPE_BOOL:
    {
        uint8_t v;

        if (size_desc != 0)
            return BT_ERR_INVALID_ARGUMENT;
        st = bt_buf_reader_read_u8(r, &v);
        if (st != BT_OK)
            return st;
        out->type = BT_SDP_ELEM_BOOL;
        out->width = 1;
        out->uint = v ? 1u : 0u;
        return BT_OK;
    }

    case SDP_TYPE_SINT:
    {
        /* same layout as UINT; sign is the caller's business */
        uint8_t width;

        switch (size_desc)
        {
        case 0: width = 1; break;
        case 1: width = 2; break;
        case 2: width = 4; break;
        default:
            {
                size_t n = (size_desc == 3) ? 8u : 16u;
                const uint8_t *p = bt_buf_reader_peek(r, n);
                if (p == NULL)
                    return BT_ERR_BUFFER_UNDERFLOW;
                bt_buf_reader_skip(r, n);
                out->type = BT_SDP_ELEM_OTHER;
                out->seq_data = p;
                out->seq_len = n;
                return BT_OK;
            }
        }
        out->type = BT_SDP_ELEM_SINT;
        out->width = width;
        if (width == 1)
        {
            uint8_t v;
            st = bt_buf_reader_read_u8(r, &v);
            if (st != BT_OK)
                return st;
            out->uint = (uint32_t)(int32_t)(int8_t)v;
        }
        else if (width == 2)
        {
            uint16_t v;
            st = bt_buf_reader_read_be16(r, &v);
            if (st != BT_OK)
                return st;
            out->uint = (uint32_t)(int32_t)(int16_t)v;
        }
        else
        {
            uint32_t v;
            st = bt_buf_reader_read_be32(r, &v);
            if (st != BT_OK)
                return st;
            out->uint = v;
        }
        return BT_OK;
    }

    default:
        return BT_ERR_INVALID_ARGUMENT;
    }
}

void bt_sdp_element_iter_init(struct bt_sdp_element_iter *it, const uint8_t *data, size_t len)
{
    bt_buf_reader_init(&it->r, data, len);
}

bt_status_t bt_sdp_element_iter_next(struct bt_sdp_element_iter *it, struct bt_sdp_element *out)
{
    if (bt_buf_reader_remaining(&it->r) == 0)
        return BT_ERR_BUFFER_UNDERFLOW;

    return bt_sdp_parse_element(&it->r, out);
}

static bt_status_t write_continuation(struct bt_buf_writer *w, const struct bt_sdp_continuation *cont)
{
    bt_status_t st;

    if (cont == NULL || cont->len == 0)
        return bt_buf_writer_write_u8(w, 0);

    st = bt_buf_writer_write_u8(w, cont->len);
    if (st != BT_OK)
        return st;

    return bt_buf_writer_write_bytes(w, cont->data, cont->len);
}

static bt_status_t read_continuation(struct bt_buf_reader *r, struct bt_sdp_continuation *out)
{
    uint8_t len;
    bt_status_t st;

    st = bt_buf_reader_read_u8(r, &len);
    if (st != BT_OK)
        return st;
    if (len > sizeof(out->data))
        return BT_ERR_INVALID_ARGUMENT;

    out->len = len;
    if (len == 0)
        return BT_OK;

    return bt_buf_reader_read_bytes(r, out->data, len);
}

bt_status_t bt_sdp_encode_service_search_request(struct bt_buf_writer *w, uint16_t transaction_id,
                                                   const uint8_t *service_search_pattern,
                                                   size_t pattern_len, uint16_t max_record_count,
                                                   const struct bt_sdp_continuation *cont)
{
    uint16_t cont_len = (uint16_t)(cont != NULL ? cont->len : 0);
    uint16_t param_len = (uint16_t)(pattern_len + 2 + 1 + cont_len);
    bt_status_t st;

    st = bt_sdp_encode_header(w, BT_SDP_PDU_SERVICE_SEARCH_REQUEST, transaction_id, param_len);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_bytes(w, service_search_pattern, pattern_len);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_be16(w, max_record_count);
    if (st != BT_OK)
        return st;

    return write_continuation(w, cont);
}

bt_status_t bt_sdp_parse_service_search_response(const uint8_t *params, size_t params_len,
                                                   struct bt_sdp_service_search_response *out)
{
    struct bt_buf_reader r;
    bt_status_t st;
    size_t handles_len;

    bt_buf_reader_init(&r, params, params_len);
    st = bt_buf_reader_read_be16(&r, &out->total_record_count);
    if (st != BT_OK)
        return st;
    st = bt_buf_reader_read_be16(&r, &out->current_record_count);
    if (st != BT_OK)
        return st;

    handles_len = (size_t)out->current_record_count * 4;
    out->handles = bt_buf_reader_peek(&r, handles_len);
    if (out->handles == NULL)
        return BT_ERR_BUFFER_UNDERFLOW;
    st = bt_buf_reader_skip(&r, handles_len);
    if (st != BT_OK)
        return st;

    return read_continuation(&r, &out->continuation);
}

bt_status_t bt_sdp_encode_service_attribute_request(struct bt_buf_writer *w,
                                                      uint16_t transaction_id,
                                                      uint32_t service_record_handle,
                                                      uint16_t max_attribute_byte_count,
                                                      const uint8_t *attribute_id_list,
                                                      size_t attribute_id_list_len,
                                                      const struct bt_sdp_continuation *cont)
{
    uint16_t cont_len = (uint16_t)(cont != NULL ? cont->len : 0);
    uint16_t param_len = (uint16_t)(4 + 2 + attribute_id_list_len + 1 + cont_len);
    bt_status_t st;

    st = bt_sdp_encode_header(w, BT_SDP_PDU_SERVICE_ATTRIBUTE_REQUEST, transaction_id, param_len);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_be32(w, service_record_handle);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_be16(w, max_attribute_byte_count);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_bytes(w, attribute_id_list, attribute_id_list_len);
    if (st != BT_OK)
        return st;

    return write_continuation(w, cont);
}

bt_status_t bt_sdp_parse_service_attribute_response(const uint8_t *params, size_t params_len,
                                                      struct bt_sdp_service_attribute_response *out)
{
    struct bt_buf_reader r;
    bt_status_t st;
    uint16_t byte_count;
    struct bt_sdp_element elem;

    bt_buf_reader_init(&r, params, params_len);
    st = bt_buf_reader_read_be16(&r, &byte_count);
    if (st != BT_OK)
        return st;

    /* AttributeList is itself one Data Element (a Sequence). */
    st = bt_sdp_parse_element(&r, &elem);
    if (st != BT_OK)
        return st;
    if (elem.type != BT_SDP_ELEM_SEQUENCE)
        return BT_ERR_INVALID_ARGUMENT;

    out->attribute_list = elem.seq_data;
    out->attribute_list_len = elem.seq_len;

    (void)byte_count; /* redundant with elem.seq_len in a well-formed response */
    return read_continuation(&r, &out->continuation);
}
