#include <btcore/aros_input_bridge.h>

#include <string.h>

/*
 * HID Keyboard usage -> AROS virtual rawkey mapping. Values follow
 * devices/rawkeycodes.h and the established Poseidon bootkeyboard mapping.
 * Keeping the table in this port layer avoids leaking AROS keycodes into the
 * transport-neutral HID parser.
 */
static const uint8_t keyboard_map[0x68] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0x35, 0x33, 0x22,
    0x12, 0x23, 0x24, 0x25, 0x17, 0x26, 0x27, 0x28,
    0x37, 0x36, 0x18, 0x19, 0x10, 0x13, 0x21, 0x14,
    0x16, 0x34, 0x11, 0x32, 0x15, 0x31, 0x01, 0x02,
    0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
    0x44, 0x45, 0x41, 0x42, 0x40, 0x0B, 0x0C, 0x1A,
    0x1B, 0x0D, 0x2B, 0x29, 0x2A, 0x00, 0x38, 0x39,
    0x3A, 0x62, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55,
    0x56, 0x57, 0x58, 0x59, 0xFF, 0x5F, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x46, 0xFF, 0xFF, 0x4E,
    0x4F, 0x4D, 0x4C, 0xFF, 0x5C, 0x5D, 0x4A, 0x5E,
    0x43, 0x1D, 0x1E, 0x1F, 0x2D, 0x2E, 0x2F, 0x3D,
    0x3E, 0x3F, 0x0F, 0x3C, 0x30, 0xFF, 0xFF, 0xFF};

static bool emit(struct bt_aros_input_bridge *bridge, uint8_t event_class,
                 uint16_t code, int16_t x, int16_t y)
{
    struct bt_aros_input_event event;

    memset(&event, 0, sizeof(event));
    event.event_class = event_class;
    event.code = code;
    event.qualifier = bridge->qualifier;
    event.x = x;
    event.y = y;
    return bridge->emit(bridge->context, &event) == BT_OK;
}

static uint16_t modifier_qualifier(uint16_t usage)
{
    static const uint16_t qualifiers[8] = {
        1u << 3, 1u << 0, 1u << 4, 1u << 6,
        1u << 3, 1u << 1, 1u << 5, 1u << 7};

    return qualifiers[usage - 0xE0u];
}

static uint8_t modifier_rawkey(uint16_t usage)
{
    static const uint8_t rawkeys[8] = {
        0x63, 0x60, 0x64, 0x66, 0x63, 0x61, 0x65, 0x67};

    return rawkeys[usage - 0xE0u];
}

static bool handle_key(struct bt_aros_input_bridge *bridge,
                       const struct bt_hid_input_event *event)
{
    uint8_t rawkey;

    if (event->usage >= 0xE0 && event->usage <= 0xE7)
    {
        uint16_t qualifier = modifier_qualifier(event->usage);

        if (event->value != 0)
            bridge->qualifier |= qualifier;
        else
            bridge->qualifier &= (uint16_t)~qualifier;
        rawkey = modifier_rawkey(event->usage);
    }
    else
    {
        if (event->usage >= sizeof(keyboard_map))
            return true;
        rawkey = keyboard_map[event->usage];
        if (rawkey == 0xFF)
            return true;
    }
    if (event->value == 0)
        rawkey |= BT_AROS_IECODE_UP_PREFIX;
    return emit(bridge, BT_AROS_IECLASS_RAWKEY, rawkey, 0, 0);
}

static bool handle_button(struct bt_aros_input_bridge *bridge,
                          const struct bt_hid_input_event *event)
{
    uint16_t code;

    if (event->usage == 1)
        code = BT_AROS_IECODE_LBUTTON;
    else if (event->usage == 2)
        code = BT_AROS_IECODE_RBUTTON;
    else if (event->usage == 3)
        code = BT_AROS_IECODE_MBUTTON;
    else if (event->usage == 4)
        code = 0x7E; /* RAWKEY_NM_BUTTON_FOURTH */
    else
        return true;
    if (event->value == 0)
        code |= BT_AROS_IECODE_UP_PREFIX;
    return emit(bridge, BT_AROS_IECLASS_RAWMOUSE, code, 0, 0);
}

static bool handle_axis(struct bt_aros_input_bridge *bridge,
                        const struct bt_hid_input_event *event)
{
    if (event->usage == 0x30)
        return emit(bridge, BT_AROS_IECLASS_RAWMOUSE,
                    BT_AROS_IECODE_NOBUTTON, (int16_t)event->value, 0);
    if (event->usage == 0x31)
        return emit(bridge, BT_AROS_IECLASS_RAWMOUSE,
                    BT_AROS_IECODE_NOBUTTON, 0, (int16_t)event->value);
    if (event->usage == 0x38 && event->value != 0)
    {
        uint16_t code = event->value > 0 ? 0x7A : 0x7B;
        uint32_t count = (uint32_t)(event->value > 0
                                        ? (int64_t)event->value
                                        : -(int64_t)event->value);

        while (count-- != 0)
        {
            if (!emit(bridge, BT_AROS_IECLASS_RAWKEY, code, 0, 0) ||
                !emit(bridge, BT_AROS_IECLASS_RAWKEY,
                      (uint16_t)(code | BT_AROS_IECODE_UP_PREFIX), 0, 0))
                return false;
        }
    }
    return true;
}

static bool handle_consumer(struct bt_aros_input_bridge *bridge,
                            const struct bt_hid_input_event *event)
{
    uint16_t code;

    switch (event->usage)
    {
    case 0xB5: /* Scan Next Track */
        code = 0x75;
        break;
    case 0xB6: /* Scan Previous Track */
        code = 0x74;
        break;
    case 0xB7: /* Stop */
        code = 0x72;
        break;
    case 0xCD: /* Play/Pause */
        code = 0x73;
        break;
    default:
        return true;
    }
    if (event->value == 0)
        code |= BT_AROS_IECODE_UP_PREFIX;
    return emit(bridge, BT_AROS_IECLASS_RAWKEY, code, 0, 0);
}

void bt_aros_input_bridge_init(struct bt_aros_input_bridge *bridge,
                                bt_aros_input_emit_fn emit_fn, void *context)
{
    memset(bridge, 0, sizeof(*bridge));
    bridge->emit = emit_fn;
    bridge->context = context;
}

bool bt_aros_input_bridge_handle(const struct bt_hid_input_event *event,
                                  void *user_data)
{
    struct bt_aros_input_bridge *bridge = user_data;

    if (event == NULL || bridge == NULL || bridge->emit == NULL)
        return false;
    switch (event->kind)
    {
    case BT_HID_INPUT_EVENT_KEY:
        return handle_key(bridge, event);
    case BT_HID_INPUT_EVENT_BUTTON:
        return handle_button(bridge, event);
    case BT_HID_INPUT_EVENT_AXIS:
        return handle_axis(bridge, event);
    case BT_HID_INPUT_EVENT_CONSUMER:
        return handle_consumer(bridge, event);
    case BT_HID_INPUT_EVENT_GENERIC:
        return true;
    }
    return true;
}
