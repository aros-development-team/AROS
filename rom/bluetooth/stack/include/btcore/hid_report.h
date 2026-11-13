#ifndef BTCORE_HID_REPORT_H
#define BTCORE_HID_REPORT_H

#include <btcore/status.h>
#include <btcore/types.h>

#ifndef BT_HID_MAX_FIELDS
#define BT_HID_MAX_FIELDS 256
#endif

#ifndef BT_HID_MAX_REPORTS
#define BT_HID_MAX_REPORTS 16
#endif

#ifndef BT_HID_MAX_LOCAL_USAGES
#define BT_HID_MAX_LOCAL_USAGES 64
#endif

#define BT_HID_INPUT_CONSTANT 0x0001u
#define BT_HID_INPUT_VARIABLE 0x0002u
#define BT_HID_INPUT_RELATIVE 0x0004u

struct bt_hid_field
{
    uint8_t report_id;
    uint16_t bit_offset;
    uint8_t bit_size;
    uint16_t count;
    uint16_t flags;
    uint16_t usage_page;
    uint16_t usage_min;
    uint16_t usage_max;
    int32_t logical_min;
    int32_t logical_max;
};

struct bt_hid_report_info
{
    uint8_t report_id;
    uint16_t input_bits;
};

struct bt_hid_report_descriptor
{
    struct bt_hid_field fields[BT_HID_MAX_FIELDS];
    size_t field_count;
    struct bt_hid_report_info reports[BT_HID_MAX_REPORTS];
    size_t report_count;
    bool uses_report_ids;
};

struct bt_hid_value
{
    uint8_t report_id;
    uint16_t usage_page;
    uint16_t usage;
    int32_t value;
    uint16_t flags;
    bool is_array;
};

typedef bool (*bt_hid_value_fn)(const struct bt_hid_value *value, void *user_data);

/*
 * Parses the HID 1.11 short-item Report Descriptor format into fixed storage.
 * Long items are skipped because HID 1.11 defines no long-item tags. Only
 * Input main items become fields; Output/Feature offsets are intentionally
 * outside this first host-side decoding slice.
 */
bt_status_t bt_hid_report_parse(const uint8_t *data, size_t data_len,
                                 struct bt_hid_report_descriptor *out);

/*
 * Decodes one complete input report. If the descriptor uses Report IDs,
 * report[0] is the ID and field bit offsets begin at report[1].
 * Constant fields and zero-valued array slots are not emitted.
 */
bt_status_t bt_hid_report_decode_input(const struct bt_hid_report_descriptor *descriptor,
                                        const uint8_t *report, size_t report_len,
                                        bt_hid_value_fn callback, void *user_data);

#endif /* BTCORE_HID_REPORT_H */
