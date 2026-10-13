#include <btcore/smp_manager.h>

#include <string.h>

static void reverse_copy(uint8_t *dst, const uint8_t *src, size_t len)
{
    size_t i;

    for (i = 0; i < len; ++i)
        dst[i] = src[len - 1 - i];
}

static uint64_t timeout_deadline(uint64_t now_us)
{
    return UINT64_MAX - now_us < BT_SMP_TIMEOUT_US ? UINT64_MAX
                                                    : now_us + BT_SMP_TIMEOUT_US;
}

static void finish(struct bt_smp_manager *m, enum bt_smp_manager_result result)
{
    m->deadline_us = 0;
    m->state = result == BT_SMP_MANAGER_OK ? BT_SMP_STATE_COMPLETE : BT_SMP_STATE_FAILED;
    if (m->ops->complete != NULL)
        m->ops->complete(m->context, result, &m->negotiation);
}

static bt_status_t send_pdu(struct bt_smp_manager *m, const uint8_t *pdu, size_t len,
                            uint64_t now_us)
{
    bt_status_t st;

    /* Arm before calling the port: a synchronous test transport (or a small
     * bare-metal loopback) may deliver the response reentrantly. */
    m->deadline_us = timeout_deadline(now_us);
    st = m->ops->send(m->context, pdu, len);
    return st;
}

static void send_failed(struct bt_smp_manager *m, uint8_t reason, uint64_t now_us,
                        enum bt_smp_manager_result result)
{
    uint8_t pdu[2];
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    if (bt_smp_encode_u8_command(&w, BT_SMP_PAIRING_FAILED, reason) == BT_OK)
        (void)send_pdu(m, pdu, bt_buf_writer_len(&w), now_us);
    finish(m, result);
}

static void set_passkey_tk(struct bt_smp_manager *m, uint32_t passkey)
{
    memset(m->tk, 0, sizeof(m->tk));
    m->tk[12] = (uint8_t)(passkey >> 24);
    m->tk[13] = (uint8_t)(passkey >> 16);
    m->tk[14] = (uint8_t)(passkey >> 8);
    m->tk[15] = (uint8_t)passkey;
}

static bool sc_passkey_entry(const struct bt_smp_manager *m)
{
    return m->negotiation.association == BT_SMP_ASSOC_PASSKEY_INITIATOR_DISPLAYS ||
           m->negotiation.association == BT_SMP_ASSOC_PASSKEY_RESPONDER_DISPLAYS ||
           m->negotiation.association == BT_SMP_ASSOC_PASSKEY_BOTH_INPUT;
}

/* the 128-bit r for f6: the passkey for Passkey Entry, zero otherwise */
static const uint8_t *sc_f6_r(const struct bt_smp_manager *m, const uint8_t zero[16])
{
    return sc_passkey_entry(m) ? m->tk : zero;
}

/* z for round i of Passkey Entry: 0x80 | bit i of the passkey (both sides
 * commit to the same passkey, bit by bit, over 20 rounds) */
static uint8_t sc_passkey_z(const struct bt_smp_manager *m)
{
    uint32_t passkey = ((uint32_t)m->tk[12] << 24) | ((uint32_t)m->tk[13] << 16) |
                       ((uint32_t)m->tk[14] << 8) | m->tk[15];
    return (uint8_t)(0x80u | ((passkey >> m->sc_round) & 1u));
}

static void start_confirm_exchange(struct bt_smp_manager *m, uint64_t now_us)
{
    uint8_t confirm[16];
    uint8_t wire_confirm[16];
    uint8_t pdu[17];
    struct bt_buf_writer w;

    if (m->ops->random(m->context, m->local_random, sizeof(m->local_random)) != BT_OK ||
        bt_smp_crypto_c1(&m->aes, m->tk, m->local_random, m->preq, m->pres,
                         m->config.initiator_address_type, m->config.responder_address_type,
                         m->config.initiator_address, m->config.responder_address,
                         confirm) != BT_OK)
    {
        send_failed(m, 0x08, now_us, BT_SMP_MANAGER_ERROR_CRYPTO);
        return;
    }
    reverse_copy(wire_confirm, confirm, sizeof(confirm));
    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    m->state = BT_SMP_STATE_WAIT_PAIRING_CONFIRM;
    if (bt_smp_encode_value128(&w, BT_SMP_PAIRING_CONFIRM, wire_confirm) != BT_OK ||
        send_pdu(m, pdu, bt_buf_writer_len(&w), now_us) != BT_OK)
    {
        finish(m, BT_SMP_MANAGER_ERROR_PROTOCOL);
        return;
    }
}

/* LE Secure Connections Passkey Entry, one of the 20 rounds: fresh Nai,
 * Cai = f4(PKa, PKb, Nai, rai) sent as Pairing Confirm; the responder answers
 * with its own commitment, then the randoms are exchanged (handle_sc_confirm
 * / handle_sc_random) and the responder's commitment is verified. */
static void start_sc_passkey_round(struct bt_smp_manager *m, uint64_t now_us)
{
    uint8_t confirm[16];
    uint8_t wire[16];
    uint8_t pdu[17];
    struct bt_buf_writer w;

    if (m->ops->random(m->context, m->local_random, sizeof(m->local_random)) != BT_OK ||
        bt_smp_crypto_f4(&m->cmac, m->local_public_x, m->peer_public_x, m->local_random,
                         sc_passkey_z(m), confirm) != BT_OK)
    {
        send_failed(m, 0x08, now_us, BT_SMP_MANAGER_ERROR_CRYPTO);
        return;
    }
    reverse_copy(wire, confirm, 16);
    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    m->state = BT_SMP_STATE_WAIT_SC_CONFIRM;
    m->deadline_us = timeout_deadline(now_us);
    if (bt_smp_encode_value128(&w, BT_SMP_PAIRING_CONFIRM, wire) != BT_OK ||
        send_pdu(m, pdu, bt_buf_writer_len(&w), now_us) != BT_OK)
        finish(m, BT_SMP_MANAGER_ERROR_PROTOCOL);
}

static void start_sc_stage2(struct bt_smp_manager *m, uint64_t now_us)
{
    uint8_t a1[7];
    uint8_t a2[7];
    uint8_t io_cap[3];
    uint8_t zero[16] = {0};
    uint8_t check[16];
    uint8_t wire[16];
    uint8_t pdu[17];
    struct bt_buf_writer w;
    size_t clear;

    a1[0] = m->config.initiator_address_type;
    memcpy(a1 + 1, m->config.initiator_address, 6);
    a2[0] = m->config.responder_address_type;
    memcpy(a2 + 1, m->config.responder_address, 6);
    if (bt_smp_crypto_f5(&m->cmac, m->dhkey, m->local_random, m->peer_random,
                         a1, a2, m->mac_key, m->stk) != BT_OK)
        goto crypto_error;
    io_cap[0] = m->config.features.auth_req;
    io_cap[1] = m->config.features.oob_data_flag;
    io_cap[2] = m->config.features.io_capability;
    if (bt_smp_crypto_f6(&m->cmac, m->mac_key, m->local_random, m->peer_random,
                         sc_f6_r(m, zero), io_cap, a1, a2, check) != BT_OK)
        goto crypto_error;
    clear = 16u - m->negotiation.encryption_key_size;
    memset(m->stk, 0, clear);
    reverse_copy(wire, check, 16);
    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    m->state = BT_SMP_STATE_WAIT_DHKEY_CHECK;
    if (bt_smp_encode_value128(&w, BT_SMP_PAIRING_DHKEY_CHECK, wire) != BT_OK ||
        send_pdu(m, pdu, bt_buf_writer_len(&w), now_us) != BT_OK)
        finish(m, BT_SMP_MANAGER_ERROR_PROTOCOL);
    return;

crypto_error:
    send_failed(m, 0x08, now_us, BT_SMP_MANAGER_ERROR_CRYPTO);
}

