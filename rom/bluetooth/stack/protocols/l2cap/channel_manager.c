#include <btcore/l2cap_channel.h>

static struct bt_l2cap_channel *find_free_channel(struct bt_l2cap_channel_manager *mgr)
{
    size_t i;

    for (i = 0; i < BT_L2CAP_CHANNEL_MANAGER_MAX_CHANNELS; i++)
    {
        if (mgr->channels[i].state == BT_L2CAP_CHAN_FREE)
            return &mgr->channels[i];
    }
    return NULL;
}

static struct bt_l2cap_channel *find_channel_by_identifier(struct bt_l2cap_channel_manager *mgr,
                                                             uint8_t identifier)
{
    size_t i;

    for (i = 0; i < BT_L2CAP_CHANNEL_MANAGER_MAX_CHANNELS; i++)
    {
        struct bt_l2cap_channel *chan = &mgr->channels[i];

        if (chan->state != BT_L2CAP_CHAN_FREE && chan->pending_identifier == identifier)
            return chan;
    }
    return NULL;
}

static struct bt_l2cap_channel *find_channel_by_local_cid(struct bt_l2cap_channel_manager *mgr,
                                                            uint16_t local_cid)
{
    size_t i;

    for (i = 0; i < BT_L2CAP_CHANNEL_MANAGER_MAX_CHANNELS; i++)
    {
        struct bt_l2cap_channel *chan = &mgr->channels[i];

        if (chan->state != BT_L2CAP_CHAN_FREE && chan->local_cid == local_cid)
            return chan;
    }
    return NULL;
}

static uint16_t alloc_local_cid(struct bt_l2cap_channel_manager *mgr)
{
    uint16_t cid = mgr->next_local_cid;

    mgr->next_local_cid++;
    if (mgr->next_local_cid < BT_L2CAP_CID_DYNAMIC_START)
        mgr->next_local_cid = BT_L2CAP_CID_DYNAMIC_START; /* guard the extremely unlikely wraparound */

    return cid;
}

static uint8_t alloc_identifier(struct bt_l2cap_channel_manager *mgr)
{
    uint8_t id = mgr->next_identifier;

    mgr->next_identifier++;
    if (mgr->next_identifier == 0)
        mgr->next_identifier = 1; /* 0 is never used as a real identifier */

    return id;
}

static bt_status_t send_fragmented(struct bt_l2cap_channel_manager *mgr, const uint8_t *pdu,
                                    size_t pdu_len)
{
    struct bt_l2cap_fragmenter fr;

    bt_l2cap_fragmenter_init(&fr, mgr->handle, mgr->acl_frag_size, pdu, pdu_len);

    for (;;)
    {
        uint8_t acl[BT_HCI_ACL_HEADER_LEN + BT_L2CAP_MAX_ACL_FRAGMENT];
        struct bt_buf_writer aw;
        bt_status_t st;
        int rc;

        bt_buf_writer_init(&aw, acl, sizeof(acl));
        st = bt_l2cap_fragmenter_next(&fr, &aw);
        if (st == BT_ERR_BUFFER_UNDERFLOW)
            return BT_OK;
        if (st != BT_OK)
            return st;

        rc = mgr->transport->ops->send_acl(mgr->transport, acl, bt_buf_writer_len(&aw));
        if (rc != 0)
            return BT_ERR_INVALID_ARGUMENT;
    }
}

static bt_status_t send_l2cap_pdu(struct bt_l2cap_channel_manager *mgr, uint16_t cid,
                                   const uint8_t *payload, size_t payload_len)
{
    uint8_t pdu[BT_L2CAP_HEADER_LEN + 64]; /* signaling commands are small */
    struct bt_buf_writer w;
    bt_status_t st;

    if (payload_len > sizeof(pdu) - BT_L2CAP_HEADER_LEN)
        return BT_ERR_INVALID_ARGUMENT;

    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    st = bt_l2cap_encode_header(&w, (uint16_t)payload_len, cid);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_bytes(&w, payload, payload_len);
    if (st != BT_OK)
        return st;

    return send_fragmented(mgr, pdu, bt_buf_writer_len(&w));
}

