/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics library
    Lang: english
*/

#define DEBUG 0

#include <aros/debug.h>

#include <exec/resident.h>
#include <proto/exec.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>
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

extern int  driver_init (struct GfxBase *);
extern int  driver_open (struct GfxBase *);
extern void driver_close (struct GfxBase *);
extern void driver_expunge (struct GfxBase *);

AROS_UFP4(ULONG, TOF_VBlank,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(void *, data, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6));

#ifndef SYSFONTNAME
#   define SYSFONTNAME  "topaz.font"
#endif

static struct TextAttr sysTA;
BOOL InitROMFont(struct GfxBase *);

AROS_SET_LIBFUNC(GfxInit, struct GfxBase, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    WORD i;
    
    NEWLIST(&LIBBASE->TextFonts);
    InitSemaphore( &PrivGBase(GfxBase)->tfe_hashtab_sema );
    InitSemaphore( &PrivGBase(GfxBase)->fontsem );

#if REGIONS_USE_MEMPOOL
    InitSemaphore( &PrivGBase(GfxBase)->regionsem );
    if (!(PrivGBase(GfxBase)->regionpool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR,
						      sizeof(struct Region) * 20,
						      sizeof(struct Region) * 20)))
    {
    	return FALSE;
    }

    NEWLIST(&PrivGBase(GfxBase)->ChunkPoolList);
#endif

    InitSemaphore( &PrivGBase(GfxBase)->driverdatasem );
    if (!(PrivGBase(GfxBase)->driverdatapool = CreatePool(MEMF_PUBLIC | MEMF_SEM_PROTECTED,
    	    	    	    	    	    	          1024,
    	    	    	    	    	    	          1024)))
    {
    	return FALSE;
    }

    for(i = 0; i < DRIVERDATALIST_HASHSIZE; i++)
    {
	NEWLIST((struct List *)&PrivGBase(GfxBase)->driverdatalist[i]);
    }
    
    OOPBase = OpenLibrary(AROSOOP_NAME, 0);
    if (!OOPBase)
	return FALSE;
    
    UtilityBase = OpenLibrary (UTILITYNAME,0L);
    if (!UtilityBase)
        return FALSE;

    if (!InitROMFont(LIBBASE)) return FALSE;

    Disable();
    if (!driver_init (LIBBASE))
    {
        Enable();
        return FALSE;
    }
    Enable();
    
    return TRUE;
    AROS_SET_LIBFUNC_EXIT
}

AROS_SET_LIBFUNC(GfxOpen, struct GfxBase, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    struct TextFont * def;

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

    return TRUE;

    AROS_SET_LIBFUNC_EXIT
}


AROS_SET_LIBFUNC(GfxClose, struct GfxBase, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    driver_close (LIBBASE);

    return TRUE;
    AROS_SET_LIBFUNC_EXIT
}

AROS_SET_LIBFUNC(GfxExpunge, struct GfxBase, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    /* Allow the driver to release uneccessary memory */
    driver_expunge (LIBBASE);

    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(GfxInit, 0);
ADD2OPENLIB(GfxOpen, 0);
ADD2CLOSELIB(GfxClose, 0);
ADD2EXPUNGELIB(GfxExpunge, 0);

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


