/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/08/13 13:55:59  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:08  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include "exec_intern.h"
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <exec/io.h>
	#include <exec/ports.h>
	#include <clib/exec_protos.h>

	__AROS_LH2(struct IORequest *, CreateIORequest,

/*  SYNOPSIS */
	__AROS_LHA(struct MsgPort *, ioReplyPort, A0),
	__AROS_LHA(ULONG,            size,        D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 109, Exec)

/*  FUNCTION
	Create an I/O request structure bound to a given messageport.
	I/O requests are normally used to communicate with exec devices
	but can be used as normal messages just as well.

    INPUTS
	ioReplyPort - Pointer to that one of your messageports where
		      the messages are replied to. A NULL port is legal
		      but then the function fails always.
	size	    - Size of the message structure including the struct
		      IORequest header. The minimal allowable size is that
		      of a struct Message.

    RESULT
	Pointer to a new I/O request structure or NULL if the function
	failed.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    __AROS_FUNC_INIT

    struct IORequest *ret=NULL;

    /* A NULL ioReplyPort is legal but has no effect */
    if(ioReplyPort==NULL)
	return NULL;

    /* Allocate the memory */
    ret=(struct IORequest *)AllocMem(size,MEMF_PUBLIC|MEMF_CLEAR);

    if(ret!=NULL)
    {
	/* Initialize it. */
	ret->io_Message.mn_ReplyPort=ioReplyPort;

	/* This size is needed to free the memory at DeleteIORequest() time. */
	ret->io_Message.mn_Length=size;
    }

    /* All done. */
    return ret;
    __AROS_FUNC_EXIT
} /* CreateIORequest */