static void fire_event(struct bt_l2cap_channel *chan, struct bt_l2cap_channel_event_info *info)
{
    if (chan->on_event != NULL)
        chan->on_event(info, chan->user_data);
}

static void fire_opened(struct bt_l2cap_channel *chan)
{
    struct bt_l2cap_channel_event_info info;

    info.event = BT_L2CAP_CHANNEL_EVENT_OPENED;
    info.local_cid = chan->local_cid;
    info.close_reason = BT_L2CAP_CLOSE_LOCAL; /* unused for this event */
    info.data = NULL;
    info.data_len = 0;
    info.now_us = 0; /* synchronous open has no clock argument */
    fire_event(chan, &info);
}

static void finish_close(struct bt_l2cap_channel *chan, enum bt_l2cap_close_reason reason)
{
    struct bt_l2cap_channel_event_info info;

    if (chan->rtx_timer.pending)
        bt_timer_list_cancel(&chan->owner->timers, &chan->rtx_timer);

    info.event = BT_L2CAP_CHANNEL_EVENT_CLOSED;
    info.local_cid = chan->local_cid;
    info.close_reason = reason;
    info.data = NULL;
    info.data_len = 0;
    info.now_us = 0;

    /* Free the slot before invoking the callback: a well-behaved consumer
     * may react by opening a new channel, which should be able to reuse
     * this slot immediately. */
    chan->state = BT_L2CAP_CHAN_FREE;
    chan->pending_identifier = 0;

    fire_event(chan, &info);
}

static void rtx_timeout_callback(struct bt_timer *timer, void *user_data)
{
    struct bt_l2cap_channel *chan = (struct bt_l2cap_channel *)user_data;

    (void)timer;
    finish_close(chan, BT_L2CAP_CLOSE_TIMEOUT);
}

static bt_status_t send_connection_request(struct bt_l2cap_channel_manager *mgr,
                                            struct bt_l2cap_channel *chan)
{
    uint8_t buf[BT_L2CAP_SIG_HEADER_LEN + 4];
    struct bt_buf_writer w;
    bt_status_t st;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    st = bt_l2cap_sig_encode_connection_request(&w, chan->pending_identifier, chan->psm,
                                                 chan->local_cid);
    if (st != BT_OK)
        return st;

    return send_l2cap_pdu(mgr, mgr->signaling_cid, buf, bt_buf_writer_len(&w));
}

static void start_our_configure_request(struct bt_l2cap_channel_manager *mgr,
                                         struct bt_l2cap_channel *chan, uint64_t now_us)
{
    uint8_t buf[BT_L2CAP_SIG_HEADER_LEN + 8];
    struct bt_buf_writer w;

    chan->pending_identifier = alloc_identifier(mgr);
    bt_timer_list_add(&mgr->timers, &chan->rtx_timer, now_us + BT_L2CAP_RTX_TIMEOUT_US);

    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_configure_request(&w, chan->pending_identifier, chan->remote_cid, 0,
                                           chan->local_mtu);
    /* Best-effort: a send failure here surfaces later via this request's
     * own rtx timeout rather than being handled inline. */
    send_l2cap_pdu(mgr, mgr->signaling_cid, buf, bt_buf_writer_len(&w));
}

static void handle_connection_response(struct bt_l2cap_channel_manager *mgr, uint8_t identifier,
                                        const uint8_t *cmd_data, size_t cmd_data_len,
                                        uint64_t now_us)
{
    struct bt_l2cap_channel *chan = find_channel_by_identifier(mgr, identifier);
    struct bt_l2cap_connection_response rsp;

    if (chan == NULL || chan->state != BT_L2CAP_CHAN_WAIT_CONNECT_RSP)
        return; /* stale or unmatched response -- ignore */

    if (bt_l2cap_sig_parse_connection_response(cmd_data, cmd_data_len, &rsp) != BT_OK)
        return;

    if (rsp.result == BT_L2CAP_CONN_RESULT_PENDING)
        return; /* keep waiting -- see header's documented scope reduction */

    bt_timer_list_cancel(&mgr->timers, &chan->rtx_timer);

    if (rsp.result != BT_L2CAP_CONN_RESULT_SUCCESS)
    {
        finish_close(chan, BT_L2CAP_CLOSE_REFUSED);
        return;
    }

    chan->remote_cid = rsp.destination_cid;
    chan->state = BT_L2CAP_CHAN_CONFIG;
    start_our_configure_request(mgr, chan, now_us);
}

