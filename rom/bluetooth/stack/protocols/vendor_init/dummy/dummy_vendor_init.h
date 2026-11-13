#ifndef BLUETOOTH_VENDOR_INIT_DUMMY_H
#define BLUETOOTH_VENDOR_INIT_DUMMY_H

#include <btcore/vendor_init.h>

/* Reference module for controllers that need no vendor bring-up (e.g. CSR
 * chips, or the virtual test transport): run() is a no-op that always
 * succeeds. matches_usb_id() always returns false -- this module is never
 * auto-selected by a dispatcher. A port wires it in explicitly when it
 * already knows no firmware step applies, or a test uses it to exercise the
 * bt_vendor_init_ops call site without needing real firmware. */
extern const struct bt_vendor_init_ops bt_vendor_init_dummy_ops;

#endif /* BLUETOOTH_VENDOR_INIT_DUMMY_H */
