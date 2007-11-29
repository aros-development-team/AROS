/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    An Hash Table implementation.
*/

#include  "HashTable.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <proto/alib.h>
#include <proto/exec.h>

#define  DEBUG 0
#include <aros/debug.h>

#ifdef UtilityBase
    #undef UtilityBase
#endif
#define UtilityBase rambase->utilitybase

extern STRPTR Strdup(struct rambase *rambase, STRPTR string);
extern void Strfree(struct rambase *rambase, STRPTR string);

typedef struct _HashNode
{
    struct Node  node;
    void        *key;
    struct List  requests;
} HashNode;

static HashNode *find(struct rambase *rambase, HashTable *ht,
		      const void *key, struct List *list);

HashTable *HashTable_new(struct rambase *rambase, ULONG size,
			 ULONG (*hash)(struct rambase *rambase,
				       const void *key),
			 int (*compare)(struct rambase *rambase,
					const void *key1,
					const void *key2),
			 void (*delete)(struct rambase *rambase,
					void *key,
					struct List *list))
{
    ULONG       i;		/* Loop variable */
    HashTable *ht = AllocVec(sizeof(HashTable), MEMF_ANY);

    if (ht == NULL)
    {
	return NULL;
    }

    ht->size = size;
    ht->nElems = 0;
    ht->array = AllocVec(sizeof(struct List)*size, MEMF_ANY);

    if (ht->array == NULL)
    {
	FreeVec(ht);

	return NULL;
    }

    ht->hash = hash;
    ht->compare = compare;
    ht->delete = delete;

    for (i = 0; i < size; i++)
    {
	NewList(&ht->array[i]);
    }
    
    return ht;
}


/* TODO: Fix the double list thing, remove the (void *) hack */
void HashTable_delete(struct rambase *rambase, HashTable *ht)
{
    ULONG i;

    for (i = 0; i < ht->size; i++)
    {
	while (!IsListEmpty(&ht->array[i]))
	{
	    HashNode *hn = (HashNode *)RemHead(&ht->array[i]);
	    
	    ht->delete(rambase, hn->key, (void *)&hn->requests);
	    FreeVec(hn);
	}
    }

    FreeVec(ht);
}


void HashTable_insert(struct rambase *rambase, HashTable *ht, void *key,
		      struct Receiver *rr)
{
    ULONG      pos;
    HashNode *hn;

    pos = ht->hash(rambase, key) % ht->size;

    hn = find(rambase, ht, key, &ht->array[pos]);

    if (hn == NULL)
    {
	/* There was no previous request for this entity */
	hn = AllocVec(sizeof(HashNode), MEMF_ANY);

	if (hn == NULL)
	{
	    return;
	}

	hn->key = Strdup(rambase, key);

	if (hn->key == NULL)
	{
	    FreeVec(hn);
	    return;
	}

	NewList(&hn->requests);
	AddHead(&ht->array[pos], &hn->node);
    }

    AddHead(&hn->requests, &rr->node);

    ht->nElems++;
}


void HashTable_remove(struct rambase *rambase, HashTable *ht, void *key)
{
    HashNode    *hn;
    ULONG        pos;
    struct Node *tempNode;
    struct List *list;

    pos = ht->hash(rambase, key) % ht->size;
    list = &ht->array[pos];

    ForeachNodeSafe(list, hn, tempNode)
    {
	if (ht->compare(rambase, key, hn->key) == 0)
	{
	    Remove(&hn->node);
	    ht->delete(rambase, hn->key, (void *)&hn->requests);
	    FreeVec(hn);
	    break;
	}
    }
}


static HashNode *find(struct rambase *rambase, HashTable *ht,
		      const void *key, struct List *list)
{
    HashNode *hn;		/* Loop variable */

    ForeachNode(list, hn)
    {
	if (ht->compare(rambase, key, hn->key) == 0)
	{
	    return hn;
	}
    }

    return NULL;
}


struct List *HashTable_find(struct rambase *rambase, HashTable *ht,
			    const void *key)
{
    HashNode *hn;
    ULONG     pos;

    pos = ht->hash(rambase, key) % ht->size;
    hn = find(rambase, ht, key, &ht->array[pos]);

    if (hn != NULL)
	return &hn->requests;

    return NULL;
}


inline ULONG HashTable_size(HashTable *ht)
{
    return ht->nElems;
}

