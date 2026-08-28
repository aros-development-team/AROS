#include "test_command_queue.h"
#include "../support/test.h"

#include <btcore/buffer.h>
#include <btcore/command_queue.h>
#include <btcore/hci.h>

/*
 * A transport that only records what was sent -- it never answers on its
 * own. Tests inject Command Complete/Status events by hand via
 * bt_cmdq_on_event(), so ordering, credit accounting, and timeouts can be
 * driven precisely instead of depending on a simulated controller.
 */
struct fake_transport
{
    struct bt_hci_transport base;
    int send_count;
    uint16_t last_opcode;
    bool fail_send;
};

static int fake_open(struct bt_hci_transport *t)
{
    (void)t;
    return 0;
}

static void fake_close(struct bt_hci_transport *t)
{
    (void)t;
}

static int fake_send_command(struct bt_hci_transport *t, const uint8_t *data, size_t length)
{
    struct fake_transport *ft = (struct fake_transport *)t->impl;
    struct bt_buf_reader r;
    uint16_t opcode;
    uint8_t param_len;

    if (ft->fail_send)
        return -1;

    bt_buf_reader_init(&r, data, length);
    if (bt_buf_reader_read_le16(&r, &opcode) != BT_OK)
        return -1;
    if (bt_buf_reader_read_u8(&r, &param_len) != BT_OK)
        return -1;

    ft->send_count++;
    ft->last_opcode = opcode;
    return 0;
}

static int fake_send_unsupported(struct bt_hci_transport *t, const uint8_t *data, size_t length)
{
    (void)t;
    (void)data;
    (void)length;
    return -1;
}

static int fake_start_receive(struct bt_hci_transport *t, bt_hci_transport_recv_fn recv,
                               void *user_data)
{
    (void)t;
    (void)recv;
    (void)user_data;
    return 0;
}

static void fake_stop_receive(struct bt_hci_transport *t)
{
    (void)t;
}

static const struct bt_hci_transport_ops fake_ops = {
    .open = fake_open,
    .close = fake_close,
    .send_command = fake_send_command,
    .send_acl = fake_send_unsupported,
    .send_sco = fake_send_unsupported,
    .send_iso = fake_send_unsupported,
    .start_receive = fake_start_receive,
    .stop_receive = fake_stop_receive,
};

static void fake_transport_init(struct fake_transport *ft)
{
    ft->send_count = 0;
    ft->last_opcode = 0;
    ft->fail_send = false;
    ft->base.ops = &fake_ops;
    ft->base.impl = ft;
}

static size_t build_command_complete(uint8_t *buf, uint16_t opcode, uint8_t credits,
                                      uint8_t status)
{
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, buf, 16);
    bt_buf_writer_write_u8(&w, BT_HCI_EVENT_COMMAND_COMPLETE);
    bt_buf_writer_write_u8(&w, 4);
    bt_buf_writer_write_u8(&w, credits);
    bt_buf_writer_write_le16(&w, opcode);
    bt_buf_writer_write_u8(&w, status);
    return bt_buf_writer_len(&w);
}

static size_t build_command_status(uint8_t *buf, uint16_t opcode, uint8_t credits, uint8_t status)
{
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, buf, 16);
    bt_buf_writer_write_u8(&w, BT_HCI_EVENT_COMMAND_STATUS);
    bt_buf_writer_write_u8(&w, 4);
    bt_buf_writer_write_u8(&w, status);
    bt_buf_writer_write_u8(&w, credits);
    bt_buf_writer_write_le16(&w, opcode);
    return bt_buf_writer_len(&w);
}

struct completion_record
{
    int count;
    enum bt_cmdq_result result;
    uint16_t opcode;
    uint8_t status;
};

static void record_completion(struct bt_cmdq_completion *completion, void *user_data)
{
    struct completion_record *rec = (struct completion_record *)user_data;

    rec->count++;
    rec->result = completion->result;
    rec->opcode = completion->opcode;
    rec->status = completion->status;
}

static void test_submit_and_complete(void)
{
    struct fake_transport ft;
    struct bt_timer_list timers;
    struct bt_cmdq q;
    struct completion_record rec = {0, 0, 0, 0};
    uint8_t event[16];
    size_t event_len;

    fake_transport_init(&ft);
    bt_timer_list_init(&timers);
    bt_cmdq_init(&q, &ft.base, &timers);

    BT_CHECK(bt_cmdq_submit(&q, BT_HCI_OPCODE_RESET, NULL, 0, 0, record_completion, &rec) == BT_OK);
    bt_cmdq_pump(&q, 1000);

    BT_CHECK(ft.send_count == 1);
    BT_CHECK(ft.last_opcode == BT_HCI_OPCODE_RESET);
    BT_CHECK(q.command_credits == 0);
    BT_CHECK(rec.count == 0); /* not completed yet */

    event_len = build_command_complete(event, BT_HCI_OPCODE_RESET, 1, 0x00);
    bt_cmdq_on_event(&q, event, event_len, 1001);

    BT_CHECK(rec.count == 1);
    BT_CHECK(rec.result == BT_CMDQ_RESULT_COMPLETE);
    BT_CHECK(rec.opcode == BT_HCI_OPCODE_RESET);
    BT_CHECK(rec.status == 0x00);
    BT_CHECK(q.command_credits == 1);
    BT_CHECK(q.outstanding == NULL);
}

