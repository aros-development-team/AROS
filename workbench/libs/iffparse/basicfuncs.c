/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Basic help functions needed by iffparse.
*/

#define DEBUG 0
#include <aros/debug.h>
#include "iffparse_intern.h"

LONG PushContextNode
(
    struct IFFHandle   *iff,
    LONG	       type,
    LONG	       id,
    LONG	       size,
    LONG	       scan,
    struct IFFParseBase_intern *IFFParseBase
)
{
    /* Allocates and puts a new context-node into the top of the context-stack
     Also does GoodType and GoodID checking  */
    struct ContextNode	 *cn;
    BOOL composite;

    D(bug("PushContextNode(iff=%p, type=%c%c%c%c, id=%c%c%c%c, size=%d, scan=%d)\n",
	iff,
	dmkid(type),
	dmkid(id),
	size,
	scan
    ));

    /* Set the composite flag if we have a composite contextnnode */
    if (id == ID_FORM || id == ID_LIST || id == ID_CAT || id == ID_PROP)
    {
	composite = TRUE;
	/* We have a new type, check it */
    }
    else
    {
	composite = FALSE;
	/* No composite type found.  Get old type from top contextnode */
	cn = TopChunk(iff);
	type = cn->cn_Type;
    }

      /* Check if type and ids are valid */
    if (!(GoodType(type) && GoodID(id)) )
	ReturnInt ("PushContextNode",LONG,IFFERR_MANGLED);

    /* Allocate a new context node */
    if ( !(cn = AllocMem ( sizeof (struct IntContextNode), MEMF_ANY ) ) )
	ReturnInt ("PushContextNode",LONG,IFFERR_NOMEM);

    /* Put the context node at top of the stack */
    AddHead ( (struct List*)&( GetIntIH(iff)->iff_CNStack ), (struct Node*)cn );

    /* Set the contextnode attrs */
    cn->cn_Type  =  type;
    cn->cn_ID	= id;
    cn->cn_Size  = size;
    cn->cn_Scan  =  scan;

    GetIntCN(cn)->cn_Composite = composite;
    /* Initialize the LCI-list */
    NewList ((struct List*)&( GetIntCN(cn)->cn_LCIList ));

    /* Deeper stack */
    iff->iff_Depth ++;

    ReturnInt ("PushContextNode",LONG,0L);
}

VOID PopContextNode(struct IFFHandle* iff,
    struct IFFParseBase_intern *IFFParseBase)
{
    struct LocalContextItem *node,
			    *nextnode;

    struct IntContextNode *cn;

    D(bug("PopContextNode(iff=%p)\n", iff));

    cn = GetIntCN( TopChunk( iff ) );

    if (cn)
    {
	D(bug("(%c%c%c%c, %c%c%c%c)\n",
	    dmkid(cn->CN.cn_Type),
	    dmkid(cn->CN.cn_ID)
	));
    }
    else
    {
	D(bug("\n"));
    }

    /* Free all localcontextitems */
    node = (struct LocalContextItem*)cn->cn_LCIList.mlh_Head;

    while ((nextnode = (struct LocalContextItem*)node->lci_Node.mln_Succ))
    {
	PurgeLCI((struct LocalContextItem*)node, IFFParseBase);

	node = nextnode;
    }

    /* Free the contextnode itself */
    Remove((struct Node*)cn);
    FreeMem(cn,sizeof (struct IntContextNode));

    /*One less node on stack */
    iff->iff_Depth --;
}

LONG ReadStreamLong (struct IFFHandle *iff,
    APTR valptr,
    struct IFFParseBase_intern *IFFParseBase)
{
    LONG val;
#if AROS_BIG_ENDIAN
#   define bytes valptr
#else
    UBYTE bytes[4];
#endif

    D(bug("ReadStreamLong(iff=%p valptr=%p)\n", iff, valptr));

    val = ReadStream (iff, bytes, sizeof(LONG), IFFParseBase);

    D(bug("ReadStreamLong: val %ld\n", val));

    if (val < 0)
	return val;
    else if (val != sizeof(LONG))
	return IFFERR_EOF;

#if !AROS_BIG_ENDIAN
    *(LONG *)valptr = bytes[0] << 24 | bytes[1] << 16 | bytes[2] << 8 | bytes[3];
#endif

    D(bug("ReadStreamLong: *valptr 0x%08lx '%c%c%c%c'\n", *(LONG *) valptr, dmkid(*(LONG *) valptr)));

    return sizeof(LONG);
} /* ReadStreamLong */


LONG WriteStreamLong (struct IFFHandle *iff,
    APTR valptr,
    struct IFFParseBase_intern *IFFParseBase)
{
    LONG val;
#if AROS_BIG_ENDIAN
#   define bytes valptr
#else
    UBYTE bytes[4];
    val = *(LONG *)valptr;

    bytes[0] = val >> 24;
    bytes[1] = val >> 16;
    bytes[2] = val >> 8;
    bytes[3] = val;
#endif

