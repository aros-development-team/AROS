/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Basic help functions needed by iffparse.
    Lang: English.
*/
#include "iffparse_intern.h"

LONG PushContextNode
(
    struct IFFHandle   *iff,
    LONG	       type,
    LONG	       id,
    LONG	       size,
    LONG	       scan,
    struct IFFParseBase *IFFParseBase
)
{
    /* Allocates and puts a new context-node into the top of the context-stack
     Also does GoodType and GoodID checking  */

    struct ContextNode	 *cn;

    BOOL composite;


    /* Set the composite flag if we have a composite contextnnode */
    if
    (
	id == ID_FORM
    ||
	id == ID_LIST
    ||
	id == ID_CAT
    ||
	id == ID_PROP
    )
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
    if
    (
	!(
	    GoodType(type)
	&&
	    GoodID(id)
	)
    )
	return (IFFERR_MANGLED);

    /* Allocate a new context node */
    if
    (
	!(cn = AllocMem
	    (
	    sizeof (struct IntContextNode),
	    MEMF_ANY
	    )
	)
    )
	return (IFFERR_NOMEM);

    /* Put the context node at top of the stack */
    AddHead
    (
	(struct List*)&( GetIntIH(iff)->iff_CNStack ),
	(struct Node*)cn
    );


    /* Set the contextnode attrs */
    cn->cn_Type  =  type;
    cn->cn_ID	= id;
    cn->cn_Size  = size;
    cn->cn_Scan  =  scan;

    GetIntCN(cn)->cn_Composite = composite;
    /* Initialize the LCI-list */
    NewList
    (
	(struct List*)&( GetIntCN(cn)->cn_LCIList )
    );



    /* Deeper stack */
    iff->iff_Depth ++;

    return (NULL);
}

VOID PopContextNode(struct IFFHandle* iff,
    struct IFFParseBase *IFFParseBase)
{
    struct LocalContextItem *node,
			    *nextnode;

    struct IntContextNode *cn;

    cn = GetIntCN( TopChunk( iff ) );

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
    return;
}

/********************/
/* GetChunkHeader   */
/********************/

LONG GetChunkHeader(struct IFFHandle *iff,
    struct IFFParseBase *IFFParseBase)
{
    LONG type,
	 id,
	 size;

    LONG scan = 0;

    LONG bytesread;

    /* Reads in the appropriate stuff from a chunk and makes a contextnode of it */

	/* Read chunk ID */
    ReadStreamLong
    (
	iff,
	&id,
	&bytesread
    );

    /* We may have an IFF Error */
      if ( bytesread < 0) return (bytesread);

    /* Read chunk size */
    ReadStreamLong
    (
	iff,
	&size,
	&bytesread
    );

    if ( bytesread < 0) return (bytesread);

    /* We must see if we have a IFF header ("FORM", "CAT" or "LIST" */
    if
    (
	id == ID_FORM
    ||
	id == ID_CAT
    ||
	id == ID_LIST
    ||
	id == ID_PROP
    )
    {
	/* Read chunk size */
	ReadStreamLong
	(
	    iff,
	    &type,
	    &bytesread
	);

	if ( bytesread < 0) return (bytesread);

	/* Type is inside chunk so we had to add its size to the scancount. */
	scan = sizeof (LONG);

    }


    return
    (
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
    struct IFFParseBase *IFFParseBase)
{
    struct ContextNode	     *cn;
    struct HandlerInfo	    *hi;
    struct LocalContextItem *lci;

    LONG err;

    /* Either RETURN_2CLIENT or IFFERR_EOC */
    LONG stepping_retval;

    ULONG param;

    if (ident == IFFLCI_ENTRYHANDLER)
	stepping_retval = IFF_RETURN2CLIENT;
    else
	stepping_retval = IFFERR_EOC;

    /* Check for IFFPARSE_RAWSTEP *before* calling evt entryhandlers */
    if (mode == IFFPARSE_RAWSTEP)
	return (stepping_retval);


