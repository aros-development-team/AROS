/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <stdio.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#ifdef __AROS__
#include <proto/muimaster.h>
#endif

#include "muimaster_intern.h"
#include "mui.h"
#include "support.h"
#include "support_classes.h"

static CONST_STRPTR searchpaths[] = {
    "PROGDIR:Zune/%s",
    "Zune/%s",
    "Classes/Zune/%s",
    NULL,
};

static struct IClass *load_external_class(CONST_STRPTR classname, struct Library *MUIMasterBase)
{
    struct Library *mcclib;
    struct MUI_CustomClass *mcc;
    struct IClass *cl;
    CONST_STRPTR *pathptr;
    UBYTE s[255];

    for (pathptr = searchpaths; *pathptr; pathptr++)
    {
#ifdef __AROS__	    
	snprintf(s, 255, *pathptr, classname);
#else
#warning "snprintf() not used on Amiga"
	sprintf(s, *pathptr, classname);
#endif
	if ((mcclib = OpenLibrary(s, 0)))
	{
	    /* call MCC_Query(0) */
	
#ifdef __AROS__		
	    mcc = AROS_LVO_CALL1(struct MUI_CustomClass *,
				 AROS_LCA(LONG, 0, D0),
				 struct Library *, mcclib, 5, lib);
#else
#warning "You need to make MCC_Query() call here!!!!!!!!!!!!!!!!!!!"
	    mcc = 0;
#endif			
	    if (mcc)
	    {
		cl = mcc->mcc_Class;
		if (cl)
		{
		    mcc->mcc_Module = mcclib;
		    return cl;
		}
	    }
		
	    if (!cl) CloseLibrary(mcclib);
	}
    }
    return NULL;
}

/*****************************************************************************

    NAME */
#ifndef __AROS__
__asm struct IClass *MUI_GetClass(register __a0 char *classname)
#else
	AROS_LH1(struct IClass *, MUI_GetClass,

/*  SYNOPSIS */
	AROS_LHA(char *, classname, A0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 13, MUIMaster)
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
    AROS_LIBBASE_EXT_DECL(struct Library *,MUIMasterBase)

    struct IClass *cl = NULL;

    if (classname == NULL)
	return NULL;

    if (!(cl = GetPublicClass(classname,MUIMasterBase)))
    {
	/* do we have space for another class pointer? */
	if (MUIMB(MUIMasterBase)->ClassCount == MUIMB(MUIMasterBase)->ClassSpace)
	{
	  struct IClass **t;
	  int newSpace = MUIMB(MUIMasterBase)->ClassSpace ? MUIMB(MUIMasterBase)->ClassSpace+16 : 32;

	  if (!(t = (struct IClass **)AllocVec(sizeof(Class *) * newSpace, MEMF_ANY)))
	    return NULL;

	  if (MUIMB(MUIMasterBase)->Classes)
	  {
	    CopyMem(MUIMB(MUIMasterBase)->Classes, t, sizeof(struct IClass *) * MUIMB(MUIMasterBase)->ClassSpace);
	    FreeVec(MUIMB(MUIMasterBase)->Classes);
	  }

	  MUIMB(MUIMasterBase)->Classes    = t;
	  MUIMB(MUIMasterBase)->ClassSpace = newSpace;
	}

	cl = CreateBuiltinClass(classname, MUIMasterBase);

    	if (!cl)
	{
	    cl = load_external_class(classname, MUIMasterBase);
	}

	if (cl)
	{
#ifndef __MAXON__
#warning FIXME: I should increase the open count of library (use cl->hook->data)
#endif
#if 0
	    ASSERT(cl->cl_ID != NULL);
	    ASSERT(strcmp(classname, cl->cl_ID) == 0);
#endif
	    MUIMB(MUIMasterBase)->Classes[MUIMB(MUIMasterBase)->ClassCount++] = cl;
	}
    }
    return cl;


    AROS_LIBFUNC_EXIT

} /* MUIA_GetClass */

