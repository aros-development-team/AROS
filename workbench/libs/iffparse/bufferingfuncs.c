/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Funtions needed for buffering writes.
*/
#include "iffparse_intern.h"



/****************/
/* AllocBuffer	*/
/****************/

struct BufferList *AllocBuffer(ULONG bufnodesize,
    struct IFFParseBase_intern * IFFParseBase)
{
    struct BufferList *buflist;
    struct BufferNode *bufnode;

    DEBUG_ALLOCBUFFER(dprintf("AllocBuffer: bufnodesize 0x%lx\n", bufnodesize));

    /* First allocate memory for the BufferList structure */

    buflist = AllocMem(sizeof (struct BufferList), MEMF_ANY);

    if (buflist)
    {
	/* Initialize teh bufferlist */
	NewList( (struct List*)&(buflist->bl_ListHeader) );
	buflist->bl_BufferNodeSize = bufnodesize;


	/* The bufferlist should always contain at least one buffer */

	bufnode = AllocBufferNode(buflist, IFFParseBase);

	if (bufnode)
	{
	    buflist->bl_CurrentNode	 = bufnode;
	    buflist->bl_CurrentOffset	 =  0;
	    buflist->bl_BufferSize	=  0;
	    buflist->bl_CurrentNodeNum	= 1;

	    DEBUG_ALLOCBUFFER(dprintf("AllocBuffer: return %p\n", buflist));
	    return (buflist);
	}

	FreeMem(buflist, sizeof (struct BufferList) );
    }

    DEBUG_ALLOCBUFFER(dprintf("AllocBuffer: return NULL\n"));
    return NULL;
}


/**************/
/* FreeBuffer  */
/**************/

/* Frees all the buffers and buffernodes inside a bufferlist */

VOID FreeBuffer(struct BufferList *buflist,
    struct IFFParseBase_intern * IFFParseBase)
{
    /* Frees all the buffernodes in a bufferlist */
    struct BufferNode *node, *nextnode;

    /* first free all the nodes */

    node = (struct BufferNode*)buflist->bl_ListHeader.mlh_Head;

    while ((nextnode=(struct BufferNode*)node->bn_Node.mln_Succ))
    {

	FreeMem(node->bn_BufferPtr, buflist->bl_BufferNodeSize);

	FreeMem(node,sizeof(struct BufferNode));
	node = nextnode;
    }

    /* Free the bufferlist itself */

    FreeMem(buflist, sizeof (struct BufferList));
    return;
}


/********************/
/* AllocBufferNode  */
/********************/

/* Allocate a new buffernode,
  a new buffer, and add the buffernode to the TAIL of the bufferlist
*/
struct BufferNode *AllocBufferNode(struct BufferList *buflist,
    struct IFFParseBase_intern * IFFParseBase)
{
    /* New buffernodes are added at the HEAD of the list */

    struct BufferNode *n;

    DEBUG_ALLOCBUFFER(dprintf("AllocBufferNode: buflist %p\n", buflist));

    /* first allocate node */
    if ((n=(struct BufferNode*)AllocMem(sizeof(struct BufferNode),MEMF_ANY)))
    {
	/* Then allocate buffer */
	if ((n->bn_BufferPtr=(UBYTE*)AllocMem(buflist->bl_BufferNodeSize, MEMF_ANY)))
	{
	    AddTail((struct List*)buflist,(struct Node*)n);

	    DEBUG_ALLOCBUFFER(dprintf("AllocBufferNode: return %p\n", n));
	    return (n);
	}

	FreeMem(n,sizeof(struct BufferNode));
    }

    DEBUG_ALLOCBUFFER(dprintf("AllocBufferNode: return NULL\n"));
    return NULL;
}

/********************/
/* WriteToBuffer    */
/********************/

/* Copies a number of bytes into the buffer at the current position */

LONG WriteToBuffer( struct BufferList *buflist, UBYTE *mem, LONG size,
    struct IFFParseBase_intern * IFFParseBase)
{
    /* The buffernode that the routine currently will buffer to */
    struct BufferNode *bufnode;


