/*
   (C) 1997-98 AROS - The Amiga Replacement OS
   $Id$

   Desc: 
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

    /* Calulates hashsize as 2^n - 1 so that htsize >= 2*initial */
    ULONG temp = 1;
    BYTE i;
    ULONG size;
    struct HashTable *ht;
    
    EnterFunc(bug("NewHash(entries=%ld, type=%ld)\n", entries, type));
    
    ht = AllocMem(sizeof (struct HashTable), MEMF_ANY);
    if (ht)
    {
	
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
	entries --; /* 2^n - 1 */
    
    	size = UB( &ht[entries] ) - UB( &ht[0] );
    
    	ht->Table = AllocVec(size, MEMF_ANY|MEMF_CLEAR);
    	if (ht)
    	{
	    ht->HashMask = entries - 1;
	    ht->Type	 = type;
	    
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
	    D(bug("Freeing bucket %p of id %ld\n", b, b->ID));
	    next_b = b->Next;
	    D(bug("Calling freebucket\n"));
	    freebucket(b, OOPBase);
	    D(bug("Freebucket called\n"));
	}
    }
    FreeVec(ht->Table);
    FreeMem(ht, sizeof (struct HashTable));
    
    ReturnVoid("FreeHash");
}

/*******************
**  HashLookup()  **
*******************/
struct Bucket *HashLookupULONG(struct HashTable *ht, IPTR id, struct IntOOPBase *OOPBase)
{
    struct Bucket *b;
    
    for (b = ht->Table[CalcHashULONG(ht, id)]; b; b = b->Next)
    {
    	if (b->ID == id)
	    return (b);
    }
    
    return (NULL);
}

struct Bucket *HashLookupStr(struct HashTable *ht, IPTR id, struct IntOOPBase *OOPBase)
{
    struct Bucket *b;
    EnterFunc(bug("HashLookupStr(ht=%p, id=%s)\n", ht, (STRPTR)id));
    for (b = ht->Table[CalcHashStr(ht, id)]; b; b = b->Next)
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
BOOL CopyHash(struct HashTable *dest_ht
	,struct HashTable *src_ht
	,struct Bucket * (*copybucket)()
	,APTR data
	,struct IntOOPBase *OOPBase	)
{
    ULONG i;
    
    EnterFunc(bug("CopyHash(dest_ht=%p, src_ht=%p, copybucket=%p,data = %p)\n",
    	dest_ht, src_ht, copybucket, data));
    
    for (i = 0; i < HashSize(src_ht); i ++ )
    {
    	struct Bucket *b;
	        
	D(bug("idx: %ld\n", i));

    	for (b = src_ht->Table[i]; b; b = b->Next)
	{
	    /* Rehash bucket into destination hashtable */
	    struct Bucket *new_b;
	    
	    D(bug("Bucket: %p\n", b));
	    
	    new_b = copybucket(b, data, OOPBase);
	    if (!new_b)
	    	ReturnBool ("CopyHash", FALSE);
	    
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
    /* Inserts bucket into hashtable accordibg to the ID */
    ULONG idx;
    
    EnterFunc(bug("InsertBucket(ht=%p, b=%p)\n", ht, b));
    
    idx = ht->CalcHash(ht, b->ID);
    b->Next = ht->Table[idx];
    ht->Table[idx] = b;
    D(bug("b->Next=%p\n", b->Next));
    
    ReturnVoid ("InsertBucket");
    
}

/* id is an interface ID */
ULONG CalcHashULONG(struct HashTable *ht, IPTR id)
{
    return  (HashMask(ht) & id);
}

ULONG CalcHashStr(struct HashTable *ht, IPTR id)
{
    STRPTR str = (STRPTR)id;
    ULONG val, i;
    for (i = 0, val = 0; (i < MAX_HASH_CHARS) && *str; str ++)
    	{val += *str; i ++; }
	
    return  (val & HashMask(ht));
}