static void handle_connection_request(struct bt_l2cap_channel_manager *mgr, uint8_t identifier,
                                       const uint8_t *cmd_data, size_t cmd_data_len,
                                       uint64_t now_us)
{
    struct bt_l2cap_connection_request req;
    struct bt_l2cap_listener *lst = NULL;
    struct bt_l2cap_channel *chan;
    uint8_t buf[BT_L2CAP_SIG_HEADER_LEN + 8];
    struct bt_buf_writer w;
    size_t i;

    if (bt_l2cap_sig_parse_connection_request(cmd_data, cmd_data_len, &req) != BT_OK)
        return;

    for (i = 0; i < BT_L2CAP_CHANNEL_MANAGER_MAX_LISTENERS; i++)
    {
        if (mgr->listeners[i].psm == req.psm)
        {
            lst = &mgr->listeners[i];
            break;
        }
    }
    chan = (lst != NULL) ? find_free_channel(mgr) : NULL;
    if (chan == NULL)
    {
        /* nobody listening (or no slot): refuse */
        bt_buf_writer_init(&w, buf, sizeof(buf));
        bt_l2cap_sig_encode_connection_response(&w, identifier, 0, req.source_cid,
                                                 lst == NULL ? BT_L2CAP_CONN_RESULT_REFUSED_PSM
                                                             : 0x0004u /* no resources */,
                                                 0);
        send_l2cap_pdu(mgr, mgr->signaling_cid, buf, bt_buf_writer_len(&w));
        return;
    }

    chan->state = BT_L2CAP_CHAN_CONFIG;
    chan->is_fixed = false;
    chan->psm = req.psm;
    chan->local_cid = alloc_local_cid(mgr);
    chan->remote_cid = req.source_cid;
    chan->local_mtu = (lst->local_mtu != 0) ? lst->local_mtu : BT_L2CAP_DEFAULT_MTU;
    chan->remote_mtu = BT_L2CAP_DEFAULT_MTU;
    chan->outbound_config_done = false;
    chan->inbound_config_done = false;
    chan->on_event = lst->on_event;
    chan->user_data = lst->user_data;
    chan->pending_identifier = 0;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_connection_response(&w, identifier, chan->local_cid, chan->remote_cid,
                                             BT_L2CAP_CONN_RESULT_SUCCESS, 0);
    send_l2cap_pdu(mgr, mgr->signaling_cid, buf, bt_buf_writer_len(&w));

    start_our_configure_request(mgr, chan, now_us);
}

static void handle_configure_request(struct bt_l2cap_channel_manager *mgr, uint8_t identifier,
                                      const uint8_t *cmd_data, size_t cmd_data_len,
                                      uint64_t now_us)
{
    struct bt_l2cap_configure_request req;
    struct bt_l2cap_channel *chan;
    uint8_t buf[BT_L2CAP_SIG_HEADER_LEN + 10];
    struct bt_buf_writer w;

    (void)now_us;

    if (bt_l2cap_sig_parse_configure_request(cmd_data, cmd_data_len, &req) != BT_OK)
        return;

    chan = find_channel_by_local_cid(mgr, req.destination_cid);
    if (chan == NULL || chan->state != BT_L2CAP_CHAN_CONFIG)
        return; /* unknown channel, or not expecting config yet -- a strict
                  * implementation would send Command Reject here */

    if (req.has_mtu)
        chan->remote_mtu = req.mtu;

    /* Accept whatever was proposed -- see header's documented scope
     * reduction (no negotiation loop). Source CID in a Configure Response
     * names the channel endpoint of the device RECEIVING the response
     * (spec Vol 3 Part A 4.5), i.e. the requester's CID: real stacks
     * (Android) validate it and abandon the channel on a mismatch. */
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_configure_response(&w, identifier, chan->remote_cid, 0,
                                            BT_L2CAP_CONFIG_RESULT_SUCCESS, 0);
    send_l2cap_pdu(mgr, mgr->signaling_cid, buf, bt_buf_writer_len(&w));

    chan->inbound_config_done = true;
    if (chan->outbound_config_done)
    {
        chan->state = BT_L2CAP_CHAN_OPEN;
        fire_opened(chan);
    }
}

