/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Linux hidd initialization code.
    Lang: English.
*/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <exec/types.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <utility/utility.h>
#include <oop/oop.h>
#include <hidd/graphics.h>

#include "linux_intern.h"

#undef SysBase

static VOID cleanup_linux_hidd(struct linux_staticdata *lsd);
static struct linux_staticdata *init_linux_hidd(struct ExecBase *SysBase);

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->sysbase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->seglist)
#define LC_RESIDENTNAME		LinuxHidd_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		9
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->library)

#define LC_NO_INITLIB
#define LC_NO_EXPUNGELIB
#define LC_NO_CLOSELIB


/* to avoid removing the gfxhiddclass from memory add #define NOEXPUNGE */

struct linux_base
{
    struct Library library;
    struct ExecBase *sysbase;
    BPTR	seglist;
    struct linux_staticdata lsd;
};

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#define SysBase      (LC_SYSBASE_FIELD(lh))

#undef XSD
#undef SysBase
#undef OOPBase

#define OOPBase lsd->oopbase


static OOP_AttrBase HiddPixFmtAttrBase = 0;

static struct OOP_ABDescr abd[] = {
	{ IID_Hidd_PixFmt,	&HiddPixFmtAttrBase	},
	{ NULL, NULL }
};

static struct linux_staticdata *init_linux_hidd(struct ExecBase *SysBase)
{
    struct linux_staticdata *lsd = NULL;
    lsd = AllocMem( sizeof (struct linux_staticdata), MEMF_CLEAR|MEMF_PUBLIC );
    if (NULL != lsd) {
	InitSemaphore(&lsd->sema);
        lsd->sysbase = SysBase;

    #if BUFFERED_VRAM
	InitSemaphore(&lsd->framebufferlock);
    #endif
	
        lsd->oopbase = OpenLibrary(AROSOOP_NAME, 0);
	if (NULL == lsd->oopbase) goto failure;
	
	lsd->utilitybase = OpenLibrary(UTILITYNAME, 37);
	if (NULL == lsd->utilitybase) goto failure;

kprintf("GOT LIBS\n");
 	
	lsd->kbd_inited = init_linuxkbd(lsd);
	if (!lsd->kbd_inited) goto failure;

kprintf("KBD INITED\n");

	lsd->mouse_inited = init_linuxmouse(lsd);
	if (!lsd->mouse_inited) goto failure;
kprintf("MOUSE INITED\n");

	if (!OOP_ObtainAttrBases(abd))
	    goto failure;
kprintf("OBTAINED ATTRBASES\n");	    

	lsd->input_task = init_linuxinput_task(lsd);
	if (NULL == lsd->input_task) goto failure;
kprintf("GOT INPUT TASK\n");

	lsd->gfxclass = init_linuxgfxclass(lsd);
	if (NULL == lsd->gfxclass) goto failure;

	lsd->bmclass = init_linuxbmclass(lsd);
	if (NULL == lsd->bmclass) goto failure;
kprintf("GOT GFX CLASS\n");
	
	lsd->kbdclass = init_linuxkbdclass(lsd);
	if (NULL == lsd->kbdclass) goto failure;
kprintf("GOT KBD CLASS\n");
	lsd->mouseclass = init_linuxmouseclass(lsd);
	if (NULL == lsd->mouseclass) goto failure;
kprintf("GOT MOUSECLASS\n");
    }
    
    return lsd;
    
failure:
    cleanup_linux_hidd(lsd);
    
    return NULL;
	
}

static VOID cleanup_linux_hidd(struct linux_staticdata *lsd)
{
    if (NULL != lsd) {
	if (NULL != lsd->mouseclass)
	    free_linuxmouseclass(lsd);

	if (NULL != lsd->kbdclass)
	    free_linuxkbdclass(lsd);
    
	if (NULL != lsd->bmclass)
	    free_linuxbmclass(lsd);

	if (NULL != lsd->gfxclass)
	    free_linuxgfxclass(lsd);

	OOP_ReleaseAttrBases(abd);


	if (NULL != lsd->input_task)
	    kill_linuxinput_task(lsd);
	    
	if (lsd->mouse_inited)
	    cleanup_linuxmouse(lsd);

	if (lsd->kbd_inited)
	    cleanup_linuxkbd(lsd);
		
	if (NULL != lsd->utilitybase)
	    CloseLibrary(lsd->utilitybase);
	    
	if (NULL != lsd->oopbase)
	    CloseLibrary(lsd->oopbase);

	FreeMem(lsd, sizeof (*lsd));
    }
    return;	
}


ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR lh)
{
    struct linux_staticdata *lsd;

kprintf("LINUX_OPENLIB\n");    
    lsd = init_linux_hidd(SysBase);
kprintf("LSD: %p\n", lsd);
    if (NULL != lsd)
    	return TRUE;

    return FALSE;
}
