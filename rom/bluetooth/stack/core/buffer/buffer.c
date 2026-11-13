#include <btcore/buffer.h>
#include <btcore/endian.h>

#include <string.h>

void bt_buf_reader_init(struct bt_buf_reader *r, const uint8_t *data, size_t len)
{
    r->data = data;
    r->len = len;
    r->pos = 0;
}

size_t bt_buf_reader_remaining(const struct bt_buf_reader *r)
{
    return r->len - r->pos;
}

static bt_status_t reader_check(const struct bt_buf_reader *r, size_t n)
{
    if (n > bt_buf_reader_remaining(r))
        return BT_ERR_BUFFER_UNDERFLOW;
    return BT_OK;
}

bt_status_t bt_buf_reader_read_u8(struct bt_buf_reader *r, uint8_t *out)
{
    bt_status_t st = reader_check(r, 1);
    if (st != BT_OK)
        return st;
    *out = r->data[r->pos];
    r->pos += 1;
    return BT_OK;
}

bt_status_t bt_buf_reader_read_le16(struct bt_buf_reader *r, uint16_t *out)
{
    bt_status_t st = reader_check(r, 2);
    if (st != BT_OK)
        return st;
    *out = bt_read_le16(r->data + r->pos);
    r->pos += 2;
    return BT_OK;
}

bt_status_t bt_buf_reader_read_le24(struct bt_buf_reader *r, uint32_t *out)
{
    bt_status_t st = reader_check(r, 3);
    if (st != BT_OK)
        return st;
    *out = bt_read_le24(r->data + r->pos);
    r->pos += 3;
    return BT_OK;
}

bt_status_t bt_buf_reader_read_le32(struct bt_buf_reader *r, uint32_t *out)
{
    bt_status_t st = reader_check(r, 4);
    if (st != BT_OK)
        return st;
    *out = bt_read_le32(r->data + r->pos);
    r->pos += 4;
    return BT_OK;
}

bt_status_t bt_buf_reader_read_le64(struct bt_buf_reader *r, uint64_t *out)
{
    bt_status_t st = reader_check(r, 8);
    if (st != BT_OK)
        return st;
    *out = bt_read_le64(r->data + r->pos);
    r->pos += 8;
    return BT_OK;
}

bt_status_t bt_buf_reader_read_be16(struct bt_buf_reader *r, uint16_t *out)
{
    bt_status_t st = reader_check(r, 2);
    if (st != BT_OK)
        return st;
    *out = bt_read_be16(r->data + r->pos);
    r->pos += 2;
    return BT_OK;
}

bt_status_t bt_buf_reader_read_be24(struct bt_buf_reader *r, uint32_t *out)
{
    bt_status_t st = reader_check(r, 3);
    if (st != BT_OK)
        return st;
    *out = bt_read_be24(r->data + r->pos);
    r->pos += 3;
    return BT_OK;
}

bt_status_t bt_buf_reader_read_be32(struct bt_buf_reader *r, uint32_t *out)
{
    bt_status_t st = reader_check(r, 4);
    if (st != BT_OK)
        return st;
    *out = bt_read_be32(r->data + r->pos);
    r->pos += 4;
    return BT_OK;
}

bt_status_t bt_buf_reader_read_be64(struct bt_buf_reader *r, uint64_t *out)
{
    bt_status_t st = reader_check(r, 8);
    if (st != BT_OK)
        return st;
    *out = bt_read_be64(r->data + r->pos);
    r->pos += 8;
    return BT_OK;
}

bt_status_t bt_buf_reader_read_bytes(struct bt_buf_reader *r, uint8_t *out, size_t n)
{
    bt_status_t st = reader_check(r, n);
    if (st != BT_OK)
        return st;
    if (n > 0)
        memcpy(out, r->data + r->pos, n);
    r->pos += n;
    return BT_OK;
}

const uint8_t *bt_buf_reader_peek(const struct bt_buf_reader *r, size_t n)
{
    if (reader_check(r, n) != BT_OK)
        return NULL;
    return r->data + r->pos;
}

