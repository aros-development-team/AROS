#include <btcore/sdp_client.h>

#include <string.h>

#define REQUEST_BUF_LEN (BT_SDP_HEADER_LEN + 4 + 2 + BT_SDP_CLIENT_MAX_REQUEST_BODY + 1 + 16)

static void issue_request(struct bt_sdp_client *client, uint64_t now_us);

static void fail(struct bt_sdp_client *client, enum bt_sdp_client_result result)
{
    struct bt_sdp_client_completion completion;
    bt_sdp_client_complete_fn cb = client->on_complete;
    void *ud = client->complete_user_data;

    client->busy = false;
    client->deadline_us = 0;

    completion.result = result;
    completion.op = client->op;
    completion.data = NULL;
    completion.data_len = 0;
    completion.total_count = 0;

    if (cb != NULL)
        cb(&completion, ud);
}

static void succeed(struct bt_sdp_client *client)
{
    struct bt_sdp_client_completion completion;
    bt_sdp_client_complete_fn cb = client->on_complete;
    void *ud = client->complete_user_data;

    client->busy = false;
    client->deadline_us = 0;

    completion.result = BT_SDP_CLIENT_OK;
    completion.op = client->op;
    completion.data = client->result;
    completion.data_len = client->result_len;
    completion.total_count = client->last_total_count;

    if (cb != NULL)
        cb(&completion, ud);
}

static bt_status_t append_result(struct bt_sdp_client *client, const uint8_t *data, size_t len)
{
    if (client->result_len + len > sizeof(client->result))
        return BT_ERR_NO_RESOURCES;

    memcpy(client->result + client->result_len, data, len);
    client->result_len += len;
    return BT_OK;
}

static bt_status_t read_continuation_local(struct bt_buf_reader *r, struct bt_sdp_continuation *out)
{
    uint8_t len;
    bt_status_t st;

    st = bt_buf_reader_read_u8(r, &len);
    if (st != BT_OK)
        return st;
    if (len > sizeof(out->data))
        return BT_ERR_INVALID_ARGUMENT;

    out->len = len;
    if (len == 0)
        return BT_OK;

    return bt_buf_reader_read_bytes(r, out->data, len);
}

static void finish_round(struct bt_sdp_client *client, uint64_t now_us)
{
    if (client->continuation.len > 0)
    {
        issue_request(client, now_us);
        return;
    }

    if (client->op == BT_SDP_CLIENT_OP_ATTRIBUTE)
    {
        /* The AttributeList byte stream is now fully assembled (it may
         * have arrived split across several continuation rounds -- each
         * round is a raw continuation of the same Data Element encoding,
         * not an independently parseable one, so this is the first point
         * where parsing it as a Data Element is valid). Replace the raw
         * bytes with just the parsed Sequence's nested content, matching
         * what a single-round response exposes. */
        struct bt_buf_reader fr;
        struct bt_sdp_element elem;

        bt_buf_reader_init(&fr, client->result, client->result_len);
        if (bt_sdp_parse_element(&fr, &elem) != BT_OK || elem.type != BT_SDP_ELEM_SEQUENCE)
        {
            fail(client, BT_SDP_CLIENT_ERROR_PROTOCOL);
            return;
        }
        memmove(client->result, elem.seq_data, elem.seq_len);
        client->result_len = elem.seq_len;
    }

    succeed(client);
}

static void handle_search_response(struct bt_sdp_client *client, const uint8_t *params,
                                    size_t params_len, uint64_t now_us)
{
    struct bt_sdp_service_search_response rsp;

    if (bt_sdp_parse_service_search_response(params, params_len, &rsp) != BT_OK)
    {
        fail(client, BT_SDP_CLIENT_ERROR_PROTOCOL);
        return;
    }

    client->last_total_count = rsp.total_record_count;
    if (append_result(client, rsp.handles, (size_t)rsp.current_record_count * 4) != BT_OK)
    {
        fail(client, BT_SDP_CLIENT_ERROR_TOO_LARGE);
        return;
    }

    client->continuation = rsp.continuation;
    finish_round(client, now_us);
}

