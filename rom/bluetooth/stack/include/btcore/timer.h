#ifndef BTCORE_TIMER_H
#define BTCORE_TIMER_H

#include <btcore/types.h>

struct bt_timer;

typedef void (*bt_timer_fn)(struct bt_timer *timer, void *user_data);

struct bt_timer
{
    struct bt_timer *next; /* owned by whichever bt_timer_list holds it */
    uint64_t expiry_us;
    bt_timer_fn callback;
    void *user_data;
    bool pending;
};

/* Prepares timer for use. Does not schedule it -- pass it to
 * bt_timer_list_add() (or to a platform's timer_start()) for that. */
void bt_timer_init(struct bt_timer *timer, bt_timer_fn callback, void *user_data);

/*
 * Sorted-by-expiry singly-linked list of pending timers. Pure bookkeeping:
 * it doesn't know how to actually wait for time to pass. A platform's
 * bt_platform_ops.timer_start() computes expiry_us from its own time_us()
 * plus the requested delay, calls bt_timer_list_add(), and is responsible
 * for calling bt_timer_list_pop_expired() (directly or via schedule())
 * often enough to fire timers on time. Not thread-safe; meant to be owned
 * by a single event loop, matching the single-owner model in project.md.
 */
struct bt_timer_list
{
    struct bt_timer *head;
};

void bt_timer_list_init(struct bt_timer_list *list);
void bt_timer_list_add(struct bt_timer_list *list, struct bt_timer *timer, uint64_t expiry_us);

/* No-op if timer isn't currently pending in this list (already fired, or
 * never added). */
void bt_timer_list_cancel(struct bt_timer_list *list, struct bt_timer *timer);

/* Pops and returns the single earliest timer with expiry_us <= now_us, or
 * NULL if none is due yet. Caller invokes the callback -- this only
 * removes it from the list. Call repeatedly to drain everything due. */
struct bt_timer *bt_timer_list_pop_expired(struct bt_timer_list *list, uint64_t now_us);

/* Earliest pending expiry, or UINT64_MAX if the list is empty. */
uint64_t bt_timer_list_next_expiry(const struct bt_timer_list *list);

#endif /* BTCORE_TIMER_H */
