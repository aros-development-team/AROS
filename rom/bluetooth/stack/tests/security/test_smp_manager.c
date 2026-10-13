#include "test_smp_manager.h"
#include "../support/test.h"

#include <btcore/smp_manager.h>

#include <string.h>

struct fake_manager_port
{
    uint8_t sent[80];
    size_t sent_len;
    uint8_t next_random;
    int encryption_calls;
    uint8_t stk[16];
    uint8_t key_size;
    int public_key_calls;
    int dhkey_calls;
    uint8_t peer_public_x[32];
    uint8_t peer_public_y[32];
    int keys_complete_calls;
    struct bt_smp_distributed_keys peer_keys;
    struct bt_smp_distributed_keys local_keys;
    int complete_calls;
    enum bt_smp_manager_result result;
    int user_calls;
    enum bt_smp_user_action user_action;
};

static bt_status_t fake_send(void *context, const uint8_t *pdu, size_t len)
{
    struct fake_manager_port *port = context;

    memcpy(port->sent, pdu, len);
    port->sent_len = len;
    return BT_OK;
}

static bt_status_t fake_random(void *context, uint8_t *out, size_t len)
{
    struct fake_manager_port *port = context;
    size_t i;

    for (i = 0; i < len; ++i)
        out[i] = port->next_random++;
    return BT_OK;
}

static bt_status_t fake_start_encryption(void *context, const uint8_t stk[16],
                                          uint8_t key_size)
{
    struct fake_manager_port *port = context;

    ++port->encryption_calls;
    memcpy(port->stk, stk, 16);
    port->key_size = key_size;
    return BT_OK;
}

static bt_status_t fake_generate_public_key(void *context)
{
    struct fake_manager_port *port = context;

    ++port->public_key_calls;
    return BT_OK;
}

static bt_status_t fake_generate_dhkey(void *context, const uint8_t peer_x[32],
                                       const uint8_t peer_y[32])
{
    struct fake_manager_port *port = context;

    ++port->dhkey_calls;
    memcpy(port->peer_public_x, peer_x, 32);
    memcpy(port->peer_public_y, peer_y, 32);
    return BT_OK;
}

static bt_status_t fake_get_local_keys(void *context, uint8_t key_mask,
                                       struct bt_smp_distributed_keys *out)
{
    (void)context;
    memset(out, 0, sizeof(*out));
    out->key_mask = key_mask;
    out->identity_address_type = 0;
    memset(out->irk, 0x33, sizeof(out->irk));
    memset(out->identity_address, 0x44, sizeof(out->identity_address));
    return BT_OK;
}

static void fake_keys_complete(void *context, const struct bt_smp_distributed_keys *peer,
                               const struct bt_smp_distributed_keys *local)
{
    struct fake_manager_port *port = context;

    ++port->keys_complete_calls;
    port->peer_keys = *peer;
    port->local_keys = *local;
}

static void fake_user(void *context, enum bt_smp_user_action action, uint32_t passkey)
{
    struct fake_manager_port *port = context;

    (void)passkey;
    ++port->user_calls;
    port->user_action = action;
}

static void fake_complete(void *context, enum bt_smp_manager_result result,
                          const struct bt_smp_pairing_negotiation *negotiation)
{
    struct fake_manager_port *port = context;

    (void)negotiation;
    ++port->complete_calls;
    port->result = result;
}

/* Deterministic test primitive: not cryptography. It makes composition and
 * state-machine expectations transparent; real c1/s1 are separately tested
 * against official vectors. */
static bt_status_t identity_aes(void *context, const uint8_t key[16],
                                const uint8_t plaintext[16], uint8_t ciphertext[16])
{
    size_t i;

    (void)context;
    for (i = 0; i < 16; ++i)
        ciphertext[i] = plaintext[i] ^ key[i];
    return BT_OK;
}

static bt_status_t fake_cmac(void *context, const uint8_t key[16],
                             const uint8_t *message, size_t message_len,
                             uint8_t mac[16])
{
    size_t i;

    (void)context;
    memcpy(mac, key, 16);
    for (i = 0; i < message_len; ++i)
        mac[i % 16] ^= (uint8_t)(message[i] + (uint8_t)i);
    return BT_OK;
}

static const struct bt_smp_manager_ops fake_ops = {
    .send = fake_send,
    .random = fake_random,
    .start_encryption = fake_start_encryption,
    .generate_public_key = fake_generate_public_key,
    .generate_dhkey = fake_generate_dhkey,
    .get_local_keys = fake_get_local_keys,
    .keys_complete = fake_keys_complete,
    .user_action = fake_user,
    .complete = fake_complete};

static void init_manager(struct bt_smp_manager *manager, struct fake_manager_port *port,
                         uint8_t auth_req)
{
    struct bt_smp_manager_config config;
    struct bt_smp_aes128 aes = {identity_aes, NULL};
    size_t i;

    memset(port, 0, sizeof(*port));
    memset(&config, 0, sizeof(config));
    config.features.io_capability = 0x03;
    config.features.auth_req = auth_req;
    config.features.max_encryption_key_size = 12;
    config.features.initiator_key_distribution = BT_SMP_KEYDIST_ID_KEY;
    config.features.responder_key_distribution = BT_SMP_KEYDIST_ID_KEY;
    for (i = 0; i < 6; ++i)
    {
        config.initiator_address[i] = (uint8_t)(0xA1u + i);
        config.responder_address[i] = (uint8_t)(0xB1u + i);
    }
    bt_smp_manager_init(manager, &config, &aes, &fake_ops, port);
}

static void feed_pairing_response(struct bt_smp_manager *manager, uint8_t io,
                                   uint8_t auth_req, uint64_t now_us)
{
    struct bt_smp_pairing_features response = {
        io, 0, auth_req, 16, BT_SMP_KEYDIST_ID_KEY, BT_SMP_KEYDIST_ID_KEY};
    uint8_t pdu[7];
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    BT_CHECK(bt_smp_encode_pairing_features(&w, BT_SMP_PAIRING_RESPONSE, &response) == BT_OK);
    bt_smp_manager_on_pdu(manager, pdu, sizeof(pdu), now_us);
}

static void feed_pairing_response_with_keys(struct bt_smp_manager *manager, uint8_t io,
                                             uint8_t auth_req, uint8_t initiator_keys,
                                             uint8_t responder_keys, uint64_t now_us)
{
    struct bt_smp_pairing_features response = {
        io, 0, auth_req, 16, initiator_keys, responder_keys};
    uint8_t pdu[7];
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    BT_CHECK(bt_smp_encode_pairing_features(&w, BT_SMP_PAIRING_RESPONSE, &response) == BT_OK);
    bt_smp_manager_on_pdu(manager, pdu, sizeof(pdu), now_us);
}

static void feed_value(struct bt_smp_manager *manager, uint8_t code,
                       const uint8_t value_msb[16], uint64_t now_us)
{
    uint8_t pdu[17];
    uint8_t wire[16];
    struct bt_buf_writer w;
    size_t i;

    for (i = 0; i < 16; ++i)
        wire[i] = value_msb[15 - i];
    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    BT_CHECK(bt_smp_encode_value128(&w, code, wire) == BT_OK);
    bt_smp_manager_on_pdu(manager, pdu, sizeof(pdu), now_us);
}

