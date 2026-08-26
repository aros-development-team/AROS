#ifndef BTCORE_SMP_MANAGER_H
#define BTCORE_SMP_MANAGER_H

#include <btcore/smp_crypto.h>
#include <btcore/smp_pairing.h>

#define BT_SMP_TIMEOUT_US 30000000ULL

enum bt_smp_manager_state
{
    BT_SMP_STATE_IDLE,
    BT_SMP_STATE_WAIT_PAIRING_RESPONSE,
    BT_SMP_STATE_WAIT_LOCAL_PUBLIC_KEY,
    BT_SMP_STATE_WAIT_PEER_PUBLIC_KEY,
    BT_SMP_STATE_WAIT_DHKEY,
    BT_SMP_STATE_WAIT_SC_CONFIRM,
    BT_SMP_STATE_WAIT_SC_RANDOM,
    BT_SMP_STATE_WAIT_NUMERIC_CONFIRMATION,
    BT_SMP_STATE_WAIT_DHKEY_CHECK,
    BT_SMP_STATE_WAIT_TK,
    BT_SMP_STATE_WAIT_PAIRING_CONFIRM,
    BT_SMP_STATE_WAIT_PAIRING_RANDOM,
    BT_SMP_STATE_WAIT_ENCRYPTION,
    BT_SMP_STATE_WAIT_KEY_DISTRIBUTION,
    BT_SMP_STATE_COMPLETE,
    BT_SMP_STATE_FAILED
};

enum bt_smp_user_action
{
    BT_SMP_USER_DISPLAY_PASSKEY,
    BT_SMP_USER_REQUEST_PASSKEY,
    BT_SMP_USER_REQUEST_OOB_TK,
    BT_SMP_USER_CONFIRM_NUMERIC
};

enum bt_smp_manager_result
{
    BT_SMP_MANAGER_OK,
    BT_SMP_MANAGER_ERROR_PROTOCOL,
    BT_SMP_MANAGER_ERROR_CONFIRM,
    BT_SMP_MANAGER_ERROR_TIMEOUT,
    BT_SMP_MANAGER_ERROR_CRYPTO,
    BT_SMP_MANAGER_ERROR_ENCRYPTION,
    BT_SMP_MANAGER_ERROR_UNSUPPORTED
};

struct bt_smp_distributed_keys
{
    /* For LE Secure Connections, ENC_KEY identifies the shared f5-derived
     * LTK. rand and ediv are zero because they are not distributed in SC. */
    uint8_t key_mask;
    uint8_t ltk[16];
    uint8_t rand[8];
    uint16_t ediv;
    uint8_t irk[16];
    uint8_t identity_address_type;
    uint8_t identity_address[6];
    uint8_t csrk[16];
};

struct bt_smp_manager_ops
{
    bt_status_t (*send)(void *context, const uint8_t *pdu, size_t pdu_len);
    bt_status_t (*random)(void *context, uint8_t *out, size_t len);
    bt_status_t (*start_encryption)(void *context, const uint8_t stk[16],
                                    uint8_t key_size);
    bt_status_t (*generate_public_key)(void *context);
    bt_status_t (*generate_dhkey)(void *context, const uint8_t peer_x[32],
                                  const uint8_t peer_y[32]);
    bt_status_t (*get_local_keys)(void *context, uint8_t key_mask,
                                  struct bt_smp_distributed_keys *out);
    void (*keys_complete)(void *context,
                          const struct bt_smp_distributed_keys *peer,
                          const struct bt_smp_distributed_keys *local);
    void (*user_action)(void *context, enum bt_smp_user_action action,
                        uint32_t passkey);
    void (*complete)(void *context, enum bt_smp_manager_result result,
                     const struct bt_smp_pairing_negotiation *negotiation);
};

struct bt_smp_manager_config
{
    struct bt_smp_pairing_features features;
    /* Addresses in Core/FIPS display order: most-significant octet first. */
    uint8_t initiator_address_type;
    uint8_t responder_address_type;
    uint8_t initiator_address[6];
    uint8_t responder_address[6];
};

struct bt_smp_manager
{
    const struct bt_smp_manager_ops *ops;
    void *context;
    struct bt_smp_aes128 aes;
    struct bt_smp_aes_cmac cmac;
    struct bt_smp_manager_config config;
    enum bt_smp_manager_state state;
    struct bt_smp_pairing_negotiation negotiation;
    uint64_t deadline_us;

    uint8_t preq[7];
    uint8_t pres[7];
    uint8_t tk[16];
    uint8_t local_random[16];
    uint8_t peer_confirm[16];
    uint8_t stk[16];
    struct bt_smp_distributed_keys peer_keys;
    struct bt_smp_distributed_keys local_keys;
    uint8_t peer_keys_complete_mask;
    bool peer_ltk_received;
    bool peer_irk_received;
    uint8_t local_public_x[32];
    uint8_t local_public_y[32];
    uint8_t peer_public_x[32];
    uint8_t peer_public_y[32];
    uint8_t dhkey[32];
    uint8_t peer_random[16];
    uint8_t mac_key[16];
    uint8_t peer_dhkey_check[16];
    uint32_t numeric_value;
    uint8_t sc_round;              /* Passkey Entry: commitment round 0..19 */
};

void bt_smp_manager_init(struct bt_smp_manager *manager,
                          const struct bt_smp_manager_config *config,
                          const struct bt_smp_aes128 *aes,
                          const struct bt_smp_manager_ops *ops, void *context);
void bt_smp_manager_set_cmac(struct bt_smp_manager *manager,
                              const struct bt_smp_aes_cmac *cmac);

/* Central/initiator role. Sends Pairing Request immediately. */
bt_status_t bt_smp_manager_start(struct bt_smp_manager *manager, uint64_t now_us);

/* Feed one complete PDU received on BT_L2CAP_CID_SMP. */
void bt_smp_manager_on_pdu(struct bt_smp_manager *manager, const uint8_t *pdu,
                            size_t pdu_len, uint64_t now_us);

/* Satisfies REQUEST_PASSKEY (0..999999). For OOB, provide the complete TK. */
bt_status_t bt_smp_manager_provide_passkey(struct bt_smp_manager *manager,
                                            uint32_t passkey, uint64_t now_us);
bt_status_t bt_smp_manager_provide_oob_tk(struct bt_smp_manager *manager,
                                           const uint8_t tk[16], uint64_t now_us);
bt_status_t bt_smp_manager_confirm_numeric(struct bt_smp_manager *manager,
                                            bool accepted, uint64_t now_us);

void bt_smp_manager_on_local_public_key(struct bt_smp_manager *manager, bool success,
                                         const uint8_t x[32], const uint8_t y[32],
                                         uint64_t now_us);
void bt_smp_manager_on_dhkey(struct bt_smp_manager *manager, bool success,
                              const uint8_t dhkey[32], uint64_t now_us);

/* Called after the Controller reports Encryption Change. */
void bt_smp_manager_on_encryption_changed(struct bt_smp_manager *manager, bool enabled,
                                           uint64_t now_us);
void bt_smp_manager_tick(struct bt_smp_manager *manager, uint64_t now_us);

#endif /* BTCORE_SMP_MANAGER_H */
