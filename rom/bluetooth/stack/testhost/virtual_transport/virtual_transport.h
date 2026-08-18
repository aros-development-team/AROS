#ifndef BT_TEST_HOST_VIRTUAL_TRANSPORT_H
#define BT_TEST_HOST_VIRTUAL_TRANSPORT_H

#include <btcore/transport.h>
#include <btcore/types.h>

/*
 * A fake HCI controller for host-side testing (project.md, "controlador
 * virtual para testes"). Unlike a real transport, it interprets HCI itself
 * in order to fabricate believable controller responses -- that's expected
 * here and nowhere else in the transport layer.
 *
 * Responses are delivered synchronously, inline within send_command(): by
 * the time it returns, any registered recv callback has already fired.
 * Only HCI Reset is modeled so far.
 */
struct bt_virtual_transport
{
    struct bt_hci_transport base;
    bt_hci_transport_recv_fn recv;
    void *recv_user_data;
    bool is_open;
    bool reset_done;
};

void bt_virtual_transport_init(struct bt_virtual_transport *vt);

#endif /* BT_TEST_HOST_VIRTUAL_TRANSPORT_H */
