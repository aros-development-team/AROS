/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Library header for keymap
    Lang: english
*/

/****************************************************************************************/

#include <proto/exec.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <aros/symbolsets.h>
#include LC_LIBDEFS_FILE
#include "keymap_intern.h"

/****************************************************************************************/

extern struct KeyMap def_km;

#if DEBUG
struct KeymapBase *DebugKeymapBase;
#endif

/****************************************************************************************/

AROS_SET_LIBFUNC(KeymapInit, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

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
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

/****************************************************************************************/

ADD2INITLIB(KeymapInit, 0);
