/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo of new OOP system - General hashing functions.
   Lang: english
*/

#include "hash.h"
#include <stdlib.h>

#define SDEBUG 0
#define DEBUG 0
#include "debug.h"
#include <string.h>
#define UB(x) ((UBYTE *)x)

/******************
**  AllocHash()  **
******************/

/* Allocates a hashtable for given number of entries,
** so that hash table is no more than 50% full.
** Initializes table as empty.
*/

struct Bucket **NewHash(ULONG entries)
{

    /* Calulates hashsize as 2^n - 1 so that htsize >= 2*initial */
    ULONG temp = 1;
    BYTE i;
    ULONG size;
    struct Bucket **ht = NULL;
    
    EnterFunc(bug("NewHash(entries=%ld)\n", entries));

    /* Find the highest bit in 'initial' */
    for (i = 31; i >= 0; i --)
    {
	if ((temp << i) & entries)
	    break;
    }
    
    D(bug("i: %d\n", i));

    /* Make sure table is never more than 50% full */
    i ++;
    
    entries = (temp << i);
    
    /* Calulate size in bytes. + 1 because we store the size of the hashtable */
    size = UB( &ht[entries + 1] ) - UB( &ht[0] );
    
    
    ht = malloc(size);
    if (ht)
    {

	/* Clear table */
	memset(ht, 0, size);
       

	entries --; /* 2^n - 1 */
    	((IPTR *)ht)[0] = entries;
	
	D(bug("ht: %p, mask: %ld\n", ht, entries));
    	
	
    	ReturnPtr ("NewHash", struct Bucket **, &ht[1]);
    }
    ReturnPtr ("NewHash", struct Bucket **, NULL);
}

/*****************
**  FreeHash()  **
*****************/
VOID FreeHash(struct Bucket **ht, VOID (*freebucket)())
{
    ULONG i;
    
    EnterFunc(bug("FreeHash(ht=%p, freebucket=%p)\n", ht, freebucket));

    D(bug("Hashsize: %ld\n", HashSize(ht) ));
    
    
    /* Free all buckets */
    for (i = 0; i < HashSize(ht); i ++)
    {
    	struct Bucket *b, *next_b;
	D(bug("Freeing buckets at entry %ld\n", i));
	
	for (b = ht[i]; b; b = next_b)
	{
	    D(bug("Freeing bucket %p of id %ld\n", b, b->ID));
	    next_b = b->Next;
	    D(bug("Calling freebucket\n"));
	    freebucket(b);
	    D(bug("Freebucket called\n"));
	}
    }
    free(&ht[-1]);
    
    ReturnVoid("FreeHash");
}

/*******************
**  HashLookup()  **
*******************/
struct Bucket *HashLookupULONG(struct Bucket **ht, IPTR id)
{
    struct Bucket *b;
    
    for (b = ht[CalcHash(ht, id)]; b; b = b->Next)
    {
    	if (b->ID == id)
	    return (b);
    }
    
    return (NULL);
}

struct Bucket *HashLookupStr(struct Bucket **ht, IPTR id)
{
    struct Bucket *b;
    EnterFunc(bug("HashLookupStr(ht=%p, id=%s)\n", ht, (STRPTR)id));
    for (b = ht[CalcHashStr(ht, id)]; b; b = b->Next)
    {
    	D(bug("Bucket: %p\n", b));
    	if (!strcmp((STRPTR)b->ID, (STRPTR)id))
	    ReturnPtr ("HashLookupStr", struct Bucket *, b);
    }
    ReturnPtr ("HashLookupStr", struct Bucket *, NULL);
}

/*****************
**  CopyHash()  **
*****************/
BOOL CopyHash(struct Bucket **dest_ht
	,struct Bucket **src_ht
	,struct Bucket * (*copybucket)()
	,APTR data)
{
    ULONG i;
    
    EnterFunc(bug("CopyHash(dest_ht=%p, src_ht=%p, copybucket=%p,data = %p)\n",
    	dest_ht, src_ht, copybucket, data));
    
    for (i = 0; i < HashSize(src_ht); i ++ )
    {
    	struct Bucket *b;
	        
	D(bug("idx: %ld\n", i));

    	for (b = src_ht[i]; b; b = b->Next)
	{
	    /* Rehash bucket into destination hashtable */
	    struct Bucket *new_b;
	    
	    D(bug("Bucket: %p\n", b));
	    
	    new_b = copybucket(b);
	    if (!new_b)
	    	ReturnBool ("CopyHash", FALSE);
	    
	    InsertBucket(dest_ht, new_b);
	    
	    
	} /* For each bucket at current entry */
	
    } /* For each entry in source hashtable */
    
    ReturnBool ("CopyHash", TRUE);
}

/*********************
**  InsertBucket()  **
*********************/
VOID InsertBucket(struct Bucket **ht, struct Bucket *b)
{
    /* Inserts bucket into hashtable accordibg to the ID */
    ULONG idx;
    
    EnterFunc(bug("InsertBucket(ht=%p, b=%p)\n", ht, b));
    
    idx = CalcHash(ht, b->ID);
    b->Next = ht[idx];
    ht[idx] = b;
    D(bug("b->Next=%p\n", b->Next));
    
    
    ReturnVoid ("InsertBucket");
    
}

/* id is an interface ID */
ULONG CalcHashULONG(struct Bucket **ht, IPTR id)
{
    return  (HashMask(ht) & id);
}

/* id is a (interfaceid << NUM_METHOD_BITS) & methodID */
ULONG CalcHashHM(struct Bucket **ht, IPTR id)
{
    return  (HashMask(ht) & (id ^ (id >> NUM_METHOD_BITS)));
}

/* id is a (interfaceid << NUM_METHOD_BITS) & methodID */
ULONG CalcHashStr(struct Bucket **ht, IPTR id)
{
    STRPTR str = (STRPTR)id;
    ULONG val, i;
    for (i = 0, val = 0; (i < MAX_HASH_CHARS) && *str; str ++)
    	{val += *str; i ++; }
	
    return  (val & HashMask(ht));
}
