#ifndef BTCORE_SMP_H
#define BTCORE_SMP_H

#include <btcore/buffer.h>
#include <btcore/status.h>
#include <btcore/types.h>

/* Security Manager Protocol, Bluetooth Core Vol 3, Part H, section 3.
 * SMP is carried on fixed L2CAP CID 0x0006. Multi-octet integers are
 * little-endian; opaque 128/256-bit cryptographic values remain byte arrays. */
#define BT_SMP_CID 0x0006u

#define BT_SMP_PAIRING_REQUEST 0x01u
#define BT_SMP_PAIRING_RESPONSE 0x02u
#define BT_SMP_PAIRING_CONFIRM 0x03u
#define BT_SMP_PAIRING_RANDOM 0x04u
#define BT_SMP_PAIRING_FAILED 0x05u
#define BT_SMP_ENCRYPTION_INFORMATION 0x06u
#define BT_SMP_CENTRAL_IDENTIFICATION 0x07u
#define BT_SMP_IDENTITY_INFORMATION 0x08u
#define BT_SMP_IDENTITY_ADDRESS_INFORMATION 0x09u
#define BT_SMP_SIGNING_INFORMATION 0x0Au
#define BT_SMP_SECURITY_REQUEST 0x0Bu
#define BT_SMP_PAIRING_PUBLIC_KEY 0x0Cu
#define BT_SMP_PAIRING_DHKEY_CHECK 0x0Du
#define BT_SMP_PAIRING_KEYPRESS_NOTIFICATION 0x0Eu

#define BT_SMP_AUTHREQ_BONDING 0x01u
#define BT_SMP_AUTHREQ_MITM 0x04u
#define BT_SMP_AUTHREQ_SC 0x08u
#define BT_SMP_AUTHREQ_KEYPRESS 0x10u
#define BT_SMP_AUTHREQ_CT2 0x20u
#define BT_SMP_AUTHREQ_VALID_MASK 0x3Du

#define BT_SMP_KEYDIST_ENC_KEY 0x01u
#define BT_SMP_KEYDIST_ID_KEY 0x02u
#define BT_SMP_KEYDIST_SIGN_KEY 0x04u
#define BT_SMP_KEYDIST_LINK_KEY 0x08u
#define BT_SMP_KEYDIST_VALID_MASK 0x0Fu

struct bt_smp_command
{
    uint8_t code;
    const uint8_t *data;
    size_t data_len;
};

struct bt_smp_pairing_features
{
    uint8_t io_capability;
    uint8_t oob_data_flag;
    uint8_t auth_req;
    uint8_t max_encryption_key_size;
    uint8_t initiator_key_distribution;
    uint8_t responder_key_distribution;
};

struct bt_smp_central_identification
{
    const uint8_t *rand; /* 8 opaque octets, valid while input PDU is valid */
    uint16_t ediv;
};

struct bt_smp_identity_address
{
    uint8_t address_type;
    const uint8_t *address; /* 6 wire-order octets */
};

/* Validates a complete command, including its exact length and fields whose
 * ranges are defined by SMP. Reserved command codes are rejected. */
bt_status_t bt_smp_parse_command(const uint8_t *pdu, size_t pdu_len,
                                  struct bt_smp_command *out);

bt_status_t bt_smp_encode_pairing_features(struct bt_buf_writer *w, uint8_t code,
                                            const struct bt_smp_pairing_features *features);
bt_status_t bt_smp_parse_pairing_features(const struct bt_smp_command *command,
                                           struct bt_smp_pairing_features *out);

/* 128-bit payload commands: Confirm, Random, Encryption/Identity/Signing
 * Information and DHKey Check. */
bt_status_t bt_smp_encode_value128(struct bt_buf_writer *w, uint8_t code,
                                    const uint8_t value[16]);
bt_status_t bt_smp_parse_value128(const struct bt_smp_command *command,
                                   const uint8_t **out_value);

bt_status_t bt_smp_encode_central_identification(struct bt_buf_writer *w,
                                                  const uint8_t rand[8], uint16_t ediv);
bt_status_t bt_smp_parse_central_identification(const struct bt_smp_command *command,
                                                 struct bt_smp_central_identification *out);

bt_status_t bt_smp_encode_identity_address(struct bt_buf_writer *w, uint8_t address_type,
                                            const uint8_t address[6]);
bt_status_t bt_smp_parse_identity_address(const struct bt_smp_command *command,
                                           struct bt_smp_identity_address *out);

bt_status_t bt_smp_encode_public_key(struct bt_buf_writer *w, const uint8_t x[32],
                                      const uint8_t y[32]);
bt_status_t bt_smp_parse_public_key(const struct bt_smp_command *command,
                                     const uint8_t **out_x, const uint8_t **out_y);

/* One-octet payload commands: Pairing Failed, Security Request and Keypress. */
bt_status_t bt_smp_encode_u8_command(struct bt_buf_writer *w, uint8_t code, uint8_t value);

#endif /* BTCORE_SMP_H */