    /* Total number of bytes to be written */
    ULONG   bytes2write;

    /* Bytes left in bufnode that may be written into */
    ULONG  bytesleft;
	  /* The offset int bufnode that the routine will start buffering at */
    ULONG  bufoffset;

    ULONG  bufnodenum;
    /*
      This is the offset in bytes into the whole buffer, not obly into the
      current node
    */
    ULONG offset_into_bigbuf;


    DEBUG_WRITETOBUFFER(dprintf("WriteToBuffer: buflist %p mem %p size %ld\n", buflist, mem, size));

    bytes2write = size;

    /* Get pointer to current buffer in list */
    bufnode   = buflist->bl_CurrentNode;
    bufoffset  = buflist->bl_CurrentOffset;

    bufnodenum = buflist->bl_CurrentNodeNum;

    /* Calculate the offset into the buffer as a whole */
    offset_into_bigbuf	=  (bufnodenum  - 1) * buflist->bl_BufferNodeSize + bufoffset;

    while (bytes2write)
    {

	/* Find out how many bytes we can write into the current node */

	bytesleft = buflist->bl_BufferNodeSize - bufoffset;


	if (bytes2write > bytesleft)
	{
	    /* Copy into the old buffer all that there is place for */
	    CopyMem(mem,bufnode->bn_BufferPtr + bufoffset, bytesleft);

	    /* We have written bytesleft bytes */
	    bytes2write -= bytesleft;
	    mem += bytesleft;

	    /* No more space in this buffernode */
	    bytesleft = 0;

	    /* Go to the next buffer in the list */
	    bufnode = (struct BufferNode*)bufnode->bn_Node.mln_Succ;

	    bufnodenum ++;
	    /* We go to the start of the current buffernode */
	    bufoffset = 0;

	    if (!bufnode->bn_Node.mln_Succ)
	    {
		/* No more nodes in the list.	We have to allocate and add a new one */


		bufnode = AllocBufferNode(buflist, IFFParseBase);

		/* Did the allocation succeed ? */
		if (!bufnode)
		{
		    /* Return number of bytes written so far */
		    DEBUG_WRITETOBUFFER(dprintf("WriteToBuffer: return %ld\n", (size - bytes2write)));
		    return (size - bytes2write);
		}
	    }

	}
	else
	{
	    /* There is place enough to write the remaining bytes into the current buffer */
	    CopyMem
	    (
		mem,
		bufnode->bn_BufferPtr + bufoffset,
		bytes2write
	    );

	    bufoffset		  += bytes2write;

	    bytes2write 	=   0;
	}

    }  /* End of while */

    /* If we have reached here, we can be sure that bytes written == size
       (everything has been succesfull) */

    /* Update some stuff */
    buflist->bl_CurrentNode	 = bufnode;
    buflist->bl_CurrentOffset	 = bufoffset;
    buflist->bl_CurrentNodeNum	 = bufnodenum;

    /* Update the offset into buffer as a whole */
    offset_into_bigbuf	+= size;

    /* Have we expanded the buffer ? */
    if (offset_into_bigbuf > buflist->bl_BufferSize)
	 /* Update the size of the buffer */
	 buflist->bl_BufferSize = offset_into_bigbuf;

    DEBUG_WRITETOBUFFER(dprintf("WriteToBuffer: return %ld\n", size));
    return (size);
}

/****************/
/* SeekBuffer	 */
/****************/


