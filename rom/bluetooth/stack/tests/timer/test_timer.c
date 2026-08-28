#include "test_timer.h"
#include "../support/test.h"

#include <btcore/timer.h>

#include <stdint.h>

static void fire_counting_callback(struct bt_timer *timer, void *user_data)
{
    int *fire_count = (int *)user_data;
    (void)timer;
    (*fire_count)++;
}

static void test_empty_list(void)
{
    struct bt_timer_list list;

    bt_timer_list_init(&list);
    BT_CHECK(bt_timer_list_next_expiry(&list) == UINT64_MAX);
    BT_CHECK(bt_timer_list_pop_expired(&list, 1000) == NULL);
}

static void test_single_timer_not_yet_due(void)
{
    struct bt_timer_list list;
    struct bt_timer t;
    int fired = 0;

    bt_timer_list_init(&list);
    bt_timer_init(&t, fire_counting_callback, &fired);
    bt_timer_list_add(&list, &t, 1000);

    BT_CHECK(bt_timer_list_next_expiry(&list) == 1000);
    BT_CHECK(bt_timer_list_pop_expired(&list, 999) == NULL);
    BT_CHECK(fired == 0);
}

static void test_single_timer_due(void)
{
    struct bt_timer_list list;
    struct bt_timer t;
    int fired = 0;
    struct bt_timer *popped;

    bt_timer_list_init(&list);
    bt_timer_init(&t, fire_counting_callback, &fired);
    bt_timer_list_add(&list, &t, 1000);

    popped = bt_timer_list_pop_expired(&list, 1000);
    BT_CHECK(popped == &t);
    BT_CHECK(!t.pending);
    popped->callback(popped, popped->user_data);
    BT_CHECK(fired == 1);

    /* Consumed: a second pop at the same or later time finds nothing. */
    BT_CHECK(bt_timer_list_pop_expired(&list, 5000) == NULL);
}

static void test_ordering(void)
{
    struct bt_timer_list list;
    struct bt_timer a, b, c;
    int fired = 0;
    struct bt_timer *popped;

    bt_timer_list_init(&list);
    bt_timer_init(&a, fire_counting_callback, &fired);
    bt_timer_init(&b, fire_counting_callback, &fired);
    bt_timer_init(&c, fire_counting_callback, &fired);

    /* Inserted out of order; must come back out sorted by expiry. */
    bt_timer_list_add(&list, &a, 300);
    bt_timer_list_add(&list, &b, 100);
    bt_timer_list_add(&list, &c, 200);

    BT_CHECK(bt_timer_list_next_expiry(&list) == 100);

    popped = bt_timer_list_pop_expired(&list, 1000);
    BT_CHECK(popped == &b);
    popped = bt_timer_list_pop_expired(&list, 1000);
    BT_CHECK(popped == &c);
    popped = bt_timer_list_pop_expired(&list, 1000);
    BT_CHECK(popped == &a);
    BT_CHECK(bt_timer_list_pop_expired(&list, 1000) == NULL);
}

static void test_partial_drain(void)
{
    struct bt_timer_list list;
    struct bt_timer a, b;
    int fired = 0;

    bt_timer_list_init(&list);
    bt_timer_init(&a, fire_counting_callback, &fired);
    bt_timer_init(&b, fire_counting_callback, &fired);

    bt_timer_list_add(&list, &a, 100);
    bt_timer_list_add(&list, &b, 200);

    /* Only 'a' is due; 'b' must stay in the list. */
    BT_CHECK(bt_timer_list_pop_expired(&list, 150) == &a);
    BT_CHECK(bt_timer_list_pop_expired(&list, 150) == NULL);
    BT_CHECK(bt_timer_list_next_expiry(&list) == 200);

    BT_CHECK(bt_timer_list_pop_expired(&list, 200) == &b);
}

static void test_cancel(void)
{
    struct bt_timer_list list;
    struct bt_timer a, b;
    int fired = 0;

    bt_timer_list_init(&list);
    bt_timer_init(&a, fire_counting_callback, &fired);
    bt_timer_init(&b, fire_counting_callback, &fired);

    bt_timer_list_add(&list, &a, 100);
    bt_timer_list_add(&list, &b, 200);

    bt_timer_list_cancel(&list, &a);
    BT_CHECK(!a.pending);

    /* Cancelling something not pending must be a harmless no-op. */
    bt_timer_list_cancel(&list, &a);

    BT_CHECK(bt_timer_list_pop_expired(&list, 1000) == &b);
    BT_CHECK(bt_timer_list_pop_expired(&list, 1000) == NULL);
}

static void test_readd_moves_timer(void)
{
    struct bt_timer_list list;
    struct bt_timer a, b;
    int fired = 0;

    bt_timer_list_init(&list);
    bt_timer_init(&a, fire_counting_callback, &fired);
    bt_timer_init(&b, fire_counting_callback, &fired);

    bt_timer_list_add(&list, &a, 100);
    bt_timer_list_add(&list, &b, 200);

    /* Re-adding a still-pending timer (e.g. restarting a retry timeout)
     * must relocate it, not duplicate it in the list. */
    bt_timer_list_add(&list, &a, 300);

    BT_CHECK(bt_timer_list_pop_expired(&list, 1000) == &b);
    BT_CHECK(bt_timer_list_pop_expired(&list, 1000) == &a);
    BT_CHECK(bt_timer_list_pop_expired(&list, 1000) == NULL);
}

void run_timer_tests(void)
{
    test_empty_list();
    test_single_timer_not_yet_due();
    test_single_timer_due();
    test_ordering();
    test_partial_drain();
    test_cancel();
    test_readd_moves_timer();
}
