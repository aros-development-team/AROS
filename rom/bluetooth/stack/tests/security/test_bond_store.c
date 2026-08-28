#include "test_bond_store.h"
#include "../support/test.h"

#include <btcore/bond_store.h>

#include <string.h>

static struct bt_bond_entry make_entry(uint8_t seed)
{
    struct bt_bond_entry entry;
    size_t i;

    memset(&entry, 0, sizeof(entry));
    entry.adapter.type = BT_BOND_ADDR_PUBLIC;
    entry.peer_identity.type = BT_BOND_ADDR_RANDOM;
    for (i = 0; i < 6; ++i)
    {
        entry.adapter.addr.b[i] = (uint8_t)(seed + i);
        entry.peer_identity.addr.b[i] = (uint8_t)(seed + 0x10u + i);
    }
    entry.key_mask = BT_BOND_KEY_CLASSIC_LINK | BT_BOND_KEY_LE_LTK |
                     BT_BOND_KEY_LE_IRK | BT_BOND_KEY_LE_CSRK;
    entry.flags = BT_BOND_FLAG_AUTHENTICATED | BT_BOND_FLAG_SECURE_CONNECTIONS;
    entry.classic_link_key_type = 8;
    entry.ltk_ediv = (uint16_t)(0x1200u + seed);
    entry.ltk_size = 16;
    for (i = 0; i < 16; ++i)
    {
        entry.classic_link_key[i] = (uint8_t)(seed + i);
        entry.ltk[i] = (uint8_t)(seed + 0x20u + i);
        entry.irk[i] = (uint8_t)(seed + 0x40u + i);
        entry.csrk[i] = (uint8_t)(seed + 0x60u + i);
    }
    for (i = 0; i < 8; ++i)
        entry.ltk_rand[i] = (uint8_t)(seed + 0x70u + i);
    return entry;
}

static void test_upsert_find_remove(void)
{
    struct bt_bond_store store;
    struct bt_bond_entry entry = make_entry(1);
    struct bt_bond_entry *found;

    bt_bond_store_init(&store);
    BT_CHECK(bt_bond_store_upsert(&store, &entry) == BT_OK);
    BT_CHECK(store.count == 1);
    found = bt_bond_store_find(&store, &entry.adapter, &entry.peer_identity);
    BT_CHECK(found != NULL && found->ltk_ediv == 0x1201);

    entry.ltk_ediv = 0xBEEF;
    BT_CHECK(bt_bond_store_upsert(&store, &entry) == BT_OK);
    BT_CHECK(store.count == 1);
    BT_CHECK(bt_bond_store_find(&store, &entry.adapter, &entry.peer_identity)->ltk_ediv == 0xBEEF);
    BT_CHECK(bt_bond_store_remove(&store, &entry.adapter, &entry.peer_identity));
    BT_CHECK(store.count == 0);
    BT_CHECK(!bt_bond_store_remove(&store, &entry.adapter, &entry.peer_identity));
}

static void test_round_trip_and_wire_order(void)
{
    struct bt_bond_store source;
    struct bt_bond_store decoded;
    struct bt_bond_entry first = make_entry(1);
    struct bt_bond_entry second = make_entry(2);
    uint8_t encoded[512];
    struct bt_buf_writer w;
    struct bt_bond_entry *found;

    bt_bond_store_init(&source);
    BT_CHECK(bt_bond_store_upsert(&source, &first) == BT_OK);
    BT_CHECK(bt_bond_store_upsert(&source, &second) == BT_OK);
    bt_buf_writer_init(&w, encoded, sizeof(encoded));
    BT_CHECK(bt_bond_store_serialize(&source, &w) == BT_OK);
    BT_CHECK(memcmp(encoded, "BTKD", 4) == 0);
    BT_CHECK(encoded[4] == 1 && encoded[5] == 0); /* version is LE */
    BT_CHECK(encoded[8] == 2 && encoded[9] == 0); /* record count is LE */

    BT_CHECK(bt_bond_store_deserialize(&decoded, encoded, bt_buf_writer_len(&w)) == BT_OK);
    BT_CHECK(decoded.count == 2);
    found = bt_bond_store_find(&decoded, &second.adapter, &second.peer_identity);
    BT_CHECK(found != NULL);
    BT_CHECK(found->ltk_ediv == second.ltk_ediv);
    BT_CHECK(memcmp(found->irk, second.irk, 16) == 0);
    BT_CHECK(memcmp(found->classic_link_key, second.classic_link_key, 16) == 0);
}

static void test_corruption_and_truncation_are_rejected_transactionally(void)
{
    struct bt_bond_store source;
    struct bt_bond_store destination;
    struct bt_bond_entry entry = make_entry(3);
    uint8_t encoded[256];
    struct bt_buf_writer w;
    size_t len;

    bt_bond_store_init(&source);
    bt_bond_store_upsert(&source, &entry);
    bt_buf_writer_init(&w, encoded, sizeof(encoded));
    BT_CHECK(bt_bond_store_serialize(&source, &w) == BT_OK);
    len = bt_buf_writer_len(&w);

    bt_bond_store_init(&destination);
    destination.count = 7; /* prove failures do not modify the destination */
    encoded[len - 1] ^= 0x01;
    BT_CHECK(bt_bond_store_deserialize(&destination, encoded, len) == BT_ERR_INVALID_ARGUMENT);
    BT_CHECK(destination.count == 7);
    encoded[len - 1] ^= 0x01;
    BT_CHECK(bt_bond_store_deserialize(&destination, encoded, len - 1) ==
              BT_ERR_INVALID_ARGUMENT);
    BT_CHECK(destination.count == 7);
}

static void test_invalid_key_metadata_and_capacity(void)
{
    struct bt_bond_store store;
    struct bt_bond_entry entry = make_entry(0);
    size_t i;

    bt_bond_store_init(&store);
    entry.ltk_size = 6;
    BT_CHECK(bt_bond_store_upsert(&store, &entry) == BT_ERR_INVALID_ARGUMENT);
    entry.ltk_size = 16;
    for (i = 0; i < BT_BOND_STORE_MAX_ENTRIES; ++i)
    {
        entry.peer_identity.addr.b[0] = (uint8_t)i;
        BT_CHECK(bt_bond_store_upsert(&store, &entry) == BT_OK);
    }
    entry.peer_identity.addr.b[0] = 0xFF;
    BT_CHECK(bt_bond_store_upsert(&store, &entry) == BT_ERR_NO_RESOURCES);
}

void run_bond_store_tests(void)
{
    test_upsert_find_remove();
    test_round_trip_and_wire_order();
    test_corruption_and_truncation_are_rejected_transactionally();
    test_invalid_key_metadata_and_capacity();
}
