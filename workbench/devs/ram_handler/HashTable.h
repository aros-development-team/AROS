#ifndef  _HASHTABLE_H
#define  _HASHTABLE_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

struct rambase;

#include  <dos/notify.h>
#include  <exec/nodes.h>
#include  <exec/lists.h>
#include  <stdio.h>

struct Receiver
{
    struct Node node;
    struct NotifyRequest *nr;
};


typedef struct _HashTable
{
    struct List *array;

    ULONG size;
    ULONG nElems;

    ULONG (*hash)(struct rambase *rambase, const void *key);
    int  (*compare)(struct rambase *rambase, const void *key1, const void *key2);
    void (*delete)(struct rambase *rambase, void *key, struct List *list);
} HashTable;



/* HashTable_new
 *
 * Purpose:  Create a new hash table.
 *
 * Input:    ULONG size  --  number of hash positions in the hash table
 *           ULONG (*hash)(struct rambase *rambase, const void *key)
 *                      --  hash function for the elements
 *           int  (*compare)(struct rambase *rambase,
 *				const void *key1, const void *key2)
 *                      --  comparison function over keys
 *           void (*delete)(struct rambase *rambase,
 *				const void *key, void *data)
 *                      --  deletion function for an <key, data> pair
 *
 * Output:   HashTable *  --  the new hash table
 *
 */
HashTable *HashTable_new(struct rambase *rambase, ULONG size,
			 ULONG (*hash)(struct rambase *rambase,
					const void *key),
			 int (*compare)(struct rambase *rambase,
					const void *key1, const void *key2),
			 void (*delete)(struct rambase *rambase,
					void *key, struct List *list));


/* HashTable_delete
 *
 * Purpose:  Free the resources held by a HashTable.
 *
 * Input:    HashTable *ht  --  the hash table the resources of which to free
 *
 * Output:   --
 *
 */
void HashTable_delete(struct rambase *rambase, HashTable *ht);


/* HashTable_insert
 *
 * Purpose:  Insert an element into the hash table
 *
 * Input:    HashTable *ht    --  the hash table to insert the element into
 *           void      *key   --  the key for the element
 *           void      *data  --  the data for the element
 *
 * Output:   --
 *
 */
void HashTable_insert(struct rambase *rambase, HashTable *ht, void *key,
		      struct Receiver *rr);


/* HashTable_remove
 *
 * Purpose:  Remove an element from the hash table.
 *
 * Input:    HashTable *ht    --  the hash table to remove the element from
 *           void      *key   --  the key for the element to remove
 *
 * Output:   --
 *
 */
void HashTable_remove(struct rambase *rambase, HashTable *ht, void *key);


/* HashTable_find
 *
 * Purpose:  Find an element corresponing to a certain key in a hash table.
 *
 * Input:    HashTable *ht    --  the hash table search for the element in
 *           void      *key   --  the key for the element to search for
 *
 * Output:   void *  --  the data part of the element or NULL if there
 *                       was no element corresponding to 'key'
 *
 */
struct List *HashTable_find(struct rambase *rambase, HashTable *ht, void *key);


/* HashTable_size
 *
 * Purpose:  Return the number of elements in a hash table.
 *
 * Input:    HashTable *ht    --  the hash table to query the size of
 *
 * Output:   ULONG  --  the number of elements in 'ht'
 *
 */
inline ULONG HashTable_size(HashTable *ht);

#endif /* _HASHTABLE_H */
