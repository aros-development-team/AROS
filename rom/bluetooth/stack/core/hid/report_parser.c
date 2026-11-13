#include <btcore/hid_report.h>

#include <string.h>

#define HID_TYPE_MAIN 0u
#define HID_TYPE_GLOBAL 1u
#define HID_TYPE_LOCAL 2u

#define HID_MAIN_INPUT 8u
#define HID_MAIN_COLLECTION 10u
#define HID_MAIN_END_COLLECTION 12u

#define HID_GLOBAL_USAGE_PAGE 0u
#define HID_GLOBAL_LOGICAL_MIN 1u
#define HID_GLOBAL_LOGICAL_MAX 2u
#define HID_GLOBAL_REPORT_SIZE 7u
#define HID_GLOBAL_REPORT_ID 8u
#define HID_GLOBAL_REPORT_COUNT 9u
#define HID_GLOBAL_PUSH 10u
#define HID_GLOBAL_POP 11u

#define HID_LOCAL_USAGE 0u
#define HID_LOCAL_USAGE_MIN 1u
#define HID_LOCAL_USAGE_MAX 2u

#define HID_GLOBAL_STACK_MAX 4

struct hid_globals
{
    uint16_t usage_page;
    int32_t logical_min;
    int32_t logical_max;
    uint16_t report_size;
    uint16_t report_count;
    uint8_t report_id;
};

struct hid_locals
{
    uint16_t usages[BT_HID_MAX_LOCAL_USAGES];
    size_t usage_count;
    uint16_t usage_min;
    uint16_t usage_max;
    bool has_usage_min;
    bool has_usage_max;
};

static uint32_t read_unsigned(const uint8_t *data, size_t len)
{
    uint32_t value = 0;
    size_t i;

    for (i = 0; i < len; ++i)
        value |= (uint32_t)data[i] << (8u * i);
    return value;
}

static int32_t read_signed(const uint8_t *data, size_t len)
{
    uint32_t value = read_unsigned(data, len);

    if (len != 0 && len < 4 && (value & (1u << (len * 8u - 1u))) != 0)
        value |= UINT32_MAX << (len * 8u);
    return (int32_t)value;
}

static struct bt_hid_report_info *get_report(struct bt_hid_report_descriptor *out,
                                              uint8_t report_id)
{
    size_t i;

    for (i = 0; i < out->report_count; ++i)
        if (out->reports[i].report_id == report_id)
            return &out->reports[i];
    if (out->report_count == BT_HID_MAX_REPORTS)
        return NULL;
    out->reports[out->report_count].report_id = report_id;
    return &out->reports[out->report_count++];
}

static uint16_t usage_for_index(const struct hid_locals *locals, uint16_t index)
{
    if (index < locals->usage_count)
        return locals->usages[index];
    if (locals->has_usage_min && locals->has_usage_max &&
        (uint32_t)locals->usage_min + index <= locals->usage_max)
        return (uint16_t)(locals->usage_min + index);
    if (locals->usage_count != 0)
        return locals->usages[locals->usage_count - 1];
    return 0;
}

