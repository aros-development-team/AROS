/*
    Copyright © 2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#undef  SDEBUG
#undef  DEBUG
#define DEBUG  0

#include <exec/types.h>
#include <proto/dos.h>
#include <aros/debug.h>

#include "dos_intern.h"

LONG InternalFlush( struct FileHandle *fh, struct DosLibrary *DOSBase )
{
    AROS_LIBBASE_EXT_DECL( struct DosLibrary *, DOSBase )
    
    UBYTE *position;
    
    /* Make sure the input parameters are sane. */
    ASSERT_VALID_PTR( fh );
    ASSERT_VALID_PTR( fh->fh_Buf );
    ASSERT_VALID_PTR( fh->fh_Pos );
    ASSERT_VALID_PTR( fh->fh_End );

    ASSERT(fh->fh_End >  fh->fh_Buf);
    ASSERT(fh->fh_Pos >= fh->fh_Buf);
    ASSERT(fh->fh_Pos <= fh->fh_End );
    
    /* Write the data, in many pieces if the first one isn't enough. */
    position = fh->fh_Buf;

    while( position < fh->fh_Pos )
    {
    	LONG size;
    
    	size = Write( MKBADDR(fh), position, fh->fh_Pos - position );
    
    	/* An error happened? No success. */
    	if( size < 0 )
    	{
    	    fh->fh_Flags &= ~FHF_WRITE;
    
    	    return FALSE;
    	}
    
    	position += size;
    }
    
    /* Reset the buffer. */
    fh->fh_Pos = fh->fh_Buf;
    
    return TRUE;
}
