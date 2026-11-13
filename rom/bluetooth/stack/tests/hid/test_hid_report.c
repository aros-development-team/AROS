#include "test_hid_report.h"
#include "../support/test.h"

#include <btcore/hid_report.h>

#include <string.h>

struct captured_values
{
    struct bt_hid_value values[32];
    size_t count;
};

static bool capture(const struct bt_hid_value *value, void *user_data)
{
    struct captured_values *captured = user_data;

    captured->values[captured->count++] = *value;
    return true;
}

static void test_boot_keyboard_descriptor_and_report(void)
{
    static const uint8_t descriptor[] = {
        0x05, 0x01,       0x09, 0x06,       0xA1, 0x01,
        0x05, 0x07,       0x19, 0xE0,       0x29, 0xE7,
        0x15, 0x00,       0x25, 0x01,       0x75, 0x01,
        0x95, 0x08,       0x81, 0x02,       0x95, 0x01,
        0x75, 0x08,       0x81, 0x01,       0x95, 0x06,
        0x75, 0x08,       0x15, 0x00,       0x25, 0x65,
        0x05, 0x07,       0x19, 0x00,       0x29, 0x65,
        0x81, 0x00,       0xC0};
    const uint8_t report[] = {0x02, 0x00, 0x04, 0x05, 0, 0, 0, 0};
    struct bt_hid_report_descriptor parsed;
    struct captured_values captured = {0};

    BT_CHECK(bt_hid_report_parse(descriptor, sizeof(descriptor), &parsed) == BT_OK);
    BT_CHECK(parsed.report_count == 1);
    BT_CHECK(parsed.reports[0].input_bits == 64);
    BT_CHECK(parsed.field_count == 10);
    BT_CHECK(bt_hid_report_decode_input(&parsed, report, sizeof(report),
                                         capture, &captured) == BT_OK);
    BT_CHECK(captured.count == 10);
    BT_CHECK(captured.values[1].usage_page == 0x07);
    BT_CHECK(captured.values[1].usage == 0xE1 && captured.values[1].value == 1);
    BT_CHECK(captured.values[8].is_array && captured.values[8].usage == 0x04);
    BT_CHECK(captured.values[9].is_array && captured.values[9].usage == 0x05);
}

static void test_mouse_signed_relative_axes(void)
{
    static const uint8_t descriptor[] = {
        0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x09, 0x01, 0xA1, 0x00,
        0x05, 0x09, 0x19, 0x01, 0x29, 0x03, 0x15, 0x00, 0x25, 0x01,
        0x95, 0x03, 0x75, 0x01, 0x81, 0x02, 0x95, 0x01, 0x75, 0x05,
        0x81, 0x01, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x15, 0x81,
        0x25, 0x7F, 0x75, 0x08, 0x95, 0x02, 0x81, 0x06, 0xC0, 0xC0};
    const uint8_t report[] = {0x05, 0xFE, 0x7F};
    struct bt_hid_report_descriptor parsed;
    struct captured_values captured = {0};

    BT_CHECK(bt_hid_report_parse(descriptor, sizeof(descriptor), &parsed) == BT_OK);
    BT_CHECK(parsed.reports[0].input_bits == 24);
    BT_CHECK(bt_hid_report_decode_input(&parsed, report, sizeof(report),
                                         capture, &captured) == BT_OK);
    BT_CHECK(captured.count == 5);
    BT_CHECK(captured.values[0].usage_page == 0x09 &&
             captured.values[0].usage == 1 && captured.values[0].value == 1);
    BT_CHECK(captured.values[2].usage == 3 && captured.values[2].value == 1);
    BT_CHECK(captured.values[3].usage_page == 0x01 &&
             captured.values[3].usage == 0x30 && captured.values[3].value == -2);
    BT_CHECK(captured.values[4].usage == 0x31 && captured.values[4].value == 127);
    BT_CHECK((captured.values[4].flags & BT_HID_INPUT_RELATIVE) != 0);
}

static void test_report_ids_and_invalid_inputs(void)
{
    static const uint8_t descriptor[] = {
        0x05, 0x0C, 0x09, 0x01, 0xA1, 0x01,
        0x85, 0x02, 0x09, 0xE9, 0x15, 0x00,
        0x25, 0x01, 0x75, 0x01, 0x95, 0x01,
        0x81, 0x02, 0x75, 0x07, 0x95, 0x01,
        0x81, 0x01, 0xC0};
    const uint8_t report[] = {0x02, 0x01};
    struct bt_hid_report_descriptor parsed;
    struct captured_values captured = {0};
    uint8_t truncated[] = {0x75};

    BT_CHECK(bt_hid_report_parse(descriptor, sizeof(descriptor), &parsed) == BT_OK);
    BT_CHECK(parsed.uses_report_ids);
    BT_CHECK(parsed.reports[0].report_id == 2 && parsed.reports[0].input_bits == 8);
    BT_CHECK(bt_hid_report_decode_input(&parsed, report, sizeof(report),
                                         capture, &captured) == BT_OK);
    BT_CHECK(captured.count == 1);
    BT_CHECK(captured.values[0].report_id == 2);
    BT_CHECK(captured.values[0].usage_page == 0x0C &&
             captured.values[0].usage == 0xE9 && captured.values[0].value == 1);
    BT_CHECK(bt_hid_report_decode_input(&parsed, report, 1, capture, &captured) ==
              BT_ERR_INVALID_ARGUMENT);
    BT_CHECK(bt_hid_report_parse(truncated, sizeof(truncated), &parsed) ==
              BT_ERR_INVALID_ARGUMENT);
}

void run_hid_report_tests(void)
{
    test_boot_keyboard_descriptor_and_report();
    test_mouse_signed_relative_axes();
    test_report_ids_and_invalid_inputs();
}
