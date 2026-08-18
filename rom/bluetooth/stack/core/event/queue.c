#include <btcore/queue.h>

void bt_queue_init(struct bt_queue *q)
{
    q->head = NULL;
    q->tail = NULL;
}

bool bt_queue_is_empty(const struct bt_queue *q)
{
    return q->head == NULL;
}

void bt_queue_push(struct bt_queue *q, struct bt_queue_node *node)
{
    node->next = NULL;

    if (q->tail != NULL)
        q->tail->next = node;
    else
        q->head = node;

    q->tail = node;
}

struct bt_queue_node *bt_queue_pop(struct bt_queue *q)
{
    struct bt_queue_node *node = q->head;

    if (node == NULL)
        return NULL;

    q->head = node->next;
    if (q->head == NULL)
        q->tail = NULL;

    node->next = NULL;
    return node;
}
