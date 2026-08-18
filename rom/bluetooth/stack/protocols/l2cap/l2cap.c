#include <btcore/l2cap.h>

#include <string.h>

bt_status_t bt_l2cap_encode_header(struct bt_buf_writer *w, uint16_t length, uint16_t cid)
{
    bt_status_t st;

    st = bt_buf_writer_write_le16(w, length);
    if (st != BT_OK)
        return st;

    return bt_buf_writer_write_le16(w, cid);
}

bt_status_t bt_l2cap_parse_header(struct bt_buf_reader *r, struct bt_l2cap_header *out)
{
    bt_status_t st;

    st = bt_buf_reader_read_le16(r, &out->length);
    if (st != BT_OK)
        return st;

    return bt_buf_reader_read_le16(r, &out->cid);
}

void bt_l2cap_reassembler_init(struct bt_l2cap_reassembler *ra)
{
    ra->have = 0;
    ra->want = 0;
}

static enum bt_l2cap_reassembly_result finalize_header_and_check(struct bt_l2cap_reassembler *ra)
{
    if (ra->want == 0 && ra->have >= BT_L2CAP_HEADER_LEN)
    {
        struct bt_buf_reader r;
        struct bt_l2cap_header hdr;

        bt_buf_reader_init(&r, ra->buf, BT_L2CAP_HEADER_LEN);
        /* Always succeeds: ra->buf has at least BT_L2CAP_HEADER_LEN bytes here. */
        bt_l2cap_parse_header(&r, &hdr);

        ra->want = BT_L2CAP_HEADER_LEN + (size_t)hdr.length;
        if (ra->want > BT_L2CAP_REASSEMBLY_MAX)
        {
            bt_l2cap_reassembler_init(ra);
            return BT_L2CAP_REASSEMBLY_ERROR;
        }
    }

    if (ra->want != 0)
    {
        if (ra->have > ra->want)
        {
            bt_l2cap_reassembler_init(ra);
            return BT_L2CAP_REASSEMBLY_ERROR;
        }
        if (ra->have == ra->want)
            return BT_L2CAP_REASSEMBLY_COMPLETE;
    }

    return BT_L2CAP_REASSEMBLY_MORE;
}

enum bt_l2cap_reassembly_result bt_l2cap_reassembler_feed(struct bt_l2cap_reassembler *ra,
                                                           uint8_t pb_flag, const uint8_t *data,
                                                           size_t len)
{
    if (pb_flag != 0x01)
    {
        /* Start of a new PDU -- abandon anything already in progress. */
        bt_l2cap_reassembler_init(ra);

        if (len > BT_L2CAP_REASSEMBLY_MAX)
            return BT_L2CAP_REASSEMBLY_ERROR;

        if (len > 0)
            memcpy(ra->buf, data, len);
        ra->have = len;
    }
    else
    {
        if (ra->have == 0)
            return BT_L2CAP_REASSEMBLY_ERROR; /* continuation with nothing started */

        if (ra->have + len > BT_L2CAP_REASSEMBLY_MAX)
        {
            bt_l2cap_reassembler_init(ra);
            return BT_L2CAP_REASSEMBLY_ERROR;
        }

        if (len > 0)
            memcpy(ra->buf + ra->have, data, len);
        ra->have += len;
    }

    return finalize_header_and_check(ra);
}

const uint8_t *bt_l2cap_reassembler_take(struct bt_l2cap_reassembler *ra, size_t *out_len)
{
    *out_len = ra->have;
    ra->have = 0;
    ra->want = 0;
    return ra->buf;
}

void bt_l2cap_fragmenter_init(struct bt_l2cap_fragmenter *fr, uint16_t handle, size_t frag_size,
                               const uint8_t *l2cap_pdu, size_t l2cap_pdu_len)
{
    fr->data = l2cap_pdu;
    fr->len = l2cap_pdu_len;
    fr->pos = 0;
    fr->handle = handle;
    fr->frag_size = frag_size;
}

bt_status_t bt_l2cap_fragmenter_next(struct bt_l2cap_fragmenter *fr, struct bt_buf_writer *w)
{
    size_t remaining;
    size_t chunk;
    uint8_t pb_flag;
    bt_status_t st;

    if (fr->pos >= fr->len)
        return BT_ERR_BUFFER_UNDERFLOW;

    remaining = fr->len - fr->pos;
    chunk = (remaining < fr->frag_size) ? remaining : fr->frag_size;
    pb_flag = (fr->pos == 0) ? 0x00 : 0x01;

    st = bt_hci_encode_acl_header(w, fr->handle, pb_flag, 0x00, (uint16_t)chunk);
    if (st != BT_OK)
        return st;

    st = bt_buf_writer_write_bytes(w, fr->data + fr->pos, chunk);
    if (st != BT_OK)
        return st;

    fr->pos += chunk;
    return BT_OK;
}