static void handle_configure_response(struct bt_l2cap_channel_manager *mgr, uint8_t identifier,
                                       const uint8_t *cmd_data, size_t cmd_data_len,
                                       uint64_t now_us)
{
    struct bt_l2cap_channel *chan = find_channel_by_identifier(mgr, identifier);
    struct bt_l2cap_configure_response rsp;

    (void)now_us;

    if (chan == NULL || chan->state != BT_L2CAP_CHAN_CONFIG)
        return;

    if (bt_l2cap_sig_parse_configure_response(cmd_data, cmd_data_len, &rsp) != BT_OK)
        return;

    bt_timer_list_cancel(&mgr->timers, &chan->rtx_timer);
    chan->pending_identifier = 0;

    if (rsp.result != BT_L2CAP_CONFIG_RESULT_SUCCESS)
    {
        finish_close(chan, BT_L2CAP_CLOSE_CONFIG_FAILED);
        return;
    }

    chan->outbound_config_done = true;
    if (chan->inbound_config_done)
    {
        chan->state = BT_L2CAP_CHAN_OPEN;
        fire_opened(chan);
    }
}

static void handle_disconnection_request(struct bt_l2cap_channel_manager *mgr, uint8_t identifier,
                                          const uint8_t *cmd_data, size_t cmd_data_len,
                                          uint64_t now_us)
{
    struct bt_l2cap_disconnection req;
    struct bt_l2cap_channel *chan;
    uint8_t buf[BT_L2CAP_SIG_HEADER_LEN + 4];
    struct bt_buf_writer w;

    (void)now_us;

    if (bt_l2cap_sig_parse_disconnection(cmd_data, cmd_data_len, &req) != BT_OK)
        return;

    chan = find_channel_by_local_cid(mgr, req.destination_cid);
    if (chan == NULL)
        return;

    /* Always ack, even mid-negotiation or mid our-own-disconnect
     * (simultaneous disconnect). */
    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_disconnection_response(&w, identifier, req.destination_cid,
                                                req.source_cid);
    send_l2cap_pdu(mgr, mgr->signaling_cid, buf, bt_buf_writer_len(&w));

    finish_close(chan, BT_L2CAP_CLOSE_PEER_DISCONNECTED);
}

static void handle_disconnection_response(struct bt_l2cap_channel_manager *mgr,
                                           uint8_t identifier, const uint8_t *cmd_data,
                                           size_t cmd_data_len, uint64_t now_us)
{
    struct bt_l2cap_channel *chan = find_channel_by_identifier(mgr, identifier);
    struct bt_l2cap_disconnection rsp;

    (void)now_us;

    if (chan == NULL || chan->state != BT_L2CAP_CHAN_WAIT_DISCONNECT_RSP)
        return;
    if (bt_l2cap_sig_parse_disconnection(cmd_data, cmd_data_len, &rsp) != BT_OK)
        return;

    finish_close(chan, BT_L2CAP_CLOSE_LOCAL);
}

static void handle_command_reject(struct bt_l2cap_channel_manager *mgr, uint8_t identifier,
                                   const uint8_t *cmd_data, size_t cmd_data_len, uint64_t now_us)
{
    struct bt_l2cap_channel *chan = find_channel_by_identifier(mgr, identifier);

    (void)cmd_data;
    (void)cmd_data_len;
    (void)now_us;

    if (chan == NULL)
        return;

    /* Whatever we asked for was rejected outright -- treat as a failure
     * of whichever phase we were in. */
    if (chan->state == BT_L2CAP_CHAN_WAIT_CONNECT_RSP)
        finish_close(chan, BT_L2CAP_CLOSE_REFUSED);
    else
        finish_close(chan, BT_L2CAP_CLOSE_CONFIG_FAILED);
}

