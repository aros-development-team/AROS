#include "test_virtual_transport.h"
#include "../support/test.h"

#include <btcore/buffer.h>
#include <btcore/hci.h>
#include <btcore/transport.h>
#include <virtual_transport/virtual_transport.h>

/*
 * Demonstrates the sequence required by project.md's "Primeira entrega de
 * código":
 *
 *   HCI Reset Command -> transporte virtual -> Command Complete Event
 *   -> controller state = initialized
 *
 * The full controller state machine belongs to core/controller/ (Fase 2);
 * here "state" is just a flag local to the test, enough to prove the
 * round trip end to end. Nothing in this path branches on host byte order
 * -- bt_hci_encode_command/bt_hci_parse_* always go through the LE/BE
 * helpers in bluetooth/endian.h -- so the same assertions hold on a
 * little-endian or a big-endian host alike (see the fixed wire-byte
 * vectors in tests/endian and tests/hci, which pin this independently of
 * whatever CPU the test happens to run on).
 */

enum controller_state
{
    CONTROLLER_STATE_UNINITIALIZED,
    CONTROLLER_STATE_INITIALIZED
};

struct recv_context
{
    enum controller_state state;
    bool got_event;
    uint8_t status;
};

static void on_recv(struct bt_hci_transport *transport, enum bt_hci_packet_type type,
                     const uint8_t *data, size_t length, void *user_data)
{
    struct recv_context *ctx = (struct recv_context *)user_data;
    struct bt_buf_reader r;
    struct bt_hci_event_header hdr;
    struct bt_hci_command_complete cc;

    (void)transport;

    if (type != BT_HCI_PACKET_EVENT)
        return;

    bt_buf_reader_init(&r, data, length);
    if (bt_hci_parse_event_header(&r, &hdr) != BT_OK)
        return;
    if (hdr.event_code != BT_HCI_EVENT_COMMAND_COMPLETE)
        return;
    if (bt_hci_parse_command_complete(&r, hdr.param_len, &cc) != BT_OK)
        return;
    if (cc.command_opcode != BT_HCI_OPCODE_RESET || cc.return_params_len < 1)
        return;

    ctx->got_event = true;
    ctx->status = cc.return_params[0];
    if (ctx->status == 0x00)
        ctx->state = CONTROLLER_STATE_INITIALIZED;
}

static void test_hci_reset_sequence(void)
{
    struct bt_virtual_transport vt;
    struct recv_context ctx = {CONTROLLER_STATE_UNINITIALIZED, false, 0xff};
    struct bt_buf_writer w;
    uint8_t cmd[BT_HCI_COMMAND_HEADER_LEN];

    bt_virtual_transport_init(&vt);
    BT_CHECK(vt.base.ops->open(&vt.base) == 0);
    BT_CHECK(vt.base.ops->start_receive(&vt.base, on_recv, &ctx) == 0);

    bt_buf_writer_init(&w, cmd, sizeof(cmd));
    BT_CHECK(bt_hci_encode_command(&w, BT_HCI_OPCODE_RESET, NULL, 0) == BT_OK);

    BT_CHECK(ctx.state == CONTROLLER_STATE_UNINITIALIZED);

    BT_CHECK(vt.base.ops->send_command(&vt.base, cmd, bt_buf_writer_len(&w)) == 0);

    /* send_command() delivers the event synchronously, so by the time it
     * returns the sequence is already complete. */
    BT_CHECK(ctx.got_event);
    BT_CHECK(ctx.status == 0x00);
    BT_CHECK(ctx.state == CONTROLLER_STATE_INITIALIZED);
    BT_CHECK(vt.reset_done);

    vt.base.ops->stop_receive(&vt.base);
    vt.base.ops->close(&vt.base);
}

void run_virtual_transport_tests(void)
{
    test_hci_reset_sequence();
}
