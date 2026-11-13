#include "test_listener_pair.h"
#include "../support/test.h"
#include <btcore/l2cap_channel.h>
#include <string.h>

static int send_acl(struct bt_hci_transport *transport, const uint8_t *data, size_t len)
{
    (void)transport; (void)data; (void)len;
    return 0;
}
static const struct bt_hci_transport_ops ops = {.send_acl = send_acl};
struct endpoint { unsigned opened, received; };
static void event(struct bt_l2cap_channel_event_info *info, void *context)
{
    struct endpoint *ep = context;
    if(info->event == BT_L2CAP_CHANNEL_EVENT_OPENED) ep->opened++;
    if(info->event == BT_L2CAP_CHANNEL_EVENT_DATA) ep->received++;
}
static void feed(struct bt_l2cap_channel_manager *mgr, uint16_t cid, const uint8_t *data, size_t len)
{
    uint8_t packet[128];
    struct bt_buf_writer w;
    bt_buf_writer_init(&w, packet, sizeof(packet));
    bt_l2cap_encode_header(&w, len, cid);
    bt_buf_writer_write_bytes(&w, data, len);
    bt_l2cap_channel_manager_on_acl(mgr, 0, packet, bt_buf_writer_len(&w), 100);
}
static void connect_peer(struct bt_l2cap_channel_manager *mgr, uint16_t remote)
{
    uint8_t packet[32]; struct bt_buf_writer w;
    bt_buf_writer_init(&w, packet, sizeof(packet));
    bt_l2cap_sig_encode_connection_request(&w, 1, 0x19, remote);
    feed(mgr, mgr->signaling_cid, packet, bt_buf_writer_len(&w));
}
static void configure(struct bt_l2cap_channel_manager *mgr, struct bt_l2cap_channel *ch)
{
    uint8_t packet[32]; struct bt_buf_writer w;
    bt_buf_writer_init(&w, packet, sizeof(packet));
    bt_l2cap_sig_encode_configure_response(&w, ch->pending_identifier,
        ch->remote_cid, 0, BT_L2CAP_CONFIG_RESULT_SUCCESS, 0);
    feed(mgr, mgr->signaling_cid, packet, bt_buf_writer_len(&w));
    bt_buf_writer_init(&w, packet, sizeof(packet));
    bt_l2cap_sig_encode_configure_request(&w, 2, ch->local_cid, 0, 672);
    feed(mgr, mgr->signaling_cid, packet, bt_buf_writer_len(&w));
}
void run_l2cap_listener_pair_tests(void)
{
    struct bt_l2cap_channel_manager mgr;
    struct bt_hci_transport transport = {.ops = &ops};
    struct endpoint signal = {0}, media = {0};
    uint16_t signal_cid;
    uint8_t sample = 42;
    unsigned i, active = 0;
    bt_l2cap_channel_manager_init(&mgr, &transport, 1, BT_L2CAP_CID_SIGNALING_CLASSIC, 1024);
    BT_CHECK(bt_l2cap_channel_manager_listen_pair(&mgr, 0x19, 672, event, &signal, &media) == BT_OK);
    connect_peer(&mgr, 0x60);
    BT_CHECK(mgr.channels[0].user_data == &signal);
    signal_cid = mgr.channels[0].local_cid;
    /* A second request while the first is still configuring must also be
       assigned to media, never overwrite the signalling endpoint. */
    connect_peer(&mgr, 0x61);
    BT_CHECK(mgr.channels[1].user_data == &media);
    BT_CHECK(mgr.channels[0].local_cid == signal_cid);
    configure(&mgr, &mgr.channels[0]);
    configure(&mgr, &mgr.channels[1]);
    BT_CHECK(signal.opened == 1 && media.opened == 1);
    feed(&mgr, signal_cid, &sample, 1);
    BT_CHECK(signal.received == 1 && media.received == 0);
    feed(&mgr, mgr.channels[1].local_cid, &sample, 1);
    BT_CHECK(signal.received == 1 && media.received == 1);
    connect_peer(&mgr, 0x62);
    for(i = 0; i < BT_L2CAP_CHANNEL_MANAGER_MAX_CHANNELS; i++)
        if(mgr.channels[i].state != BT_L2CAP_CHAN_FREE) active++;
    BT_CHECK(active == 2);
    bt_l2cap_channel_manager_detach(&mgr, &media);
    BT_CHECK(mgr.channels[0].user_data == &signal);
    BT_CHECK(mgr.channels[1].user_data == NULL);
    BT_CHECK(mgr.listeners[0].psm == 0);
    /* Re-enumeration restores the listener for surviving endpoints. */
    BT_CHECK(bt_l2cap_channel_manager_listen_pair(&mgr, 0x19, 672, event, &signal, &media) == BT_OK);
    connect_peer(&mgr, 0x63);
    BT_CHECK(mgr.channels[2].user_data == &media);
    BT_CHECK(mgr.channels[0].local_cid == signal_cid);
}