static void handle_echo_request(struct bt_l2cap_channel_manager *mgr, uint8_t identifier,
                                 const uint8_t *cmd_data, size_t cmd_data_len)
{
    uint8_t buf[BT_L2CAP_SIG_HEADER_LEN + 44];
    struct bt_buf_writer w;
    size_t n = (cmd_data_len > 44) ? 44 : cmd_data_len;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_header(&w, BT_L2CAP_SIG_ECHO_RESPONSE, identifier, (uint16_t)n);
    if (n > 0)
        bt_buf_writer_write_bytes(&w, cmd_data, n);
    send_l2cap_pdu(mgr, mgr->signaling_cid, buf, bt_buf_writer_len(&w));
}

static void handle_information_request(struct bt_l2cap_channel_manager *mgr, uint8_t identifier,
                                        const uint8_t *cmd_data, size_t cmd_data_len)
{
    /* Android sends these on every new BR/EDR link and waits for the answer
     * before it completes channel setup: leaving them unanswered stalls (and
     * eventually aborts) every outgoing connection to such a peer. */
    uint8_t buf[BT_L2CAP_SIG_HEADER_LEN + 4 + 8];
    struct bt_buf_writer w;
    struct bt_buf_reader r;
    uint16_t info_type;

    bt_buf_reader_init(&r, cmd_data, cmd_data_len);
    if (bt_buf_reader_read_le16(&r, &info_type) != BT_OK)
        return;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    if (info_type == 0x0002u)
    {
        /* extended features supported: fixed channels only (no ERTM,
         * streaming, FCS options or connectionless traffic) */
        bt_l2cap_sig_encode_header(&w, BT_L2CAP_SIG_INFORMATION_RESPONSE, identifier, 8);
        bt_buf_writer_write_le16(&w, info_type);
        bt_buf_writer_write_le16(&w, 0x0000u); /* success */
        bt_buf_writer_write_le32(&w, 0x00000080u); /* bit 7: fixed channels */
    }
    else if (info_type == 0x0003u)
    {
        /* fixed channels supported: only the signaling channel (CID 1) */
        bt_l2cap_sig_encode_header(&w, BT_L2CAP_SIG_INFORMATION_RESPONSE, identifier, 12);
        bt_buf_writer_write_le16(&w, info_type);
        bt_buf_writer_write_le16(&w, 0x0000u); /* success */
        bt_buf_writer_write_le32(&w, 0x00000002u); /* bit 1: CID 0x0001 */
        bt_buf_writer_write_le32(&w, 0x00000000u);
    }
    else
    {
        /* connectionless MTU and anything newer: not supported */
        bt_l2cap_sig_encode_header(&w, BT_L2CAP_SIG_INFORMATION_RESPONSE, identifier, 4);
        bt_buf_writer_write_le16(&w, info_type);
        bt_buf_writer_write_le16(&w, 0x0001u); /* not supported */
    }
    send_l2cap_pdu(mgr, mgr->signaling_cid, buf, bt_buf_writer_len(&w));
}

static void send_command_reject(struct bt_l2cap_channel_manager *mgr, uint8_t identifier)
{
    uint8_t buf[BT_L2CAP_SIG_HEADER_LEN + 2];
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, buf, sizeof(buf));
    bt_l2cap_sig_encode_header(&w, BT_L2CAP_SIG_COMMAND_REJECT, identifier, 2);
    bt_buf_writer_write_le16(&w, 0x0000u); /* command not understood */
    send_l2cap_pdu(mgr, mgr->signaling_cid, buf, bt_buf_writer_len(&w));
}