static void test_multiple_simultaneous_submits(void)
{
    struct fake_transport ft;
    struct bt_timer_list timers;
    struct bt_cmdq q;
    struct completion_record rec_a = {0, 0, 0, 0};
    struct completion_record rec_b = {0, 0, 0, 0};
    struct completion_record rec_c = {0, 0, 0, 0};
    uint8_t event[16];
    size_t event_len;

    fake_transport_init(&ft);
    bt_timer_list_init(&timers);
    bt_cmdq_init(&q, &ft.base, &timers);

    /* Three commands submitted before any of them has a chance to be
     * dispatched -- must be sent strictly one at a time. */
    BT_CHECK(bt_cmdq_submit(&q, BT_HCI_OPCODE_RESET, NULL, 0, 0, record_completion, &rec_a) ==
              BT_OK);
    BT_CHECK(bt_cmdq_submit(&q, BT_HCI_OPCODE_READ_LOCAL_VERSION_INFO, NULL, 0, 0,
                             record_completion, &rec_b) == BT_OK);
    BT_CHECK(bt_cmdq_submit(&q, BT_HCI_OPCODE_READ_BUFFER_SIZE, NULL, 0, 0, record_completion,
                             &rec_c) == BT_OK);

    bt_cmdq_pump(&q, 0);
    BT_CHECK(ft.send_count == 1);
    BT_CHECK(ft.last_opcode == BT_HCI_OPCODE_RESET);

    event_len = build_command_complete(event, BT_HCI_OPCODE_RESET, 1, 0x00);
    bt_cmdq_on_event(&q, event, event_len, 10);
    BT_CHECK(rec_a.count == 1);
    BT_CHECK(ft.send_count == 2);
    BT_CHECK(ft.last_opcode == BT_HCI_OPCODE_READ_LOCAL_VERSION_INFO);
    BT_CHECK(rec_b.count == 0);

    event_len = build_command_complete(event, BT_HCI_OPCODE_READ_LOCAL_VERSION_INFO, 1, 0x00);
    bt_cmdq_on_event(&q, event, event_len, 20);
    BT_CHECK(rec_b.count == 1);
    BT_CHECK(ft.send_count == 3);
    BT_CHECK(ft.last_opcode == BT_HCI_OPCODE_READ_BUFFER_SIZE);

    event_len = build_command_complete(event, BT_HCI_OPCODE_READ_BUFFER_SIZE, 1, 0x00);
    bt_cmdq_on_event(&q, event, event_len, 30);
    BT_CHECK(rec_c.count == 1);
    BT_CHECK(ft.send_count == 3); /* nothing left to send */
}

static void test_zero_credits_blocks_dispatch(void)
{
    struct fake_transport ft;
    struct bt_timer_list timers;
    struct bt_cmdq q;
    struct completion_record rec_a = {0, 0, 0, 0};
    struct completion_record rec_b = {0, 0, 0, 0};
    uint8_t event[16];
    size_t event_len;

    fake_transport_init(&ft);
    bt_timer_list_init(&timers);
    bt_cmdq_init(&q, &ft.base, &timers);

    BT_CHECK(bt_cmdq_submit(&q, BT_HCI_OPCODE_RESET, NULL, 0, 0, record_completion, &rec_a) ==
              BT_OK);
    BT_CHECK(bt_cmdq_submit(&q, BT_HCI_OPCODE_READ_BUFFER_SIZE, NULL, 0, 0, record_completion,
                             &rec_b) == BT_OK);
    bt_cmdq_pump(&q, 0);
    BT_CHECK(ft.send_count == 1);

    /* Controller reports zero credit: "don't send anything else right
     * now." rec_b must stay queued even though nothing is outstanding. */
    event_len = build_command_complete(event, BT_HCI_OPCODE_RESET, 0, 0x00);
    bt_cmdq_on_event(&q, event, event_len, 10);

    BT_CHECK(rec_a.count == 1);
    BT_CHECK(q.command_credits == 0);
    BT_CHECK(q.outstanding == NULL);
    BT_CHECK(ft.send_count == 1); /* still just the Reset */
    BT_CHECK(rec_b.count == 0);

    /* Credit recovers (e.g. a later, unrelated event reports it); pumping
     * again must now let the queued command through. */
    q.command_credits = 1;
    bt_cmdq_pump(&q, 20);
    BT_CHECK(ft.send_count == 2);
    BT_CHECK(ft.last_opcode == BT_HCI_OPCODE_READ_BUFFER_SIZE);
}

