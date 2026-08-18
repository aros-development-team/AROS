#include <btcore/smp_pairing.h>

static bool features_valid(const struct bt_smp_pairing_features *f)
{
    return f != NULL && f->io_capability <= 4 && f->oob_data_flag <= 1 &&
           (f->auth_req & ~BT_SMP_AUTHREQ_VALID_MASK) == 0 &&
           (f->auth_req & 0x03u) <= BT_SMP_AUTHREQ_BONDING &&
           f->max_encryption_key_size >= 7 && f->max_encryption_key_size <= 16 &&
           (f->initiator_key_distribution & ~BT_SMP_KEYDIST_VALID_MASK) == 0 &&
           (f->responder_key_distribution & ~BT_SMP_KEYDIST_VALID_MASK) == 0;
}

static bool has_yes_no(uint8_t io)
{
    return io == 0x01u || io == 0x04u; /* DisplayYesNo or KeyboardDisplay */
}

static enum bt_smp_association_method io_association(uint8_t initiator, uint8_t responder,
                                                      bool secure_connections)
{
    if (secure_connections && has_yes_no(initiator) && has_yes_no(responder))
        return BT_SMP_ASSOC_NUMERIC_COMPARISON;
    if (initiator == 0x03u || responder == 0x03u)
        return BT_SMP_ASSOC_JUST_WORKS; /* either side NoInputNoOutput */

    if (initiator == 0x02u) /* initiator KeyboardOnly */
    {
        if (responder == 0x02u)
            return BT_SMP_ASSOC_PASSKEY_BOTH_INPUT;
        if (responder == 0x00u || responder == 0x01u || responder == 0x04u)
            return BT_SMP_ASSOC_PASSKEY_RESPONDER_DISPLAYS;
    }
    if (responder == 0x02u) /* responder KeyboardOnly */
    {
        if (initiator == 0x00u || initiator == 0x01u || initiator == 0x04u)
            return BT_SMP_ASSOC_PASSKEY_INITIATOR_DISPLAYS;
    }
    return BT_SMP_ASSOC_JUST_WORKS;
}

bt_status_t bt_smp_negotiate_pairing(const struct bt_smp_pairing_features *request,
                                      const struct bt_smp_pairing_features *response,
                                      struct bt_smp_pairing_negotiation *out)
{
    bool initiator_mitm;
    bool responder_mitm;

    if (!features_valid(request) || !features_valid(response) || out == NULL)
        return BT_ERR_INVALID_ARGUMENT;
    /* A Pairing Response is not allowed to add key-distribution flags that
     * were absent from the request. Treat that as invalid parameters rather
     * than silently accepting an inconsistent feature exchange. */
    if ((response->initiator_key_distribution & ~request->initiator_key_distribution) != 0 ||
        (response->responder_key_distribution & ~request->responder_key_distribution) != 0)
        return BT_ERR_INVALID_ARGUMENT;

    out->secure_connections =
        (request->auth_req & response->auth_req & BT_SMP_AUTHREQ_SC) != 0;
    out->bonding =
        (request->auth_req & response->auth_req & BT_SMP_AUTHREQ_BONDING) != 0;
    initiator_mitm = (request->auth_req & BT_SMP_AUTHREQ_MITM) != 0;
    responder_mitm = (response->auth_req & BT_SMP_AUTHREQ_MITM) != 0;
    out->mitm_requested = initiator_mitm || responder_mitm;
    out->encryption_key_size =
        request->max_encryption_key_size < response->max_encryption_key_size
            ? request->max_encryption_key_size
            : response->max_encryption_key_size;
    out->initiator_key_distribution = response->initiator_key_distribution;
    out->responder_key_distribution = response->responder_key_distribution;

    if ((out->secure_connections &&
         (request->oob_data_flag != 0 || response->oob_data_flag != 0)) ||
        (!out->secure_connections && request->oob_data_flag != 0 &&
         response->oob_data_flag != 0))
        out->association = BT_SMP_ASSOC_OOB;
    else if (!out->mitm_requested)
        out->association = BT_SMP_ASSOC_JUST_WORKS;
    else
        out->association =
            io_association(request->io_capability, response->io_capability,
                           out->secure_connections);
    return BT_OK;
}