static void handle_attribute_response(struct bt_sdp_client *client, const uint8_t *params,
                                       size_t params_len, uint64_t now_us)
{
    struct bt_buf_reader r;
    uint16_t byte_count;
    const uint8_t *chunk;

    bt_buf_reader_init(&r, params, params_len);
    if (bt_buf_reader_read_be16(&r, &byte_count) != BT_OK)
    {
        fail(client, BT_SDP_CLIENT_ERROR_PROTOCOL);
        return;
    }

    chunk = bt_buf_reader_peek(&r, byte_count);
    if (chunk == NULL || bt_buf_reader_skip(&r, byte_count) != BT_OK)
    {
        fail(client, BT_SDP_CLIENT_ERROR_PROTOCOL);
        return;
    }

    if (append_result(client, chunk, byte_count) != BT_OK)
    {
        fail(client, BT_SDP_CLIENT_ERROR_TOO_LARGE);
        return;
    }

    if (read_continuation_local(&r, &client->continuation) != BT_OK)
    {
        fail(client, BT_SDP_CLIENT_ERROR_PROTOCOL);
        return;
    }

    finish_round(client, now_us);
}

static void handle_response(struct bt_sdp_client *client, const uint8_t *data, size_t len,
                             uint64_t now_us)
{
    struct bt_buf_reader r;
    struct bt_sdp_header hdr;
    const uint8_t *params;

    if (!client->busy)
        return; /* unsolicited, or arrived after we already gave up -- ignore */

    bt_buf_reader_init(&r, data, len);
    if (bt_sdp_parse_header(&r, &hdr) != BT_OK)
    {
        fail(client, BT_SDP_CLIENT_ERROR_PROTOCOL);
        return;
    }
    if (hdr.transaction_id != client->pending_transaction_id)
        return; /* stale response for an earlier, already-finished request */

    params = bt_buf_reader_peek(&r, hdr.param_len);
    if (params == NULL || hdr.pdu_id == BT_SDP_PDU_ERROR_RESPONSE)
    {
        fail(client, BT_SDP_CLIENT_ERROR_PROTOCOL);
        return;
    }

    if (client->op == BT_SDP_CLIENT_OP_SEARCH)
    {
        if (hdr.pdu_id != BT_SDP_PDU_SERVICE_SEARCH_RESPONSE)
        {
            fail(client, BT_SDP_CLIENT_ERROR_PROTOCOL);
            return;
        }
        handle_search_response(client, params, hdr.param_len, now_us);
    }
    else
    {
        if (hdr.pdu_id != BT_SDP_PDU_SERVICE_ATTRIBUTE_RESPONSE)
        {
            fail(client, BT_SDP_CLIENT_ERROR_PROTOCOL);
            return;
        }
        handle_attribute_response(client, params, hdr.param_len, now_us);
    }
}

static void issue_request(struct bt_sdp_client *client, uint64_t now_us)
{
    uint8_t buf[REQUEST_BUF_LEN];
    struct bt_buf_writer w;
    bt_status_t st;
    const struct bt_sdp_continuation *cont = (client->continuation.len > 0) ? &client->continuation : NULL;

    client->pending_transaction_id = client->next_transaction_id++;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    if (client->op == BT_SDP_CLIENT_OP_SEARCH)
        st = bt_sdp_encode_service_search_request(&w, client->pending_transaction_id,
                                                    client->search_pattern,
                                                    client->search_pattern_len,
                                                    client->search_max_count, cont);
    else
        st = bt_sdp_encode_service_attribute_request(&w, client->pending_transaction_id,
                                                       client->attr_handle, client->attr_max_bytes,
                                                       client->attr_id_list,
                                                       client->attr_id_list_len, cont);

    if (st != BT_OK)
    {
        fail(client, BT_SDP_CLIENT_ERROR_PROTOCOL);
        return;
    }

    if (bt_l2cap_channel_manager_send(client->l2cap, client->local_cid, buf, bt_buf_writer_len(&w),
                                       now_us) != BT_OK)
        fail(client, BT_SDP_CLIENT_ERROR_CLOSED);
}

