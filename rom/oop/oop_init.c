/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: OOP Library
    Lang: english
*/

#define NUM_IDS 10

#include <utility/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include "intern.h"
#include "libdefs.h"

#include "hash.h"
#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>


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
    OOP_Class *cl;

    while((cl = (OOP_Class *)RemHead((struct List *)&GetOBase(OOPBase)->bb_ClassList)))
    {
	if(cl != &rootclass)
	    OOP_FreeClass(cl);
    }
}
#endif
*/


static ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LIBBASETYPEPTR LIBBASE)
{

    struct IDDescr intern_ids[] =
    {
    	/* We must make sure that Root gets ID 0 and Meta gets ID 1 */
	{ IID_Root,		&__IRoot		},
	{ IID_Meta,		&__IMeta		},
	
#if 0	
	{ IID_Method,		&__IMethod		},
	{ IID_Server,		&__IServer		},
	{ IID_Proxy,		&__IProxy		},
	{ IID_Interface,	&__IInterface		},
#endif
	{ NULL,	NULL }
    };

    NEWLIST(&GetOBase(LIBBASE)->ob_ClassList);
    InitSemaphore(&GetOBase(LIBBASE)->ob_ClassListLock);

    NEWLIST(&GetOBase(LIBBASE)->ob_ServerList);
    InitSemaphore(&GetOBase(LIBBASE)->ob_ServerListLock);

    InitSemaphore(&GetOBase(LIBBASE)->ob_IIDTableLock);
    
    SDInit();
	
    UtilityBase = OpenLibrary (UTILITYNAME, 0);
    if (UtilityBase)
    {
    	GetOBase(LIBBASE)->ob_IIDTable = NewHash(NUM_IDS, HT_STRING, GetOBase(LIBBASE));
    	if (GetOBase(LIBBASE)->ob_IIDTable)
	{
	    /* Get some IDs that are used internally */
    	    if (GetIDs(intern_ids, (struct IntOOPBase *)OOPBase))
    	    {
            	if (init_rootclass(GetOBase(OOPBase)))
	    	{
            	    if (init_basemeta(GetOBase(OOPBase)))
	    	    {
		    	if (init_ifmetaclass(GetOBase(OOPBase)))
			{
			    GetOBase(OOPBase)->ob_HIDDMetaClass
			    	= init_hiddmetaclass(GetOBase(OOPBase));
			    if (GetOBase(OOPBase)->ob_HIDDMetaClass)
			    {
	    	    	    	if (InitUtilityClasses((struct IntOOPBase *)OOPBase))
			    	    return (TRUE);
			    }
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
#if 0
    D(bug("Initializing methodclass\n"));
    if ((GetOBase(OOPBase)->ob_MethodClass = init_methodclass(GetOBase(OOPBase) )))
    {
    	D(bug("Initializing serverclass\n"));
    	OOPBase->ob_ServerClass = init_serverclass((struct Library *)OOPBase);
    	if (OOPBase->ob_ServerClass)
    	{
	    D(bug("Initializing proxyclass\n"));
    	    OOPBase->ob_ProxyClass = init_proxyclass((struct Library *)OOPBase);
	    if (OOPBase->ob_ProxyClass)
	    {
    		D(bug("Initializing interfaceclass\n"));
	    	OOPBase->ob_InterfaceClass = init_interfaceclass((struct Library *)OOPBase);
	    	if (OOPBase->ob_InterfaceClass)
	    	{
#endif		
    	    	    D(bug("Everything initialized\n"));
    	    	    return TRUE;
#if 0		    
		}
	    }
	}
    }
#endif    
    
    return FALSE;
}

