#include <btcore/smp_crypto.h>

#include <string.h>

static bool args_valid(const struct bt_smp_aes128 *aes)
{
    return aes != NULL && aes->encrypt != NULL;
}

static void cmac_double(const uint8_t in[16], uint8_t out[16])
{
    uint8_t carry = 0;
    size_t i;

    for (i = 16; i-- > 0;)
    {
        uint8_t next_carry = (uint8_t)(in[i] >> 7);

        out[i] = (uint8_t)((in[i] << 1) | carry);
        carry = next_carry;
    }
    if (carry != 0)
        out[15] ^= 0x87u;
}

void bt_smp_cmac_aes128_init(struct bt_smp_cmac_aes128 *context,
                              const struct bt_smp_aes128 *aes)
{
    context->aes = *aes;
}

bt_status_t bt_smp_cmac_aes128_calculate(void *context, const uint8_t key[16],
                                          const uint8_t *message, size_t message_len,
                                          uint8_t mac[16])
{
    struct bt_smp_cmac_aes128 *cmac = context;
    uint8_t zero[16] = {0};
    uint8_t l[16];
    uint8_t k1[16];
    uint8_t k2[16];
    uint8_t x[16] = {0};
    uint8_t block[16];
    size_t block_count;
    size_t full_before_last;
    size_t offset;
    size_t i;
    size_t last_len;
    bt_status_t st;

    if (cmac == NULL || !args_valid(&cmac->aes) || key == NULL || mac == NULL ||
        (message == NULL && message_len != 0))
        return BT_ERR_INVALID_ARGUMENT;
    st = cmac->aes.encrypt(cmac->aes.context, key, zero, l);
    if (st != BT_OK)
        return st;
    cmac_double(l, k1);
    cmac_double(k1, k2);

    block_count = message_len == 0 ? 1 : (message_len + 15) / 16;
    full_before_last = block_count - 1;
    for (offset = 0; offset < full_before_last * 16; offset += 16)
    {
        for (i = 0; i < 16; ++i)
            block[i] = x[i] ^ message[offset + i];
        st = cmac->aes.encrypt(cmac->aes.context, key, block, x);
        if (st != BT_OK)
            return st;
    }

    last_len = message_len - full_before_last * 16;
    memset(block, 0, sizeof(block));
    if (last_len == 16)
    {
        memcpy(block, message + full_before_last * 16, 16);
        for (i = 0; i < 16; ++i)
            block[i] ^= k1[i];
    }
    else
    {
        if (last_len != 0)
            memcpy(block, message + full_before_last * 16, last_len);
        block[last_len] = 0x80u;
        for (i = 0; i < 16; ++i)
            block[i] ^= k2[i];
    }
    for (i = 0; i < 16; ++i)
        block[i] ^= x[i];
    return cmac->aes.encrypt(cmac->aes.context, key, block, mac);
}

bt_status_t bt_smp_crypto_ah(const struct bt_smp_aes128 *aes, const uint8_t irk[16],
                             const uint8_t prand[3], uint8_t hash[3])
{
    uint8_t plaintext[16] = {0};
    uint8_t encrypted[16];
    bt_status_t st;

    if (!args_valid(aes) || irk == NULL || prand == NULL || hash == NULL)
        return BT_ERR_INVALID_ARGUMENT;
    memcpy(plaintext + 13, prand, 3);
    st = aes->encrypt(aes->context, irk, plaintext, encrypted);
    if (st != BT_OK)
        return st;
    memcpy(hash, encrypted + 13, 3);
    return BT_OK;
}

bt_status_t bt_smp_crypto_c1(const struct bt_smp_aes128 *aes, const uint8_t key[16],
                             const uint8_t random[16], const uint8_t preq[7],
                             const uint8_t pres[7], uint8_t initiator_addr_type,
                             uint8_t responder_addr_type, const uint8_t initiator_addr[6],
                             const uint8_t responder_addr[6], uint8_t confirm[16])
{
    uint8_t block[16];
    uint8_t encrypted[16];
    size_t i;
    bt_status_t st;

    if (!args_valid(aes) || key == NULL || random == NULL || preq == NULL || pres == NULL ||
        initiator_addr == NULL || responder_addr == NULL || confirm == NULL ||
        initiator_addr_type > 1 || responder_addr_type > 1)
        return BT_ERR_INVALID_ARGUMENT;

    /* p1 = pres || preq || rat' || iat' */
    memcpy(block, pres, 7);
    memcpy(block + 7, preq, 7);
    block[14] = responder_addr_type;
    block[15] = initiator_addr_type;
    for (i = 0; i < 16; ++i)
        block[i] ^= random[i];
    st = aes->encrypt(aes->context, key, block, encrypted);
    if (st != BT_OK)
        return st;

    /* p2 = padding || ia || ra */
    memcpy(block, encrypted, 16);
    for (i = 0; i < 6; ++i)
        block[4 + i] ^= initiator_addr[i];
    for (i = 0; i < 6; ++i)
        block[10 + i] ^= responder_addr[i];
    return aes->encrypt(aes->context, key, block, confirm);
}

bt_status_t bt_smp_crypto_s1(const struct bt_smp_aes128 *aes, const uint8_t key[16],
                             const uint8_t r1[16], const uint8_t r2[16], uint8_t stk[16])
{
    uint8_t block[16];

    if (!args_valid(aes) || key == NULL || r1 == NULL || r2 == NULL || stk == NULL)
        return BT_ERR_INVALID_ARGUMENT;
    memcpy(block, r1 + 8, 8);
    memcpy(block + 8, r2 + 8, 8);
    return aes->encrypt(aes->context, key, block, stk);
}

