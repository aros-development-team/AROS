/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Graphics hidd initialization code.
    Lang: English.
*/
#define AROS_ALMOST_COMPATIBLE
#include <stddef.h>
#include <exec/libraries.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <aros/asmcall.h>
#include "initstruct.h"
#include "gfxhidd_intern.h"

#define DEBUG 1
#include <aros/debug.h>


static const UBYTE name[];
static const UBYTE version[];
static ULONG AROS_SLIB_ENTRY(init,GfxHIDD)();
extern const char GfxHIDD_End;

int gfxhidd_entry(void)
{
    return -1;
}

const struct Resident GfxHIDD_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&GfxHIDD_resident,
    (APTR)&GfxHIDD_End,
    RTF_COLDSTART,
    41,
    NT_UNKNOWN,
    90, /* Has to be after OOP */
    (UBYTE *)name,
    (UBYTE *)version,
    (APTR)&AROS_SLIB_ENTRY(init,GfxHIDD)
};

static const UBYTE name[] = "gfxhiddclass";
static const UBYTE version[] = "gfxhiddclass 41.1 (5.8.1998)\r\n";




/* Predeclaration */

#undef SysBase
#undef OOPBase

AROS_UFH3(static ULONG, AROS_SLIB_ENTRY(init, GfxHIDD),
    AROS_UFHA(ULONG, dummy1, D0),
    AROS_UFHA(ULONG, dummy2, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    struct Library *OOPBase;
    struct class_static_data *csd; /* GfxHidd static data */
    
    EnterFunc(bug("GfxHIDD_Init()\n"));
    

    /*
	We map the memory into the shared memory space, because it is
	to be accessed by many processes, eg searching for a HIDD etc.

	Well, maybe once we've got MP this might help...:-)
    */
    csd = AllocMem(sizeof(struct class_static_data), MEMF_CLEAR|MEMF_PUBLIC);
    if(csd)
    {
	csd->sysbase = SysBase;
	
	D(bug("Got csd\n"));

    	csd->oopbase = OpenLibrary("oop.library", 0);
	if (csd->oopbase)
	{
	    D(bug("Got OOPBase\n"));
	    csd->utilitybase = OpenLibrary("utility.library", 37);
	    if (csd->utilitybase)
	    {
	    	D(bug("Got UtilityBase\n"));
	    	csd->gfxhiddclass = init_gfxhiddclass(csd);
	    	D(bug("GfxHiddClass: %p\n", csd->gfxhiddclass));

		if(csd->gfxhiddclass)
		{

/*	    	    csd->bitmapclass = init_bitmapclass(csd);
		    if (csd->bitmapclass)
		    {	
*/		    	csd->gcclass = init_gcclass(csd);
	    		D(bug("GCClass: %p\n", csd->gcclass));
			if (csd->gcclass)
			{	
	    		    D(bug("All fine\n"));
			    ReturnInt("GfxHIDD_Init", ULONG, TRUE);
			}
/*
		    	DisposeObject((Object *)csd->bitmapclass);
		    }	
*/		    DisposeObject((Object *)csd->gfxhiddclass);
		}
	    	CloseLibrary(csd->utilitybase);
	    }
	    CloseLibrary(csd->oopbase);
	}
	FreeMem(csd, sizeof (struct class_static_data));
    }
    
    ReturnInt("GfxHIDD_Init", ULONG, FALSE);
	
}