static void process_signaling_pdu(struct bt_l2cap_channel_manager *mgr, const uint8_t *payload,
                                   size_t payload_len, uint64_t now_us)
{
    struct bt_buf_reader r;
    struct bt_l2cap_sig_header hdr;
    const uint8_t *cmd_data;

    bt_buf_reader_init(&r, payload, payload_len);
    if (bt_l2cap_sig_parse_header(&r, &hdr) != BT_OK)
        return;

    cmd_data = bt_buf_reader_peek(&r, hdr.length);
    if (cmd_data == NULL)
        return; /* truncated -- drop */

    switch (hdr.code)
    {
    case BT_L2CAP_SIG_CONNECTION_REQUEST:
        handle_connection_request(mgr, hdr.identifier, cmd_data, hdr.length, now_us);
        break;
    case BT_L2CAP_SIG_CONNECTION_RESPONSE:
        handle_connection_response(mgr, hdr.identifier, cmd_data, hdr.length, now_us);
        break;
    case BT_L2CAP_SIG_CONFIGURE_REQUEST:
        handle_configure_request(mgr, hdr.identifier, cmd_data, hdr.length, now_us);
        break;
    case BT_L2CAP_SIG_CONFIGURE_RESPONSE:
        handle_configure_response(mgr, hdr.identifier, cmd_data, hdr.length, now_us);
        break;
    case BT_L2CAP_SIG_DISCONNECTION_REQUEST:
        handle_disconnection_request(mgr, hdr.identifier, cmd_data, hdr.length, now_us);
        break;
    case BT_L2CAP_SIG_DISCONNECTION_RESPONSE:
        handle_disconnection_response(mgr, hdr.identifier, cmd_data, hdr.length, now_us);
        break;
    case BT_L2CAP_SIG_COMMAND_REJECT:
        handle_command_reject(mgr, hdr.identifier, cmd_data, hdr.length, now_us);
        break;
    case BT_L2CAP_SIG_ECHO_REQUEST:
        handle_echo_request(mgr, hdr.identifier, cmd_data, hdr.length);
        break;
    case BT_L2CAP_SIG_INFORMATION_REQUEST:
        handle_information_request(mgr, hdr.identifier, cmd_data, hdr.length);
        break;
    case BT_L2CAP_SIG_ECHO_RESPONSE:
    case BT_L2CAP_SIG_INFORMATION_RESPONSE:
        break; /* we never ask, but do not Command Reject a response */
    default:
        send_command_reject(mgr, hdr.identifier);
        break;
    }
}

void bt_l2cap_channel_manager_init(struct bt_l2cap_channel_manager *mgr,
                                    struct bt_hci_transport *transport, uint16_t handle,
                                    uint16_t signaling_cid, size_t acl_frag_size)
{
    size_t i;

    mgr->transport = transport;
    mgr->handle = handle;
    mgr->signaling_cid = signaling_cid;
    mgr->acl_frag_size =
        (acl_frag_size > BT_L2CAP_MAX_ACL_FRAGMENT || acl_frag_size == 0)
            ? BT_L2CAP_MAX_ACL_FRAGMENT
            : acl_frag_size;

    bt_timer_list_init(&mgr->timers);
    bt_l2cap_reassembler_init(&mgr->reassembler);
    mgr->next_local_cid = BT_L2CAP_CID_DYNAMIC_START;
    mgr->next_identifier = 1;

    for (i = 0; i < BT_L2CAP_CHANNEL_MANAGER_MAX_CHANNELS; i++)
    {
        struct bt_l2cap_channel *chan = &mgr->channels[i];

        chan->owner = mgr;
        chan->state = BT_L2CAP_CHAN_FREE;
        chan->pending_identifier = 0;
        bt_timer_init(&chan->rtx_timer, rtx_timeout_callback, chan);
    }
    for (i = 0; i < BT_L2CAP_CHANNEL_MANAGER_MAX_LISTENERS; i++)
        mgr->listeners[i].psm = 0;
}

bt_status_t bt_l2cap_channel_manager_listen(struct bt_l2cap_channel_manager *mgr, uint16_t psm,
                                             uint16_t local_mtu, bt_l2cap_channel_event_fn on_event,
                                             void *user_data)
{
    size_t i;
    struct bt_l2cap_listener *free_slot = NULL;

    if (psm == 0)
        return BT_ERR_INVALID_ARGUMENT;
    for (i = 0; i < BT_L2CAP_CHANNEL_MANAGER_MAX_LISTENERS; i++)
    {
        if (mgr->listeners[i].psm == psm)
        {
            free_slot = &mgr->listeners[i]; /* replace */
            break;
        }
        if (mgr->listeners[i].psm == 0 && free_slot == NULL)
            free_slot = &mgr->listeners[i];
    }
    if (free_slot == NULL)
        return BT_ERR_NO_RESOURCES;
    free_slot->psm = psm;
    free_slot->local_mtu = local_mtu;
    free_slot->on_event = on_event;
    free_slot->user_data = user_data;
    return BT_OK;
}

