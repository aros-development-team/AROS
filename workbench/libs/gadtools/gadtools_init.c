/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
    /* This function is single-threaded by exec by calling Forbid. */

    LIBBASE->buttonclass = NULL;
    LIBBASE->textclass	 = NULL;
    LIBBASE->sliderclass = NULL;
    LIBBASE->scrollerclass = NULL;
    LIBBASE->arrowclass = NULL;
    LIBBASE->stringclass = NULL;
    LIBBASE->listviewclass = NULL;
    LIBBASE->checkboxclass = NULL;
    LIBBASE->cycleclass = NULL;
    LIBBASE->mxclass = NULL;
    LIBBASE->paletteclass = NULL;
    
    InitSemaphore(&LIBBASE->bevelsema);
    LIBBASE->bevel = NULL;
    InitSemaphore(&LIBBASE->classsema);
    
    /* You would return NULL here if the init failed. */
    return TRUE;
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
}

/****************************************************************************************/

AROS_SET_LIBFUNC(Expunge, LIBBASETYPE, LIBBASE)
{
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    if (LIBBASE->bevel)
	DisposeObject(LIBBASE->bevel);
    LIBBASE->bevel = NULL;

    if (LIBBASE->buttonclass)
	FreeClass(LIBBASE->buttonclass);
    LIBBASE->buttonclass = NULL;

    if (LIBBASE->textclass)
	FreeClass(LIBBASE->textclass);
    LIBBASE->textclass = NULL;

    if (LIBBASE->sliderclass)
	FreeClass(LIBBASE->sliderclass);
    LIBBASE->sliderclass = NULL;
	    
    if (LIBBASE->scrollerclass)
	FreeClass(LIBBASE->scrollerclass);
    LIBBASE->scrollerclass = NULL;

    if (LIBBASE->arrowclass)
	FreeClass(LIBBASE->arrowclass);
    LIBBASE->arrowclass = NULL;

    if (LIBBASE->stringclass)
	FreeClass(LIBBASE->stringclass);
    LIBBASE->stringclass = NULL;

    if (LIBBASE->listviewclass)
	freelistviewclass(LIBBASE->listviewclass, LIBBASE);
    LIBBASE->listviewclass = NULL;

    if (LIBBASE->checkboxclass)
	FreeClass(LIBBASE->checkboxclass);
    LIBBASE->checkboxclass = NULL;
	
    if (LIBBASE->cycleclass)
	FreeClass(LIBBASE->cycleclass);
    LIBBASE->cycleclass = NULL;
	
    if (LIBBASE->mxclass)
	FreeClass(LIBBASE->mxclass);
    LIBBASE->mxclass = NULL;

    if (LIBBASE->paletteclass)
	FreeClass(LIBBASE->paletteclass);
    LIBBASE->paletteclass = NULL;
	    
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(Init, 0);
ADD2OPENLIB(Open, 0);
ADD2EXPUNGELIB(Expunge, 0);
