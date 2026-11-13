#ifndef BTCORE_H4_H
#define BTCORE_H4_H

#include <btcore/status.h>
#include <btcore/transport.h>

#define BT_H4_MAX_PACKET_SIZE 4100u

typedef void (*bt_h4_packet_fn)(enum bt_hci_packet_type type,
                                const uint8_t *data, size_t length,
                                void *user_data);

struct bt_h4_rx
{
    uint8_t packet[BT_H4_MAX_PACKET_SIZE];
    size_t length;
    size_t expected;
    size_t header_length;
    enum bt_hci_packet_type type;
    bool have_type;
};

void bt_h4_rx_init(struct bt_h4_rx *rx);
bt_status_t bt_h4_rx_feed(struct bt_h4_rx *rx, const uint8_t *data,
                          size_t length, bt_h4_packet_fn packet,
                          void *user_data);
uint8_t bt_h4_wire_type(enum bt_hci_packet_type type);

#endif /* BTCORE_H4_H */
