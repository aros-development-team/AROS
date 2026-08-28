#include "test_smp_pairing.h"
#include "../support/test.h"

#include <btcore/smp_pairing.h>

static struct bt_smp_pairing_features features(uint8_t io, uint8_t auth)
{
    struct bt_smp_pairing_features f = {
        io, 0, auth, 16, BT_SMP_KEYDIST_ID_KEY | BT_SMP_KEYDIST_SIGN_KEY,
        BT_SMP_KEYDIST_ID_KEY};
    return f;
}

static void test_basic_negotiation(void)
{
    struct bt_smp_pairing_features req =
        features(0x03, BT_SMP_AUTHREQ_BONDING | BT_SMP_AUTHREQ_SC);
    struct bt_smp_pairing_features rsp =
        features(0x03, BT_SMP_AUTHREQ_BONDING | BT_SMP_AUTHREQ_SC);
    struct bt_smp_pairing_negotiation out;

    rsp.max_encryption_key_size = 12;
    BT_CHECK(bt_smp_negotiate_pairing(&req, &rsp, &out) == BT_OK);
    BT_CHECK(out.secure_connections);
    BT_CHECK(out.bonding);
    BT_CHECK(!out.mitm_requested);
    BT_CHECK(out.encryption_key_size == 12);
    BT_CHECK(out.association == BT_SMP_ASSOC_JUST_WORKS);
}

static void test_association_models(void)
{
    struct bt_smp_pairing_features req;
    struct bt_smp_pairing_features rsp;
    struct bt_smp_pairing_negotiation out;

    req = features(0x01, BT_SMP_AUTHREQ_SC | BT_SMP_AUTHREQ_MITM);
    rsp = features(0x04, BT_SMP_AUTHREQ_SC);
    BT_CHECK(bt_smp_negotiate_pairing(&req, &rsp, &out) == BT_OK);
    BT_CHECK(out.association == BT_SMP_ASSOC_NUMERIC_COMPARISON);

    req = features(0x00, BT_SMP_AUTHREQ_MITM);
    rsp = features(0x02, 0);
    BT_CHECK(bt_smp_negotiate_pairing(&req, &rsp, &out) == BT_OK);
    BT_CHECK(!out.secure_connections);
    BT_CHECK(out.association == BT_SMP_ASSOC_PASSKEY_INITIATOR_DISPLAYS);

    req = features(0x02, BT_SMP_AUTHREQ_MITM);
    rsp = features(0x02, 0);
    BT_CHECK(bt_smp_negotiate_pairing(&req, &rsp, &out) == BT_OK);
    BT_CHECK(out.association == BT_SMP_ASSOC_PASSKEY_BOTH_INPUT);

    req = features(0x02, BT_SMP_AUTHREQ_MITM);
    rsp = features(0x00, 0);
    BT_CHECK(bt_smp_negotiate_pairing(&req, &rsp, &out) == BT_OK);
    BT_CHECK(out.association == BT_SMP_ASSOC_PASSKEY_RESPONDER_DISPLAYS);
}

static void test_oob_rules(void)
{
    struct bt_smp_pairing_features req = features(0x03, BT_SMP_AUTHREQ_SC);
    struct bt_smp_pairing_features rsp = features(0x03, BT_SMP_AUTHREQ_SC);
    struct bt_smp_pairing_negotiation out;

    req.oob_data_flag = 1;
    BT_CHECK(bt_smp_negotiate_pairing(&req, &rsp, &out) == BT_OK);
    BT_CHECK(out.association == BT_SMP_ASSOC_OOB); /* one side is enough for SC */

    req.auth_req &= (uint8_t)~BT_SMP_AUTHREQ_SC;
    rsp.auth_req &= (uint8_t)~BT_SMP_AUTHREQ_SC;
    BT_CHECK(bt_smp_negotiate_pairing(&req, &rsp, &out) == BT_OK);
    BT_CHECK(out.association == BT_SMP_ASSOC_JUST_WORKS); /* legacy needs both */
    rsp.oob_data_flag = 1;
    BT_CHECK(bt_smp_negotiate_pairing(&req, &rsp, &out) == BT_OK);
    BT_CHECK(out.association == BT_SMP_ASSOC_OOB);
}

static void test_invalid_response_escalation(void)
{
    struct bt_smp_pairing_features req = features(0x03, 0);
    struct bt_smp_pairing_features rsp = features(0x03, 0);
    struct bt_smp_pairing_negotiation out;

    req.initiator_key_distribution = BT_SMP_KEYDIST_ID_KEY;
    rsp.initiator_key_distribution = BT_SMP_KEYDIST_ID_KEY | BT_SMP_KEYDIST_SIGN_KEY;
    BT_CHECK(bt_smp_negotiate_pairing(&req, &rsp, &out) == BT_ERR_INVALID_ARGUMENT);
    rsp.initiator_key_distribution = BT_SMP_KEYDIST_ID_KEY;
    rsp.max_encryption_key_size = 6;
    BT_CHECK(bt_smp_negotiate_pairing(&req, &rsp, &out) == BT_ERR_INVALID_ARGUMENT);
}

void run_smp_pairing_tests(void)
{
    test_basic_negotiation();
    test_association_models();
    test_oob_rules();
    test_invalid_response_escalation();
}
