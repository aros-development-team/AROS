/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ReqTools initialization code.
    Lang: English.
*/

/****************************************************************************************/

#define  AROS_ALMOST_COMPATIBLE

#include "reqtools_intern.h"
#include <exec/types.h>
#include <exec/resident.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <devices/conunit.h>
#include <utility/utility.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/classes.h>
#include <libraries/reqtools.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/macros.h>

#include "initstruct.h"
#include <stddef.h>
#include <string.h>

#define DEBUG 1
#include <aros/debug.h>

#include <exec/libraries.h>
#include <exec/alerts.h>
#include "libdefs.h"

#include "general.h"
#include "boopsigads.h"
#include "rtfuncs.h"

/****************************************************************************************/

#define INIT	AROS_SLIB_ENTRY(init, ReqTools)

struct inittable;
extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const LIBFUNCTABLE[];
extern const struct inittable datatable;
extern struct IntReqToolsBase *INIT();
extern struct IntReqToolsBase *AROS_SLIB_ENTRY(open, ReqTools)();
extern BPTR AROS_SLIB_ENTRY(close, ReqTools)();
extern BPTR AROS_SLIB_ENTRY(expunge, ReqTools)();
extern int AROS_SLIB_ENTRY(null, ReqTools)();
extern const char LIBEND;

/****************************************************************************************/

int entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

/****************************************************************************************/

const struct Resident resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&resident,
    (APTR)&LIBEND,
    RTF_AUTOINIT,
    VERSION_NUMBER,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

const char name[] = NAME_STRING;

const char version[] = VERSION_STRING;

const APTR inittabl[4]=
{
    (APTR)sizeof(struct IntReqToolsBase),
    (APTR)LIBFUNCTABLE,
    (APTR)&datatable,
    &INIT
};

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

#define O(n) offsetof(struct ReqToolsBase, n)

const struct inittable datatable=
{
    { { I_CPYO(1,B,O(LibNode.lib_Node.ln_Type)), { NT_LIBRARY } } },
    { { I_CPYO(1,L,O(LibNode.lib_Node.ln_Name)), { (IPTR)name } } },
    { { I_CPYO(1,B,O(LibNode.lib_Flags       )), { LIBF_SUMUSED|LIBF_CHANGED } } },
    { { I_CPYO(1,W,O(LibNode.lib_Version     )), { 1 } } },
    { { I_CPYO(1,W,O(LibNode.lib_Revision    )), { 0 } } },
    { { I_CPYO(1,L,O(LibNode.lib_IdString    )), { (IPTR)&version[6] } } },
	I_END ()
};

#undef O

/****************************************************************************************/

/* Global variables */

#define extern
#include "globalvars.h"
#undef extern

/****************************************************************************************/

AROS_LH2(struct IntReqToolsBase *, init,
 AROS_LHA(struct IntReqToolsBase *, RTBase, D0),
 AROS_LHA(BPTR,               segList,   A0),
	   struct ExecBase *, sysBase, 0, ReqTools)
{
    AROS_LIBFUNC_INIT
    
    ReqToolsBase = (struct ReqToolsBase *)RTBase;
        
    /* This function is single-threaded by exec by calling Forbid. */

    RTBase->rt_SysBase = SysBase = sysBase;

    D(bug("reqtools.library: Inside libinit func\n"));
    
    return (struct IntReqToolsBase *)RTFuncs_Init(&RTBase->rt, segList);

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(struct IntReqToolsBase *, open,
 AROS_LHA(ULONG, version, D0),
	   struct IntReqToolsBase *, RTBase, 1, ReqTools)
{
    AROS_LIBFUNC_INIT
    
    D(bug("reqtools.library: Inside libopen func\n"));
 
    /*
      This function is single-threaded by exec by calling Forbid.
      If you break the Forbid() another task may enter this function
      at the same time. Take care.
    */
    
    /* Keep compiler happy */
    (void)version;

    D(bug("reqtools.library: Inside libopen func\n"));
    
    return (struct IntReqToolsBase *)RTFuncs_Open(&RTBase->rt, version);

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(BPTR, close, struct IntReqToolsBase *, RTBase, 2, ReqTools)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    D(bug("reqtools.library: Inside libclose func.\n"));

    return RTFuncs_Close(&RTBase->rt);
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(BPTR, expunge, struct IntReqToolsBase *, RTBase, 3, ReqTools)
{
    AROS_LIBFUNC_INIT

    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
 
    D(bug("reqtools.library: Inside libexpunge func.\n"));

    return RTFuncs_Expunge(&RTBase->rt);
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(int, null, struct IntReqToolsBase *, RTBase, 4, ReqTools)
{
    AROS_LIBFUNC_INIT

    return RTFuncs_Null(&RTBase->rt);
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/