static void start_secure_connections(struct bt_smp_manager *m, uint64_t now_us)
{
    if (m->negotiation.association != BT_SMP_ASSOC_JUST_WORKS &&
        m->negotiation.association != BT_SMP_ASSOC_NUMERIC_COMPARISON &&
        !sc_passkey_entry(m))
    {
        send_failed(m, 0x07, now_us, BT_SMP_MANAGER_ERROR_UNSUPPORTED); /* OOB */
        return;
    }
    m->sc_round = 0;
    if (m->cmac.calculate == NULL || m->ops->generate_public_key == NULL ||
        m->ops->generate_dhkey == NULL)
    {
        send_failed(m, 0x07, now_us, BT_SMP_MANAGER_ERROR_UNSUPPORTED);
        return;
    }
    m->state = BT_SMP_STATE_WAIT_LOCAL_PUBLIC_KEY;
    m->deadline_us = timeout_deadline(now_us);
    if (m->ops->generate_public_key(m->context) != BT_OK)
        finish(m, BT_SMP_MANAGER_ERROR_CRYPTO);
}

static void handle_pairing_response(struct bt_smp_manager *m,
                                     const struct bt_smp_command *command,
                                     const uint8_t *raw_pdu, uint64_t now_us)
{
    struct bt_smp_pairing_features response;
    uint8_t random_bytes[4];
    uint32_t passkey;

    if (m->state != BT_SMP_STATE_WAIT_PAIRING_RESPONSE ||
        bt_smp_parse_pairing_features(command, &response) != BT_OK ||
        bt_smp_negotiate_pairing(&m->config.features, &response, &m->negotiation) != BT_OK)
    {
        send_failed(m, 0x0A, now_us, BT_SMP_MANAGER_ERROR_PROTOCOL);
        return;
    }
    reverse_copy(m->pres, raw_pdu, 7);
    if (m->negotiation.secure_connections)
    {
        /* EncKey is ignored for LE Secure Connections; the LTK is derived
         * by f5 rather than distributed. */
        m->negotiation.initiator_key_distribution &=
            (uint8_t)~BT_SMP_KEYDIST_ENC_KEY;
        m->negotiation.responder_key_distribution &=
            (uint8_t)~BT_SMP_KEYDIST_ENC_KEY;
        start_secure_connections(m, now_us);
        return;
    }
    if (((m->negotiation.initiator_key_distribution |
          m->negotiation.responder_key_distribution) &
         BT_SMP_KEYDIST_LINK_KEY) != 0)
    {
        send_failed(m, 0x07, now_us, BT_SMP_MANAGER_ERROR_UNSUPPORTED);
        return;
    }

    switch (m->negotiation.association)
    {
    case BT_SMP_ASSOC_JUST_WORKS:
        memset(m->tk, 0, sizeof(m->tk));
        start_confirm_exchange(m, now_us);
        break;
    case BT_SMP_ASSOC_PASSKEY_INITIATOR_DISPLAYS:
        if (m->ops->random(m->context, random_bytes, sizeof(random_bytes)) != BT_OK)
        {
            send_failed(m, 0x08, now_us, BT_SMP_MANAGER_ERROR_CRYPTO);
            return;
        }
        passkey = (((uint32_t)random_bytes[0] << 24) |
                   ((uint32_t)random_bytes[1] << 16) |
                   ((uint32_t)random_bytes[2] << 8) | random_bytes[3]) %
                  1000000u;
        set_passkey_tk(m, passkey);
        if (m->ops->user_action != NULL)
            m->ops->user_action(m->context, BT_SMP_USER_DISPLAY_PASSKEY, passkey);
        start_confirm_exchange(m, now_us);
        break;
    case BT_SMP_ASSOC_PASSKEY_RESPONDER_DISPLAYS:
    case BT_SMP_ASSOC_PASSKEY_BOTH_INPUT:
        m->state = BT_SMP_STATE_WAIT_TK;
        if (m->ops->user_action != NULL)
            m->ops->user_action(m->context, BT_SMP_USER_REQUEST_PASSKEY, 0);
        break;
    case BT_SMP_ASSOC_OOB:
        m->state = BT_SMP_STATE_WAIT_TK;
        if (m->ops->user_action != NULL)
            m->ops->user_action(m->context, BT_SMP_USER_REQUEST_OOB_TK, 0);
        break;
    case BT_SMP_ASSOC_NUMERIC_COMPARISON:
        send_failed(m, 0x07, now_us, BT_SMP_MANAGER_ERROR_UNSUPPORTED);
        break;
    }
}

static void handle_confirm(struct bt_smp_manager *m, const struct bt_smp_command *command,
                           uint64_t now_us)
{
    const uint8_t *wire;
    uint8_t wire_random[16];
    uint8_t pdu[17];
    struct bt_buf_writer w;

    if (m->state != BT_SMP_STATE_WAIT_PAIRING_CONFIRM ||
        bt_smp_parse_value128(command, &wire) != BT_OK)
    {
        send_failed(m, 0x0A, now_us, BT_SMP_MANAGER_ERROR_PROTOCOL);
        return;
    }
    reverse_copy(m->peer_confirm, wire, 16);
    reverse_copy(wire_random, m->local_random, 16);
    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    m->state = BT_SMP_STATE_WAIT_PAIRING_RANDOM;
    if (bt_smp_encode_value128(&w, BT_SMP_PAIRING_RANDOM, wire_random) != BT_OK ||
        send_pdu(m, pdu, bt_buf_writer_len(&w), now_us) != BT_OK)
    {
        finish(m, BT_SMP_MANAGER_ERROR_PROTOCOL);
        return;
    }
}

static void handle_responder_confirm(struct bt_smp_manager *m,
                                     const struct bt_smp_command *command,
                                     uint64_t now_us)
{
    const uint8_t *wire;
    uint8_t confirm[16], wire_confirm[16], pdu[17];
    struct bt_buf_writer w;

    if (m->state != BT_SMP_STATE_WAIT_PAIRING_CONFIRM ||
        bt_smp_parse_value128(command, &wire) != BT_OK)
        goto invalid;
    reverse_copy(m->peer_confirm, wire, 16);
    if (m->ops->random(m->context, m->local_random, 16) != BT_OK ||
        bt_smp_crypto_c1(&m->aes, m->tk, m->local_random, m->preq, m->pres,
                         m->config.initiator_address_type,
                         m->config.responder_address_type,
                         m->config.initiator_address,
                         m->config.responder_address, confirm) != BT_OK)
        goto crypto;
    reverse_copy(wire_confirm, confirm, 16);
    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    m->state = BT_SMP_STATE_WAIT_PAIRING_RANDOM;
    if (bt_smp_encode_value128(&w, BT_SMP_PAIRING_CONFIRM, wire_confirm) != BT_OK ||
        send_pdu(m, pdu, bt_buf_writer_len(&w), now_us) != BT_OK)
        finish(m, BT_SMP_MANAGER_ERROR_PROTOCOL);
    return;
crypto:
    send_failed(m, 0x08, now_us, BT_SMP_MANAGER_ERROR_CRYPTO);
    return;
invalid:
    send_failed(m, 0x0A, now_us, BT_SMP_MANAGER_ERROR_PROTOCOL);
}