static void test_command_status_success_frees_slot(void)
{
    /* Stand-in for a command that only ever answers via Command Status
     * and then keeps running in the background, e.g. real HCI Inquiry. */
    const uint16_t status_only_opcode = 0x0401u;
    struct fake_transport ft;
    struct bt_timer_list timers;
    struct bt_cmdq q;
    struct completion_record rec_a = {0, 0, 0, 0};
    struct completion_record rec_b = {0, 0, 0, 0};
    uint8_t event[16];
    size_t event_len;

    fake_transport_init(&ft);
    bt_timer_list_init(&timers);
    bt_cmdq_init(&q, &ft.base, &timers);

    /* A successful status must still free the queue for the next command,
     * not wait for a Command Complete that will never come for this
     * opcode. */
    BT_CHECK(bt_cmdq_submit(&q, status_only_opcode, NULL, 0, 0, record_completion, &rec_a) ==
              BT_OK);
    BT_CHECK(bt_cmdq_submit(&q, BT_HCI_OPCODE_RESET, NULL, 0, 0, record_completion, &rec_b) ==
              BT_OK);
    bt_cmdq_pump(&q, 0);
    BT_CHECK(ft.send_count == 1);
    BT_CHECK(ft.last_opcode == status_only_opcode);

    event_len = build_command_status(event, status_only_opcode, 1, 0x00);
    bt_cmdq_on_event(&q, event, event_len, 10);

    BT_CHECK(rec_a.count == 1);
    BT_CHECK(rec_a.result == BT_CMDQ_RESULT_COMPLETE);
    BT_CHECK(rec_a.status == 0x00);
    /* The Inquiry slot was freed, which let the queued Reset dispatch
     * immediately -- so outstanding is Reset now, not NULL. */
    BT_CHECK(ft.send_count == 2);
    BT_CHECK(ft.last_opcode == BT_HCI_OPCODE_RESET);
}

static void test_timeout(void)
{
    struct fake_transport ft;
    struct bt_timer_list timers;
    struct bt_cmdq q;
    struct completion_record rec = {0, 0, 0, 0};

    fake_transport_init(&ft);
    bt_timer_list_init(&timers);
    bt_cmdq_init(&q, &ft.base, &timers);

    BT_CHECK(bt_cmdq_submit(&q, BT_HCI_OPCODE_RESET, NULL, 0, 100, record_completion, &rec) ==
              BT_OK);
    bt_cmdq_pump(&q, 0);
    BT_CHECK(ft.send_count == 1);

    /* Not due yet. */
    bt_cmdq_tick(&q, 50);
    BT_CHECK(rec.count == 0);
    BT_CHECK(q.outstanding != NULL);

    /* Due: fires exactly once, and the queue recovers (assumes 1 credit
     * again) instead of deadlocking. */
    bt_cmdq_tick(&q, 150);
    BT_CHECK(rec.count == 1);
    BT_CHECK(rec.result == BT_CMDQ_RESULT_TIMEOUT);
    BT_CHECK(rec.opcode == BT_HCI_OPCODE_RESET);
    BT_CHECK(q.outstanding == NULL);
    BT_CHECK(q.command_credits == 1);

    /* A late response after giving up must not double-complete. */
    uint8_t event[16];
    size_t event_len = build_command_complete(event, BT_HCI_OPCODE_RESET, 1, 0x00);
    bt_cmdq_on_event(&q, event, event_len, 200);
    BT_CHECK(rec.count == 1);
}

static void test_send_error_completes_immediately(void)
{
    struct fake_transport ft;
    struct bt_timer_list timers;
    struct bt_cmdq q;
    struct completion_record rec = {0, 0, 0, 0};

    fake_transport_init(&ft);
    ft.fail_send = true;
    bt_timer_list_init(&timers);
    bt_cmdq_init(&q, &ft.base, &timers);

    BT_CHECK(bt_cmdq_submit(&q, BT_HCI_OPCODE_RESET, NULL, 0, 0, record_completion, &rec) ==
              BT_OK);
    bt_cmdq_pump(&q, 0);

    BT_CHECK(ft.send_count == 0);
    BT_CHECK(rec.count == 1);
    BT_CHECK(rec.result == BT_CMDQ_RESULT_SEND_ERROR);
    BT_CHECK(q.outstanding == NULL);
}

static void test_pool_exhaustion(void)
{
    struct fake_transport ft;
    struct bt_timer_list timers;
    struct bt_cmdq q;
    struct completion_record recs[BT_CMDQ_MAX_PENDING];
    int i;

    fake_transport_init(&ft);
    bt_timer_list_init(&timers);
    bt_cmdq_init(&q, &ft.base, &timers);

    for (i = 0; i < BT_CMDQ_MAX_PENDING; i++)
    {
        recs[i].count = 0;
        BT_CHECK(bt_cmdq_submit(&q, BT_HCI_OPCODE_RESET, NULL, 0, 0, record_completion,
                                 &recs[i]) == BT_OK);
    }

    BT_CHECK(bt_cmdq_submit(&q, BT_HCI_OPCODE_RESET, NULL, 0, 0, record_completion, &recs[0]) ==
              BT_ERR_NO_RESOURCES);
}

void run_command_queue_tests(void)
{
    test_submit_and_complete();
    test_multiple_simultaneous_submits();
    test_zero_credits_blocks_dispatch();
    test_command_status_success_frees_slot();
    test_timeout();
    test_send_error_completes_immediately();
    test_pool_exhaustion();
}