static void feed_public_key(struct bt_smp_manager *manager, const uint8_t x[32],
                            const uint8_t y[32], uint64_t now_us)
{
    uint8_t pdu[65];
    uint8_t wire_x[32];
    uint8_t wire_y[32];
    struct bt_buf_writer w;
    size_t i;

    for (i = 0; i < 32; ++i)
    {
        wire_x[i] = x[31 - i];
        wire_y[i] = y[31 - i];
    }
    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    BT_CHECK(bt_smp_encode_public_key(&w, wire_x, wire_y) == BT_OK);
    bt_smp_manager_on_pdu(manager, pdu, sizeof(pdu), now_us);
}

static void feed_identity_address(struct bt_smp_manager *manager, uint8_t type,
                                   const uint8_t address[6], uint64_t now_us)
{
    uint8_t pdu[8];
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    BT_CHECK(bt_smp_encode_identity_address(&w, type, address) == BT_OK);
    bt_smp_manager_on_pdu(manager, pdu, sizeof(pdu), now_us);
}

static void feed_central_identification(struct bt_smp_manager *manager,
                                         const uint8_t rand[8], uint16_t ediv,
                                         uint64_t now_us)
{
    uint8_t pdu[11];
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    BT_CHECK(bt_smp_encode_central_identification(&w, rand, ediv) == BT_OK);
    bt_smp_manager_on_pdu(manager, pdu, sizeof(pdu), now_us);
}

static void test_legacy_just_works_success(void)
{
    struct bt_smp_manager manager;
    struct fake_manager_port port;
    uint8_t peer_random[16];
    uint8_t peer_confirm[16];
    size_t i;

    init_manager(&manager, &port, BT_SMP_AUTHREQ_BONDING);
    BT_CHECK(bt_smp_manager_start(&manager, 10) == BT_OK);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_PAIRING_RESPONSE);
    BT_CHECK(port.sent_len == 7 && port.sent[0] == BT_SMP_PAIRING_REQUEST);

    feed_pairing_response(&manager, 0x03, BT_SMP_AUTHREQ_BONDING, 20);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_PAIRING_CONFIRM);
    BT_CHECK(port.sent[0] == BT_SMP_PAIRING_CONFIRM);
    for (i = 0; i < 16; ++i)
        peer_random[i] = (uint8_t)(0x80u + i);
    BT_CHECK(bt_smp_crypto_c1(&manager.aes, manager.tk, peer_random, manager.preq,
                              manager.pres, manager.config.initiator_address_type,
                              manager.config.responder_address_type,
                              manager.config.initiator_address,
                              manager.config.responder_address, peer_confirm) == BT_OK);
    feed_value(&manager, BT_SMP_PAIRING_CONFIRM, peer_confirm, 30);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_PAIRING_RANDOM);
    BT_CHECK(port.sent[0] == BT_SMP_PAIRING_RANDOM);

    feed_value(&manager, BT_SMP_PAIRING_RANDOM, peer_random, 40);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_ENCRYPTION);
    BT_CHECK(port.encryption_calls == 1 && port.key_size == 12);
    BT_CHECK(port.stk[0] == 0 && port.stk[1] == 0 && port.stk[2] == 0 && port.stk[3] == 0);

    bt_smp_manager_on_encryption_changed(&manager, true, 50);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_KEY_DISTRIBUTION);
    {
        uint8_t irk[16];
        uint8_t identity[6] = {1, 2, 3, 4, 5, 6};

        memset(irk, 0xA5, sizeof(irk));
        feed_value(&manager, BT_SMP_IDENTITY_INFORMATION, irk, 60);
        BT_CHECK(manager.state == BT_SMP_STATE_WAIT_KEY_DISTRIBUTION);
        feed_identity_address(&manager, 1, identity, 70);
        BT_CHECK(port.keys_complete_calls == 1);
        BT_CHECK(port.peer_keys.key_mask == BT_SMP_KEYDIST_ID_KEY);
        BT_CHECK(port.peer_keys.identity_address_type == 1);
    }
    BT_CHECK(manager.state == BT_SMP_STATE_COMPLETE);
    BT_CHECK(port.complete_calls == 1 && port.result == BT_SMP_MANAGER_OK);
}

static void test_legacy_responder_just_works(void)
{
    struct bt_smp_manager manager;
    struct fake_manager_port port;
    struct bt_smp_pairing_features request = {
        0x03, 0, BT_SMP_AUTHREQ_BONDING | BT_SMP_AUTHREQ_SC, 16,
        BT_SMP_KEYDIST_ID_KEY, BT_SMP_KEYDIST_ID_KEY};
    struct bt_buf_writer w;
    uint8_t request_pdu[7], peer_random[16], peer_confirm[16];
    uint8_t irk[16], identity[6] = {1, 3, 5, 7, 9, 11};
    size_t i;

    init_manager(&manager, &port, BT_SMP_AUTHREQ_BONDING | BT_SMP_AUTHREQ_SC);
    bt_buf_writer_init(&w, request_pdu, sizeof(request_pdu));
    BT_CHECK(bt_smp_encode_pairing_features(&w, BT_SMP_PAIRING_REQUEST,
                                             &request) == BT_OK);
    BT_CHECK(bt_smp_manager_accept(&manager, request_pdu, sizeof(request_pdu),
                                    10) == BT_OK);
    BT_CHECK(manager.responder && !manager.negotiation.secure_connections);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_PAIRING_CONFIRM);
    BT_CHECK(port.sent[0] == BT_SMP_PAIRING_RESPONSE);

    for (i = 0; i < sizeof(peer_random); ++i)
        peer_random[i] = (uint8_t)(0x90u + i);
    BT_CHECK(bt_smp_crypto_c1(&manager.aes, manager.tk, peer_random,
                              manager.preq, manager.pres,
                              manager.config.initiator_address_type,
                              manager.config.responder_address_type,
                              manager.config.initiator_address,
                              manager.config.responder_address,
                              peer_confirm) == BT_OK);
    feed_value(&manager, BT_SMP_PAIRING_CONFIRM, peer_confirm, 20);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_PAIRING_RANDOM);
    BT_CHECK(port.sent[0] == BT_SMP_PAIRING_CONFIRM);
    feed_value(&manager, BT_SMP_PAIRING_RANDOM, peer_random, 30);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_ENCRYPTION);
    BT_CHECK(port.sent[0] == BT_SMP_PAIRING_RANDOM);
    BT_CHECK(port.encryption_calls == 1);

    bt_smp_manager_on_encryption_changed(&manager, true, 40);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_KEY_DISTRIBUTION);
    /* Phase 3: the responder distributes first, unprompted */
    BT_CHECK(port.sent[0] == BT_SMP_IDENTITY_ADDRESS_INFORMATION);
    port.sent[0] = 0;
    memset(irk, 0x5a, sizeof(irk));
    feed_value(&manager, BT_SMP_IDENTITY_INFORMATION, irk, 50);
    feed_identity_address(&manager, 0, identity, 60);
    BT_CHECK(manager.state == BT_SMP_STATE_COMPLETE);
    BT_CHECK(port.sent[0] == 0); /* nothing more to send */
    BT_CHECK(port.keys_complete_calls == 1);
    BT_CHECK(port.complete_calls == 1 && port.result == BT_SMP_MANAGER_OK);
}

