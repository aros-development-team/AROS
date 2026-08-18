#include <btcore/manager.h>

#include <string.h>

static void manager_receive(struct bt_hci_transport *transport,
                            enum bt_hci_packet_type type,
                            const uint8_t *data, size_t length,
                            void *user_data)
{
    struct bt_manager *manager = user_data;

    (void)transport;
    if (manager->state == BT_MANAGER_STATE_STOPPED ||
        manager->state == BT_MANAGER_STATE_ERROR)
        return;
    if (type == BT_HCI_PACKET_EVENT)
        bt_controller_on_event(&manager->controller, data, length,
                               manager->controller.cmdq.last_tick_us);
}

void bt_manager_init(struct bt_manager *manager,
                     struct bt_hci_transport *transport)
{
    if (manager == NULL)
        return;
    memset(manager, 0, sizeof(*manager));
    manager->transport = transport;
    manager->state = BT_MANAGER_STATE_STOPPED;
    if (transport != NULL)
        bt_controller_init(&manager->controller, transport);
}

bt_status_t bt_manager_start(struct bt_manager *manager, uint64_t now_us)
{
    bt_status_t status;

    if (manager == NULL || manager->transport == NULL ||
        manager->transport->ops == NULL ||
        manager->state != BT_MANAGER_STATE_STOPPED)
        return BT_ERR_INVALID_ARGUMENT;

    manager->state = BT_MANAGER_STATE_STARTING;
    if (manager->transport->ops->open(manager->transport) != 0)
        goto error;
    if (manager->transport->ops->start_receive(
            manager->transport, manager_receive, manager) != 0)
    {
        manager->transport->ops->close(manager->transport);
        goto error;
    }
    status = bt_controller_start(&manager->controller, now_us);
    if (status != BT_OK)
    {
        manager->transport->ops->stop_receive(manager->transport);
        manager->transport->ops->close(manager->transport);
        manager->state = BT_MANAGER_STATE_ERROR;
        return status;
    }
    manager->state = manager->controller.state == BT_CONTROLLER_STATE_ERROR
                         ? BT_MANAGER_STATE_ERROR
                         : BT_MANAGER_STATE_RUNNING;
    return manager->state == BT_MANAGER_STATE_RUNNING ? BT_OK : BT_ERR_IO;

error:
    manager->state = BT_MANAGER_STATE_ERROR;
    return BT_ERR_IO;
}

void bt_manager_tick(struct bt_manager *manager, uint64_t now_us)
{
    if (manager == NULL || manager->state != BT_MANAGER_STATE_RUNNING)
        return;
    bt_controller_tick(&manager->controller, now_us);
    if (manager->controller.state == BT_CONTROLLER_STATE_ERROR)
        manager->state = BT_MANAGER_STATE_ERROR;
}

void bt_manager_stop(struct bt_manager *manager)
{
    if (manager == NULL || manager->transport == NULL ||
        manager->state == BT_MANAGER_STATE_STOPPED)
        return;
    manager->transport->ops->stop_receive(manager->transport);
    manager->transport->ops->close(manager->transport);
    bt_controller_init(&manager->controller, manager->transport);
    manager->state = BT_MANAGER_STATE_STOPPED;
}
