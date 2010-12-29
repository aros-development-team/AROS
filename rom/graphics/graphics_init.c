/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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
#include "fakegfxhidd.h"

#include LC_LIBDEFS_FILE

#include <stdio.h>

extern int  driver_init (struct GfxBase *);
extern void driver_expunge (struct GfxBase *);

AROS_UFP4(ULONG, TOF_VBlank,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(void *, data, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6));

#ifndef SYSFONTNAME
#   define SYSFONTNAME  "topaz.font"
#endif

BOOL InitROMFont(struct GfxBase *);

#if defined(mc68000) && (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
void InitCustom(struct GfxBase *);
#endif

static int GfxInit(struct GfxBase *LIBBASE)
{
    WORD i;
    
    OOPBase = (APTR)OpenLibrary("oop.library", 41);
    if (OOPBase == NULL)
        return FALSE;

    UtilityBase = (APTR)OpenLibrary("utility.library", 0);
    if (UtilityBase == NULL) {
        CloseLibrary((APTR)OOPBase);
        return FALSE;
    }

#if defined(mc68000) && (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
    InitCustom(GfxBase);
#endif

    NEWLIST(&LIBBASE->BlitWaitQ);
    NEWLIST(&LIBBASE->TextFonts);
    InitSemaphore( &PrivGBase(GfxBase)->hashtab_sema );
    InitSemaphore( &PrivGBase(GfxBase)->view_sema );
    InitSemaphore( &PrivGBase(GfxBase)->tfe_hashtab_sema );
    InitSemaphore( &PrivGBase(GfxBase)->fontsem );

    NEWLIST(&LIBBASE->MonitorList);
    LIBBASE->MonitorList.lh_Type = MONITOR_SPEC_TYPE;
    GfxBase->MonitorListSemaphore = &PrivGBase(GfxBase)->monitors_sema;
    InitSemaphore(GfxBase->MonitorListSemaphore);

    LIBBASE->hash_table = AllocMem(GFXASSOCIATE_HASHSIZE * sizeof(APTR), MEMF_CLEAR|MEMF_PUBLIC);
    if (!LIBBASE->hash_table)
	return FALSE;

    LIBBASE->HashTableSemaphore = &PrivGBase(GfxBase)->hashtab_sema;
    LIBBASE->ActiViewCprSemaphore = &PrivGBase(GfxBase)->view_sema;

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
    if (!(PrivGBase(GfxBase)->driverdatapool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR | MEMF_SEM_PROTECTED,
    	    	    	    	    	    	          1024,
    	    	    	    	    	    	          1024)))
    {
    	return FALSE;
    }

    for(i = 0; i < DRIVERDATALIST_HASHSIZE; i++)
    {
	NEWLIST((struct List *)&PrivGBase(GfxBase)->driverdatalist[i]);
    }
    
    if (!InitROMFont(LIBBASE)) return FALSE;

    return driver_init (LIBBASE);
}

static int GfxOpen(struct GfxBase *LIBBASE)
{
    struct TextFont * def;

    if (!LIBBASE->DefaultFont)
    {
    	struct TextAttr sysTA;
        sysTA.ta_Name  = (STRPTR)SYSFONTNAME;
        sysTA.ta_YSize = 8;
        sysTA.ta_Style = FS_NORMAL;
        sysTA.ta_Flags = 0;

        def = OpenFont (&sysTA);

        if (!def)
            return 0;

        LIBBASE->DefaultFont = def;
        sysTA.ta_YSize = def->tf_YSize;
    }

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

    if (!GfxBase->gb_LayersBase)
        GfxBase->gb_LayersBase = (ULONG *)OpenLibrary("layers.library", 0);

    return TRUE;
}

ADD2INITLIB(GfxInit, 0);
ADD2OPENLIB(GfxOpen, 0);

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


