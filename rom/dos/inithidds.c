/*
    Copyright (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Code that loads and initializes necessary HIDDs.
    Lang: english
*/

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

#include "../graphics/graphics_private.h"

#ifdef _AROS
#include <aros/asmcall.h>
#endif /* _AROS */

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

struct initbase
{
    struct ExecBase	*sysbase;
    struct DosLibrary	*dosbase;
    struct Library	*oopbase;
};

#define SysBase	(base->sysbase)
#define DOSBase (base->dosbase)
#define OOPBase (base->oopbase)

struct hiddprefs
{
    STRPTR basehiddfile;
    STRPTR prefsfile;
    BOOL (*init)(struct Library *hiddbase, struct initbase *base);
    
};

static BOOL init_gfx(struct Library *hiddbase, struct initbase *base);
static BOOL init_hiddtype(struct hiddprefs *hp, struct initbase *base);

/************************************************************************/



static const struct hiddprefs hprefs[] =
{
    { "Sys:hidds/graphics.hidd", "Sys:s/gfxhidd.prefs", init_gfx },
    { NULL, NULL, 0 }
}; 


#define HIDDPATH "Sys:hidds/"
#define BUFSIZE 100


BOOL init_hidds(struct ExecBase *sysBase, struct DosLibrary *dosBase)
{
/* This is the initialisation code for InitHIDDs module */


    struct initbase stack_b, *base = &stack_b;
    BOOL success = TRUE;
    
    
    base->sysbase = sysBase;
    base->dosbase = dosBase;

    EnterFunc(bug("init_hidds\n"));
    
    OOPBase = OpenLibrary(AROSOOP_NAME, 0);
    if (!OOPBase)
    {
    	success = FALSE;
    }
    else
    {
    	struct hiddprefs *hp;
    
	D(bug("OOP opened\n"));
    
    	for (hp = (struct hiddprefs *)hprefs; hp->basehiddfile && success; hp ++)
    	{
	     D(bug("Initing %s\n", hp->basehiddfile));
    	     success = init_hiddtype(hp, base);
	    
	} /* for (each hidd to init) */
	
	CloseLibrary(OOPBase);
    }
    
    ReturnBool("init_hidds", success);
}

/**********************
**  init_hiddtype()  **
**********************/

/* Init hidd of a single type f.ex gfx */
static BOOL init_hiddtype(struct hiddprefs *hp, struct initbase *base)
{
    struct Library *basehidd;
    BOOL success = FALSE;
    
    EnterFunc(bug("init_hiddtype(basehiddfile=%s, prefsfile=%s)\n",
    		hp->basehiddfile, hp->prefsfile));
    
    basehidd = OpenLibrary(hp->basehiddfile, 0);
    if (basehidd)
    {

	/* Read the prefs file */
	BPTR fh;
	    
    	D(bug("basehidd opened\n"));
	
	D(bug("Trying to open %s\n", hp->prefsfile));
	fh = Open(hp->prefsfile, MODE_OLDFILE);
	if (fh)
	{	    
	    UBYTE hiddfile[BUFSIZE];
	    UBYTE *append = hiddfile;
	    
	    D(bug("Opened file %s\n", hp->prefsfile));
	    
/*	    strcpy(hiddfile, HIDDPATH);
	    
	     Find end of hiddfile buffer
	    for (append = hiddfile; *append; append ++)
		;
		
*/	    /* Get name of HIDD */
	    if (FGets(fh, append, BUFSIZE - sizeof (HIDDPATH) + 1))
	    {
		struct Library *subhiddbase;
		
	    	/* Open the HIDD */
		D(bug("Trying to open HIDD %s\n", hiddfile));
		subhiddbase = OpenLibrary(hiddfile, 0);
		if (subhiddbase)
		{
		    success = hp->init(subhiddbase, base);
		    
		}
		
	   } /* if (able to read hidd prefs) */
	   	
	   Close(fh);
	} /* if (prefsfile opened) */
	    
	if (!success)
	     CloseLibrary(basehidd);
    } /* if (basehidd inited) */
    
    ReturnBool("init_hiddtype", success);
}
	
/*****************
**  init_gfx()  **
*****************/

static BOOL init_gfx(struct Library *hiddbase, struct initbase *base)
{
    struct GfxBase *GfxBase;
    BOOL success = FALSE;
    
    EnterFunc(bug("init_gfx(hiddbase=%p)\n", hiddbase));
    
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
    if (GfxBase)
    {
    	D(bug("gfx.library opened\n"));

	/*  Call private gfx.library call to init the HIDD.
	    Gfx library is responsable for closing the HIDD
	    library (although it will probably not be neccesary).
	*/

    	D(bug("calling private gfx LateGfxInit()\n"));
	if (LateGfxInit(hiddbase))
	{
	    D(bug("success\n"));
	    success = TRUE;
	}
	
	CloseLibrary((struct Library *)GfxBase);
	
    }
    ReturnBool ("init_gfxhidd", success);
}	    

/*
#include <dos/dosextens.h>
static VOID printdoslist(struct initbase *base)
{
    struct DosList *dl;
    dl = LockDosList(LDF_READ|LDF_DEVICES|LDF_VOLUMES|LDF_ASSIGNS);
    
    while ((dl = NextDosEntry(dl, LDF_READ|LDF_DEVICES|LDF_VOLUMES|LDF_ASSIGNS)))
    {
    	D(bug("device: %s\n", dl->dol_DevName));    
    }
    UnLockDosList(LDF_READ|LDF_DEVICES|LDF_VOLUMES|LDF_ASSIGNS);
    return;
}

*/
