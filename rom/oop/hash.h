#ifndef HASH_H
#define HASH_H
/*
    Copyright © 1995-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Demo of new OOP system - General hashing definitions.
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef INTERN_H
#   include "intern.h"
#endif

/* Macros */
/* The table has HashSize() buckets, a power of two, and HashMask() is
   HashSize() - 1, so that a hash value ANDed with it is a bucket index. */
#define HashMask(ht) ( (ht)->HashMask )
#define HashSize(ht)  ( HashMask(ht) + 1 )
#define MAX_HASH_CHARS 50

/* Bucket index of an integer key. The integer keys are interface method
   bases, whose low NUM_METHOD_BITS bits are always zero, so the bits above
   them are folded in. Every lookup of an integer table, including the
   method dispatch in metaclass.c, uses this. */
#define HashIndexULONG(mask, id) \
    ((ULONG)(((id) >> NUM_METHOD_BITS) ^ (id)) & (mask))

/* Types of hashtables. Can hash integers or strings */
#define HT_INTEGER	1
#define HT_STRING	2
#define HT_UUID     3

/* structs */

/* Default Bucket struct. The userdefined bucket must
** have this structure defined at the top
*/
struct Bucket
{
    /* Each entry in the hashtable is a linked list of buckets */
    struct Bucket *Next;
    /* The ID used to lookup hashed items */
    IPTR ID;
};


struct HashTable
{
    /* Pointer hash array itself */
    struct Bucket **Table;
    
    /* function for looking up the correct bucket for a supplied ID.
    ** Returns NULL if none found.
    ** Implementations is dependant on whether it's a HT_STRING
    ** or HT_INTEGER hash table. Do not try to look up a string id
    ** in an integer hash table or vice versa.
    */
    struct Bucket * (*Lookup)(struct HashTable *ht, IPTR id, struct IntOOPBase *OOPBase);
    
    /* Calcultes an offset into the hashtable according to the ID.
    ** Implementations dependes whether hashtable is of type
    ** HT_STRING or HT_INTEGER. Do not try to hash a string id
    ** in an integer hash table or vice versa.
    */
    
    ULONG (*CalcHash)(struct HashTable *ht, IPTR id);
    
    /* Number of buckets - 1. The number of buckets is a power of two. */
    ULONG HashMask;
    
    /* Hash table type: HT_STRING or HT_INTEGER */
    UBYTE Type; /* String or integer */
};

/* Protos */

/* Create a new empty hashtable for about 'entries' items, with at least
   twice as many buckets. Type can be HT_STRING, HT_INTEGER or HT_UUID */
struct HashTable *NewHash(ULONG entries, UBYTE type, struct IntOOPBase *OOPBase);

/* Free a hashtable previously allocated with NewHash().
   You must supply a function for freeing evt. buckets in the table.
*/
VOID FreeHash(struct HashTable *ht, VOID (*freebucket)(), struct IntOOPBase *OOPBase);

/* The implemntations of hashtable->HashLookup() */
struct Bucket *HashLookupULONG(struct HashTable *ht, IPTR id, struct IntOOPBase *OOPBase);
struct Bucket *HashLookupUUID(struct HashTable *ht, IPTR id, struct IntOOPBase *OOPBase);
struct Bucket *HashLookupStr(struct HashTable *ht, IPTR id, struct IntOOPBase *OOPBase);

/* Copies all the buckets of one hashtable into an other one.
   You must supply a function to copy the buckets.
   The user-suppplied copybucket() will get a parameter to
   the bucket as first parameter, and a pointer to the user
   supplied data as second parameter
*/
   
BOOL CopyHash(struct HashTable *dest_ht
	,struct HashTable *src_ht
	,struct Bucket * (*copybucket)()
	,APTR data
	,struct IntOOPBase *OOPBase);
	
/* Inserts a bucket into the hashtable, according to it's ID */	
VOID InsertBucket(struct HashTable *ht, struct Bucket *b, struct IntOOPBase *OOPBase);

/* Removes a bucket from a hashtable */
VOID RemoveBucket(struct  HashTable *ht, struct Bucket *b);

/* The implemntations of hashtable->CalcHash() */
ULONG CalcHashStr(struct HashTable *ht, IPTR id);
ULONG CalcHashULONG(struct HashTable *ht, IPTR id);
ULONG CalcHashUUID(struct HashTable *ht, IPTR id);

VOID print_table(struct HashTable *ht, struct IntOOPBase *OOPBase);
#endif /* HASH_H */
