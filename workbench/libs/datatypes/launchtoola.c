/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "datatypes_intern.h"
#include <datatypes/datatypesclass.h>
#include <utility/tagitem.h>
#include <dos/dostags.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/workbench.h>

/* Putchar procedure needed by RawDoFmt() */

AROS_UFH2(void, dt_putchr,
	  AROS_UFHA(UBYTE,    chr, D0),
	  AROS_UFHA(STRPTR *, p,   A3))
{
    AROS_USERFUNC_INIT
    *(*p)++ = chr;
    AROS_USERFUNC_EXIT
}

void dt__sprintf(struct Library *DataTypesBase, UBYTE *buffer,
		 UBYTE *format, ...)
{
    RawDoFmt(format, &format+1, (VOID_FUNC)dt_putchr, &buffer);
}


/*****************************************************************************

    NAME */
#include <proto/datatypes.h>

	AROS_LH3(ULONG, LaunchToolA,

/*  SYNOPSIS */
	AROS_LHA(struct Tool *   , tool,    A0),
        AROS_LHA(STRPTR          , project, A1),
	AROS_LHA(struct TagItem *, attrs,   A2),

/*  LOCATION */
	struct Library *, DataTypesBase, 42, DataTypes)

/*  FUNCTION

    Launch an application with a particular project.

    INPUTS

    tool     --  tool to use (may be NULL in which case this function
                 returns 0)
    project  --  name of the project to execute or NULL
    attrs    --  additional attributes

    TAGS

    NP_Priority (BYTE) -- priority of the launched tool (default is the
                          priority of the currect process except for
			  Workbench applications where the default priority
			  is 0 if not overridden by the TOOLPRI tooltype).
			  
    NP_Synchronous (BOOL) -- don't return until lauched application process
                             finishes (defaults to FALSE).

    RESULT

    Zero for failure, non-zero otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    NewDTObjectA()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BOOL isSynchronous;
    BPTR output;

    if (tool == NULL)
    {
	SetIoErr(ERROR_REQUIRED_ARG_MISSING);
	
	return 0;
    }

    isSynchronous = (BOOL)GetTagData(NP_Synchronous, (IPTR)FALSE, attrs);

    switch (tool->tn_Flags & TF_LAUNCH_MASK)
    {
    case TF_SHELL:
	{
	    char tBuffer[512];
	    LONG ret;

	    dt__sprintf
            (
                DataTypesBase, tBuffer,
                "\"%s\" \"%s\"", tool->tn_Program, project
            );

	    output = Open("CON:////Output window/AUTO/WAIT/CLOSE/INACTIVE",
			  MODE_NEWFILE);

	    if (output != BNULL)
	    {
		struct TagItem tags[] = { { SYS_Asynch, !isSynchronous },
					  { SYS_Input , (IPTR)NULL      },
					  { SYS_Output, (IPTR)output   },
					  { TAG_DONE,                  } };


		ret = SystemTagList(tBuffer, tags);

		/* Error? */
		if (ret == -1)
		{
		    return 0;
		}

		Close(output);
	    }
	    else
	    {
		return 0;
	    }
	}

	break;

    case TF_WORKBENCH:
	{
            BOOL            success       = FALSE;
            struct Library *WorkbenchBase = OpenLibrary("workbench.library", 39L);
            
            if (WorkbenchBase != NULL)
            {
                BPTR lock = Lock(project, ACCESS_READ);
                
                if (lock != BNULL)
                {
                    BPTR parent = ParentDir(lock);
                    
                    success = OpenWorkbenchObject
                    (
                        tool->tn_Program, 
                        WBOPENA_ArgLock, (IPTR) parent,
                        WBOPENA_ArgName, (IPTR) FilePart(project),
                        TAG_DONE
                    );
                    
                    UnLock(lock);
                }
                
                CloseLibrary(WorkbenchBase);
            }
            
            if (!success) return 0;
        }	
	break;

    case TF_RX:
	/* Sorry, no Arexx in AROS yet. */
        /* FIXME: No Arexx compatibility yet */

	/* Do some "RX command" here */
	return 0;

    default:
	return 0;
    }

    return 1;
    
    AROS_LIBFUNC_EXIT
} /* LaunchToolA */

