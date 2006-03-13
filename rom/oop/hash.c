/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Common code for handling hashing.
   Lang: english
*/

#include "hash.h"
#include "intern.h"

#include <proto/exec.h>
#include <exec/memory.h>
#include <stdlib.h>

#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include <string.h>

#define UB(x) ((UBYTE *)x)

/******************
**  AllocHash()  **
******************/

/* Allocates a hashtable for given number of entries,
** so that hash table is no more than 50% full.
** Initializes table as empty.
*/

struct HashTable *NewHash(ULONG entries, UBYTE type, struct IntOOPBase *OOPBase)
{


    ULONG temp = 1;
    BYTE i;
    ULONG size;
    struct HashTable *ht;
    
    EnterFunc(bug("NewHash(entries=%ld, type=%ld)\n", entries, type));

    /* Allocate hashtable struct */
    
    
    ht = AllocMem(sizeof (struct HashTable), MEMF_ANY);
    if (ht)
    {
	/* Allocate table of size 2^n - 1	*/
    	/* Find the highest bit in 'initial'	*/
    	for (i = 31; i >= 0; i --)
    	{
	    if ((temp << i) & entries)
	    	break;
    	}
    
    	D(bug("i: %d\n", i));

    	/* Make sure table is never more than 50% full */
    	i ++;
    
    	entries = (temp << i);
	entries --; /* 2^n - 1 */
    
    	/* Get table size */
    	size = UB( &ht[entries] ) - UB( &ht[0] );
    	
	/* Allocate the table */
    	ht->Table = AllocVec(size, MEMF_ANY|MEMF_CLEAR);
    	if (ht)
    	{
	    ht->HashMask = entries - 1;
	    ht->Type	 = type;
	    
	    /* Initialize the hashtable's Lookup and CalcHash
	    ** accordint to if we want to hash integers or strings
	    */
	    switch (type)
	    {
	    case HT_INTEGER:
	    	ht->Lookup	= HashLookupULONG;
		ht->CalcHash	= CalcHashULONG;
		break;

	    case HT_STRING:
	    	ht->Lookup	= HashLookupStr;
		ht->CalcHash	= CalcHashStr;
		break;
	    }
	
    	
	
    	    ReturnPtr ("NewHash", struct HashTable *, ht);
	}
	FreeMem(ht, sizeof (struct HashTable));
    }
    ReturnPtr ("NewHash", struct HashTable *, NULL);
}

/*****************
**  FreeHash()  **
*****************/
VOID FreeHash(struct HashTable *ht, VOID (*freebucket)(), struct IntOOPBase *OOPBase)
{
    ULONG i;
    
    EnterFunc(bug("FreeHash(ht=%p, freebucket=%p)\n", ht, freebucket));

    D(bug("Hashsize: %ld\n", HashSize(ht) ));
    
    
    /* Free all buckets */
    for (i = 0; i < HashSize(ht); i ++)
    {
    	struct Bucket *b, *next_b;
	D(bug("Freeing buckets at entry %ld\n", i));
	
	for (b = ht->Table[i]; b; b = next_b)
	{
	    next_b = b->Next;
	    /* USe usersupplied func to free bucket */
	    freebucket(b, OOPBase);
	}
    }
    /* Free the table */
    FreeVec(ht->Table);
    /* Free the hashtable struct */
    FreeMem(ht, sizeof (struct HashTable));
    
    ReturnVoid("FreeHash");
}

/*******************
**  HashLookup()  **
*******************/
struct Bucket *HashLookupULONG(struct HashTable *ht, IPTR id, struct IntOOPBase *OOPBase)
{
    struct Bucket *b;
    EnterFunc(bug("HashLookupULONG(ht=%p, id=%ld)\n", ht, id));
    /* Function for looking up integers in the table */
    for (b = ht->Table[CalcHashULONG(ht, id)]; b; b = b->Next)
    {
    	D(bug("Current bucket: %p of id %ld\n", b, b->ID));
    	if (b->ID == id)
	    ReturnPtr ("HashLookupULONG", struct Bucket *, b);
    }
    
    ReturnPtr ("HashLookupULONG", struct Bucket *, NULL);
}

struct Bucket *HashLookupStr(struct HashTable *ht, IPTR id, struct IntOOPBase *OOPBase)
{
    struct Bucket *b;
    
    /* Function for looking up strings in the table */
    EnterFunc(bug("HashLookupStr(ht=%p, id=%s)\n", ht, (STRPTR)id));
    for (b = ht->Table[CalcHashStr(ht, id)]; b; b = b->Next)
    {
    	D(bug("Bucket: %p\n", b));
	D(bug("ID: %p\n", b->ID));
	D(bug("ID: %s\n", b->ID));
    	if (!strcmp((STRPTR)b->ID, (STRPTR)id))
	    ReturnPtr ("HashLookupStr", struct Bucket *, b);
    }
    ReturnPtr ("HashLookupStr", struct Bucket *, NULL);
}

