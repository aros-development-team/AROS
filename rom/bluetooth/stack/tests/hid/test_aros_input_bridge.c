#include "test_aros_input_bridge.h"
#include "../support/test.h"

#include <btcore/aros_input_bridge.h>

struct bridge_log
{
    struct bt_aros_input_event events[16];
    size_t count;
};

static bt_status_t capture(void *context, const struct bt_aros_input_event *event)
{
    struct bridge_log *log = context;

    log->events[log->count++] = *event;
    return BT_OK;
}

static void feed(struct bt_aros_input_bridge *bridge,
                 enum bt_hid_input_event_kind kind, uint16_t usage,
                 int32_t value, bool relative)
{
    struct bt_hid_input_event event = {
        kind, 0, kind == BT_HID_INPUT_EVENT_KEY ? 0x07 : 0x01,
        usage, value, relative};

    BT_CHECK(bt_aros_input_bridge_handle(&event, bridge));
}

static void test_keyboard_and_qualifiers(void)
{
    struct bt_aros_input_bridge bridge;
    struct bridge_log log = {0};

    bt_aros_input_bridge_init(&bridge, capture, &log);
    feed(&bridge, BT_HID_INPUT_EVENT_KEY, 0xE1, 1, false);
    feed(&bridge, BT_HID_INPUT_EVENT_KEY, 0x04, 1, false);
    feed(&bridge, BT_HID_INPUT_EVENT_KEY, 0x04, 0, false);
    feed(&bridge, BT_HID_INPUT_EVENT_KEY, 0xE1, 0, false);

    BT_CHECK(log.count == 4);
    BT_CHECK(log.events[0].code == 0x60 && log.events[0].qualifier == 1);
    BT_CHECK(log.events[1].code == 0x20 && log.events[1].qualifier == 1);
    BT_CHECK(log.events[2].code == (0x20 | BT_AROS_IECODE_UP_PREFIX));
    BT_CHECK(log.events[3].code == (0x60 | BT_AROS_IECODE_UP_PREFIX));
    BT_CHECK(log.events[3].qualifier == 0);
}

static void test_mouse_and_consumer(void)
{
    struct bt_aros_input_bridge bridge;
    struct bridge_log log = {0};
    struct bt_hid_input_event event = {
        BT_HID_INPUT_EVENT_BUTTON, 0, 0x09, 1, 1, false};

    bt_aros_input_bridge_init(&bridge, capture, &log);
    BT_CHECK(bt_aros_input_bridge_handle(&event, &bridge));
    event.value = 0;
    BT_CHECK(bt_aros_input_bridge_handle(&event, &bridge));
    feed(&bridge, BT_HID_INPUT_EVENT_AXIS, 0x30, -4, true);
    feed(&bridge, BT_HID_INPUT_EVENT_AXIS, 0x31, 7, true);
    feed(&bridge, BT_HID_INPUT_EVENT_AXIS, 0x38, 1, true);

    BT_CHECK(log.count == 6);
    BT_CHECK(log.events[0].event_class == BT_AROS_IECLASS_RAWMOUSE);
    BT_CHECK(log.events[0].code == BT_AROS_IECODE_LBUTTON);
    BT_CHECK(log.events[1].code ==
             (BT_AROS_IECODE_LBUTTON | BT_AROS_IECODE_UP_PREFIX));
    BT_CHECK(log.events[2].x == -4 && log.events[2].y == 0);
    BT_CHECK(log.events[3].x == 0 && log.events[3].y == 7);
    BT_CHECK(log.events[4].code == 0x7A);
    BT_CHECK(log.events[5].code == (0x7A | BT_AROS_IECODE_UP_PREFIX));

    log.count = 0;
    event.kind = BT_HID_INPUT_EVENT_CONSUMER;
    event.usage_page = 0x0C;
    event.usage = 0xCD;
    event.value = 1;
    BT_CHECK(bt_aros_input_bridge_handle(&event, &bridge));
    BT_CHECK(log.events[0].event_class == BT_AROS_IECLASS_RAWKEY);
    BT_CHECK(log.events[0].code == 0x73);
}

void run_aros_input_bridge_tests(void)
{
    test_keyboard_and_qualifiers();
    test_mouse_and_consumer();
}
