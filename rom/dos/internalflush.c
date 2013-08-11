/*
    Copyright © 2002-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <proto/dos.h>
#include <aros/debug.h>

#include "dos_intern.h"

LONG InternalFlush( struct FileHandle *fh, struct DosLibrary *DOSBase )
{
    LONG position, pos;
    
    /* Make sure the input parameters are sane. */
    ASSERT_VALID_PTR( fh );
    ASSERT_VALID_PTR( BADDR(fh->fh_Buf) );

    ASSERT(fh->fh_End >  0);
    ASSERT(fh->fh_Pos >= 0);
    ASSERT(fh->fh_Pos <= fh->fh_End );

    /* If other task is flushing the buffer wait for completion.
       Currently we just delay 1 tick, we assume task that is flushing will
       be scheduled during first Delay() call so we have to wait only once.
    */
    while (fh->fh_Flags & FHF_FLUSHING)
        Delay(1);

    /* TODO: Is following line race free ? */
    fh->fh_Flags |= FHF_FLUSHING;

    /* Write the data, in many pieces if the first one isn't enough. */
    position = 0;
    /* Remember current position */
    pos = fh->fh_Pos;

    while( position < pos )
    {
        LONG size;
    
        size = Write( MKBADDR(fh), BADDR(fh->fh_Buf) + position, fh->fh_Pos - position );
    
        /* An error happened? No success. */
        if( size < 0 )
        {
            fh->fh_Flags &= ~(FHF_WRITE | FHF_FLUSHING);

            return FALSE;
        }
    
        position += size;
    }
    
    /* Reset the buffer. */
    if (fh->fh_Pos == pos)
        fh->fh_Pos = 0;
    else
    {
        /* If we are here it means that During Flush() characters were added
           to the buffer. Try to remove only the part that was flushed.
           This implementation is not fully race free but we try our best to
           reduce data loss as much as possible.
        */
        int i, len = fh->fh_Pos - pos;
        char *dest = BADDR(fh->fh_Buf);
        const char *src = dest + len;
        for (i = 0; i < len; i++)
            *dest++ = *src++;
        fh->fh_Pos -= pos;
    }

    fh->fh_Flags &= ~FHF_FLUSHING;

    return TRUE;
}
