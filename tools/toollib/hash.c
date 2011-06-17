/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <toollib/hash.h>

typedef struct _HashNode HashNode;

struct _HashNode
{
    HashNode   * Next;
    const char * key;
    const void * data;
};

struct _Hash
{
    HashNode * Nodes[256];
};

static int calchash (const char * key)
{
    int code = 0;

    while (*key)
	code = (code + *key++) & 0xFF;

    return code;
}

static int calchashNC (const char * key)
{
    int code = 0;

    while (*key)
    {
	code = (code + toupper(*key)) & 0xFF;
	key ++;
    }

    return code;
}

Hash * Hash_New (void)
{
    Hash * hash = xmalloc (sizeof (Hash));
    memset (hash, 0, sizeof(Hash));
    return hash;
}

void Hash_Store (Hash * hash, const char * key, const void * data)
{
    int code = calchash (key);
    HashNode * node, * newNode;

    for (node=hash->Nodes[code]; node; node=node->Next)
    {
	if (node->key[0] == *key && !strcmp (node->key, key))
	{
	    node->data = data;
	    return;
	}
    }

    newNode = xmalloc (sizeof (HashNode));

    newNode->Next = hash->Nodes[code];
    newNode->key  = key;
    newNode->data = data;

    hash->Nodes[code] = newNode;
}

void Hash_StoreNC (Hash * hash, const char * key, const void * data)
{
    int code = calchashNC (key);
    HashNode * node, * newNode;

    for (node=hash->Nodes[code]; node; node=node->Next)
    {
	if (node->key[0] == *key && !strcmp (node->key, key))
	{
	    node->data = data;
	    return;
	}
    }

    newNode = xmalloc (sizeof (HashNode));

    newNode->Next = hash->Nodes[code];
    newNode->key  = key;
    newNode->data = data;

    hash->Nodes[code] = newNode;
}

void * Hash_Find (Hash * hash, const char * key)
{
    int code = calchash (key);
    HashNode * node;

    for (node=hash->Nodes[code]; node; node=node->Next)
    {
	if (node->key[0] == *key && !strcmp (node->key, key))
	{
	    return ((void *)node->data);
	}
    }

    return NULL;
}

void * Hash_FindNC (Hash * hash, const char * key)
{
    int code = calchashNC (key);
    HashNode * node;

    for (node=hash->Nodes[code]; node; node=node->Next)
    {
	if (node->key[0] == *key && !strcmp (node->key, key))
	{
	    return ((void *)node->data);
	}
    }

    return NULL;
}

void Hash_Traverse (Hash * hash, CB tp, CBD userdata)
{
    int t;
    HashNode * node;

    for (t=0; t<256; t++)
    {
	for (node=hash->Nodes[t]; node; node=node->Next)
	{
	    CallCB (tp, (void *)node->key, node->data, userdata);
	}
    }

    xfree (hash);
}

void Hash_Delete (Hash * hash, CB dnp, CBD userdata)
{
    int t;
    HashNode * node, * next;

    for (t=0; t<256; t++)
    {
	for (next=hash->Nodes[t]; (node=next); )
	{
	    next=next->Next;

	    if (dnp)
		CallCB (dnp, (void *)node->key, node->data, userdata);

	    xfree (node);
	}
    }

    xfree (hash);
}
