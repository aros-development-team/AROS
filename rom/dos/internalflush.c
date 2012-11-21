/*
    Copyright © 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <proto/dos.h>
#include <aros/debug.h>

#include "dos_intern.h"

LONG InternalFlush( struct FileHandle *fh, struct DosLibrary *DOSBase )
{
    LONG position;
    
    /* Make sure the input parameters are sane. */
    ASSERT_VALID_PTR( fh );
    ASSERT_VALID_PTR( BADDR(fh->fh_Buf) );

    ASSERT(fh->fh_End >  0);
    ASSERT(fh->fh_Pos >= 0);
    ASSERT(fh->fh_Pos <= fh->fh_End );
    
    /* Write the data, in many pieces if the first one isn't enough. */
    position = 0;

    while( position < fh->fh_Pos )
    {
        LONG size;
    
        size = Write( MKBADDR(fh), BADDR(fh->fh_Buf) + position, fh->fh_Pos - position );
    
        /* An error happened? No success. */
        if( size < 0 )
        {
            fh->fh_Flags &= ~FHF_WRITE;
    
            return FALSE;
        }
    
        position += size;
    }
    
    /* Reset the buffer. */
    fh->fh_Pos = 0;
    
    return TRUE;
}
