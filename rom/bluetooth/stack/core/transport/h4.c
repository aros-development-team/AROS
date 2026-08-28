#include <btcore/h4.h>

#include <string.h>

static void reset_packet(struct bt_h4_rx *rx)
{
    rx->length = 0;
    rx->expected = 0;
    rx->header_length = 0;
    rx->have_type = false;
}

void bt_h4_rx_init(struct bt_h4_rx *rx)
{
    if (rx == NULL)
        return;
    memset(rx, 0, sizeof(*rx));
}

uint8_t bt_h4_wire_type(enum bt_hci_packet_type type)
{
    switch (type)
    {
    case BT_HCI_PACKET_COMMAND: return 0x01;
    case BT_HCI_PACKET_ACL: return 0x02;
    case BT_HCI_PACKET_SCO: return 0x03;
    case BT_HCI_PACKET_EVENT: return 0x04;
    case BT_HCI_PACKET_ISO: return 0x05;
    default: return 0;
    }
}

static bool begin_packet(struct bt_h4_rx *rx, uint8_t wire_type)
{
    switch (wire_type)
    {
    case 0x02:
        rx->type = BT_HCI_PACKET_ACL;
        rx->header_length = 4;
        break;
    case 0x03:
        rx->type = BT_HCI_PACKET_SCO;
        rx->header_length = 3;
        break;
    case 0x04:
        rx->type = BT_HCI_PACKET_EVENT;
        rx->header_length = 2;
        break;
    case 0x05:
        rx->type = BT_HCI_PACKET_ISO;
        rx->header_length = 4;
        break;
    default:
        return false;
    }
    rx->have_type = true;
    rx->length = 0;
    rx->expected = rx->header_length;
    return true;
}

static size_t payload_length(const struct bt_h4_rx *rx)
{
    switch (rx->type)
    {
    case BT_HCI_PACKET_EVENT:
        return rx->packet[1];
    case BT_HCI_PACKET_ACL:
        return (size_t)rx->packet[2] | ((size_t)rx->packet[3] << 8);
    case BT_HCI_PACKET_SCO:
        return rx->packet[2];
    case BT_HCI_PACKET_ISO:
        return ((size_t)rx->packet[2] | ((size_t)rx->packet[3] << 8)) & 0x3fff;
    default:
        return BT_H4_MAX_PACKET_SIZE;
    }
}

bt_status_t bt_h4_rx_feed(struct bt_h4_rx *rx, const uint8_t *data,
                          size_t length, bt_h4_packet_fn packet,
                          void *user_data)
{
    size_t offset = 0;

    if (rx == NULL || packet == NULL || (data == NULL && length != 0))
        return BT_ERR_INVALID_ARGUMENT;
    while (offset < length)
    {
        size_t available;
        size_t needed;
        size_t take;

        if (!rx->have_type)
        {
            if (!begin_packet(rx, data[offset++]))
            {
                reset_packet(rx);
                return BT_ERR_IO;
            }
            continue;
        }

        available = length - offset;
        needed = rx->expected - rx->length;
        take = available < needed ? available : needed;
        memcpy(rx->packet + rx->length, data + offset, take);
        rx->length += take;
        offset += take;
        if (rx->length != rx->expected)
            continue;

        if (rx->expected == rx->header_length)
        {
            size_t payload = payload_length(rx);

            if (payload > BT_H4_MAX_PACKET_SIZE - rx->header_length)
            {
                reset_packet(rx);
                return BT_ERR_BUFFER_OVERFLOW;
            }
            rx->expected = rx->header_length + payload;
            if (payload != 0)
                continue;
        }

        packet(rx->type, rx->packet, rx->expected, user_data);
        reset_packet(rx);
    }
    return BT_OK;
}