static void handle_responder_random(struct bt_smp_manager *m,
                                    const struct bt_smp_command *command,
                                    uint64_t now_us)
{
    const uint8_t *wire;
    uint8_t peer_random[16], expected[16], wire_random[16], pdu[17];
    struct bt_buf_writer w;
    size_t clear;

    if (m->state != BT_SMP_STATE_WAIT_PAIRING_RANDOM ||
        bt_smp_parse_value128(command, &wire) != BT_OK)
        goto invalid;
    reverse_copy(peer_random, wire, 16);
    if (bt_smp_crypto_c1(&m->aes, m->tk, peer_random, m->preq, m->pres,
                         m->config.initiator_address_type,
                         m->config.responder_address_type,
                         m->config.initiator_address,
                         m->config.responder_address, expected) != BT_OK)
        goto crypto;
    if (memcmp(expected, m->peer_confirm, 16) != 0) {
        send_failed(m, 0x04, now_us, BT_SMP_MANAGER_ERROR_CONFIRM);
        return;
    }
    if (bt_smp_crypto_s1(&m->aes, m->tk, m->local_random, peer_random,
                         m->stk) != BT_OK)
        goto crypto;
    clear = 16u - m->negotiation.encryption_key_size;
    memset(m->stk, 0, clear);
    reverse_copy(wire_random, m->local_random, 16);
    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    if (bt_smp_encode_value128(&w, BT_SMP_PAIRING_RANDOM, wire_random) != BT_OK ||
        send_pdu(m, pdu, bt_buf_writer_len(&w), now_us) != BT_OK) {
        finish(m, BT_SMP_MANAGER_ERROR_PROTOCOL);
        return;
    }
    m->state = BT_SMP_STATE_WAIT_ENCRYPTION;
    m->deadline_us = timeout_deadline(now_us);
    if (m->ops->start_encryption(m->context, m->stk,
                                  m->negotiation.encryption_key_size) != BT_OK)
        finish(m, BT_SMP_MANAGER_ERROR_ENCRYPTION);
    return;
crypto:
    send_failed(m, 0x08, now_us, BT_SMP_MANAGER_ERROR_CRYPTO);
    return;
invalid:
    send_failed(m, 0x0A, now_us, BT_SMP_MANAGER_ERROR_PROTOCOL);
}

static void handle_random(struct bt_smp_manager *m, const struct bt_smp_command *command,
                          uint64_t now_us)
{
    const uint8_t *wire;
    uint8_t peer_random[16];
    uint8_t expected_confirm[16];
    size_t clear;

    if (m->state != BT_SMP_STATE_WAIT_PAIRING_RANDOM ||
        bt_smp_parse_value128(command, &wire) != BT_OK)
    {
        send_failed(m, 0x0A, now_us, BT_SMP_MANAGER_ERROR_PROTOCOL);
        return;
    }
    reverse_copy(peer_random, wire, 16);
    if (bt_smp_crypto_c1(&m->aes, m->tk, peer_random, m->preq, m->pres,
                         m->config.initiator_address_type, m->config.responder_address_type,
                         m->config.initiator_address, m->config.responder_address,
                         expected_confirm) != BT_OK)
    {
        send_failed(m, 0x08, now_us, BT_SMP_MANAGER_ERROR_CRYPTO);
        return;
    }
    if (memcmp(expected_confirm, m->peer_confirm, 16) != 0)
    {
        send_failed(m, 0x04, now_us, BT_SMP_MANAGER_ERROR_CONFIRM);
        return;
    }
    if (bt_smp_crypto_s1(&m->aes, m->tk, peer_random, m->local_random, m->stk) != BT_OK)
    {
        send_failed(m, 0x08, now_us, BT_SMP_MANAGER_ERROR_CRYPTO);
        return;
    }
    clear = 16u - m->negotiation.encryption_key_size;
    memset(m->stk, 0, clear); /* mask most-significant octets */
    m->state = BT_SMP_STATE_WAIT_ENCRYPTION;
    m->deadline_us = timeout_deadline(now_us);
    if (m->ops->start_encryption(m->context, m->stk,
                                  m->negotiation.encryption_key_size) != BT_OK)
    {
        finish(m, BT_SMP_MANAGER_ERROR_ENCRYPTION);
        return;
    }
}

static void handle_sc_public_key(struct bt_smp_manager *m,
                                 const struct bt_smp_command *command,
                                 uint64_t now_us)
{
    const uint8_t *wire_x;
    const uint8_t *wire_y;

    if (m->state != BT_SMP_STATE_WAIT_PEER_PUBLIC_KEY ||
        bt_smp_parse_public_key(command, &wire_x, &wire_y) != BT_OK)
        goto invalid;
    reverse_copy(m->peer_public_x, wire_x, 32);
    reverse_copy(m->peer_public_y, wire_y, 32);
    if (m->responder)
    {
        /* answer with our key once the controller has produced it */
        m->state = BT_SMP_STATE_WAIT_LOCAL_PUBLIC_KEY;
        m->deadline_us = timeout_deadline(now_us);
        if (m->ops->generate_public_key(m->context) != BT_OK)
            finish(m, BT_SMP_MANAGER_ERROR_CRYPTO);
        return;
    }
    if (memcmp(m->local_public_x, m->peer_public_x, 32) == 0)
        goto invalid;
    m->state = BT_SMP_STATE_WAIT_DHKEY;
    m->deadline_us = timeout_deadline(now_us);
    if (m->ops->generate_dhkey(m->context, m->peer_public_x,
                                m->peer_public_y) != BT_OK)
        finish(m, BT_SMP_MANAGER_ERROR_CRYPTO);
    return;
invalid:
    send_failed(m, 0x0A, now_us, BT_SMP_MANAGER_ERROR_PROTOCOL);
}

static void send_sc_random(struct bt_smp_manager *m, uint64_t now_us)
{
    uint8_t wire_random[16];
    uint8_t pdu[17];
    struct bt_buf_writer w;

    reverse_copy(wire_random, m->local_random, 16);
    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    m->state = BT_SMP_STATE_WAIT_SC_RANDOM;
    if (bt_smp_encode_value128(&w, BT_SMP_PAIRING_RANDOM, wire_random) != BT_OK ||
        send_pdu(m, pdu, bt_buf_writer_len(&w), now_us) != BT_OK)
        finish(m, BT_SMP_MANAGER_ERROR_PROTOCOL);
}

static void handle_sc_confirm(struct bt_smp_manager *m,
                              const struct bt_smp_command *command,
                              uint64_t now_us)
{
    const uint8_t *wire;

    if (bt_smp_parse_value128(command, &wire) != BT_OK)
        goto invalid;
    if (m->state == BT_SMP_STATE_WAIT_SC_CONFIRM)
    {
        reverse_copy(m->peer_confirm, wire, 16);
        send_sc_random(m, now_us);
        return;
    }
    /* Just Works / Numeric Comparison: the responder may commit before our
     * controller has reported the DHKey; answer once it has. */
    if (m->state == BT_SMP_STATE_WAIT_DHKEY && !sc_passkey_entry(m) &&
        !m->peer_confirm_pending)
    {
        reverse_copy(m->peer_confirm, wire, 16);
        m->peer_confirm_pending = true;
        return;
    }
invalid:
    send_failed(m, 0x0A, now_us, BT_SMP_MANAGER_ERROR_PROTOCOL);
}

