#include "test_endian.h"
#include "../support/test.h"

#include <btcore/endian.h>

static void test_known_vectors(void)
{
    static const uint8_t le16[] = {0x34, 0x12};
    static const uint8_t be16[] = {0x12, 0x34};
    static const uint8_t le24[] = {0x56, 0x34, 0x12};
    static const uint8_t be24[] = {0x12, 0x34, 0x56};
    static const uint8_t le32[] = {0x78, 0x56, 0x34, 0x12};
    static const uint8_t be32[] = {0x12, 0x34, 0x56, 0x78};
    static const uint8_t le64[] = {0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12};
    static const uint8_t be64[] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0};

    BT_CHECK(bt_read_le16(le16) == 0x1234u);
    BT_CHECK(bt_read_be16(be16) == 0x1234u);
    BT_CHECK(bt_read_le24(le24) == 0x123456u);
    BT_CHECK(bt_read_be24(be24) == 0x123456u);
    BT_CHECK(bt_read_le32(le32) == 0x12345678u);
    BT_CHECK(bt_read_be32(be32) == 0x12345678u);
    BT_CHECK(bt_read_le64(le64) == 0x123456789abcdef0ull);
    BT_CHECK(bt_read_be64(be64) == 0x123456789abcdef0ull);

    /* Writers must produce the exact same wire bytes, regardless of host endianness. */
    uint8_t buf[8];

    bt_write_le16(buf, 0x1234u);
    BT_CHECK(buf[0] == 0x34 && buf[1] == 0x12);

    bt_write_be16(buf, 0x1234u);
    BT_CHECK(buf[0] == 0x12 && buf[1] == 0x34);

    bt_write_le24(buf, 0x123456u);
    BT_CHECK(buf[0] == 0x56 && buf[1] == 0x34 && buf[2] == 0x12);

    bt_write_be24(buf, 0x123456u);
    BT_CHECK(buf[0] == 0x12 && buf[1] == 0x34 && buf[2] == 0x56);

    bt_write_le32(buf, 0x12345678u);
    BT_CHECK(buf[0] == 0x78 && buf[1] == 0x56 && buf[2] == 0x34 && buf[3] == 0x12);

    bt_write_be32(buf, 0x12345678u);
    BT_CHECK(buf[0] == 0x12 && buf[1] == 0x34 && buf[2] == 0x56 && buf[3] == 0x78);

    bt_write_le64(buf, 0x123456789abcdef0ull);
    BT_CHECK(buf[0] == 0xf0 && buf[7] == 0x12);

    bt_write_be64(buf, 0x123456789abcdef0ull);
    BT_CHECK(buf[0] == 0x12 && buf[7] == 0xf0);
}

static const uint64_t test_values[] = {
    0x0ull, 0x1ull, 0x7full, 0x80ull, 0xffull,
    0x1234ull, 0xfffeull, 0xffffull,
    0x123456ull, 0xfffffeull, 0xffffffull,
    0x12345678ull, 0xfffffffeull, 0xffffffffull,
    0x123456789abcdef0ull, 0xfffffffffffffffeull, 0xffffffffffffffffull,
};
#define NUM_TEST_VALUES (sizeof(test_values) / sizeof(test_values[0]))

static void test_round_trip(void)
{
    for (size_t i = 0; i < NUM_TEST_VALUES; i++)
    {
        uint64_t v = test_values[i];
        uint8_t buf[8];

        bt_write_le16(buf, (uint16_t)v);
        BT_CHECK(bt_read_le16(buf) == (uint16_t)v);
        bt_write_be16(buf, (uint16_t)v);
        BT_CHECK(bt_read_be16(buf) == (uint16_t)v);

        bt_write_le24(buf, (uint32_t)v & 0xffffffu);
        BT_CHECK(bt_read_le24(buf) == ((uint32_t)v & 0xffffffu));
        bt_write_be24(buf, (uint32_t)v & 0xffffffu);
        BT_CHECK(bt_read_be24(buf) == ((uint32_t)v & 0xffffffu));

        bt_write_le32(buf, (uint32_t)v);
        BT_CHECK(bt_read_le32(buf) == (uint32_t)v);
        bt_write_be32(buf, (uint32_t)v);
        BT_CHECK(bt_read_be32(buf) == (uint32_t)v);

        bt_write_le64(buf, v);
        BT_CHECK(bt_read_le64(buf) == v);
        bt_write_be64(buf, v);
        BT_CHECK(bt_read_be64(buf) == v);
    }
}

/* Values must round-trip correctly no matter where the field starts inside a
 * larger buffer -- these helpers never cast the pointer to a wider type, so
 * there is no natural-alignment requirement. */
static void test_misaligned(void)
{
    uint8_t buf[64];

    for (size_t offset = 0; offset < 9; offset++)
    {
        uint8_t *p = buf + offset;

        bt_write_le64(p, 0x123456789abcdef0ull);
        BT_CHECK(bt_read_le64(p) == 0x123456789abcdef0ull);

        bt_write_be64(p, 0x123456789abcdef0ull);
        BT_CHECK(bt_read_be64(p) == 0x123456789abcdef0ull);

        bt_write_le32(p, 0x89abcdefu);
        BT_CHECK(bt_read_le32(p) == 0x89abcdefu);

        bt_write_be32(p, 0x89abcdefu);
        BT_CHECK(bt_read_be32(p) == 0x89abcdefu);

        bt_write_le24(p, 0xabcdefu);
        BT_CHECK(bt_read_le24(p) == 0xabcdefu);

        bt_write_be24(p, 0xabcdefu);
        BT_CHECK(bt_read_be24(p) == 0xabcdefu);

        bt_write_le16(p, 0xcdefu);
        BT_CHECK(bt_read_le16(p) == 0xcdefu);

        bt_write_be16(p, 0xcdefu);
        BT_CHECK(bt_read_be16(p) == 0xcdefu);
    }
}

void run_endian_tests(void)
{
    test_known_vectors();
    test_round_trip();
    test_misaligned();
}
