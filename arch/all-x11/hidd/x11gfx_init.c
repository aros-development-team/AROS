/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: X11 graphics hidd initialization code.
    Lang: English.
*/
#define AROS_ALMOST_COMPATIBLE
#include <stddef.h>
#include <exec/types.h>

#include <proto/exec.h>

#include "x11gfx_intern.h"

#warning FIXME: define NT_HIDD in libraries.h or something else
#define NT_HIDD NT_LIBRARY

#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->sysbase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->seglist)
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->library)

#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB

/* to avoid removing the gfxhiddclass from memory add #define NOEXPUNGE */

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

#define SysBase      (LC_SYSBASE_FIELD(lh))

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{

    if (!lh->oopbase)
    	lh->oopbase = OpenLibrary(AROSOOP_NAME, 37);
    if (!lh->oopbase)
	return (NULL);


    if (!lh->utilitybase)
	lh->utilitybase = OpenLibrary("utility.library", 0);
    if (!lh->utilitybase)
	return(NULL);


    /* Create HIDD classes */
    if (!lh->gfxclass)
    	lh->gfxclass = init_gfxclass(lh);
    if (!lh->gfxclass)
    	return (NULL);
    
    if (!lh->gcclass)
    	lh->gcclass = init_gcclass(lh);
    if (!lh->gcclass)
    	return (NULL);
    	
    if (!lh->bitmapclass)
    	lh->bitmapclass = init_bitmapclass(lh);
    if (!lh->bitmapclass)
    	return (NULL);
	
    return TRUE;
        
}


void  SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR lh)
{
    if (lh->bitmapclass)
	cleanup_class(lh->bitmapclass, lh);
	    
    if (lh->gcclass)
	cleanup_class(lh->gcclass, lh);
	    
    if (lh->gfxclass)
	cleanup_class(lh->gfxclass, lh);
	
    if (lh->utilitybase)
    {
	CloseLibrary(lh->utilitybase);
	lh->utilitybase = NULL;
    }
	
    if (lh->oopbase)
    {
	CloseLibrary(lh->oopbase);
	lh->oopbase = NULL;
    }

    return;
}