void bt_l2cap_channel_manager_unlisten(struct bt_l2cap_channel_manager *mgr, uint16_t psm)
{
    size_t i;

    for (i = 0; i < BT_L2CAP_CHANNEL_MANAGER_MAX_LISTENERS; i++)
    {
        if (mgr->listeners[i].psm == psm)
            mgr->listeners[i].psm = 0;
    }
}

bt_status_t bt_l2cap_channel_manager_open(struct bt_l2cap_channel_manager *mgr, uint16_t psm,
                                           uint16_t local_mtu, bt_l2cap_channel_event_fn on_event,
                                           void *user_data, uint16_t *out_local_cid,
                                           uint64_t now_us)
{
    struct bt_l2cap_channel *chan = find_free_channel(mgr);
    uint16_t local_cid;
    bt_status_t st;

    if (chan == NULL)
        return BT_ERR_NO_RESOURCES;

    local_cid = alloc_local_cid(mgr);

    chan->state = BT_L2CAP_CHAN_WAIT_CONNECT_RSP;
    chan->is_fixed = false;
    chan->psm = psm;
    chan->local_cid = local_cid;
    chan->remote_cid = 0;
    chan->local_mtu = (local_mtu != 0) ? local_mtu : BT_L2CAP_DEFAULT_MTU;
    chan->remote_mtu = BT_L2CAP_DEFAULT_MTU;
    chan->outbound_config_done = false;
    chan->inbound_config_done = false;
    chan->on_event = on_event;
    chan->user_data = user_data;
    chan->pending_identifier = alloc_identifier(mgr);

    bt_timer_list_add(&mgr->timers, &chan->rtx_timer, now_us + BT_L2CAP_RTX_TIMEOUT_US);

    st = send_connection_request(mgr, chan);
    if (st != BT_OK)
    {
        /* Couldn't even get the request out -- fail cleanly rather than
         * leaving a half-open slot behind. chan may already have been
         * reused by the time we get here if the transport called back
         * into us synchronously, so this only touches it when it's still
         * safe to (state check would be needed for full rigor; in
         * practice a send failure here is itself synchronous and never
         * recurses, since nothing was sent). */
        bt_timer_list_cancel(&mgr->timers, &chan->rtx_timer);
        chan->state = BT_L2CAP_CHAN_FREE;
        return st;
    }

    *out_local_cid = local_cid;
    return BT_OK;
}

bt_status_t bt_l2cap_channel_manager_open_fixed(struct bt_l2cap_channel_manager *mgr, uint16_t cid,
                                                 bt_l2cap_channel_event_fn on_event,
                                                 void *user_data)
{
    struct bt_l2cap_channel *chan;

    if (find_channel_by_local_cid(mgr, cid) != NULL)
        return BT_ERR_INVALID_ARGUMENT; /* already registered */

    chan = find_free_channel(mgr);
    if (chan == NULL)
        return BT_ERR_NO_RESOURCES;

    chan->is_fixed = true;
    chan->psm = 0;
    chan->local_cid = cid;
    chan->remote_cid = cid;
    chan->local_mtu = BT_L2CAP_DEFAULT_MTU;
    chan->remote_mtu = BT_L2CAP_DEFAULT_MTU;
    chan->outbound_config_done = true;
    chan->inbound_config_done = true;
    chan->pending_identifier = 0;
    chan->on_event = on_event;
    chan->user_data = user_data;
    chan->state = BT_L2CAP_CHAN_OPEN;

    fire_opened(chan);
    return BT_OK;
}

