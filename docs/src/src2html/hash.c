#include "hash.h"
#include <string.h>
#include <stdlib.h>
#include "util.h"

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

int calchash (const char * key)
{
    int code = 0;

    while (*key)
	code = (code + *key++) & 0xFF;

    return code;
}

Hash * createhash (void)
{
    Hash * hash = xmalloc (sizeof (Hash));
    memset (hash, 0, sizeof(Hash));
    return hash;
}

void storedata (Hash * hash, const char * key, const void * data)
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

void * retrievedata (Hash * hash, const char * key)
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

void traversehash (Hash * hash, TraverseProc tp, void * userdata)
{
    int t;
    HashNode * node;

    for (t=0; t<256; t++)
    {
	for (node=hash->Nodes[t]; node; node=node->Next)
	{
	    (*tp) (node->key, (void *)node->data, userdata);
	}
    }

    xfree (hash);
}

void deletehash (Hash * hash, DeleteNodeProc dnp)
{
    int t;
    HashNode * node, * next;

    for (t=0; t<256; t++)
    {
	for (next=hash->Nodes[t]; (node=next); )
	{
	    next=next->Next;

	    if (dnp)
		(*dnp) ((char *)node->key, (void *)node->data);

	    xfree (node);
	}
    }

    xfree (hash);
}
