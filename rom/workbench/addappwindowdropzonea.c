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

        AROS_LH4(struct AppWindowDropZone *, AddAppWindowDropZoneA,

/*  SYNOPSIS */
        AROS_LHA(struct AppWindow *, aw      , A0),
        AROS_LHA(ULONG             , id      , D0),
        AROS_LHA(ULONG             , userdata, D1),
        AROS_LHA(struct TagItem *  , tags    , A1),

/*  LOCATION */
        struct WorkbenchBase *, WorkbenchBase, 19, Workbench)

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

    struct TagItem           *tagState = tags;
    struct TagItem           *tag;

    struct AppWindowDropZone *dropZone;

    if( !(dropZone = AllocVec( sizeof( struct AppWindowDropZone ), MEMF_ANY | MEMF_CLEAR )) ) {
        return NULL;
    }

    dropZone->awdz_ID       = id;
    dropZone->awdz_UserData = userdata;

    while( (tag = NextTagItem( (const struct TagItem **) &tagState )) ) {
        switch( tag->ti_Tag ) {
            case WBDZA_Left:
                dropZone->awdz_Box.Left = (WORD) tag->ti_Data;
                break;

            case WBDZA_RelRight:
                dropZone->awdz_Box.Left = aw->aw_Window->Width + (WORD) tag->ti_Data;
                break;

            case WBDZA_Top:
                dropZone->awdz_Box.Top  = (WORD) tag->ti_Data;
                break;

            case WBDZA_RelBottom:
                dropZone->awdz_Box.Top  = aw->aw_Window->Height + (WORD) tag->ti_Data;
                break;

            case WBDZA_Width:
                dropZone->awdz_Box.Width = (WORD) tag->ti_Data;
                break;

            case WBDZA_RelWidth:
                dropZone->awdz_Box.Width = aw->aw_Window->Width + (WORD) tag->ti_Data;
                break;

            case WBDZA_Height:
                dropZone->awdz_Box.Height = (WORD) tag->ti_Data;
                break;

            case WBDZA_RelHeight:
                dropZone->awdz_Box.Height = aw->aw_Window->Height + (WORD) tag->ti_Data;
                break;

            case WBDZA_Box:
                if( tag->ti_Data != NULL ) {
                    dropZone->awdz_Box.Left   = ((struct IBox *) tag->ti_Data)->Left;
                    dropZone->awdz_Box.Top    = ((struct IBox *) tag->ti_Data)->Top;
                    dropZone->awdz_Box.Width  = ((struct IBox *) tag->ti_Data)->Width;
                    dropZone->awdz_Box.Height = ((struct IBox *) tag->ti_Data)->Height;
                }
                break;

            case WBDZA_Hook:
                if( tag->ti_Data != NULL )
                    dropZone->awdz_Hook = (struct Hook *) tag->ti_Data;
                break;
        }
    }

    AddTail( &(aw->aw_DropZones), (struct Node *) dropZone );

    /* TODO: Notify the Workbench Apps about this. */

    return dropZone;

    AROS_LIBFUNC_EXIT
} /* AddAppWindowDropZoneA */

