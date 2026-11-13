#ifndef BTCORE_BOND_STORE_H
#define BTCORE_BOND_STORE_H

#include <btcore/addr.h>
#include <btcore/buffer.h>
#include <btcore/status.h>
#include <btcore/types.h>

#ifndef BT_BOND_STORE_MAX_ENTRIES
#define BT_BOND_STORE_MAX_ENTRIES 16
#endif

#define BT_BOND_KEY_CLASSIC_LINK 0x01u
#define BT_BOND_KEY_LE_LTK 0x02u
#define BT_BOND_KEY_LE_IRK 0x04u
#define BT_BOND_KEY_LE_CSRK 0x08u
#define BT_BOND_KEY_VALID_MASK 0x0Fu

#define BT_BOND_FLAG_AUTHENTICATED 0x01u
#define BT_BOND_FLAG_SECURE_CONNECTIONS 0x02u
#define BT_BOND_FLAG_VALID_MASK 0x03u

#define BT_BOND_ADDR_PUBLIC 0u
#define BT_BOND_ADDR_RANDOM 1u

struct bt_bond_address
{
    struct bt_addr addr;
    uint8_t type;
};

/* A port-independent in-memory representation. Key bytes are opaque and are
 * never converted according to host endianness. EDIV is an integer and is
 * explicitly serialized little-endian. */
struct bt_bond_entry
{
    struct bt_bond_address adapter;
    struct bt_bond_address peer_identity;
    uint8_t key_mask;
    uint8_t flags;

    uint8_t classic_link_key[16];
    uint8_t classic_link_key_type;

    uint8_t ltk[16];
    uint8_t ltk_rand[8];
    uint16_t ltk_ediv;
    uint8_t ltk_size;

    uint8_t irk[16];
    uint8_t csrk[16];
};

struct bt_bond_store
{
    struct bt_bond_entry entries[BT_BOND_STORE_MAX_ENTRIES];
    size_t count;
};

void bt_bond_store_init(struct bt_bond_store *store);

/* Identity is scoped by adapter, so identical peers bonded through different
 * adapters remain distinct. Upsert copies the whole record into fixed storage. */
bt_status_t bt_bond_store_upsert(struct bt_bond_store *store,
                                  const struct bt_bond_entry *entry);
struct bt_bond_entry *bt_bond_store_find(struct bt_bond_store *store,
                                          const struct bt_bond_address *adapter,
                                          const struct bt_bond_address *peer_identity);
bool bt_bond_store_remove(struct bt_bond_store *store, const struct bt_bond_address *adapter,
                           const struct bt_bond_address *peer_identity);

/* Versioned keys.db wire format. serialize writes a complete database with a
 * CRC-32 over its TLV payload. deserialize validates the entire input before
 * modifying out_store and ignores unknown record types for forward
 * compatibility. No native C structure is written to disk. */
bt_status_t bt_bond_store_serialize(const struct bt_bond_store *store,
                                     struct bt_buf_writer *writer);
bt_status_t bt_bond_store_deserialize(struct bt_bond_store *out_store,
                                       const uint8_t *data, size_t data_len);

#endif /* BTCORE_BOND_STORE_H */
