/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Open a drawer or launch a program.
    Lang: english
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
        AROS_LHA(STRPTR,           name,   A0),
        AROS_LHA(struct TagItem *, tags  , A1),

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

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct WorkbenchBase *, WorkbenchBase)

    struct TagItem       *tagState = tags;
    struct TagItem       *tag;

    struct FileInfoBlock  fib;
    BPTR                  lock;

    /* Test whether the named file exist. */
    if( (lock = Lock( name, ACCESS_READ )) ) {
        /* Find out some info about the file. */
        if( Examine( lock, &fib ) ) {
            if( fib.fib_DirEntryType > 0 ) {
                /* Since it's a directory, tell the Workbench Application
                   to open the corresponding drawer. */

            } else if( fib.fib_Protection & FIBF_EXECUTE ) {
                /* It's an executable. Before I launch it, I must check
                   whether it is a Workbench program or a CLI program. */

                // if( has_icon ) {
                //     /* It's a Workbench program. Build an WBArg array
                //        and WBStartup message and launch it. */
                // } else {
                //     /* It's a CLI program. Build the arguments as an
                //        valid argument string and launch the program. */
                // }

            } else if( fib.fib_Protection & FIBF_SCRIPT ) {
                /* It's a script. Launch it as an CLI program as such:
                   C:Execute <filename> <args>, building the arguments as
                   for a normal CLI program. */

            } else {
                /* Since it's not a directory nor an executable, it must be
                   a plain data file. Test whether it has an icon, and if
                   so try to launch it's default tool (if it has one). */

                // if( has_icon ) {
                //     /* Test whether it has an valid default tool. */
                //
                //     if( has_valid_default_tool ) {
                //         /* Build a correct WBArg array and launch the tool. */
                //     } else {
                //         /* Complain to the user about it. */
                //     }
                // } else {
                //     panic_or_something
                // }
            }
        }

        UnLock( lock );

    } else {
        /* If the name was not an absolute path, we need to search
         * the default search path for an executable of that name.
         * So, first let's check if it is an absolute path. */

        /* Like, TODO man. */
    }


    /* TODO: Put the TagList parsing into separate support functions,
             BuildWBArgs() and BuildCLIArgs(). */

    while( (tag = NextTagItem( (const struct TagItem **) &tagState )) ) {
        switch( tag->ti_Tag ) {
            case WBOPENA_ArgLock:
                /* TODO: Do something... */
                break;

            case WBOPENA_ArgName:
                /* TODO: Do something... */
                break;
        }
    }

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* OpenWorkbenchObjectA */

