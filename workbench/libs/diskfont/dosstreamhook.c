/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Hook used by endian funcs to read from a dos file.
    Lang: english
*/

/****************************************************************************************/

#include <proto/dos.h>
#include <aros/debug.h>
#include <aros/asmcall.h>
#include <aros/bigendianio.h>

/****************************************************************************************/
	
AROS_UFH3(LONG, dosstreamhook,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(BPTR,          fh,   A2),
    AROS_UFHA(ULONG       *, msg,  A1)
)
{
    AROS_USERFUNC_INIT

    LONG rc = 0;

    switch (*msg)
    {
    case BEIO_READ:
	rc = FGetC (fh);
#if 0
kprintf ("dsh: Read: %02X\n", rc);
#endif

	break;

    case BEIO_WRITE:
	rc = FPutC (fh, ((struct BEIOM_Write *)msg)->Data);
	break;

    case BEIO_IGNORE:
	Flush (fh);

	rc = Seek (fh, ((struct BEIOM_Ignore *)msg)->Count, OFFSET_CURRENT);
#if 0
kprintf ("dsh: Skip %d\n", ((struct BEIOM_Ignore *)msg)->Count);
#endif
	break;

    }

    return rc;
    AROS_USERFUNC_EXIT
} /* dosstreamhook */

/****************************************************************************************/