static void handle_sc_random(struct bt_smp_manager *m,
                             const struct bt_smp_command *command,
                             uint64_t now_us)
{
    const uint8_t *wire;
    uint8_t expected[16];

    if (m->state != BT_SMP_STATE_WAIT_SC_RANDOM ||
        bt_smp_parse_value128(command, &wire) != BT_OK)
        goto invalid;
    reverse_copy(m->peer_random, wire, 16);
    if (bt_smp_crypto_f4(&m->cmac, m->peer_public_x, m->local_public_x,
                         m->peer_random, sc_passkey_entry(m) ? sc_passkey_z(m) : 0,
                         expected) != BT_OK)
    {
        send_failed(m, 0x08, now_us, BT_SMP_MANAGER_ERROR_CRYPTO);
        return;
    }
    if (memcmp(expected, m->peer_confirm, 16) != 0)
    {
        send_failed(m, 0x04, now_us, BT_SMP_MANAGER_ERROR_CONFIRM);
        return;
    }
    if (sc_passkey_entry(m) && ++m->sc_round < 20)
    {
        start_sc_passkey_round(m, now_us);   /* next bit; the 20th random pair feeds f5/f6 */
        return;
    }
    if (m->negotiation.association == BT_SMP_ASSOC_NUMERIC_COMPARISON)
    {
        if (bt_smp_crypto_g2(&m->cmac, m->local_public_x, m->peer_public_x,
                             m->local_random, m->peer_random,
                             &m->numeric_value) != BT_OK)
        {
            send_failed(m, 0x08, now_us, BT_SMP_MANAGER_ERROR_CRYPTO);
            return;
        }
        m->numeric_value %= 1000000u;
        m->state = BT_SMP_STATE_WAIT_NUMERIC_CONFIRMATION;
        if (m->ops->user_action != NULL)
            m->ops->user_action(m->context, BT_SMP_USER_CONFIRM_NUMERIC,
                                m->numeric_value);
        return;
    }
    start_sc_stage2(m, now_us);
    return;
invalid:
    send_failed(m, 0x0A, now_us, BT_SMP_MANAGER_ERROR_PROTOCOL);
}

static void handle_sc_dhkey_check(struct bt_smp_manager *m,
                                   const struct bt_smp_command *command,
                                   uint64_t now_us)
{
    const uint8_t *wire;
    uint8_t a1[7];
    uint8_t a2[7];
    uint8_t io_cap[3];
    uint8_t zero[16] = {0};
    uint8_t expected[16];

    if (m->state != BT_SMP_STATE_WAIT_DHKEY_CHECK ||
        bt_smp_parse_value128(command, &wire) != BT_OK)
        goto invalid;
    reverse_copy(m->peer_dhkey_check, wire, 16);
    a1[0] = m->config.initiator_address_type;
    memcpy(a1 + 1, m->config.initiator_address, 6);
    a2[0] = m->config.responder_address_type;
    memcpy(a2 + 1, m->config.responder_address, 6);
    io_cap[0] = m->pres[3];
    io_cap[1] = m->pres[4];
    io_cap[2] = m->pres[5];
    if (bt_smp_crypto_f6(&m->cmac, m->mac_key, m->peer_random, m->local_random,
                         sc_f6_r(m, zero), io_cap, a2, a1, expected) != BT_OK)
    {
        send_failed(m, 0x08, now_us, BT_SMP_MANAGER_ERROR_CRYPTO);
        return;
    }
    if (memcmp(expected, m->peer_dhkey_check, 16) != 0)
    {
        send_failed(m, 0x0B, now_us, BT_SMP_MANAGER_ERROR_CONFIRM);
        return;
    }
    m->state = BT_SMP_STATE_WAIT_ENCRYPTION;
    m->deadline_us = timeout_deadline(now_us);
    if (m->ops->start_encryption(m->context, m->stk,
                                  m->negotiation.encryption_key_size) != BT_OK)
        finish(m, BT_SMP_MANAGER_ERROR_ENCRYPTION);
    return;
invalid:
    send_failed(m, 0x0A, now_us, BT_SMP_MANAGER_ERROR_PROTOCOL);
}

/* LE Secure Connections, responder role. The initiator sends its public key
 * first; the responder answers with its own, commits with Cb, and (Passkey
 * Entry) verifies one initiator commitment per round. Ea is checked before Eb
 * is sent, and only after the local user accepted Numeric Comparison. */
static void sc_addresses(const struct bt_smp_manager *m, uint8_t a1[7], uint8_t a2[7])
{
    a1[0] = m->config.initiator_address_type;
    memcpy(a1 + 1, m->config.initiator_address, 6);
    a2[0] = m->config.responder_address_type;
    memcpy(a2 + 1, m->config.responder_address, 6);
}

static bool responder_sc_derive_keys(struct bt_smp_manager *m, uint64_t now_us)
{
    uint8_t a1[7];
    uint8_t a2[7];

    sc_addresses(m, a1, a2);
    if (bt_smp_crypto_f5(&m->cmac, m->dhkey, m->peer_random, m->local_random,
                         a1, a2, m->mac_key, m->stk) != BT_OK)
    {
        send_failed(m, 0x08, now_us, BT_SMP_MANAGER_ERROR_CRYPTO);
        return false;
    }
    memset(m->stk, 0, 16u - m->negotiation.encryption_key_size);
    return true;
}

static void responder_sc_send_confirm(struct bt_smp_manager *m, uint64_t now_us)
{
    uint8_t confirm[16];
    uint8_t wire[16];
    uint8_t pdu[17];
    struct bt_buf_writer w;

    if (m->ops->random(m->context, m->local_random, sizeof(m->local_random)) != BT_OK ||
        bt_smp_crypto_f4(&m->cmac, m->local_public_x, m->peer_public_x, m->local_random,
                         sc_passkey_entry(m) ? sc_passkey_z(m) : 0, confirm) != BT_OK)
    {
        send_failed(m, 0x08, now_us, BT_SMP_MANAGER_ERROR_CRYPTO);
        return;
    }
    reverse_copy(wire, confirm, 16);
    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    m->state = BT_SMP_STATE_WAIT_SC_RANDOM;
    if (bt_smp_encode_value128(&w, BT_SMP_PAIRING_CONFIRM, wire) != BT_OK ||
        send_pdu(m, pdu, bt_buf_writer_len(&w), now_us) != BT_OK)
        finish(m, BT_SMP_MANAGER_ERROR_PROTOCOL);
}

/* Passkey Entry: the passkey is known, so the rounds can run. */
static void responder_sc_passkey_ready(struct bt_smp_manager *m, uint64_t now_us)
{
    m->state = BT_SMP_STATE_WAIT_SC_CONFIRM;
    m->deadline_us = timeout_deadline(now_us);
    if (m->peer_confirm_pending)
    {
        m->peer_confirm_pending = false;
        responder_sc_send_confirm(m, now_us);
    }
}

static void responder_sc_on_confirm(struct bt_smp_manager *m,
                                    const struct bt_smp_command *command,
                                    uint64_t now_us)
{
    const uint8_t *wire;

    if (!sc_passkey_entry(m) || bt_smp_parse_value128(command, &wire) != BT_OK)
        goto invalid;
    if (m->state == BT_SMP_STATE_WAIT_SC_CONFIRM)
    {
        reverse_copy(m->peer_confirm, wire, 16);
        responder_sc_send_confirm(m, now_us);
        return;
    }
    /* the initiator may commit to round 0 while our user is still typing */
    if ((m->state == BT_SMP_STATE_WAIT_DHKEY || m->state == BT_SMP_STATE_WAIT_TK) &&
        m->sc_round == 0 && !m->peer_confirm_pending)
    {
        reverse_copy(m->peer_confirm, wire, 16);
        m->peer_confirm_pending = true;
        return;
    }
invalid:
    send_failed(m, 0x0A, now_us, BT_SMP_MANAGER_ERROR_PROTOCOL);
}

