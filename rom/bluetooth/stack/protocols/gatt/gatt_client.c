#include <btcore/endian.h>
#include <btcore/gatt_client.h>

#include <string.h>

static void issue_current_request(struct bt_gatt_client *client, uint64_t now_us);

static uint64_t request_deadline(uint64_t now_us)
{
    if (UINT64_MAX - now_us < BT_GATT_CLIENT_REQUEST_TIMEOUT_US)
        return UINT64_MAX;
    return now_us + BT_GATT_CLIENT_REQUEST_TIMEOUT_US;
}

static void complete(struct bt_gatt_client *client, enum bt_gatt_client_result result,
                      uint8_t att_error_code)
{
    struct bt_gatt_client_completion completion;
    bt_gatt_client_complete_fn cb = client->on_complete;
    void *ud = client->complete_user_data;

    client->busy = false;
    client->request_deadline_us = 0;

    memset(&completion, 0, sizeof(completion));
    completion.result = result;
    completion.op = client->op;
    completion.att_error_code = att_error_code;

    if (result == BT_GATT_CLIENT_OK)
    {
        switch (client->op)
        {
        case BT_GATT_CLIENT_OP_DISCOVER_SERVICES:
            completion.services = client->result.services;
            completion.count = client->result_count;
            break;
        case BT_GATT_CLIENT_OP_DISCOVER_CHARACTERISTICS:
            completion.characteristics = client->result.characteristics;
            completion.count = client->result_count;
            break;
        case BT_GATT_CLIENT_OP_DISCOVER_DESCRIPTORS:
            completion.descriptors = client->result.descriptors;
            completion.count = client->result_count;
            break;
        case BT_GATT_CLIENT_OP_READ:
            completion.value = client->result.value;
            completion.value_len = client->result_len;
            break;
        case BT_GATT_CLIENT_OP_WRITE:
        case BT_GATT_CLIENT_OP_MTU:
            break;
        }
    }

    if (cb != NULL)
        cb(&completion, ud);
}

static void handle_mtu_response(struct bt_gatt_client *client, uint8_t opcode,
                                 const uint8_t *params, size_t params_len)
{
    uint16_t server_mtu;

    client->busy = false;
    client->request_deadline_us = 0;

    if (opcode == BT_ATT_OPCODE_EXCHANGE_MTU_RESPONSE &&
        bt_att_parse_exchange_mtu_response(params, params_len, &server_mtu) == BT_OK)
        client->mtu = (server_mtu < BT_GATT_CLIENT_REQUEST_MTU) ? server_mtu
                                                                 : BT_GATT_CLIENT_REQUEST_MTU;
    /* Anything else (ATT Error Response, malformed data) just means the
     * peer doesn't support Exchange MTU -- keep BT_ATT_DEFAULT_MTU. Not a
     * connection failure; Exchange MTU support is optional per spec. */

    if (client->on_connect != NULL)
    {
        bt_gatt_client_connect_fn cb = client->on_connect;
        void *ud = client->connect_user_data;

        client->on_connect = NULL;
        cb(true, ud);
    }
}

static void handle_discover_services_response(struct bt_gatt_client *client, uint8_t opcode,
                                                const uint8_t *params, size_t params_len,
                                                uint64_t now_us)
{
    struct bt_att_group_type_iter it;
    struct bt_att_group_entry entry;
    uint16_t last_end = client->pending_handle;
    bool any = false;

    if (opcode != BT_ATT_OPCODE_READ_BY_GROUP_TYPE_RESPONSE ||
        bt_att_read_by_group_type_response_iter_init(&it, params, params_len) != BT_OK)
    {
        complete(client, BT_GATT_CLIENT_ERROR_PROTOCOL, 0);
        return;
    }

    while (bt_att_read_by_group_type_response_iter_next(&it, &entry) == BT_OK)
    {
        any = true;
        last_end = entry.end_group_handle;

        if (entry.value_len == 2)
        {
            struct bt_gatt_service *svc;

            if (client->result_count >= BT_GATT_CLIENT_MAX_SERVICES)
            {
                complete(client, BT_GATT_CLIENT_ERROR_TOO_LARGE, 0);
                return;
            }
            svc = &client->result.services[client->result_count++];
            svc->start_handle = entry.handle;
            svc->end_handle = entry.end_group_handle;
            svc->uuid16 = bt_read_le16(entry.value);
        }
        /* 128-bit service UUIDs (value_len == 16) are skipped -- see the
         * scope reduction documented in btcore/gatt_client.h. */
    }

    if (!any)
    {
        complete(client, BT_GATT_CLIENT_ERROR_PROTOCOL, 0);
        return;
    }

    if (last_end == 0xFFFFu)
    {
        complete(client, BT_GATT_CLIENT_OK, 0);
        return;
    }

    client->pending_handle = (uint16_t)(last_end + 1);
    issue_current_request(client, now_us);
}