/*****************
**  CopyHash()  **
*****************/
BOOL CopyHash(struct HashTable *dest_ht
	,struct HashTable *src_ht
	,struct Bucket * (*copybucket)()
	,APTR data
	,struct IntOOPBase *OOPBase	)
{
    ULONG i;
    
    /* Copies all buckets of src_ht into dest_ht */
    
    EnterFunc(bug("CopyHash(dest_ht=%p, src_ht=%p, copybucket=%p,data = %p)\n",
    	dest_ht, src_ht, copybucket, data));
    
    /* for each entry in the table */
    for (i = 0; i < HashSize(src_ht); i ++ )
    {
    	struct Bucket *b;
	        
	D(bug("idx: %ld\n", i));
	
	/* for each bucket at curent entry */
    	for (b = src_ht->Table[i]; b; b = b->Next)
	{
	    /* Rehash bucket into destination hashtable */
	    struct Bucket *new_b;
	    
	    D(bug("Bucket: %p\n", b));
	    
	    /* use user-supllied func to copy the bucket */
	    new_b = copybucket(b, data, OOPBase);
	    if (!new_b)
	    	ReturnBool ("CopyHash", FALSE);
	    
	    /* insert the new bucket into detsination table */
	    InsertBucket(dest_ht, new_b, OOPBase);
	    
	    
	} /* For each bucket at current entry */
	
    } /* For each entry in source hashtable */
    
    ReturnBool ("CopyHash", TRUE);
}

/*********************
**  InsertBucket()  **
*********************/
VOID InsertBucket(struct HashTable *ht, struct Bucket *b, struct IntOOPBase *OOPBase)
{
    /* Inserts bucket into hashtable according to its ID */
    ULONG idx;
    
    EnterFunc(bug("InsertBucket(ht=%p, b=%p)\n", ht, b));
    
    idx = ht->CalcHash(ht, b->ID);
    b->Next = ht->Table[idx];
    ht->Table[idx] = b;
    D(bug("b->Next=%p\n", b->Next));
    
    ReturnVoid ("InsertBucket");
    
}

VOID RemoveBucket(struct  HashTable *ht, struct Bucket *b)
{
    ULONG idx;
    struct Bucket *last_b = NULL,
    		  *cur_b;
    idx = ht->CalcHash(ht, b->ID);
    
    /* Search for bucket to remove */
    for (cur_b = ht->Table[idx]; cur_b; cur_b = cur_b->Next)
    {
    	if (cur_b == b)
	{
	    /* Bucket found */
	    if (last_b)
	    {
	    	/* Remove bucket from chain */
	    	last_b->Next = cur_b->Next;
		
		/* Not really neccessar, but ... */
		b->Next = NULL;
	    }
	    else
	    {
	    	/* We are removing the first bucket */
		ht->Table[idx] = cur_b->Next;
	    }
	}
	
	last_b = cur_b;
	
    } /* for (each bucket at idx) */
     
}


ULONG CalcHashULONG(struct HashTable *ht, IPTR id)
{
    /* Return idx into hashtable for an integer */
    return  (id % HashMask(ht));
}

static ULONG CalcHashStr_KR(struct HashTable *ht, IPTR id)
{
    STRPTR str = (STRPTR)id;
    ULONG val, i;
    /* Return idx into hashtable for a string */
    for (i = 0, val = 0; (i < MAX_HASH_CHARS) && *str; str ++)
        {val += *str; i ++; }

    return  (val % HashMask(ht));
}

static ULONG CalcHashStr_DJB2(struct HashTable *ht, IPTR id)
{
    STRPTR str = (STRPTR)id;
    ULONG val, i;
    /* Return idx into hashtable for a string */
    for (i = 0, val = 0; (i < MAX_HASH_CHARS) && *str; str ++)
        {val = ((val << 5) + val) ^ *str; i ++; }

    return  (val % HashMask(ht));
}

ULONG CalcHashStr_SDBM(struct HashTable *ht, IPTR id)
{
    STRPTR str = (STRPTR)id;
    ULONG val, i;
    /* Return idx into hashtable for a string */
    for (i = 0, val = 0; (i < MAX_HASH_CHARS) && *str; str ++)
        {val = *str + (val << 6) + (val << 16) - val; i ++; }

    return  (val % HashMask(ht));
}

ULONG CalcHashStr_Org(struct HashTable *ht, IPTR id)
{
    STRPTR str = (STRPTR)id;
    ULONG val, i;
    /* Return idx into hashtable for a string */    
    for (i = 0, val = 0; (i < MAX_HASH_CHARS) && *str; str ++)
    	{val += *str; i ++; }
	
    return  (val % HashMask(ht));
}

ULONG CalcHashStr(struct HashTable *ht, IPTR id)
{
    return CalcHashStr_DJB2(ht, id);
}

/* Prints contents of a hastable */
VOID print_table(struct HashTable *ht, struct IntOOPBase *OOPBase)
{
    ULONG idx;
    
    for (idx = 0; idx < HashSize(ht); idx ++)
    {
    	struct Bucket *b;
	D(bug("idx %ld: ", idx));
	for (b = ht->Table[idx]; b; b = b->Next)
	{
#if DEBUG
	    if (ht->Type == HT_INTEGER)
	    	bug("%ld ", b->ID);
	    else
	    	bug("%s (%ld)", (STRPTR)b->ID, ((struct iid_bucket *)b)->refcount);
#endif	    
	}
	D(bug("\n"));
	
    }
    return;
    
}