    D(bug("WriteStreamLong(iff=%p valptr=%p)\n", iff, valptr));

    val = WriteStream (iff, bytes, sizeof(LONG), IFFParseBase);

    D(bug("WriteStreamLong: val %ld\n", val));

    if (val < 0)
	return val;
    else if (val != sizeof(LONG))
	return IFFERR_EOF;

    return sizeof(LONG);
} /* WriteStreamLong */

/********************/
/* GetChunkHeader   */
/********************/

LONG GetChunkHeader(struct IFFHandle *iff,
    struct IFFParseBase_intern *IFFParseBase)
{
    LONG type,
	 id,
	 size;
    LONG scan = 0;
    LONG bytesread;

    D(bug("GetChunkHeader (iff=%p)\n", iff));

    /* Reads in the appropriate stuff from a chunk and makes a contextnode of it */

    /* Read chunk ID */
    bytesread = ReadStreamLong ( iff, &id, IFFParseBase );

    /* We may have an IFF Error */
    if (bytesread < 0)
	ReturnInt ("GetChunkHeader",LONG,bytesread);

    /* Read chunk size */
    bytesread = ReadStreamLong ( iff, &size, IFFParseBase );

    if (bytesread < 0)
	ReturnInt ("GetChunkHeader",LONG,bytesread);

    /* We must see if we have a IFF header ("FORM", "CAT" or "LIST" */
    if ( id == ID_FORM || id == ID_CAT || id == ID_LIST || id == ID_PROP )
    {
	/* Read chunk size */
	bytesread = ReadStreamLong ( iff, &type, IFFParseBase );

	if (bytesread < 0)
	    ReturnInt ("GetChunkHeader",LONG,bytesread);

	DB2(bug("  Found Chunk %c%c%c%c size=%d type %c%c%c%c\n",
	    dmkid(id),
	    size,
	    dmkid(type)
	));

	/* Type is inside chunk so we had to add its size to the scancount. */
	scan = sizeof (LONG);
    }
    else
    {
	type = 0L;
	DB2(bug("  Found Chunk %c%c%c%c size=%d\n",
	    dmkid(id),
	    size
	));
    }

    ReturnInt
    (
	"GetChunkHeader",
	LONG,
	PushContextNode
	(
	    iff,
	    type,
	    id,
	    size,
	    scan,
	    IFFParseBase
	)
    );
}

/********************/
/* InvokeHandlers    */
/********************/

LONG InvokeHandlers(struct IFFHandle *iff, LONG mode, LONG ident,
    struct IFFParseBase_intern *IFFParseBase)
{
    struct ContextNode	     *cn;
    struct HandlerInfo	    *hi;
    struct LocalContextItem *lci;

    LONG err;
    /* Either RETURN_2CLIENT or IFFERR_EOC */
    LONG stepping_retval;
    ULONG param;

    D(bug("InvokeHandlers (Iff=%p, mode=%d, ident=0x%08lx\n",
	iff, mode, ident
    ));

    if (ident == IFFLCI_ENTRYHANDLER)
	stepping_retval = IFF_RETURN2CLIENT;
    else
	stepping_retval = IFFERR_EOC;

    /* Check for IFFPARSE_RAWSTEP *before* calling evt entryhandlers */
    if (mode == IFFPARSE_RAWSTEP)
	ReturnInt ("InvokeHandlers(1)",LONG,stepping_retval);

    /* Get top of contextstack */
    cn = TopChunk(iff);

    /* Scan downwards to find a contextnode with a matching LCI */
    lci = FindLocalItem ( iff, cn->cn_Type, cn->cn_ID, ident );

    if (lci)
    {
	/* Get the HandlerInfo userdata */
	hi = LocalItemData(lci);


	/* First check if a hook really is present */
	if (! hi->hi_Hook)
	    ReturnInt ("InvokeHandlers",LONG,IFFERR_NOHOOK);

	/* What kind off command shall the hook receive */

	if (ident == IFFLCI_ENTRYHANDLER)
	    param = IFFCMD_ENTRY;
	else
	    param = IFFCMD_EXIT;


	/* Call the handler */
	if ( (err = CallHookPkt (  hi->hi_Hook, hi->hi_Object, (APTR)param)) )
	    ReturnInt ("InvokeHandlers(2)",LONG,err);
    }

    /* Check for IFFPARSE_STEP. (stepping through file WITH handlers enabled */
    if (mode == IFFPARSE_STEP)
	ReturnInt ("InvokeHandlers(3)",LONG,stepping_retval);

    ReturnInt ("InvokeHandlers",LONG,0L);
}

/******************/
/* PurgeLCI	   */
/******************/

VOID PurgeLCI(struct LocalContextItem *lci,
    struct IFFParseBase_intern *IFFParseBase)
{
    /* Look in the RKM SetLocalItemPurge autodoc for explanation on that one below */

