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

/* Types */
typedef struct _Node Node;

struct _Node
{
    Node * next,
	 * prev;
    char * name;
};

typedef struct
{
    Node * first,
	 * last,
	 * prelast;
}
List;

/* Macros */
#   define NewList(l)       (((List *)l)->prelast = (Node *)(l), \
			    ((List *)l)->last = 0, \
			    ((List *)l)->first = (Node *)&(((List *)l)->last))

#   define AddHead(l,n)     ((void)(\
	((Node *)n)->next        = ((List *)l)->first, \
	((Node *)n)->prev        = (Node *)&((List *)l)->first, \
	((List *)l)->first->prev = ((Node *)n), \
	((List *)l)->first       = ((Node *)n)))

#   define AddTail(l,n)     ((void)(\
	((Node *)n)->next          = (Node *)&((List *)l)->last, \
	((Node *)n)->prev          = ((List *)l)->prelast, \
	((List *)l)->prelast->next = ((Node *)n), \
	((List *)l)->prelast       = ((Node *)n) ))

#   define Remove(n)        ((void)(\
	((Node *)n)->prev->next = ((Node *)n)->next,\
	((Node *)n)->next->prev = ((Node *)n)->prev ))

#   define GetHead(l)       (void *)(((List *)l)->first->next \
				? ((List *)l)->first \
				: (Node *)0)
#   define GetTail(l)       (void *)(((List *)l)->prelast->prev \
				? ((List *)l)->prelast \
				: (Node *)0)
#   define GetNext(n)       (void *)(((Node *)n)->next->next \
				? ((Node *)n)->next \
				: (Node *)0)
#   define GetPrev(n)       (void *)(((Node *)n)->prev->prev \
				? ((Node *)n)->prev \
				: (Node *)0)
#   define ForeachNode(l,n) \
	for (n=(void *)(((List *)(l))->first); \
	    ((Node *)(n))->next; \
	    n=(void *)(((Node *)(n))->next))
#   define ForeachNodeSafe(l,node,nextnode) \
	for (node=(void *)(((List *)(l))->first); \
	    ((nextnode)=(void*)((Node *)(node))->next); \
	    (node)=(void *)(nextnode))

/* Functions */
void *FindNode (const List * l, const char * name);
void printlist (List * l);
void freelist (List * l);
Node *newnode (const char * name);
void *newnodesize (const char * name, size_t size);
Node *addnodeonce (List * l, const char * name);
void *addnodeoncesize (List * l, const char * name, size_t size);

#endif /* __MMAKE_LIST_H */
