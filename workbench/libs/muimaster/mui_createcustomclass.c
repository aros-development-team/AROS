/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

/*****************************************************************************

    NAME */
#ifndef _AROS
__asm struct MUI_CustomClass *MUI_CreateCustomClass(register __a0 struct Library *base, register __a1 char *supername, register __a2 struct MUI_CustomClass *supermcc,register __d0 int datasize,register __a3 APTR dispatcher)
#else
	AROS_LH5(struct MUI_CustomClass *, MUI_CreateCustomClass,

/*  SYNOPSIS */
	AROS_LHA(struct Library *, base, A0),
	AROS_LHA(char *, supername, A1),
	AROS_LHA(struct MUI_CustomClass *, supermcc, A2),
	AROS_LHA(int, datasize, D0),
	AROS_LHA(APTR, dispatcher, A3),

/*  LOCATION */
	struct Library *, MUIMasterBase, 18, MUIMaster)
#endif
/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
	The function itself is a bug ;-) Remove it!

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct MUIMasterBase *,MUIMasterBase)

    struct MUI_CustomClass *mcc;
    struct IClass	*cl, *super;
    ClassID		 id = NULL;

    if ((supername == NULL) && (supermcc == NULL))
	return NULL;

    if (!supermcc)
    {
	if (!(super = MUI_GetClass(supername)))
	    return NULL;
    }   else super = supermcc->mcc_Class;

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

#ifdef __MAXON__
    cl->cl_Dispatcher.h_Entry = (HOOKFUNC)dispatcher;
#else
    cl->cl_Dispatcher.h_Entry    = (HOOKFUNC)metaDispatcher;
    cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC)dispatcher;
#endif
    cl->cl_Dispatcher.h_Data     = base;

    return mcc;
    
    AROS_LIBFUNC_EXIT

} /* MUIA_CreateCustomClass */