BOOL SeekBuffer
(
    struct BufferList *buflist,
    LONG offset

)
{
    struct BufferNode *bufnode;

    /*	The number of bytes we potentially can seek in this buffernode's buffer
    */
    ULONG  left2seekinbuf,
	  /* The size of each buffer. (same for all buffers) */
	  bufnodesize,
	  bufoffset,
	  bufnodenum;

    LONG offset_into_bigbuf,
	  new_offset_into_bigbuf;

    DEBUG_SEEKBUFFER(dprintf("WriteToBuffer: buflist %p offset %ld\n", buflist, offset));

    /* Get the size of the buffers */
    bufnodesize  = buflist->bl_BufferNodeSize;
    bufnode	 = buflist->bl_CurrentNode;
    bufoffset	= buflist->bl_CurrentOffset;
    bufnodenum	= buflist->bl_CurrentNodeNum;

    offset_into_bigbuf	=  (bufnodenum - 1) * bufnodesize + bufoffset;

    /* Calculate the new offset into whole buffer. Remember: If the offset is negative,
       this will be substarction.*/
    new_offset_into_bigbuf = offset_into_bigbuf + offset;

    /* Are we seeking forwards, backwords or not at all ? */
    if (offset > 0)
    {
	/* Forwards seek */

	/* Are we trying to seek outside the bounds of the buffer */
	if (new_offset_into_bigbuf > buflist->bl_BufferSize)
	{
	    DEBUG_SEEKBUFFER(dprintf("WriteToBuffer: return FALSE #1\n"));
	    return (FALSE);
	}

	/* How much is there left to seek towards the end of the first buf ? */
	left2seekinbuf	= bufnodesize - bufoffset;

	while (offset) /* While there are more bytes to seek */
	{
	    if (offset > left2seekinbuf)
	    {
		/* We have to jump to the next buffer in the list */
		bufnode = (struct BufferNode*)bufnode->bn_Node.mln_Succ;

#if 0
		if (!bufnode->bn_Node.mln_Succ)
		{
		    /* Oops !! No more buffers to seek in */
		    DEBUG_SEEKBUFFER(dprintf("WriteToBuffer: return FALSE #2\n"));
		    return (FALSE);
		}
#endif

		offset -= left2seekinbuf;
		bufnodenum ++;

		/* Current offset is at start of the buffer */
		bufoffset  =  0;

		/* The number of bytes written to this buffer
		is the number we can potentially seek.
		 */
		left2seekinbuf = bufnodesize;

	    }
	    else
	    {
		/* We are at the last buffernode we have to seek. */
		bufoffset += offset;

		/* not more to seek */
		offset	 = 0;
	    }
	}  /* End of while */
    }

    else if (offset < 0)
    {

	/* Backwards seek */

	/* Are we trying to seek outside the bounds of the buffer */
	if (new_offset_into_bigbuf < 0)
	{
	    DEBUG_SEEKBUFFER(dprintf("WriteToBuffer: return FALSE #3\n"));
	    return (FALSE);
	}

	/* For simplicity we take the absolute value of offset */
	offset = abs(offset);

	/* How much is there left to seek towards the start of the first buf ? */
	left2seekinbuf	= bufoffset;

	while (offset) /* While there are more bytes to seek */
	{
	    if (offset > left2seekinbuf)
	    {

		/* We have to jump to the next buffer in the list */
		bufnode = (struct BufferNode*)bufnode->bn_Node.mln_Pred;

#if 0
		if (bufnode->bn_Node.mln_Pred == NULL )
		{
		    /* Oops !! No more buffers to seek in */
		    DEBUG_SEEKBUFFER(dprintf("WriteToBuffer: return FALSE #4\n"));
		    return (FALSE);
		}
#endif

		offset -= left2seekinbuf;
		bufnodenum --;

		/* Current offset is at end of the buffer */
		bufoffset  =  bufnodesize;

		/* The number of bytes written to this buffer
		is the number we can potentially seek.
		 */
		left2seekinbuf = bufoffset;

	    }
	    else
	    {
		/* We are at the last buffernode we have to seek. */
		bufoffset -= offset;
		offset	  = 0;

		/* not more to seek */

	    }
	}  /* End of while */
    }

    /* if offset is 0, we are finished seeking */

    buflist->bl_CurrentNode	= bufnode;
    buflist->bl_CurrentOffset	 = bufoffset;
    buflist->bl_CurrentNodeNum	= bufnodenum;

    DEBUG_SEEKBUFFER(dprintf("WriteToBuffer: return TRUE\n"));
    return (TRUE);
}

