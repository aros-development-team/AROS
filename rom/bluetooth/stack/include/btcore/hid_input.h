#ifndef BTCORE_HID_INPUT_H
#define BTCORE_HID_INPUT_H

#include <btcore/hid_report.h>

#ifndef BT_HID_INPUT_MAX_VARIABLES
#define BT_HID_INPUT_MAX_VARIABLES 128
#endif

#ifndef BT_HID_INPUT_MAX_ARRAY_USAGES
#define BT_HID_INPUT_MAX_ARRAY_USAGES 32
#endif

enum bt_hid_input_event_kind
{
    BT_HID_INPUT_EVENT_KEY,
    BT_HID_INPUT_EVENT_BUTTON,
    BT_HID_INPUT_EVENT_AXIS,
    BT_HID_INPUT_EVENT_CONSUMER,
    BT_HID_INPUT_EVENT_GENERIC
};

struct bt_hid_input_event
{
    enum bt_hid_input_event_kind kind;
    uint8_t report_id;
    uint16_t usage_page;
    uint16_t usage;
    int32_t value;
    bool relative;
};

typedef bool (*bt_hid_input_event_fn)(const struct bt_hid_input_event *event,
                                      void *user_data);

struct bt_hid_input_variable_state
{
    uint8_t report_id;
    uint16_t usage_page;
    uint16_t usage;
    int32_t value;
    bool used;
};

struct bt_hid_input_array_usage
{
    uint8_t report_id;
    uint16_t usage_page;
    uint16_t usage;
};

struct bt_hid_input
{
    const struct bt_hid_report_descriptor *descriptor;
    struct bt_hid_input_variable_state variables[BT_HID_INPUT_MAX_VARIABLES];
    struct bt_hid_input_array_usage arrays[BT_HID_INPUT_MAX_ARRAY_USAGES];
    size_t array_count;
};

void bt_hid_input_init(struct bt_hid_input *input,
                        const struct bt_hid_report_descriptor *descriptor);

/*
 * Converts one raw HID input report into stateful, transport-neutral events.
 * Variable absolute values are emitted only when they change; relative values
 * are emitted whenever nonzero. Array fields are diffed so a keyboard report
 * produces both key-down and key-up events.
 */
bt_status_t bt_hid_input_process(struct bt_hid_input *input,
                                  const uint8_t *report, size_t report_len,
                                  bt_hid_input_event_fn callback, void *user_data);

#endif /* BTCORE_HID_INPUT_H */
