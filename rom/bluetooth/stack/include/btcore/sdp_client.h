#ifndef BTCORE_SDP_CLIENT_H
#define BTCORE_SDP_CLIENT_H

#include <btcore/l2cap_channel.h>
#include <btcore/sdp.h>
#include <btcore/status.h>
#include <btcore/types.h>

/*
 * Orchestrates an SDP request/response exchange over a connection-
 * oriented L2CAP channel to BT_SDP_PSM, looping on continuation state
 * automatically until a result is complete. Sits on top of
 * btcore/sdp.h's wire codec and btcore/l2cap_channel.h's channel
 * manager -- one bt_sdp_client per remote device, one operation
 * in flight at a time.
 *
 * Scope reduction, documented: results (handle list or attribute list)
 * accumulate into a fixed BT_SDP_CLIENT_MAX_RESULT-byte buffer; a result
 * that would exceed it fails with BT_SDP_CLIENT_ERROR_TOO_LARGE rather
 * than growing without bound.
 */

#ifndef BT_SDP_CLIENT_MAX_RESULT
#define BT_SDP_CLIENT_MAX_RESULT 4096
#endif

#ifndef BT_SDP_CLIENT_MAX_REQUEST_BODY
#define BT_SDP_CLIENT_MAX_REQUEST_BODY 64
#endif

enum bt_sdp_client_op
{
    BT_SDP_CLIENT_OP_SEARCH,
    BT_SDP_CLIENT_OP_ATTRIBUTE
};

enum bt_sdp_client_result
{
    BT_SDP_CLIENT_OK,
    BT_SDP_CLIENT_ERROR_CONNECT,  /* the L2CAP channel to the SDP server failed to open */
    BT_SDP_CLIENT_ERROR_CLOSED,   /* the channel closed while a request was in flight */
    BT_SDP_CLIENT_ERROR_PROTOCOL, /* malformed or mismatched response */
    BT_SDP_CLIENT_ERROR_TOO_LARGE /* accumulated result exceeds BT_SDP_CLIENT_MAX_RESULT */,
    BT_SDP_CLIENT_ERROR_TIMEOUT   /* the server never answered (bt_sdp_client_tick()) */
};

struct bt_sdp_client_completion
{
    enum bt_sdp_client_result result;
    enum bt_sdp_client_op op;
    const uint8_t *data;  /* op==SEARCH: back-to-back 4-byte handles.
                           * op==ATTRIBUTE: one Data Element Sequence's nested
                           * bytes (walk with bt_sdp_element_iter). */
    size_t data_len;
    uint16_t total_count; /* op==SEARCH only: TotalServiceRecordCount from the last response */
};

typedef void (*bt_sdp_client_complete_fn)(struct bt_sdp_client_completion *completion,
                                           void *user_data);
typedef void (*bt_sdp_client_connect_fn)(bool success, void *user_data);

struct bt_sdp_client
{
    struct bt_l2cap_channel_manager *l2cap;
    uint16_t local_cid;
    bool channel_ready;
    uint16_t next_transaction_id;

    bt_sdp_client_connect_fn on_connect;
    void *connect_user_data;

    bool busy;
    enum bt_sdp_client_op op;
    uint16_t pending_transaction_id;

    uint8_t search_pattern[BT_SDP_CLIENT_MAX_REQUEST_BODY];
    size_t search_pattern_len;
    uint16_t search_max_count;

    uint32_t attr_handle;
    uint16_t attr_max_bytes;
    uint8_t attr_id_list[BT_SDP_CLIENT_MAX_REQUEST_BODY];
    size_t attr_id_list_len;

    struct bt_sdp_continuation continuation;

    uint8_t result[BT_SDP_CLIENT_MAX_RESULT];
    size_t result_len;
    uint16_t last_total_count;

    bt_sdp_client_complete_fn on_complete;
    void *complete_user_data;
    uint64_t deadline_us;  /* whole-operation deadline, 0 = none */
};

/* An SDP server that never answers (or answers a transaction id we are not
 * waiting for) must not leave the caller hanging: the whole operation,
 * continuation rounds included, has this long. */
#ifndef BT_SDP_CLIENT_REQUEST_TIMEOUT_US
#define BT_SDP_CLIENT_REQUEST_TIMEOUT_US (10u * 1000000u)
#endif

/* Call periodically; fails the in-flight operation with
 * BT_SDP_CLIENT_ERROR_TIMEOUT once its deadline passes. */
void bt_sdp_client_tick(struct bt_sdp_client *client, uint64_t now_us);

void bt_sdp_client_init(struct bt_sdp_client *client, struct bt_l2cap_channel_manager *l2cap);

/* Opens the L2CAP channel to the SDP server (BT_SDP_PSM). on_connect
 * fires once, reporting whether the channel came up. */
bt_status_t bt_sdp_client_connect(struct bt_sdp_client *client, bt_sdp_client_connect_fn on_connect,
                                   void *user_data, uint64_t now_us);

void bt_sdp_client_disconnect(struct bt_sdp_client *client, uint64_t now_us);

/* service_search_pattern must be one complete, encoded Data Element
 * Sequence (as for bt_sdp_encode_service_search_request()). Requires a
 * successful connect() first and no other operation in flight. */
bt_status_t bt_sdp_client_search(struct bt_sdp_client *client, const uint8_t *service_search_pattern,
                                  size_t pattern_len, uint16_t max_record_count,
                                  bt_sdp_client_complete_fn on_complete, void *user_data,
                                  uint64_t now_us);

/* attribute_id_list must be one complete, encoded Data Element Sequence
 * (as for bt_sdp_encode_service_attribute_request()). */
bt_status_t bt_sdp_client_get_attributes(struct bt_sdp_client *client, uint32_t service_record_handle,
                                          uint16_t max_attribute_byte_count,
                                          const uint8_t *attribute_id_list,
                                          size_t attribute_id_list_len,
                                          bt_sdp_client_complete_fn on_complete, void *user_data,
                                          uint64_t now_us);

#endif /* BTCORE_SDP_CLIENT_H */
