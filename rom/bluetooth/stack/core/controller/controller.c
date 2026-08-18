#include <btcore/controller.h>

#include <string.h>

/* Set by the port; null means no tracing, which is the default. Bytes rather
 * than a format string, because a freestanding core has no vprintf to forward
 * varargs to and the port is the only side that can render anything. */
void (*bt_hci_raw_hook)(const char *what, const uint8_t *data, size_t length);

static void on_reset_complete(struct bt_cmdq_completion *completion, void *user_data);
static void on_version_complete(struct bt_cmdq_completion *completion, void *user_data);
static void on_features_complete(struct bt_cmdq_completion *completion, void *user_data);
static void on_buffer_size_complete(struct bt_cmdq_completion *completion, void *user_data);

void bt_controller_init(struct bt_controller *ctrl, struct bt_hci_transport *transport)
{
    ctrl->transport = transport;
    ctrl->state = BT_CONTROLLER_STATE_UNINITIALIZED;
    memset(&ctrl->info, 0, sizeof(ctrl->info));
    bt_timer_list_init(&ctrl->timers);
    bt_cmdq_init(&ctrl->cmdq, transport, &ctrl->timers);
    bt_device_registry_init(&ctrl->devices);
}

bt_status_t bt_controller_start(struct bt_controller *ctrl, uint64_t now_us)
{
    bt_status_t st;

    if (ctrl->state != BT_CONTROLLER_STATE_UNINITIALIZED)
        return BT_ERR_INVALID_ARGUMENT;

    st = bt_cmdq_submit(&ctrl->cmdq, BT_HCI_OPCODE_RESET, NULL, 0, 0, on_reset_complete, ctrl);
    if (st != BT_OK)
        return st;

    ctrl->state = BT_CONTROLLER_STATE_RESETTING;
    bt_cmdq_pump(&ctrl->cmdq, now_us);
    return BT_OK;
}

