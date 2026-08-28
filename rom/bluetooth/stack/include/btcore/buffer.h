#ifndef BTCORE_BUFFER_H
#define BTCORE_BUFFER_H

#include <btcore/status.h>
#include <btcore/types.h>

/*
 * Bounds-checked cursors over a caller-owned byte buffer. Every read/write
 * validates remaining space before touching memory, so a reader fed hostile
 * or truncated input never reads past its buffer, and a writer never
 * overflows its capacity. Multi-byte fields go through btcore/endian.h,
 * never through a struct cast (see project.md, regra de endianness).
 */

struct bt_buf_reader
{
    const uint8_t *data;
    size_t len;
    size_t pos;
};

void bt_buf_reader_init(struct bt_buf_reader *r, const uint8_t *data, size_t len);
size_t bt_buf_reader_remaining(const struct bt_buf_reader *r);

bt_status_t bt_buf_reader_read_u8(struct bt_buf_reader *r, uint8_t *out);
bt_status_t bt_buf_reader_read_le16(struct bt_buf_reader *r, uint16_t *out);
bt_status_t bt_buf_reader_read_le24(struct bt_buf_reader *r, uint32_t *out);
bt_status_t bt_buf_reader_read_le32(struct bt_buf_reader *r, uint32_t *out);
bt_status_t bt_buf_reader_read_le64(struct bt_buf_reader *r, uint64_t *out);
bt_status_t bt_buf_reader_read_be16(struct bt_buf_reader *r, uint16_t *out);
bt_status_t bt_buf_reader_read_be24(struct bt_buf_reader *r, uint32_t *out);
bt_status_t bt_buf_reader_read_be32(struct bt_buf_reader *r, uint32_t *out);
bt_status_t bt_buf_reader_read_be64(struct bt_buf_reader *r, uint64_t *out);

/* Copies n bytes out. For zero-copy access to the wire bytes themselves
 * (e.g. to hand off a sub-range), use bt_buf_reader_peek + bt_buf_reader_skip. */
bt_status_t bt_buf_reader_read_bytes(struct bt_buf_reader *r, uint8_t *out, size_t n);

/* Pointer to the current position, without consuming any bytes. NULL if
 * fewer than n bytes remain. */
const uint8_t *bt_buf_reader_peek(const struct bt_buf_reader *r, size_t n);
bt_status_t bt_buf_reader_skip(struct bt_buf_reader *r, size_t n);

struct bt_buf_writer
{
    uint8_t *data;
    size_t cap;
    size_t pos;
};

void bt_buf_writer_init(struct bt_buf_writer *w, uint8_t *data, size_t cap);
size_t bt_buf_writer_len(const struct bt_buf_writer *w);
size_t bt_buf_writer_remaining(const struct bt_buf_writer *w);

bt_status_t bt_buf_writer_write_u8(struct bt_buf_writer *w, uint8_t value);
bt_status_t bt_buf_writer_write_le16(struct bt_buf_writer *w, uint16_t value);
bt_status_t bt_buf_writer_write_le24(struct bt_buf_writer *w, uint32_t value);
bt_status_t bt_buf_writer_write_le32(struct bt_buf_writer *w, uint32_t value);
bt_status_t bt_buf_writer_write_le64(struct bt_buf_writer *w, uint64_t value);
bt_status_t bt_buf_writer_write_be16(struct bt_buf_writer *w, uint16_t value);
bt_status_t bt_buf_writer_write_be24(struct bt_buf_writer *w, uint32_t value);
bt_status_t bt_buf_writer_write_be32(struct bt_buf_writer *w, uint32_t value);
bt_status_t bt_buf_writer_write_be64(struct bt_buf_writer *w, uint64_t value);

bt_status_t bt_buf_writer_write_bytes(struct bt_buf_writer *w, const uint8_t *src, size_t n);

#endif /* BTCORE_BUFFER_H */
