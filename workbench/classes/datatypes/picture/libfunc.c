/********************************************************************** 
 text.datatype - (c) 2000 by Sebastian Bauer

 This module is the library initializion module for the
 datatype.
 It must be compiled with sc's libcode option
***********************************************************************/

/*
 *  picture.datatype - a rippoff
 */

#include <proto/exec.h>
#include <proto/intuition.h>

#include "compilerspecific.h"
#include "debug.h"


struct IClass           *dt_class;

struct ExecBase         *SysBase;
struct IntuitionBase    *IntuitionBase;
struct GfxBase          *GfxBase;
#ifdef _AROS
struct UtilityBase      *UtilityBase;
#else
struct Library          *UtilityBase;
#endif
struct DosLibrary       *DOSBase;
struct Library          *LayersBase;
struct Library          *DiskfontBase;
struct Library          *DataTypesBase;
struct Library          *IFFParseBase;
struct Library          *CyberGfxBase;

/* inside pictureclass.c */
struct IClass *DT_MakeClass(struct Library *picturebase);

#ifdef _AROS
#undef  register
#define register

#undef __a6
#define __a6
#endif

/**************************************************************************************************/

ASM SAVEDS int __UserLibInit( register __a6 struct Library *libbase )
{
#ifndef _AROS
    SysBase = *(struct ExecBase**)4;
#endif

    D(bug("picture.datatype/__UserLibInit\n"));

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
#ifdef _AROS
			if((UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 37)))
#else
			if((UtilityBase = (struct Library *)OpenLibrary("utility.library", 37)))
#endif
			{
			    if((DataTypesBase = OpenLibrary("datatypes.library", 37)))
			    {
				if((IFFParseBase = OpenLibrary("iffparse.library", 37)))
				{
				    if((CyberGfxBase = OpenLibrary("cybergraphics.library", 37)))
				    {
					if((dt_class = DT_MakeClass(libbase)))
					{
					    AddClass(dt_class);
					
					    D(bug("picture.datatype/__UserLibInit: Returning success\n"));

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
    }
    
    D(bug("picture.datatype/__UserLibInit: Returning failure\n"));

    return -1;
}

/**************************************************************************************************/

ASM SAVEDS void __UserLibCleanup( register __a6 struct Library *libbase )
{
    D(bug("picture.datatype/__UserLibCleanup\n"));

    D(bug("picture.datatype/__UserLibCleanup: Freeing class\n"));

    if(dt_class)
    {
	RemoveClass(dt_class);
	FreeClass(dt_class);
	dt_class = NULL;
    }

    D(bug("picture.datatype/__UserLibCleanup: Closing Libraries\n"));
    
    if(CyberGfxBase) CloseLibrary(CyberGfxBase);
    if(IFFParseBase) CloseLibrary(IFFParseBase);
    if(DataTypesBase) CloseLibrary(DataTypesBase);
    if(UtilityBase) CloseLibrary((struct Library *)UtilityBase);
    if(DiskfontBase) CloseLibrary(DiskfontBase);
    if(DOSBase) CloseLibrary((struct Library *)DOSBase);
    if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if(GfxBase) CloseLibrary((struct Library *)GfxBase);
    if(LayersBase) CloseLibrary(LayersBase);

    D(bug("picture.datatype/__UserLibCleanup: Done\n"));
}

/**************************************************************************************************/

SAVEDS STDARGS struct IClass *ObtainEngine(void)
{
    D(bug("picture.datatype/ObtainEngine: returning %x\n", dt_class));

    return dt_class;
}

/**************************************************************************************************/
