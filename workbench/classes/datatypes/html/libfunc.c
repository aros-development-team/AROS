/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/intuition.h>

#undef DEBUG
#define DEBUG 1
#include <aros/debug.h>

/**************************************************************************************************/

struct IClass 		*dt_class;

struct ExecBase 	*SysBase;
struct IntuitionBase 	*IntuitionBase;
struct GfxBase	 	*GfxBase;
#ifdef __AROS__
struct UtilityBase	*UtilityBase;
#else
struct Library		*UtilityBase;
#endif
struct DosLibrary 	*DOSBase;
struct Library 		*DataTypesBase;
struct Library 		*IFFParseBase;
struct Library		*TextBase;

/* inside htmlclass.c */
struct IClass *DT_MakeClass(struct Library *htmlbase);

/**************************************************************************************************/

int __UserLibInit(struct Library *libbase )
{
#ifndef __AROS__
    SysBase = *(struct ExecBase**)4;
#endif

    D(bug("html.datatype/__UserLibInit\n"));
    
    if ((TextBase = OpenLibrary("datatypes/text.datatype", 0)))
    {
	if((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39)))
	{
	    if((IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39)))
	    {
		if((DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 39)))
		{
#ifdef __AROS__
		    if((UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 37)))
#else
		    if((UtilityBase = (struct Library *)OpenLibrary("utility.library", 37)))
#endif
		    {
			if((DataTypesBase = OpenLibrary("datatypes.library", 37)))
			{
			    if((IFFParseBase = OpenLibrary("iffparse.library", 37)))
			    {
				if((dt_class = DT_MakeClass(libbase)))
				{
				    AddClass(dt_class);

				    D(bug("html.datatype/__UserLibInit: Returning success\n"));
 
				    return 0;
				}
			    }
			}
		    }
		}
	    }
	}
    }

    D(bug("html.datatype/__UserLibInit: Returning failure\n"));
    
    return -1;
}

/**************************************************************************************************/

void __UserLibCleanup(struct Library *libbase )
{
    D(bug("html.datatype/__UserLibCleanup\n"));

    if(dt_class)
    {
	RemoveClass(dt_class);
	FreeClass(dt_class);
	dt_class = NULL;
    }

    if(TextBase) CloseLibrary(TextBase);
    if(IFFParseBase) CloseLibrary(IFFParseBase);
    if(DataTypesBase) CloseLibrary(DataTypesBase);
    if(UtilityBase) CloseLibrary((struct Library *)UtilityBase);
    if(DOSBase) CloseLibrary((struct Library *)DOSBase);
    if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if(GfxBase) CloseLibrary((struct Library *)GfxBase);
    
    D(bug("html.datatype/__UserLibCleanup: Done\n"));
}

/**************************************************************************************************/

struct IClass *ObtainEngine(void)
{
    D(bug("html.datatype/ObtainEngine: returning %x\n", dt_class));

    return dt_class;
}

/**************************************************************************************************/
