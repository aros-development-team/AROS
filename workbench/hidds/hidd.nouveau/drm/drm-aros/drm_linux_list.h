/* drm_linux_list.h -- linux list functions for the BSDs.
 * Created: Mon Apr 7 14:30:16 1999 by anholt@FreeBSD.org
 */
/*-
 * Copyright 2003 Eric Anholt
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <anholt@FreeBSD.org>
 *
 */
#ifndef _DRM_LINUX_LIST_
#define _DRM_LINUX_LIST_

struct list_head {
	struct list_head *next, *prev;
};

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

static __inline__ void
INIT_LIST_HEAD(struct list_head *head) {
	(head)->next = head;
	(head)->prev = head;
}

#define LIST_HEAD(name) struct list_head name = {&name, &name}

static __inline__ int
list_empty(struct list_head *head) {
	return (head)->next == head;
}

static __inline__ int 
list_is_singular(struct list_head *head) {
    return !list_empty(head) && ((head)->next == (head->prev));
}

/* Insert just after head (at start of list) */
static __inline__ void
list_add(struct list_head *entry, struct list_head *head) {
    (head)->next->prev = entry;
    (entry)->next = (head)->next;
    (head)->next = entry;
    (entry)->prev = head;
}

/* Insert just before head (at end of list) */
static __inline__ void
list_add_tail(struct list_head *entry, struct list_head *head) {
	(entry)->prev = (head)->prev;
	(entry)->next = head;
	(head)->prev->next = entry;
	(head)->prev = entry;
}

static __inline__ void
list_del(struct list_head *entry) {
	(entry)->next->prev = (entry)->prev;
	(entry)->prev->next = (entry)->next;
}

static __inline__ void
list_replace(struct list_head *old, struct list_head *head) {
	(head)->next = (old)->next;
	(head)->next->prev = head;
	(head)->prev = (old)->prev;
	(head)->prev->next = head;
}


#define list_for_each(entry, head)				\
    for (entry = (head)->next; entry != head; entry = (entry)->next)

#define list_for_each_prev(entry, head) \
        for (entry = (head)->prev; entry != (head); \
                entry = entry->prev)

#define list_for_each_safe(entry, temp, head)			\
    for (entry = (head)->next, temp = (entry)->next;		\
	entry != head; 						\
	entry = temp, temp = entry->next)

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		n = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))

static inline void list_del_init(struct list_head *entry)
{
    list_del(entry);
    INIT_LIST_HEAD(entry);
}

#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

static inline void list_move_tail(struct list_head *list,
				  struct list_head *head)
{
	list_del(list);
	list_add_tail(list, head);
}

static inline void list_move(struct list_head *list, struct list_head *head)
{
	list_del(list);
	list_add(list, head);
}

static inline void list_sort(void * priv, struct list_head *head,
	       int (*cmp)(void * priv, struct list_head *a, struct list_head *b))
{
    int finished = 0;
    
    if (list_empty(head))
        return;
    do
    {
        struct list_head * elem = head->next;
        finished = 1;

        while(elem->next != head)
        {
            if (cmp(priv, elem, elem->next) < 0)
            {
                /* Need to switch both elements */
                struct list_head * elemnext = elem->next;
                elem->prev->next = elemnext;
                elemnext->prev = elem->prev;
                elem->prev = elemnext;
                elem->next = elemnext->next;
                elemnext->next->prev = elem;
                elemnext->next = elem;
                finished = 0;
                break;
            }
            
            elem = elem->next;
        }
    }while(!finished);
}
#endif /* _DRM_LINUX_LIST_ */
