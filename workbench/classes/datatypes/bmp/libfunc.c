/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/intuition.h>

#include "compilerspecific.h"
#include "debug.h"

/**************************************************************************************************/

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
struct Library          *DataTypesBase;
struct Library          *IFFParseBase;
struct Library          *PictureBase;

/* inside ilbmclass.c */
struct IClass *DT_MakeClass(struct Library *bmpbase);

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

    D(bug("bmp.datatype/__UserLibInit\n"));
    
    if ((PictureBase = OpenLibrary("datatypes/picture.datatype", 0)))
    {
	if((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39)))
	{
	    if((IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39)))
	    {
		if((DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 39)))
		{
#ifdef _AROS
		    if((UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 37)))
#else
		    if((UtilityBase = (struct Library *)OpenLibrary("utility.library", 37)))
#endif
		    {
			if((DataTypesBase = OpenLibrary("datatypes.library", 37)))
			{
				if((dt_class = DT_MakeClass(libbase)))
				{
				    AddClass(dt_class);

				    D(bug("bmp.datatype/__UserLibInit: Returning success\n"));

				    return 0;
				}
			}
		    }
		}
	    }
	}
    }

    D(bug("bmp.datatype/__UserLibInit: Returning failure\n"));
    
    return -1;
}

/**************************************************************************************************/

ASM SAVEDS void __UserLibCleanup( register __a6 struct Library *libbase )
{
    D(bug("bmp.datatype/__UserLibCleanup\n"));

    D(bug("bmp.datatype/__UserLibCleanup: Freeing class\n"));

    if(dt_class)
    {
	RemoveClass(dt_class);
	FreeClass(dt_class);
	dt_class = NULL;
    }

    D(bug("bmp.datatype/__UserLibCleanup: Closing Libraries\n"));

    if(PictureBase) CloseLibrary(PictureBase);
    if(DataTypesBase) CloseLibrary(DataTypesBase);
    if(UtilityBase) CloseLibrary((struct Library *)UtilityBase);
    if(DOSBase) CloseLibrary((struct Library *)DOSBase);
    if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if(GfxBase) CloseLibrary((struct Library *)GfxBase);
    
    D(bug("bmp.datatype/__UserLibCleanup: Done\n"));

}

/**************************************************************************************************/

SAVEDS STDARGS struct IClass *ObtainEngine(void)
{
    D(bug("bmp.datatype/ObtainEngine: returning 0x%lx\n", (unsigned long) dt_class));

    return dt_class;
}

/**************************************************************************************************/
