#ifndef BTCORE_GATT_CLIENT_H
#define BTCORE_GATT_CLIENT_H

#include <btcore/att.h>
#include <btcore/l2cap_channel.h>
#include <btcore/status.h>
#include <btcore/types.h>

/*
 * Orchestrates GATT Client operations over the fixed ATT channel
 * (BT_L2CAP_CID_ATT), the LE analogue of btcore/sdp_client.h: opens
 * the channel, negotiates MTU, and drives service/characteristic
 * discovery loops (Read By Group Type / Read By Type, terminated by the
 * server's Attribute Not Found response, which is the protocol's normal
 * "no more results" signal, not treated as an error).
 *
 * Scope reductions, documented:
 *   - 16-bit UUIDs only for discovered services/characteristics (128-bit
 *     custom UUIDs are skipped during discovery, not reported).
 *   - Discovery results accumulate into fixed-size arrays
 *     (BT_GATT_CLIENT_MAX_SERVICES / _CHARACTERISTICS); a device with
 *     more doesn't get the rest.
 *   - GATT Server, and anything beyond Exchange MTU/discovery/Read/
 *     Write/notifications (e.g. Write Without Response, Reliable
 *     Writes and Long Write) aren't implemented. Read automatically
 *     continues with Read Blob up to the fixed result limit.
 */

#ifndef BT_GATT_CLIENT_MAX_SERVICES
#define BT_GATT_CLIENT_MAX_SERVICES 16
#endif

#ifndef BT_GATT_CLIENT_MAX_CHARACTERISTICS
#define BT_GATT_CLIENT_MAX_CHARACTERISTICS 16
#endif

#ifndef BT_GATT_CLIENT_MAX_VALUE_LEN
#define BT_GATT_CLIENT_MAX_VALUE_LEN 512
#endif

#ifndef BT_GATT_CLIENT_MAX_DESCRIPTORS
#define BT_GATT_CLIENT_MAX_DESCRIPTORS 32
#endif

#ifndef BT_GATT_CLIENT_REQUEST_MTU
#define BT_GATT_CLIENT_REQUEST_MTU 247 /* a common modern client preference */
#endif

#ifndef BT_GATT_CLIENT_REQUEST_TIMEOUT_US
#define BT_GATT_CLIENT_REQUEST_TIMEOUT_US 30000000ULL
#endif

struct bt_gatt_service
{
    uint16_t start_handle;
    uint16_t end_handle;
    uint16_t uuid16;
};

struct bt_gatt_characteristic
{
    uint16_t declaration_handle;
    uint16_t value_handle;
    uint8_t properties;
    uint16_t uuid16;
};

enum bt_gatt_client_op
{
    BT_GATT_CLIENT_OP_MTU,
    BT_GATT_CLIENT_OP_DISCOVER_SERVICES,
    BT_GATT_CLIENT_OP_DISCOVER_CHARACTERISTICS,
    BT_GATT_CLIENT_OP_DISCOVER_DESCRIPTORS,
    BT_GATT_CLIENT_OP_READ,
    BT_GATT_CLIENT_OP_WRITE
};

enum bt_gatt_client_result
{
    BT_GATT_CLIENT_OK,
    BT_GATT_CLIENT_ERROR_CONNECT,
    BT_GATT_CLIENT_ERROR_CLOSED,
    BT_GATT_CLIENT_ERROR_PROTOCOL,
    BT_GATT_CLIENT_ERROR_TOO_LARGE,
    BT_GATT_CLIENT_ERROR_TIMEOUT,
    BT_GATT_CLIENT_ERROR_ATT /* server returned an ATT Error Response; see att_error_code */
};

struct bt_gatt_client_completion
{
    enum bt_gatt_client_result result;
    enum bt_gatt_client_op op;
    uint8_t att_error_code; /* meaningful only for BT_GATT_CLIENT_ERROR_ATT */

    /* DISCOVER_SERVICES: services[0..count), DISCOVER_CHARACTERISTICS:
     * characteristics[0..count), READ: value[0..value_len). Unused
     * fields for a given op are left zeroed. */
    const struct bt_gatt_service *services;
    const struct bt_gatt_characteristic *characteristics;
    const struct bt_gatt_descriptor *descriptors;
    size_t count;
    const uint8_t *value;
    size_t value_len;
};

