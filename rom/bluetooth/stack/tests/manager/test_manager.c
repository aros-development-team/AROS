#include "test_manager.h"
#include "../support/test.h"

#include <btcore/manager.h>
#include <virtual_transport/virtual_transport.h>

static void test_manager_owns_transport_lifecycle(void)
{
    struct bt_virtual_transport transport;
    struct bt_manager manager;

    bt_virtual_transport_init(&transport);
    bt_manager_init(&manager, &transport.base);
    BT_CHECK(manager.state == BT_MANAGER_STATE_STOPPED);
    BT_CHECK(bt_manager_start(&manager, 100) == BT_OK);
    BT_CHECK(manager.state == BT_MANAGER_STATE_RUNNING);
    BT_CHECK(manager.controller.state == BT_CONTROLLER_STATE_READY);
    BT_CHECK(transport.is_open);
    BT_CHECK(bt_manager_start(&manager, 100) == BT_ERR_INVALID_ARGUMENT);

    bt_manager_stop(&manager);
    BT_CHECK(manager.state == BT_MANAGER_STATE_STOPPED);
    BT_CHECK(!transport.is_open);
    BT_CHECK(manager.controller.state == BT_CONTROLLER_STATE_UNINITIALIZED);
}

static void test_manager_rejects_missing_transport(void)
{
    struct bt_manager manager;

    bt_manager_init(&manager, NULL);
    BT_CHECK(bt_manager_start(&manager, 0) == BT_ERR_INVALID_ARGUMENT);
}

void run_manager_tests(void)
{
    test_manager_owns_transport_lifecycle();
    test_manager_rejects_missing_transport();
}
