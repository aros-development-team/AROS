#include "test_smp.h"
#include "../support/test.h"

#include <btcore/smp.h>

#include <string.h>

static void test_pairing_features(void)
{
    uint8_t buf[7];
    struct bt_buf_writer w;
    struct bt_smp_command command;
    struct bt_smp_pairing_features in = {0x04, 0x00, BT_SMP_AUTHREQ_BONDING | BT_SMP_AUTHREQ_SC,
                                         16, BT_SMP_KEYDIST_ID_KEY,
                                         BT_SMP_KEYDIST_ID_KEY | BT_SMP_KEYDIST_SIGN_KEY};
    struct bt_smp_pairing_features out;
    static const uint8_t expected[] = {0x01, 0x04, 0x00, 0x09, 0x10, 0x02, 0x06};

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_smp_encode_pairing_features(&w, BT_SMP_PAIRING_REQUEST, &in) == BT_OK);
    BT_CHECK(memcmp(buf, expected, sizeof(expected)) == 0);
    BT_CHECK(bt_smp_parse_command(buf, sizeof(buf), &command) == BT_OK);
    BT_CHECK(bt_smp_parse_pairing_features(&command, &out) == BT_OK);
    BT_CHECK(out.io_capability == in.io_capability);
    BT_CHECK(out.auth_req == in.auth_req);
    BT_CHECK(out.max_encryption_key_size == 16);
    BT_CHECK(out.responder_key_distribution == in.responder_key_distribution);

    buf[4] = 6; /* encryption keys shorter than 7 octets are forbidden */
    BT_CHECK(bt_smp_parse_command(buf, sizeof(buf), &command) == BT_ERR_INVALID_ARGUMENT);
    in.auth_req = 0x02; /* reserved Bonding_Flags value */
    BT_CHECK(bt_smp_encode_pairing_features(&w, BT_SMP_PAIRING_RESPONSE, &in) ==
              BT_ERR_INVALID_ARGUMENT);
}

static void test_exact_lengths_and_reserved_values(void)
{
    struct bt_smp_command command;
    static const uint8_t valid_failed[] = {BT_SMP_PAIRING_FAILED, 0x0A};
    static const uint8_t bad_failed[] = {BT_SMP_PAIRING_FAILED, 0x11};
    static const uint8_t bad_keypress[] = {BT_SMP_PAIRING_KEYPRESS_NOTIFICATION, 5};
    static const uint8_t reserved[] = {0x0F};

    BT_CHECK(bt_smp_parse_command(valid_failed, sizeof(valid_failed), &command) == BT_OK);
    BT_CHECK(bt_smp_parse_command(valid_failed, 1, &command) == BT_ERR_INVALID_ARGUMENT);
    BT_CHECK(bt_smp_parse_command(bad_failed, sizeof(bad_failed), &command) ==
              BT_ERR_INVALID_ARGUMENT);
    BT_CHECK(bt_smp_parse_command(bad_keypress, sizeof(bad_keypress), &command) ==
              BT_ERR_INVALID_ARGUMENT);
    BT_CHECK(bt_smp_parse_command(reserved, sizeof(reserved), &command) ==
              BT_ERR_INVALID_ARGUMENT);
}

static void test_opaque_values(void)
{
    uint8_t buf[65];
    uint8_t x[32];
    uint8_t y[32];
    struct bt_buf_writer w;
    struct bt_smp_command command;
    const uint8_t *parsed_x;
    const uint8_t *parsed_y;
    size_t i;

    for (i = 0; i < 32; ++i)
    {
        x[i] = (uint8_t)i;
        y[i] = (uint8_t)(0x80u + i);
    }
    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_smp_encode_public_key(&w, x, y) == BT_OK);
    BT_CHECK(bt_smp_parse_command(buf, sizeof(buf), &command) == BT_OK);
    BT_CHECK(bt_smp_parse_public_key(&command, &parsed_x, &parsed_y) == BT_OK);
    BT_CHECK(memcmp(parsed_x, x, 32) == 0);
    BT_CHECK(memcmp(parsed_y, y, 32) == 0);
}

static void test_key_distribution_payloads(void)
{
    uint8_t buf[17];
    uint8_t value[16];
    uint8_t rand[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    uint8_t address[6] = {6, 5, 4, 3, 2, 1};
    struct bt_buf_writer w;
    struct bt_smp_command command;
    struct bt_smp_central_identification central;
    struct bt_smp_identity_address identity;
    const uint8_t *parsed;

    memset(value, 0xA5, sizeof(value));
    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_smp_encode_value128(&w, BT_SMP_IDENTITY_INFORMATION, value) == BT_OK);
    BT_CHECK(bt_smp_parse_command(buf, 17, &command) == BT_OK);
    BT_CHECK(bt_smp_parse_value128(&command, &parsed) == BT_OK);
    BT_CHECK(memcmp(parsed, value, 16) == 0);

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_smp_encode_central_identification(&w, rand, 0x1234) == BT_OK);
    BT_CHECK(buf[1] == 0x34 && buf[2] == 0x12 && memcmp(&buf[3], rand, 8) == 0);   /* EDIV before Rand */
    BT_CHECK(bt_smp_parse_command(buf, 11, &command) == BT_OK);
    BT_CHECK(bt_smp_parse_central_identification(&command, &central) == BT_OK);
    BT_CHECK(central.ediv == 0x1234 && memcmp(central.rand, rand, 8) == 0);

    bt_buf_writer_init(&w, buf, sizeof(buf));
    BT_CHECK(bt_smp_encode_identity_address(&w, 1, address) == BT_OK);
    BT_CHECK(bt_smp_parse_command(buf, 8, &command) == BT_OK);
    BT_CHECK(bt_smp_parse_identity_address(&command, &identity) == BT_OK);
    BT_CHECK(identity.address_type == 1 && memcmp(identity.address, address, 6) == 0);
}

static void test_writer_bounds(void)
{
    uint8_t tiny[1];
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, tiny, sizeof(tiny));
    BT_CHECK(bt_smp_encode_u8_command(&w, BT_SMP_SECURITY_REQUEST,
                                      BT_SMP_AUTHREQ_BONDING) == BT_ERR_BUFFER_OVERFLOW);
    BT_CHECK(bt_buf_writer_len(&w) == 1);
}

void run_smp_tests(void)
{
    test_pairing_features();
    test_exact_lengths_and_reserved_values();
    test_opaque_values();
    test_key_distribution_payloads();
    test_writer_bounds();
}
