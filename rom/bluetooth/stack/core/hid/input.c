#include <btcore/hid_input.h>

#include <string.h>

struct decode_context
{
    struct bt_hid_input *input;
    bt_hid_input_event_fn callback;
    void *user_data;
    struct bt_hid_input_array_usage current[BT_HID_INPUT_MAX_ARRAY_USAGES];
    size_t current_count;
    bt_status_t status;
};

static enum bt_hid_input_event_kind classify(uint16_t usage_page, uint16_t usage)
{
    if (usage_page == 0x07)
        return BT_HID_INPUT_EVENT_KEY;
    if (usage_page == 0x09)
        return BT_HID_INPUT_EVENT_BUTTON;
    if (usage_page == 0x0C)
        return BT_HID_INPUT_EVENT_CONSUMER;
    if (usage_page == 0x01 && usage >= 0x30 && usage <= 0x39)
        return BT_HID_INPUT_EVENT_AXIS;
    return BT_HID_INPUT_EVENT_GENERIC;
}

static bool emit(struct decode_context *context, uint8_t report_id,
                 uint16_t usage_page, uint16_t usage, int32_t value,
                 bool relative)
{
    struct bt_hid_input_event event;

    event.kind = classify(usage_page, usage);
    event.report_id = report_id;
    event.usage_page = usage_page;
    event.usage = usage;
    event.value = value;
    event.relative = relative;
    return context->callback(&event, context->user_data);
}

static bool array_equal(const struct bt_hid_input_array_usage *a,
                        const struct bt_hid_input_array_usage *b)
{
    return a->report_id == b->report_id && a->usage_page == b->usage_page &&
           a->usage == b->usage;
}

static bool contains_array(const struct bt_hid_input_array_usage *items, size_t count,
                           const struct bt_hid_input_array_usage *target)
{
    size_t i;

    for (i = 0; i < count; ++i)
        if (array_equal(&items[i], target))
            return true;
    return false;
}

static struct bt_hid_input_variable_state *get_variable(
    struct bt_hid_input *input, const struct bt_hid_value *value)
{
    struct bt_hid_input_variable_state *free_slot = NULL;
    size_t i;

    for (i = 0; i < BT_HID_INPUT_MAX_VARIABLES; ++i)
    {
        struct bt_hid_input_variable_state *state = &input->variables[i];

        if (!state->used)
        {
            if (free_slot == NULL)
                free_slot = state;
            continue;
        }
        if (state->report_id == value->report_id &&
            state->usage_page == value->usage_page && state->usage == value->usage)
            return state;
    }
    if (free_slot != NULL)
    {
        free_slot->used = true;
        free_slot->report_id = value->report_id;
        free_slot->usage_page = value->usage_page;
        free_slot->usage = value->usage;
    }
    return free_slot;
}

static bool decoded_value(const struct bt_hid_value *value, void *user_data)
{
    struct decode_context *context = user_data;

    if (value->is_array)
    {
        struct bt_hid_input_array_usage usage;

        usage.report_id = value->report_id;
        usage.usage_page = value->usage_page;
        usage.usage = value->usage;
        if (!contains_array(context->current, context->current_count, &usage))
        {
            if (context->current_count == BT_HID_INPUT_MAX_ARRAY_USAGES)
            {
                context->status = BT_ERR_NO_RESOURCES;
                return false;
            }
            context->current[context->current_count++] = usage;
        }
        return true;
    }
    else
    {
        struct bt_hid_input_variable_state *state =
            get_variable(context->input, value);
        bool relative = (value->flags & BT_HID_INPUT_RELATIVE) != 0;

        if (state == NULL)
        {
            context->status = BT_ERR_NO_RESOURCES;
            return false;
        }
        if ((relative && value->value != 0) ||
            (!relative && state->value != value->value))
        {
            if (!emit(context, value->report_id, value->usage_page, value->usage,
                      value->value, relative))
                return false;
        }
        if (!relative)
            state->value = value->value;
        return true;
    }
}

void bt_hid_input_init(struct bt_hid_input *input,
                        const struct bt_hid_report_descriptor *descriptor)
{
    memset(input, 0, sizeof(*input));
    input->descriptor = descriptor;
}

bt_status_t bt_hid_input_process(struct bt_hid_input *input,
                                  const uint8_t *report, size_t report_len,
                                  bt_hid_input_event_fn callback, void *user_data)
{
    struct decode_context context;
    uint8_t report_id = 0;
    size_t i;
    bt_status_t status;

    if (input == NULL || input->descriptor == NULL || report == NULL ||
        callback == NULL)
        return BT_ERR_INVALID_ARGUMENT;
    if (input->descriptor->uses_report_ids)
    {
        if (report_len == 0)
            return BT_ERR_INVALID_ARGUMENT;
        report_id = report[0];
    }
    memset(&context, 0, sizeof(context));
    context.input = input;
    context.callback = callback;
    context.user_data = user_data;
    context.status = BT_OK;
    status = bt_hid_report_decode_input(input->descriptor, report, report_len,
                                         decoded_value, &context);
    if (status != BT_OK)
        return status;
    if (context.status != BT_OK)
        return context.status;

    for (i = 0; i < input->array_count; ++i)
    {
        const struct bt_hid_input_array_usage *old = &input->arrays[i];

        if (old->report_id == report_id &&
            !contains_array(context.current, context.current_count, old) &&
            !emit(&context, old->report_id, old->usage_page, old->usage, 0, false))
            return BT_OK;
    }
    for (i = 0; i < context.current_count; ++i)
    {
        const struct bt_hid_input_array_usage *current = &context.current[i];

        if (!contains_array(input->arrays, input->array_count, current) &&
            !emit(&context, current->report_id, current->usage_page,
                  current->usage, 1, false))
            return BT_OK;
    }

    {
        struct bt_hid_input_array_usage retained[BT_HID_INPUT_MAX_ARRAY_USAGES];
        size_t retained_count = 0;

        for (i = 0; i < input->array_count; ++i)
            if (input->arrays[i].report_id != report_id)
                retained[retained_count++] = input->arrays[i];
        if (retained_count + context.current_count > BT_HID_INPUT_MAX_ARRAY_USAGES)
            return BT_ERR_NO_RESOURCES;
        memcpy(retained + retained_count, context.current,
               context.current_count * sizeof(context.current[0]));
        memcpy(input->arrays, retained,
               (retained_count + context.current_count) * sizeof(retained[0]));
        input->array_count = retained_count + context.current_count;
    }
    return BT_OK;
}
