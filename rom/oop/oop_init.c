/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: OOP Library
    Lang: english
*/

#define NUM_IDS 31

#include <utility/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <aros/symbolsets.h>

#include "intern.h"
#include LC_LIBDEFS_FILE

#include "hash.h"
//#undef SDEBUG
#undef DEBUG
//#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>


BOOL InitUtilityClasses(struct IntOOPBase *OOPBase);

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

AROS_SET_LIBFUNC(OOPInit, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    D(bug("Enter OOPInit\n"));
    
    NEWLIST(&LIBBASE->ob_ClassList);
    InitSemaphore(&LIBBASE->ob_ClassListLock);

    NEWLIST(&LIBBASE->ob_ServerList);
    InitSemaphore(&LIBBASE->ob_ServerListLock);

    InitSemaphore(&LIBBASE->ob_IIDTableLock);
    
    SDInit();
	
    LIBBASE->ob_IIDTable = NewHash(NUM_IDS, HT_STRING, LIBBASE);
    if (LIBBASE->ob_IIDTable)
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

	/* Get some IDs that are used internally */
	if (GetIDs(intern_ids, LIBBASE))
	{
	    if (init_rootclass(LIBBASE))
	    {
		if (init_basemeta(LIBBASE))
		{
		    if (init_ifmetaclass(LIBBASE))
		    {
			LIBBASE->ob_HIDDMetaClass
			    = init_hiddmetaclass(LIBBASE);
			if (LIBBASE->ob_HIDDMetaClass)
			{
			    if (InitUtilityClasses(LIBBASE))
			    {
				D(bug("OOPInit all OK\n"));
				return (TRUE);
			    }
			}
		    }
		}
	    }
	}
    }

    D(bug("OOPInit failed\n"));
	
    return (FALSE);
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(OOPInit, 0);

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

