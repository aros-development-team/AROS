/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

#include <exec/types.h>
#include <exec/ports.h>
#include <utility/tagitem.h>
#include <intuition/intuition.h>

#include "workbench_intern.h"
#include <workbench/workbench.h>

/*****************************************************************************

    NAME */

        #include <proto/workbench.h>

        AROS_LH2(BOOL, WorkbenchControlA,

/*  SYNOPSIS */
        AROS_LHA(STRPTR,           name, A0),
        AROS_LHA(struct TagItem *, tags,   A1),

/*  LOCATION */
        struct WorkbenchBase *, WorkbenchBase, 18, Workbench)

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

    struct TagItem *tagState = tags;
    struct TagItem *tag;

    while( (tag = NextTagItem( (const struct TagItem **) &tagState )) ) {
        switch( tag->ti_Tag ) {
            case WBCTRLA_IsOpen:
                /* TODO: Do something... */
                break;

            case WBCTRLA_DuplicateSearchPath:
                /* TODO: Do something... */
                break;

            case WBCTRLA_FreeSearchPath:
                /* TODO: Do something... */
                break;

            case WBCTRLA_GetDefaultStackSize:
                /* TODO: Do something... */
                break;

            case WBCTRLA_SetDefaultStackSize:
                /* TODO: Do something... */
                break;

            case WBCTRLA_RedrawAppIcon:
                /* TODO: Do something... */
                break;

            case WBCTRLA_GetProgramList:
                /* TODO: Do something... */
                break;

            case WBCTRLA_FreeProgramList:
                /* TODO: Do something... */
                break;

            case WBCTRLA_GetSelectedIconList:
                /* TODO: Do something... */
                break;

            case WBCTRLA_FreeSelectedIconList:
                /* TODO: Do something... */
                break;

            case WBCTRLA_GetOpenDrawerList:
                /* TODO: Do something... */
                break;

            case WBCTRLA_FreeOpenDrawerList:
                /* TODO: Do something... */
                break;

            case WBCTRLA_GetHiddenDeviceList:
                /* TODO: Do something... */
                break;

            case WBCTRLA_FreeHiddenDeviceList:
                /* TODO: Do something... */
                break;

            case WBCTRLA_AddHiddenDeviceName:
                /* TODO: Do something... */
                break;

            case WBCTRLA_RemoveHiddenDeviceName:
                /* TODO: Do something... */
                break;

            case WBCTRLA_GetTypeRestartTime:
                /* TODO: Do something... */
                break;

            case WBCTRLA_SetTypeRestartTime:
                /* TODO: Do something... */
                break;
        }
    }

    return NULL;

    AROS_LIBFUNC_EXIT
} /* WorkbenchControlA */

