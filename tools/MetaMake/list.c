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

/* This file contains the code for the list functions */

#include <stddef.h>
#include <assert.h>

#include "mem.h"
#include "list.h"

void *
FindNode (const List * l, const char * name)
{
    Node * n;

    ForeachNode (l, n)
    {
	if (!strcmp (n->name, name))
	    return n;
    }

    return NULL;
}

void
printlist (List * l)
{
    Node * n;

    ForeachNode (l,n)
    {
	printf ("    \"%s\"\n", n->name);
    }
}

void
freelist (List * l)
{
    Node * node, * next;

    ForeachNodeSafe(l,node,next)
    {
	Remove (node);

	cfree (node->name);
	free (node);
    }
}

Node *
newnode (const char * name)
{
    Node * n;

    assert (name);

    n = new (Node);

    n->name = xstrdup (name);

    return n;
}

void *
newnodesize (const char * name, size_t size)
{
    Node * n;

    assert (name);

    n = (Node *)xmalloc (size);
    memset (n, 0, size);
    
    n->name = xstrdup (name);

    return (void *)n;
}

Node *
addnodeonce (List * l, const char * name)
{
    Node * n;

    n = FindNode (l, name);

    if (!n)
    {
	n = newnode (name);
	AddTail(l,n);
    }

    return n;
}

void *
addnodeoncesize (List * l, const char * name, size_t size)
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
