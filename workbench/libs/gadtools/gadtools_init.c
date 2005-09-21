/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GadTools initialization code.
    Lang: English.
*/
#include <exec/libraries.h>
#include <exec/types.h>

#include <aros/symbolsets.h>
#include <utility/tagitem.h>
#include <utility/utility.h>
#include <intuition/classes.h>
#include <intuition/imageclass.h>
#include <proto/exec.h>

#include "gadtools_intern.h"
#include LC_LIBDEFS_FILE

#ifndef INTUITIONNAME
#define INTUITIONNAME "intuition.library"
#endif
/****************************************************************************************/


AROS_SET_LIBFUNC(Init, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT;
    
    /* This function is single-threaded by exec by calling Forbid. */

    InitSemaphore(&LIBBASE->bevelsema);
    LIBBASE->bevel = NULL;
    
    /* You would return NULL here if the init failed. */
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT;
}

/****************************************************************************************/

Object *makebevelobj(struct GadToolsBase_intern *GadToolsBase)
{
    Object * obj;
    struct TagItem tags[4];

    tags[0].ti_Tag  = IA_EdgesOnly;
    tags[0].ti_Data = TRUE;
    tags[1].ti_Tag  = IA_Left;
    tags[1].ti_Data = 0UL;
    tags[2].ti_Tag  = IA_Top;
    tags[2].ti_Data = 0UL;
    tags[3].ti_Tag  = TAG_DONE;
    obj = NewObjectA(NULL, FRAMEICLASS, tags);

    return obj;
}

/****************************************************************************************/

AROS_SET_LIBFUNC(Open, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT;
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    if (!LIBBASE->bevel)
	LIBBASE->bevel = (struct Image *)makebevelobj(GadToolsBase);
    if (!LIBBASE->bevel)
	return FALSE;

    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT;
}

/****************************************************************************************/

AROS_SET_LIBFUNC(Expunge, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT;
    
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    if (LIBBASE->bevel)
	DisposeObject(LIBBASE->bevel);
    LIBBASE->bevel = NULL;

    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT;
}

/****************************************************************************************/

ADD2INITLIB(Init, 0);
ADD2OPENLIB(Open, 0);
ADD2EXPUNGELIB(Expunge, 0);
