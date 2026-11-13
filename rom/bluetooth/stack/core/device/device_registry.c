#include <btcore/device_registry.h>

#include <string.h>

void bt_device_registry_init(struct bt_device_registry *reg)
{
    reg->count = 0;
}

size_t bt_device_registry_count(const struct bt_device_registry *reg)
{
    return reg->count;
}

const struct bt_discovered_device *bt_device_registry_get(const struct bt_device_registry *reg,
                                                            size_t index)
{
    if (index >= reg->count)
        return NULL;
    return &reg->devices[index];
}

const struct bt_discovered_device *bt_device_registry_find(const struct bt_device_registry *reg,
                                                             const struct bt_addr *addr)
{
    size_t i;

    for (i = 0; i < reg->count; i++)
    {
        if (bt_addr_equal(&reg->devices[i].addr, addr))
            return &reg->devices[i];
    }
    return NULL;
}

static struct bt_discovered_device *find_or_create(struct bt_device_registry *reg,
                                                     const struct bt_addr *addr)
{
    size_t i;
    struct bt_discovered_device *dev;

    for (i = 0; i < reg->count; i++)
    {
        if (bt_addr_equal(&reg->devices[i].addr, addr))
            return &reg->devices[i];
    }

    if (reg->count >= BT_DEVICE_REGISTRY_MAX)
        return NULL;

    /*
     * Clear the whole entry rather than naming its fields.
     *
     * Naming them meant that adding one to the struct -- appearance, then the
     * name and its state -- left it holding whatever the slot held before,
     * which is a bug that only appears once a slot is reused and is invisible
     * in a fresh registry.
     */
    dev = &reg->devices[reg->count++];
    memset(dev, 0, sizeof(*dev));
    dev->addr = *addr;
    return dev;
}

struct bt_discovered_device *bt_device_registry_note_classic(struct bt_device_registry *reg,
                                                               const struct bt_addr *addr,
                                                               uint32_t class_of_device)
{
    struct bt_discovered_device *dev = find_or_create(reg, addr);

    if (dev == NULL)
        return NULL;

    dev->flags |= BT_DEVICE_FLAG_CLASSIC;
    if (bt_cod_is_hid(class_of_device))
        dev->flags |= BT_DEVICE_FLAG_HID;
    {
        const char *label = bt_label_from_cod(class_of_device);

        if (label != NULL)
            bt_device_set_name(dev, label, BT_DEVICE_NAME_LEN, 1);
    }
    dev->class_of_device = class_of_device;
    dev->sightings++;
    return dev;
}

/*
 * Class of device: 24 bits, of which bits 12..8 are the major device class and
 * bits 7..2 the minor. Major 5 is Peripheral, and inside it bit 6 means
 * keyboard and bit 7 pointing device -- a combined keyboard-and-touchpad sets
 * both. Nothing else in the peripheral class is an input device we care about,
 * so requiring one of those two bits keeps joysticks and remotes out.
 */
/*
 * A public address is an identity by definition. Among random addresses only
 * the static kind is stable: the top two bits of the most significant byte are
 * 11 for static random, 01 for resolvable private and 00 for non-resolvable,
 * and the last two rotate.
 */
bool bt_le_addr_is_stable(const struct bt_addr *addr, uint8_t address_type)
{
    if (address_type == 0 || address_type >= 2)
        return true;                    /* public, or an identity address resolved by the controller */
    return (addr->b[BT_ADDR_LEN - 1u] & 0xc0u) == 0xc0u;   /* static random */
}

bool bt_cod_is_hid(uint32_t class_of_device)
{
    const uint32_t major = (class_of_device >> 8) & 0x1fu;
    const uint32_t minor = (class_of_device >> 2) & 0x3fu;

    if (major != 0x05u)
        return false;
    return (minor & 0x30u) != 0u;
}

/*
 * Advertising data is a sequence of length-prefixed structures: one byte of
 * length covering the type byte and the payload, then the type, then the
 * payload. A zero length ends the list, which is how padding to the fixed
 * 31-byte field is expressed.
 *
 * Two things here say HID. The service UUID list (type 0x02 incomplete, 0x03
 * complete) may contain 0x1812, HID over GATT. The Appearance (0x19) is a
 * 16-bit value whose top ten bits are a category, and category 15 is HID --
 * which covers keyboard (0x03C1) and mouse (0x03C2) without enumerating them.
 */
