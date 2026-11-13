#include "dummy_vendor_init.h"

static bool dummy_matches_usb_id(uint16_t vendor_id, uint16_t product_id)
{
    (void)vendor_id;
    (void)product_id;
    return false;
}

static bt_status_t dummy_run(struct bt_hci_transport *transport,
                             const uint8_t *firmware_data,
                             size_t firmware_length)
{
    (void)transport;
    (void)firmware_data;
    (void)firmware_length;
    return BT_OK;
}

const struct bt_vendor_init_ops bt_vendor_init_dummy_ops = {
    "dummy",
    dummy_matches_usb_id,
    dummy_run
};
