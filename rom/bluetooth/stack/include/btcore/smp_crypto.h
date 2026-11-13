#ifndef BTCORE_SMP_CRYPTO_H
#define BTCORE_SMP_CRYPTO_H

#include <btcore/status.h>
#include <btcore/types.h>

/* AES is injected by the port: it may be backed by a reviewed software
 * library, hardware, or HCI LE Encrypt. Arrays use the FIPS/Core-spec
 * convention: most-significant octet first. */
typedef bt_status_t (*bt_smp_aes128_fn)(void *context, const uint8_t key[16],
                                        const uint8_t plaintext[16],
                                        uint8_t ciphertext[16]);

struct bt_smp_aes128
{
    bt_smp_aes128_fn encrypt;
    void *context;
};

typedef bt_status_t (*bt_smp_aes_cmac_fn)(void *context, const uint8_t key[16],
                                          const uint8_t *message, size_t message_len,
                                          uint8_t mac[16]);

struct bt_smp_aes_cmac
{
    bt_smp_aes_cmac_fn calculate;
    void *context;
};

struct bt_smp_cmac_aes128
{
    struct bt_smp_aes128 aes;
};

void bt_smp_cmac_aes128_init(struct bt_smp_cmac_aes128 *context,
                              const struct bt_smp_aes128 *aes);
/* Matches bt_smp_aes_cmac_fn; set bt_smp_aes_cmac.context to a
 * bt_smp_cmac_aes128 and calculate to this function. */
bt_status_t bt_smp_cmac_aes128_calculate(void *context, const uint8_t key[16],
                                          const uint8_t *message, size_t message_len,
                                          uint8_t mac[16]);

/* LE legacy cryptographic toolbox, Core Vol 3 Part H 2.2.2-2.2.4.
 * preq/pres are the complete seven-octet commands in spec display order;
 * ia/ra are six-octet addresses in most-significant-octet-first order. */
bt_status_t bt_smp_crypto_ah(const struct bt_smp_aes128 *aes, const uint8_t irk[16],
                             const uint8_t prand[3], uint8_t hash[3]);
bt_status_t bt_smp_crypto_c1(const struct bt_smp_aes128 *aes, const uint8_t key[16],
                             const uint8_t random[16], const uint8_t preq[7],
                             const uint8_t pres[7], uint8_t initiator_addr_type,
                             uint8_t responder_addr_type, const uint8_t initiator_addr[6],
                             const uint8_t responder_addr[6], uint8_t confirm[16]);
bt_status_t bt_smp_crypto_s1(const struct bt_smp_aes128 *aes, const uint8_t key[16],
                             const uint8_t r1[16], const uint8_t r2[16], uint8_t stk[16]);

/* LE Secure Connections toolbox, Core Vol 3 Part H 2.2.6-2.2.11. All
 * arrays are most-significant-octet first. A1/A2 are address type followed
 * by the six address octets. */
bt_status_t bt_smp_crypto_f4(const struct bt_smp_aes_cmac *cmac, const uint8_t u[32],
                             const uint8_t v[32], const uint8_t x[16], uint8_t z,
                             uint8_t out[16]);
bt_status_t bt_smp_crypto_f5(const struct bt_smp_aes_cmac *cmac, const uint8_t w[32],
                             const uint8_t n1[16], const uint8_t n2[16],
                             const uint8_t a1[7], const uint8_t a2[7],
                             uint8_t mac_key[16], uint8_t ltk[16]);
bt_status_t bt_smp_crypto_f6(const struct bt_smp_aes_cmac *cmac, const uint8_t w[16],
                             const uint8_t n1[16], const uint8_t n2[16],
                             const uint8_t r[16], const uint8_t io_cap[3],
                             const uint8_t a1[7], const uint8_t a2[7], uint8_t out[16]);
bt_status_t bt_smp_crypto_g2(const struct bt_smp_aes_cmac *cmac, const uint8_t u[32],
                             const uint8_t v[32], const uint8_t x[16], const uint8_t y[16],
                             uint32_t *out_value);
bt_status_t bt_smp_crypto_h6(const struct bt_smp_aes_cmac *cmac, const uint8_t w[16],
                             const uint8_t key_id[4], uint8_t out[16]);
bt_status_t bt_smp_crypto_h7(const struct bt_smp_aes_cmac *cmac, const uint8_t salt[16],
                             const uint8_t w[16], uint8_t out[16]);

#endif
