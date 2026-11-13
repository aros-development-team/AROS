#include "test_device_registry.h"
#include "../support/test.h"

#include <btcore/device_registry.h>

static struct bt_addr make_addr(uint8_t last_byte)
{
    struct bt_addr a = {{0x11, 0x22, 0x33, 0x44, 0x55, last_byte}};
    return a;
}

static void test_empty_registry(void)
{
    struct bt_device_registry reg;
    struct bt_addr a = make_addr(0x01);

    bt_device_registry_init(&reg);
    BT_CHECK(bt_device_registry_count(&reg) == 0);
    BT_CHECK(bt_device_registry_find(&reg, &a) == NULL);
    BT_CHECK(bt_device_registry_get(&reg, 0) == NULL);
}

static void test_classic_duplicate_filtering(void)
{
    struct bt_device_registry reg;
    struct bt_addr a = make_addr(0x01);
    struct bt_discovered_device *dev;

    bt_device_registry_init(&reg);

    dev = bt_device_registry_note_classic(&reg, &a, 0x1234);
    BT_CHECK(dev != NULL);
    BT_CHECK(bt_device_registry_count(&reg) == 1);
    BT_CHECK((dev->flags & BT_DEVICE_FLAG_CLASSIC) != 0);
    BT_CHECK((dev->flags & BT_DEVICE_FLAG_LE) == 0);
    BT_CHECK(dev->class_of_device == 0x1234);
    BT_CHECK(dev->sightings == 1);

    /* Same address seen again: updated in place, not duplicated. */
    dev = bt_device_registry_note_classic(&reg, &a, 0x5678);
    BT_CHECK(bt_device_registry_count(&reg) == 1);
    BT_CHECK(dev->class_of_device == 0x5678);
    BT_CHECK(dev->sightings == 2);
}

static void test_dual_mode_detection(void)
{
    struct bt_device_registry reg;
    struct bt_addr a = make_addr(0x01);
    struct bt_discovered_device *dev;

    bt_device_registry_init(&reg);

    bt_device_registry_note_classic(&reg, &a, 0x1234);
    dev = bt_device_registry_note_le(&reg, &a, 0x00, -42);

    BT_CHECK(bt_device_registry_count(&reg) == 1); /* same device, not two entries */
    BT_CHECK((dev->flags & BT_DEVICE_FLAG_CLASSIC) != 0);
    BT_CHECK((dev->flags & BT_DEVICE_FLAG_LE) != 0);
    BT_CHECK(dev->last_rssi == -42);
}

static void test_distinct_addresses(void)
{
    struct bt_device_registry reg;
    struct bt_addr a = make_addr(0x01);
    struct bt_addr b = make_addr(0x02);

    bt_device_registry_init(&reg);
    bt_device_registry_note_le(&reg, &a, 0x00, -50);
    bt_device_registry_note_le(&reg, &b, 0x01, -60);

    BT_CHECK(bt_device_registry_count(&reg) == 2);
    BT_CHECK(bt_device_registry_find(&reg, &a) != NULL);
    BT_CHECK(bt_device_registry_find(&reg, &b) != NULL);
}

static void test_registry_full(void)
{
    struct bt_device_registry reg;
    int i;

    bt_device_registry_init(&reg);

    for (i = 0; i < BT_DEVICE_REGISTRY_MAX; i++)
    {
        struct bt_addr a = make_addr((uint8_t)i);
        BT_CHECK(bt_device_registry_note_le(&reg, &a, 0x00, 0) != NULL);
    }
    BT_CHECK(bt_device_registry_count(&reg) == BT_DEVICE_REGISTRY_MAX);

    {
        struct bt_addr overflow = make_addr(0xff);
        BT_CHECK(bt_device_registry_note_le(&reg, &overflow, 0x00, 0) == NULL);
        BT_CHECK(bt_device_registry_count(&reg) == BT_DEVICE_REGISTRY_MAX);
    }

    /* An address already present must still update fine even when full. */
    {
        struct bt_addr existing = make_addr(0x00);
        BT_CHECK(bt_device_registry_note_le(&reg, &existing, 0x00, -10) != NULL);
        BT_CHECK(bt_device_registry_count(&reg) == BT_DEVICE_REGISTRY_MAX);
    }
}

void run_device_registry_tests(void)
{
    test_empty_registry();
    test_classic_duplicate_filtering();
    test_dual_mode_detection();
    test_distinct_addresses();
    test_registry_full();
}
