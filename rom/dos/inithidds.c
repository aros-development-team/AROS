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
#include <ctype.h>

#include "../graphics/graphics_private.h"	/* LateGfxInit proto	*/
#include "../intuition/intuition_private.h"	/* LateIntuiInit proto	*/

#ifdef _AROS
#include <aros/asmcall.h>
#endif /* _AROS */

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#warning This is just a temporary and hackish way to get the HIDDs up and working

struct initbase
{
    struct ExecBase	*sysbase;
    struct DosLibrary	*dosbase;
    struct Library	*oopbase;
};

#define SysBase	(base->sysbase)
#define DOSBase (base->dosbase)
#define OOPBase (base->oopbase)


static BOOL init_gfx(STRPTR gfxclassname, struct initbase *base);

/************************************************************************/


#define HIDDPATH "Sys:hidds/"
#define BUFSIZE 100

#define HIDDPREFSFILE "Sys:s/hidd.prefs"


BOOL init_hidds(struct ExecBase *sysBase, struct DosLibrary *dosBase)
{
/* This is the initialisation code for InitHIDDs module */


    struct initbase stack_b, *base = &stack_b;
    BOOL success = TRUE;
    UBYTE buf[BUFSIZE];
    
    
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
	BPTR fh;
    
	D(bug("OOP opened\n"));
	
	/* Open the hidd prefsfile */
	
	fh = Open(HIDDPREFSFILE, MODE_OLDFILE);
	if (!fh)
	{
	    success = FALSE;
	}
	else
	{
	    D(bug("hiddprefs file opened\n"));
	    while (FGets(fh, buf, BUFSIZE))
	    {
	        STRPTR keyword = buf, arg, end;
		
		D(bug("Got line\n"));
		D(bug("Line: %s\n", buf));
		  
		  /* Get keywoard */
		while ((*keyword != 0) && isspace(*keyword))
		    keyword ++;
		
		if (*keyword == 0)
		    continue;
			
		  /* terminate keyword */
		arg = keyword;
		while ((*arg != 0) && (!isblank(*arg)))
		{
		    arg ++;
		}
		if (*arg == 0)
		    continue;
		    
		*arg = 0;
		  
		arg ++;
		
		  /* Find start of argument */
		D(bug("Find argument at %s\n", arg));
		while ((*arg != 0) && isblank(*arg))
		    arg ++;
		 
		if (*arg == 0)
		    continue;
		    
		D(bug("Terminate argument at %s\n", arg));
		  /* terminate argument */
		end = arg;
		while ( (*end != 0) && (!isblank(*end)))
		    end ++;
		if (*end != 0)
		    *end = 0;
		 
		D(bug("Got keyword \"%s\"\n", keyword));
		D(bug("Got arg \"%s\"\n", arg));
		
		if (0 == strcmp(keyword, "library"))
		{
		    D(bug("Opening library\n"));
		      /* Open a specified library */
		    if (NULL == OpenLibrary(arg, 0))
		    {
		        success = FALSE;
			break;
		    }
		    
		}
		else if (0 == strcmp(keyword, "gfx"))
		{
		    if (!init_gfx(arg, base))
		    {
		        success = FALSE;
			break;
		    }
		    
		}
		
/*		else if (0 == strcmp(keyword, "mouse"))
		{
		    if (!init_mouse(arg, OOPBase))
		    {
		        success = FALSE;
			break;
		    }
		}
		else if (0 == strcmp(keyword, "kbd"))
		{
		    if (!init_kbd(arg, OOPBase))
		    {
		        success = FALSE;
			break;
		    }
		}
*/		  
		  
	    }
	    
	    Close(fh);
	
	}
    
	CloseLibrary(OOPBase);
    }
    
    ReturnBool("init_hidds", success);
}

/*****************
**  init_gfx()  **
*****************/

static BOOL init_gfx(STRPTR gfxclassname, struct initbase *base)
{
    struct GfxBase *GfxBase;
    BOOL success = FALSE;
    
    EnterFunc(bug("init_gfx(hiddbase=%s)\n", gfxclassname));
    
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
    if (GfxBase)
    {
    	D(bug("gfx.library opened\n"));

	/*  Call private gfx.library call to init the HIDD.
	    Gfx library is responsable for closing the HIDD
	    library (although it will probably not be neccesary).
	*/

    	D(bug("calling private gfx LateGfxInit()\n"));
	if (LateGfxInit(gfxclassname))
	{
	    struct IntuitionBase *IntuitionBase;
	    D(bug("success\n"));
	    
	    /* Now that gfx. is guaranteed to be up & working, let intuition open WB screen */
	    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
	    if (IntuitionBase)
	    {
	    	if (LateIntuiInit(NULL))
	    	{
	    	    success = TRUE;
		}
		CloseLibrary((struct Library *)IntuitionBase);
	    }
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
