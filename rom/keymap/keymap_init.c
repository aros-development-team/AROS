/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Library header for keymap
    Lang: english
*/

/****************************************************************************************/


#define INIT AROS_SLIB_ENTRY(init,Keymap)

#include <proto/exec.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <aros/asmcall.h>
#include "libdefs.h"
#include "keymap_intern.h"

/****************************************************************************************/

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const LIBFUNCTABLE[];

LIBBASETYPEPTR INIT();

extern const char LIBEND;

/****************************************************************************************/

int Keymap_entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

/****************************************************************************************/

const struct Resident Keymap_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&Keymap_resident,
    (APTR)&LIBEND,
    RTF_AUTOINIT|RTF_COLDSTART,
    VERSION_NUMBER,
    NT_LIBRARY,
    40,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]=KEYMAPNAME;

static const char version[]=VERSION_STRING;

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct KeymapBase),
    (APTR)LIBFUNCTABLE,
    NULL,
    &INIT
};

/****************************************************************************************/

extern struct KeyMap def_km;

#if DEBUG
struct KeymapBase *DebugKeymapBase;
#endif

/****************************************************************************************/

AROS_UFH3(LIBBASETYPEPTR, AROS_SLIB_ENTRY(init,Keymap),
    AROS_UFHA(LIBBASETYPEPTR, LIBBASE, D0),
    AROS_UFHA(BPTR, segList, A0),
    AROS_UFHA(struct ExecBase *, sysBase, A6)
)
{
    AROS_USERFUNC_INIT
    SysBase = sysBase;

#if DEBUG
    DebugKeymapBase = LIBBASE;
#endif

    LIBBASE->DefaultKeymap = &def_km;

    /* Initialize and add the keymap.resource */
    
    LIBBASE->DefKeymapNode = AllocMem(sizeof (struct KeyMapNode), MEMF_PUBLIC);
    if (!LIBBASE->DefKeymapNode)
    	return (NULL);

    /* Initialise the keymap.resource */
    LIBBASE->KeymapResource.kr_Node.ln_Type = NT_RESOURCE;
    LIBBASE->KeymapResource.kr_Node.ln_Name = "keymap.resource";
    NEWLIST( &(LIBBASE->KeymapResource.kr_List) );
    AddResource(&LIBBASE->KeymapResource);
    	
    /* Copy default keymap into DefKeymapNode */
    CopyMem(&def_km, &(LIBBASE->DefKeymapNode->kn_KeyMap), sizeof (struct KeyMap));
    
    LIBBASE->DefKeymapNode->kn_Node.ln_Name = "default keymap";

    /*
	We are being called under Forbid(), so I don't have to arbitrate
	That notwithstanding, if keymap resource or exec library loading
	ever become semaphore based, there may be some problems.
     */
    AddTail( &(LIBBASE->KeymapResource.kr_List), &(LIBBASE->DefKeymapNode->kn_Node));

    /* You would return NULL if the init failed */
    return LIBBASE;
    
    AROS_USERFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(LIBBASETYPEPTR, open,
    AROS_LHA(ULONG, version, D0),
    LIBBASETYPEPTR, LIBBASE, 1, Keymap)
{
    AROS_LIBFUNC_INIT
    
    /* Keep compiler happy */
    version=0;

    /* I have one more opener. */
    LIBBASE->LibNode.lib_OpenCnt++;
    LIBBASE->LibNode.lib_Flags&=~LIBF_DELEXP;
    
    /* You would return NULL if the open failed. */
    return LIBBASE;
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(BPTR, close,
    LIBBASETYPEPTR, LIBBASE, 2, Keymap)
{
    AROS_LIBFUNC_INIT

    /* I have one fewer opener. */
    if(!--LIBBASE->LibNode.lib_OpenCnt)
    {
    	/* Wer don't free the keymap resource, as some might use pointers to
    	** keymaps in it
    	*/

	/* Delayed expunge pending? */
	if(LIBBASE->LibNode.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    
    return 0;
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(BPTR, expunge,
	   LIBBASETYPEPTR, LIBBASE, 3, Keymap)
{
    AROS_LIBFUNC_INIT

    /* Test for openers. */
    if(LIBBASE->LibNode.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	LIBBASE->LibNode.lib_Flags|=LIBF_DELEXP;
	
	return 0;
    }


    /* Free unecessary memory */

#ifdef DISK_BASED /* Don't remove a ROM library */

    /* Get rid of the library. Remove it from the list. */
    Remove(&LIBBASE->LibNode.lib_Node);

    /* Free the memory. */
    FreeMem((char *)LIBBASE-LIBBASE->LibNode.lib_NegSize,
	    LIBBASE->LibNode.lib_NegSize+LIBBASE->LibNode.lib_PosSize);
#endif

    return 0L;
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0I(int, null,
    LIBBASETYPEPTR, LIBBASE, 4, Keymap)
{
    AROS_LIBFUNC_INIT
    
    return 0;
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/
