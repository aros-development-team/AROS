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
#ifdef __AROS__
struct UtilityBase      *UtilityBase;
#else
struct Library          *UtilityBase;
#endif
struct DosLibrary       *DOSBase;
struct Library          *DataTypesBase;
struct Library          *IFFParseBase;
struct Library          *PictureBase;

/* inside ilbmclass.c */
struct IClass *DT_MakeClass(struct Library *ilbmbase);

#ifdef __AROS__
#undef  register
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

    D(bug("ilbm.datatype/__UserLibInit\n"));
    
    if ((PictureBase = OpenLibrary("datatypes/picture.datatype", 0)))
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

				    D(bug("ilbm.datatype/__UserLibInit: Returning success\n"));

				    return 0;
				}
			    }
			}
		    }
		}
	    }
	}
    }

    D(bug("ilbm.datatype/__UserLibInit: Returning failure\n"));
    
    return -1;
}

/**************************************************************************************************/

ASM SAVEDS void __UserLibCleanup( register __a6 struct Library *libbase )
{
    D(bug("ilbm.datatype/__UserLibCleanup\n"));

    D(bug("ilbm.datatype/__UserLibCleanup: Freeing class\n"));

    if(dt_class)
    {
	RemoveClass(dt_class);
	FreeClass(dt_class);
	dt_class = NULL;
    }

    D(bug("ilbm.datatype/__UserLibCleanup: Closing Libraries\n"));

    if(PictureBase) CloseLibrary(PictureBase);
    if(IFFParseBase) CloseLibrary(IFFParseBase);
    if(DataTypesBase) CloseLibrary(DataTypesBase);
    if(UtilityBase) CloseLibrary((struct Library *)UtilityBase);
    if(DOSBase) CloseLibrary((struct Library *)DOSBase);
    if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if(GfxBase) CloseLibrary((struct Library *)GfxBase);
    
    D(bug("ilbm.datatype/__UserLibCleanup: Done\n"));

}

/**************************************************************************************************/

SAVEDS STDARGS struct IClass *ObtainEngine(void)
{
    D(bug("ilbm.datatype/ObtainEngine: returning 0x%lx\n", (unsigned long) dt_class));

    return dt_class;
}

/**************************************************************************************************/

ASM IPTR DT_Dispatcher(register __a0 struct IClass *cl, register __a2 Object * o, register __a1 Msg msg)
{
    IPTR retval;

    putreg(REG_A4, (long) cl->cl_Dispatcher.h_SubEntry);        /* Small Data */

    // D(bug("ilbm.datatype/DT_Dispatcher: Entering\n"));

    switch(msg->MethodID)
    {
	case OM_NEW:
	    D(bug("ilbm.datatype/DT_Dispatcher: Method OM_NEW\n"));
	    retval = ILBM__OM_NEW(cl, o, (struct opSet *)msg);
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;

    } /* switch(msg->MethodID) */

    // D(bug("ilbm.datatype/DT_Dispatcher: Leaving\n"));

    return retval;
    
}

/**************************************************************************************************/

struct IClass *DT_MakeClass(struct Library *ilbmbase)
{
    struct IClass *cl;
    
    cl = MakeClass("ilbm.datatype", "picture.datatype", 0, 0, 0);

    D(bug("ilbm.datatype/DT_MakeClass: DT_Dispatcher 0x%lx\n", (unsigned long) DT_Dispatcher));

    if (cl)
    {
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) DT_Dispatcher;
	cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC) getreg(REG_A4);
	cl->cl_UserData = (IPTR)ilbmbase; /* Required by datatypes (see disposedtobject) */
    }

    return cl;
}

/**************************************************************************************************/