static bt_status_t add_input(struct bt_hid_report_descriptor *out,
                              const struct hid_globals *globals,
                              const struct hid_locals *locals, uint16_t flags)
{
    struct bt_hid_report_info *report = get_report(out, globals->report_id);
    uint16_t i;
    uint16_t emitted = (flags & BT_HID_INPUT_VARIABLE) != 0
                           ? globals->report_count
                           : (uint16_t)1;
    uint32_t added_bits = (uint32_t)globals->report_size * globals->report_count;

    if (globals->report_size > 32 || globals->report_count == 0 || added_bits > UINT16_MAX)
        return BT_ERR_INVALID_ARGUMENT;          /* malformed item */
    if (report == NULL || report->input_bits > UINT16_MAX - added_bits ||
        out->field_count + emitted > BT_HID_MAX_FIELDS)
    {
        /* beyond our tables (too many reports/fields): drop this input item but
         * keep the report's bit accounting right so later items stay aligned */
        if (report != NULL && report->input_bits <= UINT16_MAX - added_bits)
            report->input_bits = (uint16_t)(report->input_bits + added_bits);
        return BT_OK;
    }
    for (i = 0; i < emitted; ++i)
    {
        struct bt_hid_field *field = &out->fields[out->field_count++];

        field->report_id = globals->report_id;
        field->bit_offset = (uint16_t)(report->input_bits +
                                       (uint32_t)i * globals->report_size);
        field->bit_size = (uint8_t)globals->report_size;
        field->count = (flags & BT_HID_INPUT_VARIABLE) != 0 ? 1 : globals->report_count;
        field->flags = flags;
        field->usage_page = globals->usage_page;
        field->logical_min = globals->logical_min;
        field->logical_max = globals->logical_max;
        if ((flags & BT_HID_INPUT_VARIABLE) != 0)
            field->usage_min = field->usage_max = usage_for_index(locals, i);
        else
        {
            field->usage_min = locals->has_usage_min ? locals->usage_min : 0;
            field->usage_max = locals->has_usage_max ? locals->usage_max : UINT16_MAX;
        }
    }
    report->input_bits = (uint16_t)(report->input_bits + added_bits);
    return BT_OK;
}

bt_status_t bt_hid_report_parse(const uint8_t *data, size_t data_len,
                                 struct bt_hid_report_descriptor *out)
{
    struct hid_globals globals = {0};
    struct hid_globals stack[HID_GLOBAL_STACK_MAX];
    struct hid_locals locals = {0};
    size_t stack_depth = 0;
    size_t offset = 0;

    if (data == NULL || out == NULL)
        return BT_ERR_INVALID_ARGUMENT;
    memset(out, 0, sizeof(*out));
    while (offset < data_len)
    {
        uint8_t prefix = data[offset++];
        uint8_t size_code;
        size_t item_size;
        uint8_t type;
        uint8_t tag;
        const uint8_t *item;
        uint32_t value;

        if (prefix == 0xFE)
        {
            size_t long_size;

            if (data_len - offset < 2)
                return BT_ERR_INVALID_ARGUMENT;
            long_size = data[offset];
            offset += 2;
            if (long_size > data_len - offset)
                return BT_ERR_INVALID_ARGUMENT;
            offset += long_size;
            continue;
        }
        size_code = prefix & 0x03u;
        item_size = size_code == 3 ? 4u : size_code;
        type = (prefix >> 2) & 0x03u;
        tag = prefix >> 4;
        if (item_size > data_len - offset)
            return BT_ERR_INVALID_ARGUMENT;
        item = data + offset;
        offset += item_size;
        value = read_unsigned(item, item_size);

        if (type == HID_TYPE_GLOBAL)
        {
            switch (tag)
            {
            case HID_GLOBAL_USAGE_PAGE:
                globals.usage_page = (uint16_t)value;
                break;
            case HID_GLOBAL_LOGICAL_MIN:
                globals.logical_min = read_signed(item, item_size);
                break;
            case HID_GLOBAL_LOGICAL_MAX:
                globals.logical_max = globals.logical_min < 0
                                          ? read_signed(item, item_size)
                                          : (int32_t)value;
                break;
            case HID_GLOBAL_REPORT_SIZE:
                globals.report_size = (uint16_t)value;
                break;
            case HID_GLOBAL_REPORT_ID:
                if (value == 0 || value > UINT8_MAX)
                    return BT_ERR_INVALID_ARGUMENT;
                globals.report_id = (uint8_t)value;
                out->uses_report_ids = true;
                break;
            case HID_GLOBAL_REPORT_COUNT:
                globals.report_count = (uint16_t)value;
                break;
            case HID_GLOBAL_PUSH:
                if (stack_depth == HID_GLOBAL_STACK_MAX)
                    return BT_ERR_NO_RESOURCES;
                stack[stack_depth++] = globals;
                break;
            case HID_GLOBAL_POP:
                if (stack_depth == 0)
                    return BT_ERR_INVALID_ARGUMENT;
                globals = stack[--stack_depth];
                break;
            default:
                break;
            }
        }
        else if (type == HID_TYPE_LOCAL)
        {
            switch (tag)
            {
            case HID_LOCAL_USAGE:
                /* more usages than we can list: keep what we have (the last one
                 * is reused for the rest) rather than rejecting the whole map */
                if (locals.usage_count < BT_HID_MAX_LOCAL_USAGES)
                    locals.usages[locals.usage_count++] = (uint16_t)value;
                break;
            case HID_LOCAL_USAGE_MIN:
                locals.usage_min = (uint16_t)value;
                locals.has_usage_min = true;
                break;
            case HID_LOCAL_USAGE_MAX:
                locals.usage_max = (uint16_t)value;
                locals.has_usage_max = true;
                break;
            default:
                break;
            }
        }
        else if (type == HID_TYPE_MAIN)
        {
            bt_status_t st = BT_OK;

            if (tag == HID_MAIN_INPUT)
                st = add_input(out, &globals, &locals, (uint16_t)value);
            else if (tag == HID_MAIN_END_COLLECTION && item_size != 0)
                st = BT_ERR_INVALID_ARGUMENT;
            else if (tag == HID_MAIN_COLLECTION && item_size != 1)
                st = BT_ERR_INVALID_ARGUMENT;
            memset(&locals, 0, sizeof(locals));
            if (st != BT_OK)
                return st;
        }
    }
    return out->report_count == 0 ? BT_ERR_INVALID_ARGUMENT : BT_OK;
}

