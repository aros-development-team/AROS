/*
    (C) 1995-96 AROS - The Amiga Replacement OS
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
#include "initstruct.h"
#include "iffparse_intern.h"
#include <aros/debug.h>

struct inittable;
extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const IFFParse_functable[];
extern const struct inittable datatable;
extern struct IFFParseBase_intern *AROS_SLIB_ENTRY(init,IFFParse)();
extern struct IFFParseBase_intern *AROS_SLIB_ENTRY(open,IFFParse)();
extern BPTR AROS_SLIB_ENTRY(close,IFFParse)();
extern BPTR AROS_SLIB_ENTRY(expunge,IFFParse)();
extern int AROS_SLIB_ENTRY(null,IFFParse)();
extern const char IFFParse_end;

int entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&resident,
    (APTR)&IFFParse_end,
    RTF_AUTOINIT,
    41,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

const char name[]="iffparse.library";

const char version[]="$VER: iffparse 41.1 (10.2.97)\n\015";

const APTR inittabl[4]=
{
    (APTR)sizeof(struct IFFParseBase_intern),
    (APTR)IFFParse_functable,
    (APTR)&datatable,
    &AROS_SLIB_ENTRY(init,IFFParse)
};

struct inittable
{
    S_CPYO(1,1,B);
    S_CPYO(2,1,L);
    S_CPYO(3,1,B);
    S_CPYO(4,1,W);
    S_CPYO(5,1,W);
    S_CPYO(6,1,L);
    S_END (IFFParse_end);
};

#define O(n) offsetof(struct IFFParseBase_intern,n)

const struct inittable datatable=
{
    { { I_CPYO(1,B,O(library.lib_Node.ln_Type)), { NT_LIBRARY } } },
    { { I_CPYO(1,L,O(library.lib_Node.ln_Name)), { (IPTR)name } } },
    { { I_CPYO(1,B,O(library.lib_Flags       )), { LIBF_SUMUSED|LIBF_CHANGED } } },
    { { I_CPYO(1,W,O(library.lib_Version     )), { 1 } } },
    { { I_CPYO(1,W,O(library.lib_Revision    )), { 0 } } },
    { { I_CPYO(1,L,O(library.lib_IdString    )), { (IPTR)&version[6] } } },
  I_END ()
};

#define EasyHook(hook, func)  \
    IFFParseBase->hook.h_Entry = HookEntry; \
    IFFParseBase->hook.h_SubEntry = (IPTR(*)())func; \
    IFFParseBase->hook.h_Data = IFFParseBase

#undef O
#undef SysBase

AROS_LH2(struct IFFParseBase_intern *, init,
 AROS_LHA(struct IFFParseBase_intern *, IFFParseBase, D0),
 AROS_LHA(BPTR,               segList,   A0),
     struct ExecBase *, SysBase, 0, IFFParse)
{
    AROS_LIBFUNC_INIT
    /* This function is single-threaded by exec by calling Forbid. */

    /* Store arguments */
    IFFParseBase->sysbase=SysBase;
    IFFParseBase->seglist=segList;

    EasyHook(stophook,             StopFunc           );
    EasyHook(prophook,             PropFunc           );
    EasyHook(collectionhook,       CollectionFunc     );
    EasyHook(doshook,              DOSStreamHandler   );
    EasyHook(cliphook,             ClipStreamHandler  );
    EasyHook(bufhook,              BufStreamHandler   );
    EasyHook(collectionpurgehook,  CollectionPurgeFunc);
    EasyHook(proppurgehook,        PropPurgeFunc      );

    /* You would return NULL here if the init failed. */
    return IFFParseBase;
    AROS_LIBFUNC_EXIT
}

/* Use This from now on */
#define SysBase IFFParseBase->sysbase

AROS_LH1(struct IFFParseBase_intern *, open,
 AROS_LHA(ULONG, version, D0),
     struct IFFParseBase_intern *, IFFParseBase, 1, IFFParse)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* Keep compiler happy */
    version=0;

    if (!DOSBase)
	DOSBase = OpenLibrary (DOSNAME, 39);

    if (!DOSBase)
	return NULL;

    if (!UtilityBase)
	UtilityBase = OpenLibrary (UTILITYNAME, 39);

    if (!UtilityBase)
	return NULL;

    /* I have one more opener. */
    IFFParseBase->library.lib_OpenCnt++;
    IFFParseBase->library.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return IFFParseBase;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close, struct IFFParseBase_intern *, IFFParseBase, 2, IFFParse)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* I have one fewer opener. */
    if(!--IFFParseBase->library.lib_OpenCnt)
    {
	if (DOSBase)
	    CloseLibrary (DOSBase);

	if (UtilityBase)
	    CloseLibrary (UtilityBase);

	/* Delayed expunge pending? */
	if(IFFParseBase->library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct IFFParseBase_intern *, IFFParseBase, 3, IFFParse)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(IFFParseBase->library.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	IFFParseBase->library.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Get rid of the library. Remove it from the list. */
    Remove(&IFFParseBase->library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=IFFParseBase->seglist;

    /* Free the memory. */
    FreeMem((char *)IFFParseBase-IFFParseBase->library.lib_NegSize,
	IFFParseBase->library.lib_NegSize+IFFParseBase->library.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, struct IFFParseBase_intern *, IFFParseBase, 4, IFFParse)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}


