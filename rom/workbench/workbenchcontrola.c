/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Change global options and control the Workbench in various ways.
    Lang: english
*/

#include <exec/types.h>
#include <exec/ports.h>
#include <utility/tagitem.h>
#include <intuition/intuition.h>

#include "workbench_intern.h"
#include <workbench/workbench.h>
#include <proto/utility.h>

#include "support.h"

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

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct WorkbenchBase *, WorkbenchBase)

    BOOL            rc       = TRUE;  /* Return Code */

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
                if( tag->ti_Data != NULL )
                    *((ULONG *) tag->ti_Data) = WorkbenchBase->wb_DefaultStackSize;
                break;

            case WBCTRLA_SetDefaultStackSize:
                WorkbenchBase->wb_DefaultStackSize = tag->ti_Data;
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
                AddHiddenDevice((STRPTR) tag->ti_Data);
                break;

            case WBCTRLA_RemoveHiddenDeviceName:
                RemoveHiddenDevice((STRPTR) tag->ti_Data);
                break;

            case WBCTRLA_GetTypeRestartTime:
                if( tag->ti_Data != NULL )
                    *((ULONG *) tag->ti_Data) = WorkbenchBase->wb_TypeRestartTime;
                break;

            case WBCTRLA_SetTypeRestartTime:
                WorkbenchBase->wb_TypeRestartTime = tag->ti_Data;
                break;
        }
    }

    return rc;

    AROS_LIBFUNC_EXIT
} /* WorkbenchControlA */