static void test_confirm_failure_and_timeout(void)
{
    struct bt_smp_manager manager;
    struct fake_manager_port port;
    uint8_t wrong[16] = {0};
    uint8_t peer_random[16] = {0};

    init_manager(&manager, &port, 0);
    BT_CHECK(bt_smp_manager_start(&manager, 0) == BT_OK);
    feed_pairing_response(&manager, 0x03, 0, 1);
    feed_value(&manager, BT_SMP_PAIRING_CONFIRM, wrong, 2);
    peer_random[0] = 1;
    feed_value(&manager, BT_SMP_PAIRING_RANDOM, peer_random, 3);
    BT_CHECK(manager.state == BT_SMP_STATE_FAILED);
    BT_CHECK(port.result == BT_SMP_MANAGER_ERROR_CONFIRM);
    BT_CHECK(port.sent[0] == BT_SMP_PAIRING_FAILED && port.sent[1] == 0x04);

    init_manager(&manager, &port, 0);
    BT_CHECK(bt_smp_manager_start(&manager, 100) == BT_OK);
    bt_smp_manager_tick(&manager, 100 + BT_SMP_TIMEOUT_US - 1);
    BT_CHECK(port.complete_calls == 0);
    bt_smp_manager_tick(&manager, 100 + BT_SMP_TIMEOUT_US);
    BT_CHECK(port.complete_calls == 1 && port.result == BT_SMP_MANAGER_ERROR_TIMEOUT);
}

static void test_passkey_input(void)
{
    struct bt_smp_manager manager;
    struct fake_manager_port port;

    init_manager(&manager, &port, BT_SMP_AUTHREQ_MITM);
    manager.config.features.io_capability = 0x02; /* KeyboardOnly */
    BT_CHECK(bt_smp_manager_start(&manager, 0) == BT_OK);
    feed_pairing_response(&manager, 0x00, 0, 1); /* responder displays */
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_TK);
    BT_CHECK(port.user_calls == 1 && port.user_action == BT_SMP_USER_REQUEST_PASSKEY);
    BT_CHECK(bt_smp_manager_provide_passkey(&manager, 1000000, 2) ==
              BT_ERR_INVALID_ARGUMENT);
    BT_CHECK(bt_smp_manager_provide_passkey(&manager, 123456, 2) == BT_OK);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_PAIRING_CONFIRM);
    BT_CHECK(manager.tk[14] == 0xE2 && manager.tk[15] == 0x40);
}

static void setup_sc_until_peer_random(struct bt_smp_manager *manager,
                                        struct fake_manager_port *port,
                                        uint8_t local_io, uint8_t peer_io,
                                        uint8_t auth_req, uint8_t peer_random[16])
{
    struct bt_smp_aes_cmac cmac = {fake_cmac, NULL};
    uint8_t local_x[32];
    uint8_t local_y[32];
    uint8_t peer_x[32];
    uint8_t peer_y[32];
    uint8_t dhkey[32];
    uint8_t peer_confirm[16];
    size_t i;

    init_manager(manager, port, auth_req);
    manager->config.features.io_capability = local_io;
    bt_smp_manager_set_cmac(manager, &cmac);
    BT_CHECK(bt_smp_manager_start(manager, 0) == BT_OK);
    feed_pairing_response_with_keys(manager, peer_io, auth_req, 0, 0, 1);
    BT_CHECK(manager->state == BT_SMP_STATE_WAIT_LOCAL_PUBLIC_KEY);
    BT_CHECK(port->public_key_calls == 1);
    for (i = 0; i < 32; ++i)
    {
        local_x[i] = (uint8_t)(0x10u + i);
        local_y[i] = (uint8_t)(0x40u + i);
        peer_x[i] = (uint8_t)(0x80u + i);
        peer_y[i] = (uint8_t)(0xC0u + i);
        dhkey[i] = (uint8_t)(0x55u ^ i);
    }
    bt_smp_manager_on_local_public_key(manager, true, local_x, local_y, 2);
    BT_CHECK(manager->state == BT_SMP_STATE_WAIT_PEER_PUBLIC_KEY);
    BT_CHECK(port->sent[0] == BT_SMP_PAIRING_PUBLIC_KEY);
    feed_public_key(manager, peer_x, peer_y, 3);
    BT_CHECK(manager->state == BT_SMP_STATE_WAIT_DHKEY);
    BT_CHECK(port->dhkey_calls == 1);
    BT_CHECK(memcmp(port->peer_public_x, peer_x, 32) == 0);
    bt_smp_manager_on_dhkey(manager, true, dhkey, 4);
    BT_CHECK(manager->state == BT_SMP_STATE_WAIT_SC_CONFIRM);
    for (i = 0; i < 16; ++i)
        peer_random[i] = (uint8_t)(0xE0u + i);
    BT_CHECK(bt_smp_crypto_f4(&manager->cmac, peer_x, local_x, peer_random, 0,
                              peer_confirm) == BT_OK);
    feed_value(manager, BT_SMP_PAIRING_CONFIRM, peer_confirm, 5);
    BT_CHECK(manager->state == BT_SMP_STATE_WAIT_SC_RANDOM);
    BT_CHECK(port->sent[0] == BT_SMP_PAIRING_RANDOM);
    feed_value(manager, BT_SMP_PAIRING_RANDOM, peer_random, 6);
}

static void test_secure_connections_just_works(void)
{
    struct bt_smp_manager manager;
    struct fake_manager_port port;
    uint8_t peer_random[16];
    uint8_t responder_check[16];
    uint8_t a1[7];
    uint8_t a2[7];
    uint8_t io_cap[3] = {BT_SMP_AUTHREQ_SC, 0, 0x03};
    uint8_t zero[16] = {0};

    setup_sc_until_peer_random(&manager, &port, 0x03, 0x03,
                               BT_SMP_AUTHREQ_SC, peer_random);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_DHKEY_CHECK);
    BT_CHECK(port.sent[0] == BT_SMP_PAIRING_DHKEY_CHECK);
    a1[0] = manager.config.initiator_address_type;
    memcpy(a1 + 1, manager.config.initiator_address, 6);
    a2[0] = manager.config.responder_address_type;
    memcpy(a2 + 1, manager.config.responder_address, 6);
    BT_CHECK(bt_smp_crypto_f6(&manager.cmac, manager.mac_key, peer_random,
                              manager.local_random, zero, io_cap, a2, a1,
                              responder_check) == BT_OK);
    feed_value(&manager, BT_SMP_PAIRING_DHKEY_CHECK, responder_check, 7);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_ENCRYPTION);
    BT_CHECK(port.encryption_calls == 1);
    BT_CHECK(memcmp(port.stk, manager.stk, 16) == 0);
    bt_smp_manager_on_encryption_changed(&manager, true, 8);
    BT_CHECK(manager.state == BT_SMP_STATE_COMPLETE);
    BT_CHECK(port.complete_calls == 1 && port.result == BT_SMP_MANAGER_OK);
    BT_CHECK(port.keys_complete_calls == 1);
    BT_CHECK(port.peer_keys.key_mask == BT_SMP_KEYDIST_ENC_KEY);
    BT_CHECK(memcmp(port.peer_keys.ltk, manager.stk, 16) == 0);
    BT_CHECK(port.local_keys.key_mask == BT_SMP_KEYDIST_ENC_KEY);
    BT_CHECK(memcmp(port.local_keys.ltk, manager.stk, 16) == 0);
    {
        uint8_t zero[8] = {0};

        BT_CHECK(memcmp(port.peer_keys.rand, zero, sizeof(zero)) == 0);
        BT_CHECK(port.peer_keys.ediv == 0);
    }
}

