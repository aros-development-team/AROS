/********************************************************************** 
 text.datatype - (c) 2000 by Sebastian Bauer

 This module is the library initializion module for the
 datatype.
 It must be compiled with sc's libcode option
***********************************************************************/

#include <proto/exec.h>
#include <proto/intuition.h>
#include "compilerspecific.h"
#if defined(__AROS__) && !defined(__MORPHOS__)
#include LC_LIBDEFS_FILE
#else
#include "libdefs.h"
#endif

#warning "All this global stuff sucks and should be removed"
struct ExecBase 	*SysBase;
struct IntuitionBase 	*IntuitionBase;
struct GfxBase	 	*GfxBase;
#ifdef __AROS__
#ifdef __MORPHOS__
struct Library         *UtilityBase;
#else
struct UtilityBase	*UtilityBase;
#endif
#else
struct Library		*UtilityBase;
#endif
struct DosLibrary 	*DOSBase;
struct Library 		*LayersBase;
struct Library 		*DiskfontBase;
struct Library 		*DataTypesBase;
struct Library 		*IFFParseBase;

/* inside textclass.c */
struct IClass *DT_MakeClass(LIBBASETYPEPTR);

#ifdef __AROS__
#undef	register
#define register

#undef __a6
#define __a6
#endif

#undef SysBase

/**************************************************************************************************/

#ifdef __MORPHOS__
int __UserLibInit(LIBBASETYPEPTR       LIBBASE )
#else
ASM SAVEDS int __UserLibInit( register __a6 LIBBASETYPEPTR     LIBBASE )
#endif
{

    SysBase = LIBBASE->sysbase;

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
#ifdef __MORPHOS__
			if((UtilityBase = (struct Library *)OpenLibrary("utility.library", 37)))
#else
			if((UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 37)))
#endif
#else
			if((UtilityBase = (struct Library *)OpenLibrary("utility.library", 37)))
#endif
			{
			    if((DataTypesBase = OpenLibrary("datatypes.library", 37)))
			    {
				if((IFFParseBase = OpenLibrary("iffparse.library", 37)))
				{
				    if((LIBBASE->class = DT_MakeClass(LIBBASE)))
				    {
					AddClass(LIBBASE->class);
					
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

#ifdef __MORPHOS__
void __UserLibCleanup(LIBBASETYPEPTR   LIBBASE )
#else
ASM SAVEDS void __UserLibCleanup( register __a6 LIBBASETYPEPTR	LIBBASE )
#endif
{
    if(LIBBASE->class)
    {
	RemoveClass(LIBBASE->class);
	FreeClass(LIBBASE->class);
	LIBBASE->class = NULL;
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

SAVEDS STDARGS struct IClass *ObtainEngine(LIBBASETYPEPTR      LIBBASE)
{
    return (LIBBASE->class);
}

/**************************************************************************************************/
