/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: IFFParse initialization code.
    Lang: English.
*/
#include <stddef.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <aros/libcall.h>
#include <utility/utility.h>

#ifdef __MORPHOS__
/* For HookEntry */
#include <intuition/classusr.h>
#define CLIB_ALIB_PROTOS_H
#endif

#include "initstruct.h"
#include "iffparse_intern.h"
#include <aros/debug.h>
#include LC_LIBDEFS_FILE
//#include "libdefs.h"
#include <aros/asmcall.h>
#include <clib/alib_protos.h>

#define INIT	AROS_SLIB_ENTRY(init,IFFParse)

#ifdef __MORPHOS__
unsigned long __abox__ = 1;
#endif

struct inittable;
extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const LIBFUNCTABLE[];
extern const struct inittable datatable;
extern struct IFFParseBase_intern *INIT();
extern struct IFFParseBase_intern *AROS_SLIB_ENTRY(open,IFFParse)();
extern BPTR AROS_SLIB_ENTRY(close,IFFParse)();
extern BPTR AROS_SLIB_ENTRY(expunge,IFFParse)();
extern int AROS_SLIB_ENTRY(null,IFFParse)();
extern const char LIBEND;

int entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

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

#endif

const struct Resident resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&resident,
    (APTR)&LIBEND,
#ifdef __MORPHOS__
    RTF_PPC | RTF_EXTENDED |RTF_AUTOINIT,
#else
    RTF_AUTOINIT,
#endif
    VERSION_NUMBER,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
#ifdef __MORPHOS__
    ,
    REVISION_NUMBER,	/* Revision */
    NULL /* Tags */
#endif
};

const char name[]=NAME_STRING;

const char version[]=VERSION_STRING;

const APTR inittabl[4]=
{
    (APTR)sizeof(struct IFFParseBase_intern),
    (APTR)LIBFUNCTABLE,
#ifdef __MORPHOS__
    NULL,
#else
    (APTR)&datatable,
#endif
    &INIT
};

#ifndef __MORPHOS__
struct inittable
{
    S_CPYO(1,1,B);
    S_CPYO(2,1,L);
    S_CPYO(3,1,B);
    S_CPYO(4,1,W);
    S_CPYO(5,1,W);
    S_CPYO(6,1,L);
    S_END (LIBEND);
};

#define O(n) offsetof(struct IFFParseBase_intern,n)

const struct inittable datatable=
{
    { { I_CPYO(1,B,O(library.lib_Node.ln_Type)), { NT_LIBRARY } } },
    { { I_CPYO(1,L,O(library.lib_Node.ln_Name)), { (IPTR)name } } },
    { { I_CPYO(1,B,O(library.lib_Flags       )), { LIBF_SUMUSED|LIBF_CHANGED } } },
    { { I_CPYO(1,W,O(library.lib_Version     )), { VERSION_NUMBER } } },
    { { I_CPYO(1,W,O(library.lib_Revision    )), { REVISION_NUMBER } } },
    { { I_CPYO(1,L,O(library.lib_IdString    )), { (IPTR)&version[6] } } },
  I_END ()
};

#undef O

#endif /* ! __MORPHOS__ */

#ifdef __MORPHOS__
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

#undef SysBase

#ifdef __MORPHOS__
struct IFFParseBase_intern *LIB_init(struct IFFParseBase_intern *LIBBASE, BPTR segList, struct ExecBase *SysBase)
#else
AROS_UFH3(struct IFFParseBase_intern *, AROS_SLIB_ENTRY(init,BASENAME),
 AROS_UFHA(struct IFFParseBase_intern *, LIBBASE, D0),
 AROS_UFHA(BPTR,               segList,   A0),
 AROS_UFHA(struct ExecBase *, SysBase, A6)
)
#endif
{
#ifndef __MORPHOS__
    AROS_USERFUNC_INIT
#endif
    /* This function is single-threaded by exec by calling Forbid. */

    /* Store arguments */
    LIBBASE->sysbase=SysBase;
    LIBBASE->seglist=segList;

    EasyHook(stophook,             StopFunc           );
    EasyHook(prophook,             PropFunc           );
    EasyHook(collectionhook,       CollectionFunc     );
    EasyHook(doshook,              DOSStreamHandler   );
    EasyHook(cliphook,             ClipStreamHandler  );
    EasyHook(bufhook,              BufStreamHandler   );
    EasyHook(collectionpurgehook,  CollectionPurgeFunc);
    EasyHook(proppurgehook,        PropPurgeFunc      );
    EasyHook(exitcontexthook,      ExitContextFunc    );

    /* You would return NULL here if the init failed. */
    return LIBBASE;
    
#ifndef __MORPHOS__
    AROS_USERFUNC_EXIT
#endif
}

/* Use This from now on */
#define SysBase LIBBASE->sysbase

AROS_LH1(struct IFFParseBase_intern *, open,
 AROS_LHA(ULONG, version, D0),
     struct IFFParseBase_intern *, LIBBASE, 1, BASENAME)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* Keep compiler happy */
    (void)version;

    if (!DOSBase)
	DOSBase = OpenLibrary (DOSNAME, 39);

    if (!DOSBase)
	return NULL;

    if (!UtilityBase)
	UtilityBase = OpenLibrary (UTILITYNAME, 39);

    if (!UtilityBase)
	return NULL;

    /* I have one more opener. */
    LIBBASE->library.lib_OpenCnt++;
    LIBBASE->library.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close, struct IFFParseBase_intern *, LIBBASE, 2, BASENAME)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* I have one fewer opener. */
    if(!--LIBBASE->library.lib_OpenCnt)
    {
	if (DOSBase)
	    CloseLibrary (DOSBase);

	if (UtilityBase)
	    CloseLibrary (UtilityBase);

	/* Delayed expunge pending? */
	if(LIBBASE->library.lib_Flags&LIBF_DELEXP)
    	{
	    /* Then expunge the library */
    	#ifdef __MORPHOS__
    	    return LIB_expunge();
    	#else
	    return expunge();
    	#endif
    	}
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct IFFParseBase_intern *, LIBBASE, 3, BASENAME)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(LIBBASE->library.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	LIBBASE->library.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Get rid of the library. Remove it from the list. */
    Remove(&LIBBASE->library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=LIBBASE->seglist;

    /* Free the memory. */
    FreeMem((char *)LIBBASE-LIBBASE->library.lib_NegSize,
	LIBBASE->library.lib_NegSize+LIBBASE->library.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, struct IFFParseBase_intern *, LIBBASE, 4, BASENAME)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}