static void handle_discover_characteristics_response(struct bt_gatt_client *client, uint8_t opcode,
                                                       const uint8_t *params, size_t params_len,
                                                       uint64_t now_us)
{
    struct bt_att_read_by_type_iter it;
    struct bt_att_type_entry entry;
    uint16_t last_handle = client->pending_handle;
    bool any = false;

    if (opcode != BT_ATT_OPCODE_READ_BY_TYPE_RESPONSE ||
        bt_att_read_by_type_response_iter_init(&it, params, params_len) != BT_OK)
    {
        complete(client, BT_GATT_CLIENT_ERROR_PROTOCOL, 0);
        return;
    }

    while (bt_att_read_by_type_response_iter_next(&it, &entry) == BT_OK)
    {
        any = true;
        last_handle = entry.handle;

        if (entry.value_len == 5) /* properties(1) + value_handle(2) + uuid16(2) */
        {
            struct bt_gatt_characteristic *ch;

            if (client->result_count >= BT_GATT_CLIENT_MAX_CHARACTERISTICS)
            {
                complete(client, BT_GATT_CLIENT_ERROR_TOO_LARGE, 0);
                return;
            }
            ch = &client->result.characteristics[client->result_count++];
            ch->declaration_handle = entry.handle;
            ch->properties = entry.value[0];
            ch->value_handle = bt_read_le16(entry.value + 1);
            ch->uuid16 = bt_read_le16(entry.value + 3);
        }
        /* 128-bit characteristic UUIDs (value_len == 19) skipped, same
         * reduction as service discovery. */
    }

    if (!any)
    {
        complete(client, BT_GATT_CLIENT_ERROR_PROTOCOL, 0);
        return;
    }

    if (last_handle >= client->discover_range_end)
    {
        complete(client, BT_GATT_CLIENT_OK, 0);
        return;
    }

    client->pending_handle = (uint16_t)(last_handle + 1);
    issue_current_request(client, now_us);
}

static void handle_discover_descriptors_response(struct bt_gatt_client *client,
                                                  uint8_t opcode,
                                                  const uint8_t *params,
                                                  size_t params_len,
                                                  uint64_t now_us)
{
    struct bt_att_find_information_iter it;
    struct bt_att_information_entry entry;
    uint16_t last_handle = client->pending_handle;
    bool any = false;

    if (opcode != BT_ATT_OPCODE_FIND_INFORMATION_RESPONSE ||
        bt_att_find_information_response_iter_init(&it, params, params_len) != BT_OK)
    {
        complete(client, BT_GATT_CLIENT_ERROR_PROTOCOL, 0);
        return;
    }
    while (bt_att_find_information_response_iter_next(&it, &entry) == BT_OK)
    {
        any = true;
        last_handle = entry.handle;
        if (entry.uuid_len == 2)
        {
            struct bt_gatt_descriptor *descriptor;

            if (client->result_count == BT_GATT_CLIENT_MAX_DESCRIPTORS)
            {
                complete(client, BT_GATT_CLIENT_ERROR_TOO_LARGE, 0);
                return;
            }
            descriptor = &client->result.descriptors[client->result_count++];
            descriptor->handle = entry.handle;
            descriptor->uuid16 = bt_read_le16(entry.uuid);
        }
    }
    if (!any)
    {
        complete(client, BT_GATT_CLIENT_ERROR_PROTOCOL, 0);
        return;
    }
    if (last_handle >= client->discover_range_end || last_handle == 0xFFFFu)
    {
        complete(client, BT_GATT_CLIENT_OK, 0);
        return;
    }
    client->pending_handle = (uint16_t)(last_handle + 1u);
    issue_current_request(client, now_us);
}

static void handle_read_response(struct bt_gatt_client *client, uint8_t opcode,
                                  const uint8_t *params, size_t params_len,
                                  uint64_t now_us)
{
    uint8_t expected = client->result_len == 0 ? BT_ATT_OPCODE_READ_RESPONSE
                                                : BT_ATT_OPCODE_READ_BLOB_RESPONSE;

    if (opcode != expected)
    {
        complete(client, BT_GATT_CLIENT_ERROR_PROTOCOL, 0);
        return;
    }
    if (params_len > sizeof(client->result.value) - client->result_len)
    {
        complete(client, BT_GATT_CLIENT_ERROR_TOO_LARGE, 0);
        return;
    }

    if (params_len > 0)
        memcpy(client->result.value + client->result_len, params, params_len);
    client->result_len += params_len;
    if (params_len == (size_t)(client->mtu - 1u))
    {
        if (client->result_len > UINT16_MAX)
        {
            complete(client, BT_GATT_CLIENT_ERROR_TOO_LARGE, 0);
            return;
        }
        issue_current_request(client, now_us);
        return;
    }
    complete(client, BT_GATT_CLIENT_OK, 0);
}