static void responder_sc_on_random(struct bt_smp_manager *m,
                                   const struct bt_smp_command *command,
                                   uint64_t now_us)
{
    const uint8_t *wire;
    uint8_t expected[16];
    uint8_t wire_random[16];
    uint8_t pdu[17];
    struct bt_buf_writer w;
    bool numeric = m->negotiation.association == BT_SMP_ASSOC_NUMERIC_COMPARISON;

    if (m->state != BT_SMP_STATE_WAIT_SC_RANDOM ||
        bt_smp_parse_value128(command, &wire) != BT_OK)
        goto invalid;
    reverse_copy(m->peer_random, wire, 16);
    if (sc_passkey_entry(m))
    {
        if (bt_smp_crypto_f4(&m->cmac, m->peer_public_x, m->local_public_x,
                             m->peer_random, sc_passkey_z(m), expected) != BT_OK)
            goto crypto;
        if (memcmp(expected, m->peer_confirm, 16) != 0)
        {
            send_failed(m, 0x04, now_us, BT_SMP_MANAGER_ERROR_CONFIRM);
            return;
        }
    }
    /* Pick the next state before Nb leaves: a synchronous transport may
     * deliver the initiator's next PDU reentrantly. */
    if (sc_passkey_entry(m) && ++m->sc_round < 20)
        m->state = BT_SMP_STATE_WAIT_SC_CONFIRM;
    else if (numeric)
    {
        if (bt_smp_crypto_g2(&m->cmac, m->peer_public_x, m->local_public_x,
                             m->peer_random, m->local_random,
                             &m->numeric_value) != BT_OK)
            goto crypto;
        m->numeric_value %= 1000000u;
        m->state = BT_SMP_STATE_WAIT_NUMERIC_CONFIRMATION;
    }
    else
    {
        if (!responder_sc_derive_keys(m, now_us))
            return;
        m->state = BT_SMP_STATE_WAIT_DHKEY_CHECK;
    }
    reverse_copy(wire_random, m->local_random, 16);
    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    if (bt_smp_encode_value128(&w, BT_SMP_PAIRING_RANDOM, wire_random) != BT_OK ||
        send_pdu(m, pdu, bt_buf_writer_len(&w), now_us) != BT_OK)
    {
        finish(m, BT_SMP_MANAGER_ERROR_PROTOCOL);
        return;
    }
    if (numeric && m->state == BT_SMP_STATE_WAIT_NUMERIC_CONFIRMATION &&
        m->ops->user_action != NULL)
        m->ops->user_action(m->context, BT_SMP_USER_CONFIRM_NUMERIC, m->numeric_value);
    return;
crypto:
    send_failed(m, 0x08, now_us, BT_SMP_MANAGER_ERROR_CRYPTO);
    return;
invalid:
    send_failed(m, 0x0A, now_us, BT_SMP_MANAGER_ERROR_PROTOCOL);
}

static void responder_sc_check_and_reply(struct bt_smp_manager *m, uint64_t now_us)
{
    uint8_t a1[7];
    uint8_t a2[7];
    uint8_t io_cap[3];
    uint8_t zero[16] = {0};
    uint8_t value[16];
    uint8_t wire[16];
    uint8_t pdu[17];
    struct bt_buf_writer w;

    sc_addresses(m, a1, a2);
    io_cap[0] = m->preq[3];
    io_cap[1] = m->preq[4];
    io_cap[2] = m->preq[5];
    if (bt_smp_crypto_f6(&m->cmac, m->mac_key, m->peer_random, m->local_random,
                         sc_f6_r(m, zero), io_cap, a1, a2, value) != BT_OK)
        goto crypto;
    if (memcmp(value, m->peer_dhkey_check, 16) != 0)
    {
        send_failed(m, 0x0B, now_us, BT_SMP_MANAGER_ERROR_CONFIRM);
        return;
    }
    io_cap[0] = m->pres[3];
    io_cap[1] = m->pres[4];
    io_cap[2] = m->pres[5];
    if (bt_smp_crypto_f6(&m->cmac, m->mac_key, m->local_random, m->peer_random,
                         sc_f6_r(m, zero), io_cap, a2, a1, value) != BT_OK)
        goto crypto;
    /* Hand the LTK to the port before Eb lets the central start encryption. */
    m->state = BT_SMP_STATE_WAIT_ENCRYPTION;
    m->deadline_us = timeout_deadline(now_us);
    if (m->ops->start_encryption(m->context, m->stk,
                                  m->negotiation.encryption_key_size) != BT_OK)
    {
        finish(m, BT_SMP_MANAGER_ERROR_ENCRYPTION);
        return;
    }
    reverse_copy(wire, value, 16);
    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    if (bt_smp_encode_value128(&w, BT_SMP_PAIRING_DHKEY_CHECK, wire) != BT_OK ||
        send_pdu(m, pdu, bt_buf_writer_len(&w), now_us) != BT_OK)
        finish(m, BT_SMP_MANAGER_ERROR_PROTOCOL);
    return;
crypto:
    send_failed(m, 0x08, now_us, BT_SMP_MANAGER_ERROR_CRYPTO);
}

static void responder_sc_on_dhkey_check(struct bt_smp_manager *m,
                                        const struct bt_smp_command *command,
                                        uint64_t now_us)
{
    const uint8_t *wire;

    if (bt_smp_parse_value128(command, &wire) != BT_OK)
        goto invalid;
    if (m->state == BT_SMP_STATE_WAIT_DHKEY_CHECK)
    {
        reverse_copy(m->peer_dhkey_check, wire, 16);
        responder_sc_check_and_reply(m, now_us);
        return;
    }
    /* the initiator's user may accept the comparison before ours does */
    if (m->state == BT_SMP_STATE_WAIT_NUMERIC_CONFIRMATION && !m->peer_dhkey_check_pending)
    {
        reverse_copy(m->peer_dhkey_check, wire, 16);
        m->peer_dhkey_check_pending = true;
        return;
    }
invalid:
    send_failed(m, 0x0A, now_us, BT_SMP_MANAGER_ERROR_PROTOCOL);
}

static bool send_key_distribution(struct bt_smp_manager *m, uint64_t now_us)
{
    uint8_t pdu[17];
    struct bt_buf_writer w;
    uint8_t mask = m->responder ? m->negotiation.responder_key_distribution
                                : m->negotiation.initiator_key_distribution;

    memset(&m->local_keys, 0, sizeof(m->local_keys));
    if (mask != 0 &&
        (m->ops->get_local_keys == NULL ||
         m->ops->get_local_keys(m->context, mask, &m->local_keys) != BT_OK ||
         (m->local_keys.key_mask & mask) != mask))
        return false;
    if (m->negotiation.secure_connections) {
        m->local_keys.key_mask |= BT_SMP_KEYDIST_ENC_KEY;
        memcpy(m->local_keys.ltk, m->stk, sizeof(m->local_keys.ltk));
    }

#define SEND_VALUE(code, value)                                      \
    do                                                               \
    {                                                                \
        bt_buf_writer_init(&w, pdu, sizeof(pdu));                     \
        if (bt_smp_encode_value128(&w, (code), (value)) != BT_OK ||   \
            send_pdu(m, pdu, bt_buf_writer_len(&w), now_us) != BT_OK) \
            return false;                                             \
    } while (0)
    if ((mask & BT_SMP_KEYDIST_ENC_KEY) != 0)
    {
        SEND_VALUE(BT_SMP_ENCRYPTION_INFORMATION, m->local_keys.ltk);
        bt_buf_writer_init(&w, pdu, sizeof(pdu));
        if (bt_smp_encode_central_identification(&w, m->local_keys.rand,
                                                  m->local_keys.ediv) != BT_OK ||
            send_pdu(m, pdu, bt_buf_writer_len(&w), now_us) != BT_OK)
            return false;
    }
    if ((mask & BT_SMP_KEYDIST_ID_KEY) != 0)
    {
        SEND_VALUE(BT_SMP_IDENTITY_INFORMATION, m->local_keys.irk);
        bt_buf_writer_init(&w, pdu, sizeof(pdu));
        if (bt_smp_encode_identity_address(&w, m->local_keys.identity_address_type,
                                            m->local_keys.identity_address) != BT_OK ||
            send_pdu(m, pdu, bt_buf_writer_len(&w), now_us) != BT_OK)
            return false;
    }
    if ((mask & BT_SMP_KEYDIST_SIGN_KEY) != 0)
        SEND_VALUE(BT_SMP_SIGNING_INFORMATION, m->local_keys.csrk);
#undef SEND_VALUE
    return true;
}

