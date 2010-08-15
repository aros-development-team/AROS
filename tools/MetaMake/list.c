/* MetaMake - A Make extension
   Copyright � 1995-2004, The AROS Development Team. All rights reserved.

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

/* This file contains the code for the list functions */

#include "config.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#ifdef HAVE_STRING_H
#   include <string.h>
#else
#   include <strings.h>
#endif

#include "mem.h"
#include "list.h"

void
AssignList (struct List * dest, struct List * src)
{
    NewList (dest);

    if (src->first->next != NULL)
    {
	src->first->prev = (struct Node *)&dest->first;
	dest->first = src->first;
	src->prelast->next = (struct Node *)&dest->last;
	dest->prelast = src->prelast;
    }
}

void *
FindNode (const struct List * l, const char * name)
{
    struct Node * n;

    ForeachNode (l, n)
    {
	if (!strcmp (n->name, name))
	    return n;
    }

    return NULL;
}

void
printlist (struct List * l)
{
    struct Node * n;

    ForeachNode (l,n)
    {
	printf ("    \"%s\"\n", n->name);
    }
}

void
freelist (struct List * l)
{
    struct Node * node, * next;

    ForeachNodeSafe(l,node,next)
    {
	Remove (node);

	cfree (node->name);
	free (node);
    }
}

struct Node *
newnode (const char * name)
{
    struct Node * n;

    assert (name);

    n = new (struct Node);

    n->name = xstrdup (name);

    return n;
}

void *
newnodesize (const char * name, size_t size)
{
    struct Node * n;

    assert (name);

    n = (struct Node *)xmalloc (size);
    memset (n, 0, size);

    n->name = xstrdup (name);

    return (void *)n;
}

struct Node *
addnodeonce (struct List * l, const char * name)
{
    struct Node * n;

    n = FindNode (l, name);

    if (!n)
    {
	n = newnode (name);
	AddTail(l,n);
    }

    return n;
}

void *
addnodeoncesize (struct List * l, const char * name, size_t size)
{
    void * n;

    n = FindNode (l, name);

    if (!n)
    {
	n = newnodesize (name, size);
	AddTail(l,n);
    }

    return n;
}