static void handle_write_response(struct bt_gatt_client *client, uint8_t opcode,
                                   const uint8_t *params, size_t params_len)
{
    (void)params;
    (void)params_len;

    if (opcode != BT_ATT_OPCODE_WRITE_RESPONSE)
    {
        complete(client, BT_GATT_CLIENT_ERROR_PROTOCOL, 0);
        return;
    }
    complete(client, BT_GATT_CLIENT_OK, 0);
}

static void handle_response(struct bt_gatt_client *client, const uint8_t *data, size_t len,
                            uint64_t now_us)
{
    uint8_t opcode;
    const uint8_t *params;
    size_t params_len;

    if (len < 1)
        return;
    client->event_now_us = now_us;

    opcode = data[0];
    params = data + 1;
    params_len = len - 1;

    if (opcode == BT_ATT_OPCODE_HANDLE_VALUE_NOTIFICATION ||
        opcode == BT_ATT_OPCODE_HANDLE_VALUE_INDICATION)
    {
        struct bt_att_handle_value hv;

        if (bt_att_parse_handle_value(params, params_len, &hv) == BT_OK && client->on_notify != NULL)
            client->on_notify(hv.handle, hv.value, hv.value_len,
                               opcode == BT_ATT_OPCODE_HANDLE_VALUE_INDICATION,
                               client->notify_user_data);

        if (opcode == BT_ATT_OPCODE_HANDLE_VALUE_INDICATION)
        {
            uint8_t conf[1];
            struct bt_buf_writer w;

            bt_buf_writer_init(&w, conf, sizeof(conf));
            bt_att_encode_handle_value_confirmation(&w);
            bt_l2cap_channel_manager_send(client->l2cap, BT_L2CAP_CID_ATT, conf,
                                           bt_buf_writer_len(&w), 0);
        }
        return;
    }

    if (client->on_connect != NULL)
    {
        /* Still doing (or just failed) Exchange MTU. */
        handle_mtu_response(client, opcode, params, params_len);
        return;
    }

    if (!client->busy)
        return; /* unsolicited response-shaped PDU, or arrived too late -- ignore */

    if (opcode == BT_ATT_OPCODE_ERROR_RESPONSE)
    {
        struct bt_att_error_response err;

        if (bt_att_parse_error_response(params, params_len, &err) != BT_OK)
        {
            complete(client, BT_GATT_CLIENT_ERROR_PROTOCOL, 0);
            return;
        }
        if ((client->op == BT_GATT_CLIENT_OP_DISCOVER_SERVICES ||
             client->op == BT_GATT_CLIENT_OP_DISCOVER_CHARACTERISTICS ||
             client->op == BT_GATT_CLIENT_OP_DISCOVER_DESCRIPTORS) &&
            err.error_code == BT_ATT_ERROR_ATTRIBUTE_NOT_FOUND)
        {
            /* Normal termination: no more results, not a failure. */
            complete(client, BT_GATT_CLIENT_OK, 0);
            return;
        }
        if (client->op == BT_GATT_CLIENT_OP_READ && client->result_len != 0 &&
            err.request_opcode == BT_ATT_OPCODE_READ_BLOB_REQUEST &&
            err.error_code == BT_ATT_ERROR_INVALID_OFFSET)
        {
            complete(client, BT_GATT_CLIENT_OK, 0);
            return;
        }
        complete(client, BT_GATT_CLIENT_ERROR_ATT, err.error_code);
        return;
    }

    switch (client->op)
    {
    case BT_GATT_CLIENT_OP_DISCOVER_SERVICES:
        handle_discover_services_response(client, opcode, params, params_len, now_us);
        break;
    case BT_GATT_CLIENT_OP_DISCOVER_CHARACTERISTICS:
        handle_discover_characteristics_response(client, opcode, params, params_len, now_us);
        break;
    case BT_GATT_CLIENT_OP_DISCOVER_DESCRIPTORS:
        handle_discover_descriptors_response(client, opcode, params, params_len, now_us);
        break;
    case BT_GATT_CLIENT_OP_READ:
        handle_read_response(client, opcode, params, params_len, now_us);
        break;
    case BT_GATT_CLIENT_OP_WRITE:
        handle_write_response(client, opcode, params, params_len);
        break;
    case BT_GATT_CLIENT_OP_MTU:
        break; /* handled above via on_connect, unreachable here */
    }
}

