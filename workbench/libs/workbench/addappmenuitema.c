/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Add a menuitem to Workbench's list of AppMenuItems.
*/

#include <exec/ports.h>
#include <utility/tagitem.h>
#include <workbench/workbench.h>
#include <proto/utility.h>

#include <string.h>

#include "workbench_intern.h"

BOOL keyUsed(STRPTR key, struct WorkbenchBase *WorkbenchBase);

/*****************************************************************************

    NAME */
        #include <proto/workbench.h>

        AROS_LH5(struct AppMenuItem *, AddAppMenuItemA,
/*  SYNOPSIS */
        AROS_LHA(IPTR            , id      , D0),
        AROS_LHA(IPTR            , userdata, D1),
        AROS_LHA(APTR            , text    , A0),
        AROS_LHA(struct MsgPort *, msgport , A1),
        AROS_LHA(struct TagItem *, taglist , A3),

/*  LOCATION */
        struct WorkbenchBase *, WorkbenchBase, 12, Workbench)

/*  FUNCTION

    Try to add a menu item to workbench.library's list of AppMenuItems (this
    will be shown in the 'Tools' menu strip in Workbench).

    INPUTS

    id        --  menu item identifier; for your convenience (ignored by
                  workbench.library)
    userdata  --  user specific data (ignored by workbench.library)
    text      --  menu item text; any text consisting merely of '-','_' and
                  '~' characters corresponds to a separator bar instead of
		  a textual item
    msgport   --  port to which notification messages regarding the menu
                  item will be sent
    taglist   --  tags (see below)
   
    TAGS

    WBAPPMENUA_CommandKeyString (STRPTR)
    Command key assigned to this menu item. If the string is empty, it will
    be ignored as will it if the command key is already in use by another
    menu item. Only the first character of the string will be used.
    [default = NULL]

    RESULT

    A pointer to an AppMenuItem which you pass to RemoveAppMenuItem() when
    you want to remove the menu item. If it was not possible to add the menu
    item, NULL will be returned.

    NOTES

    Contrary to AmigaOS, this function will report success even when there
    is no running workbench application.

    EXAMPLE

    BUGS

    SEE ALSO

    RemoveAppMenuItem()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem     *tagState = taglist;
    struct TagItem     *tag;

    struct AppMenuItem *appMenuItem;

    appMenuItem = AllocVec(sizeof(struct AppMenuItem), MEMF_CLEAR);

    if (appMenuItem == NULL)
    {
        return NULL;
    }

    appMenuItem->ami_ID       = id;
    appMenuItem->ami_UserData = userdata;
    appMenuItem->ami_Text     = text;
    appMenuItem->ami_MsgPort  = msgport;

    while ((tag = NextTagItem(&tagState)))
    {
        switch (tag->ti_Tag)
	{
	case WBAPPMENUA_CommandKeyString:
	    {
		STRPTR key = (STRPTR)tag->ti_Data;

		if (keyUsed(key, WorkbenchBase))
		{
		    appMenuItem->ami_CommandKey = "";
		}
		else
		{
		    appMenuItem->ami_CommandKey = key;
		}

		break;
	    }
        }
    }

    LockWorkbench();
    AddTail(&(WorkbenchBase->wb_AppMenuItems), (struct Node *)appMenuItem);
    UnlockWorkbench();

    /* NotifyWorkbench(WBNOTIFY_Create, WBNOTIFY_AppMenuItem, WorkbenchBase);
     */
    
    return appMenuItem;

    AROS_LIBFUNC_EXIT
} /* AddAppMenuItemA */


BOOL keyUsed(STRPTR key, struct WorkbenchBase *WorkbenchBase)
{
    struct AppMenuItem *ami;
    BOOL                found = FALSE;

    if (strlen(key) == 0)
    {
	return FALSE;
    }

    LockWorkbench();

    ForeachNode(&WorkbenchBase->wb_AppMenuItems, ami)
    {
	if (strlen(ami->ami_CommandKey) != 0 &&
	    ami->ami_CommandKey[0] == key[0])
	{
	    found = TRUE;
	    break;
	}
    }

    UnlockWorkbench();

    return found;
}
