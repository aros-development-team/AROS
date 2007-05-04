#ifndef HASH_H
#define HASH_H
/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo of new OOP system - General hashing definitions.
   Lang: english
*/

#ifndef TYPES_H
#   include "types.h"
#endif
#ifndef OOP_H
#   include "oop.h"
#endif

/* Macros */
#define HashMask(ht) ( ((IPTR *)ht)[-1] )
#define HashSize(ht)  ( HashMask(ht) + 1 )
#define MAX_HASH_CHARS 3    

/* structs */
struct Bucket
{
    struct Bucket *Next;
    IPTR ID;
};

/* Protos */
struct Bucket **NewHash(ULONG entries);
VOID FreeHash(struct Bucket **ht, VOID (*freebucket)());
struct Bucket *HashLookupULONG(struct Bucket **ht, IPTR id);
struct Bucket *HashLookupStr(struct Bucket **ht, IPTR id);
BOOL CopyHash(struct Bucket **dest_ht
	,struct Bucket **src_ht
	,struct Bucket * (*copybucket)()
	,APTR data);
VOID InsertBucket(struct Bucket **ht, struct Bucket *b);

ULONG CalcHashStr(struct Bucket **ht, IPTR id);
ULONG CalcHashULONG(struct Bucket **ht, IPTR id);

#ifdef HASHED_METHODS
ULONG CalcHashHM(struct Bucket **ht, IPTR id);
#   define CalcHash CalcHashHM
#endif
#ifdef HASHED_IFS
#   define CalcHash CalcHashULONG
#endif
#ifdef HASHED_STRINGS
#   define CalcHash CalcHashStr
#endif

#endif /* HASH_H */