static void finish_key_distribution(struct bt_smp_manager *m, uint64_t now_us)
{
    /* Phase 3: the responder distributes its keys first; the initiator
     * answers with its own once it has received all of them. */
    if (!m->responder && !send_key_distribution(m, now_us))
    {
        finish(m, BT_SMP_MANAGER_ERROR_CRYPTO);
        return;
    }
    if (m->ops->keys_complete != NULL)
        m->ops->keys_complete(m->context, &m->peer_keys, &m->local_keys);
    finish(m, BT_SMP_MANAGER_OK);
}

static void handle_distributed_key(struct bt_smp_manager *m,
                                    const struct bt_smp_command *command,
                                    uint64_t now_us)
{
    const uint8_t *value;
    struct bt_smp_central_identification central;
    struct bt_smp_identity_address identity;
    uint8_t expected = m->responder ? m->negotiation.initiator_key_distribution
                                    : m->negotiation.responder_key_distribution;

    if (m->state != BT_SMP_STATE_WAIT_KEY_DISTRIBUTION)
        goto invalid;
    /* Legacy key distribution order is normative: EncInfo+CentralID,
     * IdentityInfo+IdentityAddress, then SigningInfo. */
    if ((expected & BT_SMP_KEYDIST_ENC_KEY) != 0 &&
        (m->peer_keys_complete_mask & BT_SMP_KEYDIST_ENC_KEY) == 0)
    {
        if ((!m->peer_ltk_received && command->code != BT_SMP_ENCRYPTION_INFORMATION) ||
            (m->peer_ltk_received && command->code != BT_SMP_CENTRAL_IDENTIFICATION))
            goto invalid;
    }
    else if ((expected & BT_SMP_KEYDIST_ID_KEY) != 0 &&
             (m->peer_keys_complete_mask & BT_SMP_KEYDIST_ID_KEY) == 0)
    {
        if ((!m->peer_irk_received && command->code != BT_SMP_IDENTITY_INFORMATION) ||
            (m->peer_irk_received &&
             command->code != BT_SMP_IDENTITY_ADDRESS_INFORMATION))
            goto invalid;
    }
    else if ((expected & BT_SMP_KEYDIST_SIGN_KEY) != 0 &&
             (m->peer_keys_complete_mask & BT_SMP_KEYDIST_SIGN_KEY) == 0 &&
             command->code != BT_SMP_SIGNING_INFORMATION)
        goto invalid;
    switch (command->code)
    {
    case BT_SMP_ENCRYPTION_INFORMATION:
        if ((expected & BT_SMP_KEYDIST_ENC_KEY) == 0 || m->peer_ltk_received ||
            bt_smp_parse_value128(command, &value) != BT_OK)
            goto invalid;
        memcpy(m->peer_keys.ltk, value, 16);
        m->peer_ltk_received = true;
        break;
    case BT_SMP_CENTRAL_IDENTIFICATION:
        if (!m->peer_ltk_received ||
            (m->peer_keys_complete_mask & BT_SMP_KEYDIST_ENC_KEY) != 0 ||
            bt_smp_parse_central_identification(command, &central) != BT_OK)
            goto invalid;
        memcpy(m->peer_keys.rand, central.rand, 8);
        m->peer_keys.ediv = central.ediv;
        m->peer_keys.key_mask |= BT_SMP_KEYDIST_ENC_KEY;
        m->peer_keys_complete_mask |= BT_SMP_KEYDIST_ENC_KEY;
        break;
    case BT_SMP_IDENTITY_INFORMATION:
        if ((expected & BT_SMP_KEYDIST_ID_KEY) == 0 || m->peer_irk_received ||
            bt_smp_parse_value128(command, &value) != BT_OK)
            goto invalid;
        memcpy(m->peer_keys.irk, value, 16);
        m->peer_irk_received = true;
        break;
    case BT_SMP_IDENTITY_ADDRESS_INFORMATION:
        if (!m->peer_irk_received ||
            (m->peer_keys_complete_mask & BT_SMP_KEYDIST_ID_KEY) != 0 ||
            bt_smp_parse_identity_address(command, &identity) != BT_OK)
            goto invalid;
        m->peer_keys.identity_address_type = identity.address_type;
        memcpy(m->peer_keys.identity_address, identity.address, 6);
        m->peer_keys.key_mask |= BT_SMP_KEYDIST_ID_KEY;
        m->peer_keys_complete_mask |= BT_SMP_KEYDIST_ID_KEY;
        break;
    case BT_SMP_SIGNING_INFORMATION:
        if ((expected & BT_SMP_KEYDIST_SIGN_KEY) == 0 ||
            (m->peer_keys_complete_mask & BT_SMP_KEYDIST_SIGN_KEY) != 0 ||
            bt_smp_parse_value128(command, &value) != BT_OK)
            goto invalid;
        memcpy(m->peer_keys.csrk, value, 16);
        m->peer_keys.key_mask |= BT_SMP_KEYDIST_SIGN_KEY;
        m->peer_keys_complete_mask |= BT_SMP_KEYDIST_SIGN_KEY;
        break;
    default:
        goto invalid;
    }
    if ((m->peer_keys_complete_mask & expected) == expected)
        finish_key_distribution(m, now_us);
    return;

invalid:
    send_failed(m, 0x0A, now_us, BT_SMP_MANAGER_ERROR_PROTOCOL);
}

void bt_smp_manager_init(struct bt_smp_manager *m,
                          const struct bt_smp_manager_config *config,
                          const struct bt_smp_aes128 *aes,
                          const struct bt_smp_manager_ops *ops, void *context)
{
    memset(m, 0, sizeof(*m));
    m->config = *config;
    m->aes = *aes;
    m->ops = ops;
    m->context = context;
}

void bt_smp_manager_set_cmac(struct bt_smp_manager *m,
                              const struct bt_smp_aes_cmac *cmac)
{
    if (m == NULL)
        return;
    if (cmac == NULL)
        memset(&m->cmac, 0, sizeof(m->cmac));
    else
        m->cmac = *cmac;
}

bt_status_t bt_smp_manager_start(struct bt_smp_manager *m, uint64_t now_us)
{
    uint8_t pdu[7];
    struct bt_buf_writer w;

    if (m == NULL || m->ops == NULL || m->ops->send == NULL ||
        m->ops->random == NULL || m->ops->start_encryption == NULL ||
        m->aes.encrypt == NULL || m->state != BT_SMP_STATE_IDLE)
        return BT_ERR_INVALID_ARGUMENT;
    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    if (bt_smp_encode_pairing_features(&w, BT_SMP_PAIRING_REQUEST,
                                        &m->config.features) != BT_OK)
        return BT_ERR_INVALID_ARGUMENT;
    reverse_copy(m->preq, pdu, sizeof(pdu));
    m->state = BT_SMP_STATE_WAIT_PAIRING_RESPONSE;
    {
        bt_status_t st = send_pdu(m, pdu, sizeof(pdu), now_us);

        if (st != BT_OK)
        {
            m->state = BT_SMP_STATE_IDLE;
            return st;
        }
    }
    return BT_OK;
}