void bt_l2cap_channel_manager_close(struct bt_l2cap_channel_manager *mgr, uint16_t local_cid,
                                     uint64_t now_us)
{
    struct bt_l2cap_channel *chan = find_channel_by_local_cid(mgr, local_cid);
    uint8_t buf[BT_L2CAP_SIG_HEADER_LEN + 4];
    struct bt_buf_writer w;

    if (chan == NULL)
        return;

    if (chan->state == BT_L2CAP_CHAN_OPEN && !chan->is_fixed)
    {
        chan->pending_identifier = alloc_identifier(mgr);
        chan->state = BT_L2CAP_CHAN_WAIT_DISCONNECT_RSP;
        bt_timer_list_add(&mgr->timers, &chan->rtx_timer, now_us + BT_L2CAP_RTX_TIMEOUT_US);

        bt_buf_writer_init(&w, buf, sizeof(buf));
        bt_l2cap_sig_encode_disconnection_request(&w, chan->pending_identifier, chan->remote_cid,
                                                   chan->local_cid);
        send_l2cap_pdu(mgr, mgr->signaling_cid, buf, bt_buf_writer_len(&w));
        return;
    }

    /* Still connecting/configuring, or already tearing down: nothing
     * meaningful to say on the wire for the former (project.md's "remoção
     * durante negociação"); for the latter, this just force-finishes it. */
    finish_close(chan, BT_L2CAP_CLOSE_LOCAL);
}

bt_status_t bt_l2cap_channel_manager_send(struct bt_l2cap_channel_manager *mgr, uint16_t local_cid,
                                           const uint8_t *data, size_t len, uint64_t now_us)
{
    struct bt_l2cap_channel *chan;
    uint8_t pdu[BT_L2CAP_HEADER_LEN + BT_L2CAP_MAX_SEND_LEN];
    struct bt_buf_writer w;
    bt_status_t st;

    (void)now_us;

    if (len > BT_L2CAP_MAX_SEND_LEN)
        return BT_ERR_INVALID_ARGUMENT;

    chan = find_channel_by_local_cid(mgr, local_cid);
    if (chan == NULL || chan->state != BT_L2CAP_CHAN_OPEN)
        return BT_ERR_INVALID_ARGUMENT;

    bt_buf_writer_init(&w, pdu, sizeof(pdu));
    st = bt_l2cap_encode_header(&w, (uint16_t)len, chan->remote_cid);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_bytes(&w, data, len);
    if (st != BT_OK)
        return st;

    return send_fragmented(mgr, pdu, bt_buf_writer_len(&w));
}

void bt_l2cap_channel_manager_on_acl(struct bt_l2cap_channel_manager *mgr, uint8_t pb_flag,
                                      const uint8_t *data, size_t len, uint64_t now_us)
{
    enum bt_l2cap_reassembly_result rr =
        bt_l2cap_reassembler_feed(&mgr->reassembler, pb_flag, data, len);
    const uint8_t *pdu;
    size_t pdu_len;
    struct bt_buf_reader r;
    struct bt_l2cap_header hdr;
    const uint8_t *payload;
    struct bt_l2cap_channel *chan;

    if (rr != BT_L2CAP_REASSEMBLY_COMPLETE)
        return; /* MORE: wait for the rest; ERROR: reassembler already reset itself */

    pdu = bt_l2cap_reassembler_take(&mgr->reassembler, &pdu_len);

    bt_buf_reader_init(&r, pdu, pdu_len);
    if (bt_l2cap_parse_header(&r, &hdr) != BT_OK)
        return;

    payload = bt_buf_reader_peek(&r, hdr.length);
    if (payload == NULL)
        return; /* declared length exceeds what reassembly delivered -- drop defensively */

    if (hdr.cid == mgr->signaling_cid)
    {
        process_signaling_pdu(mgr, payload, hdr.length, now_us);
        return;
    }

    chan = find_channel_by_local_cid(mgr, hdr.cid);
    if (chan != NULL && chan->state == BT_L2CAP_CHAN_OPEN)
    {
        struct bt_l2cap_channel_event_info info;

        info.event = BT_L2CAP_CHANNEL_EVENT_DATA;
        info.local_cid = chan->local_cid;
        info.close_reason = BT_L2CAP_CLOSE_LOCAL; /* unused for this event */
        info.data = payload;
        info.data_len = hdr.length;
        info.now_us = now_us;
        fire_event(chan, &info);
    }
    /* Unknown CID, or channel not OPEN yet: silently drop. */
}

void bt_l2cap_channel_manager_tick(struct bt_l2cap_channel_manager *mgr, uint64_t now_us)
{
    struct bt_timer *t;

    while ((t = bt_timer_list_pop_expired(&mgr->timers, now_us)) != NULL)
        t->callback(t, t->user_data);
}
