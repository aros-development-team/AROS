/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

#include <exec/types.h>
#include <exec/ports.h>
#include <utility/tagitem.h>

#include "workbench_intern.h"
#include <workbench/workbench.h>

/*****************************************************************************

    NAME */
        #include <proto/workbench.h>

        AROS_LH5(struct AppMenuItem *, AddAppMenuItemA,
/*  SYNOPSIS */
        AROS_LHA(ULONG           , id       , D0),
        AROS_LHA(ULONG           , userdata , D1),
        AROS_LHA(APTR            , text     , A0),
        AROS_LHA(struct MsgPort *, msgport  , A1),
        AROS_LHA(struct TagItem *, taglist  , A3),

/*  LOCATION */
        struct WorkbenchBase *, WorkbenchBase, 12, Workbench)

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

    struct TagItem     *tagState = taglist;
    struct TagItem     *tag;

    struct AppMenuItem *appMenuItem;

    if( !(appMenuItem = AllocVec( sizeof( struct AppMenuItem ), MEMF_ANY | MEMF_CLEAR )) ) {
        return NULL;
    }

    appMenuItem->ami_ID       = id;
    appMenuItem->ami_UserData = userdata;
    appMenuItem->ami_Text     = text;
    appMenuItem->ami_MsgPort  = msgport;

    while( (tag = NextTagItem( (const struct TagItem **) &tagState )) ) {
        switch( tag->ti_Tag ) {
            case WBAPPMENUA_CommandKeyString:
                /* TODO: Check if an other menu item already uses
                 *       this command key before accepting it.
                 * Should probably take a copy of the string instead... */
                appMenuItem->ami_CommandKey = (STRPTR) tag->ti_Data;
                break;
        }
    }

    AddTail( &(WorkbenchBase->wb_AppMenuItems), (struct Node *) appMenuItem );

    /* TODO: Notify the Workbench Apps about this. */

    return appMenuItem;

    AROS_LIBFUNC_EXIT
} /* AddAppMenuItemA */

