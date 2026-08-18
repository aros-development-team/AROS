#ifndef BTCORE_COMMAND_QUEUE_H
#define BTCORE_COMMAND_QUEUE_H

#include <btcore/hci.h>
#include <btcore/queue.h>
#include <btcore/status.h>
#include <btcore/timer.h>
#include <btcore/transport.h>
#include <btcore/types.h>

/*
 * Dispatches HCI commands one at a time, honouring the controller's command
 * credit (Num_HCI_Command_Packets, project.md: "controle de créditos") and
 * enforcing a timeout per outstanding command. Callers may submit several
 * commands while one is still outstanding (project.md's "fila de comandos"
 * / "tratamento correto de comandos simultâneos") -- they queue up and are
 * sent strictly one at a time as credit allows.
 *
 * No dynamic allocation: a fixed pool of BT_CMDQ_MAX_PENDING slots backs
 * both the wait queue and the single outstanding command.
 */

#ifndef BT_CMDQ_MAX_PENDING
#define BT_CMDQ_MAX_PENDING 4
#endif

#ifndef BT_CMDQ_DEFAULT_TIMEOUT_US
#define BT_CMDQ_DEFAULT_TIMEOUT_US 5000000ull /* 5s: typical HCI command timeout */
#endif

enum bt_cmdq_result
{
    BT_CMDQ_RESULT_COMPLETE,    /* a Command Complete, or a failing Command Status, arrived */
    BT_CMDQ_RESULT_TIMEOUT,     /* no response within the command's timeout */
    BT_CMDQ_RESULT_SEND_ERROR   /* the transport rejected the command outright */
};

struct bt_cmdq_completion
{
    enum bt_cmdq_result result;
    uint16_t opcode;
    uint8_t status;                /* HCI status byte; meaningful for BT_CMDQ_RESULT_COMPLETE */
    const uint8_t *return_params;  /* NULL unless a genuine Command Complete arrived */
    size_t return_params_len;
};

typedef void (*bt_cmdq_complete_fn)(struct bt_cmdq_completion *completion, void *user_data);

struct bt_cmdq;

struct bt_cmdq_slot
{
    struct bt_queue_node node;
    struct bt_timer timeout_timer;
    struct bt_cmdq *owner;
    bool in_use;
    uint16_t opcode;
    uint8_t params[BT_HCI_MAX_PARAM_LEN];
    uint8_t params_len;
    uint64_t timeout_us;
    bt_cmdq_complete_fn on_complete;
    void *user_data;
};

struct bt_cmdq
{
    struct bt_hci_transport *transport;
    struct bt_timer_list *timers;
    struct bt_queue pending;          /* slots not yet sent */
    struct bt_cmdq_slot *outstanding; /* NULL, or the one command in flight */
    struct bt_cmdq_slot pool[BT_CMDQ_MAX_PENDING];
    uint8_t command_credits;          /* controller's Num_HCI_Command_Packets */
    uint64_t last_tick_us;
};

void bt_cmdq_init(struct bt_cmdq *q, struct bt_hci_transport *transport,
                   struct bt_timer_list *timers);

/* Queues opcode/params for sending. timeout_us of 0 means
 * BT_CMDQ_DEFAULT_TIMEOUT_US. Returns BT_ERR_NO_RESOURCES if all
 * BT_CMDQ_MAX_PENDING slots are in use. */
bt_status_t bt_cmdq_submit(struct bt_cmdq *q, uint16_t opcode, const uint8_t *params,
                            uint8_t params_len, uint64_t timeout_us,
                            bt_cmdq_complete_fn on_complete, void *user_data);

/* Sends the next queued command if none is outstanding and the controller
 * has at least one command credit. Called internally after every
 * completion; safe (and a no-op when there's nothing to do) to call
 * directly too, e.g. right after submit(). */
void bt_cmdq_pump(struct bt_cmdq *q, uint64_t now_us);

/* Feeds a received HCI Event packet in. Recognizes Command Complete and
 * Command Status for the outstanding command and updates command credit
 * from either; any other event is ignored here (other layers handle
 * those). */
void bt_cmdq_on_event(struct bt_cmdq *q, const uint8_t *data, size_t length, uint64_t now_us);

/* Fires timeout completions for anything overdue, then pumps. Call
 * whenever bt_timer_list_next_expiry(timers) indicates something in this
 * queue's timer list is due. */
void bt_cmdq_tick(struct bt_cmdq *q, uint64_t now_us);

#endif /* BTCORE_COMMAND_QUEUE_H */