static void test_secure_connections_numeric_confirmation(void)
{
    struct bt_smp_manager manager;
    struct fake_manager_port port;
    uint8_t peer_random[16];

    setup_sc_until_peer_random(&manager, &port, 0x01, 0x01,
                               BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_MITM,
                               peer_random);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_NUMERIC_CONFIRMATION);
    BT_CHECK(port.user_calls == 1 && port.user_action == BT_SMP_USER_CONFIRM_NUMERIC);
    BT_CHECK(manager.numeric_value <= 999999u);
    BT_CHECK(bt_smp_manager_confirm_numeric(&manager, true, 7) == BT_OK);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_DHKEY_CHECK);

    setup_sc_until_peer_random(&manager, &port, 0x01, 0x01,
                               BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_MITM,
                               peer_random);
    BT_CHECK(bt_smp_manager_confirm_numeric(&manager, false, 7) == BT_OK);
    BT_CHECK(manager.state == BT_SMP_STATE_FAILED);
    BT_CHECK(port.sent[0] == BT_SMP_PAIRING_FAILED && port.sent[1] == 0x0C);
}

/* LE Secure Connections Passkey Entry, initiator displays (a keyboard peer):
 * 20 commitment rounds, each committing to one passkey bit, then f5/f6 with
 * r = passkey. The responder is emulated here round by round. */
static void test_secure_connections_passkey_entry(void)
{
    struct bt_smp_manager manager;
    struct fake_manager_port port;
    struct bt_smp_aes_cmac cmac = {fake_cmac, NULL};
    uint8_t local_x[32], local_y[32], peer_x[32], peer_y[32], dhkey[32];
    uint8_t peer_random[16];
    uint8_t peer_confirm[16];
    uint8_t our_confirm[16];
    uint8_t expected[16];
    uint8_t responder_check[16];
    uint8_t a1[7], a2[7];
    uint8_t io_cap[3] = {BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_MITM, 0, 0x02};
    uint32_t passkey;
    size_t i;
    int round;

    init_manager(&manager, &port, BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_MITM);
    manager.config.features.io_capability = 0x01; /* DisplayYesNo */
    bt_smp_manager_set_cmac(&manager, &cmac);
    BT_CHECK(bt_smp_manager_start(&manager, 0) == BT_OK);
    feed_pairing_response_with_keys(&manager, 0x02 /* KeyboardOnly */,
                                    BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_MITM, 0, 0, 1);
    BT_CHECK(manager.negotiation.secure_connections);
    BT_CHECK(manager.negotiation.association == BT_SMP_ASSOC_PASSKEY_INITIATOR_DISPLAYS);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_LOCAL_PUBLIC_KEY);
    for (i = 0; i < 32; ++i)
    {
        local_x[i] = (uint8_t)(0x10u + i);
        local_y[i] = (uint8_t)(0x40u + i);
        peer_x[i] = (uint8_t)(0x80u + i);
        peer_y[i] = (uint8_t)(0xC0u + i);
        dhkey[i] = (uint8_t)(0x55u ^ i);
    }
    bt_smp_manager_on_local_public_key(&manager, true, local_x, local_y, 2);
    feed_public_key(&manager, peer_x, peer_y, 3);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_DHKEY);
    bt_smp_manager_on_dhkey(&manager, true, dhkey, 4);

    /* the passkey was generated and shown, and round 0's commitment went out */
    BT_CHECK(port.user_calls == 1 && port.user_action == BT_SMP_USER_DISPLAY_PASSKEY);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_SC_CONFIRM);
    BT_CHECK(port.sent[0] == BT_SMP_PAIRING_CONFIRM);
    passkey = ((uint32_t)manager.tk[12] << 24) | ((uint32_t)manager.tk[13] << 16) |
              ((uint32_t)manager.tk[14] << 8) | manager.tk[15];
    BT_CHECK(passkey <= 999999u);

    for (round = 0; round < 20; ++round)
    {
        uint8_t z = (uint8_t)(0x80u | ((passkey >> round) & 1u));

        BT_CHECK(manager.sc_round == round);
        BT_CHECK(manager.state == BT_SMP_STATE_WAIT_SC_CONFIRM);
        /* our commitment must be f4(PKa, PKb, Nai, rai) */
        for (i = 0; i < 16; ++i)
            our_confirm[i] = port.sent[16 - i];
        BT_CHECK(bt_smp_crypto_f4(&manager.cmac, local_x, peer_x, manager.local_random, z,
                                  expected) == BT_OK);
        BT_CHECK(memcmp(our_confirm, expected, 16) == 0);
        /* responder's commitment to the same bit, then the randoms */
        for (i = 0; i < 16; ++i)
            peer_random[i] = (uint8_t)(0xE0u + i + round);
        BT_CHECK(bt_smp_crypto_f4(&manager.cmac, peer_x, local_x, peer_random, z,
                                  peer_confirm) == BT_OK);
        feed_value(&manager, BT_SMP_PAIRING_CONFIRM, peer_confirm, 5 + round);
        BT_CHECK(manager.state == BT_SMP_STATE_WAIT_SC_RANDOM);
        BT_CHECK(port.sent[0] == BT_SMP_PAIRING_RANDOM);
        feed_value(&manager, BT_SMP_PAIRING_RANDOM, peer_random, 6 + round);
    }
    /* after the 20th round: DHKey check with r = passkey */
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_DHKEY_CHECK);
    BT_CHECK(port.sent[0] == BT_SMP_PAIRING_DHKEY_CHECK);
    a1[0] = manager.config.initiator_address_type;
    memcpy(a1 + 1, manager.config.initiator_address, 6);
    a2[0] = manager.config.responder_address_type;
    memcpy(a2 + 1, manager.config.responder_address, 6);
    BT_CHECK(bt_smp_crypto_f6(&manager.cmac, manager.mac_key, peer_random,
                              manager.local_random, manager.tk, io_cap, a2, a1,
                              responder_check) == BT_OK);
    feed_value(&manager, BT_SMP_PAIRING_DHKEY_CHECK, responder_check, 40);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_ENCRYPTION);
    BT_CHECK(port.encryption_calls == 1);
    bt_smp_manager_on_encryption_changed(&manager, true, 41);
    BT_CHECK(manager.state == BT_SMP_STATE_COMPLETE);
    BT_CHECK(port.complete_calls == 1 && port.result == BT_SMP_MANAGER_OK);

    /* a responder committing to the wrong bit is caught in that round */
    init_manager(&manager, &port, BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_MITM);
    manager.config.features.io_capability = 0x01;
    bt_smp_manager_set_cmac(&manager, &cmac);
    BT_CHECK(bt_smp_manager_start(&manager, 0) == BT_OK);
    feed_pairing_response_with_keys(&manager, 0x02, BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_MITM, 0, 0, 1);
    bt_smp_manager_on_local_public_key(&manager, true, local_x, local_y, 2);
    feed_public_key(&manager, peer_x, peer_y, 3);
    bt_smp_manager_on_dhkey(&manager, true, dhkey, 4);
    passkey = ((uint32_t)manager.tk[12] << 24) | ((uint32_t)manager.tk[13] << 16) |
              ((uint32_t)manager.tk[14] << 8) | manager.tk[15];
    {
        uint8_t wrong = (uint8_t)(0x80u | (((passkey >> 0) & 1u) ^ 1u));
        for (i = 0; i < 16; ++i)
            peer_random[i] = (uint8_t)(0x70u + i);
        BT_CHECK(bt_smp_crypto_f4(&manager.cmac, peer_x, local_x, peer_random, wrong,
                                  peer_confirm) == BT_OK);
        feed_value(&manager, BT_SMP_PAIRING_CONFIRM, peer_confirm, 5);
        feed_value(&manager, BT_SMP_PAIRING_RANDOM, peer_random, 6);
        BT_CHECK(manager.state == BT_SMP_STATE_FAILED);
        BT_CHECK(port.result == BT_SMP_MANAGER_ERROR_CONFIRM);
        BT_CHECK(port.sent[0] == BT_SMP_PAIRING_FAILED && port.sent[1] == 0x04);
    }

    /* the responder-displays variant waits for the passkey first */
    init_manager(&manager, &port, BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_MITM);
    manager.config.features.io_capability = 0x02; /* KeyboardOnly */
    bt_smp_manager_set_cmac(&manager, &cmac);
    BT_CHECK(bt_smp_manager_start(&manager, 0) == BT_OK);
    feed_pairing_response_with_keys(&manager, 0x00 /* DisplayOnly */,
                                    BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_MITM, 0, 0, 1);
    BT_CHECK(manager.negotiation.association == BT_SMP_ASSOC_PASSKEY_RESPONDER_DISPLAYS);
    bt_smp_manager_on_local_public_key(&manager, true, local_x, local_y, 2);
    feed_public_key(&manager, peer_x, peer_y, 3);
    bt_smp_manager_on_dhkey(&manager, true, dhkey, 4);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_TK);
    BT_CHECK(port.user_action == BT_SMP_USER_REQUEST_PASSKEY);
    BT_CHECK(bt_smp_manager_provide_passkey(&manager, 123456, 5) == BT_OK);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_SC_CONFIRM);
    BT_CHECK(port.sent[0] == BT_SMP_PAIRING_CONFIRM);
}

