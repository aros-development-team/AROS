#include <btcore/timer.h>

#include <stdint.h>

void bt_timer_init(struct bt_timer *timer, bt_timer_fn callback, void *user_data)
{
    timer->next = NULL;
    timer->expiry_us = 0;
    timer->callback = callback;
    timer->user_data = user_data;
    timer->pending = false;
}

void bt_timer_list_init(struct bt_timer_list *list)
{
    list->head = NULL;
}

void bt_timer_list_add(struct bt_timer_list *list, struct bt_timer *timer, uint64_t expiry_us)
{
    struct bt_timer **cur = &list->head;

    /* Re-adding an already pending timer would corrupt the list; drop it
     * from wherever it currently sits first. */
    bt_timer_list_cancel(list, timer);

    timer->expiry_us = expiry_us;
    timer->pending = true;

    while (*cur != NULL && (*cur)->expiry_us <= expiry_us)
        cur = &(*cur)->next;

    timer->next = *cur;
    *cur = timer;
}

void bt_timer_list_cancel(struct bt_timer_list *list, struct bt_timer *timer)
{
    struct bt_timer **cur = &list->head;

    if (!timer->pending)
        return;

    while (*cur != NULL)
    {
        if (*cur == timer)
        {
            *cur = timer->next;
            break;
        }
        cur = &(*cur)->next;
    }

    timer->pending = false;
    timer->next = NULL;
}

struct bt_timer *bt_timer_list_pop_expired(struct bt_timer_list *list, uint64_t now_us)
{
    struct bt_timer *timer;

    if (list->head == NULL || list->head->expiry_us > now_us)
        return NULL;

    timer = list->head;
    list->head = timer->next;
    timer->next = NULL;
    timer->pending = false;
    return timer;
}

uint64_t bt_timer_list_next_expiry(const struct bt_timer_list *list)
{
    if (list->head == NULL)
        return UINT64_MAX;
    return list->head->expiry_us;
}