bt_status_t bt_buf_reader_skip(struct bt_buf_reader *r, size_t n)
{
    bt_status_t st = reader_check(r, n);
    if (st != BT_OK)
        return st;
    r->pos += n;
    return BT_OK;
}

void bt_buf_writer_init(struct bt_buf_writer *w, uint8_t *data, size_t cap)
{
    w->data = data;
    w->cap = cap;
    w->pos = 0;
}

size_t bt_buf_writer_len(const struct bt_buf_writer *w)
{
    return w->pos;
}

size_t bt_buf_writer_remaining(const struct bt_buf_writer *w)
{
    return w->cap - w->pos;
}

static bt_status_t writer_check(const struct bt_buf_writer *w, size_t n)
{
    if (n > bt_buf_writer_remaining(w))
        return BT_ERR_BUFFER_OVERFLOW;
    return BT_OK;
}

bt_status_t bt_buf_writer_write_u8(struct bt_buf_writer *w, uint8_t value)
{
    bt_status_t st = writer_check(w, 1);
    if (st != BT_OK)
        return st;
    w->data[w->pos] = value;
    w->pos += 1;
    return BT_OK;
}

bt_status_t bt_buf_writer_write_le16(struct bt_buf_writer *w, uint16_t value)
{
    bt_status_t st = writer_check(w, 2);
    if (st != BT_OK)
        return st;
    bt_write_le16(w->data + w->pos, value);
    w->pos += 2;
    return BT_OK;
}

bt_status_t bt_buf_writer_write_le24(struct bt_buf_writer *w, uint32_t value)
{
    bt_status_t st = writer_check(w, 3);
    if (st != BT_OK)
        return st;
    bt_write_le24(w->data + w->pos, value);
    w->pos += 3;
    return BT_OK;
}

bt_status_t bt_buf_writer_write_le32(struct bt_buf_writer *w, uint32_t value)
{
    bt_status_t st = writer_check(w, 4);
    if (st != BT_OK)
        return st;
    bt_write_le32(w->data + w->pos, value);
    w->pos += 4;
    return BT_OK;
}

bt_status_t bt_buf_writer_write_le64(struct bt_buf_writer *w, uint64_t value)
{
    bt_status_t st = writer_check(w, 8);
    if (st != BT_OK)
        return st;
    bt_write_le64(w->data + w->pos, value);
    w->pos += 8;
    return BT_OK;
}

bt_status_t bt_buf_writer_write_be16(struct bt_buf_writer *w, uint16_t value)
{
    bt_status_t st = writer_check(w, 2);
    if (st != BT_OK)
        return st;
    bt_write_be16(w->data + w->pos, value);
    w->pos += 2;
    return BT_OK;
}

bt_status_t bt_buf_writer_write_be24(struct bt_buf_writer *w, uint32_t value)
{
    bt_status_t st = writer_check(w, 3);
    if (st != BT_OK)
        return st;
    bt_write_be24(w->data + w->pos, value);
    w->pos += 3;
    return BT_OK;
}

bt_status_t bt_buf_writer_write_be32(struct bt_buf_writer *w, uint32_t value)
{
    bt_status_t st = writer_check(w, 4);
    if (st != BT_OK)
        return st;
    bt_write_be32(w->data + w->pos, value);
    w->pos += 4;
    return BT_OK;
}

bt_status_t bt_buf_writer_write_be64(struct bt_buf_writer *w, uint64_t value)
{
    bt_status_t st = writer_check(w, 8);
    if (st != BT_OK)
        return st;
    bt_write_be64(w->data + w->pos, value);
    w->pos += 8;
    return BT_OK;
}

bt_status_t bt_buf_writer_write_bytes(struct bt_buf_writer *w, const uint8_t *src, size_t n)
{
    bt_status_t st = writer_check(w, n);
    if (st != BT_OK)
        return st;
    if (n > 0)
        memcpy(w->data + w->pos, src, n);
    w->pos += n;
    return BT_OK;
}
