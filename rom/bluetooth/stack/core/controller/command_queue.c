#include <btcore/command_queue.h>

#include <string.h>

static void complete_slot(struct bt_cmdq *q, struct bt_cmdq_slot *slot, enum bt_cmdq_result result,
                           uint8_t status, const uint8_t *return_params, size_t return_params_len,
                           uint64_t now_us)
{
    struct bt_cmdq_completion completion;
    bt_cmdq_complete_fn cb = slot->on_complete;
    void *user_data = slot->user_data;

    if (slot->timeout_timer.pending)
        bt_timer_list_cancel(q->timers, &slot->timeout_timer);

    if (q->outstanding == slot)
        q->outstanding = NULL;

    slot->in_use = false;

    completion.result = result;
    completion.opcode = slot->opcode;
    completion.status = status;
    completion.return_params = return_params;
    completion.return_params_len = return_params_len;

    if (cb != NULL)
        cb(&completion, user_data);

    bt_cmdq_pump(q, now_us);
}

static void timeout_callback(struct bt_timer *timer, void *user_data)
{
    struct bt_cmdq_slot *slot = (struct bt_cmdq_slot *)user_data;
    struct bt_cmdq *q = slot->owner;

    (void)timer;

    /* A missed response doesn't tell us whether the controller is wedged
     * or just slow; assume it can still take one more command so a single
     * timeout doesn't permanently deadlock the queue. A real deployment
     * may instead want to force a full HCI Reset after N consecutive
     * timeouts -- left for whoever drives this queue to decide. */
    q->command_credits = 1;
    complete_slot(q, slot, BT_CMDQ_RESULT_TIMEOUT, 0xff, NULL, 0, q->last_tick_us);
}

void bt_cmdq_init(struct bt_cmdq *q, struct bt_hci_transport *transport,
                   struct bt_timer_list *timers)
{
    size_t i;

    q->transport = transport;
    q->timers = timers;
    bt_queue_init(&q->pending);
    q->outstanding = NULL;
    q->command_credits = 1; /* HCI convention: assume 1 credit before the first response */
    q->last_tick_us = 0;

    for (i = 0; i < BT_CMDQ_MAX_PENDING; i++)
    {
        struct bt_cmdq_slot *slot = &q->pool[i];

        slot->in_use = false;
        slot->owner = q;
        bt_timer_init(&slot->timeout_timer, timeout_callback, slot);
    }
}

bt_status_t bt_cmdq_submit(struct bt_cmdq *q, uint16_t opcode, const uint8_t *params,
                            uint8_t params_len, uint64_t timeout_us,
                            bt_cmdq_complete_fn on_complete, void *user_data)
{
    struct bt_cmdq_slot *slot = NULL;
    size_t i;

    if (params_len > 0 && params == NULL)
        return BT_ERR_INVALID_ARGUMENT;

    for (i = 0; i < BT_CMDQ_MAX_PENDING; i++)
    {
        if (!q->pool[i].in_use)
        {
            slot = &q->pool[i];
            break;
        }
    }
    if (slot == NULL)
        return BT_ERR_NO_RESOURCES;

    slot->in_use = true;
    slot->opcode = opcode;
    slot->params_len = params_len;
    if (params_len > 0)
        memcpy(slot->params, params, params_len);
    slot->timeout_us = (timeout_us != 0) ? timeout_us : BT_CMDQ_DEFAULT_TIMEOUT_US;
    slot->on_complete = on_complete;
    slot->user_data = user_data;

    bt_queue_push(&q->pending, &slot->node);
    return BT_OK;
}