/******************/
/* BufferToStream  */
/******************/

/* Writes a whole buffer to a stream.
It is VERY important that intiff->BufferStartDepth is 0, when this
function is called. Else, WriteStream will buffer the writes, and we will
get a nice loop allocating ALL the memory of the machine. (imagine that with VM ;-)

Well, I guess I'll set that to 0 at the start of this function, just for safety.

*/

BOOL BufferToStream
(
    struct BufferList *buflist,
    struct IFFHandle   *iff,
    struct IFFParseBase_intern * IFFParseBase
)
{
    struct BufferNode *node,
		      *nextnode;


    /* Number of bytes axtually writen to the stream,
      or if negative: an IFFERR_.. error */

    LONG   byteswritten = 0,
	  bytes2write,
	  numbytes;


    DEBUG_BUFFERTOSTREAM(dprintf("BufferToStream: buflist %p iff %p\n", buflist, iff));

    numbytes = buflist->bl_BufferSize;
    /* For safety. Read at the function header. */
    GetIntIH(iff)->iff_BufferStartDepth = 0;

    nextnode = (struct BufferNode*)buflist->bl_ListHeader.mlh_Head;

    while (numbytes)
    {
	node = nextnode;

	/* Should we write more than the first buffer ? */

	if (numbytes > buflist->bl_BufferNodeSize)
	{

	    nextnode = (struct BufferNode*)node->bn_Node.mln_Succ;
	    /* Check if there are enough buffers to write numbytesf */
	    if (!nextnode)
	    {
		DEBUG_BUFFERTOSTREAM(dprintf("BufferToStream: return FALSE #1\n"));
		return (FALSE);
	    }


	    bytes2write = buflist->bl_BufferNodeSize;

	}
	else
	{
	    /* We are at the last buffer to write */

	    bytes2write = numbytes;
	}

	/* Write a buffernode to the stream */
	byteswritten = WriteStream
	(
	    iff,
	    node->bn_BufferPtr,
	    bytes2write,
	    IFFParseBase
	);

	/* Do we have a IFFERR_.. ? */
	if (byteswritten < 0)
	{
	    DEBUG_BUFFERTOSTREAM(dprintf("BufferToStream: return FALSE #2\n"));
	    return (FALSE);
	}


	/* There is bytes2write less to write */

	numbytes -= bytes2write;
    }


    DEBUG_BUFFERTOSTREAM(dprintf("BufferToStream: return %ld\n", byteswritten));
    return (byteswritten);
}

/*****************************************/
/* BufferStream initialization & cleanup */
/*****************************************/

/* Put in own functions just to make code tidier */

LONG  InitBufferedStream(struct IFFHandle *iff,
    struct IFFParseBase_intern * IFFParseBase)
{
    DEBUG_INITBUFFEREDSTREAM(dprintf("InitBufferedStream: iff %p\n", iff));

    if
    (
	!(GetIntIH(iff)->iff_BufferStartDepth)
    )
    {
	/* Preserve the stream */
	GetIntIH(iff)->iff_PreservedStream = iff->iff_Stream;

	/* Allocate buffers that WriteStream can buffer its output to */
	if (!( iff->iff_Stream = (IPTR)AllocBuffer(BUFFERNODESIZE, IFFParseBase)))
	{
	    DEBUG_INITBUFFEREDSTREAM(dprintf("InitBufferedStream: return IFFERR_NOMEM\n"));
	    return (IFFERR_NOMEM);
	}

	/* To inform WriteStream that it should buffer all input */
	/* + 1 because we have not PushContextNode()ed yet */

	GetIntIH(iff)->iff_BufferStartDepth = iff->iff_Depth + 1;

	/* Preserve the old hook and the old flags */

	GetIntIH(iff)->iff_PreservedHandler = GetIntIH(iff)->iff_StreamHandler;


	GetIntIH(iff)->iff_PreservedFlags = iff->iff_Flags;


	/* Insert the BufStreamHandler into the hook instead */
	InitIFF(iff, IFFF_RSEEK, &IFFParseBase->bufhook);

    }

