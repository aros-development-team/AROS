/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/13 14:00:53  digulla
    Added calls to driver
    Init local SysBase
    Replaced __AROS_LA with __AROS_LHA

    Revision 1.1  1996/08/12 14:27:50  digulla
    Base of graphics library

    Revision 1.2  1996/08/01 17:41:42  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/resident.h>
#include <clib/exec_protos.h>
#include <aros/libcall.h>
#include <dos/dos.h>
#include <exec/execbase.h>
#include <graphics/gfxbase.h>
#include "graphics_intern.h"

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const Graphics_functable[];
struct GfxBase *Graphics_init();
extern const char Graphics_end;

extern int  driver_init (struct GfxBase *);
extern int  driver_open (struct GfxBase *);
extern void driver_close (struct GfxBase *);
extern void driver_expunge (struct GfxBase *);

int Graphics_entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident Graphics_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&Graphics_resident,
    (APTR)&Graphics_end,
    RTF_AUTOINIT,
    39,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]=GRAPHICSNAME;

static const char version[]="$VER: graphics.library 39.0 (12.8.96)\n\015";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct GfxBase),
    (APTR)Graphics_functable,
    NULL,
    &Graphics_init
};

__AROS_LH2(struct GfxBase *, init,
 __AROS_LHA(struct GfxBase *, GfxBase, D0),
 __AROS_LHA(BPTR,               segList,   A0),
	   struct ExecBase *, sysBase, 0, Graphics)
{
    __AROS_FUNC_INIT

    SysBase = sysBase;

    if (!driver_init (GfxBase))
	return NULL;

    /* You would return NULL if the init failed */
    return GfxBase;
    __AROS_FUNC_EXIT
}

__AROS_LH1(struct GfxBase *, open,
 __AROS_LHA(ULONG, version, D0),
	   struct GfxBase *, GfxBase, 1, Graphics)
{
    __AROS_FUNC_INIT

    /* Keep compiler happy */
    version=0;

    if (!driver_open (GfxBase))
	return NULL;

    /* I have one more opener. */
    GfxBase->LibNode.lib_OpenCnt++;
    GfxBase->LibNode.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return GfxBase;
    __AROS_FUNC_EXIT
}

__AROS_LH0(BPTR, close,
	   struct GfxBase *, GfxBase, 2, Graphics)
{
    __AROS_FUNC_INIT

    /* I have one fewer opener. */
    if(!--GfxBase->LibNode.lib_OpenCnt)
    {
	driver_close (GfxBase);

	/* Delayed expunge pending? */
	if(GfxBase->LibNode.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    __AROS_FUNC_EXIT
}

__AROS_LH0(BPTR, expunge,
	   struct GfxBase *, GfxBase, 3, Graphics)
{
    __AROS_FUNC_INIT

    BPTR ret;

    /* Test for openers. */
    if(GfxBase->LibNode.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	GfxBase->LibNode.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    driver_expunge (GfxBase);

    /* Get rid of the library. Remove it from the list. */
    Remove(&GfxBase->LibNode.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=0L;

    /* Free the memory. */
    FreeMem((char *)GfxBase-GfxBase->LibNode.lib_NegSize,
	    GfxBase->LibNode.lib_NegSize+GfxBase->LibNode.lib_PosSize);

    return ret;
    __AROS_FUNC_EXIT
}

__AROS_LH0I(int, null,
	    struct GfxBase *, GfxBase, 4, Graphics)
{
    __AROS_FUNC_INIT
    return 0;
    __AROS_FUNC_EXIT
}
