#ifndef BTCORE_DEVICE_REGISTRY_H
#define BTCORE_DEVICE_REGISTRY_H

#include <btcore/addr.h>
#include <btcore/types.h>

#include <stdbool.h>
#include <stddef.h>

/*
 * A single "banco unificado de dispositivos" (project.md, Fase 4): every
 * device seen during Classic inquiry or LE scanning is recorded once per
 * address, updated in place on repeated sightings (duplicate filtering),
 * and flagged as dual-mode if it's been seen on both. Fixed-size, no
 * allocation.
 *
 * Known limitation: LE devices using resolvable private addresses will
 * show up as distinct entries each time their address rotates, and won't
 * be matched against a Classic sighting of the same physical device --
 * proper identity resolution needs IRK/bonding data from SMP, out of
 * scope here.
 */

#define BT_DEVICE_FLAG_CLASSIC (1u << 0)
#define BT_DEVICE_FLAG_LE (1u << 1)
/* Human interface device: a keyboard, a mouse or a combination of the two.
 * Set from the Classic class-of-device or from LE advertising data; either
 * sighting is enough, and a dual-mode device usually says so on both. */
#define BT_DEVICE_FLAG_HID (1u << 2)
/* Long enough for the names devices actually advertise; the field is truncated
 * rather than the entry rejected. */
#define BT_DEVICE_NAME_LEN 25

struct bt_discovered_device
{
    struct bt_addr addr;
    unsigned flags; /* BT_DEVICE_FLAG_* bitmask; both set means dual-mode */
    uint32_t class_of_device; /* meaningful iff BT_DEVICE_FLAG_CLASSIC is set */
    uint8_t le_address_type;  /* meaningful iff BT_DEVICE_FLAG_LE is set */
    uint16_t appearance;      /* LE appearance, 0 when not advertised */
    /*
     * A name, and how much it is worth.
     *
     * 0 nothing, 1 a label this code synthesized from appearance or class of
     * device, 2 a name the device gave. The state exists so a real name is
     * never overwritten by a label, and so a caller can tell "<mouse>" from a
     * device that calls itself that.
     *
     * A remote-name-request would be the third source and is deliberately not
     * here: the legacy port planned that phase, removed it after it did not
     * work, and shipped with exactly this ladder instead.
     */
    char name[BT_DEVICE_NAME_LEN];
    uint8_t name_state;
    int8_t last_rssi;
    uint32_t sightings;
};

#ifndef BT_DEVICE_REGISTRY_MAX
#define BT_DEVICE_REGISTRY_MAX 32
#endif


/*
 * count before devices[], deliberately.
 *
 * With the array first, count sits immediately after it, and any write that
 * strides through the array with the wrong element size lands on the count --
 * which reads afterwards as an empty registry rather than as corruption. That
 * is not hypothetical: it is what a partial rebuild produced after
 * bt_discovered_device grew, and "the scan listed devices and then reported
 * none" is a much harder symptom to place than a bad entry would have been.
 */
struct bt_device_registry
{
    size_t count;
    struct bt_discovered_device devices[BT_DEVICE_REGISTRY_MAX];
};

void bt_device_registry_init(struct bt_device_registry *reg);
size_t bt_device_registry_count(const struct bt_device_registry *reg);
const struct bt_discovered_device *bt_device_registry_get(const struct bt_device_registry *reg,
                                                            size_t index);
const struct bt_discovered_device *bt_device_registry_find(const struct bt_device_registry *reg,
                                                             const struct bt_addr *addr);

/* Records a Classic inquiry sighting, creating a new entry if addr is
 * unseen. Returns NULL only if the registry is full and addr is new. */
struct bt_discovered_device *bt_device_registry_note_classic(struct bt_device_registry *reg,
                                                               const struct bt_addr *addr,
                                                               uint32_t class_of_device);

/* Records an LE advertising sighting, creating a new entry if addr is
 * unseen. Returns NULL only if the registry is full and addr is new. */
/* True if a Classic class-of-device describes a keyboard or pointing device.
 * Major device class 5 is Peripheral; within it bit 6 is keyboard and bit 7 is
 * pointing, and a combo device sets both. */
bool bt_cod_is_hid(uint32_t class_of_device);

/* True if this LE address will still name the same device in ten minutes:
 * public, or static random. Resolvable and non-resolvable private addresses
 * rotate and cannot be an identity without an IRK from bonding. */
bool bt_le_addr_is_stable(const struct bt_addr *addr, uint8_t address_type);

/* True if an LE advertising payload announces HID: the HID-over-GATT service
 * UUID 0x1812 in either the complete or the incomplete 16-bit UUID list, or an
 * Appearance whose category is 15 (HID). Writes the appearance out when it is
 * present, so a caller can keep it whether or not it decided HID. */
/* What an advertising payload says about a device. Every field is optional;
 * `name` points into `data` and is not NUL-terminated. */
struct bt_le_adv_info
{
    bool hid;
    uint16_t appearance;
    const uint8_t *name;
    size_t name_len;
    bool name_complete;   /* EIR 0x09 rather than the shortened 0x08 */
    bool has_flags;       /* AD type 0x01 (Flags) was present */
    uint8_t flags;        /* bit0 LE limited discoverable, bit1 LE general discoverable */
};

void bt_le_adv_parse(const uint8_t *data, size_t length, struct bt_le_adv_info *out);

bool bt_le_adv_is_hid(const uint8_t *data, size_t length, uint16_t *appearance_out);

/* Record a name the device gave (state 2) or a label we made up (state 1). A
 * label never replaces a real name. */
void bt_device_set_name(struct bt_discovered_device *dev, const char *name,
                        size_t length, uint8_t state);

/* "<keyboard>", "<mouse>", "<HID device>" ... or NULL when nothing is known. */
const char *bt_label_from_appearance(uint16_t appearance);
const char *bt_label_from_cod(uint32_t class_of_device);

struct bt_discovered_device *bt_device_registry_note_le(struct bt_device_registry *reg,
                                                          const struct bt_addr *addr,
                                                          uint8_t le_address_type, int8_t rssi);

#endif /* BTCORE_DEVICE_REGISTRY_H */