static void issue_current_request(struct bt_gatt_client *client, uint64_t now_us)
{
    uint8_t buf[3 + BT_GATT_CLIENT_MAX_VALUE_LEN];
    struct bt_buf_writer w;
    bt_status_t st = BT_OK;

    bt_buf_writer_init(&w, buf, sizeof(buf));

    switch (client->op)
    {
    case BT_GATT_CLIENT_OP_MTU:
        st = bt_att_encode_exchange_mtu_request(&w, BT_GATT_CLIENT_REQUEST_MTU);
        break;
    case BT_GATT_CLIENT_OP_DISCOVER_SERVICES:
        st = bt_att_encode_read_by_group_type_request(&w, client->pending_handle,
                                                        client->discover_range_end,
                                                        BT_GATT_UUID_PRIMARY_SERVICE);
        break;
    case BT_GATT_CLIENT_OP_DISCOVER_CHARACTERISTICS:
        st = bt_att_encode_read_by_type_request(&w, client->pending_handle,
                                                  client->discover_range_end,
                                                  BT_GATT_UUID_CHARACTERISTIC);
        break;
    case BT_GATT_CLIENT_OP_DISCOVER_DESCRIPTORS:
        st = bt_att_encode_find_information_request(&w, client->pending_handle,
                                                     client->discover_range_end);
        break;
    case BT_GATT_CLIENT_OP_READ:
        if (client->result_len == 0)
            st = bt_att_encode_read_request(&w, client->pending_handle);
        else
            st = bt_att_encode_read_blob_request(&w, client->pending_handle,
                                                  (uint16_t)client->result_len);
        break;
    case BT_GATT_CLIENT_OP_WRITE:
        st = bt_att_encode_write_request(&w, client->pending_handle, client->result.value,
                                          client->result_len);
        break;
    }

    if (st != BT_OK)
    {
        if (client->op == BT_GATT_CLIENT_OP_MTU)
            handle_mtu_response(client, 0, NULL, 0); /* fall back to default MTU, still connect */
        else
            complete(client, BT_GATT_CLIENT_ERROR_PROTOCOL, 0);
        return;
    }

    if (bt_l2cap_channel_manager_send(client->l2cap, BT_L2CAP_CID_ATT, buf, bt_buf_writer_len(&w),
                                       0) != BT_OK)
    {
        if (client->op == BT_GATT_CLIENT_OP_MTU)
            handle_mtu_response(client, 0, NULL, 0);
        else
            complete(client, BT_GATT_CLIENT_ERROR_CLOSED, 0);
        return;
    }

    client->request_deadline_us = request_deadline(now_us);
}

static void on_l2cap_event(struct bt_l2cap_channel_event_info *info, void *user_data)
{
    struct bt_gatt_client *client = (struct bt_gatt_client *)user_data;

    switch (info->event)
    {
    case BT_L2CAP_CHANNEL_EVENT_OPENED:
        client->channel_ready = true;
        client->busy = true;
        client->op = BT_GATT_CLIENT_OP_MTU;
        issue_current_request(client, client->connect_started_us);
        break;

    case BT_L2CAP_CHANNEL_EVENT_CLOSED:
        client->channel_ready = false;
        if (client->on_connect != NULL)
        {
            bt_gatt_client_connect_fn cb = client->on_connect;
            void *ud = client->connect_user_data;

            client->on_connect = NULL;
            cb(false, ud);
        }
        else if (client->busy)
        {
            complete(client, BT_GATT_CLIENT_ERROR_CLOSED, 0);
        }
        break;

    case BT_L2CAP_CHANNEL_EVENT_DATA:
        handle_response(client, info->data, info->data_len, info->now_us);
        break;
    }
}

void bt_gatt_client_init(struct bt_gatt_client *client, struct bt_l2cap_channel_manager *l2cap)
{
    memset(client, 0, sizeof(*client));
    client->l2cap = l2cap;
    client->mtu = BT_ATT_DEFAULT_MTU;
}

bt_status_t bt_gatt_client_connect(struct bt_gatt_client *client, bt_gatt_client_connect_fn on_connect,
                                    void *user_data, uint64_t now_us)
{
    client->on_connect = on_connect;
    client->connect_user_data = user_data;
    client->connect_started_us = now_us;

    return bt_l2cap_channel_manager_open_fixed(client->l2cap, BT_L2CAP_CID_ATT, on_l2cap_event,
                                                client);
}

