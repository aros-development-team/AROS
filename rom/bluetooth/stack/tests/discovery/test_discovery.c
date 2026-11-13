#include "test_discovery.h"
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

static void bring_up(struct bt_virtual_transport *vt, struct bt_controller *ctrl)
{
    bt_virtual_transport_init(vt);
    BT_CHECK(vt->base.ops->open(&vt->base) == 0);

    bt_controller_init(ctrl, &vt->base);
    BT_CHECK(vt->base.ops->start_receive(&vt->base, on_recv, ctrl) == 0);

    BT_CHECK(bt_controller_start(ctrl, 0) == BT_OK);
    BT_CHECK(ctrl->state == BT_CONTROLLER_STATE_READY);
}

static void test_classic_and_le_discovery_populate_unified_registry(void)
{
    struct bt_virtual_transport vt;
    struct bt_controller ctrl;
    const struct bt_discovered_device *classic_dev;
    const struct bt_discovered_device *le_dev;
    struct bt_addr classic_addr = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
    struct bt_addr le_addr = {{0x11, 0x12, 0x13, 0x14, 0x15, 0x16}};

    bring_up(&vt, &ctrl);

    BT_CHECK(bt_device_registry_count(&ctrl.devices) == 0);

    /* The virtual transport answers synchronously, so this single call
     * drives Command Status -> Inquiry Result -> Inquiry Complete, per
     * project.md's discovery acceptance criterion of a deterministic,
     * fully host-testable sequence. */
    BT_CHECK(bt_controller_start_classic_inquiry(&ctrl, 0x08, 0) == BT_OK);

    BT_CHECK(bt_device_registry_count(&ctrl.devices) == 1);
    classic_dev = bt_device_registry_find(&ctrl.devices, &classic_addr);
    BT_CHECK(classic_dev != NULL);
    BT_CHECK((classic_dev->flags & BT_DEVICE_FLAG_CLASSIC) != 0);
    BT_CHECK((classic_dev->flags & BT_DEVICE_FLAG_LE) == 0);
    BT_CHECK(classic_dev->class_of_device == 0x1F0104);

    /* LE Set Scan Parameters -> Command Complete, then LE Set Scan Enable
     * -> Command Complete followed by a simulated advertising report. */
    BT_CHECK(bt_controller_start_le_scan(&ctrl, 0) == BT_OK);

    BT_CHECK(bt_device_registry_count(&ctrl.devices) == 2);
    le_dev = bt_device_registry_find(&ctrl.devices, &le_addr);
    BT_CHECK(le_dev != NULL);
    BT_CHECK((le_dev->flags & BT_DEVICE_FLAG_LE) != 0);
    BT_CHECK((le_dev->flags & BT_DEVICE_FLAG_CLASSIC) == 0);
    BT_CHECK(le_dev->last_rssi == -55);

    /* Distinct addresses stay distinct entries -- not (incorrectly)
     * merged into a single dual-mode device. */
    BT_CHECK(!bt_addr_equal(&classic_dev->addr, &le_dev->addr));
}

static void test_discovery_requires_ready_state(void)
{
    struct bt_virtual_transport vt;
    struct bt_controller ctrl;

    bt_virtual_transport_init(&vt);
    BT_CHECK(vt.base.ops->open(&vt.base) == 0);
    bt_controller_init(&ctrl, &vt.base);

    /* Not READY yet (start() never called). */
    BT_CHECK(bt_controller_start_classic_inquiry(&ctrl, 0x08, 0) == BT_ERR_INVALID_ARGUMENT);
    BT_CHECK(bt_controller_start_le_scan(&ctrl, 0) == BT_ERR_INVALID_ARGUMENT);
}

void run_discovery_tests(void)
{
    test_classic_and_le_discovery_populate_unified_registry();
    test_discovery_requires_ready_state();
}
