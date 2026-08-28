#include <btcore/att.h>

bt_status_t bt_att_parse_error_response(const uint8_t *params, size_t params_len,
                                         struct bt_att_error_response *out)
{
    struct bt_buf_reader r;
    bt_status_t st;

    if (params_len != 4)
        return BT_ERR_INVALID_ARGUMENT;

    bt_buf_reader_init(&r, params, params_len);
    st = bt_buf_reader_read_u8(&r, &out->request_opcode);
    if (st != BT_OK)
        return st;
    st = bt_buf_reader_read_le16(&r, &out->handle_in_error);
    if (st != BT_OK)
        return st;

    return bt_buf_reader_read_u8(&r, &out->error_code);
}

bt_status_t bt_att_encode_exchange_mtu_request(struct bt_buf_writer *w, uint16_t client_rx_mtu)
{
    bt_status_t st;

    st = bt_buf_writer_write_u8(w, BT_ATT_OPCODE_EXCHANGE_MTU_REQUEST);
    if (st != BT_OK)
        return st;

    return bt_buf_writer_write_le16(w, client_rx_mtu);
}

bt_status_t bt_att_parse_exchange_mtu_response(const uint8_t *params, size_t params_len,
                                                uint16_t *out_server_rx_mtu)
{
    struct bt_buf_reader r;

    if (params_len != 2)
        return BT_ERR_INVALID_ARGUMENT;

    bt_buf_reader_init(&r, params, params_len);
    return bt_buf_reader_read_le16(&r, out_server_rx_mtu);
}

bt_status_t bt_att_encode_find_information_request(struct bt_buf_writer *w,
                                                    uint16_t starting_handle,
                                                    uint16_t ending_handle)
{
    bt_status_t st;

    st = bt_buf_writer_write_u8(w, BT_ATT_OPCODE_FIND_INFORMATION_REQUEST);
    if (st == BT_OK)
        st = bt_buf_writer_write_le16(w, starting_handle);
    return st == BT_OK ? bt_buf_writer_write_le16(w, ending_handle) : st;
}

bt_status_t bt_att_find_information_response_iter_init(
    struct bt_att_find_information_iter *it, const uint8_t *params, size_t params_len)
{
    uint8_t format;

    if (it == NULL || params == NULL || params_len < 1)
        return BT_ERR_INVALID_ARGUMENT;
    bt_buf_reader_init(&it->r, params, params_len);
    if (bt_buf_reader_read_u8(&it->r, &format) != BT_OK || (format != 1 && format != 2))
        return BT_ERR_INVALID_ARGUMENT;
    it->entry_len = format == 1 ? 4 : 18;
    if (bt_buf_reader_remaining(&it->r) == 0 ||
        bt_buf_reader_remaining(&it->r) % it->entry_len != 0)
        return BT_ERR_INVALID_ARGUMENT;
    return BT_OK;
}

bt_status_t bt_att_find_information_response_iter_next(
    struct bt_att_find_information_iter *it, struct bt_att_information_entry *out)
{
    if (it == NULL || out == NULL)
        return BT_ERR_INVALID_ARGUMENT;
    if (bt_buf_reader_remaining(&it->r) == 0)
        return BT_ERR_BUFFER_UNDERFLOW;
    if (bt_buf_reader_read_le16(&it->r, &out->handle) != BT_OK)
        return BT_ERR_BUFFER_UNDERFLOW;
    out->uuid_len = (uint8_t)(it->entry_len - 2);
    out->uuid = bt_buf_reader_peek(&it->r, out->uuid_len);
    return out->uuid == NULL ? BT_ERR_BUFFER_UNDERFLOW
                             : bt_buf_reader_skip(&it->r, out->uuid_len);
}

bt_status_t bt_att_encode_read_by_group_type_request(struct bt_buf_writer *w,
                                                       uint16_t starting_handle,
                                                       uint16_t ending_handle,
                                                       uint16_t group_type_uuid16)
{
    bt_status_t st;

    st = bt_buf_writer_write_u8(w, BT_ATT_OPCODE_READ_BY_GROUP_TYPE_REQUEST);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_le16(w, starting_handle);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_le16(w, ending_handle);
    if (st != BT_OK)
        return st;

    return bt_buf_writer_write_le16(w, group_type_uuid16);
}

bt_status_t bt_att_read_by_group_type_response_iter_init(struct bt_att_group_type_iter *it,
                                                           const uint8_t *params, size_t params_len)
{
    uint8_t entry_len;
    bt_status_t st;

    bt_buf_reader_init(&it->r, params, params_len);
    st = bt_buf_reader_read_u8(&it->r, &entry_len);
    if (st != BT_OK)
        return st;
    if (entry_len < 4)
        return BT_ERR_INVALID_ARGUMENT;

    it->entry_len = entry_len;
    return BT_OK;
}

bt_status_t bt_att_read_by_group_type_response_iter_next(struct bt_att_group_type_iter *it,
                                                           struct bt_att_group_entry *out)
{
    bt_status_t st;
    const uint8_t *value;