bt_status_t bt_smp_manager_accept(struct bt_smp_manager *m,
                                   const uint8_t *request, size_t request_len,
                                   uint64_t now_us)
{
    struct bt_smp_command command;
    struct bt_smp_pairing_features peer, response;
    struct bt_buf_writer w;
    uint8_t pdu[7];
    bool sc;

    if (m == NULL || request == NULL || m->ops == NULL || m->ops->send == NULL ||
        m->ops->random == NULL || m->ops->start_encryption == NULL ||
        m->aes.encrypt == NULL || m->state != BT_SMP_STATE_IDLE ||
        bt_smp_parse_command(request, request_len, &command) != BT_OK ||
        command.code != BT_SMP_PAIRING_REQUEST ||
        bt_smp_parse_pairing_features(&command, &peer) != BT_OK)
        return BT_ERR_INVALID_ARGUMENT;

    sc = (m->config.features.auth_req & peer.auth_req & BT_SMP_AUTHREQ_SC) != 0 &&
         m->cmac.calculate != NULL && m->ops->generate_public_key != NULL &&
         m->ops->generate_dhkey != NULL;
    if (sc)
    {
        response = m->config.features;
        response.oob_data_flag = 0;
        response.initiator_key_distribution &= peer.initiator_key_distribution;
        response.responder_key_distribution &= peer.responder_key_distribution;
        /* Secure Connections derives the LTK with f5. EncKey is reserved in
         * Phase 3 and must not be advertised or transmitted. */
        response.initiator_key_distribution &= (uint8_t)~BT_SMP_KEYDIST_ENC_KEY;
        response.responder_key_distribution &= (uint8_t)~BT_SMP_KEYDIST_ENC_KEY;
        /* an SC peer holding OOB data would select OOB, which we lack */
        sc = bt_smp_negotiate_pairing(&peer, &response, &m->negotiation) == BT_OK &&
             m->negotiation.secure_connections &&
             m->negotiation.association != BT_SMP_ASSOC_OOB;
    }
    if (!sc)
    {
        response = m->config.features;
        response.auth_req &= (uint8_t)~(BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_MITM);
        response.io_capability = 0x03u; /* NoInputNoOutput: legacy Just Works */
        response.oob_data_flag = 0;
        response.initiator_key_distribution &= peer.initiator_key_distribution;
        response.responder_key_distribution &= peer.responder_key_distribution;
        if (bt_smp_negotiate_pairing(&peer, &response, &m->negotiation) != BT_OK ||
            m->negotiation.secure_connections ||
            m->negotiation.association != BT_SMP_ASSOC_JUST_WORKS)
            return BT_ERR_INVALID_ARGUMENT;
    }

    reverse_copy(m->preq, request, 7);
    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    if (bt_smp_encode_pairing_features(&w, BT_SMP_PAIRING_RESPONSE, &response) != BT_OK)
        return BT_ERR_INVALID_ARGUMENT;
    reverse_copy(m->pres, pdu, 7);
    memset(m->tk, 0, sizeof(m->tk));
    m->responder = true;
    m->sc_round = 0;
    m->peer_confirm_pending = false;
    m->peer_dhkey_check_pending = false;
    if (sc)
    {
        m->state = BT_SMP_STATE_WAIT_PEER_PUBLIC_KEY;
    }
    else
        m->state = BT_SMP_STATE_WAIT_PAIRING_CONFIRM;
    if (send_pdu(m, pdu, sizeof(pdu), now_us) != BT_OK) {
        m->state = BT_SMP_STATE_IDLE;
        m->responder = false;
        return BT_ERR_NO_RESOURCES;
    }
    return BT_OK;
}

void bt_smp_manager_on_pdu(struct bt_smp_manager *m, const uint8_t *pdu,
                            size_t pdu_len, uint64_t now_us)
{
    struct bt_smp_command command;

    if (m == NULL || m->state == BT_SMP_STATE_IDLE || m->state == BT_SMP_STATE_COMPLETE ||
        m->state == BT_SMP_STATE_FAILED)
        return;
    if (bt_smp_parse_command(pdu, pdu_len, &command) != BT_OK)
    {
        send_failed(m, 0x0A, now_us, BT_SMP_MANAGER_ERROR_PROTOCOL);
        return;
    }
    m->deadline_us = timeout_deadline(now_us);
    switch (command.code)
    {
    case BT_SMP_PAIRING_RESPONSE:
        handle_pairing_response(m, &command, pdu, now_us);
        break;
    case BT_SMP_PAIRING_PUBLIC_KEY:
        handle_sc_public_key(m, &command, now_us);
        break;
    case BT_SMP_PAIRING_CONFIRM:
        if (m->responder && m->negotiation.secure_connections)
            responder_sc_on_confirm(m, &command, now_us);
        else if (m->responder)
            handle_responder_confirm(m, &command, now_us);
        else if (m->negotiation.secure_connections &&
                 (m->state == BT_SMP_STATE_WAIT_SC_CONFIRM ||
                  m->state == BT_SMP_STATE_WAIT_DHKEY))
            handle_sc_confirm(m, &command, now_us);
        else
            handle_confirm(m, &command, now_us);
        break;
    case BT_SMP_PAIRING_RANDOM:
        if (m->responder && m->negotiation.secure_connections)
            responder_sc_on_random(m, &command, now_us);
        else if (m->responder)
            handle_responder_random(m, &command, now_us);
        else if (m->state == BT_SMP_STATE_WAIT_SC_RANDOM)
            handle_sc_random(m, &command, now_us);
        else
            handle_random(m, &command, now_us);
        break;
    case BT_SMP_PAIRING_DHKEY_CHECK:
        if (m->responder)
            responder_sc_on_dhkey_check(m, &command, now_us);
        else
            handle_sc_dhkey_check(m, &command, now_us);
        break;
    case BT_SMP_ENCRYPTION_INFORMATION:
    case BT_SMP_CENTRAL_IDENTIFICATION:
    case BT_SMP_IDENTITY_INFORMATION:
    case BT_SMP_IDENTITY_ADDRESS_INFORMATION:
    case BT_SMP_SIGNING_INFORMATION:
        handle_distributed_key(m, &command, now_us);
        break;
    case BT_SMP_PAIRING_FAILED:
        finish(m, BT_SMP_MANAGER_ERROR_PROTOCOL);
        break;
    case BT_SMP_PAIRING_KEYPRESS_NOTIFICATION:
    case BT_SMP_SECURITY_REQUEST:
        /* progress hints from a peer typing a passkey, or a Security Request
         * arriving while we already pair: informational, not a violation */
        break;
    default:
        send_failed(m, 0x07, now_us, BT_SMP_MANAGER_ERROR_PROTOCOL);
        break;
    }
}

bt_status_t bt_smp_manager_provide_passkey(struct bt_smp_manager *m,
                                            uint32_t passkey, uint64_t now_us)
{
    if (m == NULL || m->state != BT_SMP_STATE_WAIT_TK || passkey > 999999u ||
        m->negotiation.association == BT_SMP_ASSOC_OOB)
        return BT_ERR_INVALID_ARGUMENT;
    set_passkey_tk(m, passkey);
    if (m->negotiation.secure_connections && m->responder)
        responder_sc_passkey_ready(m, now_us);
    else if (m->negotiation.secure_connections)
        start_sc_passkey_round(m, now_us);
    else
        start_confirm_exchange(m, now_us);
    return m->state == BT_SMP_STATE_FAILED ? BT_ERR_INVALID_ARGUMENT : BT_OK;
}

bt_status_t bt_smp_manager_provide_oob_tk(struct bt_smp_manager *m,
                                           const uint8_t tk[16], uint64_t now_us)
{
    if (m == NULL || tk == NULL || m->state != BT_SMP_STATE_WAIT_TK ||
        m->negotiation.association != BT_SMP_ASSOC_OOB)
        return BT_ERR_INVALID_ARGUMENT;
    memcpy(m->tk, tk, 16);
    start_confirm_exchange(m, now_us);
    return m->state == BT_SMP_STATE_FAILED ? BT_ERR_INVALID_ARGUMENT : BT_OK;
}

