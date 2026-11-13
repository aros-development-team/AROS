#include <btcore/smp.h>

static size_t command_data_len(uint8_t code)
{
    switch (code)
    {
    case BT_SMP_PAIRING_REQUEST:
    case BT_SMP_PAIRING_RESPONSE:
        return 6;
    case BT_SMP_PAIRING_CONFIRM:
    case BT_SMP_PAIRING_RANDOM:
    case BT_SMP_ENCRYPTION_INFORMATION:
    case BT_SMP_IDENTITY_INFORMATION:
    case BT_SMP_SIGNING_INFORMATION:
    case BT_SMP_PAIRING_DHKEY_CHECK:
        return 16;
    case BT_SMP_PAIRING_FAILED:
    case BT_SMP_SECURITY_REQUEST:
    case BT_SMP_PAIRING_KEYPRESS_NOTIFICATION:
        return 1;
    case BT_SMP_CENTRAL_IDENTIFICATION:
        return 10;
    case BT_SMP_IDENTITY_ADDRESS_INFORMATION:
        return 7;
    case BT_SMP_PAIRING_PUBLIC_KEY:
        return 64;
    default:
        return SIZE_MAX;
    }
}

static bool pairing_features_valid(const struct bt_smp_pairing_features *f)
{
    return f->io_capability <= 0x04u && f->oob_data_flag <= 0x01u &&
           (f->auth_req & ~BT_SMP_AUTHREQ_VALID_MASK) == 0 &&
           (f->auth_req & 0x03u) <= BT_SMP_AUTHREQ_BONDING &&
           f->max_encryption_key_size >= 7 && f->max_encryption_key_size <= 16 &&
           (f->initiator_key_distribution & ~BT_SMP_KEYDIST_VALID_MASK) == 0 &&
           (f->responder_key_distribution & ~BT_SMP_KEYDIST_VALID_MASK) == 0;
}

bt_status_t bt_smp_parse_command(const uint8_t *pdu, size_t pdu_len,
                                  struct bt_smp_command *out)
{
    size_t expected;

    if (pdu == NULL || out == NULL || pdu_len == 0)
        return BT_ERR_INVALID_ARGUMENT;
    expected = command_data_len(pdu[0]);
    if (expected == SIZE_MAX || pdu_len != expected + 1)
        return BT_ERR_INVALID_ARGUMENT;

    if (pdu[0] == BT_SMP_PAIRING_FAILED && (pdu[1] < 0x01u || pdu[1] > 0x10u))
        return BT_ERR_INVALID_ARGUMENT;
    if (pdu[0] == BT_SMP_PAIRING_REQUEST || pdu[0] == BT_SMP_PAIRING_RESPONSE)
    {
        struct bt_smp_pairing_features f;

        f.io_capability = pdu[1];
        f.oob_data_flag = pdu[2];
        f.auth_req = pdu[3];
        f.max_encryption_key_size = pdu[4];
        f.initiator_key_distribution = pdu[5];
        f.responder_key_distribution = pdu[6];
        if (!pairing_features_valid(&f))
            return BT_ERR_INVALID_ARGUMENT;
    }
    if (pdu[0] == BT_SMP_SECURITY_REQUEST &&
        ((pdu[1] & ~BT_SMP_AUTHREQ_VALID_MASK) != 0 || (pdu[1] & 0x03u) > 1))
        return BT_ERR_INVALID_ARGUMENT;
    if (pdu[0] == BT_SMP_PAIRING_KEYPRESS_NOTIFICATION && pdu[1] > 4)
        return BT_ERR_INVALID_ARGUMENT;
    if (pdu[0] == BT_SMP_IDENTITY_ADDRESS_INFORMATION && pdu[1] > 1)
        return BT_ERR_INVALID_ARGUMENT;

    out->code = pdu[0];
    out->data = pdu + 1;
    out->data_len = expected;
    return BT_OK;
}

bt_status_t bt_smp_encode_pairing_features(struct bt_buf_writer *w, uint8_t code,
                                            const struct bt_smp_pairing_features *f)
{
    bt_status_t st;

    if (w == NULL || f == NULL ||
        (code != BT_SMP_PAIRING_REQUEST && code != BT_SMP_PAIRING_RESPONSE) ||
        !pairing_features_valid(f))
        return BT_ERR_INVALID_ARGUMENT;

#define WRITE_BYTE(value)                       \
    do                                          \
    {                                           \
        st = bt_buf_writer_write_u8(w, (value)); \
        if (st != BT_OK)                        \
            return st;                          \
    } while (0)
    WRITE_BYTE(code);
    WRITE_BYTE(f->io_capability);
    WRITE_BYTE(f->oob_data_flag);
    WRITE_BYTE(f->auth_req);
    WRITE_BYTE(f->max_encryption_key_size);
    WRITE_BYTE(f->initiator_key_distribution);
    WRITE_BYTE(f->responder_key_distribution);
#undef WRITE_BYTE
    return BT_OK;
}

bt_status_t bt_smp_parse_pairing_features(const struct bt_smp_command *command,
                                           struct bt_smp_pairing_features *out)
{
    if (command == NULL || out == NULL ||
        (command->code != BT_SMP_PAIRING_REQUEST &&
         command->code != BT_SMP_PAIRING_RESPONSE) ||
        command->data_len != 6)
        return BT_ERR_INVALID_ARGUMENT;

