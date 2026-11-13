#ifndef BTCORE_AROS_INPUT_BRIDGE_H
#define BTCORE_AROS_INPUT_BRIDGE_H

#include <btcore/hid_input.h>

#define BT_AROS_IECLASS_RAWKEY 1u
#define BT_AROS_IECLASS_RAWMOUSE 2u
#define BT_AROS_IECODE_UP_PREFIX 0x80u
#define BT_AROS_IECODE_LBUTTON 0x68u
#define BT_AROS_IECODE_RBUTTON 0x69u
#define BT_AROS_IECODE_MBUTTON 0x6Au
#define BT_AROS_IECODE_NOBUTTON 0xFFu

struct bt_aros_input_event
{
    uint8_t event_class;
    uint16_t code;
    uint16_t qualifier;
    int16_t x;
    int16_t y;
};

typedef bt_status_t (*bt_aros_input_emit_fn)(
    void *context, const struct bt_aros_input_event *event);

struct bt_aros_input_bridge
{
    bt_aros_input_emit_fn emit;
    void *context;
    uint16_t qualifier;
};

void bt_aros_input_bridge_init(struct bt_aros_input_bridge *bridge,
                                bt_aros_input_emit_fn emit, void *context);

/* Matches bt_hid_input_event_fn and can be installed directly on HOGP. */
bool bt_aros_input_bridge_handle(const struct bt_hid_input_event *event,
                                  void *user_data);

#endif /* BTCORE_AROS_INPUT_BRIDGE_H */
