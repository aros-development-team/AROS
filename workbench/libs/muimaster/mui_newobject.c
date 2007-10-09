/*
    Copyright © 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "mui.h"
#include "muimaster_intern.h"

 #define MYDEBUG 1 
#include "debug.h"

/*****************************************************************************

    NAME */
	AROS_LH2(Object *, MUI_NewObjectA,

/*  SYNOPSIS */
	AROS_LHA(ClassID, classid,     A0),
	AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
	struct Library *, MUIMasterBase, 5, MUIMaster)

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

    Class  *cl;

bug("[MUI_NewObject] NewObject('%s',\n", classid);
struct TagItem *t = tags, *ti;
while(ti = NextTagItem(&t))
{
  bug("[MUI_NewObject]    ti_Tag=%016lx, ti_Data=%016lx\n", ti->ti_Tag, ti->ti_Data);
}
bug("[MUI_NewObject] );\n");


    cl = MUI_GetClass(classid);
    if (cl)
    {
	Object *obj = NewObjectA(cl, NULL, tags);

	if (obj) return obj;

        bug("*** Could not create object of %s\n", classid);
	MUI_FreeClass(cl);
    }

    bug("*** Couldn't find %s\n", classid);

    return NULL;

    AROS_LIBFUNC_EXIT
} /* MUIA_NewObjectA */