void bt_controller_on_event(struct bt_controller *ctrl, const uint8_t *data, size_t length,
                             uint64_t now_us)
{
    struct bt_buf_reader r;
    struct bt_hci_event_header hdr;
    const uint8_t *params;

    /* Command Complete/Status for whatever this controller has
     * outstanding, regardless of event type below. */
    bt_cmdq_on_event(&ctrl->cmdq, data, length, now_us);

    /* Independently inspect the same event for discovery events. These
     * never share an event code with Command Complete/Status, so
     * re-parsing the header here is safe and keeps cmdq ignorant of
     * anything beyond generic command completion. */
    bt_buf_reader_init(&r, data, length);
    if (bt_hci_parse_event_header(&r, &hdr) != BT_OK)
        return;

    params = bt_buf_reader_peek(&r, hdr.param_len);
    if (params == NULL)
        return;

    if (hdr.event_code == BT_HCI_EVENT_INQUIRY_RESULT)
    {
        struct bt_hci_inquiry_result_iter it;
        struct bt_hci_inquiry_result_entry entry;

        /*
         * The raw event, once.
         *
         * Two Classic entries have come back with addresses sharing a 12:34
         * prefix across every run, while their classes of device were stable
         * and sensible. Reading the parser twice has not explained that, and
         * two attempts to fix it from the layout alone were wrong -- so print
         * what actually arrived instead of proposing a third.
         *
         * Remove once the addresses are understood; this is a probe, not
         * telemetry.
         */
        if (ctrl->inquiry_traced < 2u && bt_hci_raw_hook != NULL)
        {
            ctrl->inquiry_traced++;
            bt_hci_raw_hook("inquiry", params, hdr.param_len);
        }

        if (bt_hci_inquiry_result_iter_init(&it, params, hdr.param_len) != BT_OK)
            return;
        while (bt_hci_inquiry_result_iter_next(&it, &entry) == BT_OK)
            bt_device_registry_note_classic(&ctrl->devices, &entry.bd_addr, entry.class_of_device);
    }
    else if (hdr.event_code == BT_HCI_EVENT_EXTENDED_INQUIRY_RESULT)
    {
        /*
         * One response, and a layout of its own: address, page-scan repetition
         * mode, ONE reserved byte where the plain result has two, class of
         * device, clock offset, RSSI, then 240 bytes of EIR.
         */
        struct bt_addr addr;
        uint32_t cod;
        int8_t rssi;
        size_t i;

        if (hdr.param_len < 15u)
            return;
        for (i = 0; i < BT_ADDR_LEN; i++)
            addr.b[i] = params[1u + i];
        cod = (uint32_t)params[10] | ((uint32_t)params[11] << 8)
            | ((uint32_t)params[12] << 16);
        rssi = (int8_t)params[15];

        {
            struct bt_discovered_device *dev =
                bt_device_registry_note_classic(&ctrl->devices, &addr, cod);
            struct bt_le_adv_info info;

            if (dev == NULL)
                return;
            dev->last_rssi = rssi;
            /* The EIR is advertising data in the same length-type-value form,
             * so the same parser reads the name out of it. */
            bt_le_adv_parse(params + 16, hdr.param_len - 16u, &info);
            if (info.hid)
                dev->flags |= BT_DEVICE_FLAG_HID;
            if (info.name != NULL)
                bt_device_set_name(dev, (const char *)info.name, info.name_len, 2);
        }
    }
    else if (hdr.event_code == BT_HCI_EVENT_LE_META)
    {
        struct bt_hci_le_adv_report_iter it;
        struct bt_hci_le_adv_report report;

        if (bt_hci_le_adv_report_iter_init(&it, params, hdr.param_len) != BT_OK)
            return; /* not an advertising-report subevent; nothing to do here */
        while (bt_hci_le_adv_report_iter_next(&it, &report) == BT_OK)
        {
            struct bt_discovered_device *dev;
            struct bt_le_adv_info info;

            bt_le_adv_parse(report.data, report.data_len, &info);

            /*
             * Do not create an entry for an anonymous rotating address.
             *
             * A resolvable private address changes every few minutes, so every
             * neighbouring phone becomes a parade of devices that are each seen
             * once and never again -- the registry fills with strangers and the
             * two devices someone actually owns are lost in it.
             *
             * An entry is worth creating when the address is a stable identity
             * -- public, or static random -- or when the advertisement says who
             * it is: a name, the HID service, or an appearance. A device that
             * announces nothing and will not be there next minute is noise.
             *
             * An address already known is always updated, so a device that
             * earns an entry keeps it even when a later advert is bare.
             */
            if (bt_device_registry_find(&ctrl->devices, &report.address) == NULL &&
                !bt_le_addr_is_stable(&report.address, report.address_type) &&
                info.name == NULL && !info.hid && info.appearance == 0)
                continue;

            dev = bt_device_registry_note_le(&ctrl->devices, &report.address,
                                             report.address_type, report.rssi);
            if (dev != NULL)
            {
                if (info.hid)
                    dev->flags |= BT_DEVICE_FLAG_HID;
                if (info.appearance != 0)
                {
                    dev->appearance = info.appearance;
                    bt_device_set_name(dev,
                                       bt_label_from_appearance(info.appearance),
                                       BT_DEVICE_NAME_LEN, 1);
                }
                if (info.name != NULL)
                    bt_device_set_name(dev, (const char *)info.name,
                                       info.name_len, 2);
            }

        }
    }
}

void bt_controller_tick(struct bt_controller *ctrl, uint64_t now_us)
{
    bt_cmdq_tick(&ctrl->cmdq, now_us);
}

static void ignore_completion(struct bt_cmdq_completion *completion, void *user_data)
{
    /* Inquiry/LE-scan-enable complete via Command Status; nothing further
     * to do here (results stream in separately as discovery events). A
     * failing status is silently dropped for now -- there's no discovery
     * error signal yet to surface it through. */
    (void)completion;
    (void)user_data;
}

/*
 * bt_cmdq_submit() wants raw command parameters (it encodes the
 * opcode+length header itself in bt_cmdq_pump()) -- so these build just
 * the parameter bytes with a plain writer, rather than going through
 * bt_hci_encode_inquiry()/bt_hci_encode_le_set_scan_*() in btcore/hci.h,
 * which produce a full ready-to-send command (header included) for
 * callers that talk to a transport directly.
 */