    DEBUG_INITBUFFEREDSTREAM(dprintf("InitBufferedStream: return 0\n"));
    return 0;
}

LONG ExitBufferedStream(struct IFFHandle *iff,
    struct IFFParseBase_intern * IFFParseBase)
{
    /*
      If we have come to the chunk that started internal buffering, then we should do the following.

	    - Turn it off, so that WriteStream again writes to the REAL stream.
	    - Write the whole buffer to this stream.
	    - Free the buffer/
    */
    LONG err = 0;

    struct BufferList *buflist;

    DEBUG_EXITBUFFEREDSTREAM(dprintf("ExitBufferedStream: iff %p\n", iff));

    /* Turn off buffering */
    GetIntIH(iff)->iff_BufferStartDepth = 0;

      /* Read out the bufferlist stream pointer */
    buflist = (struct BufferList*)iff->iff_Stream;


    /* Now we can reinstall the old StreamHandler */
    GetIntIH(iff)->iff_StreamHandler =   GetIntIH(iff)->iff_PreservedHandler;

    /* Reinstall the old flags */
    iff->iff_Flags = GetIntIH(iff)->iff_PreservedFlags;


    /* Reinstall the old stream */
    iff->iff_Stream = GetIntIH(iff)->iff_PreservedStream;


    /* Write all the buffers into the old stream */
    if (!BufferToStream(buflist,
	                iff,
	                IFFParseBase))
    {
	err = IFFERR_WRITE;
    }

    FreeBuffer(buflist, IFFParseBase);

    DEBUG_EXITBUFFEREDSTREAM(dprintf("ExitBufferedStream: return %ld\n", err));
    return (err);
}

/**********************/
/* BufStreamHandler    */
/**********************/

#define IFFParseBase	    (IPB(hook->h_Data))

ULONG BufStreamHandler
(
    struct Hook 	*hook,
    struct IFFHandle	*iff,
    struct IFFStreamCmd  *cmd
)
{

    LONG error = 0;

    DEBUG_BUFSTREAMHANDLER(dprintf("BufStreamHandler: hook %p iff %p cmd %p\n", hook, iff, cmd));

    switch (cmd->sc_Command)
    {

	case IFFCMD_READ:

	    DEBUG_BUFSTREAMHANDLER(dprintf("BufStreamHandler: IFFCMD_READ...\n"));

	    /* It should NEVER be needed to read a buffered stream.
	      To output the buffered stream to the real stream,
	      we have the routne BufferToStream which is MUCH more effective
	    */

	    error = 1;
	    break;

	case IFFCMD_WRITE:

	    DEBUG_BUFSTREAMHANDLER(dprintf("BufStreamHandler: IFFCMD_WRITE...\n"));

	    error =
	    (
		WriteToBuffer
		(
		    (struct BufferList*)iff->iff_Stream,
		    cmd->sc_Buf,
		    cmd->sc_NBytes,
		    IFFParseBase
		)
	    !=
		cmd->sc_NBytes
	    );

	    break;

	case IFFCMD_SEEK:

	    DEBUG_BUFSTREAMHANDLER(dprintf("BufStreamHandler: IFFCMD_SEEK...\n"));

	    error =
	    (!
		SeekBuffer
		(
		    (struct BufferList*)iff->iff_Stream,
		    cmd->sc_NBytes
		)
	    );

	    break;

	case IFFCMD_INIT:
	case IFFCMD_CLEANUP:

	    DEBUG_BUFSTREAMHANDLER(dprintf("BufStreamHandler: IFFCMD_INIT/IFFCMD_CLEANUP...\n"));

	    error = 0;
	    break;

    }

    DEBUG_BUFSTREAMHANDLER(dprintf("BufStreamHandler: return %ld\n", error));
    return (error);
}

