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

#undef SysBase
#define SysBase     IPB(hook->h_Data)->sysbase
#define IFFParseBase IPB(hook->h_Data)

IPTR PropPurgeFunc
(
    struct Hook 	    * hook,
    struct LocalContextItem * lci,
    ULONG		      p
)
{
    struct StoredProperty *sp;

    /* Get the stored property structure */
    sp = (struct StoredProperty*)LocalItemData(lci);

    /* Free the chunk buffer */
    FreeMem(sp->sp_Data, sp->sp_Size);

    /* Free the local item itself */
    FreeLocalItem(lci);

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

#undef SysBase
#undef IFFParseBase
#define SysBase     IFFParseBase->sysbase

VOID PF_FreeResources(struct PF_ResourceInfo *ri,
    struct IFFParseBase_intern * IFFParseBase)
{
    if (ri->LCI)     FreeLocalItem(ri->LCI);
    if (ri->Buffer)  FreeMem(ri->Buffer, ri->BufferSize);

    return;
}


#undef SysBase
#define SysBase     IPB(hook->h_Data)->sysbase
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
    if (!lci) return (IFFERR_NOMEM);

    resinfo.LCI = lci;


    /* Get userdata (storedproperty) */
    sp = (struct StoredProperty*)LocalItemData(lci);


    /* Allocate buffer to read chunk into */
    size = cn->cn_Size;


    buf = AllocMem
    (
	size,
	MEMF_ANY
    );

    if (!buf)
    {
	PF_FreeResources(&resinfo, IFFParseBase);
	return (IFFERR_NOMEM);
    }

    resinfo.Buffer = buf;
    resinfo.BufferSize = size;

    sp->sp_Data = buf;
    sp->sp_Size = size;

    /* Read chunk into the buffer */

    bytesread = ReadChunkBytes
    (
	iff,
	buf,
	size
    );

    /* Sucess ? */
    if (bytesread != size)
    {
	PF_FreeResources(&resinfo, IFFParseBase);

	/* IFFERR_.. ? */
	if (bytesread >= 0)
	    err = IFFERR_MANGLED;
    }


    /* Store the new item IN PROP, so it may be found with FindProp() */
    err = StoreLocalItem(iff,lci,IFFSLI_PROP/*IFFSLI_ROOT*/);

    if (err)
    {
	PF_FreeResources(&resinfo, IFFParseBase);
	return (err);
    }


    SetLocalItemPurge(lci, &IFFParseBase->proppurgehook);

    return (NULL);
}