bt_status_t bt_controller_start_classic_inquiry(struct bt_controller *ctrl, uint8_t inquiry_length,
                                                 uint64_t now_us)
{
    uint8_t params[5];
    struct bt_buf_writer w;
    bt_status_t st;

    if (ctrl->state != BT_CONTROLLER_STATE_READY)
        return BT_ERR_INVALID_ARGUMENT;

    /*
     * Ask for RSSI and EIR in the results, every time.
     *
     * Without this the controller sends the plain Inquiry Result, which has
     * neither a name nor a signal strength -- an inquiry that returns an
     * address and a class of device and nothing else. Mode 2 makes it send the
     * Extended Inquiry Result instead, with the name inline.
     *
     * It is also the route that avoids Remote Name Request, which the legacy
     * port found crashed the BCM43430A1 -- though that was observed on a
     * controller running unpatched ROM firmware, so it is a reason to prefer
     * this and not a proof that the command is unusable.
     */
    {
        const uint8_t mode = BT_HCI_INQUIRY_MODE_RSSI_EIR;

        st = bt_cmdq_submit(&ctrl->cmdq, BT_HCI_OPCODE_WRITE_INQUIRY_MODE,
                             &mode, 1u, 0, ignore_completion, ctrl);
        if (st != BT_OK)
            return st;
    }

    bt_buf_writer_init(&w, params, sizeof(params));
    bt_buf_writer_write_le24(&w, BT_HCI_GIAC_LAP);
    bt_buf_writer_write_u8(&w, inquiry_length);
    bt_buf_writer_write_u8(&w, 0); /* num_responses: 0 = unlimited */

    st = bt_cmdq_submit(&ctrl->cmdq, BT_HCI_OPCODE_INQUIRY, params,
                         (uint8_t)bt_buf_writer_len(&w), 0, ignore_completion, ctrl);
    if (st != BT_OK)
        return st;

    bt_cmdq_pump(&ctrl->cmdq, now_us);
    return BT_OK;
}

bt_status_t bt_controller_start_le_scan(struct bt_controller *ctrl, uint64_t now_us)
{
    uint8_t params[7];
    struct bt_buf_writer w;
    bt_status_t st;

    if (ctrl->state != BT_CONTROLLER_STATE_READY)
        return BT_ERR_INVALID_ARGUMENT;

    bt_buf_writer_init(&w, params, sizeof(params));
    bt_buf_writer_write_u8(&w, 0x00);     /* passive scan */
    bt_buf_writer_write_le16(&w, 0x0010); /* scan interval */
    bt_buf_writer_write_le16(&w, 0x0010); /* scan window */
    bt_buf_writer_write_u8(&w, 0x00);     /* public own address */
    bt_buf_writer_write_u8(&w, 0x00);     /* no filter policy */

    st = bt_cmdq_submit(&ctrl->cmdq, BT_HCI_OPCODE_LE_SET_SCAN_PARAMETERS, params,
                         (uint8_t)bt_buf_writer_len(&w), 0, ignore_completion, ctrl);
    if (st != BT_OK)
        return st;

    bt_buf_writer_init(&w, params, sizeof(params));
    bt_buf_writer_write_u8(&w, 0x01); /* scan enable */
    bt_buf_writer_write_u8(&w, 0x01); /* filter duplicates */

    st = bt_cmdq_submit(&ctrl->cmdq, BT_HCI_OPCODE_LE_SET_SCAN_ENABLE, params,
                         (uint8_t)bt_buf_writer_len(&w), 0, ignore_completion, ctrl);
    if (st != BT_OK)
        return st;

    bt_cmdq_pump(&ctrl->cmdq, now_us);
    return BT_OK;
}

/*
 * Stop scanning, which on this hardware is a precondition for inquiry rather
 * than politeness: the CYW43438 shares one radio between BR/EDR and LE, so a
 * discovery that wants both has to alternate. A Classic keyboard that is
 * already bonded never advertises, and is only ever found by inquiry.
 */
