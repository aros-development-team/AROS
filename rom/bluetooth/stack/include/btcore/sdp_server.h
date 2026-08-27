/*
 * SDP server: answers ServiceSearch, ServiceAttribute and
 * ServiceSearchAttribute requests on the SDP channel (PSM 0x0001) from a
 * list of service records the owner provides. One instance per L2CAP
 * channel (it keeps the continuation state of the response in progress).
 * Transport agnostic: the owner hands in each received PDU and sends the
 * PDU this produces.
 */
#ifndef BTCORE_SDP_SERVER_H
#define BTCORE_SDP_SERVER_H

#include <btcore/types.h>
#include <btcore/status.h>

/* largest complete response body (before continuation splitting) */
#ifndef BT_SDP_SERVER_MAX_BODY
#define BT_SDP_SERVER_MAX_BODY 1024
#endif

/* a record: its handle and the attribute list content - the (uint16 id,
 * value) element pairs in ascending id order, WITHOUT a sequence header.
 * Attribute 0x0000 (ServiceRecordHandle) is expected to be present. */
struct bt_sdp_record
{
    uint32_t handle;
    const uint8_t *attrs;
    size_t attrs_len;
};

/* the owner's record list, walked by index until NULL */
typedef const struct bt_sdp_record *(*bt_sdp_record_at_fn)(void *context, size_t index);

struct bt_sdp_server
{
    bt_sdp_record_at_fn record_at;
    void *context;
    /* response in progress for continuation */
    uint8_t body[BT_SDP_SERVER_MAX_BODY];
    size_t body_len;
    size_t body_pos;
    uint8_t body_pdu;          /* response PDU id the body belongs to */
    uint16_t body_extra;       /* ServiceSearch: total record count */
};

void bt_sdp_server_init(struct bt_sdp_server *srv, bt_sdp_record_at_fn record_at, void *context);

/* Handle one request PDU; the response PDU is written to rsp (at most
 * rsp_max bytes, which should be the channel's outgoing MTU). Returns the
 * response length, 0 when the request was not an SDP request at all. */
size_t bt_sdp_server_handle(struct bt_sdp_server *srv, const uint8_t *req, size_t req_len,
                            uint8_t *rsp, size_t rsp_max);

/* does the record carry every UUID of the (encoded) search pattern? */
bool bt_sdp_record_matches(const struct bt_sdp_record *rec, const uint8_t *pattern, size_t pattern_len);

#endif /* BTCORE_SDP_SERVER_H */
