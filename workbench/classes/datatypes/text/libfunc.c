/********************************************************************** 
 text.datatype - (c) 2000 by Sebastian Bauer

 This module is the library initializion module for the
 datatype.
 It must be compiled with sc's libcode option
***********************************************************************/

#include <proto/exec.h>
#include <proto/intuition.h>

#include "compilerspecific.h"

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
struct Library 		*LayersBase;
struct Library 		*DiskfontBase;
struct Library 		*DataTypesBase;
struct Library 		*IFFParseBase;

/* inside textclass.c */
struct IClass *DT_MakeClass(struct Library *textbase);

#ifdef __AROS__
#undef	register
#define register

#undef __a6
#define __a6
#endif

/**************************************************************************************************/

ASM SAVEDS int __UserLibInit( register __a6 struct Library *libbase )
{
#ifndef __AROS__
    SysBase = *(struct ExecBase**)4;
#endif

    if((LayersBase = OpenLibrary("layers.library", 39)))
    {
	if((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39)))
	{
	    if((IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39)))
	    {
		if((DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 39)))
		{
		    if((DiskfontBase = OpenLibrary("diskfont.library", 37)))
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
					
					return 0;
				    }
				}
			    }
			}
		    }
		}
	    }
	}
    }
    
    return -1;
}

/**************************************************************************************************/

ASM SAVEDS void __UserLibCleanup( register __a6 struct Library *libbase )
{
    if(dt_class)
    {
	RemoveClass(dt_class);
	FreeClass(dt_class);
	dt_class = NULL;
    }
    
    if(IFFParseBase) CloseLibrary(IFFParseBase);
    if(DataTypesBase) CloseLibrary(DataTypesBase);
    if(UtilityBase) CloseLibrary((struct Library *)UtilityBase);
    if(DiskfontBase) CloseLibrary(DiskfontBase);
    if(DOSBase) CloseLibrary((struct Library *)DOSBase);
    if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if(GfxBase) CloseLibrary((struct Library *)GfxBase);
    if(LayersBase) CloseLibrary(LayersBase);
}

/**************************************************************************************************/

SAVEDS STDARGS struct IClass *ObtainEngine(void)
{
    return dt_class;
}

/**************************************************************************************************/
