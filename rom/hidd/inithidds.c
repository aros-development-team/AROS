/*
    Copyright (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Code that loads and initializes necessary HIDDs.
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <hidd/hidd.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <oop/oop.h>

#include "graphics_private.h"

#ifdef _AROS
#include <aros/asmcall.h>
#endif /* _AROS */

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

static const UBYTE name[];
static const UBYTE version[];
static ULONG AROS_SLIB_ENTRY(init,InitHIDDs)();
extern const char InitHIDDs_End;

int InitHIDDs_entry(void)
{
    return -1;
}

const struct Resident InitHIDDs_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&InitHIDDs_resident,
    (APTR)&InitHIDDs_End,
    RTF_AFTERDOS,
    41,
    NT_UNKNOWN,
    -126, /* Has to be after OOP */
    (UBYTE *)name,
    (UBYTE *)version,
    (APTR)&AROS_SLIB_ENTRY(init,InitHIDDs)
};

static const UBYTE name[] = "inithidds";
static const UBYTE version[] = "inithidds 41.1 (16.9.1998)\r\n";


struct initbase
{
    struct ExecBase	*sysbase;
    struct DosLibrary	*dosbase;
    struct Library	*oopbase;
};

#define DOSBase (base->dosbase)
#define OOPBase (base->oopbase)

static BOOL init_hidds(struct initbase *);
static BOOL init_gfx(struct Library *hiddbase, struct initbase *base);

/************************************************************************/

static BOOL init_hidds(struct initbase *base);

/* This is the initialisation code for InitHIDDs module */

AROS_UFH3(static ULONG, AROS_SLIB_ENTRY(init, InitHIDDs),
    AROS_UFHA(ULONG, dummy1, D0),
    AROS_UFHA(ULONG, dummy2, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{

    struct initbase stack_b, *base = &stack_b;
    BOOL success = FALSE;
    
    base->sysbase = SysBase;
    
    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37);
    if (DOSBase)
    {
        OOPBase = OpenLibrary(AROSOOP_NAME, 0);
	if (OOPBase)
	{
	    if (init_hidds(base))
	    {
		success = TRUE;
	    }
	    CloseLibrary(OOPBase);
	}
	CloseLibrary((struct Library *)DOSBase);
    }
    

    return success;
}

#define SysBase (base->sysbase)

#define HIDDPATH "Sys:hidds/"
#define BUFSIZE 100


const struct hiddprefs
{
    STRPTR file;
    BOOL (*init)(struct Library *hiddbase, struct initbase *base);
    
}
hprefs[] =
{
    { "S:gfxhidd.prefs", init_gfx },
    { NULL, 0 }
}; 


static BOOL init_hidds(struct initbase *base)
{

    struct hiddprefs *hp;
    BOOL success = TRUE;
    
    for (hp = hprefs; hprefs->file && success; hp ++)
    {
    
    	/* Read the prefs file */
	BPTR fh;
	fh = Open(hp->file, MODE_OLDFILE);
	if (!fh)
	{
	    success = FALSE;
	}
	else
	{
	    UBYTE hiddfile[BUFSIZE];
	    UBYTE *append;
	    
	    strcpy(hiddfile, HIDDPATH);
	    
	    /* Find end of hiddfile buffer */
	    for (append = hiddfile; *append; append ++)
	    	;
		
	    /* Get name of HIDD */
	    if (!FGets(fh, append, BUFSIZE - sizeof (HIDDPATH) + 1))
	    {
	        success = FALSE;
	    }
	    {
	    	struct Library *HiddBase;
	    	/* Open the HIDD */
		HiddBase = OpenLibrary(hiddfile, 0);
		if (HiddBase)
		{
		    success = hp->init(HiddBase, base);
		    
		}
	    }
	    Close(fh);
	}
	
    }
    return success;
}

/*****************
**  init_gfx()  **
*****************/

static BOOL init_gfx(struct Library *hiddbase, struct initbase *base)
{
    struct GfxBase *GfxBase;
    BOOL success = FALSE;
    
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
    if (GfxBase)
    {

	/*  Call private gfx.library call to init the HIDD.
	    Gfx library is responsable for closing the HIDD
	    library (although it will probably not be neccesary).
	*/

	if (InitGfxHidd(hiddbase))
	{
	    success = TRUE;
	}
	
	CloseLibrary((struct Library *)GfxBase);
	
    }
    return success;
}	    
		    

const char InitHIDDs_end = NULL;
