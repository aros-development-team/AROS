/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Graphics library
    Lang: english
*/
#include <exec/resident.h>
#include <proto/exec.h>
#include <aros/libcall.h>
#include <dos/dos.h>
#include <exec/execbase.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <proto/graphics.h>
#include <utility/utility.h>
#include "graphics_intern.h"

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const Graphics_functable[];
struct GfxBase * AROS_SLIB_ENTRY(init,Graphics)();
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
    &AROS_SLIB_ENTRY(init,Graphics)
};

#ifndef SYSFONTNAME
#   define SYSFONTNAME	"topaz.font"
#endif

static struct TextAttr sysTA;

AROS_LH2(struct GfxBase *, init,
 AROS_LHA(struct GfxBase *, GfxBase, D0),
 AROS_LHA(BPTR,               segList,   A0),
	   struct ExecBase *, sysBase, 0, Graphics)
{
    AROS_LIBFUNC_INIT

    SysBase = sysBase;

    Disable();
    if (!driver_init (GfxBase))
    {
	Enable();
	return NULL;
    }
    Enable();

    /* You would return NULL if the init failed */
    return GfxBase;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(struct GfxBase *, open,
 AROS_LHA(ULONG, version, D0),
	   struct GfxBase *, GfxBase, 1, Graphics)
{
    AROS_LIBFUNC_INIT
    struct TextFont * def;

    /* Keep compiler happy */
    version=0;

    if (!GfxBase->DefaultFont)
    {
	sysTA.ta_Name  = (STRPTR)SYSFONTNAME;
	sysTA.ta_YSize = 8;
	sysTA.ta_Style = FS_NORMAL;
	sysTA.ta_Flags = 0;

	def = OpenFont (&sysTA);

	if (!def)
	    return NULL;

	GfxBase->DefaultFont = def;
	sysTA.ta_YSize = def->tf_YSize;
    }

    UtilityBase = OpenLibrary (UTILITYNAME,0L);

    if (!UtilityBase)
	return NULL;

    Disable();
    if (!driver_open (GfxBase))
    {
	Enable();
	return NULL;
    }
    Enable();

    /* I have one more opener. */
    GfxBase->LibNode.lib_OpenCnt++;
    GfxBase->LibNode.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return GfxBase;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close,
	   struct GfxBase *, GfxBase, 2, Graphics)
{
    AROS_LIBFUNC_INIT

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
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge,
	   struct GfxBase *, GfxBase, 3, Graphics)
{
    AROS_LIBFUNC_INIT
#ifndef DISK_BASED
    if (!(GfxBase->LibNode.lib_OpenCnt) )
    {
	if (GfxBase->DefaultFont)
	{
	    CloseFont (GfxBase->DefaultFont);

	    GfxBase->DefaultFont = NULL;
	}

	/* Allow the driver to release uneccessary memory */
	driver_expunge (GfxBase);
    }

    /* Don't delete this library. It's in ROM and therefore cannot be
       deleted */
    return 0L;
#else
    BPTR ret;

    /* Test for openers. */
    if (GfxBase->LibNode.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	GfxBase->LibNode.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Get rid of the library. Remove it from the list. */
    Remove(&GfxBase->LibNode.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=0L;

    /* Free the memory. */
    FreeMem((char *)GfxBase-GfxBase->LibNode.lib_NegSize,
	    GfxBase->LibNode.lib_NegSize+GfxBase->LibNode.lib_PosSize);

    return ret;
#endif
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null,
	    struct GfxBase *, GfxBase, 4, Graphics)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}