void bt_le_adv_parse(const uint8_t *data, size_t length, struct bt_le_adv_info *out)
{
    size_t i = 0;

    if (out == NULL)
        return;
    out->hid = false;
    out->appearance = 0;
    out->name = NULL;
    out->name_len = 0;
    out->name_complete = false;
    out->has_flags = false;
    out->flags = 0;
    if (data == NULL)
        return;

    while (i < length)
    {
        const uint8_t field_len = data[i];
        uint8_t type;
        const uint8_t *payload;
        size_t payload_len;

        if (field_len == 0)
            break;                  /* end of list; the rest is padding */
        if (i + 1u + field_len > length)
            break;                  /* truncated field: keep what was valid */
        type = data[i + 1u];
        payload = &data[i + 2u];
        payload_len = field_len - 1u;

        switch (type)
        {
        case 0x01u:                 /* flags: discoverable mode bits */
            if (payload_len >= 1u)
            {
                out->has_flags = true;
                out->flags = payload[0];
            }
            break;
        case 0x02u:                 /* incomplete list of 16-bit service UUIDs */
        case 0x03u:                 /* complete list */
        {
            size_t u;

            for (u = 0; u + 1u < payload_len; u += 2u)
            {
                const uint16_t uuid =
                    (uint16_t)payload[u] | ((uint16_t)payload[u + 1u] << 8);

                if (uuid == 0x1812u)    /* HID over GATT */
                    out->hid = true;
            }
            break;
        }
        case 0x08u:                 /* shortened local name */
        case 0x09u:                 /* complete local name */
            /*
             * Prefer the complete name, but take the shortened one when that is
             * all there is -- many devices advertise only 0x08 because the
             * payload is 31 bytes and a full name does not fit beside the
             * service UUIDs.
             */
            if (out->name == NULL || (type == 0x09u && !out->name_complete))
            {
                out->name = payload;
                out->name_len = payload_len;
                out->name_complete = (type == 0x09u);
            }
            break;
        case 0x19u:                 /* appearance */
            if (payload_len >= 2u)
            {
                out->appearance =
                    (uint16_t)payload[0] | ((uint16_t)payload[1] << 8);
                if ((out->appearance >> 6) == 15u)  /* category 15: HID */
                    out->hid = true;
            }
            break;
        default:
            break;
        }
        i += 1u + field_len;
    }
}

bool bt_le_adv_is_hid(const uint8_t *data, size_t length, uint16_t *appearance_out)
{
    struct bt_le_adv_info info;

    bt_le_adv_parse(data, length, &info);
    if (appearance_out != NULL)
        *appearance_out = info.appearance;
    return info.hid;
}

/*
 * Names are stored printable and trimmed.
 *
 * Advertising and EIR names are NUL-padded to the field width, so stopping at
 * the first NUL is what keeps the padding from being rendered; anything outside
 * printable ASCII becomes '?' rather than being dropped, so a mojibake name
 * still has the right shape and length.
 */
void bt_device_set_name(struct bt_discovered_device *dev, const char *name,
                        size_t length, uint8_t state)
{
    size_t i;

    if (dev == NULL || name == NULL || state == 0)
        return;
    if (state <= dev->name_state)
        return;                 /* never downgrade a real name to a label */

    for (i = 0; i < length && i + 1u < BT_DEVICE_NAME_LEN && name[i] != '\0'; i++)
    {
        const char c = name[i];

        dev->name[i] = (c >= 0x20 && c <= 0x7e) ? c : '?';
    }
    while (i > 0 && dev->name[i - 1u] == ' ')
        i--;
    dev->name[i] = '\0';
    dev->name_state = state;
}

const char *bt_label_from_appearance(uint16_t appearance)
{
    switch (appearance)
    {
    case 961: return "<keyboard>";
    case 962: return "<mouse>";
    case 963: return "<joystick>";
    case 964: return "<gamepad>";
    default:  return (appearance >> 6) == 15u ? "<HID device>" : NULL;
    }
}

const char *bt_label_from_cod(uint32_t class_of_device)
{
    if (((class_of_device >> 8) & 0x1fu) != 0x05u)   /* major: peripheral */
        return NULL;
    switch ((class_of_device >> 2) & 0x0fu)          /* minor: pointing/gaming */
    {
    case 1: return "<joystick>";
    case 2: return "<gamepad>";
    default: break;
    }
    switch ((class_of_device >> 6) & 0x03u)          /* minor: keyboard/pointing */
    {
    case 1: return "<keyboard>";
    case 2: return "<mouse>";
    case 3: return "<keyboard+mouse>";
    default: return "<HID device>";
    }
}

struct bt_discovered_device *bt_device_registry_note_le(struct bt_device_registry *reg,
                                                          const struct bt_addr *addr,
                                                          uint8_t le_address_type, int8_t rssi)
{
    struct bt_discovered_device *dev = find_or_create(reg, addr);

    if (dev == NULL)
        return NULL;

    dev->flags |= BT_DEVICE_FLAG_LE;
    dev->le_address_type = le_address_type;
    dev->last_rssi = rssi;
    dev->sightings++;
    return dev;
}