static bool cmac_valid(const struct bt_smp_aes_cmac *cmac)
{
    return cmac != NULL && cmac->calculate != NULL;
}

bt_status_t bt_smp_crypto_f4(const struct bt_smp_aes_cmac *cmac, const uint8_t u[32],
                             const uint8_t v[32], const uint8_t x[16], uint8_t z,
                             uint8_t out[16])
{
    uint8_t message[65];

    if (!cmac_valid(cmac) || u == NULL || v == NULL || x == NULL || out == NULL)
        return BT_ERR_INVALID_ARGUMENT;
    memcpy(message, u, 32);
    memcpy(message + 32, v, 32);
    message[64] = z;
    return cmac->calculate(cmac->context, x, message, sizeof(message), out);
}

bt_status_t bt_smp_crypto_f5(const struct bt_smp_aes_cmac *cmac, const uint8_t w[32],
                             const uint8_t n1[16], const uint8_t n2[16],
                             const uint8_t a1[7], const uint8_t a2[7],
                             uint8_t mac_key[16], uint8_t ltk[16])
{
    static const uint8_t salt[16] = {
        0x6C, 0x88, 0x83, 0x91, 0xAA, 0xF5, 0xA5, 0x38,
        0x60, 0x37, 0x0B, 0xDB, 0x5A, 0x60, 0x83, 0xBE};
    uint8_t t[16];
    uint8_t message[53];
    bt_status_t st;

    if (!cmac_valid(cmac) || w == NULL || n1 == NULL || n2 == NULL || a1 == NULL ||
        a2 == NULL || mac_key == NULL || ltk == NULL)
        return BT_ERR_INVALID_ARGUMENT;
    st = cmac->calculate(cmac->context, salt, w, 32, t);
    if (st != BT_OK)
        return st;
    message[0] = 0;
    memcpy(message + 1, "btle", 4);
    memcpy(message + 5, n1, 16);
    memcpy(message + 21, n2, 16);
    memcpy(message + 37, a1, 7);
    memcpy(message + 44, a2, 7);
    message[51] = 0x01;
    message[52] = 0x00;
    st = cmac->calculate(cmac->context, t, message, sizeof(message), mac_key);
    if (st != BT_OK)
        return st;
    message[0] = 1;
    return cmac->calculate(cmac->context, t, message, sizeof(message), ltk);
}

bt_status_t bt_smp_crypto_f6(const struct bt_smp_aes_cmac *cmac, const uint8_t w[16],
                             const uint8_t n1[16], const uint8_t n2[16],
                             const uint8_t r[16], const uint8_t io_cap[3],
                             const uint8_t a1[7], const uint8_t a2[7], uint8_t out[16])
{
    uint8_t message[65];

    if (!cmac_valid(cmac) || w == NULL || n1 == NULL || n2 == NULL || r == NULL ||
        io_cap == NULL || a1 == NULL || a2 == NULL || out == NULL)
        return BT_ERR_INVALID_ARGUMENT;
    memcpy(message, n1, 16);
    memcpy(message + 16, n2, 16);
    memcpy(message + 32, r, 16);
    memcpy(message + 48, io_cap, 3);
    memcpy(message + 51, a1, 7);
    memcpy(message + 58, a2, 7);
    return cmac->calculate(cmac->context, w, message, sizeof(message), out);
}

bt_status_t bt_smp_crypto_g2(const struct bt_smp_aes_cmac *cmac, const uint8_t u[32],
                             const uint8_t v[32], const uint8_t x[16], const uint8_t y[16],
                             uint32_t *out_value)
{
    uint8_t message[80];
    uint8_t mac[16];
    bt_status_t st;

    if (!cmac_valid(cmac) || u == NULL || v == NULL || x == NULL || y == NULL ||
        out_value == NULL)
        return BT_ERR_INVALID_ARGUMENT;
    memcpy(message, u, 32);
    memcpy(message + 32, v, 32);
    memcpy(message + 64, y, 16);
    st = cmac->calculate(cmac->context, x, message, sizeof(message), mac);
    if (st != BT_OK)
        return st;
    *out_value = ((uint32_t)mac[12] << 24) | ((uint32_t)mac[13] << 16) |
                 ((uint32_t)mac[14] << 8) | mac[15];
    return BT_OK;
}

bt_status_t bt_smp_crypto_h6(const struct bt_smp_aes_cmac *cmac, const uint8_t w[16],
                             const uint8_t key_id[4], uint8_t out[16])
{
    if (!cmac_valid(cmac) || w == NULL || key_id == NULL || out == NULL)
        return BT_ERR_INVALID_ARGUMENT;
    return cmac->calculate(cmac->context, w, key_id, 4, out);
}

bt_status_t bt_smp_crypto_h7(const struct bt_smp_aes_cmac *cmac, const uint8_t salt[16],
                             const uint8_t w[16], uint8_t out[16])
{
    if (!cmac_valid(cmac) || salt == NULL || w == NULL || out == NULL)
        return BT_ERR_INVALID_ARGUMENT;
    return cmac->calculate(cmac->context, salt, w, 16, out);
}
