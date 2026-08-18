#ifndef BTCORE_ATT_H
#define BTCORE_ATT_H

#include <btcore/buffer.h>
#include <btcore/status.h>
#include <btcore/types.h>

/*
 * ATT (Attribute Protocol), project.md Fase 6 (LE side). Unlike SDP, ATT
 * integers are little-endian (matching HCI/L2CAP) -- this and SDP are the
 * two ends of the endianness split project.md calls out explicitly.
 *
 * Scope reduction, documented: client role only (encode requests, parse
 * responses/server-initiated notifications) -- no server-side encode of
 * responses or parsing of requests, since project.md itself defers "GATT
 * Server" to later work. Only the request/response pairs a basic GATT
 * Client needs are covered: MTU exchange, primary service discovery
 * (Read By Group Type), characteristic discovery (Read By Type), Read,
 * Write, Find Information, Read Blob, and Handle Value Notification/
 * Indication. Find By Type Value isn't implemented yet. Attribute Type
 * fields in requests are always 16-bit
 * UUIDs here (every GATT declaration type is SIG-defined and 16-bit);
 * 128-bit UUIDs appearing *inside* a response's opaque value bytes are
 * unaffected and are the caller's concern to interpret.
 */

#define BT_ATT_OPCODE_ERROR_RESPONSE 0x01u
#define BT_ATT_OPCODE_EXCHANGE_MTU_REQUEST 0x02u
#define BT_ATT_OPCODE_EXCHANGE_MTU_RESPONSE 0x03u
#define BT_ATT_OPCODE_FIND_INFORMATION_REQUEST 0x04u
#define BT_ATT_OPCODE_FIND_INFORMATION_RESPONSE 0x05u
#define BT_ATT_OPCODE_READ_BY_TYPE_REQUEST 0x08u
#define BT_ATT_OPCODE_READ_BY_TYPE_RESPONSE 0x09u
#define BT_ATT_OPCODE_READ_REQUEST 0x0Au
#define BT_ATT_OPCODE_READ_RESPONSE 0x0Bu
#define BT_ATT_OPCODE_READ_BLOB_REQUEST 0x0Cu
#define BT_ATT_OPCODE_READ_BLOB_RESPONSE 0x0Du
#define BT_ATT_OPCODE_READ_BY_GROUP_TYPE_REQUEST 0x10u
#define BT_ATT_OPCODE_READ_BY_GROUP_TYPE_RESPONSE 0x11u
#define BT_ATT_OPCODE_WRITE_REQUEST 0x12u
#define BT_ATT_OPCODE_WRITE_RESPONSE 0x13u
#define BT_ATT_OPCODE_HANDLE_VALUE_NOTIFICATION 0x1Bu
#define BT_ATT_OPCODE_HANDLE_VALUE_INDICATION 0x1Du
#define BT_ATT_OPCODE_HANDLE_VALUE_CONFIRMATION 0x1Eu

#define BT_ATT_DEFAULT_MTU 23 /* spec default before any Exchange MTU */

/* The one error code a GATT client must treat specially: it's how a
 * server signals "no more results", the normal way Read By Group
 * Type/Read By Type discovery loops terminate -- not a real failure. */
#define BT_ATT_ERROR_ATTRIBUTE_NOT_FOUND 0x0Au
#define BT_ATT_ERROR_INVALID_OFFSET 0x07u

#define BT_GATT_UUID_PRIMARY_SERVICE 0x2800u
#define BT_GATT_UUID_CHARACTERISTIC 0x2803u

struct bt_att_error_response
{
    uint8_t request_opcode;
    uint16_t handle_in_error;
    uint8_t error_code;
};

bt_status_t bt_att_parse_error_response(const uint8_t *params, size_t params_len,
                                         struct bt_att_error_response *out);

