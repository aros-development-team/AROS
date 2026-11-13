#ifndef BTCORE_QUEUE_H
#define BTCORE_QUEUE_H

#include <btcore/types.h>

/*
 * Intrusive singly-linked FIFO. Embed struct bt_queue_node as the first
 * member of whatever you want to queue; the queue itself never allocates.
 * Meant for single-producer/single-consumer use (project.md: "preferir
 * filas single-producer/single-consumer quando apropriado") -- it does no
 * locking of its own, so pushing and popping from different tasks still
 * needs whatever platform signal/lock wires the two sides together.
 */

struct bt_queue_node
{
    struct bt_queue_node *next;
};

struct bt_queue
{
    struct bt_queue_node *head;
    struct bt_queue_node *tail;
};

void bt_queue_init(struct bt_queue *q);
bool bt_queue_is_empty(const struct bt_queue *q);
void bt_queue_push(struct bt_queue *q, struct bt_queue_node *node);
struct bt_queue_node *bt_queue_pop(struct bt_queue *q);

#endif /* BTCORE_QUEUE_H */
