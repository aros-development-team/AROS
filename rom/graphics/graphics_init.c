/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics library
    Lang: english
*/

#define DEBUG 0

#include <aros/debug.h>

#include <exec/resident.h>
#include <proto/exec.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <hardware/intbits.h>
#include <dos/dos.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <graphics/regions.h>
#include <proto/graphics.h>
#include <utility/utility.h>
#include "graphics_intern.h"
#include "default_font.h"
#include LC_LIBDEFS_FILE

#include <stdio.h>

#define INIT    AROS_SLIB_ENTRY(init,Graphics)

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const LIBFUNCTABLE[];
LIBBASETYPEPTR INIT();
extern const char LIBEND;

extern int  driver_init (LIBBASETYPEPTR);
extern int  driver_open (LIBBASETYPEPTR);
extern void driver_close (LIBBASETYPEPTR);
extern void driver_expunge (LIBBASETYPEPTR);

AROS_UFP4(ULONG, TOF_VBlank,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(void *, data, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6));


int Graphics_entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident Graphics_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&Graphics_resident,
    (APTR)&LIBEND,
    RTF_AUTOINIT|RTF_COLDSTART,
    VERSION_NUMBER,
    NT_LIBRARY,
    65,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]=GRAPHICSNAME;

static const char version[]=VERSION_STRING;

static const APTR inittabl[4]=
{

    /* !!!!! Hack warning: The below was sizeof (LIBBASETYPE),
       but if I set libbasetype to GfxBase_intern in lib.conf
       much gets broken. Should maybe be 'publibbasetype'
       and 'privlibbasetype' in lib.conf
    */
    (APTR)sizeof(struct GfxBase_intern),
    (APTR)LIBFUNCTABLE,
    NULL,
    &INIT
};

#ifndef SYSFONTNAME
#   define SYSFONTNAME  "topaz.font"
#endif

static struct TextAttr sysTA;
BOOL InitROMFont(struct GfxBase *);

AROS_UFH3(LIBBASETYPEPTR, AROS_SLIB_ENTRY(init,Graphics),
 AROS_UFHA(LIBBASETYPEPTR,	LIBBASE,    D0),
 AROS_UFHA(BPTR,		segList,    A0),
 AROS_UFHA(struct ExecBase *,	sysBase,    A6)
)
{
    AROS_USERFUNC_INIT

    WORD i;
    
    SysBase = sysBase;
    
    NEWLIST(&LIBBASE->TextFonts);
    InitSemaphore( &PrivGBase(GfxBase)->tfe_hashtab_sema );
    InitSemaphore( &PrivGBase(GfxBase)->fontsem );

#if REGIONS_USE_MEMPOOL
    InitSemaphore( &PrivGBase(GfxBase)->regionsem );
    if (!(PrivGBase(GfxBase)->regionpool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR,
    	    	    	    	    	    	      sizeof(struct Region) * 20,
    	    	    	    	    	    	      sizeof(struct Region) * 20)))
    {
    	return NULL;
    }

    NEWLIST(&PrivGBase(GfxBase)->ChunkPoolList);
#endif

    InitSemaphore( &PrivGBase(GfxBase)->driverdatasem );
    if (!(PrivGBase(GfxBase)->driverdatapool = CreatePool(MEMF_PUBLIC | MEMF_SEM_PROTECTED,
    	    	    	    	    	    	          1024,
    	    	    	    	    	    	          1024)))
    {
    	return NULL;
    }

    for(i = 0; i < DRIVERDATALIST_HASHSIZE; i++)
    {
	NEWLIST((struct List *)&PrivGBase(GfxBase)->driverdatalist[i]);
    }
    
    OOPBase = OpenLibrary(AROSOOP_NAME, 0);
    if (!OOPBase) return NULL;
    
    if (!InitROMFont(LIBBASE)) return NULL;
    
    Disable();
    if (!driver_init (LIBBASE))
    {
        Enable();
        return NULL;
    }
    Enable();

    /* You would return NULL if the init failed */
    return LIBBASE;
    AROS_USERFUNC_EXIT
}


