#ifndef BTCORE_TRANSPORT_H
#define BTCORE_TRANSPORT_H

#include <btcore/types.h>

/* project.md, "Interface de transporte HCI": a transport moves raw HCI
 * bytes between the controller and the core. It must never convert
 * endianness or interpret protocols above HCI. */

enum bt_hci_packet_type
{
    BT_HCI_PACKET_COMMAND,
    BT_HCI_PACKET_EVENT,
    BT_HCI_PACKET_ACL,
    BT_HCI_PACKET_SCO,
    BT_HCI_PACKET_ISO
};

struct bt_hci_transport;

/* Invoked by a transport implementation when a packet arrives from the
 * controller. data/length cover only that packet's payload bytes (no
 * transport-level framing). The transport instance itself identifies which
 * adapter the packet came from. */
typedef void (*bt_hci_transport_recv_fn)(struct bt_hci_transport *transport,
                                          enum bt_hci_packet_type type,
                                          const uint8_t *data, size_t length,
                                          void *user_data);

struct bt_hci_transport_ops
{
    int (*open)(struct bt_hci_transport *transport);
    void (*close)(struct bt_hci_transport *transport);

    int (*send_command)(struct bt_hci_transport *transport, const uint8_t *data, size_t length);
    int (*send_acl)(struct bt_hci_transport *transport, const uint8_t *data, size_t length);
    int (*send_sco)(struct bt_hci_transport *transport, const uint8_t *data, size_t length);
    int (*send_iso)(struct bt_hci_transport *transport, const uint8_t *data, size_t length);

    int (*start_receive)(struct bt_hci_transport *transport, bt_hci_transport_recv_fn recv,
                          void *user_data);
    void (*stop_receive)(struct bt_hci_transport *transport);
};

struct bt_hci_transport
{
    const struct bt_hci_transport_ops *ops;
    void *impl; /* transport-specific state */
};

#endif /* BTCORE_TRANSPORT_H */
