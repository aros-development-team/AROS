#include "test_hid_input.h"
#include "../support/test.h"

#include <btcore/hid_input.h>

struct event_log
{
    struct bt_hid_input_event events[32];
    size_t count;
};

static bool log_event(const struct bt_hid_input_event *event, void *user_data)
{
    struct event_log *log = user_data;

    log->events[log->count++] = *event;
    return true;
}

static void test_keyboard_array_press_release(void)
{
    static const uint8_t descriptor[] = {
        0x05, 0x07, 0x19, 0x00, 0x29, 0x65,
        0x15, 0x00, 0x25, 0x65, 0x75, 0x08,
        0x95, 0x02, 0x81, 0x00};
    const uint8_t first[] = {0x04, 0x05};
    const uint8_t second[] = {0x05, 0x06};
    const uint8_t empty[] = {0, 0};
    struct bt_hid_report_descriptor parsed;
    struct bt_hid_input input;
    struct event_log log = {0};

    BT_CHECK(bt_hid_report_parse(descriptor, sizeof(descriptor), &parsed) == BT_OK);
    bt_hid_input_init(&input, &parsed);
    BT_CHECK(bt_hid_input_process(&input, first, sizeof(first), log_event, &log) == BT_OK);
    BT_CHECK(log.count == 2);
    BT_CHECK(log.events[0].kind == BT_HID_INPUT_EVENT_KEY);
    BT_CHECK(log.events[0].usage == 4 && log.events[0].value == 1);
    BT_CHECK(log.events[1].usage == 5 && log.events[1].value == 1);

    log.count = 0;
    BT_CHECK(bt_hid_input_process(&input, second, sizeof(second), log_event, &log) == BT_OK);
    BT_CHECK(log.count == 2);
    BT_CHECK(log.events[0].usage == 4 && log.events[0].value == 0);
    BT_CHECK(log.events[1].usage == 6 && log.events[1].value == 1);

    log.count = 0;
    BT_CHECK(bt_hid_input_process(&input, empty, sizeof(empty), log_event, &log) == BT_OK);
    BT_CHECK(log.count == 2);
    BT_CHECK(log.events[0].value == 0 && log.events[1].value == 0);
}

static void test_buttons_and_relative_axes(void)
{
    static const uint8_t descriptor[] = {
        0x05, 0x09, 0x09, 0x01, 0x15, 0x00, 0x25, 0x01,
        0x75, 0x01, 0x95, 0x01, 0x81, 0x02,
        0x75, 0x07, 0x95, 0x01, 0x81, 0x01,
        0x05, 0x01, 0x09, 0x30, 0x15, 0x81, 0x25, 0x7F,
        0x75, 0x08, 0x95, 0x01, 0x81, 0x06};
    const uint8_t first[] = {1, 0xFE};
    const uint8_t same_button[] = {1, 0};
    struct bt_hid_report_descriptor parsed;
    struct bt_hid_input input;
    struct event_log log = {0};

    BT_CHECK(bt_hid_report_parse(descriptor, sizeof(descriptor), &parsed) == BT_OK);
    bt_hid_input_init(&input, &parsed);
    BT_CHECK(bt_hid_input_process(&input, first, sizeof(first), log_event, &log) == BT_OK);
    BT_CHECK(log.count == 2);
    BT_CHECK(log.events[0].kind == BT_HID_INPUT_EVENT_BUTTON);
    BT_CHECK(log.events[0].value == 1 && !log.events[0].relative);
    BT_CHECK(log.events[1].kind == BT_HID_INPUT_EVENT_AXIS);
    BT_CHECK(log.events[1].value == -2 && log.events[1].relative);

    log.count = 0;
    BT_CHECK(bt_hid_input_process(&input, same_button, sizeof(same_button),
                                   log_event, &log) == BT_OK);
    BT_CHECK(log.count == 0);
}

void run_hid_input_tests(void)
{
    test_keyboard_array_press_release();
    test_buttons_and_relative_axes();
}
