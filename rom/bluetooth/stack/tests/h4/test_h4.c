#include "test_h4.h"
#include "../support/test.h"

#include <btcore/h4.h>

#include <string.h>

struct capture
{
    unsigned int count;
    enum bt_hci_packet_type type;
    uint8_t data[16];
    size_t length;
};

static void capture_packet(enum bt_hci_packet_type type, const uint8_t *data,
                           size_t length, void *user_data)
{
    struct capture *capture = user_data;

    capture->count++;
    capture->type = type;
    capture->length = length;
    if (length <= sizeof(capture->data))
        memcpy(capture->data, data, length);
}

static void test_fragmented_event(void)
{
    struct bt_h4_rx rx;
    struct capture capture = {0};
    const uint8_t first[] = {0x04, 0x0e};
    const uint8_t second[] = {0x04, 0x01, 0x03, 0x0c, 0x00};
    const uint8_t expected[] = {0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00};

    bt_h4_rx_init(&rx);
    BT_CHECK(bt_h4_rx_feed(&rx, first, sizeof(first), capture_packet,
                           &capture) == BT_OK);
    BT_CHECK(capture.count == 0);
    BT_CHECK(bt_h4_rx_feed(&rx, second, sizeof(second), capture_packet,
                           &capture) == BT_OK);
    BT_CHECK(capture.count == 1);
    BT_CHECK(capture.type == BT_HCI_PACKET_EVENT);
    BT_CHECK(capture.length == sizeof(expected));
    BT_CHECK(memcmp(capture.data, expected, sizeof(expected)) == 0);
}

static void test_multiple_packets(void)
{
    struct bt_h4_rx rx;
    struct capture capture = {0};
    const uint8_t bytes[] = {
        0x04, 0x0f, 0x00,
        0x02, 0x01, 0x00, 0x01, 0x00, 0xaa
    };

    bt_h4_rx_init(&rx);
    BT_CHECK(bt_h4_rx_feed(&rx, bytes, sizeof(bytes), capture_packet,
                           &capture) == BT_OK);
    BT_CHECK(capture.count == 2);
    BT_CHECK(capture.type == BT_HCI_PACKET_ACL);
    BT_CHECK(capture.length == 5);
    BT_CHECK(capture.data[4] == 0xaa);
}

static void test_invalid_type_resets(void)
{
    struct bt_h4_rx rx;
    struct capture capture = {0};
    const uint8_t invalid[] = {0x99};
    const uint8_t valid[] = {0x04, 0x0f, 0x00};

    bt_h4_rx_init(&rx);
    BT_CHECK(bt_h4_rx_feed(&rx, invalid, sizeof(invalid), capture_packet,
                           &capture) == BT_ERR_IO);
    BT_CHECK(bt_h4_rx_feed(&rx, valid, sizeof(valid), capture_packet,
                           &capture) == BT_OK);
    BT_CHECK(capture.count == 1);
}

void run_h4_tests(void)
{
    test_fragmented_event();
    test_multiple_packets();
    test_invalid_type_resets();
}
