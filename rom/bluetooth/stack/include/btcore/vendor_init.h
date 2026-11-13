#ifndef BTCORE_VENDOR_INIT_H
#define BTCORE_VENDOR_INIT_H

#include <btcore/status.h>
#include <btcore/transport.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Vendor-specific controller bring-up (firmware download) that must run
 * before the generic HCI Reset sequence in core/controller. See
 * ai-context/deteccao-adaptador-firmware-propostas.md for the research this
 * is based on: every known vendor does this via ordinary HCI vendor-specific
 * commands (OGF 0x3F), so a module drives it over the same bt_hci_transport
 * as everything else -- it never needs raw USB/UART access.
 *
 * A module never bundles firmware bytes. The port layer locates and reads
 * them from the host filesystem (see protocols/vendor_init/README.md) and
 * passes them in. */
struct bt_vendor_init_ops
{
    const char *name;

    /* True if this module handles a controller identified by these USB
     * vendor/product IDs. Non-USB ports identify chips by whatever fits
     * that transport (board config, device tree, ...) and call the matching
     * module's run() directly instead of using this. */
    bool (*matches_usb_id)(uint16_t vendor_id, uint16_t product_id);

    /* Drives the vendor bring-up sequence over an already-open transport.
     * Must leave the controller ready for a normal HCI Reset afterwards. */
    bt_status_t (*run)(struct bt_hci_transport *transport,
                       const uint8_t *firmware_data, size_t firmware_length);
};

#endif /* BTCORE_VENDOR_INIT_H */
