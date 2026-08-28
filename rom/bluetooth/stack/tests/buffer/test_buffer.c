#include "test_buffer.h"
#include "../support/test.h"

#include <btcore/buffer.h>

static void test_round_trip(void)
{
    uint8_t raw[64];
    struct bt_buf_writer w;
    struct bt_buf_reader r;

    bt_buf_writer_init(&w, raw, sizeof(raw));
    BT_CHECK(bt_buf_writer_write_u8(&w, 0xab) == BT_OK);
    BT_CHECK(bt_buf_writer_write_le16(&w, 0x1234) == BT_OK);
    BT_CHECK(bt_buf_writer_write_be16(&w, 0x5678) == BT_OK);
    BT_CHECK(bt_buf_writer_write_le24(&w, 0x123456) == BT_OK);
    BT_CHECK(bt_buf_writer_write_be24(&w, 0x789abc) == BT_OK);
    BT_CHECK(bt_buf_writer_write_le32(&w, 0x11223344) == BT_OK);
    BT_CHECK(bt_buf_writer_write_be32(&w, 0x55667788) == BT_OK);
    BT_CHECK(bt_buf_writer_write_le64(&w, 0x0102030405060708ull) == BT_OK);
    BT_CHECK(bt_buf_writer_write_be64(&w, 0x1112131415161718ull) == BT_OK);

    static const uint8_t tail[] = {0xde, 0xad, 0xbe, 0xef};
    BT_CHECK(bt_buf_writer_write_bytes(&w, tail, sizeof(tail)) == BT_OK);

    size_t written = bt_buf_writer_len(&w);
    BT_CHECK(written == 1 + 2 + 2 + 3 + 3 + 4 + 4 + 8 + 8 + 4);

    bt_buf_reader_init(&r, raw, written);

    uint8_t u8;
    uint16_t u16;
    uint32_t u24;
    uint32_t u32;
    uint64_t u64;

    BT_CHECK(bt_buf_reader_read_u8(&r, &u8) == BT_OK && u8 == 0xab);
    BT_CHECK(bt_buf_reader_read_le16(&r, &u16) == BT_OK && u16 == 0x1234);
    BT_CHECK(bt_buf_reader_read_be16(&r, &u16) == BT_OK && u16 == 0x5678);
    BT_CHECK(bt_buf_reader_read_le24(&r, &u24) == BT_OK && u24 == 0x123456);
    BT_CHECK(bt_buf_reader_read_be24(&r, &u24) == BT_OK && u24 == 0x789abc);
    BT_CHECK(bt_buf_reader_read_le32(&r, &u32) == BT_OK && u32 == 0x11223344);
    BT_CHECK(bt_buf_reader_read_be32(&r, &u32) == BT_OK && u32 == 0x55667788);
    BT_CHECK(bt_buf_reader_read_le64(&r, &u64) == BT_OK && u64 == 0x0102030405060708ull);
    BT_CHECK(bt_buf_reader_read_be64(&r, &u64) == BT_OK && u64 == 0x1112131415161718ull);

    uint8_t tail_out[4];
    BT_CHECK(bt_buf_reader_read_bytes(&r, tail_out, sizeof(tail_out)) == BT_OK);
    BT_CHECK(tail_out[0] == 0xde && tail_out[1] == 0xad &&
             tail_out[2] == 0xbe && tail_out[3] == 0xef);

    BT_CHECK(bt_buf_reader_remaining(&r) == 0);
}

static void test_writer_overflow(void)
{
    uint8_t raw[3];
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, raw, sizeof(raw));
    BT_CHECK(bt_buf_writer_write_u8(&w, 1) == BT_OK);
    BT_CHECK(bt_buf_writer_write_u8(&w, 2) == BT_OK);
    /* Only one byte left: a 2-byte write must fail without touching pos. */
    size_t pos_before = bt_buf_writer_len(&w);
    BT_CHECK(bt_buf_writer_write_le16(&w, 0xffff) == BT_ERR_BUFFER_OVERFLOW);
    BT_CHECK(bt_buf_writer_len(&w) == pos_before);
    BT_CHECK(bt_buf_writer_write_u8(&w, 3) == BT_OK);
    BT_CHECK(bt_buf_writer_write_u8(&w, 4) == BT_ERR_BUFFER_OVERFLOW);
    BT_CHECK(bt_buf_writer_remaining(&w) == 0);
}

static void test_reader_underflow(void)
{
    static const uint8_t raw[3] = {0x01, 0x02, 0x03};
    struct bt_buf_reader r;

    bt_buf_reader_init(&r, raw, sizeof(raw));

    uint32_t u32;
    /* Deliberately hostile: ask for more than the buffer holds. Must fail
     * cleanly (and, under ASan, must not read out of bounds). */
    BT_CHECK(bt_buf_reader_read_le32(&r, &u32) == BT_ERR_BUFFER_UNDERFLOW);
    BT_CHECK(bt_buf_reader_remaining(&r) == 3);

    uint8_t out[8];
    BT_CHECK(bt_buf_reader_read_bytes(&r, out, 4) == BT_ERR_BUFFER_UNDERFLOW);
    BT_CHECK(bt_buf_reader_skip(&r, 4) == BT_ERR_BUFFER_UNDERFLOW);
    BT_CHECK(bt_buf_reader_peek(&r, 4) == NULL);

    BT_CHECK(bt_buf_reader_skip(&r, 3) == BT_OK);
    BT_CHECK(bt_buf_reader_remaining(&r) == 0);
}

static void test_peek_skip(void)
{
    static const uint8_t raw[4] = {0xaa, 0xbb, 0xcc, 0xdd};
    struct bt_buf_reader r;

    bt_buf_reader_init(&r, raw, sizeof(raw));

    const uint8_t *p = bt_buf_reader_peek(&r, 2);
    BT_CHECK(p != NULL && p[0] == 0xaa && p[1] == 0xbb);
    /* peek must not consume */
    BT_CHECK(bt_buf_reader_remaining(&r) == 4);

    BT_CHECK(bt_buf_reader_skip(&r, 2) == BT_OK);
    BT_CHECK(bt_buf_reader_remaining(&r) == 2);

    p = bt_buf_reader_peek(&r, 2);
    BT_CHECK(p != NULL && p[0] == 0xcc && p[1] == 0xdd);
}

void run_buffer_tests(void)
{
    test_round_trip();
    test_writer_overflow();
    test_reader_underflow();
    test_peek_skip();
}