    if (bt_buf_reader_remaining(&it->r) == 0)
        return BT_ERR_BUFFER_UNDERFLOW;
    if (bt_buf_reader_remaining(&it->r) < it->entry_len)
        return BT_ERR_BUFFER_UNDERFLOW; /* truncated entry */

    st = bt_buf_reader_read_le16(&it->r, &out->handle);
    if (st != BT_OK)
        return st;
    st = bt_buf_reader_read_le16(&it->r, &out->end_group_handle);
    if (st != BT_OK)
        return st;

    out->value_len = (uint8_t)(it->entry_len - 4);
    value = bt_buf_reader_peek(&it->r, out->value_len);
    if (value == NULL)
        return BT_ERR_BUFFER_UNDERFLOW;
    st = bt_buf_reader_skip(&it->r, out->value_len);
    if (st != BT_OK)
        return st;

    out->value = value;
    return BT_OK;
}

bt_status_t bt_att_encode_read_by_type_request(struct bt_buf_writer *w, uint16_t starting_handle,
                                                uint16_t ending_handle,
                                                uint16_t attribute_type_uuid16)
{
    bt_status_t st;

    st = bt_buf_writer_write_u8(w, BT_ATT_OPCODE_READ_BY_TYPE_REQUEST);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_le16(w, starting_handle);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_le16(w, ending_handle);
    if (st != BT_OK)
        return st;

    return bt_buf_writer_write_le16(w, attribute_type_uuid16);
}

bt_status_t bt_att_read_by_type_response_iter_init(struct bt_att_read_by_type_iter *it,
                                                     const uint8_t *params, size_t params_len)
{
    uint8_t entry_len;
    bt_status_t st;

    bt_buf_reader_init(&it->r, params, params_len);
    st = bt_buf_reader_read_u8(&it->r, &entry_len);
    if (st != BT_OK)
        return st;
    if (entry_len < 2)
        return BT_ERR_INVALID_ARGUMENT;

    it->entry_len = entry_len;
    return BT_OK;
}

bt_status_t bt_att_read_by_type_response_iter_next(struct bt_att_read_by_type_iter *it,
                                                     struct bt_att_type_entry *out)
{
    bt_status_t st;
    const uint8_t *value;

    if (bt_buf_reader_remaining(&it->r) == 0)
        return BT_ERR_BUFFER_UNDERFLOW;
    if (bt_buf_reader_remaining(&it->r) < it->entry_len)
        return BT_ERR_BUFFER_UNDERFLOW;

    st = bt_buf_reader_read_le16(&it->r, &out->handle);
    if (st != BT_OK)
        return st;

    out->value_len = (uint8_t)(it->entry_len - 2);
    value = bt_buf_reader_peek(&it->r, out->value_len);
    if (value == NULL)
        return BT_ERR_BUFFER_UNDERFLOW;
    st = bt_buf_reader_skip(&it->r, out->value_len);
    if (st != BT_OK)
        return st;

    out->value = value;
    return BT_OK;
}

bt_status_t bt_att_encode_read_request(struct bt_buf_writer *w, uint16_t handle)
{
    bt_status_t st;

    st = bt_buf_writer_write_u8(w, BT_ATT_OPCODE_READ_REQUEST);
    if (st != BT_OK)
        return st;

    return bt_buf_writer_write_le16(w, handle);
}

bt_status_t bt_att_encode_read_blob_request(struct bt_buf_writer *w, uint16_t handle,
                                             uint16_t value_offset)
{
    bt_status_t st;

    st = bt_buf_writer_write_u8(w, BT_ATT_OPCODE_READ_BLOB_REQUEST);
    if (st == BT_OK)
        st = bt_buf_writer_write_le16(w, handle);
    return st == BT_OK ? bt_buf_writer_write_le16(w, value_offset) : st;
}

bt_status_t bt_att_encode_write_request(struct bt_buf_writer *w, uint16_t handle,
                                         const uint8_t *value, size_t value_len)
{
    bt_status_t st;

    st = bt_buf_writer_write_u8(w, BT_ATT_OPCODE_WRITE_REQUEST);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_le16(w, handle);
    if (st != BT_OK)
        return st;

    return bt_buf_writer_write_bytes(w, value, value_len);
}

bt_status_t bt_att_parse_handle_value(const uint8_t *params, size_t params_len,
                                       struct bt_att_handle_value *out)
{
    struct bt_buf_reader r;
    bt_status_t st;

    if (params_len < 2)
        return BT_ERR_INVALID_ARGUMENT;

    bt_buf_reader_init(&r, params, params_len);
    st = bt_buf_reader_read_le16(&r, &out->handle);
    if (st != BT_OK)
        return st;

    out->value_len = bt_buf_reader_remaining(&r);
    out->value = bt_buf_reader_peek(&r, out->value_len);
    return BT_OK;
}

bt_status_t bt_att_encode_handle_value_confirmation(struct bt_buf_writer *w)
{
    return bt_buf_writer_write_u8(w, BT_ATT_OPCODE_HANDLE_VALUE_CONFIRMATION);
}