static uint32_t extract_bits(const uint8_t *data, uint16_t bit_offset, uint8_t bit_size)
{
    uint32_t value = 0;
    uint8_t i;

    for (i = 0; i < bit_size; ++i)
        if ((data[(bit_offset + i) / 8u] & (1u << ((bit_offset + i) % 8u))) != 0)
            value |= 1u << i;
    return value;
}

static int32_t sign_value(uint32_t value, uint8_t bit_size)
{
    if (bit_size != 0 && bit_size < 32 && (value & (1u << (bit_size - 1u))) != 0)
        value |= UINT32_MAX << bit_size;
    return (int32_t)value;
}

bt_status_t bt_hid_report_decode_input(const struct bt_hid_report_descriptor *descriptor,
                                        const uint8_t *report, size_t report_len,
                                        bt_hid_value_fn callback, void *user_data)
{
    const uint8_t *payload = report;
    size_t payload_len = report_len;
    uint8_t report_id = 0;
    size_t i;

    if (descriptor == NULL || report == NULL || callback == NULL)
        return BT_ERR_INVALID_ARGUMENT;
    if (descriptor->uses_report_ids)
    {
        if (report_len == 0)
            return BT_ERR_INVALID_ARGUMENT;
        report_id = report[0];
        payload = report + 1;
        payload_len = report_len - 1;
    }
    for (i = 0; i < descriptor->field_count; ++i)
    {
        const struct bt_hid_field *field = &descriptor->fields[i];
        uint16_t element;

        if (field->report_id != report_id)
            continue;
        if ((uint32_t)field->bit_offset + (uint32_t)field->bit_size * field->count >
            payload_len * 8u)
            return BT_ERR_INVALID_ARGUMENT;
        if ((field->flags & BT_HID_INPUT_CONSTANT) != 0)
            continue;
        for (element = 0; element < field->count; ++element)
        {
            uint32_t raw = extract_bits(payload,
                                        (uint16_t)(field->bit_offset +
                                                   element * field->bit_size),
                                        field->bit_size);
            struct bt_hid_value value;

            value.report_id = report_id;
            value.usage_page = field->usage_page;
            value.flags = field->flags;
            value.is_array = (field->flags & BT_HID_INPUT_VARIABLE) == 0;
            if (value.is_array)
            {
                if (raw == 0)
                    continue;
                value.usage = (uint16_t)raw;
                value.value = 1;
            }
            else
            {
                value.usage = field->usage_min;
                value.value = field->logical_min < 0
                                  ? sign_value(raw, field->bit_size)
                                  : (int32_t)raw;
            }
            if (!callback(&value, user_data))
                return BT_OK;
        }
    }
    return BT_OK;
}
