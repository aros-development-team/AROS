#ifndef BTCORE_SMP_PAIRING_H
#define BTCORE_SMP_PAIRING_H

#include <btcore/smp.h>

enum bt_smp_association_method
{
    BT_SMP_ASSOC_JUST_WORKS,
    BT_SMP_ASSOC_OOB,
    BT_SMP_ASSOC_NUMERIC_COMPARISON,
    BT_SMP_ASSOC_PASSKEY_INITIATOR_DISPLAYS,
    BT_SMP_ASSOC_PASSKEY_RESPONDER_DISPLAYS,
    BT_SMP_ASSOC_PASSKEY_BOTH_INPUT
};

struct bt_smp_pairing_negotiation
{
    bool secure_connections;
    bool bonding;
    bool mitm_requested;
    uint8_t encryption_key_size;
    uint8_t initiator_key_distribution;
    uint8_t responder_key_distribution;
    enum bt_smp_association_method association;
};

/* Applies Core Vol 3 Part H 2.3.4/2.3.5.1 and Tables 2.6-2.8.
 * Request is the Central/initiator's features and response is the
 * Peripheral/responder's. Inputs must already have passed the SMP codec's
 * range validation. */
bt_status_t bt_smp_negotiate_pairing(const struct bt_smp_pairing_features *request,
                                      const struct bt_smp_pairing_features *response,
                                      struct bt_smp_pairing_negotiation *out);

#endif