    out->io_capability = command->data[0];
    out->oob_data_flag = command->data[1];
    out->auth_req = command->data[2];
    out->max_encryption_key_size = command->data[3];
    out->initiator_key_distribution = command->data[4];
    out->responder_key_distribution = command->data[5];
    return pairing_features_valid(out) ? BT_OK : BT_ERR_INVALID_ARGUMENT;
}

static bool is_value128(uint8_t code)
{
    return command_data_len(code) == 16;
}

bt_status_t bt_smp_encode_value128(struct bt_buf_writer *w, uint8_t code,
                                    const uint8_t value[16])
{
    bt_status_t st;

    if (w == NULL || value == NULL || !is_value128(code))
        return BT_ERR_INVALID_ARGUMENT;
    st = bt_buf_writer_write_u8(w, code);
    return st == BT_OK ? bt_buf_writer_write_bytes(w, value, 16) : st;
}

bt_status_t bt_smp_parse_value128(const struct bt_smp_command *command,
                                   const uint8_t **out_value)
{
    if (command == NULL || out_value == NULL || !is_value128(command->code) ||
        command->data_len != 16)
        return BT_ERR_INVALID_ARGUMENT;
    *out_value = command->data;
    return BT_OK;
}

bt_status_t bt_smp_encode_central_identification(struct bt_buf_writer *w,
                                                  const uint8_t rand[8], uint16_t ediv)
{
    bt_status_t st;

    if (w == NULL || rand == NULL)
        return BT_ERR_INVALID_ARGUMENT;
    /* Core Vol 3 Part H 3.6.3: Code, EDIV (2 octets), Rand (8 octets) */
    st = bt_buf_writer_write_u8(w, BT_SMP_CENTRAL_IDENTIFICATION);
    if (st == BT_OK)
        st = bt_buf_writer_write_le16(w, ediv);
    return st == BT_OK ? bt_buf_writer_write_bytes(w, rand, 8) : st;
}

bt_status_t bt_smp_parse_central_identification(const struct bt_smp_command *command,
                                                 struct bt_smp_central_identification *out)
{
    if (command == NULL || out == NULL || command->code != BT_SMP_CENTRAL_IDENTIFICATION ||
        command->data_len != 10)
        return BT_ERR_INVALID_ARGUMENT;
    out->ediv = (uint16_t)command->data[0] | ((uint16_t)command->data[1] << 8);
    out->rand = command->data + 2;
    return BT_OK;
}

bt_status_t bt_smp_encode_identity_address(struct bt_buf_writer *w, uint8_t address_type,
                                            const uint8_t address[6])
{
    bt_status_t st;

    if (w == NULL || address == NULL || address_type > 1)
        return BT_ERR_INVALID_ARGUMENT;
    st = bt_buf_writer_write_u8(w, BT_SMP_IDENTITY_ADDRESS_INFORMATION);
    if (st == BT_OK)
        st = bt_buf_writer_write_u8(w, address_type);
    return st == BT_OK ? bt_buf_writer_write_bytes(w, address, 6) : st;
}

bt_status_t bt_smp_parse_identity_address(const struct bt_smp_command *command,
                                           struct bt_smp_identity_address *out)
{
    if (command == NULL || out == NULL ||
        command->code != BT_SMP_IDENTITY_ADDRESS_INFORMATION || command->data_len != 7 ||
        command->data[0] > 1)
        return BT_ERR_INVALID_ARGUMENT;
    out->address_type = command->data[0];
    out->address = command->data + 1;
    return BT_OK;
}

bt_status_t bt_smp_encode_public_key(struct bt_buf_writer *w, const uint8_t x[32],
                                      const uint8_t y[32])
{
    bt_status_t st;

    if (w == NULL || x == NULL || y == NULL)
        return BT_ERR_INVALID_ARGUMENT;
    st = bt_buf_writer_write_u8(w, BT_SMP_PAIRING_PUBLIC_KEY);
    if (st == BT_OK)
        st = bt_buf_writer_write_bytes(w, x, 32);
    return st == BT_OK ? bt_buf_writer_write_bytes(w, y, 32) : st;
}

bt_status_t bt_smp_parse_public_key(const struct bt_smp_command *command,
                                     const uint8_t **out_x, const uint8_t **out_y)
{
    if (command == NULL || out_x == NULL || out_y == NULL ||
        command->code != BT_SMP_PAIRING_PUBLIC_KEY || command->data_len != 64)
        return BT_ERR_INVALID_ARGUMENT;
    *out_x = command->data;
    *out_y = command->data + 32;
    return BT_OK;
}

bt_status_t bt_smp_encode_u8_command(struct bt_buf_writer *w, uint8_t code, uint8_t value)
{
    bt_status_t st;
    uint8_t pdu[2];
    struct bt_smp_command command;

    pdu[0] = code;
    pdu[1] = value;
    if (w == NULL || bt_smp_parse_command(pdu, sizeof(pdu), &command) != BT_OK)
        return BT_ERR_INVALID_ARGUMENT;
    st = bt_buf_writer_write_u8(w, code);
    return st == BT_OK ? bt_buf_writer_write_u8(w, value) : st;
}