void bt_gatt_client_disconnect(struct bt_gatt_client *client, uint64_t now_us)
{
    if (client->channel_ready)
        bt_l2cap_channel_manager_close(client->l2cap, BT_L2CAP_CID_ATT, now_us);
    client->channel_ready = false;
}

void bt_gatt_client_set_notify_handler(struct bt_gatt_client *client, bt_gatt_client_notify_fn fn,
                                        void *user_data)
{
    client->on_notify = fn;
    client->notify_user_data = user_data;
}

bt_status_t bt_gatt_client_discover_services(struct bt_gatt_client *client,
                                              bt_gatt_client_complete_fn on_complete,
                                              void *user_data, uint64_t now_us)
{
    if (!client->channel_ready || client->busy)
        return BT_ERR_INVALID_ARGUMENT;

    client->busy = true;
    client->op = BT_GATT_CLIENT_OP_DISCOVER_SERVICES;
    client->pending_handle = 0x0001;
    client->discover_range_end = 0xFFFF;
    client->result_count = 0;
    client->on_complete = on_complete;
    client->complete_user_data = user_data;

    issue_current_request(client, now_us);
    return BT_OK;
}

bt_status_t bt_gatt_client_discover_characteristics(struct bt_gatt_client *client,
                                                      uint16_t service_start_handle,
                                                      uint16_t service_end_handle,
                                                      bt_gatt_client_complete_fn on_complete,
                                                      void *user_data, uint64_t now_us)
{
    if (!client->channel_ready || client->busy)
        return BT_ERR_INVALID_ARGUMENT;

    client->busy = true;
    client->op = BT_GATT_CLIENT_OP_DISCOVER_CHARACTERISTICS;
    client->pending_handle = service_start_handle;
    client->discover_range_end = service_end_handle;
    client->result_count = 0;
    client->on_complete = on_complete;
    client->complete_user_data = user_data;

    issue_current_request(client, now_us);
    return BT_OK;
}

bt_status_t bt_gatt_client_read(struct bt_gatt_client *client, uint16_t handle,
                                 bt_gatt_client_complete_fn on_complete, void *user_data,
                                 uint64_t now_us)
{
    if (!client->channel_ready || client->busy)
        return BT_ERR_INVALID_ARGUMENT;

    client->busy = true;
    client->op = BT_GATT_CLIENT_OP_READ;
    client->pending_handle = handle;
    client->result_len = 0;
    client->on_complete = on_complete;
    client->complete_user_data = user_data;

    issue_current_request(client, now_us);
    return BT_OK;
}

bt_status_t bt_gatt_client_discover_descriptors(struct bt_gatt_client *client,
                                                 uint16_t start_handle,
                                                 uint16_t end_handle,
                                                 bt_gatt_client_complete_fn on_complete,
                                                 void *user_data, uint64_t now_us)
{
    if (!client->channel_ready || client->busy || start_handle == 0 ||
        start_handle > end_handle)
        return BT_ERR_INVALID_ARGUMENT;
    client->busy = true;
    client->op = BT_GATT_CLIENT_OP_DISCOVER_DESCRIPTORS;
    client->pending_handle = start_handle;
    client->discover_range_end = end_handle;
    client->result_count = 0;
    client->on_complete = on_complete;
    client->complete_user_data = user_data;
    issue_current_request(client, now_us);
    return BT_OK;
}

bt_status_t bt_gatt_client_write(struct bt_gatt_client *client, uint16_t handle,
                                  const uint8_t *value, size_t value_len,
                                  bt_gatt_client_complete_fn on_complete, void *user_data,
                                  uint64_t now_us)
{
    if (!client->channel_ready || client->busy)
        return BT_ERR_INVALID_ARGUMENT;
    if (value_len > sizeof(client->result.value))
        return BT_ERR_INVALID_ARGUMENT;

    client->busy = true;
    client->op = BT_GATT_CLIENT_OP_WRITE;
    client->pending_handle = handle;
    if (value_len > 0)
        memcpy(client->result.value, value, value_len);
    client->result_len = value_len;
    client->on_complete = on_complete;
    client->complete_user_data = user_data;

    issue_current_request(client, now_us);
    return BT_OK;
}

void bt_gatt_client_tick(struct bt_gatt_client *client, uint64_t now_us)
{
    if (!client->busy || client->request_deadline_us == 0 || now_us < client->request_deadline_us)
        return;

    if (client->on_connect != NULL)
        handle_mtu_response(client, 0, NULL, 0); /* optional MTU exchange timed out */
    else
        complete(client, BT_GATT_CLIENT_ERROR_TIMEOUT, 0);
}
