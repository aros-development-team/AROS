/*
    Copyright (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: OOP Library
    Lang: english
*/

#define NUM_IDS 1000

#define AROS_ALMOST_COMPATIBLE
#include <utility/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <oop/root.h>
#include <oop/meta.h>
#include <oop/method.h>
#include <oop/server.h>
#include <oop/proxy.h>
#include <oop/interface.h>

#include "intern.h"
#include "libdefs.h"

#include "hash.h"


BOOL InitUtilityClasses(struct IntOOPBase *OOPBase);

#ifdef SysBase
#   undef SysBase
#endif

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((struct IntOOPBase *)(lib))->ob_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((struct IntOOPBase *)(lib))->ob_SegList)
#define LC_RESIDENTNAME 	OOP_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		94
#define LC_LIBBASESIZE		sizeof(struct IntOOPBase)
#define LC_LIBHEADERTYPEPTR	LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((struct IntOOPBase *)(lib))->ob_LibNode)
#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB
#define LC_NO_EXPUNGELIB
#define LC_STATIC_INITLIB

#include <libcore/libheader.c>


#define SysBase (GetOBase(OOPBase)->ob_SysBase)

/*
#if 0
static void FreeAllClasses(struct Library *BOOPIBase)
{
    Class *cl;

    while((cl = (Class *)RemHead((struct List *)&GetOBase(OOPBase)->bb_ClassList)))
    {
	if(cl != &rootclass)
	    FreeClass(cl);
    }
}
#endif
*/


static ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LIBBASETYPEPTR LIBBASE)
{

    struct IDDescr intern_ids[] =
    {
	{ GUID_Root,		&__OOPI_Root		},
	{ GUID_Meta,		&__OOPI_Meta		},
	{ GUID_Method,		&__OOPI_Method		},
	{ GUID_Server,		&__OOPI_Server		},
	{ GUID_Proxy,		&__OOPI_Proxy		},
	{ GUID_Interface,	&__OOPI_Interface	},
	{ NULL,	NULL }
    };

    NEWLIST(&GetOBase(LIBBASE)->ob_ClassList);
    InitSemaphore(&GetOBase(LIBBASE)->ob_ClassListLock);

    NEWLIST(&GetOBase(LIBBASE)->ob_ServerList);
    InitSemaphore(&GetOBase(LIBBASE)->ob_ServerListLock);

	
    UtilityBase = OpenLibrary (UTILITYNAME, 0);
    if (UtilityBase)
    {
    	GetOBase(LIBBASE)->ob_IDTable = NewHash(NUM_IDS, HT_STRING, GetOBase(LIBBASE));
    	if (GetOBase(LIBBASE)->ob_IDTable)
	{
	    
	    /* Get some IDs that are used internally */
    	    if (GetIDs(intern_ids, (struct IntOOPBase *)OOPBase))
    	    {
            	if (InitRootClass(GetOBase(OOPBase)))
	    	{
            	    if (InitMetaClass(GetOBase(OOPBase)))
	    	    {
		        if ((GetOBase(OOPBase)->ob_MethodClass =
					InitMethodClass((struct IntOOPBase *)OOPBase) ))
			{
	    	    	    if (InitUtilityClasses((struct IntOOPBase *)OOPBase))
			    	return (TRUE);
			}
		    }
	    	}
   	    }
	}
	
	CloseLibrary(UtilityBase);
    }
    
    return (FALSE);
    
}

/**************************
** InitUtilityClasses()  **
**************************/
BOOL InitUtilityClasses(struct IntOOPBase *OOPBase)
{
    OOPBase->ob_ServerClass = InitServerClass((struct Library *)OOPBase);
    if (OOPBase->ob_ServerClass)
    {
        OOPBase->ob_ProxyClass = InitProxyClass((struct Library *)OOPBase);
	if (OOPBase->ob_ProxyClass)
	{
	    OOPBase->ob_InterfaceClass = InitInterfaceClass((struct Library *)OOPBase);
	    if (OOPBase->ob_InterfaceClass)
	    {
    	    	return (TRUE);
	    }
	}
    }
    
    return (FALSE);
}

