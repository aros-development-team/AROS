/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Add an icon to Workbench's list of AppIcons.
    Lang: english
*/

#include <exec/types.h>
#include <exec/ports.h>
#include <utility/tagitem.h>
#include <dos/dos.h>

#include <proto/utility.h>

#include "workbench_intern.h"
#include <workbench/workbench.h>

/*****************************************************************************

    NAME */
    #include <proto/workbench.h>

        AROS_LH7(struct AppIcon *, AddAppIconA,
/*  SYNOPSIS */
        AROS_LHA(ULONG,               id,       D0),
        AROS_LHA(ULONG,               userdata, D1),
        AROS_LHA(char *,              text,     A0),
        AROS_LHA(struct MsgPort *,    msgport,  A1),
        AROS_LHA(BPTR,                lock,     A2),
        AROS_LHA(struct DiskObject *, diskobj,  A3),
        AROS_LHA(struct TagItem *,    taglist,  A4),

/*  LOCATION */
        struct WorkbenchBase *, WorkbenchBase, 10, Workbench)

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

    struct TagItem *tagState = taglist;
    struct TagItem *tag;

    struct AppIcon *appIcon;

    if( !(appIcon = AllocVec( sizeof( struct AppIcon ), MEMF_CLEAR | MEMF_ANY )) ) {
        return NULL;
    }

    appIcon->ai_ID         = id;
    appIcon->ai_UserData   = userdata;
    appIcon->ai_Text       = text;
    appIcon->ai_MsgPort    = msgport;
    appIcon->ai_DiskObject = diskobj;

    while( (tag = NextTagItem( (const struct TagItem **) &tagState )) ) {
        switch( tag->ti_Tag ) {
            case WBAPPICONA_SupportsOpen:
                if( tag->ti_Data == TRUE )
                    appIcon->ai_Flags |= WBAPPICONF_SupportsOpen;
                break;

            case WBAPPICONA_SupportsCopy:
                if( tag->ti_Data == TRUE )
                    appIcon->ai_Flags |= WBAPPICONF_SupportsCopy;
                break;

            case WBAPPICONA_SupportsRename:
                if( tag->ti_Data == TRUE )
                    appIcon->ai_Flags |= WBAPPICONF_SupportsRename;
                break;

            case WBAPPICONA_SupportsInformation:
                if( tag->ti_Data == TRUE )
                    appIcon->ai_Flags |= WBAPPICONF_SupportsInformation;
                break;

            case WBAPPICONA_SupportsSnapshot:
                if( tag->ti_Data == TRUE )
                    appIcon->ai_Flags |= WBAPPICONF_SupportsSnapshot;
                break;

            case WBAPPICONA_SupportsUnSnapshot:
                if( tag->ti_Data == TRUE )
                    appIcon->ai_Flags |= WBAPPICONF_SupportsUnSnapshot;
                break;

            case WBAPPICONA_SupportsLeaveOut:
                if( tag->ti_Data == TRUE )
                    appIcon->ai_Flags |= WBAPPICONF_SupportsLeaveOut;
                break;

            case WBAPPICONA_SupportsPutAway:
                if( tag->ti_Data == TRUE )
                    appIcon->ai_Flags |= WBAPPICONF_SupportsPutAway;
                break;

            case WBAPPICONA_SupportsDelete:
                if( tag->ti_Data == TRUE )
                    appIcon->ai_Flags |= WBAPPICONF_SupportsDelete;
                break;

            case WBAPPICONA_SupportsFormatDisk:
                if( tag->ti_Data == TRUE )
                    appIcon->ai_Flags |= WBAPPICONF_SupportsFormatDisk;
                break;

            case WBAPPICONA_SupportsEmptyTrash:
                if( tag->ti_Data == TRUE )
                    appIcon->ai_Flags |= WBAPPICONF_SupportsEmptyTrash;
                break;

            case WBAPPICONA_PropagatePosition:
                if( tag->ti_Data == TRUE )
                    appIcon->ai_Flags |= WBAPPICONF_PropagatePosition;
                break;

            case WBAPPICONA_RenderHook:
                if( appIcon->ai_RenderHook != NULL )
                    appIcon->ai_RenderHook = (struct Hook *) tag->ti_Data;
                break;

            case WBAPPICONA_NotifySelectState:
                if( tag->ti_Data == TRUE )
                    appIcon->ai_Flags |= WBAPPICONF_NotifySelectState;
                break;
        }
    }

    AddTail( &(WorkbenchBase->wb_AppIcons), (struct Node *) appIcon );

    /* TODO: Notify the Workbench Apps about this. */

    return appIcon;

    AROS_LIBFUNC_EXIT
} /* AddAppIconA */