static void on_l2cap_event(struct bt_l2cap_channel_event_info *info, void *user_data)
{
    struct bt_sdp_client *client = (struct bt_sdp_client *)user_data;

    switch (info->event)
    {
    case BT_L2CAP_CHANNEL_EVENT_OPENED:
        client->channel_ready = true;
        if (client->on_connect != NULL)
        {
            bt_sdp_client_connect_fn cb = client->on_connect;
            void *ud = client->connect_user_data;

            client->on_connect = NULL;
            cb(true, ud);
        }
        break;

    case BT_L2CAP_CHANNEL_EVENT_CLOSED:
        client->channel_ready = false;
        if (client->on_connect != NULL)
        {
            bt_sdp_client_connect_fn cb = client->on_connect;
            void *ud = client->connect_user_data;

            client->on_connect = NULL;
            cb(false, ud);
        }
        if (client->busy)
            fail(client, BT_SDP_CLIENT_ERROR_CLOSED);
        break;

    case BT_L2CAP_CHANNEL_EVENT_DATA:
        /* now_us isn't carried by L2CAP channel events; 0 is safe here
         * since bt_l2cap_channel_manager_send() doesn't currently use it. */
        handle_response(client, info->data, info->data_len, 0);
        break;
    }
}

void bt_sdp_client_init(struct bt_sdp_client *client, struct bt_l2cap_channel_manager *l2cap)
{
    memset(client, 0, sizeof(*client));
    client->l2cap = l2cap;
    client->next_transaction_id = 1;
}

bt_status_t bt_sdp_client_connect(struct bt_sdp_client *client, bt_sdp_client_connect_fn on_connect,
                                   void *user_data, uint64_t now_us)
{
    client->on_connect = on_connect;
    client->connect_user_data = user_data;

    return bt_l2cap_channel_manager_open(client->l2cap, BT_SDP_PSM, 0, on_l2cap_event, client,
                                          &client->local_cid, now_us);
}

void bt_sdp_client_disconnect(struct bt_sdp_client *client, uint64_t now_us)
{
    if (client->channel_ready)
        bt_l2cap_channel_manager_close(client->l2cap, client->local_cid, now_us);
    client->channel_ready = false;
}

bt_status_t bt_sdp_client_search(struct bt_sdp_client *client, const uint8_t *service_search_pattern,
                                  size_t pattern_len, uint16_t max_record_count,
                                  bt_sdp_client_complete_fn on_complete, void *user_data,
                                  uint64_t now_us)
{
    if (!client->channel_ready || client->busy)
        return BT_ERR_INVALID_ARGUMENT;
    if (pattern_len > sizeof(client->search_pattern))
        return BT_ERR_INVALID_ARGUMENT;

    client->busy = true;
    client->op = BT_SDP_CLIENT_OP_SEARCH;
    memcpy(client->search_pattern, service_search_pattern, pattern_len);
    client->search_pattern_len = pattern_len;
    client->search_max_count = max_record_count;
    client->continuation.len = 0;
    client->result_len = 0;
    client->last_total_count = 0;
    client->on_complete = on_complete;
    client->complete_user_data = user_data;
    client->deadline_us = now_us + BT_SDP_CLIENT_REQUEST_TIMEOUT_US;

    issue_request(client, now_us);
    return BT_OK;
}

bt_status_t bt_sdp_client_get_attributes(struct bt_sdp_client *client, uint32_t service_record_handle,
                                          uint16_t max_attribute_byte_count,
                                          const uint8_t *attribute_id_list,
                                          size_t attribute_id_list_len,
                                          bt_sdp_client_complete_fn on_complete, void *user_data,
                                          uint64_t now_us)
{
    if (!client->channel_ready || client->busy)
        return BT_ERR_INVALID_ARGUMENT;
    if (attribute_id_list_len > sizeof(client->attr_id_list))
        return BT_ERR_INVALID_ARGUMENT;

    client->busy = true;
    client->op = BT_SDP_CLIENT_OP_ATTRIBUTE;
    client->attr_handle = service_record_handle;
    client->attr_max_bytes = max_attribute_byte_count;
    memcpy(client->attr_id_list, attribute_id_list, attribute_id_list_len);
    client->attr_id_list_len = attribute_id_list_len;
    client->continuation.len = 0;
    client->result_len = 0;
    client->on_complete = on_complete;
    client->complete_user_data = user_data;
    client->deadline_us = now_us + BT_SDP_CLIENT_REQUEST_TIMEOUT_US;

    issue_request(client, now_us);
    return BT_OK;
}

void bt_sdp_client_tick(struct bt_sdp_client *client, uint64_t now_us)
{
    if (!client->busy || client->deadline_us == 0 || now_us < client->deadline_us)
        return;
    fail(client, BT_SDP_CLIENT_ERROR_TIMEOUT);
}
