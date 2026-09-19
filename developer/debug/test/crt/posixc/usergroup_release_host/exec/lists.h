/* Host stand-in for <exec/lists.h>: the exec list layout and the list
   operations used by posixc and stdc, with exec's semantics (a header node
   pair lh_Head/lh_Tail/lh_TailPred, REMHEAD returning NULL on an empty list). */
#ifndef EXEC_LISTS_H
#define EXEC_LISTS_H
#include <exec/nodes.h>
struct List {
    struct Node *lh_Head;
    struct Node *lh_Tail;
    struct Node *lh_TailPred;
    UBYTE        lh_Type;
};
struct MinList {
    struct Node *mlh_Head;
    struct Node *mlh_Tail;
    struct Node *mlh_TailPred;
};
/* The operations work on the two link pointers every node starts with and
   on the three pointers every list header starts with, as exec's do, so that
   a MinList, a List and the header's sentinel nodes are all accessed only
   within their own storage. */
struct host_link { struct host_link *succ, *pred; };
struct host_head { struct host_link *head, *tail, *tailpred; };
#define HOST_HEAD(l) ((struct host_head *)(void *)(l))
#define HOST_LINK(n) ((struct host_link *)(void *)(n))
static inline void host_newlist(struct host_head *l)
{
    l->head = (struct host_link *)(void *)&l->tail;
    l->tail = NULL;
    l->tailpred = (struct host_link *)(void *)&l->head;
}
static inline void host_addhead(struct host_head *l, struct host_link *n)
{
    n->succ = l->head;
    n->pred = (struct host_link *)(void *)&l->head;
    l->head->pred = n;
    l->head = n;
}
static inline void host_addtail(struct host_head *l, struct host_link *n)
{
    n->succ = (struct host_link *)(void *)&l->tail;
    n->pred = l->tailpred;
    l->tailpred->succ = n;
    l->tailpred = n;
}
static inline struct Node *host_remhead(struct host_head *l)
{
    struct host_link *n = l->head;
    if (n->succ == NULL)
        return NULL;
    l->head = n->succ;
    n->succ->pred = (struct host_link *)(void *)&l->head;
    return (struct Node *)(void *)n;
}
static inline int host_isempty(struct host_head *l)
{
    return l->tailpred == (struct host_link *)(void *)l;
}
#define NEWLIST(l)      host_newlist(HOST_HEAD(l))
#define ADDHEAD(l, n)   host_addhead(HOST_HEAD(l), HOST_LINK(n))
#define ADDTAIL(l, n)   host_addtail(HOST_HEAD(l), HOST_LINK(n))
#define REMHEAD(l)      host_remhead(HOST_HEAD(l))
#define IsListEmpty(l)  host_isempty(HOST_HEAD(l))
#endif