bt_status_t bt_att_encode_exchange_mtu_request(struct bt_buf_writer *w, uint16_t client_rx_mtu);
bt_status_t bt_att_parse_exchange_mtu_response(const uint8_t *params, size_t params_len,
                                                uint16_t *out_server_rx_mtu);

bt_status_t bt_att_encode_find_information_request(struct bt_buf_writer *w,
                                                    uint16_t starting_handle,
                                                    uint16_t ending_handle);

struct bt_att_information_entry
{
    uint16_t handle;
    const uint8_t *uuid;
    uint8_t uuid_len;
};

struct bt_att_find_information_iter
{
    struct bt_buf_reader r;
    uint8_t entry_len;
};

bt_status_t bt_att_find_information_response_iter_init(
    struct bt_att_find_information_iter *it, const uint8_t *params, size_t params_len);
bt_status_t bt_att_find_information_response_iter_next(
    struct bt_att_find_information_iter *it, struct bt_att_information_entry *out);

/* Read By Group Type: primary/secondary service discovery. */
bt_status_t bt_att_encode_read_by_group_type_request(struct bt_buf_writer *w,
                                                       uint16_t starting_handle,
                                                       uint16_t ending_handle,
                                                       uint16_t group_type_uuid16);

struct bt_att_group_entry
{
    uint16_t handle;
    uint16_t end_group_handle;
    const uint8_t *value;
    uint8_t value_len;
};

struct bt_att_group_type_iter
{
    struct bt_buf_reader r;
    uint8_t entry_len; /* Length field from the response: 4 + value_len */
};

bt_status_t bt_att_read_by_group_type_response_iter_init(struct bt_att_group_type_iter *it,
                                                           const uint8_t *params, size_t params_len);
/* Returns BT_ERR_BUFFER_UNDERFLOW once exhausted (not a real error). */
bt_status_t bt_att_read_by_group_type_response_iter_next(struct bt_att_group_type_iter *it,
                                                           struct bt_att_group_entry *out);

/* Read By Type: characteristic discovery (type=BT_GATT_UUID_CHARACTERISTIC)
 * and generic attribute-by-type lookups within a handle range. */
bt_status_t bt_att_encode_read_by_type_request(struct bt_buf_writer *w, uint16_t starting_handle,
                                                uint16_t ending_handle, uint16_t attribute_type_uuid16);

struct bt_att_type_entry
{
    uint16_t handle;
    const uint8_t *value;
    uint8_t value_len;
};

struct bt_att_read_by_type_iter
{
    struct bt_buf_reader r;
    uint8_t entry_len; /* Length field from the response: 2 + value_len */
};

bt_status_t bt_att_read_by_type_response_iter_init(struct bt_att_read_by_type_iter *it,
                                                     const uint8_t *params, size_t params_len);
bt_status_t bt_att_read_by_type_response_iter_next(struct bt_att_read_by_type_iter *it,
                                                     struct bt_att_type_entry *out);

bt_status_t bt_att_encode_read_request(struct bt_buf_writer *w, uint16_t handle);
bt_status_t bt_att_encode_read_blob_request(struct bt_buf_writer *w, uint16_t handle,
                                             uint16_t value_offset);
/* Read Response has no fixed structure beyond "the rest of the PDU is the
 * value" -- callers read params directly once the opcode is confirmed. */

bt_status_t bt_att_encode_write_request(struct bt_buf_writer *w, uint16_t handle,
                                         const uint8_t *value, size_t value_len);
/* Write Response has no parameters at all -- confirming the opcode is enough. */

struct bt_att_handle_value
{
    uint16_t handle;
    const uint8_t *value;
    size_t value_len;
};

/* Shared layout for Handle Value Notification and Handle Value Indication. */
bt_status_t bt_att_parse_handle_value(const uint8_t *params, size_t params_len,
                                       struct bt_att_handle_value *out);

bt_status_t bt_att_encode_handle_value_confirmation(struct bt_buf_writer *w);

#endif /* BTCORE_ATT_H */
