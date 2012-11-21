/*
    Copyright © 2002-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/dos.h>
#include "dos_intern.h"

LONG InternalSeek
( 
    struct FileHandle *fh, 
    LONG               position, 
    LONG               mode, 
    struct DosLibrary *DOSBase 
)
{
    LONG  status;
    SIPTR doserror = 0;

    /* Make sure the input parameters are sane. */
    ASSERT_VALID_PTR( fh );
    ASSERT
    ( 
           mode == OFFSET_BEGINNING 
        || mode == OFFSET_END 
        || mode == OFFSET_CURRENT 
    );
    D(bug("[seek] %x:%d:%d\n", fh, position, mode));
    do {
        status = dopacket3(DOSBase, &doserror, fh->fh_Type, ACTION_SEEK, fh->fh_Arg1, position, mode);
        D(bug("=%d %d\n", status, doserror));
    } while (status == -1 && !ErrorReport(doserror, REPORT_STREAM, (IPTR)fh, NULL));

    return status;
}
