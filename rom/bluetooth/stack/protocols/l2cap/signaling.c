#include <btcore/l2cap.h>

bt_status_t bt_l2cap_sig_encode_header(struct bt_buf_writer *w, uint8_t code, uint8_t identifier,
                                        uint16_t length)
{
    bt_status_t st;

    st = bt_buf_writer_write_u8(w, code);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_u8(w, identifier);
    if (st != BT_OK)
        return st;

    return bt_buf_writer_write_le16(w, length);
}

bt_status_t bt_l2cap_sig_parse_header(struct bt_buf_reader *r, struct bt_l2cap_sig_header *out)
{
    bt_status_t st;

    st = bt_buf_reader_read_u8(r, &out->code);
    if (st != BT_OK)
        return st;
    st = bt_buf_reader_read_u8(r, &out->identifier);
    if (st != BT_OK)
        return st;

    return bt_buf_reader_read_le16(r, &out->length);
}

bt_status_t bt_l2cap_sig_encode_connection_request(struct bt_buf_writer *w, uint8_t identifier,
                                                     uint16_t psm, uint16_t source_cid)
{
    bt_status_t st;

    st = bt_l2cap_sig_encode_header(w, BT_L2CAP_SIG_CONNECTION_REQUEST, identifier, 4);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_le16(w, psm);
    if (st != BT_OK)
        return st;

    return bt_buf_writer_write_le16(w, source_cid);
}

bt_status_t bt_l2cap_sig_parse_connection_request(const uint8_t *cmd_data, size_t cmd_data_len,
                                                    struct bt_l2cap_connection_request *out)
{
    struct bt_buf_reader r;
    bt_status_t st;

    if (cmd_data_len != 4)
        return BT_ERR_INVALID_ARGUMENT;

    bt_buf_reader_init(&r, cmd_data, cmd_data_len);
    st = bt_buf_reader_read_le16(&r, &out->psm);
    if (st != BT_OK)
        return st;

    return bt_buf_reader_read_le16(&r, &out->source_cid);
}

bt_status_t bt_l2cap_sig_encode_connection_response(struct bt_buf_writer *w, uint8_t identifier,
                                                      uint16_t destination_cid,
                                                      uint16_t source_cid, uint16_t result,
                                                      uint16_t status)
{
    bt_status_t st;

    st = bt_l2cap_sig_encode_header(w, BT_L2CAP_SIG_CONNECTION_RESPONSE, identifier, 8);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_le16(w, destination_cid);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_le16(w, source_cid);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_le16(w, result);
    if (st != BT_OK)
        return st;

    return bt_buf_writer_write_le16(w, status);
}

bt_status_t bt_l2cap_sig_parse_connection_response(const uint8_t *cmd_data, size_t cmd_data_len,
                                                     struct bt_l2cap_connection_response *out)
{
    struct bt_buf_reader r;
    bt_status_t st;

    if (cmd_data_len != 8)
        return BT_ERR_INVALID_ARGUMENT;

    bt_buf_reader_init(&r, cmd_data, cmd_data_len);
    st = bt_buf_reader_read_le16(&r, &out->destination_cid);
    if (st != BT_OK)
        return st;
    st = bt_buf_reader_read_le16(&r, &out->source_cid);
    if (st != BT_OK)
        return st;
    st = bt_buf_reader_read_le16(&r, &out->result);
    if (st != BT_OK)
        return st;

    return bt_buf_reader_read_le16(&r, &out->status);
}

bt_status_t bt_l2cap_sig_encode_configure_request(struct bt_buf_writer *w, uint8_t identifier,
                                                    uint16_t destination_cid, uint16_t flags,
                                                    uint16_t mtu)
{
    uint16_t length = 4 + (mtu != 0 ? 4u : 0u);
    bt_status_t st;

    st = bt_l2cap_sig_encode_header(w, BT_L2CAP_SIG_CONFIGURE_REQUEST, identifier, length);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_le16(w, destination_cid);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_le16(w, flags);
    if (st != BT_OK)
        return st;

    if (mtu == 0)
        return BT_OK;

    st = bt_buf_writer_write_u8(w, BT_L2CAP_CONFIG_OPTION_MTU);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_u8(w, 2);
    if (st != BT_OK)
        return st;

    return bt_buf_writer_write_le16(w, mtu);
}

/* Scans a Configuration Option list for an MTU option (type 0x01),
 * skipping any other option types by their declared length. */
static bt_status_t scan_for_mtu_option(struct bt_buf_reader *r, bool *out_has_mtu,
                                        uint16_t *out_mtu)
{
    *out_has_mtu = false;
    *out_mtu = 0;

    while (bt_buf_reader_remaining(r) > 0)
    {
        uint8_t type;
        uint8_t len;
        bt_status_t st;

        st = bt_buf_reader_read_u8(r, &type);
        if (st != BT_OK)
            return st;
        st = bt_buf_reader_read_u8(r, &len);
        if (st != BT_OK)
            return st;

        if (type == BT_L2CAP_CONFIG_OPTION_MTU)
        {
            uint16_t mtu;

            if (len != 2)
                return BT_ERR_INVALID_ARGUMENT;
            st = bt_buf_reader_read_le16(r, &mtu);
            if (st != BT_OK)
                return st;
            *out_has_mtu = true;
            *out_mtu = mtu;
        }
        else
        {
            st = bt_buf_reader_skip(r, len);
            if (st != BT_OK)
                return st;
        }
    }

    return BT_OK;
}

