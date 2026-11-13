#include <btcore/bond_store.h>

#include <string.h>

#define BT_BOND_DB_VERSION 1u
#define BT_BOND_DB_HEADER_LEN 20u
#define BT_BOND_RECORD_TYPE_ENTRY 1u
#define BT_BOND_RECORD_HEADER_LEN 4u
#define BT_BOND_ENTRY_WIRE_LEN 92u

static const uint8_t db_magic[4] = {'B', 'T', 'K', 'D'};

static uint32_t crc32(const uint8_t *data, size_t len)
{
    uint32_t crc = UINT32_C(0xFFFFFFFF);
    size_t i;
    unsigned bit;

    for (i = 0; i < len; ++i)
    {
        crc ^= data[i];
        for (bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (UINT32_C(0xEDB88320) & (uint32_t)-(int32_t)(crc & 1u));
    }
    return ~crc;
}

static bool address_valid(const struct bt_bond_address *addr)
{
    return addr->type <= BT_BOND_ADDR_RANDOM;
}

static bool bond_address_equal(const struct bt_bond_address *a,
                               const struct bt_bond_address *b)
{
    return a->type == b->type && bt_addr_equal(&a->addr, &b->addr);
}

static bool entry_valid(const struct bt_bond_entry *entry)
{
    if (!address_valid(&entry->adapter) || !address_valid(&entry->peer_identity) ||
        (entry->key_mask & ~BT_BOND_KEY_VALID_MASK) != 0 ||
        (entry->flags & ~BT_BOND_FLAG_VALID_MASK) != 0)
        return false;
    if ((entry->key_mask & BT_BOND_KEY_LE_LTK) != 0 &&
        (entry->ltk_size < 7 || entry->ltk_size > 16))
        return false;
    return true;
}

void bt_bond_store_init(struct bt_bond_store *store)
{
    memset(store, 0, sizeof(*store));
}

struct bt_bond_entry *bt_bond_store_find(struct bt_bond_store *store,
                                          const struct bt_bond_address *adapter,
                                          const struct bt_bond_address *peer_identity)
{
    size_t i;

    if (store == NULL || adapter == NULL || peer_identity == NULL)
        return NULL;
    for (i = 0; i < store->count; ++i)
    {
        if (bond_address_equal(&store->entries[i].adapter, adapter) &&
            bond_address_equal(&store->entries[i].peer_identity, peer_identity))
            return &store->entries[i];
    }
    return NULL;
}

bt_status_t bt_bond_store_upsert(struct bt_bond_store *store,
                                  const struct bt_bond_entry *entry)
{
    struct bt_bond_entry *existing;

    if (store == NULL || entry == NULL || !entry_valid(entry))
        return BT_ERR_INVALID_ARGUMENT;
    existing = bt_bond_store_find(store, &entry->adapter, &entry->peer_identity);
    if (existing != NULL)
    {
        *existing = *entry;
        return BT_OK;
    }
    if (store->count >= BT_BOND_STORE_MAX_ENTRIES)
        return BT_ERR_NO_RESOURCES;
    store->entries[store->count++] = *entry;
    return BT_OK;
}

bool bt_bond_store_remove(struct bt_bond_store *store, const struct bt_bond_address *adapter,
                           const struct bt_bond_address *peer_identity)
{
    struct bt_bond_entry *entry;
    size_t index;

    entry = bt_bond_store_find(store, adapter, peer_identity);
    if (entry == NULL)
        return false;
    index = (size_t)(entry - store->entries);
    if (index + 1 < store->count)
        memmove(&store->entries[index], &store->entries[index + 1],
                (store->count - index - 1) * sizeof(store->entries[0]));
    --store->count;
    memset(&store->entries[store->count], 0, sizeof(store->entries[0]));
    return true;
}

static bt_status_t write_addr(struct bt_buf_writer *w, const struct bt_bond_address *addr)
{
    bt_status_t st = bt_buf_writer_write_u8(w, (uint8_t)addr->type);

    return st == BT_OK ? bt_buf_writer_write_bytes(w, addr->addr.b, sizeof(addr->addr.b)) : st;
}

static bt_status_t write_entry(struct bt_buf_writer *w, const struct bt_bond_entry *entry)
{
    bt_status_t st;

#define WRITE(call)        \
    do                     \
    {                      \
        st = (call);       \
        if (st != BT_OK)   \
            return st;     \
    } while (0)
    WRITE(bt_buf_writer_write_u8(w, BT_BOND_RECORD_TYPE_ENTRY));
    WRITE(bt_buf_writer_write_u8(w, 0));
    WRITE(bt_buf_writer_write_le16(w, BT_BOND_ENTRY_WIRE_LEN));
    WRITE(write_addr(w, &entry->adapter));
    WRITE(write_addr(w, &entry->peer_identity));
    WRITE(bt_buf_writer_write_u8(w, entry->key_mask));
    WRITE(bt_buf_writer_write_u8(w, entry->flags));
    WRITE(bt_buf_writer_write_bytes(w, entry->classic_link_key, 16));
    WRITE(bt_buf_writer_write_u8(w, entry->classic_link_key_type));
    WRITE(bt_buf_writer_write_bytes(w, entry->ltk, 16));
    WRITE(bt_buf_writer_write_bytes(w, entry->ltk_rand, 8));
    WRITE(bt_buf_writer_write_le16(w, entry->ltk_ediv));
    WRITE(bt_buf_writer_write_u8(w, entry->ltk_size));
    WRITE(bt_buf_writer_write_bytes(w, entry->irk, 16));
    WRITE(bt_buf_writer_write_bytes(w, entry->csrk, 16));
#undef WRITE
    return BT_OK;
}

bt_status_t bt_bond_store_serialize(const struct bt_bond_store *store,
                                     struct bt_buf_writer *writer)
{
    size_t start;
    size_t payload_start;
    size_t i;
    uint32_t payload_len;
    uint32_t checksum;
    bt_status_t st;

    if (store == NULL || writer == NULL || store->count > BT_BOND_STORE_MAX_ENTRIES)
        return BT_ERR_INVALID_ARGUMENT;
    for (i = 0; i < store->count; ++i)
        if (!entry_valid(&store->entries[i]))
            return BT_ERR_INVALID_ARGUMENT;

    start = bt_buf_writer_len(writer);
    st = bt_buf_writer_write_bytes(writer, db_magic, sizeof(db_magic));
    if (st == BT_OK)
        st = bt_buf_writer_write_le16(writer, BT_BOND_DB_VERSION);
    if (st == BT_OK)
        st = bt_buf_writer_write_le16(writer, BT_BOND_DB_HEADER_LEN);
    if (st == BT_OK)
        st = bt_buf_writer_write_le16(writer, (uint16_t)store->count);
    if (st == BT_OK)
        st = bt_buf_writer_write_le16(writer, 0);
    if (st == BT_OK)
        st = bt_buf_writer_write_le32(writer, 0);
    if (st == BT_OK)
        st = bt_buf_writer_write_le32(writer, 0);
    if (st != BT_OK)
        return st;

    payload_start = bt_buf_writer_len(writer);
    for (i = 0; i < store->count; ++i)
    {
        st = write_entry(writer, &store->entries[i]);
        if (st != BT_OK)
            return st;
    }
    payload_len = (uint32_t)(bt_buf_writer_len(writer) - payload_start);
    checksum = crc32(writer->data + payload_start, payload_len);
    writer->data[start + 12] = (uint8_t)payload_len;
    writer->data[start + 13] = (uint8_t)(payload_len >> 8);
    writer->data[start + 14] = (uint8_t)(payload_len >> 16);
    writer->data[start + 15] = (uint8_t)(payload_len >> 24);
    writer->data[start + 16] = (uint8_t)checksum;
    writer->data[start + 17] = (uint8_t)(checksum >> 8);
    writer->data[start + 18] = (uint8_t)(checksum >> 16);
    writer->data[start + 19] = (uint8_t)(checksum >> 24);
    return BT_OK;
}

static bt_status_t read_addr(struct bt_buf_reader *r, struct bt_bond_address *addr)
{
    uint8_t type;
    bt_status_t st = bt_buf_reader_read_u8(r, &type);

    if (st != BT_OK || type > BT_BOND_ADDR_RANDOM)
        return BT_ERR_INVALID_ARGUMENT;
    addr->type = type;
    return bt_buf_reader_read_bytes(r, addr->addr.b, sizeof(addr->addr.b));
}

static bt_status_t read_entry(const uint8_t *data, size_t len, struct bt_bond_entry *entry)
{
    struct bt_buf_reader r;
    bt_status_t st;

    if (len != BT_BOND_ENTRY_WIRE_LEN)
        return BT_ERR_INVALID_ARGUMENT;
    memset(entry, 0, sizeof(*entry));
    bt_buf_reader_init(&r, data, len);
#define READ(call)         \
    do                     \
    {                      \
        st = (call);       \
        if (st != BT_OK)   \
            return st;     \
    } while (0)
    READ(read_addr(&r, &entry->adapter));
    READ(read_addr(&r, &entry->peer_identity));
    READ(bt_buf_reader_read_u8(&r, &entry->key_mask));
    READ(bt_buf_reader_read_u8(&r, &entry->flags));
    READ(bt_buf_reader_read_bytes(&r, entry->classic_link_key, 16));
    READ(bt_buf_reader_read_u8(&r, &entry->classic_link_key_type));
    READ(bt_buf_reader_read_bytes(&r, entry->ltk, 16));
    READ(bt_buf_reader_read_bytes(&r, entry->ltk_rand, 8));
    READ(bt_buf_reader_read_le16(&r, &entry->ltk_ediv));
    READ(bt_buf_reader_read_u8(&r, &entry->ltk_size));
    READ(bt_buf_reader_read_bytes(&r, entry->irk, 16));
    READ(bt_buf_reader_read_bytes(&r, entry->csrk, 16));
#undef READ
    return entry_valid(entry) ? BT_OK : BT_ERR_INVALID_ARGUMENT;
}

static bt_status_t validate_payload(const uint8_t *payload, size_t payload_len,
                                     uint16_t expected_entries)
{
    struct bt_buf_reader r;
    uint16_t known_entries = 0;

    bt_buf_reader_init(&r, payload, payload_len);
    while (bt_buf_reader_remaining(&r) != 0)
    {
        uint8_t type;
        uint8_t reserved;
        uint16_t len;
        const uint8_t *record;
        struct bt_bond_entry scratch;

        if (bt_buf_reader_read_u8(&r, &type) != BT_OK ||
            bt_buf_reader_read_u8(&r, &reserved) != BT_OK ||
            bt_buf_reader_read_le16(&r, &len) != BT_OK || reserved != 0)
            return BT_ERR_INVALID_ARGUMENT;
        record = bt_buf_reader_peek(&r, len);
        if (record == NULL || bt_buf_reader_skip(&r, len) != BT_OK)
            return BT_ERR_INVALID_ARGUMENT;
        if (type == BT_BOND_RECORD_TYPE_ENTRY)
        {
            if (read_entry(record, len, &scratch) != BT_OK ||
                ++known_entries > BT_BOND_STORE_MAX_ENTRIES)
                return BT_ERR_INVALID_ARGUMENT;
        }
    }
    return known_entries == expected_entries ? BT_OK : BT_ERR_INVALID_ARGUMENT;
}

bt_status_t bt_bond_store_deserialize(struct bt_bond_store *out_store,
                                       const uint8_t *data, size_t data_len)
{
    struct bt_buf_reader r;
    uint8_t magic[4];
    uint16_t version;
    uint16_t header_len;
    uint16_t count;
    uint16_t reserved;
    uint32_t payload_len;
    uint32_t expected_crc;
    const uint8_t *payload;

    if (out_store == NULL || data == NULL || data_len < BT_BOND_DB_HEADER_LEN)
        return BT_ERR_INVALID_ARGUMENT;
    bt_buf_reader_init(&r, data, data_len);
    if (bt_buf_reader_read_bytes(&r, magic, sizeof(magic)) != BT_OK ||
        bt_buf_reader_read_le16(&r, &version) != BT_OK ||
        bt_buf_reader_read_le16(&r, &header_len) != BT_OK ||
        bt_buf_reader_read_le16(&r, &count) != BT_OK ||
        bt_buf_reader_read_le16(&r, &reserved) != BT_OK ||
        bt_buf_reader_read_le32(&r, &payload_len) != BT_OK ||
        bt_buf_reader_read_le32(&r, &expected_crc) != BT_OK ||
        memcmp(magic, db_magic, sizeof(magic)) != 0 || version != BT_BOND_DB_VERSION ||
        header_len != BT_BOND_DB_HEADER_LEN || reserved != 0 ||
        payload_len != bt_buf_reader_remaining(&r) ||
        count > BT_BOND_STORE_MAX_ENTRIES)
        return BT_ERR_INVALID_ARGUMENT;
    payload = bt_buf_reader_peek(&r, payload_len);
    if (payload == NULL || crc32(payload, payload_len) != expected_crc ||
        validate_payload(payload, payload_len, count) != BT_OK)
        return BT_ERR_INVALID_ARGUMENT;

    bt_bond_store_init(out_store);
    bt_buf_reader_init(&r, payload, payload_len);
    while (bt_buf_reader_remaining(&r) != 0)
    {
        uint8_t type;
        uint8_t record_reserved;
        uint16_t len;
        const uint8_t *record;

        bt_buf_reader_read_u8(&r, &type);
        bt_buf_reader_read_u8(&r, &record_reserved);
        bt_buf_reader_read_le16(&r, &len);
        record = bt_buf_reader_peek(&r, len);
        bt_buf_reader_skip(&r, len);
        if (type == BT_BOND_RECORD_TYPE_ENTRY)
        {
            read_entry(record, len, &out_store->entries[out_store->count]);
            ++out_store->count;
        }
    }
    return BT_OK;
}
