/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "muimaster_intern.h"
#include "support.h"
#include <stdio.h>

/*****************************************************************************

    NAME */
#ifndef _AROS
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

#if 1
#warning "check this!"
    	if (!cl)
	{
    	    struct Library *mcclib;
	    struct MUI_CustomClass *mcc;
	    UBYTE s[64];
	    
	    snprintf(s, 64, "Zune/%s", classname);
    	    if ((mcclib = OpenLibrary(s, 0)))
	    {
	    	/* call MCC_Query(0) */

#ifdef _AROS		
	    	mcc = AROS_LVO_CALL1(struct MUI_CustomClass *,
		    	AROS_LCA(LONG, 0, D0),
			struct Library *, mcclib, 5, lib);
#else
#warning "You need to make MCC_Query() call here!!!!!!!!!!!!!!!!!!!"
    	    	mcc = 0
#endif			
	    	if (mcc)
		{
		    cl = mcc->mcc_Class;
		    if (cl)
		    {
		    	mcc->mcc_Module = mcclib;
		    }
		}
		
		if (!cl) CloseLibrary(mcclib);
	    }
	}
#endif
	
/*	if (!cl)
	{
	    cl = _zune_class_load(className);
	}
*/
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