bt_status_t bt_l2cap_sig_parse_configure_request(const uint8_t *cmd_data, size_t cmd_data_len,
                                                   struct bt_l2cap_configure_request *out)
{
    struct bt_buf_reader r;
    bt_status_t st;

    if (cmd_data_len < 4)
        return BT_ERR_INVALID_ARGUMENT;

    bt_buf_reader_init(&r, cmd_data, cmd_data_len);
    st = bt_buf_reader_read_le16(&r, &out->destination_cid);
    if (st != BT_OK)
        return st;
    st = bt_buf_reader_read_le16(&r, &out->flags);
    if (st != BT_OK)
        return st;

    return scan_for_mtu_option(&r, &out->has_mtu, &out->mtu);
}

bt_status_t bt_l2cap_sig_encode_configure_response(struct bt_buf_writer *w, uint8_t identifier,
                                                     uint16_t source_cid, uint16_t flags,
                                                     uint16_t result, uint16_t mtu)
{
    uint16_t length = 6 + (mtu != 0 ? 4u : 0u);
    bt_status_t st;

    st = bt_l2cap_sig_encode_header(w, BT_L2CAP_SIG_CONFIGURE_RESPONSE, identifier, length);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_le16(w, source_cid);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_le16(w, flags);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_le16(w, result);
    if (st != BT_OK)
        return st;

    if (mtu == 0)
        return BT_OK;

    st = bt_buf_writer_write_u8(w, BT_L2CAP_CONFIG_OPTION_MTU);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_u8(w, 2);
    if (st != BT_OK)
        return st;

    return bt_buf_writer_write_le16(w, mtu);
}

bt_status_t bt_l2cap_sig_parse_configure_response(const uint8_t *cmd_data, size_t cmd_data_len,
                                                    struct bt_l2cap_configure_response *out)
{
    struct bt_buf_reader r;
    bt_status_t st;

    if (cmd_data_len < 6)
        return BT_ERR_INVALID_ARGUMENT;

    bt_buf_reader_init(&r, cmd_data, cmd_data_len);
    st = bt_buf_reader_read_le16(&r, &out->source_cid);
    if (st != BT_OK)
        return st;
    st = bt_buf_reader_read_le16(&r, &out->flags);
    if (st != BT_OK)
        return st;
    st = bt_buf_reader_read_le16(&r, &out->result);
    if (st != BT_OK)
        return st;

    return scan_for_mtu_option(&r, &out->has_mtu, &out->mtu);
}

bt_status_t bt_l2cap_sig_encode_disconnection_request(struct bt_buf_writer *w, uint8_t identifier,
                                                        uint16_t destination_cid,
                                                        uint16_t source_cid)
{
    bt_status_t st;

    st = bt_l2cap_sig_encode_header(w, BT_L2CAP_SIG_DISCONNECTION_REQUEST, identifier, 4);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_le16(w, destination_cid);
    if (st != BT_OK)
        return st;

    return bt_buf_writer_write_le16(w, source_cid);
}

bt_status_t bt_l2cap_sig_encode_disconnection_response(struct bt_buf_writer *w, uint8_t identifier,
                                                         uint16_t destination_cid,
                                                         uint16_t source_cid)
{
    bt_status_t st;

    st = bt_l2cap_sig_encode_header(w, BT_L2CAP_SIG_DISCONNECTION_RESPONSE, identifier, 4);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_le16(w, destination_cid);
    if (st != BT_OK)
        return st;

    return bt_buf_writer_write_le16(w, source_cid);
}

bt_status_t bt_l2cap_sig_parse_disconnection(const uint8_t *cmd_data, size_t cmd_data_len,
                                              struct bt_l2cap_disconnection *out)
{
    struct bt_buf_reader r;
    bt_status_t st;

    if (cmd_data_len != 4)
        return BT_ERR_INVALID_ARGUMENT;

    bt_buf_reader_init(&r, cmd_data, cmd_data_len);
    st = bt_buf_reader_read_le16(&r, &out->destination_cid);
    if (st != BT_OK)
        return st;

    return bt_buf_reader_read_le16(&r, &out->source_cid);
}

bt_status_t bt_l2cap_sig_encode_command_reject(struct bt_buf_writer *w, uint8_t identifier,
                                                uint16_t reason)
{
    bt_status_t st;

    st = bt_l2cap_sig_encode_header(w, BT_L2CAP_SIG_COMMAND_REJECT, identifier, 2);
    if (st != BT_OK)
        return st;

    return bt_buf_writer_write_le16(w, reason);
}

bt_status_t bt_l2cap_sig_parse_command_reject(const uint8_t *cmd_data, size_t cmd_data_len,
                                               struct bt_l2cap_command_reject *out)
{
    struct bt_buf_reader r;
    bt_status_t st;

    if (cmd_data_len < 2)
        return BT_ERR_INVALID_ARGUMENT;

    bt_buf_reader_init(&r, cmd_data, cmd_data_len);
    st = bt_buf_reader_read_le16(&r, &out->reason);
    if (st != BT_OK)
        return st;

    out->data = bt_buf_reader_peek(&r, bt_buf_reader_remaining(&r));
    out->data_len = bt_buf_reader_remaining(&r);
    return BT_OK;
}
