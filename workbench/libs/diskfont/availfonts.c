/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Main fileof diskfont function AvailFonts()
    Lang: english
*/


#include "diskfont_intern.h"

#ifndef TURN_OFF_DEBUG
#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#endif

#  include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <clib/diskfont_protos.h>

	AROS_LH3(LONG, AvailFonts,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, buffer, A0),
	AROS_LHA(LONG  , bufBytes, D0),
	AROS_LHA(LONG  , flags, D1),

/*  LOCATION */
	struct Library *, DiskfontBase, 6, Diskfont)

/*  FUNCTION
        Fill the supplied buffer with info about the available fonts.
        The buffer will after function execution first contains a 
        struct AvailFontsHeader, and then an array of struct AvailFonts 
        element (or TAvailFonts elements if AFF_TAGGED is specified in the
        flags parameter). If the buffer is not big enough for the
        descriptions than the additional length needed will be returned.

    INPUTS
        buffer    - pointer to a buffer in which the font descriptions
                    should be placed.
                    
        bufBytes  - size of the supplied buffer.
        
        flags     - flags telling what kind of fonts to load,
                    for example AFF_TAGGED for tagged fonts also,
                    AFF_MEMORY for fonts in memory, AFF_DISK for fonts
                    on disk.

    RESULT
        shortage  - 0 if buffer was big enough or a number telling
                    how much additional place is needed.

    NOTES
        If the routine failes, then the afh_Numentries field
        in the AvailFontsHeader will be 0.

    EXAMPLE

    BUGS

    SEE ALSO
        OpenDiskfont(), <diskfont/diskfont.h>

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    diskfont_lib.fd and clib/diskfont_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,DiskfontBase)

    LONG retval = 0;  
    struct MinList 	filist;
    struct CopyState	cs;
#ifndef NO_FONT_CACHE
    BOOL  rebuild_cache = FALSE;
#endif    
 	D(bug("AvailFonts(buffer=%p, bufbytes=%d,flags=%d)\n", buffer, bufBytes, flags));
  
    /* Initialize the list */
  
    NEWLIST( &filist );

#ifndef NO_FONT_CACHE
    
    /* Shall we read fonts from disk ? If so, read from cache */

    if (flags & AFF_DISK)
    {
        /* OK to read cahce ? */
        if (OKToReadCache( DFB(DiskfontBase) ))
        {
            if (!ReadCache(flags, &filist,  DFB(DiskfontBase) ))
            {
                
                /* If reading the cache failed we must free everything that was read
                    and try scanning manually. Also the cache should be rebuilt 
                */
                    
                 FreeFontList(&filist, DFB(DiskfontBase) );
                 rebuild_cache = TRUE;
            }
            else
                /* Everything went successfull. No nead to scan for disk fonts
                  anymore
                */
                flags &= ~AFF_DISK;
        }
        else
            /*Set a flag to tell that cache must be rebuilt */
            rebuild_cache = TRUE;
        
            
    }
#endif
    
    /* Scan for all available fontinfos. If the cache nees to be rebuilt,
    then also scan for tags */
    if 
    (
        !ScanFontInfo
        (
        	/* If the cache is going to be rebuilt we should scan for tags too */
    	#ifndef NO_FONT_CACHE
            (rebuild_cache ? (flags | AFF_TAGGED) : flags),
	#else
	    flags,
	#endif
            &filist,
            DFB(DiskfontBase)
        )
    )
    {
        retval = 0L;
        AFH(buffer)->afh_NumEntries = 0;
    }
    else
    {
    #ifndef NO_FONT_CACHE
        /* If necessary, write the cache */
        if (rebuild_cache)
            WriteCache( &filist, DFB(DiskfontBase) );
    #endif
    
        /* Copy the fontdescriptions into the font buffer */
        if 
        (
            !CopyDescrToBuffer
            (
                UB(buffer),
                bufBytes,
                flags,
                &filist,
                &cs,
                DFB(DiskfontBase) 
            )
        )
        {
            /* To small buffer. Count how many more bytes are needed */
        
            retval = CountBytesNeeded
            (
                UB(buffer) + bufBytes,
                flags,
                &cs,
                DFB(DiskfontBase) 
            );
        
            AFH(buffer)->afh_NumEntries = 0;
        }
        else
        {
            /* Update the pointers to fontnames and tags inside the buffer */
            UpdatePointers( UB(buffer), flags, &filist, DFB(DiskfontBase) );
            retval = 0L;

        }
            
    }
    
    /* Free the fontinfo lists */
    FreeFontList( &filist, DFB(DiskfontBase) );
    
    ReturnInt ("AvailFonts", ULONG, retval);

    AROS_LIBFUNC_EXIT
    
} /* AvailFonts */