bt_status_t bt_controller_stop_le_scan(struct bt_controller *ctrl, uint64_t now_us)
{
    uint8_t params[2];
    struct bt_buf_writer w;
    bt_status_t st;

    if (ctrl->state != BT_CONTROLLER_STATE_READY)
        return BT_ERR_INVALID_ARGUMENT;

    bt_buf_writer_init(&w, params, sizeof(params));
    bt_buf_writer_write_u8(&w, 0x00); /* scan disable */
    bt_buf_writer_write_u8(&w, 0x00); /* filter duplicates: ignored when off */

    st = bt_cmdq_submit(&ctrl->cmdq, BT_HCI_OPCODE_LE_SET_SCAN_ENABLE, params,
                         (uint8_t)bt_buf_writer_len(&w), 0, ignore_completion, ctrl);
    if (st != BT_OK)
        return st;

    bt_cmdq_pump(&ctrl->cmdq, now_us);
    return BT_OK;
}

static bool step_ok(struct bt_controller *ctrl, struct bt_cmdq_completion *completion)
{
    if (completion->result != BT_CMDQ_RESULT_COMPLETE || completion->status != 0x00)
    {
        ctrl->state = BT_CONTROLLER_STATE_ERROR;
        return false;
    }
    return true;
}

static void on_reset_complete(struct bt_cmdq_completion *completion, void *user_data)
{
    struct bt_controller *ctrl = (struct bt_controller *)user_data;

    if (!step_ok(ctrl, completion))
        return;

    ctrl->state = BT_CONTROLLER_STATE_READING_VERSION;
    if (bt_cmdq_submit(&ctrl->cmdq, BT_HCI_OPCODE_READ_LOCAL_VERSION_INFO, NULL, 0, 0,
                        on_version_complete, ctrl) != BT_OK)
        ctrl->state = BT_CONTROLLER_STATE_ERROR;
}

static void on_version_complete(struct bt_cmdq_completion *completion, void *user_data)
{
    struct bt_controller *ctrl = (struct bt_controller *)user_data;

    if (!step_ok(ctrl, completion))
        return;

    if (bt_hci_parse_local_version(completion->return_params, completion->return_params_len,
                                    &ctrl->info.version) != BT_OK)
    {
        ctrl->state = BT_CONTROLLER_STATE_ERROR;
        return;
    }

    ctrl->state = BT_CONTROLLER_STATE_READING_FEATURES;
    if (bt_cmdq_submit(&ctrl->cmdq, BT_HCI_OPCODE_READ_LOCAL_SUPPORTED_FEATURES, NULL, 0, 0,
                        on_features_complete, ctrl) != BT_OK)
        ctrl->state = BT_CONTROLLER_STATE_ERROR;
}

static void on_features_complete(struct bt_cmdq_completion *completion, void *user_data)
{
    struct bt_controller *ctrl = (struct bt_controller *)user_data;

    if (!step_ok(ctrl, completion))
        return;

    if (bt_hci_parse_local_features(completion->return_params, completion->return_params_len,
                                     &ctrl->info.features) != BT_OK)
    {
        ctrl->state = BT_CONTROLLER_STATE_ERROR;
        return;
    }

    ctrl->state = BT_CONTROLLER_STATE_READING_BUFFER_SIZE;
    if (bt_cmdq_submit(&ctrl->cmdq, BT_HCI_OPCODE_READ_BUFFER_SIZE, NULL, 0, 0,
                        on_buffer_size_complete, ctrl) != BT_OK)
        ctrl->state = BT_CONTROLLER_STATE_ERROR;
}

static void on_buffer_size_complete(struct bt_cmdq_completion *completion, void *user_data)
{
    struct bt_controller *ctrl = (struct bt_controller *)user_data;

    if (!step_ok(ctrl, completion))
        return;

    if (bt_hci_parse_buffer_size(completion->return_params, completion->return_params_len,
                                  &ctrl->info.buffer_size) != BT_OK)
    {
        ctrl->state = BT_CONTROLLER_STATE_ERROR;
        return;
    }

    ctrl->state = BT_CONTROLLER_STATE_READY;
}
