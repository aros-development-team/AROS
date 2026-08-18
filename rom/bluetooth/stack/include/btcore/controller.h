#ifndef BTCORE_CONTROLLER_H
#define BTCORE_CONTROLLER_H

#include <btcore/command_queue.h>
#include <btcore/device_registry.h>
#include <btcore/hci.h>
#include <btcore/status.h>
#include <btcore/timer.h>
#include <btcore/transport.h>
#include <btcore/types.h>

/*
 * Drives the deterministic controller bring-up sequence from project.md's
 * Fase 2 (Reset -> Read Local Version -> Read Local Supported Features ->
 * Read Buffer Size -> ready), on top of bt_cmdq. One command in flight at
 * a time, by construction of bt_cmdq itself.
 *
 * Owns a private bt_timer_list for its command queue's timeouts. Nothing
 * else shares timers yet, so this is fine for now; once a real event loop
 * (Bluetooth Manager Task or test-host equivalent) exists, it will likely
 * want to own one shared timer list instead.
 *
 * Caller is responsible for transport->ops->open() before
 * bt_controller_start(), and for feeding every received HCI Event to
 * bt_controller_on_event() (e.g. from the transport's recv callback).
 */

enum bt_controller_state
{
    BT_CONTROLLER_STATE_UNINITIALIZED,
    BT_CONTROLLER_STATE_RESETTING,
    BT_CONTROLLER_STATE_READING_VERSION,
    BT_CONTROLLER_STATE_READING_FEATURES,
    BT_CONTROLLER_STATE_READING_BUFFER_SIZE,
    BT_CONTROLLER_STATE_READY,
    BT_CONTROLLER_STATE_ERROR
};

struct bt_controller_info
{
    struct bt_hci_local_version version;
    struct bt_hci_local_features features;
    struct bt_hci_buffer_size buffer_size;
};

/* Set by the port so core code can hand raw bytes out for inspection without
 * owning a console. Null means no tracing, which is the default. */
extern void (*bt_hci_raw_hook)(const char *what, const uint8_t *data, size_t length);

struct bt_controller
{
    struct bt_hci_transport *transport;
    struct bt_timer_list timers;
    struct bt_cmdq cmdq;
    enum bt_controller_state state;
    struct bt_controller_info info;
    struct bt_device_registry devices;
    unsigned inquiry_traced; /* Fase 4: unified Classic/LE discovery results */
};

void bt_controller_init(struct bt_controller *ctrl, struct bt_hci_transport *transport);

/* Submits HCI Reset and begins the bring-up sequence. Fails only if the
 * controller isn't in BT_CONTROLLER_STATE_UNINITIALIZED, or if the
 * underlying command queue has no free slot (shouldn't happen this early). */
bt_status_t bt_controller_start(struct bt_controller *ctrl, uint64_t now_us);

void bt_controller_on_event(struct bt_controller *ctrl, const uint8_t *data, size_t length,
                             uint64_t now_us);

/* Call periodically (or whenever the owning timer list has something due)
 * so a stalled bring-up command can time out and move to
 * BT_CONTROLLER_STATE_ERROR instead of hanging forever. */
void bt_controller_tick(struct bt_controller *ctrl, uint64_t now_us);

/*
 * Discovery (Fase 4). Both require BT_CONTROLLER_STATE_READY. Results
 * accumulate in ctrl->devices as Inquiry Result / LE Advertising Report
 * events arrive via bt_controller_on_event() -- there's no separate
 * "discovery complete" callback here yet; poll bt_device_registry_count()
 * or read ctrl->devices directly.
 */

/* inquiry_length is in 1.28s units (project.md / HCI spec unit). */
bt_status_t bt_controller_start_classic_inquiry(struct bt_controller *ctrl, uint8_t inquiry_length,
                                                 uint64_t now_us);

/* Uses reasonable default scan parameters (passive, 10ms interval/window,
 * public own address, no filter, duplicate filtering on). */
bt_status_t bt_controller_stop_le_scan(struct bt_controller *ctrl, uint64_t now_us);

bt_status_t bt_controller_start_le_scan(struct bt_controller *ctrl, uint64_t now_us);

#endif /* BTCORE_CONTROLLER_H */
