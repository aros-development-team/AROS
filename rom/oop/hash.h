#ifndef HASH_H
#define HASH_H
/*
   (C) 1997-98 AROS - The Amiga Replacement OS
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
#define HashMask(ht) ( (ht)->HashMask )
#define HashSize(ht)  ( HashMask(ht) + 1 )
#define MAX_HASH_CHARS 3    

#define HT_INTEGER	1
#define HT_STRING	2

/* structs */
struct Bucket
{
    struct Bucket *Next;
    IPTR ID;
};

struct HashTable
{
    struct Bucket **Table;
    
    struct Bucket * (*Lookup)(struct HashTable *, IPTR, struct IntOOPBase *);
    ULONG (*CalcHash)(struct HashTable *, IPTR);

    ULONG HashMask;
    UBYTE Type; /* String or integer */
};

/* Protos */
struct HashTable *NewHash(ULONG entries, UBYTE type, struct IntOOPBase *OOPBase);
VOID FreeHash(struct HashTable *ht, VOID (*freebucket)(), struct IntOOPBase *OOPBase);
struct Bucket *HashLookupULONG(struct HashTable *ht, IPTR id, struct IntOOPBase *OOPBase);
struct Bucket *HashLookupStr(struct HashTable *ht, IPTR id, struct IntOOPBase *OOPBase);

BOOL CopyHash(struct HashTable *dest_ht
	,struct HashTable *src_ht
	,struct Bucket * (*copybucket)()
	,APTR data
	,struct IntOOPBase *OOPBase);
VOID InsertBucket(struct HashTable *ht, struct Bucket *b, struct IntOOPBase *OOPBase);

ULONG CalcHashStr(struct HashTable *ht, IPTR id);
ULONG CalcHashULONG(struct HashTable *ht, IPTR id);


#endif /* HASH_H */
