#include "test_controller.h"
#include "../support/test.h"

#include <btcore/controller.h>
#include <btcore/transport.h>
#include <virtual_transport/virtual_transport.h>

static void on_recv(struct bt_hci_transport *transport, enum bt_hci_packet_type type,
                     const uint8_t *data, size_t length, void *user_data)
{
    struct bt_controller *ctrl = (struct bt_controller *)user_data;

    (void)transport;
    if (type == BT_HCI_PACKET_EVENT)
        bt_controller_on_event(ctrl, data, length, 0);
}

static void test_init_sequence_reaches_ready(void)
{
    struct bt_virtual_transport vt;
    struct bt_controller ctrl;

    bt_virtual_transport_init(&vt);
    BT_CHECK(vt.base.ops->open(&vt.base) == 0);

    bt_controller_init(&ctrl, &vt.base);
    BT_CHECK(vt.base.ops->start_receive(&vt.base, on_recv, &ctrl) == 0);

    BT_CHECK(bt_controller_start(&ctrl, 0) == BT_OK);

    /* The virtual transport answers synchronously, so a single
     * bt_controller_start() call drives Reset -> Read Local Version ->
     * Read Local Supported Features -> Read Buffer Size -> READY without
     * needing bt_controller_tick() at all -- this is project.md's
     * "sequência determinística de inicialização" acceptance criterion. */
    BT_CHECK(ctrl.state == BT_CONTROLLER_STATE_READY);
    BT_CHECK(vt.reset_done);

    BT_CHECK(ctrl.info.version.status == 0x00);
    BT_CHECK(ctrl.info.version.hci_version == 0x0c);
    BT_CHECK(ctrl.info.version.manufacturer_name == 0xffff);

    BT_CHECK(ctrl.info.features.status == 0x00);
    BT_CHECK(ctrl.info.features.features[0] == 0xff);

    BT_CHECK(ctrl.info.buffer_size.status == 0x00);
    BT_CHECK(ctrl.info.buffer_size.acl_data_packet_length == 200);
    BT_CHECK(ctrl.info.buffer_size.total_num_acl_data_packets == 8);
}

static void test_start_twice_rejected(void)
{
    struct bt_virtual_transport vt;
    struct bt_controller ctrl;

    bt_virtual_transport_init(&vt);
    BT_CHECK(vt.base.ops->open(&vt.base) == 0);

    bt_controller_init(&ctrl, &vt.base);
    BT_CHECK(vt.base.ops->start_receive(&vt.base, on_recv, &ctrl) == 0);

    BT_CHECK(bt_controller_start(&ctrl, 0) == BT_OK);
    BT_CHECK(ctrl.state == BT_CONTROLLER_STATE_READY);
    BT_CHECK(bt_controller_start(&ctrl, 0) == BT_ERR_INVALID_ARGUMENT);
}

/* A transport that accepts every send_command() but never answers, to
 * exercise the bring-up timeout path. */
struct silent_transport
{
    struct bt_hci_transport base;
};

static int silent_open(struct bt_hci_transport *t)
{
    (void)t;
    return 0;
}

static void silent_close(struct bt_hci_transport *t)
{
    (void)t;
}

static int silent_send(struct bt_hci_transport *t, const uint8_t *data, size_t length)
{
    (void)t;
    (void)data;
    (void)length;
    return 0;
}

static int silent_send_unsupported(struct bt_hci_transport *t, const uint8_t *data, size_t length)
{
    (void)t;
    (void)data;
    (void)length;
    return -1;
}

static int silent_start_receive(struct bt_hci_transport *t, bt_hci_transport_recv_fn recv,
                                 void *user_data)
{
    (void)t;
    (void)recv;
    (void)user_data;
    return 0;
}

static void silent_stop_receive(struct bt_hci_transport *t)
{
    (void)t;
}

static const struct bt_hci_transport_ops silent_ops = {
    .open = silent_open,
    .close = silent_close,
    .send_command = silent_send,
    .send_acl = silent_send_unsupported,
    .send_sco = silent_send_unsupported,
    .send_iso = silent_send_unsupported,
    .start_receive = silent_start_receive,
    .stop_receive = silent_stop_receive,
};

static void test_timeout_during_bringup(void)
{
    struct silent_transport st;
    struct bt_controller ctrl;

    st.base.ops = &silent_ops;
    st.base.impl = &st;

    BT_CHECK(st.base.ops->open(&st.base) == 0);
    bt_controller_init(&ctrl, &st.base);

    BT_CHECK(bt_controller_start(&ctrl, 0) == BT_OK);
    BT_CHECK(ctrl.state == BT_CONTROLLER_STATE_RESETTING);

    /* No response ever arrives; ticking past the default timeout must move
     * to ERROR instead of hanging forever. */
    bt_controller_tick(&ctrl, BT_CMDQ_DEFAULT_TIMEOUT_US + 1);
    BT_CHECK(ctrl.state == BT_CONTROLLER_STATE_ERROR);
}

void run_controller_tests(void)
{
    test_init_sequence_reaches_ready();
    test_start_twice_rejected();
    test_timeout_during_bringup();
}