AROS_LH1(LIBBASETYPEPTR, open,
 AROS_LHA(ULONG, version, D0),
           LIBBASETYPEPTR, LIBBASE, 1, Graphics)
{
    AROS_LIBFUNC_INIT
    struct TextFont * def;

    /* Keep compiler happy */
    version=0;

    if (!LIBBASE->DefaultFont)
    {
        sysTA.ta_Name  = (STRPTR)SYSFONTNAME;
        sysTA.ta_YSize = 8;
        sysTA.ta_Style = FS_NORMAL;
        sysTA.ta_Flags = 0;

        def = OpenFont (&sysTA);

        if (!def)
            return NULL;

        LIBBASE->DefaultFont = def;
        sysTA.ta_YSize = def->tf_YSize;
    }

    UtilityBase = OpenLibrary (UTILITYNAME,0L);

    if (!UtilityBase)
        return NULL;

    Disable();
    if (!driver_open (LIBBASE))
    {
        Enable();
        return NULL;
    }
    Enable();

    /* Allocate 8 IPTR's for a hash list needed by
       GfxAssociate(), GfxLookUp()                  */

    if (!LIBBASE->hash_table)
    	LIBBASE->hash_table = (LONG *)AllocMem(8*sizeof(LONG *), 
                                           MEMF_CLEAR|MEMF_PUBLIC);
    if (!LIBBASE->hash_table)
	return NULL;


    if(LIBBASE->LibNode.lib_OpenCnt == 0)
    {
	NEWLIST(&LIBBASE->TOF_WaitQ);
	LIBBASE->vbsrv.is_Code         = (APTR)TOF_VBlank;
	LIBBASE->vbsrv.is_Data         = LIBBASE;
	LIBBASE->vbsrv.is_Node.ln_Name = "Graphics TOF server";
	LIBBASE->vbsrv.is_Node.ln_Pri  = 10;
	LIBBASE->vbsrv.is_Node.ln_Type = NT_INTERRUPT;
	
	/* Add a VBLANK server to take care of TOF waiting tasks. */
	AddIntServer(INTB_VERTB, &LIBBASE->vbsrv);
    }


    /* I have one more opener. */
    LIBBASE->LibNode.lib_OpenCnt++;
    LIBBASE->LibNode.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}


AROS_LH0(BPTR, close,
           LIBBASETYPEPTR, LIBBASE, 2, Graphics)
{
    AROS_LIBFUNC_INIT

    /* I have one fewer opener. */
    if(!--LIBBASE->LibNode.lib_OpenCnt)
    {
	driver_close (LIBBASE);

        /* Delayed expunge pending? */
        if(LIBBASE->LibNode.lib_Flags&LIBF_DELEXP)
            /* Then expunge the library */
            return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge,
           LIBBASETYPEPTR, LIBBASE, 3, Graphics)
{
    AROS_LIBFUNC_INIT
#ifndef DISK_BASED
    if (!(LIBBASE->LibNode.lib_OpenCnt) )
    {
        if (LIBBASE->DefaultFont)
        {
            CloseFont (LIBBASE->DefaultFont);

            LIBBASE->DefaultFont = NULL;
        }

	if (LIBBASE->gb_LayersBase)
	    CloseLibrary((struct Library *)LIBBASE->gb_LayersBase);
	    
	/* Allow the driver to release uneccessary memory */
        driver_expunge (LIBBASE);
    }

    /* Don't delete this library. It's in ROM and therefore cannot be
       deleted */
    return 0L;
#else
    BPTR ret;

    /* Test for openers. */
    if (LIBBASE->LibNode.lib_OpenCnt)
    {
        /* Set the delayed expunge flag and return. */
        LIBBASE->LibNode.lib_Flags|=LIBF_DELEXP;
        return 0;
    }

    /* Get rid of the library. Remove it from the list. */
    Remove(&LIBBASE->LibNode.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=0L;

    /* Free the memory. */
    FreeMem((char *)LIBBASE-LIBBASE->LibNode.lib_NegSize,
            LIBBASE->LibNode.lib_NegSize+LIBBASE->LibNode.lib_PosSize);

    return ret;
#endif
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null,
            LIBBASETYPEPTR, LIBBASE, 4, Graphics)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}


#undef SysBase

AROS_UFH4(ULONG, TOF_VBlank,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(void *, data, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct Node *tNode;
    struct GfxBase * GfxBase = (struct GfxBase *)data;

    if(!IsListEmpty(&GfxBase->TOF_WaitQ))
    {
	ForeachNode(&GfxBase->TOF_WaitQ, tNode)
	{
	    Signal((struct Task *)tNode->ln_Name, SIGF_SINGLE);
	}
    }

    return 0;

    AROS_USERFUNC_EXIT
}