    /* Get top of contextstack */
    cn = TopChunk(iff);

    /* Scan downwards to find a contextnode with a matching LCI */

    lci = FindLocalItem
    (
	iff,
	cn->cn_Type,
	cn->cn_ID,
	ident
    );

    if (lci)
    {
	/* Get the HandlerInfo userdata */
	hi = LocalItemData(lci);


	/* First check if a hook really is present */
	if (! hi->hi_Hook)
	    return (IFFERR_NOHOOK);

	/* What kind off command shall the hook receive */

	if (ident == IFFLCI_ENTRYHANDLER)
	    param = IFFCMD_ENTRY;
	else
	    param = IFFCMD_EXIT;


	/* Call the handler */
	if ( (err = CallHookPkt (  hi->hi_Hook, hi->hi_Object, (APTR)param)) )
	    return (err);
    }

    /* Check for IFFPARSE_STEP. (stepping through file WITH handlers enabled */
    if (mode == IFFPARSE_STEP)
	return (stepping_retval);

    return (NULL);
}

/******************/
/* PurgeLCI	   */
/******************/

VOID PurgeLCI(struct LocalContextItem *lci,
    struct IFFParseBase *IFFParseBase)
{
    /* Look in the RKM SetLocalItemPurge autodoc for explanation on that one below */

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

    return;
}



/******************/
/* Readstream	   */
/******************/

/* Reads from the current StreamHandler */

/* returns one of the standar IFF errors */
LONG ReadStream(struct IFFHandle *iff, APTR buf, LONG bytestoread,
    struct IFFParseBase *IFFParseBase)
{
    LONG   retval,
	  err;

    /* For use with custom streams */
    struct IFFStreamCmd cmd;

    /* Now we can do the actual reading of the stream */
    cmd.sc_Command  = IFFCMD_READ;
    cmd.sc_Buf	    = buf;
    cmd.sc_NBytes    = bytestoread;

    err = CallHookPkt
    (
	GetIntIH(iff)->iff_StreamHandler,
	iff,
	&cmd
    );
    if (err)
	    retval = IFFERR_READ;
	else
	    retval = bytestoread;

    return (retval);
}


/****************/
/* WriteStream	*/
/****************/

LONG WriteStream(struct IFFHandle *iff, APTR buf, LONG bytestowrite,
    struct IFFParseBase *IFFParseBase)
{
    LONG   retval,
	  err;

    struct IFFStreamCmd cmd;

    /* Call the custom hook with a write command */

    cmd.sc_Command   = IFFCMD_WRITE;
    cmd.sc_Buf	    = buf;
    cmd.sc_NBytes    = bytestowrite;

    err = CallHookPkt
    (
	GetIntIH(iff)->iff_StreamHandler,
	iff,
	&cmd
    );

    if (err)
	retval = IFFERR_WRITE;
    else
	retval = bytestowrite;

    return (retval);
}
/***************/
/* SeekStream */
/***************/

LONG SeekStream(struct IFFHandle *iff,LONG offset,
    struct IFFParseBase *IFFParseBase)
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

    LONG retval = NULL;

    LONG err;

    UBYTE *seekbuf;

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

	err =  CallHookPkt
	(
	    GetIntIH(iff)->iff_StreamHandler,
	    iff,
	    &cmd
	);

	if (err)
	      retval = IFFERR_SEEK;
    }

    return (retval);
}


/********************/
/* Buffering stuff  */
/********************/




/************************/
/* Endian conversions	 */
/************************/

/* If the CPU is little endian, the id will be switched to the opposite
endianess of what it is now.

Have tried to put this into a macro, but only got heaps of errors.

*/

LONG SwitchIfLittleEndian(LONG id)
{

#if (AROS_BIG_ENDIAN == 0)
    id = ((id & 0xFF000000) >> 24)
      | ((id & 0x00FF0000) >> 8)
      | ((id & 0x0000FF00) << 8)
      | ((id & 0x000000FF) << 24);
#endif

  return (id);
}
