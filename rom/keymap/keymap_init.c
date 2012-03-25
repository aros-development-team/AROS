/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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

static struct KeyMapNode *AddKeymap(char *name, struct KeyMap *data, struct KeymapBase *LIBBASE)
{
    struct KeyMapNode *kmn = AllocMem(sizeof(struct KeyMapNode), MEMF_CLEAR | MEMF_PUBLIC);

    if (kmn)
    {
        kmn->kn_Node.ln_Name = name;
        CopyMem(data, &kmn->kn_KeyMap, sizeof(struct KeyMap));

        /*
         * We are being called under Forbid(), so I don't have to arbitrate
         * That notwithstanding, if keymap resource or exec library loading
         * ever become semaphore based, there may be some problems.
         */
        AddTail(&(LIBBASE->KeymapResource.kr_List), &kmn->kn_Node);
    }
    return kmn;
}

static int KeymapInit(LIBBASETYPEPTR LIBBASE)
{
#if DEBUG
    DebugKeymapBase = LIBBASE;
#endif

    LIBBASE->DefaultKeymap = &def_km;

    /* Initialize and add the keymap.resource */    
    LIBBASE->KeymapResource.kr_Node.ln_Type = NT_RESOURCE;
    LIBBASE->KeymapResource.kr_Node.ln_Name = "keymap.resource";
    NEWLIST( &(LIBBASE->KeymapResource.kr_List) );
    AddResource(&LIBBASE->KeymapResource);

    /* AmigaOS default built-in keymap has "usa" name */
    if (!AddKeymap("usa", &def_km, LIBBASE))
        return FALSE;

#ifdef __mc68000
    /* Add ROM built-in usa1 keymap, keeps WB3.0 C:IPrefs quiet
     * TODO: add correct keymap instead of using default keymap
     */
    AddKeymap("usa1", &def_km, LIBBASE);
#endif

    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(KeymapInit, 0);
