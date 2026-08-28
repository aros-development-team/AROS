#include "test_queue.h"
#include "../support/test.h"

#include <btcore/queue.h>

struct item
{
    struct bt_queue_node node; /* must be first */
    int value;
};

static void test_empty_queue(void)
{
    struct bt_queue q;

    bt_queue_init(&q);
    BT_CHECK(bt_queue_is_empty(&q));
    BT_CHECK(bt_queue_pop(&q) == NULL);
}

static void test_fifo_order(void)
{
    struct bt_queue q;
    struct item items[4];
    int i;

    bt_queue_init(&q);
    for (i = 0; i < 4; i++)
    {
        items[i].value = i;
        bt_queue_push(&q, &items[i].node);
    }

    BT_CHECK(!bt_queue_is_empty(&q));

    for (i = 0; i < 4; i++)
    {
        struct bt_queue_node *n = bt_queue_pop(&q);
        BT_CHECK(n != NULL);
        BT_CHECK(((struct item *)n)->value == i);
    }

    BT_CHECK(bt_queue_is_empty(&q));
    BT_CHECK(bt_queue_pop(&q) == NULL);
}

static void test_interleaved_push_pop(void)
{
    struct bt_queue q;
    struct item a, b, c;
    struct bt_queue_node *n;

    bt_queue_init(&q);
    a.value = 1;
    b.value = 2;
    c.value = 3;

    bt_queue_push(&q, &a.node);
    bt_queue_push(&q, &b.node);

    n = bt_queue_pop(&q);
    BT_CHECK(n == &a.node);

    bt_queue_push(&q, &c.node);

    n = bt_queue_pop(&q);
    BT_CHECK(n == &b.node);
    n = bt_queue_pop(&q);
    BT_CHECK(n == &c.node);
    BT_CHECK(bt_queue_is_empty(&q));
}

void run_queue_tests(void)
{
    test_empty_queue();
    test_fifo_order();
    test_interleaved_push_pop();
}
