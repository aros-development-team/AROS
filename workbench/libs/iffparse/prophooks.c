/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Hook funtions needed for PropChunk().
    Lang: English.
*/

#include "iffparse_intern.h"



/****************************/
/* PropLCI Purge func  */
/****************************/

#define IFFParseBase IPB(hook->h_Data)

IPTR PropPurgeFunc
(
    struct Hook 	    * hook,
    struct LocalContextItem * lci,
    ULONG		      p
)
{
    struct StoredProperty *sp;

    DEBUG_PROPHOOKS(dprintf("PropPurgeFunc: hook %p lci %p p 0x%lx", hook, lci, p));

    /* Get the stored property structure */
    sp = (struct StoredProperty*)LocalItemData(lci);

    /* Free the chunk buffer */
    FreeMem(sp->sp_Data, sp->sp_Size);

    /* Free the local item itself */
    FreeLocalItem(lci);

    DEBUG_PROPHOOKS(dprintf("PropPurgeFunc: return NULL\n"));

    return (NULL);
}

/****************************/
/* PropChunk entry-handler  */
/****************************/

struct PF_ResourceInfo
{
    struct LocalContextItem *LCI;
    APTR		    Buffer;
    LONG		    BufferSize;
};

#undef IFFParseBase

VOID PF_FreeResources(struct PF_ResourceInfo *ri,
    struct IFFParseBase_intern * IFFParseBase)
{
    if (ri->LCI)     FreeLocalItem(ri->LCI);
    if (ri->Buffer)  FreeMem(ri->Buffer, ri->BufferSize);

    return;
}


#define IFFParseBase IPB(hook->h_Data)

LONG PropFunc
(
    struct Hook 	* hook,
    struct IFFHandle	* iff,
    APTR		  p
)
{
    struct LocalContextItem *lci;


    struct StoredProperty    *sp;
    struct ContextNode	    *cn;

    struct PF_ResourceInfo resinfo = {0}; /* = {0} is important */


    LONG   type,
	  id,
	  size;

    LONG  bytesread,
	  err;

    APTR  buf;

    DEBUG_PROPHOOKS(dprintf("PropFunc: hook %p iff %p p %p\n", hook, iff, p));

    /* The Chunk that caused us to be invoked is always the top chunk */
    cn = TopChunk(iff);

    type   = cn->cn_Type;
    id	  = cn->cn_ID;

    /* Allocate new LCI for containig the property */

    lci = AllocLocalItem
    (
	type,
	id,
	IFFLCI_PROP,
	sizeof (struct StoredProperty)
    );
    if (!lci)
    {
	DEBUG_PROPHOOKS(dprintf("PropFunc: return IFFERR_NOMEM #1\n"));

	return IFFERR_NOMEM;
    }

    resinfo.LCI = lci;


    /* Get userdata (storedproperty) */
    sp = (struct StoredProperty*)LocalItemData(lci);


    /* Allocate buffer to read chunk into */
    size = cn->cn_Size;


    buf = AllocMem(size, MEMF_ANY);
    if (!buf)
    {
	DEBUG_PROPHOOKS(dprintf("PropFunc: return IFFERR_NOMEM #2\n"));

	PF_FreeResources(&resinfo, IFFParseBase);

	return (IFFERR_NOMEM);
    }

    resinfo.Buffer = buf;
    resinfo.BufferSize = size;

    sp->sp_Data = buf;
    sp->sp_Size = size;

    /* Read chunk into the buffer */

    DEBUG_PROPHOOKS(dprintf("PropFunc: ReadChunkBytes(iff %p, buf %p, size %ld)\n", iff, buf, size));

    bytesread = ReadChunkBytes(iff, buf, size);

    DEBUG_PROPHOOKS(dprintf("PropFunc: ReadChunkBytes returned %lu\n", bytesread));

    /* Success ? */
    if (bytesread != size)
    {
	DEBUG_PROPHOOKS(dprintf("PropFunc: incomplete read! (%ld != %ld)\n", bytesread, size));
	PF_FreeResources(&resinfo, IFFParseBase);

	/* IFFERR_.. ? */
	if (bytesread >= 0)
	{
	    DEBUG_PROPHOOKS(dprintf("PropFunc: err = IFFERR_MANGLED\n"));
	    err = IFFERR_MANGLED;
#warning FIXME: should return err here?
	}
    }


    /* Store the new item IN PROP, so it may be found with FindProp() */
    err = StoreLocalItem(iff,lci,IFFSLI_PROP/*IFFSLI_ROOT*/);

    if (err)
    {
	DEBUG_PROPHOOKS(dprintf("PropFunc: return %ld\n", err));

	PF_FreeResources(&resinfo, IFFParseBase);

	return err;
    }


    SetLocalItemPurge(lci, &IFFParseBase->proppurgehook);

    DEBUG_PROPHOOKS(dprintf("PropFunc: return 0\n"));
    return 0;
}