struct bt_gatt_descriptor
{
    uint16_t handle;
    uint16_t uuid16;
};

typedef void (*bt_gatt_client_complete_fn)(struct bt_gatt_client_completion *completion,
                                            void *user_data);
typedef void (*bt_gatt_client_connect_fn)(bool success, void *user_data);
typedef void (*bt_gatt_client_notify_fn)(uint16_t handle, const uint8_t *value, size_t value_len,
                                          bool is_indication, void *user_data);

struct bt_gatt_client
{
    struct bt_l2cap_channel_manager *l2cap;
    bool channel_ready;
    uint16_t mtu;

    bt_gatt_client_connect_fn on_connect;
    void *connect_user_data;

    bt_gatt_client_notify_fn on_notify;
    void *notify_user_data;

    bool busy;
    enum bt_gatt_client_op op;
    uint64_t connect_started_us;
    uint64_t request_deadline_us;
    uint64_t event_now_us;

    /* Cursor for discovery loops (next handle to resume from, and the
     * end of the range being searched); doubles as the single target
     * handle for READ/WRITE, which need only the first field. */
    uint16_t pending_handle;
    uint16_t discover_range_end;

    union
    {
        struct bt_gatt_service services[BT_GATT_CLIENT_MAX_SERVICES];
        struct bt_gatt_characteristic characteristics[BT_GATT_CLIENT_MAX_CHARACTERISTICS];
        struct bt_gatt_descriptor descriptors[BT_GATT_CLIENT_MAX_DESCRIPTORS];
        uint8_t value[BT_GATT_CLIENT_MAX_VALUE_LEN];
    } result;
    size_t result_count;
    size_t result_len;

    bt_gatt_client_complete_fn on_complete;
    void *complete_user_data;
};

void bt_gatt_client_init(struct bt_gatt_client *client, struct bt_l2cap_channel_manager *l2cap);

/* Opens the fixed ATT channel and negotiates MTU; on_connect fires once
 * that's done (an ATT Error Response to Exchange MTU just means the peer
 * keeps BT_ATT_DEFAULT_MTU -- still reported as success). */
bt_status_t bt_gatt_client_connect(struct bt_gatt_client *client, bt_gatt_client_connect_fn on_connect,
                                    void *user_data, uint64_t now_us);

void bt_gatt_client_disconnect(struct bt_gatt_client *client, uint64_t now_us);

/* Registers a handler for server-initiated Handle Value Notification/
 * Indication -- delivered any time, independent of client->busy. Confirms
 * indications automatically (Handle Value Confirmation). */
void bt_gatt_client_set_notify_handler(struct bt_gatt_client *client, bt_gatt_client_notify_fn fn,
                                        void *user_data);

bt_status_t bt_gatt_client_discover_services(struct bt_gatt_client *client,
                                              bt_gatt_client_complete_fn on_complete,
                                              void *user_data, uint64_t now_us);

bt_status_t bt_gatt_client_discover_characteristics(struct bt_gatt_client *client,
                                                      uint16_t service_start_handle,
                                                      uint16_t service_end_handle,
                                                      bt_gatt_client_complete_fn on_complete,
                                                      void *user_data, uint64_t now_us);

bt_status_t bt_gatt_client_discover_descriptors(struct bt_gatt_client *client,
                                                 uint16_t start_handle,
                                                 uint16_t end_handle,
                                                 bt_gatt_client_complete_fn on_complete,
                                                 void *user_data, uint64_t now_us);

bt_status_t bt_gatt_client_read(struct bt_gatt_client *client, uint16_t handle,
                                 bt_gatt_client_complete_fn on_complete, void *user_data,
                                 uint64_t now_us);

bt_status_t bt_gatt_client_write(struct bt_gatt_client *client, uint16_t handle,
                                  const uint8_t *value, size_t value_len,
                                  bt_gatt_client_complete_fn on_complete, void *user_data,
                                  uint64_t now_us);

/* Must be called by the owning event loop as time advances. Times out the
 * current ATT transaction; late responses are ignored. */
void bt_gatt_client_tick(struct bt_gatt_client *client, uint64_t now_us);

#endif /* BTCORE_GATT_CLIENT_H */