void bt_cmdq_pump(struct bt_cmdq *q, uint64_t now_us)
{
    struct bt_cmdq_slot *slot;
    struct bt_buf_writer w;
    uint8_t wire[BT_HCI_COMMAND_HEADER_LEN + BT_HCI_MAX_PARAM_LEN];
    int rc;

    /*
     * Advance the queue's clock here too, not only in bt_cmdq_tick().
     *
     * last_tick_us is what a transport callback has to stamp an inbound event
     * with, having no clock of its own -- see manager_receive(). It started at
     * zero and was written only by the tick, so a reply that arrived before the
     * first tick was dated to the epoch. Whatever the completion handler then
     * submitted got a deadline five seconds after the epoch, and the first real
     * tick -- a GetSysTime() value around 10^15 -- timed it out at once.
     *
     * The symptom was a controller that reset, received a correct Command
     * Complete, and went back to UNINITIALIZED one tick later. Intermittent by
     * construction: it depended on whether the controller answered before or
     * after the first tick.
     *
     * Monotonic, so a caller that passes a stale or zero timestamp cannot drag
     * the queue backwards.
     */
    if (now_us > q->last_tick_us)
        q->last_tick_us = now_us;

    if (q->outstanding != NULL)
        return; /* one command in flight at a time */
    if (q->command_credits == 0)
        return;
    if (bt_queue_is_empty(&q->pending))
        return;

    slot = (struct bt_cmdq_slot *)bt_queue_pop(&q->pending);

    bt_buf_writer_init(&w, wire, sizeof(wire));
    if (bt_hci_encode_command(&w, slot->opcode, slot->params, slot->params_len) != BT_OK)
    {
        complete_slot(q, slot, BT_CMDQ_RESULT_SEND_ERROR, 0xff, NULL, 0, now_us);
        return;
    }

    /* Mark this command outstanding and arm its timeout *before* handing
     * bytes to the transport. A transport is free to call back into
     * bt_cmdq_on_event() from inside send_command() itself (the virtual
     * transport does exactly that, and a real synchronous/polling
     * transport plausibly could too) -- which can recursively complete
     * this command, and cascade through several more queued behind it,
     * before send_command() returns. Setting state afterwards would
     * clobber whatever that recursive processing already did. */
    q->command_credits--;
    q->outstanding = slot;
    bt_timer_list_add(q->timers, &slot->timeout_timer, now_us + slot->timeout_us);

    rc = q->transport->ops->send_command(q->transport, wire, bt_buf_writer_len(&w));
    if (rc != 0 && q->outstanding == slot)
    {
        /* Still outstanding as itself: no recursive completion happened,
         * so this failure is genuinely ours to handle. Restore the credit
         * -- the controller never actually saw this command. */
        q->command_credits++;
        complete_slot(q, slot, BT_CMDQ_RESULT_SEND_ERROR, 0xff, NULL, 0, now_us);
    }
}

void bt_cmdq_on_event(struct bt_cmdq *q, const uint8_t *data, size_t length, uint64_t now_us)
{
    struct bt_buf_reader r;
    struct bt_hci_event_header hdr;

    bt_buf_reader_init(&r, data, length);
    if (bt_hci_parse_event_header(&r, &hdr) != BT_OK)
        return;

    if (hdr.event_code == BT_HCI_EVENT_COMMAND_COMPLETE)
    {
        struct bt_hci_command_complete cc;

        if (bt_hci_parse_command_complete(&r, hdr.param_len, &cc) != BT_OK)
            return;

        q->command_credits = cc.num_hci_command_packets;

        if (q->outstanding != NULL && q->outstanding->opcode == cc.command_opcode)
        {
            uint8_t status = (cc.return_params_len >= 1) ? cc.return_params[0] : 0xff;

            complete_slot(q, q->outstanding, BT_CMDQ_RESULT_COMPLETE, status, cc.return_params,
                          cc.return_params_len, now_us);
        }
        else
        {
            bt_cmdq_pump(q, now_us);
        }
    }
    else if (hdr.event_code == BT_HCI_EVENT_COMMAND_STATUS)
    {
        struct bt_hci_command_status cs;

        if (bt_hci_parse_command_status(&r, hdr.param_len, &cs) != BT_OK)
            return;

        q->command_credits = cs.num_hci_command_packets;

        /*
         * Command Status always frees the command-credit slot, whether it
         * reports success or failure: it's the controller's ack that it
         * has processed this command request (Num_HCI_Command_Packets is
         * already updated accordingly). Some commands (Inquiry, Create
         * Connection, ...) only ever complete via Command Status and then
         * run in the background, signalling their real outcome later
         * through a distinct, business-logic-level event (Inquiry
         * Complete, Connection Complete) that isn't tied to
         * command_opcode at all -- if this queue instead waited for a
         * matching Command Complete before freeing such a slot, it would
         * stay "outstanding" forever and deadlock every later command.
         * Tracking that background completion, if a caller cares, is up
         * to whoever submitted the command (see bt_controller's discovery
         * support), not this queue.
         */
        if (q->outstanding != NULL && q->outstanding->opcode == cs.command_opcode)
            complete_slot(q, q->outstanding, BT_CMDQ_RESULT_COMPLETE, cs.status, NULL, 0, now_us);
        else
            bt_cmdq_pump(q, now_us);
    }
    /* other event types are not this module's concern */
}

void bt_cmdq_tick(struct bt_cmdq *q, uint64_t now_us)
{
    struct bt_timer *t;

    if (now_us > q->last_tick_us)
        q->last_tick_us = now_us;
    while ((t = bt_timer_list_pop_expired(q->timers, now_us)) != NULL)
        t->callback(t, t->user_data);

    bt_cmdq_pump(q, now_us);
}
