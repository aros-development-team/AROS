/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Open a drawer or launch a program.
*/

#include <exec/types.h>
#include <exec/ports.h>
#include <utility/tagitem.h>
#include <intuition/intuition.h>

#include "workbench_intern.h"
#include <workbench/workbench.h>
#include <proto/utility.h>

/*****************************************************************************

    NAME */

        #include <proto/workbench.h>

        AROS_LH2(BOOL, OpenWorkbenchObjectA,

/*  SYNOPSIS */
        AROS_LHA(STRPTR,           name, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct WorkbenchBase *, WorkbenchBase, 16, Workbench)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct WorkbenchBase *, WorkbenchBase)

    struct TagItem       *tagState = tags;
    struct TagItem       *tag      = NULL;

    struct FileInfoBlock  fib      = { 0 };
    BPTR                  lock     = NULL;

    /* Test whether the named path exist, as-is. */
    if ((lock = Lock(name, ACCESS_READ)))
    {
        /* Find out some info about the file. */
        if (Examine(lock, &fib))
        {
            if (fib.fib_DirEntryType > 0)
            {
                /* 
                    Since it's a directory, tell the Workbench Application
                    to open the corresponding drawer. 
                */
                // FIXME
            }
            else if (fib.fib_Protection & FIBF_EXECUTE)
            {
                /*
                    It's an executable. Before I launch it, I must check
                    whether it is a Workbench program or a CLI program.
                */
                
                
                // FIXME
                // if (has_icon) 
                // {
                //     /*
                //        It's a Workbench program. Build an WBArg array
                //        and WBStartup message and launch it.
                //     */
                // } 
                // else
                // {
                //     /* 
                //        It's a CLI program. Build the arguments as an
                //        valid argument string and launch the program.
                //     */
                // }
                
            }
            else if (fib.fib_Protection & FIBF_SCRIPT)
            {
                /* 
                    It's a script. Launch it as an CLI program as such:
                    C:Execute <filename> <args>, building the arguments as
                    for a normal CLI program.
                */
                // FIXME
                // [ach] Hmm... We don't seem to handle the script bit in AROS...
                
            }
            else
            {
                /*
                    Since it's not a directory nor an executable, it must be
                    a plain data file. Test whether it has an icon, and if
                    so try to launch it's default tool (if it has one). 
                */
                
                // if (has_icon)
                // {
                //     /* Test whether it has an valid default tool. */
                //
                //     if (has_valid_default_tool)
                //     {
                //         /* Build a correct WBArg array and launch the tool. */
                //     }
                //     else
                //     {
                //         /* Complain to the user about it. */
                //     }
                // } 
                // else
                // {
                //     panic_or_something
                // }
            }
        }
        
        UnLock(lock);
        
    }
    else
    {
        /*
            Locking the named path failed, and therefore we need to search the
            default search path for an executable of that name and launch 
            it. This only makes sense if the name is *not* an (absolute or 
            relative) path: that is, it must not contain any ':' or '/'.
        */
        // FIXME
    }


    /* 
        FIXME: Put the TagList parsing into separate support functions,
        BuildWBArgs() and BuildCLIArgs(). 
    */

    while( (tag = NextTagItem( (const struct TagItem **) &tagState )) ) {
        switch( tag->ti_Tag ) {
            case WBOPENA_ArgLock:
                /* FIXME: Do something... */
                break;

            case WBOPENA_ArgName:
                /* FIXME: Do something... */
                break;
        }
    }

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* OpenWorkbenchObjectA() */
