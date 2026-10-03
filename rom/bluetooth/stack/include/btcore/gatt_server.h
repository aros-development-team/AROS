#ifndef BTCORE_GATT_SERVER_H
#define BTCORE_GATT_SERVER_H

#include <btcore/types.h>

/*
 * GATT server: answers the ATT requests of a peer from an attribute database
 * the caller owns. The server keeps no copy of the database - it asks for
 * the attributes it needs, in handle order, through bt_gatt_server_next_fn -
 * so services can come and go between two requests.
 *
 * It shares the ATT bearer with the GATT client: ATT opcodes with bit 0
 * clear (requests, commands, the Handle Value Confirmation) are for the
 * server, the others (responses, notifications, indications) for the client.
 *
 * Supported: Exchange MTU, Find Information, Find By Type Value, Read By
 * Type, Read, Read Blob, Read By Group Type (primary services), Write
 * Request, Write Command, Handle Value Notification/Indication. Not
 * supported, and answered with Request Not Supported: Read Multiple,
 * Prepare/Execute Write, Signed Write. No attribute needs an encrypted or
 * authenticated link.
 */

#ifndef BT_GATT_SERVER_RX_MTU
#define BT_GATT_SERVER_RX_MTU 247
#endif

#define BT_GATT_ATTR_READ 0x01u     /* Read / Read Blob / Read By Type */
#define BT_GATT_ATTR_WRITE 0x02u    /* Write Request */
#define BT_GATT_ATTR_WRITE_NR 0x04u /* Write Command */
#define BT_GATT_ATTR_SERVICE 0x08u  /* a primary service declaration */

struct bt_gatt_server_attr
{
    uint16_t handle;
    uint16_t group_end; /* service declarations: the last handle of the service */
    uint8_t type_len;   /* 2 or 16 */
    uint8_t type[16];   /* little-endian, as on the wire */
    uint8_t flags;      /* BT_GATT_ATTR_xxx */
    const uint8_t *value;
    uint16_t value_len;
    uint8_t scratch[19]; /* for a value the database builds on the fly */
    void *ref;           /* the database's own: handed back on a write */
    uint16_t ref_index;
    uint8_t ref_kind;
};

/* The first attribute whose handle is >= from_handle; false if there is none. */
typedef bool (*bt_gatt_server_next_fn)(void *context, uint16_t from_handle,
                                        struct bt_gatt_server_attr *out);

/* Store a written value. Returns 0 or the ATT error code to answer with. */
typedef uint8_t (*bt_gatt_server_write_fn)(void *context, const struct bt_gatt_server_attr *attr,
                                            const uint8_t *data, size_t len);

struct bt_gatt_server
{
    bt_gatt_server_next_fn next;
    bt_gatt_server_write_fn write;
    void *context;
    uint16_t mtu;
    bool indication_pending;
};

void bt_gatt_server_init(struct bt_gatt_server *server, bt_gatt_server_next_fn next,
                          bt_gatt_server_write_fn write, void *context);

/* Handle one ATT PDU meant for the server. Returns the length of the answer
 * written to rsp (0: nothing to send). rsp_max bounds the MTU we agree to. */
size_t bt_gatt_server_handle_pdu(struct bt_gatt_server *server, const uint8_t *pdu, size_t len,
                                  uint8_t *rsp, size_t rsp_max);

/* Build a Handle Value Notification or Indication. Returns its length, or 0
 * if it does not fit or an earlier indication is still unconfirmed. */
size_t bt_gatt_server_encode_handle_value(struct bt_gatt_server *server, uint16_t handle,
                                           const uint8_t *value, size_t len, bool indicate,
                                           uint8_t *out, size_t out_max);

/* The 128-bit form of a 16-bit UUID, little-endian. */
void bt_gatt_uuid16_to_128(uint16_t uuid16, uint8_t out[16]);

#endif /* BTCORE_GATT_SERVER_H */