    D(bug("PurgeLCI(lci=%p)\n", lci));

    Remove((struct Node*)lci);
    /* Has the user specified a Purge hook ? */

    if ( GetIntLCI(lci)->lci_PurgeHook)
	CallHookPkt
	(
	    GetIntLCI(lci)->lci_PurgeHook,
	    lci,
	    (APTR)IFFCMD_PURGELCI
	);

    else
	FreeLocalItem(lci);
}



/******************/
/* Readstream	   */
/******************/

/* Reads from the current StreamHandler */

/* returns one of the standard IFF errors */
LONG ReadStream(struct IFFHandle *iff, APTR buf, LONG bytestoread,
    struct IFFParseBase_intern *IFFParseBase)
{
    LONG   retval,
	  err;

    /* For use with custom streams */
    struct IFFStreamCmd cmd;

    D(bug("ReadStream(iff=%p buf=%p bytestoread=%d)\n", iff, buf, bytestoread));

    retval = bytestoread;
    if (bytestoread)
    {
        /* Now we can do the actual reading of the stream */
        cmd.sc_Command  = IFFCMD_READ;
        cmd.sc_Buf      = buf;
        cmd.sc_NBytes   = bytestoread;

        err = CallHookPkt
        (
	    GetIntIH(iff)->iff_StreamHandler,
	    iff,
	    &cmd
        );

        if (err)
	    retval = IFFERR_READ;
    }

    D(bug("ReadStream: return %d\n", retval));
    return (retval);
}


/****************/
/* WriteStream	*/
/****************/

LONG WriteStream(struct IFFHandle *iff, APTR buf, LONG bytestowrite,
    struct IFFParseBase_intern *IFFParseBase)
{
    LONG   retval,
	  err;

    struct IFFStreamCmd cmd;

    /* Call the custom hook with a write command */

    D(bug("WriteStream(iff=%p buf=%p bytestowrite=%d)\n", iff, buf, bytestowrite));

    retval = bytestowrite;

    if (bytestowrite)
    {
	cmd.sc_Command   = IFFCMD_WRITE;
	cmd.sc_Buf	 = buf;
	cmd.sc_NBytes    = bytestowrite;

        err = CallHookPkt
        (
	    GetIntIH(iff)->iff_StreamHandler,
	    iff,
	    &cmd
        );

        if (err)
	    retval = IFFERR_WRITE;
    }

    D(bug("WriteStream: return %d\n", retval));
    return (retval);
}
/***************/
/* SeekStream */
/***************/

LONG SeekStream(struct IFFHandle *iff,LONG offset,
    struct IFFParseBase_intern *IFFParseBase)
{

    /* Some different problem - situations:

	1. Backwards seek in non back seekable stream. In this case the stream is buffered,
	  and we may seek in the buffer. This is done automagically, since PushChunk
	  then has inserted a Buffering streamhandle.


	2. Forwards seek in a non - seekable stream. Simulate the seek with a bunch
	  of ReadStream's

    */

    struct IFFStreamCmd cmd;
    ULONG flags;
    LONG retval = 0;
    LONG err;
    UBYTE *seekbuf;

    D(bug("SeekStream(iff=%p offset=%d)\n", iff, offset));

    flags = iff->iff_Flags;


    /* Problem 2. */
    if
    (
	(offset > 0)
    &&
	(
	    !(
		(flags & IFFF_RSEEK)
	    ||
		(flags & IFFF_FSEEK)
	    )
	)
    )
    {
	/* We should use consecutive ReadStream()s to simulate a Seek */

	/* Allocote a buffer to use with the read */
	seekbuf = AllocMem(SEEKBUFSIZE, MEMF_ANY);

	if (!seekbuf)
	    retval = IFFERR_NOMEM;
	else
	{

	    for (; offset > SEEKBUFSIZE; offset -= SEEKBUFSIZE)
	    {
		retval = ReadStream(iff, seekbuf, SEEKBUFSIZE, IFFParseBase);

		if (retval != SEEKBUFSIZE)
		    retval = IFFERR_SEEK;
	    }

	    /* Seek what is left of offset  */
	    retval = ReadStream(iff, seekbuf, SEEKBUFSIZE, IFFParseBase);
	    if ( retval != SEEKBUFSIZE)
		retval = IFFERR_SEEK;

	    FreeMem(seekbuf, SEEKBUFSIZE);
	}

    }
    else if (offset == 0)
    {
	; /* Do nothing */
    }
    else
    {

	/* Everything is just fine... Seek in a normal manner */

	cmd.sc_Command = IFFCMD_SEEK;
	cmd.sc_NBytes	   = offset;

	err = CallHookPkt
	(
	    GetIntIH(iff)->iff_StreamHandler,
	    iff,
	    &cmd
	);

	if (err)
	      retval = IFFERR_SEEK;
    }

    D(bug("SeekStream: return %d\n", retval));
    return (retval);
}


/********************/
/* Buffering stuff  */
/********************/
