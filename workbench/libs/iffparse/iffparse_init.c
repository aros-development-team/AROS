/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: IFFParse initialization code.
    Lang: English.
*/
#include <exec/types.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>
#include <utility/utility.h>

#ifdef __MORPHOS__
/* For HookEntry */
#include <intuition/classusr.h>
#define CLIB_ALIB_PROTOS_H
#endif

#include "iffparse_intern.h"
#include <aros/debug.h>
#include LC_LIBDEFS_FILE

#include <proto/alib.h>

#ifdef __MORPHOS__
ULONG HookEntry(void)
{
	struct Hook *h=(struct Hook *)REG_A0;
    Msg msg=(Msg) REG_A1;
    Object *obj=(Object*) REG_A2;
    
    return ((ULONG(*)(APTR,APTR,APTR))h->h_SubEntry)(h,obj,msg);
}

static struct EmulLibEntry    HookEntry_Gate=
{
	TRAP_LIB, 0, (void (*)(void))HookEntry
};

#define EasyHook(hook, func)  \
	    IFFParseBase->hook.h_Entry = (IPTR (*)())&HookEntry_Gate;\
        IFFParseBase->hook.h_SubEntry = (IPTR(*)())func;\
        IFFParseBase->hook.h_Data = IFFParseBase
#else
#define EasyHook(hook, func)  \
	    IFFParseBase->hook.h_Entry = HookEntry; \
        IFFParseBase->hook.h_SubEntry = (IPTR(*)())func; \
        IFFParseBase->hook.h_Data = IFFParseBase
#endif

AROS_SET_LIBFUNC(Init, LIBBASETYPE, LIBBASE)
{
    /* This function is single-threaded by exec by calling Forbid. */

    EasyHook(stophook,             StopFunc           );
    EasyHook(prophook,             PropFunc           );
    EasyHook(collectionhook,       CollectionFunc     );
    EasyHook(doshook,              DOSStreamHandler   );
    EasyHook(cliphook,             ClipStreamHandler  );
    EasyHook(bufhook,              BufStreamHandler   );
    EasyHook(collectionpurgehook,  CollectionPurgeFunc);
    EasyHook(proppurgehook,        PropPurgeFunc      );
    EasyHook(exitcontexthook,      ExitContextFunc    );

    DOSBase = OpenLibrary (DOSNAME, 39);
    if (!DOSBase)
	return FALSE;

    UtilityBase = OpenLibrary (UTILITYNAME, 39);
    if (!UtilityBase)
	return FALSE;

    return TRUE;
}

AROS_SET_LIBFUNC(Expunge, LIBBASETYPE, LIBBASE)
{
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    if (DOSBase)
    {
	CloseLibrary (DOSBase);
	DOSBase = NULL;
    }
    
    if (UtilityBase)
    {
	CloseLibrary (UtilityBase);
	UtilityBase = NULL;
    }

    return TRUE;
}

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);
