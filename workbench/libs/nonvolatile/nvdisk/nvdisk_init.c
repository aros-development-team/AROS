/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Nonvolatile disk based storage library initialization code.
    Lang: English
*/

#define  AROS_ALMOST_COMPATIBLE

#define  DEBUG  1

#include "nvdisk_intern.h"

#include <aros/debug.h>

#include <exec/types.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <devices/timer.h>
#include <aros/libcall.h>

#include "initstruct.h"
#include <stddef.h>

#include <exec/libraries.h>
#include <exec/memory.h>
#include <exec/alerts.h>
#include "libdefs.h"


#define INIT	AROS_SLIB_ENTRY(init, NVDisk)

struct inittable;
extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const LIBFUNCTABLE[];
extern const struct inittable datatable;
extern struct NVDBase *INIT();
extern struct NVDBase *AROS_SLIB_ENTRY(open, NVDisk)();
extern BPTR AROS_SLIB_ENTRY(close, NVDisk)();
extern BPTR AROS_SLIB_ENTRY(expunge, NVDisk)();
extern int AROS_SLIB_ENTRY(null, NVDisk)();
extern const char LIBEND;

int entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

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
    (APTR)sizeof(struct NVDBase),
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

#define O(n) offsetof(struct NVDBase, n)

const struct inittable datatable=
{
    { { I_CPYO(1,B,O(nvd_LibNode.lib_Node.ln_Type)), { NT_LIBRARY } } },
    { { I_CPYO(1,L,O(nvd_LibNode.lib_Node.ln_Name)), { (IPTR)name } } },
    { { I_CPYO(1,B,O(nvd_LibNode.lib_Flags       )), { LIBF_SUMUSED | LIBF_CHANGED } } },
    { { I_CPYO(1,W,O(nvd_LibNode.lib_Version     )), { 1 } } },
    { { I_CPYO(1,W,O(nvd_LibNode.lib_Revision    )), { 0 } } },
    { { I_CPYO(1,L,O(nvd_LibNode.lib_IdString    )), { (IPTR)&version[6] } } },
	I_END ()
};

#undef O


AROS_LH2(struct NVDBase *, init,
 AROS_LHA(struct NVDBase *, nvdBase, D0),
 AROS_LHA(BPTR,               segList,   A0),
	   struct ExecBase *, sysBase, 0, NVDisk)
{
    AROS_LIBFUNC_INIT
    /* This function is single-threaded by exec by calling Forbid. */

    /* Store arguments */
    nvdBase->nvd_SysBase = sysBase;
    nvdBase->nvd_SegList = segList;

    return nvdBase;

    AROS_LIBFUNC_EXIT
}


AROS_LH1(struct NVDBase *, open,
 AROS_LHA(ULONG, version, D0),
	   struct NVDBase *, nvdBase, 1, NVDisk)
{
    AROS_LIBFUNC_INIT

    /*
      This function is single-threaded by exec by calling Forbid.
      If you break the Forbid() another task may enter this function
      at the same time. Take care.
    */
    
    BOOL error = TRUE;    // Notice the initialization to 'TRUE' here.
    char *temp = NULL;

    /* Keep compiler happy */
    version = 0;
    
    nvdBase->nvd_LibNode.lib_OpenCnt++;
    nvdBase->nvd_LibNode.lib_Flags &= ~LIBF_DELEXP;

    D(bug("Opening nvdisk.library\n"));

    if(nvdBase->nvd_LibNode.lib_OpenCnt == 1)
    {
	nvdBase->nvd_DOSBase = OpenLibrary("dos.library", 41);

	if(nvdBase->nvd_DOSBase != NULL)
	{
	    BPTR locFile = Open("SYS:prefs/env-archive/sys/nv_location",
				MODE_OLDFILE);

	    D(bug("Getting location\n"));

	    if(locFile != NULL)
	    {
		D(bug("Location file exists!\n"));

		temp = AllocVec(512, MEMF_CLEAR);
		
		if(temp != NULL)
		{
		    int i = 0;         // Loop variable

		    Read(locFile, temp, 512);

		    // End string if a carriage return is encountered
		    while(temp[i] != 0)
		    {
			if(temp[i] == '\n')
			{
			    temp[i] = 0;
			    break;
			}

			i++;
		    }

		    nvdBase->nvd_location = Lock(temp, SHARED_LOCK);
		    
		    D(bug("NV location = %s\n", temp));

		    FreeVec(temp);
		    
		    D(bug("Got lock = %p\n", nvdBase->nvd_location));

		    if(nvdBase->nvd_location != NULL)
		    {
			error = FALSE;
		    }
		}
		
		Close(locFile);
	    }
	}
    }
    else
	error = FALSE;
	
    if(error == TRUE)
    {
	D(bug("Nvdisk.library failed to open\n"));

	nvdBase->nvd_LibNode.lib_OpenCnt--;
	return NULL;
    }
    else
	return nvdBase;

    AROS_LIBFUNC_EXIT
}


AROS_LH0(BPTR, close, struct NVDBase *, nvdBase, 2, NVDisk)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* I have one fewer opener. */
    --(nvdBase->nvd_LibNode.lib_OpenCnt);
    
    if((nvdBase->nvd_LibNode.lib_Flags & LIBF_DELEXP) != 0)
    {
	if(nvdBase->nvd_LibNode.lib_OpenCnt == 0)
	    return expunge();
	
	nvdBase->nvd_LibNode.lib_Flags &= ~LIBF_DELEXP;
	return NULL;
    }

    return NULL;

    AROS_LIBFUNC_EXIT
}


AROS_LH0(BPTR, expunge, struct NVDBase *, nvdBase, 3, NVDisk)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(nvdBase->nvd_LibNode.lib_OpenCnt != 0)
    {
	/* Set the delayed expunge flag and return. */
	nvdBase->nvd_LibNode.lib_Flags |= LIBF_DELEXP;
	return 0;
    }

    UnLock(nvdBase->nvd_location);

    /* Get rid of the library. Remove it from the list. */
    Remove(&nvdBase->nvd_LibNode.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret = nvdBase->nvd_SegList;

    /* Free the memory. */
    FreeMem((char *)nvdBase - nvdBase->nvd_LibNode.lib_NegSize,
	    nvdBase->nvd_LibNode.lib_NegSize +
	    nvdBase->nvd_LibNode.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}


AROS_LH0I(int, null, struct NVDBase *, nvdBase, 4, NVDisk)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}
