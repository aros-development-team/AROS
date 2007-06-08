/*
    Copyright © 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#undef SDEBUG
#undef DEBUG
#define DEBUG 0
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
    
    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys iofs;

    /* Make sure the input parameters are sane. */
    ASSERT_VALID_PTR( fh );
    ASSERT
    ( 
	   mode == OFFSET_BEGINNING 
	|| mode == OFFSET_END 
	|| mode == OFFSET_CURRENT 
    );
        
    /* Prepare I/O request. */
    InitIOFS( &iofs, FSA_SEEK, DOSBase );

    iofs.IOFS.io_Device = fh->fh_Device;
    iofs.IOFS.io_Unit   = fh->fh_Unit;

    iofs.io_Union.io_SEEK.io_Offset   = (QUAD)position;
    iofs.io_Union.io_SEEK.io_SeekMode = mode;

    /* send the request, with error reporting */
    do {
        DosDoIO(&iofs.IOFS);
    } while (iofs.io_DosError != 0 && ErrorReport(iofs.io_DosError, REPORT_STREAM, fh, NULL) == DOSFALSE);

    return iofs.io_DosError == 0 ? (LONG) iofs.io_Union.io_SEEK.io_Offset : -1;
}
