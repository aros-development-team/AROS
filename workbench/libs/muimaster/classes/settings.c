/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

/****** Settings.mui/MUIC_Settings ************
*
*   NAME
*       MUIC_Settings
*
*   SUPERCLASS
*       Group.mui
*
*   DESCRIPTION
*       Construct a preliminary settings layout consisting of a Listview, two
*       Popimage controls and a Settingsgroup. The outer child is a horizontal
*       group; the left vertical group contains the list and a row of two image
*       selectors, while Settingsgroup is on the right. The parent Group owns
*       the child hierarchy.
*       The List uses an input-list frame, AdjustWidth TRUE and a three-column
*       format. Its display hook supplies empty strings for the first two
*       columns and the entry pointer for the third. This constructor inserts no
*       entries. Both popup controls are Popimage objects with fixed height
*       twenty and Imageadjust Type All; the first is not a Popframe despite the
*       source comment.
*       No selection-to-editor notifications, configuration load/save routing or
*       application of selected settings is installed here. The class header
*       declares only its name, and the constructor handles no additional
*       attributes. All other messages delegate to Group. This layout is not a
*       complete preferences editor.
*       Construction returns zero on superclass failure; nested child creation
*       has no separate comprehensive local failure handling. Normal Group
*       disposal owns the constructed hierarchy. The static display hook borrows
*       each entry pointer and requires three writable display slots; it returns
*       zero and does not copy or free entry data.
*
*/

extern struct Library *MUIMasterBase;

struct MUI_SettingsData
{
    int dummy;
};

/*
 * Write two empty-string pointers and the borrowed entry pointer into three
 * display slots, returning zero. Requires writable array of at least three
 * pointers; entry can be NULL and is neither copied nor freed. Hook
 * argument unused; data lifetime belongs to the List caller.
 */
static IPTR ListDisplayFunc(struct Hook *hook, char **array, char *entry)
{
    *array++ = "";
    *array++ = "";
    *array = entry;

    return 0;
}

/**************************************************************************
 OM_NEW
**************************************************************************/
/*
 * Create nested owning groups with empty three-column Listview, two
 * Popimage controls and empty Settingsgroup. Return object or zero on
 * superclass failure; no comprehensive local child-allocation rollback.
 * Parse no extra attributes, insert no list entries and install no
 * configuration wiring.
 */
IPTR Settings__OM_NEW(struct IClass * cl, Object * obj, struct opSet * msg)
{
    struct MUI_Settings *data;
    struct TagItem *tag, *tags;
    Object *listobj;
    static const struct Hook list_display_hook =
    {
        {NULL, NULL}, HookEntry, ListDisplayFunc, NULL
    };

    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
        Child, HGroup,
            Child, VGroup,
                Child, ListviewObject,
                    MUIA_Listview_List, listobj = ListObject,
                        InputListFrame,
                        MUIA_List_AdjustWidth, TRUE,
                        MUIA_List_Format, "DELTA=2,DELTA=5,",
                        MUIA_List_DisplayHook, &list_display_hook,
                        End, /* ListObject */
                    End, /* ListviewObject */
                Child, HGroup,
                    Child, MUI_NewObject(MUIC_Popimage,
                        MUIA_FixHeight, 20,
                        MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_All,
                        TAG_DONE), /* Popframe really */
                    Child, MUI_NewObject(MUIC_Popimage,
                        MUIA_FixHeight, 20,
                        MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_All,
                        TAG_DONE),
                    End, /* HGroup */
                End, /* VGroup */
            Child, MUI_NewObject(MUIC_Settingsgroup,
                TAG_DONE),
            End, /* HGroup */
        TAG_MORE, msg->ops_AttrList);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        }
    }

    return (IPTR) obj;
}

/*
 * Dispatch local overrides and return handler results; delegate other
 * messages to the declared superclass. Requires correct method ABI and live
 * object except construction. No implicit additional ownership,
 * synchronization or conversion is supplied by this dispatcher.
 */
BOOPSI_DISPATCHER(IPTR, Settings_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Settings__OM_NEW(cl, obj, (struct opSet *)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Settings_desc =
{
    MUIC_Settings,
    MUIC_Group,
    sizeof(struct MUI_SettingsData),
    (void *) Settings_Dispatcher
};
