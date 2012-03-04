#ifndef __MMAKE_LIST_H
#define __MMAKE_LIST_H

/* MetaMake - A Make extension
   Copyright © 1995-2004, The AROS Development Team. All rights reserved.

This file is part of MetaMake.

MetaMake is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

MetaMake is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include <stddef.h>

/* This file specifies the API for the list functions */

#include "compiler.h"

/* Types */
struct Node __mayalias;
struct Node
{
    struct Node * next,
		* prev;
    char 	* name;
};

#ifndef __GNUC__
struct List
{
    struct Node * first,
		* last,
		* prelast;
};
#else
struct List __mayalias;
struct List
{
    union
    {
        struct Node * first;
        struct Node ** _first;
    };
    struct Node * last;
    union
    {
        struct Node * prelast;
        struct List * _prelast;
    };
};
#endif

/* inline functions and macros */
static inline void __NewList(struct List *l)
{
#ifndef __GNUC__
    l->prelast = (struct Node *)l;
    l->_first = (struct Node *)&l->last;
#else
    l->_prelast = l;
    l->_first = &l->last;
#endif
    l->last = 0;
}
#define NewList(l) __NewList((struct List *)l)

static inline void __AddHead(struct List *l, struct Node *n)
{
    n->next        = l->first;
    n->prev        = (struct Node *)&l->first;
    l->first->prev = n;
    l->first       = n;
}
#define AddHead(l,n) __AddHead((struct List *)l, (struct Node *)n)

static inline void __AddTail(struct List *l, struct Node *n)
{
    n->next          = (struct Node *)&l->last;
    n->prev          = l->prelast;
    l->prelast->next = n;
    l->prelast       = n;
}
#define AddTail(l,n) __AddTail((struct List *)l, (struct Node *)n)

static inline void __Remove(struct Node *n)
{
    n->prev->next = n->next;
    n->next->prev = n->prev;
}
#define Remove(n) __Remove((struct Node *)n)

#   define GetHead(l)       (void *)(((struct List *)l)->first->next \
				? ((struct List *)l)->first \
				: (struct Node *)0)
#   define GetTail(l)       (void *)(((struct List *)l)->prelast->prev \
				? ((struct List *)l)->prelast \
				: (struct Node *)0)
#   define GetNext(n)       (void *)(((struct Node *)n)->next->next \
				? ((struct Node *)n)->next \
				: (struct Node *)0)
#   define GetPrev(n)       (void *)(((struct Node *)n)->prev->prev \
				? ((struct Node *)n)->prev \
				: (struct Node *)0)
#   define ForeachNode(l,n) \
	for (n=(void *)(((struct List *)(l))->first); \
	    ((struct Node *)(n))->next; \
	    n=(void *)(((struct Node *)(n))->next))
#   define ForeachNodeSafe(l,node,nextnode) \
	for (node=(void *)(((struct List *)(l))->first); \
	    ((nextnode)=(void*)((struct Node *)(node))->next); \
	    (node)=(void *)(nextnode))

/* Functions */
void AssignList (struct List * dest, struct List * src); /* After assignment only dest may be used !!! */
void *FindNode (const struct List * l, const char * name);
void printlist (struct List * l);
void freelist (struct List * l);
struct Node *newnode (const char * name);
void *newnodesize (const char * name, size_t size);
struct Node *addnodeonce (struct List * l, const char * name);
void *addnodeoncesize (struct List * l, const char * name, size_t size);

#endif /* __MMAKE_LIST_H */