bt_status_t bt_smp_manager_confirm_numeric(struct bt_smp_manager *m,
                                            bool accepted, uint64_t now_us)
{
    if (m == NULL || m->state != BT_SMP_STATE_WAIT_NUMERIC_CONFIRMATION)
        return BT_ERR_INVALID_ARGUMENT;
    if (!accepted)
    {
        send_failed(m, 0x0C, now_us, BT_SMP_MANAGER_ERROR_CONFIRM);
        return BT_OK;
    }
    if (m->responder)
    {
        if (responder_sc_derive_keys(m, now_us))
        {
            m->state = BT_SMP_STATE_WAIT_DHKEY_CHECK;
            m->deadline_us = timeout_deadline(now_us);
            if (m->peer_dhkey_check_pending)
            {
                m->peer_dhkey_check_pending = false;
                responder_sc_check_and_reply(m, now_us);
            }
        }
    }
    else
        start_sc_stage2(m, now_us);
    return m->state == BT_SMP_STATE_FAILED ? BT_ERR_INVALID_ARGUMENT : BT_OK;
}

void bt_smp_manager_on_local_public_key(struct bt_smp_manager *m, bool success,
                                         const uint8_t x[32], const uint8_t y[32],
                                         uint64_t now_us)
{
    uint8_t wire_x[32];
    uint8_t wire_y[32];
    uint8_t pdu[65];
    struct bt_buf_writer w;

    if (m == NULL || m->state != BT_SMP_STATE_WAIT_LOCAL_PUBLIC_KEY)
        return;
    if (!success || x == NULL || y == NULL)
    {
        send_failed(m, 0x08, now_us, BT_SMP_MANAGER_ERROR_CRYPTO);
        return;
    }
    memcpy(m->local_public_x, x, 32);
    memcpy(m->local_public_y, y, 32);
    if (m->responder && memcmp(m->local_public_x, m->peer_public_x, 32) == 0)
    {
        send_failed(m, 0x0A, now_us, BT_SMP_MANAGER_ERROR_PROTOCOL); /* reflected key */
        return;
    }
    reverse_copy(wire_x, x, 32);
    reverse_copy(wire_y, y, 32);
    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    m->state = m->responder ? BT_SMP_STATE_WAIT_DHKEY : BT_SMP_STATE_WAIT_PEER_PUBLIC_KEY;
    if (bt_smp_encode_public_key(&w, wire_x, wire_y) != BT_OK ||
        send_pdu(m, pdu, bt_buf_writer_len(&w), now_us) != BT_OK)
    {
        finish(m, BT_SMP_MANAGER_ERROR_PROTOCOL);
        return;
    }
    if (m->responder && m->state == BT_SMP_STATE_WAIT_DHKEY &&
        m->ops->generate_dhkey(m->context, m->peer_public_x, m->peer_public_y) != BT_OK)
        finish(m, BT_SMP_MANAGER_ERROR_CRYPTO);
}

void bt_smp_manager_on_dhkey(struct bt_smp_manager *m, bool success,
                              const uint8_t dhkey[32], uint64_t now_us)
{
    if (m == NULL || m->state != BT_SMP_STATE_WAIT_DHKEY)
        return;
    if (!success || dhkey == NULL ||
        m->ops->random(m->context, m->local_random, sizeof(m->local_random)) != BT_OK)
    {
        send_failed(m, 0x08, now_us, BT_SMP_MANAGER_ERROR_CRYPTO);
        return;
    }
    memcpy(m->dhkey, dhkey, 32);
    if (sc_passkey_entry(m))
    {
        m->sc_round = 0;
        if (m->negotiation.association ==
            (m->responder ? BT_SMP_ASSOC_PASSKEY_RESPONDER_DISPLAYS
                          : BT_SMP_ASSOC_PASSKEY_INITIATOR_DISPLAYS))
        {
            uint8_t random_bytes[4];
            uint32_t passkey;

            if (m->ops->random(m->context, random_bytes, sizeof(random_bytes)) != BT_OK)
            {
                send_failed(m, 0x08, now_us, BT_SMP_MANAGER_ERROR_CRYPTO);
                return;
            }
            passkey = (((uint32_t)random_bytes[0] << 24) | ((uint32_t)random_bytes[1] << 16) |
                       ((uint32_t)random_bytes[2] << 8) | random_bytes[3]) % 1000000u;
            set_passkey_tk(m, passkey);
            if (m->ops->user_action != NULL)
                m->ops->user_action(m->context, BT_SMP_USER_DISPLAY_PASSKEY, passkey);
            if (m->responder)
                responder_sc_passkey_ready(m, now_us);
            else
                start_sc_passkey_round(m, now_us);
            return;
        }
        /* the peer displays (or both type): the rounds start once the
         * passkey is provided */
        m->state = BT_SMP_STATE_WAIT_TK;
        m->deadline_us = timeout_deadline(now_us);
        if (m->ops->user_action != NULL)
            m->ops->user_action(m->context, BT_SMP_USER_REQUEST_PASSKEY, 0);
        return;
    }
    if (m->responder)
    {
        responder_sc_send_confirm(m, now_us);
        return;
    }
    m->state = BT_SMP_STATE_WAIT_SC_CONFIRM;
    m->deadline_us = timeout_deadline(now_us);
    if (m->peer_confirm_pending)
    {
        m->peer_confirm_pending = false;
        send_sc_random(m, now_us);
    }
}

void bt_smp_manager_on_encryption_changed(struct bt_smp_manager *m, bool enabled,
                                           uint64_t now_us)
{
    if (m == NULL || m->state != BT_SMP_STATE_WAIT_ENCRYPTION)
        return;
    if (!enabled)
    {
        finish(m, BT_SMP_MANAGER_ERROR_ENCRYPTION);
        return;
    }
    memset(&m->peer_keys, 0, sizeof(m->peer_keys));
    if (m->negotiation.secure_connections)
    {
        m->peer_keys.key_mask = BT_SMP_KEYDIST_ENC_KEY;
        memcpy(m->peer_keys.ltk, m->stk, sizeof(m->peer_keys.ltk));
    }
    m->peer_keys_complete_mask = 0;
    m->peer_ltk_received = false;
    m->peer_irk_received = false;
    /* Phase 3 starts with the responder's keys. Wait for the initiator's
     * before sending: a synchronous transport may deliver them reentrantly. */
    m->state = BT_SMP_STATE_WAIT_KEY_DISTRIBUTION;
    m->deadline_us = timeout_deadline(now_us);
    if (m->responder)
    {
        if (!send_key_distribution(m, now_us))
        {
            finish(m, BT_SMP_MANAGER_ERROR_CRYPTO);
            return;
        }
        if (m->negotiation.initiator_key_distribution == 0 &&
            m->state == BT_SMP_STATE_WAIT_KEY_DISTRIBUTION)
        {
            if (m->ops->keys_complete != NULL)
                m->ops->keys_complete(m->context, &m->peer_keys, &m->local_keys);
            finish(m, BT_SMP_MANAGER_OK);
        }
        return;
    }
    if (m->negotiation.responder_key_distribution == 0)
        finish_key_distribution(m, now_us);
}

void bt_smp_manager_tick(struct bt_smp_manager *m, uint64_t now_us)
{
    if (m != NULL && m->deadline_us != 0 && now_us >= m->deadline_us &&
        m->state != BT_SMP_STATE_COMPLETE && m->state != BT_SMP_STATE_FAILED)
        finish(m, BT_SMP_MANAGER_ERROR_TIMEOUT);
}
