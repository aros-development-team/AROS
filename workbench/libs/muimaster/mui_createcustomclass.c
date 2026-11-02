/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"

/*****************************************************************************

    NAME */
        AROS_LH5(struct MUI_CustomClass *, MUI_CreateCustomClass,

/*  SYNOPSIS */
        AROS_LHA(struct Library *,         base,       A0),
        AROS_LHA(ClassID,                  supername,  A1),
        AROS_LHA(struct MUI_CustomClass *, supermcc,   A2),
        AROS_LHA(ULONG,                    datasize,   D0),
        AROS_LHA(APTR,                     dispatcher, A3),

/*  LOCATION */
        struct Library *, MUIMasterBase, 18, MUIMaster)

/*  FUNCTION
        Create a BOOPSI class derived from a named MUI class or an existing
        custom class, and return a descriptor for the new class.
        If supermcc is supplied, it takes precedence over supername.

    INPUTS
        base - Library base for a class module, or NULL for a private class.
               With a non-NULL base, its node name supplies the class ID
               through dos.library/FilePart(). The base is also passed to
               the dispatcher through the class dispatcher's data field.
        supername - NUL-terminated superclass identifier, used only when
                    supermcc is NULL.
        supermcc - Existing superclass descriptor, or NULL to use supername.
                   Its mcc_Class must be valid.
        datasize - Number of instance-data bytes added by the new class.
        dispatcher - Non-NULL dispatcher entrypoint using the AROS class
                     dispatcher calling convention.

    RESULT
        A newly allocated MUI_CustomClass descriptor, or NULL if neither
        superclass argument is supplied, the named superclass cannot be
        obtained, descriptor allocation fails, or MakeClass() fails.

    NOTES
        Keep the dispatcher and any supplied module base valid while the
        class is in use. Dispose of all instances and subclasses before
        releasing the descriptor with MUI_DeleteCustomClass().
        The supplied superclass descriptor is not copied or freed here.

    BUGS
        If a named superclass was obtained and a later allocation or
        MakeClass() fails, this implementation does not release that
        superclass. A supplied superclass is not separately acquired,
        although successful deletion calls MUI_FreeClass() on it.

    SEE ALSO
        MUI_DeleteCustomClass(), MUI_GetClass(), intuition.library/MakeClass()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MUI_CustomClass *mcc;
    struct IClass        *cl, *super;
    ClassID                 id = NULL;

    if ((supername == NULL) && (supermcc == NULL))
        return NULL;

    if (!supermcc)
    {
        super = MUI_GetClass(supername);
        if (!super) return NULL;
    }
    else super = supermcc->mcc_Class;

    if (!(mcc = mui_alloc_struct(struct MUI_CustomClass)))
        return NULL;

    if (base)
        id = FilePart(((struct Node *)base)->ln_Name);

    if (!(cl = MakeClass(id, NULL, super, datasize, 0)))
    {
        mui_free(mcc);
        return NULL;
    }

    mcc->mcc_UtilityBase   = (struct Library *)UtilityBase;
    mcc->mcc_DOSBase       = (struct Library *)DOSBase;
    mcc->mcc_GfxBase       = (struct Library *)GfxBase;
    mcc->mcc_IntuitionBase = (struct Library *)IntuitionBase;

    mcc->mcc_Class  = cl;
    mcc->mcc_Super  = super;
    mcc->mcc_Module = NULL; /* _zune_class_load() will set this */

#if defined(__MAXON__) || defined(__amigaos4__)
    cl->cl_Dispatcher.h_Entry    = (HOOKFUNC)dispatcher;
#else
    cl->cl_Dispatcher.h_Entry    = (HOOKFUNC)metaDispatcher;
    cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC)dispatcher;
#endif
    cl->cl_Dispatcher.h_Data     = base;

    return mcc;
    
    AROS_LIBFUNC_EXIT
} /* MUIA_CreateCustomClass */
