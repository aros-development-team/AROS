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
#define LC_RESIDENTNAME		X11Gfx_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		9
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->library)

#define LC_NO_INITLIB
#define LC_NO_EXPUNGELIB

/* to avoid removing the gfxhiddclass from memory add #define NOEXPUNGE */

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

#define SysBase      (LC_SYSBASE_FIELD(lh))

ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR lh)
{

    if (!lh->oopbase)
    	lh->oopbase = OpenLibrary(AROSOOP_NAME, 37);
    if (!lh->oopbase)
	return 0;


    if (!lh->utilitybase)
	lh->utilitybase = OpenLibrary("utility.library", 0);
    if (!lh->utilitybase)
	return 0;


    /* Create HIDD classes */
    if (!lh->gfxclass)
    	lh->gfxclass = init_gfxclass(lh);
    if (!lh->gfxclass)
    	return 0;
    
    if (!lh->osbitmapclass)
    	lh->osbitmapclass = init_osbitmapclass(lh);
    if (!lh->osbitmapclass)
    	return 0;
	
    if (!lh->bitmapclass)
    	lh->bitmapclass = init_bitmapclass(lh);
    if (!lh->bitmapclass)
    	return 0;
	

    return TRUE;
        
}


void  SAVEDS STDARGS LC_BUILDNAME(L_CloseLib) (LC_LIBHEADERTYPEPTR lh)
{
    if (lh->bitmapclass)
    {
	free_bitmapclass( lh );
	lh->bitmapclass = NULL;
    }
	    
    if (lh->osbitmapclass)
    {
	free_osbitmapclass( lh );
	lh->osbitmapclass = NULL;
    }
    
    if (lh->gfxclass)
    {
	free_gfxclass( lh );
	lh->gfxclass = NULL;
    }
	
	
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