static void test_full_key_distribution_and_order_validation(void)
{
    struct bt_smp_manager manager;
    struct fake_manager_port port;
    uint8_t value[16];
    uint8_t rand[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    uint8_t identity[6] = {6, 5, 4, 3, 2, 1};
    uint8_t all = BT_SMP_KEYDIST_ENC_KEY | BT_SMP_KEYDIST_ID_KEY |
                  BT_SMP_KEYDIST_SIGN_KEY;

    init_manager(&manager, &port, 0);
    manager.state = BT_SMP_STATE_WAIT_ENCRYPTION;
    manager.negotiation.initiator_key_distribution = all;
    manager.negotiation.responder_key_distribution = all;
    bt_smp_manager_on_encryption_changed(&manager, true, 10);
    BT_CHECK(manager.state == BT_SMP_STATE_WAIT_KEY_DISTRIBUTION);

    memset(value, 0x11, sizeof(value));
    feed_value(&manager, BT_SMP_ENCRYPTION_INFORMATION, value, 20);
    feed_central_identification(&manager, rand, 0x1234, 30);
    memset(value, 0x22, sizeof(value));
    feed_value(&manager, BT_SMP_IDENTITY_INFORMATION, value, 40);
    feed_identity_address(&manager, 0, identity, 50);
    memset(value, 0x33, sizeof(value));
    feed_value(&manager, BT_SMP_SIGNING_INFORMATION, value, 60);

    BT_CHECK(manager.state == BT_SMP_STATE_COMPLETE);
    BT_CHECK(port.keys_complete_calls == 1);
    BT_CHECK(port.peer_keys.key_mask == all);
    BT_CHECK(port.peer_keys.ediv == 0x1234);
    BT_CHECK(memcmp(port.peer_keys.rand, rand, 8) == 0);
    BT_CHECK(port.sent[0] == BT_SMP_SIGNING_INFORMATION); /* last local key */

    init_manager(&manager, &port, 0);
    manager.state = BT_SMP_STATE_WAIT_ENCRYPTION;
    manager.negotiation.responder_key_distribution =
        BT_SMP_KEYDIST_ENC_KEY | BT_SMP_KEYDIST_ID_KEY;
    bt_smp_manager_on_encryption_changed(&manager, true, 10);
    memset(value, 0x22, sizeof(value));
    feed_value(&manager, BT_SMP_IDENTITY_INFORMATION, value, 20); /* out of order */
    BT_CHECK(manager.state == BT_SMP_STATE_FAILED);
    BT_CHECK(port.result == BT_SMP_MANAGER_ERROR_PROTOCOL);
    BT_CHECK(port.sent[0] == BT_SMP_PAIRING_FAILED && port.sent[1] == 0x0A);
}

/* Responder-role Secure Connections: an initiator manager and a responder
 * manager exchange real PDUs through queues. The CMAC is the deterministic
 * fake above, so both sides agree only if they feed f4/f5/f6/g2 the same
 * values in the same order the specification requires. */
#define LOOP_QUEUE 8

struct loop_side
{
    struct bt_smp_manager m;
    struct loop_side *peer;
    uint8_t inbox[LOOP_QUEUE][65];
    size_t inbox_len[LOOP_QUEUE];
    size_t inbox_count;
    uint8_t next_random;
    uint8_t public_x[32];
    uint8_t public_y[32];
    bool public_key_requested;
    bool dhkey_requested;
    bool user_pending;
    enum bt_smp_user_action user_action;
    uint32_t user_value;
    int encryption_calls;
    uint8_t stk[16];
    int complete_calls;
    enum bt_smp_manager_result result;
    struct bt_smp_pairing_negotiation negotiation;
    uint8_t keys_complete_mask;
    uint8_t local_keys_complete_mask;
    bool pdus_first;   /* a slow controller: PDUs overtake its crypto events */
};

static bt_status_t loop_send(void *context, const uint8_t *pdu, size_t len)
{
    struct loop_side *side = context;
    struct loop_side *peer = side->peer;

    if (peer->inbox_count == LOOP_QUEUE || len > sizeof(peer->inbox[0]))
        return BT_ERR_NO_RESOURCES;
    memcpy(peer->inbox[peer->inbox_count], pdu, len);
    peer->inbox_len[peer->inbox_count++] = len;
    return BT_OK;
}

static bt_status_t loop_random(void *context, uint8_t *out, size_t len)
{
    struct loop_side *side = context;
    size_t i;

    for (i = 0; i < len; ++i)
        out[i] = side->next_random++;
    return BT_OK;
}

static bt_status_t loop_start_encryption(void *context, const uint8_t stk[16],
                                          uint8_t key_size)
{
    struct loop_side *side = context;

    (void)key_size;
    ++side->encryption_calls;
    memcpy(side->stk, stk, 16);
    return BT_OK;
}

static bt_status_t loop_generate_public_key(void *context)
{
    ((struct loop_side *)context)->public_key_requested = true;
    return BT_OK;
}

static bt_status_t loop_generate_dhkey(void *context, const uint8_t peer_x[32],
                                       const uint8_t peer_y[32])
{
    struct loop_side *side = context;

    BT_CHECK(memcmp(peer_x, side->peer->public_x, 32) == 0);
    BT_CHECK(memcmp(peer_y, side->peer->public_y, 32) == 0);
    side->dhkey_requested = true;
    return BT_OK;
}

static void loop_keys_complete(void *context, const struct bt_smp_distributed_keys *peer,
                               const struct bt_smp_distributed_keys *local)
{
    struct loop_side *side = context;

    side->keys_complete_mask = peer->key_mask;
    side->local_keys_complete_mask = local->key_mask;
}

static void loop_user(void *context, enum bt_smp_user_action action, uint32_t value)
{
    struct loop_side *side = context;

    side->user_pending = true;
    side->user_action = action;
    side->user_value = value;
}

static void loop_complete(void *context, enum bt_smp_manager_result result,
                          const struct bt_smp_pairing_negotiation *negotiation)
{
    struct loop_side *side = context;

    ++side->complete_calls;
    side->result = result;
    side->negotiation = *negotiation;
}

static const struct bt_smp_manager_ops loop_ops = {
    .send = loop_send,
    .random = loop_random,
    .start_encryption = loop_start_encryption,
    .generate_public_key = loop_generate_public_key,
    .generate_dhkey = loop_generate_dhkey,
    .get_local_keys = fake_get_local_keys,
    .keys_complete = loop_keys_complete,
    .user_action = loop_user,
    .complete = loop_complete};

static void loop_init(struct loop_side *side, struct loop_side *peer, uint8_t io,
                      uint8_t auth_req, uint8_t seed)
{
    struct bt_smp_manager_config config;
    struct bt_smp_aes128 aes = {identity_aes, NULL};
    struct bt_smp_aes_cmac cmac = {fake_cmac, NULL};
    size_t i;

    memset(side, 0, sizeof(*side));
    memset(&config, 0, sizeof(config));
    config.features.io_capability = io;
    config.features.auth_req = auth_req;
    config.features.max_encryption_key_size = 16;
    config.features.initiator_key_distribution = BT_SMP_KEYDIST_ID_KEY;
    config.features.responder_key_distribution =
        BT_SMP_KEYDIST_ENC_KEY | BT_SMP_KEYDIST_ID_KEY;
    config.responder_address_type = 1;
    for (i = 0; i < 6; ++i)
    {
        config.initiator_address[i] = (uint8_t)(0xA1u + i);
        config.responder_address[i] = (uint8_t)(0xB1u + i);
    }
    for (i = 0; i < 32; ++i)
    {
        side->public_x[i] = (uint8_t)(seed + i);
        side->public_y[i] = (uint8_t)(seed ^ (uint8_t)(3u * i));
    }
    side->peer = peer;
    side->next_random = seed;
    bt_smp_manager_init(&side->m, &config, &aes, &loop_ops, side);
    bt_smp_manager_set_cmac(&side->m, &cmac);
}

/* Completes the port's asynchronous crypto, then delivers queued PDUs, until
 * both sides are idle or wait for their user. A controller answers its own
 * P-256 commands before the peer can react to the same exchange. The responder accepts the
 * initiator's Pairing Request the way hwconn.c does. */
static void loop_pump(struct loop_side *initiator, struct loop_side *responder,
                      uint64_t *now_us)
{
    static const uint8_t dhkey[32] = {0x5d, 0x4b, 0x13, 0x7e};
    struct loop_side *sides[2];
    bool progress = true;
    size_t i;

    sides[0] = initiator;
    sides[1] = responder;
    while (progress)
    {
        progress = false;
        for (i = 0; i < 2; ++i)
        {
            struct loop_side *side = sides[i];
            uint8_t pdu[65];
            size_t len;

            if (side->inbox_count != 0 && side->pdus_first)
                goto deliver;
            if (side->public_key_requested)
            {
                side->public_key_requested = false;
                bt_smp_manager_on_local_public_key(&side->m, true, side->public_x,
                                                   side->public_y, *now_us);
                progress = true;
            }
            else if (side->dhkey_requested)
            {
                side->dhkey_requested = false;
                bt_smp_manager_on_dhkey(&side->m, true, dhkey, *now_us);
                progress = true;
            }
            else if (side->inbox_count != 0)
            {
            deliver:
                len = side->inbox_len[0];
                memcpy(pdu, side->inbox[0], len);
                memmove(side->inbox[0], side->inbox[1],
                        (side->inbox_count - 1) * sizeof(side->inbox[0]));
                memmove(side->inbox_len, side->inbox_len + 1,
                        (side->inbox_count - 1) * sizeof(side->inbox_len[0]));
                --side->inbox_count;
                *now_us += 10;
                if (side == responder && side->m.state == BT_SMP_STATE_IDLE)
                    BT_CHECK(bt_smp_manager_accept(&side->m, pdu, len, *now_us) == BT_OK);
                else
                    bt_smp_manager_on_pdu(&side->m, pdu, len, *now_us);
                progress = true;
            }
        }
    }
}

static void loop_encrypt(struct loop_side *initiator, struct loop_side *responder,
                         uint64_t *now_us)
{
    BT_CHECK(initiator->m.state == BT_SMP_STATE_WAIT_ENCRYPTION);
    BT_CHECK(responder->m.state == BT_SMP_STATE_WAIT_ENCRYPTION);
    BT_CHECK(initiator->encryption_calls == 1 && responder->encryption_calls == 1);
    BT_CHECK(memcmp(initiator->stk, responder->stk, 16) == 0);
    /* Phase 3 order: the initiator waits for the responder's keys, even
     * when its own Encryption Change is reported first. */
    bt_smp_manager_on_encryption_changed(&initiator->m, true, ++*now_us);
    BT_CHECK(initiator->m.state == BT_SMP_STATE_WAIT_KEY_DISTRIBUTION);
    BT_CHECK(responder->inbox_count == 0);
    bt_smp_manager_on_encryption_changed(&responder->m, true, ++*now_us);
    BT_CHECK(initiator->inbox_count == 2); /* IdentityInfo + IdentityAddress */
    BT_CHECK(responder->inbox_count == 0);
    loop_pump(initiator, responder, now_us);
    BT_CHECK(initiator->complete_calls == 1 && initiator->result == BT_SMP_MANAGER_OK);
    BT_CHECK(responder->complete_calls == 1 && responder->result == BT_SMP_MANAGER_OK);
    /* SC: the LTK is f5-derived, so only identities travel */
    BT_CHECK(initiator->keys_complete_mask ==
             (BT_SMP_KEYDIST_ENC_KEY | BT_SMP_KEYDIST_ID_KEY));
    BT_CHECK(responder->keys_complete_mask ==
             (BT_SMP_KEYDIST_ENC_KEY | BT_SMP_KEYDIST_ID_KEY));
    BT_CHECK(initiator->local_keys_complete_mask ==
             (BT_SMP_KEYDIST_ENC_KEY | BT_SMP_KEYDIST_ID_KEY));
    BT_CHECK(responder->local_keys_complete_mask ==
             (BT_SMP_KEYDIST_ENC_KEY | BT_SMP_KEYDIST_ID_KEY));
}

static void loop_start(struct loop_side *initiator, struct loop_side *responder,
                       uint8_t initiator_io, uint8_t initiator_auth,
                       uint8_t responder_io, uint8_t responder_auth, uint64_t *now_us)
{
    loop_init(initiator, responder, initiator_io, initiator_auth, 0x10);
    loop_init(responder, initiator, responder_io, responder_auth, 0x80);
    *now_us = 0;
    BT_CHECK(bt_smp_manager_start(&initiator->m, 0) == BT_OK);
    loop_pump(initiator, responder, now_us);
}

static void test_sc_responder_just_works(void)
{
    struct loop_side initiator, responder;
    uint64_t now = 0;
    uint8_t sc_bond = BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_BONDING;

    loop_start(&initiator, &responder, 0x03, sc_bond, 0x04,
               sc_bond | BT_SMP_AUTHREQ_MITM, &now);
    BT_CHECK(responder.m.responder && responder.m.negotiation.secure_connections);
    BT_CHECK(initiator.m.negotiation.responder_key_distribution == BT_SMP_KEYDIST_ID_KEY);
    BT_CHECK(responder.m.negotiation.responder_key_distribution == BT_SMP_KEYDIST_ID_KEY);
    BT_CHECK(responder.m.negotiation.association == BT_SMP_ASSOC_JUST_WORKS);
    BT_CHECK(!initiator.user_pending && !responder.user_pending);
    loop_encrypt(&initiator, &responder, &now);
    BT_CHECK(initiator.negotiation.secure_connections &&
             responder.negotiation.secure_connections);
}

static void test_sc_responder_numeric_comparison(bool initiator_first)
{
    struct loop_side initiator, responder;
    uint64_t now = 0;
    uint8_t auth = BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_BONDING | BT_SMP_AUTHREQ_MITM;

    loop_start(&initiator, &responder, 0x01, auth, 0x04, auth, &now);
    BT_CHECK(responder.m.negotiation.association == BT_SMP_ASSOC_NUMERIC_COMPARISON);
    BT_CHECK(initiator.user_pending && responder.user_pending);
    BT_CHECK(initiator.user_action == BT_SMP_USER_CONFIRM_NUMERIC);
    BT_CHECK(responder.user_action == BT_SMP_USER_CONFIRM_NUMERIC);
    BT_CHECK(initiator.user_value == responder.user_value);
    BT_CHECK(responder.user_value < 1000000u);
    if (initiator_first)
    {
        /* Ea reaches the responder before its user answered: it waits */
        BT_CHECK(bt_smp_manager_confirm_numeric(&initiator.m, true, ++now) == BT_OK);
        loop_pump(&initiator, &responder, &now);
        BT_CHECK(responder.m.peer_dhkey_check_pending);
        BT_CHECK(responder.m.state == BT_SMP_STATE_WAIT_NUMERIC_CONFIRMATION);
        BT_CHECK(responder.encryption_calls == 0);
        BT_CHECK(bt_smp_manager_confirm_numeric(&responder.m, true, ++now) == BT_OK);
    }
    else
    {
        BT_CHECK(bt_smp_manager_confirm_numeric(&responder.m, true, ++now) == BT_OK);
        BT_CHECK(responder.m.state == BT_SMP_STATE_WAIT_DHKEY_CHECK);
        BT_CHECK(bt_smp_manager_confirm_numeric(&initiator.m, true, ++now) == BT_OK);
    }
    loop_pump(&initiator, &responder, &now);
    loop_encrypt(&initiator, &responder, &now);
    BT_CHECK(responder.negotiation.mitm_requested);
}

/* The upstream central must not fail when the responder's Cb overtakes the
 * DHKey event from its own controller. */
static void test_sc_initiator_early_responder_confirm(void)
{
    struct loop_side initiator, responder;
    uint64_t now = 0;
    uint8_t auth = BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_BONDING | BT_SMP_AUTHREQ_MITM;

    loop_init(&initiator, &responder, 0x01, auth, 0x10);
    loop_init(&responder, &initiator, 0x03, auth, 0x80);
    initiator.pdus_first = true;
    BT_CHECK(bt_smp_manager_start(&initiator.m, 0) == BT_OK);
    loop_pump(&initiator, &responder, &now);
    BT_CHECK(!initiator.m.peer_confirm_pending);
    BT_CHECK(initiator.m.negotiation.association == BT_SMP_ASSOC_JUST_WORKS);
    loop_encrypt(&initiator, &responder, &now);

    loop_init(&initiator, &responder, 0x01, auth, 0x10);
    loop_init(&responder, &initiator, 0x01, auth, 0x80);
    initiator.pdus_first = true;
    BT_CHECK(bt_smp_manager_start(&initiator.m, 0) == BT_OK);
    now = 0;
    loop_pump(&initiator, &responder, &now);
    BT_CHECK(initiator.m.state == BT_SMP_STATE_WAIT_NUMERIC_CONFIRMATION);
    BT_CHECK(initiator.user_value == responder.user_value);
}

static void test_sc_responder_numeric_rejected(void)
{
    struct loop_side initiator, responder;
    uint64_t now = 0;
    uint8_t auth = BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_BONDING | BT_SMP_AUTHREQ_MITM;

    loop_start(&initiator, &responder, 0x01, auth, 0x01, auth, &now);
    BT_CHECK(bt_smp_manager_confirm_numeric(&responder.m, false, ++now) == BT_OK);
    loop_pump(&initiator, &responder, &now);
    BT_CHECK(responder.result == BT_SMP_MANAGER_ERROR_CONFIRM);
    BT_CHECK(initiator.m.state == BT_SMP_STATE_FAILED);
    BT_CHECK(responder.encryption_calls == 0);
}

static void test_sc_responder_passkey(uint8_t initiator_io, uint8_t responder_io,
                                      enum bt_smp_association_method association,
                                      bool wrong_passkey)
{
    struct loop_side initiator, responder;
    struct loop_side *display = NULL;
    uint64_t now = 0;
    uint8_t auth = BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_BONDING | BT_SMP_AUTHREQ_MITM;
    uint32_t passkey = 424242u;

    loop_start(&initiator, &responder, initiator_io, auth, responder_io, auth, &now);
    BT_CHECK(responder.m.negotiation.association == association);
    BT_CHECK(initiator.user_pending && responder.user_pending);
    if (initiator.user_action == BT_SMP_USER_DISPLAY_PASSKEY)
        display = &initiator;
    else if (responder.user_action == BT_SMP_USER_DISPLAY_PASSKEY)
        display = &responder;
    if (display != NULL)
    {
        passkey = display->user_value;
        BT_CHECK(passkey < 1000000u);
    }
    /* the initiator's typist is fast: its round-0 commitment is queued
     * before the responder's user has entered anything */
    if (initiator.user_action == BT_SMP_USER_REQUEST_PASSKEY)
    {
        BT_CHECK(bt_smp_manager_provide_passkey(&initiator.m, passkey, ++now) == BT_OK);
        loop_pump(&initiator, &responder, &now);
    }
    if (responder.user_action == BT_SMP_USER_REQUEST_PASSKEY)
    {
        if (initiator.user_action == BT_SMP_USER_REQUEST_PASSKEY)
            BT_CHECK(responder.m.peer_confirm_pending);
        BT_CHECK(bt_smp_manager_provide_passkey(&responder.m,
                                                wrong_passkey ? passkey ^ 0x400u : passkey,
                                                ++now) == BT_OK);
    }
    loop_pump(&initiator, &responder, &now);
    if (wrong_passkey)
    {
        BT_CHECK(initiator.m.state == BT_SMP_STATE_FAILED);
        BT_CHECK(responder.m.state == BT_SMP_STATE_FAILED);
        BT_CHECK(responder.encryption_calls == 0);
        return;
    }
    BT_CHECK(initiator.m.sc_round == 20 && responder.m.sc_round == 20);
    loop_encrypt(&initiator, &responder, &now);
}

static void test_sc_responder_fallbacks(void)
{
    struct loop_side initiator, responder;
    uint64_t now = 0;
    uint8_t request[7] = {BT_SMP_PAIRING_REQUEST, 0x04, 0x01,
                          BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_BONDING, 16,
                          BT_SMP_KEYDIST_ID_KEY, BT_SMP_KEYDIST_ID_KEY};

    /* a legacy-only central gets legacy Just Works, whatever we offer */
    loop_start(&initiator, &responder, 0x01, BT_SMP_AUTHREQ_BONDING | BT_SMP_AUTHREQ_MITM,
               0x04, BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_BONDING | BT_SMP_AUTHREQ_MITM, &now);
    BT_CHECK(!responder.m.negotiation.secure_connections);
    BT_CHECK(responder.m.negotiation.association == BT_SMP_ASSOC_JUST_WORKS);
    BT_CHECK(responder.m.pres[5] == 0x03); /* NoInputNoOutput in the response */
    BT_CHECK(initiator.m.state == BT_SMP_STATE_WAIT_ENCRYPTION);
    BT_CHECK(memcmp(initiator.stk, responder.stk, 16) == 0);

    /* an SC central with OOB data would select OOB: answer legacy instead */
    loop_init(&responder, &initiator, 0x04,
              BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_BONDING | BT_SMP_AUTHREQ_MITM, 0x80);
    BT_CHECK(bt_smp_manager_accept(&responder.m, request, sizeof(request), 1) == BT_OK);
    BT_CHECK(!responder.m.negotiation.secure_connections);
    BT_CHECK(responder.m.state == BT_SMP_STATE_WAIT_PAIRING_CONFIRM);

    /* without CMAC there is no Secure Connections */
    loop_init(&responder, &initiator, 0x04,
              BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_BONDING, 0x80);
    bt_smp_manager_set_cmac(&responder.m, NULL);
    request[2] = 0;
    BT_CHECK(bt_smp_manager_accept(&responder.m, request, sizeof(request), 1) == BT_OK);
    BT_CHECK(!responder.m.negotiation.secure_connections);

    /* a reflected public key is refused */
    loop_init(&responder, &initiator, 0x03, BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_BONDING, 0x80);
    BT_CHECK(bt_smp_manager_accept(&responder.m, request, sizeof(request), 1) == BT_OK);
    BT_CHECK(responder.m.state == BT_SMP_STATE_WAIT_PEER_PUBLIC_KEY);
    feed_public_key(&responder.m, responder.public_x, responder.public_y, 2);
    BT_CHECK(responder.public_key_requested);
    bt_smp_manager_on_local_public_key(&responder.m, true, responder.public_x,
                                       responder.public_y, 3);
    BT_CHECK(responder.m.state == BT_SMP_STATE_FAILED);
    BT_CHECK(responder.result == BT_SMP_MANAGER_ERROR_PROTOCOL);

    /* a commitment in Just Works is a protocol violation */
    loop_init(&responder, &initiator, 0x03, BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_BONDING, 0x80);
    BT_CHECK(bt_smp_manager_accept(&responder.m, request, sizeof(request), 1) == BT_OK);
    {
        uint8_t value[16] = {0};
        feed_value(&responder.m, BT_SMP_PAIRING_CONFIRM, value, 2);
    }
    BT_CHECK(responder.m.state == BT_SMP_STATE_FAILED);
}

void run_smp_manager_tests(void)
{
    test_legacy_just_works_success();
    test_legacy_responder_just_works();
    test_confirm_failure_and_timeout();
    test_passkey_input();
    test_secure_connections_just_works();
    test_secure_connections_numeric_confirmation();
    test_secure_connections_passkey_entry();
    test_full_key_distribution_and_order_validation();
    test_sc_responder_just_works();
    test_sc_responder_numeric_comparison(true);
    test_sc_responder_numeric_comparison(false);
    test_sc_responder_numeric_rejected();
    test_sc_initiator_early_responder_confirm();
    test_sc_responder_passkey(0x02, 0x04, BT_SMP_ASSOC_PASSKEY_RESPONDER_DISPLAYS, false);
    test_sc_responder_passkey(0x00, 0x04, BT_SMP_ASSOC_PASSKEY_INITIATOR_DISPLAYS, false);
    test_sc_responder_passkey(0x02, 0x02, BT_SMP_ASSOC_PASSKEY_BOTH_INPUT, false);
    test_sc_responder_passkey(0x02, 0x02, BT_SMP_ASSOC_PASSKEY_BOTH_INPUT, true);
    test_sc_responder_fallbacks();
}
